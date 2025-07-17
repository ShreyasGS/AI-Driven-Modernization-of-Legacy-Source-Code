#include "mozilla/Result.h"
#include "modernized_nsSelection.h"
#include "nsTypedSelection.h"
#include "nsISupports.h"

// Original implementation:
/*
nsresult
nsTypedSelection::AddItem(nsIDOMRange *aItem)
{
  if (!mRangeArray)
    return NS_ERROR_FAILURE;
  if (!aItem)
    return NS_ERROR_NULL_POINTER;
  nsresult result;
  nsCOMPtr<nsISupports> isupp = do_QueryInterface(aItem, &result);
  if (NS_SUCCEEDED(result)) {
    result = mRangeArray->AppendElement(isupp);
  }
  return result;
}
*/

/**
 * AddItemResult - Adds a range to the selection
 *
 * This modernized implementation uses Result<T> pattern to explicitly handle errors
 * and provide a more type-safe interface. It checks for null inputs and properly
 * handles all error conditions.
 *
 * @param aItem  The range to add to the selection
 * @return       Result<bool> containing either success (true) or an error code
 *               - On success: Result containing true
 *               - On failure: Result containing one of these error codes:
 *                   NS_ERROR_FAILURE - If mRangeArray is null
 *                   NS_ERROR_NULL_POINTER - If aItem is null
 *                   Other nsresult values from QueryInterface or AppendElement
 */
mozilla::Result<bool>
nsTypedSelection::AddItemResult(nsIDOMRange* aItem)
{
  // Check if the range array exists
  if (!mRangeArray)
    return mozilla::Result<bool>(NS_ERROR_FAILURE);
  
  // Validate input parameter
  if (!aItem)
    return mozilla::Result<bool>(NS_ERROR_NULL_POINTER);
  
  // Convert the range to an nsISupports interface
  nsCOMPtr<nsISupports> isupp;
  nsresult rv = aItem->QueryInterface(NS_GET_IID(nsISupports), getter_AddRefs(isupp));
  if (NS_FAILED(rv))
    return mozilla::Result<bool>(rv);
  
  // Add the range to the array
  rv = mRangeArray->AppendElement(isupp);
  if (NS_FAILED(rv))
    return mozilla::Result<bool>(rv);
  
  // Return success
  return mozilla::Result<bool>(true);
}

/**
 * AddItem - Backward compatibility wrapper for AddItemResult
 *
 * This method maintains the original API while using the modernized implementation
 * internally. It converts between the Result<bool> return type and the traditional
 * nsresult return type.
 *
 * @param aItem  The range to add to the selection
 * @return       nsresult indicating success or failure
 *               - NS_OK on success
 *               - Error code on failure
 */
nsresult
nsTypedSelection::AddItem(nsIDOMRange* aItem)
{
  // Call the modernized implementation
  auto result = AddItemResult(aItem);
  
  // Handle error case
  if (!result.isOk())
    return result.unwrapErr();
  
  // Return success
  return NS_OK;
}

/*
Modernization changes:
1. Created a new method that returns a Result<bool> instead of an nsresult
2. Added a backward compatibility wrapper with the original signature
3. Improved error handling with early returns
4. Used getter_AddRefs instead of passing a pointer to result
5. Replaced do_QueryInterface with direct QueryInterface call
6. Added explicit check for AppendElement failure
7. Improved code formatting for better readability
8. Added comprehensive documentation for both functions
*/ 