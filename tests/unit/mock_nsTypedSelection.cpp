#include "mock_nsTypedSelection.h"

// Implementation of FetchAnchorParent for testing
NS_IMETHODIMP
nsTypedSelection::FetchAnchorParent(nsIDOMNode** _retval)
{
  if (!_retval)
    return NS_ERROR_NULL_POINTER;
  *_retval = nullptr;
  
  nsresult rv = NS_OK;
  if (mAnchorNode) {
    nsCOMPtr<nsIDOMNode> parent;
    rv = mAnchorNode->GetParentNode(getter_AddRefs(parent));
    if (NS_SUCCEEDED(rv) && parent) {
      *_retval = parent;
      NS_ADDREF(*_retval);
    }
  }
  return rv;
}

// Implementation of FetchAnchorOffset for testing
NS_IMETHODIMP
nsTypedSelection::FetchAnchorOffset(int32_t* _retval)
{
  if (!_retval)
    return NS_ERROR_NULL_POINTER;
  *_retval = 0;
  
  if (mAnchorNode) {
    *_retval = mAnchorOffset;
  }
  return NS_OK;
} 