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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   Seth Spitzer <sspitzer@netscape.com>
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

#include "nsAbDirProperty.h"	 
#include "nsIRDFService.h"
#include "nsIRDFResource.h"
#include "nsIServiceManager.h"
#include "nsRDFCID.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsCOMPtr.h"
#include "nsAbBaseCID.h"
#include "nsIAbCard.h"

#include "rdf.h"

#include "mdb.h"

nsAbDirProperty::nsAbDirProperty(void)
  : m_LastModifiedDate(0)
{
	NS_INIT_ISUPPORTS();

	m_IsMailList = PR_FALSE;
}

nsAbDirProperty::~nsAbDirProperty(void)
{
}

NS_IMPL_ISUPPORTS1(nsAbDirProperty,nsIAbDirectory)

NS_IMETHODIMP nsAbDirProperty::GetOperations(PRInt32 *aOperations)
{
  // Default is to support all operations.
  // Inheriting implementations may override
  // to reduce supported operations
	*aOperations = nsIAbDirectory::opRead |
		nsIAbDirectory::opWrite |
		nsIAbDirectory::opSearch;

	return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::GetDirName(PRUnichar **aDirName)
{
	if (aDirName)
	{
		*aDirName = ToNewUnicode(m_DirName);
		if (!(*aDirName)) 
			return NS_ERROR_OUT_OF_MEMORY;
		else
			return NS_OK;
	}
	else
		return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsAbDirProperty::SetDirName(const PRUnichar * aDirName)
{
	if (aDirName)
		m_DirName = aDirName;
	return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::GetLastModifiedDate(PRUint32 *aLastModifiedDate)
{
	if (aLastModifiedDate)
	{
		*aLastModifiedDate = m_LastModifiedDate;
		return NS_OK;
	}
	else
		return NS_RDF_NO_VALUE;
}

NS_IMETHODIMP nsAbDirProperty::SetLastModifiedDate(PRUint32 aLastModifiedDate)
{
	if (aLastModifiedDate)
	{
		m_LastModifiedDate = aLastModifiedDate;
	}
	return NS_OK;
}

nsresult nsAbDirProperty::GetAttributeName(PRUnichar **aName, nsString& value)
{
	if (aName)
	{
		*aName = ToNewUnicode(value);
		if (!(*aName)) 
			return NS_ERROR_OUT_OF_MEMORY;
		else
			return NS_OK;
	}
	else
		return NS_ERROR_NULL_POINTER;

}

nsresult nsAbDirProperty::SetAttributeName(const PRUnichar *aName, nsString& arrtibute)
{
	if (aName)
		arrtibute = aName;
	return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::GetListNickName(PRUnichar * *aListNickName)
{ return GetAttributeName(aListNickName, m_ListNickName); }

NS_IMETHODIMP nsAbDirProperty::SetListNickName(const PRUnichar * aListNickName)
{ return SetAttributeName(aListNickName, m_ListNickName); }

NS_IMETHODIMP nsAbDirProperty::GetDescription(PRUnichar * *aDescription)
{ return GetAttributeName(aDescription, m_Description); }

NS_IMETHODIMP nsAbDirProperty::SetDescription(const PRUnichar * aDescription)
{ return SetAttributeName(aDescription, m_Description); }

NS_IMETHODIMP nsAbDirProperty::GetIsMailList(PRBool *aIsMailList)
{
	*aIsMailList = m_IsMailList;
	return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::SetIsMailList(PRBool aIsMailList)
{
	m_IsMailList = aIsMailList;
	return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::GetAddressLists(nsISupportsArray * *aAddressLists)
{
	if (!m_AddressList)
	{
		NS_NewISupportsArray(getter_AddRefs(m_AddressList));
	}

	*aAddressLists = m_AddressList;
	NS_ADDREF(*aAddressLists);
	return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::SetAddressLists(nsISupportsArray * aAddressLists)
{
	m_AddressList = aAddressLists;
	return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::AddMailListToDatabase(const char *uri)
{
	nsresult rv = NS_OK;
	nsCOMPtr<nsIRDFService> rdf(do_GetService("@mozilla.org/rdf/rdf-service;1", &rv));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIRDFResource> res;
	rv = rdf->GetResource(uri, getter_AddRefs(res));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIAbDirectory> directory(do_QueryInterface(res, &rv));
	NS_ENSURE_SUCCESS(rv, rv);      

	rv = directory->AddMailList(this);
  NS_ENSURE_SUCCESS(rv, rv);   
	return rv;
}

NS_IMETHODIMP nsAbDirProperty::CopyMailList(nsIAbDirectory* srcList)
{
  nsXPIDLString str;
	srcList->GetDirName(getter_Copies(str));
	SetDirName(str);
	srcList->GetListNickName(getter_Copies(str));
	SetListNickName(str);
	srcList->GetDescription(getter_Copies(str));
	SetDescription(str);

	SetIsMailList(PR_TRUE);

	nsCOMPtr <nsISupportsArray> pAddressLists;
	srcList->GetAddressLists(getter_AddRefs(pAddressLists));
	SetAddressLists(pAddressLists);
	return NS_OK;
}

// nsIAbDirectory NOT IMPLEMENTED methods

NS_IMETHODIMP
nsAbDirProperty::GetChildNodes(nsIEnumerator **childList)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP
nsAbDirProperty::GetChildCards(nsIEnumerator **childCards)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP
nsAbDirProperty::DeleteDirectory(nsIAbDirectory *dierctory)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP
nsAbDirProperty::DeleteCards(nsISupportsArray *cards)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP
nsAbDirProperty::HasCard(nsIAbCard *cards, PRBool *hasCard)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP
nsAbDirProperty::HasDirectory(nsIAbDirectory *dir, PRBool *hasDir)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP
nsAbDirProperty::CreateNewDirectory(nsIAbDirectoryProperties *aProperties)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP nsAbDirProperty::CreateDirectoryByURI(const PRUnichar *dirName, const char *uri, PRBool migrating)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP nsAbDirProperty::AddMailList(nsIAbDirectory *list)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP nsAbDirProperty::EditMailListToDatabase(const char *uri, nsIAbCard *listCard)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP nsAbDirProperty::AddCard(nsIAbCard *childCard, nsIAbCard **addedCard)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP nsAbDirProperty::DropCard(nsIAbCard *childCard, PRBool needToCopyCard)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP nsAbDirProperty::GetValueForCard(nsIAbCard *card, const char *name, PRUnichar **value)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP nsAbDirProperty::SetValueForCard(nsIAbCard *card, const char *name, const PRUnichar *value)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP nsAbDirProperty::GetSupportsMailingLists(PRBool *aSupportsMailingsLists)
{
  NS_ENSURE_ARG_POINTER(aSupportsMailingsLists);
  *aSupportsMailingsLists = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::GetIsRemote(PRBool *aIsRemote)
{
  NS_ENSURE_ARG_POINTER(aIsRemote);
  *aIsRemote = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::GetSearchDuringLocalAutocomplete(PRBool *aSearchDuringLocalAutocomplete)
{
  NS_ENSURE_ARG_POINTER(aSearchDuringLocalAutocomplete);
  *aSearchDuringLocalAutocomplete = PR_TRUE;
  return NS_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

nsAbDirectoryProperties::nsAbDirectoryProperties(void)
{
  NS_INIT_ISUPPORTS();
}

nsAbDirectoryProperties::~nsAbDirectoryProperties(void)
{
}

NS_IMPL_ISUPPORTS1(nsAbDirectoryProperties,nsIAbDirectoryProperties)

NS_IMETHODIMP 
nsAbDirectoryProperties::SetDescription(const nsAString &aDescription)
{
  mDescription = aDescription;
  return NS_OK;
}

NS_IMETHODIMP 
nsAbDirectoryProperties::GetDescription(nsAString &aDescription)
{
  aDescription = mDescription;
  return NS_OK;
}

NS_IMETHODIMP 
nsAbDirectoryProperties::SetURI(const char *aURI)
{
  mURI = aURI;
  return NS_OK;
}

NS_IMETHODIMP 
nsAbDirectoryProperties::GetURI(char **aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);
  *aURI = ToNewCString(mURI);
  return NS_OK;
}

NS_IMETHODIMP 
nsAbDirectoryProperties::SetFileName(const char *aFileName)
{
  mFileName = aFileName;
  return NS_OK;
}

NS_IMETHODIMP 
nsAbDirectoryProperties::GetFileName(char **aFileName)
{
  NS_ENSURE_ARG_POINTER(aFileName);
  *aFileName = ToNewCString(mFileName);
  return NS_OK;
}

NS_IMETHODIMP 
nsAbDirectoryProperties::SetPrefName(const char *aPrefName)
{
  mPrefName = aPrefName;
  return NS_OK;
}

NS_IMETHODIMP 
nsAbDirectoryProperties::GetPrefName(char **aPrefName)
{
  NS_ENSURE_ARG_POINTER(aPrefName);
  *aPrefName = ToNewCString(mPrefName);
  return NS_OK;
}
