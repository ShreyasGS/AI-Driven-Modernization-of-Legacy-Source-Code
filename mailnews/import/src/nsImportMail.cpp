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

/*
*/


#include "prthread.h"
#include "prprf.h"
#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsISupportsArray.h"

#include "nsIImportMail.h"
#include "nsIImportGeneric.h"
#include "nsISupportsPrimitives.h"
#include "nsIImportMailboxDescriptor.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsIProxyObjectManager.h"
#include "nsXPIDLString.h"

#include "nsIFileSpec.h"

#include "nsIMsgAccountManager.h"
#include "nsIMessengerMigrator.h"
#include "nsIMsgMailSession.h"
#include "nsMsgBaseCID.h"
#include "nsIMsgFolder.h"
#include "nsImportStringBundle.h"
#include "nsIStringBundle.h"
#include "nsTextFormatter.h"

#include "nsIImportService.h"

#include "ImportDebug.h"

#define IMPORT_MSGS_URL       "chrome://messenger/locale/importMsgs.properties"

static NS_DEFINE_CID(kMsgAccountCID, NS_MSGACCOUNT_CID);
static NS_DEFINE_CID(kMsgIdentityCID, NS_MSGIDENTITY_CID);
static NS_DEFINE_CID(kMsgBiffManagerCID, NS_MSGBIFFMANAGER_CID);
static NS_DEFINE_CID(kProxyObjectManagerCID, NS_PROXYEVENT_MANAGER_CID);
static NS_DEFINE_CID(kSupportsWStringCID, NS_SUPPORTS_WSTRING_CID);

////////////////////////////////////////////////////////////////////////

PR_STATIC_CALLBACK( void) ImportMailThread( void *stuff);

static nsCOMPtr<nsIImportService>	gService;

class ImportThreadData;

class nsImportGenericMail : public nsIImportGeneric
{
public:

	nsImportGenericMail();
	virtual ~nsImportGenericMail();
	
	NS_DECL_ISUPPORTS

	/* nsISupports GetData (in string dataId); */
	NS_IMETHOD GetData(const char *dataId, nsISupports **_retval);

	NS_IMETHOD SetData(const char *dataId, nsISupports *pData);

	NS_IMETHOD GetStatus( const char *statusKind, PRInt32 *_retval);

	/* boolean WantsProgress (); */
	NS_IMETHOD WantsProgress(PRBool *_retval);

    /* boolean BeginImport (in nsISupportsWString successLog, in nsISupportsWString errorLog, in boolean isAddrLocHome); */
    NS_IMETHODIMP BeginImport(nsISupportsWString *successLog, nsISupportsWString *errorLog, PRBool isAddrLocHome, PRBool *_retval) ;

	/* boolean ContinueImport (); */
	NS_IMETHOD ContinueImport(PRBool *_retval);

	/* long GetProgress (); */
	NS_IMETHOD GetProgress(PRInt32 *_retval);

	/* void CancelImport (); */
	NS_IMETHOD CancelImport(void);

private:
	PRBool	CreateFolder( nsIMsgFolder **ppFolder);
	void	GetDefaultMailboxes( void);
	void	GetDefaultLocation( void);
	void	GetDefaultDestination( void);
	void	GetMailboxName( PRUint32 index, nsISupportsWString *pStr);

public:
	static void	SetLogs( nsString& success, nsString& error, nsISupportsWString *pSuccess, nsISupportsWString *pError);
	static void ReportError( PRInt32 id, const PRUnichar *pName, nsString *pStream);

private:
	PRUnichar *			m_pName;	// module name that created this interface
	nsIMsgFolder *		m_pDestFolder;
	PRBool				m_deleteDestFolder;
	PRBool				m_createdFolder;
	nsIFileSpec *		m_pSrcLocation;
	PRBool				m_gotLocation;
	PRBool				m_found;
	PRBool				m_userVerify;
	nsIImportMail *		m_pInterface;
	nsISupportsArray *	m_pMailboxes;
	nsISupportsWString *m_pSuccessLog;
	nsISupportsWString *m_pErrorLog;
	PRUint32			m_totalSize;
	PRBool				m_doImport;
	ImportThreadData *	m_pThreadData;
};

class ImportThreadData {
public:
	PRBool					driverAlive;
	PRBool					threadAlive;
	PRBool					abort;
	PRBool					fatalError;
	PRUint32				currentTotal;
	PRUint32				currentSize;
	nsIMsgFolder *			destRoot;
	PRBool					ownsDestRoot;
	nsISupportsArray *		boxes;
	nsIImportMail *			mailImport;
	nsISupportsWString *	successLog;
	nsISupportsWString *	errorLog;
	PRUint32				currentMailbox;

	ImportThreadData();
	~ImportThreadData();
	void DriverDelete();
	void ThreadDelete();
	void DriverAbort();
};


nsresult NS_NewGenericMail(nsIImportGeneric** aImportGeneric)
{
    NS_PRECONDITION(aImportGeneric != nsnull, "null ptr");
    if (! aImportGeneric)
        return NS_ERROR_NULL_POINTER;
	
	nsImportGenericMail *pGen = new nsImportGenericMail();

	if (pGen == nsnull)
		return NS_ERROR_OUT_OF_MEMORY;

	NS_ADDREF( pGen);
	nsresult rv = pGen->QueryInterface( NS_GET_IID(nsIImportGeneric), (void **)aImportGeneric);
	NS_RELEASE( pGen);
    
    return( rv);
}

nsImportGenericMail::nsImportGenericMail()
{
    NS_INIT_REFCNT();
	m_pSrcLocation = nsnull;
	m_found = PR_FALSE;
	m_userVerify = PR_FALSE;
	m_gotLocation = PR_FALSE;
	m_pInterface = nsnull;
	m_pMailboxes = nsnull;
	m_pSuccessLog = nsnull;
	m_pErrorLog = nsnull;
	m_totalSize = 0;
	m_doImport = PR_FALSE;
	m_pThreadData = nsnull;

	m_pDestFolder = nsnull;
	m_deleteDestFolder = PR_FALSE;
	m_createdFolder = PR_FALSE;
	m_pName = nsnull;

}


nsImportGenericMail::~nsImportGenericMail()
{
	if (m_pThreadData) {
		m_pThreadData->DriverAbort();
		m_pThreadData = nsnull;
	}
	
	if (m_pName)
		nsCRT::free( m_pName);

	NS_IF_RELEASE( m_pDestFolder);
	NS_IF_RELEASE( m_pSrcLocation);
	NS_IF_RELEASE( m_pInterface);
	NS_IF_RELEASE( m_pMailboxes);
	NS_IF_RELEASE( m_pSuccessLog);
	NS_IF_RELEASE( m_pErrorLog);
}



NS_IMPL_THREADSAFE_ISUPPORTS1(nsImportGenericMail, nsIImportGeneric)


NS_IMETHODIMP nsImportGenericMail::GetData(const char *dataId, nsISupports **_retval)
{
	nsresult rv = NS_OK;

    NS_PRECONDITION(_retval != nsnull, "null ptr");
	if (!_retval)
		return NS_ERROR_NULL_POINTER;
	
	*_retval = nsnull;
	if (!nsCRT::strcasecmp( dataId, "mailInterface")) {
		*_retval = m_pInterface;
		NS_IF_ADDREF( m_pInterface);
	}
	
	if (!nsCRT::strcasecmp( dataId, "mailBoxes")) {
		if (!m_pMailboxes)
			GetDefaultMailboxes();
		*_retval = m_pMailboxes;
		NS_IF_ADDREF( m_pMailboxes);
	}

	if (!nsCRT::strcasecmp( dataId, "mailLocation")) {
		if (!m_pSrcLocation)
			GetDefaultLocation();
		*_retval = m_pSrcLocation;
		NS_IF_ADDREF( m_pSrcLocation);
	}
	
	if (!nsCRT::strcasecmp( dataId, "mailDestination")) {
		if (!m_pDestFolder)
			GetDefaultDestination();
		*_retval = m_pDestFolder;
		NS_IF_ADDREF( m_pDestFolder);
	}
	
	if (!nsCRT::strcasecmp( dataId, "currentMailbox")) {
		// create an nsISupportsWString, get the current mailbox
		// name being imported and put it in the string
		nsCOMPtr<nsISupportsWString>	data;
		rv = nsComponentManager::CreateInstance( kSupportsWStringCID, nsnull, NS_GET_IID(nsISupportsWString), getter_AddRefs( data));
		if (NS_FAILED( rv))
			return( rv);
		if (m_pThreadData) {
			GetMailboxName( m_pThreadData->currentMailbox, data);
		}
		*_retval = data;
		NS_ADDREF( *_retval);
	}

	return( rv);
}

NS_IMETHODIMP nsImportGenericMail::SetData( const char *dataId, nsISupports *item)
{
    nsresult rv = NS_OK;
	NS_PRECONDITION(dataId != nsnull, "null ptr");
    if (!dataId)
        return NS_ERROR_NULL_POINTER;

	if (!nsCRT::strcasecmp( dataId, "mailInterface")) {
		NS_IF_RELEASE( m_pInterface);
		if (item)
			item->QueryInterface( NS_GET_IID(nsIImportMail), (void **) &m_pInterface);
	}
	if (!nsCRT::strcasecmp( dataId, "mailBoxes")) {
		NS_IF_RELEASE( m_pMailboxes);
		if (item)
			item->QueryInterface( NS_GET_IID(nsISupportsArray), (void **) &m_pMailboxes);
	}
	
	if (!nsCRT::strcasecmp( dataId, "mailLocation")) {
		NS_IF_RELEASE( m_pMailboxes);
		NS_IF_RELEASE( m_pSrcLocation);
		if (item)
			item->QueryInterface( NS_GET_IID(nsIFileSpec), (void **) &m_pSrcLocation);
	}
	
	if (!nsCRT::strcasecmp( dataId, "mailDestination")) {
		NS_IF_RELEASE( m_pDestFolder);
		if (item)
			item->QueryInterface( NS_GET_IID(nsIMsgFolder), (void **) &m_pDestFolder);
		m_deleteDestFolder = PR_FALSE;
	}
	
	if (!nsCRT::strcasecmp( dataId, "name")) {
		if (m_pName)
			nsCRT::free( m_pName);
		m_pName = nsnull;
		nsCOMPtr<nsISupportsWString> nameString;
		if (item) {
			item->QueryInterface( NS_GET_IID(nsISupportsWString), getter_AddRefs(nameString));
			rv = nameString->GetData(&m_pName);
		}
	}

	return rv;
}

NS_IMETHODIMP nsImportGenericMail::GetStatus( const char *statusKind, PRInt32 *_retval)
{
	NS_PRECONDITION(statusKind != nsnull, "null ptr");
	NS_PRECONDITION(_retval != nsnull, "null ptr");
	if (!statusKind || !_retval)
		return( NS_ERROR_NULL_POINTER);
	
	*_retval = 0;

	if (!nsCRT::strcasecmp( statusKind, "isInstalled")) {
		GetDefaultLocation();
		*_retval = (PRInt32) m_found;
	}

	if (!nsCRT::strcasecmp( statusKind, "canUserSetLocation")) {
		GetDefaultLocation();
		*_retval = (PRInt32) m_userVerify;
	}

	return( NS_OK);
}


void nsImportGenericMail::GetDefaultLocation( void)
{
	if (!m_pInterface)
		return;
	
	if (m_pSrcLocation && m_gotLocation)
		return;

	m_gotLocation = PR_TRUE;

	nsIFileSpec *	pLoc = nsnull;
	m_pInterface->GetDefaultLocation( &pLoc, &m_found, &m_userVerify);
	if (!m_pSrcLocation)
		m_pSrcLocation = pLoc;
	else {
		NS_IF_RELEASE( pLoc);
	}
}

void nsImportGenericMail::GetDefaultMailboxes( void)
{
	if (!m_pInterface || m_pMailboxes || !m_pSrcLocation)
		return;
	
	m_pInterface->FindMailboxes( m_pSrcLocation, &m_pMailboxes);
}

void nsImportGenericMail::GetDefaultDestination( void)
{
	if (m_pDestFolder)
		return;
	if (!m_pInterface)
		return;

	nsIMsgFolder *	rootFolder;
	m_deleteDestFolder = PR_FALSE;
	m_createdFolder = PR_FALSE;
	if (CreateFolder( &rootFolder)) {
		m_pDestFolder = rootFolder;
		m_deleteDestFolder = PR_TRUE;
		m_createdFolder = PR_TRUE;
		return;
	}
	IMPORT_LOG0( "*** FAILED to create a default import destination\n");
}

NS_IMETHODIMP nsImportGenericMail::WantsProgress(PRBool *_retval)
{
	NS_PRECONDITION(_retval != nsnull, "null ptr");
    if (!_retval)
        return NS_ERROR_NULL_POINTER;
	
	if (m_pThreadData) {
		m_pThreadData->DriverAbort();
		m_pThreadData = nsnull;
	}

	if (!m_pMailboxes) {
		GetDefaultLocation();
		GetDefaultMailboxes();
	}

	if (!m_pDestFolder) {
		GetDefaultDestination();
	}

	PRUint32		totalSize = 0;
	PRBool			result = PR_FALSE;

	if (m_pMailboxes) {
		nsISupports *	pSupports;
		PRUint32		i;
		PRBool			import;
		PRUint32		count = 0;
		nsresult		rv;
		PRUint32		size;

		rv = m_pMailboxes->Count( &count);
		
		for (i = 0; i < count; i++) {
			pSupports = m_pMailboxes->ElementAt( i);
			if (pSupports) {
				nsCOMPtr<nsISupports> interface( dont_AddRef( pSupports));
				nsCOMPtr<nsIImportMailboxDescriptor> box( do_QueryInterface( pSupports));
				if (box) {
					import = PR_FALSE;
					size = 0;
					rv = box->GetImport( &import);
					if (import) {
						rv = box->GetSize( &size);
						result = PR_TRUE;
					}
					totalSize += size;
				}
			}
		}

		m_totalSize = totalSize;
	}
	
	m_doImport = result;

	*_retval = result;	

	return( NS_OK);
}

void nsImportGenericMail::GetMailboxName( PRUint32 index, nsISupportsWString *pStr)
{
	if (m_pMailboxes) {
		nsISupports *pSupports = m_pMailboxes->ElementAt( index);
		if (pSupports) {
			nsCOMPtr<nsISupports> iFace( dont_AddRef( pSupports));
			nsCOMPtr<nsIImportMailboxDescriptor> box( do_QueryInterface( pSupports));
			if (box) {
				PRUnichar *pName = nsnull;
				box->GetDisplayName( &pName);
				if (pName) {
					pStr->SetData( pName);
					nsCRT::free( pName);
				}
			}
		}
	}		
}

NS_IMETHODIMP nsImportGenericMail::BeginImport(nsISupportsWString *successLog, nsISupportsWString *errorLog, PRBool isAddrLocHome, PRBool *_retval)
{
	NS_PRECONDITION(_retval != nsnull, "null ptr");
    if (!_retval)
        return NS_ERROR_NULL_POINTER;

	nsString	success;
	nsString	error;

	if (!m_doImport) {
		nsImportStringBundle::GetStringByID( IMPORT_NO_MAILBOXES, success);
		SetLogs( success, error, successLog, errorLog);			
		*_retval = PR_TRUE;
		return( NS_OK);		
	}
	
	if (!m_pInterface || !m_pMailboxes) {
		IMPORT_LOG0( "*** Something is not set properly, interface, mailboxes, or destination.\n");
		nsImportStringBundle::GetStringByID( IMPORT_ERROR_MB_NOTINITIALIZED, error);
		SetLogs( success, error, successLog, errorLog);
		*_retval = PR_FALSE;
		return( NS_OK);
	}
	
	if (!m_pDestFolder) {
		IMPORT_LOG0( "*** No import destination.\n");
		nsImportStringBundle::GetStringByID( IMPORT_ERROR_MB_NODESTFOLDER, error);
		SetLogs( success, error, successLog, errorLog);
		*_retval = PR_FALSE;
		return( NS_OK);
	}

	if (m_pThreadData) {
		m_pThreadData->DriverAbort();
		m_pThreadData = nsnull;
	}

	NS_IF_RELEASE( m_pSuccessLog);
	NS_IF_RELEASE( m_pErrorLog);
	m_pSuccessLog = successLog;
	m_pErrorLog = errorLog;
	NS_IF_ADDREF( m_pSuccessLog);
	NS_IF_ADDREF( m_pErrorLog);
	
	
	// kick off the thread to do the import!!!!
	m_pThreadData = new ImportThreadData();
	m_pThreadData->boxes = m_pMailboxes;
	NS_ADDREF( m_pMailboxes);
	m_pThreadData->mailImport = m_pInterface;
	NS_ADDREF( m_pInterface);
	m_pThreadData->errorLog = m_pErrorLog;
	NS_IF_ADDREF( m_pErrorLog);
	m_pThreadData->successLog = m_pSuccessLog;
	NS_IF_ADDREF( m_pSuccessLog);
	
	m_pThreadData->ownsDestRoot = m_deleteDestFolder;
	m_pThreadData->destRoot = m_pDestFolder;
	NS_IF_ADDREF( m_pDestFolder);


	PRThread *pThread = PR_CreateThread( PR_USER_THREAD, &ImportMailThread, m_pThreadData, 
									PR_PRIORITY_NORMAL, 
									PR_LOCAL_THREAD, 
									PR_UNJOINABLE_THREAD,
									0);
	if (!pThread) {
		m_pThreadData->ThreadDelete();
		m_pThreadData->abort = PR_TRUE;
		m_pThreadData->DriverAbort();
		m_pThreadData = nsnull;
		*_retval = PR_FALSE;
		nsImportStringBundle::GetStringByID( IMPORT_ERROR_MB_NOTHREAD, error);
		SetLogs( success, error, successLog, errorLog);
	}
	else
		*_retval = PR_TRUE;
					
	return( NS_OK);

}


NS_IMETHODIMP nsImportGenericMail::ContinueImport(PRBool *_retval)
{
    NS_PRECONDITION(_retval != nsnull, "null ptr");
    if (!_retval)
        return NS_ERROR_NULL_POINTER;
	
	*_retval = PR_TRUE;
	if (m_pThreadData) {
		if (m_pThreadData->fatalError)
			*_retval = PR_FALSE;
	}

	return( NS_OK);
}


NS_IMETHODIMP nsImportGenericMail::GetProgress(PRInt32 *_retval)
{
	// This returns the progress from the the currently
	// running import mail or import address book thread.
    NS_PRECONDITION(_retval != nsnull, "null ptr");
    if (!_retval)
        return NS_ERROR_NULL_POINTER;

	if (!m_pThreadData || !(m_pThreadData->threadAlive)) {
		*_retval = 100;				
		return( NS_OK);	
	}
	
	PRUint32 sz = 0;
	if (m_pThreadData->currentSize && m_pInterface) {
		if (NS_FAILED( m_pInterface->GetImportProgress( &sz)))
			sz = 0;
	}
	

	// *_retval = (PRInt32) (((PRUint32)(m_pThreadData->currentTotal + sz) * (PRUint32)100) / m_totalSize);

	if (m_totalSize) {
		PRFloat64	perc;
		perc = (PRFloat64) m_pThreadData->currentTotal;
		perc += sz;
		perc *= 100;
		perc /= m_totalSize;
		*_retval = (PRInt32) perc;
		if (*_retval > 100)
			*_retval = 100;
	}
	else
		*_retval = 0;
	
	// never return 100% while the thread is still alive
	if (*_retval > 99)
		*_retval = 99;

	return( NS_OK);
}

void nsImportGenericMail::ReportError( PRInt32 id, const PRUnichar *pName, nsString *pStream)
{
	if (!pStream)
		return;
	// load the error string
	nsIStringBundle *pBundle = nsImportStringBundle::GetStringBundleProxy();
	PRUnichar *pFmt = nsImportStringBundle::GetStringByID( id, pBundle);
	PRUnichar *pText = nsTextFormatter::smprintf( pFmt, pName);
	pStream->Append( pText);
	nsTextFormatter::smprintf_free( pText);
	nsImportStringBundle::FreeString( pFmt);
	pStream->AppendWithConversion( NS_LINEBREAK);
	NS_IF_RELEASE( pBundle);
}


void nsImportGenericMail::SetLogs( nsString& success, nsString& error, nsISupportsWString *pSuccess, nsISupportsWString *pError)
{
	nsString	str;
	PRUnichar *	pStr = nsnull;
	if (pSuccess) {
		pSuccess->GetData( &pStr);
		if (pStr) {
			str = pStr;
			nsCRT::free( pStr);
			pStr = nsnull;
			str.Append( success);
			pSuccess->SetData( str.get());
		}
		else {
			pSuccess->SetData( success.get());			
		}
	}
	if (pError) {
		pError->GetData( &pStr);
		if (pStr) {
			str = pStr;
			nsCRT::free( pStr);
			str.Append( error);
			pError->SetData( str.get());
		}
		else {
			pError->SetData( error.get());			
		}
	}	
}

NS_IMETHODIMP nsImportGenericMail::CancelImport(void)
{
	if (m_pThreadData) {
		m_pThreadData->abort = PR_TRUE;
		m_pThreadData->DriverAbort();
		m_pThreadData = nsnull;
	}

	return( NS_OK);
}


ImportThreadData::ImportThreadData()
{
	fatalError = PR_FALSE;
	driverAlive = PR_TRUE;
	threadAlive = PR_TRUE;
	abort = PR_FALSE;
	currentTotal = 0;
	currentSize = 0;
	destRoot = nsnull;
	ownsDestRoot = PR_FALSE;
	boxes = nsnull;
	mailImport = nsnull;
	successLog = nsnull;
	errorLog = nsnull;
}

ImportThreadData::~ImportThreadData()
{
	NS_IF_RELEASE( destRoot);
	NS_IF_RELEASE( boxes);
	NS_IF_RELEASE( mailImport);
	NS_IF_RELEASE( errorLog);
	NS_IF_RELEASE( successLog);
}

void ImportThreadData::DriverDelete( void)
{
	driverAlive = PR_FALSE;
	if (!driverAlive && !threadAlive)
		delete this;
}

void ImportThreadData::ThreadDelete()
{
	threadAlive = PR_FALSE;
	if (!driverAlive && !threadAlive)
		delete this;
}

void ImportThreadData::DriverAbort()
{
	if (abort && !threadAlive && destRoot) {
		if (ownsDestRoot) {
			destRoot->RecursiveDelete(PR_TRUE, nsnull);
		}
		else {
			// FIXME: just delete the stuff we created?
		}
	}
	else
		abort = PR_TRUE;
	DriverDelete();
}



PR_STATIC_CALLBACK( void)
ImportMailThread( void *stuff)
{
	IMPORT_LOG0( "In Begin ImporMailThread\n");

	ImportThreadData *pData = (ImportThreadData *)stuff;
	
	IMPORT_LOG0( "In ImportMailThread!!!!!!!\n");
		
	nsresult rv = NS_OK;

	nsCOMPtr<nsIMsgFolder>  	destRoot( pData->destRoot);		
				
	PRUint32	count = 0;
	rv = pData->boxes->Count( &count);
	
	nsISupports *	pSupports;
	PRUint32		i;
	PRBool			import;
	PRUint32		size;	
	PRUint32		depth = 1;
	PRUint32		newDepth;
	nsString		lastName;
	PRUnichar *		pName;
	
	nsCOMPtr<nsIMsgFolder>  	curFolder( destRoot);
	nsCOMPtr<nsIMsgFolder>		curProxy;

	nsCOMPtr<nsIMsgFolder>		newFolder;
	nsCOMPtr<nsIFileSpec>  		outBox;
	nsCOMPtr<nsISupports>		subFolder;
	nsCOMPtr<nsIEnumerator>     enumerator;

	PRBool						exists;
	
	nsString	success;
	nsString	error;

	nsCOMPtr<nsIStringBundle>	bundle( dont_AddRef( nsImportStringBundle::GetStringBundleProxy()));

	// Initialize the curFolder proxy object
	nsCOMPtr<nsIProxyObjectManager> proxyMgr = 
	         do_GetService(kProxyObjectManagerCID, &rv);
	if (NS_SUCCEEDED(rv)) {
		rv = proxyMgr->GetProxyForObject( NS_UI_THREAD_EVENTQ, NS_GET_IID(nsIMsgFolder),
										curFolder, PROXY_SYNC | PROXY_ALWAYS, getter_AddRefs( curProxy));

		IMPORT_LOG1( "Proxy result for curFolder: 0x%lx\n", (long) rv);
		if (NS_SUCCEEDED(rv))
			// GetSubfolders() will initialize folders if they are not already initialized.
			curProxy->GetSubFolders(getter_AddRefs(enumerator));
	}
	else {
		IMPORT_LOG0( "Unable to obtain proxy service to do import\n");
		nsImportStringBundle::GetStringByID( IMPORT_ERROR_MB_NOPROXY, error, bundle);
		pData->abort = PR_TRUE;
	}

	IMPORT_LOG1( "Total number of mailboxes: %d\n", (int) count);

  // Note that the front-end js script only displays one import result string so 
  // we combine both good and bad import status into one string (in var 'success').

	for (i = 0; (i < count) && !(pData->abort); i++) {
		pSupports = pData->boxes->ElementAt( i);
		if (pSupports) {
			nsCOMPtr<nsISupports> iFace( dont_AddRef( pSupports));
			nsCOMPtr<nsIImportMailboxDescriptor> box( do_QueryInterface( pSupports));
			if (box) {
				pData->currentMailbox = i;

				import = PR_FALSE;
				size = 0;
				rv = box->GetImport( &import);
				if (import)
					rv = box->GetSize( &size);
				rv = box->GetDepth( &newDepth);
				if (newDepth > depth) {
          // OK, we are going to add a subfolder under the last/previous folder we processed, so
          // find this folder (stored in 'lastName') who is going to be the new parent folder.
					IMPORT_LOG1( "* Finding folder for child named: %s\n", lastName.get());
					rv = curProxy->GetChildNamed( lastName.get(), getter_AddRefs( subFolder));
					if (NS_FAILED( rv)) {
						nsImportGenericMail::ReportError( IMPORT_ERROR_MB_FINDCHILD, lastName.get(), &success);
						pData->fatalError = PR_TRUE;
						break;
					}

					rv = proxyMgr->GetProxyForObject( NS_UI_THREAD_EVENTQ, NS_GET_IID(nsIMsgFolder), 
													subFolder, PROXY_SYNC | PROXY_ALWAYS, getter_AddRefs( curProxy));
					if (NS_FAILED( rv)) {
						nsImportStringBundle::GetStringByID( IMPORT_ERROR_MB_NOPROXY, error, bundle);
						pData->fatalError = PR_TRUE;
						break;
					}

          // Make sure this new parent folder obj has the correct subfolder list so far.
          rv = curProxy->GetSubFolders(getter_AddRefs(enumerator));

					IMPORT_LOG1( "Created proxy for new subFolder: 0x%lx\n", (long) rv);
				}
				else if (newDepth < depth) {
					rv = NS_OK;
					while ((newDepth < depth) && NS_SUCCEEDED( rv)) {
						nsCOMPtr<nsIFolder> parFolder;
						curProxy->GetParent( getter_AddRefs( parFolder));
						rv = proxyMgr->GetProxyForObject( NS_UI_THREAD_EVENTQ, NS_GET_IID(nsIMsgFolder),
														parFolder, PROXY_SYNC | PROXY_ALWAYS, getter_AddRefs( curProxy));
						depth--;
					}
					if (NS_FAILED( rv)) {
						nsImportStringBundle::GetStringByID( IMPORT_ERROR_MB_NOPROXY, error, bundle);
						pData->fatalError = PR_TRUE;
						break;
					}
				}
				depth = newDepth;
				pName = nsnull;
				box->GetDisplayName( &pName);
				if (pName) {
					lastName = pName;
					nsCRT::free( pName);
				}
				else
					lastName.Assign(NS_LITERAL_STRING("Unknown!"));
				
				exists = PR_FALSE;
				rv = curProxy->ContainsChildNamed( lastName.get(), &exists);
				if (exists) {
					nsXPIDLString subName;
					curProxy->GenerateUniqueSubfolderName( lastName.get(), nsnull, getter_Copies(subName));
					if (!subName.IsEmpty()) 
                        lastName.Assign(subName);
				}
				
				IMPORT_LOG1( "* Creating new import folder: %s\n", lastName.get());

				rv = curProxy->CreateSubfolder( lastName.get(),nsnull);
				
				IMPORT_LOG1( "New folder created, rv: 0x%lx\n", (long) rv);
				if (NS_SUCCEEDED( rv)) {
					rv = curProxy->GetChildNamed( lastName.get(), getter_AddRefs( subFolder));
					IMPORT_LOG1( "GetChildNamed for new folder returned rv: 0x%lx\n", (long) rv);
					if (NS_SUCCEEDED( rv)) {
						newFolder = do_QueryInterface( subFolder);
						if (newFolder) {
							newFolder->GetPath( getter_AddRefs( outBox));
							IMPORT_LOG0( "Got path for newly created folder\n");
						}
						else {
							IMPORT_LOG0( "Newly created folder not found\n");
						}
					}
				}
				
				if (NS_FAILED( rv)) {
					nsImportGenericMail::ReportError( IMPORT_ERROR_MB_CREATE, lastName.get(), &success);
				}

				if (size && import && newFolder && outBox && NS_SUCCEEDED( rv)) {
					PRBool fatalError = PR_FALSE;
					pData->currentSize = size;
					PRUnichar *pSuccess = nsnull;
					PRUnichar *pError = nsnull;
					rv = pData->mailImport->ImportMailbox( box, outBox, &pError, &pSuccess, &fatalError);
					if (pError) {
						error.Append( pError);
						nsCRT::free( pError);
					}
					if (pSuccess) {
						success.Append( pSuccess);
						nsCRT::free( pSuccess);
					}

					pData->currentSize = 0;
					pData->currentTotal += size;
					
					outBox->CloseStream();

          // OK, we've copied the actual folder/file over if the folder size is not 0
          // (ie, the msg summary is no longer valid) so close the msg database so that
          // when the folder is reopened the folder db can be reconstructed (which
          // validates msg summary and forces folder to be reparsed).
          newFolder->ForceDBClosed();

					if (fatalError) {
						IMPORT_LOG1( "*** ImportMailbox returned fatalError, mailbox #%d\n", (int) i);
						pData->fatalError = PR_TRUE;
						break;
					}
				}
			}
		}
	}

	// Now save the new acct info to pref file.
	nsCOMPtr <nsIMsgAccountManager> accMgr = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
        if (NS_SUCCEEDED(rv) && accMgr) {
	  rv = accMgr->SaveAccountInfo();
	  NS_ASSERTION(NS_SUCCEEDED(rv), "Can't save account info to pref file");
	}
	
	nsImportGenericMail::SetLogs( success, error, pData->successLog, pData->errorLog);

	if (pData->abort || pData->fatalError) {
		IMPORT_LOG0( "Abort or fatalError set\n");
		if (pData->ownsDestRoot) {
			IMPORT_LOG0( "Calling destRoot->RecursiveDelete\n");

			destRoot->RecursiveDelete( PR_TRUE, nsnull);
		}
		else {
			// FIXME: just delete the stuff we created?
		}
	}

	IMPORT_LOG1( "Import mailbox thread done: %d\n", (int) pData->currentTotal);

	pData->ThreadDelete();	

}

// Creates a folder in Local Folders with the module name + mail
// for e.g: Outlook Mail
PRBool nsImportGenericMail::CreateFolder( nsIMsgFolder **ppFolder)
{
  nsresult rv;
  *ppFolder = nsnull;

  nsCOMPtr<nsIStringBundle> bundle;
  nsCOMPtr<nsIStringBundleService> bundleService(do_GetService(
                                     NS_STRINGBUNDLE_CONTRACTID, &rv));
  if (NS_FAILED(rv) || !bundleService) 
      return PR_FALSE;
  rv = bundleService->CreateBundle(IMPORT_MSGS_URL, getter_AddRefs(bundle));
  if (NS_FAILED(rv)) 
      return PR_FALSE;
  nsXPIDLString folderName;
  if (m_pName) {
    const PRUnichar *moduleName[] = { m_pName };
    rv = bundle->FormatStringFromName(NS_LITERAL_STRING("ModuleFolderName").get(),
                                      moduleName, 1,
                                      getter_Copies(folderName));
  }
  else {
    rv = bundle->GetStringFromName(NS_LITERAL_STRING("DefaultFolderName").get(),
                                   getter_Copies(folderName));
  }
  if (NS_FAILED(rv)) {
      IMPORT_LOG0( "*** Failed to get Folder Name!\n");
      return PR_FALSE;
  }
  nsCOMPtr <nsIMsgAccountManager> accMgr = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    IMPORT_LOG0( "*** Failed to create account manager!\n");
    return PR_FALSE;
  }

  nsCOMPtr <nsIMsgIncomingServer> server;
  rv = accMgr->GetLocalFoldersServer(getter_AddRefs(server)); 
  // if Local Folders does not exist already, create it
  if (NS_FAILED(rv) || !server)
  {
    nsCOMPtr <nsIMessengerMigrator> messengerMigrator = do_GetService(NS_MESSENGERMIGRATOR_CONTRACTID, &rv);
    if (NS_FAILED(rv)) {
      IMPORT_LOG0( "*** Failed to create messenger migrator!\n");
      return PR_FALSE;
    }
    rv = messengerMigrator->CreateLocalMailAccount(PR_FALSE);
    if (NS_FAILED(rv)) {
      IMPORT_LOG0( "*** Failed to create Local Folders!\n");
      return PR_FALSE;
    }
    rv = accMgr->GetLocalFoldersServer(getter_AddRefs(server)); 
  }
  if (NS_SUCCEEDED(rv) && server) {
    nsCOMPtr <nsIMsgFolder> localRootFolder;
    rv = server->GetRootMsgFolder(getter_AddRefs(localRootFolder)); 
    if (localRootFolder) {
      // we need to call GetSubFolders() so that the folders get initialized 
      // if they are not initialized yet. 
      nsCOMPtr <nsIEnumerator> aEnumerator;
      rv = localRootFolder->GetSubFolders(getter_AddRefs(aEnumerator));
      if (NS_SUCCEEDED(rv)) {
        // check if the folder name we picked already exists.
        PRBool exists = PR_FALSE;
        rv = localRootFolder->ContainsChildNamed(folderName.get(), &exists);
        if (exists) {
          nsXPIDLString name;
          localRootFolder->GenerateUniqueSubfolderName(folderName.get(), nsnull, getter_Copies(name));
          if (!name.IsEmpty())
            folderName.Assign(name);
          else {
            IMPORT_LOG0( "*** Failed to find a unique folder name!\n");
            return PR_FALSE;
          }
        }
        IMPORT_LOG1( "* Creating folder for importing mail: %s\n", folderName.get());
        rv = localRootFolder->CreateSubfolder(folderName.get(), nsnull);
        if (NS_SUCCEEDED(rv)) {
          nsCOMPtr<nsISupports> subFolder;
          rv = localRootFolder->GetChildNamed(folderName.get(), getter_AddRefs(subFolder));
          if (subFolder) {
            subFolder->QueryInterface(NS_GET_IID(nsIMsgFolder), (void **) ppFolder);
            if (*ppFolder) {
              IMPORT_LOG0("****** CREATED NEW FOLDER FOR IMPORT\n");
              return PR_TRUE;
            }
          } // if subFolder
        }
      }
    } // if localRootFolder
  } // if server
  IMPORT_LOG0("****** FAILED TO CREATE FOLDER FOR IMPORT\n");
  return PR_FALSE;
}
