/*
 *  Simple test driver for MPI library
 *
 *  Test 7: Random and divisibility tests
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
 * may use your version of this file under either the MPL or the GPL.
 *
 *  $Id: mptest-7.c,v 1.2.144.1 2002/04/10 03:27:40 cltbld%netscape.com Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>

#define MP_IOFUNC
#include "mpi.h"

#include "mpprime.h"

int main(int argc, char *argv[])
{
  mp_digit  num;
  mp_int    a, b;

  srand(time(NULL));

  if(argc < 3) {
    fprintf(stderr, "Usage: %s <a> <b>\n", argv[0]);
    return 1;
  }

  printf("Test 7: Random & divisibility tests\n\n");

  mp_init(&a);
  mp_init(&b);

  mp_read_radix(&a, argv[1], 10);
  mp_read_radix(&b, argv[2], 10);

  printf("a = "); mp_print(&a, stdout); fputc('\n', stdout);
  printf("b = "); mp_print(&b, stdout); fputc('\n', stdout);

  if(mpp_divis(&a, &b) == MP_YES)
    printf("a is divisible by b\n");
  else
    printf("a is not divisible by b\n");

  if(mpp_divis(&b, &a) == MP_YES)
    printf("b is divisible by a\n");
  else
    printf("b is not divisible by a\n");

  printf("\nb = mpp_random()\n");
  mpp_random(&b);
  printf("b = "); mp_print(&b, stdout); fputc('\n', stdout);
  mpp_random(&b);
  printf("b = "); mp_print(&b, stdout); fputc('\n', stdout);
  mpp_random(&b);
  printf("b = "); mp_print(&b, stdout); fputc('\n', stdout);

  printf("\nTesting a for divisibility by first 170 primes\n");
  num = 170;
  if(mpp_divis_primes(&a, &num) == MP_YES)
    printf("It is divisible by at least one of them\n");
  else
    printf("It is not divisible by any of them\n");

  mp_clear(&b);
  mp_clear(&a);

  return 0;
}
