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

#ifndef nsXULTooltipListener_h__
#define nsXULTooltipListener_h__

#include "nsIDOMMouseListener.h"
#include "nsIDOMMouseMotionListener.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMXULListener.h"
#include "nsIContent.h"
#include "nsIDOMElement.h"
#include "nsITimer.h"
#include "nsIRootBox.h"
#include "nsITreeBoxObject.h"
#include "nsCOMPtr.h"
#include "nsString.h"

class nsXULTooltipListener : public nsIDOMMouseListener,
                             public nsIDOMMouseMotionListener,
                             public nsIDOMKeyListener,
                             public nsIDOMXULListener
{
public:

  nsXULTooltipListener();
  virtual ~nsXULTooltipListener();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIDOMMouseListener
  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent) { return NS_OK; };
  NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent) { return NS_OK; };
  NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent) { return NS_OK; };
  NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent) { return NS_OK; };
  NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent);

  // nsIDOMMouseMotionListener
  NS_IMETHOD DragMove(nsIDOMEvent* aMouseEvent) { return NS_OK; };
  NS_IMETHOD MouseMove(nsIDOMEvent* aMouseEvent);

  // nsIDOMKeyListener
  NS_IMETHOD KeyDown(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyUp(nsIDOMEvent* aKeyEvent) { return NS_OK; };
  NS_IMETHOD KeyPress(nsIDOMEvent* aKeyEvent) { return NS_OK; };

  // nsIDOMXULListener
  NS_IMETHOD PopupShowing(nsIDOMEvent* aEvent) { return NS_OK; };
  NS_IMETHOD PopupShown(nsIDOMEvent* aEvent) { return NS_OK; };
  NS_IMETHOD PopupHiding(nsIDOMEvent* aEvent);
  NS_IMETHOD PopupHidden(nsIDOMEvent* aEvent) { return NS_OK; };
  NS_IMETHOD Close(nsIDOMEvent* aEvent) { return NS_OK; };
  NS_IMETHOD Command(nsIDOMEvent* aEvent) { return NS_OK; };
  NS_IMETHOD Broadcast(nsIDOMEvent* aEvent) { return NS_OK; };
  NS_IMETHOD CommandUpdate(nsIDOMEvent* aEvent) { return NS_OK; };

  // nsIDOMEventListener
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

  nsresult Init(nsIContent* aSourceNode, nsIRootBox* aRootBox);
  nsresult SetDefaultTooltip(nsIContent* aDefaultTooltip);
  nsresult GetDefaultTooltip(nsIContent** aDefaultTooltip);
  nsresult AddTooltipSupport(nsIContent* aNode);
  nsresult RemoveTooltipSupport(nsIContent* aNode);

protected:

  // pref callback for when the "show tooltips" pref changes
  static int sTooltipPrefChanged (const char* aPref, void* aData);
  static PRBool sShowTooltips;

  void KillTooltipTimer();
  void CreateAutoHideTimer();

  void CheckTreeBodyMove(nsIDOMMouseEvent* aMouseEvent);
  nsresult GetSourceTreeBoxObject(nsITreeBoxObject** aBoxObject);

  nsresult ShowTooltip();
  nsresult LaunchTooltip(nsIContent* aTarget, PRInt32 aX, PRInt32 aY);
  nsresult HideTooltip();
  nsresult DestroyTooltip();
  nsresult GetTooltipFor(nsIContent* aTarget, nsIContent** aTooltip);

  nsIRootBox* mRootBox;
  nsIContent* mSourceNode;
  nsCOMPtr<nsIContent> mTargetNode;
  nsCOMPtr<nsIContent> mCurrentTooltip;

  // a timer for showing the tooltip
  nsCOMPtr<nsITimer> mTooltipTimer;
  static void sTooltipCallback (nsITimer* aTimer, void* aListener);
  PRInt32 mMouseClientX, mMouseClientY;

  // a timer for auto-hiding the tooltip after a certain delay
  nsCOMPtr<nsITimer> mAutoHideTimer;
  static void sAutoHideCallback (nsITimer* aTimer, void* aListener);
  
  // various delays for tooltips
  enum {
    kTooltipAutoHideTime = 5000,       // 5000ms = 5 seconds
    kTooltipShowTime = 500             // 500ms = 0.5 seconds
  };

  // special members for handling trees
  PRBool mIsSourceTree;
  PRBool mNeedTitletip;
  PRInt32 mLastTreeRow;
  nsAutoString mLastTreeCol;
};

#endif // nsXULTooltipListener
