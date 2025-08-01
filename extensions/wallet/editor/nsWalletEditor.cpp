/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

#include "nscore.h"
#include "nsIMemory.h"
#include "plstr.h"
#include <stdio.h>
#include "nsIWalletService.h"
#include "nsIServiceManager.h"
#include "nsIDOMWindowInternal.h"
#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
#include "nsIDocShell.h"
#include "nsIScriptGlobalObject.h"
#include "nsWalletEditor.h"

static NS_DEFINE_IID(kWalletServiceCID, NS_WALLETSERVICE_CID);

////////////////////////////////////////////////////////////////////////

WalletEditorImpl::WalletEditorImpl()
{
  NS_INIT_REFCNT();
}

WalletEditorImpl::~WalletEditorImpl()
{
}

NS_IMPL_ISUPPORTS1(WalletEditorImpl, nsIWalletEditor);

NS_IMETHODIMP
WalletEditorImpl::GetValue(PRUnichar** aValue)
{
  NS_PRECONDITION(aValue != nsnull, "null ptr");
  if (!aValue) {
    return NS_ERROR_NULL_POINTER;
  }
  nsresult res;
  nsCOMPtr<nsIWalletService> walletservice = 
           do_GetService(kWalletServiceCID, &res);
  if (NS_FAILED(res)) return res;
  nsAutoString walletList;
  res = walletservice->WALLET_PreEdit(walletList);
  if (NS_SUCCEEDED(res)) {
    *aValue = ToNewUnicode(walletList);
  }
  return res;
}

NS_IMETHODIMP
WalletEditorImpl::SetValue(const PRUnichar* aValue, nsIDOMWindowInternal* win)
{
  /* process the value */
  NS_PRECONDITION(aValue != nsnull, "null ptr");
  if (! aValue) {
    return NS_ERROR_NULL_POINTER;
  }
  nsresult res;
  nsCOMPtr<nsIWalletService> walletservice = 
           do_GetService(kWalletServiceCID, &res);
  if (NS_FAILED(res)) return res;
  nsAutoString walletList( aValue );
  res = walletservice->WALLET_PostEdit(walletList);
  return res;
}
