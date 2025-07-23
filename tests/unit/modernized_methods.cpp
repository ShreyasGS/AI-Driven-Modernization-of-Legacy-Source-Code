#include "modernized_methods.h"
#include "mock_nsTypedSelection.h"

namespace mozilla {

// Implementation of FetchAnchorParentModern for testing
Result<nsCOMPtr<nsIDOMNode>, nsresult>
FetchAnchorParentModern(nsTypedSelection* aSelection)
{
  // Check if the anchor node exists
  if (!aSelection->mAnchorNode) {
    // No anchor node, return nullptr (not an error, just no parent)
    return Ok(nullptr);
  }
  
  // Get the parent node of the anchor node
  nsCOMPtr<nsIDOMNode> parent;
  nsresult rv = aSelection->mAnchorNode->GetParentNode(getter_AddRefs(parent));
  
  // Check if getting the parent node succeeded
  if (NS_FAILED(rv)) {
    return Err(rv);
  }
  
  // Return the parent node (might be nullptr if the anchor node has no parent)
  return Ok(parent);
}

// Implementation of FetchAnchorOffsetModern for testing
Maybe<int32_t>
FetchAnchorOffsetModern(nsTypedSelection* aSelection)
{
  // Check if the anchor node exists
  if (!aSelection->mAnchorNode) {
    // No anchor node, return Nothing
    return Nothing<int32_t>();
  }
  
  // Return the anchor offset
  return Some(aSelection->mAnchorOffset);
}

} // namespace mozilla 