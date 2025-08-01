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

/*
 * CMS miscellaneous utility functions.
 *
 * $Id: cmsutil.c,v 1.7.18.1 2002/04/10 03:29:02 cltbld%netscape.com Exp $
 */

#include "nssrenam.h"

#include "cmslocal.h"

#include "cert.h"
#include "key.h"
#include "secasn1.h"
#include "secitem.h"
#include "secoid.h"
#include "pk11func.h"
#include "secerr.h"
#include "sechash.h"

/*
 * NSS_CMSArray_SortByDER - sort array of objects by objects' DER encoding
 *
 * make sure that the order of the objects guarantees valid DER (which must be
 * in lexigraphically ascending order for a SET OF); if reordering is necessary it
 * will be done in place (in objs).
 */
SECStatus
NSS_CMSArray_SortByDER(void **objs, const SEC_ASN1Template *objtemplate, void **objs2)
{
    PRArenaPool *poolp;
    int num_objs;
    SECItem **enc_objs;
    SECStatus rv = SECFailure;
    int i;

    if (objs == NULL)					/* already sorted */
	return SECSuccess;

    num_objs = NSS_CMSArray_Count((void **)objs);
    if (num_objs == 0 || num_objs == 1)		/* already sorted. */
	return SECSuccess;

    poolp = PORT_NewArena (1024);	/* arena for temporaries */
    if (poolp == NULL)
	return SECFailure;		/* no memory; nothing we can do... */

    /*
     * Allocate arrays to hold the individual encodings which we will use
     * for comparisons and the reordered attributes as they are sorted.
     */
    enc_objs = (SECItem **)PORT_ArenaZAlloc(poolp, (num_objs + 1) * sizeof(SECItem *));
    if (enc_objs == NULL)
	goto loser;

    /* DER encode each individual object. */
    for (i = 0; i < num_objs; i++) {
	enc_objs[i] = SEC_ASN1EncodeItem(poolp, NULL, objs[i], objtemplate);
	if (enc_objs[i] == NULL)
	    goto loser;
    }
    enc_objs[num_objs] = NULL;

    /* now compare and sort objs by the order of enc_objs */
    NSS_CMSArray_Sort((void **)enc_objs, NSS_CMSUtil_DERCompare, objs, objs2);

    rv = SECSuccess;

loser:
    PORT_FreeArena (poolp, PR_FALSE);
    return rv;
}

/*
 * NSS_CMSUtil_DERCompare - for use with NSS_CMSArray_Sort to
 *  sort arrays of SECItems containing DER
 */
int
NSS_CMSUtil_DERCompare(void *a, void *b)
{
    SECItem *der1 = (SECItem *)a;
    SECItem *der2 = (SECItem *)b;
    int j;

    /*
     * Find the lowest (lexigraphically) encoding.  One that is
     * shorter than all the rest is known to be "less" because each
     * attribute is of the same type (a SEQUENCE) and so thus the
     * first octet of each is the same, and the second octet is
     * the length (or the length of the length with the high bit
     * set, followed by the length, which also works out to always
     * order the shorter first).  Two (or more) that have the
     * same length need to be compared byte by byte until a mismatch
     * is found.
     */
    if (der1->len != der2->len)
	return (der1->len < der2->len) ? -1 : 1;

    for (j = 0; j < der1->len; j++) {
	if (der1->data[j] == der2->data[j])
	    continue;
	return (der1->data[j] < der2->data[j]) ? -1 : 1;
    }
    return 0;
}

/*
 * NSS_CMSAlgArray_GetIndexByAlgID - find a specific algorithm in an array of 
 * algorithms.
 *
 * algorithmArray - array of algorithm IDs
 * algid - algorithmid of algorithm to pick
 *
 * Returns:
 *  An integer containing the index of the algorithm in the array or -1 if 
 *  algorithm was not found.
 */
int
NSS_CMSAlgArray_GetIndexByAlgID(SECAlgorithmID **algorithmArray, SECAlgorithmID *algid)
{
    int i;

    if (algorithmArray == NULL || algorithmArray[0] == NULL)
	return -1;

    for (i = 0; algorithmArray[i] != NULL; i++) {
	if (SECOID_CompareAlgorithmID(algorithmArray[i], algid) == SECEqual)
	    break;	/* bingo */
    }

    if (algorithmArray[i] == NULL)
	return -1;	/* not found */

    return i;
}

/*
 * NSS_CMSAlgArray_GetIndexByAlgTag - find a specific algorithm in an array of 
 * algorithms.
 *
 * algorithmArray - array of algorithm IDs
 * algtag - algorithm tag of algorithm to pick
 *
 * Returns:
 *  An integer containing the index of the algorithm in the array or -1 if 
 *  algorithm was not found.
 */
int
NSS_CMSAlgArray_GetIndexByAlgTag(SECAlgorithmID **algorithmArray, SECOidTag algtag)
{
    SECOidData *algid;
    int i;

    if (algorithmArray == NULL || algorithmArray[0] == NULL)
	return -1;

    for (i = 0; algorithmArray[i] != NULL; i++) {
	algid = SECOID_FindOID(&(algorithmArray[i]->algorithm));
	if (algid->offset == algtag)
	    break;	/* bingo */
    }

    if (algorithmArray[i] == NULL)
	return -1;	/* not found */

    return i;
}

const SECHashObject *
NSS_CMSUtil_GetHashObjByAlgID(SECAlgorithmID *algid)
{
    SECOidData *oiddata;
    const SECHashObject *digobj;

    /* here are the algorithms we know */
    oiddata = SECOID_FindOID(&(algid->algorithm));
    if (oiddata == NULL) {
	digobj = NULL;
    } else {
	switch (oiddata->offset) {
	case SEC_OID_MD2:
	    digobj = HASH_GetHashObject(HASH_AlgMD2);
	    break;
	case SEC_OID_MD5:
	    digobj = HASH_GetHashObject(HASH_AlgMD5);
	    break;
	case SEC_OID_SHA1:
	    digobj = HASH_GetHashObject(HASH_AlgSHA1);
	    break;
	default:
	    digobj = NULL;
	    break;
	}
    }
    return digobj;
}

/*
 * XXX I would *really* like to not have to do this, but the current
 * signing interface gives me little choice.
 */
SECOidTag
NSS_CMSUtil_MakeSignatureAlgorithm(SECOidTag hashalg, SECOidTag encalg)
{
    switch (encalg) {
      case SEC_OID_PKCS1_RSA_ENCRYPTION:
	switch (hashalg) {
	  case SEC_OID_MD2:
	    return SEC_OID_PKCS1_MD2_WITH_RSA_ENCRYPTION;
	  case SEC_OID_MD5:
	    return SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION;
	  case SEC_OID_SHA1:
	    return SEC_OID_PKCS1_SHA1_WITH_RSA_ENCRYPTION;
	  default:
	    return SEC_OID_UNKNOWN;
	}
      case SEC_OID_ANSIX9_DSA_SIGNATURE:
      case SEC_OID_MISSI_KEA_DSS:
      case SEC_OID_MISSI_DSS:
	switch (hashalg) {
	  case SEC_OID_SHA1:
	    return SEC_OID_ANSIX9_DSA_SIGNATURE_WITH_SHA1_DIGEST;
	  default:
	    return SEC_OID_UNKNOWN;
	}
      default:
	break;
    }

    return encalg;		/* maybe it is already the right algid */
}

const SEC_ASN1Template *
NSS_CMSUtil_GetTemplateByTypeTag(SECOidTag type)
{
    const SEC_ASN1Template *template;
    extern const SEC_ASN1Template NSSCMSSignedDataTemplate[];
    extern const SEC_ASN1Template NSSCMSEnvelopedDataTemplate[];
    extern const SEC_ASN1Template NSSCMSEncryptedDataTemplate[];
    extern const SEC_ASN1Template NSSCMSDigestedDataTemplate[];

    switch (type) {
    case SEC_OID_PKCS7_SIGNED_DATA:
	template = NSSCMSSignedDataTemplate;
	break;
    case SEC_OID_PKCS7_ENVELOPED_DATA:
	template = NSSCMSEnvelopedDataTemplate;
	break;
    case SEC_OID_PKCS7_ENCRYPTED_DATA:
	template = NSSCMSEncryptedDataTemplate;
	break;
    case SEC_OID_PKCS7_DIGESTED_DATA:
	template = NSSCMSDigestedDataTemplate;
	break;
    default:
    case SEC_OID_PKCS7_DATA:
	template = NULL;
	break;
    }
    return template;
}

size_t
NSS_CMSUtil_GetSizeByTypeTag(SECOidTag type)
{
    size_t size;

    switch (type) {
    case SEC_OID_PKCS7_SIGNED_DATA:
	size = sizeof(NSSCMSSignedData);
	break;
    case SEC_OID_PKCS7_ENVELOPED_DATA:
	size = sizeof(NSSCMSEnvelopedData);
	break;
    case SEC_OID_PKCS7_ENCRYPTED_DATA:
	size = sizeof(NSSCMSEncryptedData);
	break;
    case SEC_OID_PKCS7_DIGESTED_DATA:
	size = sizeof(NSSCMSDigestedData);
	break;
    default:
    case SEC_OID_PKCS7_DATA:
	size = 0;
	break;
    }
    return size;
}

NSSCMSContentInfo *
NSS_CMSContent_GetContentInfo(void *msg, SECOidTag type)
{
    NSSCMSContent c;
    NSSCMSContentInfo *cinfo;

    PORT_Assert(msg != NULL);

    c.pointer = msg;
    switch (type) {
    case SEC_OID_PKCS7_SIGNED_DATA:
	cinfo = &(c.signedData->contentInfo);
	break;
    case SEC_OID_PKCS7_ENVELOPED_DATA:
	cinfo = &(c.envelopedData->contentInfo);
	break;
    case SEC_OID_PKCS7_ENCRYPTED_DATA:
	cinfo = &(c.encryptedData->contentInfo);
	break;
    case SEC_OID_PKCS7_DIGESTED_DATA:
	cinfo = &(c.digestedData->contentInfo);
	break;
    default:
	cinfo = NULL;
    }
    return cinfo;
}

const char *
NSS_CMSUtil_VerificationStatusToString(NSSCMSVerificationStatus vs)
{
    switch (vs) {
    case NSSCMSVS_Unverified:			return "Unverified";
    case NSSCMSVS_GoodSignature:		return "GoodSignature";
    case NSSCMSVS_BadSignature:			return "BadSignature";
    case NSSCMSVS_DigestMismatch:		return "DigestMismatch";
    case NSSCMSVS_SigningCertNotFound:		return "SigningCertNotFound";
    case NSSCMSVS_SigningCertNotTrusted:	return "SigningCertNotTrusted";
    case NSSCMSVS_SignatureAlgorithmUnknown:	return "SignatureAlgorithmUnknown";
    case NSSCMSVS_SignatureAlgorithmUnsupported: return "SignatureAlgorithmUnsupported";
    case NSSCMSVS_MalformedSignature:		return "MalformedSignature";
    case NSSCMSVS_ProcessingError:		return "ProcessingError";
    default:					return "Unknown";
    }
}

SECStatus
NSS_CMSDEREncode(NSSCMSMessage *cmsg, SECItem *input, SECItem *derOut, 
                 PLArenaPool *arena)
{
    NSSCMSEncoderContext *ecx;
    SECStatus rv = SECSuccess;
    if (!cmsg || !derOut || !arena) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    ecx = NSS_CMSEncoder_Start(cmsg, 0, 0, derOut, arena, 0, 0, 0, 0, 0, 0);
    if (!ecx) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    if (input) {
	rv = NSS_CMSEncoder_Update(ecx, (const char*)input->data, input->len);
	if (rv) {
	    PORT_SetError(SEC_ERROR_BAD_DATA);
	}
    }
    rv |= NSS_CMSEncoder_Finish(ecx);
    if (rv) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
    }
    return rv;
}
