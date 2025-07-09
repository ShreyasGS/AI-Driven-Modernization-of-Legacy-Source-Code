#include "mozilla/Result.h"

// Original implementation:
/*
nsresult
nsTypedSelection::GetPresContext(nsIPresContext **aPresContext)
{
  if (!aPresContext)
    return NS_ERROR_NULL_POINTER;

  nsresult result = NS_ERROR_FAILURE;

  if (mFrameSelection)
  {
    nsCOMPtr<nsIFocusTracker> tracker;
    result = mFrameSelection->GetFocusTracker(getter_AddRefs(tracker));
    if (NS_SUCCEEDED(result))
    {
      result = tracker->GetPresContext(aPresContext);
    }
  }
  return result;
}
*/

// Modernized implementation using Result type:
mozilla::Result<nsCOMPtr<nsIPresContext>>
nsTypedSelection::GetPresContext()
{
  if (!mFrameSelection)
    return mozilla::Result<nsCOMPtr<nsIPresContext>>(NS_ERROR_FAILURE);

  nsCOMPtr<nsIFocusTracker> tracker;
  nsresult rv = mFrameSelection->GetFocusTracker(getter_AddRefs(tracker));
  if (NS_FAILED(rv))
    return mozilla::Result<nsCOMPtr<nsIPresContext>>(rv);

  nsCOMPtr<nsIPresContext> presContext;
  rv = tracker->GetPresContext(getter_AddRefs(presContext));
  if (NS_FAILED(rv))
    return mozilla::Result<nsCOMPtr<nsIPresContext>>(rv);

  return mozilla::Result<nsCOMPtr<nsIPresContext>>(presContext);
}

// Backward compatibility wrapper:
nsresult
nsTypedSelection::GetPresContext(nsIPresContext **aPresContext)
{
  if (!aPresContext)
    return NS_ERROR_NULL_POINTER;

  auto result = GetPresContext();
  if (!result.isOk())
    return result.unwrapErr();

  result.unwrap().forget(aPresContext);
  return NS_OK;
}

/*
Modernization changes:
1. Created a new method that returns a Result<nsCOMPtr<nsIPresContext>> instead of using an out parameter
2. Added a backward compatibility wrapper with the original signature
3. Used nsCOMPtr::forget() to transfer ownership to the caller without manual AddRef
4. Improved error handling with early returns
5. Improved code formatting for better readability
*/ 