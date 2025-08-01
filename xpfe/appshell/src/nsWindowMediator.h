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

#ifndef __nsWindowMediator_h
#define __nsWindowMediator_h

#include "nsCOMPtr.h"
#include "nsIWindowMediator.h"
#include "nsIWindowWatcher.h"
#include "nsVoidArray.h"
#include "nsXPIDLString.h"
#include "nsCRT.h"
#include "nsIRDFObserver.h"
#include "nsIRDFContainer.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFObserver.h"

class nsAppShellWindowEnumerator;
class nsASXULWindowEarlyToLateEnumerator;
class nsASDOMWindowEarlyToLateEnumerator;
class nsASDOMWindowFrontToBackEnumerator;
class nsASXULWindowFrontToBackEnumerator;
class nsASDOMWindowBackToFrontEnumerator;
class nsASXULWindowBackToFrontEnumerator;
struct nsWindowInfo;
struct PRLock;

class nsWindowMediator : public nsIWindowMediator,
                         public nsIRDFDataSource,
                         public nsIRDFObserver
{
friend class nsAppShellWindowEnumerator;
friend class nsASXULWindowEarlyToLateEnumerator;
friend class nsASDOMWindowEarlyToLateEnumerator;
friend class nsASDOMWindowFrontToBackEnumerator;
friend class nsASXULWindowFrontToBackEnumerator;
friend class nsASDOMWindowBackToFrontEnumerator;
friend class nsASXULWindowBackToFrontEnumerator;

public:
  nsWindowMediator();
  virtual ~nsWindowMediator();
  nsresult Init();

  NS_DECL_NSIWINDOWMEDIATOR
  
  // COM and RDF 
  NS_DECL_ISUPPORTS 

  // nsIRDFDataSource
  NS_DECL_NSIRDFDATASOURCE

  // nsIRDFObserver
  NS_DECL_NSIRDFOBSERVER

private:
  // Helper functions
  nsresult AddWindowToRDF( nsWindowInfo* ioWindowInfo );
  PRInt32 AddEnumerator( nsAppShellWindowEnumerator* inEnumerator );
  PRInt32 RemoveEnumerator( nsAppShellWindowEnumerator* inEnumerator);
  nsWindowInfo *MostRecentWindowInfo(const PRUnichar* inType);
  nsresult RemoveAndUpdateSynthetics(nsIRDFNode *node);

  NS_IMETHOD UnregisterWindow( nsWindowInfo *inInfo );

  nsVoidArray   mEnumeratorList;
  nsWindowInfo *mOldestWindow,
               *mTopmostWindow;
  PRInt32       mTimeStamp;
  PRInt32       mUpdateBatchNest;
  PRLock       *mListLock;
  nsCOMPtr<nsIWindowWatcher> mWatcher;
  nsCOMPtr<nsISupportsArray> mObservers;

  // pseudo-constants for RDF
  static nsIRDFResource* kNC_WindowMediatorRoot;
  static nsIRDFResource* kNC_Name;
  static nsIRDFResource* kNC_URL;
  static nsIRDFResource* kNC_KeyIndex;
  static PRInt32 gRefCnt;
  static nsIRDFContainer*  mContainer;
  static nsIRDFDataSource* mInner;
};

#endif
