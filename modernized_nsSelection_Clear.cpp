#include "mozilla/Result.h"

// Original implementation:
/*
nsresult
nsTypedSelection::Clear(nsIPresContext* aPresContext)
{
  setAnchorFocusRange(-1);
  if (!mRangeArray)
    return NS_ERROR_FAILURE;
  // Get an iterator
  while (PR_TRUE)
  {
    PRUint32 cnt;
    nsresult rv = mRangeArray->Count(&cnt);
    if (NS_FAILED(rv)) return rv;
    if (cnt == 0)
      break;
    nsCOMPtr<nsISupports> isupportsindex = dont_AddRef(mRangeArray->ElementAt(0));
    nsCOMPtr<nsIDOMRange> range = do_QueryInterface(isupportsindex);
    mRangeArray->RemoveElementAt(0);
    selectFrames(aPresContext, range, 0);
    // Does RemoveElementAt also delete the elements?
  }
  // Reset direction so for more dependable table selection range handling
  SetDirection(eDirNext);
  return NS_OK;
}
*/

// Modernized implementation using Result type:
mozilla::Result<void>
nsTypedSelection::ClearResult(nsIPresContext* aPresContext)
{
  setAnchorFocusRange(-1);
  
  if (!mRangeArray)
    return mozilla::Result<void>(NS_ERROR_FAILURE);
  
  // Clear all ranges
  while (true)
  {
    PRUint32 cnt;
    nsresult rv = mRangeArray->Count(&cnt);
    if (NS_FAILED(rv))
      return mozilla::Result<void>(rv);
    
    if (cnt == 0)
      break;
    
    // Get the first range
    nsCOMPtr<nsISupports> isupportsindex = dont_AddRef(mRangeArray->ElementAt(0));
    nsCOMPtr<nsIDOMRange> range = do_QueryInterface(isupportsindex);
    
    // Remove it from the array
    rv = mRangeArray->RemoveElementAt(0);
    if (NS_FAILED(rv))
      return mozilla::Result<void>(rv);
    
    // Update the display
    if (aPresContext) {
      rv = selectFrames(aPresContext, range, false);
      if (NS_FAILED(rv))
        return mozilla::Result<void>(rv);
    }
  }
  
  // Reset direction for more dependable table selection range handling
  SetDirection(eDirNext);
  
  return mozilla::Result<void>();
}

// Backward compatibility wrapper:
nsresult
nsTypedSelection::Clear(nsIPresContext* aPresContext)
{
  auto result = ClearResult(aPresContext);
  if (!result.isOk())
    return result.unwrapErr();
  
  return NS_OK;
}

/*
Modernization changes:
1. Created a new method that returns a Result<void> instead of an nsresult
2. Added a backward compatibility wrapper with the original signature
3. Improved error handling with early returns
4. Added explicit check for RemoveElementAt failure
5. Added null check for aPresContext before calling selectFrames
6. Changed PR_TRUE to C++ standard true
7. Added comments to clarify the code
8. Improved code formatting for better readability
*/ 