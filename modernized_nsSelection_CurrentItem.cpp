#include "mozilla/Result.h"
#include "modernized_nsSelection.h"
#include "nsTypedSelection.h"
#include "nsIDOMRange.h"
#include "nsCOMPtr.h"

// Original implementation:
/*
nsIDOMRange*
nsTypedSelection::CurrentItem()
{
  // If we don't have an mRangeArray, then we don't have a current item
  if (!mRangeArray)
    return nsnull;
    
  PRUint32 cnt;
  nsresult rv = mRangeArray->Count(&cnt);
  if (NS_FAILED(rv) || cnt <= 0)
    return nsnull;
    
  // We always work with the first range in the selection
  nsISupports* element = mRangeArray->ElementAt(0);
  if (!element)
    return nsnull;
    
  nsCOMPtr<nsIDOMRange> range = do_QueryInterface(element);
  return range;
}
*/

/**
 * CurrentItemModern - Gets the current range in the selection
 *
 * This modernized implementation uses Result<T> pattern to explicitly handle errors
 * and provide a more type-safe interface. It checks for null inputs and properly
 * handles all error conditions.
 *
 * @param aSelection  The selection object to query
 * @return            Result containing either the range or an error code
 *                    - On success: Result containing nsCOMPtr<nsIDOMRange> with the current range
 *                                 (may be nullptr if no ranges exist)
 *                    - On failure: Result containing an error code
 */
mozilla::Result<nsCOMPtr<nsIDOMRange>, nsresult>
mozilla::CurrentItemModern(nsTypedSelection* aSelection)
{
  // Check if the selection is valid
  if (!aSelection)
    return mozilla::Err(NS_ERROR_INVALID_ARG);
    
  // Check if the range array exists
  if (!aSelection->mRangeArray)
    return mozilla::Ok(nsCOMPtr<nsIDOMRange>(nullptr));
    
  // Get the count of ranges
  PRUint32 cnt;
  nsresult rv = aSelection->mRangeArray->Count(&cnt);
  if (NS_FAILED(rv))
    return mozilla::Err(rv);
    
  // If there are no ranges, return null
  if (cnt <= 0)
    return mozilla::Ok(nsCOMPtr<nsIDOMRange>(nullptr));
    
  // Get the first range in the selection
  nsISupports* element = aSelection->mRangeArray->ElementAt(0);
  if (!element)
    return mozilla::Ok(nsCOMPtr<nsIDOMRange>(nullptr));
    
  // Convert the element to a range
  nsCOMPtr<nsIDOMRange> range = do_QueryInterface(element);
  
  // Return the range
  return mozilla::Ok(range);
}

/**
 * CurrentItem - Backward compatibility wrapper for CurrentItemModern
 *
 * This method maintains the original API while using the modernized implementation
 * internally. It converts between the Result<nsCOMPtr<nsIDOMRange>> return type 
 * and the traditional raw pointer return type.
 *
 * @return  nsIDOMRange pointer to the current range, or nullptr if no range exists
 */
nsIDOMRange*
nsTypedSelection::CurrentItem()
{
  // Call the modernized implementation
  auto result = mozilla::CurrentItemModern(this);
  
  // If there was an error, return null
  if (result.isErr())
    return nullptr;
    
  // Get the range from the result
  nsCOMPtr<nsIDOMRange> range = result.unwrap();
  
  // AddRef the range before returning it (the caller will own a reference)
  if (range)
    range.get()->AddRef();
    
  // Return the raw pointer
  return range;
}

/*
Modernization changes:
1. Created a new function that returns a Result<nsCOMPtr<nsIDOMRange>> instead of a raw pointer
2. Added a backward compatibility wrapper with the original signature
3. Improved error handling with early returns
4. Added comprehensive documentation for both functions
5. Used nullptr instead of NULL/nsnull
6. Improved code formatting for better readability
7. Added explicit namespace qualification for Result, Ok, and Err
8. Properly handled AddRef/Release semantics in the compatibility wrapper
*/ 