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
#ifndef nsJSEnvironment_h___
#define nsJSEnvironment_h___

#include "nsIScriptContext.h"
#include "nsCOMPtr.h"
#include "jsapi.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsIScriptSecurityManager.h"
#include "nsIXPCScriptNotify.h"
#include "nsITimerCallback.h"


class nsJSContext : public nsIScriptContext,
                    public nsIXPCScriptNotify,
                    public nsITimerCallback
{
public:
  nsJSContext(JSRuntime *aRuntime);
  virtual ~nsJSContext();

  NS_DECL_ISUPPORTS

  NS_IMETHOD       EvaluateString(const nsAString& aScript,
                                  void *aScopeObject,
                                  nsIPrincipal *principal,
                                  const char *aURL,
                                  PRUint32 aLineNo,
                                  const char* aVersion,
                                  nsAString& aRetValue,
                                  PRBool* aIsUndefined);
  NS_IMETHOD       EvaluateStringWithValue(const nsAString& aScript,
                                     void *aScopeObject,
                                     nsIPrincipal *aPrincipal,
                                     const char *aURL,
                                     PRUint32 aLineNo,
                                     const char* aVersion,
                                     void* aRetValue,
                                     PRBool* aIsUndefined);

  NS_IMETHOD       CompileScript(const PRUnichar* aText,
                                 PRInt32 aTextLength,
                                 void *aScopeObject,
                                 nsIPrincipal *principal,
                                 const char *aURL,
                                 PRUint32 aLineNo,
                                 const char* aVersion,
                                 void** aScriptObject);
  NS_IMETHOD       ExecuteScript(void* aScriptObject,
                                 void *aScopeObject,
                                 nsAString* aRetValue,
                                 PRBool* aIsUndefined);
  NS_IMETHOD       CompileEventHandler(void *aTarget,
                                       nsIAtom *aName,
                                       const nsAString& aBody,
                                       PRBool aShared,
                                       void** aHandler);
  NS_IMETHOD       CallEventHandler(void *aTarget, void *aHandler, 
                                    PRUint32 argc, void *argv, 
                                    PRBool *aBoolResult, PRBool aReverseReturnResult);
  NS_IMETHOD       BindCompiledEventHandler(void *aTarget,
                                            nsIAtom *aName,
                                            void *aHandler);
  NS_IMETHOD       CompileFunction(void* aTarget,
                                   const nsCString& aName,
                                   PRUint32 aArgCount,
                                   const char** aArgArray,
                                   const nsAString& aBody,
                                   const char* aURL,
                                   PRUint32 aLineNo,
                                   PRBool aShared,
                                   void** aFunctionObject);

  NS_IMETHOD SetDefaultLanguageVersion(const char* aVersion);
  NS_IMETHOD GetGlobalObject(nsIScriptGlobalObject** aGlobalObject);
  NS_IMETHOD_(void *) GetNativeContext();
  NS_IMETHOD InitContext(nsIScriptGlobalObject *aGlobalObject);
  NS_IMETHOD IsContextInitialized();
  NS_IMETHOD GC();
  NS_IMETHOD GetSecurityManager(nsIScriptSecurityManager** aInstancePtr);

  NS_IMETHOD ScriptEvaluated(PRBool aTerminated);
  NS_IMETHOD SetOwner(nsIScriptContextOwner* owner);
  NS_IMETHOD GetOwner(nsIScriptContextOwner** owner);
  NS_IMETHOD SetTerminationFunction(nsScriptTerminationFunc aFunc,
                                    nsISupports* aRef);
  NS_IMETHOD GetScriptsEnabled(PRBool *aEnabled);
  NS_IMETHOD SetScriptsEnabled(PRBool aEnabled, PRBool aFireTimeouts);

  NS_IMETHOD GetProcessingScriptTag(PRBool * aResult);
  NS_IMETHOD SetProcessingScriptTag(PRBool  aResult);

  NS_IMETHOD SetGCOnDestruction(PRBool aGCOnDestruction);

  NS_DECL_NSIXPCSCRIPTNOTIFY

  NS_IMETHOD_(void) Notify(nsITimer *timer);

protected:
  nsresult InitClasses();
  nsresult InitializeExternalClasses();
  nsresult InitializeLiveConnectClasses();

  void FireGCTimer();

private:
  JSContext *mContext;
  PRUint32 mNumEvaluations;

  nsCOMPtr<nsIScriptSecurityManager> mSecurityManager; // [OWNER]
  nsIScriptContextOwner* mOwner;  /* NB: weak reference, not ADDREF'd */
  nsScriptTerminationFunc mTerminationFunc;

  nsCOMPtr<nsISupports> mTerminationFuncArg;

  PRPackedBool mIsInitialized;
  PRPackedBool mScriptsEnabled;
  PRPackedBool mGCOnDestruction;
  PRPackedBool mProcessingScriptTag;

  PRUint32 mBranchCallbackCount;
  PRUint32 mDefaultJSOptions;

  // mGlobalWrapperRef is used only to hold a strong reference to the
  // global object wrapper while the nsJSContext is alive. This cuts
  // down on the number of rooting and unrooting calls XPConnect has
  // to make when the global object is touched in JS.

  nsCOMPtr<nsISupports> mGlobalWrapperRef;

  static int PR_CALLBACK JSOptionChangedCallback(const char *pref, void *data);

  static JSBool JS_DLL_CALLBACK DOMBranchCallback(JSContext *cx,
                                                  JSScript *script);
};

class nsIJSRuntimeService;

class nsJSEnvironment
{
private:
  static JSRuntime *sRuntime;
  static nsIJSRuntimeService *sRuntimeService;

public:
  static nsresult Init();

  static nsJSEnvironment *GetScriptingEnvironment();

  static nsresult CreateNewContext(nsIScriptContext **aContext);

  static void ShutDown();
};

/* prototypes */
void JS_DLL_CALLBACK NS_ScriptErrorReporter(JSContext *cx, const char *message, JSErrorReport *report);

#endif /* nsJSEnvironment_h___ */
