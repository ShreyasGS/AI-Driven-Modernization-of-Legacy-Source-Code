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
static const char CVS_ID[] = "@(#) $RCSfile: certdecode.c,v $ $Revision: 1.11.16.1 $ $Date: 2002/04/10 03:28:10 $ $Name: MOZILLA_1_0_RELEASE $";
#endif /* DEBUG */

#ifndef PKIT_H
#include "pkit.h"
#endif /* PKIT_H */

#ifndef PKIM_H
#include "pkim.h"
#endif /* PKIM_H */

/* XXX
 * move this to a more appropriate location
 */
NSS_IMPLEMENT PRStatus
nssPKIObject_Initialize
(
  struct nssPKIObjectBaseStr *object,
  NSSArena *arena,
  NSSTrustDomain *td,
  NSSCryptoContext *cc
)
{
    object->arena = arena;
    object->trustDomain = td;
    object->cryptoContext = cc;
    object->lock = PZ_NewLock(nssILockOther);
    if (!object->lock) {
	return PR_FAILURE;
    }
    object->instanceList = nssList_Create(arena, PR_TRUE);
    if (!object->instanceList) {
	PZ_DestroyLock(object->lock);
	return PR_FAILURE;
    }
    object->instances = nssList_CreateIterator(object->instanceList);
    if (!object->instances) {
	nssList_Destroy(object->instanceList);
	PZ_DestroyLock(object->lock);
	return PR_FAILURE;
    }
    object->refCount = 1;
    return PR_SUCCESS;
}

/* XXX
 * move this to a more appropriate location
 */
NSS_IMPLEMENT void
nssPKIObject_AddRef
(
  struct nssPKIObjectBaseStr *object
)
{
    PZ_Lock(object->lock);
    object->refCount++;
    PZ_Unlock(object->lock);
}

/* XXX
 * move this to a more appropriate location
 */
NSS_IMPLEMENT PRBool
nssPKIObject_Destroy
(
  struct nssPKIObjectBaseStr *object
)
{
    PRUint32 refCount;
    PZ_Lock(object->lock);
    PORT_Assert(object->refCount > 0);
    refCount = --object->refCount;
    PZ_Unlock(object->lock);
    if (refCount == 0) {
	PZ_DestroyLock(object->lock);
	nssListIterator_Destroy(object->instances);
	nssList_Destroy(object->instanceList);
	nssArena_Destroy(object->arena);
	return PR_TRUE;
    }
    return PR_FALSE;
}

#ifdef NSS_3_4_CODE
/* This is defined in nss3hack.c */
NSS_EXTERN nssDecodedCert *
nssDecodedPKIXCertificate_Create
(
  NSSArena *arenaOpt,
  NSSDER *encoding
);

NSS_IMPLEMENT PRStatus
nssDecodedPKIXCertificate_Destroy
(
  nssDecodedCert *dc
);
#else /* NSS_4_0_CODE */
/* This is where 4.0 PKIX code will handle the decoding */
static nssDecodedCert *
nssDecodedPKIXCertificate_Create
(
  NSSArena *arenaOpt,
  NSSDER *encoding
)
{
    return (nssDecodedCert *)NULL;
}

static PRStatus
nssDecodedPKIXCertificate_Destroy
(
  nssDecodedCert *dc
)
{
    return PR_FAILURE;
}
#endif /* not NSS_3_4_CODE */

NSS_IMPLEMENT nssDecodedCert *
nssDecodedCert_Create
(
  NSSArena *arenaOpt,
  NSSDER *encoding,
  NSSCertificateType type
)
{
    nssDecodedCert *rvDC = NULL;
    switch(type) {
    case NSSCertificateType_PKIX:
	rvDC = nssDecodedPKIXCertificate_Create(arenaOpt, encoding);
	break;
    default:
#if 0
	nss_SetError(NSS_ERROR_INVALID_ARGUMENT);
#endif
	return (nssDecodedCert *)NULL;
    }
    return rvDC;
}

NSS_IMPLEMENT PRStatus
nssDecodedCert_Destroy
(
  nssDecodedCert *dc
)
{
    if (!dc) {
	return PR_FAILURE;
    }
    switch(dc->type) {
    case NSSCertificateType_PKIX:
	return nssDecodedPKIXCertificate_Destroy(dc);
    default:
#if 0
	nss_SetError(NSS_ERROR_INVALID_ARGUMENT);
#endif
	break;
    }
    return PR_FAILURE;
}

/* Of course none of this belongs here */

/* how bad would it be to have a static now sitting around, updated whenever
 * this was called?  would avoid repeated allocs...
 */
NSS_IMPLEMENT NSSTime *
NSSTime_Now
(
  NSSTime *timeOpt
)
{
    return NSSTime_SetPRTime(timeOpt, PR_Now());
}

NSS_IMPLEMENT NSSTime *
NSSTime_SetPRTime
(
  NSSTime *timeOpt,
  PRTime prTime
)
{
    NSSTime *rvTime;
    rvTime = (timeOpt) ? timeOpt : nss_ZNEW(NULL, NSSTime);
    rvTime->prTime = prTime;
    return rvTime;
}

NSS_IMPLEMENT PRTime
NSSTime_GetPRTime
(
  NSSTime *time
)
{
  return time->prTime;
}

