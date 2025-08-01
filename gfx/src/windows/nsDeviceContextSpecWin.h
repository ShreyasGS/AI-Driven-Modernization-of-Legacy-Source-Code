/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 *
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

#ifndef nsDeviceContextSpecWin_h___
#define nsDeviceContextSpecWin_h___

#include "nsIDeviceContextSpec.h"
#include "nsIPrintOptions.h" // For nsIPrinterEnumerator
#include "nsIPrintSettings.h"
#include <windows.h>

class nsDeviceContextSpecWin : public nsIDeviceContextSpec
{
public:
  nsDeviceContextSpecWin();

  NS_DECL_ISUPPORTS

  NS_IMETHOD Init(nsIWidget* aWidget, nsIPrintSettings* aPS, PRBool aQuiet);

  void GetDriverName(char *&aDriverName) const   { aDriverName = mDriverName;     }
  void GetDeviceName(char *&aDeviceName) const   { aDeviceName = mDeviceName;     }

  // The GetDevMode will return a pointer to a DevMode
  // whether it is from the Global memory handle or just the DevMode
  // To get the DevMode from the Global memory Handle it must lock it 
  // So this call must be paired with a call to UnlockGlobalHandle
  void GetDevMode(LPDEVMODE &aDevMode);
  void UnlockDevMode()  { if (mIsDEVMODEGlobalHandle && mGlobalDevMode) ::GlobalUnlock(mGlobalDevMode); }

  // helper functions
  nsresult GetDataFromPrinter(const PRUnichar * aName, nsIPrintSettings* aPS = nsnull);

  static nsresult SetPrintSettingsFromDevMode(nsIPrintSettings* aPrintSettings, 
                                              LPDEVMODE         aDevMode);

protected:
  nsresult ShowXPPrintDialog(PRBool aQuiet);
  nsresult ShowNativePrintDialog(nsIWidget* aWidget, PRBool aQuiet);

#ifdef MOZ_REQUIRE_CURRENT_SDK
  nsresult ShowNativePrintDialogEx(nsIWidget* aWidget, PRBool aQuiet);
#endif

  void SetDeviceName(char* aDeviceName);
  void SetDriverName(char* aDriverName);
  void SetGlobalDevMode(HGLOBAL aHGlobal);
  void SetDevMode(LPDEVMODE aDevMode);

  void SetupPaperInfoFromSettings();

  virtual ~nsDeviceContextSpecWin();

  char*     mDriverName;
  char*     mDeviceName;
  HGLOBAL   mGlobalDevMode;
  LPDEVMODE mDevMode;
  PRBool    mIsDEVMODEGlobalHandle;

  nsCOMPtr<nsIPrintSettings> mPrintSettings;

  // For PrintDlgEx
  FARPROC mUseExtendedPrintDlg;
};


//-------------------------------------------------------------------------
// Printer Enumerator
//-------------------------------------------------------------------------
class nsPrinterEnumeratorWin : public nsIPrinterEnumerator
{
public:
  nsPrinterEnumeratorWin();
  ~nsPrinterEnumeratorWin();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRINTERENUMERATOR

private:
  // helper 
  nsresult DoEnumeratePrinters(PRBool aDoExtended, PRUint32* aCount, PRUnichar*** aResult);
};

#endif
