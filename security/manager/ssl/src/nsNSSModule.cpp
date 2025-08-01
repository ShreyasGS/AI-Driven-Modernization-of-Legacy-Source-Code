/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *   Hubbie Shaw
 *   Doug Turner <dougt@netscape.com>
 *   Brian Ryner <bryner@netscape.com>
 */

#include "nsIModule.h"
#include "nsIGenericFactory.h"

#include "nsNSSComponent.h"
#include "nsSSLSocketProvider.h"
#include "nsTLSSocketProvider.h"
#include "nsKeygenHandler.h"

#include "nsSDR.h"

#include "nsPK11TokenDB.h"
#include "nsPKCS11Slot.h"
#include "nsNSSCertificate.h"
#include "nsCertTree.h"
#include "nsCrypto.h"
//For the NS_CRYPTO_CONTRACTID define
#include "nsDOMCID.h"

#include "nsCMSSecureMessage.h"
#include "nsCMS.h"
#include "nsCertPicker.h"
#include "nsCURILoader.h"
#include "nsICategoryManager.h"

// We must ensure that the nsNSSComponent has been loaded before
// creating any other components.
static void EnsureNSSInitialized(PRBool triggeredByNSSComponent)
{
  static PRBool haveLoaded = PR_FALSE;
  if (haveLoaded)
    return;

  haveLoaded = PR_TRUE;
  
  if (triggeredByNSSComponent) {
    // Me must prevent a recursion, as nsNSSComponent creates
    // additional instances
    return;
  }
  
  nsCOMPtr<nsISupports> nssComponent 
    = do_GetService(PSM_COMPONENT_CONTRACTID);
}

// These two macros are ripped off from nsIGenericFactory.h and slightly
// modified.
#define NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(triggeredByNSSComponent,           \
                                                      _InstanceClass)         \
static NS_IMETHODIMP                                                          \
_InstanceClass##Constructor(nsISupports *aOuter, REFNSIID aIID,               \
                            void **aResult)                                   \
{                                                                             \
    nsresult rv;                                                              \
    _InstanceClass * inst;                                                    \
                                                                              \
    EnsureNSSInitialized(triggeredByNSSComponent);                            \
                                                                              \
    *aResult = NULL;                                                          \
    if (NULL != aOuter) {                                                     \
        rv = NS_ERROR_NO_AGGREGATION;                                         \
        return rv;                                                            \
    }                                                                         \
                                                                              \
    NS_NEWXPCOM(inst, _InstanceClass);                                        \
    if (NULL == inst) {                                                       \
        rv = NS_ERROR_OUT_OF_MEMORY;                                          \
        return rv;                                                            \
    }                                                                         \
    NS_ADDREF(inst);                                                          \
    rv = inst->QueryInterface(aIID, aResult);                                 \
    NS_RELEASE(inst);                                                         \
                                                                              \
    return rv;                                                                \
}                                                                             \

 
#define NS_NSS_GENERIC_FACTORY_CONSTRUCTOR_INIT(triggeredByNSSComponent,      \
                                                _InstanceClass, _InitMethod)  \
static NS_IMETHODIMP                                                          \
_InstanceClass##Constructor(nsISupports *aOuter, REFNSIID aIID,               \
                            void **aResult)                                   \
{                                                                             \
    nsresult rv;                                                              \
    _InstanceClass * inst;                                                    \
                                                                              \
    EnsureNSSInitialized(triggeredByNSSComponent);                            \
                                                                              \
    *aResult = NULL;                                                          \
    if (NULL != aOuter) {                                                     \
        rv = NS_ERROR_NO_AGGREGATION;                                         \
        return rv;                                                            \
    }                                                                         \
                                                                              \
    NS_NEWXPCOM(inst, _InstanceClass);                                        \
    if (NULL == inst) {                                                       \
        rv = NS_ERROR_OUT_OF_MEMORY;                                          \
        return rv;                                                            \
    }                                                                         \
    NS_ADDREF(inst);                                                          \
    rv = inst->_InitMethod();                                                 \
    if(NS_SUCCEEDED(rv)) {                                                    \
        rv = inst->QueryInterface(aIID, aResult);                             \
    }                                                                         \
    NS_RELEASE(inst);                                                         \
                                                                              \
    return rv;                                                                \
}                                                                             \

NS_NSS_GENERIC_FACTORY_CONSTRUCTOR_INIT(PR_TRUE, nsNSSComponent, Init)

// Use the special factory constructor for everything this module implements,
// because all code could potentially require the NSS library.
// Our factory constructor takes an additional boolean parameter.
// Only for the nsNSSComponent, set this to PR_TRUE.
// All other classes must have this set to PR_FALSE.

NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsSSLSocketProvider)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsTLSSocketProvider)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsSecretDecoderRing)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsPK11TokenDB)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsPKCS11ModuleDB)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR_INIT(PR_FALSE, PSMContentListener, init)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsNSSCertificateDB)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsCertTree)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsCrypto)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsPkcs11)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsCMSSecureMessage)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsCMSDecoder)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsCMSEncoder)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsCMSMessage)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsHash)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsCertPicker)

static NS_METHOD RegisterPSMContentListeners(
                      nsIComponentManager *aCompMgr,
                      nsIFile *aPath, const char *registryLocation, 
                      const char *componentType, const nsModuleComponentInfo *info)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catman = 
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsXPIDLCString previous;

  catman->AddCategoryEntry(
    NS_CONTENT_LISTENER_CATEGORYMANAGER_ENTRY,
    "application/x-x509-ca-cert",
    info->mContractID, PR_TRUE, PR_TRUE, getter_Copies(previous));

  catman->AddCategoryEntry(
    NS_CONTENT_LISTENER_CATEGORYMANAGER_ENTRY,
    "application/x-x509-server-cert",
    info->mContractID, PR_TRUE, PR_TRUE, getter_Copies(previous));

  catman->AddCategoryEntry(
    NS_CONTENT_LISTENER_CATEGORYMANAGER_ENTRY,
    "application/x-x509-user-cert",
    info->mContractID, PR_TRUE, PR_TRUE, getter_Copies(previous));

  catman->AddCategoryEntry(
    NS_CONTENT_LISTENER_CATEGORYMANAGER_ENTRY,
    "application/x-x509-email-cert",
    info->mContractID, PR_TRUE, PR_TRUE, getter_Copies(previous));

  catman->AddCategoryEntry(
    NS_CONTENT_LISTENER_CATEGORYMANAGER_ENTRY,
    "application/x-pkcs7-crl",
    info->mContractID, PR_TRUE, PR_TRUE, getter_Copies(previous));

  catman->AddCategoryEntry(
    NS_CONTENT_LISTENER_CATEGORYMANAGER_ENTRY,
    "application/x-x509-crl",
    info->mContractID, PR_TRUE, PR_TRUE, getter_Copies(previous));

  catman->AddCategoryEntry(
    NS_CONTENT_LISTENER_CATEGORYMANAGER_ENTRY,
    "application/pkix-crl",
    info->mContractID, PR_TRUE, PR_TRUE, getter_Copies(previous));

  return NS_OK;
}

static const nsModuleComponentInfo components[] =
{
  {
    PSM_COMPONENT_CLASSNAME,
    NS_NSSCOMPONENT_CID,
    PSM_COMPONENT_CONTRACTID,
    nsNSSComponentConstructor
  },
  
  {
    NS_ISSLSOCKETPROVIDER_CLASSNAME,
    NS_SSLSOCKETPROVIDER_CID,
    NS_ISSLSOCKETPROVIDER_CONTRACTID,
    nsSSLSocketProviderConstructor
  },
  
  {
    NS_TLSSTEPUPSOCKETPROVIDER_CLASSNAME,
    NS_TLSSTEPUPSOCKETPROVIDER_CID,
    NS_TLSSTEPUPSOCKETPROVIDER_CONTRACTID,
    nsTLSSocketProviderConstructor
  },
  
  {
    NS_ISSLFHSOCKETPROVIDER_CLASSNAME,
    NS_SSLSOCKETPROVIDER_CID,
    NS_ISSLFHSOCKETPROVIDER_CONTRACTID,
    nsSSLSocketProviderConstructor
  },

  {
    NS_SDR_CLASSNAME,
    NS_SDR_CID,
    NS_SDR_CONTRACTID,
    nsSecretDecoderRingConstructor
  },

  {
    "PK11 Token Database",
    NS_PK11TOKENDB_CID,
    NS_PK11TOKENDB_CONTRACTID,
    nsPK11TokenDBConstructor
  },

  {
    "PKCS11 Module Database",
    NS_PKCS11MODULEDB_CID,
    NS_PKCS11MODULEDB_CONTRACTID,
    nsPKCS11ModuleDBConstructor
  },

  {
    "Generic Certificate Content Handler",
    NS_PSMCONTENTLISTEN_CID,
    NS_PSMCONTENTLISTEN_CONTRACTID,
    PSMContentListenerConstructor
  },

  {
    "X509 Certificate Database",
    NS_X509CERTDB_CID,
    NS_X509CERTDB_CONTRACTID,
    nsNSSCertificateDBConstructor
  },

  {
    "Form Processor",
    NS_FORMPROCESSOR_CID,
    NS_FORMPROCESSOR_CONTRACTID,
    nsKeygenFormProcessor::Create
  },

  {
    "Certificate Tree",
    NS_CERTTREE_CID,
    NS_CERTTREE_CONTRACTID,
    nsCertTreeConstructor
  },

  {
    NS_PKCS11_CLASSNAME,
    NS_PKCS11_CID,
    NS_PKCS11_CONTRACTID,
    nsPkcs11Constructor
  },

  {
    NS_CRYPTO_CLASSNAME,
    NS_CRYPTO_CID,
    NS_CRYPTO_CONTRACTID,
    nsCryptoConstructor
  },

  {
    NS_CMSSECUREMESSAGE_CLASSNAME,
    NS_CMSSECUREMESSAGE_CID,
    NS_CMSSECUREMESSAGE_CONTRACTID,
    nsCMSSecureMessageConstructor
  },

  {
    NS_CMSDECODER_CLASSNAME,
    NS_CMSDECODER_CID,
    NS_CMSDECODER_CONTRACTID,
    nsCMSDecoderConstructor
  },

  {
    NS_CMSENCODER_CLASSNAME,
    NS_CMSENCODER_CID,
    NS_CMSENCODER_CONTRACTID,
    nsCMSEncoderConstructor
  },

  {
    NS_CMSMESSAGE_CLASSNAME,
    NS_CMSMESSAGE_CID,
    NS_CMSMESSAGE_CONTRACTID,
    nsCMSMessageConstructor
  },

  {
    NS_HASH_CLASSNAME,
    NS_HASH_CID,
    NS_HASH_CONTRACTID,
    nsHashConstructor
  },

  {
    NS_CERT_PICKER_CLASSNAME,
    NS_CERT_PICKER_CID,
    NS_CERT_PICKER_CONTRACTID,
    nsCertPickerConstructor
  },

  {
    "PSM Content Listeners",
    NS_PSMCONTENTLISTEN_CID,
    "@mozilla.org/uriloader/psm-external-content-listener;1",
    PSMContentListenerConstructor,
    RegisterPSMContentListeners
  }
};

NS_IMPL_NSGETMODULE(NSS, components);
