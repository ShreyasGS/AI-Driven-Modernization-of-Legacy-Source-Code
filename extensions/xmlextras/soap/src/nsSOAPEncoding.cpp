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
 * Portions created by the Initial Developer are Copyright (C) 2001
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

#include "nsString.h"
#include "nsISOAPParameter.h"
#include "nsSOAPMessage.h"
#include "nsISOAPEncoder.h"
#include "nsISOAPDecoder.h"
#include "nsSOAPEncoding.h"
#include "nsSOAPUtils.h"
#include "nsSOAPException.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIDOMNodeList.h"
#include "nsISchema.h"
#include "nsISchemaLoader.h"
#include "nsSOAPUtils.h"

//  First comes the registry which shares between associated encodings but is never seen by xpconnect.

NS_IMPL_ISUPPORTS1(nsSOAPEncodingRegistry, nsISOAPEncoding) nsSOAPEncodingRegistry::nsSOAPEncodingRegistry(nsISOAPEncoding * aEncoding) : mEncodings(new nsSupportsHashtable)
{
  NS_INIT_ISUPPORTS();

  nsAutoString style;
  nsresult rc = aEncoding->GetStyleURI(style);
  if (NS_FAILED(rc)) {
    mEncodings = nsnull;
  }
  //  If there are any failures, encodings becomes null, and calls fail.
  nsStringKey styleKey(style);
  mEncodings->Put(&styleKey, aEncoding);
  /* member initializers and constructor code */
}

nsSOAPEncodingRegistry::~nsSOAPEncodingRegistry()
{
  /* destructor code */
  delete mEncodings;
}

nsresult
    nsSOAPEncodingRegistry::
GetAssociatedEncoding(const nsAString & aStyleURI, PRBool aCreateIf,
                      nsISOAPEncoding * *aEncoding)
{
  NS_SOAP_ENSURE_ARG_STRING(aStyleURI);
  NS_ENSURE_ARG_POINTER(aEncoding);
  if (!mEncodings)
    return NS_ERROR_FAILURE;
  nsStringKey styleKey(aStyleURI);
  *aEncoding = (nsISOAPEncoding *) mEncodings->Get(&styleKey);
  if (!*aEncoding) {
    nsCOMPtr < nsISOAPEncoding > defaultEncoding;
    nsCAutoString encodingContractid;
    encodingContractid.Assign(NS_SOAPENCODING_CONTRACTID_PREFIX);
    encodingContractid.Append(NS_ConvertUCS2toUTF8(aStyleURI));
    defaultEncoding = do_GetService(encodingContractid.get());
    if (defaultEncoding || aCreateIf) {
      nsCOMPtr < nsISOAPEncoding > encoding = new nsSOAPEncoding(aStyleURI,this,defaultEncoding);
      *aEncoding = encoding;
      if (encoding) {
        NS_ADDREF(*aEncoding);
        mEncodings->Put(&styleKey, encoding);
      }
      else
      {
        return NS_ERROR_FAILURE;
      }
    }
  }
  return NS_OK;
}

nsresult
    nsSOAPEncodingRegistry::SetSchemaCollection(nsISchemaCollection *
                                                aSchemaCollection)
{
  NS_ENSURE_ARG(aSchemaCollection);
  mSchemaCollection = aSchemaCollection;
  return NS_OK;
}

nsresult
    nsSOAPEncodingRegistry::GetSchemaCollection(nsISchemaCollection **
                                                aSchemaCollection)
{
  NS_ENSURE_ARG_POINTER(aSchemaCollection);
  if (!mSchemaCollection) {
    nsresult rv;
    nsCOMPtr < nsISchemaLoader > loader =
        do_CreateInstance(NS_SCHEMALOADER_CONTRACTID, &rv);
    if (NS_FAILED(rv))
      return rv;
    mSchemaCollection = do_QueryInterface(loader);
    if (!mSchemaCollection)
      return NS_ERROR_FAILURE;
  }
  *aSchemaCollection = mSchemaCollection;
  NS_ADDREF(*aSchemaCollection);
  return NS_OK;
}

/* readonly attribute AString styleURI; */
NS_IMETHODIMP nsSOAPEncodingRegistry::GetStyleURI(nsAString & aStyleURI)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsISOAPEncoder setEncoder (in AString aKey, in nsISOAPEncoder aEncoder); */
NS_IMETHODIMP
    nsSOAPEncodingRegistry::SetEncoder(const nsAString & aKey,
                                       nsISOAPEncoder * aEncoder)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsISOAPEncoder getEncoder (in AString aKey); */
NS_IMETHODIMP
    nsSOAPEncodingRegistry::GetEncoder(const nsAString & aKey,
                                       nsISOAPEncoder ** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsISOAPDecoder setDecoder (in AString aKey, in nsISOAPDecoder aDecoder); */
NS_IMETHODIMP
    nsSOAPEncodingRegistry::SetDecoder(const nsAString & aKey,
                                       nsISOAPDecoder * aDecoder)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsISOAPDecoder getDecoder (in AString aKey); */
NS_IMETHODIMP
    nsSOAPEncodingRegistry::GetDecoder(const nsAString & aKey,
                                       nsISOAPDecoder ** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsISOAPEncoder defaultEncoder; */
NS_IMETHODIMP
    nsSOAPEncodingRegistry::GetDefaultEncoder(nsISOAPEncoder *
                                              *aDefaultEncoder)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
    nsSOAPEncodingRegistry::SetDefaultEncoder(nsISOAPEncoder *
                                              aDefaultEncoder)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsISOAPDecoder defaultDecoder; */
NS_IMETHODIMP
    nsSOAPEncodingRegistry::GetDefaultDecoder(nsISOAPDecoder *
                                              *aDefaultDecoder)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
    nsSOAPEncodingRegistry::SetDefaultDecoder(nsISOAPDecoder *
                                              aDefaultDecoder)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIDOMElement encode (in nsIVariant aSource, in AString aNamespaceURI, in AString aName, in nsISchemaType aSchemaType, in nsISOAPAttachments aAttachments, in nsIDOMElement aDestination); */
NS_IMETHODIMP
    nsSOAPEncodingRegistry::Encode(nsIVariant * aSource,
                                   const nsAString & aNamespaceURI,
                                   const nsAString & aName,
                                   nsISchemaType * aSchemaType,
                                   nsISOAPAttachments * aAttachments,
                                   nsIDOMElement * aDestination,
                                   nsIDOMElement ** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIVariant decode (in nsIDOMElement aSource, in nsISchemaType aSchemaType, in nsISOAPAttachments aAttachments); */
NS_IMETHODIMP
    nsSOAPEncodingRegistry::Decode(nsIDOMElement * aSource,
                                   nsISchemaType * aSchemaType,
                                   nsISOAPAttachments * aAttachments,
                                   nsIVariant ** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean mapSchemaURI (in AString aExternalURI, in AString aInternalURI, in boolean aOutput); */
NS_IMETHODIMP nsSOAPEncodingRegistry::MapSchemaURI(const nsAString & aExternalURI, const nsAString & aInternalURI, PRBool aOutput, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean unmapSchemaURI (in AString aExternalURI); */
NS_IMETHODIMP nsSOAPEncodingRegistry::UnmapSchemaURI(const nsAString & aExternalURI, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* AString getInternalSchemaURI (in AString aExternalURI); */
NS_IMETHODIMP nsSOAPEncodingRegistry::GetInternalSchemaURI(const nsAString & aExternalURI, nsAString & _retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* AString getExternalSchemaURI (in AString aInternalURI); */
NS_IMETHODIMP nsSOAPEncodingRegistry::GetExternalSchemaURI(const nsAString & aInternalURI, nsAString & _retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

//  Second, we create the encodings themselves.

NS_IMPL_ISUPPORTS1_CI(nsSOAPEncoding, nsISOAPEncoding) 

nsSOAPEncoding::nsSOAPEncoding() : mEncoders(new nsSupportsHashtable()),
mDecoders(new nsSupportsHashtable()), mMappedInternal(new nsSupportsHashtable()), mMappedExternal(new nsSupportsHashtable())
{
  NS_INIT_ISUPPORTS();

  /* member initializers and constructor code */

  mStyleURI.Assign(nsSOAPUtils::kSOAPEncURI11);
  mDefaultEncoding = do_GetService(NS_DEFAULTSOAPENCODER_1_1_CONTRACTID);
  mRegistry = new nsSOAPEncodingRegistry(this);
}
nsSOAPEncoding::nsSOAPEncoding(const nsAString & aStyleURI, nsSOAPEncodingRegistry * aRegistry, nsISOAPEncoding * aDefaultEncoding) : mEncoders(new nsSupportsHashtable()),
mDecoders(new nsSupportsHashtable()), mMappedInternal(new nsSupportsHashtable()), mMappedExternal(new nsSupportsHashtable())
{
  NS_INIT_ISUPPORTS();

  /* member initializers and constructor code */

  mStyleURI.Assign(aStyleURI);
  mRegistry = aRegistry;
  mDefaultEncoding = aDefaultEncoding;
}

nsSOAPEncoding::~nsSOAPEncoding()
{
  /* destructor code */
  delete mEncoders;
  delete mDecoders;
  delete mMappedExternal;
  delete mMappedInternal;
}

nsresult
    nsSOAPEncoding::SetSchemaCollection(nsISchemaCollection *
                                        aSchemaCollection)
{
  NS_ENSURE_ARG(aSchemaCollection);
  if (!mRegistry)
    return NS_ERROR_FAILURE;
  return mRegistry->SetSchemaCollection(aSchemaCollection);
}

nsresult
    nsSOAPEncoding::GetSchemaCollection(nsISchemaCollection **
                                        aSchemaCollection)
{
  NS_ENSURE_ARG_POINTER(aSchemaCollection);
  if (!mRegistry)
    return NS_ERROR_FAILURE;
  return mRegistry->GetSchemaCollection(aSchemaCollection);
}

/* readonly attribute AString styleURI; */
NS_IMETHODIMP nsSOAPEncoding::GetStyleURI(nsAString & aStyleURI)
{
  NS_ENSURE_ARG_POINTER(&aStyleURI);
  aStyleURI.Assign(mStyleURI);
  return NS_OK;
}

/* nsISOAPEncoding getAssociatedEncoding (in AString aStyleURI, in boolean aCreateIf); */
NS_IMETHODIMP
    nsSOAPEncoding::GetAssociatedEncoding(const nsAString & aStyleURI,
                                          PRBool aCreateIf,
                                          nsISOAPEncoding ** _retval)
{
  NS_SOAP_ENSURE_ARG_STRING(aStyleURI);
  NS_ENSURE_ARG_POINTER(_retval);
  if (!mRegistry)
    return NS_ERROR_FAILURE;
  return mRegistry->GetAssociatedEncoding(aStyleURI, aCreateIf, _retval);
}

/* nsISOAPEncoder setEncoder (in AString aKey, in nsISOAPEncoder aEncoder); */
NS_IMETHODIMP
    nsSOAPEncoding::SetEncoder(const nsAString & aKey,
                               nsISOAPEncoder * aEncoder)
{
  NS_SOAP_ENSURE_ARG_STRING(aKey);
  NS_ENSURE_ARG(aEncoder);
  nsStringKey nameKey(aKey);
  if (aEncoder) {
    mEncoders->Put(&nameKey, aEncoder, nsnull);
  } else {
    mEncoders->Remove(&nameKey, nsnull);
  }
  return NS_OK;
}

/* nsISOAPEncoder getEncoder (in AString aKey); */
NS_IMETHODIMP
    nsSOAPEncoding::GetEncoder(const nsAString & aKey,
                               nsISOAPEncoder ** _retval)
{
  NS_SOAP_ENSURE_ARG_STRING(aKey);
  NS_ENSURE_ARG_POINTER(_retval);
  nsStringKey nameKey(aKey);
  *_retval = (nsISOAPEncoder *) mEncoders->Get(&nameKey);
  if (*_retval == nsnull && mDefaultEncoding) {
    return mDefaultEncoding->GetEncoder(aKey, _retval);
  }
  return NS_OK;
}

/* nsISOAPDecoder setDecoder (in AString aKey, in nsISOAPDecoder aDecoder); */
NS_IMETHODIMP
    nsSOAPEncoding::SetDecoder(const nsAString & aKey,
                               nsISOAPDecoder * aDecoder)
{
  NS_SOAP_ENSURE_ARG_STRING(aKey);
  NS_ENSURE_ARG(aDecoder);
  nsStringKey nameKey(aKey);
  if (aDecoder) {
    mDecoders->Put(&nameKey, aDecoder, nsnull);
  } else {
    mDecoders->Remove(&nameKey, nsnull);
  }
  return NS_OK;
}

/* nsISOAPDecoder getDecoder (in AString aKey); */
NS_IMETHODIMP
    nsSOAPEncoding::GetDecoder(const nsAString & aKey,
                               nsISOAPDecoder ** _retval)
{
  NS_SOAP_ENSURE_ARG_STRING(aKey);
  NS_ENSURE_ARG_POINTER(_retval);
  nsStringKey nameKey(aKey);
  *_retval = (nsISOAPDecoder *) mDecoders->Get(&nameKey);
  if (*_retval == nsnull && mDefaultEncoding) {
    return mDefaultEncoding->GetDecoder(aKey, _retval);
  }
  return NS_OK;
}

/* nsIDOMElement encode (in nsIVariant aSource, in AString aNamespaceURI, in AString aName, in nsISchemaType aSchemaType, in nsISOAPAttachments aAttachments, in nsIDOMElement aDestination); */
NS_IMETHODIMP
    nsSOAPEncoding::Encode(nsIVariant * aSource,
                           const nsAString & aNamespaceURI,
                           const nsAString & aName,
                           nsISchemaType * aSchemaType,
                           nsISOAPAttachments * aAttachments,
                           nsIDOMElement * aDestination,
                           nsIDOMElement ** _retval)
{
  NS_ENSURE_ARG(aSource);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr < nsISOAPEncoder > encoder;
  nsresult rv = GetDefaultEncoder(getter_AddRefs(encoder));
  if (NS_FAILED(rv))
    return rv;
  if (encoder) {
    return encoder->Encode(this, aSource, aNamespaceURI, aName,
                           aSchemaType, aAttachments, aDestination,
                           _retval);
  }
  *_retval = nsnull;
  return SOAP_EXCEPTION(NS_ERROR_NOT_IMPLEMENTED,"SOAP_DEFAULT_ENCODER", "Encoding style does not have a default encoder.");
}

/* nsIVariant decode (in nsIDOMElement aSource, in nsISchemaType aSchemaType, in nsISOAPAttachments aAttachments); */
NS_IMETHODIMP
    nsSOAPEncoding::Decode(nsIDOMElement * aSource,
                           nsISchemaType * aSchemaType,
                           nsISOAPAttachments * aAttachments,
                           nsIVariant ** _retval)
{
  NS_ENSURE_ARG(aSource);
  NS_ENSURE_ARG_POINTER(_retval);
  nsCOMPtr < nsISOAPDecoder > decoder;
  nsresult rv = GetDefaultDecoder(getter_AddRefs(decoder));
  if (NS_FAILED(rv))
    return rv;
  if (decoder) {
    return decoder->Decode(this, aSource, aSchemaType, aAttachments,
                           _retval);
  }
  *_retval = nsnull;
  return SOAP_EXCEPTION(NS_ERROR_NOT_IMPLEMENTED,"SOAP_DEFAULT_ENCODER", "Encoding style does not have a default decoder.");
}

/* attribute nsISOAPEncoder defaultEncoder; */
NS_IMETHODIMP
    nsSOAPEncoding::GetDefaultEncoder(nsISOAPEncoder * *aDefaultEncoder)
{
  NS_ENSURE_ARG_POINTER(aDefaultEncoder);
  if (mDefaultEncoding && !mDefaultEncoder) {
    return mDefaultEncoding->GetDefaultEncoder(aDefaultEncoder);
  }
  *aDefaultEncoder = mDefaultEncoder;
  NS_IF_ADDREF(*aDefaultEncoder);
  return NS_OK;
}

NS_IMETHODIMP
    nsSOAPEncoding::SetDefaultEncoder(nsISOAPEncoder * aDefaultEncoder)
{
  mDefaultEncoder = aDefaultEncoder;
  return NS_OK;
}

/* attribute nsISOAPDecoder defaultDecoder; */
NS_IMETHODIMP
    nsSOAPEncoding::GetDefaultDecoder(nsISOAPDecoder * *aDefaultDecoder)
{
  NS_ENSURE_ARG_POINTER(aDefaultDecoder);
  if (mDefaultEncoding && !mDefaultDecoder) {
    return mDefaultEncoding->GetDefaultDecoder(aDefaultDecoder);
  }
  *aDefaultDecoder = mDefaultDecoder;
  NS_IF_ADDREF(*aDefaultDecoder);
  return NS_OK;
}

NS_IMETHODIMP
    nsSOAPEncoding::SetDefaultDecoder(nsISOAPDecoder * aDefaultDecoder)
{
  mDefaultDecoder = aDefaultDecoder;
  return NS_OK;
}

/* boolean mapSchemaURI (in AString aExternalURI, in AString aInternalURI, in boolean aOutput); */
NS_IMETHODIMP nsSOAPEncoding::MapSchemaURI(const nsAString & aExternalURI, const nsAString & aInternalURI, PRBool aOutput, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(&aExternalURI);
  NS_ENSURE_ARG_POINTER(&aInternalURI);
  if (aExternalURI.IsEmpty() || aInternalURI.IsEmpty())  //  Permit no empty URIs.
    return SOAP_EXCEPTION(NS_ERROR_ILLEGAL_VALUE,"SOAP_SCHEMA_URI_MAPPING", "No schema URI mapping possible of empty strings.");
  nsStringKey externalKey(aExternalURI);
  if (mMappedExternal->Exists(&externalKey)) {
    *_retval = PR_FALSE;  //  Do not permit duplicate external
    return NS_OK;
  }
  if (aOutput) {
    nsStringKey internalKey(aInternalURI);
    if (mMappedInternal->Exists(&internalKey)) {
      *_retval = PR_FALSE;  //  Do not permit duplicate internal
      return NS_OK;
    }
    nsresult rc;
    nsCOMPtr < nsIWritableVariant > p =
        do_CreateInstance(NS_VARIANT_CONTRACTID, &rc);
    if (NS_FAILED(rc))
      return rc;
    p->SetAsAString(aExternalURI);
    if (NS_FAILED(rc))
      return rc;
    mMappedInternal->Put(&internalKey, p);
  }
  nsresult rc;
  nsCOMPtr < nsIWritableVariant > p =
      do_CreateInstance(NS_VARIANT_CONTRACTID, &rc);
  if (NS_FAILED(rc))
    return rc;
  p->SetAsAString(aInternalURI);
  if (NS_FAILED(rc))
    return rc;
  mMappedExternal->Put(&externalKey, p);
  if (_retval)
    *_retval = PR_TRUE;
  return NS_OK;
}

/* boolean unmapSchemaURI (in AString aExternalURI); */
NS_IMETHODIMP nsSOAPEncoding::UnmapSchemaURI(const nsAString & aExternalURI, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(&aExternalURI);
  nsStringKey externalKey(aExternalURI);
  nsCOMPtr<nsIVariant> internal = dont_AddRef(NS_STATIC_CAST(nsIVariant*,mMappedExternal->Get(&externalKey)));
  if (internal) {
    nsAutoString internalstr;
    nsresult rc = internal->GetAsAString(internalstr);
    if (NS_FAILED(rc))
      return rc;
    nsStringKey internalKey(internalstr);
    mMappedExternal->Remove(&externalKey);
    mMappedInternal->Remove(&internalKey);
    if (_retval)
      *_retval = PR_TRUE;
  }
  else {
    if (_retval)
      *_retval = PR_FALSE;
  }
  return NS_OK;
}

/* AString getInternalSchemaURI (in AString aExternalURI); */
NS_IMETHODIMP nsSOAPEncoding::GetInternalSchemaURI(const nsAString & aExternalURI, nsAString & _retval)
{
  NS_ENSURE_ARG_POINTER(&aExternalURI);
  NS_ENSURE_ARG_POINTER(&_retval);
  if (mMappedExternal->Count()) {
    nsStringKey externalKey(aExternalURI);
    nsCOMPtr<nsIVariant> internal = dont_AddRef(NS_STATIC_CAST(nsIVariant*,mMappedExternal->Get(&externalKey)));
    if (internal) {
      return internal->GetAsAString(_retval);
    }
  }
  if (mDefaultEncoding) {
    return mDefaultEncoding->GetInternalSchemaURI(aExternalURI, _retval);
  }
  _retval.Assign(aExternalURI);
  return NS_OK;
}

/* AString getExternalSchemaURI (in AString aInternalURI); */
NS_IMETHODIMP nsSOAPEncoding::GetExternalSchemaURI(const nsAString & aInternalURI, nsAString & _retval)
{
  NS_ENSURE_ARG_POINTER(&aInternalURI);
  NS_ENSURE_ARG_POINTER(&_retval);
  if (mMappedInternal->Count()) {
    nsStringKey internalKey(aInternalURI);
    nsCOMPtr<nsIVariant> external = dont_AddRef(NS_STATIC_CAST(nsIVariant*,mMappedInternal->Get(&internalKey)));
    if (external) {
      return external->GetAsAString(_retval);
    }
  }
  if (mDefaultEncoding) {
    return mDefaultEncoding->GetExternalSchemaURI(aInternalURI, _retval);
  }
  _retval.Assign(aInternalURI);
  return NS_OK;
}
