/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
 * The Original Code is the Mozilla browser.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications, Inc.  Portions created by Netscape are
 * Copyright (C) 1999, Mozilla.  All Rights Reserved.
 * 
 * Contributor(s):
 *   Radha Kulkarni <radha@netscape.com>
 */

// Local Includes
#include "nsSHTransaction.h"

//*****************************************************************************
//***    nsSHTransaction: Object Management
//*****************************************************************************

nsSHTransaction::nsSHTransaction() : mPersist(PR_TRUE), mPrev(nsnull) 
{
   NS_INIT_REFCNT();
}


nsSHTransaction::~nsSHTransaction()
{
}

//*****************************************************************************
//    nsSHTransaction: nsISupports
//*****************************************************************************

NS_IMPL_ADDREF(nsSHTransaction)
NS_IMPL_RELEASE(nsSHTransaction)

NS_INTERFACE_MAP_BEGIN(nsSHTransaction)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISHTransaction)
   NS_INTERFACE_MAP_ENTRY(nsISHTransaction)
NS_INTERFACE_MAP_END

//*****************************************************************************
//    nsSHTransaction: nsISHTransaction
//*****************************************************************************

NS_IMETHODIMP
nsSHTransaction::Create(nsISHEntry* aSHEntry, nsISHTransaction* aPrev)
{
   SetSHEntry(aSHEntry);
	if(aPrev)
      aPrev->SetNext(this);

   SetPrev(aPrev);
	return NS_OK;
}

NS_IMETHODIMP
nsSHTransaction::GetSHEntry(nsISHEntry ** aResult)
{
   NS_ENSURE_ARG_POINTER(aResult);
	*aResult = mSHEntry;
	NS_IF_ADDREF(*aResult);
	return NS_OK;
}


NS_IMETHODIMP
nsSHTransaction::SetSHEntry(nsISHEntry * aSHEntry)
{
	mSHEntry = aSHEntry;
	return NS_OK;
}


NS_IMETHODIMP
nsSHTransaction::GetNext(nsISHTransaction * * aResult)
{
   NS_ENSURE_ARG_POINTER(aResult);
   *aResult = mNext;
   NS_IF_ADDREF(*aResult);
   return NS_OK;
}


NS_IMETHODIMP
nsSHTransaction::SetNext(nsISHTransaction * aNext)
{
   if(mNext)
      {   
	   /* We do not want to maintain the previous traversals
	    * and make SessionHistory grow unboundewd for the 
		* seamonkey release. We will let go off 
		* all previous traversals. However, based on a pref
		* previous traversals can be maintained. The Pref
		* work will be done at a future date.Commenting off 
		* the following lines will delete previous traversals
		*/
#if 0
		// There is already a child. Move the child to the LRV list
      mLRVList = mNext;
#endif 

	   }

   NS_ENSURE_SUCCESS(aNext->SetPrev(this), NS_ERROR_FAILURE);

   mNext = aNext;
   return NS_OK;
}

NS_IMETHODIMP
nsSHTransaction::SetPrev(nsISHTransaction * aPrev)
{
	/* This is weak reference to parent. Do not Addref it */
     mPrev = aPrev;
	 return NS_OK;
}

nsresult
nsSHTransaction::GetPrev(nsISHTransaction ** aResult)
{
   NS_ENSURE_ARG_POINTER(aResult);
   *aResult  = mPrev;
   NS_IF_ADDREF(*aResult);
   return NS_OK;
}

NS_IMETHODIMP
nsSHTransaction::GetLrvList(nsISHTransaction ** aResult)
{
   NS_ENSURE_ARG_POINTER(aResult);
   *aResult = mLRVList;
   NS_IF_ADDREF(*aResult);
   return NS_OK;
}

NS_IMETHODIMP
nsSHTransaction::SetPersist(PRBool aPersist)
{
   mPersist = aPersist;
   return NS_OK;
}

NS_IMETHODIMP
nsSHTransaction::GetPersist(PRBool* aPersist)
{
   NS_ENSURE_ARG_POINTER(aPersist);

   *aPersist = mPersist;
   return NS_OK;
}
