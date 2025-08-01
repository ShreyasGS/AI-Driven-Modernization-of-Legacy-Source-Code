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
 * base64.h - prototypes for base64 encoding/decoding
 * Note: These functions are deprecated; see nssb64.h for new routines.
 *
 * $Id: base64.h,v 1.1.162.1 2002/04/10 03:29:19 cltbld%netscape.com Exp $
 */
#ifndef _BASE64_H_
#define _BASE64_H_

#include "seccomon.h"

SEC_BEGIN_PROTOS

/*
** Return an PORT_Alloc'd ascii string which is the base64 encoded
** version of the input string.
*/
extern char *BTOA_DataToAscii(const unsigned char *data, unsigned int len);

/*
** Return an PORT_Alloc'd string which is the base64 decoded version
** of the input string; set *lenp to the length of the returned data.
*/
extern unsigned char *ATOB_AsciiToData(const char *string, unsigned int *lenp);
 
/*
** Convert from ascii to binary encoding of an item.
*/
extern SECStatus ATOB_ConvertAsciiToItem(SECItem *binary_item, char *ascii);

/*
** Convert from binary encoding of an item to ascii.
*/
extern char *BTOA_ConvertItemToAscii(SECItem *binary_item);

SEC_END_PROTOS

#endif /* _BASE64_H_ */
