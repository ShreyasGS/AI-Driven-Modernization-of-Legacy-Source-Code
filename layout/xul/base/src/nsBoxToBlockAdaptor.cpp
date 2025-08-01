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
 * The Original Code is Mozilla Communicator client code.
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

//
// Eric Vaughan
// Netscape Communications
//
// See documentation in associated header file
//

#include "nsBoxLayoutState.h"
#include "nsBoxFrame.h"
#include "nsIStyleContext.h"
#include "nsIPresContext.h"
#include "nsCOMPtr.h"
#include "nsHTMLIIDs.h"
#include "nsUnitConversion.h"
#include "nsINameSpaceManager.h"
#include "nsHTMLAtoms.h"
#include "nsXULAtoms.h"
#include "nsHTMLReflowCommand.h"
#include "nsIContent.h"
#include "nsHTMLParts.h"
#include "nsIViewManager.h"
#include "nsIView.h"
#include "nsIPresShell.h"
#include "nsFrameNavigator.h"
#include "nsCSSRendering.h"
#include "nsIPref.h"
#include "nsIServiceManager.h"
#include "nsBoxToBlockAdaptor.h"
#include "nsILineIterator.h"
#include "nsIFontMetrics.h"
#include "nsHTMLContainerFrame.h"
#include "nsWidgetsCID.h"
#include "nsIWidget.h"
#include "nsBlockFrame.h"

static NS_DEFINE_IID(kWidgetCID, NS_CHILD_CID);

//#define DEBUG_REFLOW
//#define DEBUG_GROW


#ifdef DEBUG_REFLOW
PRInt32 gIndent2 = 0;

void
nsAdaptorAddIndents()
{
    for(PRInt32 i=0; i < gIndent2; i++)
    {
        printf(" ");
    }
}

void
nsAdaptorPrintReason(nsHTMLReflowState& aReflowState)
{
    char* reflowReasonString;

    switch(aReflowState.reason) 
    {
        case eReflowReason_Initial:
          reflowReasonString = "initial";
          break;

        case eReflowReason_Resize:
          reflowReasonString = "resize";
          break;
        case eReflowReason_Dirty:
          reflowReasonString = "dirty";
          break;
        case eReflowReason_StyleChange:
          reflowReasonString = "stylechange";
          break;
        case eReflowReason_Incremental: 
        {
           nsReflowType  type;
            aReflowState.reflowCommand->GetType(type);
            switch (type) {
              case eReflowType_StyleChanged:
                 reflowReasonString = "incremental (StyleChanged)";
              break;
              case eReflowType_ReflowDirty:
                 reflowReasonString = "incremental (ReflowDirty)";
              break;
              default:
                 reflowReasonString = "incremental (Unknown)";
            }
        }                             
        break;
        default:
          reflowReasonString = "unknown";
          break;
    }

    printf("%s",reflowReasonString);
}

#endif

nsBoxToBlockAdaptor::nsBoxToBlockAdaptor(nsIPresShell* aPresShell, nsIFrame* aFrame):nsBox(aPresShell)
{
  mFrame = aFrame;
  mWasCollapsed = PR_FALSE;
  mCachedMaxElementHeight = 0;
  mStyleChange = PR_FALSE;
  mOverflow.width = 0;
  mOverflow.height = 0;
  mIncludeOverflow = PR_TRUE;
  mPresShell = aPresShell;
  NeedsRecalc();

  // If we're wrapping a block (we may not be!), be sure the block
  // gets a space manager.
  static const nsIID kBlockFrameCID = NS_BLOCK_FRAME_CID;
  void *block;
  mFrame->QueryInterface(kBlockFrameCID, &block);
  if (block) {
    nsFrameState state;
    mFrame->GetFrameState(&state);
    state |= NS_BLOCK_SPACE_MGR;
    mFrame->SetFrameState(state);
  }
#ifdef DEBUG_waterson
  else {
    printf("*** nsBoxToBlockAdaptor: wrapping non-block frame ");
    nsFrame::ListTag(stdout, mFrame);
    printf("\n");
  }
#endif /* DEBUG_waterson */
}

NS_IMETHODIMP
nsBoxToBlockAdaptor::SetParentBox(nsIBox* aParent)
{
  nsresult rv = nsBox::SetParentBox(aParent);

  nsIBox* parent = aParent;

  
  if (parent) {
    PRBool needsWidget = PR_FALSE;
    parent->ChildrenMustHaveWidgets(needsWidget);
    if (needsWidget) {
        nsCOMPtr<nsIPresContext> context;
        mPresShell->GetPresContext(getter_AddRefs(context));
        nsIView* view = nsnull;
        mFrame->GetView(context, &view);

        if (!view) {
           nsCOMPtr<nsIStyleContext> style;
           mFrame->GetStyleContext(getter_AddRefs(style));
           nsHTMLContainerFrame::CreateViewForFrame(context,mFrame,style,nsnull,PR_TRUE); 
           mFrame->GetView(context, &view);
        }

        nsCOMPtr<nsIWidget> widget;
        view->GetWidget(*getter_AddRefs(widget));

        if (!widget)
           view->CreateWidget(kWidgetCID);   
    }
  }
  

  return rv;
}

PRBool
nsBoxToBlockAdaptor::HasStyleChange()
{
  return mStyleChange;
}

void
nsBoxToBlockAdaptor::GetBoxName(nsAutoString& aName)
{
#ifdef DEBUG
   nsIFrameDebug*  frameDebug;
   nsAutoString name;
   if (NS_SUCCEEDED(mFrame->QueryInterface(NS_GET_IID(nsIFrameDebug), (void**)&frameDebug))) {
      frameDebug->GetFrameName(name);
   }

  aName = name;
#endif
}

void
nsBoxToBlockAdaptor::SetStyleChangeFlag(PRBool aDirty)
{
  nsBox::SetStyleChangeFlag(aDirty);
  mStyleChange = PR_TRUE;
}


void* 
nsBoxToBlockAdaptor::operator new(size_t sz, nsIPresShell* aPresShell)
{
    return nsBoxLayoutState::Allocate(sz,aPresShell);
}

NS_IMETHODIMP 
nsBoxToBlockAdaptor::Recycle(nsIPresShell* aPresShell)
{
  delete this;
  nsBoxLayoutState::RecycleFreedMemory(aPresShell, this);
  return NS_OK;
}

NS_IMETHODIMP
nsBoxToBlockAdaptor::GetFrame(nsIFrame** aFrame)
{
  *aFrame = mFrame;  
  return NS_OK;
}

// Overridden to prevent the global delete from being called, since the memory
// came out of an nsIArena instead of the global delete operator's heap.
void 
nsBoxToBlockAdaptor::operator delete(void* aPtr, size_t sz)
{
    nsBoxLayoutState::Free(aPtr, sz);
}

nsBoxToBlockAdaptor::~nsBoxToBlockAdaptor()
{
}


NS_IMETHODIMP
nsBoxToBlockAdaptor::NeedsRecalc()
{
  SizeNeedsRecalc(mPrefSize);
  SizeNeedsRecalc(mMinSize);
  SizeNeedsRecalc(mMaxSize);
  SizeNeedsRecalc(mBlockPrefSize);
  SizeNeedsRecalc(mBlockMinSize);
  CoordNeedsRecalc(mFlex);
  CoordNeedsRecalc(mAscent);
  return NS_OK;
}

NS_IMETHODIMP
nsBoxToBlockAdaptor::GetOverflow(nsSize& aOverflow)
{
  aOverflow = mOverflow;
  return NS_OK;
}

NS_IMETHODIMP
nsBoxToBlockAdaptor::SetIncludeOverflow(PRBool aInclude)
{
  mIncludeOverflow = aInclude;
  return NS_OK;
}

NS_IMETHODIMP
nsBoxToBlockAdaptor::RefreshSizeCache(nsBoxLayoutState& aState)
{

  // Ok we need to compute our minimum, preferred, and maximum sizes.
  // 1) Maximum size. This is easy. Its infinite unless it is overloaded by CSS.
  // 2) Preferred size. This is a little harder. This is the size the block would be 
  //      if it were laid out on an infinite canvas. So we can get this by reflowing
  //      the block with and INTRINSIC width and height. We can also do a nice optimization
  //      for incremental reflow. If the reflow is incremental then we can pass a flag to 
  //      have the block compute the preferred width for us! Preferred height can just be
  //      the minimum height;
  // 3) Minimum size. This is a toughy. We can pass the block a flag asking for the max element
  //    size. That would give us the width. Unfortunately you can only ask for a maxElementSize
  //    during an incremental reflow. So on other reflows we will just have to use 0.
  //    The min height on the other hand is fairly easy we need to get the largest
  //    line height. This can be done with the line iterator.
  
  // if we do have a reflow state
  nsresult rv = NS_OK;
  const nsHTMLReflowState* reflowState = aState.GetReflowState();
  if (reflowState) {
    nsIPresContext* presContext = aState.GetPresContext();
    nsReflowStatus status = NS_FRAME_COMPLETE;
    nsHTMLReflowMetrics desiredSize(nsnull);
    nsReflowReason reason;

    // See if we an set the max element size and return the reflow states new reason. Sometimes reflow states need to 
    // be changed. Incremental dirty reflows targeted at us can be converted to Resize if we are not dirty. So make sure
    // we look at the reason returned.
    PRBool canSetMaxElementSize = CanSetMaxElementSize(aState, reason);

    // If its a resize nothing in the block could have changed.
    // so use our cached sizes instead. Well of course that is if we have cached sizes
    // if not we have to compute them.
    if (!DoesNeedRecalc(mBlockPrefSize) && reason == eReflowReason_Resize)
     return NS_OK;

    // get the old rect.
    nsRect oldRect;
    mFrame->GetRect(oldRect);

    // the rect we plan to size to.
    nsRect rect(oldRect);
    
    // if we can set the maxElementSize then 
    // tell the metrics we want it. And also tell it we want
    // to compute the max width. This will allow us to get the min width and the pref width.
    nsSize maxElementSize(0,0);
    if (canSetMaxElementSize) {
       desiredSize.mFlags |= NS_REFLOW_CALC_MAX_WIDTH;
       desiredSize.maxElementSize = &maxElementSize;
    } else {
     // if we can't set the maxElementSize. Then we must reflow
     // uncontrained.
     rect.width = NS_UNCONSTRAINEDSIZE;
     rect.height = NS_UNCONSTRAINEDSIZE;
    }


    // do the nasty.
    rv = Reflow(aState,
                presContext, 
                desiredSize, 
                *reflowState, 
                status,
                rect.x,
                rect.y,
                rect.width,
                rect.height);

    nsRect newRect;
    mFrame->GetRect(newRect);

    // make sure we draw any size change
    if (reason == eReflowReason_Incremental && (oldRect.width != newRect.width || oldRect.height != newRect.height)) {
     newRect.x = 0;
     newRect.y = 0;
     Redraw(aState, &newRect);
    }

    // if someone asked the nsBoxLayoutState to get the max size lets handle that.
    nsSize* stateMaxElementSize = nsnull;
    aState.GetMaxElementSize(&stateMaxElementSize);

    // the max element size is the largest height and width
    if (stateMaxElementSize) {

      if (mBlockMinSize.width > stateMaxElementSize->width)
        stateMaxElementSize->width = mBlockMinSize.width;

      if (mBlockMinSize.height > stateMaxElementSize->height)
         stateMaxElementSize->height = mBlockMinSize.height;

      mCachedMaxElementHeight = stateMaxElementSize->height;
    }
 
    mBlockMinSize.height = 0;
    // if we can use the maxElmementSize then lets use it
    // if not then just use the desired.
    if (canSetMaxElementSize) {
      mBlockPrefSize.width  = desiredSize.mMaximumWidth;
      mBlockMinSize.width   = maxElementSize.width; 
      // ok we need the max ascent of the items on the line. So to do this
      // ask the block for its line iterator. Get the max ascent.
      nsresult rv;
      nsCOMPtr<nsILineIterator> lines = do_QueryInterface(mFrame, &rv);
      if (NS_SUCCEEDED(rv) && lines) 
      {
        mBlockMinSize.height = 0;
        int count = 0;
        nsIFrame* firstFrame = nsnull;
        PRInt32 framesOnLine;
        nsRect lineBounds;
        PRUint32 lineFlags;

        do {
           lines->GetLine(count, &firstFrame, &framesOnLine, lineBounds, &lineFlags);
 
           if (lineBounds.height > mBlockMinSize.height)
             mBlockMinSize.height = lineBounds.height;

           count++;
        } while(firstFrame);
      }

      mBlockPrefSize.height  = mBlockMinSize.height;
    } else {
      mBlockPrefSize.width = desiredSize.width;
      mBlockPrefSize.height = desiredSize.height;
      // this sucks. We could not get the width.
      mBlockMinSize.width = 0;
      mBlockMinSize.height = desiredSize.height;
    }

    mBlockAscent = desiredSize.ascent;

#ifdef DEBUG_adaptor
    printf("min=(%d,%d), pref=(%d,%d), ascent=%d\n", mBlockMinSize.width,
                                                     mBlockMinSize.height,
                                                     mBlockPrefSize.width,
                                                     mBlockPrefSize.height,
                                                     mBlockAscent);
#endif
  }

  return rv;
}

NS_IMETHODIMP
nsBoxToBlockAdaptor::GetPrefSize(nsBoxLayoutState& aState, nsSize& aSize)
{
  // If the size is cached. Then we are set just return it.
  if (!DoesNeedRecalc(mPrefSize)) {
     aSize = mPrefSize;
     return NS_OK;
  }

  aSize.width = 0;
  aSize.height = 0;

  PRBool isCollapsed = PR_FALSE;
  IsCollapsed(aState, isCollapsed);
  if (isCollapsed) {
    return NS_OK;
  } else {
    // get our size in CSS.
    PRBool completelyRedefined = nsIBox::AddCSSPrefSize(aState, this, mPrefSize);

    // Refresh our caches with new sizes.
    if (!completelyRedefined) {
       RefreshSizeCache(aState);
       mPrefSize = mBlockPrefSize;

       // notice we don't need to add our borders or padding
       // in. Thats because the block did it for us.
       // but we do need to add insets so debugging will work.
       AddInset(mPrefSize);
       nsIBox::AddCSSPrefSize(aState, this, mPrefSize);
    }
  }

  aSize = mPrefSize;

  return NS_OK;
}


NS_IMETHODIMP
nsBoxToBlockAdaptor::GetMinSize(nsBoxLayoutState& aState, nsSize& aSize)
{
  if (!DoesNeedRecalc(mMinSize)) {
     aSize = mMinSize;
     return NS_OK;
  }

  aSize.width = 0;
  aSize.height = 0;

  PRBool isCollapsed = PR_FALSE;
  IsCollapsed(aState, isCollapsed);
  if (isCollapsed) {
    return NS_OK;
  } else {
    // get our size in CSS.
    PRBool completelyRedefined = nsIBox::AddCSSMinSize(aState, this, mMinSize);

    // Refresh our caches with new sizes.
    if (!completelyRedefined) {
       RefreshSizeCache(aState);
       mMinSize = mBlockMinSize;
       AddInset(mMinSize);
       nsIBox::AddCSSMinSize(aState, this, mMinSize);
    }
  }

  aSize = mMinSize;

  return NS_OK;
}

NS_IMETHODIMP
nsBoxToBlockAdaptor::GetMaxSize(nsBoxLayoutState& aState, nsSize& aSize)
{
  if (!DoesNeedRecalc(mMaxSize)) {
     aSize = mMaxSize;
     return NS_OK;
  }

  aSize.width = NS_INTRINSICSIZE;
  aSize.height = NS_INTRINSICSIZE;

  PRBool isCollapsed = PR_FALSE;
  IsCollapsed(aState, isCollapsed);
  if (isCollapsed) {
    return NS_OK;
  } else {
    mMaxSize.width = NS_INTRINSICSIZE;
    mMaxSize.height = NS_INTRINSICSIZE;
    nsBox::GetMaxSize(aState, mMaxSize);
  }

  aSize = mMaxSize;

  return NS_OK;
}

NS_IMETHODIMP
nsBoxToBlockAdaptor::GetFlex(nsBoxLayoutState& aState, nscoord& aFlex)
{
  if (!DoesNeedRecalc(mFlex)) {
     aFlex = mFlex;
     return NS_OK;
  }

  mFlex = 0;
  nsBox::GetFlex(aState, mFlex);

  aFlex = mFlex;

  return NS_OK;
}

NS_IMETHODIMP
nsBoxToBlockAdaptor::GetAscent(nsBoxLayoutState& aState, nscoord& aAscent)
{
  if (!DoesNeedRecalc(mAscent)) {
    aAscent = mAscent;
    return NS_OK;
  }

  PRBool isCollapsed = PR_FALSE;
  IsCollapsed(aState, isCollapsed);
  if (isCollapsed) {
    mAscent = 0;
  } else {
    // Refresh our caches with new sizes.
    RefreshSizeCache(aState);
    mAscent = mBlockAscent;
    nsMargin m(0,0,0,0);
    GetInset(m);
    mAscent += m.top;    
  }

  return NS_OK;
}

NS_IMETHODIMP
nsBoxToBlockAdaptor::IsCollapsed(nsBoxLayoutState& aState, PRBool& aCollapsed)
{
  return nsBox::IsCollapsed(aState, aCollapsed);
}

nsresult
nsBoxToBlockAdaptor::DoLayout(nsBoxLayoutState& aState)
{
   nsRect ourRect(0,0,0,0);
   GetBounds(ourRect);

   const nsHTMLReflowState* reflowState = aState.GetReflowState();
   nsIPresContext* presContext = aState.GetPresContext();
   nsReflowStatus status = NS_FRAME_COMPLETE;
   nsHTMLReflowMetrics desiredSize(nsnull);
   nsresult rv = NS_OK;
 
   if (reflowState) {

    nsSize* currentSize = nsnull;
    aState.GetMaxElementSize(&currentSize);
    nsSize maxElementSize(0,0);

    if (currentSize) {
       desiredSize.maxElementSize = &maxElementSize;
    }

     rv = Reflow(aState,
                 presContext, 
                  desiredSize, 
                  *reflowState, 
                  status,
                  ourRect.x,
                  ourRect.y,
                  ourRect.width,
                  ourRect.height);
 
    if (currentSize) {
      if (maxElementSize.width > currentSize->width)
         currentSize->width = maxElementSize.width;
  
      if (mCachedMaxElementHeight > currentSize->height) {
        currentSize->height = mCachedMaxElementHeight;
      }
    }

     PRBool collapsed = PR_FALSE;
     IsCollapsed(aState, collapsed);
     if (collapsed) {
       mFrame->SizeTo(presContext, 0, 0);
     } else {

       // if our child needs to be bigger. This might happend with wrapping text. There is no way to predict its
       // height until we reflow it. Now that we know the height reshuffle upward.
       if (desiredSize.width > ourRect.width || desiredSize.height > ourRect.height) {

#ifdef DEBUG_GROW
            DumpBox(stdout);
            printf(" GREW from (%d,%d) -> (%d,%d)\n", ourRect.width, ourRect.height, desiredSize.width, desiredSize.height);
#endif

         if (desiredSize.width > ourRect.width)
            ourRect.width = desiredSize.width;

         if (desiredSize.height > ourRect.height)
            ourRect.height = desiredSize.height;
       }

       // ensure our size is what we think is should be. Someone could have
       // reset the frame to be smaller or something dumb like that. 
       mFrame->SizeTo(presContext, ourRect.width, ourRect.height);
     }
   }

   SyncLayout(aState);

  return rv;
}

nsresult
nsBoxToBlockAdaptor::Reflow(nsBoxLayoutState& aState,
                     nsIPresContext*   aPresContext,
                     nsHTMLReflowMetrics&     aDesiredSize,
                     const nsHTMLReflowState& aReflowState,
                     nsReflowStatus&          aStatus,
                     nscoord aX,
                     nscoord aY,
                     nscoord aWidth,
                     nscoord aHeight,
                     PRBool aMoveFrame)
{
  DO_GLOBAL_REFLOW_COUNT("nsBoxToBlockAdaptor", aReflowState.reason);

#ifdef DEBUG_REFLOW
  nsAdaptorAddIndents();
  printf("Reflowing: ");
  nsFrame::ListTag(stdout, mFrame);
  printf("\n");
  gIndent2++;
#endif

  //printf("width=%d, height=%d\n", aWidth, aHeight);
  /*
  nsIBox* parent;
  GetParentBox(&parent);
  nsIFrame* frame;
  parent->GetFrame(&frame);
  nsFrameState frameState = 0;
  frame->GetFrameState(&frameState);

 // if (frameState & NS_STATE_CURRENTLY_IN_DEBUG)
  //   printf("In debug\n");
  */

  aStatus = NS_FRAME_COMPLETE;

  PRBool redrawAfterReflow = PR_FALSE;
  PRBool needsReflow = PR_FALSE;
  PRBool redrawNow = PR_FALSE;
  nsReflowReason reason;
  
  HandleIncrementalReflow(aState, 
                          aReflowState, 
                          reason,
                          PR_TRUE,
                          redrawNow,
                          needsReflow, 
                          redrawAfterReflow, 
                          aMoveFrame);

  if (redrawNow)
     Redraw(aState);

  // if we don't need a reflow then 
  // lets see if we are already that size. Yes? then don't even reflow. We are done.
  if (!needsReflow) {
      
      if (aWidth != NS_INTRINSICSIZE && aHeight != NS_INTRINSICSIZE) {
      
          // if the new calculated size has a 0 width or a 0 height
          if ((mLastSize.width == 0 || mLastSize.height == 0) && (aWidth == 0 || aHeight == 0)) {
               needsReflow = PR_FALSE;
               aDesiredSize.width = aWidth; 
               aDesiredSize.height = aHeight; 
               mFrame->SizeTo(aPresContext, aDesiredSize.width, aDesiredSize.height);
          } else {
            aDesiredSize.width = mLastSize.width;
            aDesiredSize.height = mLastSize.height;

            // remove the margin. The rect of our child does not include it but our calculated size does.
            nscoord calcWidth = aWidth; 
            nscoord calcHeight = aHeight; 
            // don't reflow if we are already the right size
            if (mLastSize.width == calcWidth && mLastSize.height == calcHeight)
                  needsReflow = PR_FALSE;
            else
                  needsReflow = PR_TRUE;
   
          }
      } else {
          // if the width or height are intrinsic alway reflow because
          // we don't know what it should be.
         needsReflow = PR_TRUE;
      }
  }
                             
  // ok now reflow the child into the spacers calculated space
  if (needsReflow) {

    nsMargin border(0,0,0,0);
    GetBorderAndPadding(border);


    aDesiredSize.width = 0;
    aDesiredSize.height = 0;

    nsSize size(aWidth, aHeight);

    // create a reflow state to tell our child to flow at the given size.
    if (size.height != NS_INTRINSICSIZE) {
        size.height -= (border.top + border.bottom);
        if (size.height < 0)
          size.height = 0;
    }

    if (size.width != NS_INTRINSICSIZE) {
        size.width -= (border.left + border.right);
        if (size.width < 0)
          size.width = 0;
    }

    nsHTMLReflowState   reflowState(aPresContext, aReflowState, mFrame, nsSize(size.width, NS_INTRINSICSIZE));
    reflowState.reason = reason;
    if (reason != eReflowReason_Incremental)
       reflowState.reflowCommand = nsnull;

    // XXX this needs to subtract out the border and padding of mFrame since it is content size
    reflowState.mComputedWidth = size.width;
    reflowState.mComputedHeight = size.height;

    // if we were marked for style change.
    // 1) see if we are just supposed to do a resize if so convert to a style change. Kill 2 birds
    //    with 1 stone.
    // 2) If the command is incremental. See if its style change. If it is everything is ok if not
    //    we need to do a second reflow with the style change.
    if (mStyleChange) {
      if (reflowState.reason == eReflowReason_Resize) {
         // maxElementSize does not work on style change reflows.
         // so remove it if set.
         aDesiredSize.maxElementSize = nsnull;

         reflowState.reason = eReflowReason_StyleChange;
      } else if (reason == eReflowReason_Incremental) {
         nsReflowType  type;
          reflowState.reflowCommand->GetType(type);

          if (type != eReflowType_StyleChanged) {
             #ifdef DEBUG_REFLOW
                nsAdaptorAddIndents();
                printf("Size=(%d,%d)\n",reflowState.mComputedWidth, reflowState.mComputedHeight);
                nsAdaptorAddIndents();
                nsAdaptorPrintReason(reflowState);
                printf("\n");
             #endif

             mFrame->WillReflow(aPresContext);
             mFrame->Reflow(aPresContext, aDesiredSize, reflowState, aStatus);
             mFrame->DidReflow(aPresContext, &reflowState, NS_FRAME_REFLOW_FINISHED);
             reflowState.mComputedWidth = aDesiredSize.width - (border.left + border.right);
             reflowState.availableWidth = reflowState.mComputedWidth;
             reflowState.reason = eReflowReason_StyleChange;
             reflowState.reflowCommand = nsnull;
          }
      }

      mStyleChange = PR_FALSE;
    }

    #ifdef DEBUG_REFLOW
      nsAdaptorAddIndents();
      printf("Size=(%d,%d)\n",reflowState.mComputedWidth, reflowState.mComputedHeight);
      nsAdaptorAddIndents();
      nsAdaptorPrintReason(reflowState);
      printf("\n");
    #endif

       // place the child and reflow
    mFrame->WillReflow(aPresContext);

    mFrame->Reflow(aPresContext, aDesiredSize, reflowState, aStatus);

    NS_ASSERTION(NS_FRAME_IS_COMPLETE(aStatus), "bad status");

    // Save the ascent.  (bug 103925)
    PRBool isCollapsed = PR_FALSE;
    IsCollapsed(aState, isCollapsed);
    if (isCollapsed) {
      mAscent = 0;
    } else {
      mAscent = aDesiredSize.ascent;
    }

    nsFrameState  kidState;
    mFrame->GetFrameState(&kidState);

   // printf("width: %d, height: %d\n", aDesiredSize.mCombinedArea.width, aDesiredSize.mCombinedArea.height);

    // see if the overflow option is set. If it is then if our child's bounds overflow then
    // we will set the child's rect to include the overflow size.
       if (kidState & NS_FRAME_OUTSIDE_CHILDREN) {
         // make sure we store the overflow size
         mOverflow.width  = aDesiredSize.mOverflowArea.width;
         mOverflow.height = aDesiredSize.mOverflowArea.height;

         // include the overflow size in our child's rect?
         if (mIncludeOverflow) {
             //printf("OutsideChildren width=%d, height=%d\n", aDesiredSize.mOverflowArea.width, aDesiredSize.mOverflowArea.height);
             aDesiredSize.width = aDesiredSize.mOverflowArea.width;
             if (aDesiredSize.width <= aWidth)
               aDesiredSize.height = aDesiredSize.mOverflowArea.height;
             else {
              if (aDesiredSize.width > aWidth)
              {
                 reflowState.mComputedWidth = aDesiredSize.width - (border.left + border.right);
                 reflowState.availableWidth = reflowState.mComputedWidth;
                 reflowState.reason = eReflowReason_Resize;
                 reflowState.reflowCommand = nsnull;
                 mFrame->DidReflow(aPresContext, &reflowState, NS_FRAME_REFLOW_FINISHED);
                 #ifdef DEBUG_REFLOW
                  nsAdaptorAddIndents();
                  nsAdaptorPrintReason(reflowState);
                  printf("\n");
                 #endif
                 mFrame->WillReflow(aPresContext);
                 mFrame->Reflow(aPresContext, aDesiredSize, reflowState, aStatus);
                 mFrame->GetFrameState(&kidState);
                 if (kidState & NS_FRAME_OUTSIDE_CHILDREN)
                    aDesiredSize.height = aDesiredSize.mOverflowArea.height;

              }
             }
         }
       } else {
         mOverflow.width  = aDesiredSize.width;
         mOverflow.height = aDesiredSize.height;
       }

    if (redrawAfterReflow) {
       nsIFrame* frame = nsnull;
       GetFrame(&frame);
       nsRect r;
       frame->GetRect(r);
       r.width = aDesiredSize.width;
       r.height = aDesiredSize.height;
       Redraw(aState, &r);
    }

    PRBool changedSize = PR_FALSE;

    if (mLastSize.width != aDesiredSize.width || mLastSize.height != aDesiredSize.height)
       changedSize = PR_TRUE;
  
    nsContainerFrame::FinishReflowChild(mFrame, aPresContext, &reflowState,
                                        aDesiredSize, aX, aY, NS_FRAME_NO_MOVE_FRAME);
  } else {
    aDesiredSize.ascent = mBlockAscent;
  }
  
#ifdef DEBUG_REFLOW
  if (aHeight != NS_INTRINSICSIZE && aDesiredSize.height != aHeight)
  {
          nsAdaptorAddIndents();
          printf("*****got taller!*****\n");
         
  }
  if (aWidth != NS_INTRINSICSIZE && aDesiredSize.width != aWidth)
  {
          nsAdaptorAddIndents();
          printf("*****got wider!******\n");
         
  }
#endif

  if (aWidth == NS_INTRINSICSIZE)
     aWidth = aDesiredSize.width;

  if (aHeight == NS_INTRINSICSIZE)
     aHeight = aDesiredSize.height;

  mLastSize.width = aDesiredSize.width;
  mLastSize.height = aDesiredSize.height;

#ifdef DEBUG_REFLOW
  gIndent2--;
#endif

  return NS_OK;
}

void
nsBoxToBlockAdaptor::HandleIncrementalReflow(nsBoxLayoutState& aState, 
                                          const nsHTMLReflowState aReflowState,
                                          nsReflowReason& aReason,
                                          PRBool aPopOffIncremental,
                                          PRBool& aRedrawNow,
                                          PRBool& aNeedsReflow,
                                          PRBool& aRedrawAfterReflow,
                                          PRBool& aMoveFrame)
{
  nsFrameState childState;
  mFrame->GetFrameState(&childState);

  aReason = aReflowState.reason;

    // handle or different types of reflow
  switch(aReason)
  {
   // if the child we are reflowing is the child we popped off the incremental 
   // reflow chain then we need to reflow it no matter what.
   // if its not the child we got from the reflow chain then this child needs reflow
   // because as a side effect of the incremental child changing size it needs to be resized.
   // This will happen a lot when a box that contains 2 children with different flexibilities
   // if on child gets bigger the other is affected because it is proprotional to the first.
   // so it might need to be resized. But we don't need to reflow it. If it is already the
   // needed size then we will do nothing. 
   case eReflowReason_Incremental: {

      // if incremental see if the next child in the chain is the child. If so then
      // we will just let it go down. If not then convert it to a dirty. It will get converted back when 
      // needed in the case just below this one.
      nsIFrame* incrementalChild = nsnull;
      aReflowState.reflowCommand->GetNext(incrementalChild, PR_FALSE);
      
      // if the increment child is our child then
      // pop it off and continue sending it down
      if (incrementalChild == mFrame ) {
          aNeedsReflow = PR_TRUE;

          if (aPopOffIncremental)
             aReflowState.reflowCommand->GetNext(incrementalChild);

          // if we hit the target then we have used up the chain.
          // next time a layout 
          break;
      } 

      // fall into dirty if the incremental child was use. It should be treated as a 
   }

   // if its dirty then see if the child we want to reflow is dirty. If it is then
   // mark it as needing to be reflowed.
   case eReflowReason_Dirty: {
        // XXX nsBlockFrames don't seem to be able to handle a reason of Dirty. So we  
        // send down a resize instead. If we did send down the dirty we would have wrapping problems. If you 
        // look at the main page it will initially come up ok but will have a unneeded horizontal 
        // scrollbar if you resize it will fix it self. The real fix is to fix block frame but
        // this will fix it for beta3.
        if (childState & NS_FRAME_FIRST_REFLOW) 
           aReason = eReflowReason_Initial;
        else
           aReason = eReflowReason_Resize;

        // get the frame state to see if it needs reflow
        aNeedsReflow = mStyleChange || (childState & NS_FRAME_IS_DIRTY) || (childState & NS_FRAME_HAS_DIRTY_CHILDREN);

        // but of course by definition dirty reflows are supposed to redraw so
        // lets signal that we need to do that. We want to do it after as well because
        // the object may have changed size.
        if (aNeedsReflow) {
           aRedrawNow = PR_TRUE;
           aRedrawAfterReflow = PR_TRUE;
           //printf("Redrawing!!!/n");
        }

   } break;

   // if the a resize reflow then it doesn't need to be reflowed. Only if the size is different
   // from the new size would we actually do a reflow
   case eReflowReason_Resize:
       // blocks sometimes send resizes even when its children are dirty! We need to make sure we
       // repair in these cases. So check the flags here.
       aNeedsReflow = mStyleChange || (childState & NS_FRAME_IS_DIRTY) || (childState & NS_FRAME_HAS_DIRTY_CHILDREN);
   break;

   // if its an initial reflow we must place the child.
   // otherwise we might think it was already placed when it wasn't
   case eReflowReason_Initial:
       aMoveFrame = PR_TRUE;
       aNeedsReflow = PR_TRUE;
   break;

   default:
       aNeedsReflow = PR_TRUE;
 
  }
}

PRBool
nsBoxToBlockAdaptor::GetWasCollapsed(nsBoxLayoutState& aState)
{
  return mWasCollapsed;
}

void
nsBoxToBlockAdaptor::SetWasCollapsed(nsBoxLayoutState& aState, PRBool aCollapsed)
{
  mWasCollapsed = aCollapsed;
}

PRBool 
nsBoxToBlockAdaptor::CanSetMaxElementSize(nsBoxLayoutState& aState, nsReflowReason& aReason)
{
      PRBool redrawAfterReflow = PR_FALSE;
      PRBool needsReflow = PR_FALSE;
      PRBool redrawNow = PR_FALSE;
      PRBool move = PR_TRUE;
      const nsHTMLReflowState* reflowState = aState.GetReflowState();

      HandleIncrementalReflow(aState, 
                              *reflowState, 
                              aReason,
                              PR_FALSE,
                              redrawNow,
                              needsReflow, 
                              redrawAfterReflow, 
                              move);

      // only  incremental reflows can handle maxelementsize being set.
      if (reflowState->reason == eReflowReason_Incremental) {
        if (reflowState->reflowCommand) {
          // MaxElement doesn't work on style change reflows.. :-(
          nsReflowType  type;
          reflowState->reflowCommand->GetType(type);

          if (type == eReflowType_StyleChanged) 
            return PR_FALSE;
        }

        return PR_TRUE;
      }

      return PR_FALSE;
}

NS_IMPL_ADDREF_INHERITED(nsBoxToBlockAdaptor, nsBox);
NS_IMPL_RELEASE_INHERITED(nsBoxToBlockAdaptor, nsBox);

//
// QueryInterface
//
NS_INTERFACE_MAP_BEGIN(nsBoxToBlockAdaptor)
  NS_INTERFACE_MAP_ENTRY(nsIBoxToBlockAdaptor)
  if (NS_SUCCEEDED(mFrame->QueryInterface(aIID, aInstancePtr)))                             
    return NS_OK;                 
  else
NS_INTERFACE_MAP_END_INHERITING(nsBox)

