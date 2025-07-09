#include "mozilla/Result.h"
#include <optional>

// Original implementation:
/*
nsresult
nsTypedSelection::RemoveItem(nsIDOMRange *aItem)
{
  if (!mRangeArray)
    return NS_ERROR_FAILURE;
  if (!aItem )
    return NS_ERROR_NULL_POINTER;
  PRUint32 cnt;
  nsresult rv = mRangeArray->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  for (PRUint32 i = 0; i < cnt;i++)
  {
    nsCOMPtr<nsISupports> indexIsupports = dont_AddRef(mRangeArray->ElementAt(i));
    nsCOMPtr<nsISupports> isupp;
    aItem->QueryInterface(NS_GET_IID(nsISupports),getter_AddRefs(isupp));
    if (isupp.get() == indexIsupports.get())
    {
      mRangeArray->RemoveElementAt(i);
      return NS_OK;
    }
  }
  return NS_COMFALSE;
}
*/

// Modernized implementation using Result type:
mozilla::Result<bool>
nsTypedSelection::RemoveItemResult(nsIDOMRange* aItem)
{
  if (!mRangeArray)
    return mozilla::Result<bool>(NS_ERROR_FAILURE);
  if (!aItem)
    return mozilla::Result<bool>(NS_ERROR_NULL_POINTER);
  
  PRUint32 cnt;
  nsresult rv = mRangeArray->Count(&cnt);
  if (NS_FAILED(rv))
    return mozilla::Result<bool>(rv);
  
  // Get the nsISupports interface from the range
  nsCOMPtr<nsISupports> isupp;
  rv = aItem->QueryInterface(NS_GET_IID(nsISupports), getter_AddRefs(isupp));
  if (NS_FAILED(rv))
    return mozilla::Result<bool>(rv);
  
  // Search for the item in the array
  for (PRUint32 i = 0; i < cnt; i++)
  {
    nsCOMPtr<nsISupports> indexIsupports = dont_AddRef(mRangeArray->ElementAt(i));
    if (isupp.get() == indexIsupports.get())
    {
      rv = mRangeArray->RemoveElementAt(i);
      if (NS_FAILED(rv))
        return mozilla::Result<bool>(rv);
      
      return mozilla::Result<bool>(true);
    }
  }
  
  // Item not found
  return mozilla::Result<bool>(false);
}

// Alternative implementation using std::optional:
std::optional<PRUint32>
nsTypedSelection::FindItemIndex(nsIDOMRange* aItem)
{
  if (!mRangeArray || !aItem)
    return std::nullopt;
  
  PRUint32 cnt;
  nsresult rv = mRangeArray->Count(&cnt);
  if (NS_FAILED(rv))
    return std::nullopt;
  
  // Get the nsISupports interface from the range
  nsCOMPtr<nsISupports> isupp;
  rv = aItem->QueryInterface(NS_GET_IID(nsISupports), getter_AddRefs(isupp));
  if (NS_FAILED(rv))
    return std::nullopt;
  
  // Search for the item in the array
  for (PRUint32 i = 0; i < cnt; i++)
  {
    nsCOMPtr<nsISupports> indexIsupports = dont_AddRef(mRangeArray->ElementAt(i));
    if (isupp.get() == indexIsupports.get())
    {
      return i;
    }
  }
  
  return std::nullopt;
}

// Backward compatibility wrapper:
nsresult
nsTypedSelection::RemoveItem(nsIDOMRange* aItem)
{
  auto result = RemoveItemResult(aItem);
  if (!result.isOk())
    return result.unwrapErr();
  
  // Original function returns NS_COMFALSE if item not found
  return result.unwrap() ? NS_OK : NS_COMFALSE;
}

/*
Modernization changes:
1. Created a new method that returns a Result<bool> instead of an nsresult
2. Added a backward compatibility wrapper with the original signature
3. Improved error handling with early returns
4. Moved the QueryInterface call outside the loop for better performance
5. Added explicit check for RemoveElementAt failure
6. Created an alternative FindItemIndex method using std::optional
7. Improved code formatting for better readability
*/ 