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
using mozilla::Maybe;
using mozilla::Some;
using mozilla::Nothing;

// Forward declarations
class nsTypedSelection;
class nsIPresContext;

// Modernized methods for nsTypedSelection
// These methods use modern C++ patterns like Result<T> and smart pointers

namespace mozilla {

/**
 * FetchFocusNodeModern - Gets the current focus node of the selection
 *
 * @param aSelection  The selection object to query
 * @return            Result containing either the focus node or an error code
 *                    - On success: nsCOMPtr<nsIDOMNode> containing the focus node (may be nullptr)
 *                    - On failure: nsresult error code
 */
Result<nsCOMPtr<nsIDOMNode>, nsresult> FetchFocusNodeModern(nsTypedSelection* aSelection);

/**
 * FetchFocusOffsetModern - Gets the current focus offset within the focus node
 *
 * @param aSelection  The selection object to query
 * @return            Result containing either the focus offset or an error code
 *                    - On success: PRInt32 containing the offset
 *                    - On failure: nsresult error code
 */
Result<PRInt32, nsresult> FetchFocusOffsetModern(nsTypedSelection* aSelection);

/**
 * FetchStartParentModern - Gets the parent node of the start point of the specified range
 *
 * @param aSelection  The selection object to query
 * @param aRange      The range to get the start parent from
 * @return            Result containing either the start parent node or an error code
 *                    - On success: nsCOMPtr<nsIDOMNode> containing the start parent node (may be nullptr)
 *                    - On failure: nsresult error code
 */
Result<nsCOMPtr<nsIDOMNode>, nsresult> FetchStartParentModern(nsTypedSelection* aSelection, nsIDOMRange* aRange);

/**
 * FetchStartOffsetModern - Gets the offset within the start parent of the specified range
 *
 * @param aSelection  The selection object to query
 * @param aRange      The range to get the start offset from
 * @return            Result containing either the start offset or an error code
 *                    - On success: PRInt32 containing the offset
 *                    - On failure: nsresult error code
 */
Result<PRInt32, nsresult> FetchStartOffsetModern(nsTypedSelection* aSelection, nsIDOMRange* aRange);

/**
 * FetchAnchorParentModern - Gets the parent node of the anchor node
 *
 * @param aSelection  The selection object to query
 * @return            Result containing either the parent node or an error code
 *                    - On success: nsCOMPtr<nsIDOMNode> with the parent node (may be nullptr)
 *                    - On failure: nsresult error code
 */
Result<nsCOMPtr<nsIDOMNode>, nsresult> FetchAnchorParentModern(nsTypedSelection* aSelection);

/**
 * FetchAnchorOffsetModern - Gets the offset of the anchor node
 *
 * @param aSelection  The selection object to query
 * @return            Maybe<int32_t> containing the offset if anchor node exists,
 *                    or Nothing() if no anchor node exists
 */
Maybe<int32_t> FetchAnchorOffsetModern(nsTypedSelection* aSelection);

/**
 * GetRangeAtModern - Gets the range at the specified index in the selection
 *
 * @param aSelection  The selection object to query
 * @param aIndex      The index of the range to retrieve
 * @return            Result containing either the range or an error code
 *                    - On success: nsCOMPtr<nsIDOMRange> containing the range (may be nullptr)
 *                    - On failure: nsresult error code (NS_ERROR_INVALID_ARG if index out of bounds)
 */
Result<nsCOMPtr<nsIDOMRange>, nsresult> GetRangeAtModern(nsTypedSelection* aSelection, int32_t aIndex);

/**
 * GetPresContextModern - Gets the presentation context associated with the selection
 *
 * @param aSelection  The selection object to query
 * @return            Result containing either the presentation context or an error code
 *                    - On success: nsCOMPtr<nsIPresContext> containing the context (may be nullptr)
 *                    - On failure: nsresult error code
 */
Result<nsCOMPtr<nsIPresContext>, nsresult> GetPresContextModern(nsTypedSelection* aSelection);

/**
 * GetAnchorNodeModern - Gets the anchor node of the selection
 *
 * The anchor node is where the selection began. This may be different from
 * the focus node if the selection was made from focus to anchor.
 *
 * @param aSelection  The selection object to query
 * @return            Result containing either the anchor node or an error code
 *                    - On success: nsCOMPtr<nsIDOMNode> containing the anchor node (may be nullptr)
 *                    - On failure: nsresult error code
 */
Result<nsCOMPtr<nsIDOMNode>, nsresult> GetAnchorNodeModern(nsTypedSelection* aSelection);

/**
 * AddItemModern - Adds a range to the selection
 *
 * @param aSelection  The selection object to modify
 * @param aRange      The range to add to the selection
 * @return            Result containing either void (success) or an error code
 *                    - On success: void
 *                    - On failure: nsresult error code (NS_ERROR_NULL_POINTER if aRange is null,
 *                                 NS_ERROR_FAILURE if mRangeArray is null)
 */
Result<void, nsresult> AddItemModern(nsTypedSelection* aSelection, nsIDOMRange* aRange);

/**
 * RemoveItemModern - Removes a range from the selection
 *
 * @param aSelection  The selection object to modify
 * @param aRange      The range to remove from the selection
 * @return            Result containing either void (success) or an error code
 *                    - On success: void
 *                    - On failure: nsresult error code (NS_ERROR_NULL_POINTER if aRange is null,
 *                                 NS_ERROR_FAILURE if mRangeArray is null or range not found)
 */
Result<void, nsresult> RemoveItemModern(nsTypedSelection* aSelection, nsIDOMRange* aRange);

/**
 * ClearModern - Clears all ranges from the selection
 *
 * @param aSelection   The selection object to modify
 * @param aPresContext The presentation context (can be null)
 * @return             Result containing either void (success) or an error code
 *                     - On success: void
 *                     - On failure: nsresult error code (NS_ERROR_FAILURE if mRangeArray is null)
 */
Result<void, nsresult> ClearModern(nsTypedSelection* aSelection, nsIPresContext* aPresContext);

/**
 * CurrentItemModern - Gets the current range in the selection
 *
 * @param aSelection  The selection object to query
 * @return            Result containing either the current range or an error code
 *                    - On success: nsCOMPtr<nsIDOMRange> containing the current range (may be nullptr)
 *                    - On failure: nsresult error code
 */
Result<nsCOMPtr<nsIDOMRange>, nsresult> CurrentItemModern(nsTypedSelection* aSelection);

/**
 * FetchDesiredXModern - Gets the desired X coordinate for vertical navigation
 *
 * This is used when moving the caret up and down to maintain the horizontal position.
 *
 * @param aSelection  The selection object to query
 * @return            Result containing either the desired X coordinate or an error code
 *                    - On success: nscoord containing the X coordinate
 *                    - On failure: nsresult error code
 */
Result<nscoord, nsresult> FetchDesiredXModern(nsTypedSelection* aSelection);

} // namespace mozilla

#endif // modernized_nsSelection_h 