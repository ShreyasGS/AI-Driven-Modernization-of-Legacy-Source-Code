#include "nsSpaceFrameModern.h"
#include "nsIPresContext.h"
#include "nsHTMLReflowMetrics.h"
#include "nsHTMLReflowState.h"

namespace mozilla {

//----------------------------------------------------------------------

already_AddRefed<SpaceFrame>
SpaceFrame::Create(nsIPresContext* aPresContext,
                  nsIStyleContext* aStyleContext,
                  nscoord aWidth,
                  nscoord aHeight)
{
  RefPtr<SpaceFrame> frame = new SpaceFrame(aStyleContext, aWidth, aHeight);
  return frame.forget();
}

SpaceFrame::SpaceFrame(nsIStyleContext* aStyleContext, nscoord aWidth, nscoord aHeight)
  : LeafFrame(aStyleContext)
  , mWidth(aWidth)
  , mHeight(aHeight)
{
}

SpaceFrame::~SpaceFrame() = default;

NS_IMPL_ADDREF_INHERITED(SpaceFrame, LeafFrame)
NS_IMPL_RELEASE_INHERITED(SpaceFrame, LeafFrame)

// QueryInterface implementation for SpaceFrame
NS_INTERFACE_MAP_BEGIN(SpaceFrame)
  // No additional interfaces
NS_INTERFACE_MAP_END_INHERITING(LeafFrame)

void
SpaceFrame::GetDesiredSize(nsIPresContext* aPresContext,
                          const nsHTMLReflowState& aReflowState,
                          nsHTMLReflowMetrics& aDesiredSize)
{
  // Simply use our fixed size
  aDesiredSize.width = mWidth;
  aDesiredSize.height = mHeight;
  aDesiredSize.ascent = mHeight;
  aDesiredSize.descent = 0;
}

} // namespace mozilla

//----------------------------------------------------------------------
// Legacy compatibility function

nsresult
NS_NewSpaceFrameModern(nsIPresContext* aPresContext,
                      nsIFrame** aNewFrame,
                      nscoord aWidth,
                      nscoord aHeight)
{
  NS_ENSURE_ARG_POINTER(aNewFrame);
  *aNewFrame = nullptr;
  
  RefPtr<mozilla::SpaceFrame> frame = 
    mozilla::SpaceFrame::Create(aPresContext, nullptr, aWidth, aHeight);
  if (!frame) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  frame.forget(aNewFrame);
  return NS_OK;
} 