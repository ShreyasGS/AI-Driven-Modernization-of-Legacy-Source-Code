/*
 *  bbs_rand.h
 *
 *  Blum, Blum & Shub PRNG using the MPI library
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
 * The Original Code is the MPI Arbitrary Precision Integer Arithmetic
 * library.
 *
 * The Initial Developer of the Original Code is Michael J. Fromberger.
 * Portions created by Michael J. Fromberger are 
 * Copyright (C) 1998, 1999, 2000 Michael J. Fromberger. 
 * All Rights Reserved.
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
 *  $Id: bbs_rand.h,v 1.1.144.1 2002/04/10 03:27:43 cltbld%netscape.com Exp $
 */

#ifndef _H_BBSRAND_
#define _H_BBSRAND_

#include <limits.h>
#include "mpi.h"

#define  BBS_RAND_MAX    UINT_MAX

/* Suggested length of seed data */
extern int bbs_seed_size;

void         bbs_srand(unsigned char *data, int len);
unsigned int bbs_rand(void);

#endif /* end _H_BBSRAND_ */
