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
static const char CVS_ID[] = "@(#) $Source: /cvsroot/mozilla/security/nss/lib/pkix/src/RDNSequence/PRemoveRelativeDistinguishedName.c,v $ $Revision: 1.1.160.1 $ $Date: 2002/04/10 03:28:46 $ $Name: MOZILLA_1_0_RELEASE $";
#endif /* DEBUG */

#ifndef PKIX_H
#include "pkix.h"
#endif /* PKIX_H */

/*
 * nssPKIXRDNSequence_RemoveRelativeDistinguishedName
 *
 * -- fgmr comments --
 *
 * The error may be one of the following values:
 *  NSS_ERROR_INVALID_PKIX_RDN_SEQUENCE
 *  NSS_ERROR_VALUE_OUT_OF_RANGE
 *
 * Return value:
 *  PR_SUCCESS upon success
 *  PR_FAILURE upon failure
 */

NSS_IMPLEMENT PRStatus
nssPKIXRDNSequence_RemoveRelativeDistinguishedName
(
  NSSPKIXRDNSequence *rdnseq,
  PRInt32 i
)
{
  NSSPKIXRelativeDistinguishedName **na;
  PRInt32 c;

#ifdef NSSDEBUG
  if( PR_SUCCESS != nssPKIXRDNSequence_verifyPointer(rdnseq) ) {
    return PR_FAILURE;
  }
#endif /* NSSDEBUG */

  if( 0 == rdnseq->count ) {
    nss_pkix_RDNSequence_Count(rdnseq);
  }

#ifdef PEDANTIC
  if( 0 == rdnseq->count ) {
    nss_SetError(NSS_ERROR_VALUE_OUT_OF_RANGE);
    return (NSSPKIXRelativeDistinguishedName *)NULL;
  }
#endif /* PEDANTIC */

  if( (i < 0) || (i >= rdnseq->count) ) {
    nss_SetError(NSS_ERROR_VALUE_OUT_OF_RANGE);
    return (NSSPKIXRelativeDistinguishedName *)NULL;
  }

  nssPKIXRelativeDistinguishedName_Destroy(rdnseq->rdns[i]);

  rdnseq->rdns[i] = rdnseq->rdns[ rdnseq->count ];
  rdnseq->rdns[ rdnseq->count ] = (NSSPKIXRelativeDistinguishedName *)NULL;
  rdnseq->count--;

  na = (NSSPKIXRelativeDistinguishedName **)
    nss_ZRealloc(rdnseq->rdns, ((rdnseq->count) * 
      sizeof(NSSPKIXRelativeDistinguishedName *)));
  if( (NSSPKIXRelativeDistinguishedName **)NULL == na ) {
    return (NSSPKIXRelativeDistinguishedName *)NULL;
  }

  rdnseq->rdns = na;

  return nss_pkix_RDNSequence_Clear(rdnseq);
}
