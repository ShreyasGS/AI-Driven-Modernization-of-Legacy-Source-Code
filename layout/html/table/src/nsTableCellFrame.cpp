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
 *   Pierre Phaneuf <pp@ludusdesign.com>
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
#include "nsTableFrame.h"
#include "nsTableColFrame.h"
#include "nsTableCellFrame.h"
#include "nsTableFrame.h"
#include "nsTableRowGroupFrame.h"
#include "nsHTMLReflowCommand.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsIRenderingContext.h"
#include "nsCSSRendering.h"
#include "nsIContent.h"
#include "nsIHTMLContent.h"
#include "nsHTMLIIDs.h"
#include "nsHTMLParts.h"
#include "nsHTMLValue.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsVoidArray.h"
#include "nsIView.h"
#include "nsStyleUtil.h"
#include "nsLayoutAtoms.h"
#include "nsIFrameManager.h"
#include "nsIPresShell.h"
#include "nsCOMPtr.h"
#include "nsIHTMLTableCellElement.h"
#include "nsIDOMHTMLTableCellElement.h"
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#endif
#include "nsIServiceManager.h"
#include "nsIDOMNode.h"
#include "nsINameSpaceManager.h"


//TABLECELL SELECTION
#include "nsIFrameSelection.h"
#include "nsILookAndFeel.h"


nsTableCellFrame::nsTableCellFrame()
{
  mBits.mColIndex  = 0;
  mPriorAvailWidth = 0;

  SetContentEmpty(PR_FALSE);
  SetNeedSpecialReflow(PR_FALSE);
  SetHadSpecialReflow(PR_FALSE);
  SetHasPctOverHeight(PR_FALSE);

#ifdef DEBUG_TABLE_REFLOW_TIMING
  mTimer = new nsReflowTimer(this);
  mBlockTimer = new nsReflowTimer(this);
#endif
}

nsTableCellFrame::~nsTableCellFrame()
{
#ifdef DEBUG_TABLE_REFLOW_TIMING
  nsTableFrame::DebugReflowDone(this);
#endif
}

nsTableCellFrame*  
nsTableCellFrame::GetNextCell() const
{
  nsIFrame* childFrame;
  GetNextSibling(&childFrame);
  while (childFrame) {
    nsCOMPtr<nsIAtom> frameType;
    childFrame->GetFrameType(getter_AddRefs(frameType));
    if (IS_TABLE_CELL(frameType.get())) {
      return (nsTableCellFrame*)childFrame;
    }
    childFrame->GetNextSibling(&childFrame);
  }
  return nsnull;
}

NS_IMETHODIMP
nsTableCellFrame::Init(nsIPresContext*  aPresContext,
                       nsIContent*      aContent,
                       nsIFrame*        aParent,
                       nsIStyleContext* aContext,
                       nsIFrame*        aPrevInFlow)
{
  nsresult  rv;
  
  // Let the base class do its initialization
  rv = nsHTMLContainerFrame::Init(aPresContext, aContent, aParent, aContext,
                                  aPrevInFlow);

  if (aPrevInFlow) {
    // Set the column index
    nsTableCellFrame* cellFrame = (nsTableCellFrame*)aPrevInFlow;
    PRInt32           colIndex;
    cellFrame->GetColIndex(colIndex);
    InitCellFrame(colIndex);
  }

  return rv;
}
static PRBool
NoComputedHeightBetween(const nsHTMLReflowState&  aReflowState,
                        nsIFrame*                 aCellFrame)
{
  for (const nsHTMLReflowState* rs = aReflowState.parentReflowState; rs; rs = rs->parentReflowState) {
    if ((NS_UNCONSTRAINEDSIZE != rs->mComputedHeight) || (0 != rs->mComputedHeight)) {
      return PR_FALSE;
    }
    // stop when we hit the cell frame
    if (rs->frame == aCellFrame) {
      return PR_TRUE;
    }
  }
  NS_ASSERTION(PR_FALSE, "program error in NoComputedHeightBetween");
  return PR_FALSE;
}

// nsIPercentHeightObserver methods

void
nsTableCellFrame::NotifyPercentHeight(const nsHTMLReflowState& aReflowState)
{
  if (!NeedSpecialReflow()) {
    // Only initiate a special reflow if we will be able to construct a computed height 
    // on the cell that will result in the frame getting a computed height. This can only 
    // happen (but not sufficient) if there is no computed height already set between the 
    // initiating frame and the cell.
    for (const nsHTMLReflowState* rs = aReflowState.parentReflowState; rs; rs = rs->parentReflowState) {
      if ((NS_UNCONSTRAINEDSIZE != rs->mComputedHeight) && (0 != rs->mComputedHeight)) {
        return;
      }
      // stop when we reach the cell frame
      if (rs->frame == this) {
        nsTableFrame::RequestSpecialHeightReflow(*rs);
        return;
      }
    }
    NS_ASSERTION(PR_FALSE, "program error in NotifyPercentHeight");
  }
}

// The cell needs to observe its block and things inside its block but nothing below that
PRBool 
nsTableCellFrame::NeedsToObserve(const nsHTMLReflowState& aReflowState)
{
  PRBool result = PR_FALSE;
  const nsHTMLReflowState* parentRS = aReflowState.parentReflowState;
  if (parentRS && (parentRS->mPercentHeightObserver == this)) { // cell observes the parent
    result = PR_TRUE;
    parentRS = parentRS->parentReflowState;
    if (parentRS && (parentRS->mPercentHeightObserver == this)) { // cell observers the grand parent
      parentRS = parentRS->parentReflowState;
      if (parentRS && (parentRS->mPercentHeightObserver == this)) { 
        // cell observes the great grand parent, so we have gone too deep
        result = PR_FALSE;
      }
    }
  }
  return result;
}

nsresult 
nsTableCellFrame::GetRowIndex(PRInt32 &aRowIndex) const
{
  nsresult result;
  nsTableRowFrame * row;
  GetParent((nsIFrame **)&row);
  if (row) {
    aRowIndex = row->GetRowIndex();
    result = NS_OK;
  }
  else {
    aRowIndex = 0;
    result = NS_ERROR_NOT_INITIALIZED;
  }
  return result;
}

nsresult 
nsTableCellFrame::GetColIndex(PRInt32 &aColIndex) const
{  
  if (mPrevInFlow) {
    return ((nsTableCellFrame*)GetFirstInFlow())->GetColIndex(aColIndex);
  }
  else {
    aColIndex = mBits.mColIndex;
    return  NS_OK;
  }
}

NS_IMETHODIMP
nsTableCellFrame::AttributeChanged(nsIPresContext* aPresContext,
                                   nsIContent*     aChild,
                                   PRInt32         aNameSpaceID,
                                   nsIAtom*        aAttribute,
                                   PRInt32         aModType, 
                                   PRInt32         aHint)
{
  // let the table frame decide what to do
  nsTableFrame* tableFrame = nsnull; 
  nsresult rv = nsTableFrame::GetTableFrame(this, tableFrame);
  if ((NS_SUCCEEDED(rv)) && (tableFrame)) {
    tableFrame->AttributeChangedFor(aPresContext, this, aChild, aAttribute); 
  }
  return NS_OK;
}

void nsTableCellFrame::SetPass1MaxElementWidth(nscoord aMaxWidth,
                                               nscoord aMaxElementWidth)
{ 
  nscoord maxElemWidth = aMaxElementWidth;
  const nsStylePosition* stylePosition;
  const nsStyleText* styleText;
  // check for fixed width and not nowrap and not pre
  GetStyleData(eStyleStruct_Position, ((const nsStyleStruct *&)stylePosition));
  GetStyleData(eStyleStruct_Text, ((const nsStyleStruct *&)styleText));
  if (stylePosition->mWidth.GetUnit() == eStyleUnit_Coord &&
      styleText->mWhiteSpace != NS_STYLE_WHITESPACE_NOWRAP &&
      styleText->mWhiteSpace != NS_STYLE_WHITESPACE_PRE) {
    // has fixed width, check the content for nowrap
    nsAutoString nowrap;
    nsCOMPtr<nsIContent> cellContent;
    GetContent(getter_AddRefs(cellContent));
    nsresult result = cellContent->GetAttr(kNameSpaceID_None, nsHTMLAtoms::nowrap, nowrap);
    if(NS_CONTENT_ATTR_NOT_THERE != result) {
      // content has nowrap (is not mapped to style be cause it has width)
      // set the max element size to the value of the fixed width (NAV/IE quirk)
      maxElemWidth = NS_MAX(maxElemWidth, stylePosition->mWidth.GetCoordValue());
    }
  }
  mPass1MaxElementWidth = maxElemWidth;
}

NS_IMETHODIMP
nsTableCellFrame::AppendFrames(nsIPresContext* aPresContext,
                               nsIPresShell&   aPresShell,
                               nsIAtom*        aListName,
                               nsIFrame*       aFrameList)
{
  NS_PRECONDITION(PR_FALSE, "unsupported operation");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsTableCellFrame::InsertFrames(nsIPresContext* aPresContext,
                               nsIPresShell&   aPresShell,
                               nsIAtom*        aListName,
                               nsIFrame*       aPrevFrame,
                               nsIFrame*       aFrameList)
{
  NS_PRECONDITION(PR_FALSE, "unsupported operation");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsTableCellFrame::RemoveFrame(nsIPresContext* aPresContext,
                              nsIPresShell&   aPresShell,
                              nsIAtom*        aListName,
                              nsIFrame*       aOldFrame)
{
  NS_PRECONDITION(PR_FALSE, "unsupported operation");
  return NS_ERROR_NOT_IMPLEMENTED;
}

void nsTableCellFrame::InitCellFrame(PRInt32 aColIndex)
{
  nsTableFrame* tableFrame=nsnull;  // I should be checking my own style context, but border-collapse isn't inheriting correctly
  nsresult rv = nsTableFrame::GetTableFrame(this, tableFrame);
  if ((NS_SUCCEEDED(rv)) && tableFrame) {
    SetColIndex(aColIndex);
  }
}

nsresult nsTableCellFrame::SetColIndex(PRInt32 aColIndex)
{  
  mBits.mColIndex = aColIndex;
  // for style context optimization, set the content's column index if possible.
  // this can only be done if we really have an nsTableCell.  
  // other tags mapped to table cell display won't benefit from this optimization
  // see nsHTMLStyleSheet::RulesMatching

  //nsIContent* cell;
  //kidFrame->GetContent(&cell);
  nsCOMPtr<nsIContent> cell;
  nsresult rv = GetContent(getter_AddRefs(cell));
  if (NS_FAILED(rv) || !cell)
    return rv;

  nsIHTMLTableCellElement* cellContent = nsnull;
  rv = cell->QueryInterface(NS_GET_IID(nsIHTMLTableCellElement), 
                            (void **)&cellContent);  // cellContent: REFCNT++
  if (cellContent && NS_SUCCEEDED(rv)) { // it's a table cell
    cellContent->SetColIndex(aColIndex);
    NS_RELEASE(cellContent);
  }
  return rv;
}


//ASSURE DIFFERENT COLORS for selection
inline nscolor EnsureDifferentColors(nscolor colorA, nscolor colorB)
{
    if (colorA == colorB)
    {
      nscolor res;
      res = NS_RGB(NS_GET_R(colorA) ^ 0xff,
                   NS_GET_G(colorA) ^ 0xff,
                   NS_GET_B(colorA) ^ 0xff);
      return res;
    }
    return colorA;
}


nsresult
nsTableCellFrame::DecorateForSelection(nsIPresContext* aPresContext,
                                       nsIRenderingContext& aRenderingContext,
                                       const nsStyleBackground *aStyleColor)
{
  PRInt16 displaySelection;
  displaySelection = DisplaySelection(aPresContext);
  if (displaySelection) {
    nsFrameState  frameState;
    PRBool        isSelected;
    GetFrameState(&frameState);
    isSelected = (frameState & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT;
    if (isSelected) {
      nsCOMPtr<nsIPresShell> shell;
      nsresult result = aPresContext->GetShell(getter_AddRefs(shell));
      if (NS_FAILED(result))
        return result;
      nsCOMPtr<nsIFrameSelection> frameSelection;
      result = shell->GetFrameSelection(getter_AddRefs(frameSelection));
      if (NS_SUCCEEDED(result)) {
        PRBool tableCellSelectionMode;
        result = frameSelection->GetTableCellSelection(&tableCellSelectionMode);
        if (NS_SUCCEEDED(result) && tableCellSelectionMode) {
          nscolor       bordercolor;
          if(displaySelection == nsISelectionController::SELECTION_DISABLED) {
            bordercolor = NS_RGB(176,176,176);// disabled color
          }
          else {
  	        nsILookAndFeel* look = nsnull;
	          if (NS_SUCCEEDED(aPresContext->GetLookAndFeel(&look)) && look) {
	            look->GetColor(nsILookAndFeel::eColor_TextSelectBackground, bordercolor);
	            NS_RELEASE(look);
	          }
          }
          float t2pfloat;
          if (NS_SUCCEEDED(aPresContext->GetPixelsToTwips(&t2pfloat)))
          {
            PRInt16 t2p = (PRInt16)t2pfloat;
            if ((mRect.width >(3*t2p)) && (mRect.height > (3*t2p)))
            {
              //compare bordercolor to ((nsStyleColor *)myColor)->mBackgroundColor)
              bordercolor = EnsureDifferentColors(bordercolor, aStyleColor->mBackgroundColor);
              //outerrounded
              aRenderingContext.SetColor(bordercolor);
              aRenderingContext.DrawLine(t2p, 0, mRect.width, 0);
              aRenderingContext.DrawLine(0, t2p, 0, mRect.height);
              aRenderingContext.DrawLine(t2p, mRect.height, mRect.width, mRect.height);
              aRenderingContext.DrawLine(mRect.width, t2p, mRect.width, mRect.height);
              //middle
              aRenderingContext.DrawRect(t2p, t2p, mRect.width-t2p, mRect.height-t2p);
              //shading
              aRenderingContext.DrawLine(2*t2p, mRect.height-2*t2p, mRect.width-t2p, mRect.height- (2*t2p));
              aRenderingContext.DrawLine(mRect.width - (2*t2p), 2*t2p, mRect.width - (2*t2p), mRect.height-t2p);
            }
          }
        }
      }
    }
  }
  return NS_OK;
}

void
nsTableCellFrame::PaintUnderlay(nsIPresContext&           aPresContext,
                                nsIRenderingContext&      aRenderingContext,
                                const nsRect&             aDirtyRect,
                                PRUint32&                 aFlags,
                                const nsStyleTableBorder& aCellTableStyle,
                                const nsStyleBorder&      aStyleBorder,
                                PRBool                    aVisibleBackground,
                                PRBool&                   aPaintChildren)
{
  if (aVisibleBackground) {
    nsRect rect(0, 0, mRect.width, mRect.height);
    nsCSSRendering::PaintBackground(&aPresContext, aRenderingContext, this,
                                    aDirtyRect, rect, aStyleBorder, 0, 0, PR_TRUE);
    // draw the border only when there is content or showing empty cells
    if (!GetContentEmpty() || NS_STYLE_TABLE_EMPTY_CELLS_SHOW == aCellTableStyle.mEmptyCells) {
      PRIntn skipSides = GetSkipSides();
      nsCSSRendering::PaintBorder(&aPresContext, aRenderingContext, this,
                                  aDirtyRect, rect, aStyleBorder, mStyleContext, skipSides);
    }
  }

  // tell Paint to paint the children
  aPaintChildren = PR_TRUE;
}

NS_METHOD 
nsTableCellFrame::Paint(nsIPresContext*      aPresContext,
                        nsIRenderingContext& aRenderingContext,
                        const nsRect&        aDirtyRect,
                        nsFramePaintLayer    aWhichLayer,
                        PRUint32             aFlags)
{
  NS_ENSURE_TRUE(aPresContext, NS_ERROR_NULL_POINTER);
  PRBool isVisible;
  if (NS_SUCCEEDED(IsVisibleForPainting(aPresContext, aRenderingContext, PR_FALSE, &isVisible)) && !isVisible) {
    return NS_OK;
  }

  PRBool paintChildren   = PR_TRUE;

  if (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer) {
    PRBool paintBackground = PR_FALSE;
    const nsStyleBorder*      myBorder       = nsnull;
    const nsStyleTableBorder* cellTableStyle = nsnull;
    const nsStyleVisibility* vis = 
       (const nsStyleVisibility*)mStyleContext->GetStyleData(eStyleStruct_Visibility);
    if (vis->IsVisibleOrCollapsed()) {
      myBorder = (const nsStyleBorder*)mStyleContext->GetStyleData(eStyleStruct_Border); NS_ENSURE_TRUE(myBorder, NS_ERROR_NULL_POINTER);

      GetStyleData(eStyleStruct_TableBorder, ((const nsStyleStruct *&)cellTableStyle));

      // paint the background when the cell is not empty or when showing empty cells or background
      paintBackground = (!GetContentEmpty()                                             ||
                         NS_STYLE_TABLE_EMPTY_CELLS_SHOW == cellTableStyle->mEmptyCells || 
                         NS_STYLE_TABLE_EMPTY_CELLS_SHOW_BACKGROUND == cellTableStyle->mEmptyCells);
    }
  
    PaintUnderlay(*aPresContext, aRenderingContext, aDirtyRect, aFlags, *cellTableStyle,
                  *myBorder, paintBackground, paintChildren);

    if (vis->IsVisibleOrCollapsed()) {
      const nsStyleBackground* myColor = 
        (const nsStyleBackground*)mStyleContext->GetStyleData(eStyleStruct_Background); NS_ENSURE_TRUE(myColor, NS_ERROR_NULL_POINTER);
      DecorateForSelection(aPresContext, aRenderingContext,myColor); //ignore return value
    }
  }

#ifdef DEBUG
  // for debug...
  if ((NS_FRAME_PAINT_LAYER_DEBUG == aWhichLayer) && GetShowFrameBorders()) {
    aRenderingContext.SetColor(NS_RGB(0, 0, 128));
    aRenderingContext.DrawRect(0, 0, mRect.width, mRect.height);
  }
#endif

  // paint the children unless we've been told not to
  if (paintChildren) {
    const nsStyleDisplay* disp =
      (const nsStyleDisplay*)mStyleContext->GetStyleData(eStyleStruct_Display);
    // if the cell originates in a row and/or col that is collapsed, the
    // bottom and/or right portion of the cell is painted by translating
    // the rendering context.
    PRBool clipState;
    nsPoint offset;
    GetCollapseOffset(aPresContext, offset);
    if ((0 != offset.x) || (0 != offset.y)) {
      aRenderingContext.PushState();
      aRenderingContext.Translate(offset.x, offset.y);
      aRenderingContext.SetClipRect(nsRect(-offset.x, -offset.y, mRect.width, mRect.height),
                                    nsClipCombine_kIntersect, clipState);
    }
    else {
      if ((NS_STYLE_OVERFLOW_HIDDEN == disp->mOverflow) || HasPctOverHeight()) {
        aRenderingContext.PushState();
        SetOverflowClipRect(aRenderingContext);
      }    
    }

    PaintChildren(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer, aFlags);

    if ((0 != offset.x) || (0 != offset.y)) {
      aRenderingContext.PopState(clipState);
    }
    else { 
      if ((NS_STYLE_OVERFLOW_HIDDEN == disp->mOverflow) || HasPctOverHeight()) {
        aRenderingContext.PopState(clipState);         
      }
    }
  } 
  
  DO_GLOBAL_REFLOW_COUNT_DSP_J("nsTableCellFrame", &aRenderingContext, 0);
  return NS_OK;
  /*nsFrame::Paint(aPresContext,
                        aRenderingContext,
                        aDirtyRect,
                        aWhichLayer);*/
}

NS_IMETHODIMP
nsTableCellFrame::GetFrameForPoint(nsIPresContext*   aPresContext,
                                   const nsPoint&    aPoint, 
                                   nsFramePaintLayer aWhichLayer,
                                   nsIFrame**        aFrame)
{
  // this should act like a block, so we need to override
  return GetFrameForPointUsing(aPresContext, aPoint, nsnull, aWhichLayer, (aWhichLayer == NS_FRAME_PAINT_LAYER_BACKGROUND), aFrame);
}

//null range means the whole thing
NS_IMETHODIMP
nsTableCellFrame::SetSelected(nsIPresContext* aPresContext,
                              nsIDOMRange*    aRange,
                              PRBool          aSelected,
                              nsSpread        aSpread)
{
  //traverse through children unselect tables
#if 0
  if ((aSpread == eSpreadDown)){
    nsIFrame* kid;
    FirstChild(nsnull, &kid);
    while (nsnull != kid) {
      kid->SetSelected(nsnull,aSelected,eSpreadDown);

      kid->GetNextSibling(&kid);
    }
  }
  //return nsFrame::SetSelected(aRange,aSelected,eSpreadNone);
#endif
  // Must call base class to set mSelected state and trigger repaint of frame
  // Note that in current version, aRange and aSpread are ignored,
  //   only this frame is considered
  nsFrame::SetSelected(aPresContext, aRange, aSelected, aSpread);

  nsCOMPtr<nsIPresShell> shell;
  nsresult result = aPresContext->GetShell(getter_AddRefs(shell));
  if (NS_FAILED(result))
    return result;
  nsCOMPtr<nsIFrameSelection> frameSelection;
  result = shell->GetFrameSelection(getter_AddRefs(frameSelection));
  if (NS_SUCCEEDED(result) && frameSelection) {
    PRBool tableCellSelectionMode;
    result = frameSelection->GetTableCellSelection(&tableCellSelectionMode);
    if (NS_SUCCEEDED(result) && tableCellSelectionMode) {
      nsRect frameRect;
      GetRect(frameRect);
      nsRect rect(0, 0, frameRect.width, frameRect.height);
      Invalidate(aPresContext, rect, PR_FALSE);
    }
  }
  return NS_OK;
}

PRIntn
nsTableCellFrame::GetSkipSides() const
{
  PRIntn skip = 0;
  if (nsnull != mPrevInFlow) {
    skip |= 1 << NS_SIDE_TOP;
  }
  if (nsnull != mNextInFlow) {
    skip |= 1 << NS_SIDE_BOTTOM;
  }
  return skip;
}

PRBool nsTableCellFrame::ParentDisablesSelection() const //override default behavior
{
  PRBool returnval;
  if (NS_FAILED(GetSelected(&returnval)))
    return PR_FALSE;
  if (returnval)
    return PR_TRUE;
  return nsFrame::ParentDisablesSelection();
}


// Align the cell's child frame within the cell

void nsTableCellFrame::VerticallyAlignChild(nsIPresContext*          aPresContext,
                                            const nsHTMLReflowState& aReflowState,
                                            nscoord                  aMaxAscent)
{
  const nsStyleTextReset* textStyle =
      (const nsStyleTextReset*)mStyleContext->GetStyleData(eStyleStruct_TextReset);
  /* XXX: remove tableFrame when border-collapse inherits */
  GET_PIXELS_TO_TWIPS(aPresContext, p2t);
  nsMargin borderPadding;
  GetBorderWidth (p2t, borderPadding);
  nsMargin padding = nsTableFrame::GetPadding(aReflowState, this);
  borderPadding += padding;
  
  nscoord topInset = borderPadding.top;
  nscoord bottomInset = borderPadding.bottom;

  // As per bug 10207, we map 'sub', 'super', 'text-top', 'text-bottom', 
  // length and percentage values to 'baseline'
  // XXX It seems that we don't get to see length and percentage values here
  //     because the Style System has already fixed the error and mapped them
  //     to whatever is inherited from the parent, i.e, 'middle' in most cases.
  PRUint8 verticalAlignFlags = NS_STYLE_VERTICAL_ALIGN_BASELINE;
  if (textStyle->mVerticalAlign.GetUnit() == eStyleUnit_Enumerated) {
    verticalAlignFlags = textStyle->mVerticalAlign.GetIntValue();
    if (verticalAlignFlags != NS_STYLE_VERTICAL_ALIGN_TOP &&
        verticalAlignFlags != NS_STYLE_VERTICAL_ALIGN_MIDDLE &&
        verticalAlignFlags != NS_STYLE_VERTICAL_ALIGN_BOTTOM)
    {
      verticalAlignFlags = NS_STYLE_VERTICAL_ALIGN_BASELINE;
    }
  }

  nscoord height = mRect.height;
  nsRect kidRect;  
  nsIFrame* firstKid = mFrames.FirstChild();
  firstKid->GetRect(kidRect);
  nscoord childHeight = kidRect.height;
  

  // Vertically align the child
  nscoord kidYTop = 0;
  switch (verticalAlignFlags) 
  {
    case NS_STYLE_VERTICAL_ALIGN_BASELINE:
      // Align the baselines of the child frame with the baselines of 
      // other children in the same row which have 'vertical-align: baseline'
      kidYTop = topInset + aMaxAscent - GetDesiredAscent();
    break;

    case NS_STYLE_VERTICAL_ALIGN_TOP:
      // Align the top of the child frame with the top of the content area, 
      kidYTop = topInset;
    break;

    case NS_STYLE_VERTICAL_ALIGN_BOTTOM:
      // Align the bottom of the child frame with the bottom of the content area, 
      kidYTop = height - childHeight - bottomInset;
    break;

    default:
    case NS_STYLE_VERTICAL_ALIGN_MIDDLE:
      // Align the middle of the child frame with the middle of the content area, 
      kidYTop = (height - childHeight - bottomInset + topInset) / 2;
      float p2t;
      aPresContext->GetScaledPixelsToTwips(&p2t);
      kidYTop = nsTableFrame::RoundToPixel(kidYTop, p2t, eAlwaysRoundDown);
  }
  firstKid->MoveTo(aPresContext, kidRect.x, kidYTop);
  if (kidYTop != kidRect.y) {
    // Make sure any child views are correctly positioned. We know the inner table
    // cell won't have a view
    nsContainerFrame::PositionChildViews(aPresContext, firstKid);
  }
}

// As per bug 10207, we map 'sub', 'super', 'text-top', 'text-bottom', 
// length and percentage values to 'baseline'
// XXX It seems that we don't get to see length and percentage values here
//     because the Style System has already fixed the error and mapped them
//     to whatever is inherited from the parent, i.e, 'middle' in most cases.
PRBool
nsTableCellFrame::HasVerticalAlignBaseline()
{
  const nsStyleTextReset* textStyle;
  GetStyleData(eStyleStruct_TextReset, (const nsStyleStruct*&)textStyle);
  if (textStyle->mVerticalAlign.GetUnit() == eStyleUnit_Enumerated) {
    PRUint8 verticalAlignFlags = textStyle->mVerticalAlign.GetIntValue();
    if (verticalAlignFlags == NS_STYLE_VERTICAL_ALIGN_TOP ||
        verticalAlignFlags == NS_STYLE_VERTICAL_ALIGN_MIDDLE ||
        verticalAlignFlags == NS_STYLE_VERTICAL_ALIGN_BOTTOM)
    {
      return PR_FALSE;
    }
  }
  return PR_TRUE;
}

PRInt32 nsTableCellFrame::GetRowSpan()
{  
  PRInt32 rowSpan=1;
  nsIHTMLContent *hc=nsnull;
  nsresult rv = mContent->QueryInterface(kIHTMLContentIID, (void**) &hc);
  if (NS_OK==rv) {
    nsHTMLValue val;
    hc->GetHTMLAttribute(nsHTMLAtoms::rowspan, val); 
    if (eHTMLUnit_Integer == val.GetUnit()) { 
       rowSpan=val.GetIntValue(); 
    }
    NS_RELEASE(hc);
  }
  return rowSpan;
}

PRInt32 nsTableCellFrame::GetColSpan()
{  
  PRInt32 colSpan=1;
  nsIHTMLContent *hc=nsnull;
  nsresult rv = mContent->QueryInterface(kIHTMLContentIID, (void**) &hc);
  if (NS_OK==rv) {
    nsHTMLValue val;
    hc->GetHTMLAttribute(nsHTMLAtoms::colspan, val); 
    if (eHTMLUnit_Integer == val.GetUnit()) { 
       colSpan=val.GetIntValue(); 
    }
    NS_RELEASE(hc);
  }
  return colSpan;
}

#define PROBABLY_TOO_LARGE 1000000
static
void DebugCheckChildSize(nsIFrame*            aChild, 
                         nsHTMLReflowMetrics& aMet, 
                         nsSize&              aAvailSize,
                         PRBool               aIsPass2Reflow)
{
  if (aIsPass2Reflow) {
    if ((aMet.width < 0) || (aMet.width > PROBABLY_TOO_LARGE)) {
      printf("WARNING: cell content %p has large width %d \n", aChild, aMet.width);
    }
  }
  if (aMet.maxElementSize) {
    nscoord tmp = aMet.maxElementSize->width;
    if ((tmp < 0) || (tmp > PROBABLY_TOO_LARGE)) {
      printf("WARNING: cell content %p has large max element width %d \n", aChild, tmp);
    }
  }
}

// the computed height for the cell, which descendents use for percent height calculations
// it is the height (minus border, padding) of the cell's first in flow during its final 
// reflow without an unconstrained height.
static nscoord
CalcUnpaginagedHeight(nsIPresContext*       aPresContext,
                      nsTableCellFrame&     aCellFrame, 
                      nsTableFrame&         aTableFrame,
                      nscoord               aVerticalBorderPadding)
{
  const nsTableCellFrame* firstCellInFlow   = (nsTableCellFrame*)aCellFrame.GetFirstInFlow();
  nsTableFrame*           firstTableInFlow  = (nsTableFrame*)aTableFrame.GetFirstInFlow();
  nsTableRowGroupFrame*   firstRGInFlow;
  nsTableRowFrame*        row;
  firstCellInFlow->GetParent((nsIFrame**)&row);
  row->GetParent((nsIFrame**)&firstRGInFlow);

  PRInt32 rowIndex;
  firstCellInFlow->GetRowIndex(rowIndex);
  PRInt32 rowSpan = aTableFrame.GetEffectiveRowSpan(*firstCellInFlow);
  nscoord cellSpacing = firstTableInFlow->GetCellSpacingX();

  nscoord computedHeight = ((rowSpan - 1) * cellSpacing) - aVerticalBorderPadding;
  PRInt32 rowX;
  for (row = firstRGInFlow->GetFirstRow(), rowX = 0; row; row = row->GetNextRow(), rowX++) {
    if (rowX > rowIndex + rowSpan - 1) {
      break;
    }
    else if (rowX >= rowIndex) {
      computedHeight += row->GetUnpaginatedHeight(aPresContext);
    }
  }
  return computedHeight;
}

NS_METHOD nsTableCellFrame::Reflow(nsIPresContext*          aPresContext,
                                   nsHTMLReflowMetrics&     aDesiredSize,
                                   const nsHTMLReflowState& aReflowState,
                                   nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsTableCellFrame", aReflowState.reason);
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
#if defined DEBUG_TABLE_REFLOW_TIMING
  nsTableFrame::DebugReflow(this, (nsHTMLReflowState&)aReflowState);
#endif
  float p2t;
  aPresContext->GetScaledPixelsToTwips(&p2t);

  // work around pixel rounding errors, round down to ensure we don't exceed the avail height in
  nscoord availHeight = aReflowState.availableHeight;
  if (NS_UNCONSTRAINEDSIZE != availHeight) {
    availHeight = nsTableFrame::RoundToPixel(availHeight, p2t, eAlwaysRoundDown);
  }

  nsresult rv = NS_OK;

  // see if a special height reflow needs to occur due to having a pct height
  if (!NeedSpecialReflow()) 
    nsTableFrame::CheckRequestSpecialHeightReflow(aReflowState);

  // this should probably be cached somewhere
  nsCompatibility compatMode;
  aPresContext->GetCompatibilityMode(&compatMode);

  // Initialize out parameter
  if (nsnull != aDesiredSize.maxElementSize) {
    aDesiredSize.maxElementSize->width = 0;
    aDesiredSize.maxElementSize->height = 0;
  }

  aStatus = NS_FRAME_COMPLETE;
  nsSize availSize(aReflowState.availableWidth, availHeight);
  nsSize maxElementSize;
  nsSize* pMaxElementSize = aDesiredSize.maxElementSize;
  if (NS_UNCONSTRAINEDSIZE==aReflowState.availableWidth)
    pMaxElementSize = &maxElementSize;

  PRBool contentEmptyBeforeReflow = GetContentEmpty();
  /* XXX: remove tableFrame when border-collapse inherits */
  nsTableFrame* tableFrame = nsnull;
  rv = nsTableFrame::GetTableFrame(this, tableFrame); if (!tableFrame) ABORT1(NS_ERROR_NULL_POINTER);
  nsTableFrame* tableFrameFirstInFlow = (nsTableFrame*)tableFrame->GetFirstInFlow();

  nsMargin borderPadding = aReflowState.mComputedPadding;
  nsMargin border;
  GetBorderWidth(p2t, border);
  if ((NS_UNCONSTRAINEDSIZE == availSize.width) || !contentEmptyBeforeReflow) {
    borderPadding += border;
  }
  
  nscoord topInset    = borderPadding.top;
  nscoord rightInset  = borderPadding.right;
  nscoord bottomInset = borderPadding.bottom;
  nscoord leftInset   = borderPadding.left;

  // reduce available space by insets, if we're in a constrained situation
  if (NS_UNCONSTRAINEDSIZE!=availSize.width)
    availSize.width -= leftInset+rightInset;
  if (NS_UNCONSTRAINEDSIZE!=availSize.height)
    availSize.height -= topInset+bottomInset;

  PRBool  isStyleChanged = PR_FALSE;
  if (eReflowReason_Incremental == aReflowState.reason) {
    // We *must* do this otherwise incremental reflow that's
    // passing through will not work right.
    nsIFrame* next;
    aReflowState.reflowCommand->GetNext(next);

    // if it is a StyleChanged reflow targeted at this cell frame,
    // handle that here
    // first determine if this frame is the target or not
    nsIFrame *target=nsnull;
    rv = aReflowState.reflowCommand->GetTarget(target);
    if ((PR_TRUE==NS_SUCCEEDED(rv)) && target) {
      if (this == target) {
        nsReflowType type;
        aReflowState.reflowCommand->GetType(type);
        if (eReflowType_StyleChanged == type) {
          isStyleChanged = PR_TRUE;
        }
        else {
          NS_ASSERTION(PR_FALSE, "table cell target of illegal incremental reflow type");
        }
      }
    }
    // if any of these conditions are not true, we just pass the reflow command down
  }

  // Try to reflow the child into the available space. It might not
  // fit or might need continuing.
  if (availSize.height < 0)
    availSize.height = 1;

  nsHTMLReflowMetrics kidSize(pMaxElementSize, aDesiredSize.mFlags);
  kidSize.width=kidSize.height=kidSize.ascent=kidSize.descent=0;
  SetPriorAvailWidth(aReflowState.availableWidth);
  nsIFrame* firstKid = mFrames.FirstChild();

  PRBool isPaginated;
  aPresContext->IsPaginated(&isPaginated);
  nscoord computedPaginatedHeight = 0;

  if (aReflowState.mFlags.mSpecialHeightReflow || 
      (HadSpecialReflow() && (eReflowReason_Incremental == aReflowState.reason))) {
    ((nsHTMLReflowState&)aReflowState).mComputedHeight = mRect.height - topInset - bottomInset;
    DISPLAY_REFLOW_CHANGE();
  }
  else if (isPaginated) {
    computedPaginatedHeight = CalcUnpaginagedHeight(aPresContext, (nsTableCellFrame&)*this, *tableFrame, topInset + bottomInset);
    if (computedPaginatedHeight > 0) {
      ((nsHTMLReflowState&)aReflowState).mComputedHeight = computedPaginatedHeight;
      DISPLAY_REFLOW_CHANGE();
    }
  }      
  else {
    SetHasPctOverHeight(PR_FALSE);
  }
  nsHTMLReflowState kidReflowState(aPresContext, aReflowState, firstKid, availSize);
  // mIPercentHeightObserver is for non table related frames inside cells
  kidReflowState.mPercentHeightObserver = (nsIPercentHeightObserver *)this;

  // If it was a style change targeted at us, then reflow the child using
  // the special reflow reason
  if (isStyleChanged) {
    kidReflowState.reason = eReflowReason_StyleChange;
    kidReflowState.reflowCommand = nsnull;
    // the following could be optimized with a fair amount of effort
    tableFrame->SetNeedStrategyInit(PR_TRUE);
  }

  // Assume the inner child will stay positioned exactly where it is. Later in
  // VerticallyAlignChild() we'll move it if it turns out to be wrong. This
  // avoids excessive movement and is more stable
  nsPoint kidOrigin;
  if (isStyleChanged || 
      (eReflowReason_Initial == aReflowState.reason) ||
      (eReflowReason_StyleChange == aReflowState.reason)) {
    kidOrigin.MoveTo(leftInset, topInset);
  } else {
    // handle percent padding-left which was 0 during initial reflow
    if (eStyleUnit_Percent == aReflowState.mStylePadding->mPadding.GetLeftUnit()) {
      nsRect kidRect;
      firstKid->GetRect(kidRect);
      // only move in the x direction for the same reason as above
      kidOrigin.MoveTo(leftInset, kidRect.y);
      firstKid->MoveTo(aPresContext, leftInset, kidRect.y);
    }
    firstKid->GetOrigin(kidOrigin);
  }

#if defined DEBUG_TABLE_REFLOW_TIMING
  nsTableFrame::DebugReflow(firstKid, (nsHTMLReflowState&)kidReflowState);
#endif
  nscoord priorBlockHeight = GetLastBlockHeight();
  ReflowChild(firstKid, aPresContext, kidSize, kidReflowState,
              kidOrigin.x, kidOrigin.y, 0, aStatus);
  SetLastBlockHeight(kidSize.height);
  if (isStyleChanged) {
    Invalidate(aPresContext, mRect);
  }

#if defined DEBUG_TABLE_REFLOW_TIMING
  nsTableFrame::DebugReflow(firstKid, (nsHTMLReflowState&)kidReflowState, &kidSize, aStatus);
#endif

#ifdef NS_DEBUG
  DebugCheckChildSize(firstKid, kidSize, availSize, (NS_UNCONSTRAINEDSIZE != aReflowState.availableWidth));
#endif

  // 0 dimensioned cells need to be treated specially in Standard/NavQuirks mode 
  // see testcase "emptyCells.html"
  if ((0 == kidSize.width) || (0 == kidSize.height)) { // XXX why was this &&
    SetContentEmpty(PR_TRUE);
    if (NS_UNCONSTRAINEDSIZE == kidReflowState.availableWidth) {
      // need to reduce the insets by border if the cell is empty
      leftInset   -= border.left;
      rightInset  -= border.right;
      topInset    -= border.top;
      bottomInset -= border.bottom;
    }
  }
  else {
    SetContentEmpty(PR_FALSE);
    if ((eReflowReason_Incremental == aReflowState.reason) && contentEmptyBeforeReflow) {
      // need to consider borders, since they were factored out above
      leftInset   += border.left;
      rightInset  += border.right;
      topInset    += border.top;
      bottomInset += border.bottom;
      kidOrigin.MoveTo(leftInset, topInset);
    }
  }

  const nsStylePosition* pos;
  GetStyleData(eStyleStruct_Position, ((const nsStyleStruct *&)pos));

  // calculate the min cell width
  nscoord onePixel = NSIntPixelsToTwips(1, p2t); 
  nscoord smallestMinWidth = 0;
  if (eCompatibility_NavQuirks == compatMode) {
    if ((pos->mWidth.GetUnit() != eStyleUnit_Coord)   &&
        (pos->mWidth.GetUnit() != eStyleUnit_Percent)) {
      if (PR_TRUE == GetContentEmpty()) {
        if (border.left > 0) 
          smallestMinWidth += onePixel;
        if (border.right > 0) 
          smallestMinWidth += onePixel;
      }
    }
  }
  PRInt32 colspan = tableFrame->GetEffectiveColSpan(*this);
  if (colspan > 1) {
    smallestMinWidth = PR_MAX(smallestMinWidth, colspan * onePixel);
    nscoord spacingX = tableFrame->GetCellSpacingX();
    nscoord spacingExtra = spacingX * (colspan - 1);
    smallestMinWidth += spacingExtra;
    if (aReflowState.mComputedPadding.left > 0) {
      smallestMinWidth -= onePixel;
    }
  }
 
  if ((0 == kidSize.width) && (NS_UNCONSTRAINEDSIZE != kidReflowState.availableWidth)) {
    // empty content has to be forced to the assigned width for resize or incremental reflow
    kidSize.width = kidReflowState.availableWidth;
  }
  if (0 == kidSize.height) {
    if ((pos->mHeight.GetUnit() != eStyleUnit_Coord) &&
        (pos->mHeight.GetUnit() != eStyleUnit_Percent)) {
      // Standard mode should probably be 0 pixels high instead of 1
      PRInt32 pixHeight = (eCompatibility_Standard == compatMode) ? 1 : 2;
      kidSize.height = NSIntPixelsToTwips(pixHeight, p2t);
      if ((nsnull != aDesiredSize.maxElementSize) && (0 == pMaxElementSize->height))
        pMaxElementSize->height = kidSize.height;
    }
  }
  // end 0 dimensioned cells

  kidSize.width = PR_MAX(kidSize.width, smallestMinWidth); 
  if (!tableFrame->IsAutoLayout()) {
    // a cell in a fixed layout table is constrained to the avail width
    kidSize.width = PR_MIN(kidSize.width, availSize.width);
  }
  //if (eReflowReason_Resize == aReflowState.reason) {
  //  NS_ASSERTION(kidSize.width <= availSize.width, "child needed more space during resize reflow");
  //}
  // Place the child
  FinishReflowChild(firstKid, aPresContext, &kidReflowState, kidSize,
                    kidOrigin.x, kidOrigin.y, 0);
    
  // first, compute the height which can be set w/o being restricted by aMaxSize.height
  nscoord cellHeight = kidSize.height;

  if (NS_UNCONSTRAINEDSIZE != cellHeight) {
    cellHeight += topInset + bottomInset;
    // work around block rounding errors, round down to ensure we don't exceed the avail height in
    nsPixelRound roundMethod = (NS_UNCONSTRAINEDSIZE == availHeight) ? eAlwaysRoundUp : eAlwaysRoundDown;
    cellHeight = nsTableFrame::RoundToPixel(cellHeight, p2t, roundMethod); 
  }

  // next determine the cell's width
  nscoord cellWidth = kidSize.width;      // at this point, we've factored in the cell's style attributes

  // factor in border and padding
  if (NS_UNCONSTRAINEDSIZE != cellWidth) {
    cellWidth += leftInset + rightInset;    
  }
  cellWidth = nsTableFrame::RoundToPixel(cellWidth, p2t); // work around block rounding errors

  // set the cell's desired size and max element size
  aDesiredSize.width   = cellWidth;
  aDesiredSize.height  = cellHeight;
  aDesiredSize.ascent  = topInset;
  aDesiredSize.descent = bottomInset;

  aDesiredSize.ascent  += kidSize.ascent;
  aDesiredSize.descent += kidSize.descent;

  if (aDesiredSize.mFlags & NS_REFLOW_CALC_MAX_WIDTH) {
    aDesiredSize.mMaximumWidth = kidSize.mMaximumWidth;
    if (NS_UNCONSTRAINEDSIZE != aDesiredSize.mMaximumWidth) {
      aDesiredSize.mMaximumWidth += leftInset + rightInset;
      aDesiredSize.mMaximumWidth = nsTableFrame::RoundToPixel(aDesiredSize.mMaximumWidth, p2t);
    }
  }
  if (aDesiredSize.maxElementSize) {
    *aDesiredSize.maxElementSize = *pMaxElementSize;
    if ((0 != pMaxElementSize->height) && (NS_UNCONSTRAINEDSIZE != pMaxElementSize->height)) {
      aDesiredSize.maxElementSize->height += topInset + bottomInset;
      aDesiredSize.maxElementSize->height = nsTableFrame::RoundToPixel(aDesiredSize.maxElementSize->height, p2t);
    }
    aDesiredSize.maxElementSize->width = PR_MAX(smallestMinWidth, aDesiredSize.maxElementSize->width); 
    if (NS_UNCONSTRAINEDSIZE != aDesiredSize.maxElementSize->width) {
      aDesiredSize.maxElementSize->width += leftInset + rightInset;
      aDesiredSize.maxElementSize->width = nsTableFrame::RoundToPixel(aDesiredSize.maxElementSize->width, p2t);
    }
  }

  if (aReflowState.mFlags.mSpecialHeightReflow) {
    if (aDesiredSize.height > mRect.height) {
      // set a bit indicating that the pct height contents exceeded 
      // the height that they could honor in the pass 2 reflow
      SetHasPctOverHeight(PR_TRUE);
    }
    if (NS_UNCONSTRAINEDSIZE == aReflowState.availableHeight) {
      aDesiredSize.height = mRect.height;
    }
    SetNeedSpecialReflow(PR_FALSE);
    SetHadSpecialReflow(PR_TRUE);
  }
  else if (HadSpecialReflow()) {
    if ((eReflowReason_Incremental == aReflowState.reason) || 
        (eReflowReason_Resize == aReflowState.reason)) {
      // with an unconstrained height, if the block height value hasn't changed, 
      // use the last height of the cell.
      if ((NS_UNCONSTRAINEDSIZE == aReflowState.availableHeight) && 
          (GetLastBlockHeight() == priorBlockHeight)) {
        aDesiredSize.height = mRect.height;
      }
    }
    // XXX should probably call SetHadSpecialReflow(PR_FALSE) when things change so that
    // nothing inside the cell has a percent height, but it is not easy determining this 
  }

  // remember the desired size for this reflow
  SetDesiredSize(aDesiredSize);

#if defined DEBUG_TABLE_REFLOW_TIMING
  nsTableFrame::DebugReflow(this, (nsHTMLReflowState&)aReflowState, &aDesiredSize, aStatus);
#endif

  return NS_OK;
}

PRBool nsTableCellFrame::ConvertToPixelValue(nsHTMLValue& aValue, PRInt32 aDefault, PRInt32& aResult)
{
  if (aValue.GetUnit() == eHTMLUnit_Pixel)
    aResult = aValue.GetPixelValue();
  else if (aValue.GetUnit() == eHTMLUnit_Empty)
    aResult = aDefault;
  else {
    NS_ERROR("Unit must be pixel or empty");
    return PR_FALSE;
  }
  return PR_TRUE;
}

void nsTableCellFrame::MapBorderPadding(nsIPresContext* aPresContext)
{
  // Check to see if the table has cell padding or defined for the table. If true, 
  // then this setting overrides any specific border or padding information in the 
  // cell. If these attributes are not defined, the the cells attributes are used

  nsTableFrame* tableFrame;
  nsTableFrame::GetTableFrame(this, tableFrame);
  NS_ASSERTION(tableFrame,"Table must not be null");
  if (!tableFrame)
    return;

  // get the table frame style context, and from it get cellpadding, cellspacing, and border info
  const nsStyleTable* tableStyle;
  tableFrame->GetStyleData(eStyleStruct_Table, (const nsStyleStruct *&)tableStyle);

  MapVAlignAttribute(aPresContext, tableFrame);
  MapHAlignAttribute(aPresContext, tableFrame);
  
}

/* XXX: this code will not work properly until the style and layout code has been updated 
 * as outlined in Bugzilla bug report 1802 and 915 */
void nsTableCellFrame::MapVAlignAttribute(nsIPresContext* aPresContext, nsTableFrame *aTableFrame)
{
#if 0
  const nsStyleTextReset* textStyle;
  GetStyleData(eStyleStruct_TextReset,(const nsStyleStruct *&)textStyle);
  // check if valign is set on the cell
  // this condition will also be true if we inherited valign from the row or rowgroup
  if (textStyle->mVerticalAlign.GetUnit() == eStyleUnit_Enumerated) {
    return; // valign is already set on this cell
  }

  // check if valign is set on the cell's COL (or COLGROUP by inheritance)
  PRInt32 colIndex;
  GetColIndex(colIndex);
  nsTableColFrame* colFrame = aTableFrame->GetColFrame(colIndex);
  if (colFrame) {
    const nsStyleTextReset* colTextStyle;
    colFrame->GetStyleData(eStyleStruct_TextReset,(const nsStyleStruct *&)colTextStyle);
    if (colTextStyle->mVerticalAlign.GetUnit() == eStyleUnit_Enumerated) {
      nsStyleTextReset* cellTextStyle = (nsStyleTextReset*)mStyleContext->GetMutableStyleData(eStyleStruct_TextReset);
      cellTextStyle->mVerticalAlign.SetIntValue(colTextStyle->mVerticalAlign.GetIntValue(), eStyleUnit_Enumerated);
      return; // valign set from COL info
    }
  }

  // otherwise, set the vertical align attribute to the HTML default
  nsStyleTextReset* cellTextStyle = (nsStyleTextReset*)mStyleContext->GetMutableStyleData(eStyleStruct_TextReset);
  cellTextStyle->mVerticalAlign.SetIntValue(NS_STYLE_VERTICAL_ALIGN_MIDDLE, eStyleUnit_Enumerated);
#endif
}

/* XXX: this code will not work properly until the style and layout code has been updated 
 * as outlined in Bugzilla bug report 1802 and 915.
 * In particular, mTextAlign has to be an nsStyleCoord, not just an int */
void nsTableCellFrame::MapHAlignAttribute(nsIPresContext* aPresContext, 
                                          nsTableFrame*   aTableFrame)
{
#if 0
  const nsStyleText* textStyle;
  GetStyleData(eStyleStruct_Text,(const nsStyleStruct *&)textStyle);
  // check if halign is set on the cell
  // cells do not inherited halign from the row or rowgroup
  if (NS_STYLE_TEXT_ALIGN_DEFAULT != textStyle->mTextAlign) {
    return; // text align is already set on this cell
  }

  // check if halign is set on the cell's ROW (or ROWGROUP by inheritance)
  nsIFrame* rowFrame;
  GetParent(&rowFrame);
  const nsStyleText* rowTextStyle;
  rowFrame->GetStyleData(eStyleStruct_Text,(const nsStyleStruct *&)rowTextStyle);
  if (NS_STYLE_TEXT_ALIGN_DEFAULT != rowTextStyle->mTextAlign) {
    nsStyleText* cellTextStyle = (nsStyleText*)mStyleContext->GetMutableStyleData(eStyleStruct_Text);
    cellTextStyle->mTextAlign = rowTextStyle->mTextAlign;
    return; // halign set from ROW info
  }

  // check if halign is set on the cell's COL (or COLGROUP by inheritance)
  PRInt32 colIndex;
  GetColIndex(colIndex);
  nsTableColFrame* colFrame = aTableFrame->GetColFrame(colIndex);
  if (colFrame) {
    const nsStyleText* colTextStyle;
    colFrame->GetStyleData(eStyleStruct_Text,(const nsStyleStruct *&)colTextStyle);
    if (NS_STYLE_TEXT_ALIGN_DEFAULT != colTextStyle->mTextAlign) {
      nsStyleText* cellTextStyle = (nsStyleText*)mStyleContext->GetMutableStyleData(eStyleStruct_Text);
      cellTextStyle->mTextAlign = colTextStyle->mTextAlign;
      return; // halign set from COL info
    }
  }

  // otherwise, set the text align to the HTML default (center for TH, left for TD)
  nsStyleText* cellTextStyle = (nsStyleText*)mStyleContext->GetMutableStyleData(eStyleStruct_Text);
  nsIAtom* tag;
  if (mContent) {
    mContent->GetTag(tag);
    if (tag) {
      cellTextStyle->mTextAlign = (nsHTMLAtoms::th == tag)
                                  ? NS_STYLE_TEXT_ALIGN_CENTER 
                                  : NS_STYLE_TEXT_ALIGN_LEFT;
    }
    NS_IF_RELEASE(tag);
  }
#endif
}  

/* ----- global methods ----- */

NS_IMPL_ADDREF_INHERITED(nsTableCellFrame, nsHTMLContainerFrame)
NS_IMPL_RELEASE_INHERITED(nsTableCellFrame, nsHTMLContainerFrame)

nsresult nsTableCellFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  if (aIID.Equals(NS_GET_IID(nsITableCellLayout))) {
    *aInstancePtr = (void*) (nsITableCellLayout *)this;
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIPercentHeightObserver))) {
    *aInstancePtr = (void*) (nsIPercentHeightObserver *)this;
    return NS_OK;
  }

  return nsHTMLContainerFrame::QueryInterface(aIID, aInstancePtr);
}

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsTableCellFrame::GetAccessible(nsIAccessible** aAccessible)
{
  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");

  if (accService) {
    nsIAccessible* acc = nsnull;
    return accService->CreateHTMLTableCellAccessible(NS_STATIC_CAST(nsIFrame*, this), aAccessible);
  }

  return NS_ERROR_FAILURE;
}
#endif

/* This is primarily for editor access via nsITableLayout */
NS_IMETHODIMP
nsTableCellFrame::GetCellIndexes(PRInt32 &aRowIndex, PRInt32 &aColIndex)
{
  nsresult res = GetRowIndex(aRowIndex);
  if (NS_FAILED(res))
  {
    aColIndex = 0;
    return res;
  }
  aColIndex = mBits.mColIndex;
  return  NS_OK;
}

NS_IMETHODIMP
nsTableCellFrame::GetPreviousCellInColumn(nsITableCellLayout **aCellLayout)
{
  if (!aCellLayout) return NS_ERROR_NULL_POINTER;
  *aCellLayout = nsnull;

  nsTableFrame* tableFrame = nsnull; 
  nsresult rv = nsTableFrame::GetTableFrame(this, tableFrame);
  if (NS_FAILED(rv)) return rv;
  if (!tableFrame) return NS_ERROR_FAILURE;

  // Get current cell location
  PRInt32 rowIndex, colIndex;
  GetCellIndexes(rowIndex, colIndex);
  if (colIndex > 0)
  {
    // Get the cellframe at previous colIndex
    nsTableCellFrame *cellFrame = tableFrame->GetCellFrameAt(rowIndex, colIndex-1);
    if (cellFrame)
      cellFrame->QueryInterface(NS_GET_IID(nsITableCellLayout), (void **)aCellLayout);  
  }

  return NS_OK;
}

NS_IMETHODIMP
nsTableCellFrame::GetNextCellInColumn(nsITableCellLayout **aCellLayout)
{
  if (!aCellLayout) return NS_ERROR_NULL_POINTER;
  *aCellLayout = nsnull;

  nsTableFrame* tableFrame = nsnull; 
  nsresult rv = nsTableFrame::GetTableFrame(this, tableFrame);
  if (NS_FAILED(rv)) return rv;
  if (!tableFrame) return NS_ERROR_FAILURE;

  // Get current cell location
  PRInt32 rowIndex, colIndex;
  GetCellIndexes(rowIndex, colIndex);

  // Get the cellframe at next colIndex
  nsTableCellFrame *cellFrame = tableFrame->GetCellFrameAt(rowIndex, colIndex+1);
  if (cellFrame)
    cellFrame->QueryInterface(NS_GET_IID(nsITableCellLayout), (void **)aCellLayout);  

  return NS_OK;
}

nsresult 
NS_NewTableCellFrame(nsIPresShell* aPresShell, 
                     PRBool        aIsBorderCollapse,
                     nsIFrame**    aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsTableCellFrame* it = (aIsBorderCollapse) ? new (aPresShell) nsBCTableCellFrame 
                                             : new (aPresShell) nsTableCellFrame;
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}

nsMargin* 
nsTableCellFrame::GetBorderWidth(float      aPixelsToTwips,
                                 nsMargin&  aBorder) const
{
  aBorder.left = aBorder.right = aBorder.top = aBorder.bottom = 0;

  const nsStyleBorder* borderData;
  GetStyleData(eStyleStruct_Border, (const nsStyleStruct*&)borderData);
  borderData->GetBorder(aBorder);
  return &aBorder;
}

NS_IMETHODIMP
nsTableCellFrame::GetFrameType(nsIAtom** aType) const
{
  NS_PRECONDITION(nsnull != aType, "null OUT parameter pointer");
  *aType = nsLayoutAtoms::tableCellFrame; 
  NS_ADDREF(*aType);
  return NS_OK;
}

#ifdef DEBUG
NS_IMETHODIMP
nsTableCellFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("TableCell"), aResult);
}
#endif

void nsTableCellFrame::SetCollapseOffsetX(nsIPresContext* aPresContext,
                                          nscoord         aXOffset)
{
  // Get the frame property (creating a point struct if necessary)
  nsPoint* offset = (nsPoint*)nsTableFrame::GetProperty(aPresContext, this, nsLayoutAtoms::collapseOffsetProperty, PR_TRUE);

  if (offset) {
    offset->x = aXOffset;
  }
}

void nsTableCellFrame::SetCollapseOffsetY(nsIPresContext* aPresContext,
                                          nscoord         aYOffset)
{
  // Get the property (creating a point struct if necessary)
  nsPoint* offset = (nsPoint*)nsTableFrame::GetProperty(aPresContext, this, nsLayoutAtoms::collapseOffsetProperty, PR_TRUE);

  if (offset) {
    offset->y = aYOffset;
  }
}

void nsTableCellFrame::GetCollapseOffset(nsIPresContext* aPresContext,
                                         nsPoint&        aOffset)
{
  // See if the property is set
  nsPoint* offset = (nsPoint*)nsTableFrame::GetProperty(aPresContext, this, nsLayoutAtoms::collapseOffsetProperty);

  if (offset) {
    aOffset = *offset;
  } else {
    aOffset.MoveTo(0, 0);
  }
}

#ifdef DEBUG
NS_IMETHODIMP
nsTableCellFrame::SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const
{
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  PRUint32 sum = sizeof(*this);
  *aResult = sum;
  return NS_OK;
}
#endif

// nsBCTableCellFrame

nsBCTableCellFrame::nsBCTableCellFrame()
:nsTableCellFrame()
{
  mTopBorder = mRightBorder = mBottomBorder = mLeftBorder = 0;
}

nsBCTableCellFrame::~nsBCTableCellFrame()
{
}

NS_IMETHODIMP
nsBCTableCellFrame::GetFrameType(nsIAtom** aType) const
{
  NS_PRECONDITION(nsnull != aType, "null OUT parameter pointer");
  *aType = nsLayoutAtoms::bcTableCellFrame; 
  NS_ADDREF(*aType);
  return NS_OK;
}

#ifdef DEBUG
NS_IMETHODIMP
nsBCTableCellFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("BCTableCell"), aResult);
}
#endif

void 
nsBCTableCellFrame::SetBorderWidth(const nsMargin& aBorder)
{
  mTopBorder    = aBorder.top;
  mRightBorder  = aBorder.right;
  mBottomBorder = aBorder.bottom;
  mLeftBorder   = aBorder.left;
}

nsMargin* 
nsBCTableCellFrame::GetBorderWidth(float      aPixelsToTwips,
                                   nsMargin&  aBorder) const
{
  aBorder.top    = (aPixelsToTwips) ? NSToCoordRound(aPixelsToTwips * mTopBorder) :   mTopBorder;
  aBorder.right  = (aPixelsToTwips) ? NSToCoordRound(aPixelsToTwips * mRightBorder) : mRightBorder;
  aBorder.bottom = (aPixelsToTwips) ? NSToCoordRound(aPixelsToTwips * mBottomBorder): mBottomBorder;
  aBorder.left   = (aPixelsToTwips) ? NSToCoordRound(aPixelsToTwips * mLeftBorder):   mLeftBorder;
  return &aBorder;
}

nscoord 
nsBCTableCellFrame::GetBorderWidth(PRUint8 aSide) const
{
  switch(aSide) {
  case NS_SIDE_TOP:
    return (PRUint8)mTopBorder;
  case NS_SIDE_RIGHT:
    return (PRUint8)mRightBorder;
  case NS_SIDE_BOTTOM:
    return (PRUint8)mBottomBorder;
  default:
    return (PRUint8)mLeftBorder;
  }
}

void 
nsBCTableCellFrame::SetBorderWidth(PRUint8 aSide,
                                   nscoord aValue)
{
  switch(aSide) {
  case NS_SIDE_TOP:
    mTopBorder = aValue;
    break;
  case NS_SIDE_RIGHT:
    mRightBorder = aValue;
    break;
  case NS_SIDE_BOTTOM:
    mBottomBorder = aValue;
    break;
  default:
    mLeftBorder = aValue;
  }
}

void
nsBCTableCellFrame::PaintUnderlay(nsIPresContext&           aPresContext,
                                  nsIRenderingContext&      aRenderingContext,
                                  const nsRect&             aDirtyRect,
                                  PRUint32&                 aFlags,
                                  const nsStyleTableBorder& aCellTableStyle,
                                  const nsStyleBorder&      aStyleBorder,
                                  PRBool                    aVisibleBackground,
                                  PRBool&                   aPaintChildren)
{
  // Draw the background only during pass1.
  if (aVisibleBackground && !(aFlags & BORDER_COLLAPSE_BACKGROUNDS)) { 
    // make border-width reflect border-collapse assigned border
    GET_PIXELS_TO_TWIPS(&aPresContext, p2t);
    nsMargin borderWidth;
    GetBorderWidth(p2t, borderWidth);

    nsStyleBorder myBorder = aStyleBorder;

    nsStyleCoord coord(borderWidth.top);
    myBorder.mBorder.SetTop(coord);
    coord.SetCoordValue(borderWidth.right);
    myBorder.mBorder.SetRight(coord);
    coord.SetCoordValue(borderWidth.bottom);
    myBorder.mBorder.SetBottom(coord);
    coord.SetCoordValue(borderWidth.left);
    myBorder.mBorder.SetLeft(coord);
    myBorder.RecalcData();

    nsRect rect(0, 0, mRect.width, mRect.height);
    nsCSSRendering::PaintBackground(&aPresContext, aRenderingContext, this,
                                    aDirtyRect, rect, myBorder, 0, 0, PR_TRUE);
    // borders are painted by nsTableFrame
  }

  // don't paint the children if it's pass1
  aPaintChildren = (aFlags & BORDER_COLLAPSE_BACKGROUNDS);  
}

#ifdef DEBUG
NS_IMETHODIMP
nsBCTableCellFrame::SizeOf(nsISizeOfHandler* aHandler, 
                           PRUint32*         aResult) const
{
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  PRUint32 sum = sizeof(*this);
  *aResult = sum;
  return NS_OK;
}
#endif
