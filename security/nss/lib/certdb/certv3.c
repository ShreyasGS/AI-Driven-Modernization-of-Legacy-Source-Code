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
 * Code for dealing with X509.V3 extensions.
 *
 * $Id: certv3.c,v 1.1.162.1 2002/04/10 03:26:33 cltbld%netscape.com Exp $
 */

#include "cert.h"
#include "secitem.h"
#include "secoid.h"
#include "secder.h"
#include "secasn1.h"
#include "certxutl.h"
#include "secerr.h"

SECStatus
CERT_FindCertExtensionByOID(CERTCertificate *cert, SECItem *oid,
			    SECItem *value)
{
    return (cert_FindExtensionByOID (cert->extensions, oid, value));
}
    

SECStatus
CERT_FindCertExtension(CERTCertificate *cert, int tag, SECItem *value)
{
    return (cert_FindExtension (cert->extensions, tag, value));
}

static void
SetExts(void *object, CERTCertExtension **exts)
{
    CERTCertificate *cert = (CERTCertificate *)object;

    cert->extensions = exts;
    DER_SetUInteger (cert->arena, &(cert->version), SEC_CERTIFICATE_VERSION_3);
}

void *
CERT_StartCertExtensions(CERTCertificate *cert)
{
    return (cert_StartExtensions ((void *)cert, cert->arena, SetExts));
}

/* find the given extension in the certificate of the Issuer of 'cert' */
SECStatus
CERT_FindIssuerCertExtension(CERTCertificate *cert, int tag, SECItem *value)
{
    CERTCertificate *issuercert;
    SECStatus rv;

    issuercert = CERT_FindCertByName(cert->dbhandle, &cert->derIssuer);
    if ( issuercert ) {
	rv = cert_FindExtension(issuercert->extensions, tag, value);
	CERT_DestroyCertificate(issuercert);
    } else {
	rv = SECFailure;
    }
    
    return(rv);
}

/* find a URL extension in the cert or its CA
 * apply the base URL string if it exists
 */
char *
CERT_FindCertURLExtension(CERTCertificate *cert, int tag, int catag)
{
    SECStatus rv;
    SECItem urlitem;
    SECItem baseitem;
    SECItem urlstringitem = {siBuffer,0};
    SECItem basestringitem = {siBuffer,0};
    PRArenaPool *arena = NULL;
    PRBool hasbase;
    char *urlstring;
    char *str;
    int len;
    int i;
    
    urlstring = NULL;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( ! arena ) {
	goto loser;
    }
    
    hasbase = PR_FALSE;
    urlitem.data = NULL;
    baseitem.data = NULL;
    
    rv = cert_FindExtension(cert->extensions, tag, &urlitem);
    if ( rv == SECSuccess ) {
	rv = cert_FindExtension(cert->extensions, SEC_OID_NS_CERT_EXT_BASE_URL,
				   &baseitem);
	if ( rv == SECSuccess ) {
	    hasbase = PR_TRUE;
	}
	
    } else if ( catag ) {
	/* if the cert doesn't have the extensions, see if the issuer does */
	rv = CERT_FindIssuerCertExtension(cert, catag, &urlitem);
	if ( rv != SECSuccess ) {
	    goto loser;
	}	    
	rv = CERT_FindIssuerCertExtension(cert, SEC_OID_NS_CERT_EXT_BASE_URL,
					 &baseitem);
	if ( rv == SECSuccess ) {
	    hasbase = PR_TRUE;
	}
    } else {
	goto loser;
    }

    rv = SEC_ASN1DecodeItem(arena, &urlstringitem, SEC_IA5StringTemplate, 
			    &urlitem);

    if ( rv != SECSuccess ) {
	goto loser;
    }
    if ( hasbase ) {
	rv = SEC_ASN1DecodeItem(arena, &basestringitem, SEC_IA5StringTemplate,
				&baseitem);

	if ( rv != SECSuccess ) {
	    goto loser;
	}
    }
    
    len = urlstringitem.len + ( hasbase ? basestringitem.len : 0 ) + 1;
    
    str = urlstring = (char *)PORT_Alloc(len);
    if ( urlstring == NULL ) {
	goto loser;
    }
    
    /* copy the URL base first */
    if ( hasbase ) {

	/* if the urlstring has a : in it, then we assume it is an absolute
	 * URL, and will not get the base string pre-pended
	 */
	for ( i = 0; i < urlstringitem.len; i++ ) {
	    if ( urlstringitem.data[i] == ':' ) {
		goto nobase;
	    }
	}
	
	PORT_Memcpy(str, basestringitem.data, basestringitem.len);
	str += basestringitem.len;
	
    }

nobase:
    /* copy the rest (or all) of the URL */
    PORT_Memcpy(str, urlstringitem.data, urlstringitem.len);
    str += urlstringitem.len;
    
    *str = '\0';
    goto done;
    
loser:
    if ( urlstring ) {
	PORT_Free(urlstring);
    }
    
    urlstring = NULL;
done:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    if ( baseitem.data ) {
	PORT_Free(baseitem.data);
    }
    if ( urlitem.data ) {
	PORT_Free(urlitem.data);
    }

    return(urlstring);
}

/*
 * get the value of the Netscape Certificate Type Extension
 */
SECStatus
CERT_FindNSCertTypeExtension(CERTCertificate *cert, SECItem *retItem)
{

    return (CERT_FindBitStringExtension
	    (cert->extensions, SEC_OID_NS_CERT_EXT_CERT_TYPE, retItem));    
}


/*
 * get the value of a string type extension
 */
char *
CERT_FindNSStringExtension(CERTCertificate *cert, int oidtag)
{
    SECItem wrapperItem, tmpItem = {siBuffer,0};
    SECStatus rv;
    PRArenaPool *arena = NULL;
    char *retstring = NULL;
    
    wrapperItem.data = NULL;
    tmpItem.data = NULL;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    if ( ! arena ) {
	goto loser;
    }
    
    rv = cert_FindExtension(cert->extensions, oidtag,
			       &wrapperItem);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    rv = SEC_ASN1DecodeItem(arena, &tmpItem, SEC_IA5StringTemplate, 
			    &wrapperItem);

    if ( rv != SECSuccess ) {
	goto loser;
    }

    retstring = (char *)PORT_Alloc(tmpItem.len + 1 );
    if ( retstring == NULL ) {
	goto loser;
    }
    
    PORT_Memcpy(retstring, tmpItem.data, tmpItem.len);
    retstring[tmpItem.len] = '\0';

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    if ( wrapperItem.data ) {
	PORT_Free(wrapperItem.data);
    }

    return(retstring);
}

/*
 * get the value of the X.509 v3 Key Usage Extension
 */
SECStatus
CERT_FindKeyUsageExtension(CERTCertificate *cert, SECItem *retItem)
{

    return (CERT_FindBitStringExtension(cert->extensions,
					SEC_OID_X509_KEY_USAGE, retItem));    
}

/*
 * get the value of the X.509 v3 Key Usage Extension
 */
SECStatus
CERT_FindSubjectKeyIDExten(CERTCertificate *cert, SECItem *retItem)
{

    SECItem encodedValue;
    SECStatus rv;

    encodedValue.data = NULL;
    rv = cert_FindExtension
	 (cert->extensions, SEC_OID_X509_SUBJECT_KEY_ID, &encodedValue);
    if (rv != SECSuccess)
	return (rv);
    rv = SEC_ASN1DecodeItem (NULL, retItem, SEC_OctetStringTemplate,
			     &encodedValue);
    PORT_Free (encodedValue.data);

    return (rv);
}

SECStatus
CERT_FindBasicConstraintExten(CERTCertificate *cert,
			      CERTBasicConstraints *value)
{
    SECItem encodedExtenValue;
    SECStatus rv;

    encodedExtenValue.data = NULL;
    encodedExtenValue.len = 0;

    rv = cert_FindExtension(cert->extensions, SEC_OID_X509_BASIC_CONSTRAINTS,
			    &encodedExtenValue);
    if ( rv != SECSuccess ) {
	return (rv);
    }

    rv = CERT_DecodeBasicConstraintValue (value, &encodedExtenValue);
    
    /* free the raw extension data */
    PORT_Free(encodedExtenValue.data);
    encodedExtenValue.data = NULL;
    
    return(rv);
}

CERTAuthKeyID *
CERT_FindAuthKeyIDExten (PRArenaPool *arena, CERTCertificate *cert)
{
    SECItem encodedExtenValue;
    SECStatus rv;
    CERTAuthKeyID *ret;
    
    encodedExtenValue.data = NULL;
    encodedExtenValue.len = 0;

    rv = cert_FindExtension(cert->extensions, SEC_OID_X509_AUTH_KEY_ID,
			    &encodedExtenValue);
    if ( rv != SECSuccess ) {
	return (NULL);
    }

    ret = CERT_DecodeAuthKeyID (arena, &encodedExtenValue);

    PORT_Free(encodedExtenValue.data);
    encodedExtenValue.data = NULL;
    
    return(ret);
}

SECStatus
CERT_CheckCertUsage(CERTCertificate *cert, unsigned char usage)
{
    PRBool critical;    
    SECItem keyUsage;
    SECStatus rv;

    /* There is no extension, v1 or v2 certificate */
    if (cert->extensions == NULL) {
	return (SECSuccess);
    }
    
    keyUsage.data = NULL;

    do {
	/* if the keyUsage extension exists and is critical, make sure that the
	   CA certificate is used for certificate signing purpose only. If the
	   extension does not exist, we will assum that it can be used for
	   certificate signing purpose.
	*/
	rv = CERT_GetExtenCriticality(cert->extensions,
				      SEC_OID_X509_KEY_USAGE,
				      &critical);
	if (rv == SECFailure) {
	    rv = (PORT_GetError () == SEC_ERROR_EXTENSION_NOT_FOUND) ?
		SECSuccess : SECFailure;
	    break;
	}

	if (critical == PR_FALSE) {
	    rv = SECSuccess;
	    break;
	}

	rv = CERT_FindKeyUsageExtension(cert, &keyUsage);
	if (rv != SECSuccess) {
	    break;
	}	
	if (!(keyUsage.data[0] & usage)) {
	    PORT_SetError (SEC_ERROR_CERT_USAGES_INVALID);
	    rv = SECFailure;
	}
    }while (0);
    PORT_Free (keyUsage.data);
    return (rv);
}
