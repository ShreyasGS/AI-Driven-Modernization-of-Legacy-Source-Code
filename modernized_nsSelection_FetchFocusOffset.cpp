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
 * Modernized implementation of nsTypedSelection::FetchFocusOffset
 * 
 * This file demonstrates the modernization of the FetchFocusOffset method
 * from the Mozilla 1.0 codebase, applying these modernization patterns:
 * 
 * 1. Result<T> return type instead of raw return values and error codes
 * 2. Explicit error handling with early returns
 * 3. Consistent naming and formatting
 */

#include "nscore.h"
#include "nsISelection.h"
#include "modernized_nsSelection_Result.h"

using mozilla::Result;
using mozilla::Ok;
using mozilla::Err;

/**
 * Original implementation:
 *
 * PRInt32
 * nsTypedSelection::FetchFocusOffset()
 * {
 *   PRInt32 returnval;
 *   if (NS_SUCCEEDED(GetFocusOffset(&returnval)))//this queries
 *     return returnval;
 *   return NS_OK;
 * }
 */

/**
 * Modernized implementation:
 * 
 * Returns the focus offset as a Result type that either contains
 * the offset value or an error code.
 * 
 * @return Result<PRInt32, nsresult> - Success: The focus offset
 *                                   - Error: The error code
 */
Result<PRInt32, nsresult>
nsTypedSelection::FetchFocusOffsetModern()
{
  // Get the focus offset
  PRInt32 returnval;
  nsresult rv = GetFocusOffset(&returnval);
  
  // Handle error case
  if (NS_FAILED(rv)) {
    return Err(rv);
  }
  
  // Return success with the offset value
  return Ok(returnval);
}

/**
 * Compatibility wrapper:
 * 
 * Maintains the original API while using the modernized implementation.
 * 
 * @return PRInt32 - The focus offset or 0 on error
 */
PRInt32
nsTypedSelection::FetchFocusOffset()
{
  return FetchFocusOffsetModern().unwrapOr(0);
} 