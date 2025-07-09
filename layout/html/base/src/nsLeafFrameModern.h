#ifndef nsLeafFrameModern_h___
#define nsLeafFrameModern_h___

#include "nsFrame.h"
#include <memory>
#include <optional>

namespace mozilla {

/**
 * Modern implementation of nsLeafFrame
 * 
 * Abstract class that provides simple fixed-size layout for leaf objects
 * (e.g. images, form elements, etc.). Derivations provide the implementation
 * of the GetDesiredSize method. The rendering method knows how to render
 * borders and backgrounds.
 */
class LeafFrame : public nsFrame {
public:
  /**
   * Factory method for creating a new LeafFrame
   * @return RefPtr to the newly created LeafFrame
   */
  static already_AddRefed<LeafFrame> Create(nsIPresContext* aPresContext,
                                           nsIStyleContext* aStyleContext);

  // nsIFrame replacements
  NS_IMETHOD Paint(nsIPresContext*      aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect,
                   nsFramePaintLayer    aWhichLayer,
                   PRUint32             aFlags = 0) override;

  NS_IMETHOD Reflow(nsIPresContext*      aPresContext,
                   nsHTMLReflowMetrics& aDesiredSize,
                   const nsHTMLReflowState& aReflowState,
                   nsReflowStatus&      aStatus) override;

  NS_IMETHOD ContentChanged(nsIPresContext* aPresContext,
                           nsIContent*     aChild,
                           nsISupports*    aSubContent) override;

#ifdef DEBUG
  NS_IMETHOD SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const override;
#endif

  /**
   * Modern version of Paint that returns a result type
   */
  struct PaintResult {
    nsresult rv;
    
    explicit operator bool() const { return NS_SUCCEEDED(rv); }
  };
  
  PaintResult PaintModern(nsIPresContext* aPresContext,
                         nsIRenderingContext& aRenderingContext,
                         const nsRect& aDirtyRect,
                         nsFramePaintLayer aWhichLayer,
                         PRUint32 aFlags = 0);

  /**
   * Modern version of Reflow that returns a result type
   */
  struct ReflowResult {
    nsresult rv;
    nsReflowStatus status;
    
    explicit operator bool() const { return NS_SUCCEEDED(rv); }
  };
  
  ReflowResult ReflowModern(nsIPresContext* aPresContext,
                           nsHTMLReflowMetrics& aMetrics,
                           const nsHTMLReflowState& aReflowState);

protected:
  // Constructor is protected since this is an abstract class
  explicit LeafFrame(nsIStyleContext* aStyleContext);
  virtual ~LeafFrame();

  /**
   * Return the desired size of the frame's content area. Note that this
   * method doesn't need to deal with padding or borders (the caller will
   * deal with it). In addition, the ascent will be set to the height
   * and the descent will be set to zero.
   */
  virtual void GetDesiredSize(nsIPresContext* aPresContext,
                             const nsHTMLReflowState& aReflowState,
                             nsHTMLReflowMetrics& aDesiredSize) = 0;

  /**
   * Subroutine to add in borders and padding
   * @return The border and padding margins
   */
  nsMargin AddBordersAndPadding(nsIPresContext* aPresContext,
                               const nsHTMLReflowState& aReflowState,
                               nsHTMLReflowMetrics& aDesiredSize);

private:
  // Prevent copying
  LeafFrame(const LeafFrame&) = delete;
  LeafFrame& operator=(const LeafFrame&) = delete;
};

} // namespace mozilla

#endif /* nsLeafFrameModern_h___ */ 