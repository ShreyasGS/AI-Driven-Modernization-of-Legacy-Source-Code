#include <optional>
#include "mozilla/Result.h"

// Original implementation:
/*
nsresult
nsTypedSelection::FetchDesiredX(nscoord &aDesiredX)
{
  if (!mFrameSelection)
    return NS_ERROR_NULL_POINTER;
  if (!mDesiredXSet)
  {
    aDesiredX = 0;
    return NS_OK;
  }
  aDesiredX = mDesiredX;
  return NS_OK;
}
*/

// Modernized implementation using std::optional:
std::optional<nscoord>
nsTypedSelection::GetDesiredX()
{
  if (!mFrameSelection)
    return std::nullopt;
  
  if (!mDesiredXSet)
    return 0; // Return 0 as default value when not set
  
  return mDesiredX;
}

// Backward compatibility wrapper:
nsresult
nsTypedSelection::FetchDesiredX(nscoord &aDesiredX)
{
  if (!mFrameSelection)
    return NS_ERROR_NULL_POINTER;
  
  auto result = GetDesiredX();
  if (!result)
    return NS_ERROR_FAILURE;
  
  aDesiredX = *result;
  return NS_OK;
}

// Alternative implementation using Result type:
mozilla::Result<nscoord>
nsTypedSelection::GetDesiredXResult()
{
  if (!mFrameSelection)
    return mozilla::Result<nscoord>(NS_ERROR_NULL_POINTER);
  
  if (!mDesiredXSet)
    return mozilla::Result<nscoord>(0); // Return 0 as default value when not set
  
  return mozilla::Result<nscoord>(mDesiredX);
}

/*
Modernization changes:
1. Created a new method that returns std::optional<nscoord> instead of using an out parameter
2. Added a backward compatibility wrapper with the original signature
3. Also provided an alternative implementation using Result<nscoord>
4. Improved error handling with early returns
5. Improved code formatting for better readability
6. Used std::nullopt to represent the absence of a value
*/ 