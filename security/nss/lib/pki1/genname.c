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
static const char CVS_ID[] = "@(#) $RCSfile: genname.c,v $ $Revision: 1.1.162.1 $ $Date: 2002/04/10 03:28:17 $ $Name: MOZILLA_1_0_RELEASE $";
#endif /* DEBUG */

/*
 * genname.c
 *
 * This file contains the implementation of the PKIX part-1 object
 * GeneralName.
 */

#ifndef NSSBASE_H
#include "nssbase.h"
#endif /* NSSBASE_H */

#ifndef ASN1_H
#include "asn1.h"
#endif /* ASN1_H */

#ifndef PKI1_H
#include "pki1.h"
#endif /* PKI1_H */

/*
 * GeneralName
 *
 * From draft-ietf-pkix-ipki-part1-10:
 *
 *  GeneralName ::= CHOICE {
 *          otherName                   [0] INSTANCE OF OTHER-NAME,
 *          rfc822Name                  [1] IA5String,
 *          dNSName                     [2] IA5String,
 *          x400Address                 [3] ORAddress,
 *          directoryName               [4] Name,
 *          ediPartyName                [5] EDIPartyName,
 *          uniformResourceIdentifier   [6] IA5String,
 *          iPAddress                   [7] OCTET STRING,
 *          registeredID                [8] OBJECT IDENTIFIER }
 *  
 *  OTHER-NAME ::= TYPE-IDENTIFIER
 *  
 *  EDIPartyName ::= SEQUENCE {
 *          nameAssigner        [0] DirectoryString {ub-name} OPTIONAL,
 *          partyName           [1] DirectoryString {ub-name} }
 *
 */

struct nssGeneralNameStr {
  NSSGeneralNameChoice choice;
  union {
    /* OTHER-NAME otherName */
    NSSUTF8 *rfc822Name;
    NSSUTF8 *dNSName;
    /* ORAddress x400Address */
    NSSName *directoryName;
    /* EDIPartyName ediPartyName */
    NSSUTF8 *uniformResourceIdentifier;
    NSSItem *iPAddress;
    NSSOID *registeredID;
  } u;
};
