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

#ifndef _SECOIDT_H_
#define _SECOIDT_H_
/*
 * secoidt.h - public data structures for ASN.1 OID functions
 *
 * $Id: secoidt.h,v 1.6.18.1 2002/04/10 03:29:24 cltbld%netscape.com Exp $
 */

#include "secitem.h"

typedef struct SECOidDataStr SECOidData;
typedef struct SECAlgorithmIDStr SECAlgorithmID;

/*
** An X.500 algorithm identifier
*/
struct SECAlgorithmIDStr {
    SECItem algorithm;
    SECItem parameters;
};

/*
 * Misc object IDs - these numbers are for convenient handling.
 * They are mapped into real object IDs
 *
 * NOTE: the order of these entries must mach the array "oids" of SECOidData
 * in util/secoid.c.
 */
typedef enum {
    SEC_OID_UNKNOWN = 0,
    SEC_OID_MD2 = 1,
    SEC_OID_MD4 = 2,
    SEC_OID_MD5 = 3,
    SEC_OID_SHA1 = 4,
    SEC_OID_RC2_CBC = 5,
    SEC_OID_RC4 = 6,
    SEC_OID_DES_EDE3_CBC = 7,
    SEC_OID_RC5_CBC_PAD = 8,
    SEC_OID_DES_ECB = 9,
    SEC_OID_DES_CBC = 10,
    SEC_OID_DES_OFB = 11,
    SEC_OID_DES_CFB = 12,
    SEC_OID_DES_MAC = 13,
    SEC_OID_DES_EDE = 14,
    SEC_OID_ISO_SHA_WITH_RSA_SIGNATURE = 15,
    SEC_OID_PKCS1_RSA_ENCRYPTION = 16,
    SEC_OID_PKCS1_MD2_WITH_RSA_ENCRYPTION = 17,
    SEC_OID_PKCS1_MD4_WITH_RSA_ENCRYPTION = 18,
    SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION = 19,
    SEC_OID_PKCS1_SHA1_WITH_RSA_ENCRYPTION = 20,
    SEC_OID_PKCS5_PBE_WITH_MD2_AND_DES_CBC = 21,
    SEC_OID_PKCS5_PBE_WITH_MD5_AND_DES_CBC = 22,
    SEC_OID_PKCS5_PBE_WITH_SHA1_AND_DES_CBC = 23,
    SEC_OID_PKCS7 = 24,
    SEC_OID_PKCS7_DATA = 25,
    SEC_OID_PKCS7_SIGNED_DATA = 26,
    SEC_OID_PKCS7_ENVELOPED_DATA = 27,
    SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA = 28,
    SEC_OID_PKCS7_DIGESTED_DATA = 29,
    SEC_OID_PKCS7_ENCRYPTED_DATA = 30,
    SEC_OID_PKCS9_EMAIL_ADDRESS = 31,
    SEC_OID_PKCS9_UNSTRUCTURED_NAME = 32,
    SEC_OID_PKCS9_CONTENT_TYPE = 33,
    SEC_OID_PKCS9_MESSAGE_DIGEST = 34,
    SEC_OID_PKCS9_SIGNING_TIME = 35,
    SEC_OID_PKCS9_COUNTER_SIGNATURE = 36,
    SEC_OID_PKCS9_CHALLENGE_PASSWORD = 37,
    SEC_OID_PKCS9_UNSTRUCTURED_ADDRESS = 38,
    SEC_OID_PKCS9_EXTENDED_CERTIFICATE_ATTRIBUTES = 39,
    SEC_OID_PKCS9_SMIME_CAPABILITIES = 40,
    SEC_OID_AVA_COMMON_NAME = 41,
    SEC_OID_AVA_COUNTRY_NAME = 42,
    SEC_OID_AVA_LOCALITY = 43,
    SEC_OID_AVA_STATE_OR_PROVINCE = 44,
    SEC_OID_AVA_ORGANIZATION_NAME = 45,
    SEC_OID_AVA_ORGANIZATIONAL_UNIT_NAME = 46,
    SEC_OID_AVA_DN_QUALIFIER = 47,
    SEC_OID_AVA_DC = 48,

    SEC_OID_NS_TYPE_GIF = 49,
    SEC_OID_NS_TYPE_JPEG = 50,
    SEC_OID_NS_TYPE_URL = 51,
    SEC_OID_NS_TYPE_HTML = 52,
    SEC_OID_NS_TYPE_CERT_SEQUENCE = 53,
    SEC_OID_MISSI_KEA_DSS_OLD = 54,
    SEC_OID_MISSI_DSS_OLD = 55,
    SEC_OID_MISSI_KEA_DSS = 56,
    SEC_OID_MISSI_DSS = 57,
    SEC_OID_MISSI_KEA = 58,
    SEC_OID_MISSI_ALT_KEA = 59,

    /* Netscape private certificate extensions */
    SEC_OID_NS_CERT_EXT_NETSCAPE_OK = 60,
    SEC_OID_NS_CERT_EXT_ISSUER_LOGO = 61,
    SEC_OID_NS_CERT_EXT_SUBJECT_LOGO = 62,
    SEC_OID_NS_CERT_EXT_CERT_TYPE = 63,
    SEC_OID_NS_CERT_EXT_BASE_URL = 64,
    SEC_OID_NS_CERT_EXT_REVOCATION_URL = 65,
    SEC_OID_NS_CERT_EXT_CA_REVOCATION_URL = 66,
    SEC_OID_NS_CERT_EXT_CA_CRL_URL = 67,
    SEC_OID_NS_CERT_EXT_CA_CERT_URL = 68,
    SEC_OID_NS_CERT_EXT_CERT_RENEWAL_URL = 69,
    SEC_OID_NS_CERT_EXT_CA_POLICY_URL = 70,
    SEC_OID_NS_CERT_EXT_HOMEPAGE_URL = 71,
    SEC_OID_NS_CERT_EXT_ENTITY_LOGO = 72,
    SEC_OID_NS_CERT_EXT_USER_PICTURE = 73,
    SEC_OID_NS_CERT_EXT_SSL_SERVER_NAME = 74,
    SEC_OID_NS_CERT_EXT_COMMENT = 75,
    SEC_OID_NS_CERT_EXT_LOST_PASSWORD_URL = 76,
    SEC_OID_NS_CERT_EXT_CERT_RENEWAL_TIME = 77,
    SEC_OID_NS_KEY_USAGE_GOVT_APPROVED = 78,

    /* x.509 v3 Extensions */
    SEC_OID_X509_SUBJECT_DIRECTORY_ATTR = 79,
    SEC_OID_X509_SUBJECT_KEY_ID = 80,
    SEC_OID_X509_KEY_USAGE = 81,
    SEC_OID_X509_PRIVATE_KEY_USAGE_PERIOD = 82,
    SEC_OID_X509_SUBJECT_ALT_NAME = 83,
    SEC_OID_X509_ISSUER_ALT_NAME = 84,
    SEC_OID_X509_BASIC_CONSTRAINTS = 85,
    SEC_OID_X509_NAME_CONSTRAINTS = 86,
    SEC_OID_X509_CRL_DIST_POINTS = 87,
    SEC_OID_X509_CERTIFICATE_POLICIES = 88,
    SEC_OID_X509_POLICY_MAPPINGS = 89,
    SEC_OID_X509_POLICY_CONSTRAINTS = 90,
    SEC_OID_X509_AUTH_KEY_ID = 91,
    SEC_OID_X509_EXT_KEY_USAGE = 92,
    SEC_OID_X509_AUTH_INFO_ACCESS = 93,

    SEC_OID_X509_CRL_NUMBER = 94,
    SEC_OID_X509_REASON_CODE = 95,
    SEC_OID_X509_INVALID_DATE = 96,
    /* End of x.509 v3 Extensions */    

    SEC_OID_X500_RSA_ENCRYPTION = 97,

    /* alg 1485 additions */
    SEC_OID_RFC1274_UID = 98,
    SEC_OID_RFC1274_MAIL = 99,

    /* PKCS 12 additions */
    SEC_OID_PKCS12 = 100,
    SEC_OID_PKCS12_MODE_IDS = 101,
    SEC_OID_PKCS12_ESPVK_IDS = 102,
    SEC_OID_PKCS12_BAG_IDS = 103,
    SEC_OID_PKCS12_CERT_BAG_IDS = 104,
    SEC_OID_PKCS12_OIDS = 105,
    SEC_OID_PKCS12_PBE_IDS = 106,
    SEC_OID_PKCS12_SIGNATURE_IDS = 107,
    SEC_OID_PKCS12_ENVELOPING_IDS = 108,
   /* SEC_OID_PKCS12_OFFLINE_TRANSPORT_MODE,
    SEC_OID_PKCS12_ONLINE_TRANSPORT_MODE, */
    SEC_OID_PKCS12_PKCS8_KEY_SHROUDING = 109,
    SEC_OID_PKCS12_KEY_BAG_ID = 110,
    SEC_OID_PKCS12_CERT_AND_CRL_BAG_ID = 111,
    SEC_OID_PKCS12_SECRET_BAG_ID = 112,
    SEC_OID_PKCS12_X509_CERT_CRL_BAG = 113,
    SEC_OID_PKCS12_SDSI_CERT_BAG = 114,
    SEC_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC4 = 115,
    SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC4 = 116,
    SEC_OID_PKCS12_PBE_WITH_SHA1_AND_TRIPLE_DES_CBC = 117,
    SEC_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC2_CBC = 118,
    SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC = 119,
    SEC_OID_PKCS12_RSA_ENCRYPTION_WITH_128_BIT_RC4 = 120,
    SEC_OID_PKCS12_RSA_ENCRYPTION_WITH_40_BIT_RC4 = 121,
    SEC_OID_PKCS12_RSA_ENCRYPTION_WITH_TRIPLE_DES = 122,
    SEC_OID_PKCS12_RSA_SIGNATURE_WITH_SHA1_DIGEST = 123,
    /* end of PKCS 12 additions */

    /* DSA signatures */
    SEC_OID_ANSIX9_DSA_SIGNATURE = 124,
    SEC_OID_ANSIX9_DSA_SIGNATURE_WITH_SHA1_DIGEST = 125,
    SEC_OID_BOGUS_DSA_SIGNATURE_WITH_SHA1_DIGEST = 126,

    /* Verisign OIDs */
    SEC_OID_VERISIGN_USER_NOTICES = 127,

    /* PKIX OIDs */
    SEC_OID_PKIX_CPS_POINTER_QUALIFIER = 128,
    SEC_OID_PKIX_USER_NOTICE_QUALIFIER = 129,
    SEC_OID_PKIX_OCSP = 130,
    SEC_OID_PKIX_OCSP_BASIC_RESPONSE = 131,
    SEC_OID_PKIX_OCSP_NONCE = 132,
    SEC_OID_PKIX_OCSP_CRL = 133,
    SEC_OID_PKIX_OCSP_RESPONSE = 134,
    SEC_OID_PKIX_OCSP_NO_CHECK = 135,
    SEC_OID_PKIX_OCSP_ARCHIVE_CUTOFF = 136,
    SEC_OID_PKIX_OCSP_SERVICE_LOCATOR = 137,
    SEC_OID_PKIX_REGCTRL_REGTOKEN = 138,
    SEC_OID_PKIX_REGCTRL_AUTHENTICATOR = 139,
    SEC_OID_PKIX_REGCTRL_PKIPUBINFO = 140,
    SEC_OID_PKIX_REGCTRL_PKI_ARCH_OPTIONS = 141,
    SEC_OID_PKIX_REGCTRL_OLD_CERT_ID = 142,
    SEC_OID_PKIX_REGCTRL_PROTOCOL_ENC_KEY = 143,
    SEC_OID_PKIX_REGINFO_UTF8_PAIRS = 144,
    SEC_OID_PKIX_REGINFO_CERT_REQUEST = 145,
    SEC_OID_EXT_KEY_USAGE_SERVER_AUTH = 146,
    SEC_OID_EXT_KEY_USAGE_CLIENT_AUTH = 147,
    SEC_OID_EXT_KEY_USAGE_CODE_SIGN = 148,
    SEC_OID_EXT_KEY_USAGE_EMAIL_PROTECT = 149,
    SEC_OID_EXT_KEY_USAGE_TIME_STAMP = 150,
    SEC_OID_OCSP_RESPONDER = 151,

    /* Netscape Algorithm OIDs */
    SEC_OID_NETSCAPE_SMIME_KEA = 152,

    /* Skipjack OID -- ### mwelch temporary */
    SEC_OID_FORTEZZA_SKIPJACK = 153,

    /* PKCS 12 V2 oids */
    SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_128_BIT_RC4 = 154,
    SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_40_BIT_RC4 = 155,
    SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_3KEY_TRIPLE_DES_CBC = 156,
    SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_2KEY_TRIPLE_DES_CBC = 157,
    SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_128_BIT_RC2_CBC = 158,
    SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC = 159,
    SEC_OID_PKCS12_SAFE_CONTENTS_ID = 160,
    SEC_OID_PKCS12_PKCS8_SHROUDED_KEY_BAG_ID = 161,

    SEC_OID_PKCS12_V1_KEY_BAG_ID = 162,
    SEC_OID_PKCS12_V1_PKCS8_SHROUDED_KEY_BAG_ID = 163,
    SEC_OID_PKCS12_V1_CERT_BAG_ID = 164,
    SEC_OID_PKCS12_V1_CRL_BAG_ID = 165,
    SEC_OID_PKCS12_V1_SECRET_BAG_ID = 166,
    SEC_OID_PKCS12_V1_SAFE_CONTENTS_BAG_ID = 167,
    SEC_OID_PKCS9_X509_CERT = 168,
    SEC_OID_PKCS9_SDSI_CERT = 169,
    SEC_OID_PKCS9_X509_CRL = 170,
    SEC_OID_PKCS9_FRIENDLY_NAME = 171,
    SEC_OID_PKCS9_LOCAL_KEY_ID = 172,
    SEC_OID_PKCS12_KEY_USAGE = 173,

    /*Diffe Helman OIDS */
    SEC_OID_X942_DIFFIE_HELMAN_KEY = 174,

    /* Netscape other name types */
    SEC_OID_NETSCAPE_NICKNAME = 175,

    /* Cert Server OIDS */
    SEC_OID_NETSCAPE_RECOVERY_REQUEST = 176,

    /* New PSM certificate management OIDs */
    SEC_OID_CERT_RENEWAL_LOCATOR = 177,
    SEC_OID_NS_CERT_EXT_SCOPE_OF_USE = 178,
    
    /* CMS (RFC2630) OIDs */
    SEC_OID_CMS_EPHEMERAL_STATIC_DIFFIE_HELLMAN = 179,
    SEC_OID_CMS_3DES_KEY_WRAP = 180,
    SEC_OID_CMS_RC2_KEY_WRAP = 181,

    /* SMIME attributes */
    SEC_OID_SMIME_ENCRYPTION_KEY_PREFERENCE = 182,

    /* AES OIDs */
    SEC_OID_AES_128_ECB 	= 183,
    SEC_OID_AES_128_CBC 	= 184,
    SEC_OID_AES_192_ECB 	= 185,
    SEC_OID_AES_192_CBC 	= 186,
    SEC_OID_AES_256_ECB 	= 187,
    SEC_OID_AES_256_CBC 	= 188,

    SEC_OID_SDN702_DSA_SIGNATURE = 189,

    SEC_OID_TOTAL
} SECOidTag;

/* fake OID for DSS sign/verify */
#define SEC_OID_SHA SEC_OID_MISS_DSS

typedef enum {
    INVALID_CERT_EXTENSION = 0,
    UNSUPPORTED_CERT_EXTENSION = 1,
    SUPPORTED_CERT_EXTENSION = 2
} SECSupportExtenTag;

struct SECOidDataStr {
    SECItem            oid;
    SECOidTag          offset;
    const char *       desc;
    unsigned long      mechanism;
    SECSupportExtenTag supportedExtension;	
    				/* only used for x.509 v3 extensions, so
				   that we can print the names of those
				   extensions that we don't even support */
};

#endif /* _SECOIDT_H_ */
