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
 * The Original Code is Mozilla Communicator client code.
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

#include "nsScriptNameSpaceManager.h"
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsICategoryManager.h"
#include "nsIServiceManager.h"
#include "nsISupportsPrimitives.h"
#include "nsIScriptExternalNameSet.h"
#include "nsIScriptNameSpaceManager.h"
#include "nsIScriptContext.h"
#include "nsIInterfaceInfoManager.h"
#include "nsIInterfaceInfo.h"
#include "xptinfo.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsDOMClassInfo.h"
#include "nsCRT.h"

#define NS_INTERFACE_PREFIX "nsI"
#define NS_DOM_INTERFACE_PREFIX "nsIDOM"

// Our extended PLDHashEntryHdr
class GlobalNameMapEntry : public PLDHashEntryHdr
{
public:
  // Our hash table ops don't care about the order of these members
  nsString mKey;
  nsGlobalNameStruct mGlobalName;
};


PR_STATIC_CALLBACK(const void *)
GlobalNameHashGetKey(PLDHashTable *table, PLDHashEntryHdr *entry)
{
  GlobalNameMapEntry *e = NS_STATIC_CAST(GlobalNameMapEntry *, entry);

  return NS_STATIC_CAST(const nsAString *, &e->mKey);
}

PR_STATIC_CALLBACK(PLDHashNumber)
GlobalNameHashHashKey(PLDHashTable *table, const void *key)
{
  const nsAString *str = NS_STATIC_CAST(const nsAString *, key);

  return HashString(*str);
}

PR_STATIC_CALLBACK(PRBool)
GlobalNameHashMatchEntry(PLDHashTable *table, const PLDHashEntryHdr *entry,
                         const void *key)
{
  const GlobalNameMapEntry *e =
    NS_STATIC_CAST(const GlobalNameMapEntry *, entry);
  const nsAString *str = NS_STATIC_CAST(const nsAString *, key);

  return str->Equals(e->mKey);
}

PR_STATIC_CALLBACK(void)
GlobalNameHashClearEntry(PLDHashTable *table, PLDHashEntryHdr *entry)
{
  GlobalNameMapEntry *e = NS_STATIC_CAST(GlobalNameMapEntry *, entry);

  // An entry is being cleared, let the key (nsString) do its own
  // cleanup.
  e->mKey.~nsString();
  if (e->mGlobalName.mType == nsGlobalNameStruct::eTypeExternalClassInfo) {
    nsIClassInfo* ci = GET_CLEAN_CI_PTR(e->mGlobalName.mData->mCachedClassInfo);

    // If we constructed an internal helper, we'll let the helper delete 
    // the nsDOMClassInfoData structure, if not we do it here.
    if (!ci || e->mGlobalName.mData->u.mExternalConstructorFptr) {
      delete e->mGlobalName.mData;
    }

    // Release our pointer to the helper.
    NS_IF_RELEASE(ci);
  }

  // This will set e->mGlobalName.mType to
  // nsGlobalNameStruct::eTypeNotInitialized
  memset(&e->mGlobalName, 0, sizeof(nsGlobalNameStruct));
}

PR_STATIC_CALLBACK(void)
GlobalNameHashInitEntry(PLDHashTable *table, PLDHashEntryHdr *entry,
                        const void *key)
{
  GlobalNameMapEntry *e = NS_STATIC_CAST(GlobalNameMapEntry *, entry);
  const nsAString *keyStr = NS_STATIC_CAST(const nsAString *, key);

  // Initialize the key in the entry with placement new
  nsString *str = new (&e->mKey) nsString(*keyStr);

  // This will set e->mGlobalName.mType to
  // nsGlobalNameStruct::eTypeNotInitialized
  memset(&e->mGlobalName, 0, sizeof(nsGlobalNameStruct));
}

nsScriptNameSpaceManager::nsScriptNameSpaceManager()
  : mIsInitialized(PR_FALSE)
{
}

nsScriptNameSpaceManager::~nsScriptNameSpaceManager()
{
  if (mIsInitialized) {
    // Destroy the hash
    PL_DHashTableFinish(&mGlobalNames);
  }
}

nsGlobalNameStruct *
nsScriptNameSpaceManager::AddToHash(const nsAString& aKey)
{
  GlobalNameMapEntry *entry =
    NS_STATIC_CAST(GlobalNameMapEntry *,
                   PL_DHashTableOperate(&mGlobalNames, &aKey, PL_DHASH_ADD));

  if (!entry) {
    return nsnull;
  }

  return &entry->mGlobalName;
}

nsresult
nsScriptNameSpaceManager::FillHash(nsICategoryManager *aCategoryManager,
                                   const char *aCategory,
                                   nsGlobalNameStruct::nametype aType)
{
  nsCOMPtr<nsISimpleEnumerator> e;
  nsresult rv = aCategoryManager->EnumerateCategory(aCategory,
                                                    getter_AddRefs(e));
  NS_ENSURE_SUCCESS(rv, rv);

  nsXPIDLCString categoryEntry;
  nsXPIDLCString contractId;
  nsCOMPtr<nsISupports> entry;

  while (NS_SUCCEEDED(e->GetNext(getter_AddRefs(entry)))) {
    nsCOMPtr<nsISupportsString> category(do_QueryInterface(entry));

    if (!category) {
      NS_WARNING("Category entry not an nsISupportsString!");

      continue;
    }

    rv = category->GetData(getter_Copies(categoryEntry));

    aCategoryManager->GetCategoryEntry(aCategory, categoryEntry,
                                       getter_Copies(contractId));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCID cid;

    rv = nsComponentManager::ContractIDToClassID(contractId, &cid);

    if (NS_FAILED(rv)) {
      NS_WARNING("Bad contract id registed with the script namespace manager");

      // Make sure we don't leak this error code to the caller.
      rv = NS_OK;

      continue;
    }

    nsGlobalNameStruct *s = AddToHash(NS_ConvertASCIItoUCS2(categoryEntry));
    NS_ENSURE_TRUE(s, NS_ERROR_OUT_OF_MEMORY);

    if (s->mType == nsGlobalNameStruct::eTypeNotInitialized) {
      s->mType = aType;
      s->mCID = cid;
    } else {
      NS_WARNING("Global script name not overwritten!");
    }
  }

  return rv;
}


// This method enumerates over all installed interfaces (in .xpt
// files) and finds ones that start with "nsIDOM" and has constants
// defined in the interface itself (inherited constants doesn't
// count), once such an interface is found the "nsIDOM" prefix is cut
// off the name and the rest of the name is added into the hash for
// global names. This makes things like 'Node.ELEMENT_NODE' work in
// JS. See nsWindowSH::GlobalResolve() for detais on how this is used.

nsresult
nsScriptNameSpaceManager::FillHashWithDOMInterfaces()
{
  nsCOMPtr<nsIInterfaceInfoManager> iim =
    dont_AddRef(XPTI_GetInterfaceInfoManager());
  NS_ENSURE_TRUE(iim, NS_ERROR_UNEXPECTED);

  // First look for all interfaces whose name starts with nsIDOM
  nsCOMPtr<nsIEnumerator> domInterfaces;
  nsresult rv =
    iim->EnumerateInterfacesWhoseNamesStartWith(NS_DOM_INTERFACE_PREFIX,
                                                getter_AddRefs(domInterfaces));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> entry;

  rv = domInterfaces->First();

  if (NS_FAILED(rv)) {
    // Empty interface list?

    NS_WARNING("What, no nsIDOM interfaces installed?");

    return NS_OK;
  }

  PRBool found_old;
  nsCOMPtr<nsIInterfaceInfo> if_info;
  nsXPIDLCString if_name;

  for ( ; domInterfaces->IsDone() == NS_COMFALSE; domInterfaces->Next()) {
    rv = domInterfaces->CurrentItem(getter_AddRefs(entry));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIInterfaceInfo> if_info(do_QueryInterface(entry));
    if_info->GetName(getter_Copies(if_name));
    rv = RegisterInterface(if_info,
                           if_name.get() + sizeof(NS_DOM_INTERFACE_PREFIX) - 1,
                           &found_old);

#ifdef DEBUG
    NS_ASSERTION(!found_old,
                 "Whaaa, interface name already in hash!");
#endif
  }

  // Next, look for externally registered DOM interfaces
  rv = RegisterExternalInterfaces(PR_FALSE);

  return rv;
}

nsresult
nsScriptNameSpaceManager::RegisterExternalInterfaces(PRBool aAsProto)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> cm =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInterfaceInfoManager> iim =
    dont_AddRef(XPTI_GetInterfaceInfoManager());
  NS_ENSURE_TRUE(iim, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsISimpleEnumerator> enumerator;
  rv = cm->EnumerateCategory(JAVASCRIPT_DOM_INTERFACE,
                             getter_AddRefs(enumerator));
  NS_ENSURE_SUCCESS(rv, rv);

  nsXPIDLCString IID_string, category_entry;
  const char* if_name;
  nsCOMPtr<nsISupports> entry;
  nsCOMPtr<nsIInterfaceInfo> if_info;
  PRBool found_old, dom_prefix;

  while (NS_SUCCEEDED(enumerator->GetNext(getter_AddRefs(entry)))) {
    nsCOMPtr<nsISupportsString> category(do_QueryInterface(entry));

    if (!category) {
      NS_WARNING("Category entry not an nsISupportsString!");

      continue;
    }

    rv = category->GetData(getter_Copies(category_entry));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = cm->GetCategoryEntry(JAVASCRIPT_DOM_INTERFACE, category_entry,
                              getter_Copies(IID_string));
    NS_ENSURE_SUCCESS(rv, rv);

    nsIID primary_IID;
    if (!primary_IID.Parse(IID_string) ||
        primary_IID.Equals(NS_GET_IID(nsISupports))) {
      NS_ERROR("Invalid IID registered with the script namespace manager!");
      continue;
    }

    iim->GetInfoForIID(&primary_IID, getter_AddRefs(if_info));

    while (if_info) {
      const nsIID *iid;
      if_info->GetIIDShared(&iid);
      NS_ENSURE_TRUE(iid, NS_ERROR_UNEXPECTED);

      if (iid->Equals(NS_GET_IID(nsISupports))) {
        break;
      }

      if_info->GetNameShared(&if_name);
      dom_prefix = (strncmp(if_name, NS_DOM_INTERFACE_PREFIX,
                            sizeof(NS_DOM_INTERFACE_PREFIX) - 1) == 0);

      const char* name;
      if (dom_prefix) {
        if (!aAsProto) {
          // nsIDOM* interfaces have already been registered.
          break;
        }
        name = if_name + sizeof(NS_DOM_INTERFACE_PREFIX) - 1;
      } else {
        name = if_name + sizeof(NS_INTERFACE_PREFIX) - 1;
      }

      if (aAsProto) {
        RegisterClassProto(name, iid, &found_old);
      } else {
        RegisterInterface(if_info, name, &found_old);
      }

      if (found_old) {
        break;
      }

      nsCOMPtr<nsIInterfaceInfo> tmp(if_info);
      tmp->GetParent(getter_AddRefs(if_info));
    }
  }

  return NS_OK;
}

nsresult
nsScriptNameSpaceManager::RegisterInterface(nsIInterfaceInfo* aIfInfo,
                                            const char* aIfName,
                                            PRBool* aFoundOld)
{
  NS_ASSERTION(aIfInfo, "Interface info not an nsIInterfaceInfo!");

  // With the InterfaceInfo system it is actually cheaper to get the 
  // interface name than to get the count of constants. The former is 
  // always cached. The latter might require loading an xpt file!

  PRUint16 constant_count = 0;
  *aFoundOld = PR_FALSE;

  nsresult rv = aIfInfo->GetConstantCount(&constant_count);
  if (NS_FAILED(rv)) {
    NS_ERROR("can't get constant count");
    return rv;
  }

  if (constant_count) {
    PRUint16 parent_constant_count = 0;

    nsCOMPtr<nsIInterfaceInfo> parent_info;

    aIfInfo->GetParent(getter_AddRefs(parent_info));

    if (parent_info) {
      rv = parent_info->GetConstantCount(&parent_constant_count);
      if (NS_FAILED(rv)) {
        NS_ERROR("can't get constant count");
        return rv;
      }
    }

    if (constant_count != parent_constant_count) {
      nsGlobalNameStruct *s = AddToHash(NS_ConvertASCIItoUCS2(aIfName));
      NS_ENSURE_TRUE(s, NS_ERROR_OUT_OF_MEMORY);

      if (s->mType != nsGlobalNameStruct::eTypeNotInitialized) {
          *aFoundOld = PR_TRUE;
          return NS_OK;
      }

      s->mType = nsGlobalNameStruct::eTypeInterface;
    }
  }
  return NS_OK;
}

nsresult
nsScriptNameSpaceManager::Init()
{
  static PLDHashTableOps hash_table_ops =
  {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    GlobalNameHashGetKey,
    GlobalNameHashHashKey,
    GlobalNameHashMatchEntry,
    PL_DHashMoveEntryStub,
    GlobalNameHashClearEntry,
    PL_DHashFinalizeStub,
    GlobalNameHashInitEntry
  };

  mIsInitialized = PL_DHashTableInit(&mGlobalNames, &hash_table_ops, nsnull,
                                     sizeof(GlobalNameMapEntry), 128);
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = NS_OK;

  rv = FillHashWithDOMInterfaces();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsICategoryManager> cm =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = FillHash(cm, JAVASCRIPT_GLOBAL_CONSTRUCTOR_CATEGORY,
                nsGlobalNameStruct::eTypeExternalConstructor);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = FillHash(cm, JAVASCRIPT_GLOBAL_PROPERTY_CATEGORY,
                nsGlobalNameStruct::eTypeProperty);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = FillHash(cm, JAVASCRIPT_GLOBAL_STATIC_NAMESET_CATEGORY,
                nsGlobalNameStruct::eTypeStaticNameSet);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = FillHash(cm, JAVASCRIPT_GLOBAL_DYNAMIC_NAMESET_CATEGORY,
                nsGlobalNameStruct::eTypeDynamicNameSet);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

PR_STATIC_CALLBACK(PLDHashOperator)
NameSetInitCallback(PLDHashTable *table, PLDHashEntryHdr *hdr,
                    PRUint32 number, void *arg)
{
  GlobalNameMapEntry *entry = NS_STATIC_CAST(GlobalNameMapEntry *, hdr);

  if (entry->mGlobalName.mType == nsGlobalNameStruct::eTypeStaticNameSet) {
    nsresult rv = NS_OK;
    nsCOMPtr<nsIScriptExternalNameSet> ns =
      do_CreateInstance(entry->mGlobalName.mCID, &rv);
    NS_ENSURE_SUCCESS(rv, PL_DHASH_NEXT);

    rv = ns->InitializeNameSet(NS_STATIC_CAST(nsIScriptContext *, arg));
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "Initing external script classes failed!");
  }

  return PL_DHASH_NEXT;
}

nsresult
nsScriptNameSpaceManager::InitForContext(nsIScriptContext *aContext)
{
  PL_DHashTableEnumerate(&mGlobalNames, NameSetInitCallback, aContext);

  return NS_OK;
}

nsresult
nsScriptNameSpaceManager::LookupName(const nsAString& aName,
                                     const nsGlobalNameStruct **aNameStruct)
{
  GlobalNameMapEntry *entry =
    NS_STATIC_CAST(GlobalNameMapEntry *,
                   PL_DHashTableOperate(&mGlobalNames, &aName,
                                        PL_DHASH_LOOKUP));

  if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
    *aNameStruct = &entry->mGlobalName;
  } else {
    *aNameStruct = nsnull;
  }

  return NS_OK;
}

nsresult
nsScriptNameSpaceManager::RegisterClassName(const char *aClassName,
                                            PRInt32 aDOMClassInfoID)
{
  if (!nsCRT::IsAscii(aClassName)) {
    NS_ERROR("Trying to register a non-ASCII class name");
    return NS_OK;
  }
  nsGlobalNameStruct *s = AddToHash(NS_ConvertASCIItoUCS2(aClassName));
  NS_ENSURE_TRUE(s, NS_ERROR_OUT_OF_MEMORY);

  if (s->mType == nsGlobalNameStruct::eTypeClassConstructor) {
    return NS_OK;
  }

  // If a external constructor is already defined with aClassName we
  // won't overwrite it.

  if (s->mType == nsGlobalNameStruct::eTypeExternalConstructor) {
    return NS_OK;
  }

  NS_ASSERTION(s->mType == nsGlobalNameStruct::eTypeNotInitialized ||
               s->mType == nsGlobalNameStruct::eTypeInterface,
               "Whaaa, JS environment name clash!");

  s->mType = nsGlobalNameStruct::eTypeClassConstructor;
  s->mDOMClassInfoID = aDOMClassInfoID;

  return NS_OK;
}

nsresult
nsScriptNameSpaceManager::RegisterClassProto(const char *aClassName,
                                             const nsIID *aConstructorProtoIID,
                                             PRBool *aFoundOld)
{
  NS_ENSURE_ARG_POINTER(aConstructorProtoIID);

  *aFoundOld = PR_FALSE;

  nsGlobalNameStruct *s = AddToHash(NS_ConvertASCIItoUCS2(aClassName));
  NS_ENSURE_TRUE(s, NS_ERROR_OUT_OF_MEMORY);

  if (s->mType != nsGlobalNameStruct::eTypeNotInitialized &&
      s->mType != nsGlobalNameStruct::eTypeInterface) {
    *aFoundOld = PR_TRUE;

    return NS_OK;
  }

  s->mType = nsGlobalNameStruct::eTypeClassProto;
  s->mIID = *aConstructorProtoIID;

  return NS_OK;
}

nsresult
nsScriptNameSpaceManager::RegisterExternalClassName(const char *aClassName,
                                                    nsCID& aCID)
{
  nsGlobalNameStruct *s = AddToHash(NS_ConvertASCIItoUCS2(aClassName));
  NS_ENSURE_TRUE(s, NS_ERROR_OUT_OF_MEMORY);

  // If an external constructor is already defined with aClassName we
  // won't overwrite it.

  if (s->mType == nsGlobalNameStruct::eTypeExternalConstructor) {
    return NS_OK;
  }

  NS_ASSERTION(s->mType == nsGlobalNameStruct::eTypeNotInitialized ||
               s->mType == nsGlobalNameStruct::eTypeInterface,
               "Whaaa, JS environment name clash!");

  s->mType = nsGlobalNameStruct::eTypeExternalClassInfoCreator;
  s->mCID = aCID;

  return NS_OK;
}

nsresult
nsScriptNameSpaceManager::RegisterDOMCIData(const char *aName,
                                            nsDOMClassInfoExternalConstructorFnc aConstructorFptr,
                                            const nsIID *aProtoChainInterface,
                                            const nsIID **aInterfaces,
                                            PRUint32 aScriptableFlags,
                                            PRBool aHasClassInterface,
                                            const nsCID *aConstructorCID)
{
  nsGlobalNameStruct *s = AddToHash(NS_ConvertASCIItoUCS2(aName));
  NS_ENSURE_TRUE(s, NS_ERROR_OUT_OF_MEMORY);

  // If an external constructor is already defined with aClassName we
  // won't overwrite it.

  if (s->mType == nsGlobalNameStruct::eTypeClassConstructor ||
      s->mType == nsGlobalNameStruct::eTypeExternalClassInfo) {
    return NS_OK;
  }

  // XXX Should we bail out here?
  NS_ASSERTION(s->mType == nsGlobalNameStruct::eTypeNotInitialized ||
               s->mType == nsGlobalNameStruct::eTypeExternalClassInfoCreator,
               "Someone tries to register classinfo data for a class that isn't new or external!");

  s->mData = new nsExternalDOMClassInfoData;
  NS_ENSURE_TRUE(s->mData, NS_ERROR_OUT_OF_MEMORY);

  s->mType = nsGlobalNameStruct::eTypeExternalClassInfo;
  s->mData->mName = aName;
  if (aConstructorFptr)
    s->mData->u.mExternalConstructorFptr = aConstructorFptr;
  else
    // null constructor will cause us to use nsDOMGenericSH::doCreate
    s->mData->u.mExternalConstructorFptr = nsnull;
  s->mData->mCachedClassInfo = nsnull;
  s->mData->mProtoChainInterface = aProtoChainInterface;
  s->mData->mInterfaces = aInterfaces;
  s->mData->mScriptableFlags = aScriptableFlags;
  s->mData->mHasClassInterface = aHasClassInterface;
  s->mData->mConstructorCID = aConstructorCID;

  return NS_OK;
}
