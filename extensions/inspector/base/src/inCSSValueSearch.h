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
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Joe Hewitt <hewitt@netscape.com> (original author)
 *
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

#ifndef __inCSSValueSearch_h__
#define __inCSSValueSearch_h__

#include "inICSSValueSearch.h"

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "inISearchObserver.h"
#include "nsVoidArray.h"
#include "nsICSSStyleSheet.h"
#include "nsICSSStyleRule.h"
#include "nsCSSValue.h"
#include "nsIInspectorCSSUtils.h"

class inCSSValueSearch : public inICSSValueSearch
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_INISEARCHPROCESS
  NS_DECL_INICSSVALUESEARCH

  inCSSValueSearch();
  virtual ~inCSSValueSearch();

protected:
  PRBool mIsActive;
  PRInt32 mResultCount;
  PRBool mHoldResults;
  nsVoidArray* mResults;
  nsAutoString* mLastResult;
  nsCOMPtr<inISearchObserver> mObserver;

  nsCOMPtr<nsIDOMDocument> mDocument;
  nsAutoString* mBaseURL;
  PRBool mReturnRelativeURLs;
  PRBool mNormalizeChromeURLs;
  nsAutoString* mTextCriteria;
  nsCSSProperty* mProperties;
  PRUint32 mPropertyCount;
  nsCOMPtr<nsIInspectorCSSUtils> mCSSUtils;

  nsresult InitSearch();
  nsresult KillSearch(PRInt16 aResult);
  nsresult SearchStyleSheet(nsIStyleSheet* aStyleSheet);
  nsresult SearchStyleRule(nsIStyleRule* aStyleRule);
  nsresult SearchStyleValue(nsICSSStyleRule* aDec, nsCSSProperty aProp);
  nsresult EqualizeURL(nsAutoString* aURL);
};

#endif // __inCSSValueSearch_h__
