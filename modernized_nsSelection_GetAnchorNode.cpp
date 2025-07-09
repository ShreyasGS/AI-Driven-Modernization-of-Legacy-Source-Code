#include "mozilla/Result.h"

// New method that returns a Result type
mozilla::Result<nsCOMPtr<nsIDOMNode>>
nsTypedSelection::GetAnchorNodeResult()
{
  if (!mAnchorFocusRange)
    return mozilla::Result<nsCOMPtr<nsIDOMNode>>(nsCOMPtr<nsIDOMNode>(nullptr));
   
  nsCOMPtr<nsIDOMNode> startNode;
  nsresult result = mAnchorFocusRange->GetStartContainer(getter_AddRefs(startNode));
  if (NS_FAILED(result)) 
    return mozilla::Result<nsCOMPtr<nsIDOMNode>>(result);
  if (!startNode) 
    return mozilla::Result<nsCOMPtr<nsIDOMNode>>(NS_ERROR_FAILURE);

  if (GetDirection() == eDirNext)
  {
    return mozilla::Result<nsCOMPtr<nsIDOMNode>>(startNode);
  }
  else
  {
    nsCOMPtr<nsIDOMNode> endNode;
    result = mAnchorFocusRange->GetEndContainer(getter_AddRefs(endNode));
    if (NS_FAILED(result)) 
      return mozilla::Result<nsCOMPtr<nsIDOMNode>>(result);
    if (!endNode) 
      return mozilla::Result<nsCOMPtr<nsIDOMNode>>(NS_ERROR_FAILURE);
    return mozilla::Result<nsCOMPtr<nsIDOMNode>>(endNode);
  }
}

// Backward compatibility wrapper:
NS_IMETHODIMP
nsTypedSelection::GetAnchorNode(nsIDOMNode** aAnchorNode)
{
  if (!aAnchorNode)
    return NS_ERROR_NULL_POINTER;
  
  *aAnchorNode = nullptr;
  
  auto result = GetAnchorNodeResult();
  if (!result.isOk())
    return result.unwrapErr();
  
  // If we have a successful result but it contains a null node,
  // that's still a success case (no anchor node)
  if (result.unwrap())
    result.unwrap().forget(aAnchorNode);
  
  return NS_OK;
}

/*
Modernization changes:
1. Created a new method that returns a Result<nsCOMPtr<nsIDOMNode>> instead of using an out parameter
2. Added a backward compatibility wrapper with the original signature
3. Used nsCOMPtr::forget() to transfer ownership to the caller without manual AddRef
4. Improved error handling with early returns
5. Improved code formatting for better readability
6. Properly handled the case where there is no anchor node (nullptr is a valid result)
*/ 