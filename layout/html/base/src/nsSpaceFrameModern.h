#ifndef nsSpaceFrameModern_h___
#define nsSpaceFrameModern_h___

#include "nsLeafFrameModern.h"

namespace mozilla {

/**
 * A simple frame that renders a fixed-size space
 * This demonstrates how to use the modernized LeafFrame
 */
class SpaceFrame : public LeafFrame {
public:
  /**
   * Factory method for creating a new SpaceFrame
   * @return RefPtr to the newly created SpaceFrame
   */
  static already_AddRefed<SpaceFrame> Create(nsIPresContext* aPresContext,
                                            nsIStyleContext* aStyleContext,
                                            nscoord aWidth,
                                            nscoord aHeight);

  // nsISupports interface
  NS_DECL_ISUPPORTS_INHERITED

protected:
  SpaceFrame(nsIStyleContext* aStyleContext, nscoord aWidth, nscoord aHeight);
  virtual ~SpaceFrame();

  // LeafFrame
  void GetDesiredSize(nsIPresContext* aPresContext,
                     const nsHTMLReflowState& aReflowState,
                     nsHTMLReflowMetrics& aDesiredSize) override;

private:
  // The fixed size of this space frame
  nscoord mWidth;
  nscoord mHeight;

  // Prevent copying
  SpaceFrame(const SpaceFrame&) = delete;
  SpaceFrame& operator=(const SpaceFrame&) = delete;
};

} // namespace mozilla

// Legacy compatibility function
nsresult NS_NewSpaceFrameModern(nsIPresContext* aPresContext,
                               nsIFrame** aNewFrame,
                               nscoord aWidth,
                               nscoord aHeight);

#endif /* nsSpaceFrameModern_h___ */ 