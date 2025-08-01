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
static const char CVS_ID[] = "@(#) $Source: /cvsroot/mozilla/security/nss/lib/pkix/src/AttributeTypeAndValue/PDuplicate.c,v $ $Revision: 1.1.162.1 $ $Date: 2002/04/10 03:28:36 $ $Name: MOZILLA_1_0_RELEASE $";
#endif /* DEBUG */

#ifndef PKIX_H
#include "pkix.h"
#endif /* PKIX_H */

/*
 * nssPKIXAttributeTypeAndValue_Duplicate
 *
 * 
 *
 * The error may be one of the following values:
 *  NSS_ERROR_INVALID_PKIX_ATAV
 *  NSS_ERROR_INVALID_ARENA
 *  NSS_ERROR_NO_MEMORY
 *
 * Return value:
 *  A valid pointer to an NSSPKIXAttributeTypeAndValue upon success
 *  NULL upon failure
 */

NSS_IMPLEMENT NSSPKIXAttributeTypeAndValue *
nssPKIXAttributeTypeAndValue_Duplicate
(
  NSSPKIXAttributeTypeAndValue *atav,
  NSSArena *arenaOpt
)
{
  NSSArena *arena;
  PRBool arena_allocated = PR_FALSE;
  nssArenaMark *mark = (nssArenaMark *)NULL;
  NSSPKIXAttributeTypeAndValue *rv = (NSSPKIXAttributeTypeAndValue *)NULL;

#ifdef NSSDEBUG
  if( PR_SUCCESS != nssPKIXAttributeTypeAndValue_verifyPointer(atav) ) {
    return PR_FAILURE;
  }

  if( (NSSArena *)NULL != arenaOpt ) {
    if( PR_SUCCESS != nssArena_verifyPointer(arenaOpt) ) {
      return (NSSPKIXAttributeTypeAndValue *)NULL;
    }
  }
#endif /* NSSDEBUG */

  if( (NSSArena *)NULL == arenaOpt ) {
    arena = nssArena_Create();
    if( (NSSArena *)NULL == arena ) {
      goto loser;
    }
    arena_allocated = PR_TRUE;
  } else {
    arena = arenaOpt;
    mark = nssArena_Mark(arena);
    if( (nssArenaMark *)NULL == mark ) {
      goto loser;
    }
  }

  rv = nss_ZNEW(arena, NSSPKIXAttributeTypeAndValue);
  if( (NSSPKIXAttributeTypeAndValue *)NULL == rv ) {
    goto loser;
  }

  rv->arena = arena;
  rv->i_allocated_arena = arena_allocated;

  if( (NSSDER *)NULL != atav->der ) {
    rv->der = nssItem_Duplicate(atav->der, arena, (NSSItem *)NULL);
    if( (NSSDER *)NULL == rv->der ) {
      goto loser; /* actually, this isn't fatal */
    }
  }

  {
    NSSItem src, dst;
    src.size = atav->asn1type.size;
    src.data = atav->asn1type.data;
    if( (NSSItem *)NULL == nssItem_Duplicate(&src, arena, &dst) ) {
      goto loser;
    }
    rv->asn1type.size = dst.size;
    rv->asn1type.data = dst.data;
  }

  {
    NSSItem src, dst;
    src.size = atav->asn1value.size;
    src.data = atav->asn1value.data;
    if( (NSSItem *)NULL == nssItem_Duplicate(&src, arena, &dst) ) {
      goto loser;
    }
    rv->asn1value.size = dst.size;
    rv->asn1value.data = dst.data;
  }

  rv->type = atav->type;

  if( (NSSUTF8 *)NULL != atav->utf8 ) {
    rv->utf8 = nssUTF8_Duplicate(atav->utf8, arena);
    if( (NSSUTF8 *)NULL == rv->utf8 ) {
      goto loser; /* actually, this isn't fatal */
    }
  }

  if( (nssArenaMark *)NULL != mark ) {
    if( PR_SUCCESS != nssArena_Unmark(arena, mark) ) {
      goto loser;
    }
  }

#ifdef DEBUG
  if( PR_SUCCESS != nss_pkix_AttributeTypeAndValue_add_pointer(rv) ) {
    goto loser;
  }
#endif /* DEBUG */

  return rv;

 loser:
  if( (nssArenaMark *)NULL != mark ) {
    (void)nssArena_Release(arena, mark);
  }

  if( PR_TRUE == arena_allocated ) {
    (void)nssArena_Destroy(arena);
  }

  return (NSSPKIXAttributeTypeAndValue *)NULL;
}
