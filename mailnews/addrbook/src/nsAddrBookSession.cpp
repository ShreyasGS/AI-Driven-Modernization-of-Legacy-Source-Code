/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 *   Pierre Phaneuf <pp@ludusdesign.com>
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

#include "nsCOMPtr.h"
#include "nsAddrBookSession.h"
#include "nsIAddrBookSession.h"
#include "nsIFileSpec.h"
#include "nsILocalFile.h"
#include "nsIDirectoryService.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsXPIDLString.h"
#include "nsIDOMXULDocument.h"
#include "nsIDocument.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsAddrBookSession, nsIAddrBookSession)
    
nsAddrBookSession::nsAddrBookSession()
{
  NS_INIT_ISUPPORTS();
}

nsAddrBookSession::~nsAddrBookSession()
{
}


// nsIAddrBookSession

NS_IMETHODIMP nsAddrBookSession::AddAddressBookListener(nsIAbListener *listener, PRUint32 notifyFlags)
{
  if (!mListeners)
    NS_NewISupportsArray(getter_AddRefs(mListeners));

  NS_ENSURE_TRUE(mListeners, NS_ERROR_NULL_POINTER);
  mListeners->AppendElement(listener);
  mListenerNotifyFlags.Add(notifyFlags);
  return NS_OK;
}

NS_IMETHODIMP nsAddrBookSession::RemoveAddressBookListener(nsIAbListener * listener)
{
  NS_ENSURE_TRUE(mListeners, NS_ERROR_NULL_POINTER);

  PRInt32 index;
  nsresult rv = mListeners->GetIndexOf(listener, &index);
  NS_ENSURE_SUCCESS(rv,rv);
  NS_ASSERTION(index >= 0, "removing non-existent listener");
  if (index >= 0)
  {
    mListenerNotifyFlags.RemoveAt(index);
    mListeners->RemoveElement(listener);
  }
  return NS_OK;
}

NS_IMETHODIMP nsAddrBookSession::NotifyItemPropertyChanged(nsISupports *item, const char *property, const PRUnichar* oldValue, const PRUnichar* newValue)
{
  NS_ENSURE_TRUE(mListeners, NS_ERROR_NULL_POINTER);

  PRUint32 count = 0;
  PRUint32 i;
  nsresult rv = mListeners->Count(&count);
  NS_ENSURE_SUCCESS(rv, rv);
  for(i = 0; i < count; i++)
  {
    if (mListenerNotifyFlags[i] & nsIAbListener::changed) {
      nsCOMPtr<nsIAbListener> listener = getter_AddRefs((nsIAbListener*)mListeners->ElementAt(i));
      NS_ASSERTION(listener, "listener is null");
      if (listener)
        listener->OnItemPropertyChanged(item, property, oldValue, newValue);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP nsAddrBookSession::NotifyDirectoryItemAdded(nsIAbDirectory *directory, nsISupports *item)
{
  if (mListeners)
  {
    PRUint32 count = 0;
    PRUint32 i;
    nsresult rv = mListeners->Count(&count);
    NS_ENSURE_SUCCESS(rv, rv);
    for(i = 0; i < count; i++)
    {
      if (mListenerNotifyFlags[i] & nsIAbListener::added) {
        nsCOMPtr<nsIAbListener> listener = getter_AddRefs((nsIAbListener*)mListeners->ElementAt(i));
        NS_ASSERTION(listener, "listener is null");
        if (listener)
          listener->OnItemAdded(directory, item);
      }
    }
  }
  return NS_OK;

}

NS_IMETHODIMP nsAddrBookSession::NotifyDirectoryItemDeleted(nsIAbDirectory *directory, nsISupports *item)
{
  if (mListeners)
  {
    PRUint32 count = 0;
    PRUint32 i;
    nsresult rv = mListeners->Count(&count);
    NS_ENSURE_SUCCESS(rv, rv);
    for(i = 0; i < count; i++)
    {
      if (mListenerNotifyFlags[i] & nsIAbListener::directoryItemRemoved) {
        nsCOMPtr<nsIAbListener> listener = getter_AddRefs((nsIAbListener*)mListeners->ElementAt(i));
        NS_ASSERTION(listener, "listener is null");
        if (listener)
          listener->OnItemRemoved(directory, item);
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP nsAddrBookSession::NotifyDirectoryDeleted(nsIAbDirectory *directory, nsISupports *item)
{
  if (mListeners)
  {
    PRUint32 count = 0;
    PRUint32 i;
    nsresult rv = mListeners->Count(&count);
    NS_ENSURE_SUCCESS(rv, rv);
    for(i = 0; i < count; i++)
    {
      if (mListenerNotifyFlags[i] & nsIAbListener::directoryRemoved) {
        nsCOMPtr<nsIAbListener> listener = getter_AddRefs(NS_STATIC_CAST(nsIAbListener *,mListeners->ElementAt(i)));
        NS_ASSERTION(listener, "listener is null");
        if (listener)
          listener->OnItemRemoved(directory, item);
      }
    }
  }
  return NS_OK;
}


NS_IMETHODIMP nsAddrBookSession::GetUserProfileDirectory(nsFileSpec * *userDir)
{
  NS_ENSURE_ARG_POINTER(userDir);
  *userDir = nsnull;

  nsresult rv;		
  nsCOMPtr<nsIFile> profileDir;
  nsCAutoString pathBuf;

  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(profileDir));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = profileDir->GetNativePath(pathBuf);
  NS_ENSURE_SUCCESS(rv, rv);

  *userDir = new nsFileSpec(pathBuf.get());
  NS_ENSURE_TRUE(*userDir, NS_ERROR_OUT_OF_MEMORY);

  return rv;
}

#define kDisplayName 0
#define kLastFirst   1
#define kFirstLast   2

NS_IMETHODIMP nsAddrBookSession::GenerateNameFromCard(nsIAbCard *card, PRInt32 generateFormat, PRUnichar **aName)
{
  nsresult rv = NS_OK;
	
  if (generateFormat == kDisplayName) {
    rv = card->GetDisplayName(aName);
  }
  else  {
    nsXPIDLString firstName;
    nsXPIDLString lastName;
    
    rv = card->GetFirstName(getter_Copies(firstName));
    NS_ENSURE_SUCCESS(rv, rv);       

    rv = card->GetLastName(getter_Copies(lastName));
    NS_ENSURE_SUCCESS(rv,rv);

    if (!lastName.IsEmpty() && !firstName.IsEmpty()) {
      if (!mBundle) {       
        nsCOMPtr<nsIStringBundleService> stringBundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv); 
        NS_ENSURE_SUCCESS(rv,rv);
        
        rv = stringBundleService->CreateBundle("chrome://messenger/locale/addressbook/addressBook.properties", getter_AddRefs(mBundle));
        NS_ENSURE_SUCCESS(rv,rv);
      }

      nsXPIDLString generatedName;

      if (generateFormat == kLastFirst) {
        const PRUnichar *stringParams[2] = {lastName.get(), firstName.get()};
        
        rv = mBundle->FormatStringFromName(NS_LITERAL_STRING("lastFirstFormat").get(), stringParams, 2, 
          getter_Copies(generatedName));
      }
      else {
        const PRUnichar *stringParams[2] = {firstName.get(), lastName.get()};
        
        rv = mBundle->FormatStringFromName(NS_LITERAL_STRING("firstLastFormat").get(), stringParams, 2, 
          getter_Copies(generatedName));
        
      }
      
  NS_ENSURE_SUCCESS(rv,rv); 
      *aName = ToNewUnicode(generatedName);
    }
    else {
      if (lastName.Length())
        *aName = ToNewUnicode(lastName);
      else if (firstName.Length())
        *aName = ToNewUnicode(firstName);
      else
        *aName = ToNewUnicode(NS_LITERAL_STRING(""));
    }
  }

  return NS_OK;
}


