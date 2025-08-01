/*
 * crypto.h - public data structures and prototypes for the crypto library
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
 *
 * $Id: cryptohi.h,v 1.4.18.1 2002/04/10 03:27:02 cltbld%netscape.com Exp $
 */

#ifndef _CRYPTOHI_H_
#define _CRYPTOHI_H_

#include "blapi.h"

#include "seccomon.h"
#include "secoidt.h"
#include "secdert.h"
#include "cryptoht.h"
#include "keyt.h"
#include "certt.h"


SEC_BEGIN_PROTOS


/****************************************/
/*
** DER encode/decode DSA signatures
*/

/* ANSI X9.57 defines DSA signatures as DER encoded data.  Our DSA code (and
 * most of the rest of the world) just generates 40 bytes of raw data.  These
 * functions convert between formats.
 */
extern SECStatus DSAU_EncodeDerSig(SECItem *dest, SECItem *src);
extern SECItem *DSAU_DecodeDerSig(SECItem *item);



/****************************************/
/*
** Signature creation operations
*/

/*
** Create a new signature context used for signing a data stream.
**	"alg" the signature algorithm to use (e.g. SEC_OID_RSA_WITH_MD5)
**	"privKey" the private key to use
*/
extern SGNContext *SGN_NewContext(SECOidTag alg, SECKEYPrivateKey *privKey);

/*
** Destroy a signature-context object
**	"key" the object
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void SGN_DestroyContext(SGNContext *cx, PRBool freeit);

/*
** Reset the signing context "cx" to its initial state, preparing it for
** another stream of data.
*/
extern SECStatus SGN_Begin(SGNContext *cx);

/*
** Update the signing context with more data to sign.
**	"cx" the context
**	"input" the input data to sign
**	"inputLen" the length of the input data
*/
extern SECStatus SGN_Update(SGNContext *cx, unsigned char *input,
			   unsigned int inputLen);

/*
** Finish the signature process. Use either k0 or k1 to sign the data
** stream that was input using SGN_Update. The resulting signature is
** formatted using PKCS#1 and then encrypted using RSA private or public
** encryption.
**	"cx" the context
**	"result" the final signature data (memory is allocated)
*/
extern SECStatus SGN_End(SGNContext *cx, SECItem *result);

/*
** Sign a single block of data using private key encryption and given
** signature/hash algorithm.
**	"result" the final signature data (memory is allocated)
**	"buf" the input data to sign
**	"len" the amount of data to sign
**	"pk" the private key to encrypt with
**	"algid" the signature/hash algorithm to sign with 
**		(must be compatible with the key type).
*/
extern SECStatus SEC_SignData(SECItem *result, unsigned char *buf, int len,
			     SECKEYPrivateKey *pk, SECOidTag algid);

/*
** Sign a pre-digested block of data using private key encryption, encoding
**  The given signature/hash algorithm.
**	"result" the final signature data (memory is allocated)
**	"digest" the digest to sign
**	"pk" the private key to encrypt with
**	"algtag" The algorithm tag to encode (need for RSA only)
*/
extern SECStatus SGN_Digest(SECKEYPrivateKey *privKey,
                SECOidTag algtag, SECItem *result, SECItem *digest);

/*
** DER sign a single block of data using private key encryption and the
** MD5 hashing algorithm. This routine first computes a digital signature
** using SEC_SignData, then wraps it with an CERTSignedData and then der
** encodes the result.
**	"arena" is the memory arena to use to allocate data from
** 	"result" the final der encoded data (memory is allocated)
** 	"buf" the input data to sign
** 	"len" the amount of data to sign
** 	"pk" the private key to encrypt with
*/
extern SECStatus SEC_DerSignData(PRArenaPool *arena, SECItem *result,
				unsigned char *buf, int len,
				SECKEYPrivateKey *pk, SECOidTag algid);

/*
** Destroy a signed-data object.
**	"sd" the object
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void SEC_DestroySignedData(CERTSignedData *sd, PRBool freeit);

/****************************************/
/*
** Signature verification operations
*/

/*
** Create a signature verification context.
**	"key" the public key to verify with
**	"sig" the encrypted signature data if sig is NULL then
**	   VFY_EndWithSignature must be called with the correct signature at
**	   the end of the processing.
**	"algid" specifies the signing algorithm to use.  This must match
**	    the key type.
**	"wincx" void pointer to the window context
*/
extern VFYContext *VFY_CreateContext(SECKEYPublicKey *key, SECItem *sig,
				     SECOidTag algid, void *wincx);

/*
** Destroy a verification-context object.
**	"cx" the context to destroy
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void VFY_DestroyContext(VFYContext *cx, PRBool freeit);

extern SECStatus VFY_Begin(VFYContext *cx);

/*
** Update a verification context with more input data. The input data
** is fed to a secure hash function (depending on what was in the
** encrypted signature data).
**	"cx" the context
**	"input" the input data
**	"inputLen" the amount of input data
*/
extern SECStatus VFY_Update(VFYContext *cx, unsigned char *input,
			    unsigned int inputLen);

/*
** Finish the verification process. The return value is a status which
** indicates success or failure. On success, the SECSuccess value is
** returned. Otherwise, SECFailure is returned and the error code found
** using PORT_GetError() indicates what failure occurred.
** 	"cx" the context
*/
extern SECStatus VFY_End(VFYContext *cx);

/*
** Finish the verification process. The return value is a status which
** indicates success or failure. On success, the SECSuccess value is
** returned. Otherwise, SECFailure is returned and the error code found
** using PORT_GetError() indicates what failure occurred. If signature is
** supplied the verification uses this signature to verify, otherwise the
** signature passed in VFY_CreateContext() is used. 
** VFY_EndWithSignature(cx,NULL); is identical to VFY_End(cx);.
** 	"cx" the context
** 	"sig" the encrypted signature data
*/
extern SECStatus VFY_EndWithSignature(VFYContext *cx, SECItem *sig);


/*
** Verify the signature on a block of data for which we already have
** the digest. The signature data is an RSA private key encrypted
** block of data formatted according to PKCS#1.
** 	"dig" the digest
** 	"key" the public key to check the signature with
** 	"sig" the encrypted signature data
**	"algid" specifies the signing algorithm to use.  This must match
**	    the key type.
**/
extern SECStatus VFY_VerifyDigest(SECItem *dig, SECKEYPublicKey *key,
				  SECItem *sig, SECOidTag algid, void *wincx);

/*
** Verify the signature on a block of data. The signature data is an RSA
** private key encrypted block of data formatted according to PKCS#1.
** 	"buf" the input data
** 	"len" the length of the input data
** 	"key" the public key to check the signature with
** 	"sig" the encrypted signature data
**	"algid" specifies the signing algorithm to use.  This must match
**	    the key type.
*/
extern SECStatus VFY_VerifyData(unsigned char *buf, int len,
				SECKEYPublicKey *key, SECItem *sig,
				SECOidTag algid, void *wincx);


SEC_END_PROTOS

#endif /* _CRYPTOHI_H_ */
