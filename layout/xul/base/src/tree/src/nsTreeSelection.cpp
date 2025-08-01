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
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 *   Brian Ryner <bryner@netscape.com>
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

#include "nsCOMPtr.h"
#include "nsTreeSelection.h"
#include "nsIBoxObject.h"
#include "nsITreeBoxObject.h"
#include "nsITreeView.h"
#include "nsString.h"
#include "nsIDOMElement.h"
#include "nsIDOMClassInfo.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsGUIEvent.h"
#include "nsINameSpaceManager.h"
#include "nsXULAtoms.h"

// A helper class for managing our ranges of selection.
struct nsTreeRange
{
  nsTreeSelection* mSelection;

  nsTreeRange* mPrev;
  nsTreeRange* mNext;

  PRInt32 mMin;
  PRInt32 mMax;

  nsTreeRange(nsTreeSelection* aSel, PRInt32 aSingleVal)
    :mSelection(aSel), mPrev(nsnull), mNext(nsnull), mMin(aSingleVal), mMax(aSingleVal) {};
  nsTreeRange(nsTreeSelection* aSel, PRInt32 aMin, PRInt32 aMax) 
    :mSelection(aSel), mPrev(nsnull), mNext(nsnull), mMin(aMin), mMax(aMax) {};

  ~nsTreeRange() { delete mNext; };

  void Connect(nsTreeRange* aPrev = nsnull, nsTreeRange* aNext = nsnull) {
    if (aPrev)
      aPrev->mNext = this;
    else
      mSelection->mFirstRange = this;

    if (aNext)
      aNext->mPrev = this;

    mPrev = aPrev;
    mNext = aNext;
  };

  void RemoveRange(PRInt32 aStart, PRInt32 aEnd) {
    // Do the start and end encompass us?
    if (mMin >= aStart && mMax <= aEnd) {
      // They do.  We should simply be excised from the list.
      if (mPrev)
        mPrev->mNext = mNext;
      else
        mSelection->mFirstRange = mNext;
        
      nsTreeRange* next = mNext;
      if (next)
        next->mPrev = mPrev;
      mPrev = mNext = nsnull;
      delete this;
      if (next)
        next->RemoveRange(aStart, aEnd);
      return;
    }
    // See if this range overlaps.
    else if (aStart >= mMin && aStart <= mMax) {
      // We start within this range.
      // Do we also end within this range?
      if (aEnd >= mMin && aEnd <= mMax) {
        // We do.  This range should be split into
        // two new ranges.
        PRInt32 aNewStart = aEnd+1;
        PRInt32 aNewEnd = mMax;
        mMax = aStart-1;
        nsTreeRange* range = new nsTreeRange(mSelection, aNewStart, aNewEnd);
        range->Connect(this, mNext);
        return; // We're done, since we were entirely contained within this range.
      }
      else {
        // The end goes outside our range.  We should
        // move our max down to before the start.
        mMax = aStart-1;
        if (mNext)
          mNext->RemoveRange(aStart, aEnd);
      }
    }
    else if (aEnd >= mMin && aEnd <= mMax) {
      // The start precedes our range, but the end is contained
      // within us.  Pull the range up past the end.
      mMin = aEnd+1;
      return; // We're done, since we contained the end.
    }
    else {
      // Neither the start nor the end was contained inside us, move on.
      if (mNext)
        mNext->RemoveRange(aStart, aEnd);
    }
  };

  void Remove(PRInt32 aIndex) {
    if (aIndex >= mMin && aIndex <= mMax) {
      // We have found the range that contains us.
      if (mMin == mMax) {
        // Delete the whole range.
        if (mPrev)
          mPrev->mNext = mNext;
        if (mNext)
          mNext->mPrev = mPrev;
        nsTreeRange* first = mSelection->mFirstRange;
        if (first == this)
          mSelection->mFirstRange = mNext;
        mNext = mPrev = nsnull;
        delete this;
      }
      else if (aIndex == mMin)
        mMin++;
      else if (aIndex == mMax)
        mMax--;
      else {
        // We have to break this range.
        nsTreeRange* newRange = new nsTreeRange(mSelection, aIndex + 1, mMax);
        newRange->Connect(this, mNext);
        mMax = aIndex - 1;
      }
    }
    else if (mNext)
      mNext->Remove(aIndex);
  };

  void Add(PRInt32 aIndex) {
    if (aIndex < mMin) {
      // We have found a spot to insert.
      if (aIndex + 1 == mMin)
        mMin = aIndex;
      else if (mPrev && mPrev->mMax+1 == aIndex)
        mPrev->mMax = aIndex;
      else {
        // We have to create a new range.
        nsTreeRange* newRange = new nsTreeRange(mSelection, aIndex);
        newRange->Connect(mPrev, this);
      }
    }
    else if (mNext)
      mNext->Add(aIndex);
    else {
      // Insert on to the end.
      if (mMax+1 == aIndex)
        mMax = aIndex;
      else {
        // We have to create a new range.
        nsTreeRange* newRange = new nsTreeRange(mSelection, aIndex);
        newRange->Connect(this, nsnull);
      }
    }
  };

  PRBool Contains(PRInt32 aIndex) {
    if (aIndex >= mMin && aIndex <= mMax)
      return PR_TRUE;

    if (mNext)
      return mNext->Contains(aIndex);

    return PR_FALSE;
  };

  PRInt32 Count() {
    PRInt32 total = mMax - mMin + 1;
    if (mNext)
      total += mNext->Count();
    return total;
  };

  void Invalidate() {
    mSelection->mTree->InvalidateRange(mMin, mMax);
    if (mNext)
      mNext->Invalidate();
  };

  void RemoveAllBut(PRInt32 aIndex) {
    if (aIndex >= mMin && aIndex <= mMax) {

      // Invalidate everything in this list.
      mSelection->mFirstRange->Invalidate();

      mMin = aIndex;
      mMax = aIndex;
      
      nsTreeRange* first = mSelection->mFirstRange;
      if (mPrev)
        mPrev->mNext = mNext;
      if (mNext)
        mNext->mPrev = mPrev;
      mNext = mPrev = nsnull;
      
      if (first != this) {
        delete mSelection->mFirstRange;
        mSelection->mFirstRange = this;
      }
    }
    else if (mNext)
      mNext->RemoveAllBut(aIndex);
  };

  void Insert(nsTreeRange* aRange) {
    if (mMin >= aRange->mMax)
      aRange->Connect(mPrev, this);
    else if (mNext)
      mNext->Insert(aRange);
    else 
      aRange->Connect(this, nsnull);
  };
};

nsTreeSelection::nsTreeSelection(nsITreeBoxObject* aTree)
{
  NS_INIT_ISUPPORTS();
  mTree = aTree;
  mSuppressed = PR_FALSE;
  mFirstRange = nsnull;
  mShiftSelectPivot = -1;
  mCurrentIndex = -1;
}

nsTreeSelection::~nsTreeSelection()
{
  delete mFirstRange;
}

// QueryInterface implementation for nsBoxObject
NS_INTERFACE_MAP_BEGIN(nsTreeSelection)
  NS_INTERFACE_MAP_ENTRY(nsITreeSelection)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_DOM_CLASSINFO(TreeSelection)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsTreeSelection)
NS_IMPL_RELEASE(nsTreeSelection)

NS_IMETHODIMP nsTreeSelection::GetTree(nsITreeBoxObject * *aTree)
{
  NS_IF_ADDREF(mTree);
  *aTree = mTree;
  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::SetTree(nsITreeBoxObject * aTree)
{
  mTree = aTree; // WEAK
  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::IsSelected(PRInt32 aIndex, PRBool* aResult)
{
  if (mFirstRange)
    *aResult = mFirstRange->Contains(aIndex);
  else
    *aResult = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::TimedSelect(PRInt32 aIndex, PRInt32 aMsec)
{
  PRBool suppressSelect = mSuppressed;

  if (aMsec != -1)
    mSuppressed = PR_TRUE;

  nsresult rv = Select(aIndex);
  if (NS_FAILED(rv))
    return rv;

  if (aMsec != -1) {
    mSuppressed = suppressSelect;
    if (!mSuppressed) {
      if (mSelectTimer)
        mSelectTimer->Cancel();

      mSelectTimer = do_CreateInstance("@mozilla.org/timer;1");
      mSelectTimer->Init(SelectCallback, this, aMsec, NS_PRIORITY_HIGH);
    }
  }

  return NS_OK;
}

void
nsTreeSelection::SelectCallback(nsITimer *aTimer, void *aClosure)
{
  nsTreeSelection* self = NS_STATIC_CAST(nsTreeSelection*, aClosure);
  if (self) {
    self->FireOnSelectHandler();
    aTimer->Cancel();
    self->mSelectTimer = nsnull;
  }
}

NS_IMETHODIMP nsTreeSelection::Select(PRInt32 aIndex)
{
  mShiftSelectPivot = -1;

  SetCurrentIndex(aIndex);

  if (mFirstRange) {
    PRBool alreadySelected = mFirstRange->Contains(aIndex);

    if (alreadySelected) {
      PRInt32 count = mFirstRange->Count();
      if (count > 1) {
        // We need to deselect everything but our item.
        mFirstRange->RemoveAllBut(aIndex);
        FireOnSelectHandler();
      }
      return NS_OK;
    }
    else {
       // Clear out our selection.
       mFirstRange->Invalidate();
       delete mFirstRange;
    }
  }

  // Create our new selection.
  mFirstRange = new nsTreeRange(this, aIndex);
  mFirstRange->Invalidate();

  // Fire the select event
  FireOnSelectHandler();
  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::ToggleSelect(PRInt32 aIndex)
{
  // There are six cases that can occur on a ToggleSelect with our
  // range code.
  // (1) A new range should be made for a selection.
  // (2) A single range is removed from the selection.
  // (3) The item is added to an existing range.
  // (4) The item is removed from an existing range.
  // (5) The addition of the item causes two ranges to be merged.
  // (6) The removal of the item causes two ranges to be split.
  mShiftSelectPivot = -1;
  SetCurrentIndex(aIndex);

  if (!mFirstRange)
    Select(aIndex);
  else {
    if (!mFirstRange->Contains(aIndex)) {
      if (! SingleSelection())
        mFirstRange->Add(aIndex);
    }
    else
      mFirstRange->Remove(aIndex);
    
    mTree->InvalidateRow(aIndex);

    FireOnSelectHandler();
  }

  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::RangedSelect(PRInt32 aStartIndex, PRInt32 aEndIndex, PRBool aAugment)
{
  if ((mFirstRange || (aStartIndex != aEndIndex)) && SingleSelection())
    return NS_OK;

  if (!aAugment) {
    // Clear our selection.
    if (mFirstRange) {
        mFirstRange->Invalidate();
        delete mFirstRange;
    }
  }

  if (aStartIndex == -1) {
    if (mShiftSelectPivot != -1)
      aStartIndex = mShiftSelectPivot;
    else aStartIndex = mCurrentIndex;
  }

  mShiftSelectPivot = aStartIndex;
  SetCurrentIndex(aEndIndex);
  
  PRInt32 start = aStartIndex < aEndIndex ? aStartIndex : aEndIndex;
  PRInt32 end = aStartIndex < aEndIndex ? aEndIndex : aStartIndex;

  if (aAugment && mFirstRange) {
    // We need to remove all the items within our selected range from the selection,
    // and then we insert our new range into the list.
    mFirstRange->RemoveRange(start, end);
  }

  nsTreeRange* range = new nsTreeRange(this, start, end);
  range->Invalidate();

  if (aAugment && mFirstRange)
    mFirstRange->Insert(range);
  else
    mFirstRange = range;

  FireOnSelectHandler();

  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::ClearRange(PRInt32 aStartIndex, PRInt32 aEndIndex)
{
  SetCurrentIndex(aEndIndex);

  if (mFirstRange) {
    PRInt32 start = aStartIndex < aEndIndex ? aStartIndex : aEndIndex;
    PRInt32 end = aStartIndex < aEndIndex ? aEndIndex : aStartIndex;

    mFirstRange->RemoveRange(start, end);

    mTree->InvalidateRange(start, end);
  }
  
  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::ClearSelection()
{
  if (mFirstRange) {
    mFirstRange->Invalidate();
    delete mFirstRange;
    mFirstRange = nsnull;
  }
  mShiftSelectPivot = -1;

  FireOnSelectHandler();

  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::InvertSelection()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsTreeSelection::SelectAll()
{
  nsCOMPtr<nsITreeView> view;
  mTree->GetView(getter_AddRefs(view));
  if (!view)
    return NS_OK;

  PRInt32 rowCount;
  view->GetRowCount(&rowCount);
  if (rowCount == 0 || (rowCount > 1 && SingleSelection()))
    return NS_OK;

  mShiftSelectPivot = -1;

  // Invalidate not necessary when clearing selection, since 
  // we're going to invalidate the world on the SelectAll.
  delete mFirstRange;

  mFirstRange = new nsTreeRange(this, 0, rowCount-1);
  mFirstRange->Invalidate();

  FireOnSelectHandler();

  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::GetRangeCount(PRInt32* aResult)
{
  PRInt32 count = 0;
  nsTreeRange* curr = mFirstRange;
  while (curr) {
    count++;
    curr = curr->mNext;
  }

  *aResult = count;
  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::GetRangeAt(PRInt32 aIndex, PRInt32* aMin, PRInt32* aMax)
{
  *aMin = *aMax = -1;
  PRInt32 i = -1;
  nsTreeRange* curr = mFirstRange;
  while (curr) {
    i++;
    if (i == aIndex) {
      *aMin = curr->mMin;
      *aMax = curr->mMax;
      break;
    }
    curr = curr->mNext;
  }

  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::GetCount(PRInt32 *count)
{
  if (mFirstRange)
    *count = mFirstRange->Count();
  else // No range available, so there's no selected row.
    *count = 0;
  
  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::GetSelectEventsSuppressed(PRBool *aSelectEventsSuppressed)
{
  *aSelectEventsSuppressed = mSuppressed;
  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::SetSelectEventsSuppressed(PRBool aSelectEventsSuppressed)
{
  mSuppressed = aSelectEventsSuppressed;
  if (!mSuppressed)
    FireOnSelectHandler();
  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::GetCurrentIndex(PRInt32 *aCurrentIndex)
{
  *aCurrentIndex = mCurrentIndex;
  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::SetCurrentIndex(PRInt32 aIndex)
{
  if (mCurrentIndex != -1)
    mTree->InvalidateRow(mCurrentIndex);
  
  mCurrentIndex = aIndex;
  
  if (aIndex != -1)
    mTree->InvalidateRow(aIndex);

  return NS_OK;
}

#define ADD_NEW_RANGE(macro_range, macro_selection, macro_start, macro_end) \
  { \
    nsTreeRange* macro_new_range = new nsTreeRange(macro_selection, (macro_start), (macro_end)); \
    if (macro_range) \
      macro_range->Insert(macro_new_range); \
    else \
      macro_range = macro_new_range; \
  }

NS_IMETHODIMP
nsTreeSelection::AdjustSelection(PRInt32 aIndex, PRInt32 aCount)
{
  NS_ASSERTION(aCount != 0, "adjusting by zero");
  if (!aCount) return NS_OK;

  // adjust mShiftSelectPivot, if necessary
  if ((mShiftSelectPivot != 1) && (aIndex <= mShiftSelectPivot)) {
    // if we are deleting and the delete includes the shift select pivot, reset it
    if (aCount < 0 && (mShiftSelectPivot <= (aIndex -aCount -1))) {
        mShiftSelectPivot = -1;
    }
    else {
        mShiftSelectPivot += aCount;
    }
  }

  // adjust mCurrentIndex, if necessary
  if ((mCurrentIndex != -1) && (aIndex <= mCurrentIndex)) {
    // if we are deleting and the delete includes the current index, reset it
    if (aCount < 0 && (mCurrentIndex <= (aIndex -aCount -1))) {
        mCurrentIndex = -1;
    }
    else {
        mCurrentIndex += aCount;
    }
  }

  // no selection, so nothing to do.
  if (!mFirstRange) return NS_OK;

  nsTreeRange* newRange = nsnull;

  PRBool selChanged = PR_FALSE;
  nsTreeRange* curr = mFirstRange;
  while (curr) {
    if (aCount > 0) {
      // inserting
      if (aIndex > curr->mMax) {
        // adjustment happens after the range, so no change
        ADD_NEW_RANGE(newRange, this, curr->mMin, curr->mMax);
      }
      else if (aIndex <= curr->mMin) {  
        // adjustment happens before the start of the range, so shift down
        ADD_NEW_RANGE(newRange, this, curr->mMin + aCount, curr->mMax + aCount);
        selChanged = PR_TRUE;
      }
      else {
        // adjustment happen inside the range.
        // break apart the range and create two ranges
        ADD_NEW_RANGE(newRange, this, curr->mMin, aIndex - 1);
        ADD_NEW_RANGE(newRange, this, aIndex + aCount, curr->mMax + aCount);
        selChanged = PR_TRUE;
      }
    }
    else {
      // deleting
      if (aIndex > curr->mMax) {
        // adjustment happens after the range, so no change
        ADD_NEW_RANGE(newRange, this, curr->mMin, curr->mMax);
      }
      else {
        // remember, aCount is negative
        selChanged = PR_TRUE;
        PRInt32 lastIndexOfAdjustment = aIndex - aCount - 1;
        if (aIndex <= curr->mMin) {
          if (lastIndexOfAdjustment < curr->mMin) {
            // adjustment happens before the start of the range, so shift up
            ADD_NEW_RANGE(newRange, this, curr->mMin + aCount, curr->mMax + aCount);
          }
          else if (lastIndexOfAdjustment >= curr->mMax) {
            // adjustment contains the range.  remove the range by not adding it to the newRange
          }
          else {
            // adjustment starts before the range, and ends in the middle of it, so trim the range
            ADD_NEW_RANGE(newRange, this, aIndex, curr->mMax + aCount)
          }
        }
        else if (lastIndexOfAdjustment >= curr->mMax) {
         // adjustment starts in the middle of the current range, and contains the end of the range, so trim the range
         ADD_NEW_RANGE(newRange, this, curr->mMin, aIndex - 1)
        }
        else {
          // range contains the adjustment, so shorten the range
          ADD_NEW_RANGE(newRange, this, curr->mMin, curr->mMax + aCount)
        }
      }
    }
    curr = curr->mNext;
  }

  delete mFirstRange;
  mFirstRange = newRange;

  // Fire the select event
  if (selChanged)
    FireOnSelectHandler();

  return NS_OK;
}

NS_IMETHODIMP
nsTreeSelection::InvalidateSelection()
{
  if (mFirstRange)
    mFirstRange->Invalidate();
  return NS_OK;
}

NS_IMETHODIMP
nsTreeSelection::GetShiftSelectPivot(PRInt32* aIndex)
{
  *aIndex = mShiftSelectPivot;
  return NS_OK;
}

nsresult
nsTreeSelection::FireOnSelectHandler()
{
  if (mSuppressed)
    return NS_OK;

  nsCOMPtr<nsIBoxObject> boxObject = do_QueryInterface(mTree);
  nsCOMPtr<nsIDOMElement> elt;
  boxObject->GetElement(getter_AddRefs(elt));

  nsCOMPtr<nsIContent> content(do_QueryInterface(elt));
  nsCOMPtr<nsIDocument> document;
  content->GetDocument(*getter_AddRefs(document));
  
  // we might be firing on a delay, so it's possible in rare cases that
  // the document may have been destroyed by the time it fires
  if (!document)
    return NS_OK;

  PRInt32 count = document->GetNumberOfShells();
  for (PRInt32 i = 0; i < count; i++) {
    nsCOMPtr<nsIPresShell> shell;
    document->GetShellAt(i, getter_AddRefs(shell));
    if (!shell)
      continue;

    // Retrieve the context in which our DOM event will fire.
    nsCOMPtr<nsIPresContext> aPresContext;
    shell->GetPresContext(getter_AddRefs(aPresContext));

    nsEventStatus status = nsEventStatus_eIgnore;
    nsEvent event;
    event.eventStructType = NS_EVENT;
    event.message = NS_FORM_SELECTED;

    content->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
  }

  return NS_OK;
}

PRBool nsTreeSelection::SingleSelection()
{
  nsCOMPtr<nsIBoxObject> boxObject = do_QueryInterface(mTree);
  nsCOMPtr<nsIDOMElement> element;
  boxObject->GetElement(getter_AddRefs(element));
  nsCOMPtr<nsIContent> content = do_QueryInterface(element);
  nsAutoString seltype;
  content->GetAttr(kNameSpaceID_None, nsXULAtoms::seltype, seltype);
  if (seltype.Equals(NS_LITERAL_STRING("single")))
    return PR_TRUE;
  return PR_FALSE;
}


///////////////////////////////////////////////////////////////////////////////////

nsresult
NS_NewTreeSelection(nsITreeBoxObject* aTree, nsITreeSelection** aResult)
{
  *aResult = new nsTreeSelection(aTree);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult);
  return NS_OK;
}
