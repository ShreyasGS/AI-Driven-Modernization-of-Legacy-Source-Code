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

#ifndef nsPluginHostImpl_h__
#define nsPluginHostImpl_h__

#include "nsIPluginManager.h"
#include "nsIPluginManager2.h"
#include "nsIPluginHost.h"
#include "nsIObserver.h"
#include "nsPIPluginHost.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "prlink.h"
#include "nsIFileUtilities.h"
#include "nsICookieStorage.h"
#include "nsPluginsDir.h"
#include "nsVoidArray.h"  // array for holding "active" streams
#include "nsIDirectoryService.h"

class ns4xPlugin;
class nsFileSpec;
class nsIComponentManager;
class nsIFile;
class nsIChannel;
class nsIRegistry;

/**
 * A linked-list of plugin information that is used for
 * instantiating plugins and reflecting plugin information
 * into JavaScript.
 */
class nsPluginTag
{
public:
  nsPluginTag();
  nsPluginTag(nsPluginTag* aPluginTag);
  nsPluginTag(nsPluginInfo* aPluginInfo);

  nsPluginTag(const char* aName,
              const char* aDescription,
              const char* aFileName,
              const char* aFullPath,
              const char* const* aMimeTypes,
              const char* const* aMimeDescriptions,
              const char* const* aExtensions,
              PRInt32 aVariants,
              PRInt64 aLastModifiedTime = 0,
              PRBool aCanUnload = PR_TRUE);

  ~nsPluginTag();

  void TryUnloadPlugin(PRBool aForceShutdown = PR_FALSE);
  void Mark(PRUint32 mask) { mFlags |= mask; }
  PRBool Equals(nsPluginTag* aPluginTag);

  nsPluginTag   *mNext;
  char          *mName;
  char          *mDescription;
  PRInt32       mVariants;
  char          **mMimeTypeArray;
  char          **mMimeDescriptionArray;
  char          **mExtensionsArray;
  PRLibrary     *mLibrary;
  PRBool        mCanUnloadLibrary;
  nsIPlugin     *mEntryPoint;
  PRUint32      mFlags;
  PRBool        mXPConnected;
  char          *mFileName;
  char          *mFullPath;
  PRInt64       mLastModifiedTime;
};

struct nsActivePlugin
{
  nsActivePlugin*        mNext;
  char*                  mURL;
  nsIPluginInstancePeer* mPeer;
  nsPluginTag*           mPluginTag;
  nsIPluginInstance*     mInstance;
  PRBool                 mStopped;
  PRTime                 mllStopTime;
  PRBool                 mDefaultPlugin;
  PRBool                 mXPConnected;
  nsVoidArray*           mStreams;

  nsActivePlugin(nsPluginTag* aPluginTag,
                 nsIPluginInstance* aInstance, 
                 const char * url,
                 PRBool aDefaultPlugin);
  ~nsActivePlugin();

  void setStopped(PRBool stopped);
};

class nsActivePluginList
{
public:
  nsActivePlugin * mFirst;
  nsActivePlugin * mLast;
  PRInt32 mCount;

  nsActivePluginList();
  ~nsActivePluginList();

  void shut();
  PRBool add(nsActivePlugin * plugin);
  PRBool remove(nsActivePlugin * plugin, PRBool * aUnloadLibraryLater);
  nsActivePlugin * find(nsIPluginInstance* instance);
  nsActivePlugin * find(const char * mimetype);
  nsActivePlugin * findStopped(const char * url);
  PRUint32 getStoppedCount();
  nsActivePlugin * findOldestStopped();
  void removeAllStopped();
  void stopRunning();
  PRBool IsLastInstance(nsActivePlugin * plugin);
};

// The purpose of this list is to keep track of unloaded plugin libs
// we need to keep some libs in memory when we destroy mPlugins list
// during refresh with reload if the plugin is currently running
// on the page. They should be unloaded later, see bug #61388
// There could also be other reasons to have this list. XPConnected
// plugins e.g. may still be held at the time we normally unload the library
class nsUnusedLibrary
{
public:
  nsUnusedLibrary *mNext;
  PRLibrary *mLibrary;

  nsUnusedLibrary(PRLibrary * aLibrary);
  ~nsUnusedLibrary();
};

#define NS_PLUGIN_FLAG_ENABLED    0x0001    //is this plugin enabled?
#define NS_PLUGIN_FLAG_OLDSCHOOL  0x0002    //is this a pre-xpcom plugin?
#define NS_PLUGIN_FLAG_FROMCACHE  0x0004    // this plugintag info was loaded from cache
#define NS_PLUGIN_FLAG_UNWANTED   0x0008    // this is an unwanted plugin

class nsPluginHostImpl : public nsIPluginManager2,
                         public nsIPluginHost,
                         public nsIFileUtilities,
                         public nsICookieStorage,
                         public nsIObserver,
                         public nsPIPluginHost
{
public:
  nsPluginHostImpl();
  virtual ~nsPluginHostImpl();

  static NS_METHOD
  Create(nsISupports* aOuter, REFNSIID aIID, void** aResult);

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  NS_DECL_ISUPPORTS

  //nsIPluginManager interface - the main interface nsIPlugin communicates to

  NS_IMETHOD
  GetValue(nsPluginManagerVariable variable, void *value);

  NS_IMETHOD
  ReloadPlugins(PRBool reloadPages);

  NS_IMETHOD
  UserAgent(const char* *resultingAgentString);

  NS_IMETHOD
  GetURL(nsISupports* pluginInst, 
           const char* url, 
           const char* target = NULL,
           nsIPluginStreamListener* streamListener = NULL,
           const char* altHost = NULL,
           const char* referrer = NULL,
           PRBool forceJSEnabled = PR_FALSE);

  NS_IMETHOD
  GetURLWithHeaders(nsISupports* pluginInst, 
                    const char* url, 
                    const char* target = NULL,
                    nsIPluginStreamListener* streamListener = NULL,
                    const char* altHost = NULL,
                    const char* referrer = NULL,
                    PRBool forceJSEnabled = PR_FALSE,
                    PRUint32 getHeadersLength = 0, 
                    const char* getHeaders = NULL);
  
  NS_IMETHOD
  PostURL(nsISupports* pluginInst,
            const char* url,
            PRUint32 postDataLen, 
            const char* postData,
            PRBool isFile = PR_FALSE,
            const char* target = NULL,
            nsIPluginStreamListener* streamListener = NULL,
            const char* altHost = NULL, 
            const char* referrer = NULL,
            PRBool forceJSEnabled = PR_FALSE,
            PRUint32 postHeadersLength = 0, 
            const char* postHeaders = NULL);

  NS_IMETHOD
  RegisterPlugin(REFNSIID aCID,
                 const char* aPluginName,
                 const char* aDescription,
                 const char** aMimeTypes,
                 const char** aMimeDescriptions,
                 const char** aFileExtensions,
                 PRInt32 aCount);

  NS_IMETHOD
  UnregisterPlugin(REFNSIID aCID);

  //nsIPluginHost interface - used to communicate to the nsPluginInstanceOwner

  NS_IMETHOD
  Init(void);

  NS_IMETHOD
  Destroy(void);

  NS_IMETHOD
  LoadPlugins(void);

  NS_IMETHOD
  GetPluginFactory(const char *aMimeType, nsIPlugin** aPlugin);

  NS_IMETHOD
  InstantiateEmbededPlugin(const char *aMimeType, nsIURI* aURL, nsIPluginInstanceOwner *aOwner);

  NS_IMETHOD
  InstantiateFullPagePlugin(const char *aMimeType, nsString& aURLSpec, nsIStreamListener *&aStreamListener, nsIPluginInstanceOwner *aOwner);

  NS_IMETHOD
  SetUpPluginInstance(const char *aMimeType, nsIURI *aURL, nsIPluginInstanceOwner *aOwner);

  NS_IMETHOD
  IsPluginEnabledForType(const char* aMimeType);

  NS_IMETHOD
  IsPluginEnabledForExtension(const char* aExtension, const char* &aMimeType);

  NS_IMETHOD
  GetPluginCount(PRUint32* aPluginCount);
  
  NS_IMETHOD
  GetPlugins(PRUint32 aPluginCount, nsIDOMPlugin* aPluginArray[]);

  NS_IMETHOD
  HandleBadPlugin(PRLibrary* aLibrary);

  //nsIPluginManager2 interface - secondary methods that nsIPlugin communicates to

  NS_IMETHOD
  BeginWaitCursor(void);

  NS_IMETHOD
  EndWaitCursor(void);

  NS_IMETHOD
  SupportsURLProtocol(const char* protocol, PRBool *result);

  NS_IMETHOD
  NotifyStatusChange(nsIPlugin* plugin, nsresult errorStatus);
  
  NS_IMETHOD
  FindProxyForURL(const char* url, char* *result);

  NS_IMETHOD
  RegisterWindow(nsIEventHandler* handler, nsPluginPlatformWindowRef window);
  
  NS_IMETHOD
  UnregisterWindow(nsIEventHandler* handler, nsPluginPlatformWindowRef window);

  NS_IMETHOD
  AllocateMenuID(nsIEventHandler* handler, PRBool isSubmenu, PRInt16 *result);

  NS_IMETHOD
  DeallocateMenuID(nsIEventHandler* handler, PRInt16 menuID);

  NS_IMETHOD
  HasAllocatedMenuID(nsIEventHandler* handler, PRInt16 menuID, PRBool *result);

  NS_IMETHOD
  ProcessNextEvent(PRBool *bEventHandled);

  // nsIFactory interface, from nsIPlugin.
  // XXX not currently used?
  NS_IMETHOD CreateInstance(nsISupports *aOuter,
                            REFNSIID aIID,
                            void **aResult);

  NS_IMETHOD LockFactory(PRBool aLock);

  // nsIFileUtilities interface

  NS_IMETHOD GetProgramPath(const char* *result);

  NS_IMETHOD GetTempDirPath(const char* *result);

  NS_IMETHOD NewTempFileName(const char* prefix, PRUint32 bufLen, char* resultBuf);

  // nsICookieStorage interface

  /**
   * Retrieves a cookie from the browser's persistent cookie store.
   * @param inCookieURL        URL string to look up cookie with.
   * @param inOutCookieBuffer  buffer large enough to accomodate cookie data.
   * @param inOutCookieSize    on input, size of the cookie buffer, on output cookie's size.
   */
  NS_IMETHOD
  GetCookie(const char* inCookieURL, void* inOutCookieBuffer, PRUint32& inOutCookieSize);
  
  /**
   * Stores a cookie in the browser's persistent cookie store.
   * @param inCookieURL        URL string store cookie with.
   * @param inCookieBuffer     buffer containing cookie data.
   * @param inCookieSize       specifies  size of cookie data.
   */
  NS_IMETHOD
  SetCookie(const char* inCookieURL, const void* inCookieBuffer, PRUint32 inCookieSize);
  
  // Methods from nsIObserver
  NS_IMETHOD
  Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData);

  // Methods from nsPIPluginHost
  NS_IMETHOD
  SetIsScriptableInstance(nsCOMPtr<nsIPluginInstance> aPluginInstance, PRBool aScriptable);

  NS_IMETHOD
  ParsePostBufferToFixHeaders(const char *inPostData, PRUint32 inPostDataLen, 
              char **outPostData, PRUint32 *outPostDataLen);
  
  NS_IMETHOD
  CreateTmpFileToPost(const char *postDataURL, char **pTmpFileName);

  /* Called by GetURL and PostURL */

  NS_IMETHOD
  NewPluginURLStream(const nsString& aURL, 
                     nsIPluginInstance *aInstance, 
                     nsIPluginStreamListener *aListener,
                     const char *aPostData = nsnull, 
                     PRBool isFile = PR_FALSE,
                     PRUint32 aPostDataLen = 0, 
                     const char *aHeadersData = nsnull, 
                     PRUint32 aHeadersDataLen = 0);

  NS_IMETHOD
  AddHeadersToChannel(const char *aHeadersData, PRUint32 aHeadersDataLen, 
                      nsIChannel *aGenericChannel);

  NS_IMETHOD
  StopPluginInstance(nsIPluginInstance* aInstance);

private:
  nsresult
  LoadXPCOMPlugins(nsIComponentManager* aComponentManager, nsIFile* aPath);

  nsresult
  NewEmbededPluginStream(nsIURI* aURL, nsIPluginInstanceOwner *aOwner, nsIPluginInstance* aInstance);

  nsresult
  NewFullPagePluginStream(nsIStreamListener *&aStreamListener, nsIPluginInstance *aInstance);

  nsresult
  FindPluginEnabledForType(const char* aMimeType, nsPluginTag* &aPlugin);

  nsresult
  FindStoppedPluginForURL(nsIURI* aURL, nsIPluginInstanceOwner *aOwner);

  nsresult
  SetUpDefaultPluginInstance(const char *aMimeType, nsIURI *aURL, nsIPluginInstanceOwner *aOwner);

  void
  AddInstanceToActiveList(nsCOMPtr<nsIPlugin> aPlugin,
                          nsIPluginInstance* aInstance,
                          nsIURI* aURL, PRBool aDefaultPlugin);

  nsresult 
  RegisterPluginMimeTypesWithLayout(nsPluginTag *pluginTag, nsIComponentManager * compManager, nsIFile * layoutPath);

  nsresult
  FindPlugins(PRBool aCreatePluginList, PRBool * aPluginsChanged);

  nsresult
  ScanPluginsDirectory(nsIFile * pluginsDir, 
                       nsIComponentManager * compManager, 
                       nsIFile * layoutPath,
                       PRBool aCreatePluginList,
                       PRBool * aPluginsChanged,
                       PRBool checkForUnwantedPlugins = PR_FALSE);
                       
  nsresult
  ScanPluginsDirectoryList(nsISimpleEnumerator * dirEnum,
                           nsIComponentManager * compManager, 
                           nsIFile * layoutPath,
                           PRBool aCreatePluginList,
                           PRBool * aPluginsChanged,
                           PRBool checkForUnwantedPlugins = PR_FALSE);

  PRBool IsRunningPlugin(nsPluginTag * plugin);
  void AddToUnusedLibraryList(PRLibrary * aLibrary);
  void CleanUnusedLibraries();

  // Loads all cached plugins info into mCachedPlugins
  nsresult LoadCachedPluginsInfo(nsIRegistry* registry);

  // Stores all plugins info into the registry
  nsresult CachePluginsInfo(nsIRegistry* registry);

  // Given a filename, returns the plugins info from our cache
  // and removes it from the cache.
  nsPluginTag* RemoveCachedPluginsInfo(const char *filename);

  //checks if the list already have the same plugin as given
  nsPluginTag* HaveSamePlugin(nsPluginTag * aPluginTag);

  // checks if given plugin is a duplicate of what we already have
  // in the plugin list but found in some different place
  PRBool IsDuplicatePlugin(nsPluginTag * aPluginTag);
  
  // destroys plugin info list
  void ClearCachedPluginInfoList();
  
  nsresult EnsurePrivateDirServiceProvider();

  // one-off hack to include nppl3260.dll from the components folder
  nsresult ScanForRealInComponentsFolder(nsIComponentManager * aCompManager, nsIFile * aLayoutPath);

  char        *mPluginPath;
  nsPluginTag *mPlugins;
  nsPluginTag *mCachedPlugins;
  PRBool      mPluginsLoaded;
  PRBool      mDontShowBadPluginMessage;
  PRBool      mIsDestroyed;
  PRBool      mOverrideInternalTypes; // set by pref plugin.override_internal_types
  PRBool      mAllowAlienStarHandler; // set by pref plugin.allow_alien_star_handler

  nsActivePluginList mActivePluginList;
  nsUnusedLibrary *mUnusedLibraries;

  nsCOMPtr<nsIDirectoryServiceProvider> mPrivateDirServiceProvider;  
};

#endif
