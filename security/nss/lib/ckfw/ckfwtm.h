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

#ifndef CKFWTM_H
#define CKFWTM_H

#ifdef DEBUG
static const char CKFWTM_CVS_ID[] = "@(#) $RCSfile: ckfwtm.h,v $ $Revision: 1.1.162.1 $ $Date: 2002/04/10 03:26:41 $ $Name: MOZILLA_1_0_RELEASE $";
#endif /* DEBUG */

/*
 * ckfwtm.h
 *
 * This file declares the module-private types of the NSS Cryptoki Framework.
 */

#ifndef NSSBASET_H
#include "nssbaset.h"
#endif /* NSSBASET_H */

struct nssCKFWHashStr;
typedef struct nssCKFWHashStr nssCKFWHash;

typedef void (PR_CALLBACK *nssCKFWHashIterator)(const void *key, void *value, void *closure);

#endif /* CKFWTM_H */
