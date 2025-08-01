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
static const char CVS_ID[] = "@(#) $RCSfile: item.c,v $ $Revision: 1.2.18.1 $ $Date: 2002/04/10 03:26:29 $ $Name: MOZILLA_1_0_RELEASE $";
#endif /* DEBUG */

/*
 * item.c
 *
 * This contains some item-manipulation code.
 */

#ifndef BASE_H
#include "base.h"
#endif /* BASE_H */

/*
 * nssItem_Create
 *
 * -- fgmr comments --
 *
 * The error may be one of the following values:
 *  NSS_ERROR_INVALID_ARENA
 *  NSS_ERROR_NO_MEMORY
 *  NSS_ERROR_ARENA_MARKED_BY_ANOTHER_THREAD
 *  NSS_ERROR_INVALID_POINTER
 *  
 * Return value:
 *  A pointer to an NSSItem upon success
 *  NULL upon failure
 */

NSS_IMPLEMENT NSSItem *
nssItem_Create
(
  NSSArena *arenaOpt,
  NSSItem *rvOpt,
  PRUint32 length,
  const void *data
)
{
  NSSItem *rv = (NSSItem *)NULL;

#ifdef DEBUG
  if( (NSSArena *)NULL != arenaOpt ) {
    if( PR_SUCCESS != nssArena_verifyPointer(arenaOpt) ) {
      return (NSSItem *)NULL;
    }
  }

  if( (const void *)NULL == data ) {
    if( length > 0 ) {
      nss_SetError(NSS_ERROR_INVALID_POINTER);
      return (NSSItem *)NULL;
    }
  }
#endif /* DEBUG */

  if( (NSSItem *)NULL == rvOpt ) {
    rv = (NSSItem *)nss_ZNEW(arenaOpt, NSSItem);
    if( (NSSItem *)NULL == rv ) {
      goto loser;
    }
  } else {
    rv = rvOpt;
  }

  rv->size = length;
  rv->data = nss_ZAlloc(arenaOpt, length);
  if( (void *)NULL == rv->data ) {
    goto loser;
  }

  if( length > 0 ) {
    (void)nsslibc_memcpy(rv->data, data, length);
  }

  return rv;

 loser:
  if( rv != rvOpt ) {
    nss_ZFreeIf(rv);
  }

  return (NSSItem *)NULL;
}

NSS_IMPLEMENT void
nssItem_Destroy
(
  NSSItem *item
)
{
  nss_ClearErrorStack();

  nss_ZFreeIf(item->data);
  nss_ZFreeIf(item);

}

/*
 * nssItem_Duplicate
 *
 * -- fgmr comments --
 *
 * The error may be one of the following values:
 *  NSS_ERROR_INVALID_ARENA
 *  NSS_ERROR_NO_MEMORY
 *  NSS_ERROR_ARENA_MARKED_BY_ANOTHER_THREAD
 *  NSS_ERROR_INVALID_ITEM
 *  
 * Return value:
 *  A pointer to an NSSItem upon success
 *  NULL upon failure
 */

NSS_IMPLEMENT NSSItem *
nssItem_Duplicate
(
  NSSItem *obj,
  NSSArena *arenaOpt,
  NSSItem *rvOpt
)
{
#ifdef DEBUG
  if( (NSSArena *)NULL != arenaOpt ) {
    if( PR_SUCCESS != nssArena_verifyPointer(arenaOpt) ) {
      return (NSSItem *)NULL;
    }
  }

  if( (NSSItem *)NULL == obj ) {
    nss_SetError(NSS_ERROR_INVALID_ITEM);
    return (NSSItem *)NULL;
  }
#endif /* DEBUG */

  return nssItem_Create(arenaOpt, rvOpt, obj->size, obj->data);
}

#ifdef DEBUG
/*
 * nssItem_verifyPointer
 *
 * -- fgmr comments --
 *
 * The error may be one of the following values:
 *  NSS_ERROR_INVALID_ITEM
 *
 * Return value:
 *  PR_SUCCESS upon success
 *  PR_FAILURE upon failure
 */

NSS_IMPLEMENT PRStatus
nssItem_verifyPointer
(
  const NSSItem *item
)
{
  if( ((const NSSItem *)NULL == item) ||
      (((void *)NULL == item->data) && (item->size > 0)) ) {
    nss_SetError(NSS_ERROR_INVALID_ITEM);
    return PR_FAILURE;
  }

  return PR_SUCCESS;
}
#endif /* DEBUG */

/*
 * nssItem_Equal
 *
 * -- fgmr comments --
 *
 * The error may be one of the following values:
 *  NSS_ERROR_INVALID_ITEM
 *
 * Return value:
 *  PR_TRUE if the items are identical
 *  PR_FALSE if they aren't
 *  PR_FALSE upon error
 */

NSS_IMPLEMENT PRBool
nssItem_Equal
(
  const NSSItem *one,
  const NSSItem *two,
  PRStatus *statusOpt
)
{
  if( (PRStatus *)NULL != statusOpt ) {
    *statusOpt = PR_SUCCESS;
  }

  if( ((const NSSItem *)NULL == one) && ((const NSSItem *)NULL == two) ) {
    return PR_TRUE;
  }

  if( ((const NSSItem *)NULL == one) || ((const NSSItem *)NULL == two) ) {
    return PR_FALSE;
  }

  if( one->size != two->size ) {
    return PR_FALSE;
  }

  return nsslibc_memequal(one->data, two->data, one->size, statusOpt);
}
