#include "mozilla/Result.h"

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

// Modernized implementation using Result type:
mozilla::Result<bool>
nsTypedSelection::AddItemResult(nsIDOMRange* aItem)
{
  if (!mRangeArray)
    return mozilla::Result<bool>(NS_ERROR_FAILURE);
  if (!aItem)
    return mozilla::Result<bool>(NS_ERROR_NULL_POINTER);
  
  nsCOMPtr<nsISupports> isupp;
  nsresult rv = aItem->QueryInterface(NS_GET_IID(nsISupports), getter_AddRefs(isupp));
  if (NS_FAILED(rv))
    return mozilla::Result<bool>(rv);
  
  rv = mRangeArray->AppendElement(isupp);
  if (NS_FAILED(rv))
    return mozilla::Result<bool>(rv);
  
  return mozilla::Result<bool>(true);
}

// Backward compatibility wrapper:
nsresult
nsTypedSelection::AddItem(nsIDOMRange* aItem)
{
  auto result = AddItemResult(aItem);
  if (!result.isOk())
    return result.unwrapErr();
  
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
*/ 