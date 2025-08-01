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
 *   Ashish Bhatt <ashishbhatt@netscape.com> 
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
// Test cases for the nsIClipBoardCommand Interface

#include "stdafx.h"
#include "QaUtils.h"
#include <stdio.h>
#include "nsIFile.h"

CNsIFile::CNsIFile()
{
}


CNsIFile::~CNsIFile()
{
}

void CNsIFile::OnStartTests(UINT nMenuID)
{
	// Calls  all or indivdual test cases on the basis of the 
	// option selected from menu.
   nsCOMPtr<nsILocalFile> theTestFile(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID));
   nsCOMPtr<nsILocalFile> theFileOpDir(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID));

    if (!theTestFile)
 	{
		QAOutput("File object doesn't exist. No File tests performed.", 2);
		return;
	}
	if (!theFileOpDir)
 	{
		QAOutput("File object doesn't exist. No File tests performed.", 2);
		return;
	}

	QAOutput("Begin nsIFile tests.", 2);

	switch(nMenuID)
	{
		
		case ID_INTERFACES_NSIFILE_RUNALLTESTS	:
			RunAllTests(theTestFile,theFileOpDir);
			break ;
		case ID_INTERFACES_NSIFILE_INITWITHPATH	:
			InitWithPathTest(theTestFile);
			break ;
		case ID_INTERFACES_NSIFILE_APPENDRELATICEPATH :
			AppendRelativePathTest(theTestFile);
			break ;
		case ID_INTERFACES_NSIFILE_EXISTS :
			FileCreateTest(theTestFile);
			break ;
		case ID_INTERFACES_NSIFILE_CREATE :
			FileExistsTest(theTestFile);
			break ;
		case ID_INTERFACES_NSIFILE_COPYTO :
			FileCopyTest(theTestFile, theFileOpDir);	
			break ;
		case ID_INTERFACES_NSIFILE_MOVETO :
			FileMoveTest(theTestFile, theFileOpDir);	
			break ;

	}

}

// ***********************************************************************
// ************************** Interface Tests ****************************
// ***********************************************************************

// nsIFile:

void CNsIFile::RunAllTests(nsILocalFile *theTestFile, nsILocalFile *theFileOpDir)
{
	InitWithPathTest(theTestFile);
	AppendRelativePathTest(theTestFile);
	FileCreateTest(theTestFile);
	FileExistsTest(theTestFile);

	FileCopyTest(theTestFile, theFileOpDir);	

	FileMoveTest(theTestFile, theFileOpDir);	
}


// ***********************************************************************
// Individual nsIFile tests

void CNsIFile::InitWithPathTest(nsILocalFile *theTestFile)
{
	rv = theTestFile->InitWithPath("c:\\temp\\");
	RvTestResult(rv, "InitWithPath() test (initializing file path)", 2);
}

void CNsIFile::AppendRelativePathTest(nsILocalFile *theTestFile)
{
	rv = theTestFile->AppendRelativePath("myFile.txt");
	RvTestResult(rv, "AppendRelativePath() test (append file to the path)", 2);
}

void CNsIFile::FileCreateTest(nsILocalFile *theTestFile)
{
	PRBool exists =  PR_TRUE;
	rv = theTestFile->InitWithPath("c:\\temp\\");
	rv = theTestFile->AppendRelativePath("myFile.txt");

	rv = theTestFile->Exists(&exists);
	if (!exists)
	{
		QAOutput("File doesn't exist. We'll try creating it.", 2);
		rv = theTestFile->Create(nsIFile::NORMAL_FILE_TYPE, 0777);
		RvTestResult(rv, " File Create() test ('myFile.txt')", 2);
	}
	else
		QAOutput("File already exists (myFile.txt). We won't create it.", 2);
}

void CNsIFile::FileExistsTest(nsILocalFile *theTestFile)
{
	PRBool exists =  PR_TRUE;
	rv = theTestFile->InitWithPath("c:\\temp\\");
	rv = theTestFile->AppendRelativePath("myFile.txt");

	rv = theTestFile->Exists(&exists);

	if (!exists)
		QAOutput("Exists() test Failed. File (myFile.txt) doesn't exist.", 2);
	else
		QAOutput("Exists() test Passed. File (myFile.txt) exists.", 2);

}

void CNsIFile::FileCopyTest(nsILocalFile *theTestFile, nsILocalFile *theFileOpDir)
{
	
	PRBool exists =  PR_TRUE;

	QAOutput("Start File Copy test.", 2);

	rv = theFileOpDir->InitWithPath("c:\\temp\\");
	if (NS_FAILED(rv))
		QAOutput("The target dir wasn't found.", 2);
	else
		QAOutput("The target dir was found.", 2);

	rv = theTestFile->InitWithPath("c:\\temp\\myFile.txt");
	if (NS_FAILED(rv))
		QAOutput("The path wasn't found.", 2);
	else
		QAOutput("The path was found.", 2);

	rv = theTestFile->CopyTo(theFileOpDir, "myFile2.txt");
	RvTestResult(rv, "rv CopyTo() test", 2);

	rv = theTestFile->InitWithPath("c:\\temp\\myFile2.txt");
	rv = theTestFile->Exists(&exists);
	if (!exists)
		QAOutput("File didn't copy. CopyTo() test Failed.", 2);
	else
		QAOutput("File copied. CopyTo() test Passed.", 2);
}

void CNsIFile::FileMoveTest(nsILocalFile *theTestFile, nsILocalFile *theFileOpDir)
{
	PRBool exists =  PR_TRUE;

	QAOutput("Start File Move test.", 2);

	rv = theFileOpDir->InitWithPath("c:\\Program Files\\");
	if (NS_FAILED(rv))
		QAOutput("The target dir wasn't found.", 2);

	rv = theTestFile->InitWithPath("c:\\temp\\myFile2.txt");
	if (NS_FAILED(rv))
		QAOutput("The path wasn't found.", 2);

	rv = theTestFile->MoveTo(theFileOpDir, "myFile2.txt");
	RvTestResult(rv, "MoveTo() test", 2);

	rv = theTestFile->InitWithPath("c:\\Program Files\\myFile2.txt");
	rv = theTestFile->Exists(&exists);
	if (!exists)
		QAOutput("File wasn't moved. MoveTo() test Failed.", 2);
	else
		QAOutput("File was moved. MoveTo() test Passed.", 2);
}
