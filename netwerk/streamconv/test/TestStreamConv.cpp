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

#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"
#include "nsIStreamConverterService.h"
#include "nsIStreamConverter.h"
#include "nsICategoryManager.h"
#include "nsIFactory.h"
#include "nsIStringStream.h"
#include "nsCOMPtr.h"
#include "nsNetUtil.h"

#include "nspr.h"

#define ASYNC_TEST // undefine this if you want to test sycnronous conversion.

/////////////////////////////////
// Event pump setup
/////////////////////////////////
#include "nsIEventQueueService.h"
#ifdef XP_WIN
#include <windows.h>
#endif
#ifdef XP_OS2
#include <os2.h>
#endif

static int gKeepRunning = 0;
static nsIEventQueue* gEventQ = nsnull;
/////////////////////////////////
// Event pump END
/////////////////////////////////


/////////////////////////////////
// Test converters include
/////////////////////////////////
#include "Converters.h"

// CID setup
static NS_DEFINE_CID(kEventQueueServiceCID,      NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kStreamConverterServiceCID, NS_STREAMCONVERTERSERVICE_CID);
static NS_DEFINE_CID(kComponentManagerCID,       NS_COMPONENTMANAGER_CID);

////////////////////////////////////////////////////////////////////////
// EndListener - This listener is the final one in the chain. It
//   receives the fully converted data, although it doesn't do anything with
//   the data.
////////////////////////////////////////////////////////////////////////
class EndListener : public nsIStreamListener {
public:
    // nsISupports declaration
    NS_DECL_ISUPPORTS

    EndListener() {NS_INIT_ISUPPORTS();};

    // nsIStreamListener method
    NS_IMETHOD OnDataAvailable(nsIRequest* request, nsISupports *ctxt, nsIInputStream *inStr, 
                               PRUint32 sourceOffset, PRUint32 count)
    {
        nsresult rv;
        PRUint32 read, len;
        rv = inStr->Available(&len);
        if (NS_FAILED(rv)) return rv;

        char *buffer = (char*)nsMemory::Alloc(len + 1);
        if (!buffer) return NS_ERROR_OUT_OF_MEMORY;

        rv = inStr->Read(buffer, len, &read);
        buffer[len] = '\0';
        if (NS_SUCCEEDED(rv)) {
            printf("CONTEXT %p: Received %u bytes and the following data: \n %s\n\n", ctxt, read, buffer);
        }
        nsMemory::Free(buffer);

        return NS_OK;
    }

    // nsIRequestObserver methods
    NS_IMETHOD OnStartRequest(nsIRequest* request, nsISupports *ctxt) { return NS_OK; }

    NS_IMETHOD OnStopRequest(nsIRequest* request, nsISupports *ctxt, 
                             nsresult aStatus) { return NS_OK; }
};

NS_IMPL_ISUPPORTS1(EndListener, nsIStreamListener);
////////////////////////////////////////////////////////////////////////
// EndListener END
////////////////////////////////////////////////////////////////////////


nsresult SendData(const char * aData, nsIStreamListener* aListener, nsIRequest* request) {
    nsString data;
    data.AssignWithConversion(aData);
    nsCOMPtr<nsIInputStream> dataStream;
    nsresult rv = NS_NewStringInputStream(getter_AddRefs(dataStream), data);
    if (NS_FAILED(rv)) return rv;

    return aListener->OnDataAvailable(request, nsnull, dataStream, 0, -1);
}
#define SEND_DATA(x) SendData(x, converterListener, request)

int
main(int argc, char* argv[])
{
    nsresult rv;
    nsCOMPtr<nsIServiceManager> servMan;
    NS_InitXPCOM2(getter_AddRefs(servMan), nsnull, nsnull);
    nsCOMPtr<nsIComponentRegistrar> registrar = do_QueryInterface(servMan);
    NS_ASSERTION(registrar, "Null nsIComponentRegistrar");
    registrar->AutoRegister(nsnull);
    
    // Create the Event Queue for this thread...
    nsCOMPtr<nsIEventQueueService> eventQService = 
             do_GetService(kEventQueueServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    eventQService->GetThreadEventQueue(NS_CURRENT_THREAD, &gEventQ);

    nsCOMPtr<nsICategoryManager> catman =
        do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;
    nsXPIDLCString previous;

    ///////////////////////////////////////////
    // BEGIN - Stream converter registration
    //   All stream converters must register with the ComponentManager
    ///////////////////////////////////////////

    // these stream converters are just for testing. running this harness
    // from the dist/bin dir will also pickup converters registered
    // in other modules (necko converters for example).

    PRUint32 converterListSize = 7;
    const char *const converterList[] = {
        "?from=a/foo&to=b/foo",
        "?from=b/foo&to=c/foo",
        "?from=b/foo&to=d/foo",
        "?from=c/foo&to=d/foo",
        "?from=d/foo&to=e/foo",
        "?from=d/foo&to=f/foo",
        "?from=t/foo&to=k/foo",
    };

    TestConverterFactory *convFactory = new TestConverterFactory(kTestConverterCID, "TestConverter", NS_ISTREAMCONVERTER_KEY);
    nsCOMPtr<nsIFactory> convFactSup(do_QueryInterface(convFactory, &rv));
    if (NS_FAILED(rv)) return rv;

    PRUint32 count = 0;
    while (count < converterListSize) {
        // register the TestConverter with the component manager. One contractid registration
        // per conversion pair (from - to pair).
        nsCString contractID(NS_ISTREAMCONVERTER_KEY);
        contractID.Append(converterList[count]);
        rv = nsComponentManager::RegisterFactory(kTestConverterCID,
                                                 "TestConverter",
                                                 contractID.get(),
                                                 convFactSup,
                                                 PR_TRUE);
        if (NS_FAILED(rv)) return rv;
        rv = catman->AddCategoryEntry(NS_ISTREAMCONVERTER_KEY, converterList[count], "x",
                                        PR_TRUE, PR_TRUE, getter_Copies(previous));
        if (NS_FAILED(rv)) return rv;
        count++;
    }

    nsCOMPtr<nsIStreamConverterService> StreamConvService = 
             do_GetService(kStreamConverterServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    // Define the *from* content type and *to* content-type for conversion.
    nsString fromStr;
    fromStr.Assign(NS_LITERAL_STRING("a/foo"));
    nsString toStr;
    toStr.Assign(NS_LITERAL_STRING("c/foo"));
    
#ifdef ASYNC_TEST
    // ASYNCRONOUS conversion


    // Build up a channel that represents the content we're
    // starting the transaction with.
    //
    // sample multipart mixed content-type string:
    // "multipart/x-mixed-replacE;boundary=thisrandomstring"
    /*nsCOMPtr<nsIChannel> channel;
    nsCOMPtr<nsIURI> dummyURI;
    rv = NS_NewURI(getter_AddRefs(dummyURI), "http://meaningless");
    if (NS_FAILED(rv)) return rv;

    rv = NS_NewInputStreamChannel(getter_AddRefs(channel),
                                  dummyURI,
                                  nsnull,   // inStr
                                  "text/plain", // content-type
                                  -1);      // XXX fix contentLength
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIRequest> request(do_QueryInterface(channel));*/

    nsCOMPtr<nsIRequest> request;

    // setup a listener to receive the converted data. This guy is the end
    // listener in the chain, he wants the fully converted (toType) data.
    // An example of this listener in mozilla would be the DocLoader.
    nsIStreamListener *dataReceiver = new EndListener();
    NS_ADDREF(dataReceiver);

    // setup a listener to push the data into. This listener sits inbetween the
    // unconverted data of fromType, and the final listener in the chain (in this case
    // the dataReceiver.
    nsIStreamListener *converterListener = nsnull;
    rv = StreamConvService->AsyncConvertData(fromStr.get(), toStr.get(), 
                                             dataReceiver, nsnull, &converterListener);
    if (NS_FAILED(rv)) return rv;
    NS_RELEASE(dataReceiver);

    // at this point we have a stream listener to push data to, and the one
    // that will receive the converted data. Let's mimic On*() calls and get the conversion
    // going. Typically these On*() calls would be made inside their respective wrappers On*()
    // methods.
    rv = converterListener->OnStartRequest(request, nsnull);
    if (NS_FAILED(rv)) return rv;


    rv = SEND_DATA("aaa");
    if (NS_FAILED(rv)) return rv;
    
    rv = SEND_DATA("aaa");
    if (NS_FAILED(rv)) return rv;    

    // Finish the request.
    rv = converterListener->OnStopRequest(request, nsnull, rv);
    if (NS_FAILED(rv)) return rv;

    NS_RELEASE(converterListener);


#else
    // SYNCRONOUS conversion
    nsCOMPtr<nsIInputStream> convertedData;
    rv = StreamConvService->Convert(inputData, fromStr.get(), toStr.get(), 
                                    nsnull, getter_AddRefs(convertedData));
    if (NS_FAILED(rv)) return rv;
#endif

    // Enter the message pump to allow the URL load to proceed.
    while ( gKeepRunning ) {
#ifdef XP_WIN
        MSG msg;

        if (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else
            gKeepRunning = PR_FALSE;
#else
#ifdef XP_MAC
        /* Mac stuff is missing here! */
#else
#ifdef XP_OS2
        QMSG qmsg;

        if (WinGetMsg(0, &qmsg, 0, 0, 0))
            WinDispatchMsg(0, &qmsg);
        else
            gKeepRunning = PR_FALSE;
#else
#ifdef XP_UNIX
        PLEvent *gEvent;
        rv = gEventQ->GetEvent(&gEvent);
        rv = gEventQ->HandleEvent(gEvent);
        /* gKeepRunning = PR_FALSE; */
#else
        /* Other stuff is missing here! */
#endif /* XP_UNIX */
#endif /* XP_OS2 */
#endif /* XP_MAC */
#endif /* XP_WIN */
    }

    //return NS_ShutdownXPCOM(NULL);
    return rv;
}
