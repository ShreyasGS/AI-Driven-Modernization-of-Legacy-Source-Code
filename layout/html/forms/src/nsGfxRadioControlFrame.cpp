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

#include "nsGfxRadioControlFrame.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLParts.h"
#include "nsFormFrame.h"
#include "nsIFormControl.h"
#include "nsIContent.h"
#include "nsWidgetsCID.h"
#include "nsIComponentManager.h"
#include "nsCOMPtr.h"
#include "nsCSSRendering.h"
#include "nsIPresState.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "nsINameSpaceManager.h"
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#endif
#include "nsIServiceManager.h"
#include "nsIDOMNode.h"


nsresult
NS_NewGfxRadioControlFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsGfxRadioControlFrame* it = new (aPresShell) nsGfxRadioControlFrame;
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}

nsGfxRadioControlFrame::nsGfxRadioControlFrame()
{
   // Initialize GFX-rendered state
  mRadioButtonFaceStyle = nsnull;
}

nsGfxRadioControlFrame::~nsGfxRadioControlFrame()
{
  NS_IF_RELEASE(mRadioButtonFaceStyle);
}

// Frames are not refcounted, no need to AddRef
NS_IMETHODIMP
nsGfxRadioControlFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(0 != aInstancePtr, "null ptr");
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(NS_GET_IID(nsIRadioControlFrame))) {
    *aInstancePtr = (void*) ((nsIRadioControlFrame*) this);
    return NS_OK;
  }

  return nsFormControlFrame::QueryInterface(aIID, aInstancePtr);
}

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsGfxRadioControlFrame::GetAccessible(nsIAccessible** aAccessible)
{
  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");

  if (accService) {
    return accService->CreateHTMLRadioButtonAccessible(NS_STATIC_CAST(nsIFrame*, this), aAccessible);
  }

  return NS_ERROR_FAILURE;
}
#endif

//--------------------------------------------------------------
NS_IMETHODIMP
nsGfxRadioControlFrame::GetAdditionalStyleContext(PRInt32 aIndex, 
                                                  nsIStyleContext** aStyleContext) const
{
  NS_PRECONDITION(nsnull != aStyleContext, "null OUT parameter pointer");
  if (aIndex < 0) {
    return NS_ERROR_INVALID_ARG;
  }
  *aStyleContext = nsnull;
  switch (aIndex) {
  case NS_GFX_RADIO_CONTROL_FRAME_FACE_CONTEXT_INDEX:
    *aStyleContext = mRadioButtonFaceStyle;
    NS_IF_ADDREF(*aStyleContext);
    break;
  default:
    return NS_ERROR_INVALID_ARG;
  }
  return NS_OK;
}

//--------------------------------------------------------------
NS_IMETHODIMP
nsGfxRadioControlFrame::SetAdditionalStyleContext(PRInt32 aIndex, 
                                                  nsIStyleContext* aStyleContext)
{
  if (aIndex < 0) {
    return NS_ERROR_INVALID_ARG;
  }
  switch (aIndex) {
  case NS_GFX_RADIO_CONTROL_FRAME_FACE_CONTEXT_INDEX:
    NS_IF_RELEASE(mRadioButtonFaceStyle);
    mRadioButtonFaceStyle = aStyleContext;
    NS_IF_ADDREF(aStyleContext);
    break;
  }
  return NS_OK;
}

//--------------------------------------------------------------
NS_IMETHODIMP
nsGfxRadioControlFrame::SetRadioButtonFaceStyleContext(nsIStyleContext *aRadioButtonFaceStyleContext)
{
  mRadioButtonFaceStyle = aRadioButtonFaceStyleContext;
  NS_ADDREF(mRadioButtonFaceStyle);
  return NS_OK;
}

//--------------------------------------------------------------
NS_IMETHODIMP
nsGfxRadioControlFrame::HandleEvent(nsIPresContext* aPresContext, 
                                          nsGUIEvent* aEvent,
                                          nsEventStatus* aEventStatus)
{
  // Check for user-input:none style
  const nsStyleUserInterface* uiStyle;
  GetStyleData(eStyleStruct_UserInterface,  (const nsStyleStruct *&)uiStyle);
  if (uiStyle->mUserInput == NS_STYLE_USER_INPUT_NONE || uiStyle->mUserInput == NS_STYLE_USER_INPUT_DISABLED)
    return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);

  // otherwise, do nothing. Events are handled by the DOM.
  return NS_OK;
}

//--------------------------------------------------------------
void
nsGfxRadioControlFrame::PaintRadioButton(nsIPresContext* aPresContext,
                                      nsIRenderingContext& aRenderingContext,
                                      const nsRect& aDirtyRect)
{
   
  PRBool checked = PR_TRUE;
  GetCurrentCheckState(&checked); // Get check state from the content model
  if (checked) {
   // Paint the button for the radio button using CSS background rendering code
   if (nsnull != mRadioButtonFaceStyle) {
     const nsStyleBackground* myColor = (const nsStyleBackground*)
          mRadioButtonFaceStyle->GetStyleData(eStyleStruct_Background);

     const nsStyleColor* color = (const nsStyleColor*)
          mRadioButtonFaceStyle->GetStyleData(eStyleStruct_Color);

     const nsStyleBorder* myBorder = (const nsStyleBorder*)
          mRadioButtonFaceStyle->GetStyleData(eStyleStruct_Border);
     const nsStylePosition* myPosition = (const nsStylePosition*)
          mRadioButtonFaceStyle->GetStyleData(eStyleStruct_Position);

     nscoord width = myPosition->mWidth.GetCoordValue();
     nscoord height = myPosition->mHeight.GetCoordValue();
       // Position the button centered within the radio control's rectangle.
     nscoord x = (mRect.width - width) / 2;
     nscoord y = (mRect.height - height) / 2;
     nsRect rect(x, y, width, height); 

     // So we will use the PaintBackground to paint the dot, 
     // but it uses the mBackgroundColor for painting and we need to use the mColor
     // so create a temporary style color struct and set it up appropriately
     // XXXldb It would make more sense to use
     // |aRenderingContext.FillEllipse| here, but on at least GTK that
     // doesn't draw a round enough circle.
     nsStyleBackground tmpColor     = *myColor;
     tmpColor.mBackgroundColor = color->mColor;
     nsCSSRendering::PaintBackgroundWithSC(aPresContext, aRenderingContext,
                                           this, aDirtyRect, rect,
                                           tmpColor, *myBorder, 0, 0);
     nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
                                  aDirtyRect, rect, *myBorder, mRadioButtonFaceStyle, 0);
   }
  }
}

//--------------------------------------------------------------
NS_METHOD 
nsGfxRadioControlFrame::Paint(nsIPresContext*   aPresContext,
                           nsIRenderingContext& aRenderingContext,
                           const nsRect&        aDirtyRect,
                           nsFramePaintLayer    aWhichLayer,
                           PRUint32             aFlags)
{
  PRBool isVisible;
  if (NS_SUCCEEDED(IsVisibleForPainting(aPresContext, aRenderingContext, PR_TRUE, &isVisible)) && !isVisible) {
    return NS_OK;
  }
     // Paint the background
  nsresult rv = nsFormControlFrame::Paint(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);

  if (NS_FRAME_PAINT_LAYER_FOREGROUND == aWhichLayer) {
    PaintRadioButton(aPresContext, aRenderingContext, aDirtyRect);
  }
  return rv;
}


//--------------------------------------------------------------
NS_IMETHODIMP
nsGfxRadioControlFrame::OnChecked(nsIPresContext* aPresContext,
                                  PRBool aChecked)
{
  nsFormControlHelper::ForceDrawFrame(aPresContext, this);
  return NS_OK;
}


//----------------------------------------------------------------------
// Extra Debug Helper Methods
//----------------------------------------------------------------------
#ifdef DEBUG_rodsXXX
NS_IMETHODIMP 
nsGfxRadioControlFrame::Reflow(nsIPresContext*          aPresContext, 
                               nsHTMLReflowMetrics&     aDesiredSize,
                               const nsHTMLReflowState& aReflowState, 
                               nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsGfxRadioControlFrame", aReflowState.reason);
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  nsresult rv = nsNativeFormControlFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);

  COMPARE_QUIRK_SIZE("nsGfxRadioControlFrame", 12, 11) 
  return rv;
}
#endif

NS_IMETHODIMP
nsGfxRadioControlFrame::OnContentReset()
{
  return NS_OK;
}
