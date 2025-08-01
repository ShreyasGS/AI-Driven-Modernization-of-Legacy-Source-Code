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
 * Communications Corporation.	Portions created by Netscape are 
 * Copyright (C) 1994-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.	If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#include "prerr.h"
#include "secerr.h"

#include "prtypes.h"
#include "blapi.h"

/* Architecture-dependent defines */

#if defined(SOLARIS) || defined(HPUX) || defined(i386) || defined(IRIX)
/* Convert the byte-stream to a word-stream */
#define CONVERT_TO_WORDS
#endif

#if defined(AIX) || defined(OSF1)
/* Treat array variables as longs, not bytes */
#define USE_LONG
#endif

#if defined(NSS_USE_HYBRID) && !defined(SOLARIS) && !defined(NSS_USE_64) 
typedef unsigned long long WORD;
#else
typedef unsigned long WORD;
#endif
#define WORDSIZE sizeof(WORD)

#ifdef USE_LONG
typedef unsigned long Stype;
#else
typedef PRUint8 Stype;
#endif

#define ARCFOUR_STATE_SIZE 256

#define MASK1BYTE (WORD)(0xff)

#define SWAP(a, b) \
	tmp = a; \
	a = b; \
	b = tmp;

/*
 * State information for stream cipher.
 */
struct RC4ContextStr
{
	Stype S[ARCFOUR_STATE_SIZE];
	PRUint8 i;
	PRUint8 j;
};

/*
 * array indices [0..255] to initialize cx->S array (faster than loop).
 */
static const Stype Kinit[256] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
	0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
	0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
	0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
	0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
	0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
	0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

/*
 * Initialize a new generator.
 */
RC4Context *
RC4_CreateContext(unsigned char *key, int len)
{
	int i;
	PRUint8 j, tmp;
	RC4Context *cx;
	PRUint8 K[256];
	PRUint8 *L;
	/* verify the key length. */
	PORT_Assert(len > 0 && len < ARCFOUR_STATE_SIZE);
	if (len < 0 || len >= ARCFOUR_STATE_SIZE) {
		PORT_SetError(SEC_ERROR_INVALID_ARGS);
		return NULL;
	}
	/* Create space for the context. */
	cx = (RC4Context *)PORT_ZAlloc(sizeof(RC4Context));
	if (cx == NULL) {
		PORT_SetError(PR_OUT_OF_MEMORY_ERROR);
		return NULL;
	}
	/* Initialize the state using array indices. */
	memcpy(cx->S, Kinit, sizeof cx->S);
	/* Fill in K repeatedly with values from key. */
	L = K;
	for (i = sizeof K; i > len; i-= len) {
		memcpy(L, key, len);
		L += len;
	}
	memcpy(L, key, i);
	/* Stir the state of the generator.  At this point it is assumed
	 * that the key is the size of the state buffer.  If this is not
	 * the case, the key bytes are repeated to fill the buffer.
	 */
	j = 0;
#define ARCFOUR_STATE_STIR(ii) \
	j = j + cx->S[ii] + K[ii]; \
	SWAP(cx->S[ii], cx->S[j]);
	for (i=0; i<ARCFOUR_STATE_SIZE; i++) {
		ARCFOUR_STATE_STIR(i);
	}
	cx->i = 0;
	cx->j = 0;
	return cx;
}

void 
RC4_DestroyContext(RC4Context *cx, PRBool freeit)
{
	if (freeit)
		PORT_ZFree(cx, sizeof(*cx));
}

/*
 * Generate the next byte in the stream.
 */
#define ARCFOUR_NEXT_BYTE() \
	tmpSi = cx->S[++tmpi]; \
	tmpj += tmpSi; \
	tmpSj = cx->S[tmpj]; \
	cx->S[tmpi] = tmpSj; \
	cx->S[tmpj] = tmpSi; \
	t = tmpSi + tmpSj;

#ifdef CONVERT_TO_WORDS
/*
 * Straight RC4 op.  No optimization.
 */
static SECStatus 
rc4_no_opt(RC4Context *cx, unsigned char *output,
           unsigned int *outputLen, unsigned int maxOutputLen,
           const unsigned char *input, unsigned int inputLen)
{
    PRUint8 t;
	Stype tmpSi, tmpSj;
	register PRUint8 tmpi = cx->i;
	register PRUint8 tmpj = cx->j;
	unsigned int index;
	PORT_Assert(maxOutputLen >= inputLen);
	if (maxOutputLen < inputLen) {
		PORT_SetError(SEC_ERROR_INVALID_ARGS);
		return SECFailure;
	}
	for (index=0; index < inputLen; index++) {
		/* Generate next byte from stream. */
		ARCFOUR_NEXT_BYTE();
		/* output = next stream byte XOR next input byte */
		output[index] = cx->S[t] ^ input[index];
	}
	*outputLen = inputLen;
	cx->i = tmpi;
	cx->j = tmpj;
	return SECSuccess;
}
#endif

#ifndef CONVERT_TO_WORDS
/*
 * Byte-at-a-time RC4, unrolling the loop into 8 pieces.
 */
static SECStatus 
rc4_unrolled(RC4Context *cx, unsigned char *output,
             unsigned int *outputLen, unsigned int maxOutputLen,
             const unsigned char *input, unsigned int inputLen)
{
	PRUint8 t;
	Stype tmpSi, tmpSj;
	register PRUint8 tmpi = cx->i;
	register PRUint8 tmpj = cx->j;
	int index;
	PORT_Assert(maxOutputLen >= inputLen);
	if (maxOutputLen < inputLen) {
		PORT_SetError(SEC_ERROR_INVALID_ARGS);
		return SECFailure;
	}
	for (index = inputLen / 8; index-- > 0; input += 8, output += 8) {
		ARCFOUR_NEXT_BYTE();
		output[0] = cx->S[t] ^ input[0];
		ARCFOUR_NEXT_BYTE();
		output[1] = cx->S[t] ^ input[1];
		ARCFOUR_NEXT_BYTE();
		output[2] = cx->S[t] ^ input[2];
		ARCFOUR_NEXT_BYTE();
		output[3] = cx->S[t] ^ input[3];
		ARCFOUR_NEXT_BYTE();
		output[4] = cx->S[t] ^ input[4];
		ARCFOUR_NEXT_BYTE();
		output[5] = cx->S[t] ^ input[5];
		ARCFOUR_NEXT_BYTE();
		output[6] = cx->S[t] ^ input[6];
		ARCFOUR_NEXT_BYTE();
		output[7] = cx->S[t] ^ input[7];
	}
	index = inputLen % 8;
	if (index) {
		input += index;
		output += index;
		switch (index) {
		case 7:
			ARCFOUR_NEXT_BYTE();
			output[-7] = cx->S[t] ^ input[-7]; /* FALLTHRU */
		case 6:
			ARCFOUR_NEXT_BYTE();
			output[-6] = cx->S[t] ^ input[-6]; /* FALLTHRU */
		case 5:
			ARCFOUR_NEXT_BYTE();
			output[-5] = cx->S[t] ^ input[-5]; /* FALLTHRU */
		case 4:
			ARCFOUR_NEXT_BYTE();
			output[-4] = cx->S[t] ^ input[-4]; /* FALLTHRU */
		case 3:
			ARCFOUR_NEXT_BYTE();
			output[-3] = cx->S[t] ^ input[-3]; /* FALLTHRU */
		case 2:
			ARCFOUR_NEXT_BYTE();
			output[-2] = cx->S[t] ^ input[-2]; /* FALLTHRU */
		case 1:
			ARCFOUR_NEXT_BYTE();
			output[-1] = cx->S[t] ^ input[-1]; /* FALLTHRU */
		default:
			/* FALLTHRU */
			; /* hp-ux build breaks without this */
		}
	}
	cx->i = tmpi;
	cx->j = tmpj;
	*outputLen = inputLen;
	return SECSuccess;
}
#endif

#ifdef IS_LITTLE_ENDIAN
#define ARCFOUR_NEXT4BYTES_L(n) \
	ARCFOUR_NEXT_BYTE(); streamWord |= (WORD)cx->S[t] << (n     ); \
	ARCFOUR_NEXT_BYTE(); streamWord |= (WORD)cx->S[t] << (n +  8); \
	ARCFOUR_NEXT_BYTE(); streamWord |= (WORD)cx->S[t] << (n + 16); \
	ARCFOUR_NEXT_BYTE(); streamWord |= (WORD)cx->S[t] << (n + 24);
#else
#define ARCFOUR_NEXT4BYTES_B(n) \
	ARCFOUR_NEXT_BYTE(); streamWord |= (WORD)cx->S[t] << (n + 24); \
	ARCFOUR_NEXT_BYTE(); streamWord |= (WORD)cx->S[t] << (n + 16); \
	ARCFOUR_NEXT_BYTE(); streamWord |= (WORD)cx->S[t] << (n +  8); \
	ARCFOUR_NEXT_BYTE(); streamWord |= (WORD)cx->S[t] << (n     );
#endif

#if (defined(NSS_USE_HYBRID) && !defined(SOLARIS)) || defined(NSS_USE_64)
/* 64-bit wordsize */
#ifdef IS_LITTLE_ENDIAN
#define ARCFOUR_NEXT_WORD() \
	{ streamWord = 0; ARCFOUR_NEXT4BYTES_L(0); ARCFOUR_NEXT4BYTES_L(32); }
#else
#define ARCFOUR_NEXT_WORD() \
	{ streamWord = 0; ARCFOUR_NEXT4BYTES_B(32); ARCFOUR_NEXT4BYTES_B(0); }
#endif
#else
/* 32-bit wordsize */
#ifdef IS_LITTLE_ENDIAN
#define ARCFOUR_NEXT_WORD() \
	{ streamWord = 0; ARCFOUR_NEXT4BYTES_L(0); }
#else
#define ARCFOUR_NEXT_WORD() \
	{ streamWord = 0; ARCFOUR_NEXT4BYTES_B(0); }
#endif
#endif

#ifdef IS_LITTLE_ENDIAN
#define RSH <<
#define LSH >>
#else
#define RSH >>
#define LSH <<
#endif

#ifdef CONVERT_TO_WORDS
/*
 * Convert input and output buffers to words before performing
 * RC4 operations.
 */
static SECStatus 
rc4_wordconv(RC4Context *cx, unsigned char *output,
             unsigned int *outputLen, unsigned int maxOutputLen,
             const unsigned char *input, unsigned int inputLen)
{
	ptrdiff_t inOffset = (ptrdiff_t)input % WORDSIZE;
	ptrdiff_t outOffset = (ptrdiff_t)output % WORDSIZE;
	register WORD streamWord, mask;
	register WORD *pInWord, *pOutWord;
	register WORD inWord, nextInWord;
	PRUint8 t;
	register Stype tmpSi, tmpSj;
	register PRUint8 tmpi = cx->i;
	register PRUint8 tmpj = cx->j;
	unsigned int byteCount;
	unsigned int bufShift, invBufShift;
	int i;

	PORT_Assert(maxOutputLen >= inputLen);
	if (maxOutputLen < inputLen) {
		PORT_SetError(SEC_ERROR_INVALID_ARGS);
		return SECFailure;
	}
	if (inputLen < 2*WORDSIZE) {
		/* Ignore word conversion, do byte-at-a-time */
		return rc4_no_opt(cx, output, outputLen, maxOutputLen, input, inputLen);
	}
	*outputLen = inputLen;
	pInWord = (WORD *)(input - inOffset);
	if (inOffset < outOffset) {
		bufShift = 8*(outOffset - inOffset);
		invBufShift = 8*WORDSIZE - bufShift;
	} else {
		invBufShift = 8*(inOffset - outOffset);
		bufShift = 8*WORDSIZE - invBufShift;
	}
	/*****************************************************************/
	/* Step 1:                                                       */
	/* If the first output word is partial, consume the bytes in the */
	/* first partial output word by loading one or two words of      */
	/* input and shifting them accordingly.  Otherwise, just load    */
	/* in the first word of input.  At the end of this block, at     */
	/* least one partial word of input should ALWAYS be loaded.      */
	/*****************************************************************/
	if (outOffset) {
		/* Generate input and stream words aligned relative to the
		 * partial output buffer.
		 */
		byteCount = WORDSIZE - outOffset; 
		pOutWord = (WORD *)(output - outOffset);
		mask = streamWord = 0;
#ifdef IS_LITTLE_ENDIAN
		for (i = WORDSIZE - byteCount; i < WORDSIZE; i++) {
#else
		for (i = byteCount - 1; i >= 0; --i) {
#endif
			ARCFOUR_NEXT_BYTE();
			streamWord |= (WORD)(cx->S[t]) << 8*i;
			mask |= MASK1BYTE << 8*i;
		} /* } */
		inWord = *pInWord++;
		/* If buffers are relatively misaligned, shift the bytes in inWord
		 * to be aligned to the output buffer.
		 */
		nextInWord = 0;
		if (inOffset < outOffset) {
			/* Have more bytes than needed, shift remainder into nextInWord */
			nextInWord = inWord LSH 8*(inOffset + byteCount);
			inWord = inWord RSH bufShift;
		} else if (inOffset > outOffset) {
			/* Didn't get enough bytes from current input word, load another
			 * word and then shift remainder into nextInWord.
			 */
			nextInWord = *pInWord++;
			inWord = (inWord LSH invBufShift) | 
			         (nextInWord RSH bufShift);
			nextInWord = nextInWord LSH invBufShift;
		}
		/* Store output of first partial word */
		*pOutWord = (*pOutWord & ~mask) | ((inWord ^ streamWord) & mask);
		/* Consumed byteCount bytes of input */
		inputLen -= byteCount;
		/* move to next word of output */
		pOutWord++;
		/* inWord has been consumed, but there may be bytes in nextInWord */
		inWord = nextInWord;
	} else {
		/* output is word-aligned */
		pOutWord = (WORD *)output;
		if (inOffset) {
			/* Input is not word-aligned.  The first word load of input 
			 * will not produce a full word of input bytes, so one word
			 * must be pre-loaded.  The main loop below will load in the
			 * next input word and shift some of its bytes into inWord
			 * in order to create a full input word.  Note that the main
			 * loop must execute at least once because the input must
			 * be at least two words.
			 */
			inWord = *pInWord++;
			inWord = inWord LSH invBufShift;
		} else {
			/* Input is word-aligned.  The first word load of input 
			 * will produce a full word of input bytes, so nothing
			 * needs to be loaded here.
			 */
			inWord = 0;
		}
	}
	/* Output buffer is aligned, inOffset is now measured relative to
	 * outOffset (and not a word boundary).
	 */
	inOffset = (inOffset + WORDSIZE - outOffset) % WORDSIZE;
	/*****************************************************************/
	/* Step 2: main loop                                             */
	/* At this point the output buffer is word-aligned.  Any unused  */
	/* bytes from above will be in inWord (shifted correctly).  If   */
	/* the input buffer is unaligned relative to the output buffer,  */
	/* shifting has to be done.                                      */
	/*****************************************************************/
	if (inOffset) {
		for (; inputLen >= WORDSIZE; inputLen -= WORDSIZE) {
			nextInWord = *pInWord++;
			inWord |= nextInWord RSH bufShift;
			nextInWord = nextInWord LSH invBufShift;
			ARCFOUR_NEXT_WORD();
			*pOutWord++ = inWord ^ streamWord;
			inWord = nextInWord;
		}
		if (inputLen == 0) {
			/* Nothing left to do. */
			cx->i = tmpi;
			cx->j = tmpj;
			return SECSuccess;
		}
		/* If the amount of remaining input is greater than the amount
		 * bytes pulled from the current input word, need to do another
		 * word load.  What's left in inWord will be consumed in step 3.
		 */
		if (inputLen > WORDSIZE - inOffset)
			inWord |= *pInWord RSH bufShift;
	} else {
		for (; inputLen >= WORDSIZE; inputLen -= WORDSIZE) {
			inWord = *pInWord++;
			ARCFOUR_NEXT_WORD();
			*pOutWord++ = inWord ^ streamWord;
		}
		if (inputLen == 0) {
			/* Nothing left to do. */
			cx->i = tmpi;
			cx->j = tmpj;
			return SECSuccess;
		} else {
			/* A partial input word remains at the tail.  Load it.  The
			 * relevant bytes will be consumed in step 3.
			 */
			inWord = *pInWord;
		}
	}
	/*****************************************************************/
	/* Step 3:                                                       */
	/* A partial word of input remains, and it is already loaded     */
	/* into nextInWord.  Shift appropriately and consume the bytes   */
	/* used in the partial word.                                     */
	/*****************************************************************/
	mask = streamWord = 0;
#ifdef IS_LITTLE_ENDIAN
	for (i = 0; i < inputLen; ++i) {
#else
	for (i = WORDSIZE - 1; i >= WORDSIZE - inputLen; --i) {
#endif
		ARCFOUR_NEXT_BYTE();
		streamWord |= (WORD)(cx->S[t]) << 8*i;
		mask |= MASK1BYTE << 8*i;
	} /* } */
	*pOutWord = (*pOutWord & ~mask) | ((inWord ^ streamWord) & mask);
	cx->i = tmpi;
	cx->j = tmpj;
	return SECSuccess;
}
#endif

SECStatus 
RC4_Encrypt(RC4Context *cx, unsigned char *output,
            unsigned int *outputLen, unsigned int maxOutputLen,
            const unsigned char *input, unsigned int inputLen)
{
	PORT_Assert(maxOutputLen >= inputLen);
	if (maxOutputLen < inputLen) {
		PORT_SetError(SEC_ERROR_INVALID_ARGS);
		return SECFailure;
	}
#ifdef CONVERT_TO_WORDS
	/* Convert the byte-stream to a word-stream */
	return rc4_wordconv(cx, output, outputLen, maxOutputLen, input, inputLen);
#else
	/* Operate on bytes, but unroll the main loop */
	return rc4_unrolled(cx, output, outputLen, maxOutputLen, input, inputLen);
#endif
}

SECStatus RC4_Decrypt(RC4Context *cx, unsigned char *output,
                      unsigned int *outputLen, unsigned int maxOutputLen,
                      const unsigned char *input, unsigned int inputLen)
{
	PORT_Assert(maxOutputLen >= inputLen);
	if (maxOutputLen < inputLen) {
		PORT_SetError(SEC_ERROR_INVALID_ARGS);
		return SECFailure;
	}
	/* decrypt and encrypt are same operation. */
#ifdef CONVERT_TO_WORDS
	/* Convert the byte-stream to a word-stream */
	return rc4_wordconv(cx, output, outputLen, maxOutputLen, input, inputLen);
#else
	/* Operate on bytes, but unroll the main loop */
	return rc4_unrolled(cx, output, outputLen, maxOutputLen, input, inputLen);
#endif
}

#undef CONVERT_TO_WORDS
#undef USE_LONG
