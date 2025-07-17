#ifndef modernized_nsSelection_h
#define modernized_nsSelection_h

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsIDOMNode.h"
#include "nsIDOMRange.h"
#include "nsIPresContext.h"
#include "modernized_nsSelection_Result.h"

using mozilla::Result;
using mozilla::Ok;
using mozilla::Err;

// Forward declarations
class nsTypedSelection;
class nsIPresContext;

// Modernized methods for nsTypedSelection
// These methods use modern C++ patterns like Result<T> and smart pointers

namespace mozilla {

// Modernized version of FetchFocusNode
Result<nsCOMPtr<nsIDOMNode>, nsresult> FetchFocusNodeModern(nsTypedSelection* aSelection);

// Modernized version of FetchFocusOffset
Result<PRInt32, nsresult> FetchFocusOffsetModern(nsTypedSelection* aSelection);

// Modernized version of FetchStartParent
Result<nsCOMPtr<nsIDOMNode>, nsresult> FetchStartParentModern(nsTypedSelection* aSelection, nsIDOMRange* aRange);

// Modernized version of FetchStartOffset
Result<PRInt32, nsresult> FetchStartOffsetModern(nsTypedSelection* aSelection, nsIDOMRange* aRange);

// Modernized version of GetRangeAt
Result<nsCOMPtr<nsIDOMRange>, nsresult> GetRangeAtModern(nsTypedSelection* aSelection, int32_t aIndex);

// Modernized version of GetPresContext
Result<nsCOMPtr<nsIPresContext>, nsresult> GetPresContextModern(nsTypedSelection* aSelection);

// Modernized version of GetAnchorNode
Result<nsCOMPtr<nsIDOMNode>, nsresult> GetAnchorNodeModern(nsTypedSelection* aSelection);

// Modernized version of AddItem
Result<void, nsresult> AddItemModern(nsTypedSelection* aSelection, nsIDOMRange* aRange);

// Modernized version of RemoveItem
Result<void, nsresult> RemoveItemModern(nsTypedSelection* aSelection, nsIDOMRange* aRange);

// Modernized version of Clear
Result<void, nsresult> ClearModern(nsTypedSelection* aSelection, nsIPresContext* aPresContext);

// Modernized version of CurrentItem
Result<nsCOMPtr<nsIDOMRange>, nsresult> CurrentItemModern(nsTypedSelection* aSelection);

// Modernized version of FetchDesiredX
Result<nscoord, nsresult> FetchDesiredXModern(nsTypedSelection* aSelection);

} // namespace mozilla

#endif // modernized_nsSelection_h 