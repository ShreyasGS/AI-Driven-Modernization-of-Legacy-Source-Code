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

#ifndef _DIALOGS_H_
#define _DIALOGS_H_

#include "resource.h"

class CPromptDialog : public CDialog
{
public:
    CPromptDialog(CWnd* pParent, const char* pTitle, const char* pText,
                  const char* pInitPromptText,
                  BOOL bHasCheck, const char* pCheckText, int initCheckVal);
	
	// Dialog Data
    //{{AFX_DATA(CPromptDialog)
    enum { IDD = IDD_PROMPT_DIALOG };
    CString m_csPromptAnswer;
    //}}AFX_DATA

    CString m_csDialogTitle;
	CString m_csPromptText;
	BOOL m_bHasCheckBox;
	CString m_csCheckBoxText;
	int m_bCheckBoxValue;
    
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPromptDialog)
	protected:
    virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CPromptDialog)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

class CPromptPasswordDialog : public CDialog
{
public:
    CPromptPasswordDialog(CWnd* pParent, const char* pTitle, const char* pText,
                          const char* pInitPasswordText,
                          BOOL bHasCheck, const char* pCheckText, int initCheckVal);
	
	// Dialog Data
    //{{AFX_DATA(CPromptPasswordDialog)
    enum { IDD = IDD_PROMPT_PASSWORD_DIALOG };
    //}}AFX_DATA

    CString m_csDialogTitle;
	CString m_csPromptText;
	CString m_csPassword;
	BOOL m_bHasCheckBox;
	CString m_csCheckBoxText;
	int m_bCheckBoxValue;

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPromptPasswordDialog)		
	protected:
    virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CPromptPasswordDialog)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

class CPromptUsernamePasswordDialog : public CDialog
{
public:
    CPromptUsernamePasswordDialog(CWnd* pParent, const char* pTitle, const char* pText,
                                  const char* pInitUsername, const char* pInitPassword, 
		                          BOOL bHasCheck, const char* pCheckText, int initCheckVal);
	
	// Dialog Data
    //{{AFX_DATA(CPromptUsernamePasswordDialog)
    enum { IDD = IDD_PROMPT_USERPASS_DIALOG };
    //}}AFX_DATA

    CString m_csDialogTitle;
	CString m_csPromptText;
	CString m_csUserNameLabel;
	CString m_csPasswordLabel;
	CString m_csPassword;
	CString m_csUserName;
	BOOL m_bHasCheckBox;
	CString m_csCheckBoxText;
	int m_bCheckBoxValue;

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPromptUsernamePasswordDialog)	
	protected:
    virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CPromptUsernamePasswordDialog)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

#endif //_DIALOG_H_
