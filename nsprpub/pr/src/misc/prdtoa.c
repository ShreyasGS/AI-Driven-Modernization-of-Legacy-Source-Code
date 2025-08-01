/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is the Netscape Portable Runtime (NSPR).
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998-2000 Netscape Communications Corporation.  All
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

#include "primpl.h"

/****************************************************************
 *
 * The author of this software is David M. Gay.
 *
 * Copyright (c) 1991 by AT&T.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHOR NOR AT&T MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 ***************************************************************/

/* Please send bug reports to
	David M. Gay
	AT&T Bell Laboratories, Room 2C-463
	600 Mountain Avenue
	Murray Hill, NJ 07974-2070
	U.S.A.
	dmg@research.att.com or research!dmg
 */

/* PR_strtod for IEEE-, VAX-, and IBM-arithmetic machines.
 *
 * This PR_strtod returns a nearest machine number to the input decimal
 * string (or sets the error code to PR_RANGE_ERROR).  With IEEE
 * arithmetic, ties are broken by the IEEE round-even rule.  Otherwise
 * ties are broken by biased rounding (add half and chop).
 *
 * Inspired loosely by William D. Clinger's paper "How to Read
 * Floating-Point Numbers Accurately" [Proc. ACM SIGPLAN '90, pp. 92-101].
 *
 * Modifications:
 *
 *	1. We only require IEEE, IBM, or VAX double-precision
 *		arithmetic (not IEEE double-extended).
 *	2. We get by with floating-point arithmetic in a case that
 *		Clinger missed -- when we're computing d * 10^n
 *		for a small integer d and the integer n is not too
 *		much larger than 22 (the maximum integer k for which
 *		we can represent 10^k exactly), we may be able to
 *		compute (d*10^k) * 10^(e-k) with just one roundoff.
 *	3. Rather than a bit-at-a-time adjustment of the binary
 *		result in the hard case, we use floating-point
 *		arithmetic to determine the adjustment to within
 *		one bit; only in really hard cases do we need to
 *		compute a second residual.
 *	4. Because of 3., we don't need a large table of powers of 10
 *		for ten-to-e (just some small tables, e.g. of 10^k
 *		for 0 <= k <= 22).
 */

/*
 * #define IEEE_8087 for IEEE-arithmetic machines where the least
 *	significant byte has the lowest address.
 * #define IEEE_MC68k for IEEE-arithmetic machines where the most
 *	significant byte has the lowest address.
 * #define Long int on machines with 32-bit ints and 64-bit longs.
 * #define Sudden_Underflow for IEEE-format machines without gradual
 *	underflow (i.e., that flush to zero on underflow).
 * #define IBM for IBM mainframe-style floating-point arithmetic.
 * #define VAX for VAX-style floating-point arithmetic.
 * #define Unsigned_Shifts if >> does treats its left operand as unsigned.
 * #define No_leftright to omit left-right logic in fast floating-point
 *	computation of PR_dtoa.
 * #define Check_FLT_ROUNDS if FLT_ROUNDS can assume the values 2 or 3.
 * #define RND_PRODQUOT to use rnd_prod and rnd_quot (assembly routines
 *	that use extended-precision instructions to compute rounded
 *	products and quotients) with IBM.
 * #define ROUND_BIASED for IEEE-format with biased rounding.
 * #define Inaccurate_Divide for IEEE-format with correctly rounded
 *	products but inaccurate quotients, e.g., for Intel i860.
 * #define Just_16 to store 16 bits per 32-bit Long when doing high-precision
 *	integer arithmetic.  Whether this speeds things up or slows things
 *	down depends on the machine and the number being converted.
 * #define KR_headers for old-style C function headers.
 * #define Bad_float_h if your system lacks a float.h or if it does not
 *	define some or all of DBL_DIG, DBL_MAX_10_EXP, DBL_MAX_EXP,
 *	FLT_RADIX, FLT_ROUNDS, and DBL_MAX.
 * #define MALLOC your_malloc, where your_malloc(n) acts like malloc(n)
 *	if memory is available and otherwise does something you deem
 *	appropriate.  If MALLOC is undefined, malloc will be invoked
 *	directly -- and assumed always to succeed.
 */
#if  defined(IS_LITTLE_ENDIAN)
#define IEEE_8087
#else
#define IEEE_MC68k
#endif

#ifndef Long
#if PR_BYTES_PER_LONG == 4
#define Long long
#elif PR_BYTES_PER_INT == 4
#define Long int
#else
#error "No suitable type for Long"
#endif
#endif

#ifdef DEBUG_DTOA
#include "stdio.h"
#define Bug(x) {fprintf(stderr, "%s\n", x); exit(1);}
#else
#define Bug(x)
#endif

#include "stdlib.h"
#include "string.h"

#ifdef MALLOC
extern void *MALLOC(size_t);
#else
#define MALLOC PR_MALLOC
#endif

#ifdef Bad_float_h
#undef __STDC__
#ifdef IEEE_MC68k
#define IEEE_ARITHMETIC
#endif
#ifdef IEEE_8087
#define IEEE_ARITHMETIC
#endif

#ifdef IEEE_ARITHMETIC
#define DBL_DIG 15
#define DBL_MAX_10_EXP 308
#define DBL_MAX_EXP 1024
#define FLT_RADIX 2
#define FLT_ROUNDS 1
#define DBL_MAX 1.7976931348623157e+308
#endif

#ifdef IBM
#define DBL_DIG 16
#define DBL_MAX_10_EXP 75
#define DBL_MAX_EXP 63
#define FLT_RADIX 16
#define FLT_ROUNDS 0
#define DBL_MAX 7.2370055773322621e+75
#endif

#ifdef VAX
#define DBL_DIG 16
#define DBL_MAX_10_EXP 38
#define DBL_MAX_EXP 127
#define FLT_RADIX 2
#define FLT_ROUNDS 1
#define DBL_MAX 1.7014118346046923e+38
#endif

#ifndef LONG_MAX
#define LONG_MAX 2147483647
#endif
#else
#include "float.h"
#endif
#ifndef __MATH_H__
#include "math.h"
#endif

#ifndef CONST
#define CONST const
#endif

#ifdef Unsigned_Shifts
#define Sign_Extend(a,b) if (b < 0) a |= 0xffff0000;
#else
#define Sign_Extend(a,b) /*no-op*/
#endif

#if defined(IEEE_8087) + defined(IEEE_MC68k) + defined(VAX) + defined(IBM)	!= 1
Exactly one of IEEE_8087, IEEE_MC68k, VAX, or IBM should be defined.
#endif

#ifdef IEEE_8087
#define word0(x) ((unsigned Long *)&x)[1]
#define word1(x) ((unsigned Long *)&x)[0]
#else
#define word0(x) ((unsigned Long *)&x)[0]
#define word1(x) ((unsigned Long *)&x)[1]
#endif

/* The following definition of Storeinc is appropriate for MIPS processors.
 * An alternative that might be better on some machines is
 * #define Storeinc(a,b,c) (*a++ = b << 16 | c & 0xffff)
 */
#if defined(IEEE_8087) + defined(VAX)
#define Storeinc(a,b,c) (((unsigned short *)a)[1] = (unsigned short)b, \
((unsigned short *)a)[0] = (unsigned short)c, a++)
#else
#define Storeinc(a,b,c) (((unsigned short *)a)[0] = (unsigned short)b, \
((unsigned short *)a)[1] = (unsigned short)c, a++)
#endif

/* #define P DBL_MANT_DIG */
/* Ten_pmax = floor(P*log(2)/log(5)) */
/* Bletch = (highest power of 2 < DBL_MAX_10_EXP) / 16 */
/* Quick_max = floor((P-1)*log(FLT_RADIX)/log(10) - 1) */
/* Int_max = floor(P*log(FLT_RADIX)/log(10) - 1) */

#if defined(IEEE_8087) + defined(IEEE_MC68k)
#define Exp_shift  20
#define Exp_shift1 20
#define Exp_msk1    0x100000
#define Exp_msk11   0x100000
#define Exp_mask  0x7ff00000
#define P 53
#define Bias 1023
#define IEEE_Arith
#define Emin (-1022)
#define Exp_1  0x3ff00000
#define Exp_11 0x3ff00000
#define Ebits 11
#define Frac_mask  0xfffff
#define Frac_mask1 0xfffff
#define Ten_pmax 22
#define Bletch 0x10
#define Bndry_mask  0xfffff
#define Bndry_mask1 0xfffff
#define LSB 1
#define Sign_bit 0x80000000
#define Log2P 1
#define Tiny0 0
#define Tiny1 1
#define Quick_max 14
#define Int_max 14
#define Infinite(x) (word0(x) == 0x7ff00000) /* sufficient test for here */
#else
#undef  Sudden_Underflow
#define Sudden_Underflow
#ifdef IBM
#define Exp_shift  24
#define Exp_shift1 24
#define Exp_msk1   0x1000000
#define Exp_msk11  0x1000000
#define Exp_mask  0x7f000000
#define P 14
#define Bias 65
#define Exp_1  0x41000000
#define Exp_11 0x41000000
#define Ebits 8	/* exponent has 7 bits, but 8 is the right value in b2d */
#define Frac_mask  0xffffff
#define Frac_mask1 0xffffff
#define Bletch 4
#define Ten_pmax 22
#define Bndry_mask  0xefffff
#define Bndry_mask1 0xffffff
#define LSB 1
#define Sign_bit 0x80000000
#define Log2P 4
#define Tiny0 0x100000
#define Tiny1 0
#define Quick_max 14
#define Int_max 15
#else /* VAX */
#define Exp_shift  23
#define Exp_shift1 7
#define Exp_msk1    0x80
#define Exp_msk11   0x800000
#define Exp_mask  0x7f80
#define P 56
#define Bias 129
#define Exp_1  0x40800000
#define Exp_11 0x4080
#define Ebits 8
#define Frac_mask  0x7fffff
#define Frac_mask1 0xffff007f
#define Ten_pmax 24
#define Bletch 2
#define Bndry_mask  0xffff007f
#define Bndry_mask1 0xffff007f
#define LSB 0x10000
#define Sign_bit 0x8000
#define Log2P 1
#define Tiny0 0x80
#define Tiny1 0
#define Quick_max 15
#define Int_max 15
#endif
#endif

#ifndef IEEE_Arith
#define ROUND_BIASED
#endif

#ifdef RND_PRODQUOT
#define rounded_product(a,b) a = rnd_prod(a, b)
#define rounded_quotient(a,b) a = rnd_quot(a, b)
extern double rnd_prod(double, double), rnd_quot(double, double);
#else
#define rounded_product(a,b) a *= b
#define rounded_quotient(a,b) a /= b
#endif

#define Big0 (Frac_mask1 | Exp_msk1*(DBL_MAX_EXP+Bias-1))
#define Big1 0xffffffff

#ifndef Just_16
/* When Pack_32 is not defined, we store 16 bits per 32-bit Long.
 * This makes some inner loops simpler and sometimes saves work
 * during multiplications, but it often seems to make things slightly
 * slower.  Hence the default is now to store 32 bits per Long.
 */
#ifndef Pack_32
#define Pack_32
#endif
#endif

#define Kmax 15

/*
 * Note: if you ever change struct Bigint, make sure that the
 * definition of the Bcopy(x,y) macro is still correct.
 */
struct Bigint {
	struct Bigint *next;
	PRInt32 k, maxwds, sign, wds;
	unsigned Long x[1];
};

typedef struct Bigint Bigint;

static Bigint *freelist[Kmax+1];

static PRLock *freelist_lock;

static Bigint *Balloc(PRInt32 k)
{
	PRInt32 x;
	Bigint *rv;

	PR_Lock(freelist_lock);
	if ((rv = freelist[k]) != NULL) {
		freelist[k] = rv->next;
	}
	PR_Unlock(freelist_lock);
	if (rv == NULL) {
		x = 1 << k;
		rv = (Bigint *)MALLOC(sizeof(Bigint) + (x-1)*sizeof(Long));
		rv->k = k;
		rv->maxwds = x;
	}
	rv->sign = rv->wds = 0;
	return rv;
}

static void Bfree (Bigint *v)
{
	if (v) {
		PR_Lock(freelist_lock);
		v->next = freelist[v->k];
		freelist[v->k] = v;
		PR_Unlock(freelist_lock);
	}
}

/*
 * The definition of the Bcopy macro is highly dependent on the
 * ordering of members in struct Bigint.
 */
#define Bcopy(x,y) memcpy((char *)&x->sign, (char *)&y->sign, \
						  y->wds*sizeof(Long) + 2*sizeof(PRInt32))

static Bigint *multadd(Bigint *b, PRInt32 m, PRInt32 a)	/* multiply by m and add a */
{
	PRInt32 i, wds;
	unsigned Long *x, y;
#ifdef Pack_32
	unsigned Long xi, z;
#endif
	Bigint *b1;

	wds = b->wds;
	x = b->x;
	i = 0;
	do {
#ifdef Pack_32
		xi = *x;
		y = (xi & 0xffff) * m + a;
		z = (xi >> 16) * m + (y >> 16);
		a = (PRInt32)(z >> 16);
		*x++ = (z << 16) + (y & 0xffff);
#else
		y = *x * m + a;
		a = (PRInt32)(y >> 16);
		*x++ = y & 0xffff;
#endif
	}
	while(++i < wds);
	if (a) {
		if (wds >= b->maxwds) {
			b1 = Balloc(b->k+1);
			Bcopy(b1, b);
			Bfree(b);
			b = b1;
		}
		b->x[wds++] = a;
		b->wds = wds;
	}
	return b;
}

static Bigint *s2b(CONST char *s, PRInt32 nd0, PRInt32 nd, unsigned Long y9)
{
	Bigint *b;
	PRInt32 i, k;
	Long x, y;

	x = (nd + 8) / 9;
	for(k = 0, y = 1; x > y; y <<= 1, k++) ;
#ifdef Pack_32
	b = Balloc(k);
	b->x[0] = y9;
	b->wds = 1;
#else
	b = Balloc(k+1);
	b->x[0] = y9 & 0xffff;
	b->wds = (b->x[1] = y9 >> 16) ? 2 : 1;
#endif

	i = 9;
	if (9 < nd0) {
		s += 9;
		do b = multadd(b, 10, *s++ - '0');
		while(++i < nd0);
		s++;
	}
	else
		s += 10;
	for(; i < nd; i++)
		b = multadd(b, 10, *s++ - '0');
	return b;
}

static PRInt32 hi0bits(register unsigned Long x)
{
	register PRInt32 k = 0;

	if (!(x & 0xffff0000)) {
		k = 16;
		x <<= 16;
	}
	if (!(x & 0xff000000)) {
		k += 8;
		x <<= 8;
	}
	if (!(x & 0xf0000000)) {
		k += 4;
		x <<= 4;
	}
	if (!(x & 0xc0000000)) {
		k += 2;
		x <<= 2;
	}
	if (!(x & 0x80000000)) {
		k++;
		if (!(x & 0x40000000))
			return 32;
	}
	return k;
}

static PRInt32 lo0bits(unsigned Long *y)
{
	register PRInt32 k;
	register unsigned Long x = *y;

	if (x & 7) {
		if (x & 1)
			return 0;
		if (x & 2) {
			*y = x >> 1;
			return 1;
		}
		*y = x >> 2;
		return 2;
	}
	k = 0;
	if (!(x & 0xffff)) {
		k = 16;
		x >>= 16;
	}
	if (!(x & 0xff)) {
		k += 8;
		x >>= 8;
	}
	if (!(x & 0xf)) {
		k += 4;
		x >>= 4;
	}
	if (!(x & 0x3)) {
		k += 2;
		x >>= 2;
	}
	if (!(x & 1)) {
		k++;
		x >>= 1;
		if (!x & 1)
			return 32;
	}
	*y = x;
	return k;
}

static Bigint *i2b(PRInt32 i)
{
	Bigint *b;

	b = Balloc(1);
	b->x[0] = i;
	b->wds = 1;
	return b;
}

static Bigint *mult(CONST Bigint *a, CONST Bigint *b)
{
	CONST Bigint *t;
	Bigint *c;
	PRInt32 k, wa, wb, wc;
	unsigned Long carry, y, z;
	unsigned Long *xc, *xc0, *xce;
	CONST unsigned Long *x, *xa, *xae, *xb, *xbe;
#ifdef Pack_32
	unsigned Long z2;
#endif

	if (a->wds < b->wds) {
		t = a;
		a = b;
		b = t;
	}
	k = a->k;
	wa = a->wds;
	wb = b->wds;
	wc = wa + wb;
	if (wc > a->maxwds)
		k++;
	c = Balloc(k);
	for(xc = c->x, xce = xc + wc; xc < xce; xc++)
		*xc = 0;
	xa = a->x;
	xae = xa + wa;
	xb = b->x;
	xbe = xb + wb;
	xc0 = c->x;
#ifdef Pack_32
	for(; xb < xbe; xb++, xc0++) {
		if ((y = *xb & 0xffff) != 0) {
			x = xa;
			xc = xc0;
			carry = 0;
			do {
				z = (*x & 0xffff) * y + (*xc & 0xffff) + carry;
				carry = z >> 16;
				z2 = (*x++ >> 16) * y + (*xc >> 16) + carry;
				carry = z2 >> 16;
				Storeinc(xc, z2, z);
			}
			while(x < xae);
			*xc = carry;
		}
		if ((y = *xb >> 16) != 0) {
			x = xa;
			xc = xc0;
			carry = 0;
			z2 = *xc;
			do {
				z = (*x & 0xffff) * y + (*xc >> 16) + carry;
				carry = z >> 16;
				Storeinc(xc, z, z2);
				z2 = (*x++ >> 16) * y + (*xc & 0xffff) + carry;
				carry = z2 >> 16;
			}
			while(x < xae);
			*xc = z2;
		}
	}
#else
	for(; xb < xbe; xc0++) {
		if (y = *xb++) {
			x = xa;
			xc = xc0;
			carry = 0;
			do {
				z = *x++ * y + *xc + carry;
				carry = z >> 16;
				*xc++ = z & 0xffff;
			}
			while(x < xae);
			*xc = carry;
		}
	}
#endif
	for(xc0 = c->x, xc = xc0 + wc; wc > 0 && !*--xc; --wc) ;
	c->wds = wc;
	return c;
}

/*
 * 'p5s' points to a linked list of Bigints that are powers of 5.
 * This list grows on demand, and it can only grow: it won't change
 * in any other way.  So if we read 'p5s' or the 'next' field of
 * some Bigint on the list, and it is not NULL, we know it won't
 * change to NULL or some other value.  Only when the value of
 * 'p5s' or 'next' is NULL do we need to acquire the lock and add
 * a new Bigint to the list.
 */

static Bigint *p5s;

static PRLock *p5s_lock;

static Bigint *pow5mult(Bigint *b, PRInt32 k)
{
	Bigint *b1, *p5, *p51;
	PRInt32 i;
	static CONST PRInt32 p05[3] = { 5, 25, 125 };

	if ((i = k & 3) != 0)
		b = multadd(b, p05[i-1], 0);

	if (!(k >>= 2))
		return b;
	if (!(p5 = p5s)) {
		/*
		 * We take great care to not call i2b() and Bfree()
		 * while holding the lock.
		 */
		Bigint *wasted_effort = NULL;
		p5 = i2b(625);
		/* lock and check again */
		PR_Lock(p5s_lock);
		if (!p5s) {
			/* first time */
			p5s = p5;
			p5->next = 0;
		} else {
			/* some other thread just beat us */
			wasted_effort = p5;
			p5 = p5s;
		}
		PR_Unlock(p5s_lock);
		if (wasted_effort) {
			Bfree(wasted_effort);
		}
	}
	for(;;) {
		if (k & 1) {
			b1 = mult(b, p5);
			Bfree(b);
			b = b1;
		}
		if (!(k >>= 1))
			break;
		if (!(p51 = p5->next)) {
			Bigint *wasted_effort = NULL;
			p51 = mult(p5, p5);
			PR_Lock(p5s_lock);
			if (!p5->next) {
				p5->next = p51;
				p51->next = 0;
			} else {
				wasted_effort = p51;
				p51 = p5->next;
			}
			PR_Unlock(p5s_lock);
			if (wasted_effort) {
				Bfree(wasted_effort);
			}
		}
		p5 = p51;
	}
	return b;
}

static Bigint *lshift(Bigint *b, PRInt32 k)
{
	PRInt32 i, k1, n, n1;
	Bigint *b1;
	unsigned Long *x, *x1, *xe, z;

#ifdef Pack_32
	n = k >> 5;
#else
	n = k >> 4;
#endif
	k1 = b->k;
	n1 = n + b->wds + 1;
	for(i = b->maxwds; n1 > i; i <<= 1)
		k1++;
	b1 = Balloc(k1);
	x1 = b1->x;
	for(i = 0; i < n; i++)
		*x1++ = 0;
	x = b->x;
	xe = x + b->wds;
#ifdef Pack_32
	if (k &= 0x1f) {
		k1 = 32 - k;
		z = 0;
		do {
			*x1++ = *x << k | z;
			z = *x++ >> k1;
		}
		while(x < xe);
		if ((*x1 = z) != 0)
			++n1;
	}
#else
	if (k &= 0xf) {
		k1 = 16 - k;
		z = 0;
		do {
			*x1++ = *x << k  & 0xffff | z;
			z = *x++ >> k1;
		}
		while(x < xe);
		if ((*x1 = z) != 0)
			++n1;
	}
#endif
	else do
		*x1++ = *x++;
		 while(x < xe);
	b1->wds = n1 - 1;
	Bfree(b);
	return b1;
}

static PRInt32 cmp(Bigint *a, Bigint *b)
{
	unsigned Long *xa, *xa0, *xb, *xb0;
	PRInt32 i, j;

	i = a->wds;
	j = b->wds;
#ifdef DEBUG_DTOA
	if ((i > 1 && !a->x[i-1]))
		Bug("cmp called with a->x[a->wds-1] == 0");
	if ((j > 1 && !b->x[j-1]))
		Bug("cmp called with b->x[b->wds-1] == 0");
#endif
	if (i -= j)
		return i;
	xa0 = a->x;
	xa = xa0 + j;
	xb0 = b->x;
	xb = xb0 + j;
	for(;;) {
		if (*--xa != *--xb)
			return *xa < *xb ? -1 : 1;
		if (xa <= xa0)
			break;
	}
	return 0;
}

static Bigint *diff(Bigint *a, Bigint *b)
{
	Bigint *c;
	PRInt32 i, wa, wb;
	Long borrow, y;	/* We need signed shifts here. */
	unsigned Long *xa, *xae, *xb, *xbe, *xc;
#ifdef Pack_32
	Long z;
#endif

	i = cmp(a,b);
	if (!i) {
		c = Balloc(0);
		c->wds = 1;
		c->x[0] = 0;
		return c;
	}
	if (i < 0) {
		c = a;
		a = b;
		b = c;
		i = 1;
	}
	else
		i = 0;
	c = Balloc(a->k);
	c->sign = i;
	wa = a->wds;
	xa = a->x;
	xae = xa + wa;
	wb = b->wds;
	xb = b->x;
	xbe = xb + wb;
	xc = c->x;
	borrow = 0;
#ifdef Pack_32
	do {
		y = (long)((*xa & 0xffff) - (*xb & 0xffff) + borrow);
		borrow = y >> 16;
		Sign_Extend(borrow, y);
		z = (long)((*xa++ >> 16) - (*xb++ >> 16) + borrow);
		borrow = z >> 16;
		Sign_Extend(borrow, z);
		Storeinc(xc, z, y);
	}
	while(xb < xbe);
	while(xa < xae) {
		y = (long)((*xa & 0xffff) + borrow);
		borrow = y >> 16;
		Sign_Extend(borrow, y);
		z = (long)((*xa++ >> 16) + borrow);
		borrow = z >> 16;
		Sign_Extend(borrow, z);
		Storeinc(xc, z, y);
	}
#else
	do {
		y = *xa++ - *xb++ + borrow;
		borrow = y >> 16;
		Sign_Extend(borrow, y);
		*xc++ = y & 0xffff;
	}
	while(xb < xbe);
	while(xa < xae) {
		y = *xa++ + borrow;
		borrow = y >> 16;
		Sign_Extend(borrow, y);
		*xc++ = y & 0xffff;
	}
#endif
	while(!*--xc)
		wa--;
	c->wds = wa;
	return c;
}

static double ulp(double x)
{
	register Long L;
	double a;

	L = (long)((word0(x) & Exp_mask) - (P-1)*Exp_msk1);
#ifndef Sudden_Underflow
	if (L > 0) {
#endif
#ifdef IBM
		L |= Exp_msk1 >> 4;
#endif
		word0(a) = L;
		word1(a) = 0;
#ifndef Sudden_Underflow
	}
	else {
		L = -L >> Exp_shift;
		if (L < Exp_shift) {
			word0(a) = 0x80000 >> L;
			word1(a) = 0;
		}
		else {
			word0(a) = 0;
			L -= Exp_shift;
			word1(a) = L >= 31 ? 1 : 1 << (31 - L);
		}
	}
#endif
	return a;
}

static double
b2d
#ifdef KR_headers
(a, e) Bigint *a; PRInt32 *e;
#else
(Bigint *a, PRInt32 *e)
#endif
{
	unsigned Long *xa, *xa0, w, y, z;
	PRInt32 k;
	double d;
#ifdef VAX
	unsigned Long d0, d1;
#else
#define d0 word0(d)
#define d1 word1(d)
#endif

	xa0 = a->x;
	xa = xa0 + a->wds;
	y = *--xa;
#ifdef DEBUG_DTOA
	if (!y) Bug("zero y in b2d");
#endif
	k = hi0bits(y);
	*e = 32 - k;
#ifdef Pack_32
	if (k < Ebits) {
		d0 = Exp_1 | y >> (Ebits - k);
		w = xa > xa0 ? *--xa : 0;
		d1 = y << (32 - Ebits + k) | w >> (Ebits - k);
		goto ret_d;
	}
	z = xa > xa0 ? *--xa : 0;
	if (k -= Ebits) {
		d0 = Exp_1 | y << k | z >> (32 - k);
		y = xa > xa0 ? *--xa : 0;
		d1 = z << k | y >> (32 - k);
	}
	else {
		d0 = Exp_1 | y;
		d1 = z;
	}
#else
	if (k < Ebits + 16) {
		z = xa > xa0 ? *--xa : 0;
		d0 = Exp_1 | y << k - Ebits | z >> Ebits + 16 - k;
		w = xa > xa0 ? *--xa : 0;
		y = xa > xa0 ? *--xa : 0;
		d1 = z << k + 16 - Ebits | w << k - Ebits | y >> 16 + Ebits - k;
		goto ret_d;
	}
	z = xa > xa0 ? *--xa : 0;
	w = xa > xa0 ? *--xa : 0;
	k -= Ebits + 16;
	d0 = Exp_1 | y << k + 16 | z << k | w >> 16 - k;
	y = xa > xa0 ? *--xa : 0;
	d1 = w << k + 16 | y << k;
#endif
ret_d:
#ifdef VAX
	word0(d) = d0 >> 16 | d0 << 16;
	word1(d) = d1 >> 16 | d1 << 16;
#else
#undef d0
#undef d1
#endif
	return d;
}

static Bigint *
d2b
#ifdef KR_headers
(d, e, bits) double d; PRInt32 *e, *bits;
#else
(double d, PRInt32 *e, PRInt32 *bits)
#endif
{
	Bigint *b;
	PRInt32 de, i, k;
	unsigned Long *x, y, z;
#ifdef VAX
	unsigned Long d0, d1;
	d0 = word0(d) >> 16 | word0(d) << 16;
	d1 = word1(d) >> 16 | word1(d) << 16;
#else
#define d0 word0(d)
#define d1 word1(d)
#endif

#ifdef Pack_32
	b = Balloc(1);
#else
	b = Balloc(2);
#endif
	x = b->x;

	z = d0 & Frac_mask;
	d0 &= 0x7fffffff;	/* clear sign bit, which we ignore */
#ifdef Sudden_Underflow
	de = (PRInt32)(d0 >> Exp_shift);
#ifndef IBM
	z |= Exp_msk11;
#endif
#else
	if ((de = (PRInt32)(d0 >> Exp_shift)) != 0)
		z |= Exp_msk1;
#endif
#ifdef Pack_32
	if ((y = d1) != 0) {
		if ((k = lo0bits(&y)) != 0) {
			x[0] = y | z << (32 - k);
			z >>= k;
		}
		else
			x[0] = y;
		i = b->wds = (x[1] = z) ? 2 : 1;
	}
	else {
#ifdef DEBUG_DTOA
		if (!z)
			Bug("Zero passed to d2b");
#endif
		k = lo0bits(&z);
		x[0] = z;
		i = b->wds = 1;
		k += 32;
	}
#else
	if ((y = d1) != 0) {
		if ((k = lo0bits(&y)) != 0)
			if (k >= 16) {
				x[0] = y | z << 32 - k & 0xffff;
				x[1] = z >> k - 16 & 0xffff;
				x[2] = z >> k;
				i = 2;
			}
			else {
				x[0] = y & 0xffff;
				x[1] = y >> 16 | z << 16 - k & 0xffff;
				x[2] = z >> k & 0xffff;
				x[3] = z >> k+16;
				i = 3;
			}
		else {
			x[0] = y & 0xffff;
			x[1] = y >> 16;
			x[2] = z & 0xffff;
			x[3] = z >> 16;
			i = 3;
		}
	}
	else {
#ifdef DEBUG_DTOA
		if (!z)
			Bug("Zero passed to d2b");
#endif
		k = lo0bits(&z);
		if (k >= 16) {
			x[0] = z;
			i = 0;
		}
		else {
			x[0] = z & 0xffff;
			x[1] = z >> 16;
			i = 1;
		}
		k += 32;
	}
	while(!x[i])
		--i;
	b->wds = i + 1;
#endif
#ifndef Sudden_Underflow
	if (de) {
#endif
#ifdef IBM
		*e = (de - Bias - (P-1) << 2) + k;
		*bits = 4*P + 8 - k - hi0bits(word0(d) & Frac_mask);
#else
		*e = de - Bias - (P-1) + k;
		*bits = P - k;
#endif
#ifndef Sudden_Underflow
	}
	else {
		*e = de - Bias - (P-1) + 1 + k;
#ifdef Pack_32
		*bits = 32*i - hi0bits(x[i-1]);
#else
		*bits = (i+2)*16 - hi0bits(x[i]);
#endif
	}
#endif
	return b;
}
#undef d0
#undef d1

static double
ratio
#ifdef KR_headers
(a, b) Bigint *a, *b;
#else
(Bigint *a, Bigint *b)
#endif
{
	double da, db;
	PRInt32 k, ka, kb;

	da = b2d(a, &ka);
	db = b2d(b, &kb);
#ifdef Pack_32
	k = ka - kb + 32*(a->wds - b->wds);
#else
	k = ka - kb + 16*(a->wds - b->wds);
#endif
#ifdef IBM
	if (k > 0) {
		word0(da) += (k >> 2)*Exp_msk1;
		if (k &= 3)
			da *= 1 << k;
	}
	else {
		k = -k;
		word0(db) += (k >> 2)*Exp_msk1;
		if (k &= 3)
			db *= 1 << k;
	}
#else
	if (k > 0)
		word0(da) += k*Exp_msk1;
	else {
		k = -k;
		word0(db) += k*Exp_msk1;
	}
#endif
	return da / db;
}

static CONST double
tens[] = {
	1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9,
	1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19,
	1e20, 1e21, 1e22
#ifdef VAX
	, 1e23, 1e24
#endif
};

static CONST double
#ifdef IEEE_Arith
bigtens[] = { 1e16, 1e32, 1e64, 1e128, 1e256 };
static CONST double tinytens[] = { 1e-16, 1e-32, 1e-64, 1e-128, 1e-256 };
#define n_bigtens 5
#else
#ifdef IBM
bigtens[] = { 1e16, 1e32, 1e64 };
static CONST double tinytens[] = { 1e-16, 1e-32, 1e-64 };
#define n_bigtens 3
#else
bigtens[] = { 1e16, 1e32 };
static CONST double tinytens[] = { 1e-16, 1e-32 };
#define n_bigtens 2
#endif
#endif

void _PR_InitDtoa(void)
{
	freelist_lock = PR_NewLock();
	p5s_lock = PR_NewLock();
}

void _PR_CleanupDtoa(void)
{
    PR_DestroyLock(freelist_lock);
    freelist_lock = NULL;
    PR_DestroyLock(p5s_lock);
    p5s_lock = NULL;

    /* FIXME: deal with freelist and p5s. */
}

#if defined(HAVE_WATCOM_BUG_1)
PRFloat64 __pascal __loadds  __export
#else
PR_IMPLEMENT(PRFloat64) 
#endif
PR_strtod(CONST char *s00, char **se)
{
	PRInt32 bb2, bb5, bbe, bd2, bd5, bbbits, bs2, c, dsign,
		e, e1, esign, i, j, k, nd, nd0, nf, nz, nz0, sign;
	CONST char *s, *s0, *s1;
	PRFloat64 aadj, aadj1, adj, rv, rv0;
	Long L;
	unsigned Long y, z;
	Bigint *bb, *bb1, *bd, *bd0, *bs, *delta;


	if (!_pr_initialized) _PR_ImplicitInitialization();

	sign = nz0 = nz = 0;
	rv = 0.;
	for(s = s00;;s++) switch(*s) {
	case '-':
		sign = 1;
		/* no break */
	case '+':
		if (*++s)
			goto break2;
		/* no break */
	case 0:
		s = s00;
		goto ret;
	case '\t':
	case '\n':
	case '\v':
	case '\f':
	case '\r':
	case ' ':
		continue;
	default:
		goto break2;
	}
break2:
	if (*s == '0') {
		nz0 = 1;
		while(*++s == '0') ;
		if (!*s)
			goto ret;
	}
	s0 = s;
	y = z = 0;
	for(nd = nf = 0; (c = *s) >= '0' && c <= '9'; nd++, s++)
		if (nd < 9)
			y = 10*y + c - '0';
		else if (nd < 16)
			z = 10*z + c - '0';
	nd0 = nd;
	if (c == '.') {
		c = *++s;
		if (!nd) {
			for(; c == '0'; c = *++s)
				nz++;
			if (c > '0' && c <= '9') {
				s0 = s;
				nf += nz;
				nz = 0;
				goto have_dig;
			}
			goto dig_done;
		}
		for(; c >= '0' && c <= '9'; c = *++s) {
		have_dig:
			nz++;
			if (c -= '0') {
				nf += nz;
				for(i = 1; i < nz; i++)
					if (nd++ < 9)
						y *= 10;
					else if (nd <= DBL_DIG + 1)
						z *= 10;
				if (nd++ < 9)
					y = 10*y + c;
				else if (nd <= DBL_DIG + 1)
					z = 10*z + c;
				nz = 0;
			}
		}
	}
dig_done:
	e = 0;
	if (c == 'e' || c == 'E') {
		if (!nd && !nz && !nz0) {
			s = s00;
			goto ret;
		}
		s00 = s;
		esign = 0;
		switch(c = *++s) {
		case '-':
			esign = 1;
		case '+':
			c = *++s;
		}
		if (c >= '0' && c <= '9') {
			while(c == '0')
				c = *++s;
			if (c > '0' && c <= '9') {
				L = c - '0';
				s1 = s;
				while((c = *++s) >= '0' && c <= '9')
					L = 10*L + c - '0';
				if (s - s1 > 8 || L > 19999)
					/* Avoid confusion from exponents
					 * so large that e might overflow.
					 */
					e = 19999; /* safe for 16 bit ints */
				else
					e = (PRInt32)L;
				if (esign)
					e = -e;
			}
			else
				e = 0;
		}
		else
			s = s00;
	}
	if (!nd) {
		if (!nz && !nz0)
			s = s00;
		goto ret;
	}
	e1 = e -= nf;

	/* Now we have nd0 digits, starting at s0, followed by a
	 * decimal point, followed by nd-nd0 digits.  The number we're
	 * after is the integer represented by those digits times
	 * 10**e */

	if (!nd0)
		nd0 = nd;
	k = nd < DBL_DIG + 1 ? nd : DBL_DIG + 1;
	rv = y;
	if (k > 9)
		rv = tens[k - 9] * rv + z;
	bd0 = 0;
	if (nd <= DBL_DIG
#ifndef RND_PRODQUOT
		&& FLT_ROUNDS == 1
#endif
		) {
		if (!e)
			goto ret;
		if (e > 0) {
			if (e <= Ten_pmax) {
#ifdef VAX
				goto vax_ovfl_check;
#else
				/* rv = */ rounded_product(rv, tens[e]);
				goto ret;
#endif
			}
			i = DBL_DIG - nd;
			if (e <= Ten_pmax + i) {
				/* A fancier test would sometimes let us do
				 * this for larger i values.
				 */
				e -= i;
				rv *= tens[i];
#ifdef VAX
				/* VAX exponent range is so narrow we must
				 * worry about overflow here...
				 */
			vax_ovfl_check:
				word0(rv) -= P*Exp_msk1;
				/* rv = */ rounded_product(rv, tens[e]);
				if ((word0(rv) & Exp_mask)
					> Exp_msk1*(DBL_MAX_EXP+Bias-1-P))
					goto ovfl;
				word0(rv) += P*Exp_msk1;
#else
				/* rv = */ rounded_product(rv, tens[e]);
#endif
				goto ret;
			}
		}
#ifndef Inaccurate_Divide
		else if (e >= -Ten_pmax) {
			/* rv = */ rounded_quotient(rv, tens[-e]);
			goto ret;
		}
#endif
	}
	e1 += nd - k;

	/* Get starting approximation = rv * 10**e1 */

	if (e1 > 0) {
		if ((i = e1 & 15) != 0)
			rv *= tens[i];
		if (e1 &= ~15) {
			if (e1 > DBL_MAX_10_EXP) {
			ovfl:
				PR_SetError(PR_RANGE_ERROR, 0);
#ifdef __STDC__
				rv = HUGE_VAL;
#else
				/* Can't trust HUGE_VAL */
#ifdef IEEE_Arith
				word0(rv) = Exp_mask;
				word1(rv) = 0;
#else
				word0(rv) = Big0;
				word1(rv) = Big1;
#endif
#endif
				if (bd0)
					goto retfree;
				goto ret;
			}
			if (e1 >>= 4) {
				for(j = 0; e1 > 1; j++, e1 >>= 1)
					if (e1 & 1)
						rv *= bigtens[j];
				/* The last multiplication could overflow. */
				word0(rv) -= P*Exp_msk1;
				rv *= bigtens[j];
				if ((z = word0(rv) & Exp_mask)
					> Exp_msk1*(DBL_MAX_EXP+Bias-P))
					goto ovfl;
				if (z > Exp_msk1*(DBL_MAX_EXP+Bias-1-P)) {
					/* set to largest number */
					/* (Can't trust DBL_MAX) */
					word0(rv) = Big0;
					word1(rv) = Big1;
				}
				else
					word0(rv) += P*Exp_msk1;
			}

		}
	}
	else if (e1 < 0) {
		e1 = -e1;
		if ((i = e1 & 15) != 0)
			rv /= tens[i];
		if (e1 &= ~15) {
			e1 >>= 4;
			if (e1 >= 1 << n_bigtens)
				goto undfl;
			for(j = 0; e1 > 1; j++, e1 >>= 1)
				if (e1 & 1)
					rv *= tinytens[j];
			/* The last multiplication could underflow. */
			rv0 = rv;
			rv *= tinytens[j];
			if (!rv) {
				rv = 2.*rv0;
				rv *= tinytens[j];
				if (!rv) {
				undfl:
					rv = 0.;
					PR_SetError(PR_RANGE_ERROR, 0);
					if (bd0)
						goto retfree;
					goto ret;
				}
				word0(rv) = Tiny0;
				word1(rv) = Tiny1;
				/* The refinement below will clean
				 * this approximation up.
				 */
			}
		}
	}

	/* Now the hard part -- adjusting rv to the correct value.*/

	/* Put digits into bd: true value = bd * 10^e */

	bd0 = s2b(s0, nd0, nd, y);

	for(;;) {
		bd = Balloc(bd0->k);
		Bcopy(bd, bd0);
		bb = d2b(rv, &bbe, &bbbits);	/* rv = bb * 2^bbe */
		bs = i2b(1);

		if (e >= 0) {
			bb2 = bb5 = 0;
			bd2 = bd5 = e;
		}
		else {
			bb2 = bb5 = -e;
			bd2 = bd5 = 0;
		}
		if (bbe >= 0)
			bb2 += bbe;
		else
			bd2 -= bbe;
		bs2 = bb2;
#ifdef Sudden_Underflow
#ifdef IBM
		j = 1 + 4*P - 3 - bbbits + ((bbe + bbbits - 1) & 3);
#else
		j = P + 1 - bbbits;
#endif
#else
		i = bbe + bbbits - 1;	/* logb(rv) */
		if (i < Emin)	/* denormal */
			j = bbe + (P-Emin);
		else
			j = P + 1 - bbbits;
#endif
		bb2 += j;
		bd2 += j;
		i = bb2 < bd2 ? bb2 : bd2;
		if (i > bs2)
			i = bs2;
		if (i > 0) {
			bb2 -= i;
			bd2 -= i;
			bs2 -= i;
		}
		if (bb5 > 0) {
			bs = pow5mult(bs, bb5);
			bb1 = mult(bs, bb);
			Bfree(bb);
			bb = bb1;
		}
		if (bb2 > 0)
			bb = lshift(bb, bb2);
		if (bd5 > 0)
			bd = pow5mult(bd, bd5);
		if (bd2 > 0)
			bd = lshift(bd, bd2);
		if (bs2 > 0)
			bs = lshift(bs, bs2);
		delta = diff(bb, bd);
		dsign = delta->sign;
		delta->sign = 0;
		i = cmp(delta, bs);
		if (i < 0) {
			/* Error is less than half an ulp -- check for
			 * special case of mantissa a power of two.
			 */
			if (dsign || word1(rv) || word0(rv) & Bndry_mask)
				break;
			delta = lshift(delta,Log2P);
			if (cmp(delta, bs) > 0)
				goto drop_down;
			break;
		}
		if (i == 0) {
			/* exactly half-way between */
			if (dsign) {
				if ((word0(rv) & Bndry_mask1) == Bndry_mask1
					&&  word1(rv) == 0xffffffff) {
					/*boundary case -- increment exponent*/
					word0(rv) = (word0(rv) & Exp_mask)
						+ Exp_msk1
#ifdef IBM
						| Exp_msk1 >> 4
#endif
						;
					word1(rv) = 0;
					break;
				}
			}
			else if (!(word0(rv) & Bndry_mask) && !word1(rv)) {
			drop_down:
				/* boundary case -- decrement exponent */
#ifdef Sudden_Underflow
				L = word0(rv) & Exp_mask;
#ifdef IBM
				if (L <  Exp_msk1)
#else
					if (L <= Exp_msk1)
#endif
						goto undfl;
				L -= Exp_msk1;
#else
				L = (long)((word0(rv) & Exp_mask) - Exp_msk1);
#endif
				word0(rv) = L | Bndry_mask1;
				word1(rv) = 0xffffffff;
#ifdef IBM
				goto cont;
#else
				break;
#endif
			}
#ifndef ROUND_BIASED
			if (!(word1(rv) & LSB))
				break;
#endif
			if (dsign)
				rv += ulp(rv);
#ifndef ROUND_BIASED
			else {
				rv -= ulp(rv);
#ifndef Sudden_Underflow
				if (!rv)
					goto undfl;
#endif
			}
#endif
			break;
		}
		if ((aadj = ratio(delta, bs)) <= 2.) {
			if (dsign)
				aadj = aadj1 = 1.;
			else if (word1(rv) || word0(rv) & Bndry_mask) {
#ifndef Sudden_Underflow
				if (word1(rv) == Tiny1 && !word0(rv))
					goto undfl;
#endif
				aadj = 1.;
				aadj1 = -1.;
			}
			else {
				/* special case -- power of FLT_RADIX to be */
				/* rounded down... */

				if (aadj < 2./FLT_RADIX)
					aadj = 1./FLT_RADIX;
				else
					aadj *= 0.5;
				aadj1 = -aadj;
			}
		}
		else {
			aadj *= 0.5;
			aadj1 = dsign ? aadj : -aadj;
#ifdef Check_FLT_ROUNDS
			switch(FLT_ROUNDS) {
			case 2: /* towards +infinity */
				aadj1 -= 0.5;
				break;
			case 0: /* towards 0 */
			case 3: /* towards -infinity */
				aadj1 += 0.5;
			}
#else
			if (FLT_ROUNDS == 0)
				aadj1 += 0.5;
#endif
		}
		y = word0(rv) & Exp_mask;

		/* Check for overflow */

		if (y == Exp_msk1*(DBL_MAX_EXP+Bias-1)) {
			rv0 = rv;
			word0(rv) -= P*Exp_msk1;
			adj = aadj1 * ulp(rv);
			rv += adj;
			if ((word0(rv) & Exp_mask) >=
				Exp_msk1*(DBL_MAX_EXP+Bias-P)) {
				if (word0(rv0) == Big0 && word1(rv0) == Big1)
					goto ovfl;
				word0(rv) = Big0;
				word1(rv) = Big1;
				goto cont;
			}
			else
				word0(rv) += P*Exp_msk1;
		}
		else {
#ifdef Sudden_Underflow
			if ((word0(rv) & Exp_mask) <= P*Exp_msk1) {
				rv0 = rv;
				word0(rv) += P*Exp_msk1;
				adj = aadj1 * ulp(rv);
				rv += adj;
#ifdef IBM
				if ((word0(rv) & Exp_mask) <  P*Exp_msk1)
#else
					if ((word0(rv) & Exp_mask) <= P*Exp_msk1)
#endif
						{
							if (word0(rv0) == Tiny0
								&& word1(rv0) == Tiny1)
								goto undfl;
							word0(rv) = Tiny0;
							word1(rv) = Tiny1;
							goto cont;
						}
					else
						word0(rv) -= P*Exp_msk1;
			}
			else {
				adj = aadj1 * ulp(rv);
				rv += adj;
			}
#else
			/* Compute adj so that the IEEE rounding rules will
			 * correctly round rv + adj in some half-way cases.
			 * If rv * ulp(rv) is denormalized (i.e.,
			 * y <= (P-1)*Exp_msk1), we must adjust aadj to avoid
			 * trouble from bits lost to denormalization;
			 * example: 1.2e-307 .
			 */
			if (y <= (P-1)*Exp_msk1 && aadj >= 1.) {
				aadj1 = (double)(PRInt32)(aadj + 0.5);
				if (!dsign)
					aadj1 = -aadj1;
			}
			adj = aadj1 * ulp(rv);
			rv += adj;
#endif
		}
		z = word0(rv) & Exp_mask;
		if (y == z) {
			/* Can we stop now? */
			L = (Long) aadj;
			aadj -= L;
			/* The tolerances below are conservative. */
			if (dsign || word1(rv) || word0(rv) & Bndry_mask) {
				if (aadj < .4999999 || aadj > .5000001)
					break;
			}
			else if (aadj < .4999999/FLT_RADIX)
				break;
		}
	cont:
		Bfree(bb);
		Bfree(bd);
		Bfree(bs);
		Bfree(delta);
	}
retfree:
	Bfree(bb);
	Bfree(bd);
	Bfree(bs);
	Bfree(bd0);
	Bfree(delta);
ret:
	if (se)
		*se = (char *)s;
	return sign ? -rv : rv;
}

static PRInt32
quorem(Bigint *b, Bigint *S)
{
	PRInt32 n;
	Long borrow, y;
	unsigned Long carry, q, ys;
	unsigned Long *bx, *bxe, *sx, *sxe;
#ifdef Pack_32
	Long z;
	unsigned Long si, zs;
#endif

	n = S->wds;
#ifdef DEBUG_DTOA
	/*debug*/ if (b->wds > n)
		/*debug*/	Bug("oversize b in quorem");
#endif
	if (b->wds < n)
		return 0;
	sx = S->x;
	sxe = sx + --n;
	bx = b->x;
	bxe = bx + n;
	q = *bxe / (*sxe + 1);	/* ensure q <= true quotient */
#ifdef DEBUG_DTOA
	/*debug*/ if (q > 9)
		/*debug*/	Bug("oversized quotient in quorem");
#endif
	if (q) {
		borrow = 0;
		carry = 0;
		do {
#ifdef Pack_32
			si = *sx++;
			ys = (si & 0xffff) * q + carry;
			zs = (si >> 16) * q + (ys >> 16);
			carry = zs >> 16;
			y = (long)((*bx & 0xffff) - (ys & 0xffff) + borrow);
			borrow = y >> 16;
			Sign_Extend(borrow, y);
			z = (long)((*bx >> 16) - (zs & 0xffff) + borrow);
			borrow = z >> 16;
			Sign_Extend(borrow, z);
			Storeinc(bx, z, y);
#else
			ys = *sx++ * q + carry;
			carry = ys >> 16;
			y = *bx - (ys & 0xffff) + borrow;
			borrow = y >> 16;
			Sign_Extend(borrow, y);
			*bx++ = y & 0xffff;
#endif
		}
		while(sx <= sxe);
		if (!*bxe) {
			bx = b->x;
			while(--bxe > bx && !*bxe)
				--n;
			b->wds = n;
		}
	}
	if (cmp(b, S) >= 0) {
		q++;
		borrow = 0;
		carry = 0;
		bx = b->x;
		sx = S->x;
		do {
#ifdef Pack_32
			si = *sx++;
			ys = (si & 0xffff) + carry;
			zs = (si >> 16) + (ys >> 16);
			carry = zs >> 16;
			y = (long)((*bx & 0xffff) - (ys & 0xffff) + borrow);
			borrow = y >> 16;
			Sign_Extend(borrow, y);
			z = (long)((*bx >> 16) - (zs & 0xffff) + borrow);
			borrow = z >> 16;
			Sign_Extend(borrow, z);
			Storeinc(bx, z, y);
#else
			ys = *sx++ + carry;
			carry = ys >> 16;
			y = *bx - (ys & 0xffff) + borrow;
			borrow = y >> 16;
			Sign_Extend(borrow, y);
			*bx++ = y & 0xffff;
#endif
		}
		while(sx <= sxe);
		bx = b->x;
		bxe = bx + n;
		if (!*bxe) {
			while(--bxe > bx && !*bxe)
				--n;
			b->wds = n;
		}
	}
	return (int)q;
}

/* PR_dtoa for IEEE arithmetic (dmg): convert double to ASCII string.
 *
 * Inspired by "How to Print Floating-Point Numbers Accurately" by
 * Guy L. Steele, Jr. and Jon L. White [Proc. ACM SIGPLAN '90, pp. 112-126].
 *
 * Modifications:
 *	1. Rather than iterating, we use a simple numeric overestimate
 *	   to determine k = floor(log10(d)).  We scale relevant
 *	   quantities using O(log2(k)) rather than O(k) multiplications.
 *	2. For some modes > 2 (corresponding to ecvt and fcvt), we don't
 *	   try to generate digits strictly left to right.  Instead, we
 *	   compute with fewer bits and propagate the carry if necessary
 *	   when rounding the final digit up.  This is often faster.
 *	3. Under the assumption that input will be rounded nearest,
 *	   mode 0 renders 1e23 as 1e23 rather than 9.999999999999999e22.
 *	   That is, we allow equality in stopping tests when the
 *	   round-nearest rule will give the same floating-point value
 *	   as would satisfaction of the stopping test with strict
 *	   inequality.
 *	4. We remove common factors of powers of 2 from relevant
 *	   quantities.
 *	5. When converting floating-point integers less than 1e16,
 *	   we use floating-point arithmetic rather than resorting
 *	   to multiple-precision integers.
 *	6. When asked to produce fewer than 15 digits, we first try
 *	   to get by with floating-point arithmetic; we resort to
 *	   multiple-precision integer arithmetic only if we cannot
 *	   guarantee that the floating-point calculation has given
 *	   the correctly rounded result.  For k requested digits and
 *	   "uniformly" distributed input, the probability is
 *	   something like 10^(k-15) that we must resort to the Long
 *	   calculation.
 */

PR_IMPLEMENT(PRStatus)
PR_dtoa(PRFloat64 d, int mode, int ndigits,
	int *decpt, int *sign, char **rve, char *buf, PRSize bufsize)
{
	/*	Arguments ndigits, decpt, sign are similar to those
		of ecvt and fcvt; trailing zeros are suppressed from
		the returned string.  If not null, *rve is set to point
		to the end of the return value.  If d is +-Infinity or NaN,
		then *decpt is set to 9999.

		mode:
		0 ==> shortest string that yields d when read in
		and rounded to nearest.
		1 ==> like 0, but with Steele & White stopping rule;
		e.g. with IEEE P754 arithmetic , mode 0 gives
		1e23 whereas mode 1 gives 9.999999999999999e22.
		2 ==> max(1,ndigits) significant digits.  This gives a
		return value similar to that of ecvt, except
		that trailing zeros are suppressed.
		3 ==> through ndigits past the decimal point.  This
		gives a return value similar to that from fcvt,
		except that trailing zeros are suppressed, and
		ndigits can be negative.
		4-9 should give the same return values as 2-3, i.e.,
		4 <= mode <= 9 ==> same return as mode
		2 + (mode & 1).  These modes are mainly for
		debugging; often they run slower but sometimes
		faster than modes 2-3.
		4,5,8,9 ==> left-to-right digit generation.
		6-9 ==> don't try fast floating-point estimate
		(if applicable).

		Values of mode other than 0-9 are treated as mode 0.

		Sufficient space is allocated to the return value
		to hold the suppressed trailing zeros.
	*/

	PRInt32 bbits, b2, b5, be, dig, i, ieps, ilim, ilim0, ilim1,
		j, j1, k, k0, k_check, leftright, m2, m5, s2, s5,
		spec_case, try_quick;
	Long L;
#ifndef Sudden_Underflow
	PRInt32 denorm;
	unsigned Long x;
#endif
	Bigint *b, *b1, *delta, *mlo, *mhi, *S;
	double d2, ds, eps;
	char *s, *s0;
	Bigint *result = 0;
	PRInt32 result_k;
	PRStatus retval;
	PRSize strsize;

	if (!_pr_initialized) _PR_ImplicitInitialization();

	if (word0(d) & Sign_bit) {
		/* set sign for everything, including 0's and NaNs */
		*sign = 1;
		word0(d) &= ~Sign_bit;	/* clear sign bit */
	}
	else
		*sign = 0;

#if defined(IEEE_Arith) + defined(VAX)
#ifdef IEEE_Arith
	if ((word0(d) & Exp_mask) == Exp_mask)
#else
		if (word0(d)  == 0x8000)
#endif
			{
				/* Infinity or NaN */
				*decpt = 9999;
				s =
#ifdef IEEE_Arith
					!word1(d) && !(word0(d) & 0xfffff) ? "Infinity" :
#endif
					"NaN";
				if ((s[0] == 'I' && bufsize < 9) || (s[0] == 'N' && bufsize < 4)) {
					PR_SetError(PR_BUFFER_OVERFLOW_ERROR, 0);
					return PR_FAILURE;
				}
				strcpy(buf, s);
				if (rve) {
					*rve =
#ifdef IEEE_Arith
						buf[3] ? buf + 8 :
#endif
					buf + 3;
					PR_ASSERT(**rve == '\0');
				}
				return PR_SUCCESS;
			}
#endif
#ifdef IBM
	d += 0; /* normalize */
#endif
	if (!d) {
		*decpt = 1;
		if (bufsize < 2) {
			PR_SetError(PR_BUFFER_OVERFLOW_ERROR, 0);
			return PR_FAILURE;
		}
		buf[0] = '0'; buf[1] = '\0';  /* copy "0" to buffer */
		if (rve) {
			*rve = buf + 1;
			PR_ASSERT(**rve == '\0');
		}
		return PR_SUCCESS;
	}

	b = d2b(d, &be, &bbits);
#ifdef Sudden_Underflow
	i = (PRInt32)(word0(d) >> Exp_shift1 & (Exp_mask>>Exp_shift1));
#else
	if ((i = (PRInt32)(word0(d) >> Exp_shift1 & (Exp_mask>>Exp_shift1))) != 0) {
#endif
		d2 = d;
		word0(d2) &= Frac_mask1;
		word0(d2) |= Exp_11;
#ifdef IBM
		if (j = 11 - hi0bits(word0(d2) & Frac_mask))
			d2 /= 1 << j;
#endif

		/* log(x)	~=~ log(1.5) + (x-1.5)/1.5
		 * log10(x)	 =  log(x) / log(10)
		 *		~=~ log(1.5)/log(10) + (x-1.5)/(1.5*log(10))
		 * log10(d) = (i-Bias)*log(2)/log(10) + log10(d2)
		 *
		 * This suggests computing an approximation k to log10(d) by
		 *
		 * k = (i - Bias)*0.301029995663981
		 *	+ ( (d2-1.5)*0.289529654602168 + 0.176091259055681 );
		 *
		 * We want k to be too large rather than too small.
		 * The error in the first-order Taylor series approximation
		 * is in our favor, so we just round up the constant enough
		 * to compensate for any error in the multiplication of
		 * (i - Bias) by 0.301029995663981; since |i - Bias| <= 1077,
		 * and 1077 * 0.30103 * 2^-52 ~=~ 7.2e-14,
		 * adding 1e-13 to the constant term more than suffices.
		 * Hence we adjust the constant term to 0.1760912590558.
		 * (We could get a more accurate k by invoking log10,
		 *  but this is probably not worthwhile.)
		 */

		i -= Bias;
#ifdef IBM
		i <<= 2;
		i += j;
#endif
#ifndef Sudden_Underflow
		denorm = 0;
	}
	else {
		/* d is denormalized */

		i = bbits + be + (Bias + (P-1) - 1);
		x = i > 32  ? word0(d) << (64 - i) | word1(d) >> (i - 32)
			: word1(d) << (32 - i);
		d2 = x;
		word0(d2) -= 31*Exp_msk1; /* adjust exponent */
		i -= (Bias + (P-1) - 1) + 1;
		denorm = 1;
	}
#endif
	ds = (d2-1.5)*0.289529654602168 + 0.1760912590558 + i*0.301029995663981;
	k = (PRInt32)ds;
	if (ds < 0. && ds != k)
		k--;	/* want k = floor(ds) */
	k_check = 1;
	if (k >= 0 && k <= Ten_pmax) {
		if (d < tens[k])
			k--;
		k_check = 0;
	}
	j = bbits - i - 1;
	if (j >= 0) {
		b2 = 0;
		s2 = j;
	}
	else {
		b2 = -j;
		s2 = 0;
	}
	if (k >= 0) {
		b5 = 0;
		s5 = k;
		s2 += k;
	}
	else {
		b2 -= k;
		b5 = -k;
		s5 = 0;
	}
	if (mode < 0 || mode > 9)
		mode = 0;
	try_quick = 1;
	if (mode > 5) {
		mode -= 4;
		try_quick = 0;
	}
	leftright = 1;
	switch(mode) {
	case 0:
	case 1:
		ilim = ilim1 = -1;
		i = 18;
		ndigits = 0;
		break;
	case 2:
		leftright = 0;
		/* no break */
	case 4:
		if (ndigits <= 0)
			ndigits = 1;
		ilim = ilim1 = i = ndigits;
		break;
	case 3:
		leftright = 0;
		/* no break */
	case 5:
		i = ndigits + k + 1;
		ilim = i;
		ilim1 = i - 1;
		if (i <= 0)
			i = 1;
	}
	j = sizeof(unsigned Long);
	for(result_k = 0; sizeof(Bigint) - sizeof(unsigned Long) <= i - j;
		j <<= 1) result_k++;
	result = Balloc(result_k);
	s = s0 = (char *)result;

	if (ilim >= 0 && ilim <= Quick_max && try_quick) {

		/* Try to get by with floating-point arithmetic. */

		i = 0;
		d2 = d;
		k0 = k;
		ilim0 = ilim;
		ieps = 2; /* conservative */
		if (k > 0) {
			ds = tens[k&0xf];
			j = k >> 4;
			if (j & Bletch) {
				/* prevent overflows */
				j &= Bletch - 1;
				d /= bigtens[n_bigtens-1];
				ieps++;
			}
			for(; j; j >>= 1, i++)
				if (j & 1) {
					ieps++;
					ds *= bigtens[i];
				}
			d /= ds;
		}
		else if ((j1 = -k) != 0) {
			d *= tens[j1 & 0xf];
			for(j = j1 >> 4; j; j >>= 1, i++)
				if (j & 1) {
					ieps++;
					d *= bigtens[i];
				}
		}
		if (k_check && d < 1. && ilim > 0) {
			if (ilim1 <= 0)
				goto fast_failed;
			ilim = ilim1;
			k--;
			d *= 10.;
			ieps++;
		}
		eps = ieps*d + 7.;
		word0(eps) -= (P-1)*Exp_msk1;
		if (ilim == 0) {
			S = mhi = 0;
			d -= 5.;
			if (d > eps)
				goto one_digit;
			if (d < -eps)
				goto no_digits;
			goto fast_failed;
		}
#ifndef No_leftright
		if (leftright) {
			/* Use Steele & White method of only
			 * generating digits needed.
			 */
			eps = 0.5/tens[ilim-1] - eps;
			for(i = 0;;) {
				L = (Long) d;
				d -= L;
				*s++ = '0' + (PRInt32)L;
				if (d < eps)
					goto ret1;
				if (1. - d < eps)
					goto bump_up;
				if (++i >= ilim)
					break;
				eps *= 10.;
				d *= 10.;
			}
		}
		else {
#endif
			/* Generate ilim digits, then fix them up. */
			eps *= tens[ilim-1];
			for(i = 1;; i++, d *= 10.) {
				L = (Long) d;
				d -= L;
				*s++ = '0' + (PRInt32)L;
				if (i == ilim) {
					if (d > 0.5 + eps)
						goto bump_up;
					else if (d < 0.5 - eps) {
						while(*--s == '0'){} /* just count -- nothing to execute */
						s++;
						goto ret1;
					}
					break;
				}
			}
#ifndef No_leftright
		}
#endif
	fast_failed:
		s = s0;
		d = d2;
		k = k0;
		ilim = ilim0;
	}

	/* Do we have a "small" integer? */

	if (be >= 0 && k <= Int_max) {
		/* Yes. */
		ds = tens[k];
		if (ndigits < 0 && ilim <= 0) {
			S = mhi = 0;
			if (ilim < 0 || d <= 5*ds)
				goto no_digits;
			goto one_digit;
		}
		for(i = 1;; i++) {
			L = (Long) (d / ds);
			d -= L*ds;
#ifdef Check_FLT_ROUNDS
			/* If FLT_ROUNDS == 2, L will usually be high by 1 */
			if (d < 0) {
				L--;
				d += ds;
			}
#endif
			*s++ = '0' + (PRInt32)L;
			if (i == ilim) {
				d += d;
				if ((d > ds) || (d == ds && L & 1)) {
				bump_up:
					while(*--s == '9')
						if (s == s0) {
							k++;
							*s = '0';
							break;
						}
					++*s++;
				}
				break;
			}
			if (!(d *= 10.))
				break;
		}
		goto ret1;
	}

	m2 = b2;
	m5 = b5;
	mhi = mlo = 0;
	if (leftright) {
		if (mode < 2) {
			i =
#ifndef Sudden_Underflow
				denorm ? be + (Bias + (P-1) - 1 + 1) :
#endif
#ifdef IBM
				1 + 4*P - 3 - bbits + ((bbits + be - 1) & 3);
#else
			1 + P - bbits;
#endif
		}
		else {
			j = ilim - 1;
			if (m5 >= j)
				m5 -= j;
			else {
				s5 += j -= m5;
				b5 += j;
				m5 = 0;
			}
			if ((i = ilim) < 0) {
				m2 -= i;
				i = 0;
			}
		}
		b2 += i;
		s2 += i;
		mhi = i2b(1);
	}
	if (m2 > 0 && s2 > 0) {
		i = m2 < s2 ? m2 : s2;
		b2 -= i;
		m2 -= i;
		s2 -= i;
	}
	if (b5 > 0) {
		if (leftright) {
			if (m5 > 0) {
				mhi = pow5mult(mhi, m5);
				b1 = mult(mhi, b);
				Bfree(b);
				b = b1;
			}
			if ((j = b5 - m5) != 0)
				b = pow5mult(b, j);
		}
		else
			b = pow5mult(b, b5);
	}
	S = i2b(1);
	if (s5 > 0)
		S = pow5mult(S, s5);

	/* Check for special case that d is a normalized power of 2. */

	if (mode < 2) {
		if (!word1(d) && !(word0(d) & Bndry_mask)
#ifndef Sudden_Underflow
			&& word0(d) & Exp_mask
#endif
			) {
			/* The special case */
			b2 += Log2P;
			s2 += Log2P;
			spec_case = 1;
		}
		else
			spec_case = 0;
	}

	/* Arrange for convenient computation of quotients:
	 * shift left if necessary so divisor has 4 leading 0 bits.
	 *
	 * Perhaps we should just compute leading 28 bits of S once
	 * and for all and pass them and a shift to quorem, so it
	 * can do shifts and ors to compute the numerator for q.
	 */
#ifdef Pack_32
	if ((i = ((s5 ? 32 - hi0bits(S->x[S->wds-1]) : 1) + s2) & 0x1f) != 0)
		i = 32 - i;
#else
	if (i = ((s5 ? 32 - hi0bits(S->x[S->wds-1]) : 1) + s2) & 0xf)
		i = 16 - i;
#endif
	if (i > 4) {
		i -= 4;
		b2 += i;
		m2 += i;
		s2 += i;
	}
	else if (i < 4) {
		i += 28;
		b2 += i;
		m2 += i;
		s2 += i;
	}
	if (b2 > 0)
		b = lshift(b, b2);
	if (s2 > 0)
		S = lshift(S, s2);
	if (k_check) {
		if (cmp(b,S) < 0) {
			k--;
			b = multadd(b, 10, 0);	/* we botched the k estimate */
			if (leftright)
				mhi = multadd(mhi, 10, 0);
			ilim = ilim1;
		}
	}
	if (ilim <= 0 && mode > 2) {
		if (ilim < 0 || cmp(b,S = multadd(S,5,0)) <= 0) {
			/* no digits, fcvt style */
		no_digits:
			k = -1 - ndigits;
			goto ret;
		}
	one_digit:
		*s++ = '1';
		k++;
		goto ret;
	}
	if (leftright) {
		if (m2 > 0)
			mhi = lshift(mhi, m2);

		/* Compute mlo -- check for special case
		 * that d is a normalized power of 2.
		 */

		mlo = mhi;
		if (spec_case) {
			mhi = Balloc(mhi->k);
			Bcopy(mhi, mlo);
			mhi = lshift(mhi, Log2P);
		}

		for(i = 1;;i++) {
			dig = quorem(b,S) + '0';
			/* Do we yet have the shortest decimal string
			 * that will round to d?
			 */
			j = cmp(b, mlo);
			delta = diff(S, mhi);
			j1 = delta->sign ? 1 : cmp(b, delta);
			Bfree(delta);
#ifndef ROUND_BIASED
			if (j1 == 0 && !mode && !(word1(d) & 1)) {
				if (dig == '9')
					goto round_9_up;
				if (j > 0)
					dig++;
				*s++ = dig;
				goto ret;
			}
#endif
			if ((j < 0) || ((j == 0) && (!mode)
#ifndef ROUND_BIASED
				&& (!(word1(d) & 1)))
#endif
				) {
				if (j1 > 0) {
					b = lshift(b, 1);
					j1 = cmp(b, S);
					if (((j1 > 0) || (j1 == 0 && dig & 1))
						&& (dig++ == '9'))
						goto round_9_up;
				}
				*s++ = dig;
				goto ret;
			}
			if (j1 > 0) {
				if (dig == '9') { /* possible if i == 1 */
				round_9_up:
					*s++ = '9';
					goto roundoff;
				}
				*s++ = dig + 1;
				goto ret;
			}
			*s++ = dig;
			if (i == ilim)
				break;
			b = multadd(b, 10, 0);
			if (mlo == mhi)
				mlo = mhi = multadd(mhi, 10, 0);
			else {
				mlo = multadd(mlo, 10, 0);
				mhi = multadd(mhi, 10, 0);
			}
		}
	}
	else
		for(i = 1;; i++) {
			*s++ = dig = quorem(b,S) + '0';
			if (i >= ilim)
				break;
			b = multadd(b, 10, 0);
		}

	/* Round off last digit */

	b = lshift(b, 1);
	j = cmp(b, S);
	if ((j > 0) || (j == 0 && dig & 1)) {
	roundoff:
		while(*--s == '9')
			if (s == s0) {
				k++;
				*s++ = '1';
				goto ret;
			}
		++*s++;
	}
	else {
		while(*--s == '0'){} /* just count -- nothing to execute */
		s++;
	}
ret:
	Bfree(S);
	if (mhi) {
		if (mlo && mlo != mhi)
			Bfree(mlo);
		Bfree(mhi);
	}
ret1:
	Bfree(b);
	*s = 0;
	*decpt = k + 1;
	strsize = (s - s0) + 1;
	if (strsize <= bufsize) {
		retval = PR_SUCCESS;
		memcpy(buf, s0, strsize);
		if (rve) {
			*rve = buf + strsize - 1;
			PR_ASSERT(**rve == '\0');
		}
	} else {
		PR_SetError(PR_BUFFER_OVERFLOW_ERROR, 0);
		retval = PR_FAILURE;
	}

	/* cleanup */
	result->k = result_k;
	result->maxwds = 1 << result_k;
	Bfree(result);

	return retval;
}

/*
** conversion routines for floating point
** prcsn - number of digits of precision to generate floating
** point value.
** This should be reparameterized so that you can send in a
**   prcn for the positive and negative ranges.  For now, 
**   conform to the ECMA JavaScript spec which says numbers
**   less than 1e-6 are in scientific notation.
** Also, the ECMA spec says that there should always be a
**   '+' or '-' after the 'e' in scientific notation
*/
PR_IMPLEMENT(void)
PR_cnvtf(char *buf,int bufsz, int prcsn,double fval)
{
    PRIntn decpt, sign, numdigits;
    char *num, *nump;
    char *bufp = buf;
    char *endnum;

	/* If anything fails, we store an empty string in 'buf' */
	num = (char*)PR_MALLOC(bufsz);
	if (num == NULL) {
		buf[0] = '\0';
		return;
	}
	/* XXX Why use mode 1? */
	if (PR_dtoa(fval,1,prcsn,&decpt,&sign,&endnum,num,bufsz)
			== PR_FAILURE) {
		buf[0] = '\0';
		goto done;
	}
	numdigits = endnum - num;
	nump = num;

	if (sign &&
	    !(word0(fval) == Sign_bit && word1(fval) == 0) &&
	    !((word0(fval) & Exp_mask) == Exp_mask &&
	      (word1(fval) || (word0(fval) & 0xfffff)))) {
	    *bufp++ = '-';
	}
	
	if(decpt == 9999){
		while((*bufp++ = *nump++) != 0){} /* nothing to execute */
		goto done;
	}

	if(decpt > (prcsn+1) || decpt < -(prcsn-1) || decpt < -5){
	    *bufp++ = *nump++;
	    if(numdigits != 1){
			*bufp++ = '.';
	    }
		
	    while(*nump != '\0'){
			*bufp++ = *nump++;
	    }
	    *bufp++ = 'e';
	    PR_snprintf(bufp,bufsz - (bufp - buf), "%+d",decpt-1);
	}
	else if(decpt >= 0){
	    if(decpt == 0){
			*bufp++ = '0';
	    }
	    else {
		while(decpt--){
			if(*nump != '\0'){
				*bufp++ = *nump++;
			}
			else {
				*bufp++ = '0';
			}
		}
	    }
	    if(*nump != '\0'){
			*bufp++ = '.';
			while(*nump != '\0'){
				*bufp++ = *nump++;
			}
	    }
	    *bufp++ = '\0';
	}
	else if(decpt < 0){
	    *bufp++ = '0';
	    *bufp++ = '.';
	    while(decpt++){
			*bufp++ = '0';
	    }
	    
	    while(*nump != '\0'){
			*bufp++ = *nump++;
	    }
	    *bufp++ = '\0';
	}
done:
	PR_DELETE(num);
}
