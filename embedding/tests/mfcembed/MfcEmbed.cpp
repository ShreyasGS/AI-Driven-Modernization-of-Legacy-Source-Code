/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 *   Chak Nanga <chak@netscape.com> 
 *   Conrad Carlen <ccarlen@netscape.com> 
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

// File Overview....
//
// The typical MFC app, frame creation code + AboutDlg handling
//
// NS_InitEmbedding() is called in InitInstance()
// 
// NS_TermEmbedding() is called in ExitInstance()
// ExitInstance() also takes care of cleaning up of
// multiple browser frame windows on app exit
//
// Code to handle the creation of a new browser window

// Next suggested file to look at : BrowserFrm.cpp

// Local Includes
#include "stdafx.h"
#include "MfcEmbed.h"
#include "BrowserFrm.h"
#include "EditorFrm.h"
#include "winEmbedFileLocProvider.h"
#include "ProfileMgr.h"
#include "BrowserImpl.h"
#include "nsIWindowWatcher.h"
#include "plstr.h"
#include "Preferences.h"
#include "nsCRT.h"
#include <io.h>
#include <fcntl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// this is for overriding the Mozilla default PromptService component
#include "PromptService.h"
#define kComponentsLibname "mfcEmbedComponents.dll"
#define NS_PROMPTSERVICE_CID \
 {0xa2112d6a, 0x0e28, 0x421f, {0xb4, 0x6a, 0x25, 0xc0, 0xb3, 0x8, 0xcb, 0xd0}}
static NS_DEFINE_CID(kPromptServiceCID, NS_PROMPTSERVICE_CID);

// this is for overriding the Mozilla default HelperAppLauncherDialog
#include "HelperAppService.h"
#define NS_HELPERAPPLAUNCHERDIALOG_CID \
    {0xf68578eb, 0x6ec2, 0x4169, {0xae, 0x19, 0x8c, 0x62, 0x43, 0xf0, 0xab, 0xe1}}
static NS_DEFINE_CID(kHelperAppLauncherDialogCID, NS_HELPERAPPLAUNCHERDIALOG_CID);

BEGIN_MESSAGE_MAP(CMfcEmbedApp, CWinApp)
	//{{AFX_MSG_MAP(CMfcEmbedApp)
	ON_COMMAND(ID_NEW_BROWSER, OnNewBrowser)
	ON_COMMAND(ID_NEW_EDITORWINDOW, OnNewEditor)
	ON_COMMAND(ID_MANAGE_PROFILES, OnManageProfiles)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_EDIT_PREFERENCES, OnEditPreferences)
	// NOTE - the ClassWizard will add and remove mapping macros here.
	//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CMfcEmbedApp::CMfcEmbedApp() :
    m_ProfileMgr(NULL)
{
    mRefCnt = 1; // Start at one - nothing is going to addref this object

    m_strHomePage = "";

    m_iStartupPage = 0; 

    m_bChrome = FALSE;
}

CMfcEmbedApp theApp;

BOOL CMfcEmbedApp::IsCmdLineSwitch(const char *pSwitch, BOOL bRemove)
{
    //  Search for the switch in the command line.
    //  Don't take it out of m_lpCmdLine by default
    char *pFound = PL_strcasestr(m_lpCmdLine, pSwitch);
    if(pFound == NULL ||
        // Switch must be at beginning of command line
        // or have a space in front of it to avoid
        // mangling filenames
        ( (pFound != m_lpCmdLine) &&
          *(pFound-1) != ' ' ) ) 
    {
        return(FALSE);
    }

    if (bRemove) 
    {
        // remove the flag from the command line
        char *pTravEnd = pFound + strlen(pSwitch);
        char *pTraverse = pFound;

        *pTraverse = *pTravEnd;
        while(*pTraverse != '\0')   
        {
            pTraverse++;
            pTravEnd++;
            *pTraverse = *pTravEnd;
        }
    }

    return(TRUE);
}

void CMfcEmbedApp::ParseCmdLine()
{
    // Show Debug Console?
    if(IsCmdLineSwitch("-console"))
        ShowDebugConsole();
    if(IsCmdLineSwitch("-chrome"))
        m_bChrome = TRUE;
}

/* Some Gecko interfaces are implemented as components, automatically
   registered at application initialization. nsIPrompt is an example:
   the default implementation uses XUL, not native windows. Embedding
   apps can override the default implementation by implementing the
   nsIPromptService interface and registering a factory for it with
   the same CID and Contract ID as the default's.

   Note that this example implements the service in a separate DLL,
   replacing the default if the override DLL is present. This could
   also have been done in the same module, without a separate DLL.
   See the PowerPlant example for, well, an example.
*/
nsresult CMfcEmbedApp::OverrideComponents()
{
    nsresult rv = NS_OK;

    // replace Mozilla's default PromptService with our own, if the
    // expected override DLL is present
    HMODULE overlib = ::LoadLibrary(kComponentsLibname);
    if (overlib) {
        InitPromptServiceType InitLib;
        MakeFactoryType MakeFactory;
        InitLib = reinterpret_cast<InitPromptServiceType>(::GetProcAddress(overlib, kPromptServiceInitFuncName));
        MakeFactory = reinterpret_cast<MakeFactoryType>(::GetProcAddress(overlib, kPromptServiceFactoryFuncName));

        if (InitLib && MakeFactory) {
            InitLib(overlib);

            nsCOMPtr<nsIFactory> promptFactory;
            rv = MakeFactory(getter_AddRefs(promptFactory));
            if (NS_SUCCEEDED(rv))
                nsComponentManager::RegisterFactory(kPromptServiceCID,
                                                    "Prompt Service",
                                                    "@mozilla.org/embedcomp/prompt-service;1",
                                                    promptFactory,
                                                    PR_TRUE); // replace existing
        } else
          ::FreeLibrary(overlib);
    }

    // Replace Mozilla's helper app launcher dialog with our own
    overlib = ::LoadLibrary(kComponentsLibname);
    if (overlib) {
        InitHelperAppDlgType InitLib;
        MakeFactoryType MakeFactory;
        InitLib = reinterpret_cast<InitHelperAppDlgType>(::GetProcAddress(overlib, kHelperAppDlgInitFuncName));
        MakeFactory = reinterpret_cast<MakeFactoryType>(::GetProcAddress(overlib, kHelperAppDlgFactoryFuncName));

        if (InitLib && MakeFactory) {
            InitLib(overlib);

            nsCOMPtr<nsIFactory> helperAppDlgFactory;
            rv = MakeFactory(getter_AddRefs(helperAppDlgFactory));
            if (NS_SUCCEEDED(rv))
                nsComponentManager::RegisterFactory(kHelperAppLauncherDialogCID,
                                                    "Helper App Launcher Dialog",
                                                    "@mozilla.org/helperapplauncherdialog;1",
                                                    helperAppDlgFactory,
                                                    PR_TRUE); // replace existing
        } else
          ::FreeLibrary(overlib);
    }

    return rv;
}

void CMfcEmbedApp::ShowDebugConsole()
{
#ifdef _DEBUG
    // Show console only in debug mode

    if(! AllocConsole())
        return;

    // Redirect stdout to the console
    int hCrtOut = _open_osfhandle(
                (long) GetStdHandle(STD_OUTPUT_HANDLE),
                _O_TEXT);
    if(hCrtOut == -1)
        return;

    FILE *hfOut = _fdopen(hCrtOut, "w");
    if(hfOut != NULL)
    {
        // Setup for unbuffered I/O so the console 
        // output shows up right away
        *stdout = *hfOut;
        setvbuf(stdout, NULL, _IONBF, 0); 
    }

    // Redirect stderr to the console
    int hCrtErr = _open_osfhandle(
                (long) GetStdHandle(STD_ERROR_HANDLE),
                _O_TEXT);
    if(hCrtErr == -1)
        return;

    FILE *hfErr = _fdopen(hCrtErr, "w");
    if(hfErr != NULL)
    {
        // Setup for unbuffered I/O so the console 
        // output shows up right away
        *stderr = *hfErr;
        setvbuf(stderr, NULL, _IONBF, 0); 
    }
#endif
}

// Initialize our MFC application and also init
// the Gecko embedding APIs
// Note that we're also init'ng the profile switching
// code here
// Then, create a new BrowserFrame and load our
// default homepage
//
BOOL CMfcEmbedApp::InitInstance()
{
    ParseCmdLine();

	Enable3dControls();

	// Take a look at 
	// http://www.mozilla.org/projects/xpcom/file_locations.html
	// for more info on File Locations

    winEmbedFileLocProvider *provider = new winEmbedFileLocProvider("MfcEmbed");
    if(!provider)
    {
        ASSERT(FALSE);
        return FALSE;
    }

    nsresult rv;
    rv = NS_InitEmbedding(nsnull, provider);
    if(NS_FAILED(rv))
    {
        ASSERT(FALSE);
        return FALSE;
    }

    rv = OverrideComponents();
    if(NS_FAILED(rv))
    {
        ASSERT(FALSE);
        return FALSE;
    }


    rv = InitializeWindowCreator();
    if (NS_FAILED(rv))
    {
        ASSERT(FALSE);
        return FALSE;
    }

	if(!InitializeProfiles())
	{
        ASSERT(FALSE);
        NS_TermEmbedding();
		return FALSE;
	}


    if(!CreateHiddenWindow())
	{
        ASSERT(FALSE);
        NS_TermEmbedding();
		return FALSE;
	}

	// Create the first browser frame window
	OnNewBrowser();

	return TRUE;
}

CBrowserFrame* CMfcEmbedApp::CreateNewBrowserFrame(PRUint32 chromeMask,
												   PRInt32 x, PRInt32 y,
												   PRInt32 cx, PRInt32 cy,
                           PRBool bIsEditor,
												   PRBool bShowWindow)
{
	UINT resId = bIsEditor ? IDR_EDITOR : IDR_MAINFRAME;

	// Setup a CRect with the requested window dimensions
	CRect winSize(x, y, cx, cy);

	// Use the Windows default if all are specified as -1
	if(x == -1 && y == -1 && cx == -1 && cy == -1)
		winSize = CFrameWnd::rectDefault;

	// Load the window title from the string resource table
	CString strTitle;
	strTitle.LoadString(IDR_MAINFRAME);

	// Now, create the browser frame
	CBrowserFrame* pFrame = bIsEditor ? ( new  CEditorFrame(chromeMask) ) :
					    ( new  CBrowserFrame(chromeMask) );
	pFrame->SetEditable(bIsEditor);

	if (!pFrame->Create(NULL, strTitle, WS_OVERLAPPEDWINDOW, 
					winSize, NULL, MAKEINTRESOURCE(resId), 0L, NULL))
	{
		return NULL;
	}

	// load accelerator resource
	pFrame->LoadAccelTable(MAKEINTRESOURCE(IDR_MAINFRAME));

	// Show the window...
	if(bShowWindow)
	{
		pFrame->ShowWindow(SW_SHOW);
		pFrame->UpdateWindow();
	}

	// Add to the list of BrowserFrame windows
	m_FrameWndLst.AddHead(pFrame);

	return pFrame;
}

void CMfcEmbedApp::OnNewBrowser()
{
	CBrowserFrame *pBrowserFrame = CreateNewBrowserFrame();

	//Load the HomePage into the browser view
	if(pBrowserFrame && (GetStartupPageMode() == 1))
		pBrowserFrame->m_wndBrowserView.LoadHomePage();
}

void CMfcEmbedApp::OnNewEditor() 
{
    CEditorFrame *pEditorFrame = (CEditorFrame *)CreateNewBrowserFrame(nsIWebBrowserChrome::CHROME_ALL, 
                                    -1, -1, -1, -1,
                                    PR_TRUE,PR_TRUE);
    if (pEditorFrame)
    {
        pEditorFrame->m_wndBrowserView.OpenURL("about:blank");
        pEditorFrame->InitEditor();
    }
}

// This gets called anytime a BrowserFrameWindow is
// closed i.e. by choosing the "close" menu item from
// a window's system menu or by dbl clicking on the
// system menu box
// 
// Sends a WM_QUIT to the hidden window which results
// in ExitInstance() being called and the app is
// properly cleaned up and shutdown
//
void CMfcEmbedApp::RemoveFrameFromList(CBrowserFrame* pFrm, BOOL bCloseAppOnLastFrame/*= TRUE*/)
{
	POSITION pos = m_FrameWndLst.Find(pFrm);
	m_FrameWndLst.RemoveAt(pos);

	// Send a WM_QUIT msg. to the hidden window if we've
	// just closed the last browserframe window and
	// if the bCloseAppOnLastFrame is TRUE. This be FALSE
	// only in the case we're switching profiles
	// Without this the hidden window will stick around
	// i.e. the app will never die even after all the 
	// visible windows are gone.
	if(m_FrameWndLst.GetCount() == 0 && bCloseAppOnLastFrame)
		m_pMainWnd->PostMessage(WM_QUIT);
}

int CMfcEmbedApp::ExitInstance()
{
	// When File/Exit is chosen and if the user
	// has opened multiple browser windows shut all
	// of them down properly before exiting the app

	CBrowserFrame* pBrowserFrame = NULL;

	POSITION pos = m_FrameWndLst.GetHeadPosition();
	while( pos != NULL )
	{
		pBrowserFrame = (CBrowserFrame *) m_FrameWndLst.GetNext(pos);
		if(pBrowserFrame)
		{
			pBrowserFrame->ShowWindow(false);
			pBrowserFrame->DestroyWindow();
		}
	}
	m_FrameWndLst.RemoveAll();

    if (m_pMainWnd)
        m_pMainWnd->DestroyWindow();

    delete m_ProfileMgr;

	NS_TermEmbedding();

	return 1;
}

BOOL CMfcEmbedApp::OnIdle(LONG lCount)
{
	CWinApp::OnIdle(lCount);

	return FALSE;
}

void CMfcEmbedApp::OnManageProfiles()
{
    m_ProfileMgr->DoManageProfilesDialog(PR_FALSE);
}

void CMfcEmbedApp::OnEditPreferences()
{
    CPreferences prefs(_T("Preferences"));
    
    prefs.m_startupPage.m_iStartupPage = m_iStartupPage;
    prefs.m_startupPage.m_strHomePage = m_strHomePage;   

    if(prefs.DoModal() == IDOK)
    {
        // Update our member vars with these new pref values
        m_iStartupPage = prefs.m_startupPage.m_iStartupPage;
        m_strHomePage = prefs.m_startupPage.m_strHomePage;

        // Save these changes to disk now
        nsresult rv;
        nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
        if (NS_SUCCEEDED(rv)) 
        {
            prefs->SetIntPref("browser.startup.page", m_iStartupPage);
            rv = prefs->SetCharPref("browser.startup.homepage", m_strHomePage);
            if (NS_SUCCEEDED(rv))
                rv = prefs->SavePrefFile(nsnull);
        }
        else
		    NS_ASSERTION(PR_FALSE, "Could not get preferences service");
    }
}

BOOL CMfcEmbedApp::InitializeProfiles()
{
    m_ProfileMgr = new CProfileMgr;
    if (!m_ProfileMgr)
        return FALSE;

	  nsresult rv;
    nsCOMPtr<nsIObserverService> observerService = 
             do_GetService("@mozilla.org/observer-service;1", &rv);
    observerService->AddObserver(this, "profile-approve-change", PR_TRUE);
    observerService->AddObserver(this, "profile-change-teardown", PR_TRUE);
    observerService->AddObserver(this, "profile-after-change", PR_TRUE);

    m_ProfileMgr->StartUp();

	return TRUE;
}

// When the profile switch happens, all open browser windows need to be 
// closed. 
// In order for that not to kill off the app, we just make the MFC app's 
// mainframe be an invisible window which doesn't get closed on profile 
// switches
BOOL CMfcEmbedApp::CreateHiddenWindow()
{
	CFrameWnd *hiddenWnd = new CFrameWnd;
	if(!hiddenWnd)
		return FALSE;

    RECT bounds = { -10010, -10010, -10000, -10000 };
    hiddenWnd->Create(NULL, "main", WS_DISABLED, bounds, NULL, NULL, 0, NULL);
    m_pMainWnd = hiddenWnd;

	return TRUE;
}

nsresult CMfcEmbedApp::InitializePrefs()
{
   nsresult rv;
   nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
   if (NS_SUCCEEDED(rv)) {	  

		// We are using the default prefs from mozilla. If you were
		// disributing your own, this would be done simply by editing
		// the default pref files.
		
		PRBool inited;
		rv = prefs->GetBoolPref("mfcbrowser.prefs_inited", &inited);
		if (NS_FAILED(rv) || !inited)
		{
            m_iStartupPage = 1;
            m_strHomePage = "http://www.mozilla.org/projects/embedding";

            prefs->SetIntPref("browser.startup.page", m_iStartupPage);
            prefs->SetCharPref("browser.startup.homepage", m_strHomePage);
            prefs->SetIntPref("font.size.variable.x-western", 16);
            prefs->SetIntPref("font.size.fixed.x-western", 13);
            rv = prefs->SetBoolPref("mfcbrowser.prefs_inited", PR_TRUE);
            if (NS_SUCCEEDED(rv))
                rv = prefs->SavePrefFile(nsnull);
        }
        else
        {
            // The prefs are present, read them in

            prefs->GetIntPref("browser.startup.page", &m_iStartupPage);

            CString strBuf;
            char *pBuf = strBuf.GetBuffer(_MAX_PATH);
            prefs->CopyCharPref("browser.startup.homepage", &pBuf);
            strBuf.ReleaseBuffer(-1);
            if(pBuf)
                m_strHomePage = pBuf;
        }       
	}
	else
		NS_ASSERTION(PR_FALSE, "Could not get preferences service");
		
    return rv;
}


/* InitializeWindowCreator creates and hands off an object with a callback
   to a window creation function. This will be used by Gecko C++ code
   (never JS) to create new windows when no previous window is handy
   to begin with. This is done in a few exceptional cases, like PSM code.
   Failure to set this callback will only disable the ability to create
   new windows under these circumstances. */
nsresult CMfcEmbedApp::InitializeWindowCreator()
{
  // give an nsIWindowCreator to the WindowWatcher service
  nsCOMPtr<nsIWindowCreator> windowCreator(NS_STATIC_CAST(nsIWindowCreator *, this));
  if (windowCreator) {
    nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService("@mozilla.org/embedcomp/window-watcher;1"));
    if (wwatch) {
      wwatch->SetWindowCreator(windowCreator);
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

// ---------------------------------------------------------------------------
//  CMfcEmbedApp : nsISupports
// ---------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS3(CMfcEmbedApp, nsIObserver, nsIWindowCreator, nsISupportsWeakReference);

// ---------------------------------------------------------------------------
//  CMfcEmbedApp : nsIObserver
// ---------------------------------------------------------------------------

// Mainly needed to support "on the fly" profile switching

NS_IMETHODIMP CMfcEmbedApp::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
    nsresult rv = NS_OK;
    
    if (nsCRT::strcmp(aTopic, "profile-approve-change") == 0)
    {
        // Ask the user if they want to
        int result = MessageBox(NULL, "Do you want to close all windows in order to switch the profile?", "Confirm", MB_YESNO | MB_ICONQUESTION);
        if (result != IDYES)
        {
            nsCOMPtr<nsIProfileChangeStatus> status = do_QueryInterface(aSubject);
            NS_ENSURE_TRUE(status, NS_ERROR_FAILURE);
            status->VetoChange();
        }
    }
    else if (nsCRT::strcmp(aTopic, "profile-change-teardown") == 0)
    {
        // Close all open windows. Alternatively, we could just call CBrowserWindow::Stop()
        // on each. Either way, we have to stop all network activity on this phase.
        
	    POSITION pos = m_FrameWndLst.GetHeadPosition();
	    while( pos != NULL )
	    {
		    CBrowserFrame *pBrowserFrame = (CBrowserFrame *) m_FrameWndLst.GetNext(pos);
		    if(pBrowserFrame)
		    {
			    pBrowserFrame->ShowWindow(false);

				// Passing in FALSE below so that we do not
				// kill the main app during a profile switch
				RemoveFrameFromList(pBrowserFrame, FALSE);

				pBrowserFrame->DestroyWindow();
		    }
	    }
    }
    else if (nsCRT::strcmp(aTopic, "profile-after-change") == 0)
    {
        InitializePrefs(); // In case we have just switched to a newly created profile.
        
        // Only make a new browser window on a switch. This also gets
        // called at start up and we already make a window then.
        if (!nsCRT::strcmp(someData, NS_LITERAL_STRING("switch").get()))      
            OnNewBrowser();
    }
    return rv;
}

// ---------------------------------------------------------------------------
//  CMfcEmbedApp : nsIWindowCreator
// ---------------------------------------------------------------------------
NS_IMETHODIMP CMfcEmbedApp::CreateChromeWindow(nsIWebBrowserChrome *parent,
                                               PRUint32 chromeFlags,
                                               nsIWebBrowserChrome **_retval)
{
  // XXX we're ignoring the "parent" parameter
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = 0;

  CBrowserFrame *pBrowserFrame = CreateNewBrowserFrame(chromeFlags);
  if(pBrowserFrame) {
    *_retval = NS_STATIC_CAST(nsIWebBrowserChrome *, pBrowserFrame->GetBrowserImpl());
    NS_ADDREF(*_retval);
  }
  return NS_OK;
}

// AboutDlg Stuff

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// Show the AboutDlg
void CMfcEmbedApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}
