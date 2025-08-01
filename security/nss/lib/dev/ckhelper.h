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

/*
 * ckhelper.h
 *
 * This file contains some helper utilities for interaction with cryptoki.
 */

#ifndef CKHELPER_H
#define CKHELPER_H

#ifdef DEBUG
static const char CKHELPER_CVS_ID[] = "@(#) $RCSfile: ckhelper.h,v $ $Revision: 1.13.18.1 $ $Date: 2002/04/10 03:27:05 $ $Name: MOZILLA_1_0_RELEASE $";
#endif /* DEBUG */

#ifndef NSSCKT_H
#include "nssckt.h"
#endif /* NSSCKT_H */

PR_BEGIN_EXTERN_C

/* Shortcut to cryptoki API functions. */
#define CKAPI(x) \
    ((CK_FUNCTION_LIST_PTR)((x)->epv))

/* Some globals to keep from constantly redeclaring common cryptoki
 * attribute types on the stack.
 */

/* Boolean values */
NSS_EXTERN_DATA const NSSItem g_ck_true;
NSS_EXTERN_DATA const NSSItem g_ck_false;

/* Object classes */
NSS_EXTERN_DATA const NSSItem g_ck_class_cert;
NSS_EXTERN_DATA const NSSItem g_ck_class_pubkey;
NSS_EXTERN_DATA const NSSItem g_ck_class_privkey;

#define NSS_CK_TEMPLATE_START(_template, attr, size)   \
    attr = _template;                                  \
    size = 0;

#define NSS_CK_SET_ATTRIBUTE_ITEM(pattr, kind, item)  \
    (pattr)->type = kind;                             \
    (pattr)->pValue = (CK_VOID_PTR)(item)->data;      \
    (pattr)->ulValueLen = (CK_ULONG)(item)->size;     \
    (pattr)++;

#define NSS_CK_SET_ATTRIBUTE_UTF8(pattr, kind, utf8)  \
    (pattr)->type = kind;                             \
    (pattr)->pValue = (CK_VOID_PTR)utf8;              \
    (pattr)->ulValueLen = (CK_ULONG)nssUTF8_Size(utf8, NULL); \
    if ((pattr)->ulValueLen) ((pattr)->ulValueLen)--; \
    (pattr)++;

#define NSS_CK_SET_ATTRIBUTE_VAR(pattr, kind, var)    \
    (pattr)->type = kind;                             \
    (pattr)->pValue = (CK_VOID_PTR)&var;              \
    (pattr)->ulValueLen = (CK_ULONG)sizeof(var);      \
    (pattr)++;

#define NSS_CK_TEMPLATE_FINISH(_template, attr, size) \
    size = (attr) - (_template);                      \
    PR_ASSERT(size <= sizeof(_template)/sizeof(_template[0]));

/* NSS_CK_ATTRIBUTE_TO_ITEM(attrib, item)
 *
 * Convert a CK_ATTRIBUTE to an NSSItem.
 */
#define NSS_CK_ATTRIBUTE_TO_ITEM(attrib, item)         \
    if ((CK_LONG)(attrib)->ulValueLen > 0) {           \
	(item)->data = (void *)(attrib)->pValue;       \
	(item)->size = (PRUint32)(attrib)->ulValueLen; \
    } else {                                           \
	(item)->data = 0;                              \
	(item)->size = 0;                              \
    }

/* NSS_CK_ATTRIBUTE_TO_UTF8(attrib, str)
 *
 * Convert a CK_ATTRIBUTE to a string.
 */
#define NSS_CK_ATTRIBUTE_TO_UTF8(attrib, str)      \
    str = (NSSUTF8 *)((attrib)->pValue);

/* NSS_CK_ITEM_TO_ATTRIBUTE(item, attrib)
 *
 * Convert an NSSItem to a  CK_ATTRIBUTE.
 */
#define NSS_CK_ITEM_TO_ATTRIBUTE(item, attrib)     \
    (attrib)->pValue = (CK_VOID_PTR)(item)->data;  \
    (attrib)->ulValueLen = (CK_ULONG)(item)->size; \

/* Get an array of attributes from an object. */
NSS_EXTERN PRStatus 
nssCKObject_GetAttributes
(
  CK_OBJECT_HANDLE object,
  CK_ATTRIBUTE_PTR obj_template,
  CK_ULONG count,
  NSSArena *arenaOpt,
  nssSession *session,
  NSSSlot  *slot
);

/* Get a single attribute as an item. */
NSS_EXTERN PRStatus
nssCKObject_GetAttributeItem
(
  CK_OBJECT_HANDLE object,
  CK_ATTRIBUTE_TYPE attribute,
  NSSArena *arenaOpt,
  nssSession *session,
  NSSSlot *slot,
  NSSItem *rvItem
);

NSS_EXTERN PRBool
nssCKObject_IsAttributeTrue
(
  CK_OBJECT_HANDLE object,
  CK_ATTRIBUTE_TYPE attribute,
  nssSession *session,
  NSSSlot *slot,
  PRStatus *rvStatus
);

NSS_EXTERN PRStatus 
nssCKObject_SetAttributes
(
  CK_OBJECT_HANDLE object,
  CK_ATTRIBUTE_PTR obj_template,
  CK_ULONG count,
  nssSession *session,
  NSSSlot  *slot
);

NSS_EXTERN PRBool
nssCKObject_IsTokenObjectTemplate
(
  CK_ATTRIBUTE_PTR objectTemplate, 
  CK_ULONG otsize
);

PR_END_EXTERN_C

#endif /* CKHELPER_H */
