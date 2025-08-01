/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsView.h"
#include "nsIWidget.h"
#include "nsViewManager.h"
#include "nsIFrame.h"
#include "nsIPresContext.h"
#include "nsIWidget.h"
#include "nsIButton.h"
#include "nsIScrollbar.h"
#include "nsGUIEvent.h"
#include "nsIDeviceContext.h"
#include "nsIComponentManager.h"
#include "nsIRenderingContext.h"
#include "nsTransform2D.h"
#include "nsIScrollableView.h"
#include "nsVoidArray.h"
#include "nsGfxCIID.h"
#include "nsIRegion.h"

static NS_DEFINE_IID(kRegionCID, NS_REGION_CID);

//mmptemp

static nsEventStatus PR_CALLBACK HandleEvent(nsGUIEvent *aEvent);


//#define SHOW_VIEW_BORDERS
//#define HIDE_ALL_WIDGETS

//
// Main events handler
//
nsEventStatus PR_CALLBACK HandleEvent(nsGUIEvent *aEvent)
{ 
//printf(" %d %d %d (%d,%d) \n", aEvent->widget, aEvent->widgetSupports, 
//       aEvent->message, aEvent->point.x, aEvent->point.y);
  nsEventStatus result = nsEventStatus_eIgnore;
  nsView       *view = nsView::GetViewFor(aEvent->widget);

  if (nsnull != view)
  {
    nsViewManager *vm = view->GetViewManager();

    vm->DispatchEvent(aEvent, &result);
  }

  return result;
}

MOZ_DECL_CTOR_COUNTER(nsView)

nsView::nsView()
{
  MOZ_COUNT_CTOR(nsView);

  mVis = nsViewVisibility_kShow;
  mVFlags = 0;
  mOpacity = 1.0f;
  mViewManager = nsnull;
  mCompositorFlags = 0;
  mChildRemoved = PR_FALSE;
}

nsView::~nsView()
{
  MOZ_COUNT_DTOR(nsView);

  while (GetFirstChild() != nsnull)
  {
    nsView* child = GetFirstChild();
    if (child->GetViewManager() == mViewManager) {
      child->Destroy();
    } else {
      // just unhook it. Someone else will want to destroy this.
      RemoveChild(child);
    }
  }

  if (nsnull != mViewManager)
  {
    nsView *rootView = mViewManager->GetRootView();
    
    if (nsnull != rootView)
    {
      // Root views can have parents!
      if (nsnull != mParent)
      {
        mViewManager->RemoveChild(this);
      }

      if (rootView == this)
      {
        // Inform the view manager that the root view has gone away...
        mViewManager->SetRootView(nsnull);
      }
    }
    else if (nsnull != mParent)
    {
      mParent->RemoveChild(this);
    }
    
    nsView* grabbingView = mViewManager->GetMouseEventGrabber(); //check to see if we are capturing!!!
    if (grabbingView == this)
    {
      PRBool boolResult;//not used
      mViewManager->GrabMouseEvents(nsnull,boolResult);
    }

    mViewManager = nsnull;
  }
  else if (nsnull != mParent)
  {
    mParent->RemoveChild(this);
  }

  if (nsnull != mZParent)
  {
    mZParent->RemoveReparentedView();
    mZParent->Destroy();
  }

  // Destroy and release the widget
  if (nsnull != mWindow)
  {
    mWindow->SetClientData(nsnull);
    mWindow->Destroy();
    NS_RELEASE(mWindow);
  }
  NS_IF_RELEASE(mDirtyRegion);
}

nsresult nsView::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  
  *aInstancePtr = nsnull;
  
  if (aIID.Equals(NS_GET_IID(nsIView)) || (aIID.Equals(NS_GET_IID(nsISupports)))) {
    *aInstancePtr = (void*)(nsIView*)this;
    return NS_OK;
  }

  return NS_NOINTERFACE;
}

nsrefcnt nsView::AddRef() 
{
  NS_WARNING("not supported for views");
  return 1;
}

nsrefcnt nsView::Release()
{
  NS_WARNING("not supported for views");
  return 1;
}

nsView* nsView::GetViewFor(nsIWidget* aWidget)
{           
  void*    clientData;

  NS_PRECONDITION(nsnull != aWidget, "null widget ptr");
	
  // The widget's client data points back to the owning view
  if (aWidget && NS_SUCCEEDED(aWidget->GetClientData(clientData))) {
    nsISupports* data = (nsISupports*)clientData;
    
    if (nsnull != data) {
      nsIView* view = nsnull;
      if (NS_SUCCEEDED(data->QueryInterface(NS_GET_IID(nsIView), (void **)&view))) {
        return NS_STATIC_CAST(nsView*, view);
      }
    }
  }
  return nsnull;
}

NS_IMETHODIMP nsView::Init(nsIViewManager* aManager,
                             const nsRect &aBounds,
                             const nsIView *aParent,
                             nsViewVisibility aVisibilityFlag)
{
  //printf(" \n callback=%d data=%d", aWidgetCreateCallback, aCallbackData);
  NS_PRECONDITION(nsnull != aManager, "null ptr");
  if (nsnull == aManager) {
    return NS_ERROR_NULL_POINTER;
  }
  if (nsnull != mViewManager) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }
  // we don't hold a reference to the view manager
  mViewManager = NS_STATIC_CAST(nsViewManager*, aManager);

  mChildClip.mLeft = 0;
  mChildClip.mRight = 0;
  mChildClip.mTop = 0;
  mChildClip.mBottom = 0;

  SetPosition(aBounds.x, aBounds.y);
  nsRect dim(0, 0, aBounds.width, aBounds.height);

  SetDimensions(dim, PR_FALSE);

  //temporarily set it...
  SetParent(NS_CONST_CAST(nsView*, NS_STATIC_CAST(const nsView*, aParent)));

  SetVisibility(aVisibilityFlag);

  // XXX Don't clear this or we hork the scrolling view when creating the clip
  // view's widget. It needs to stay set and later the view manager will reset it
  // when the view is inserted into the view hierarchy...
#if 0
  //clear this again...
  SetParent(nsnull);
#endif

  return NS_OK;
}

NS_IMETHODIMP nsView::Destroy()
{
  delete this;
  return NS_OK;
}

NS_IMETHODIMP nsView::GetViewManager(nsIViewManager *&aViewMgr) const
{
  NS_IF_ADDREF(mViewManager);
  aViewMgr = mViewManager;
  return NS_OK;
}

NS_IMETHODIMP nsView::Paint(nsIRenderingContext& rc, const nsRect& rect,
                              PRUint32 aPaintFlags, PRBool &aResult)
{
    // Just paint, assume compositor knows what it's doing.
    if (nsnull != mClientData) {
      nsCOMPtr<nsIViewObserver> observer;
      if (NS_OK == mViewManager->GetViewObserver(*getter_AddRefs(observer))) {
        observer->Paint((nsIView *)this, rc, rect);
      }
    }
	return NS_OK;
}

NS_IMETHODIMP nsView::Paint(nsIRenderingContext& rc, const nsIRegion& region,
                              PRUint32 aPaintFlags, PRBool &aResult)
{
  // XXX apply region to rc
  // XXX get bounding rect from region
  //if (nsnull != mClientData)
  //{
  //  nsIViewObserver *obs;
  //
  //  if (NS_OK == mViewManager->GetViewObserver(obs))
  //  {
  //    obs->Paint((nsIView *)this, rc, rect, aPaintFlags);
  //    NS_RELEASE(obs);
  //  }
  //}
  aResult = PR_FALSE;
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsEventStatus nsView::HandleEvent(nsViewManager* aVM, nsGUIEvent *aEvent, PRBool aCaptured)
{
  return aVM->HandleEvent(this, aEvent, aCaptured);
}

// XXX Start Temporary fix for Bug #19416
NS_IMETHODIMP nsView::IgnoreSetPosition(PRBool aShouldIgnore)
{
  mShouldIgnoreSetPosition = aShouldIgnore;
  // resync here
  if (!mShouldIgnoreSetPosition) {
    SetPosition(mPosX, mPosY);
  }
  return NS_OK;
}
// XXX End Temporary fix for Bug #19416

void nsView::SetPosition(nscoord aX, nscoord aY)
{
  nscoord x = aX;
  nscoord y = aY;
  if (IsRoot()) {
    // Add view manager's coordinate offset to the root view
    // This allows the view manager to offset it's coordinate space
    // while allowing layout to assume it's coordinate space origin is (0,0)
    nscoord offsetX;
    nscoord offsetY;
    mViewManager->GetWindowOffset(&offsetX, &offsetY);
    x += offsetX;
    y += offsetY;
  }
  mDimBounds.x += aX - mPosX;
  mDimBounds.y += aY - mPosY;
  mPosX = aX;
  mPosY = aY;

  // XXX Start Temporary fix for Bug #19416
  if (mShouldIgnoreSetPosition) {
    return;
  }
  // XXX End Temporary fix for Bug #19416

  if (nsnull != mWindow)
  {
    // see if we are caching our widget changes. Yes? 
    // mark us as changed. Later we will actually move the 
    // widget.
    PRBool caching = PR_FALSE;
    mViewManager->IsCachingWidgetChanges(&caching);
    if (caching) {
      mVFlags |= NS_VIEW_FLAG_WIDGET_MOVED;
      return;
    }

    nsIDeviceContext  *dx;
    float             scale;
    nsIWidget         *pwidget = nsnull;
    nscoord           parx = 0, pary = 0;
  
    mViewManager->GetDeviceContext(dx);
    dx->GetAppUnitsToDevUnits(scale);
    NS_RELEASE(dx);

    GetOffsetFromWidget(&parx, &pary, pwidget);
    NS_IF_RELEASE(pwidget);

    mWindow->Move(NSTwipsToIntPixels((mDimBounds.x + parx), scale),
                  NSTwipsToIntPixels((mDimBounds.y + pary), scale));
  }
}

NS_IMETHODIMP nsView::SynchWidgetSizePosition()
{
  // if the widget was moved or resized
  if (mVFlags & NS_VIEW_FLAG_WIDGET_MOVED || mVFlags & NS_VIEW_FLAG_WIDGET_RESIZED)
  {
    nsIDeviceContext  *dx;
    float             t2p;

    mViewManager->GetDeviceContext(dx);
    dx->GetAppUnitsToDevUnits(t2p);
    NS_RELEASE(dx);
#if 0
    /* You would think that doing a move and resize all in one operation would
     * be faster but its not. Something is really broken here. So I'm comenting 
     * this out for now 
     */
    // if we moved and resized do it all in one shot
    if (mVFlags & NS_VIEW_PUBLIC_FLAG_WIDGET_MOVED && mVFlags & NS_VIEW_PUBLIC_FLAG_WIDGET_RESIZED)
    {

      nscoord parx = 0, pary = 0;
      nsIWidget         *pwidget = nsnull;

      GetOffsetFromWidget(&parx, &pary, pwidget);
      NS_IF_RELEASE(pwidget);

      PRInt32 x = NSTwipsToIntPixels(mDimBounds.x + parx, t2p);
      PRInt32 y = NSTwipsToIntPixels(mDimBounds.y + pary, t2p);
      PRInt32 width = NSTwipsToIntPixels(mDimBounds.width, t2p);
      PRInt32 height = NSTwipsToIntPixels(mDimBounds.height, t2p);

      nsRect bounds;
      mWindow->GetBounds(bounds);
      if (bounds.x == x && bounds.y == y ) 
         mVFlags &= ~NS_VIEW_FLAG_WIDGET_MOVED;
      else if (bounds.width == width && bounds.height == bounds.height)
         mVFlags &= ~NS_VIEW_FLAG_WIDGET_RESIZED;
      else {
         mWindow->Resize(x,y,width,height, PR_TRUE);
         mVFlags &= ~NS_VIEW_FLAG_WIDGET_RESIZED;
         mVFlags &= ~NS_VIEW_FLAG_WIDGET_MOVED;
         return NS_OK;
      }
    } 
#endif
    // if we just resized do it
    if (mVFlags & NS_VIEW_FLAG_WIDGET_RESIZED) 
    {

      PRInt32 width = NSTwipsToIntPixels(mDimBounds.width, t2p);
      PRInt32 height = NSTwipsToIntPixels(mDimBounds.height, t2p);

      nsRect bounds;
      mWindow->GetBounds(bounds);

      if (bounds.width != width || bounds.height != bounds.height) {
#ifdef DEBUG_evaughan
        printf("%d) Resize(%d,%d)\n", this, width, height);
#endif
        mWindow->Resize(width,height, PR_TRUE);
      }

      mVFlags &= ~NS_VIEW_FLAG_WIDGET_RESIZED;
    } 
    
    if (mVFlags & NS_VIEW_FLAG_WIDGET_MOVED) {
      // if we just moved do it.
      nscoord parx = 0, pary = 0;
      nsIWidget         *pwidget = nsnull;

      GetOffsetFromWidget(&parx, &pary, pwidget);
      NS_IF_RELEASE(pwidget);

      PRInt32 x = NSTwipsToIntPixels(mDimBounds.x + parx, t2p);
      PRInt32 y = NSTwipsToIntPixels(mDimBounds.y + pary, t2p);

      nsRect bounds;
      mWindow->GetBounds(bounds);
      
      if (bounds.x != x || bounds.y != y) {
#ifdef DEBUG_evaughan
         printf("%d) Move(%d,%d)\n", this, x, y);
#endif
         mWindow->Move(x,y);
      }

      mVFlags &= ~NS_VIEW_FLAG_WIDGET_MOVED;
    }        
  }
  

  return NS_OK;
}

NS_IMETHODIMP nsView::GetPosition(nscoord *x, nscoord *y) const
{

  nsView *rootView = mViewManager->GetRootView();

  if (this == rootView)
    *x = *y = 0;
  else
  {

    *x = mPosX;
    *y = mPosY;

  }


  return NS_OK;
}

void nsView::SetDimensions(const nsRect& aRect, PRBool aPaint)
{
  nsRect dims = aRect;
  dims.MoveBy(mPosX, mPosY);

  if (mDimBounds.x == dims.x && mDimBounds.y == dims.y && mDimBounds.width == dims.width
      && mDimBounds.height == dims.height) {
    return;
  }
  
  if (nsnull == mWindow)
  {
    mDimBounds = dims;
  }
  else
  {
    PRBool needToMoveWidget = mDimBounds.x != dims.x || mDimBounds.y != dims.y;

    mDimBounds = dims;

    PRBool caching = PR_FALSE;
    mViewManager->IsCachingWidgetChanges(&caching);
    if (caching) {
      mVFlags |= NS_VIEW_FLAG_WIDGET_RESIZED | (needToMoveWidget ? NS_VIEW_FLAG_WIDGET_MOVED : 0);
      return;
    }

    nsIDeviceContext  *dx;
    float             t2p;
    nsIWidget         *pwidget = nsnull;
    nscoord           parx = 0, pary = 0;
  
    mViewManager->GetDeviceContext(dx);
    dx->GetAppUnitsToDevUnits(t2p);

    GetOffsetFromWidget(&parx, &pary, pwidget);
    NS_IF_RELEASE(pwidget);
    
    if (needToMoveWidget) {
      mWindow->Move(NSTwipsToIntPixels((mDimBounds.x + parx), t2p),
                    NSTwipsToIntPixels((mDimBounds.y + pary), t2p));
    }
    mWindow->Resize(NSTwipsToIntPixels(mDimBounds.width, t2p), NSTwipsToIntPixels(mDimBounds.height, t2p),
                    aPaint);

    NS_RELEASE(dx);
  }
}

NS_IMETHODIMP nsView::GetBounds(nsRect &aBounds) const
{
  NS_ASSERTION(mViewManager, "mViewManager is null!");
  if (!mViewManager) {
    aBounds.x = aBounds.y = 0;
    return NS_ERROR_FAILURE;
  }

  nsView *rootView = mViewManager->GetRootView();
  aBounds = mDimBounds;

  if (this == rootView)
    aBounds.x = aBounds.y = 0;

  return NS_OK;
}

NS_IMETHODIMP nsView::SetChildClip(nscoord aLeft, nscoord aTop, nscoord aRight, nscoord aBottom)
{
  NS_PRECONDITION(aLeft <= aRight && aTop <= aBottom, "bad clip values");
  mChildClip.mLeft = aLeft;
  mChildClip.mTop = aTop;
  mChildClip.mRight = aRight;
  mChildClip.mBottom = aBottom;

  

  return NS_OK;
}

NS_IMETHODIMP nsView::GetChildClip(nscoord *aLeft, nscoord *aTop, nscoord *aRight, nscoord *aBottom) const
{
  *aLeft = mChildClip.mLeft;
  *aTop = mChildClip.mTop;
  *aRight = mChildClip.mRight;
  *aBottom = mChildClip.mBottom; 
  return NS_OK;
}

NS_IMETHODIMP nsView::SetVisibility(nsViewVisibility aVisibility)
{

  mVis = aVisibility;

  if (aVisibility == nsViewVisibility_kHide)
  {
    nsView* grabbingView = mViewManager->GetMouseEventGrabber(); //check to see if we are grabbing events
    if (grabbingView == this)
    {
      //if yes then we must release them before we become hidden and can't get them
      PRBool boolResult;//not used
      mViewManager->GrabMouseEvents(nsnull,boolResult);
    }
  }

  if (nsnull != mWindow)
  {
#ifndef HIDE_ALL_WIDGETS
    if (mVis == nsViewVisibility_kShow)
      mWindow->Show(PR_TRUE);
    else
#endif
      mWindow->Show(PR_FALSE);
  }

  return NS_OK;
}

NS_IMETHODIMP nsView::GetVisibility(nsViewVisibility &aVisibility) const
{
  aVisibility = mVis;
  return NS_OK;
}

NS_IMETHODIMP nsView::GetZIndex(PRBool &aAuto, PRInt32 &aZIndex) const
{
	aAuto = (mVFlags & NS_VIEW_FLAG_AUTO_ZINDEX) != 0;
  aZIndex = mZIndex;
  return NS_OK;
}

NS_IMETHODIMP nsView::SetFloating(PRBool aFloatingView)
{
	if (aFloatingView)
		mVFlags |= NS_VIEW_FLAG_FLOATING;
	else
		mVFlags &= ~NS_VIEW_FLAG_FLOATING;

#if 0
	// recursively make all sub-views "floating" grr.
	nsIView *child = mFirstChild;
	while (nsnull != child) {
		child->SetFloating(aFloatingView);
		child->GetNextSibling(child);
	}
#endif

	return NS_OK;
}

NS_IMETHODIMP nsView::GetFloating(PRBool &aFloatingView) const
{
	aFloatingView = ((mVFlags & NS_VIEW_FLAG_FLOATING) != 0);
	return NS_OK;
}

NS_IMETHODIMP nsView::GetParent(nsIView *&aParent) const
{
  aParent = mParent;
  return NS_OK;
}

NS_IMETHODIMP nsView::GetNextSibling(nsIView *&aNextSibling) const
{
  aNextSibling = mNextSibling;
  return NS_OK;
}

void nsView::InsertChild(nsView *aChild, nsView *aSibling)
{
  NS_PRECONDITION(nsnull != aChild, "null ptr");

  if (nsnull != aChild)
  {
    if (nsnull != aSibling)
    {
#ifdef NS_DEBUG
      nsView* siblingParent = aSibling->GetParent();
      NS_ASSERTION(siblingParent == this, "tried to insert view with invalid sibling");
#endif
      //insert after sibling
      nsView* siblingNextSibling = aSibling->GetNextSibling();
      aChild->SetNextSibling(siblingNextSibling);
      aSibling->SetNextSibling(aChild);
    }
    else
    {
      aChild->SetNextSibling(mFirstChild);
      mFirstChild = aChild;
    }
    aChild->SetParent(this);
    mNumKids++;
  }
}

void nsView::RemoveChild(nsView *child)
{
  NS_PRECONDITION(nsnull != child, "null ptr");

  if (nsnull != child)
  {
    nsView* prevKid = nsnull;
    nsView* kid = mFirstChild;
    PRBool found = PR_FALSE;
    while (nsnull != kid) {
      if (kid == child) {
        if (nsnull != prevKid) {
          nsView* kidNextSibling = kid->GetNextSibling();
          prevKid->SetNextSibling(kidNextSibling);
        } else {
          mFirstChild = kid->GetNextSibling();
        }
        child->SetParent(nsnull);
        mNumKids--;
        found = PR_TRUE;
        break;
      }
      prevKid = kid;
      mChildRemoved = PR_TRUE;
	    kid = kid->GetNextSibling();
    }
    NS_ASSERTION(found, "tried to remove non child");
  }
}

nsView* nsView::GetChild(PRInt32 aIndex) const
{ 
  for (nsView* child = GetFirstChild(); child != nsnull; child = child->GetNextSibling()) {
    if (aIndex == 0) {
      return child;
    }
    --aIndex;
  }
  return nsnull;
}

NS_IMETHODIMP nsView::SetOpacity(float opacity)
{
  mOpacity = opacity;
  return NS_OK;
}

NS_IMETHODIMP nsView::GetOpacity(float &aOpacity) const
{
  aOpacity = mOpacity;
  return NS_OK;
}

NS_IMETHODIMP nsView::HasTransparency(PRBool &aTransparent) const
{
  aTransparent = (mVFlags & NS_VIEW_FLAG_TRANSPARENT) ? PR_TRUE : PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsView::SetContentTransparency(PRBool aTransparent)
{
  if (aTransparent == PR_TRUE)
    mVFlags |= NS_VIEW_FLAG_TRANSPARENT;
  else
    mVFlags &= ~NS_VIEW_FLAG_TRANSPARENT;

  return NS_OK;
}

NS_IMETHODIMP nsView::SetClientData(void *aData)
{
  mClientData = aData;
  return NS_OK;
}

NS_IMETHODIMP nsView::GetClientData(void *&aData) const
{
  aData = mClientData;
  return NS_OK;
}

NS_IMETHODIMP nsView::CreateWidget(const nsIID &aWindowIID,
                                     nsWidgetInitData *aWidgetInitData,
                                     nsNativeWidget aNative,
                                     PRBool aEnableDragDrop,
                                     PRBool aResetVisibility)
{
  nsIDeviceContext  *dx;
  nsRect            trect = mDimBounds;
  float             scale;

  NS_IF_RELEASE(mWindow);

  mViewManager->GetDeviceContext(dx);
  dx->GetAppUnitsToDevUnits(scale);

  trect *= scale;

  if (NS_OK == LoadWidget(aWindowIID))
  {
    PRBool usewidgets;

    dx->SupportsNativeWidgets(usewidgets);

    if (PR_TRUE == usewidgets)
    {
      if (aNative)
        mWindow->Create(aNative, trect, ::HandleEvent, dx, nsnull, nsnull, aWidgetInitData);
      else
      {
        nsIWidget *parent;
        GetOffsetFromWidget(nsnull, nsnull, parent);
        mWindow->Create(parent, trect, ::HandleEvent, dx, nsnull, nsnull, aWidgetInitData);
        NS_IF_RELEASE(parent);
      }
      if (aEnableDragDrop) {
        mWindow->EnableDragDrop(PR_TRUE);
      }
      
      // propagate the z-index to the widget.
      mWindow->SetZIndex(mZIndex);
    }
  }

  //make sure visibility state is accurate

  if (aResetVisibility) {
    nsViewVisibility vis;
    
    GetVisibility(vis);
    SetVisibility(vis);
  }

  NS_RELEASE(dx);

  return NS_OK;
}

void nsView::SetZIndex(PRBool aAuto, PRInt32 aZIndex)
{
  mVFlags = (mVFlags & ~NS_VIEW_FLAG_AUTO_ZINDEX) | (aAuto ? NS_VIEW_FLAG_AUTO_ZINDEX : 0);
  mZIndex = aZIndex;

  if (nsnull != mWindow) {
    mWindow->SetZIndex(aZIndex);
  }
}

NS_IMETHODIMP nsView::SetWidget(nsIWidget *aWidget)
{
  NS_IF_RELEASE(mWindow);
  mWindow = aWidget;

  if (nsnull != mWindow)
  {
    NS_ADDREF(mWindow);
    mWindow->SetClientData((void *)this);
  }

  return NS_OK;
}

NS_IMETHODIMP nsView::GetWidget(nsIWidget *&aWidget) const
{
  NS_IF_ADDREF(mWindow);
  aWidget = mWindow;
  return NS_OK;
}

NS_IMETHODIMP nsView::HasWidget(PRBool *aHasWidget) const
{
	*aHasWidget = (mWindow != nsnull);
	return NS_OK;
}

//
// internal window creation functions
//
nsresult nsView::LoadWidget(const nsCID &aClassIID)
{
  nsresult rv = nsComponentManager::CreateInstance(aClassIID, nsnull, NS_GET_IID(nsIWidget), (void**)&mWindow);

  if (NS_OK == rv) {
    // Set the widget's client data
    mWindow->SetClientData((void*)this);
  }

  return rv;
}

NS_IMETHODIMP nsView::List(FILE* out, PRInt32 aIndent) const
{
  PRInt32 i;
  for (i = aIndent; --i >= 0; ) fputs("  ", out);
  fprintf(out, "%p ", (void*)this);
  if (nsnull != mWindow) {
    nsRect windowBounds;
    nsRect nonclientBounds;
    float p2t;
    nsIDeviceContext *dx;
    mViewManager->GetDeviceContext(dx);
    dx->GetDevUnitsToAppUnits(p2t);
    NS_RELEASE(dx);
    mWindow->GetClientBounds(windowBounds);
    windowBounds *= p2t;
    mWindow->GetBounds(nonclientBounds);
    nonclientBounds *= p2t;
    nsrefcnt widgetRefCnt = mWindow->AddRef() - 1;
    mWindow->Release();
    fprintf(out, "(widget=%p[%d] pos={%d,%d,%d,%d}) ",
            (void*)mWindow, widgetRefCnt,
            nonclientBounds.x, nonclientBounds.y,
            windowBounds.width, windowBounds.height);
  }
  nsRect brect;
  GetBounds(brect);
  fprintf(out, "{%d,%d,%d,%d}",
          brect.x, brect.y, brect.width, brect.height);
  PRBool  hasTransparency;
  HasTransparency(hasTransparency);
  fprintf(out, " z=%d vis=%d opc=%1.3f tran=%d clientData=%p <\n", mZIndex, mVis, mOpacity, hasTransparency, mClientData);
  nsView* kid = mFirstChild;
  while (nsnull != kid) {
    kid->List(out, aIndent + 1);
    kid = kid->GetNextSibling();
  }
  for (i = aIndent; --i >= 0; ) fputs("  ", out);
  fputs(">\n", out);
  
  return NS_OK;
}

NS_IMETHODIMP nsView::GetOffsetFromWidget(nscoord *aDx, nscoord *aDy, nsIWidget *&aWidget)
{
  nsView   *ancestor = GetParent();
  aWidget = nsnull;
  
  // XXX aDx and aDy are OUT parameters and so we should initialize them
  // to 0 rather than relying on the caller to do so...
  while (nsnull != ancestor)
  {
    ancestor->GetWidget(aWidget);
	  if (nsnull != aWidget) {
      // the widget's (0,0) is at the top left of the view's bounds, NOT its position
      nsRect r;
      ancestor->GetDimensions(r);
      aDx -= r.x;
      aDy -= r.y;
	    return NS_OK;
    }

    if ((nsnull != aDx) && (nsnull != aDy))
    {
      ancestor->ConvertToParentCoords(aDx, aDy);
    }

	  ancestor = ancestor->GetParent();
  }

  
  if (nsnull == aWidget) {
       // The root view doesn't have a widget
       // but maybe the view manager does.
    GetViewManager()->GetWidget(&aWidget);
  }

  return NS_OK;
}

NS_IMETHODIMP nsView::GetDirtyRegion(nsIRegion *&aRegion) const
{
	if (nsnull == mDirtyRegion) {
		// The view doesn't have a dirty region so create one
		nsresult rv = nsComponentManager::CreateInstance(kRegionCID, 
		                               nsnull, 
		                               NS_GET_IID(nsIRegion), 
		                               (void**) &mDirtyRegion);

		if (NS_FAILED(rv))
			return rv;
		
		rv = mDirtyRegion->Init();
		if (NS_FAILED(rv))
			return rv;
	}

	aRegion = mDirtyRegion;
	NS_ADDREF(aRegion);
	
	return NS_OK;
}


NS_IMETHODIMP nsView::SetCompositorFlags(PRUint32 aFlags)
{
	mCompositorFlags = aFlags;
	return NS_OK;
}

NS_IMETHODIMP nsView::GetCompositorFlags(PRUint32 *aFlags)
{
	NS_ASSERTION((aFlags != nsnull), "no flags");
	*aFlags = mCompositorFlags;
	return NS_OK;
}

PRBool nsView::IsRoot()
{
  NS_ASSERTION(mViewManager != nsnull," View manager is null in nsView::IsRoot()");
  return mViewManager->GetRootView() == this;
}

PRBool nsView::PointIsInside(nsView& aView, nscoord x, nscoord y) const
{
  nsRect clippedRect;
  PRBool empty;
  PRBool clipped;
  aView.GetClippedRect(clippedRect, clipped, empty);
  if (PR_TRUE == empty) {
    // Rect is completely clipped out so point can not
    // be inside it.
    return PR_FALSE;
  }

  // Check to see if the point is within the clipped rect.
  if (clippedRect.Contains(x, y)) {
    return PR_TRUE;
  } else {
    return PR_FALSE;
  } 
}

NS_IMETHODIMP nsView::GetClippedRect(nsRect& aClippedRect, PRBool& aIsClipped, PRBool& aEmpty) const
{
  // Keep track of the view's offset
  // from its ancestor.
  nscoord ancestorX = 0;
  nscoord ancestorY = 0;

  aEmpty = PR_FALSE;
  aIsClipped = PR_FALSE;
  
  GetBounds(aClippedRect);
  nsView* parentView = GetParent();

  // Walk all of the way up the views to see if any
  // ancestor sets the NS_VIEW_PUBLIC_FLAG_CLIPCHILDREN
  while (parentView) {  
    if ((parentView->GetViewFlags() & NS_VIEW_FLAG_CLIPCHILDREN) != 0) {
      aIsClipped = PR_TRUE;
      // Adjust for clip specified by ancestor
      nscoord clipLeft;
      nscoord clipTop;
      nscoord clipRight;
      nscoord clipBottom;
      parentView->GetChildClip(&clipLeft, &clipTop, &clipRight, &clipBottom);
      nsRect clipRect;
      //Offset the cliprect by the amount the child offsets from the parent
      clipRect.x = clipLeft + ancestorX;
      clipRect.y = clipTop + ancestorY;
      clipRect.width = clipRight - clipLeft;
      clipRect.height = clipBottom - clipTop;
      PRBool overlap = aClippedRect.IntersectRect(clipRect, aClippedRect);
      if (!overlap) {
        aEmpty = PR_TRUE; // Does not intersect so the rect is empty.
        return NS_OK;
      }
    }

    parentView->ConvertFromParentCoords(&ancestorX, &ancestorY);

    parentView = parentView->GetParent();
  }
 
  return NS_OK;
}


