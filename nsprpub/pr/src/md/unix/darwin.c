/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape Portable Runtime (NSPR).
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#include "primpl.h"

void _MD_EarlyInit(void)
{
}

PRWord *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
#if !defined(_PR_PTHREADS)
    if (isCurrent) {
	(void) setjmp(CONTEXT(t));
    }
    *np = sizeof(CONTEXT(t)) / sizeof(PRWord);
    return (PRWord *) CONTEXT(t);
#else
	*np = 0;
	return NULL;
#endif
}

#if !defined(_PR_PTHREADS)
void
_MD_SET_PRIORITY(_MDThread *thread, PRUintn newPri)
{
    return;
}

PRStatus
_MD_InitializeThread(PRThread *thread)
{
	return PR_SUCCESS;
}

PRStatus
_MD_WAIT(PRThread *thread, PRIntervalTime ticks)
{
    PR_ASSERT(!(thread->flags & _PR_GLOBAL_SCOPE));
    _PR_MD_SWITCH_CONTEXT(thread);
    return PR_SUCCESS;
}

PRStatus
_MD_WAKEUP_WAITER(PRThread *thread)
{
    if (thread) {
	PR_ASSERT(!(thread->flags & _PR_GLOBAL_SCOPE));
    }
    return PR_SUCCESS;
}

/* These functions should not be called for Rhapsody */
void
_MD_YIELD(void)
{
    PR_NOT_REACHED("_MD_YIELD should not be called for Rhapsody.");
}

PRStatus
_MD_CREATE_THREAD(
    PRThread *thread,
    void (*start) (void *),
    PRThreadPriority priority,
    PRThreadScope scope,
    PRThreadState state,
    PRUint32 stackSize)
{
    PR_NOT_REACHED("_MD_CREATE_THREAD should not be called for Rhapsody.");
	return PR_FAILURE;
}
#endif /* ! _PR_PTHREADS */

#if defined(_PR_PTHREADS)

/*
** Stubs for unimplemented functions
*/

int pthread_condattr_init(pthread_condattr_t *attr)
{
    return 0;
}

int pthread_kill(pthread_t thread, int sig)
{
    return ENOSYS;
}

typedef struct siginfo_t siginfo_t;

int sigtimedwait(const sigset_t *set, siginfo_t *info,
    const struct timespec *timeout)
{
    errno = ENOSYS;
    return -1;
}

#endif /* _PR_PTHREADS */

/* darwin.c */

