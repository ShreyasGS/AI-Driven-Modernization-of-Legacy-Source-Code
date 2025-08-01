/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Author:
 *   Simon Fraser <sfraser@netscape.com>
 */


#include "nsDocShellEnumerator.h"

#include "nsIDocShellTreeNode.h"

nsDocShellEnumerator::nsDocShellEnumerator(PRInt32 inEnumerationDirection)
: mRootItem(nsnull)
, mItemArray(nsnull)
, mCurIndex(0)
, mDocShellType(nsIDocShellTreeItem::typeAll)
, mEnumerationDirection(inEnumerationDirection)
{
  NS_INIT_REFCNT();
}

nsDocShellEnumerator::~nsDocShellEnumerator()
{
  delete mItemArray;
}

NS_IMPL_ISUPPORTS1(nsDocShellEnumerator, nsISimpleEnumerator);


/* nsISupports getNext (); */
NS_IMETHODIMP nsDocShellEnumerator::GetNext(nsISupports **outCurItem)
{
  NS_ENSURE_ARG_POINTER(outCurItem);
  *outCurItem = nsnull;
  
  nsresult rv = EnsureDocShellArray();
  if (NS_FAILED(rv)) return rv;
  
  if (mCurIndex >= 0 && mCurIndex < mItemArray->Count())
  {
    nsIDocShellTreeItem* thisItem = NS_REINTERPRET_CAST(nsIDocShellTreeItem*, mItemArray->ElementAt(mCurIndex));
    rv = thisItem->QueryInterface(NS_GET_IID(nsISupports), (void **)outCurItem);
    if (NS_FAILED(rv)) return rv;
  }
  else
    return NS_ERROR_FAILURE;
  
  mCurIndex ++;
  
  return NS_OK;
}

/* boolean hasMoreElements (); */
NS_IMETHODIMP nsDocShellEnumerator::HasMoreElements(PRBool *outHasMore)
{
  NS_ENSURE_ARG_POINTER(outHasMore);
  *outHasMore = PR_FALSE;

  nsresult rv = EnsureDocShellArray();
  if (NS_FAILED(rv)) return rv;

  *outHasMore = (mCurIndex < mItemArray->Count());
  return NS_OK;
}

nsresult nsDocShellEnumerator::GetEnumerationRootItem(nsIDocShellTreeItem * *aEnumerationRootItem)
{
  NS_ENSURE_ARG_POINTER(aEnumerationRootItem);
  *aEnumerationRootItem = mRootItem;
  NS_IF_ADDREF(*aEnumerationRootItem);
  return NS_OK;
}

nsresult nsDocShellEnumerator::SetEnumerationRootItem(nsIDocShellTreeItem * aEnumerationRootItem)
{
  mRootItem = aEnumerationRootItem;
  ClearState();
  return NS_OK;
}

nsresult nsDocShellEnumerator::GetEnumDocShellType(PRInt32 *aEnumerationItemType)
{
  NS_ENSURE_ARG_POINTER(aEnumerationItemType);
  *aEnumerationItemType = mDocShellType;
  return NS_OK;
}

nsresult nsDocShellEnumerator::SetEnumDocShellType(PRInt32 aEnumerationItemType)
{
  mDocShellType = aEnumerationItemType;
  ClearState();
  return NS_OK;
}

nsresult nsDocShellEnumerator::First()
{
  mCurIndex = 0;
  return EnsureDocShellArray();
}

nsresult nsDocShellEnumerator::EnsureDocShellArray()
{
  if (!mItemArray)
  {
    mItemArray = new nsVoidArray;
    if (!mItemArray) return NS_ERROR_OUT_OF_MEMORY;
  
    return BuildDocShellArray(*mItemArray);
  }
  
  return NS_OK;
}

nsresult nsDocShellEnumerator::ClearState()
{
  delete mItemArray;
  mItemArray = nsnull;
  
  mCurIndex = 0;
  return NS_OK;
}

nsresult nsDocShellEnumerator::BuildDocShellArray(nsVoidArray& inItemArray)
{
  NS_ENSURE_TRUE(mRootItem, NS_ERROR_NOT_INITIALIZED);
  inItemArray.Clear();
  return BuildArrayRecursive(mRootItem, inItemArray);
}

nsresult nsDocShellForwardsEnumerator::BuildArrayRecursive(nsIDocShellTreeItem* inItem, nsVoidArray& inItemArray)
{
  nsresult rv;
  nsCOMPtr<nsIDocShellTreeNode> itemAsNode = do_QueryInterface(inItem, &rv);
  if (NS_FAILED(rv)) return rv;

  PRInt32   itemType;
  // add this item to the array
  if ((mDocShellType == nsIDocShellTreeItem::typeAll) ||
      (NS_SUCCEEDED(inItem->GetItemType(&itemType)) && (itemType == mDocShellType)))
  {
    rv = inItemArray.AppendElement((void *)inItem);
    if (NS_FAILED(rv)) return rv;
  }

  PRInt32   numChildren;
  rv = itemAsNode->GetChildCount(&numChildren);
  if (NS_FAILED(rv)) return rv;
  
  for (PRInt32 i = 0; i < numChildren; ++i)
  {
    nsCOMPtr<nsIDocShellTreeItem> curChild;
    rv = itemAsNode->GetChildAt(i, getter_AddRefs(curChild));
    if (NS_FAILED(rv)) return rv;
      
    rv = BuildArrayRecursive(curChild, inItemArray);
    if (NS_FAILED(rv)) return rv;
  }

  return NS_OK;
}


nsresult nsDocShellBackwardsEnumerator::BuildArrayRecursive(nsIDocShellTreeItem* inItem, nsVoidArray& inItemArray)
{
  nsresult rv;
  nsCOMPtr<nsIDocShellTreeNode> itemAsNode = do_QueryInterface(inItem, &rv);
  if (NS_FAILED(rv)) return rv;

  PRInt32   numChildren;
  rv = itemAsNode->GetChildCount(&numChildren);
  if (NS_FAILED(rv)) return rv;
  
  for (PRInt32 i = numChildren - 1; i >= 0; --i)
  {
    nsCOMPtr<nsIDocShellTreeItem> curChild;
    rv = itemAsNode->GetChildAt(i, getter_AddRefs(curChild));
    if (NS_FAILED(rv)) return rv;
      
    rv = BuildArrayRecursive(curChild, inItemArray);
    if (NS_FAILED(rv)) return rv;
  }

  PRInt32   itemType;
  // add this item to the array
  if ((mDocShellType == nsIDocShellTreeItem::typeAll) ||
      (NS_SUCCEEDED(inItem->GetItemType(&itemType)) && (itemType == mDocShellType)))
  {
    rv = inItemArray.AppendElement((void *)inItem);
    if (NS_FAILED(rv)) return rv;
  }


  return NS_OK;
}


