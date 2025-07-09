# XPCOM Modernization Example: nsConsoleService

This document demonstrates how to modernize a typical XPCOM component using modern C++ practices. We'll use `nsConsoleService` as our example.

## Original Implementation

Here's a simplified version of the original `nsConsoleService` implementation:

```cpp
// nsConsoleService.h
class nsConsoleService : public nsIConsoleService
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICONSOLESERVICE
    
    nsConsoleService();
    
private:
    ~nsConsoleService();
    
    PRUint32 mCurrent;
    PRBool mFull;
    nsIConsoleMessage **mMessages;
    PRUint32 mBufferSize;
    PRBool mListening;
    PRLock *mLock;
    nsSupportsHashtable mListeners;
};

// nsConsoleService.cpp
NS_IMPL_THREADSAFE_ISUPPORTS1(nsConsoleService, nsIConsoleService);

nsConsoleService::nsConsoleService()
    : mCurrent(0), mFull(PR_FALSE), mListening(PR_FALSE), mLock(nsnull)
{
    NS_INIT_REFCNT();
    mBufferSize = 250;
    mMessages = (nsIConsoleMessage **)
        nsMemory::Alloc(mBufferSize * sizeof(nsIConsoleMessage *));
    mLock = PR_NewLock();
    
    for (PRUint32 i = 0; i < mBufferSize; i++) {
        mMessages[i] = nsnull;
    }
}

nsConsoleService::~nsConsoleService()
{
    PRUint32 i = 0;
    while (i < mBufferSize && mMessages[i] != nsnull) {
        NS_RELEASE(mMessages[i]);
        i++;
    }
    
    nsMemory::Free(mMessages);
    if (mLock)
        PR_DestroyLock(mLock);
}

NS_IMETHODIMP
nsConsoleService::LogMessage(nsIConsoleMessage *message)
{
    if (message == nsnull)
        return NS_ERROR_INVALID_ARG;
        
    nsSupportsArray listenersSnapshot;
    nsIConsoleMessage *retiredMessage;
    
    NS_ADDREF(message); // early, in case it's same as replaced below.
    
    {
        nsAutoLock lock(mLock);
        retiredMessage = mMessages[mCurrent];
        mMessages[mCurrent++] = message;
        if (mCurrent == mBufferSize) {
            mCurrent = 0; // wrap around.
            mFull = PR_TRUE;
        }
        
        mListeners.Enumerate(snapshot_enum_func, &listenersSnapshot);
    }
    
    if (retiredMessage != nsnull)
        NS_RELEASE(retiredMessage);
        
    // Notify listeners...
    return NS_OK;
}
```

## Modernized Implementation

Here's how we can modernize the same component using modern C++ practices:

```cpp
// ModernConsoleService.h
class ModernConsoleService : public nsIConsoleService
{
public:
    NS_DECL_NSICONSOLESERVICE
    
    // Use standard refcounting with std::shared_ptr
    ModernConsoleService();
    
private:
    ~ModernConsoleService() = default; // Default destructor - no manual cleanup needed
    
    // Use modern C++ types
    size_t mCurrent = 0;
    bool mFull = false;
    bool mListening = false;
    size_t mBufferSize = 250;
    
    // Use RAII containers
    std::vector<nsCOMPtr<nsIConsoleMessage>> mMessages;
    std::mutex mMutex;
    std::unordered_map<nsISupports*, nsCOMPtr<nsIConsoleListener>> mListeners;
};

// ModernConsoleService.cpp
ModernConsoleService::ModernConsoleService()
    : mMessages(mBufferSize) // Pre-allocate vector with nullptr values
{
    // No manual initialization needed
}

NS_IMETHODIMP
ModernConsoleService::LogMessage(nsIConsoleMessage *aMessage)
{
    if (!aMessage)
        return NS_ERROR_INVALID_ARG;
        
    // Use smart pointer to manage reference count
    nsCOMPtr<nsIConsoleMessage> message = aMessage;
    
    // Snapshot of listeners using modern containers
    std::vector<nsCOMPtr<nsIConsoleListener>> listenersSnapshot;
    
    {
        // Modern thread safety with std::lock_guard
        std::lock_guard<std::mutex> lock(mMutex);
        
        // No need to manually release the old message - smart pointer handles it
        mMessages[mCurrent] = message;
        
        if (++mCurrent == mBufferSize) {
            mCurrent = 0;
            mFull = true;
        }
        
        // Copy listeners to snapshot
        listenersSnapshot.reserve(mListeners.size());
        for (const auto& entry : mListeners) {
            listenersSnapshot.push_back(entry.second);
        }
    }
    
    // Notify listeners with range-based for loop
    for (const auto& listener : listenersSnapshot) {
        listener->Observe(message);
    }
    
    return NS_OK;
}
```

## Compatibility Layer

To ensure backward compatibility during the transition, we can create a compatibility layer:

```cpp
// nsConsoleServiceModern.h - Compatibility header
#include "ModernConsoleService.h"

// Macro to create XPCOM-compatible implementation
NS_IMPL_ADDREF(ModernConsoleService)
NS_IMPL_RELEASE(ModernConsoleService)
NS_IMPL_QUERY_INTERFACE1(ModernConsoleService, nsIConsoleService)

// Factory function that returns nsIConsoleService interface
extern "C" NS_EXPORT nsresult
NS_NewConsoleService(nsIConsoleService** aResult)
{
    NS_PRECONDITION(aResult != nullptr, "null ptr");
    if (!aResult)
        return NS_ERROR_NULL_POINTER;
        
    ModernConsoleService* service = new ModernConsoleService();
    if (!service)
        return NS_ERROR_OUT_OF_MEMORY;
        
    NS_ADDREF(service);
    *aResult = service;
    return NS_OK;
}
```

## Key Modernization Patterns Applied

1. **Smart Pointers**
   - Replaced manual `NS_ADDREF`/`NS_RELEASE` with `nsCOMPtr<T>`
   - Automatic memory management for contained objects

2. **Modern C++ Types**
   - Replaced `PRUint32` with `size_t`
   - Replaced `PRBool` with `bool`
   - Used `std::vector` instead of manual array allocation

3. **RAII for Resource Management**
   - No manual lock creation/destruction
   - No manual memory allocation/deallocation

4. **Modern Threading**
   - Replaced `PRLock` with `std::mutex`
   - Replaced `nsAutoLock` with `std::lock_guard<std::mutex>`

5. **Modern Containers**
   - Replaced `nsSupportsHashtable` with `std::unordered_map`
   - Replaced C-style arrays with `std::vector`

6. **Modern Loops**
   - Replaced C-style loops with range-based for loops
   - Used container methods like `reserve()` for efficiency

## Migration Strategy

1. **Incremental Approach**
   - Create modern implementations alongside legacy code
   - Use compatibility layer to maintain XPCOM interfaces
   - Gradually migrate callers to use modern implementations

2. **Testing**
   - Create unit tests that verify identical behavior
   - Test both implementations with the same inputs
   - Ensure no regressions in functionality

3. **Documentation**
   - Document each modernization pattern applied
   - Create a mapping between old and new patterns
   - Provide migration guides for other components

## Next Steps

1. Apply this modernization pattern to other core XPCOM components
2. Create automated tools to assist with common transformations
3. Update the build system to support modern C++ features
4. Gradually remove compatibility layers as code is migrated 