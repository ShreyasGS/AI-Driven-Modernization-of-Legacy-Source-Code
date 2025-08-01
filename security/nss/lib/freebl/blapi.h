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
 * $Id: blapi.h,v 1.10.18.1 2002/04/10 03:27:20 cltbld%netscape.com Exp $
 */

#ifndef _BLAPI_H_
#define _BLAPI_H_

#include "blapit.h"


SEC_BEGIN_PROTOS

/*
** RSA encryption/decryption. When encrypting/decrypting the output
** buffer must be at least the size of the public key modulus.
*/

/*
** Generate and return a new RSA public and private key.
**	Both keys are encoded in a single RSAPrivateKey structure.
**	"cx" is the random number generator context
**	"keySizeInBits" is the size of the key to be generated, in bits.
**	   512, 1024, etc.
**	"publicExponent" when not NULL is a pointer to some data that
**	   represents the public exponent to use. The data is a byte
**	   encoded integer, in "big endian" order.
*/
extern RSAPrivateKey *RSA_NewKey(int         keySizeInBits,
				 SECItem *   publicExponent);

/*
** Perform a raw public-key operation 
**	Length of input and output buffers are equal to key's modulus len.
*/
extern SECStatus RSA_PublicKeyOp(RSAPublicKey *   key,
				 unsigned char *  output,
				 const unsigned char *  input);

/*
** Perform a raw private-key operation 
**	Length of input and output buffers are equal to key's modulus len.
*/
extern SECStatus RSA_PrivateKeyOp(RSAPrivateKey *  key,
				  unsigned char *  output,
				  const unsigned char *  input);

/*
** Perform a raw private-key operation, and check the parameters used in
** the operation for validity by performing a test operation first.
**	Length of input and output buffers are equal to key's modulus len.
*/
extern SECStatus RSA_PrivateKeyOpDoubleChecked(RSAPrivateKey *  key,
				               unsigned char *  output,
				               const unsigned char *  input);

/*
** Perform a check of private key parameters for consistency.
*/
extern SECStatus RSA_PrivateKeyCheck(RSAPrivateKey *key);


/********************************************************************
** DSA signing algorithm
*/

/*
** Generate and return a new DSA public and private key pair,
**	both of which are encoded into a single DSAPrivateKey struct.
**	"params" is a pointer to the PQG parameters for the domain
**	Uses a random seed.
*/
extern SECStatus DSA_NewKey(const PQGParams *     params, 
		            DSAPrivateKey **      privKey);

/* signature is caller-supplied buffer of at least 20 bytes.
** On input,  signature->len == size of buffer to hold signature.
**            digest->len    == size of digest.
** On output, signature->len == size of signature in buffer.
** Uses a random seed.
*/
extern SECStatus DSA_SignDigest(DSAPrivateKey *   key,
				SECItem *         signature,
				const SECItem *   digest);

/* signature is caller-supplied buffer of at least 20 bytes.
** On input,  signature->len == size of buffer to hold signature.
**            digest->len    == size of digest.
*/
extern SECStatus DSA_VerifyDigest(DSAPublicKey *  key,
				  const SECItem * signature,
				  const SECItem * digest);

/* For FIPS compliance testing. Seed must be exactly 20 bytes long */
extern SECStatus DSA_NewKeyFromSeed(const PQGParams *params, 
                                    const unsigned char * seed,
                                    DSAPrivateKey **privKey);

/* For FIPS compliance testing. Seed must be exactly 20 bytes. */
extern SECStatus DSA_SignDigestWithSeed(DSAPrivateKey * key,
                                        SECItem *       signature,
                                        const SECItem * digest,
                                        const unsigned char * seed);

/******************************************************
** Diffie Helman key exchange algorithm 
*/

/* Generates parameters for Diffie-Helman key generation.
**	primeLen is the length in bytes of prime P to be generated.
*/
extern SECStatus DH_GenParam(int primeLen, DHParams ** params);

/* Generates a public and private key, both of which are encoded in a single
**	DHPrivateKey struct. Params is input, privKey are output.  
**	This is Phase 1 of Diffie Hellman.
*/
extern SECStatus DH_NewKey(DHParams *           params, 
                           DHPrivateKey **	privKey);

/* 
** DH_Derive does the Diffie-Hellman phase 2 calculation, using the 
** other party's publicValue, and the prime and our privateValue.
** maxOutBytes is the requested length of the generated secret in bytes.  
** A zero value means produce a value of any length up to the size of 
** the prime.   If successful, derivedSecret->data is set 
** to the address of the newly allocated buffer containing the derived 
** secret, and derivedSecret->len is the size of the secret produced.
** The size of the secret produced will never be larger than the length
** of the prime, and it may be smaller than maxOutBytes.
** It is the caller's responsibility to free the allocated buffer 
** containing the derived secret.
*/
extern SECStatus DH_Derive(SECItem *    publicValue, 
		           SECItem *    prime, 
			   SECItem *    privateValue, 
			   SECItem *    derivedSecret,
			   unsigned int maxOutBytes);

/* 
** KEA_CalcKey returns octet string with the private key for a dual
** Diffie-Helman  key generation as specified for government key exchange.
*/
extern SECStatus KEA_Derive(SECItem *prime, 
                            SECItem *public1, 
                            SECItem *public2, 
			    SECItem *private1, 
			    SECItem *private2,
			    SECItem *derivedSecret);

/*
 * verify that a KEA or DSA public key is a valid key for this prime and
 * subprime domain.
 */
extern PRBool KEA_Verify(SECItem *Y, SECItem *prime, SECItem *subPrime);


/******************************************/
/*
** RC4 symmetric stream cypher
*/

/*
** Create a new RC4 context suitable for RC4 encryption/decryption.
**	"key" raw key data
**	"len" the number of bytes of key data
*/
extern RC4Context *RC4_CreateContext(unsigned char *key, int len);

/*
** Destroy an RC4 encryption/decryption context.
**	"cx" the context
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void RC4_DestroyContext(RC4Context *cx, PRBool freeit);

/*
** Perform RC4 encryption.
**	"cx" the context
**	"output" the output buffer to store the encrypted data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
**	"input" the input data
**	"inputLen" the amount of input data
*/
extern SECStatus RC4_Encrypt(RC4Context *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);

/*
** Perform RC4 decryption.
**	"cx" the context
**	"output" the output buffer to store the decrypted data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
**	"input" the input data
**	"inputLen" the amount of input data
*/
extern SECStatus RC4_Decrypt(RC4Context *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);

/******************************************/
/*
** RC2 symmetric block cypher
*/

/*
** Create a new RC2 context suitable for RC2 encryption/decryption.
** 	"key" raw key data
** 	"len" the number of bytes of key data
** 	"iv" is the CBC initialization vector (if mode is NSS_RC2_CBC)
** 	"mode" one of NSS_RC2 or NSS_RC2_CBC
**	"effectiveKeyLen" is the effective key length (as specified in 
**	    RFC 2268) in bytes (not bits).
**
** When mode is set to NSS_RC2_CBC the RC2 cipher is run in "cipher block
** chaining" mode.
*/
extern RC2Context *RC2_CreateContext(unsigned char *key, unsigned int len,
		     unsigned char *iv, int mode, unsigned effectiveKeyLen);

/*
** Destroy an RC2 encryption/decryption context.
**	"cx" the context
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void RC2_DestroyContext(RC2Context *cx, PRBool freeit);

/*
** Perform RC2 encryption.
**	"cx" the context
**	"output" the output buffer to store the encrypted data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
**	"input" the input data
**	"inputLen" the amount of input data
*/
extern SECStatus RC2_Encrypt(RC2Context *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);

/*
** Perform RC2 decryption.
**	"cx" the context
**	"output" the output buffer to store the decrypted data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
**	"input" the input data
**	"inputLen" the amount of input data
*/
extern SECStatus RC2_Decrypt(RC2Context *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);

/******************************************/
/*
** RC5 symmetric block cypher -- 64-bit block size
*/

/*
** Create a new RC5 context suitable for RC5 encryption/decryption.
**      "key" raw key data
**      "len" the number of bytes of key data
**      "iv" is the CBC initialization vector (if mode is NSS_RC5_CBC)
**      "mode" one of NSS_RC5 or NSS_RC5_CBC
**
** When mode is set to NSS_RC5_CBC the RC5 cipher is run in "cipher block
** chaining" mode.
*/
extern RC5Context *RC5_CreateContext(SECItem *key, unsigned int rounds,
                     unsigned int wordSize, unsigned char *iv, int mode);

/*
** Destroy an RC5 encryption/decryption context.
**      "cx" the context
**      "freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void RC5_DestroyContext(RC5Context *cx, PRBool freeit);

/*
** Perform RC5 encryption.
**      "cx" the context
**      "output" the output buffer to store the encrypted data.
**      "outputLen" how much data is stored in "output". Set by the routine
**         after some data is stored in output.
**      "maxOutputLen" the maximum amount of data that can ever be
**         stored in "output"
**      "input" the input data
**      "inputLen" the amount of input data
*/
extern SECStatus RC5_Encrypt(RC5Context *cx, unsigned char *output,
                            unsigned int *outputLen, unsigned int maxOutputLen,
                            const unsigned char *input, unsigned int inputLen);

/*
** Perform RC5 decryption.
**      "cx" the context
**      "output" the output buffer to store the decrypted data.
**      "outputLen" how much data is stored in "output". Set by the routine
**         after some data is stored in output.
**      "maxOutputLen" the maximum amount of data that can ever be
**         stored in "output"
**      "input" the input data
**      "inputLen" the amount of input data
*/

extern SECStatus RC5_Decrypt(RC5Context *cx, unsigned char *output,
                            unsigned int *outputLen, unsigned int maxOutputLen,
                            const unsigned char *input, unsigned int inputLen);



/******************************************/
/*
** DES symmetric block cypher
*/

/*
** Create a new DES context suitable for DES encryption/decryption.
** 	"key" raw key data
** 	"len" the number of bytes of key data
** 	"iv" is the CBC initialization vector (if mode is NSS_DES_CBC or
** 	   mode is DES_EDE3_CBC)
** 	"mode" one of NSS_DES, NSS_DES_CBC, NSS_DES_EDE3 or NSS_DES_EDE3_CBC
**	"encrypt" is PR_TRUE if the context will be used for encryption
**
** When mode is set to NSS_DES_CBC or NSS_DES_EDE3_CBC then the DES
** cipher is run in "cipher block chaining" mode.
*/
extern DESContext *DES_CreateContext(unsigned char *key, unsigned char *iv,
				     int mode, PRBool encrypt);

/*
** Destroy an DES encryption/decryption context.
**	"cx" the context
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void DES_DestroyContext(DESContext *cx, PRBool freeit);

/*
** Perform DES encryption.
**	"cx" the context
**	"output" the output buffer to store the encrypted data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
**	"input" the input data
**	"inputLen" the amount of input data
**
** NOTE: the inputLen must be a multiple of DES_KEY_LENGTH
*/
extern SECStatus DES_Encrypt(DESContext *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);

/*
** Perform DES decryption.
**	"cx" the context
**	"output" the output buffer to store the decrypted data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
**	"input" the input data
**	"inputLen" the amount of input data
**
** NOTE: the inputLen must be a multiple of DES_KEY_LENGTH
*/
extern SECStatus DES_Decrypt(DESContext *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);

/******************************************/
/*
** AES symmetric block cypher (Rijndael)
*/

/*
** Create a new AES context suitable for AES encryption/decryption.
** 	"key" raw key data
** 	"keylen" the number of bytes of key data (16, 24, or 32)
**      "blocklen" is the blocksize to use (16, 24, or 32)
**                        XXX currently only blocksize==16 has been tested!
*/
extern AESContext *
AES_CreateContext(unsigned char *key, unsigned char *iv, int mode, int encrypt,
                  unsigned int keylen, unsigned int blocklen);

/*
** Destroy a AES encryption/decryption context.
**	"cx" the context
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void 
AES_DestroyContext(AESContext *cx, PRBool freeit);

/*
** Perform AES encryption.
**	"cx" the context
**	"output" the output buffer to store the encrypted data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
**	"input" the input data
**	"inputLen" the amount of input data
*/
extern SECStatus 
AES_Encrypt(AESContext *cx, unsigned char *output,
            unsigned int *outputLen, unsigned int maxOutputLen,
            const unsigned char *input, unsigned int inputLen);

/*
** Perform AES decryption.
**	"cx" the context
**	"output" the output buffer to store the decrypted data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
**	"input" the input data
**	"inputLen" the amount of input data
*/
extern SECStatus 
AES_Decrypt(AESContext *cx, unsigned char *output,
            unsigned int *outputLen, unsigned int maxOutputLen,
            const unsigned char *input, unsigned int inputLen);


/******************************************/
/*
** MD5 secure hash function
*/

/*
** Hash a null terminated string "src" into "dest" using MD5
*/
extern SECStatus MD5_Hash(unsigned char *dest, const char *src);

/*
** Hash a non-null terminated string "src" into "dest" using MD5
*/
extern SECStatus MD5_HashBuf(unsigned char *dest, const unsigned char *src,
			     uint32 src_length);

/*
** Create a new MD5 context
*/
extern MD5Context *MD5_NewContext(void);


/*
** Destroy an MD5 secure hash context.
**	"cx" the context
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void MD5_DestroyContext(MD5Context *cx, PRBool freeit);

/*
** Reset an MD5 context, preparing it for a fresh round of hashing
*/
extern void MD5_Begin(MD5Context *cx);

/*
** Update the MD5 hash function with more data.
**	"cx" the context
**	"input" the data to hash
**	"inputLen" the amount of data to hash
*/
extern void MD5_Update(MD5Context *cx,
		       const unsigned char *input, unsigned int inputLen);

/*
** Finish the MD5 hash function. Produce the digested results in "digest"
**	"cx" the context
**	"digest" where the 16 bytes of digest data are stored
**	"digestLen" where the digest length (16) is stored
**	"maxDigestLen" the maximum amount of data that can ever be
**	   stored in "digest"
*/
extern void MD5_End(MD5Context *cx, unsigned char *digest,
		    unsigned int *digestLen, unsigned int maxDigestLen);
/*
 * Return the the size of a buffer needed to flatten the MD5 Context into
 *    "cx" the context
 *  returns size;
 */
extern unsigned int MD5_FlattenSize(MD5Context *cx);

/*
 * Flatten the MD5 Context into a buffer:
 *    "cx" the context
 *    "space" the buffer to flatten to
 *  returns status;
 */
extern SECStatus MD5_Flatten(MD5Context *cx,unsigned char *space);

/*
 * Resurrect a flattened context into a MD5 Context
 *    "space" the buffer of the flattend buffer
 *    "arg" ptr to void used by cryptographic resurrect
 *  returns resurected context;
 */
extern MD5Context * MD5_Resurrect(unsigned char *space, void *arg);

/*
** trace the intermediate state info of the MD5 hash.
*/
extern void MD5_TraceState(MD5Context *cx);


/******************************************/
/*
** MD2 secure hash function
*/

/*
** Hash a null terminated string "src" into "dest" using MD2
*/
extern SECStatus MD2_Hash(unsigned char *dest, const char *src);

/*
** Create a new MD2 context
*/
extern MD2Context *MD2_NewContext(void);


/*
** Destroy an MD2 secure hash context.
**	"cx" the context
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void MD2_DestroyContext(MD2Context *cx, PRBool freeit);

/*
** Reset an MD2 context, preparing it for a fresh round of hashing
*/
extern void MD2_Begin(MD2Context *cx);

/*
** Update the MD2 hash function with more data.
**	"cx" the context
**	"input" the data to hash
**	"inputLen" the amount of data to hash
*/
extern void MD2_Update(MD2Context *cx,
		       const unsigned char *input, unsigned int inputLen);

/*
** Finish the MD2 hash function. Produce the digested results in "digest"
**	"cx" the context
**	"digest" where the 16 bytes of digest data are stored
**	"digestLen" where the digest length (16) is stored
**	"maxDigestLen" the maximum amount of data that can ever be
**	   stored in "digest"
*/
extern void MD2_End(MD2Context *cx, unsigned char *digest,
		    unsigned int *digestLen, unsigned int maxDigestLen);

/*
 * Return the the size of a buffer needed to flatten the MD2 Context into
 *    "cx" the context
 *  returns size;
 */
extern unsigned int MD2_FlattenSize(MD2Context *cx);

/*
 * Flatten the MD2 Context into a buffer:
 *    "cx" the context
 *    "space" the buffer to flatten to
 *  returns status;
 */
extern SECStatus MD2_Flatten(MD2Context *cx,unsigned char *space);

/*
 * Resurrect a flattened context into a MD2 Context
 *    "space" the buffer of the flattend buffer
 *    "arg" ptr to void used by cryptographic resurrect
 *  returns resurected context;
 */
extern MD2Context * MD2_Resurrect(unsigned char *space, void *arg);

/******************************************/
/*
** SHA-1 secure hash function
*/

/*
** Hash a null terminated string "src" into "dest" using SHA-1
*/
extern SECStatus SHA1_Hash(unsigned char *dest, const char *src);

/*
** Hash a non-null terminated string "src" into "dest" using SHA-1
*/
extern SECStatus SHA1_HashBuf(unsigned char *dest, const unsigned char *src,
			      uint32 src_length);

/*
** Create a new SHA-1 context
*/
extern SHA1Context *SHA1_NewContext(void);


/*
** Destroy a SHA-1 secure hash context.
**	"cx" the context
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void SHA1_DestroyContext(SHA1Context *cx, PRBool freeit);

/*
** Reset a SHA-1 context, preparing it for a fresh round of hashing
*/
extern void SHA1_Begin(SHA1Context *cx);

/*
** Update the SHA-1 hash function with more data.
**	"cx" the context
**	"input" the data to hash
**	"inputLen" the amount of data to hash
*/
extern void SHA1_Update(SHA1Context *cx, const unsigned char *input,
			unsigned int inputLen);

/*
** Finish the SHA-1 hash function. Produce the digested results in "digest"
**	"cx" the context
**	"digest" where the 16 bytes of digest data are stored
**	"digestLen" where the digest length (20) is stored
**	"maxDigestLen" the maximum amount of data that can ever be
**	   stored in "digest"
*/
extern void SHA1_End(SHA1Context *cx, unsigned char *digest,
		     unsigned int *digestLen, unsigned int maxDigestLen);

/*
** trace the intermediate state info of the SHA1 hash.
*/
extern void SHA1_TraceState(SHA1Context *cx);

/*
 * Return the the size of a buffer needed to flatten the SHA-1 Context into
 *    "cx" the context
 *  returns size;
 */
extern unsigned int SHA1_FlattenSize(SHA1Context *cx);

/*
 * Flatten the SHA-1 Context into a buffer:
 *    "cx" the context
 *    "space" the buffer to flatten to
 *  returns status;
 */
extern SECStatus SHA1_Flatten(SHA1Context *cx,unsigned char *space);

/*
 * Resurrect a flattened context into a SHA-1 Context
 *    "space" the buffer of the flattend buffer
 *    "arg" ptr to void used by cryptographic resurrect
 *  returns resurected context;
 */
extern SHA1Context * SHA1_Resurrect(unsigned char *space, void *arg);


/******************************************/
/*
** Pseudo Random Number Generation.  FIPS compliance desirable.
*/

/*
** Initialize the global RNG context and give it some seed input taken
** from the system.  This function is thread-safe and will only allow
** the global context to be initialized once.  The seed input is likely
** small, so it is imperative that RNG_RandomUpdate() be called with
** additional seed data before the generator is used.  A good way to
** provide the generator with additional entropy is to call
** RNG_SystemInfoForRNG().  Note that NSS_Init() does exactly that.
*/
extern SECStatus RNG_RNGInit(void);

/*
** Update the global random number generator with more seeding
** material
*/
extern SECStatus RNG_RandomUpdate(const void *data, size_t bytes);

/*
** Generate some random bytes, using the global random number generator
** object.
*/
extern SECStatus RNG_GenerateGlobalRandomBytes(void *dest, size_t len);

/* Destroy the global RNG context.  After a call to RNG_RNGShutdown()
** a call to RNG_RNGInit() is required in order to use the generator again,
** along with seed data (see the comment above RNG_RNGInit()).
*/
extern void  RNG_RNGShutdown(void);


/* Generate PQGParams and PQGVerify structs.
 * Length of seed and length of h both equal length of P. 
 * All lengths are specified by "j", according to the table above.
 */
extern SECStatus
PQG_ParamGen(unsigned int j, 	   /* input : determines length of P. */
             PQGParams **pParams,  /* output: P Q and G returned here */
	     PQGVerify **pVfy);    /* output: counter and seed. */

/* Generate PQGParams and PQGVerify structs.
 * Length of P specified by j.  Length of h will match length of P.
 * Length of SEED in bytes specified in seedBytes.
 * seedBbytes must be in the range [20..255] or an error will result.
 */
extern SECStatus
PQG_ParamGenSeedLen(
             unsigned int j, 	     /* input : determines length of P. */
	     unsigned int seedBytes, /* input : length of seed in bytes.*/
             PQGParams **pParams,    /* output: P Q and G returned here */
	     PQGVerify **pVfy);      /* output: counter and seed. */


/*  Test PQGParams for validity as DSS PQG values.
 *  If vfy is non-NULL, test PQGParams to make sure they were generated
 *       using the specified seed, counter, and h values.
 *
 *  Return value indicates whether Verification operation ran succesfully
 *  to completion, but does not indicate if PQGParams are valid or not.
 *  If return value is SECSuccess, then *pResult has these meanings:
 *       SECSuccess: PQGParams are valid.
 *       SECFailure: PQGParams are invalid.
 *
 * Verify the following 12 facts about PQG counter SEED g and h
 * 1.  Q is 160 bits long.
 * 2.  P is one of the 9 valid lengths.
 * 3.  G < P
 * 4.  P % Q == 1
 * 5.  Q is prime
 * 6.  P is prime
 * Steps 7-12 are done only if the optional PQGVerify is supplied.
 * 7.  counter < 4096
 * 8.  g >= 160 and g < 2048   (g is length of seed in bits)
 * 9.  Q generated from SEED matches Q in PQGParams.
 * 10. P generated from (L, counter, g, SEED, Q) matches P in PQGParams.
 * 11. 1 < h < P-1
 * 12. G generated from h matches G in PQGParams.
 */

extern SECStatus   PQG_VerifyParams(const PQGParams *params, 
                                    const PQGVerify *vfy, SECStatus *result);


/*
 * clean-up any global tables freebl may have allocated after it starts up.
 * This function is not thread safe and should be called only after the
 * library has been quiessed.
 */
extern void BL_Cleanup(void);

/**************************************************************************
 *  Free the PQGParams struct and the things it points to.                *
 **************************************************************************/
extern void PQG_DestroyParams(PQGParams *params);

/**************************************************************************
 *  Free the PQGVerify struct and the things it points to.                *
 **************************************************************************/
extern void PQG_DestroyVerify(PQGVerify *vfy);

SEC_END_PROTOS

#endif /* _BLAPI_H_ */
