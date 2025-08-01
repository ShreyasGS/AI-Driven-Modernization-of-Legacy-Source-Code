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
static const char CKDBM_CVS_ID[] = "@(#) $RCSfile: ckdbm.h,v $ $Revision: 1.1.146.1 $ $Date: 2002/04/10 03:26:54 $ $Name: MOZILLA_1_0_RELEASE $";
#endif /* DEBUG */

#ifndef CKDBM_H
#define CKDBM_H

#include "nssckmdt.h"
#include "nssckfw.h"

/*
 * I'm including this for access to the arena functions.
 * Looks like we should publish that API.
 */
#ifndef BASE_H
#include "base.h"
#endif /* BASE_H */

/*
 * This is where the Netscape extensions live, at least for now.
 */
#ifndef CKT_H
#include "ckt.h"
#endif /* CKT_H */

#include "mcom_db.h"

NSS_EXTERN_DATA NSSCKMDInstance nss_dbm_mdInstance;

typedef struct nss_dbm_db_struct nss_dbm_db_t;
struct nss_dbm_db_struct {
  DB *db;
  NSSCKFWMutex *crustylock;
};

typedef struct nss_dbm_dbt_struct nss_dbm_dbt_t;
struct nss_dbm_dbt_struct {
  DBT dbt;
  nss_dbm_db_t *my_db;
};

typedef struct nss_dbm_instance_struct nss_dbm_instance_t;
struct nss_dbm_instance_struct {
  NSSArena *arena;
  CK_ULONG nSlots;
  char **filenames;
  int *flags; /* e.g. O_RDONLY, O_RDWR */
};

typedef struct nss_dbm_slot_struct nss_dbm_slot_t;
struct nss_dbm_slot_struct {
  nss_dbm_instance_t *instance;
  char *filename;
  int flags;
  nss_dbm_db_t *token_db;
};

typedef struct nss_dbm_token_struct nss_dbm_token_t;
struct nss_dbm_token_struct {
  NSSArena *arena;
  nss_dbm_slot_t *slot;
  nss_dbm_db_t *session_db;
  NSSUTF8 *label;
};

struct nss_dbm_dbt_node {
  struct nss_dbm_dbt_node *next;
  nss_dbm_dbt_t *dbt;
};

typedef struct nss_dbm_session_struct nss_dbm_session_t;
struct nss_dbm_session_struct {
  NSSArena *arena;
  nss_dbm_token_t *token;
  CK_ULONG deviceError;
  struct nss_dbm_dbt_node *session_objects;
  NSSCKFWMutex *list_lock;
};

typedef struct nss_dbm_object_struct nss_dbm_object_t;
struct nss_dbm_object_struct {
  NSSArena *arena; /* token or session */
  nss_dbm_dbt_t *handle;
};

typedef struct nss_dbm_find_struct nss_dbm_find_t;
struct nss_dbm_find_struct {
  NSSArena *arena;
  struct nss_dbm_dbt_node *found;
  NSSCKFWMutex *list_lock;
};

NSS_EXTERN NSSCKMDSlot *
nss_dbm_mdSlot_factory
(
  nss_dbm_instance_t *instance,
  char *filename,
  int flags,
  CK_RV *pError
);

NSS_EXTERN NSSCKMDToken *
nss_dbm_mdToken_factory
(
  nss_dbm_slot_t *slot,
  CK_RV *pError
);

NSS_EXTERN NSSCKMDSession *
nss_dbm_mdSession_factory
(
  nss_dbm_token_t *token,
  NSSCKFWSession *fwSession,
  NSSCKFWInstance *fwInstance,
  CK_BBOOL rw,
  CK_RV *pError
);

NSS_EXTERN NSSCKMDObject *
nss_dbm_mdObject_factory
(
  nss_dbm_object_t *object,
  CK_RV *pError
);

NSS_EXTERN NSSCKMDFindObjects *
nss_dbm_mdFindObjects_factory
(
  nss_dbm_find_t *find,
  CK_RV *pError
);

NSS_EXTERN nss_dbm_db_t *
nss_dbm_db_open
(
  NSSArena *arena,
  NSSCKFWInstance *fwInstance,
  char *filename,
  int flags,
  CK_RV *pError
);

NSS_EXTERN void
nss_dbm_db_close
(
  nss_dbm_db_t *db
);

NSS_EXTERN CK_VERSION
nss_dbm_db_get_format_version
(
  nss_dbm_db_t *db
);

NSS_EXTERN CK_RV
nss_dbm_db_set_label
(
  nss_dbm_db_t *db,
  NSSUTF8 *label
);

NSS_EXTERN NSSUTF8 *
nss_dbm_db_get_label
(
  nss_dbm_db_t *db,
  NSSArena *arena,
  CK_RV *pError
);

NSS_EXTERN CK_RV
nss_dbm_db_delete_object
(
  nss_dbm_dbt_t *dbt
);

NSS_EXTERN nss_dbm_dbt_t *
nss_dbm_db_create_object
(
  NSSArena *arena,
  nss_dbm_db_t *db,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulAttributeCount,
  CK_RV *pError,
  CK_ULONG *pdbrv
);

NSS_EXTERN CK_RV
nss_dbm_db_find_objects
(
  nss_dbm_find_t *find,
  nss_dbm_db_t *db,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulAttributeCount,
  CK_ULONG *pdbrv
);

NSS_EXTERN CK_BBOOL
nss_dbm_db_object_still_exists
(
  nss_dbm_dbt_t *dbt
);

NSS_EXTERN CK_ULONG
nss_dbm_db_get_object_attribute_count
(
  nss_dbm_dbt_t *dbt,
  CK_RV *pError,
  CK_ULONG *pdbrv
);

NSS_EXTERN CK_RV
nss_dbm_db_get_object_attribute_types
(
  nss_dbm_dbt_t *dbt,
  CK_ATTRIBUTE_TYPE_PTR typeArray,
  CK_ULONG ulCount,
  CK_ULONG *pdbrv
);

NSS_EXTERN CK_ULONG
nss_dbm_db_get_object_attribute_size
(
  nss_dbm_dbt_t *dbt,
  CK_ATTRIBUTE_TYPE type,
  CK_RV *pError,
  CK_ULONG *pdbrv
);

NSS_EXTERN NSSItem *
nss_dbm_db_get_object_attribute
(
  nss_dbm_dbt_t *dbt,
  NSSArena *arena,
  CK_ATTRIBUTE_TYPE type,
  CK_RV *pError,
  CK_ULONG *pdbrv
);

NSS_EXTERN CK_RV
nss_dbm_db_set_object_attribute
(
  nss_dbm_dbt_t *dbt,
  CK_ATTRIBUTE_TYPE type,
  NSSItem *value,
  CK_ULONG *pdbrv
);

#endif /* CKDBM_H */
