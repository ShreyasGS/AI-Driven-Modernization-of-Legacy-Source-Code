/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

// as does this
#include "nsICharsetConverterManager.h"
#include "nsICharsetAlias.h"
#include "nsIPlatformCharset.h"
#include "nsIServiceManager.h"
#include "nsICharsetConverterManager2.h"

#include "nsISupports.h"
#include "nsIPref.h"
#include "nsIMimeConverter.h"
#include "msgCore.h"
#include "nsMsgI18N.h"
#include "nsFileSpec.h"
#include "nsFileStream.h"
#include "nsMsgMimeCID.h"
#include "nsMimeTypes.h"
#include "nsIEntityConverter.h"
#include "nsISaveAsCharset.h"
#include "nsHankakuToZenkakuCID.h"
#include "nsXPIDLString.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "prmem.h"
#include "nsFileSpec.h"
#include "nsUnicharUtils.h"

static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);
static NS_DEFINE_CID(kCMimeConverterCID, NS_MIME_CONVERTER_CID);
static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);
static NS_DEFINE_CID(kEntityConverterCID, NS_ENTITYCONVERTER_CID);

//
// International functions necessary for composition
//

nsresult nsMsgI18NConvertFromUnicode(const nsCString& aCharset, 
                                     const nsString& inString,
                                     nsCString& outString)
{
  if (inString.IsEmpty()) {
    outString.Truncate(0);
    return NS_OK;
  }
  // Note: this will hide a possible error when the unicode text may contain more than one charset.
  // (e.g. Latin1 + Japanese). Use nsMsgI18NSaveAsCharset instead to avoid that problem.
  else if (aCharset.IsEmpty() ||
      aCharset.EqualsIgnoreCase("us-ascii") ||
      aCharset.EqualsIgnoreCase("ISO-8859-1")) {
    outString.AssignWithConversion(inString);
    return NS_OK;
  }
  else if (aCharset.EqualsIgnoreCase("UTF-8")) {
    char *s = ToNewUTF8String(inString);
    if (NULL == s)
      return NS_ERROR_OUT_OF_MEMORY;
    outString.Assign(s);
    Recycle(s);
    return NS_OK;
  }
  nsAutoString convCharset(NS_LITERAL_STRING("ISO-8859-1"));
  nsresult res;

  // Resolve charset alias
  nsCOMPtr <nsICharsetAlias> calias = do_GetService(NS_CHARSETALIAS_CONTRACTID, &res);
  if (NS_SUCCEEDED(res)) {
    nsAutoString aAlias; aAlias.AssignWithConversion(aCharset.get());
    if (aAlias.Length()) {
      res = calias->GetPreferred(aAlias, convCharset);
    }
  }

  nsCOMPtr <nsICharsetConverterManager> ccm = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &res);
  if(NS_SUCCEEDED(res)) {
    nsCOMPtr <nsIUnicodeEncoder> encoder;

    // get an unicode converter
    res = ccm->GetUnicodeEncoder(&convCharset, getter_AddRefs(encoder));
    if(NS_SUCCEEDED(res)) {
      res = encoder->SetOutputErrorBehavior(nsIUnicodeEncoder::kOnError_Replace, nsnull, '?');
      if (NS_SUCCEEDED(res)) {

        const PRUnichar *originalSrcPtr = inString.get();
        PRUnichar *currentSrcPtr = NS_CONST_CAST(PRUnichar *, originalSrcPtr);
        PRInt32 originalUnicharLength = inString.Length();
        PRInt32 srcLength;
        PRInt32 dstLength;
        char localbuf[512];
        PRInt32 consumedLen = 0;

        outString.Assign("");

        // convert
        while (consumedLen < originalUnicharLength) {
          srcLength = originalUnicharLength - consumedLen;  
          dstLength = 512;
          res = encoder->Convert(currentSrcPtr, &srcLength, localbuf, &dstLength);
          if (NS_FAILED(res) || dstLength == 0)
            break;
          outString.Append(localbuf, dstLength);

          currentSrcPtr += srcLength;
          consumedLen = currentSrcPtr - originalSrcPtr; // src length used so far
        }
        res = encoder->Finish(localbuf, &dstLength);
        if (NS_SUCCEEDED(res))
          outString.Append(localbuf, dstLength);
      }
    }    
  }
  return res;
}

nsresult nsMsgI18NConvertToUnicode(const nsCString& aCharset, 
                                   const nsCString& inString, 
                                   nsString& outString)
{
  if (inString.IsEmpty()) {
    outString.Truncate();
    return NS_OK;
  }
  else if (aCharset.IsEmpty() ||
      aCharset.EqualsIgnoreCase("us-ascii") ||
      aCharset.EqualsIgnoreCase("ISO-8859-1")) {
    outString.AssignWithConversion(inString.get());
    return NS_OK;
  }

  nsAutoString convCharset;
  nsresult res;

  // Resolve charset alias
  nsCOMPtr <nsICharsetAlias> calias = do_GetService(NS_CHARSETALIAS_CONTRACTID, &res);
  if (NS_SUCCEEDED(res)) {
    nsAutoString aAlias; aAlias.AssignWithConversion(aCharset.get());
    if (aAlias.Length()) {
      res = calias->GetPreferred(aAlias, convCharset);
    }
  }
  if (NS_FAILED(res)) {
    return res;
  }

  nsCOMPtr <nsICharsetConverterManager> ccm = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &res);
  if(NS_SUCCEEDED(res)) {
    nsCOMPtr <nsIUnicodeDecoder> decoder;

    // get an unicode converter
    res = ccm->GetUnicodeDecoder(&convCharset, getter_AddRefs(decoder));
    if(NS_SUCCEEDED(res)) {

      const char *originalSrcPtr = inString.get();
      char *currentSrcPtr = NS_CONST_CAST(char *, originalSrcPtr);
      PRInt32 originalLength = inString.Length();
      PRInt32 srcLength;
      PRInt32 dstLength;
      PRUnichar localbuf[512];
      PRInt32 consumedLen = 0;

      outString.Assign(NS_LITERAL_STRING(""));

      // convert
      while (consumedLen < originalLength) {
        srcLength = originalLength - consumedLen;  
        dstLength = 512;
        res = decoder->Convert(currentSrcPtr, &srcLength, localbuf, &dstLength);
        if (NS_FAILED(res) || dstLength == 0)
          break;
        outString.Append(localbuf, dstLength);

        currentSrcPtr += srcLength;
        consumedLen = currentSrcPtr - originalSrcPtr; // src length used so far
      }
    }    
  }  
  return res;
}

// Convert an unicode string to a C string with a given charset.
nsresult ConvertFromUnicode(const char* aCharset,
                            const nsString& inString,
                            char** outCString)
{
#if 0 
  nsCAutoString s;
  nsresult rv;
  rv = nsMsgI18NConvertFromUnicode(aCharset, inString, s);
  *outCString = PL_strdup(s);
  return rv;
#endif

  *outCString = NULL;

  if (inString.IsEmpty()) {
    *outCString = nsCRT::strdup("");
    return (NULL == *outCString) ? NS_ERROR_OUT_OF_MEMORY : NS_OK;
  }
  // Note: this will hide a possible error when the unicode text may contain more than one charset.
  // (e.g. Latin1 + Japanese). Use nsMsgI18NSaveAsCharset instead to avoid that problem.
  else if (!*aCharset ||
           !nsCRT::strcasecmp("us-ascii", aCharset) ||
           !nsCRT::strcasecmp("ISO-8859-1", aCharset)) {
    *outCString = ToNewCString(inString);
    return *outCString ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
  }
  else if (!nsCRT::strcasecmp("UTF-8", aCharset)) {
    *outCString = ToNewUTF8String(inString);
    return *outCString ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult res;

  nsCOMPtr <nsICharsetConverterManager2> ccm2 = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &res);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr <nsIAtom> charsetAtom;
  res = ccm2->GetCharsetAtom(NS_ConvertASCIItoUCS2(aCharset).get(), getter_AddRefs(charsetAtom));
  NS_ENSURE_SUCCESS(res, res);

  // get an unicode converter
  nsCOMPtr<nsIUnicodeEncoder> encoder;
  res = ccm2->GetUnicodeEncoder(charsetAtom, getter_AddRefs(encoder));
  NS_ENSURE_SUCCESS(res, res);


  PRUnichar *unichars = (PRUnichar *) inString.get();
  PRInt32 unicharLength = inString.Length();
  PRInt32 dstLength;

  res = encoder->GetMaxLength(unichars, unicharLength, &dstLength);
  NS_ENSURE_SUCCESS(res, res);

  res = encoder->SetOutputErrorBehavior(nsIUnicodeEncoder::kOnError_Replace, nsnull, '?');
  NS_ENSURE_SUCCESS(res, res);

  // allocale an output buffer
  *outCString = (char *) PR_Malloc(dstLength + 1);
  NS_ENSURE_TRUE(*outCString, NS_ERROR_OUT_OF_MEMORY);

  PRInt32 buffLength = dstLength;
  **outCString = '\0';
  res = encoder->Convert(unichars, &unicharLength, *outCString, &dstLength);
  if (NS_SUCCEEDED(res)) {
    PRInt32 finLen = buffLength - dstLength;
    res = encoder->Finish((char *)(*outCString+dstLength), &finLen);
    if (NS_SUCCEEDED(res)) {
      dstLength += finLen;
    }
    (*outCString)[dstLength] = '\0';
  }

  return res;
}

// Convert a C string to an unicode string.
nsresult ConvertToUnicode(const char* aCharset, 
                          const char* inCString, 
                          nsString& outString)
{
#if 0 
  nsresult rv;
  rv = nsMsgI18NConvertToUnicode(aCharset, nsCAutoString(inCString), outString);
  return rv;
#endif

  if (NULL == inCString) {
    return NS_ERROR_NULL_POINTER;
  }
  else if ('\0' == *inCString) {
    outString.Truncate();
    return NS_OK;
  }
  else if ((!*aCharset ||
            !nsCRT::strcasecmp("us-ascii", aCharset) ||
            !nsCRT::strcasecmp("ISO-8859-1", aCharset)) &&
           nsCRT::IsAscii(inCString)) {
    outString.AssignWithConversion(inCString);
    return NS_OK;
  }

  nsresult res;

  nsCOMPtr <nsICharsetConverterManager2> ccm2 = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &res);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr <nsIAtom> charsetAtom;
  res = ccm2->GetCharsetAtom(NS_ConvertASCIItoUCS2(aCharset).get(), getter_AddRefs(charsetAtom));
  NS_ENSURE_SUCCESS(res, res);

  // get an unicode converter
  nsCOMPtr<nsIUnicodeDecoder> decoder;
  res = ccm2->GetUnicodeDecoder(charsetAtom, getter_AddRefs(decoder));
  NS_ENSURE_SUCCESS(res, res);

  PRUnichar *unichars;
  PRInt32 unicharLength;
  PRInt32 srcLen = PL_strlen(inCString);

  // buffer size 144 =
  // 72 (default line len for compose) 
  // times 2 (converted byte len might be larger)
  const int klocalbufsize = 144; 
  PRUnichar localbuf[klocalbufsize+1];
  PRBool usedlocalbuf;

  if (srcLen > klocalbufsize) {
    res = decoder->GetMaxLength(inCString, srcLen, &unicharLength);
    NS_ENSURE_SUCCESS(res, res);
    unichars = (PRUnichar *) nsMemory::Alloc(unicharLength * sizeof(PRUnichar));
    NS_ENSURE_TRUE(unichars, NS_ERROR_OUT_OF_MEMORY);
    usedlocalbuf = PR_FALSE;
  }
  else {
    unichars = localbuf;
    unicharLength = klocalbufsize+1;
    usedlocalbuf = PR_TRUE;
  }

  // convert to unicode
  res = decoder->Convert(inCString, &srcLen, unichars, &unicharLength);
  outString.Assign(unichars, unicharLength);
  if (!usedlocalbuf)
    nsMemory::Free(unichars);

  return res;
}

// Charset to be used for the internatl processing.
const char *msgCompHeaderInternalCharset()
{
  // UTF-8 is a super set of us-ascii. 
  // We can use the same string manipulation methods as us-ascii without breaking non us-ascii characters. 
  return "UTF-8";
}

// Charset used by the file system.
const char * nsMsgI18NFileSystemCharset()
{
	/* Get a charset used for the file. */
	static nsCAutoString fileSystemCharset;

	if (fileSystemCharset.Length() < 1) 
	{
		nsresult rv;
		nsCOMPtr <nsIPlatformCharset> platformCharset = do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
      nsAutoString charset;
      rv = platformCharset->GetCharset(kPlatformCharsetSel_FileName, charset);
      fileSystemCharset.AssignWithConversion(charset);
    }

		if (NS_FAILED(rv)) 
			fileSystemCharset.Assign("ISO-8859-1");
	}
	return fileSystemCharset.get();
}

// MIME encoder, output string should be freed by PR_FREE
char * nsMsgI18NEncodeMimePartIIStr(const char *header, PRBool structured, const char *charset, PRInt32 fieldnamelen, PRBool usemime) 
{
  // No MIME, convert to the outgoing mail charset.
  if (PR_FALSE == usemime) {
    char *convertedStr;
    if (NS_SUCCEEDED(ConvertFromUnicode(charset, NS_ConvertUTF8toUCS2(header), &convertedStr)))
      return (convertedStr);
    else
      return PL_strdup(header);
  }

  char *encodedString = nsnull;
  nsresult res;
  nsCOMPtr<nsIMimeConverter> converter = do_GetService(kCMimeConverterCID, &res);
  if (NS_SUCCEEDED(res) && nsnull != converter)
    res = converter->EncodeMimePartIIStr_UTF8(header, structured, charset, fieldnamelen, kMIME_ENCODED_WORD_SIZE, &encodedString);

  return NS_SUCCEEDED(res) ? encodedString : nsnull;
}

// Return True if a charset is stateful (e.g. JIS).
PRBool nsMsgI18Nstateful_charset(const char *charset)
{
  //TODO: use charset manager's service
  return (nsCRT::strcasecmp(charset, "ISO-2022-JP") == 0);
}

PRBool nsMsgI18Nmultibyte_charset(const char *charset)
{
  nsresult res;
  nsCOMPtr <nsICharsetConverterManager2> ccm2 = do_GetService(kCharsetConverterManagerCID, &res);
  PRBool result = PR_FALSE;

  if (NS_SUCCEEDED(res)) {
    nsCOMPtr <nsIAtom> charsetAtom;
    nsAutoString charsetData;
    res = ccm2->GetCharsetAtom(NS_ConvertASCIItoUCS2(charset).get(), getter_AddRefs(charsetAtom));
    if (NS_SUCCEEDED(res)) {
      res = ccm2->GetCharsetData2(charsetAtom, NS_LITERAL_STRING(".isMultibyte").get(), &charsetData);
      if (NS_SUCCEEDED(res)) {
        result = charsetData.Equals(NS_LITERAL_STRING("true"),
                                    nsCaseInsensitiveStringComparator());
      }
    }
  }

  return result;
}

// Check 7bit in a given buffer.
// This is expensive (both memory and performance).
// The check would be very simple if applied to an unicode text (e.g. nsString or utf-8).
// Possible optimazaion is to search ESC(0x1B) in case of iso-2022-jp and iso-2022-kr.
// Or convert and check line by line.
PRBool nsMsgI18N7bit_data_part(const char *charset, const char *inString, const PRUint32 size)
{
  nsAutoString aCharset; aCharset.AssignWithConversion(charset);
  nsresult res;
  PRBool result = PR_TRUE;
  
  nsCOMPtr <nsICharsetConverterManager> ccm = do_GetService(kCharsetConverterManagerCID, &res);

  if (NS_SUCCEEDED(res)) {
    nsIUnicodeDecoder* decoder = nsnull;

    // get an unicode converter
    res = ccm->GetUnicodeDecoder(&aCharset, &decoder);
    if(NS_SUCCEEDED(res)) {
      char *currentSrcPtr = NS_CONST_CAST(char *, inString);
      PRUint32 consumedLen = 0;
      PRUnichar unicharBuff[512];
      PRInt32 srcLen;
      PRInt32 unicharLength;

      // convert to unicode
      while (consumedLen < size) {
        srcLen = ((size - consumedLen) >= 512) ? 512 : (size - consumedLen);  // buffer len or remaining src len
        unicharLength = 512;
        res = decoder->Convert(currentSrcPtr, &srcLen, unicharBuff, &unicharLength);
        // break, if error or the input string not consumed
        if (NS_FAILED(res) || !srcLen)
          break;
        for (PRInt32 i = 0; i < unicharLength; i++) {
          if (unicharBuff[i] > 127) {
            result = PR_FALSE;
            break;
          }
        }
        currentSrcPtr += srcLen;
        consumedLen = currentSrcPtr - inString; // src length used so far
      }
      NS_IF_RELEASE(decoder);
    }    
  }

  return result;
}

PRBool nsMsgI18Ncheck_data_in_charset_range(const char *charset, const PRUnichar* inString)
{
  if (!charset || !*charset || !inString || !*inString)
    return PR_TRUE;

  nsAutoString aCharset; aCharset.AssignWithConversion(charset);
  nsresult res;
  PRBool result = PR_TRUE;
  
  nsCOMPtr <nsICharsetConverterManager> ccm = do_GetService(kCharsetConverterManagerCID, &res);

  if (NS_SUCCEEDED(res)) {
    nsCOMPtr <nsIUnicodeEncoder> encoder;

    // get an unicode converter
    res = ccm->GetUnicodeEncoder(&aCharset, getter_AddRefs(encoder));
    if(NS_SUCCEEDED(res)) {
      const PRUnichar *originalPtr = inString;
      PRInt32 originalLen = nsCRT::strlen(inString);
      PRUnichar *currentSrcPtr = NS_CONST_CAST(PRUnichar *, originalPtr);
      char localBuff[512];
      PRInt32 consumedLen = 0;
      PRInt32 srcLen;
      PRInt32 dstLength;

      // convert from unicode
      while (consumedLen < originalLen) {
        srcLen = originalLen - consumedLen;
        dstLength = 512;
        res = encoder->Convert(currentSrcPtr, &srcLen, localBuff, &dstLength);
        if (NS_ERROR_UENC_NOMAPPING == res) {
          result = PR_FALSE;
          break;
        }
        else if (NS_FAILED(res) || (0 == dstLength))
          break;

        currentSrcPtr += srcLen;
        consumedLen = currentSrcPtr - originalPtr; // src length used so far
      }
    }    
  }

  return result;
}

// Simple parser to parse META charset. 
// It only supports the case when the description is within one line. 
const char * 
nsMsgI18NParseMetaCharset(nsFileSpec* fileSpec) 
{ 
  static char charset[kMAX_CSNAME+1]; 
  char buffer[512]; 

  *charset = '\0'; 

  if (fileSpec->IsDirectory()) {
    NS_ASSERTION(0,"file is a directory");
    return charset; 
  }

  nsInputFileStream fileStream(*fileSpec); 

  while (!fileStream.eof() && !fileStream.failed() && 
         fileStream.is_open()) { 
    fileStream.readline(buffer, 512); 
    if (*buffer == nsCRT::CR || *buffer == nsCRT::LF || *buffer == 0) 
      continue; 

    for (int i = 0; i < (int)PL_strlen(buffer); i++) { 
      buffer[i] = toupper(buffer[i]); 
    } 

    if (PL_strstr(buffer, "/HEAD")) 
      break; 

    if (PL_strstr(buffer, "META") && 
        PL_strstr(buffer, "HTTP-EQUIV") && 
        PL_strstr(buffer, "CONTENT-TYPE") && 
        PL_strstr(buffer, "CHARSET") 
        ) 
    { 
      char *cp = PL_strstr(PL_strstr(buffer, "CHARSET"), "=") + 1; 
      char seps[]   = " \"\'"; 
      char *token; 
      char* newStr; 
      token = nsCRT::strtok(cp, seps, &newStr); 
      if (token != NULL) 
      { 
        PL_strncpy(charset, token, sizeof(charset));
        charset[sizeof(charset)-1] = '\0';
      } 
    } 
  } 

  return charset; 
} 

nsresult nsMsgI18NConvertToEntity(const nsString& inString, nsString* outString)
{
  nsresult res;

  outString->Truncate();
  nsCOMPtr <nsIEntityConverter> entityConv;
  res = nsComponentManager::CreateInstance(kEntityConverterCID, NULL, 
                                           NS_GET_IID(nsIEntityConverter), getter_AddRefs(entityConv));
  if(NS_SUCCEEDED(res)) {
    PRUnichar *entities = NULL;
    res = entityConv->ConvertToEntities(inString.get(), nsIEntityConverter::html40Latin1, &entities);
    if (NS_SUCCEEDED(res) && (NULL != entities)) {
      outString->Assign(entities);
      nsMemory::Free(entities);
     }
   }
 
  return res;
}

nsresult nsMsgI18NSaveAsCharset(const char* contentType, const char *charset, const PRUnichar* inString, char** outString)
{
  NS_ASSERTION(contentType, "null ptr- contentType");
  NS_ASSERTION(charset, "null ptr- charset");
  NS_ASSERTION(outString, "null ptr- outString");
  if(!contentType || !charset || !inString || !outString)
    return NS_ERROR_NULL_POINTER;
  if (0 == *inString) {
    *outString = nsCRT::strdup("");
    return (NULL != *outString) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
  }
  *outString = NULL;

  PRBool bTEXT_HTML = PR_FALSE;
  nsresult res;

  if (!nsCRT::strcasecmp(contentType, TEXT_HTML)) {
    bTEXT_HTML = PR_TRUE;
  }
  else if (nsCRT::strcasecmp(contentType, TEXT_PLAIN)) {
    return NS_ERROR_ILLEGAL_VALUE;  // not supported type
  }

  nsCOMPtr <nsICharsetConverterManager2> ccm2 = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &res);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr <nsIAtom> charsetAtom;
  res = ccm2->GetCharsetAtom(NS_ConvertASCIItoUCS2(charset).get(), getter_AddRefs(charsetAtom));
  NS_ENSURE_SUCCESS(res, res);

  const PRUnichar *charsetName;
  res = charsetAtom->GetUnicode(&charsetName);
  NS_ENSURE_SUCCESS(res, res);

  // charset converter plus entity, NCR generation
  nsCOMPtr <nsISaveAsCharset> conv = do_CreateInstance(NS_SAVEASCHARSET_CONTRACTID, &res);
  NS_ENSURE_SUCCESS(res, res);

  // attribute: 
  // html text - charset conv then fallback to entity or NCR
  // plain text - charset conv then fallback to '?'
  if (bTEXT_HTML)
    // For ISO-8859-1 only, convert to entity first (always generate entites like &nbsp;).
    res = conv->Init(NS_ConvertUCS2toUTF8(charsetName).get(), 
                     !nsCRT::strcmp(charsetName, NS_LITERAL_STRING("ISO-8859-1").get()) ?
                     nsISaveAsCharset::attr_htmlTextDefault :
                     nsISaveAsCharset::attr_EntityAfterCharsetConv + nsISaveAsCharset::attr_FallbackDecimalNCR, 
                     nsIEntityConverter::html32);
  else
    // fallback for text/plain: first try transliterate then '?'
    res = conv->Init(NS_ConvertUCS2toUTF8(charsetName).get(), 
                     nsISaveAsCharset::attr_FallbackQuestionMark + nsISaveAsCharset::attr_EntityAfterCharsetConv, 
                     nsIEntityConverter::transliterate);
  NS_ENSURE_SUCCESS(res, res);

  // Mapping characters in a certain range (required for Japanese only)
  if (!nsCRT::strcmp(charsetName, NS_LITERAL_STRING("ISO-2022-JP").get())) {
    static PRInt32 sSendHankakuKana = -1;
    if (sSendHankakuKana < 0) {
      nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &res));
      NS_ENSURE_SUCCESS(res, res);
      PRBool sendHankaku;
      // Get a hidden 4.x pref with no UI, get it only once.
      if (NS_FAILED(prefs->GetBoolPref("mailnews.send_hankaku_kana", &sendHankaku)))
        sSendHankakuKana = 0;  // no pref means need the mapping
      else
        sSendHankakuKana = sendHankaku ? 1 : 0;
    }

    if (!sSendHankakuKana) {
      nsCOMPtr <nsITextTransform> textTransform = do_CreateInstance(NS_HANKAKUTOZENKAKU_CONTRACTID, &res);
      if (NS_SUCCEEDED(res)) {
        nsAutoString mapped;
        res = textTransform->Change(inString, nsCRT::strlen(inString), mapped);
        if (NS_SUCCEEDED(res)) {
          return conv->Convert(mapped.get(), outString);
        }
      }
    }
  }

  // Convert to charset
  return conv->Convert(inString, outString);
}

nsresult nsMsgI18NFormatNNTPXPATInNonRFC1522Format(const nsCString& aCharset, 
                                                   const nsString& inString, 
                                                   nsCString& outString)
{
  outString.AssignWithConversion(inString);
  return NS_OK;
}

char *
nsMsgI18NGetAcceptLanguage(void)
{
  static char   lang[32];
  nsresult      res;

  nsCOMPtr<nsIPref> prefs(do_GetService(kPrefCID, &res)); 
  if (nsnull != prefs && NS_SUCCEEDED(res))
  {
    nsXPIDLString prefValue;

    memset(lang, 0, sizeof(lang));
    res = prefs->GetLocalizedUnicharPref("intl.accept_languages", getter_Copies(prefValue));
	  if (NS_SUCCEEDED(res) && prefValue) 
    {
      PL_strncpy(lang, NS_ConvertUCS2toUTF8(prefValue).get(), sizeof(lang));
      lang[sizeof(lang)-1] = '\0';
    }
	  else 
		  PL_strcpy(lang, "en");
  }
  else
    PL_strcpy(lang, "en");

  return (char *)lang;
}


// taken from nsFileSpec::GetNativePathString()
void
nsMsgGetNativePathString(const char *aPath, nsString& aResult)
{
  if (!aPath) {
    aResult.Truncate();
    return;
  }
  if (nsCRT::IsAscii(aPath))
    aResult.AssignWithConversion(aPath);
  else
    ConvertToUnicode(nsMsgI18NFileSystemCharset(), aPath, aResult);
}

