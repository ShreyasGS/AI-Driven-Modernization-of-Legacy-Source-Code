/*
 * arcfive.c - stubs for RC5 - NOT a working implementation!
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
 * Copyright (C) 2000 Netscape Communications Corporation.  All
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
 * $Id: arcfive.c,v 1.2.124.1 2002/04/10 03:27:19 cltbld%netscape.com Exp $
 */

#include "blapi.h"
#include "prerror.h"

/******************************************/
/*
** RC5 symmetric block cypher -- 64-bit block size
*/

/*
** Create a new RC5 context suitable for RC5 encryption/decryption.
**      "key" raw key data
**      "len" the number of bytes of key data
**      "iv" is the CBC initialization vector (if mode is NSS_RC5_CBC)
**      "mode" one of NSS_RC5 or NSS_RC5_CBC
**
** When mode is set to NSS_RC5_CBC the RC5 cipher is run in "cipher block
** chaining" mode.
*/
RC5Context *
RC5_CreateContext(SECItem *key, unsigned int rounds,
                  unsigned int wordSize, unsigned char *iv, int mode)
{
    PORT_SetError(PR_NOT_IMPLEMENTED_ERROR);
    return NULL;
}

/*
** Destroy an RC5 encryption/decryption context.
**      "cx" the context
**      "freeit" if PR_TRUE then free the object as well as its sub-objects
*/
void 
RC5_DestroyContext(RC5Context *cx, PRBool freeit) 
{
    PORT_SetError(PR_NOT_IMPLEMENTED_ERROR);
}

/*
** Perform RC5 encryption.
**      "cx" the context
**      "output" the output buffer to store the encrypted data.
**      "outputLen" how much data is stored in "output". Set by the routine
**         after some data is stored in output.
**      "maxOutputLen" the maximum amount of data that can ever be
**         stored in "output"
**      "input" the input data
**      "inputLen" the amount of input data
*/
SECStatus 
RC5_Encrypt(RC5Context *cx, unsigned char *output, unsigned int *outputLen, 
	    unsigned int maxOutputLen, 
	    const unsigned char *input, unsigned int inputLen)
{
    PORT_SetError(PR_NOT_IMPLEMENTED_ERROR);
    return SECFailure;
}

/*
** Perform RC5 decryption.
**      "cx" the context
**      "output" the output buffer to store the decrypted data.
**      "outputLen" how much data is stored in "output". Set by the routine
**         after some data is stored in output.
**      "maxOutputLen" the maximum amount of data that can ever be
**         stored in "output"
**      "input" the input data
**      "inputLen" the amount of input data
*/
SECStatus 
RC5_Decrypt(RC5Context *cx, unsigned char *output, unsigned int *outputLen, 
	    unsigned int maxOutputLen,
            const unsigned char *input, unsigned int inputLen)
{
    PORT_SetError(PR_NOT_IMPLEMENTED_ERROR);
    return SECFailure;
}

