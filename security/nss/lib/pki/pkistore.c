/* 
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
 * The Original Code is the Netscape security libraries.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1994-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: pkistore.c,v $ $Revision: 1.15.6.1 $ $Date: 2002/04/10 03:28:12 $ $Name: MOZILLA_1_0_RELEASE $";
#endif /* DEBUG */

#ifndef PKIM_H
#include "pkim.h"
#endif /* PKIM_H */

#ifndef PKI_H
#include "pki.h"
#endif /* PKI_H */

#ifndef NSSPKI_H
#include "nsspki.h"
#endif /* NSSPKI_H */

#ifndef BASE_H
#include "base.h"
#endif /* BASE_H */

#ifndef PKISTORE_H
#include "pkistore.h"
#endif /* PKISTORE_H */

#ifdef NSS_3_4_CODE
#include "cert.h"
#endif

/* 
 * Certificate Store
 *
 * This differs from the cache in that it is a true storage facility.  Items
 * stay in until they are explicitly removed.  It is only used by crypto
 * contexts at this time, but may be more generally useful...
 *
 */

struct nssCertificateStoreStr 
{
    PRBool i_alloced_arena;
    NSSArena *arena;
    PZLock *lock;
    nssHash *subject;
    nssHash *issuer_and_serial;
};

typedef struct certificate_hash_entry_str certificate_hash_entry;

struct certificate_hash_entry_str
{
    NSSCertificate *cert;
    NSSTrust *trust;
    nssSMIMEProfile *profile;
};

/* XXX This a common function that should be moved out, possibly an
 *     nssSubjectCertificateList should be created?
 */
/* sort the subject list from newest to oldest */
static PRIntn subject_list_sort(void *v1, void *v2)
{
    NSSCertificate *c1 = (NSSCertificate *)v1;
    NSSCertificate *c2 = (NSSCertificate *)v2;
    nssDecodedCert *dc1 = nssCertificate_GetDecoding(c1);
    nssDecodedCert *dc2 = nssCertificate_GetDecoding(c2);
    if (dc1->isNewerThan(dc1, dc2)) {
	return -1;
    } else {
	return 1;
    }
}

NSS_IMPLEMENT nssCertificateStore *
nssCertificateStore_Create
(
  NSSArena *arenaOpt
)
{
    NSSArena *arena;
    nssCertificateStore *store;
    PRBool i_alloced_arena;
    if (arenaOpt) {
	arena = arenaOpt;
	i_alloced_arena = PR_FALSE;
    } else {
	arena = nssArena_Create();
	if (!arena) {
	    return NULL;
	}
	i_alloced_arena = PR_TRUE;
    }
    store = nss_ZNEW(arena, nssCertificateStore);
    if (!store) {
	goto loser;
    }
    store->lock = PZ_NewLock(nssILockOther);
    if (!store->lock) {
	goto loser;
    }
    /* Create the issuer/serial --> {cert, trust, S/MIME profile } hash */
    store->issuer_and_serial = nssHash_CreateCertificate(arena, 0);
    if (!store->issuer_and_serial) {
	goto loser;
    }
    /* Create the subject DER --> subject list hash */
    store->subject = nssHash_CreateItem(arena, 0);
    if (!store->subject) {
	goto loser;
    }
    store->arena = arena;
    store->i_alloced_arena = i_alloced_arena;
    return store;
loser:
    if (store) {
	if (store->lock) {
	    PZ_DestroyLock(store->lock);
	}
	if (store->issuer_and_serial) {
	    nssHash_Destroy(store->issuer_and_serial);
	}
	if (store->subject) {
	    nssHash_Destroy(store->subject);
	}
    }
    if (i_alloced_arena) {
	nssArena_Destroy(arena);
    }
    return NULL;
}

NSS_IMPLEMENT void
nssCertificateStore_Destroy
(
  nssCertificateStore *store
)
{
    PZ_DestroyLock(store->lock);
    nssHash_Destroy(store->issuer_and_serial);
    nssHash_Destroy(store->subject);
    if (store->i_alloced_arena) {
	nssArena_Destroy(store->arena);
    } else {
	nss_ZFreeIf(store);
    }
}

static PRStatus
add_certificate_entry
(
  nssCertificateStore *store,
  NSSCertificate *cert
)
{
    PRStatus nssrv;
    certificate_hash_entry *entry;
    entry = nss_ZNEW(cert->object.arena, certificate_hash_entry);
    if (!entry) {
	return PR_FAILURE;
    }
    entry->cert = cert;
    nssrv = nssHash_Add(store->issuer_and_serial, cert, entry);
    if (nssrv != PR_SUCCESS) {
	nss_ZFreeIf(entry);
    }
    return nssrv;
}

static PRStatus
add_subject_entry
(
  nssCertificateStore *store,
  NSSCertificate *cert
)
{
    PRStatus nssrv;
    nssList *subjectList;
    subjectList = (nssList *)nssHash_Lookup(store->subject, &cert->subject);
    if (subjectList) {
	/* The subject is already in, add this cert to the list */
	nssrv = nssList_AddUnique(subjectList, cert);
    } else {
	/* Create a new subject list for the subject */
	subjectList = nssList_Create(NULL, PR_FALSE);
	if (!subjectList) {
	    return PR_FAILURE;
	}
	nssList_SetSortFunction(subjectList, subject_list_sort);
	/* Add the cert entry to this list of subjects */
	nssrv = nssList_Add(subjectList, cert);
	if (nssrv != PR_SUCCESS) {
	    return nssrv;
	}
	/* Add the subject list to the cache */
	nssrv = nssHash_Add(store->subject, &cert->subject, subjectList);
    }
    return nssrv;
}

/* declared below */
static void
remove_certificate_entry
(
  nssCertificateStore *store,
  NSSCertificate *cert
);

NSS_IMPLEMENT PRStatus
nssCertificateStore_Add
(
  nssCertificateStore *store,
  NSSCertificate *cert
)
{
    PRStatus nssrv;
    PZ_Lock(store->lock);
    if (nssHash_Exists(store->issuer_and_serial, cert)) {
	PZ_Unlock(store->lock);
	return PR_SUCCESS;
    }
    nssrv = add_certificate_entry(store, cert);
    if (nssrv == PR_SUCCESS) {
	nssrv = add_subject_entry(store, cert);
	if (nssrv == PR_SUCCESS) {
	    nssCertificate_AddRef(cert); /* obtain a reference for the store */
	} else {
	    remove_certificate_entry(store, cert);
	}
    }
    PZ_Unlock(store->lock);
    return nssrv;
}

static void
remove_certificate_entry
(
  nssCertificateStore *store,
  NSSCertificate *cert
)
{
    certificate_hash_entry *entry;
    entry = (certificate_hash_entry *)
                             nssHash_Lookup(store->issuer_and_serial, cert);
    if (entry) {
	nssHash_Remove(store->issuer_and_serial, cert);
	if (entry->trust) {
	    nssTrust_Destroy(entry->trust);
	}
	if (entry->profile) {
	    nssSMIMEProfile_Destroy(entry->profile);
	}
	nss_ZFreeIf(entry);
    }
}

static void
remove_subject_entry
(
  nssCertificateStore *store,
  NSSCertificate *cert
)
{
    nssList *subjectList;
    /* Get the subject list for the cert's subject */
    subjectList = (nssList *)nssHash_Lookup(store->subject, &cert->subject);
    if (subjectList) {
	/* Remove the cert from the subject hash */
	nssList_Remove(subjectList, cert);
	nssHash_Remove(store->subject, &cert->subject);
	if (nssList_Count(subjectList) == 0) {
	    nssList_Destroy(subjectList);
	} else {
	    /* The cert being released may have keyed the subject entry.
	     * Since there are still subject certs around, get another and
	     * rekey the entry just in case.
	     */
	    NSSCertificate *subjectCert;
	    (void)nssList_GetArray(subjectList, (void **)&subjectCert, 1);
	    nssHash_Add(store->subject, &subjectCert->subject, subjectList);
	}
    }
}

NSS_IMPLEMENT void
nssCertificateStore_Remove
(
  nssCertificateStore *store,
  NSSCertificate *cert
)
{
    certificate_hash_entry *entry;
    PZ_Lock(store->lock);
#ifdef NSS_3_4_CODE
    if (cert->object.refCount > 2) {
	/* This continues the hack described in CERT_DestroyCertificate.
	 * Because NSS 3.4 maintains a single, global, crypto context,
	 * certs must be explicitly removed from it when there are no
	 * more references to them.  This is done by destroying the cert
	 * when there are two references left, the one being destroyed,
	 * and the one here (read: temp db).
	 * However, there is a race condition with timing the removal
	 * of the cert from the temp store and deleting the last
	 * reference.  In CERT_DestroyCertificate, the refCount is checked,
	 * and if it is two, a call is made here to remove the temp cert.
	 * But by the time it gets here (and within the safety of the
	 * store's lock), another thread could have grabbed a reference
	 * to it.  Removing it now will wreak havoc.
	 * Therefore, it is necessary to check the refCount again, 
	 * after obtaining the store's lock, to make sure the cert is
	 * actually ready to be deleted.  This check is safe, because
	 * within the store's lock a cert that has only two references
	 * *must* have one in the store, and the one being deleted.
	 * See bug 125263.
	 */
	PZ_Unlock(store->lock);
	return;
    }
#endif
    entry = (certificate_hash_entry *)
                              nssHash_Lookup(store->issuer_and_serial, cert);
    if (entry && entry->cert == cert) {
	remove_certificate_entry(store, cert);
	remove_subject_entry(store, cert);
	NSSCertificate_Destroy(cert); /* release the store's reference */
    }
    PZ_Unlock(store->lock);
}

static NSSCertificate **
get_array_from_list
(
  nssList *certList,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt,
  NSSArena *arenaOpt
)
{
    PRUint32 count;
    NSSCertificate **rvArray = NULL;
    count = nssList_Count(certList);
    if (count == 0) {
	return NULL;
    }
    if (maximumOpt > 0) {
	count = PR_MIN(maximumOpt, count);
    }
    if (rvOpt) {
	nssList_GetArray(certList, (void **)rvOpt, count);
    } else {
	rvArray = nss_ZNEWARRAY(arenaOpt, NSSCertificate *, count + 1);
	if (rvArray) {
	    nssList_GetArray(certList, (void **)rvArray, count);
	}
    }
    return rvArray;
}

NSS_IMPLEMENT NSSCertificate **
nssCertificateStore_FindCertificatesBySubject
(
  nssCertificateStore *store,
  NSSDER *subject,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt,
  NSSArena *arenaOpt
)
{
    NSSCertificate **rvArray = NULL;
    nssList *subjectList;
    PZ_Lock(store->lock);
    subjectList = (nssList *)nssHash_Lookup(store->subject, subject);
    if (subjectList) {
	nssCertificateList_AddReferences(subjectList);
	rvArray = get_array_from_list(subjectList, 
	                              rvOpt, maximumOpt, arenaOpt);
    }
    PZ_Unlock(store->lock);
    return rvArray;
}

/* Because only subject indexing is implemented, all other lookups require
 * full traversal (unfortunately, PLHashTable doesn't allow you to exit
 * early from the enumeration).  The assumptions are that 1) lookups by 
 * fields other than subject will be rare, and 2) the hash will not have
 * a large number of entries.  These assumptions will be tested.
 *
 * XXX
 * For NSS 3.4, it is worth consideration to do all forms of indexing,
 * because the only crypto context is global and persistent.
 */

struct nickname_template_str
{
    NSSUTF8 *nickname;
    nssList *subjectList;
};

static void match_nickname(const void *k, void *v, void *a)
{
    PRStatus nssrv;
    NSSCertificate *c;
    NSSUTF8 *nickname;
    nssList *subjectList = (nssList *)v;
    struct nickname_template_str *nt = (struct nickname_template_str *)a;
    nssrv = nssList_GetArray(subjectList, (void **)&c, 1);
    nickname = NSSCertificate_GetNickname(c, NULL);
    if (nssrv == PR_SUCCESS && 
         nssUTF8_Equal(nickname, nt->nickname, &nssrv)) 
    {
	nt->subjectList = subjectList;
    }
}

/*
 * Find all cached certs with this label.
 */
NSS_IMPLEMENT NSSCertificate **
nssCertificateStore_FindCertificatesByNickname
(
  nssCertificateStore *store,
  NSSUTF8 *nickname,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt,
  NSSArena *arenaOpt
)
{
    NSSCertificate **rvArray = NULL;
    struct nickname_template_str nt;
    nt.nickname = nickname;
    nt.subjectList = NULL;
    PZ_Lock(store->lock);
    nssHash_Iterate(store->subject, match_nickname, &nt);
    if (nt.subjectList) {
	nssCertificateList_AddReferences(nt.subjectList);
	rvArray = get_array_from_list(nt.subjectList, 
	                              rvOpt, maximumOpt, arenaOpt);
    }
    PZ_Unlock(store->lock);
    return rvArray;
}

struct email_template_str
{
    NSSASCII7 *email;
    nssList *emailList;
};

static void match_email(const void *k, void *v, void *a)
{
    PRStatus nssrv;
    NSSCertificate *c;
    nssList *subjectList = (nssList *)v;
    struct email_template_str *et = (struct email_template_str *)a;
    nssrv = nssList_GetArray(subjectList, (void **)&c, 1);
    if (nssrv == PR_SUCCESS && 
         nssUTF8_Equal(c->email, et->email, &nssrv)) 
    {
	nssListIterator *iter = nssList_CreateIterator(subjectList);
	if (iter) {
	    for (c  = (NSSCertificate *)nssListIterator_Start(iter);
	         c != (NSSCertificate *)NULL;
	         c  = (NSSCertificate *)nssListIterator_Next(iter))
	    {
		nssList_Add(et->emailList, c);
	    }
	    nssListIterator_Finish(iter);
	    nssListIterator_Destroy(iter);
	}
    }
}

/*
 * Find all cached certs with this email address.
 */
NSS_IMPLEMENT NSSCertificate **
nssCertificateStore_FindCertificatesByEmail
(
  nssCertificateStore *store,
  NSSASCII7 *email,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt,
  NSSArena *arenaOpt
)
{
    NSSCertificate **rvArray = NULL;
    struct email_template_str et;
    et.email = email;
    et.emailList = nssList_Create(NULL, PR_FALSE);
    if (!et.emailList) {
	return NULL;
    }
    PZ_Lock(store->lock);
    nssHash_Iterate(store->subject, match_email, &et);
    if (et.emailList) {
	/* get references before leaving the store's lock protection */
	nssCertificateList_AddReferences(et.emailList);
    }
    PZ_Unlock(store->lock);
    if (et.emailList) {
	rvArray = get_array_from_list(et.emailList, 
	                              rvOpt, maximumOpt, arenaOpt);
	nssList_Destroy(et.emailList);
    }
    return rvArray;
}

NSS_IMPLEMENT NSSCertificate *
nssCertificateStore_FindCertificateByIssuerAndSerialNumber
(
  nssCertificateStore *store,
  NSSDER *issuer,
  NSSDER *serial
)
{
    certificate_hash_entry *entry;
    NSSCertificate index;
    NSSCertificate *rvCert = NULL;
    index.issuer = *issuer;
    index.serial = *serial;
    PZ_Lock(store->lock);
    entry = (certificate_hash_entry *)
                           nssHash_Lookup(store->issuer_and_serial, &index);
    if (entry) {
	rvCert = nssCertificate_AddRef(entry->cert);
    }
    PZ_Unlock(store->lock);
    return rvCert;
}

#ifdef NSS_3_4_CODE
static PRStatus
issuer_and_serial_from_encoding
(
  NSSBER *encoding, 
  NSSDER *issuer, 
  NSSDER *serial
)
{
    SECItem derCert, derIssuer, derSerial;
    SECStatus secrv;
    derCert.data = (unsigned char *)encoding->data;
    derCert.len = encoding->size;
    secrv = CERT_IssuerNameFromDERCert(&derCert, &derIssuer);
    if (secrv != SECSuccess) {
	return PR_FAILURE;
    }
    secrv = CERT_SerialNumberFromDERCert(&derCert, &derSerial);
    if (secrv != SECSuccess) {
	PORT_Free(derIssuer.data);
	return PR_FAILURE;
    }
    issuer->data = derIssuer.data;
    issuer->size = derIssuer.len;
    serial->data = derSerial.data;
    serial->size = derSerial.len;
    return PR_SUCCESS;
}
#endif

NSS_IMPLEMENT NSSCertificate *
nssCertificateStore_FindCertificateByEncodedCertificate
(
  nssCertificateStore *store,
  NSSDER *encoding
)
{
    PRStatus nssrv = PR_FAILURE;
    NSSDER issuer, serial;
    NSSCertificate *rvCert = NULL;
#ifdef NSS_3_4_CODE
    nssrv = issuer_and_serial_from_encoding(encoding, &issuer, &serial);
#endif
    if (nssrv != PR_SUCCESS) {
	return NULL;
    }
    rvCert = nssCertificateStore_FindCertificateByIssuerAndSerialNumber(store, 
                                                                     &issuer, 
                                                                     &serial);
#ifdef NSS_3_4_CODE
    PORT_Free(issuer.data);
    PORT_Free(serial.data);
#endif
    return rvCert;
}

NSS_EXTERN PRStatus
nssCertificateStore_AddTrust
(
  nssCertificateStore *store,
  NSSTrust *trust
)
{
    NSSCertificate *cert;
    certificate_hash_entry *entry;
    cert = trust->certificate;
    PZ_Lock(store->lock);
    entry = (certificate_hash_entry *)
                              nssHash_Lookup(store->issuer_and_serial, cert);
    if (entry) {
	entry->trust = nssTrust_AddRef(trust);
    }
    PZ_Unlock(store->lock);
    return (entry) ? PR_SUCCESS : PR_FAILURE;
}

NSS_IMPLEMENT NSSTrust *
nssCertificateStore_FindTrustForCertificate
(
  nssCertificateStore *store,
  NSSCertificate *cert
)
{
    certificate_hash_entry *entry;
    NSSTrust *rvTrust = NULL;
    PZ_Lock(store->lock);
    entry = (certificate_hash_entry *)
                              nssHash_Lookup(store->issuer_and_serial, cert);
    if (entry && entry->trust) {
	rvTrust = nssTrust_AddRef(entry->trust);
    }
    PZ_Unlock(store->lock);
    return rvTrust;
}

NSS_EXTERN PRStatus
nssCertificateStore_AddSMIMEProfile
(
  nssCertificateStore *store,
  nssSMIMEProfile *profile
)
{
    NSSCertificate *cert;
    certificate_hash_entry *entry;
    cert = profile->certificate;
    PZ_Lock(store->lock);
    entry = (certificate_hash_entry *)
                              nssHash_Lookup(store->issuer_and_serial, cert);
    if (entry) {
	entry->profile = nssSMIMEProfile_AddRef(profile);
    }
    PZ_Unlock(store->lock);
    return (entry) ? PR_SUCCESS : PR_FAILURE;
}

NSS_IMPLEMENT nssSMIMEProfile *
nssCertificateStore_FindSMIMEProfileForCertificate
(
  nssCertificateStore *store,
  NSSCertificate *cert
)
{
    certificate_hash_entry *entry;
    nssSMIMEProfile *rvProfile = NULL;
    PZ_Lock(store->lock);
    entry = (certificate_hash_entry *)
                              nssHash_Lookup(store->issuer_and_serial, cert);
    if (entry && entry->profile) {
	rvProfile = nssSMIMEProfile_AddRef(entry->profile);
    }
    PZ_Unlock(store->lock);
    return rvProfile;
}

/* XXX this is also used by cache and should be somewhere else */

static PLHashNumber
nss_certificate_hash
(
  const void *key
)
{
    int i;
    PLHashNumber h;
    NSSCertificate *c = (NSSCertificate *)key;
    h = 0;
    for (i=0; i<c->issuer.size; i++)
	h = (h >> 28) ^ (h << 4) ^ ((unsigned char *)c->issuer.data)[i];
    for (i=0; i<c->serial.size; i++)
	h = (h >> 28) ^ (h << 4) ^ ((unsigned char *)c->serial.data)[i];
    return h;
}

static int
nss_compare_certs(const void *v1, const void *v2)
{
    PRStatus ignore;
    NSSCertificate *c1 = (NSSCertificate *)v1;
    NSSCertificate *c2 = (NSSCertificate *)v2;
    return (int)(nssItem_Equal(&c1->issuer, &c2->issuer, &ignore) &&
                 nssItem_Equal(&c1->serial, &c2->serial, &ignore));
}

NSS_IMPLEMENT nssHash *
nssHash_CreateCertificate
(
  NSSArena *arenaOpt,
  PRUint32 numBuckets
)
{
    return nssHash_Create(arenaOpt, 
                          numBuckets, 
                          nss_certificate_hash, 
                          nss_compare_certs, 
                          PL_CompareValues);
}

NSS_IMPLEMENT void
nssCertificateStore_DumpStoreInfo
(
  nssCertificateStore *store,
  void (* cert_dump_iter)(const void *, void *, void *),
  void *arg
)
{
    PZ_Lock(store->lock);
    nssHash_Iterate(store->issuer_and_serial, cert_dump_iter, arg);
    PZ_Unlock(store->lock);
}

