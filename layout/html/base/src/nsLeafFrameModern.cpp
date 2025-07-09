#include "nsLeafFrameModern.h"
#include "nsCOMPtr.h"
#include "nsHTMLContainerFrame.h"
#include "nsCSSRendering.h"
#include "nsHTMLParts.h"
#include "nsHTMLAtoms.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsIStyleContext.h"

namespace mozilla {

//----------------------------------------------------------------------

already_AddRefed<LeafFrame>
LeafFrame::Create(nsIPresContext* aPresContext, nsIStyleContext* aStyleContext)
{
  RefPtr<LeafFrame> frame = new LeafFrame(aStyleContext);
  return frame.forget();
}

LeafFrame::LeafFrame(nsIStyleContext* aStyleContext)
  : nsFrame(aStyleContext)
{
}

LeafFrame::~LeafFrame() = default;

NS_IMETHODIMP
LeafFrame::Paint(nsIPresContext* aPresContext,
                nsIRenderingContext& aRenderingContext,
                const nsRect& aDirtyRect,
                nsFramePaintLayer aWhichLayer,
                PRUint32 aFlags)
{
  // Use the modern implementation
  PaintResult result = PaintModern(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer, aFlags);
  return result.rv;
}

LeafFrame::PaintResult
LeafFrame::PaintModern(nsIPresContext* aPresContext,
                      nsIRenderingContext& aRenderingContext,
                      const nsRect& aDirtyRect,
                      nsFramePaintLayer aWhichLayer,
                      PRUint32 aFlags)
{
  PaintResult result{NS_OK};
  
  if (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer) {
    PRBool isVisible;
    result.rv = IsVisibleForPainting(aPresContext, aRenderingContext, PR_FALSE, &isVisible);
    if (NS_FAILED(result.rv) || !isVisible) {
      // Just checks selection painting, not visibility
      return result;
    }

    const nsStyleVisibility* vis = 
      static_cast<const nsStyleVisibility*>(mStyleContext->GetStyleData(eStyleStruct_Visibility));
    
    if (vis->IsVisibleOrCollapsed()) {
      // Use RAII for context state management
      class RenderingContextStateRAII {
      public:
        explicit RenderingContextStateRAII(nsIRenderingContext& aContext) : mContext(aContext) {
          mContext.PushState();
        }
        ~RenderingContextStateRAII() {
          mContext.PopState();
        }
      private:
        nsIRenderingContext& mContext;
      };
      
      // Get style data
      const nsStyleBorder* myBorder = static_cast<const nsStyleBorder*>(
        mStyleContext->GetStyleData(eStyleStruct_Border));
      const nsStyleOutline* myOutline = static_cast<const nsStyleOutline*>(
        mStyleContext->GetStyleData(eStyleStruct_Outline));
      
      // Create the rect
      nsRect rect(0, 0, mRect.width, mRect.height);
      
      // Paint background, border, and outline
      nsCSSRendering::PaintBackground(aPresContext, aRenderingContext, this,
                                     aDirtyRect, rect, *myBorder, 0, 0);
      nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
                                 aDirtyRect, rect, *myBorder,
                                 mStyleContext, 0);
      nsCSSRendering::PaintOutline(aPresContext, aRenderingContext, this,
                                  aDirtyRect, rect, *myBorder,
                                  *myOutline, mStyleContext, 0);
    }
  }
  
  DO_GLOBAL_REFLOW_COUNT_DSP("nsLeafFrameModern", &aRenderingContext);
  return result;
}

NS_IMETHODIMP
LeafFrame::Reflow(nsIPresContext* aPresContext,
                 nsHTMLReflowMetrics& aMetrics,
                 const nsHTMLReflowState& aReflowState,
                 nsReflowStatus& aStatus)
{
  // Use the modern implementation
  ReflowResult result = ReflowModern(aPresContext, aMetrics, aReflowState);
  aStatus = result.status;
  return result.rv;
}

LeafFrame::ReflowResult
LeafFrame::ReflowModern(nsIPresContext* aPresContext,
                       nsHTMLReflowMetrics& aMetrics,
                       const nsHTMLReflowState& aReflowState)
{
  ReflowResult result{NS_OK, NS_FRAME_COMPLETE};
  
  DO_GLOBAL_REFLOW_COUNT("nsLeafFrameModern", aReflowState.reason);
  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                ("enter nsLeafFrameModern::Reflow: aMaxSize=%d,%d",
                 aReflowState.availableWidth, aReflowState.availableHeight));

  NS_PRECONDITION(mState & NS_FRAME_IN_REFLOW, "frame is not in reflow");

  // XXX add in code to check for width/height being set via css
  // and if set use them instead of calling GetDesiredSize.

  // Get desired size and add borders/padding
  GetDesiredSize(aPresContext, aReflowState, aMetrics);
  nsMargin borderPadding = AddBordersAndPadding(aPresContext, aReflowState, aMetrics);
  
  // Handle max element size
  if (aMetrics.maxElementSize) {
    aMetrics.AddBorderPaddingToMaxElementSize(borderPadding);
    aMetrics.maxElementSize->width = aMetrics.width;
    aMetrics.maxElementSize->height = aMetrics.height;
  }

  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                ("exit nsLeafFrameModern::Reflow: size=%d,%d",
                 aMetrics.width, aMetrics.height));
  return result;
}

nsMargin
LeafFrame::AddBordersAndPadding(nsIPresContext* aPresContext,
                               const nsHTMLReflowState& aReflowState,
                               nsHTMLReflowMetrics& aMetrics)
{
  nsMargin borderPadding = aReflowState.mComputedBorderPadding;
  aMetrics.width += borderPadding.left + borderPadding.right;
  aMetrics.height += borderPadding.top + borderPadding.bottom;
  aMetrics.ascent = aMetrics.height;
  aMetrics.descent = 0;
  return borderPadding;
}

NS_IMETHODIMP
LeafFrame::ContentChanged(nsIPresContext* aPresContext,
                         nsIContent* aChild,
                         nsISupports* aSubContent)
{
  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));
  mState |= NS_FRAME_IS_DIRTY;
  return mParent->ReflowDirtyChild(shell, this);
}

#ifdef DEBUG
NS_IMETHODIMP
LeafFrame::SizeOf(nsISizeOfHandler* aHandler,
                 PRUint32* aResult) const
{
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  *aResult = sizeof(*this);
  return NS_OK;
}
#endif

} // namespace mozilla 