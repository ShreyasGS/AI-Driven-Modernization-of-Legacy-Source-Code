/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the Mozilla OS/2 libraries.
 *
 * The Initial Developer of the Original Code is John Fairhurst,
 * <john_fairhurst@iname.com>.  Portions created by John Fairhurst are
 * Copyright (C) 1999 John Fairhurst. All Rights Reserved.
 *
 * Contributor(s): 
 *   2000/08/04     Henry Sobotka <sobotka@axess.com>  Update from M7
 *   2000/10/02     IBM Corp.                          Sync-up to M18 level
 *
 */

#include "nsClipboard.h"
#include "nsVoidArray.h"
#include "nsISupportsPrimitives.h"
#include "nsCOMPtr.h"
#include "nsPrimitiveHelpers.h"
#include "nsXPIDLString.h"
#include "prmem.h"

#include <unidef.h>     // for UniStrlen

// The relation between mozilla's mime formats and those understood by the
// clipboard is a little hazy, mostly in the areas of images.
//
// Could be a lot cleverer & use delayed rendering or something to provide
// a consumer with text when what we've got is unicode, or whatever.

struct FormatEntry
{
  const char   *szMimeType;
  ULONG         ulClipboardFmt;
  const char   *szFmtName;
};

FormatEntry formatEntries[] = 
{
  { kTextMime,      CF_TEXT, 0      },
  { kUnicodeMime,   0, "CF_UNICODE" },
  { kHTMLMime,      0, "CF_HTML"    },
  { kPNGImageMime,  0, "CF_PNG"     },
  { kGIFImageMime,  0, "CF_GIF"     },
  { kJPEGImageMime, 0, "CF_JPEG"    },
  { kAOLMailMime,   0, "CF_AOLMAIL" },
  { kFileMime,      0, "CF_FILE"    },
  { kURLMime,       0, "CF_URL"     }
};

// scaffolding
nsClipboard::nsClipboard() : nsBaseClipboard()
{
  for (int cnt=0 ; cnt < sizeof(formatEntries) / sizeof(formatEntries[0]) ; cnt++)
  {
    if (formatEntries[cnt].ulClipboardFmt == 0)  // Not yet registered
      formatEntries[cnt].ulClipboardFmt = gWidgetModuleData->GetAtom( formatEntries[cnt].szFmtName );
  }
}

nsClipboard::~nsClipboard()
{}

nsresult nsClipboard::SetNativeClipboardData(PRInt32 aWhichClipboard)
{
  if (aWhichClipboard != kGlobalClipboard)
    return NS_ERROR_FAILURE;

  return DoClipboardAction(Write);
}

nsresult nsClipboard::GetNativeClipboardData(nsITransferable *aTransferable, PRInt32 aWhichClipboard)
{
  // make sure we have a good transferable
  if (!aTransferable || aWhichClipboard != kGlobalClipboard)
    return NS_ERROR_FAILURE;

  nsITransferable *tmp = mTransferable;
  mTransferable = aTransferable;
  nsresult rc = DoClipboardAction(Read);
  mTransferable = tmp;
  return rc;
}

// Get some data from the clipboard
PRBool nsClipboard::GetClipboardData(const char *aFlavor)
{
  ULONG ulFormatID = GetFormatID( aFlavor );
  
  PRBool found = GetClipboardDataByID( ulFormatID, aFlavor );

  if (!found) 
  {
    if (!strcmp( aFlavor, kUnicodeMime ))
    {
      found = GetClipboardDataByID( CF_TEXT, aFlavor );
    }
    else if (strstr( aFlavor, "image/" ))
    {
      found = GetClipboardDataByID( CF_BITMAP, aFlavor );
    }
  }

  return found;
}

PRBool nsClipboard::GetClipboardDataByID(ULONG ulFormatID, const char *aFlavor)
{
  PVOID pDataMem;
  PVOID pTempBuf;
  PRUint32 NumOfBytes;
  PRBool TempBufAllocated = PR_FALSE;

  PVOID pClipboardData = NS_REINTERPRET_CAST(PVOID, WinQueryClipbrdData( 0, ulFormatID ));

  if (!pClipboardData) 
    return PR_FALSE;

  if (strstr( aFlavor, "text/" ))  // All text/.. flavors are null-terminated
  {
    pDataMem = pClipboardData;

    if (ulFormatID == CF_TEXT)     // CF_TEXT is one byte character set
    {
      PRUint32 NumOfChars = strlen( NS_STATIC_CAST (char*, pDataMem) );
      NumOfBytes = NumOfChars;

      if (!strcmp( aFlavor, kUnicodeMime ))  // Asked for unicode, but only plain text available.  Convert it!
      {
        NumOfBytes *= sizeof(UniChar);

        pTempBuf = nsMemory::Alloc( NumOfBytes + sizeof(UniChar) );
        TempBufAllocated = PR_TRUE;
        NumOfChars = gWidgetModuleData->ConvertToUcs( NS_STATIC_CAST(char*, pDataMem), NS_STATIC_CAST(PRUnichar*, pTempBuf), NumOfChars + 1 );
        NumOfBytes = NumOfChars * sizeof(UniChar);
        pDataMem = pTempBuf;
      }

    }
    else                           // All other text/.. flavors are in unicode
    {
      PRUint32 NumOfChars = UniStrlen( NS_STATIC_CAST(UniChar*, pDataMem) );
      NumOfBytes = NumOfChars * sizeof(UniChar);
      pTempBuf = nsMemory::Alloc(NumOfBytes);
      memcpy(pTempBuf, pDataMem, NumOfBytes);
      pDataMem = pTempBuf;
    }

    // DOM wants LF only, so convert from CRLF
    nsLinebreakHelpers::ConvertPlatformToDOMLinebreaks( aFlavor, &pDataMem,   // pDataMem could be reallocated !!
                                                        NS_REINTERPRET_CAST(PRInt32*, &NumOfBytes) );  // yuck

  }
  else                             // Assume rest of flavors are binary data
  {
    if (ulFormatID == CF_BITMAP)
    {
      if (!strcmp( aFlavor, kJPEGImageMime ))
      {
        // OS2TODO  Convert bitmap to jpg
#ifdef DEBUG
        printf( "nsClipboard:: No JPG found on clipboard; need to convert BMP\n");
#endif
      }
      else if (!strcmp( aFlavor, kGIFImageMime ))
      {
        // OS2TODO  Convert bitmap to gif
#ifdef DEBUG
        printf( "nsClipboard:: No GIF found on clipboard; need to convert BMP\n");
#endif
      }
      else if (!strcmp( aFlavor, kPNGImageMime ))
      {
        // OS2TODO  Convert bitmap to png
#ifdef DEBUG
        printf( "nsClipboard:: No PNG found on clipboard; need to convert BMP\n");
#endif
      }
    }
    else
    {
      pDataMem = NS_STATIC_CAST(PBYTE, pClipboardData) + sizeof(PRUint32);
      NumOfBytes = *(NS_STATIC_CAST(PRUint32*, pClipboardData));
    }
  }

  nsCOMPtr<nsISupports> genericDataWrapper;
  nsPrimitiveHelpers::CreatePrimitiveForData( aFlavor, pDataMem, NumOfBytes, getter_AddRefs(genericDataWrapper) );
  nsresult errCode = mTransferable->SetTransferData( aFlavor, genericDataWrapper, NumOfBytes );
#ifdef DEBUG
  if (errCode != NS_OK)
    printf( "nsClipboard:: Error setting data into transferable\n" );
#endif

  if (TempBufAllocated)
    nsMemory::Free(pDataMem);

  return PR_TRUE;
}


// Set some data onto the clipboard
void nsClipboard::SetClipboardData(const char *aFlavor)
{
  void *pMozData = nsnull;
  PRUint32 NumOfBytes = 0;

  // Get the data from the transferable
  nsCOMPtr<nsISupports> genericDataWrapper;
  nsresult errCode = mTransferable->GetTransferData( aFlavor, getter_AddRefs(genericDataWrapper), &NumOfBytes );
#ifdef DEBUG
  if (NS_FAILED(errCode)) printf( "nsClipboard:: Error getting data from transferable\n" );
#endif
  if (NumOfBytes == 0) return;
  nsPrimitiveHelpers::CreateDataFromPrimitive( aFlavor, genericDataWrapper, &pMozData, NumOfBytes );

  ULONG ulFormatID = GetFormatID( aFlavor );

  if (strstr( aFlavor, "text/" ))  // All text/.. flavors are null-terminated
  {
    if (ulFormatID == CF_TEXT)     // CF_TEXT is one byte character set
    {
      char* pByteMem = nsnull;

      if (DosAllocSharedMem( NS_REINTERPRET_CAST(PPVOID, &pByteMem), nsnull, NumOfBytes + sizeof(char), 
                             PAG_WRITE | PAG_COMMIT | OBJ_GIVEABLE ) == NO_ERROR)
      {
        memcpy( pByteMem, pMozData, NumOfBytes );       // Copy text string
        pByteMem[NumOfBytes] = '\0';                    // Append terminator

        WinSetClipbrdData( 0, NS_REINTERPRET_CAST(ULONG, pByteMem), ulFormatID, CFI_POINTER );
      }
    }
    else                           // All other text/.. flavors are in unicode
    {
      UniChar* pUnicodeMem = nsnull;
      PRUint32 NumOfChars = NumOfBytes / sizeof(UniChar);
   
      if (DosAllocSharedMem( NS_REINTERPRET_CAST(PPVOID, &pUnicodeMem), nsnull, NumOfBytes + sizeof(UniChar), 
                             PAG_WRITE | PAG_COMMIT | OBJ_GIVEABLE ) == NO_ERROR) 
      {
        memcpy( pUnicodeMem, pMozData, NumOfBytes );    // Copy text string
        pUnicodeMem[NumOfChars] = L'\0';                // Append terminator

        WinSetClipbrdData( 0, NS_REINTERPRET_CAST(ULONG, pUnicodeMem), ulFormatID, CFI_POINTER );
      }

      // If the flavor is unicode, we also put it on the clipboard as CF_TEXT
      // after conversion to locale charset.

      if (!strcmp( aFlavor, kUnicodeMime ))
      {
        char* pByteMem = nsnull;

        if (DosAllocSharedMem( NS_REINTERPRET_CAST(PPVOID, &pByteMem), nsnull, NumOfBytes + 1, 
                               PAG_WRITE | PAG_COMMIT | OBJ_GIVEABLE ) == NO_ERROR) 
        {
          gWidgetModuleData->ConvertFromUcs( NS_STATIC_CAST(PRUnichar*, pMozData), pByteMem, NumOfBytes + 1 );
          pByteMem [NumOfBytes] = '\0';

          WinSetClipbrdData( 0, NS_REINTERPRET_CAST(ULONG, pByteMem), CF_TEXT, CFI_POINTER );
        }
      }
    }
  }
  else                             // Assume rest of flavors are binary data
  {
    PBYTE pBinaryMem = nsnull;

    if (DosAllocSharedMem( NS_REINTERPRET_CAST(PPVOID, &pBinaryMem), nsnull, NumOfBytes + sizeof(PRUint32), 
                           PAG_WRITE | PAG_COMMIT | OBJ_GIVEABLE ) == NO_ERROR) 
    {
      *(NS_REINTERPRET_CAST(PRUint32*, pBinaryMem)) = NumOfBytes;          // First DWORD contains data length
      memcpy( pBinaryMem + sizeof(PRUint32), pMozData, NumOfBytes );  // Copy binary data

      WinSetClipbrdData( 0, NS_REINTERPRET_CAST(ULONG, pBinaryMem), ulFormatID, CFI_POINTER );
    }

    // If the flavor is image, we also put it on clipboard as CF_BITMAP
    // after conversion to OS2 bitmap

    if (strstr (aFlavor, "image/"))
    {
      //  XXX OS2TODO  Convert jpg, gif, png to bitmap
#ifdef DEBUG
      printf( "nsClipboard:: Putting image on clipboard; should also convert to BMP\n" );
#endif
    }
  }
}

// Go through the flavors in the transferable and either get or set them
nsresult nsClipboard::DoClipboardAction(ClipboardAction aAction)
{
  nsresult rc = NS_ERROR_FAILURE;

  if (WinOpenClipbrd(0/*hab*/)) {

    if (aAction == Write)
      WinEmptyClipbrd(0/*hab*/);

    // Get the list of formats the transferable can handle
    nsCOMPtr<nsISupportsArray> pFormats;
    if(aAction == Read)
      rc = mTransferable->FlavorsTransferableCanImport(getter_AddRefs(pFormats));
    else
      rc = mTransferable->FlavorsTransferableCanExport(getter_AddRefs(pFormats));

    if (NS_FAILED(rc))
      return NS_ERROR_FAILURE;

    PRUint32 cFormats = 0;
    pFormats->Count(&cFormats);

    for (PRUint32 i = 0; i < cFormats; i++) {

      nsCOMPtr<nsISupports> genericFlavor;
      pFormats->GetElementAt(i, getter_AddRefs(genericFlavor));
      nsCOMPtr<nsISupportsString> currentFlavor(do_QueryInterface(genericFlavor));

      if (currentFlavor) {
        nsXPIDLCString flavorStr;
        currentFlavor->ToString(getter_Copies(flavorStr));

        if (aAction == Read) {
          if (GetClipboardData(flavorStr))
            break;
        }
        else
          SetClipboardData(flavorStr);
      }
    }
    WinCloseClipbrd(0/*hab*/);
    rc = NS_OK;
  }
  return rc;
}

// get the format ID for a given mimetype
ULONG nsClipboard::GetFormatID(const char *aMimeStr)
{
  for (int cnt=0 ; cnt < sizeof(formatEntries) / sizeof(formatEntries[0]) ; cnt++)
  {
    if (!strcmp( aMimeStr, formatEntries[cnt].szMimeType ))
    {
      return formatEntries[cnt].ulClipboardFmt;  // Found the known flavor
    }
  }
 
  return gWidgetModuleData->GetAtom( aMimeStr );        // Unknown flavor. Register it in OS/2 atom table as is
}

NS_IMETHODIMP nsClipboard::ForceDataToClipboard(PRInt32 aWhichClipboard)
{
  // make sure we have a good transferable
  if (!mTransferable || aWhichClipboard != kGlobalClipboard)
    return NS_ERROR_FAILURE;

  if (WinOpenClipbrd(0/*hab*/)) {
    WinEmptyClipbrd(0/*hab*/);

    // get flavor list that includes all flavors that can be written (including ones
    // obtained through conversion)
    nsCOMPtr<nsISupportsArray> flavorList;
    nsresult errCode = mTransferable->FlavorsTransferableCanExport(getter_AddRefs(flavorList));
    if (NS_FAILED(errCode))
      return NS_ERROR_FAILURE;

    // Walk through flavors and put data on to clipboard
    PRUint32 i;
    PRUint32 cnt;
    flavorList->Count(&cnt);
    for (i = 0; i < cnt; i++) {
      nsCOMPtr<nsISupports> genericFlavor;
      flavorList->GetElementAt(i, getter_AddRefs(genericFlavor));
      nsCOMPtr<nsISupportsString> currentFlavor(do_QueryInterface(genericFlavor));
      if (currentFlavor) {
        nsXPIDLCString flavorStr;
        currentFlavor->ToString(getter_Copies(flavorStr));
        SetClipboardData(flavorStr);
      }
    }
    WinCloseClipbrd(0/*hab*/);
  }
  return NS_OK;
}

NS_IMETHODIMP nsClipboard::HasDataMatchingFlavors(nsISupportsArray *aFlavorList, PRInt32 aWhichClipboard,
                                                  PRBool *_retval)
{
  *_retval = PR_FALSE;
  if (aWhichClipboard != kGlobalClipboard)
    return NS_OK;

  PRUint32 cnt;
  aFlavorList->Count(&cnt);
  for (PRUint32 i = 0; i < cnt; ++i) {
    nsCOMPtr<nsISupports> genericFlavor;
    aFlavorList->GetElementAt(i, getter_AddRefs(genericFlavor));
    nsCOMPtr<nsISupportsString> currentFlavor(do_QueryInterface(genericFlavor));
    if (currentFlavor) {
      nsXPIDLCString flavorStr;
      currentFlavor->ToString(getter_Copies(flavorStr));
      ULONG fmtInfo = 0;
      ULONG format = GetFormatID(flavorStr);

      if (WinQueryClipbrdFmtInfo(0/*hab*/, format, &fmtInfo)) {
        *_retval = PR_TRUE;
        break;
      }

      // if the client asked for unicode and it wasn't present, check if we have CF_TEXT.
      if (!strcmp( flavorStr, kUnicodeMime )) {
        if (WinQueryClipbrdFmtInfo( 0/*hab*/, CF_TEXT, &fmtInfo )) {
          *_retval = PR_TRUE;
          break;
        }
      }

// OS2TODO - Support for Images
      // if the client asked for image/.. and it wasn't present, check if we have CF_BITMAP.
      if (strstr (flavorStr, "image/")) {
        if (WinQueryClipbrdFmtInfo (0, CF_BITMAP, &fmtInfo)) {
#ifdef DEBUG
          printf( "nsClipboard:: Image present on clipboard; need to add BMP conversion!\n" );
#endif
//          *_retval = PR_TRUE;
//          break;
        }
      }
    }
  }
  return NS_OK;
}
