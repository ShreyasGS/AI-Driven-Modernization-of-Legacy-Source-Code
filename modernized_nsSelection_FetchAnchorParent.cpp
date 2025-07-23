#include "mozilla/Result.h"
#include "modernized_nsSelection.h"
#include "nsTypedSelection.h"
#include "nsIDOMNode.h"
#include "nsCOMPtr.h"

// Original implementation:
/*
NS_IMETHODIMP
nsTypedSelection::FetchAnchorParent(nsIDOMNode **_retval)
{
  if (!_retval)
    return NS_ERROR_NULL_POINTER;
  *_retval = nsnull;
  
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
*/

/**
 * FetchAnchorParentModern - Gets the parent node of the anchor node
 *
 * This modernized implementation uses Result<T> pattern to explicitly handle errors
 * and provide a more type-safe interface. It properly handles the case where the
 * anchor node doesn't exist or doesn't have a parent.
 *
 * @param aSelection  The selection object to query
 * @return            Result containing either the parent node or an error code
 *                    - On success: Result containing nsCOMPtr<nsIDOMNode> with the parent node
 *                    - On failure: Result containing error code or nullptr if no parent exists
 */
mozilla::Result<nsCOMPtr<nsIDOMNode>, nsresult>
mozilla::FetchAnchorParentModern(nsTypedSelection* aSelection)
{
  // Check if the anchor node exists
  if (!aSelection->mAnchorNode) {
    // No anchor node, return nullptr (not an error, just no parent)
    return mozilla::Ok(nullptr);
  }
  
  // Get the parent node of the anchor node
  nsCOMPtr<nsIDOMNode> parent;
  nsresult rv = aSelection->mAnchorNode->GetParentNode(getter_AddRefs(parent));
  
  // Check if getting the parent node succeeded
  if (NS_FAILED(rv)) {
    return mozilla::Err(rv);
  }
  
  // Return the parent node (might be nullptr if the anchor node has no parent)
  return mozilla::Ok(parent);
}

/**
 * FetchAnchorParent - Backward compatibility wrapper for FetchAnchorParentModern
 *
 * This method maintains the original API while using the modernized implementation
 * internally. It converts between the Result<nsCOMPtr<nsIDOMNode>> return type 
 * and the traditional nsresult return type with an out parameter.
 *
 * @param _retval  Out parameter to receive the parent node
 * @return         nsresult indicating success or failure
 */
NS_IMETHODIMP
nsTypedSelection::FetchAnchorParent(nsIDOMNode** _retval)
{
  // Validate the out parameter
  if (!_retval) {
    return NS_ERROR_NULL_POINTER;
  }
  
  // Initialize the out parameter to null
  *_retval = nullptr;
  
  // Call the modernized implementation
  auto result = mozilla::FetchAnchorParentModern(this);
  
  // Handle error case
  if (result.isErr()) {
    return result.unwrapErr();
  }
  
  // Get the node from the result
  nsCOMPtr<nsIDOMNode> node = result.unwrap();
  
  // If we have a node, transfer ownership to the out parameter
  if (node) {
    node.forget(_retval);
  }
  
  // Return success
  return NS_OK;
}

/*
Modernization changes:
1. Created a new function that returns a Result<nsCOMPtr<nsIDOMNode>> instead of using an out parameter
2. Added a backward compatibility wrapper with the original signature
3. Improved error handling with early returns
4. Added comprehensive documentation for both functions
5. Used nullptr instead of NULL/nsnull
6. Improved code formatting for better readability
7. Added explicit namespace qualification for Result, Ok, and Err
8. Simplified the logic for handling the parent node
*/ 