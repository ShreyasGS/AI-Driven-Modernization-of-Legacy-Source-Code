/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * Portions created by the Initial Developer are Copyright (C) 2001
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

#ifndef _nsMsgThreadedDBView_H_
#define _nsMsgThreadedDBView_H_

#include "nsMsgDBView.h"

// this class should probably inherit from the class that
// implements the tree. Since I don't know what that is yet,
// I'll just make it inherit from nsMsgDBView for now.
class nsMsgThreadedDBView : public nsMsgDBView
{
public:
  nsMsgThreadedDBView();
  virtual ~nsMsgThreadedDBView();

  NS_IMETHOD Open(nsIMsgFolder *folder, nsMsgViewSortTypeValue sortType, nsMsgViewSortOrderValue sortOrder, nsMsgViewFlagsTypeValue viewFlags, PRInt32 *pCount);
  NS_IMETHOD Close();
  virtual nsresult AddKeys(nsMsgKey *pKeys, PRInt32 *pFlags, const char *pLevels, nsMsgViewSortTypeValue sortType, PRInt32 numKeysToAdd);
  NS_IMETHOD Sort(nsMsgViewSortTypeValue sortType, nsMsgViewSortOrderValue sortOrder);
  NS_IMETHOD GetViewType(nsMsgViewTypeValue *aViewType);
  NS_IMETHOD ReloadFolderAfterQuickSearch();
  NS_DECL_NSIMSGSEARCHNOTIFY

protected:
  virtual const char * GetViewName(void) {return "ThreadedDBView"; }
  nsresult InitThreadedView(PRInt32 *pCount);
  virtual nsresult OnNewHeader(nsMsgKey newKey, nsMsgKey aParentKey, PRBool ensureListed);
  virtual nsresult AddMsgToThreadNotInView(nsIMsgThread *threadHdr, nsIMsgDBHdr *msgHdr, PRBool ensureListed);
  nsresult ListThreadIds(nsMsgKey *startMsg, PRBool unreadOnly, nsMsgKey *pOutput, PRInt32 *pFlags, char *pLevels, 
									 PRInt32 numToList, PRInt32 *pNumListed, PRInt32 *pTotalHeaders);
  nsresult InitSort(nsMsgViewSortTypeValue sortType, nsMsgViewSortOrderValue sortOrder);
  nsresult ExpandAll();
  virtual void	    OnExtraFlagChanged(nsMsgViewIndex index, PRUint32 extraFlag);
  virtual void OnHeaderAddedOrDeleted();
	void			ClearPrevIdArray();
  void      SavePreSearchInfo();
  void      ClearPreSearchInfo();
  void      UpdateCachedFlag(PRUint32 aFlag, PRUint32 *extraFlag);
  void      UpdatePreSearchFlagInfo(nsMsgViewIndex index, PRUint32 extraFlag);
  virtual nsresult RemoveByIndex(nsMsgViewIndex index);
  nsMsgViewIndex GetInsertInfoForNewHdr(nsIMsgDBHdr *newHdr, nsMsgViewIndex threadIndex, PRInt32 targetLevel);

  // these are used to save off the previous view so that bopping back and forth
  // between two views is quick (e.g., threaded and flat sorted by date).
	PRBool		m_havePrevView;
	nsMsgKeyArray		m_prevKeys;   //this is used for caching non-threaded view.
	nsUInt32Array m_prevFlags;
	nsUint8Array m_prevLevels;
  nsMsgKeyArray m_preSearchKeys;  //this is used for caching view before issuing search.
  nsUInt32Array m_preSearchFlags;
  nsUint8Array m_preSearchLevels; 
  nsCOMPtr <nsISimpleEnumerator> m_threadEnumerator;
};

#endif
