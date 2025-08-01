/*
 * NSS utility functions
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
 *
 # $Id: nssinit.c,v 1.42.6.1 2002/04/10 03:27:56 cltbld%netscape.com Exp $
 */

#include <ctype.h>
#include "seccomon.h"
#include "prinit.h"
#include "prprf.h"
#include "prmem.h"
#include "cert.h"
#include "key.h"
#include "ssl.h"
#include "sslproto.h"
#include "secmod.h"
#include "secoid.h"
#include "nss.h"
#include "secrng.h"
#include "pk11func.h"

#include "pki3hack.h"

#define NSS_MAX_FLAG_SIZE  sizeof("readOnly")+sizeof("noCertDB")+ \
	sizeof("noModDB")+sizeof("forceOpen")+sizeof("passwordRequired")
#define NSS_DEFAULT_MOD_NAME "NSS Internal Module"
#ifdef macintosh
#define SECMOD_DB "Security Modules"
#else
#define SECMOD_DB "secmod.db"
#endif

static char *
nss_makeFlags(PRBool readOnly, PRBool noCertDB, 
		PRBool noModDB, PRBool forceOpen, PRBool passwordRequired) 
{
    char *flags = (char *)PORT_Alloc(NSS_MAX_FLAG_SIZE);
    PRBool first = PR_TRUE;

    PORT_Memset(flags,0,NSS_MAX_FLAG_SIZE);
    if (readOnly) {
        PORT_Strcat(flags,"readOnly");
        first = PR_FALSE;
    }
    if (noCertDB) {
        if (!first) PORT_Strcat(flags,",");
        PORT_Strcat(flags,"noCertDB");
        first = PR_FALSE;
    }
    if (noModDB) {
        if (!first) PORT_Strcat(flags,",");
        PORT_Strcat(flags,"noModDB");
        first = PR_FALSE;
    }
    if (forceOpen) {
        if (!first) PORT_Strcat(flags,",");
        PORT_Strcat(flags,"forceOpen");
        first = PR_FALSE;
    }
    if (passwordRequired) {
        if (!first) PORT_Strcat(flags,",");
        PORT_Strcat(flags,"passwordRequired");
        first = PR_FALSE;
    }
    return flags;
}

/*
 * statics to remember the PKCS11_ConfigurePKCS11()
 * info.
 */
static char * pk11_config_strings = NULL;
static char * pk11_config_name = NULL;
static PRBool pk11_password_required = PR_FALSE;

/*
 * this is a legacy configuration function which used to be part of
 * the PKCS #11 internal token.
 */
void
PK11_ConfigurePKCS11(const char *man, const char *libdes, const char *tokdes,
	const char *ptokdes, const char *slotdes, const char *pslotdes, 
	const char *fslotdes, const char *fpslotdes, int minPwd, int pwRequired)
{
   char *strings = NULL;
   char *newStrings;

   /* make sure the internationalization was done correctly... */
   strings = PR_smprintf("");
   if (strings == NULL) return;

    if (man) {
        newStrings = PR_smprintf("%s manufacturerID='%s'",strings,man);
	PR_smprintf_free(strings);
	strings = newStrings;
    }
   if (strings == NULL) return;

    if (libdes) {
        newStrings = PR_smprintf("%s libraryDescription='%s'",strings,libdes);
	PR_smprintf_free(strings);
	strings = newStrings;
	if (pk11_config_name != NULL) {
	    PORT_Free(pk11_config_name);
	}
	pk11_config_name = PORT_Strdup(libdes);
    }
   if (strings == NULL) return;

    if (tokdes) {
        newStrings = PR_smprintf("%s cryptoTokenDescription='%s'",strings,
								tokdes);
	PR_smprintf_free(strings);
	strings = newStrings;
    }
   if (strings == NULL) return;

    if (ptokdes) {
        newStrings = PR_smprintf("%s dbTokenDescription='%s'",strings,ptokdes);
	PR_smprintf_free(strings);
	strings = newStrings;
    }
   if (strings == NULL) return;

    if (slotdes) {
        newStrings = PR_smprintf("%s cryptoSlotDescription='%s'",strings,
								slotdes);
	PR_smprintf_free(strings);
	strings = newStrings;
    }
   if (strings == NULL) return;

    if (pslotdes) {
        newStrings = PR_smprintf("%s dbSlotDescription='%s'",strings,pslotdes);
	PR_smprintf_free(strings);
	strings = newStrings;
    }
   if (strings == NULL) return;

    if (fslotdes) {
        newStrings = PR_smprintf("%s FIPSSlotDescription='%s'",
							strings,fslotdes);
	PR_smprintf_free(strings);
	strings = newStrings;
    }
   if (strings == NULL) return;

    if (fpslotdes) {
        newStrings = PR_smprintf("%s FIPSTokenDescription='%s'",
							strings,fpslotdes);
	PR_smprintf_free(strings);
	strings = newStrings;
    }
   if (strings == NULL) return;

    newStrings = PR_smprintf("%s minPS=%d", strings, minPwd);
    PR_smprintf_free(strings);
    strings = newStrings;
   if (strings == NULL) return;

    if (pk11_config_strings != NULL) {
	PR_smprintf_free(pk11_config_strings);
    }
    pk11_config_strings = strings;
    pk11_password_required = pwRequired;

    return;
}

static char *
nss_addEscape(const char *string, char quote)
{
    char *newString = 0;
    int escapes = 0, size = 0;
    const char *src;
    char *dest;

    for (src=string; *src ; src++) {
	if ((*src == quote) || (*src == '\\')) escapes++;
	size++;
    }

    newString = PORT_ZAlloc(escapes+size+1); 
    if (newString == NULL) {
	return NULL;
    }

    for (src=string, dest=newString; *src; src++,dest++) {
	if ((*src == '\\') || (*src == quote)) {
	    *dest++ = '\\';
	}
	*dest = *src;
    }

    return newString;
}

static char *
nss_doubleEscape(const char *string)
{
    char *round1 = NULL;
    char *retValue = NULL;
    if (string == NULL) {
	goto done;
    }
    round1 = nss_addEscape(string,'\'');
    if (round1) {
	retValue = nss_addEscape(round1,'"');
	PORT_Free(round1);
    }

done:
    if (retValue == NULL) {
	retValue = PORT_Strdup("");
    }
    return retValue;
}


#ifndef XP_MAC
/*
 * The following code is an attempt to automagically find the external root
 * module. NOTE: This code should be checked out on the MAC! There must be
 * some cross platform support out there to help out with this?
 * Note: Keep the #if-defined chunks in order. HPUX must select before UNIX.
 */

static const char *dllname =
#if defined(XP_WIN32) || defined(XP_OS2)
	"nssckbi.dll";
#elif defined(HPUX)
	"libnssckbi.sl";
#elif defined(DARWIN)
	"libnssckbi.dylib";
#elif defined(XP_UNIX) || defined(XP_BEOS)
	"libnssckbi.so";
#elif defined(XP_MAC)
	"NSS Builtin Root Certs";
#else
	#error "Uh! Oh! I don't know about this platform."
#endif

/* Should we have platform ifdefs here??? */
#define FILE_SEP '/'

static void
nss_FindExternalRoot(const char *dbpath)
{
	char *path;
	int len, path_len;

	path_len = PORT_Strlen(dbpath);
	len = path_len + PORT_Strlen(dllname) + 2; /* FILE_SEP + NULL */
	
	path = PORT_Alloc(len);
	if (path == NULL) return;

	/* back up to the top of the directory */
	PORT_Memcpy(path,dbpath,path_len);
	if (path[path_len-1] != FILE_SEP) {
	    path[path_len++] = FILE_SEP;
	}
	PORT_Strcpy(&path[path_len],dllname);
	(void) SECMOD_AddNewModule("Root Certs",path, 0, 0);
	PORT_Free(path);
	return;
}
#endif

/*
 * OK there are now lots of options here, lets go through them all:
 *
 * configdir - base directory where all the cert, key, and module datbases live.
 * certPrefix - prefix added to the beginning of the cert database example: "
 * 			"https-server1-"
 * keyPrefix - prefix added to the beginning of the key database example: "
 * 			"https-server1-"
 * secmodName - name of the security module database (usually "secmod.db").
 * readOnly - Boolean: true if the databases are to be openned read only.
 * nocertdb - Don't open the cert DB and key DB's, just initialize the 
 *			Volatile certdb.
 * nomoddb - Don't open the security module DB, just initialize the 
 *			PKCS #11 module.
 * forceOpen - Continue to force initializations even if the databases cannot
 * 			be opened.
 */

static PRBool nss_IsInitted = PR_FALSE;

static SECStatus
nss_Init(const char *configdir, const char *certPrefix, const char *keyPrefix,
		 const char *secmodName, PRBool readOnly, PRBool noCertDB, 
			PRBool noModDB, PRBool forceOpen, PRBool noRootInit)
{
    char *moduleSpec = NULL;
    char *flags = NULL;
    SECStatus rv = SECFailure;
    char *lconfigdir = NULL;
    char *lcertPrefix = NULL;
    char *lkeyPrefix = NULL;
    char *lsecmodName = NULL;

    if (nss_IsInitted) {
	return SECSuccess;
    }

    flags = nss_makeFlags(readOnly,noCertDB,noModDB,forceOpen,
						pk11_password_required);
    if (flags == NULL) return rv;

    /*
     * configdir is double nested, and Windows uses the same character
     * for file seps as we use for escapes! (sigh).
     */
    lconfigdir = nss_doubleEscape(configdir);
    if (lconfigdir == NULL) {
	goto loser;
    }
    lcertPrefix = nss_doubleEscape(certPrefix);
    if (lcertPrefix == NULL) {
	goto loser;
    }
    lkeyPrefix = nss_doubleEscape(keyPrefix);
    if (lkeyPrefix == NULL) {
	goto loser;
    }
    lsecmodName = nss_doubleEscape(secmodName);
    if (lsecmodName == NULL) {
	goto loser;
    }

    moduleSpec = PR_smprintf("name=\"%s\" parameters=\"configdir='%s' certPrefix='%s' keyPrefix='%s' secmod='%s' flags=%s %s\" NSS=\"flags=internal,moduleDB,moduleDBOnly,critical\"",
		pk11_config_name ? pk11_config_name : NSS_DEFAULT_MOD_NAME,
		lconfigdir,lcertPrefix,lkeyPrefix,lsecmodName,flags,
		pk11_config_strings ? pk11_config_strings : "");

loser:
    PORT_Free(flags);
    if (lconfigdir) PORT_Free(lconfigdir);
    if (lcertPrefix) PORT_Free(lcertPrefix);
    if (lkeyPrefix) PORT_Free(lkeyPrefix);
    if (lsecmodName) PORT_Free(lsecmodName);

    if (moduleSpec) {
	SECMODModule *module = SECMOD_LoadModule(moduleSpec,NULL,PR_TRUE);
	PR_smprintf_free(moduleSpec);
	if (module) {
	    if (module->loaded) rv=SECSuccess;
	    SECMOD_DestroyModule(module);
	}
    }

    if (rv == SECSuccess) {
	/* can this function fail?? */
	STAN_LoadDefaultNSS3TrustDomain();
	CERT_SetDefaultCertDB((CERTCertDBHandle *)
				STAN_GetDefaultTrustDomain());
#ifndef XP_MAC
	/* only servers need this. We currently do not have a mac server */
	if ((!noModDB) && (!noCertDB) && (!noRootInit)) {
	    if (!SECMOD_HasRootCerts()) {
		nss_FindExternalRoot(configdir);
	    }
	}
#endif
	nss_IsInitted = PR_TRUE;
    }
    return rv;
}


SECStatus
NSS_Init(const char *configdir)
{
    return nss_Init(configdir, "", "", SECMOD_DB, PR_TRUE, 
		PR_FALSE, PR_FALSE, PR_FALSE, PR_TRUE);
}

SECStatus
NSS_InitReadWrite(const char *configdir)
{
    return nss_Init(configdir, "", "", SECMOD_DB, PR_FALSE, 
		PR_FALSE, PR_FALSE, PR_FALSE, PR_FALSE);
}

/*
 * OK there are now lots of options here, lets go through them all:
 *
 * configdir - base directory where all the cert, key, and module datbases live.
 * certPrefix - prefix added to the beginning of the cert database example: "
 * 			"https-server1-"
 * keyPrefix - prefix added to the beginning of the key database example: "
 * 			"https-server1-"
 * secmodName - name of the security module database (usually "secmod.db").
 * flags - change the open options of NSS_Initialize as follows:
 * 	NSS_INIT_READONLY - Open the databases read only.
 * 	NSS_INIT_NOCERTDB - Don't open the cert DB and key DB's, just 
 * 			initialize the volatile certdb.
 * 	NSS_INIT_NOMODDB  - Don't open the security module DB, just 
 *			initialize the 	PKCS #11 module.
 *      NSS_INIT_FORCEOPEN - Continue to force initializations even if the 
 * 			databases cannot be opened.
 */
SECStatus
NSS_Initialize(const char *configdir, const char *certPrefix, 
	const char *keyPrefix, const char *secmodName, PRUint32 flags)
{
    return nss_Init(configdir, certPrefix, keyPrefix, secmodName, 
	((flags & NSS_INIT_READONLY) == NSS_INIT_READONLY),
	((flags & NSS_INIT_NOCERTDB) == NSS_INIT_NOCERTDB),
	((flags & NSS_INIT_NOMODDB) == NSS_INIT_NOMODDB),
	((flags & NSS_INIT_FORCEOPEN) == NSS_INIT_FORCEOPEN),
	((flags & NSS_INIT_NOROOTINIT) == NSS_INIT_NOROOTINIT));
}

/*
 * initialize NSS without a creating cert db's, key db's, or secmod db's.
 */
SECStatus
NSS_NoDB_Init(const char * configdir)
{
      return nss_Init(configdir?configdir:"","","",SECMOD_DB,
				PR_TRUE,PR_TRUE,PR_TRUE,PR_TRUE,PR_TRUE);
}

void
NSS_Shutdown(void)
{
    SECOID_Shutdown();
    SECMOD_Shutdown();
    STAN_Shutdown();
    nss_IsInitted = PR_FALSE;
}



extern const char __nss_base_rcsid[];
extern const char __nss_base_sccsid[];

PRBool
NSS_VersionCheck(const char *importedVersion)
{
    /*
     * This is the secret handshake algorithm.
     *
     * This release has a simple version compatibility
     * check algorithm.  This release is not backward
     * compatible with previous major releases.  It is
     * not compatible with future major, minor, or
     * patch releases.
     */
    int vmajor = 0, vminor = 0, vpatch = 0;
    const char *ptr = importedVersion;
    volatile char c; /* force a reference that won't get optimized away */

    c = __nss_base_rcsid[0] + __nss_base_sccsid[0]; 

    while (isdigit(*ptr)) {
        vmajor = 10 * vmajor + *ptr - '0';
        ptr++;
    }
    if (*ptr == '.') {
        ptr++;
        while (isdigit(*ptr)) {
            vminor = 10 * vminor + *ptr - '0';
            ptr++;
        }
        if (*ptr == '.') {
            ptr++;
            while (isdigit(*ptr)) {
                vpatch = 10 * vpatch + *ptr - '0';
                ptr++;
            }
        }
    }

    if (vmajor != NSS_VMAJOR) {
        return PR_FALSE;
    }
    if (vmajor == NSS_VMAJOR && vminor > NSS_VMINOR) {
        return PR_FALSE;
    }
    if (vmajor == NSS_VMAJOR && vminor == NSS_VMINOR && vpatch > NSS_VPATCH) {
        return PR_FALSE;
    }
    /* Check dependent libraries */
    if (PR_VersionCheck(PR_VERSION) == PR_FALSE) {
        return PR_FALSE;
    }
    return PR_TRUE;
}


