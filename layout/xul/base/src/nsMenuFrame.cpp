/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 *   Michael Lowe <michael.lowe@bigfoot.com>
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   Dean Tessman <dean_tessman@hotmail.com>
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

#include "nsXULAtoms.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLParts.h"
#include "nsMenuFrame.h"
#include "nsBoxFrame.h"
#include "nsIContent.h"
#include "prtypes.h"
#include "nsIAtom.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIStyleContext.h"
#include "nsCSSRendering.h"
#include "nsINameSpaceManager.h"
#include "nsLayoutAtoms.h"
#include "nsMenuPopupFrame.h"
#include "nsMenuBarFrame.h"
#include "nsIView.h"
#include "nsIWidget.h"
#include "nsIDocument.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMElement.h"
#include "nsISupportsArray.h"
#include "nsIDOMText.h"
#include "nsILookAndFeel.h"
#include "nsIComponentManager.h"
#include "nsWidgetsCID.h"
#include "nsBoxLayoutState.h"
#include "nsIXBLBinding.h"
#include "nsIScrollableFrame.h"
#include "nsIViewManager.h"
#include "nsIBindingManager.h"
#include "nsIServiceManager.h"
#include "nsIXBLService.h"
#include "nsCSSFrameConstructor.h"
#include "nsIDOMKeyEvent.h"
#include "nsIPref.h"
#include "nsIScrollableView.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsIStringBundle.h"
#include "nsGUIEvent.h"
#include "nsIEventListenerManager.h"
#include "nsIEventStateManager.h"

#define NS_MENU_POPUP_LIST_INDEX   0

#ifdef XP_PC
#define NSCONTEXTMENUISMOUSEUP 1
#endif

static PRInt32 gEatMouseMove = PR_FALSE;

nsMenuDismissalListener* nsMenuFrame::sDismissalListener = nsnull;

static NS_DEFINE_IID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);

nsrefcnt nsMenuFrame::gRefCnt = 0;
nsString *nsMenuFrame::gShiftText = nsnull;
nsString *nsMenuFrame::gControlText = nsnull;
nsString *nsMenuFrame::gMetaText = nsnull;
nsString *nsMenuFrame::gAltText = nsnull;
nsString *nsMenuFrame::gModifierSeparator = nsnull;

//
// NS_NewMenuFrame
//
// Wrapper for creating a new menu popup container
//
nsresult
NS_NewMenuFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame, PRUint32 aFlags)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsMenuFrame* it = new (aPresShell) nsMenuFrame (aPresShell);
  if ( !it )
    return NS_ERROR_OUT_OF_MEMORY;
  *aNewFrame = it;
  if (aFlags)
    it->SetIsMenu(PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt) 
nsMenuFrame::AddRef(void)
{
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt)
nsMenuFrame::Release(void)
{
    return NS_OK;
}

//
// QueryInterface
//
NS_INTERFACE_MAP_BEGIN(nsMenuFrame)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
  NS_INTERFACE_MAP_ENTRY(nsIMenuFrame)
  NS_INTERFACE_MAP_ENTRY(nsIScrollableViewProvider)
NS_INTERFACE_MAP_END_INHERITING(nsBoxFrame)

//
// nsMenuFrame cntr
//
nsMenuFrame::nsMenuFrame(nsIPresShell* aShell):nsBoxFrame(aShell),
    mIsMenu(PR_FALSE),
    mMenuOpen(PR_FALSE),
    mCreateHandlerSucceeded(PR_FALSE),
    mChecked(PR_FALSE),
    mType(eMenuType_Normal),
    mMenuParent(nsnull),
    mPresContext(nsnull),
    mLastPref(-1,-1),
    mFrameConstructor(nsnull)
{

} // cntr

NS_IMETHODIMP
nsMenuFrame::SetParent(const nsIFrame* aParent)
{
  nsBoxFrame::SetParent(aParent);
  nsIFrame* currFrame = (nsIFrame*)aParent;
  while (!mMenuParent && currFrame) {
    // Set our menu parent.
    nsCOMPtr<nsIMenuParent> menuparent(do_QueryInterface(currFrame));
    mMenuParent = menuparent.get();

    currFrame->GetParent(&currFrame);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::Init(nsIPresContext*  aPresContext,
                     nsIContent*      aContent,
                     nsIFrame*        aParent,
                     nsIStyleContext* aContext,
                     nsIFrame*        aPrevInFlow)
{
  mPresContext = aPresContext; // Don't addref it.  Our lifetime is shorter.

  nsresult  rv = nsBoxFrame::Init(aPresContext, aContent, aParent, aContext, aPrevInFlow);

  nsIFrame* currFrame = aParent;
  while (!mMenuParent && currFrame) {
    // Set our menu parent.
    nsCOMPtr<nsIMenuParent> menuparent(do_QueryInterface(currFrame));
    mMenuParent = menuparent.get();

    currFrame->GetParent(&currFrame);
  }

  // Do the type="checkbox" magic
  UpdateMenuType(aPresContext);

  //load the display strings for the keyboard accelerators, but only once
  if (gRefCnt++ == 0) {
    
    nsCOMPtr<nsIStringBundleService> bundleService(do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv));
    nsCOMPtr<nsIStringBundle> bundle;
    if (NS_SUCCEEDED(rv) && bundleService) {
      rv = bundleService->CreateBundle( "chrome://global-platform/locale/platformKeys.properties",
                                        getter_AddRefs(bundle));
    }    
    
    NS_ASSERTION(NS_SUCCEEDED(rv) && bundle, "chrome://global/locale/platformKeys.properties could not be loaded");
    nsXPIDLString shiftModifier;
    nsXPIDLString metaModifier;
    nsXPIDLString altModifier;
    nsXPIDLString controlModifier;
    nsXPIDLString modifierSeparator;
    if (NS_SUCCEEDED(rv) && bundle) {
      //macs use symbols for each modifier key, so fetch each from the bundle, which also covers i18n
      rv = bundle->GetStringFromName(NS_LITERAL_STRING("VK_SHIFT").get(), getter_Copies(shiftModifier));
      rv = bundle->GetStringFromName(NS_LITERAL_STRING("VK_META").get(), getter_Copies(metaModifier));
      rv = bundle->GetStringFromName(NS_LITERAL_STRING("VK_ALT").get(), getter_Copies(altModifier));
      rv = bundle->GetStringFromName(NS_LITERAL_STRING("VK_CONTROL").get(), getter_Copies(controlModifier));
      rv = bundle->GetStringFromName(NS_LITERAL_STRING("MODIFIER_SEPARATOR").get(), getter_Copies(modifierSeparator));
    } else {
      rv = NS_ERROR_NOT_AVAILABLE;
    }
    //if any of these don't exist, we get  an empty string
    gShiftText = new nsString(shiftModifier);
    gMetaText = new nsString(metaModifier);
    gAltText = new nsString(altModifier);
    gControlText = new nsString(controlModifier);
    gModifierSeparator = new nsString(modifierSeparator);    
  }
  
  BuildAcceleratorText();
  
  return rv;
}

nsMenuFrame::~nsMenuFrame()
{
  // Clean up shared statics
  if (--gRefCnt == 0) {
    delete gShiftText;
    gShiftText = nsnull;
    delete gControlText;  
    gControlText = nsnull;
    delete gMetaText;  
    gMetaText = nsnull;
    delete gAltText;  
    gAltText = nsnull;
    delete gModifierSeparator;
    gModifierSeparator = nsnull;
  }
}

// The following methods are all overridden to ensure that the menupopup frame
// is placed in the appropriate list.
NS_IMETHODIMP
nsMenuFrame::FirstChild(nsIPresContext* aPresContext,
                        nsIAtom*        aListName,
                        nsIFrame**      aFirstChild) const
{
  if (nsLayoutAtoms::popupList == aListName) {
    *aFirstChild = mPopupFrames.FirstChild();
  } else {
    nsBoxFrame::FirstChild(aPresContext, aListName, aFirstChild);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::SetInitialChildList(nsIPresContext* aPresContext,
                                               nsIAtom*        aListName,
                                               nsIFrame*       aChildList)
{
  nsresult rv = NS_OK;
  if (nsLayoutAtoms::popupList == aListName) {
    mPopupFrames.SetFrames(aChildList);
  } else {

    nsFrameList frames(aChildList);

    // We may have a menupopup in here. Get it out, and move it into
    // the popup frame list.
    nsIFrame* frame = frames.FirstChild();
    while (frame) {
      nsCOMPtr<nsIMenuParent> menuPar(do_QueryInterface(frame));
      if (menuPar) {
        // Remove this frame from the list and place it in the other list.
        frames.RemoveFrame(frame);
        mPopupFrames.AppendFrame(this, frame);
        nsIFrame* first = frames.FirstChild();
        rv = nsBoxFrame::SetInitialChildList(aPresContext, aListName, first);
        return rv;
      }
      frame->GetNextSibling(&frame);
    }

    // Didn't find it.
    rv = nsBoxFrame::SetInitialChildList(aPresContext, aListName, aChildList);
  }
  return rv;
}

NS_IMETHODIMP
nsMenuFrame::GetAdditionalChildListName(PRInt32   aIndex,
                                        nsIAtom** aListName) const
{
  NS_PRECONDITION(nsnull != aListName, "null OUT parameter pointer");

  *aListName = nsnull;

  // don't expose the child frame list, it slows things down
#if 0
  if (NS_MENU_POPUP_LIST_INDEX == aIndex) {
    *aListName = nsLayoutAtoms::popupList;
    NS_ADDREF(*aListName);
  }
#endif

  return NS_OK;
}

nsresult
nsMenuFrame::DestroyPopupFrames(nsIPresContext* aPresContext)
{
  // Remove our frame mappings
  if (mFrameConstructor) {
    nsIFrame* curFrame = mPopupFrames.FirstChild();
    while (curFrame) {
      mFrameConstructor->RemoveMappingsForFrameSubtree(aPresContext, curFrame, nsnull);
      curFrame->GetNextSibling(&curFrame);
    }
  }

   // Cleanup frames in popup child list
  mPopupFrames.DestroyFrames(aPresContext);
  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::Destroy(nsIPresContext* aPresContext)
{
  DestroyPopupFrames(aPresContext);
  return nsBoxFrame::Destroy(aPresContext);
}

// Called to prevent events from going to anything inside the menu.
NS_IMETHODIMP
nsMenuFrame::GetFrameForPoint(nsIPresContext* aPresContext,
                              const nsPoint& aPoint, 
                              nsFramePaintLayer aWhichLayer,    
                              nsIFrame**     aFrame)
{

  if ((aWhichLayer != NS_FRAME_PAINT_LAYER_FOREGROUND))
    return NS_ERROR_FAILURE;

 // if it is not inside us or not in the layer in which we paint, fail
  if (!mRect.Contains(aPoint)) 
      return NS_ERROR_FAILURE;
  
  nsresult result = nsBoxFrame::GetFrameForPoint(aPresContext, aPoint, aWhichLayer, aFrame);
  if ((result != NS_OK) || (*aFrame == this)) {
    return result;
  }
  nsCOMPtr<nsIContent> content;
  (*aFrame)->GetContent(getter_AddRefs(content));
  if (content) {
    // This allows selective overriding for subcontent.
    nsAutoString value;
    content->GetAttr(kNameSpaceID_None, nsXULAtoms::allowevents, value);
    if (value.Equals(NS_LITERAL_STRING("true")))
      return result;
  }
  const nsStyleVisibility* vis = 
      (const nsStyleVisibility*)mStyleContext->GetStyleData(eStyleStruct_Visibility);
  if (vis->IsVisible()) {
    *aFrame = this; // Capture all events so that we can perform selection
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
nsMenuFrame::HandleEvent(nsIPresContext* aPresContext, 
                             nsGUIEvent*     aEvent,
                             nsEventStatus*  aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  *aEventStatus = nsEventStatus_eConsumeDoDefault;
  
  if (aEvent->message == NS_KEY_PRESS && !IsDisabled()) {
    nsKeyEvent* keyEvent = (nsKeyEvent*)aEvent;
    PRUint32 keyCode = keyEvent->keyCode;
    if ((keyCode == NS_VK_F4 && !mMenuParent) && IsOpen() &&
      !keyEvent->isAlt && !keyEvent->isShift && !keyEvent->isControl) 
      OpenMenu(PR_FALSE); // Close menu on unmodified F4
    else if (keyCode == NS_VK_UP || keyCode == NS_VK_DOWN || 
       (keyCode == NS_VK_F4 && !keyEvent->isAlt && !keyEvent->isShift && 
       !keyEvent->isControl && !mMenuParent)) 
      // Plain or modified down or up arrow will open any menu
      // Unmodified F4 will open <menulist> as well
      if (!IsOpen())
        OpenMenu(PR_TRUE);
  }
  else if (aEvent->message == NS_MOUSE_LEFT_BUTTON_DOWN && !IsDisabled() && IsMenu() ) {
    PRBool isMenuBar = PR_FALSE;
    if (mMenuParent)
      mMenuParent->IsMenuBar(isMenuBar);

    // The menu item was selected. Bring up the menu.
    // We have children.
    if ( isMenuBar || !mMenuParent ) {
      ToggleMenuState();

      if (!IsOpen() && mMenuParent) {
        // We closed up. The menu bar should always be
        // deactivated when this happens.
        mMenuParent->SetActive(PR_FALSE);
      }
    }
    else
      if ( !IsOpen() ) {
        // one of our siblings is probably open and even possibly waiting
        // for its close timer to fire. Tell our parent to close it down. Not
        // doing this before its timer fires will cause the rollup state to
        // get very confused.
        if ( mMenuParent )
          mMenuParent->KillPendingTimers();

        // safe to open up
        OpenMenu(PR_TRUE);
      }
  }
  else if (
#ifndef NSCONTEXTMENUISMOUSEUP
            aEvent->message == NS_MOUSE_RIGHT_BUTTON_UP &&
#else
            aEvent->message == NS_CONTEXTMENU &&
#endif
            mMenuParent && !IsMenu() && !IsDisabled()) {
    // if this menu is a context menu it accepts right-clicks...fire away!
    // Make sure we cancel default processing of the context menu event so
    // that it doesn't bubble and get seen again by the popuplistener and show
    // another context menu.
    //
    // Furthermore (there's always more, isn't there?), on some platforms (win32
    // being one of them) we get the context menu event on a mouse up while
    // on others we get it on a mouse down. For the ones where we get it on a
    // mouse down, we must continue listening for the right button up event to
    // dismiss the menu.
    PRBool isContextMenu = PR_FALSE;
    mMenuParent->GetIsContextMenu(isContextMenu);
    if ( isContextMenu ) {
      *aEventStatus = nsEventStatus_eConsumeNoDefault;
      Execute();
    }
  }
  else if (aEvent->message == NS_MOUSE_LEFT_BUTTON_UP && !IsMenu() && mMenuParent && !IsDisabled()) {
    // First, flip "checked" state if we're a checkbox menu, or
    // an un-checked radio menu
    if (mType == eMenuType_Checkbox ||
        (mType == eMenuType_Radio && !mChecked)) {
      if (mChecked) {
        mContent->UnsetAttr(kNameSpaceID_None, nsHTMLAtoms::checked,
                            PR_TRUE);
      }
      else {
        mContent->SetAttr(kNameSpaceID_None, nsHTMLAtoms::checked, NS_LITERAL_STRING("true"),
                          PR_TRUE);
      }
        
      /* the AttributeChanged code will update all the internal state */
    }

    // Execute the execute event handler.
    Execute();
  }
  else if (aEvent->message == NS_MOUSE_EXIT_SYNTH) {
    // Kill our timer if one is active.
    if (mOpenTimer) {
      mOpenTimer->Cancel();
      mOpenTimer = nsnull;
    }

    // Deactivate the menu.
    PRBool isActive = PR_FALSE;
    PRBool isMenuBar = PR_FALSE;
    if (mMenuParent) {
      mMenuParent->IsMenuBar(isMenuBar);
      PRBool cancel = PR_TRUE;
      if (isMenuBar) {
        mMenuParent->GetIsActive(isActive);
        if (isActive) cancel = PR_FALSE;
      }
      
      if (cancel) {
        if (IsMenu() && !isMenuBar && mMenuOpen) {
          // Submenus don't get closed up immediately.
        }
        else mMenuParent->SetCurrentMenuItem(nsnull);
      }
    }
  }
  else if (aEvent->message == NS_MOUSE_MOVE && mMenuParent) {
    if (gEatMouseMove) {
      gEatMouseMove = PR_FALSE;
      return NS_OK;
    }

    // we checked for mMenuParent right above

    PRBool isMenuBar = PR_FALSE;
    mMenuParent->IsMenuBar(isMenuBar);

    // Let the menu parent know we're the new item.
    mMenuParent->SetCurrentMenuItem(this);
    
    // If we're a menu (and not a menu item),
    // kick off the timer.
    if (!IsDisabled() && !isMenuBar && IsMenu() && !mMenuOpen && !mOpenTimer) {

      PRInt32 menuDelay = 300;   // ms

      nsCOMPtr<nsILookAndFeel> lookAndFeel(do_CreateInstance(kLookAndFeelCID));
      if (lookAndFeel)
        lookAndFeel->GetMetric(nsILookAndFeel::eMetric_SubmenuDelay, menuDelay);

      // We're a menu, we're built, we're closed, and no timer has been kicked off.
      mOpenTimer = do_CreateInstance("@mozilla.org/timer;1");
      mOpenTimer->Init(this, menuDelay, NS_PRIORITY_HIGHEST);
    }
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::ToggleMenuState()
{  
  if (mMenuOpen) {
    OpenMenu(PR_FALSE);
  }
  else {
    OpenMenu(PR_TRUE);
  }

  return NS_OK;
}

void
nsMenuFrame::FireMenuDOMEvent(const nsAString& aDOMEventName)
{
  // Fire a DOM event for the title change.
  nsCOMPtr<nsIDOMEvent> event;
  nsCOMPtr<nsIEventListenerManager> manager;
  mContent->GetListenerManager(getter_AddRefs(manager));
  if (manager &&
      NS_SUCCEEDED(manager->CreateEvent(mPresContext, nsnull, NS_LITERAL_STRING("Events"), getter_AddRefs(event)))) {
    event->InitEvent(aDOMEventName, PR_TRUE, PR_TRUE);
    PRBool noDefault;
    nsCOMPtr<nsIEventStateManager> esm;
    mPresContext->GetEventStateManager(getter_AddRefs(esm));
    if (esm)
      esm->DispatchNewEvent(mContent, event, &noDefault);
  }
}

NS_IMETHODIMP
nsMenuFrame::SelectMenu(PRBool aActivateFlag)
{
  if (!mContent) {
    return NS_OK;
  }

  nsAutoString domEventToFire;

  if (aActivateFlag) {
    // Highlight the menu.
    mContent->SetAttr(kNameSpaceID_None, nsXULAtoms::menuactive, NS_LITERAL_STRING("true"), PR_TRUE);
    // The menuactivated event is used by accessibility to track the user's movements through menus
    domEventToFire.Assign(NS_LITERAL_STRING("DOMMenuItemActive"));
  }
  else {
    // Unhighlight the menu.
    mContent->UnsetAttr(kNameSpaceID_None, nsXULAtoms::menuactive, PR_TRUE);
    domEventToFire.Assign(NS_LITERAL_STRING("DOMMenuItemInactive"));
  }

  FireMenuDOMEvent(domEventToFire);
  return NS_OK;
}

PRBool nsMenuFrame::IsGenerated()
{
  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  // Generate the menu if it hasn't been generated already.  This
  // takes it from display: none to display: block and gives us
  // a menu forevermore.
  if (child) {
    nsString genVal;
    child->GetAttr(kNameSpaceID_None, nsXULAtoms::menugenerated, genVal);
    if (genVal.IsEmpty())
      return PR_FALSE;
  }

  return PR_TRUE;
}

NS_IMETHODIMP
nsMenuFrame::MarkAsGenerated()
{
  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  // Generate the menu if it hasn't been generated already.  This
  // takes it from display: none to display: block and gives us
  // a menu forevermore.
  if (child) {
    nsAutoString genVal;
    child->GetAttr(kNameSpaceID_None, nsXULAtoms::menugenerated, genVal);
    if (genVal.IsEmpty())
      child->SetAttr(kNameSpaceID_None, nsXULAtoms::menugenerated, NS_LITERAL_STRING("true"), PR_TRUE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::UngenerateMenu()
{
  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  if (child) {
    nsAutoString genVal;
    child->GetAttr(kNameSpaceID_None, nsXULAtoms::menugenerated, genVal);
    if (!genVal.IsEmpty())
      child->UnsetAttr(kNameSpaceID_None, nsXULAtoms::menugenerated, PR_TRUE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::ActivateMenu(PRBool aActivateFlag)
{
  nsIFrame* frame = mPopupFrames.FirstChild();
  nsMenuPopupFrame* menuPopup = (nsMenuPopupFrame*)frame;
  
  if (!menuPopup) 
    return NS_OK;

  if (aActivateFlag) {
      nsRect rect;
      menuPopup->GetRect(rect);
      nsIView* view = nsnull;
      menuPopup->GetView(mPresContext, &view);
      nsCOMPtr<nsIViewManager> viewManager;
      view->GetViewManager(*getter_AddRefs(viewManager));
      rect.x = rect.y = 0;
      viewManager->ResizeView(view, rect);

      // make sure the scrolled window is at 0,0
      if (mLastPref.height <= rect.height) {
        nsIBox* child;
        menuPopup->GetChildBox(&child);

        nsCOMPtr<nsIScrollableFrame> scrollframe(do_QueryInterface(child));
        if (scrollframe) {
          scrollframe->ScrollTo(mPresContext, 0, 0);
        }
      }

      viewManager->UpdateView(view, rect, NS_VMREFRESH_IMMEDIATE);
      viewManager->SetViewVisibility(view, nsViewVisibility_kShow);

  } else {
    nsIView* view = nsnull;
    menuPopup->GetView(mPresContext, &view);
    NS_ASSERTION(view, "View is gone, looks like someone forgot to rollup the popup!");
    if (view) {
      nsCOMPtr<nsIViewManager> viewManager;
      view->GetViewManager(*getter_AddRefs(viewManager));
      if (viewManager) { // the view manager can be null during widget teardown
        viewManager->SetViewVisibility(view, nsViewVisibility_kHide);
        nsRect r(0, 0, 0, 0);
        viewManager->ResizeView(view, r);
      }
    }
    // set here so hide chain can close the menu as well.
    mMenuOpen = PR_FALSE;
  }
  
  return NS_OK;
}  

NS_IMETHODIMP
nsMenuFrame::AttributeChanged(nsIPresContext* aPresContext,
                              nsIContent* aChild,
                              PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType, 
                              PRInt32 aHint)
{
  nsAutoString value;

  if (aAttribute == nsXULAtoms::open) {
    aChild->GetAttr(kNameSpaceID_None, aAttribute, value);
    if (value.Equals(NS_LITERAL_STRING("true")))
      OpenMenuInternal(PR_TRUE);
    else {
      OpenMenuInternal(PR_FALSE);
      mCreateHandlerSucceeded = PR_FALSE;
    }
  } else if (aAttribute == nsHTMLAtoms::checked) {
    if (mType != eMenuType_Normal)
        UpdateMenuSpecialState(aPresContext);
  } else if (aAttribute == nsXULAtoms::acceltext) {
    // someone reset the accelText attribute, so clear the bit that says *we* set it
    nsFrameState state;
    GetFrameState(&state);
    SetFrameState(state & ~NS_STATE_ACCELTEXT_IS_DERIVED);
    BuildAcceleratorText();
  } else if (aAttribute == nsXULAtoms::key) {
    BuildAcceleratorText();
  } else if ( aAttribute == nsHTMLAtoms::type || aAttribute == nsHTMLAtoms::name )
    UpdateMenuType(aPresContext);

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::OpenMenu(PRBool aActivateFlag)
{
  nsCOMPtr<nsIDOMElement> domElement(do_QueryInterface(mContent));
  if (aActivateFlag) {
    // Now that the menu is opened, we should have a menupopup child built.
    // Mark it as generated, which ensures a frame gets built.
    MarkAsGenerated();

    domElement->SetAttribute(NS_LITERAL_STRING("open"), NS_LITERAL_STRING("true"));
  }
  else domElement->RemoveAttribute(NS_LITERAL_STRING("open"));

  return NS_OK;
}

void 
nsMenuFrame::OpenMenuInternal(PRBool aActivateFlag) 
{
  gEatMouseMove = PR_TRUE;

  if (!mIsMenu)
    return;

  if (aActivateFlag) {
    // Execute the oncreate handler
    if (!OnCreate())
      return;

    mCreateHandlerSucceeded = PR_TRUE;
  
    // Set the focus back to our view's widget.
    if (nsMenuFrame::sDismissalListener)
      nsMenuFrame::sDismissalListener->EnableListener(PR_FALSE);
    
    // XXX Only have this here because of RDF-generated content.
    MarkAsGenerated();

    nsIFrame* frame = mPopupFrames.FirstChild();
    nsMenuPopupFrame* menuPopup = (nsMenuPopupFrame*)frame;
    
    mMenuOpen = PR_TRUE;

    if (menuPopup) {
      // inherit whether or not we're a context menu from the parent
      if ( mMenuParent ) {
        PRBool parentIsContextMenu = PR_FALSE;
        mMenuParent->GetIsContextMenu(parentIsContextMenu);
        menuPopup->SetIsContextMenu(parentIsContextMenu);
      }

      // Install a keyboard navigation listener if we're the root of the menu chain.
      PRBool onMenuBar = PR_TRUE;
      if (mMenuParent)
        mMenuParent->IsMenuBar(onMenuBar);

      if (mMenuParent && onMenuBar)
        mMenuParent->InstallKeyboardNavigator();
      else if (!mMenuParent)
        menuPopup->InstallKeyboardNavigator();
      
      // Tell the menu bar we're active.
      if (mMenuParent)
        mMenuParent->SetActive(PR_TRUE);

      nsCOMPtr<nsIContent> menuPopupContent;
      menuPopup->GetContent(getter_AddRefs(menuPopupContent));

      // Sync up the view.
      nsAutoString popupAnchor, popupAlign;
      
      menuPopupContent->GetAttr(kNameSpaceID_None, nsXULAtoms::popupanchor, popupAnchor);
      menuPopupContent->GetAttr(kNameSpaceID_None, nsXULAtoms::popupalign, popupAlign);

      if (onMenuBar) {
        if (popupAnchor.IsEmpty())
          popupAnchor = NS_LITERAL_STRING("bottomleft");
        if (popupAlign.IsEmpty())
          popupAlign = NS_LITERAL_STRING("topleft");
      }
      else {
        if (popupAnchor.IsEmpty())
          popupAnchor = NS_LITERAL_STRING("topright");
        if (popupAlign.IsEmpty())
          popupAlign = NS_LITERAL_STRING("topleft");
      }

      nsBoxLayoutState state(mPresContext);

      // if height never set we need to do an initial reflow.
      if (mLastPref.height == -1)
      {
         menuPopup->MarkDirty(state);

         nsCOMPtr<nsIPresShell> shell;
         mPresContext->GetShell(getter_AddRefs(shell));
         shell->FlushPendingNotifications(PR_FALSE);
      }

      nsRect curRect;
      menuPopup->GetBounds(curRect);

      menuPopup->SetBounds(state, nsRect(0,0,mLastPref.width, mLastPref.height));

      nsIView* view = nsnull;
      menuPopup->GetView(mPresContext, &view);
      nsCOMPtr<nsIViewManager> vm;
      view->GetViewManager(*getter_AddRefs(vm));
      if (vm) {
        vm->SetViewVisibility(view, nsViewVisibility_kHide);
      }
      menuPopup->SyncViewWithFrame(mPresContext, popupAnchor, popupAlign, this, -1, -1);
      nsRect rect;
      menuPopup->GetBounds(rect);

      // if the height is different then reflow. It might need scrollbars force a reflow
      if (curRect.height != rect.height || mLastPref.height != rect.height)
      {
         menuPopup->MarkDirty(state);
         nsCOMPtr<nsIPresShell> shell;
         mPresContext->GetShell(getter_AddRefs(shell));
         shell->FlushPendingNotifications(PR_FALSE);
      }

      ActivateMenu(PR_TRUE);

      nsCOMPtr<nsIMenuParent> childPopup(do_QueryInterface(frame));
      UpdateDismissalListener(childPopup);

      OnCreated();
    }

    // Set the focus back to our view's widget.
    if (nsMenuFrame::sDismissalListener)
      nsMenuFrame::sDismissalListener->EnableListener(PR_TRUE);
    
  }
  else {

    // Close the menu. 
    // Execute the ondestroy handler, but only if we're actually open
    if ( !mCreateHandlerSucceeded || !OnDestroy() )
      return;

    mMenuOpen = PR_FALSE;

    // Set the focus back to our view's widget.
    if (nsMenuFrame::sDismissalListener) {
      nsMenuFrame::sDismissalListener->EnableListener(PR_FALSE);
      nsMenuFrame::sDismissalListener->SetCurrentMenuParent(mMenuParent);
    }

    nsIFrame* frame = mPopupFrames.FirstChild();
    nsMenuPopupFrame* menuPopup = (nsMenuPopupFrame*)frame;
  
    // Make sure we clear out our own items.
    if (menuPopup) {
      menuPopup->SetCurrentMenuItem(nsnull);
      menuPopup->KillCloseTimer();

      PRBool onMenuBar = PR_TRUE;
      if (mMenuParent)
        mMenuParent->IsMenuBar(onMenuBar);

      if (mMenuParent && onMenuBar)
        mMenuParent->RemoveKeyboardNavigator();
      else if (!mMenuParent)
        menuPopup->RemoveKeyboardNavigator();
    }

    // activate false will also set the mMenuOpen to false.
    ActivateMenu(PR_FALSE);

    OnDestroyed();

    if (nsMenuFrame::sDismissalListener)
      nsMenuFrame::sDismissalListener->EnableListener(PR_TRUE);
  }

}

void
nsMenuFrame::GetMenuChildrenElement(nsIContent** aResult)
{
  if (!mContent)
  {
    *aResult = nsnull;
    return;
  }
  
  nsresult rv;
  nsCOMPtr<nsIXBLService> xblService = 
           do_GetService("@mozilla.org/xbl;1", &rv);
  PRInt32 dummy;
  PRInt32 count;
  mContent->ChildCount(count);

  for (PRInt32 i = 0; i < count; i++) {
    nsCOMPtr<nsIContent> child;
    mContent->ChildAt(i, *getter_AddRefs(child));
    nsCOMPtr<nsIAtom> tag;
    xblService->ResolveTag(child, &dummy, getter_AddRefs(tag));
    if (tag && tag.get() == nsXULAtoms::menupopup) {
      *aResult = child.get();
      NS_ADDREF(*aResult);
      return;
    }
  }
}

PRBool
nsMenuFrame::IsSizedToPopup(nsIContent* aContent, PRBool aRequireAlways)
{
  nsCOMPtr<nsIAtom> tag;
  aContent->GetTag(*getter_AddRefs(tag));
  PRBool sizeToPopup;
  if (tag == nsHTMLAtoms::select)
    sizeToPopup = PR_TRUE;
  else {
    nsAutoString sizedToPopup;
    aContent->GetAttr(kNameSpaceID_None, nsXULAtoms::sizetopopup, sizedToPopup);
    sizeToPopup = (sizedToPopup.EqualsIgnoreCase("always") ||
                   (!aRequireAlways && sizedToPopup.EqualsIgnoreCase("pref")));
  }
  
  return sizeToPopup;
}

NS_IMETHODIMP
nsMenuFrame::GetMinSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize)
{
  // Our min size is the popup size (same as the pref size) if
  // sizetopopup="always" is set.  However, we first need to check
  // to see if a min size was set in CSS.
  PRBool collapsed = PR_FALSE;
  IsCollapsed(aBoxLayoutState, collapsed);
  if (collapsed) {
    aSize.width = aSize.height = 0;
    return NS_OK;
  }

  nsIFrame* popupChild = mPopupFrames.FirstChild();

  if (popupChild && IsSizedToPopup(mContent, PR_TRUE))
    return GetPrefSize(aBoxLayoutState, aSize);

  return nsBoxFrame::GetMinSize(aBoxLayoutState, aSize);
}

NS_IMETHODIMP
nsMenuFrame::DoLayout(nsBoxLayoutState& aState)
{
  nsRect contentRect;
  GetContentRect(contentRect);

  // lay us out
  nsresult rv = nsBoxFrame::DoLayout(aState);

  // layout the popup. First we need to get it.
  nsIFrame* popupChild = mPopupFrames.FirstChild();

  if (popupChild) {
    PRBool sizeToPopup = IsSizedToPopup(mContent, PR_FALSE);
    nsIBox* ibox = nsnull;
    nsresult rv2 = popupChild->QueryInterface(NS_GET_IID(nsIBox), (void**)&ibox);
    NS_ASSERTION(NS_SUCCEEDED(rv2) && ibox,"popupChild is not box!!");

    // then get its preferred size
    nsSize prefSize(0,0);
    nsSize minSize(0,0);
    nsSize maxSize(0,0);

    ibox->GetPrefSize(aState, prefSize);
    ibox->GetMinSize(aState, minSize);
    ibox->GetMaxSize(aState, maxSize);

    BoundsCheck(minSize, prefSize, maxSize);

    if (sizeToPopup)
        prefSize.width = contentRect.width;

    // if the pref size changed then set bounds to be the pref size
    // and sync the view. And set new pref size.
    if (mLastPref != prefSize) {
      ibox->SetBounds(aState, nsRect(0,0,prefSize.width, prefSize.height));
      RePositionPopup(aState);
      mLastPref = prefSize;
    }

    // is the new size too small? Make sure we handle scrollbars correctly
    nsIBox* child;
    ibox->GetChildBox(&child);

    nsRect bounds(0,0,0,0);
    ibox->GetBounds(bounds);

    nsCOMPtr<nsIScrollableFrame> scrollframe(do_QueryInterface(child));
    if (scrollframe) {
      nsIScrollableFrame::nsScrollPref pref;
      scrollframe->GetScrollPreference(aState.GetPresContext(), &pref);

      if (pref == nsIScrollableFrame::Auto)  
      {
        // if our pref height
        if (bounds.height < prefSize.height) {
           // layout the child
           ibox->Layout(aState);

           nscoord width;
           nscoord height;
           scrollframe->GetScrollbarSizes(aState.GetPresContext(), &width, &height);
           if (bounds.width < prefSize.width + width)
           {
             bounds.width += width;
             //printf("Width=%d\n",width);
             ibox->SetBounds(aState, bounds);
           }
        }
      }
    }
    
    // layout the child
    ibox->Layout(aState);

    // Only size the popups view if open.
    if (mMenuOpen) {
      nsIView* view = nsnull;
      popupChild->GetView(aState.GetPresContext(), &view);
      nsCOMPtr<nsIViewManager> viewManager;
      view->GetViewManager(*getter_AddRefs(viewManager));
      nsRect r(0, 0, bounds.width, bounds.height);
      viewManager->ResizeView(view, r);
    }

  }

  SyncLayout(aState);

  return rv;
}

NS_IMETHODIMP
nsMenuFrame::MarkChildrenStyleChange()  
{
  nsresult rv = nsBoxFrame::MarkChildrenStyleChange();
  if (NS_FAILED(rv))
    return rv;
   
  nsIFrame* popupChild = mPopupFrames.FirstChild();

  if (popupChild) {
    nsIBox* ibox = nsnull;
    nsresult rv2 = popupChild->QueryInterface(NS_GET_IID(nsIBox), (void**)&ibox);
    NS_ASSERTION(NS_SUCCEEDED(rv2) && ibox,"popupChild is not box!!");

    return ibox->MarkChildrenStyleChange();
  }

  return rv;
}

NS_IMETHODIMP
nsMenuFrame::SetDebug(nsBoxLayoutState& aState, PRBool aDebug)
{
  // see if our state matches the given debug state
  PRBool debugSet = mState & NS_STATE_CURRENTLY_IN_DEBUG;
  PRBool debugChanged = (!aDebug && debugSet) || (aDebug && !debugSet);

  // if it doesn't then tell each child below us the new debug state
  if (debugChanged)
  {
      nsBoxFrame::SetDebug(aState, aDebug);
      SetDebug(aState, mPopupFrames.FirstChild(), aDebug);
  }

  return NS_OK;
}

nsresult
nsMenuFrame::SetDebug(nsBoxLayoutState& aState, nsIFrame* aList, PRBool aDebug)
{
      if (!aList)
          return NS_OK;

      while (aList) {
          nsIBox* ibox = nsnull;
          if (NS_SUCCEEDED(aList->QueryInterface(NS_GET_IID(nsIBox), (void**)&ibox)) && ibox) {
              ibox->SetDebug(aState, aDebug);
          }

          aList->GetNextSibling(&aList);
      }

      return NS_OK;
}

static void ConvertPosition(nsIContent* aPopupElt, nsString& aAnchor, nsString& aAlign)
{
  nsAutoString position;
  aPopupElt->GetAttr(kNameSpaceID_None, nsXULAtoms::position, position);
  if (position.IsEmpty())
    return;

  if (position.Equals(NS_LITERAL_STRING("before_start"))) {
    aAnchor.Assign(NS_LITERAL_STRING("topleft"));
    aAlign.Assign(NS_LITERAL_STRING("bottomleft"));
  }
  else if (position.Equals(NS_LITERAL_STRING("before_end"))) {
    aAnchor.Assign(NS_LITERAL_STRING("topright"));
    aAlign.Assign(NS_LITERAL_STRING("bottomright"));
  }
  else if (position.Equals(NS_LITERAL_STRING("after_start"))) {
    aAnchor.Assign(NS_LITERAL_STRING("bottomleft"));
    aAlign.Assign(NS_LITERAL_STRING("topleft"));
  }
  else if (position.Equals(NS_LITERAL_STRING("after_end"))) {
    aAnchor.Assign(NS_LITERAL_STRING("bottomright"));
    aAlign.Assign(NS_LITERAL_STRING("topright"));
  }
  else if (position.Equals(NS_LITERAL_STRING("start_before"))) {
    aAnchor.Assign(NS_LITERAL_STRING("topleft"));
    aAlign.Assign(NS_LITERAL_STRING("topright"));
  }
  else if (position.Equals(NS_LITERAL_STRING("start_after"))) {
    aAnchor.Assign(NS_LITERAL_STRING("bottomleft"));
    aAlign.Assign(NS_LITERAL_STRING("bottomright"));
  }
  else if (position.Equals(NS_LITERAL_STRING("end_before"))) {
    aAnchor.Assign(NS_LITERAL_STRING("topright"));
    aAlign.Assign(NS_LITERAL_STRING("topleft"));
  }
  else if (position.Equals(NS_LITERAL_STRING("end_after"))) {
    aAnchor.Assign(NS_LITERAL_STRING("bottomright"));
    aAlign.Assign(NS_LITERAL_STRING("bottomleft"));
  }
  else if (position.Equals(NS_LITERAL_STRING("overlap"))) {
    aAnchor.Assign(NS_LITERAL_STRING("topleft"));
    aAlign.Assign(NS_LITERAL_STRING("topleft"));
  }
}

void
nsMenuFrame::RePositionPopup(nsBoxLayoutState& aState)
{  
  nsIPresContext* presContext = aState.GetPresContext();

  // Sync up the view.
  nsIFrame* frame = mPopupFrames.FirstChild();
  nsMenuPopupFrame* menuPopup = (nsMenuPopupFrame*)frame;
  if (mMenuOpen && menuPopup) {
    nsCOMPtr<nsIContent> menuPopupContent;
    menuPopup->GetContent(getter_AddRefs(menuPopupContent));
    nsAutoString popupAnchor, popupAlign;
      
    menuPopupContent->GetAttr(kNameSpaceID_None, nsXULAtoms::popupanchor, popupAnchor);
    menuPopupContent->GetAttr(kNameSpaceID_None, nsXULAtoms::popupalign, popupAlign);

    ConvertPosition(menuPopupContent, popupAnchor, popupAlign);

    PRBool onMenuBar = PR_TRUE;
    if (mMenuParent)
      mMenuParent->IsMenuBar(onMenuBar);

    if (onMenuBar) {
      if (popupAnchor.IsEmpty())
          popupAnchor = NS_LITERAL_STRING("bottomleft");
      if (popupAlign.IsEmpty())
          popupAlign = NS_LITERAL_STRING("topleft");
    }
    else {
      if (popupAnchor.IsEmpty())
        popupAnchor = NS_LITERAL_STRING("topright");
      if (popupAlign.IsEmpty())
        popupAlign = NS_LITERAL_STRING("topleft");
    }

    menuPopup->SyncViewWithFrame(presContext, popupAnchor, popupAlign, this, -1, -1);
  }
}

NS_IMETHODIMP
nsMenuFrame::ShortcutNavigation(nsIDOMKeyEvent* aKeyEvent, PRBool& aHandledFlag)
{
  nsIFrame* frame = mPopupFrames.FirstChild();
  if (frame) {
    nsMenuPopupFrame* popup = (nsMenuPopupFrame*)frame;
    popup->ShortcutNavigation(aKeyEvent, aHandledFlag);
  } 

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::KeyboardNavigation(PRUint32 aDirection, PRBool& aHandledFlag)
{
  nsIFrame* frame = mPopupFrames.FirstChild();
  if (frame) {
    nsMenuPopupFrame* popup = (nsMenuPopupFrame*)frame;
    popup->KeyboardNavigation(aDirection, aHandledFlag);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::Escape(PRBool& aHandledFlag)
{
  nsIFrame* frame = mPopupFrames.FirstChild();
  if (frame) {
    nsMenuPopupFrame* popup = (nsMenuPopupFrame*)frame;
    popup->Escape(aHandledFlag);
  }

  return NS_OK;
}


//
// Enter
//
// Called when the user hits the <Enter>/<Return> keys or presses the
// shortcut key. If this is a leaf item, the item's action will be executed.
// If it is a submenu parent, open the submenu and select the first time.
// In either case, do nothing if the item is disabled.
//
NS_IMETHODIMP
nsMenuFrame::Enter()
{
  if (IsDisabled()) {
#ifdef XP_WIN
    // behavior on Windows - close the popup chain
    if (mMenuParent)
      mMenuParent->DismissChain();
#endif   // #ifdef XP_WIN
    // this menu item was disabled - exit
    return NS_OK;
  }
    
  if (!mMenuOpen) {
    // The enter key press applies to us.
    if (!IsMenu() && mMenuParent)
      Execute();          // Execute our event handler
    else {
      OpenMenu(PR_TRUE);
      SelectFirstItem();
    }

    return NS_OK;
  }

  nsIFrame* frame = mPopupFrames.FirstChild();
  if (frame) {
    nsMenuPopupFrame* popup = (nsMenuPopupFrame*)frame;
    popup->Enter();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::SelectFirstItem()
{
  nsIFrame* frame = mPopupFrames.FirstChild();
  if (frame) {
    nsMenuPopupFrame* popup = (nsMenuPopupFrame*)frame;
    nsIMenuFrame* result;
    popup->GetNextMenuItem(nsnull, &result);
    popup->SetCurrentMenuItem(result);
  }

  return NS_OK;
}

PRBool
nsMenuFrame::IsMenu()
{
  return mIsMenu;
}

NS_IMETHODIMP_(void)
nsMenuFrame::Notify(nsITimer* aTimer)
{
  // Our timer has fired.
  if (aTimer == mOpenTimer.get()) {
    if (!mMenuOpen && mMenuParent) {
      nsAutoString active;
      mContent->GetAttr(kNameSpaceID_None, nsXULAtoms::menuactive, active);
      if (active.Equals(NS_LITERAL_STRING("true"))) {
        // We're still the active menu. Make sure all submenus/timers are closed
        // before opening this one
        mMenuParent->KillPendingTimers();
        OpenMenu(PR_TRUE);
      }
    }
    mOpenTimer->Cancel();
    mOpenTimer = nsnull;
  }
  
  mOpenTimer = nsnull;
}

PRBool 
nsMenuFrame::IsDisabled()
{
  nsAutoString disabled;
  mContent->GetAttr(kNameSpaceID_None, nsHTMLAtoms::disabled, disabled);
  if (disabled.Equals(NS_LITERAL_STRING("true")))
    return PR_TRUE;
  return PR_FALSE;
}

void
nsMenuFrame::UpdateMenuType(nsIPresContext* aPresContext)
{
  nsAutoString value;
  mContent->GetAttr(kNameSpaceID_None, nsHTMLAtoms::type, value);
  if (value.Equals(NS_LITERAL_STRING("checkbox")))
    mType = eMenuType_Checkbox;
  else if (value.Equals(NS_LITERAL_STRING("radio"))) {
    mType = eMenuType_Radio;

    nsAutoString valueName;
    mContent->GetAttr(kNameSpaceID_None, nsHTMLAtoms::name, valueName);
    if ( mGroupName != valueName )
      mGroupName = valueName;
  } 
  else {
    if (mType != eMenuType_Normal)
      mContent->UnsetAttr(kNameSpaceID_None, nsHTMLAtoms::checked,
                          PR_TRUE);
    mType = eMenuType_Normal;
  }
  UpdateMenuSpecialState(aPresContext);
}

/* update checked-ness for type="checkbox" and type="radio" */
void
nsMenuFrame::UpdateMenuSpecialState(nsIPresContext* aPresContext) {
  nsAutoString value;
  PRBool newChecked;

  mContent->GetAttr(kNameSpaceID_None, nsHTMLAtoms::checked,
                    value);
  newChecked = (value.Equals(NS_LITERAL_STRING("true")));

  if (newChecked == mChecked) {
    /* checked state didn't change */

    if (mType != eMenuType_Radio)
      return; // only Radio possibly cares about other kinds of change

    mContent->GetAttr(kNameSpaceID_None, nsHTMLAtoms::name, value);
    if (value == mGroupName)
      return;                   // no interesting change
  } else { 
    mChecked = newChecked;
    if (mType != eMenuType_Radio || !mChecked)
      /*
       * Unchecking something requires no further changes, and only
       * menuRadio has to do additional work when checked.
       */
      return;
  }

  /*
   * If we get this far, we're type=radio, and:
   * - our name= changed, or
   * - we went from checked="false" to checked="true"
   */

  if (!mChecked)
    /*
     * If we're not checked, then it must have been a name change, and a name
     * change on an unchecked item doesn't require any magic.
     */
    return;
  
  /*
   * Behavioural note:
   * If we're checked and renamed _into_ an existing radio group, we are
   * made the new checked item, and we unselect the previous one.
   *
   * The only other reasonable behaviour would be to check for another selected
   * item in that group.  If found, unselect ourselves, otherwise we're the
   * selected item.  That, however, would be a lot more work, and I don't think
   * it's better at all.
   */

  /* walk siblings, looking for the other checked item with the same name */
  nsIFrame *parent, *sib;
  nsIMenuFrame *sibMenu;
  nsMenuType sibType;
  nsAutoString sibGroup;
  PRBool sibChecked;
  
  nsresult rv = GetParent(&parent);
  NS_ASSERTION(NS_SUCCEEDED(rv), "couldn't get parent of radio menu frame\n");
  if (NS_FAILED(rv)) return;
  
  // get the first sibling in this menu popup. This frame may be it, and if we're
  // being called at creation time, this frame isn't yet in the parent's child list.
  // All I'm saying is that this may fail, but it's most likely alright.
  rv = parent->FirstChild(aPresContext, NULL, &sib);
  if ( NS_FAILED(rv) || !sib )
    return;

  // XXX - egcs 1.1.2 & gcc 2.95.x -Oy builds, where y > 1, 
  // are known to break if we declare nsCOMPtrs inside this loop.  
  // Moving the declaration out of the loop works around this problem.
  // http://bugzilla.mozilla.org/show_bug.cgi?id=80988

  nsCOMPtr<nsIContent> content;

  do {
    if (NS_FAILED(sib->QueryInterface(NS_GET_IID(nsIMenuFrame),
                                      (void **)&sibMenu)))
        continue;
        
    if (sibMenu != (nsIMenuFrame *)this &&        // correct way to check?
        (sibMenu->GetMenuType(sibType), sibType == eMenuType_Radio) &&
        (sibMenu->MenuIsChecked(sibChecked), sibChecked) &&
        (sibMenu->GetRadioGroupName(sibGroup), sibGroup == mGroupName)) {
      
      if (NS_FAILED(sib->GetContent(getter_AddRefs(content))))
        continue;             // break?
      
      /* uncheck the old item */
      content->UnsetAttr(kNameSpaceID_None, nsHTMLAtoms::checked,
                         PR_TRUE);

      /* XXX in DEBUG, check to make sure that there aren't two checked items */
      return;
    }

  } while(NS_SUCCEEDED(sib->GetNextSibling(&sib)) && sib);

}

void 
nsMenuFrame::BuildAcceleratorText()
{
  nsFrameState state;
  nsAutoString accelText;

  GetFrameState(&state);
  if ((state & NS_STATE_ACCELTEXT_IS_DERIVED) == 0) {
    mContent->GetAttr(kNameSpaceID_None, nsXULAtoms::acceltext, accelText);
    if (!accelText.IsEmpty())
      return;
  }
  // accelText is definitely empty here.

  // Now we're going to compute the accelerator text, so remember that we did.
  SetFrameState(state | NS_STATE_ACCELTEXT_IS_DERIVED);

  // If anything below fails, just leave the accelerator text blank.
  mContent->UnsetAttr(kNameSpaceID_None, nsXULAtoms::acceltext, PR_FALSE);

  // See if we have a key node and use that instead.
  nsAutoString keyValue;
  mContent->GetAttr(kNameSpaceID_None, nsXULAtoms::key, keyValue);
  if (keyValue.IsEmpty())
    return;

  nsCOMPtr<nsIDocument> document;
  mContent->GetDocument(*getter_AddRefs(document));

  // Turn the document into a XUL document so we can use getElementById
  nsCOMPtr<nsIDOMXULDocument> xulDocument(do_QueryInterface(document));
  if (!xulDocument)
    return;

  nsCOMPtr<nsIDOMElement> keyDOMElement;
  xulDocument->GetElementById(keyValue, getter_AddRefs(keyDOMElement));
  if (!keyDOMElement)
    return;

  nsCOMPtr<nsIContent> keyElement(do_QueryInterface(keyDOMElement));
  if (!keyElement)
    return;

  // get the string to display as accelerator text
  // check the key element's attributes in this order:
  // |keytext|, |key|, |keycode|
  nsAutoString accelString;
  keyElement->GetAttr(kNameSpaceID_None, nsXULAtoms::keytext, accelString);

  if (accelString.IsEmpty()) {
    keyElement->GetAttr(kNameSpaceID_None, nsXULAtoms::key, accelString);

    if (!accelString.IsEmpty()) {
      ToUpperCase(accelString);
    } else {
      nsAutoString keyCode;
      keyElement->GetAttr(kNameSpaceID_None, nsXULAtoms::keycode, keyCode);
      ToUpperCase(keyCode);

      nsresult rv;
      nsCOMPtr<nsIStringBundleService> bundleService(do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv));
      if (NS_SUCCEEDED(rv) && bundleService) {
        nsCOMPtr<nsIStringBundle> bundle;
        rv = bundleService->CreateBundle("chrome://global/locale/keys.properties",
                                         getter_AddRefs(bundle));

        if (NS_SUCCEEDED(rv) && bundle) {
          nsXPIDLString keyName;
          rv = bundle->GetStringFromName(keyCode.get(), getter_Copies(keyName));
          if (keyName)
            accelString = keyName;
        }
      }

      // nothing usable found, bail
      if (accelString.IsEmpty())
        return;
    }
  }

  static PRInt32 accelKey = 0;

  if (!accelKey)
  {
    // Compiled-in defaults, in case we can't get LookAndFeel --
    // command for mac, control for all other platforms.
#if defined(XP_MAC) || defined(XP_MACOSX)
    accelKey = nsIDOMKeyEvent::DOM_VK_META;
#else
    accelKey = nsIDOMKeyEvent::DOM_VK_CONTROL;
#endif

    // Get the accelerator key value from prefs, overriding the default:
    nsresult rv;
    nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
    if (NS_SUCCEEDED(rv) && prefs)
      rv = prefs->GetIntPref("ui.key.accelKey", &accelKey);
  }

  nsAutoString modifiers;
  keyElement->GetAttr(kNameSpaceID_None, nsXULAtoms::modifiers, modifiers);
  
  char* str = ToNewCString(modifiers);
  char* newStr;
  char* token = nsCRT::strtok(str, ", ", &newStr);
  while (token) {
      
    if (PL_strcmp(token, "shift") == 0)
      accelText += *gShiftText;
    else if (PL_strcmp(token, "alt") == 0) 
      accelText += *gAltText; 
    else if (PL_strcmp(token, "meta") == 0) 
      accelText += *gMetaText; 
    else if (PL_strcmp(token, "control") == 0) 
      accelText += *gControlText; 
    else if (PL_strcmp(token, "accel") == 0) {
      switch (accelKey)
      {
        case nsIDOMKeyEvent::DOM_VK_META:
          accelText += *gMetaText;
          break;

        case nsIDOMKeyEvent::DOM_VK_ALT:
          accelText += *gAltText;
          break;

        case nsIDOMKeyEvent::DOM_VK_CONTROL:
        default:
          accelText += *gControlText;
          break;
      }
    }
    
    accelText += *gModifierSeparator;

    token = nsCRT::strtok(newStr, ", ", &newStr);
  }

  nsMemory::Free(str);

  accelText += accelString;
  
  mContent->SetAttr(kNameSpaceID_None, nsXULAtoms::acceltext, accelText, PR_FALSE);
}

void
nsMenuFrame::Execute()
{
  // Temporarily disable rollup events on this menu.  This is
  // to suppress this menu getting removed in the case where
  // the oncommand handler opens a dialog, etc.
  if ( nsMenuFrame::sDismissalListener ) {
    nsMenuFrame::sDismissalListener->EnableListener(PR_FALSE);
  }

  // Get our own content node and hold on to it to keep it from going away.
  nsCOMPtr<nsIContent> content = dont_QueryInterface(mContent);

  // Deselect ourselves.
  SelectMenu(PR_FALSE);

  // Now hide all of the open menus.
  if (mMenuParent)
    mMenuParent->HideChain();

  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event;
  event.eventStructType = NS_EVENT;
  event.message = NS_XUL_COMMAND;
  event.isShift = PR_FALSE;
  event.isControl = PR_FALSE;
  event.isAlt = PR_FALSE;
  event.isMeta = PR_FALSE;
  event.clickCount = 0;
  event.widget = nsnull;
  // The order of the nsIViewManager and nsIPresShell COM pointers is
  // important below.  We want the pres shell to get released before the
  // associated view manager on exit from this function.
  // See bug 54233.
  nsCOMPtr<nsIViewManager> kungFuDeathGrip;
  nsCOMPtr<nsIPresShell> shell;
  nsresult result = mPresContext->GetShell(getter_AddRefs(shell));
  nsIFrame* me = this;
  if (NS_SUCCEEDED(result) && shell) {
    shell->GetViewManager(getter_AddRefs(kungFuDeathGrip));

    // See if we have a command elt.  If so, we execute on the command instead
    // of on our content element.
    nsAutoString command;
    mContent->GetAttr(kNameSpaceID_None, nsXULAtoms::command, command);
    if (!command.IsEmpty()) {
      nsCOMPtr<nsIDocument> doc;
      mContent->GetDocument(*getter_AddRefs(doc));
      nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(doc));
      nsCOMPtr<nsIDOMElement> commandElt;
      domDoc->GetElementById(command, getter_AddRefs(commandElt));
      nsCOMPtr<nsIContent> commandContent(do_QueryInterface(commandElt));
      if (commandContent)
        shell->HandleDOMEventWithTarget(commandContent, &event, &status);
      else
        NS_ASSERTION(PR_FALSE, "A XUL <menuitem> is attached to a command that doesn't exist! Unable to execute menu item!\n");
    }
    else
      shell->HandleDOMEventWithTarget(mContent, &event, &status);
  }

  // XXX HACK. Just gracefully exit if the node has been removed, e.g., window.close()
  // was executed.
  nsCOMPtr<nsIDocument> doc;
  content->GetDocument(*getter_AddRefs(doc));

  nsIFrame* primary = nsnull;
  if (shell) shell->GetPrimaryFrameFor(content, &primary);

  // Now properly close them all up.
  if (doc && (primary == me) && mMenuParent) // <-- HACK IS HERE. ICK.
    mMenuParent->DismissChain();
  // END HACK

  // Re-enable rollup events on this menu.
  if ( nsMenuFrame::sDismissalListener ) {
	nsMenuFrame::sDismissalListener->EnableListener(PR_TRUE);
  }
}

PRBool
nsMenuFrame::OnCreate()
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event;
  event.eventStructType = NS_EVENT;
  event.message = NS_XUL_POPUP_SHOWING;
  event.isShift = PR_FALSE;
  event.isControl = PR_FALSE;
  event.isAlt = PR_FALSE;
  event.isMeta = PR_FALSE;
  event.clickCount = 0;
  event.widget = nsnull;
  
  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  nsresult rv;
  nsCOMPtr<nsIPresShell> shell;
  rv = mPresContext->GetShell(getter_AddRefs(shell));
  if (NS_SUCCEEDED(rv) && shell) {
    if (child) {
      rv = shell->HandleDOMEventWithTarget(child, &event, &status);
    }
    else {
      rv = shell->HandleDOMEventWithTarget(mContent, &event, &status);
    }
  }

  if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
    return PR_FALSE;

  // The menu is going to show, and the create handler has executed.
  // We should now walk all of our menu item children, checking to see if any
  // of them has a command attribute.  If so, then several attributes must
  // potentially be updated.
  if (child) {
    nsCOMPtr<nsIDocument> doc;
    child->GetDocument(*getter_AddRefs(doc));
    nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(doc));

    PRInt32 count;
    child->ChildCount(count);
    for (PRInt32 i = 0; i < count; i++) {
      nsCOMPtr<nsIContent> grandChild;
      child->ChildAt(i, *getter_AddRefs(grandChild));
      nsCOMPtr<nsIAtom> tag;
      grandChild->GetTag(*getter_AddRefs(tag));
      if (tag.get() == nsXULAtoms::menuitem) {
        // See if we have a command attribute.
        nsAutoString command;
        grandChild->GetAttr(kNameSpaceID_None, nsXULAtoms::command, command);
        if (!command.IsEmpty()) {
          // We do! Look it up in our document
          nsCOMPtr<nsIDOMElement> commandElt;
          domDoc->GetElementById(command, getter_AddRefs(commandElt));
          nsCOMPtr<nsIContent> commandContent(do_QueryInterface(commandElt));

          if ( commandContent ) {
            nsAutoString commandAttr, menuAttr;
            commandContent->GetAttr(kNameSpaceID_None, nsHTMLAtoms::disabled, commandAttr);
            grandChild->GetAttr(kNameSpaceID_None, nsHTMLAtoms::disabled, menuAttr);
            if (!commandAttr.Equals(menuAttr)) {
              // The menu's disabled state needs to be updated to match the command.
              if (commandAttr.IsEmpty()) 
                grandChild->UnsetAttr(kNameSpaceID_None, nsHTMLAtoms::disabled, PR_TRUE);
              else grandChild->SetAttr(kNameSpaceID_None, nsHTMLAtoms::disabled, commandAttr, PR_TRUE);
            }

            // The menu's label, accesskey, and checked states need to be updated to match the command.
            // Note that (unlike the disabled state) if the command has *no* label for either, we
            // assume the menu is supplying its own.
            commandContent->GetAttr(kNameSpaceID_None, nsHTMLAtoms::checked, commandAttr);
            grandChild->GetAttr(kNameSpaceID_None, nsHTMLAtoms::checked, menuAttr);
            if (!commandAttr.Equals(menuAttr)) {
              if (!commandAttr.IsEmpty()) 
                grandChild->SetAttr(kNameSpaceID_None, nsHTMLAtoms::checked, commandAttr, PR_TRUE);
            }

            commandContent->GetAttr(kNameSpaceID_None, nsHTMLAtoms::accesskey, commandAttr);
            grandChild->GetAttr(kNameSpaceID_None, nsHTMLAtoms::accesskey, menuAttr);
            if (!commandAttr.Equals(menuAttr)) {
              if (!commandAttr.IsEmpty()) 
                grandChild->SetAttr(kNameSpaceID_None, nsHTMLAtoms::accesskey, commandAttr, PR_TRUE);
            }

            commandContent->GetAttr(kNameSpaceID_None, nsXULAtoms::label, commandAttr);
            grandChild->GetAttr(kNameSpaceID_None, nsXULAtoms::label, menuAttr);
            if (!commandAttr.Equals(menuAttr)) {
              if (!commandAttr.IsEmpty()) 
                grandChild->SetAttr(kNameSpaceID_None, nsXULAtoms::label, commandAttr, PR_TRUE);
            }
          }
        }
      }
    }
  }

  return PR_TRUE;
}

PRBool
nsMenuFrame::OnCreated()
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event;
  event.eventStructType = NS_EVENT;
  event.message = NS_XUL_POPUP_SHOWN;
  event.isShift = PR_FALSE;
  event.isControl = PR_FALSE;
  event.isAlt = PR_FALSE;
  event.isMeta = PR_FALSE;
  event.clickCount = 0;
  event.widget = nsnull;
  
  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  nsresult rv;
  nsCOMPtr<nsIPresShell> shell;
  rv = mPresContext->GetShell(getter_AddRefs(shell));
  if (NS_SUCCEEDED(rv) && shell) {
    if (child) {
      rv = shell->HandleDOMEventWithTarget(child, &event, &status);
    }
    else {
      rv = shell->HandleDOMEventWithTarget(mContent, &event, &status);
    }
  }

  if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
    return PR_FALSE;
  return PR_TRUE;
}

PRBool
nsMenuFrame::OnDestroy()
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event;
  event.eventStructType = NS_EVENT;
  event.message = NS_XUL_POPUP_HIDING;
  event.isShift = PR_FALSE;
  event.isControl = PR_FALSE;
  event.isAlt = PR_FALSE;
  event.isMeta = PR_FALSE;
  event.clickCount = 0;
  event.widget = nsnull;
  
  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  nsresult rv;
  nsCOMPtr<nsIPresShell> shell;
  rv = mPresContext->GetShell(getter_AddRefs(shell));
  if (NS_SUCCEEDED(rv) && shell) {
    if (child) {
      rv = shell->HandleDOMEventWithTarget(child, &event, &status);
    }
    else {
      rv = shell->HandleDOMEventWithTarget(mContent, &event, &status);
    }
  }

  if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
    return PR_FALSE;
  return PR_TRUE;
}

PRBool
nsMenuFrame::OnDestroyed()
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event;
  event.eventStructType = NS_EVENT;
  event.message = NS_XUL_POPUP_HIDDEN;
  event.isShift = PR_FALSE;
  event.isControl = PR_FALSE;
  event.isAlt = PR_FALSE;
  event.isMeta = PR_FALSE;
  event.clickCount = 0;
  event.widget = nsnull;
  
  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  nsresult rv;
  nsCOMPtr<nsIPresShell> shell;
  rv = mPresContext->GetShell(getter_AddRefs(shell));
  if (NS_SUCCEEDED(rv) && shell) {
    if (child) {
      rv = shell->HandleDOMEventWithTarget(child, &event, &status);
    }
    else {
      rv = shell->HandleDOMEventWithTarget(mContent, &event, &status);
    }
  }

  if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
    return PR_FALSE;
  return PR_TRUE;
}

NS_IMETHODIMP
nsMenuFrame::RemoveFrame(nsIPresContext* aPresContext,
                           nsIPresShell& aPresShell,
                           nsIAtom* aListName,
                           nsIFrame* aOldFrame)
{
  nsresult  rv;

  if (mPopupFrames.ContainsFrame(aOldFrame)) {
    // Go ahead and remove this frame.
    mPopupFrames.DestroyFrame(aPresContext, aOldFrame);
    nsBoxLayoutState state(aPresContext);
    rv = MarkDirtyChildren(state);
  } else {
    rv = nsBoxFrame::RemoveFrame(aPresContext, aPresShell, aListName, aOldFrame);
  }

  return rv;
}

NS_IMETHODIMP
nsMenuFrame::InsertFrames(nsIPresContext* aPresContext,
                            nsIPresShell& aPresShell,
                            nsIAtom* aListName,
                            nsIFrame* aPrevFrame,
                            nsIFrame* aFrameList)
{
  nsCOMPtr<nsIAtom> tag;
  nsresult          rv;

  nsCOMPtr<nsIMenuParent> menuPar(do_QueryInterface(aFrameList));
  if (menuPar) {
    nsCOMPtr<nsIBox> menupopup(do_QueryInterface(aFrameList));
    NS_ASSERTION(menupopup,"Popup is not a box!!!");
    menupopup->SetParentBox(this);
    mPopupFrames.InsertFrames(nsnull, nsnull, aFrameList);

    nsBoxLayoutState state(aPresContext);
    SetDebug(state, aFrameList, mState & NS_STATE_CURRENTLY_IN_DEBUG);
    rv = MarkDirtyChildren(state);
  } else {
    rv = nsBoxFrame::InsertFrames(aPresContext, aPresShell, aListName, aPrevFrame, aFrameList);  
  }

  return rv;
}

NS_IMETHODIMP
nsMenuFrame::AppendFrames(nsIPresContext* aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aFrameList)
{
  if (!aFrameList)
    return NS_OK;

  nsCOMPtr<nsIAtom> tag;
  nsresult          rv;

  nsCOMPtr<nsIMenuParent> menuPar(do_QueryInterface(aFrameList));
  if (menuPar) {
    nsCOMPtr<nsIBox> menupopup(do_QueryInterface(aFrameList));
    NS_ASSERTION(menupopup,"Popup is not a box!!!");
    menupopup->SetParentBox(this);

    mPopupFrames.AppendFrames(nsnull, aFrameList);
    nsBoxLayoutState state(aPresContext);
    SetDebug(state, aFrameList, mState & NS_STATE_CURRENTLY_IN_DEBUG);
    rv = MarkDirtyChildren(state);
  } else {
    rv = nsBoxFrame::AppendFrames(aPresContext, aPresShell, aListName, aFrameList); 
  }

  return rv;
}

void
nsMenuFrame::UpdateDismissalListener(nsIMenuParent* aMenuParent)
{
  if (!nsMenuFrame::sDismissalListener) {
    if (!aMenuParent)
       return;
    // Create the listener and attach it to the outermost window.
    aMenuParent->CreateDismissalListener();
  }
  
  // Make sure the menu dismissal listener knows what the current
  // innermost menu popup frame is.
  nsMenuFrame::sDismissalListener->SetCurrentMenuParent(aMenuParent);
}

NS_IMETHODIMP
nsMenuFrame::GetPrefSize(nsBoxLayoutState& aState, nsSize& aSize)
{
  aSize.width = 0;
  aSize.height = 0;
  nsresult rv = nsBoxFrame::GetPrefSize(aState, aSize);

  if (IsSizedToPopup(mContent, PR_FALSE)) {
    nsSize tmpSize(-1,0);
    nsIBox::AddCSSPrefSize(aState, this, tmpSize);
    nscoord flex;
    GetFlex(aState, flex);

    if (tmpSize.width == -1 && flex==0) {
      nsIFrame* frame = mPopupFrames.FirstChild();
      if (!frame) {
        MarkAsGenerated();
        frame = mPopupFrames.FirstChild();
        // No child - just return
        if (!frame) return NS_OK;
      }
    
      nsIBox* ibox = nsnull;
      nsresult rv2 = frame->QueryInterface(NS_GET_IID(nsIBox), (void**)&ibox);
      NS_ASSERTION(NS_SUCCEEDED(rv2) && ibox,"popupChild is not box!!");

      ibox->GetPrefSize(aState, tmpSize);
      aSize.width = tmpSize.width;
    }
  }

  return rv;
}

NS_IMETHODIMP
nsMenuFrame::GetActiveChild(nsIDOMElement** aResult)
{
  nsIFrame* frame = mPopupFrames.FirstChild();
  nsMenuPopupFrame* menuPopup = (nsMenuPopupFrame*)frame;
  if (!frame)
    return NS_ERROR_FAILURE;

  nsIMenuFrame* menuFrame;
  menuPopup->GetCurrentMenuItem(&menuFrame);
  
  if (!menuFrame) {
    *aResult = nsnull;
  }
  else {
    nsIFrame* f;
    menuFrame->QueryInterface(NS_GET_IID(nsIFrame), (void**)&f);
    nsCOMPtr<nsIContent> c;
    f->GetContent(getter_AddRefs(c));
    nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(c));
    *aResult = elt;
    NS_IF_ADDREF(*aResult);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::SetActiveChild(nsIDOMElement* aChild)
{
  nsIFrame* frame = mPopupFrames.FirstChild();
  nsMenuPopupFrame* menuPopup = (nsMenuPopupFrame*)frame;
  if (!frame)
    return NS_ERROR_FAILURE;

  if (!aChild) {
    // Remove the current selection
    menuPopup->SetCurrentMenuItem(nsnull);
    return NS_OK;
  }

  nsCOMPtr<nsIContent> child(do_QueryInterface(aChild));
  
  nsCOMPtr<nsIPresShell> shell;
  mPresContext->GetShell(getter_AddRefs(shell));
  nsIFrame* kid;
  shell->GetPrimaryFrameFor(child, &kid);
  if (!kid)
    return NS_ERROR_FAILURE;
  nsCOMPtr<nsIMenuFrame> menuFrame(do_QueryInterface(kid));
  if (!menuFrame)
    return NS_ERROR_FAILURE;
  menuPopup->SetCurrentMenuItem(menuFrame);
  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::GetScrollableView(nsIScrollableView** aView)
{
  *aView = nsnull;
  if (!mPopupFrames.FirstChild())
    return NS_OK;

  nsMenuPopupFrame* popup = (nsMenuPopupFrame*) mPopupFrames.FirstChild();
  nsIFrame* childFrame = nsnull;
  popup->FirstChild(mPresContext, nsnull, &childFrame);
  if (childFrame) {
    *aView = popup->GetScrollableView(childFrame);
    nsRect itemRect;
    childFrame->GetRect(itemRect);
    (*aView)->SetLineHeight(itemRect.height);
  }

  return NS_OK;
}

/* Need to figure out what this does.
NS_IMETHODIMP
nsMenuFrame::GetBoxInfo(nsIPresContext* aPresContext, const nsHTMLReflowState& aReflowState, nsBoxInfo& aSize)
{
  nsresult rv = nsBoxFrame::GetBoxInfo(aPresContext, aReflowState, aSize);
  nsCOMPtr<nsIDOMXULMenuListElement> menulist(do_QueryInterface(mContent));
  if (menulist) {
    nsCalculatedBoxInfo boxInfo(this);
    boxInfo.prefSize.width = NS_UNCONSTRAINEDSIZE;
    boxInfo.prefSize.height = NS_UNCONSTRAINEDSIZE;
    boxInfo.flex = 0;
    GetRedefinedMinPrefMax(aPresContext, this, boxInfo);
    if (boxInfo.prefSize.width == NS_UNCONSTRAINEDSIZE &&
        boxInfo.prefSize.height == NS_UNCONSTRAINEDSIZE &&
        boxInfo.flex == 0) {
      nsIFrame* frame = mPopupFrames.FirstChild();
      if (!frame) {
        MarkAsGenerated();
        frame = mPopupFrames.FirstChild();
      }
      
      nsCOMPtr<nsIBox> box(do_QueryInterface(frame));
      nsCalculatedBoxInfo childInfo(frame);
      box->GetBoxInfo(aPresContext, aReflowState, childInfo);
      GetRedefinedMinPrefMax(aPresContext, this, childInfo);
      aSize.prefSize.width = childInfo.prefSize.width;
    }

    // This retrieval guarantess that the selectedItem will
    // be set before we lay out.
    nsCOMPtr<nsIDOMElement> element;
    menulist->GetSelectedItem(getter_AddRefs(element));
  }
  return rv;
}
*/

