/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code,
 * released March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *     Sean Su <ssu@netscape.com>
 */

#ifndef _IFUNCNS_H_
#define _IFUNCNS_H_

#include "uninstall.h"

APIRET     FileUncompress(PSZ szFrom, PSZ szTo);
APIRET     FileMove(PSZ szFrom, PSZ szTo);
APIRET     FileCopy(PSZ szFrom, PSZ szTo, BOOL bFailIfExists);
APIRET     FileDelete(PSZ szDestination);
APIRET     DirectoryRemove(PSZ szDestination, BOOL bRemoveSubdirs);
APIRET     CreateDirectoriesAll(char* szPath);
HINI        ParseRootKey(PSZ szRootKey);
PSZ       GetStringRootKey(HINI hkRootKey, PSZ szString, ULONG dwStringSize);
BOOL        SearchForUninstallKeys(char *szStringToMatch);

#endif
