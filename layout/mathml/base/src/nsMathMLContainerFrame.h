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
 * The Original Code is Mozilla MathML Project.
 * 
 * The Initial Developer of the Original Code is The University Of 
 * Queensland.  Portions created by The University Of Queensland are
 * Copyright (C) 1999 The University Of Queensland.  All Rights Reserved.
 * 
 * Contributor(s): 
 *   Roger B. Sidje <rbs@maths.uq.edu.au>
 *   David J. Fiddes <D.J.Fiddes@hw.ac.uk>
 *   Shyjan Mahamud <mahamud@cs.cmu.edu> (added TeX rendering rules)
 */

#ifndef nsMathMLContainerFrame_h___
#define nsMathMLContainerFrame_h___

#include "nsCOMPtr.h"
#include "nsHTMLContainerFrame.h"
#include "nsBlockFrame.h"
#include "nsInlineFrame.h"
#include "nsMathMLAtoms.h"
#include "nsMathMLOperators.h"
#include "nsMathMLChar.h"
#include "nsMathMLFrame.h"
#include "nsMathMLParts.h"

/*
 * Base class for MathML container frames. It acts like an inferred 
 * mrow. By default, this frame uses its Reflow() method to lay its 
 * children horizontally and ensure that their baselines are aligned.
 * The Reflow() method relies upon Place() to position children.
 * By overloading Place() in derived classes, it is therefore possible
 * to position children in various customized ways.
 */

// Parameters to handle the change of font-size induced by changing the
// scriptlevel. These are hard-coded values that match with the rules in
// mathml.css. Note that mScriptLevel can exceed these bounds, but the
// scaling effect on the font-size will be bounded. The following bounds can
// be expanded provided the new corresponding rules are added in mathml.css.
#define NS_MATHML_CSS_POSITIVE_SCRIPTLEVEL_LIMIT  +5
#define NS_MATHML_CSS_NEGATIVE_SCRIPTLEVEL_LIMIT  -5
#define NS_MATHML_SCRIPTSIZEMULTIPLIER             0.71f
#define NS_MATHML_SCRIPTMINSIZE                    8

// Options for the preferred size at which to stretch our stretchy children 
#define STRETCH_CONSIDER_ACTUAL_SIZE    0x00000001 // just use our current size
#define STRETCH_CONSIDER_EMBELLISHMENTS 0x00000002 // size calculations include embellishments

class nsMathMLContainerFrame : public nsHTMLContainerFrame,
                               public nsMathMLFrame {
public:

  NS_DECL_ISUPPORTS_INHERITED

  // --------------------------------------------------------------------------
  // Overloaded nsMathMLFrame methods -- see documentation in nsIMathMLFrame.h

  NS_IMETHOD
  Stretch(nsIPresContext*      aPresContext,
          nsIRenderingContext& aRenderingContext,
          nsStretchDirection   aStretchDirection,
          nsBoundingMetrics&   aContainerSize,
          nsHTMLReflowMetrics& aDesiredStretchSize);

  NS_IMETHOD
  Place(nsIPresContext*      aPresContext,
        nsIRenderingContext& aRenderingContext,
        PRBool               aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize);

  NS_IMETHOD
  UpdatePresentationDataFromChildAt(nsIPresContext* aPresContext,
                                    PRInt32         aFirstIndex,
                                    PRInt32         aLastIndex,
                                    PRInt32         aScriptLevelIncrement,
                                    PRUint32        aFlagsValues,
                                    PRUint32        aFlagsToUpdate)
  {
    PropagatePresentationDataFromChildAt(aPresContext, this,
      aFirstIndex, aLastIndex, aScriptLevelIncrement, aFlagsValues, aFlagsToUpdate);
    return NS_OK;
  }

  NS_IMETHOD
  ReResolveScriptStyle(nsIPresContext* aPresContext,
                       PRInt32         aParentScriptLevel)
  {
    PropagateScriptStyleFor(aPresContext, this, aParentScriptLevel);
    return NS_OK;
  }

  // --------------------------------------------------------------------------
  // Overloaded nsHTMLContainerFrame methods -- see documentation in nsIFrame.h

  NS_IMETHOD
  GetFrameType(nsIAtom** aType) const;

  NS_IMETHOD
  Init(nsIPresContext*  aPresContext,
       nsIContent*      aContent,
       nsIFrame*        aParent,
       nsIStyleContext* aContext,
       nsIFrame*        aPrevInFlow);

  NS_IMETHOD
  SetInitialChildList(nsIPresContext* aPresContext,
                      nsIAtom*        aListName,
                      nsIFrame*       aChildList);

  NS_IMETHOD
  AppendFrames(nsIPresContext* aPresContext,
               nsIPresShell&   aPresShell,
               nsIAtom*        aListName,
               nsIFrame*       aFrameList);

  NS_IMETHOD
  InsertFrames(nsIPresContext* aPresContext,
               nsIPresShell&   aPresShell,
               nsIAtom*        aListName,
               nsIFrame*       aPrevFrame,
               nsIFrame*       aFrameList);

  NS_IMETHOD
  RemoveFrame(nsIPresContext* aPresContext,
              nsIPresShell&   aPresShell,
              nsIAtom*        aListName,
              nsIFrame*       aOldFrame);

  NS_IMETHOD
  ReplaceFrame(nsIPresContext* aPresContext,
               nsIPresShell&   aPresShell,
               nsIAtom*        aListName,
               nsIFrame*       aOldFrame,
               nsIFrame*       aNewFrame);

  NS_IMETHODIMP
  ReflowDirtyChild(nsIPresShell* aPresShell, 
                   nsIFrame*     aChild);

  NS_IMETHOD
  Reflow(nsIPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus);

  NS_IMETHOD
  DidReflow(nsIPresContext*           aPresContext,
            const nsHTMLReflowState*  aReflowState,
            nsDidReflowStatus         aStatus)

  {
    mPresentationData.flags &= ~NS_MATHML_STRETCH_DONE;
    return nsHTMLContainerFrame::DidReflow(aPresContext, aReflowState, aStatus);
  }

  NS_IMETHOD 
  Paint(nsIPresContext*      aPresContext,
        nsIRenderingContext& aRenderingContext,
        const nsRect&        aDirtyRect,
        nsFramePaintLayer    aWhichLayer,
        PRUint32             aFlags = 0);

  // Notification when an attribute is changed. The MathML module uses the
  // following paradigm:
  //
  // 1. If the MathML frame class doesn't have any cached automatic data that
  //    depends on the attribute: we just reflow (e.g., this happens with <msub>,
  //    <msup>, <mmultiscripts>, etc). This is the default behavior implemented
  //    by this base class.
  //
  // 2. If the MathML frame class has cached automatic data that depends on
  //    the attribute:
  //    2a. If the automatic data to update resides only within the descendants,
  //        we just re-layout them using ReLayoutChildren(aPresContext, this);
  //        (e.g., this happens with <ms>).
  //    2b. If the automatic data to update affects us in some way, we ask our parent
  //        to re-layout its children using ReLayoutChildren(aPresContext, mParent);
  //        Therefore, there is an overhead here in that our siblings are re-laid
  //        too (e.g., this happens with <mstyle>, <munder>, <mover>, <munderover>). 
  NS_IMETHOD
  AttributeChanged(nsIPresContext* aPresContext,
                   nsIContent*     aChild,
                   PRInt32         aNameSpaceID,
                   nsIAtom*        aAttribute,
                   PRInt32         aModType, 
                   PRInt32         aHint);

  // --------------------------------------------------------------------------
  // Additional methods 

  // helper to re-sync the automatic data in our children and notify our parent to
  // reflow us when changes (e.g., append/insert/remove) happen in our child list
  virtual nsresult
  ChildListChanged(nsIPresContext* aPresContext,
                   PRInt32         aModType);

  // helper to wrap non-MathML frames so that foreign elements (e.g., html:img)
  // can mix better with other surrounding MathML markups
  virtual nsresult
  WrapForeignFrames(nsIPresContext* aPresContext);

  // helper to get the preferred size that a container frame should use to fire
  // the stretch on its stretchy child frames.
  virtual void
  GetPreferredStretchSize(nsIPresContext*      aPresContext,
                          nsIRenderingContext& aRenderingContext,
                          PRUint32             aOptions,
                          nsStretchDirection   aStretchDirection,
                          nsBoundingMetrics&   aPreferredStretchSize);

  // error handlers to provide a visual feedback to the user when an error
  // (typically invalid markup) was encountered during reflow.
  virtual nsresult
  ReflowError(nsIPresContext*      aPresContext,
              nsIRenderingContext& aRenderingContext,
              nsHTMLReflowMetrics& aDesiredSize);
  virtual nsresult
  PaintError(nsIPresContext*      aPresContext,
             nsIRenderingContext& aRenderingContext,
             const nsRect&        aDirtyRect,
             nsFramePaintLayer    aWhichLayer);

  // helper function to reflow token elements
  static nsresult
  ReflowTokenFor(nsIFrame*                aFrame,
                 nsIPresContext*          aPresContext,
                 nsHTMLReflowMetrics&     aDesiredSize,
                 const nsHTMLReflowState& aReflowState,
                 nsReflowStatus&          aStatus);

  // helper function to place token elements
  static nsresult
  PlaceTokenFor(nsIFrame*            aFrame,
                nsIPresContext*      aPresContext,
                nsIRenderingContext& aRenderingContext,
                PRBool               aPlaceOrigin,  
                nsHTMLReflowMetrics& aDesiredSize);

  // helper method to reflow a child frame. We are inline frames, and we don't
  // know our positions until reflow is finished. That's why we ask the
  // base method not to worry about our position.
  nsresult 
  ReflowChild(nsIFrame*                aKidFrame,
              nsIPresContext*          aPresContext,
              nsHTMLReflowMetrics&     aDesiredSize,
              const nsHTMLReflowState& aReflowState,
              nsReflowStatus&          aStatus)
  {
    return nsHTMLContainerFrame::ReflowChild(aKidFrame, aPresContext, aDesiredSize, aReflowState,
                                             0, 0, NS_FRAME_NO_MOVE_FRAME, aStatus);
  }

  // helper to add the inter-spacing when <math> is the immediate parent.
  // Since we don't (yet) handle the root <math> element ourselves, we need to
  // take special care of the inter-frame spacing on elements for which <math>
  // is the direct xml parent. This function will be repeatedly called from
  // left to right on the childframes of <math>, and by so doing it will
  // emulate the spacing that would have been done by a <mrow> container.
  // e.g., it fixes <math> <mi>f</mi> <mo>q</mo> <mi>f</mi> <mo>I</mo> </math>
  virtual nsresult
  FixInterFrameSpacing(nsIPresContext*      aPresContext,
                       nsHTMLReflowMetrics& aDesiredSize);

  // helper method to complete the post-reflow hook and ensure that embellished
  // operators don't terminate their Reflow without receiving a Stretch command.
  virtual nsresult
  FinalizeReflow(nsIPresContext*      aPresContext,
                 nsIRenderingContext& aRenderingContext,
                 nsHTMLReflowMetrics& aDesiredSize);

  // helper method to facilitate getting the reflow and bounding metrics
  // IMPORTANT: This function is only meant to be called in Place() methods 
  // where it is assumed that the frame's rect is still acting as place holder
  // for the frame's ascent and descent information
  static void
  GetReflowAndBoundingMetricsFor(nsIFrame*            aFrame,
                                 nsHTMLReflowMetrics& aReflowMetrics,
                                 nsBoundingMetrics&   aBoundingMetrics);

  // helper to let the scriptstyle re-resolution pass through
  // a subtree that may contain non-MathML container frames
  static void
  PropagateScriptStyleFor(nsIPresContext* aPresContext,
                          nsIFrame*       aFrame,
                          PRInt32         aParentScriptLevel);

  // helper to let the update of presentation data pass through
  // a subtree that may contain non-MathML container frames
  static void
  PropagatePresentationDataFor(nsIPresContext* aPresContext,
                               nsIFrame*       aFrame,
                               PRInt32         aScriptLevelIncrement,
                               PRUint32        aFlagsValues,
                               PRUint32        aFlagsToUpdate);

  static void
  PropagatePresentationDataFromChildAt(nsIPresContext* aPresContext,
                                       nsIFrame*       aParentFrame,
                                       PRInt32         aFirstChildIndex,
                                       PRInt32         aLastChildIndex,
                                       PRInt32         aScriptLevelIncrement,
                                       PRUint32        aFlagsValues,
                                       PRUint32        aFlagsToUpdate);

  // helper to let the rebuild of automatic data (presentation data
  // and embellishement data) walk through a subtree that may contain
  // non-MathML container frames. Note that this method re-builds the
  // automatic data in the children -- not in aParentFrame itself (except
  // for those particular operations that the parent frame may do in its
  // TransmitAutomaticData()). The reason it works this way is because
  // a container frame knows what it wants for its children, whereas children
  // have no clue who their parent is. For example, it is <mfrac> who knows
  // that its children have to be in scriptsizes, and has to transmit this
  // information to them. Hence, when changes occur in a child frame, the child
  // has to request the re-build from its parent. Unfortunately, the extra cost
  // for this is that it will re-sync in the siblings of the child as well.
  static void
  RebuildAutomaticDataForChildren(nsIPresContext* aPresContext,
                                  nsIFrame*       aParentFrame);

  // helper to blow away the automatic data cached in a frame's subtree and
  // re-layout its subtree to reflect changes that may have happen. In the
  // event where aParentFrame isn't a MathML frame, it will first walk up to
  // the ancestor that is a MathML frame, and re-layout from there -- this is
  // to guarantee that automatic data will be rebuilt properly. Note that this
  // method re-builds the automatic data in the children -- not in the parent
  // frame itself (except for those particular operations that the parent frame
  // may do do its TransmitAutomaticData()). @see RebuildAutomaticDataForChildren
  static nsresult
  ReLayoutChildren(nsIPresContext* aPresContext,
                   nsIFrame*       aParentFrame);

protected:
  virtual PRIntn GetSkipSides() const { return 0; }
};


// --------------------------------------------------------------------------
// Currently, to benefit from line-breaking inside the <math> element, <math> is
// simply mapping to nsBlockFrame or nsInlineFrame.
// A separate implemention needs to provide:
// 1) line-breaking
// 2) proper inter-frame spacing
// 3) firing of Stretch() (in which case FinalizeReflow() would have to be cleaned)
// Issues: If/when mathml becomes a pluggable component, the separation will be needed.
class nsMathMLmathBlockFrame : public nsBlockFrame {
public:
  friend nsresult NS_NewMathMLmathBlockFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

  // beware, mFrames is not set by nsBlockFrame
  // cannot use mFrames{.FirstChild()|.etc} since the block code doesn't set mFrames
  NS_IMETHOD
  SetInitialChildList(nsIPresContext* aPresContext,
                      nsIAtom*        aListName,
                      nsIFrame*       aChildList)
  {
    nsresult rv = nsBlockFrame::SetInitialChildList(aPresContext, aListName, aChildList);
    // re-resolve our subtree to set any mathml-expected data
    nsMathMLContainerFrame::MapAttributesIntoCSS(aPresContext, this);
    nsMathMLContainerFrame::RebuildAutomaticDataForChildren(aPresContext, this);
    return rv;
  }

  NS_IMETHOD
  Reflow(nsIPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus)
  {
    if (mScriptStyleChanged) {
      mScriptStyleChanged = PR_FALSE;
      nsMathMLContainerFrame::PropagateScriptStyleFor(aPresContext, this, 0);
    }
    return nsBlockFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
  }

  NS_IMETHOD
  AppendFrames(nsIPresContext* aPresContext,
               nsIPresShell&   aPresShell,
               nsIAtom*        aListName,
               nsIFrame*       aFrameList)
  {
    nsresult rv = nsBlockFrame::AppendFrames(aPresContext, aPresShell, aListName, aFrameList);
    nsMathMLContainerFrame::ReLayoutChildren(aPresContext, this);
    return rv;
  }

  NS_IMETHOD
  InsertFrames(nsIPresContext* aPresContext,
               nsIPresShell&   aPresShell,
               nsIAtom*        aListName,
               nsIFrame*       aPrevFrame,
               nsIFrame*       aFrameList)
  {
    nsresult rv = nsBlockFrame::InsertFrames(aPresContext, aPresShell, aListName, aPrevFrame, aFrameList);
    nsMathMLContainerFrame::ReLayoutChildren(aPresContext, this);
    return rv;
  }

  NS_IMETHOD
  ReplaceFrame(nsIPresContext* aPresContext,
               nsIPresShell&   aPresShell,
               nsIAtom*        aListName,
               nsIFrame*       aOldFrame,
               nsIFrame*       aNewFrame)
  {
    nsresult rv = nsBlockFrame::ReplaceFrame(aPresContext, aPresShell, aListName, aOldFrame, aNewFrame);
    nsMathMLContainerFrame::ReLayoutChildren(aPresContext, this);
    return rv;
  }

  NS_IMETHOD
  RemoveFrame(nsIPresContext* aPresContext,
              nsIPresShell&   aPresShell,
              nsIAtom*        aListName,
              nsIFrame*       aOldFrame)
  {
    nsresult rv = nsBlockFrame::RemoveFrame(aPresContext, aPresShell, aListName, aOldFrame);
    nsMathMLContainerFrame::ReLayoutChildren(aPresContext, this);
    return rv;
  }

protected:
  nsMathMLmathBlockFrame() {}
  virtual ~nsMathMLmathBlockFrame() {}

  NS_IMETHOD
  DidSetStyleContext(nsIPresContext* aPresContext)
  {
    mScriptStyleChanged = PR_TRUE;
    return nsBlockFrame::DidSetStyleContext(aPresContext);
  }

  PRBool mScriptStyleChanged;
};

// --------------

class nsMathMLmathInlineFrame : public nsInlineFrame {
public:
  friend nsresult NS_NewMathMLmathInlineFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

  NS_IMETHOD
  SetInitialChildList(nsIPresContext* aPresContext,
                      nsIAtom*        aListName,
                      nsIFrame*       aChildList)
  {
    nsresult rv = nsInlineFrame::SetInitialChildList(aPresContext, aListName, aChildList);
    // re-resolve our subtree to set any mathml-expected data
    nsMathMLContainerFrame::MapAttributesIntoCSS(aPresContext, this);
    nsMathMLContainerFrame::RebuildAutomaticDataForChildren(aPresContext, this);
    return rv;
  }

  NS_IMETHOD
  Reflow(nsIPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus)
  {
    if (mScriptStyleChanged) {
      mScriptStyleChanged = PR_FALSE;
      nsMathMLContainerFrame::PropagateScriptStyleFor(aPresContext, this, 0);
    }
    return nsInlineFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
  }

  NS_IMETHOD
  AppendFrames(nsIPresContext* aPresContext,
               nsIPresShell&   aPresShell,
               nsIAtom*        aListName,
               nsIFrame*       aFrameList)
  {
    nsresult rv = nsInlineFrame::AppendFrames(aPresContext, aPresShell, aListName, aFrameList);
    nsMathMLContainerFrame::ReLayoutChildren(aPresContext, this);
    return rv;
  }

  NS_IMETHOD
  InsertFrames(nsIPresContext* aPresContext,
               nsIPresShell&   aPresShell,
               nsIAtom*        aListName,
               nsIFrame*       aPrevFrame,
               nsIFrame*       aFrameList)
  {
    nsresult rv = nsInlineFrame::InsertFrames(aPresContext, aPresShell, aListName, aPrevFrame, aFrameList);
    nsMathMLContainerFrame::ReLayoutChildren(aPresContext, this);
    return rv;
  }

  NS_IMETHOD
  ReplaceFrame(nsIPresContext* aPresContext,
               nsIPresShell&   aPresShell,
               nsIAtom*        aListName,
               nsIFrame*       aOldFrame,
               nsIFrame*       aNewFrame)
  {
    nsresult rv = nsInlineFrame::ReplaceFrame(aPresContext, aPresShell, aListName, aOldFrame, aNewFrame);
    nsMathMLContainerFrame::ReLayoutChildren(aPresContext, this);
    return rv;
  }

  NS_IMETHOD
  RemoveFrame(nsIPresContext* aPresContext,
              nsIPresShell&   aPresShell,
              nsIAtom*        aListName,
              nsIFrame*       aOldFrame)
  {
    nsresult rv = nsInlineFrame::RemoveFrame(aPresContext, aPresShell, aListName, aOldFrame);
    nsMathMLContainerFrame::ReLayoutChildren(aPresContext, this);
    return rv;
  }

protected:
  nsMathMLmathInlineFrame() {}
  virtual ~nsMathMLmathInlineFrame() {}

  NS_IMETHOD
  DidSetStyleContext(nsIPresContext* aPresContext)
  {
    mScriptStyleChanged = PR_TRUE;
    return nsInlineFrame::DidSetStyleContext(aPresContext);
  }

  PRBool mScriptStyleChanged;
};

#endif /* nsMathMLContainerFrame_h___ */
