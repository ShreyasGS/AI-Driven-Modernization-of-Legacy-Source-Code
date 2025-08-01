/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by the Initial Developer are
 * Copyright (C) 2001 the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Stuart Parmenter <pavlov@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 */

#include "nsTimerImpl.h"
#include "TimerThread.h"

#include "nsSupportsArray.h"

#include "nsIEventQueue.h"

static TimerThread *gThread = nsnull;

  // only the main thread supports idle timers.
static nsSupportsArray *gIdleTimers = nsnull;

static PRBool gFireOnIdle = PR_FALSE;

#include "prmem.h"
#include "prinit.h"

#ifdef DEBUG_TIMERS
#include <math.h>

double nsTimerImpl::sDeltaSumSquared = 0;
double nsTimerImpl::sDeltaSum = 0;
double nsTimerImpl::sDeltaNum = 0;

static void
myNS_MeanAndStdDev(double n, double sumOfValues, double sumOfSquaredValues,
                   double *meanResult, double *stdDevResult)
{
  double mean = 0.0, var = 0.0, stdDev = 0.0;
  if (n > 0.0 && sumOfValues >= 0) {
    mean = sumOfValues / n;
    double temp = (n * sumOfSquaredValues) - (sumOfValues * sumOfValues);
    if (temp < 0.0 || n <= 1)
      var = 0.0;
    else
      var = temp / (n * (n - 1));
    // for some reason, Windows says sqrt(0.0) is "-1.#J" (?!) so do this:
    stdDev = var != 0.0 ? sqrt(var) : 0.0;
  }
  *meanResult = mean;
  *stdDevResult = stdDev;
}
#endif

NS_IMPL_THREADSAFE_QUERY_INTERFACE2(nsTimerImpl, nsITimer, nsIScriptableTimer)
NS_IMPL_THREADSAFE_ADDREF(nsTimerImpl)

NS_IMETHODIMP_(nsrefcnt) nsTimerImpl::Release(void)
{
  nsrefcnt count;

  NS_PRECONDITION(0 != mRefCnt, "dup release");
  count = PR_AtomicDecrement((PRInt32 *)&mRefCnt);
  NS_LOG_RELEASE(this, count, "nsTimerImpl");
  if (count == 0) {
    mRefCnt = 1; /* stabilize */

    /* enable this to find non-threadsafe destructors: */
    /* NS_ASSERT_OWNINGTHREAD(nsTimerImpl); */
    NS_DELETEXPCOM(this);
    return 0;
  }

  // If only one reference remains, and mArmed is set, then the ref must be
  // from the TimerThread::mTimers array, so we Cancel this timer to remove
  // the mTimers element, and return 0 if Cancel in fact disarmed the timer.
  //
  // We use an inlined version of nsTimerImpl::Cancel here to check for the
  // NS_ERROR_NOT_AVAILABLE code returned by gThread->RemoveTimer when this
  // timer is not found in the mTimers array -- i.e., when the timer was not
  // in fact armed once we acquired TimerThread::mLock, in spite of mArmed
  // being true here.  That can happen if the armed timer is being fired by
  // TimerThread::Run as we race and test mArmed just before it is cleared by
  // the timer thread.  If the RemoveTimer call below doesn't find this timer
  // in the mTimers array, then the last ref to this timer is held manually
  // and temporarily by the TimerThread, so we should fall through to the
  // final return and return 1, not 0.
  //
  // The original version of this thread-based timer code kept weak refs from
  // TimerThread::mTimers, removing this timer's weak ref in the destructor,
  // but that leads to double-destructions in the race described above, and
  // adding mArmed doesn't help, because destructors can't be deferred, once
  // begun.  But by combining reference-counting and a specialized Release
  // method with "is this timer still in the mTimers array once we acquire
  // the TimerThread's lock" testing, we defer destruction until we're sure
  // that only one thread has its hot little hands on this timer.
  //
  // Note that both approaches preclude a timer creator, and everyone else
  // except the TimerThread who might have a strong ref, from dropping all
  // their strong refs without implicitly canceling the timer.  Timers need
  // non-mTimers-element strong refs to stay alive.

  if (count == 1 && mArmed) {
    mCanceled = PR_TRUE;

    if (NS_SUCCEEDED(gThread->RemoveTimer(this)))
      return 0;
  }

  return count;
}

PR_STATIC_CALLBACK(PRStatus) InitThread(void)
{
  gThread = new TimerThread();
  if (!gThread)
    return PR_FAILURE;

  NS_ADDREF(gThread);

  nsresult rv = gThread->Init();
  if (NS_FAILED(rv)) {
    NS_RELEASE(gThread);
    return PR_FAILURE;
  }

  return PR_SUCCESS;
}

nsTimerImpl::nsTimerImpl() :
  mClosure(nsnull),
  mCallbackType(CALLBACK_TYPE_UNKNOWN),
  mFiring(PR_FALSE),
  mArmed(PR_FALSE),
  mCanceled(PR_FALSE),
  mTimeout(0)
{
  NS_INIT_REFCNT();
  nsIThread::GetCurrent(getter_AddRefs(mCallingThread));

  static PRCallOnceType once;
  PR_CallOnce(&once, InitThread);

  mCallback.c = nsnull;

#ifdef DEBUG_TIMERS
  mStart = 0;
  mStart2 = 0;
#endif
}

nsTimerImpl::~nsTimerImpl()
{
  if (mCallbackType == CALLBACK_TYPE_INTERFACE)
    NS_RELEASE(mCallback.i);
  else if (mCallbackType == CALLBACK_TYPE_OBSERVER)
    NS_RELEASE(mCallback.o);
}


void nsTimerImpl::Shutdown()
{
#ifdef DEBUG_TIMERS
  if (PR_LOG_TEST(gTimerLog, PR_LOG_DEBUG)) {
    double mean = 0, stddev = 0;
    myNS_MeanAndStdDev(sDeltaNum, sDeltaSum, sDeltaSumSquared, &mean, &stddev);

    PR_LOG(gTimerLog, PR_LOG_DEBUG, ("sDeltaNum = %f, sDeltaSum = %f, sDeltaSumSquared = %f\n", sDeltaNum, sDeltaSum, sDeltaSumSquared));
    PR_LOG(gTimerLog, PR_LOG_DEBUG, ("mean: %fms, stddev: %fms\n", mean, stddev));
  }
#endif

  if (!gThread)
    return;

  gThread->Shutdown();
  NS_RELEASE(gThread);

  gFireOnIdle = PR_FALSE;
  NS_IF_RELEASE(gIdleTimers);
}


NS_IMETHODIMP nsTimerImpl::Init(nsTimerCallbackFunc aFunc,
                                void *aClosure,
                                PRUint32 aDelay,
                                PRUint32 aPriority,
                                PRUint32 aType)
{
  if (!gThread)
    return NS_ERROR_FAILURE;

  mCallback.c = aFunc;
  mCallbackType = CALLBACK_TYPE_FUNC;

  mClosure = aClosure;

  mPriority = (PRUint8)aPriority;
  mType = (PRUint8)aType;

  SetDelayInternal(aDelay);

  return gThread->AddTimer(this);
}

NS_IMETHODIMP nsTimerImpl::Init(nsITimerCallback *aCallback,
                                PRUint32 aDelay,
                                PRUint32 aPriority,
                                PRUint32 aType)
{
  if (!gThread)
    return NS_ERROR_FAILURE;

  mCallback.i = aCallback;
  NS_ADDREF(mCallback.i);
  mCallbackType = CALLBACK_TYPE_INTERFACE;

  mPriority = (PRUint8)aPriority;
  mType = (PRUint8)aType;

  SetDelayInternal(aDelay);

  return gThread->AddTimer(this);
}

NS_IMETHODIMP nsTimerImpl::Init(nsIObserver *aObserver,
                                PRUint32 aDelay,
                                PRUint32 aPriority,
                                PRUint32 aType)
{
  if (!gThread)
    return NS_ERROR_FAILURE;

  SetDelayInternal(aDelay);

  mCallback.o = aObserver;
  NS_ADDREF(mCallback.o);
  mCallbackType = CALLBACK_TYPE_OBSERVER;

  mPriority = (PRUint8)aPriority;
  mType = (PRUint8)aType;

  return gThread->AddTimer(this);
}

NS_IMETHODIMP nsTimerImpl::Cancel()
{
  mCanceled = PR_TRUE;

  if (gThread)
    gThread->RemoveTimer(this);

  return NS_OK;
}

NS_IMETHODIMP_(void) nsTimerImpl::SetDelay(PRUint32 aDelay)
{
  // If we're already repeating precisely, update mTimeout now so that the
  // new delay takes effect in the future.
  if (mTimeout != 0 && mType == NS_TYPE_REPEATING_PRECISE)
    mTimeout = PR_IntervalNow();

  SetDelayInternal(aDelay);

  if (!mFiring && gThread)
    gThread->TimerDelayChanged(this);
}

NS_IMETHODIMP_(void) nsTimerImpl::SetPriority(PRUint32 aPriority)
{
  mPriority = (PRUint8)aPriority;
}

NS_IMETHODIMP_(void) nsTimerImpl::SetType(PRUint32 aType)
{
  mType = (PRUint8)aType;
  // XXX if this is called, we should change the actual type.. this could effect
  // repeating timers.  we need to ensure in Fire() that if mType has changed
  // during the callback that we don't end up with the timer in the queue twice.
}

void nsTimerImpl::Fire()
{
  if (mCanceled)
    return;

  PRIntervalTime now = PR_IntervalNow();
#ifdef DEBUG_TIMERS
  if (PR_LOG_TEST(gTimerLog, PR_LOG_DEBUG)) {
    PRIntervalTime a = now - mStart; // actual delay in intervals
    PRUint32       b = PR_MillisecondsToInterval(mDelay); // expected delay in intervals
    PRUint32       d = PR_IntervalToMilliseconds((a > b) ? a - b : b - a); // delta in ms
    sDeltaSum += d;
    sDeltaSumSquared += double(d) * double(d);
    sDeltaNum++;

    PR_LOG(gTimerLog, PR_LOG_DEBUG, ("[this=%p] expected delay time %4dms\n", this, mDelay));
    PR_LOG(gTimerLog, PR_LOG_DEBUG, ("[this=%p] actual delay time   %4dms\n", this, PR_IntervalToMilliseconds(a)));
    PR_LOG(gTimerLog, PR_LOG_DEBUG, ("[this=%p] (mType is %d)       -------\n", this, mType));
    PR_LOG(gTimerLog, PR_LOG_DEBUG, ("[this=%p]     delta           %4dms\n", this, (a > b) ? (PRInt32)d : -(PRInt32)d));

    mStart = mStart2;
    mStart2 = 0;
  }
#endif

  PRIntervalTime timeout = mTimeout;
  if (mType == NS_TYPE_REPEATING_PRECISE) {
    // Precise repeating timers advance mTimeout by mDelay without fail before
    // calling Process().
    timeout -= PR_MillisecondsToInterval(mDelay);
  }
  gThread->UpdateFilter(mDelay, timeout, now);

  mFiring = PR_TRUE;

  switch (mCallbackType) {
    case CALLBACK_TYPE_FUNC:
      mCallback.c(this, mClosure);
      break;
    case CALLBACK_TYPE_INTERFACE:
      mCallback.i->Notify(this);
      break;
    case CALLBACK_TYPE_OBSERVER:
      mCallback.o->Observe(NS_STATIC_CAST(nsIScriptableTimer *, this),
                           NS_TIMER_CALLBACK_TOPIC,
                           nsnull);
      break;
    default:;
  }

  mFiring = PR_FALSE;

#ifdef DEBUG_TIMERS
  if (PR_LOG_TEST(gTimerLog, PR_LOG_DEBUG)) {
    PR_LOG(gTimerLog, PR_LOG_DEBUG,
           ("[this=%p] Took %dms to fire timer callback\n",
            this, PR_IntervalToMilliseconds(PR_IntervalNow() - now)));
  }
#endif

  if (mType == NS_TYPE_REPEATING_SLACK) {
    SetDelayInternal(mDelay); // force mTimeout to be recomputed.
    if (gThread)
      gThread->AddTimer(this);
  }
}


struct TimerEventType {
  PLEvent	e;
  // arguments follow...
#ifdef DEBUG_TIMERS
  PRIntervalTime mInit;
#endif
};


void* handleTimerEvent(TimerEventType* event)
{
#ifdef DEBUG_TIMERS
  if (PR_LOG_TEST(gTimerLog, PR_LOG_DEBUG)) {
    PRIntervalTime now = PR_IntervalNow();
    PR_LOG(gTimerLog, PR_LOG_DEBUG,
           ("[this=%p] time between PostTimerEvent() and Fire(): %dms\n",
            event->e.owner, PR_IntervalToMilliseconds(now - event->mInit)));
  }
#endif

  if (gFireOnIdle) {
    if (NS_STATIC_CAST(nsTimerImpl*, event->e.owner)->GetPriority() < NS_PRIORITY_HIGHEST) {
      nsCOMPtr<nsIThread> currentThread, mainThread;
      nsIThread::GetCurrent(getter_AddRefs(currentThread));
      nsIThread::GetMainThread(getter_AddRefs(mainThread));
      if (currentThread == mainThread) {
        gIdleTimers->AppendElement(NS_STATIC_CAST(nsITimer*, NS_STATIC_CAST(nsTimerImpl*, event->e.owner)));

        return NULL;
      }
    }
  }

  NS_STATIC_CAST(nsTimerImpl*, event->e.owner)->Fire();

  return NULL;
}

void destroyTimerEvent(TimerEventType* event)
{
  nsTimerImpl *timer = NS_STATIC_CAST(nsTimerImpl*, event->e.owner);
  NS_RELEASE(timer);
  PR_DELETE(event);
}


void nsTimerImpl::PostTimerEvent()
{
  // XXX we may want to reuse the PLEvent in the case of repeating timers.
  TimerEventType* event;

  // construct
  event = PR_NEW(TimerEventType);
  if (event == NULL) return;

  // initialize
  PL_InitEvent((PLEvent*)event, this,
               (PLHandleEventProc)handleTimerEvent,
               (PLDestroyEventProc)destroyTimerEvent);

  // Since TimerThread addref'd 'this' for us, we don't need to addref here.
  // We will release in destroyMyEvent.

#ifdef DEBUG_TIMERS
  if (PR_LOG_TEST(gTimerLog, PR_LOG_DEBUG)) {
    event->mInit = PR_IntervalNow();
  }
#endif

  // If this is a repeating precise timer, we need to calculate the time for
  // the next timer to fire before we make the callback.
  if (mType == NS_TYPE_REPEATING_PRECISE) {
    SetDelayInternal(mDelay);
    if (gThread)
      gThread->AddTimer(this);
  }

  PRThread *thread;
  mCallingThread->GetPRThread(&thread);

  nsCOMPtr<nsIEventQueue> queue;
  if (gThread)
    gThread->mEventQueueService->GetThreadEventQueue(thread, getter_AddRefs(queue));
  if (queue)
    queue->PostEvent(&event->e);
}

void nsTimerImpl::SetDelayInternal(PRUint32 aDelay)
{
  PRIntervalTime delayInterval = PR_MillisecondsToInterval(aDelay);
  if (delayInterval > DELAY_INTERVAL_MAX) {
    delayInterval = DELAY_INTERVAL_MAX;
    aDelay = PR_IntervalToMilliseconds(delayInterval);
  }

  mDelay = aDelay;

  PRIntervalTime now = PR_IntervalNow();
  if (mTimeout == 0 || mType != NS_TYPE_REPEATING_PRECISE)
    mTimeout = now;

  mTimeout += delayInterval;

#ifdef DEBUG_TIMERS
  if (PR_LOG_TEST(gTimerLog, PR_LOG_DEBUG)) {
    if (mStart == 0)
      mStart = now;
    else
      mStart2 = now;
  }
#endif
}

nsresult
NS_NewTimer(nsITimer* *aResult, nsTimerCallbackFunc aCallback, void *aClosure,
            PRUint32 aDelay, PRUint32 aPriority, PRUint32 aType)
{
    nsTimerImpl* timer = new nsTimerImpl();
    if (timer == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(timer);

    nsresult rv = timer->Init(aCallback, aClosure, aDelay, aPriority, aType);
    if (NS_FAILED(rv)) {
        NS_RELEASE(timer);
        return rv;
    }

    *aResult = timer;
    return NS_OK;
}


/**
 * Timer Manager code
 */

NS_IMPL_THREADSAFE_ISUPPORTS1(nsTimerManager, nsITimerManager)

nsTimerManager::nsTimerManager()
{
  NS_INIT_REFCNT();
}

nsTimerManager::~nsTimerManager()
{

}

NS_IMETHODIMP nsTimerManager::SetUseIdleTimers(PRBool aUseIdleTimers)
{
  if (aUseIdleTimers == PR_FALSE && gFireOnIdle == PR_TRUE)
    return NS_ERROR_FAILURE;

  gFireOnIdle = aUseIdleTimers;

  if (gFireOnIdle && !gIdleTimers) {
    gIdleTimers = new nsSupportsArray();
    if (!gIdleTimers)
      return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(gIdleTimers);
  }

  return NS_OK;
}

NS_IMETHODIMP nsTimerManager::GetUseIdleTimers(PRBool *aUseIdleTimers)
{
  *aUseIdleTimers = gFireOnIdle;
  return NS_OK;
}

NS_IMETHODIMP nsTimerManager::HasIdleTimers(PRBool *aHasTimers)
{
  *aHasTimers = PR_FALSE;

  if (!gFireOnIdle)
    return NS_OK;

  nsCOMPtr<nsIThread> currentThread, mainThread;
  nsIThread::GetCurrent(getter_AddRefs(currentThread));
  nsIThread::GetMainThread(getter_AddRefs(mainThread));

  if (currentThread != mainThread) {
    return NS_OK;
  }

  PRUint32 count;
  gIdleTimers->Count(&count);
  *aHasTimers = (count != 0);

  return NS_OK;
}

NS_IMETHODIMP nsTimerManager::FireNextIdleTimer()
{
  if (!gFireOnIdle)
    return NS_OK;

  nsCOMPtr<nsIThread> currentThread, mainThread;
  nsIThread::GetCurrent(getter_AddRefs(currentThread));
  nsIThread::GetMainThread(getter_AddRefs(mainThread));

  if (currentThread != mainThread) {
    return NS_OK;
  }

  PRUint32 count;
  gIdleTimers->Count(&count);

  if (count > 0) {
    nsTimerImpl *theTimer = NS_STATIC_CAST(nsTimerImpl*, NS_STATIC_CAST(nsITimer*, gIdleTimers->ElementAt(0))); // addrefs

    gIdleTimers->RemoveElement(NS_STATIC_CAST(nsITimer*, theTimer), 0);

    theTimer->Fire();

    NS_RELEASE(theTimer);
  }
  // pull out each one starting at the beginning until no more are left and fire them.

  return NS_OK;
}
