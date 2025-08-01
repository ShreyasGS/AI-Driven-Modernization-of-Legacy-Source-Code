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
	nsPluginsDirUNIX.cpp
	
	UNIX implementation of the nsPluginsDir/nsPluginsFile classes.
	
	by Alex Musil
 */

#include "nsplugin.h"
#include "ns4xPlugin.h"
#include "ns4xPluginInstance.h"
#include "nsIServiceManager.h"
#include "nsIMemory.h"
#include "nsIPluginStreamListener.h"
#include "nsPluginsDir.h"
#include "nsPluginsDirUtils.h"
#include "nsObsoleteModuleLoading.h"
#include "prmem.h"
#include "prenv.h"
#include "prerror.h"
#include <sys/stat.h>

#include "nsIPref.h"
static NS_DEFINE_CID(kPrefServiceCID, NS_PREF_CID);

#define LOCAL_PLUGIN_DLL_SUFFIX ".so"
#if defined(HPUX11)
#define DEFAULT_X11_PATH "/usr/lib/X11R6/"
#undef LOCAL_PLUGIN_DLL_SUFFIX
#define LOCAL_PLUGIN_DLL_SUFFIX ".sl"
#elif defined(SOLARIS)
#define DEFAULT_X11_PATH "/usr/openwin/lib/"
#elif defined(LINUX)
#define DEFAULT_X11_PATH "/usr/X11R6/lib/"
#else
#define DEFAULT_X11_PATH ""
#endif

#ifdef MOZ_WIDGET_GTK 

#define PLUGIN_MAX_LEN_OF_TMP_ARR 512

static void DisplayPR_LoadLibraryErrorMessage(const char *libName)
{
    char errorMsg[PLUGIN_MAX_LEN_OF_TMP_ARR] = "Cannot get error from NSPR.";
    if (PR_GetErrorTextLength() < (int) sizeof(errorMsg))
        PR_GetErrorText(errorMsg);

    fprintf(stderr, "LoadPlugin: failed to initialize shared library %s [%s]\n",
        libName, errorMsg);
}

static void SearchForSoname(const char* name, char** soname)
{
    if (!(name && soname))
        return;
    PRDir *fdDir = PR_OpenDir(DEFAULT_X11_PATH);
    if (!fdDir)
        return;       

    int n = PL_strlen(name);
    PRDirEntry *dirEntry;
    while ((dirEntry = PR_ReadDir(fdDir, PR_SKIP_BOTH))) {
        if (!PL_strncmp(dirEntry->name, name, n)) {
            if (dirEntry->name[n] == '.' && dirEntry->name[n+1] && !dirEntry->name[n+2]) {
                // name.N, wild guess this is what we need
                char out[PLUGIN_MAX_LEN_OF_TMP_ARR] = DEFAULT_X11_PATH;
                PL_strcat(out, dirEntry->name);
                *soname = PL_strdup(out);
               break;
            }
        }
    }

    PR_CloseDir(fdDir);
}

static PRBool LoadExtraSharedLib(const char *name, char **soname, PRBool tryToGetSoname)
{
    PRBool ret = PR_TRUE;
    PRLibSpec tempSpec;
    PRLibrary *handle;
    tempSpec.type = PR_LibSpec_Pathname;
    tempSpec.value.pathname = name;
    handle = PR_LoadLibraryWithFlags(tempSpec, PR_LD_NOW|PR_LD_GLOBAL);
    if (!handle) {
        ret = PR_FALSE;
        DisplayPR_LoadLibraryErrorMessage(name);
        if (tryToGetSoname) {
            SearchForSoname(name, soname);
            if (*soname) {
                ret = LoadExtraSharedLib((const char *) *soname, NULL, PR_FALSE);
            }
        }
    }
    return ret;
}

#define PLUGIN_MAX_NUMBER_OF_EXTRA_LIBS 32
#define PREF_PLUGINS_SONAME "plugin.soname.list"
#ifdef SOLARIS
#define DEFAULT_EXTRA_LIBS_LIST "libXt" LOCAL_PLUGIN_DLL_SUFFIX ":libXext" LOCAL_PLUGIN_DLL_SUFFIX ":libXm" LOCAL_PLUGIN_DLL_SUFFIX
#else
#define DEFAULT_EXTRA_LIBS_LIST "libXt" LOCAL_PLUGIN_DLL_SUFFIX ":libXext" LOCAL_PLUGIN_DLL_SUFFIX
#endif
/*
 this function looks for
 user_pref("plugin.soname.list", "/usr/X11R6/lib/libXt.so.6:libXext.so");
 in user's pref.js
 and loads all libs in specified order
*/

static void LoadExtraSharedLibs()
{
    // check out if user's prefs.js has libs name
    nsresult res;
    nsCOMPtr<nsIPref> prefs = do_GetService(kPrefServiceCID, &res);
    if (NS_SUCCEEDED(res) && (prefs != nsnull)) {
        char *sonamesListFromPref = PREF_PLUGINS_SONAME;
        char *sonameList = NULL;
        PRBool prefSonameListIsSet = PR_TRUE;
        res = prefs->CopyCharPref(sonamesListFromPref, &sonameList);
        if (!sonameList) {
            // pref is not set, lets use hardcoded list
            prefSonameListIsSet = PR_FALSE;
            sonameList = PL_strdup(DEFAULT_EXTRA_LIBS_LIST);
        }
        if (sonameList) {
            char *arrayOfLibs[PLUGIN_MAX_NUMBER_OF_EXTRA_LIBS] = {0};
            int numOfLibs = 0;
            char *nextToken;
            char *p = nsCRT::strtok(sonameList,":",&nextToken);
            if (p) {
                while (p && numOfLibs < PLUGIN_MAX_NUMBER_OF_EXTRA_LIBS) {
                    arrayOfLibs[numOfLibs++] = p;
                    p = nsCRT::strtok(nextToken,":",&nextToken);
                }
            } else // there is just one lib
                arrayOfLibs[numOfLibs++] = sonameList;

            char sonameListToSave[PLUGIN_MAX_LEN_OF_TMP_ARR] = "";
            for (int i=0; i<numOfLibs; i++) {
                // trim out head/tail white spaces (just in case)
                PRBool head = PR_TRUE;
                p = arrayOfLibs[i];
                while (*p) {
                    if (*p == ' ' || *p == '\t') {
                        if (head) {
                            arrayOfLibs[i] = ++p;
                        } else {
                            *p = 0;
                        }
                    } else {
                        head = PR_FALSE;
                        p++;
                    }
                }
                if (!arrayOfLibs[i][0]) {
                    continue; // null string
                }
                PRBool tryToGetSoname = PR_TRUE;
                if (PL_strchr(arrayOfLibs[i], '/')) {
                    //assuming it's real name, try to stat it
                    struct stat st;
                    if (stat((const char*) arrayOfLibs[i], &st)) {
                        //get just a file name
                        arrayOfLibs[i] = PL_strrchr(arrayOfLibs[i], '/') + 1;
                    } else
                        tryToGetSoname = PR_FALSE;
                }
                char *soname = NULL;
                if (LoadExtraSharedLib(arrayOfLibs[i], &soname, tryToGetSoname)) {
                    //construct soname's list to save in prefs
                    p = soname ? soname : arrayOfLibs[i];
                    int n = PLUGIN_MAX_LEN_OF_TMP_ARR -
                        (PL_strlen(sonameListToSave) + PL_strlen(p));
                    if (n > 0) {
                        PL_strcat(sonameListToSave, p);
                        PL_strcat(sonameListToSave,":");
                    }
                    if (soname) {
                        PL_strfree(soname); // it's from strdup
                    }
                    if (numOfLibs > 1)
                        arrayOfLibs[i][PL_strlen(arrayOfLibs[i])] = ':'; //restore ":" in sonameList
                }
            }
            for (p = &sonameListToSave[PL_strlen(sonameListToSave) - 1]; *p == ':'; p--)
                *p = 0; //delete tail ":" delimiters

            if (!prefSonameListIsSet || PL_strcmp(sonameList, sonameListToSave)) {
                // if user specified some bogus soname I overwrite it here,
                // otherwise it'll decrease performance by calling popen() in SearchForSoname
                // every time for each bogus name
                prefs->SetCharPref(sonamesListFromPref, (const char *)sonameListToSave);
            }
            PL_strfree(sonameList);
        }
    }
}
#endif //MOZ_WIDGET_GTK



///////////////////////////////////////////////////////////////////////////

/* nsPluginsDir implementation */

PRBool nsPluginsDir::IsPluginFile(const nsFileSpec& fileSpec)
{
    const char* pathname = fileSpec.GetCString();
    PRBool ret = PR_FALSE;
    if (pathname) {
        int n = PL_strlen(pathname) - (sizeof(LOCAL_PLUGIN_DLL_SUFFIX) - 1); 
        if (n > 0 && !PL_strcmp(&pathname[n], LOCAL_PLUGIN_DLL_SUFFIX)) {
            ret  = PR_TRUE; // *.so or *.sl file
        }
#ifdef NS_DEBUG
        printf("IsPluginFile(%s) == %s\n", pathname, ret?"TRUE":"FALSE");
#endif
    }
    return ret;
}

///////////////////////////////////////////////////////////////////////////

/* nsPluginFile implementation */

nsPluginFile::nsPluginFile(const nsFileSpec& spec)
	:	nsFileSpec(spec)
{
    // nada
}

nsPluginFile::~nsPluginFile()
{
    // nada
}

/**
 * Loads the plugin into memory using NSPR's shared-library loading
 * mechanism. Handles platform differences in loading shared libraries.
 */
nsresult nsPluginFile::LoadPlugin(PRLibrary* &outLibrary)
{
    PRLibSpec libSpec;
    libSpec.type = PR_LibSpec_Pathname;
    libSpec.value.pathname = this->GetCString();

    pLibrary = outLibrary = PR_LoadLibraryWithFlags(libSpec, 0);

#ifdef MOZ_WIDGET_GTK

    ///////////////////////////////////////////////////////////
    // Normally, Mozilla isn't linked against libXt and libXext
    // since it's a Gtk/Gdk application.  On the other hand,
    // legacy plug-ins expect the libXt and libXext symbols
    // to already exist in the global name space.  This plug-in
    // wrapper is linked against libXt and libXext, but since
    // we never call on any of these libraries, plug-ins still
    // fail to resolve Xt symbols when trying to do a dlopen
    // at runtime.  Explicitly opening Xt/Xext into the global
    // namespace before attempting to load the plug-in seems to
    // work fine.
    if (!pLibrary) {
        LoadExtraSharedLibs();
        // try reload plugin ones more
        pLibrary = outLibrary = PR_LoadLibraryWithFlags(libSpec, 0);
        if (!pLibrary)
            DisplayPR_LoadLibraryErrorMessage(libSpec.value.pathname);
    }
#endif

#ifdef NS_DEBUG
    printf("LoadPlugin() %s returned %lx\n", 
           libSpec.value.pathname, (unsigned long)pLibrary);
#endif
    
    return NS_OK;
}

/**
 * Obtains all of the information currently available for this plugin.
 */
nsresult nsPluginFile::GetPluginInfo(nsPluginInfo& info)
{
    nsresult rv;
    const char* mimedescr = 0, *name = 0, *description = 0;

    // No, this doesn't leak. GetGlobalServiceManager() doesn't addref
    // it's out pointer. Maybe it should.
    nsIServiceManagerObsolete* mgr;
    nsServiceManager::GetGlobalServiceManager((nsIServiceManager**)&mgr);

    nsFactoryProc nsGetFactory =
        (nsFactoryProc) PR_FindSymbol(pLibrary, "NSGetFactory");

    nsCOMPtr<nsIPlugin> plugin;

    if (nsGetFactory) {
        // It's an almost-new-style plugin. The "truly new" plugins
        // are just XPCOM components, but there are some Mozilla
        // Classic holdovers that live in the plugins directory but
        // implement nsIPlugin and the factory stuff.
        static NS_DEFINE_CID(kPluginCID, NS_PLUGIN_CID);

        nsCOMPtr<nsIFactory> factory;
        rv = nsGetFactory(mgr, kPluginCID, nsnull, nsnull, 
			  getter_AddRefs(factory));
        if (NS_FAILED(rv)) return rv;

        plugin = do_QueryInterface(factory);
    } else {
        // It's old sk00l
        // if fileName parameter == 0 ns4xPlugin::CreatePlugin() will not call NP_Initialize()
        rv = ns4xPlugin::CreatePlugin(mgr, 0, 0, pLibrary, 
				      getter_AddRefs(plugin));
        if (NS_FAILED(rv)) return rv;
    }

    if (plugin) {
        plugin->GetMIMEDescription(&mimedescr);
#ifdef NS_DEBUG
        printf("GetMIMEDescription() returned \"%s\"\n", mimedescr);
#endif
	if (NS_FAILED(rv = ParsePluginMimeDescription(mimedescr, info)))
            return rv;
        info.fFileName = PL_strdup(this->GetCString());
        plugin->GetValue(nsPluginVariable_NameString, &name);
        if (!name)
            name = PL_strrchr(info.fFileName, '/') + 1;
        info.fName = PL_strdup(name);

        plugin->GetValue(nsPluginVariable_DescriptionString, &description);
        if (!description)
            description = "";
        info.fDescription = PL_strdup(description);
    }
    return NS_OK;
}


nsresult nsPluginFile::FreePluginInfo(nsPluginInfo& info)
{
    if(info.fName != nsnull)
        PL_strfree(info.fName);

    if(info.fDescription != nsnull)
        PL_strfree(info.fDescription);

    for(PRUint32 i = 0; i < info.fVariantCount; i++) {
        if (info.fMimeTypeArray[i] != nsnull)
            PL_strfree(info.fMimeTypeArray[i]);

        if (info.fMimeDescriptionArray[i] != nsnull)
            PL_strfree(info.fMimeDescriptionArray[i]);

        if(info.fExtensionArray[i] != nsnull)
            PL_strfree(info.fExtensionArray[i]);
    }

    PR_FREEIF(info.fMimeTypeArray);
    PR_FREEIF(info.fMimeDescriptionArray);
    PR_FREEIF(info.fExtensionArray);

    if(info.fFileName != nsnull)
        PL_strfree(info.fFileName);

    return NS_OK;
}
