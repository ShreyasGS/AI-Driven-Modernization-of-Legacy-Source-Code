/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *     Doug Turner <dougt@netscape.com>
 */


#include "nsCOMPtr.h"
#include "nsMemory.h"

#include "nsLocalFileWin.h"

#include "nsISimpleEnumerator.h"
#include "nsIComponentManager.h"
#include "prtypes.h"
#include "prio.h"

#include "nsXPIDLString.h"
#include "nsReadableUtils.h"

#include <direct.h>
#include <windows.h>

#include "shellapi.h"
#include "shlguid.h"

#include  <io.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <mbstring.h>

#include "nsXPIDLString.h"
#include "prproces.h"
#include "nsITimelineService.h"

#include "nsAutoLock.h"


//----------------------------------------------------------------------------
// short cut resolver
//----------------------------------------------------------------------------

class ShortcutResolver
{
public:
    ShortcutResolver();
    virtual ~ShortcutResolver();

    nsresult Init();
    nsresult Resolve(const unsigned short* in, char* out);

private:
    PRLock*       mLock;
    IPersistFile* mPersistFile; 
    IShellLink*   mShellLink;
};

ShortcutResolver::ShortcutResolver()
{
    mLock = nsnull;
    mPersistFile = nsnull;
    mShellLink   = nsnull;
}

ShortcutResolver::~ShortcutResolver()
{
    if (mLock)
        PR_DestroyLock(mLock);

    // Release the pointer to the IPersistFile interface. 
    if (mPersistFile)
        mPersistFile->Release(); 
    
    // Release the pointer to the IShellLink interface. 
    if(mShellLink)
        mShellLink->Release();

    CoUninitialize();
}

nsresult
ShortcutResolver::Init()
{
    CoInitialize(NULL);  // FIX: we should probably move somewhere higher up during startup

    mLock = PR_NewLock();
    if (!mLock)
        return NS_ERROR_FAILURE;

    HRESULT hres = CoCreateInstance(CLSID_ShellLink, 
                                    NULL, 
                                    CLSCTX_INPROC_SERVER, 
                                    IID_IShellLink, 
                                    (void**)&mShellLink); 
    if (SUCCEEDED(hres)) 
    {
        // Get a pointer to the IPersistFile interface. 
        hres = mShellLink->QueryInterface(IID_IPersistFile, (void**)&mPersistFile); 
    }
    
    if (mPersistFile == nsnull || mShellLink == nsnull)
        return NS_ERROR_FAILURE;

    return NS_OK;
}

// |out| must be an allocated buffer of size MAX_PATH
nsresult 
ShortcutResolver::Resolve(const unsigned short* in, char* out)
{
    nsAutoLock lock(mLock);
    
    // see if we can Load the path.
    HRESULT hres = mPersistFile->Load(in, STGM_READ); 
    
    if (FAILED(hres)) 
        return NS_ERROR_FAILURE;

    // Resolve the link. 
    hres = mShellLink->Resolve(nsnull, SLR_NO_UI ); 
    
    if (FAILED(hres)) 
        return NS_ERROR_FAILURE;

    WIN32_FIND_DATA wfd; 
    // Get the path to the link target. 
    hres = mShellLink->GetPath( out, MAX_PATH, &wfd, SLGP_UNCPRIORITY ); 
    if (FAILED(hres)) 
        return NS_ERROR_FAILURE;
    return NS_OK;
}

static ShortcutResolver * gResolver = nsnull;

static nsresult NS_CreateShortcutResolver()
{
    gResolver = new ShortcutResolver();
    if (!gResolver)
        return NS_ERROR_OUT_OF_MEMORY;

    return gResolver->Init();
}

static void NS_DestroyShortcutResolver()
{
    delete gResolver;
    gResolver = nsnull;
}


//-----------------------------------------------------------------------------
// static helper functions
//-----------------------------------------------------------------------------

// certainly not all the error that can be 
// encountered, but many of them common ones
static nsresult ConvertWinError(DWORD winErr)
{
    nsresult rv;
    
    switch (winErr)
    {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
        case ERROR_INVALID_DRIVE:
            rv = NS_ERROR_FILE_NOT_FOUND;
            break;
        case ERROR_ACCESS_DENIED:
        case ERROR_NOT_SAME_DEVICE:
            rv = NS_ERROR_FILE_ACCESS_DENIED;
            break;
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_INVALID_BLOCK:
        case ERROR_INVALID_HANDLE:
        case ERROR_ARENA_TRASHED:
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
        case ERROR_CURRENT_DIRECTORY:
            rv = NS_ERROR_FILE_DIR_NOT_EMPTY;
            break;
        case ERROR_WRITE_PROTECT:
            rv = NS_ERROR_FILE_READ_ONLY;
            break;
        case ERROR_HANDLE_DISK_FULL:
            rv = NS_ERROR_FILE_TOO_BIG;
            break;
        case ERROR_FILE_EXISTS:
        case ERROR_ALREADY_EXISTS:
        case ERROR_CANNOT_MAKE:
            rv = NS_ERROR_FILE_ALREADY_EXISTS;
            break;
        case 0:
            rv = NS_OK;
        default:    
            rv = NS_ERROR_FAILURE;
    }
    return rv;
}


static void
myLL_II2L(PRInt32 hi, PRInt32 lo, PRInt64 *result)
{
    PRInt64 a64, b64;  // probably could have been done with 
                       // only one PRInt64, but these are macros, 
                       // and I am a wimp.

    // put hi in the low bits of a64.
    LL_I2L(a64, hi);
    // now shift it to the upperbit and place it the result in result
    LL_SHL(b64, a64, 32);
    // now put the low bits on by adding them to the result.
    LL_ADD(*result, b64, lo);
}


static void
myLL_L2II(PRInt64 result, PRInt32 *hi, PRInt32 *lo )
{
    PRInt64 a64, b64;  // probably could have been done with 
                       // only one PRInt64, but these are macros, 
                       // and I am a wimp.
    
    // shift the hi word to the low word, then push it into a long.
    LL_SHR(a64, result, 32);
    LL_L2I(*hi, a64);

    // shift the low word to the hi word first, then shift it back.
    LL_SHL(b64, result, 32);
    LL_SHR(a64, b64, 32);
    LL_L2I(*lo, a64);
}


//-----------------------------------------------------------------------------
// nsDirEnumerator
//-----------------------------------------------------------------------------

class nsDirEnumerator : public nsISimpleEnumerator
{
    public:

        NS_DECL_ISUPPORTS

        nsDirEnumerator() : mDir(nsnull) 
        {
            NS_INIT_REFCNT();
        }

        nsresult Init(nsILocalFile* parent) 
        {
            nsCAutoString filepath;
            parent->GetNativeTarget(filepath);
        
            if (filepath.IsEmpty())
            {
                parent->GetNativePath(filepath);
            }
            
            if (filepath.IsEmpty())
            {
                return NS_ERROR_UNEXPECTED;
            }

            mDir = PR_OpenDir(filepath.get());
            if (mDir == nsnull)    // not a directory?
                return NS_ERROR_FAILURE;
			
            mParent          = parent;    
            return NS_OK;
        }

        NS_IMETHOD HasMoreElements(PRBool *result) 
        {
            nsresult rv;
            if (mNext == nsnull && mDir) 
            {
                PRDirEntry* entry = PR_ReadDir(mDir, PR_SKIP_BOTH);
                if (entry == nsnull) 
                {
                    // end of dir entries

                    PRStatus status = PR_CloseDir(mDir);
                    if (status != PR_SUCCESS)
                        return NS_ERROR_FAILURE;
                    mDir = nsnull;

                    *result = PR_FALSE;
                    return NS_OK;
                }

                nsCOMPtr<nsIFile> file;
                rv = mParent->Clone(getter_AddRefs(file));
                if (NS_FAILED(rv)) 
                    return rv;
                
                rv = file->AppendNative(nsDependentCString(entry->name));
                if (NS_FAILED(rv)) 
                    return rv;
                
                // make sure the thing exists.  If it does, try the next one.
                PRBool exists;
                rv = file->Exists(&exists);
                if (NS_FAILED(rv) || !exists) 
                {
                    return HasMoreElements(result); 
                }
                
                mNext = do_QueryInterface(file);
            }
            *result = mNext != nsnull;
            return NS_OK;
        }

        NS_IMETHOD GetNext(nsISupports **result) 
        {
            nsresult rv;
            PRBool hasMore;
            rv = HasMoreElements(&hasMore);
            if (NS_FAILED(rv)) return rv;

            *result = mNext;        // might return nsnull
            NS_IF_ADDREF(*result);
            
            mNext = nsnull;
            return NS_OK;
        }

        virtual ~nsDirEnumerator() 
        {
            if (mDir) 
            {
                PRStatus status = PR_CloseDir(mDir);
                NS_ASSERTION(status == PR_SUCCESS, "close failed");
            }
        }

    protected:
        PRDir*                  mDir;
        nsCOMPtr<nsILocalFile>  mParent;
        nsCOMPtr<nsILocalFile>  mNext;
};

NS_IMPL_ISUPPORTS1(nsDirEnumerator, nsISimpleEnumerator);


//-----------------------------------------------------------------------------
// nsLocalFile <public>
//-----------------------------------------------------------------------------

nsLocalFile::nsLocalFile()
{
    NS_INIT_REFCNT();
    mLastResolution = PR_FALSE;
    mFollowSymlinks = PR_FALSE;
    MakeDirty();
}

nsLocalFile::~nsLocalFile()
{
}

NS_METHOD
nsLocalFile::nsLocalFileConstructor(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr)
{
    NS_ENSURE_ARG_POINTER(aInstancePtr);
    NS_ENSURE_NO_AGGREGATION(outer);

    nsLocalFile* inst = new nsLocalFile();
    if (inst == NULL)
        return NS_ERROR_OUT_OF_MEMORY;
    
    nsresult rv = inst->QueryInterface(aIID, aInstancePtr);
    if (NS_FAILED(rv))
    {
        delete inst;
        return rv;
    }
    return NS_OK;
}


//-----------------------------------------------------------------------------
// nsLocalFile::nsISupports
//-----------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS2(nsLocalFile, nsILocalFile, nsIFile)


//-----------------------------------------------------------------------------
// nsLocalFile <private>
//-----------------------------------------------------------------------------

// This function resets any cached information about the file.
void
nsLocalFile::MakeDirty()
{
    mDirty = PR_TRUE;
}

// ResolvePath
//  this function will walk the native path of |this| resolving any symbolic
//  links found.  The new resulting path will be placed into mResolvedPath.
nsresult 
nsLocalFile::ResolvePath(const char* workingPath, PRBool resolveTerminal, char** resolvedPath)
{

    nsresult rv = NS_OK;
    
    if (strstr(workingPath, ".lnk") == nsnull)
        return NS_ERROR_FILE_INVALID_PATH;

#ifdef DEBUG_dougt
    printf("localfile - resolving symlink\n");
#endif

    // Get the native path for |this|
    char* filePath = (char*) nsMemory::Clone( workingPath, strlen(workingPath)+1 );

    if (filePath == nsnull)
        return NS_ERROR_NULL_POINTER;
    
    // We are going to walk the native file path
    // and stop at each slash.  For each partial
    // path (the string to the left of the slash)
    // we will check to see if it is a shortcut.
    // if it is, we will resolve it and continue
    // with that resolved path.
    
    // Get the first slash.          
    char* slash = strchr(filePath, '\\');
            
    if (!slash && filePath[0] != nsnull && filePath[1] == ':' && filePath[2] == '\0')
    {
        // we have a drive letter and a colon (eg 'c:'
        // this is resolve already
        
        int filePathLen = strlen(filePath);
        char* rp = (char*) nsMemory::Alloc( filePathLen + 2 );
        if (!rp)
            return NS_ERROR_OUT_OF_MEMORY;
        memcpy( rp, filePath, filePathLen );
        rp[filePathLen] = '\\';
        rp[filePathLen+1] = 0;
        *resolvedPath = rp;
        
        nsMemory::Free(filePath);
        return NS_OK;
    }
    else
    {
        nsMemory::Free(filePath);
        return NS_ERROR_FILE_INVALID_PATH;
    }
        

    // We really cant have just a drive letter as
    // a shortcut, so we will skip the first '\\'
    slash = strchr(++slash, '\\');
    
    while (slash || resolveTerminal)
    {
        // Change the slash into a null so that
        // we can use the partial path. It is is 
        // null, we know it is the terminal node.
        
        if (slash)
        {
            *slash = '\0';
        }
        else
        {
            if (resolveTerminal)
            {
                // this is our last time in this loop.
                // set loop condition to false

                resolveTerminal = PR_FALSE;
            }
            else
            {
                // something is wrong.  we should not have
                // both slash being null and resolveTerminal 
                // not set!
                nsMemory::Free(filePath);
                return NS_ERROR_NULL_POINTER;
            }
        }
                
        WORD wsz[MAX_PATH];     // TODO, Make this dynamically allocated.

        // check to see the file is a shortcut by the magic .lnk extension.
        size_t offset = strlen(filePath) - 4;
        if ((offset > 0) && (strncmp( (filePath + offset), ".lnk", 4) == 0))
        {
            MultiByteToWideChar(CP_ACP, 0, filePath, -1, wsz, MAX_PATH); 
        }        
        else
        {
            char linkStr[MAX_PATH];
            strcpy(linkStr, filePath);
            strcat(linkStr, ".lnk");

            // Ensure that the string is Unicode. 
            MultiByteToWideChar(CP_ACP, 0, linkStr, -1, wsz, MAX_PATH); 
        }        

        char *temp = (char*) nsMemory::Alloc( MAX_PATH );
        if (temp == nsnull)
            return NS_ERROR_NULL_POINTER;

        if (gResolver)
            rv = gResolver->Resolve(wsz, temp);
        else
            rv = NS_ERROR_FAILURE;

        if (NS_SUCCEEDED(rv))
        {
            // found a new path.
            
            // addend a slash on it since it does not come out of GetPath()
            // with one only if it is a directory.  If it is not a directory
            // and there is more to append, than we have a problem.
            
            struct stat st;
            int statrv = stat(temp, &st);
            
            if (0 == statrv && (_S_IFDIR & st.st_mode))
                strcat(temp, "\\");
            
            if (slash)
            {
                // save where we left off.
                char *carot= (temp + strlen(temp) -1 );
                
                // append all the stuff that we have not done.
                strcat(temp, ++slash);
                
                slash = carot;
            }

            nsMemory::Free(filePath);
            filePath = temp;
            
        }

        else
        {
            // could not resolve shortcut.  Return error;
            nsMemory::Free(filePath);
            return NS_ERROR_FILE_INVALID_PATH;
        }
    }    
    if (slash)
    {
        *slash = '\\';
        ++slash;
        slash = strchr(slash, '\\');
    }

    // kill any trailing separator
    char* temp = filePath;
    int len = strlen(temp) - 1;
    if(temp[len] == '\\')
        temp[len] = '\0';
    
    *resolvedPath = filePath;
    return rv;
}

nsresult
nsLocalFile::ResolveAndStat(PRBool resolveTerminal)
{
    if (!mDirty && mLastResolution == resolveTerminal)
    {
        return NS_OK;
    }
    mLastResolution = resolveTerminal;
    mResolvedPath.Assign(mWorkingPath);  //until we know better.

    // First we will see if the workingPath exists.  If it does, then we
    // can simply use that as the resolved path.  This simplification can
    // be done on windows cause its symlinks (shortcuts) use the .lnk
    // file extension.

    char temp[4];
    const char* workingFilePath = mWorkingPath.get();
    const char* nsprPath = workingFilePath;

    if (mWorkingPath.Length() == 2 && mWorkingPath.CharAt(1) == ':') {
        temp[0] = workingFilePath[0];
        temp[1] = workingFilePath[1];
        temp[2] = '\\';
        temp[3] = '\0';
        nsprPath = temp;
    }

    PRStatus status = PR_GetFileInfo64(nsprPath, &mFileInfo64);
    if ( status == PR_SUCCESS )
    {
        if (!resolveTerminal)
        {
		    mDirty = PR_FALSE;
            return NS_OK;
        }
       
        // check to see that this is shortcut, i.e., the leaf is ".lnk"
        // if the length < 4, then it's not a link.
            
        int pathLen = strlen(workingFilePath);
        const char* leaf = workingFilePath + pathLen - 4;
    
        // if we found the file and we are not following symlinks, then return success.
 
        if (!mFollowSymlinks || pathLen < 4 || (strcmp(leaf, ".lnk") != 0))
        {
		    mDirty = PR_FALSE;
            return NS_OK;
        }
    }

    if (!mFollowSymlinks)
        return NS_ERROR_FILE_NOT_FOUND;  // if we are not resolving, we just give up here.

    nsresult result;

    // okay, something is wrong with the working path.  We will try to resolve it.

    char *resolvePath;
    
	result = ResolvePath(workingFilePath, resolveTerminal, &resolvePath);
    if (NS_FAILED(result))
       return NS_ERROR_FILE_NOT_FOUND;
    
	mResolvedPath.Assign(resolvePath);
    nsMemory::Free(resolvePath);

    status = PR_GetFileInfo64(mResolvedPath.get(), &mFileInfo64);
    
    if ( status == PR_SUCCESS )
		mDirty = PR_FALSE;
    else
        result = NS_ERROR_FILE_NOT_FOUND;

	return result;
}


//-----------------------------------------------------------------------------
// nsLocalFile::nsIFile,nsILocalFile
//-----------------------------------------------------------------------------

NS_IMETHODIMP  
nsLocalFile::Clone(nsIFile **file)
{
    NS_ENSURE_ARG(file);
    *file = nsnull;
    
    // Just copy-construct ourselves
    nsLocalFile *localFile = new nsLocalFile(*this);
    if (localFile == NULL)
        return NS_ERROR_OUT_OF_MEMORY;

    // don't forget to re-initialize mRefCnt
    // or the new object will have the old refcnt
    localFile->mRefCnt = 0;
    
    *file = localFile;
    NS_ADDREF(*file);
        
    return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::InitWithNativePath(const nsACString &filePath)
{
    MakeDirty();
    
    nsACString::const_iterator begin, end;
    filePath.BeginReading(begin);
    filePath.EndReading(end);

    // input string must not be empty
    if (begin == end)
        return NS_ERROR_FAILURE;

    char firstChar = *begin;
    char secondChar = *(++begin);

    // just do a sanity check.  if it has any forward slashes, it is not a Native path
    // on windows.  Also, it must have a colon at after the first char.

    char *path = nsnull;
    PRInt32 pathLen = 0;

    if ( ( (secondChar == ':') && !FindCharInReadable('/', begin, end) ) ||  // normal path
         ( (firstChar == '\\') && (secondChar == '\\') ) )  // network path
    {
        // This is a native path
        path = ToNewCString(filePath);
        pathLen = filePath.Length();
    }
    
    if (path == nsnull)
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;

    // kill any trailing '\' provided it isn't the second char of DBCS
    PRInt32 len = pathLen - 1;
    if (path[len] == '\\' && (!::IsDBCSLeadByte(path[len-1]) || 
        _mbsrchr((const unsigned char *)path, '\\') == (const unsigned char *)path+len))
    {
        path[len] = '\0';
        pathLen = len;
    }
    
    mWorkingPath.Adopt(path, pathLen);
    return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::OpenNSPRFileDesc(PRInt32 flags, PRInt32 mode, PRFileDesc **_retval)
{
    if (mDirty)
    {  
        // we will optimize here.  If we are opening a file and we are still
        // dirty, assume that the working path is vaild and try to open it.
        // If it does work, get the stat info via the file descriptor 
        mResolvedPath.Assign(mWorkingPath);
        *_retval = PR_Open(mResolvedPath.get(), flags, mode);
        if (*_retval)
        {
            PRStatus status = PR_GetOpenFileInfo64(*_retval, &mFileInfo64);
            if (status == PR_SUCCESS)
            {
                mDirty = PR_FALSE;
                mLastResolution = PR_TRUE;
            }
            else
                NS_ERROR("FileInfo64 invalid while PR_Open succeeded.");
            return NS_OK;
        }
    }
    
    nsresult rv = ResolveAndStat(PR_TRUE);
    if (NS_FAILED(rv) && rv != NS_ERROR_FILE_NOT_FOUND)
        return rv; 
   
    *_retval = PR_Open(mResolvedPath.get(), flags, mode);
    
    if (*_retval)
        return NS_OK;

    return NS_ErrorAccordingToNSPR();
}


NS_IMETHODIMP  
nsLocalFile::OpenANSIFileDesc(const char *mode, FILE * *_retval)
{
    nsresult rv = ResolveAndStat(PR_TRUE);
    if (NS_FAILED(rv) && rv != NS_ERROR_FILE_NOT_FOUND)
        return rv; 
   
    *_retval = fopen(mResolvedPath.get(), mode);
    
    if (*_retval)
        return NS_OK;

    return NS_ERROR_FAILURE;
}



NS_IMETHODIMP  
nsLocalFile::Create(PRUint32 type, PRUint32 attributes)
{ 
    if (type != NORMAL_FILE_TYPE && type != DIRECTORY_TYPE)
        return NS_ERROR_FILE_UNKNOWN_TYPE;

    nsresult rv = ResolveAndStat(PR_FALSE);
    if (NS_FAILED(rv) && rv != NS_ERROR_FILE_NOT_FOUND)
        return rv;  
    
   // create nested directories to target
    unsigned char* slash = _mbschr((const unsigned char*) mResolvedPath.get(), '\\');

    if (slash)
    {
        // skip the first '\\'
        ++slash;
        slash = _mbschr(slash, '\\');
    
        while (slash)
        {
            *slash = '\0';
        
            if (!CreateDirectoryA(mResolvedPath.get(), NULL)) {
                rv = ConvertWinError(GetLastError());
                if (rv != NS_ERROR_FILE_ALREADY_EXISTS) return rv;
            }
            *slash = '\\';
            ++slash;
            slash = _mbschr(slash, '\\');
        }
    }

    if (type == NORMAL_FILE_TYPE)
    {
        PRFileDesc* file = PR_Open(mResolvedPath.get(), PR_RDONLY | PR_CREATE_FILE | PR_APPEND | PR_EXCL, attributes);
        if (!file) return NS_ERROR_FILE_ALREADY_EXISTS;

        PR_Close(file);
        return NS_OK;
    }

    if (type == DIRECTORY_TYPE)
    {
        if (!CreateDirectoryA(mResolvedPath.get(), NULL))
            return ConvertWinError(GetLastError());
        else 
            return NS_OK;
    }

    return NS_ERROR_FILE_UNKNOWN_TYPE;
}
    
NS_IMETHODIMP  
nsLocalFile::AppendNative(const nsACString &node)
{
    // Append only one component. Check for subdirs.
    // XXX can we avoid the PromiseFlatCString call?
    if (node.IsEmpty() || (_mbschr((const unsigned char*) PromiseFlatCString(node).get(), '\\') != nsnull))
    {
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;
    }
    return AppendRelativeNativePath(node);
}

NS_IMETHODIMP  
nsLocalFile::AppendRelativeNativePath(const nsACString &node)
{
    // Cannot start with a / or have .. or have / anywhere
    nsACString::const_iterator begin, end;
    node.BeginReading(begin);
    node.EndReading(end);
    if (node.IsEmpty() || FindCharInReadable('/', begin, end))
    {
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;
    }
    MakeDirty();
    mWorkingPath.Append(NS_LITERAL_CSTRING("\\") + node);
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::Normalize()
{
    return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::GetNativeLeafName(nsACString &aLeafName)
{
    aLeafName.Truncate();

    const char* temp = mWorkingPath.get();
    if(temp == nsnull)
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;

    const char* leaf = (const char*) _mbsrchr((const unsigned char*) temp, '\\');
    
    // if the working path is just a node without any lashes.
    if (leaf == nsnull)
        leaf = temp;
    else
        leaf++;

    aLeafName.Assign(leaf);
    return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::SetNativeLeafName(const nsACString &aLeafName)
{
    MakeDirty();
    
    const unsigned char* temp = (const unsigned char*) mWorkingPath.get();
    if(temp == nsnull)
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;

    // cannot use nsCString::RFindChar() due to 0x5c problem
    PRInt32 offset = (PRInt32) (_mbsrchr(temp, '\\') - temp);
    if (offset)
    {
        mWorkingPath.Truncate(offset+1);
    }
    mWorkingPath.Append(aLeafName);

    return NS_OK;
}


NS_IMETHODIMP  
nsLocalFile::GetNativePath(nsACString &_retval)
{
    _retval = mWorkingPath;
    return NS_OK;
}

nsresult
nsLocalFile::CopySingleFile(nsIFile *sourceFile, nsIFile *destParent, const nsACString &newName, PRBool followSymlinks, PRBool move)
{
    nsresult rv;
    nsCAutoString filePath;

    // get the path that we are going to copy to.
    // Since windows does not know how to auto
    // resolve shortcust, we must work with the
    // target.
    nsCAutoString destPath;
    destParent->GetNativeTarget(destPath);  

    destPath.Append("\\");

    if (newName.IsEmpty())
    {
        nsCAutoString aFileName;
        sourceFile->GetNativeLeafName(aFileName);
        destPath.Append(aFileName);
    }
    else
    {
        destPath.Append(newName);
    }

           
    if (followSymlinks)
    {
        rv = sourceFile->GetNativeTarget(filePath);
        if (filePath.IsEmpty())
            rv = sourceFile->GetNativePath(filePath);
    }
    else
    {
        rv = sourceFile->GetNativePath(filePath);
    }

    if (NS_FAILED(rv))
        return rv;

    int copyOK;

    if (!move)
        copyOK = CopyFile(filePath.get(), destPath.get(), PR_TRUE);
    else
        copyOK = MoveFile(filePath.get(), destPath.get());
    
    if (!copyOK)  // CopyFile and MoveFile returns non-zero if succeeds (backward if you ask me).
        rv = ConvertWinError(GetLastError());

    return rv;
}


nsresult
nsLocalFile::CopyMove(nsIFile *aParentDir, const nsACString &newName, PRBool followSymlinks, PRBool move)
{
    nsCOMPtr<nsIFile> newParentDir = aParentDir;
    // check to see if this exists, otherwise return an error.
    // we will check this by resolving.  If the user wants us
    // to follow links, then we are talking about the target,
    // hence we can use the |followSymlinks| parameter.
    nsresult rv  = ResolveAndStat(followSymlinks);
    if (NS_FAILED(rv))
        return rv;

    if (!newParentDir)
    {
        // no parent was specified.  We must rename.
        
        if (newName.IsEmpty())
            return NS_ERROR_INVALID_ARG;

        move = PR_TRUE;
        
        rv = GetParent(getter_AddRefs(newParentDir));
        if (NS_FAILED(rv))
            return rv;
    }

    if (!newParentDir)
        return NS_ERROR_FILE_DESTINATION_NOT_DIR;
    
    // make sure it exists and is a directory.  Create it if not there.
    PRBool exists;
    newParentDir->Exists(&exists);
    if (exists == PR_FALSE)
    {
        rv = newParentDir->Create(DIRECTORY_TYPE, 0644);  // TODO, what permissions should we use
        if (NS_FAILED(rv))
            return rv;
    }
    else
    {
        PRBool isDir;
        newParentDir->IsDirectory(&isDir);
        if (isDir == PR_FALSE)
        {
            if (followSymlinks)
            {
                PRBool isLink;
                newParentDir->IsSymlink(&isLink);
                if (isLink)
                {
                    nsCAutoString target;
                    newParentDir->GetNativeTarget(target);

                    nsCOMPtr<nsILocalFile> realDest = new nsLocalFile();
                    if (realDest == nsnull)
                        return NS_ERROR_OUT_OF_MEMORY;

                    rv = realDest->InitWithNativePath(target);
                    
                    if (NS_FAILED(rv)) 
                        return rv;
                    
                    return CopyMove(realDest, newName, followSymlinks, move);
                }
            }
            else
            {                
                return NS_ERROR_FILE_DESTINATION_NOT_DIR;
            }
        }
    }

    // check to see if we are a directory, if so enumerate it.

    PRBool isDir;
    IsDirectory(&isDir);
    PRBool isSymlink;
    IsSymlink(&isSymlink);

    if (!isDir || (isSymlink && !followSymlinks))
    {
        rv = CopySingleFile(this, newParentDir, newName, followSymlinks, move);
        if (NS_FAILED(rv))
            return rv;
    }
    else
    {
        // create a new target destination in the new parentDir;
        nsCOMPtr<nsIFile> target;
        rv = newParentDir->Clone(getter_AddRefs(target));
        
        if (NS_FAILED(rv)) 
            return rv;
        
        nsCAutoString allocatedNewName;
        if (newName.IsEmpty())
        {
            PRBool isLink;
            IsSymlink(&isLink);
            if (isLink)
            {
                nsCAutoString temp;
                GetNativeTarget(temp);
                const char* leaf = (const char*) _mbsrchr((const unsigned char*) temp.get(), '\\');
                if (leaf[0] == '\\')
                    leaf++;
                allocatedNewName = leaf;
            }
            else
            {
                GetNativeLeafName(allocatedNewName);// this should be the leaf name of the 
            }
        }
        else
        {
            allocatedNewName = newName;
        }
        
        rv = target->AppendNative(allocatedNewName);
        if (NS_FAILED(rv)) 
            return rv;

        allocatedNewName.Truncate();

        target->Create(DIRECTORY_TYPE, 0644);  // TODO, what permissions should we use
        if (NS_FAILED(rv))
            return rv;
        
        nsDirEnumerator* dirEnum = new nsDirEnumerator();
        if (!dirEnum)
            return NS_ERROR_OUT_OF_MEMORY;
        
        rv = dirEnum->Init(this);

        nsCOMPtr<nsISimpleEnumerator> iterator = do_QueryInterface(dirEnum);

        PRBool more;
        iterator->HasMoreElements(&more);
        while (more)
        {
            nsCOMPtr<nsISupports> item;
            nsCOMPtr<nsIFile> file;
            iterator->GetNext(getter_AddRefs(item));
            file = do_QueryInterface(item);
            PRBool isDir, isLink;
            
            file->IsDirectory(&isDir);
            file->IsSymlink(&isLink);

            if (move)
            {
                if (followSymlinks)
                    rv = NS_ERROR_FAILURE;
                else
                    rv = file->MoveToNative(target, nsCString());
            }
            else
            {   
                if (followSymlinks)
                    rv = file->CopyToFollowingLinksNative(target, nsCString());
                else
                    rv = file->CopyToNative(target, nsCString());
            }
                    
            iterator->HasMoreElements(&more);
        }
        // we've finished moving all the children of this directory
        // in the new directory.  so now delete the directory
        // note, we don't need to do a recursive delete.
        // MoveTo() is recursive.  At this point,
        // we've already moved the children of the current folder 
        // to the new location.  nothing should be left in the folder.
        if (move)
          rv = Remove(PR_FALSE /* recursive */);
    }
    

    // If we moved, we want to adjust this.
    if (move)
    {
        MakeDirty();
        
        nsCAutoString newParentPath;
        newParentDir->GetNativePath(newParentPath);
        
        if (newParentPath.IsEmpty())
            return NS_ERROR_FAILURE;

        if (newName.IsEmpty())
        {
            nsCAutoString aFileName;
            GetNativeLeafName(aFileName);
            
            InitWithNativePath(newParentPath);
            AppendNative(aFileName); 
        }
        else
        {
            InitWithNativePath(newParentPath);
            AppendNative(newName);
        }
    }
        
    return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::CopyToNative(nsIFile *newParentDir, const nsACString &newName)
{
    return CopyMove(newParentDir, newName, PR_FALSE, PR_FALSE);
}

NS_IMETHODIMP  
nsLocalFile::CopyToFollowingLinksNative(nsIFile *newParentDir, const nsACString &newName)
{
    return CopyMove(newParentDir, newName, PR_TRUE, PR_FALSE);
}

NS_IMETHODIMP  
nsLocalFile::MoveToNative(nsIFile *newParentDir, const nsACString &newName)
{
    return CopyMove(newParentDir, newName, PR_FALSE, PR_TRUE);
}

NS_IMETHODIMP  
nsLocalFile::Load(PRLibrary * *_retval)
{
    PRBool isFile;
    nsresult rv = IsFile(&isFile);

    if (NS_FAILED(rv))
        return rv;
    
    if (! isFile)
        return NS_ERROR_FILE_IS_DIRECTORY;

    NS_TIMELINE_START_TIMER("PR_LoadLibrary");
    *_retval =  PR_LoadLibrary(mResolvedPath.get());
    NS_TIMELINE_STOP_TIMER("PR_LoadLibrary");
    NS_TIMELINE_MARK_TIMER1("PR_LoadLibrary", mResolvedPath.get());

    if (*_retval)
        return NS_OK;

    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP  
nsLocalFile::Remove(PRBool recursive)
{
    PRBool isDir;
    
    nsresult rv = IsDirectory(&isDir);
    if (NS_FAILED(rv))
        return rv;

    const char *filePath = mResolvedPath.get();
 
    if (isDir)
    {
        if (recursive)
        {
            nsDirEnumerator* dirEnum = new nsDirEnumerator();
            if (dirEnum == nsnull)
                return NS_ERROR_OUT_OF_MEMORY;
        
            rv = dirEnum->Init(this);

            nsCOMPtr<nsISimpleEnumerator> iterator = do_QueryInterface(dirEnum);
        
            PRBool more;
            iterator->HasMoreElements(&more);
            while (more)
            {
                nsCOMPtr<nsISupports> item;
                nsCOMPtr<nsIFile> file;
                iterator->GetNext(getter_AddRefs(item));
                file = do_QueryInterface(item);
    
                file->Remove(recursive);
                
                iterator->HasMoreElements(&more);
            }
        }
        rv = rmdir(filePath);  // todo: save return value?
    }
    else
    {
        rv = remove(filePath); // todo: save return value?
    }
    
    MakeDirty();
    return rv;
}

NS_IMETHODIMP  
nsLocalFile::GetLastModifiedTime(PRInt64 *aLastModifiedTime)
{
    NS_ENSURE_ARG(aLastModifiedTime);
    
    *aLastModifiedTime = 0;

    nsresult rv = ResolveAndStat(PR_TRUE);
    
    if (NS_FAILED(rv))
        return rv;
    
    // microseconds -> milliseconds
    *aLastModifiedTime = mFileInfo64.modifyTime / PR_USEC_PER_MSEC;
    return NS_OK;
}


NS_IMETHODIMP  
nsLocalFile::GetLastModifiedTimeOfLink(PRInt64 *aLastModifiedTime)
{
    NS_ENSURE_ARG(aLastModifiedTime);
    
    *aLastModifiedTime = 0;

    nsresult rv = ResolveAndStat(PR_FALSE);
    
    if (NS_FAILED(rv))
        return rv;
    
    // microseconds -> milliseconds
    *aLastModifiedTime = mFileInfo64.modifyTime / PR_USEC_PER_MSEC;

    return NS_OK;
}


NS_IMETHODIMP  
nsLocalFile::SetLastModifiedTime(PRInt64 aLastModifiedTime)
{
    return nsLocalFile::SetModDate(aLastModifiedTime, PR_TRUE);
}


NS_IMETHODIMP  
nsLocalFile::SetLastModifiedTimeOfLink(PRInt64 aLastModifiedTime)
{
    return nsLocalFile::SetModDate(aLastModifiedTime, PR_FALSE);
}

nsresult
nsLocalFile::SetModDate(PRInt64 aLastModifiedTime, PRBool resolveTerminal)
{
    nsresult rv = ResolveAndStat(resolveTerminal);
    
    if (NS_FAILED(rv))
        return rv;
    
    const char *filePath = mResolvedPath.get();
    
    HANDLE file = CreateFile(  filePath,          // pointer to name of the file
                               GENERIC_WRITE,     // access (write) mode
                               0,                 // share mode
                               NULL,              // pointer to security attributes
                               OPEN_EXISTING,     // how to create
                               0,                 // file attributes 
                               NULL);
    
    MakeDirty();
    
    if (!file)
    {
        return ConvertWinError(GetLastError());
    }

    FILETIME lft, ft;
    SYSTEMTIME st;
    PRExplodedTime pret;
    
    // PR_ExplodeTime expects usecs...
    PR_ExplodeTime(aLastModifiedTime * PR_USEC_PER_MSEC, PR_LocalTimeParameters, &pret);
    st.wYear            = pret.tm_year;    
    st.wMonth           = pret.tm_month + 1; // Convert start offset -- Win32: Jan=1; NSPR: Jan=0
    st.wDayOfWeek       = pret.tm_wday;    
    st.wDay             = pret.tm_mday;    
    st.wHour            = pret.tm_hour;
    st.wMinute          = pret.tm_min;    
    st.wSecond          = pret.tm_sec;
    st.wMilliseconds    = pret.tm_usec/1000;

    if ( 0 == SystemTimeToFileTime(&st, &lft) )
    {
        rv = ConvertWinError(GetLastError());
    }
    else if ( 0 == LocalFileTimeToFileTime(&lft, &ft) )
    {
        rv = ConvertWinError(GetLastError());
    }
    else if ( 0 == SetFileTime(file, NULL, &ft, &ft) )
    {
        // could not set time
        rv = ConvertWinError(GetLastError());
    }

    CloseHandle( file );
    return rv;
}

NS_IMETHODIMP  
nsLocalFile::GetPermissions(PRUint32 *aPermissions)
{
    nsresult rv = ResolveAndStat(PR_TRUE);
    
    if (NS_FAILED(rv))
        return rv;
    
    const char *filePath = mResolvedPath.get();


    return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::GetPermissionsOfLink(PRUint32 *aPermissionsOfLink)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP  
nsLocalFile::SetPermissions(PRUint32 aPermissions)
{
    nsresult rv = ResolveAndStat(PR_TRUE);
    
    if (NS_FAILED(rv))
        return rv;
    
    const char *filePath = mResolvedPath.get();
    if( chmod(filePath, aPermissions) == -1 )
        return NS_ERROR_FAILURE;
        
    return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::SetPermissionsOfLink(PRUint32 aPermissions)
{
    nsresult rv = ResolveAndStat(PR_FALSE);
    
    if (NS_FAILED(rv))
        return rv;
    
    const char *filePath = mResolvedPath.get();
    if( chmod(filePath, aPermissions) == -1 )
        return NS_ERROR_FAILURE;
        
    return NS_OK;
}


NS_IMETHODIMP  
nsLocalFile::GetFileSize(PRInt64 *aFileSize)
{
    NS_ENSURE_ARG(aFileSize);
    
    *aFileSize = 0;

    nsresult rv = ResolveAndStat(PR_TRUE);
    
    if (NS_FAILED(rv))
        return rv;
    

    *aFileSize = mFileInfo64.size;
    return NS_OK;
}


NS_IMETHODIMP  
nsLocalFile::SetFileSize(PRInt64 aFileSize)
{

    DWORD status;
    HANDLE hFile;

    nsresult rv = ResolveAndStat(PR_TRUE);
    
    if (NS_FAILED(rv))
        return rv;
    
    const char *filePath = mResolvedPath.get();


    // Leave it to Microsoft to open an existing file with a function
    // named "CreateFile".
    hFile = CreateFile(filePath,
                       GENERIC_WRITE, 
                       FILE_SHARE_READ, 
                       NULL, 
                       OPEN_EXISTING, 
                       FILE_ATTRIBUTE_NORMAL, 
                       NULL); 

    if (hFile == INVALID_HANDLE_VALUE)
    {
        MakeDirty();
        return NS_ERROR_FAILURE;
    }
    
    // Seek to new, desired end of file
    PRInt32 hi, lo;
    myLL_L2II(aFileSize, &hi, &lo );
    
    status = SetFilePointer(hFile, lo, NULL, FILE_BEGIN);
    if (status == 0xffffffff)
        goto error;

    // Truncate file at current cursor position
    if (!SetEndOfFile(hFile))
        goto error;

    if (!CloseHandle(hFile))
        return NS_ERROR_FAILURE;

    MakeDirty();
    return NS_OK;

 error:
    MakeDirty();
    CloseHandle(hFile);
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP  
nsLocalFile::GetFileSizeOfLink(PRInt64 *aFileSize)
{
    NS_ENSURE_ARG(aFileSize);
    
    *aFileSize = 0;

    nsresult rv = ResolveAndStat(PR_FALSE);
    
    if (NS_FAILED(rv))
        return rv;
    
    *aFileSize = mFileInfo64.size;
    return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::GetDiskSpaceAvailable(PRInt64 *aDiskSpaceAvailable)
{
    NS_ENSURE_ARG(aDiskSpaceAvailable);
    
    ResolveAndStat(PR_FALSE);
    
    PRInt64 int64;
    
    LL_I2L(int64 , LONG_MAX);

    // Check disk space
    DWORD dwSecPerClus, dwBytesPerSec, dwFreeClus, dwTotalClus;
    ULARGE_INTEGER liFreeBytesAvailableToCaller, liTotalNumberOfBytes, liTotalNumberOfFreeBytes;
    double nBytes = 0;

    BOOL (WINAPI* getDiskFreeSpaceExA)(LPCTSTR lpDirectoryName, 
                                       PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                                       PULARGE_INTEGER lpTotalNumberOfBytes,    
                                       PULARGE_INTEGER lpTotalNumberOfFreeBytes) = NULL;

    HINSTANCE hInst = LoadLibrary("KERNEL32.DLL");
    NS_ASSERTION(hInst != NULL, "COULD NOT LOAD KERNEL32.DLL");
    if (hInst != NULL)
    {
        getDiskFreeSpaceExA =  (BOOL (WINAPI*)(LPCTSTR lpDirectoryName, 
                                               PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                                               PULARGE_INTEGER lpTotalNumberOfBytes,    
                                               PULARGE_INTEGER lpTotalNumberOfFreeBytes)) 
        GetProcAddress(hInst, "GetDiskFreeSpaceExA");
        FreeLibrary(hInst);
    }

    if (getDiskFreeSpaceExA && (*getDiskFreeSpaceExA)(mResolvedPath.get(),
                                                      &liFreeBytesAvailableToCaller, 
                                                      &liTotalNumberOfBytes,  
                                                      &liTotalNumberOfFreeBytes))
    {
        nBytes = (double)(signed __int64)liFreeBytesAvailableToCaller.QuadPart;
    }
    else {
        char aDrive[_MAX_DRIVE + 2];
        _splitpath( mResolvedPath.get(), aDrive, NULL, NULL, NULL);
        strcat(aDrive, "\\");

        if ( GetDiskFreeSpace(aDrive, &dwSecPerClus, &dwBytesPerSec, &dwFreeClus, &dwTotalClus))
        {
            nBytes = (double)dwFreeClus*(double)dwSecPerClus*(double) dwBytesPerSec;
        }
    }
    LL_D2L(*aDiskSpaceAvailable, nBytes);

    return NS_OK;

}

NS_IMETHODIMP  
nsLocalFile::GetParent(nsIFile * *aParent)
{
    NS_ENSURE_ARG_POINTER(aParent);

    nsCAutoString parentPath(mWorkingPath);

    // cannot use nsCString::RFindChar() due to 0x5c problem
    PRInt32 offset = (PRInt32) (_mbsrchr((const unsigned char *) parentPath.get(), '\\')
                     - (const unsigned char *) parentPath.get());
    if (offset < 0)
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;

    parentPath.Truncate(offset);

    nsCOMPtr<nsILocalFile> localFile;
    nsresult rv =  NS_NewNativeLocalFile(parentPath, mFollowSymlinks, getter_AddRefs(localFile));
    
    if(NS_SUCCEEDED(rv) && localFile)
    {
        return CallQueryInterface(localFile, aParent);
    }
    return rv;
}

NS_IMETHODIMP  
nsLocalFile::Exists(PRBool *_retval)
{
    NS_ENSURE_ARG(_retval);

    MakeDirty();
    nsresult rv = ResolveAndStat( PR_TRUE );
    
    if (NS_SUCCEEDED(rv))
        *_retval = PR_TRUE;
    else 
        *_retval = PR_FALSE;
    
    return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::IsWritable(PRBool *_retval)
{
    NS_ENSURE_ARG(_retval);
    *_retval = PR_FALSE;
   
    nsresult rv = ResolveAndStat(PR_TRUE);
    
    if (NS_FAILED(rv))
        return rv;  
    
    const char *workingFilePath = mWorkingPath.get();
    DWORD word = GetFileAttributes(workingFilePath);

    *_retval = !((word & FILE_ATTRIBUTE_READONLY) != 0); 

    return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::IsReadable(PRBool *_retval)
{
    NS_ENSURE_ARG(_retval);
    *_retval = PR_FALSE;

    nsresult rv = ResolveAndStat( PR_TRUE );
    if (NS_FAILED(rv))
        return rv;

    *_retval = PR_TRUE;
    return NS_OK;
}


NS_IMETHODIMP  
nsLocalFile::IsExecutable(PRBool *_retval)
{
    NS_ENSURE_ARG(_retval);
    *_retval = PR_FALSE;


    nsresult rv = ResolveAndStat( PR_TRUE );
    if (NS_FAILED(rv))
        return rv;

    nsCAutoString path;
    PRBool symLink;
    
    rv = IsSymlink(&symLink);
    if (NS_FAILED(rv))
        return rv;
    
    if (symLink)
        GetNativeTarget(path);
    else
        GetNativePath(path);

    // Get extension.
    char* ext = ::strrchr( path.get(), '.' );
    if ( ext ) {
        // Convert extension to lower case.
        for( char *p = ext; *p; p++ ) {
            if ( ::isupper( *p ) ) {
                *p = ::tolower( *p );
            }
        }
        // Search for any of the set of executable extensions.
        const char * const executableExts[] = { ".exe",
                                                ".bat",
                                                ".com",
                                                ".pif",
                                                ".cmd",
                                                ".js",
                                                ".vbs",
                                                ".lnk",
                                                ".reg",
                                                ".wsf",
                                                0 };
        for ( int i = 0; executableExts[i]; i++ ) {
            if ( ::strcmp( executableExts[i], ext ) == 0 ) {
                // Found a match.  Set result and quit.
                *_retval = PR_TRUE;
                break;
            }
        }
    }
    
    return NS_OK;
}


NS_IMETHODIMP  
nsLocalFile::IsDirectory(PRBool *_retval)
{
    NS_ENSURE_ARG(_retval);
    *_retval = PR_FALSE;

    nsresult rv = ResolveAndStat(PR_TRUE);
    
    if (NS_FAILED(rv))
        return rv;

    *_retval = (mFileInfo64.type == PR_FILE_DIRECTORY); 

    return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::IsFile(PRBool *_retval)
{
    NS_ENSURE_ARG(_retval);
    *_retval = PR_FALSE;

    nsresult rv = ResolveAndStat(PR_TRUE);
    
    if (NS_FAILED(rv))
        return rv;

    *_retval = (mFileInfo64.type == PR_FILE_FILE); 
    return rv;
}

NS_IMETHODIMP  
nsLocalFile::IsHidden(PRBool *_retval)
{
    NS_ENSURE_ARG(_retval);
    *_retval = PR_FALSE;

    nsresult rv = ResolveAndStat(PR_TRUE);
    
    if (NS_FAILED(rv))
        return rv;
    
    const char *workingFilePath = mWorkingPath.get();
    DWORD word = GetFileAttributes(workingFilePath);

    *_retval =  ((word & FILE_ATTRIBUTE_HIDDEN)  != 0); 

    return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::IsSymlink(PRBool *_retval)
{
    NS_ENSURE_ARG(_retval);
    *_retval = PR_FALSE;

    nsCAutoString path;
    int   pathLen;
    
    GetNativePath(path);
    pathLen = path.Length();
    
    const char* leaf = path.get() + pathLen - 4;
    
    if ( (strcmp(leaf, ".lnk") == 0)) 
    {
        *_retval = PR_TRUE;
    }
    
    return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::IsSpecial(PRBool *_retval)
{
    NS_ENSURE_ARG(_retval);
    *_retval = PR_FALSE;

    nsresult rv = ResolveAndStat(PR_TRUE);
    
    if (NS_FAILED(rv))
        return rv;
    
    const char *workingFilePath = mWorkingPath.get();
    DWORD word = GetFileAttributes(workingFilePath);

    *_retval = ((word & FILE_ATTRIBUTE_SYSTEM)  != 0); 

    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::Equals(nsIFile *inFile, PRBool *_retval)
{
    NS_ENSURE_ARG(inFile);
    NS_ENSURE_ARG(_retval);

    nsCAutoString inFilePath;
    inFile->GetNativePath(inFilePath);
    
    *_retval = inFilePath.Equals(mWorkingPath);
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::Contains(nsIFile *inFile, PRBool recur, PRBool *_retval)
{
    *_retval = PR_FALSE;
       
    nsCAutoString myFilePath;
    if ( NS_FAILED(GetNativeTarget(myFilePath)))
        GetNativePath(myFilePath);
    
    PRInt32 myFilePathLen = myFilePath.Length();
    
    nsCAutoString inFilePath;
    if ( NS_FAILED(inFile->GetNativeTarget(inFilePath)))
        inFile->GetNativePath(inFilePath);

    if ( strncmp( myFilePath.get(), inFilePath.get(), myFilePathLen) == 0)
    {
        // now make sure that the |inFile|'s path has a trailing
        // separator.

        if (inFilePath[myFilePathLen] == '\\')
        {
            *_retval = PR_TRUE;
        }

    }

    return NS_OK;
}



NS_IMETHODIMP
nsLocalFile::GetNativeTarget(nsACString &_retval)
{   
    _retval.Truncate();
#if STRICT_FAKE_SYMLINKS    
    PRBool symLink;
    
    nsresult rv = IsSymlink(&symLink);
    if (NS_FAILED(rv))
        return rv;

    if (!symLink)
    {
        return NS_ERROR_FILE_INVALID_PATH;
    }
#endif
    ResolveAndStat(PR_TRUE);
        
    _retval = mResolvedPath;
    return NS_OK;
}


/* attribute PRBool followLinks; */
NS_IMETHODIMP 
nsLocalFile::GetFollowLinks(PRBool *aFollowLinks)
{
    *aFollowLinks = mFollowSymlinks;
    return NS_OK;
}
NS_IMETHODIMP 
nsLocalFile::SetFollowLinks(PRBool aFollowLinks)
{
    MakeDirty();
    mFollowSymlinks = aFollowLinks;
    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::GetDirectoryEntries(nsISimpleEnumerator * *entries)
{
    nsresult rv;
    
    *entries = nsnull;

    PRBool isDir;
    rv = IsDirectory(&isDir);
    if (NS_FAILED(rv)) 
        return rv;
    if (!isDir)
        return NS_ERROR_FILE_NOT_DIRECTORY;

    nsDirEnumerator* dirEnum = new nsDirEnumerator();
    if (dirEnum == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(dirEnum);
    rv = dirEnum->Init(this);
    if (NS_FAILED(rv)) 
    {
        NS_RELEASE(dirEnum);
        return rv;
    }
    
    *entries = dirEnum;
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetPersistentDescriptor(nsACString &aPersistentDescriptor)
{
    return GetNativePath(aPersistentDescriptor);
}

NS_IMETHODIMP
nsLocalFile::SetPersistentDescriptor(const nsACString &aPersistentDescriptor)
{
   return InitWithNativePath(aPersistentDescriptor);   
}

NS_IMETHODIMP
nsLocalFile::Reveal()
{
  nsresult rv = NS_OK;
  PRBool isDirectory = PR_FALSE;
  nsCAutoString path;
  nsAutoString unicodePath;

  IsDirectory(&isDirectory);
  if (isDirectory)
  {
    GetNativePath(path);  
  }
  else
  {
    nsCOMPtr<nsIFile> parent;
    GetParent(getter_AddRefs(parent));
    if (parent)
    {
      parent->GetNativePath(path);  
      parent->GetPath(unicodePath);
    }
  }

  // Remember the current fg window.
  HWND origWin, fgWin;
  origWin = fgWin = ::GetForegroundWindow();

  // use the app registry name to launch a shell execute....
  LONG r = (LONG) ::ShellExecute( NULL, "open", path.get(), NULL, NULL, SW_SHOWNORMAL);
  if (r < 32) 
    return NS_ERROR_FAILURE;

  // If this is a directory, then we don't need to select a file.
  if (isDirectory)
    return NS_OK;

  // Resources we may need to free when done.
  IShellFolder *desktopFolder   = 0;
  IMalloc      *shellMalloc     = 0;
  IShellFolder *folder          = 0;
  LPITEMIDLIST  folder_pidl     = 0;
  LPITEMIDLIST  file_pidl       = 0;
  LPITEMIDLIST  win95_file_pidl = 0;
  HMODULE       shell32         = 0;

  // We break out of this do/while non-loop at any point where we have to give up.
  do {
    // Wait for the window to open.  We wait a maximum of 2 seconds.
    // If we get the wrong window, that will be dealt with below.
    for (int iter = 10; iter; iter--)
    {
      fgWin = ::GetForegroundWindow();
      if (fgWin != origWin)
          break; // for loopo
      ::Sleep(200);
    }
    // If we failed to locate the new window, give up.
    if (origWin == fgWin)
      break; // do/while

    // Now we have the explorer window.  We need to send it the "select item"
    // message (which isn't trivial, so buckly your seat belt)...

    // We need the explorer's process id.
    DWORD pid = 0;
    ::GetWindowThreadProcessId(fgWin, &pid);

    // Get desktop folder.
    HRESULT rc = ::SHGetDesktopFolder(&desktopFolder);
    if (!desktopFolder)
      break;

    // Get IMalloc interface to use for shell pidls.
    rc = ::SHGetMalloc(&shellMalloc);
    if (!shellMalloc)
      break;

    // Convert folder path to pidl.  This requires the Unicode path name.
    // It returns a pidl that must be freed via shellMalloc->Free.
    ULONG eaten = 0;
    rc = desktopFolder->ParseDisplayName( 0,
                                          0,
                                          (LPOLESTR)unicodePath.get(),
                                          &eaten,
                                          &folder_pidl,
                                          0 );
    if (!folder_pidl)
      break;

    // Now get IShellFolder interface for the folder we opened.
    rc = desktopFolder->BindToObject( folder_pidl,
                                      0,
                                      IID_IShellFolder,
                                      (void**)&folder );
    if (!folder)
      break;

    // Now get file name pidl from that folder.
    nsAutoString unicodeLeaf;
    if (NS_FAILED(GetLeafName(unicodeLeaf)))
      break;
    rc = folder->ParseDisplayName( 0,
                                   0,
                                   (LPOLESTR)unicodeLeaf.get(),
                                   &eaten,
                                   &file_pidl,
                                   0 );
    if (!file_pidl)
      break;

    // We need the module handle for shell32.dll.
    shell32 = ::GetModuleHandle("shell32.dll");
    if (!shell32)
      break;

    // Allocate shared memory copy of pidl.  This uses the undocumented "SHAllocShared"
    // function.  Note that it is freed automatically after the ::SendMessage so we
    // don't have to free it.
    static HANDLE(WINAPI*SHAllocShared)(LPVOID,ULONG,DWORD) = (HANDLE(WINAPI*)(LPVOID,ULONG,DWORD))::GetProcAddress(shell32, (LPCTSTR)520);
    HANDLE pidlHandle = 0;
    if (SHAllocShared)
    {
      // We need the size of the pidl, which we get via another undocumented
      // API: "ILGetSize".
      UINT (WINAPI*ILGetSize)(LPCITEMIDLIST) = (UINT(WINAPI*)(LPCITEMIDLIST))::GetProcAddress(shell32, (LPCTSTR)152);
      if (!ILGetSize)
        break;
      pidlHandle = SHAllocShared((void*)(ITEMIDLIST*)file_pidl,
                                 ILGetSize(file_pidl),
                                 pid);
      if (!pidlHandle)
        break;
    }
    else
    {
      // On Win95, there is no SHAllocShared.  Instead, we clone the file's pidl in
      // the shell's global heap (via ILGlobalClone) and pass that.
      LPITEMIDLIST(WINAPI*ILGlobalClone)(LPCITEMIDLIST) = (LPITEMIDLIST(WINAPI*)(LPCITEMIDLIST))::GetProcAddress(shell32, (LPCTSTR)20);
      if (!ILGlobalClone)
        break;
      win95_file_pidl = ILGlobalClone(file_pidl);
      if (!win95_file_pidl)
        break;
      // Arrange so that this pidl is passed on the ::SendMessage.
      pidlHandle = win95_file_pidl;
    }

    // Send message to select this file.
    ::SendMessage(fgWin,
                  WM_USER+5,
                  SVSI_SELECT | SVSI_DESELECTOTHERS | SVSI_ENSUREVISIBLE,
                  (LPARAM)pidlHandle );
  } while ( PR_FALSE );

  // Clean up (freeing stuff as needed, in reverse order).
  if (win95_file_pidl)
  {
    // We need to free this using ILGlobalFree, another undocumented API.
    static void (WINAPI*ILGlobalFree)(LPCITEMIDLIST) = (void(WINAPI*)(LPCITEMIDLIST))::GetProcAddress(shell32,(LPCTSTR)156);
    if (ILGlobalFree)
      ILGlobalFree(win95_file_pidl);
  }
  if (file_pidl)
    shellMalloc->Free(file_pidl);
  if (folder_pidl)
    shellMalloc->Free(folder_pidl);
  if (folder)
    folder->Release();
  if (shellMalloc)
    shellMalloc->Release();
  if (desktopFolder)
    desktopFolder->Release();

  return rv;
}


NS_IMETHODIMP
nsLocalFile::Launch()
{
  nsresult rv = NS_OK;
  const nsCString &path = mWorkingPath;

  // use the app registry name to launch a shell execute....
  LONG r = (LONG) ::ShellExecute( NULL, NULL, path.get(), NULL, NULL, SW_SHOWNORMAL);

  // if the file has no association, we launch windows' "what do you want to do" dialog
  if (r == SE_ERR_NOASSOC) {
    nsCAutoString shellArg;
    shellArg.Assign(NS_LITERAL_CSTRING("shell32.dll,OpenAs_RunDLL ") + path);
    r = (LONG) ::ShellExecute(NULL, NULL, "RUNDLL32.EXE",  shellArg.get(),
                              NULL, SW_SHOWNORMAL);
  }
  if (r < 32) 
    rv = NS_ERROR_FAILURE;
	else
    rv = NS_OK;

  return rv;
}

nsresult 
NS_NewNativeLocalFile(const nsACString &path, PRBool followLinks, nsILocalFile* *result)
{
    nsLocalFile* file = new nsLocalFile();
    if (file == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(file);

    file->SetFollowLinks(followLinks);
    
    if (!path.IsEmpty()) {
        nsresult rv = file->InitWithNativePath(path);
        if (NS_FAILED(rv)) {
            NS_RELEASE(file);
            return rv;
        }
    }
    
    *result = file;
    return NS_OK;
}

//-----------------------------------------------------------------------------
// UCS2 interface
//-----------------------------------------------------------------------------

static nsresult UCS2toFS(const PRUnichar *aBuffer, char **aResult)
{
    NS_ENSURE_ARG_POINTER(aBuffer);

    // includes null termination
    size_t chars = ::WideCharToMultiByte(CP_ACP, 0,
                                         aBuffer, -1,
                                         NULL, 0, NULL, NULL);
    if (chars == 0)
        return NS_ERROR_FAILURE;
    
    *aResult = (char*)nsMemory::Alloc(chars * sizeof(char));
    if (!*aResult)
        return NS_ERROR_OUT_OF_MEMORY;
    
    // default "defaultChar" is '?', which is an illegal character on windows file system.
    // That will cause file uncreatable. Change it to '_'
    const char defaultChar = '_';
    chars = ::WideCharToMultiByte(CP_ACP, 0,
                                  aBuffer, -1,
                                  *aResult, chars, &defaultChar, NULL);
    if (chars == 0)
        return NS_ERROR_FAILURE;
    
    return NS_OK;
}

static nsresult FStoUCS2(const char* aBuffer, PRUnichar **aResult)
{
    NS_ENSURE_ARG_POINTER(aBuffer);

    // includes null termination
    size_t chars = ::MultiByteToWideChar(CP_ACP, 0, aBuffer, -1, NULL, 0);

    if (chars == 0)
        return NS_ERROR_FAILURE;
    
    *aResult = (PRUnichar*)nsMemory::Alloc(chars * sizeof(PRUnichar));
    if (!*aResult)
        return NS_ERROR_OUT_OF_MEMORY;
    
    chars = ::MultiByteToWideChar(CP_ACP, 0,
                                  aBuffer, -1,
                                  *aResult, chars);
    if (chars == 0)
        return NS_ERROR_FAILURE;
    
    return NS_OK;
}

nsresult  
nsLocalFile::InitWithPath(const nsAString &filePath)
{
    nsXPIDLCString tmp;
    nsresult rv = UCS2toFS(PromiseFlatString(filePath).get(), getter_Copies(tmp));
    if (NS_SUCCEEDED(rv))
        return InitWithNativePath(tmp);
    
    return rv;
}

nsresult  
nsLocalFile::Append(const nsAString &node)
{
    nsXPIDLCString tmp;
    nsresult rv = UCS2toFS(PromiseFlatString(node).get(), getter_Copies(tmp));
    if (NS_SUCCEEDED(rv))
        return AppendNative(tmp);
    
    return rv;
}

nsresult  
nsLocalFile::AppendRelativePath(const nsAString &node)
{
    nsXPIDLCString tmp;
    nsresult rv = UCS2toFS(PromiseFlatString(node).get(), getter_Copies(tmp));
    if (NS_SUCCEEDED(rv))
        return AppendRelativeNativePath(tmp);
    return rv;
}

nsresult  
nsLocalFile::GetLeafName(nsAString &aLeafName)
{
    nsCAutoString tmp;
    nsresult rv = GetNativeLeafName(tmp);
    if (NS_SUCCEEDED(rv)) {
        nsXPIDLString ucsBuf;
        rv = FStoUCS2(tmp.get(), getter_Copies(ucsBuf));
        if (NS_SUCCEEDED(rv))
            aLeafName = ucsBuf;
    }
    
    return rv;
}

nsresult  
nsLocalFile::SetLeafName(const nsAString &aLeafName)
{
    nsXPIDLCString tmp;
    nsresult rv = UCS2toFS(PromiseFlatString(aLeafName).get(), getter_Copies(tmp));
    if (NS_SUCCEEDED(rv))
        return SetNativeLeafName(tmp);
    
    return rv;
}

nsresult  
nsLocalFile::GetPath(nsAString &_retval)
{
    nsXPIDLString ucsBuf;
    nsresult rv = FStoUCS2(mWorkingPath.get(), getter_Copies(ucsBuf));
    if (NS_SUCCEEDED(rv))
        _retval = ucsBuf;
    return rv;
}

nsresult  
nsLocalFile::CopyTo(nsIFile *newParentDir, const nsAString &newName)
{
    if (newName.IsEmpty())
        return CopyToNative(newParentDir, nsCString());
    
    nsXPIDLCString tmp;
    nsresult rv = UCS2toFS(PromiseFlatString(newName).get(), getter_Copies(tmp));
    if (NS_SUCCEEDED(rv))
        return CopyToNative(newParentDir, tmp);
    
    return rv;
}

nsresult  
nsLocalFile::CopyToFollowingLinks(nsIFile *newParentDir, const nsAString &newName)
{
    if (newName.IsEmpty())
        return CopyToFollowingLinksNative(newParentDir, nsCString());
    
    nsXPIDLCString tmp;
    nsresult rv = UCS2toFS(PromiseFlatString(newName).get(), getter_Copies(tmp));
    if (NS_SUCCEEDED(rv))
        return CopyToFollowingLinksNative(newParentDir, tmp);
    
    return rv;
}

nsresult  
nsLocalFile::MoveTo(nsIFile *newParentDir, const nsAString &newName)
{
    if (newName.IsEmpty())
        return MoveToNative(newParentDir, nsCString());
    
    nsXPIDLCString tmp;
    nsresult rv = UCS2toFS(PromiseFlatString(newName).get(), getter_Copies(tmp));
    if (NS_SUCCEEDED(rv))
        return MoveToNative(newParentDir, tmp);

    return rv;
}

nsresult
nsLocalFile::GetTarget(nsAString &_retval)
{
    nsCAutoString tmp;
    nsresult rv = GetNativeTarget(tmp);
    if (NS_SUCCEEDED(rv)) {
        nsXPIDLString ucsBuf;
        rv = FStoUCS2(tmp.get(), getter_Copies(ucsBuf));
        if (NS_SUCCEEDED(rv))
            _retval = ucsBuf;
    }
    
    return rv;
}

nsresult 
NS_NewLocalFile(const nsAString &path, PRBool followLinks, nsILocalFile* *result)
{
    nsXPIDLCString tmp;
    nsresult rv = UCS2toFS(PromiseFlatString(path).get(), getter_Copies(tmp));
    if (NS_SUCCEEDED(rv))
        return NS_NewNativeLocalFile(tmp, followLinks, result);

    return NS_OK;
}

//-----------------------------------------------------------------------------
// nsLocalFile <static members>
//-----------------------------------------------------------------------------

void
nsLocalFile::GlobalInit()
{
    nsresult rv = NS_CreateShortcutResolver();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Shortcut resolver could not be created");
}

void
nsLocalFile::GlobalShutdown()
{
    NS_DestroyShortcutResolver();
}
