#ifndef modernized_methods_h
#define modernized_methods_h

#include "nsIDOMNode.h"
#include "nsCOMPtr.h"
#include "modernized_nsSelection_Result.h"

// Forward declarations
class nsTypedSelection;

namespace mozilla {

// FetchAnchorParentModern - Gets the parent node of the anchor node
Result<nsCOMPtr<nsIDOMNode>, nsresult> FetchAnchorParentModern(nsTypedSelection* aSelection);

// FetchAnchorOffsetModern - Gets the offset of the anchor node
Maybe<int32_t> FetchAnchorOffsetModern(nsTypedSelection* aSelection);

} // namespace mozilla

#endif // modernized_methods_h 