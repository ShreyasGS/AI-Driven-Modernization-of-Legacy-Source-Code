/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is nsDiskCacheBinding.cpp, released May 10, 2001.
 * 
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *    Patrick C. Beard <beard@netscape.com>
 *    Gordon Sheridan  <gordon@netscape.com>
 */

#include <limits.h>

#include "nscore.h"
#include "nsDiskCacheBinding.h"



/******************************************************************************
 *  static hash table callback functions
 *
 *****************************************************************************/
#ifdef XP_MAC
#pragma mark -
#pragma mark HASHTABLE CALLBACKS
#endif

struct HashTableEntry : PLDHashEntryHdr {
    nsDiskCacheBinding *  mBinding;
};


static const void * PR_CALLBACK
GetKey(PLDHashTable * /*table*/, PLDHashEntryHdr * header)
{
    return (void*) ((HashTableEntry *)header)->mBinding->mRecord.HashNumber();
}


static PLDHashNumber PR_CALLBACK
HashKey( PLDHashTable *table, const void *key)
{
    return (PLDHashNumber) NS_PTR_TO_INT32(key);
}


static PRBool PR_CALLBACK
MatchEntry(PLDHashTable *              /* table */,
            const PLDHashEntryHdr *       header,
            const void *                  key)
{
    HashTableEntry * hashEntry = (HashTableEntry *) header;
    return (hashEntry->mBinding->mRecord.HashNumber() == (PLDHashNumber) NS_PTR_TO_INT32(key));
}

static void PR_CALLBACK
MoveEntry(PLDHashTable *           /* table */,
          const PLDHashEntryHdr *     src,
          PLDHashEntryHdr       *     dst)
{
    ((HashTableEntry *)dst)->mBinding = ((HashTableEntry *)src)->mBinding;
}


static void PR_CALLBACK
ClearEntry(PLDHashTable *      /* table */,
           PLDHashEntryHdr *      header)
{
    ((HashTableEntry *)header)->mBinding = nsnull;
}


/******************************************************************************
 *  Utility Functions
 *****************************************************************************/
#ifdef XP_MAC
#pragma mark -
#pragma mark DISK CACHE BINDERY
#endif

nsDiskCacheBinding *
GetCacheEntryBinding(nsCacheEntry * entry)
{
    nsCOMPtr<nsISupports> data;
    nsresult rv = entry->GetData(getter_AddRefs(data));
    if (NS_FAILED(rv))  return nsnull;
    
    return (nsDiskCacheBinding *)data.get();
}


/******************************************************************************
 *  nsDiskCacheBinding
 *****************************************************************************/

NS_IMPL_THREADSAFE_ISUPPORTS0(nsDiskCacheBinding);

nsDiskCacheBinding::nsDiskCacheBinding(nsCacheEntry* entry, nsDiskCacheRecord * record)
    :   mCacheEntry(entry)
{
    NS_ASSERTION(record->ValidRecord(), "bad record");
    NS_INIT_ISUPPORTS();
    PR_INIT_CLIST(this);
    mRecord     = *record;
    mDoomed     = entry->IsDoomed();
    mGeneration = record->Generation();    // 0 == uninitialized, or data & meta using block files
}

nsDiskCacheBinding::~nsDiskCacheBinding()
{
    NS_ASSERTION(PR_CLIST_IS_EMPTY(this), "binding deleted while still on list");
    if (!PR_CLIST_IS_EMPTY(this))
        PR_REMOVE_LINK(this);       // XXX why are we still on a list?
}


/******************************************************************************
 *  nsDiskCacheBindery
 *
 *  Keeps track of bound disk cache entries to detect for collisions.
 *
 *****************************************************************************/

PLDHashTableOps nsDiskCacheBindery::ops =
{
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    GetKey,
    HashKey,
    MatchEntry,
    MoveEntry,
    ClearEntry,
    PL_DHashFinalizeStub
};


nsDiskCacheBindery::nsDiskCacheBindery()
    : initialized(PR_FALSE)
{
}


nsDiskCacheBindery::~nsDiskCacheBindery()
{
    Reset();
}


nsresult
nsDiskCacheBindery::Init()
{
    nsresult rv = NS_OK;
    initialized = PL_DHashTableInit(&table, &ops, nsnull, sizeof(HashTableEntry), 0);

    if (!initialized) rv = NS_ERROR_OUT_OF_MEMORY;
    
    return rv;
}

void
nsDiskCacheBindery::Reset()
{
    if (initialized) {
        PL_DHashTableFinish(&table);
        initialized = PR_FALSE;
    }
}


nsDiskCacheBinding *
nsDiskCacheBindery::CreateBinding(nsCacheEntry *       entry,
                                  nsDiskCacheRecord *  record)
{
    NS_ASSERTION(initialized, "nsDiskCacheBindery not initialized");
    nsCOMPtr<nsISupports> data;
    nsresult rv = entry->GetData(getter_AddRefs(data));
    if (NS_FAILED(rv) || data) {
        NS_ASSERTION(!data, "cache entry already has bind data");
        return nsnull;
    }
    
    nsDiskCacheBinding * binding = new nsDiskCacheBinding(entry, record);
    if (!binding)  return nsnull;
        
    // give ownership of the binding to the entry
    entry->SetData(binding);
    
    // add binding to collision detection system
    rv = AddBinding(binding);
    if (NS_FAILED(rv)) {
        entry->SetData(nsnull);
        return nsnull;
    }

    return binding;
}


/**
 *  FindActiveEntry :  to find active colliding entry so we can doom it
 */
nsDiskCacheBinding *
nsDiskCacheBindery::FindActiveBinding(PRUint32  hashNumber)
{
    NS_ASSERTION(initialized, "nsDiskCacheBindery not initialized");
    // find hash entry for key
    HashTableEntry * hashEntry;
    hashEntry = (HashTableEntry *) PL_DHashTableOperate(&table, (void*) hashNumber, PL_DHASH_LOOKUP);
    if (PL_DHASH_ENTRY_IS_FREE(hashEntry)) return nsnull;
    
    // walk list looking for active entry
    NS_ASSERTION(hashEntry->mBinding, "hash entry left with no binding");
    nsDiskCacheBinding * binding = hashEntry->mBinding;    
    while (binding->mCacheEntry->IsDoomed()) {
        binding = (nsDiskCacheBinding *)PR_NEXT_LINK(binding);
        if (binding == hashEntry->mBinding)  return nsnull;
    }
    return binding;
}


/**
 *  FindBinding :  to identify whether a record is 'in use' so we don't evict it
 */
nsDiskCacheBinding *
nsDiskCacheBindery::FindBinding(nsDiskCacheRecord * record)
{    
    NS_ASSERTION(initialized, "nsDiskCacheBindery not initialized");
    // find hash entry for key
    HashTableEntry * hashEntry;
    hashEntry = (HashTableEntry *) PL_DHashTableOperate(&table, (void*) record->HashNumber(), PL_DHASH_LOOKUP);
    if (PL_DHASH_ENTRY_IS_FREE(hashEntry)) return nsnull;

    // walk list looking for matching record (match on MetaLocation)
    NS_ASSERTION(hashEntry->mBinding, "hash entry left with no binding");
    nsDiskCacheBinding * binding = hashEntry->mBinding;    
    while (binding->mRecord.MetaLocation() != record->MetaLocation()) {
        binding = (nsDiskCacheBinding *)PR_NEXT_LINK(binding);
        if (binding == hashEntry->mBinding)  return nsnull;
    }
    return binding;
}



/**
 *  AddBinding
 *
 *  Called from FindEntry() if we read an entry off of disk
 *      - it may already have a generation number
 *      - a generation number conflict is an error
 *
 *  Called from BindEntry()
 *      - a generation number needs to be assigned
 */
nsresult
nsDiskCacheBindery::AddBinding(nsDiskCacheBinding * binding)
{
    NS_ENSURE_ARG_POINTER(binding);
    NS_ASSERTION(initialized, "nsDiskCacheBindery not initialized");

    // find hash entry for key
    HashTableEntry * hashEntry;
    hashEntry = (HashTableEntry *) PL_DHashTableOperate(&table,
                                                        (void*) binding->mRecord.HashNumber(),
                                                        PL_DHASH_ADD);
    if (!hashEntry) return NS_ERROR_OUT_OF_MEMORY;
    
    if (hashEntry->mBinding == nsnull) {
        hashEntry->mBinding = binding;
        if (binding->mGeneration == 0)
            binding->mGeneration = 1;   // if generation uninitialized, set it to 1
            
        return NS_OK;
    }
    
    
    // insert binding in generation order
    nsDiskCacheBinding * p  = hashEntry->mBinding;
    PRBool   calcGeneration = (binding->mGeneration == 0);  // do we need to calculate generation?
    if (calcGeneration)  binding->mGeneration = 1;          // initialize to 1 if uninitialized
    while (1) {
    
        if (binding->mGeneration < p->mGeneration) {
            // here we are
            PR_INSERT_BEFORE(binding, p);
            if (hashEntry->mBinding == p)
                hashEntry->mBinding = binding;
            break;
        }
        
        if (binding->mGeneration == p->mGeneration) {
            if (calcGeneration)  ++binding->mGeneration;    // try the next generation
            else {
                NS_ASSERTION(binding->mGeneration != p->mGeneration,  "generations collide!");
                return NS_ERROR_UNEXPECTED;
            }
        }

        p = (nsDiskCacheBinding *)PR_NEXT_LINK(p);
        if (p == hashEntry->mBinding) {
            // end of line: insert here or die
            p = (nsDiskCacheBinding *)PR_PREV_LINK(p);  // back up and check generation
            if (p->mGeneration == 255) {
                NS_ASSERTION(p->mGeneration < 255, "generation capacity at full, the engines canna take it cap'n");
                return NS_ERROR_UNEXPECTED;
            }
            PR_INSERT_BEFORE(binding, hashEntry->mBinding);
            break;
        }
    }
    return NS_OK;
}


/**
 *  RemoveBinding :  remove binding from collision detection on deactivation
 */
void
nsDiskCacheBindery::RemoveBinding(nsDiskCacheBinding * binding)
{
    NS_ASSERTION(initialized, "nsDiskCacheBindery not initialized");
    if (!initialized)   return;
    
    HashTableEntry * hashEntry;
    void *           key = (void *)binding->mRecord.HashNumber();

    hashEntry = (HashTableEntry*) PL_DHashTableOperate(&table, (void*) key, PL_DHASH_LOOKUP);
    if (!PL_DHASH_ENTRY_IS_BUSY(hashEntry)) {
        NS_ASSERTION(PL_DHASH_ENTRY_IS_BUSY(hashEntry), "binding not in disk cache hashtable!");
        return;
    }
    
    if (binding == hashEntry->mBinding) {
        if (PR_CLIST_IS_EMPTY(binding)) {
            // remove this hash entry
            (void) PL_DHashTableOperate(&table, (void*) binding->mRecord.HashNumber(), PL_DHASH_REMOVE);
            return;
            
        } else {
            // promote next binding to head, and unlink this binding
            hashEntry->mBinding = (nsDiskCacheBinding *)PR_NEXT_LINK(binding);
        }
    }
    PR_REMOVE_AND_INIT_LINK(binding);
}
