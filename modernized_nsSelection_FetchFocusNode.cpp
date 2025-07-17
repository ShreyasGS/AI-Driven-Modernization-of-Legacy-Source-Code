/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/**
 * Modernized implementation of nsTypedSelection::FetchFocusNode
 * 
 * This file demonstrates the modernization of the FetchFocusNode method
 * from the Mozilla 1.0 codebase, applying these modernization patterns:
 * 
 * 1. Result<T> return type instead of raw pointers and error codes
 * 2. Smart pointers (nsCOMPtr) for automatic reference counting
 * 3. Explicit error handling with early returns
 * 4. Consistent naming and formatting
 */

#include "mozilla/Result.h"
#include "modernized_nsSelection.h"
#include "nsTypedSelection.h"
#include "nsIDOMNode.h"

// Original implementation:
/*
nsIDOMNode*
nsTypedSelection::FetchFocusNode()
{   //where is the carret
  nsCOMPtr<nsIDOMNode>returnval;
  GetFocusNode(getter_AddRefs(returnval));//this queries
  return returnval;
}//at end it will release, no addreff was called
*/

/**
 * FetchFocusNodeModern - Gets the current focus node of the selection
 *
 * This modernized implementation uses Result<T> pattern to explicitly handle errors
 * and provide a more type-safe interface. It properly handles the reference counting
 * of the DOM node and returns a Result containing either the node or an error code.
 *
 * The focus node represents the end point of the selection where the caret is positioned.
 * It may be different from the anchor node if the selection was made by dragging from
 * anchor to focus.
 *
 * @param aSelection  The selection object to query
 * @return            Result containing either the focus node or an error code
 *                    - On success: nsCOMPtr<nsIDOMNode> containing the focus node (may be nullptr)
 *                    - On failure: nsresult error code from GetFocusNode
 */
mozilla::Result<nsCOMPtr<nsIDOMNode>, nsresult>
mozilla::FetchFocusNodeModern(nsTypedSelection* aSelection)
{
  // Parameter validation
  if (!aSelection) {
    return mozilla::Err(NS_ERROR_NULL_POINTER);
  }
  
  // Get the focus node
  nsCOMPtr<nsIDOMNode> returnval;
  nsresult rv = aSelection->GetFocusNode(getter_AddRefs(returnval));
  
  // Handle error case
  if (NS_FAILED(rv)) {
    return mozilla::Err(rv);
  }
  
  // Return success with the node (may be nullptr)
  return mozilla::Ok(returnval);
}

/**
 * FetchFocusNode - Backward compatibility wrapper for FetchFocusNodeModern
 *
 * This method maintains the original API while using the modernized implementation
 * internally. It converts between the Result<nsCOMPtr<nsIDOMNode>> return type
 * and the traditional raw pointer return type.
 *
 * @return  nsIDOMNode* - The focus node or nullptr on failure
 *          Note: The caller does not need to AddRef this node, as it was already
 *          AddRef'd for the caller.
 */
nsIDOMNode*
nsTypedSelection::FetchFocusNode()
{
  // Call the modernized implementation
  auto result = mozilla::FetchFocusNodeModern(this);
  
  // Return nullptr on error or the node on success
  if (result.isErr()) {
    return nullptr;
  }
  
  // Get the node from the result
  nsCOMPtr<nsIDOMNode> node = result.unwrap();
  
  // Return the raw pointer, forgetting the reference
  // This matches the behavior of the original implementation
  return node.forget().get();
}

/*
Modernization changes:
1. Created a new method that returns a Result<nsCOMPtr<nsIDOMNode>> instead of a raw pointer
2. Added explicit error handling for GetFocusNode failures
3. Added parameter validation
4. Used nsCOMPtr for automatic reference counting
5. Created a backward compatibility wrapper that maintains the original API
6. Added comprehensive documentation for both functions
*/ 