/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * Original Author:
 *   Seth Spitzer <sspitzer@netscape.com>
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

#include "nsAbView.h"
#include "nsISupports.h"
#include "nsIRDFService.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIAbCard.h"
#include "nsILocale.h"
#include "nsILocaleService.h"
#include "prmem.h"
#include "nsCollationCID.h"
#include "nsIAddrBookSession.h"
#include "nsAbBaseCID.h"

#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranchInternal.h"

#include "nsIAddrDatabase.h" // for kPriEmailColumn

#define CARD_NOT_FOUND -1
#define ALL_ROWS -1

#define PREF_MAIL_ADDR_BOOK_LASTNAMEFIRST "mail.addr_book.lastnamefirst"

// also, our default primary sort
#define GENERATED_NAME_COLUMN_ID "GeneratedName" 

static NS_DEFINE_CID(kCollationFactoryCID, NS_COLLATIONFACTORY_CID);

NS_IMPL_ISUPPORTS4(nsAbView, nsIAbView, nsITreeView, nsIAbListener, nsIObserver);

nsAbView::nsAbView()
{
  NS_INIT_ISUPPORTS();
  mMailListAtom = getter_AddRefs(NS_NewAtom("MailList"));
  mSuppressSelectionChange = PR_FALSE;
  mSuppressCountChange = PR_FALSE;
  mGeneratedNameFormat = 0;
}

nsAbView::~nsAbView()
{
  if (mDirectory) {
    nsresult rv;
    rv = Close();
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to close view");
  }
}

NS_IMETHODIMP nsAbView::Close()
{
  mURI = "";
  mDirectory = nsnull;
  mAbViewListener = nsnull;
  mTree = nsnull;
  mTreeSelection = nsnull;

  nsresult rv = NS_OK;

  rv = RemovePrefObservers();
  NS_ENSURE_SUCCESS(rv,rv);
  
  nsCOMPtr<nsIAddrBookSession> abSession = do_GetService(NS_ADDRBOOKSESSION_CONTRACTID, &rv); 
  NS_ENSURE_SUCCESS(rv,rv);

  rv = abSession->RemoveAddressBookListener(this);
  NS_ENSURE_SUCCESS(rv,rv);

  PRInt32 i = mCards.Count();
  while(i-- > 0)
  {
    rv = RemoveCardAt(i);
    NS_ASSERTION(NS_SUCCEEDED(rv), "remove card failed\n");
  }
  return NS_OK;
}

nsresult nsAbView::RemoveCardAt(PRInt32 row)
{
  nsresult rv;

  AbCard *abcard = (AbCard*) (mCards.ElementAt(row));
  NS_IF_RELEASE(abcard->card);
  mCards.RemoveElementAt(row);
  if (abcard->primaryCollationKey)
    nsMemory::Free(abcard->primaryCollationKey);
  if (abcard->secondaryCollationKey)
    nsMemory::Free(abcard->secondaryCollationKey);
  PR_FREEIF(abcard);

  if (mAbViewListener && !mSuppressCountChange) {
    rv = mAbViewListener->OnCountChanged(mCards.Count());
    NS_ENSURE_SUCCESS(rv,rv);
  }
  return NS_OK;
}

NS_IMETHODIMP nsAbView::GetURI(char **aURI)
{
  *aURI = ToNewCString(mURI);
  return NS_OK;
}

nsresult nsAbView::SetGeneratedNameFormatFromPrefs()
{
  nsresult rv;
  nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = prefs->GetBranch(nsnull, getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv,rv);

  rv = prefBranch->GetIntPref(PREF_MAIL_ADDR_BOOK_LASTNAMEFIRST, &mGeneratedNameFormat);
  NS_ENSURE_SUCCESS(rv, rv);
  return rv;
}

nsresult nsAbView::AddPrefObservers()
{
  nsresult rv;

  nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = prefs->GetBranch(nsnull, getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<nsIPrefBranchInternal> pbi = do_QueryInterface(prefBranch, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  rv = pbi->AddObserver(PREF_MAIL_ADDR_BOOK_LASTNAMEFIRST, this, PR_FALSE);
  NS_ENSURE_SUCCESS(rv,rv);
  return rv;
}

nsresult nsAbView::RemovePrefObservers()
{
  nsresult rv;

  nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = prefs->GetBranch(nsnull, getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<nsIPrefBranchInternal> pbi = do_QueryInterface(prefBranch, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  rv = pbi->RemoveObserver(PREF_MAIL_ADDR_BOOK_LASTNAMEFIRST, this);
  NS_ENSURE_SUCCESS(rv,rv);
  return rv;
}

NS_IMETHODIMP nsAbView::Init(const char *aURI, nsIAbViewListener *abViewListener, 
                             const PRUnichar *colID, const PRUnichar *sortDirection, PRUnichar **result)
{
  nsresult rv;

  NS_ENSURE_ARG_POINTER(result);

  mURI = aURI;
  mAbViewListener = abViewListener;

  rv = AddPrefObservers();
  NS_ENSURE_SUCCESS(rv,rv);

  rv = SetGeneratedNameFormatFromPrefs();
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr <nsIRDFService> rdfService = do_GetService("@mozilla.org/rdf/rdf-service;1",&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr <nsIRDFResource> resource;
  rv = rdfService->GetResource(aURI, getter_AddRefs(resource));
  NS_ENSURE_SUCCESS(rv, rv);

  mDirectory = do_QueryInterface(resource, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
    
  rv = EnumerateCards();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING(generatedNameColumnId, GENERATED_NAME_COLUMN_ID);

  // see if the persisted sortColumn is valid.
  // it may not be, if you migrated from older versions, or switched between
  // a mozilla build and a commercial build, which have different columns.
  nsAutoString actualSortColumn;
  if (!generatedNameColumnId.Equals(colID) && mCards.Count()) {
    nsIAbCard *card = ((AbCard *)(mCards.ElementAt(0)))->card;
    nsXPIDLString value;
    // XXX todo
    // need to check if _Generic is valid.  GetCardValue() will always return NS_OK for _Generic
    // we're going to have to ask mDirectory if it is.
    // it might not be.  example:  _ScreenName is valid in Netscape, but not Mozilla.
    rv = GetCardValue(card, colID, getter_Copies(value));
    if (NS_FAILED(rv))
      actualSortColumn = generatedNameColumnId.get();
    else
      actualSortColumn = colID; 
  }
  else {
    actualSortColumn = colID; 
  }

  rv = SortBy(actualSortColumn.get(), sortDirection);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<nsIAddrBookSession> abSession = do_GetService(NS_ADDRBOOKSESSION_CONTRACTID, &rv); 
  NS_ENSURE_SUCCESS(rv,rv);

  // this listener cares about all events
  rv = abSession->AddAddressBookListener(this, nsIAbListener::all);
  NS_ENSURE_SUCCESS(rv,rv);
  
  if (mAbViewListener && !mSuppressCountChange) {
    rv = mAbViewListener->OnCountChanged(mCards.Count());
    NS_ENSURE_SUCCESS(rv,rv);
  }

  *result = ToNewUnicode(actualSortColumn);
  return NS_OK;
}

NS_IMETHODIMP nsAbView::GetDirectory(nsIAbDirectory **aDirectory)
{
  NS_ENSURE_ARG_POINTER(aDirectory);
  NS_IF_ADDREF(*aDirectory = mDirectory);
  return NS_OK;
}

nsresult nsAbView::EnumerateCards()
{
  nsresult rv;    
  nsCOMPtr<nsIEnumerator> cardsEnumerator;
  nsCOMPtr<nsIAbCard> card;

  if (!mDirectory)
    return NS_ERROR_UNEXPECTED;

  rv = mDirectory->GetChildCards(getter_AddRefs(cardsEnumerator));
  if (NS_SUCCEEDED(rv) && cardsEnumerator)
  {
    nsCOMPtr<nsISupports> item;
    for (rv = cardsEnumerator->First(); NS_SUCCEEDED(rv); rv = cardsEnumerator->Next())
    {
      rv = cardsEnumerator->CurrentItem(getter_AddRefs(item));
      if (NS_SUCCEEDED(rv))
      {
        nsCOMPtr <nsIAbCard> card = do_QueryInterface(item);
        // malloc these from an arena
        AbCard *abcard = (AbCard *) PR_Calloc(1, sizeof(struct AbCard));
        if (!abcard) 
          return NS_ERROR_OUT_OF_MEMORY;

        abcard->card = card;
        NS_IF_ADDREF(abcard->card);

        // XXX todo
        // would it be better to do an insertion sort, than append and sort?
        // XXX todo
        // if we knew how many cards there was going to be
        // we could allocate an array of the size, 
        // instead of growing and copying as we append
        rv = mCards.AppendElement((void *)abcard);
        NS_ASSERTION(NS_SUCCEEDED(rv), "failed to append card");
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsAbView::GetRowCount(PRInt32 *aRowCount)
{
  *aRowCount = mCards.Count();
  return NS_OK;
}

NS_IMETHODIMP nsAbView::GetSelection(nsITreeSelection * *aSelection)
{
  NS_IF_ADDREF(*aSelection = mTreeSelection);
  return NS_OK;
}

NS_IMETHODIMP nsAbView::SetSelection(nsITreeSelection * aSelection)
{
  mTreeSelection = aSelection;
  return NS_OK;
}

NS_IMETHODIMP nsAbView::GetRowProperties(PRInt32 index, nsISupportsArray *properties)
{
    return NS_OK;
}

NS_IMETHODIMP nsAbView::GetCellProperties(PRInt32 row, const PRUnichar *colID, nsISupportsArray *properties)
{
  NS_ENSURE_TRUE(row >= 0, NS_ERROR_UNEXPECTED);

  if (mCards.Count() <= row)
    return NS_OK;

  // "G" == "GeneratedName"
  if (colID[0] != PRUnichar('G'))
    return NS_OK;

  nsIAbCard *card = ((AbCard *)(mCards.ElementAt(row)))->card;

  PRBool isMailList;
  nsresult rv = card->GetIsMailList(&isMailList);
  NS_ENSURE_SUCCESS(rv,rv);

  if (isMailList) {
    rv = properties->AppendElement(mMailListAtom);  
    NS_ENSURE_SUCCESS(rv,rv);
  }

  return NS_OK;
}

NS_IMETHODIMP nsAbView::GetColumnProperties(const PRUnichar *colID, nsIDOMElement *colElt, nsISupportsArray *properties)
{
    return NS_OK;
}

NS_IMETHODIMP nsAbView::IsContainer(PRInt32 index, PRBool *_retval)
{
    *_retval = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP nsAbView::IsContainerOpen(PRInt32 index, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::IsContainerEmpty(PRInt32 index, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::IsSeparator(PRInt32 index, PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsAbView::IsSorted(PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::CanDropOn(PRInt32 index, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::CanDropBeforeAfter(PRInt32 index, PRBool before, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::Drop(PRInt32 row, PRInt32 orientation)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::GetParentIndex(PRInt32 rowIndex, PRInt32 *_retval)
{
  *_retval = -1;
  return NS_OK;
}

NS_IMETHODIMP nsAbView::HasNextSibling(PRInt32 rowIndex, PRInt32 afterIndex, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::GetLevel(PRInt32 index, PRInt32 *_retval)
{
  *_retval = 0;
  return NS_OK;
}

NS_IMETHODIMP nsAbView::GetImageSrc(PRInt32 row, const PRUnichar *colID, nsAString& _retval)
{
  return NS_OK;
}

NS_IMETHODIMP nsAbView::GetProgressMode(PRInt32 row, const PRUnichar *colID, PRInt32* _retval)
{
  return NS_OK;
}

NS_IMETHODIMP nsAbView::GetCellValue(PRInt32 row, const PRUnichar *colID, nsAString& _retval)
{
  return NS_OK;
}

nsresult nsAbView::GetCardValue(nsIAbCard *card, const PRUnichar *colID, PRUnichar **_retval)
{
  nsresult rv;

  // "G" == "GeneratedName"
  // "_" == generic column (like _AimScreenName)
  // else, standard column (like PrimaryEmail)
  if (colID[0] == PRUnichar('G')) {
    // XXX todo 
    // cache the ab session?
    nsCOMPtr<nsIAddrBookSession> abSession = do_GetService(NS_ADDRBOOKSESSION_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv,rv);
    
    rv = abSession->GenerateNameFromCard(card, mGeneratedNameFormat, _retval);
    NS_ENSURE_SUCCESS(rv,rv);
  }
  else {
    if (colID[0] == PRUnichar('_'))
      rv = mDirectory->GetValueForCard(card, NS_LossyConvertUCS2toASCII(colID).get(), _retval);
    else 
      rv = card->GetCardValue(NS_LossyConvertUCS2toASCII(colID).get(), _retval);
  }
  return rv;
}

NS_IMETHODIMP nsAbView::GetCellText(PRInt32 row, const PRUnichar *colID, nsAString& _retval)
{
  NS_ENSURE_TRUE(row >= 0, NS_ERROR_UNEXPECTED);

  nsIAbCard *card = ((AbCard *)(mCards.ElementAt(row)))->card;
  // XXX fix me by converting GetCardValue to take an nsAString&
  nsXPIDLString cellText;
  nsresult rv = GetCardValue(card, colID, getter_Copies(cellText));
  _retval.Assign(cellText);
  return rv;
}

NS_IMETHODIMP nsAbView::SetTree(nsITreeBoxObject *tree)
{
  mTree = tree;
  return NS_OK;
}

NS_IMETHODIMP nsAbView::ToggleOpenState(PRInt32 index)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::CycleHeader(const PRUnichar *colID, nsIDOMElement *elt)
{
  return NS_OK;
}

nsresult nsAbView::InvalidateTree(PRInt32 row)
{
  if (!mTree)
    return NS_OK;
  
  if (row == ALL_ROWS)
    return mTree->Invalidate();
  else
    return mTree->InvalidateRow(row);
}

NS_IMETHODIMP nsAbView::SelectionChanged()
{
  if (mAbViewListener && !mSuppressSelectionChange) {
    nsresult rv = mAbViewListener->OnSelectionChanged();
    NS_ENSURE_SUCCESS(rv,rv);
  }
  return NS_OK;
}

NS_IMETHODIMP nsAbView::CycleCell(PRInt32 row, const PRUnichar *colID)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::IsEditable(PRInt32 row, const PRUnichar *colID, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::SetCellText(PRInt32 row, const PRUnichar *colID, const PRUnichar *value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::PerformAction(const PRUnichar *action)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::PerformActionOnRow(const PRUnichar *action, PRInt32 row)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::PerformActionOnCell(const PRUnichar *action, PRInt32 row, const PRUnichar *colID)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::GetCardFromRow(PRInt32 row, nsIAbCard **aCard)
{
  *aCard = nsnull;  
  if (mCards.Count() <= row) {
    return NS_OK;
  }

  NS_ENSURE_TRUE(row >= 0, NS_ERROR_UNEXPECTED);

  AbCard *a = ((AbCard *)(mCards.ElementAt(row)));
  if (!a)
      return NS_OK;

  NS_IF_ADDREF(*aCard = a->card);
  return NS_OK;
}

#define DESCENDING_SORT_FACTOR -1
#define ASCENDING_SORT_FACTOR 1

typedef struct SortClosure
{
  const PRUnichar *colID;
  PRInt32 factor;
  nsAbView *abView;
} SortClosure;

static int PR_CALLBACK
inplaceSortCallback(const void *data1, const void *data2, void *privateData)
{
  AbCard *card1 = (AbCard *)data1;
  AbCard *card2 = (AbCard *)data2;
  
  SortClosure *closure = (SortClosure *) privateData;
  
  PRInt32 sortValue;
  
  // if we are sorting the "PrimaryEmail", swap the collation keys, as the secondary is always the 
  // PrimaryEmail.  use the last primary key as the secondary key.
  //
  // "Pr" to distinguish "PrimaryEmail" from "PagerNumber"
  if (closure->colID[0] == PRUnichar('P') && closure->colID[1] == PRUnichar('r')) {
    sortValue = closure->abView->CompareCollationKeys(card1->secondaryCollationKey,card1->secondaryCollationKeyLen,card2->secondaryCollationKey,card2->secondaryCollationKeyLen);
    if (sortValue)
      return sortValue * closure->factor;
    else
      return closure->abView->CompareCollationKeys(card1->primaryCollationKey,card1->primaryCollationKeyLen,card2->primaryCollationKey,card2->primaryCollationKeyLen) * (closure->factor);
  }
  else {
    sortValue = closure->abView->CompareCollationKeys(card1->primaryCollationKey,card1->primaryCollationKeyLen,card2->primaryCollationKey,card2->primaryCollationKeyLen);
    if (sortValue)
      return sortValue * (closure->factor);
    else
      return closure->abView->CompareCollationKeys(card1->secondaryCollationKey,card1->secondaryCollationKeyLen,card2->secondaryCollationKey,card2->secondaryCollationKeyLen) * (closure->factor);
  }
}

static void SetSortClosure(const PRUnichar *sortColumn, const PRUnichar *sortDirection, nsAbView *abView, SortClosure *closure)
{
  closure->colID = sortColumn;
  
  if (sortDirection && !nsCRT::strcmp(sortDirection, NS_LITERAL_STRING("descending").get()))
    closure->factor = DESCENDING_SORT_FACTOR;
  else 
    closure->factor = ASCENDING_SORT_FACTOR;

  closure->abView = abView;
  return;
}

NS_IMETHODIMP nsAbView::SortBy(const PRUnichar *colID, const PRUnichar *sortDir)
{
  nsresult rv;

  PRInt32 count = mCards.Count();

  nsAutoString sortColumn;
  if (!colID) 
    sortColumn = NS_LITERAL_STRING(GENERATED_NAME_COLUMN_ID).get();  // default sort
  else
    sortColumn = colID;

  PRInt32 i;

  // if we are sorting by how we are already sorted, 
  // and just the sort direction changes, just reverse
  //
  // note, we'll call SortBy() with the existing sort column and the
  // existing sort direction, and that needs to do a complete resort.
  // for example, we do that when the PREF_MAIL_ADDR_BOOK_LASTNAMEFIRST changes
  if (!nsCRT::strcmp(mSortColumn.get(),sortColumn.get()) && !nsCRT::strcmp(mSortDirection.get(), sortDir)) {
    PRInt32 halfPoint = count / 2;
    for (i=0; i < halfPoint; i++) {
      // swap the elements.
      void *ptr1 = mCards.ElementAt(i);
      void *ptr2 = mCards.ElementAt(count - i - 1);
      mCards.ReplaceElementAt(ptr2, i);
      mCards.ReplaceElementAt(ptr1, count - i - 1);
    }

    mSortDirection = sortDir;
  }
  else {
    // generate collation keys
    for (i=0; i < count; i++) {
      AbCard *abcard = (AbCard *)(mCards.ElementAt(i));

      rv = GenerateCollationKeysForCard(sortColumn.get(), abcard);
      NS_ENSURE_SUCCESS(rv,rv);
    }

    nsAutoString sortDirection;
    if (!sortDir)
      sortDirection = NS_LITERAL_STRING("ascending").get();  // default direction
    else
      sortDirection = sortDir;

    SortClosure closure;
    SetSortClosure(sortColumn.get(), sortDirection.get(), this, &closure);
    
    nsCOMPtr <nsISupportsArray> selectedCards;
    rv = GetSelectedCards(getter_AddRefs(selectedCards));
    NS_ENSURE_SUCCESS(rv,rv);

    nsCOMPtr<nsIAbCard> indexCard;

    if (mTreeSelection) {
      PRInt32 currentIndex = -1;

      rv = mTreeSelection->GetCurrentIndex(&currentIndex);
      NS_ENSURE_SUCCESS(rv,rv);

      if (currentIndex != -1) {
        rv = GetCardFromRow(currentIndex, getter_AddRefs(indexCard));
        NS_ENSURE_SUCCESS(rv,rv);
      }
    }

    mCards.Sort(inplaceSortCallback, (void *)(&closure));
    
    rv = ReselectCards(selectedCards, indexCard);
    NS_ENSURE_SUCCESS(rv,rv);

    mSortColumn = sortColumn.get();
    mSortDirection = sortDirection.get();
  }

  rv = InvalidateTree(ALL_ROWS);
  NS_ENSURE_SUCCESS(rv,rv);
  return rv;
}

PRInt32 nsAbView::CompareCollationKeys(PRUint8 *key1, PRUint32 len1, PRUint8 *key2, PRUint32 len2)
{
  NS_ASSERTION(mCollationKeyGenerator, "no key generator");
  if (!mCollationKeyGenerator)
    return 0;

  PRInt32 result;

  nsresult rv = mCollationKeyGenerator->CompareRawSortKey(key1,len1,key2,len2,&result);
  NS_ASSERTION(NS_SUCCEEDED(rv), "key compare failed");
  if (NS_FAILED(rv))
    result = 0;
  return result;
}

nsresult nsAbView::GenerateCollationKeysForCard(const PRUnichar *colID, AbCard *abcard)
{
  nsresult rv;
  nsXPIDLString value;

  rv = GetCardValue(abcard->card, colID, getter_Copies(value));
  NS_ENSURE_SUCCESS(rv,rv);
  
  // XXX todo
  // be smarter about the allocation, use an arena?
  if (abcard->primaryCollationKey)
    nsMemory::Free(abcard->primaryCollationKey);
  rv = CreateCollationKey(value, &(abcard->primaryCollationKey), &(abcard->primaryCollationKeyLen));
  NS_ENSURE_SUCCESS(rv,rv);
  
  // XXX todo
  // fix me, do this with a const getter, to avoid the strcpy

  // hardcode email to be our secondary key
  rv = GetCardValue(abcard->card, NS_LITERAL_STRING(kPriEmailColumn).get(), getter_Copies(value));
  NS_ENSURE_SUCCESS(rv,rv);
  
  // XXX todo
  // be smarter about the allocation, use an arena?
  if (abcard->secondaryCollationKey)
    nsMemory::Free(abcard->secondaryCollationKey);
  rv = CreateCollationKey(value, &(abcard->secondaryCollationKey), &(abcard->secondaryCollationKeyLen));
  NS_ENSURE_SUCCESS(rv,rv);
  return rv;
}

nsresult nsAbView::CreateCollationKey(const PRUnichar *aSource, PRUint8 **aKey, PRUint32 *aKeyLen)
{
  NS_ENSURE_ARG_POINTER(aKey);
  NS_ENSURE_ARG_POINTER(aKeyLen);

  nsresult rv;
  if (!mCollationKeyGenerator)
  {
    nsCOMPtr<nsILocaleService> localeSvc = do_GetService(NS_LOCALESERVICE_CONTRACTID,&rv); 
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsILocale> locale; 
    rv = localeSvc->GetApplicationLocale(getter_AddRefs(locale));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr <nsICollationFactory> factory = do_CreateInstance(kCollationFactoryCID, &rv); 
    NS_ENSURE_SUCCESS(rv, rv);

    rv = factory->CreateCollation(locale, getter_AddRefs(mCollationKeyGenerator));
    NS_ENSURE_SUCCESS(rv, rv);
  }

        // XXX can we avoid this copy?
  nsAutoString sourceString(aSource);
  rv = mCollationKeyGenerator->GetSortKeyLen(kCollationCaseInSensitive, sourceString, aKeyLen);
  NS_ENSURE_SUCCESS(rv, rv);

  *aKey = (PRUint8*) nsMemory::Alloc (*aKeyLen);
  if (!aKey)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = mCollationKeyGenerator->CreateRawSortKey(kCollationCaseInSensitive, sourceString, *aKey, aKeyLen);
  NS_ASSERTION(NS_SUCCEEDED(rv), "failed to generate a raw sort key, see bug #121868");
  return NS_OK;
}

NS_IMETHODIMP nsAbView::OnItemAdded(nsISupports *parentDir, nsISupports *item)
{
  nsresult rv;
  nsCOMPtr <nsIAbDirectory> directory = do_QueryInterface(parentDir,&rv);
  NS_ENSURE_SUCCESS(rv,rv);

  if (directory.get() == mDirectory.get()) {
    nsCOMPtr <nsIAbCard> addedCard = do_QueryInterface(item);
    if (addedCard) {
      // malloc these from an arena
      AbCard *abcard = (AbCard *) PR_Calloc(1, sizeof(struct AbCard));
      if (!abcard) 
        return NS_ERROR_OUT_OF_MEMORY;

      abcard->card = addedCard;
      NS_IF_ADDREF(abcard->card);
    
      rv = GenerateCollationKeysForCard(mSortColumn.get(), abcard);
      NS_ENSURE_SUCCESS(rv,rv);

      PRInt32 index;
      rv = AddCard(abcard, PR_FALSE /* select card */, &index);
      NS_ENSURE_SUCCESS(rv,rv);
    }
  }
  return rv;
}

NS_IMETHODIMP nsAbView::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
  nsresult rv;

  if (!nsCRT::strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    nsDependentString prefName(someData);
    
    if (prefName.Equals(NS_LITERAL_STRING(PREF_MAIL_ADDR_BOOK_LASTNAMEFIRST))) {
      rv = SetGeneratedNameFormatFromPrefs();
      NS_ENSURE_SUCCESS(rv,rv);

      // the PREF_MAIL_ADDR_BOOK_LASTNAMEFIRST pref affects how the GeneratedName column looks.
      // so if the GeneratedName is our primary or secondary sort,
      // we need to resort.
      //
      // XXX optimize me
      // PrimaryEmail is always the secondary sort, unless it is currently the
      // primary sort.  So, if PrimaryEmail is the primary sort, 
      // GeneratedName might be the secondary sort.
      //
      // one day, we can get fancy and remember what the secondary sort is.
      // we we do that, we can fix this code.  at best, it will turn a sort into a invalidate.
      // 
      // if neither the primary nor the secondary sorts are GeneratedName, all we have to do is
      // invalidate (to show the new GeneratedNames), but the sort will not change.
      if (!nsCRT::strcmp(mSortColumn.get(), NS_LITERAL_STRING(GENERATED_NAME_COLUMN_ID).get()) ||
          !nsCRT::strcmp(mSortColumn.get(), NS_LITERAL_STRING(kPriEmailColumn).get())) {
        rv = SortBy(mSortColumn.get(), mSortDirection.get());
      }
      else {
        rv = InvalidateTree(ALL_ROWS);
      }
      NS_ENSURE_SUCCESS(rv,rv);
    }
  }
  return NS_OK;
}

nsresult nsAbView::AddCard(AbCard *abcard, PRBool selectCardAfterAdding, PRInt32 *index)
{
  nsresult rv = NS_OK;
  NS_ENSURE_ARG_POINTER(abcard);
  
  *index = FindIndexForInsert(abcard);
  rv = mCards.InsertElementAt((void *)abcard, *index);
  NS_ENSURE_SUCCESS(rv,rv);
    
  // this needs to happen after we insert the card, as RowCountChanged() will call GetRowCount()
  if (mTree)
    rv = mTree->RowCountChanged(*index, 1);

  if (selectCardAfterAdding && mTreeSelection) {
    mTreeSelection->SetCurrentIndex(*index);
    mTreeSelection->RangedSelect(*index, *index, PR_FALSE /* augment */);
  }

  if (mAbViewListener && !mSuppressCountChange) {
    rv = mAbViewListener->OnCountChanged(mCards.Count());
    NS_ENSURE_SUCCESS(rv,rv);
  }

  return rv;
}

PRInt32 nsAbView::FindIndexForInsert(AbCard *abcard)
{
  PRInt32 count = mCards.Count();
  PRInt32 i;

  void *item = (void *)abcard;
  
  SortClosure closure;
  SetSortClosure(mSortColumn.get(), mSortDirection.get(), this, &closure);
  
  // XXX todo
  // make this a binary search
  for (i=0; i < count; i++) {
    void *current = mCards.ElementAt(i);
    PRInt32 value = inplaceSortCallback(item, current, (void *)(&closure));
    // XXX fix me, this is not right for both ascending and descending
    if (value <= 0) 
      break;
  }
  return i;
}

NS_IMETHODIMP nsAbView::OnItemRemoved(nsISupports *parentDir, nsISupports *item)
{
  nsresult rv;

  nsCOMPtr <nsIAbDirectory> directory = do_QueryInterface(parentDir,&rv);
  NS_ENSURE_SUCCESS(rv,rv);

  if (directory.get() == mDirectory.get()) {
    rv = RemoveCardAndSelectNextCard(item);
    NS_ENSURE_SUCCESS(rv,rv);
  }
  return rv;
}

nsresult nsAbView::RemoveCardAndSelectNextCard(nsISupports *item)
{
  nsresult rv = NS_OK;
  nsCOMPtr <nsIAbCard> card = do_QueryInterface(item);
  if (card) {
    PRInt32 index = FindIndexForCard(card);
    if (index != CARD_NOT_FOUND) {
      PRBool selectNextCard = PR_FALSE;
      if (mTreeSelection) {
        PRInt32 selectedIndex;
        // XXX todo
        // make sure it works if nothing selected
        mTreeSelection->GetCurrentIndex(&selectedIndex);
        if (index == selectedIndex)
          selectNextCard = PR_TRUE;
      }

      rv = RemoveCardAt(index);
      NS_ENSURE_SUCCESS(rv,rv);

      // this needs to happen after we remove the card, as RowCountChanged() will call GetRowCount()
      if (mTree) {
        rv = mTree->RowCountChanged(index, -1);
      NS_ENSURE_SUCCESS(rv,rv);
      }

      if (selectNextCard) {
      PRInt32 count = mCards.Count();
      if (count && mTreeSelection) {
        // if we deleted the last card, adjust so we select the new "last" card
        if (index >= (count - 1)) {
          index = count -1;
        }
        mTreeSelection->SetCurrentIndex(index);
        mTreeSelection->RangedSelect(index, index, PR_FALSE /* augment */);
      }
    }
  }
  }
  return rv;
}

PRInt32 nsAbView::FindIndexForCard(nsIAbCard *card)
{
  PRInt32 count = mCards.Count();
  PRInt32 i;
 
  // you can't implement the binary search here, as all you have is the nsIAbCard
  // you might be here because one of the card properties has changed, and that property
  // could be the collation key.
  for (i=0; i < count; i++) {
    AbCard *abcard = (AbCard*) (mCards.ElementAt(i));
    PRBool equals;
    nsresult rv = card->Equals(abcard->card, &equals);
    if (NS_SUCCEEDED(rv) && equals) {
      return i;
    }
  }
  return CARD_NOT_FOUND;
}

NS_IMETHODIMP nsAbView::OnItemPropertyChanged(nsISupports *item, const char *property, const PRUnichar *oldValue, const PRUnichar *newValue)
{
  nsresult rv;

  nsCOMPtr <nsIAbCard> card = do_QueryInterface(item);
  if (!card)
    return NS_OK;

  PRInt32 index = FindIndexForCard(card);
  if (index == -1)
    return NS_OK;

  AbCard *oldCard = (AbCard*) (mCards.ElementAt(index));

  // malloc these from an arena
  AbCard *newCard = (AbCard *) PR_Calloc(1, sizeof(struct AbCard));
  if (!newCard)
    return NS_ERROR_OUT_OF_MEMORY;

  newCard->card = card;
  NS_IF_ADDREF(newCard->card);
    
  rv = GenerateCollationKeysForCard(mSortColumn.get(), newCard);
  NS_ENSURE_SUCCESS(rv,rv);

  if (!CompareCollationKeys(newCard->primaryCollationKey,newCard->primaryCollationKeyLen,oldCard->primaryCollationKey,oldCard->primaryCollationKeyLen)
    && CompareCollationKeys(newCard->secondaryCollationKey,newCard->secondaryCollationKeyLen,oldCard->secondaryCollationKey,oldCard->secondaryCollationKeyLen)) {
    // no need to remove and add, since the collation keys haven't change.
    // since they haven't chagned, the card will sort to the same place.
    // we just need to clean up what we allocated.
    NS_IF_RELEASE(newCard->card);
    if (newCard->primaryCollationKey)
      nsMemory::Free(newCard->primaryCollationKey);
    if (newCard->secondaryCollationKey)
      nsMemory::Free(newCard->secondaryCollationKey);
    PR_FREEIF(newCard);

    // still need to invalidate, as the other columns may have changed
    rv = InvalidateTree(index);
    NS_ENSURE_SUCCESS(rv,rv);
  }
  else {
    PRBool cardWasSelected = PR_FALSE;

    if (mTreeSelection) {
      rv = mTreeSelection->IsSelected(index, &cardWasSelected);
      NS_ENSURE_SUCCESS(rv,rv);
    }
    
    mSuppressSelectionChange = PR_TRUE;
    mSuppressCountChange = PR_TRUE;

    // remove the old card
    rv = RemoveCardAt(index);
    NS_ASSERTION(NS_SUCCEEDED(rv), "remove card failed\n");

    // add the card we created, and select it (to restore selection) if it was selected
    rv = AddCard(newCard, cardWasSelected /* select card */, &index);
    NS_ASSERTION(NS_SUCCEEDED(rv), "add card failed\n");

    mSuppressSelectionChange = PR_FALSE;
    mSuppressCountChange = PR_FALSE;

    // ensure restored selection is visible
    if (cardWasSelected && mTree) 
      mTree->EnsureRowIsVisible(index);
  }
  return NS_OK;
}

NS_IMETHODIMP nsAbView::SelectAll()
{
  if (mTreeSelection && mTree) {
    mTreeSelection->SelectAll();
    mTree->Invalidate();
  }
  return NS_OK;
}

NS_IMETHODIMP nsAbView::GetSortDirection(nsAString & aDirection)
{
  aDirection = mSortDirection;
  return NS_OK;
}

NS_IMETHODIMP nsAbView::GetSortColumn(nsAString & aColumn)
{
  aColumn = mSortColumn;
  return NS_OK;
}

nsresult nsAbView::ReselectCards(nsISupportsArray *cards, nsIAbCard *indexCard)
{
  PRUint32 count;
  PRUint32 i;

  if (!mTreeSelection || !cards)
    return NS_OK;

  nsresult rv = mTreeSelection->ClearSelection();
  NS_ENSURE_SUCCESS(rv,rv);

  rv = cards->Count(&count);
  NS_ENSURE_SUCCESS(rv, rv);

  for (i = 0; i < count; i++) {
    nsCOMPtr<nsISupports> cardSupports;
    cardSupports = getter_AddRefs(cards->ElementAt(i));
    nsCOMPtr <nsIAbCard> card = do_QueryInterface(cardSupports);
    if (card) {
      PRInt32 index = FindIndexForCard(card);
      if (index != CARD_NOT_FOUND) {
        mTreeSelection->RangedSelect(index, index, PR_TRUE /* augment */);
      }
    }
  }

  // reset the index card, and ensure it is visible
  if (indexCard) {
    PRInt32 currentIndex = FindIndexForCard(indexCard);
    rv = mTreeSelection->SetCurrentIndex(currentIndex);
    NS_ENSURE_SUCCESS(rv, rv);
  
    if (mTree) {
      rv = mTree->EnsureRowIsVisible(currentIndex);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsAbView::DeleteSelectedCards()
{
  nsCOMPtr <nsISupportsArray> cardsToDelete;
  
  nsresult rv = GetSelectedCards(getter_AddRefs(cardsToDelete));
  NS_ENSURE_SUCCESS(rv,rv);

  // mDirectory should not be null
  // bullet proof (and assert) to help figure out bug #127748
  NS_ENSURE_TRUE(mDirectory, NS_ERROR_UNEXPECTED);

  rv = mDirectory->DeleteCards(cardsToDelete);
  NS_ENSURE_SUCCESS(rv,rv);
  return rv;
}

nsresult nsAbView::GetSelectedCards(nsISupportsArray **selectedCards)
{
  *selectedCards = nsnull;
  if (!mTreeSelection)
    return NS_OK;
  
  PRInt32 selectionCount; 
  nsresult rv = mTreeSelection->GetRangeCount(&selectionCount);
  NS_ENSURE_SUCCESS(rv,rv);
  
  if (!selectionCount)
    return NS_OK;
  
  rv = NS_NewISupportsArray(selectedCards);
  NS_ENSURE_SUCCESS(rv,rv);
  
  for (PRInt32 i = 0; i < selectionCount; i++)
  {
    PRInt32 startRange;
    PRInt32 endRange;
    rv = mTreeSelection->GetRangeAt(i, &startRange, &endRange);
    NS_ENSURE_SUCCESS(rv, NS_OK); 
    PRInt32 totalCards = mCards.Count();
    if (startRange >= 0 && startRange < totalCards)
    {
      for (PRInt32 rangeIndex = startRange; rangeIndex <= endRange && rangeIndex < totalCards; rangeIndex++) {
        nsCOMPtr<nsIAbCard> abCard;
        rv = GetCardFromRow(rangeIndex, getter_AddRefs(abCard));
        NS_ENSURE_SUCCESS(rv,rv);
        
        nsCOMPtr<nsISupports> supports = do_QueryInterface(abCard, &rv);
        NS_ENSURE_SUCCESS(rv,rv);
        
        rv = (*selectedCards)->AppendElement(supports);
        NS_ENSURE_SUCCESS(rv,rv);
      }
    }
  }
  return NS_OK;
}

