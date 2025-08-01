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
 *     IBM Corp.
 */

#define INCL_WIN
#define INCL_PM

#include <os2.h>
#include "uninstall.h"
#include "extra.h"
#include "dialogs.h"
#include "ifuncns.h"

/* global variables */
HAB				hab;

LHANDLE      hInst;

LHANDLE          hAccelTable;

HWND            hDlgUninstall;
HWND            hDlgMessage;
HWND            hWndMain;

PSZ           szEGlobalAlloc;
PSZ           szEStringLoad;
PSZ           szEDllLoad;
PSZ           szEStringNull;
PSZ           szTempSetupPath;

PSZ           szClassName;
PSZ           szUninstallDir;
PSZ           szTempDir;
PSZ           szOSTempDir;
PSZ           szFileIniUninstall;
PSZ           gszSharedFilename;

ULONG           ulOSType;
ULONG           dwScreenX;
ULONG           dwScreenY;

ULONG           gdwWhatToDo;

uninstallGen    ugUninstall;
diU             diUninstall;

//HINSTANCE hInstance, HINSTANCE hPrevInstance, PSZ lpszCmdLine, int nCmdShow
int APIENTRY main(int argc, PSZ lpszCmdLine)
{
  /***********************************************************************/
  /* HANDLE hInstance;       handle for this instance                    */
  /* HANDLE hPrevInstance;   handle for possible previous instances      */
  /* PSZ  lpszCmdLine;     long pointer to exec command line           */
  /* int    nCmdShow;        Show code for main window display           */
  /***********************************************************************/

  PQMSG   qmsg;
  char  szBuf[MAX_BUF];

  hab = WinInitialize(0);

//  if(!hPrevInstance)
//  {
    hInst = GetModuleHandle(NULL);
    if(Initialize(hInst))
    {
      PostQuitMessage(1);
    }
    else if(!InitApplication(hInst))
    {
      char szEFailed[MAX_BUF];

      if(NS_LoadString(hInst, IDS_ERROR_FAILED, szEFailed, MAX_BUF) == WIZ_OK)
      {
        wsprintf(szBuf, szEFailed, "InitApplication().");
        PrintError(szBuf, ERROR_CODE_SHOW);
      }
      PostQuitMessage(1);
    }
    else if(ParseUninstallIni(lpszCmdLine))
    {
      PostQuitMessage(1);
    }
    else
    {
      if(diUninstall.bShowDialog == TRUE)
        hDlgUninstall = InstantiateDialog(hWndMain, DLG_UNINSTALL, diUninstall.szTitle, DlgProcUninstall);
      else
        ParseAllUninstallLogs();
    }
//  }

    while(WinGetMessage(hab, &qmsg, 0, 0, 0))
          WinDispatchMessage(hab, &qmsg);


  /* Do clean up before exiting from the application */
  DeInitialize();

  //return(qmsg.mp1);
  return 1;
} /*  End of main */

