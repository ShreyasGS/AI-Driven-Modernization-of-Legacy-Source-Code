#ifndef nsCOMPtr_h
#define nsCOMPtr_h

#include "nsIDOMNode.h"

// Forward declarations
template <class T> class nsQueryInterface;

// Helper function for nsCOMPtr
template <class T>
inline void
getter_AddRefs(T*& aPtr)
{
    // Nothing to do here, just a placeholder for the real getter_AddRefs
}

// Simplified nsCOMPtr implementation for testing
template <class T>
class nsCOMPtr {
public:
    // Default constructor
    nsCOMPtr() : mRawPtr(nullptr) {}
    
    // Constructor from raw pointer
    explicit nsCOMPtr(T* aRawPtr) : mRawPtr(aRawPtr) {
        if (mRawPtr) {
            mRawPtr->AddRef();
        }
    }
    
    // Copy constructor
    nsCOMPtr(const nsCOMPtr<T>& aSmartPtr) : mRawPtr(aSmartPtr.mRawPtr) {
        if (mRawPtr) {
            mRawPtr->AddRef();
        }
    }
    
    // Destructor
    ~nsCOMPtr() {
        if (mRawPtr) {
            mRawPtr->Release();
        }
    }
    
    // Assignment operator
    nsCOMPtr<T>& operator=(const nsCOMPtr<T>& aRhs) {
        if (this != &aRhs) {
            if (mRawPtr) {
                mRawPtr->Release();
            }
            mRawPtr = aRhs.mRawPtr;
            if (mRawPtr) {
                mRawPtr->AddRef();
            }
        }
        return *this;
    }
    
    // Assignment from raw pointer
    nsCOMPtr<T>& operator=(T* aRhs) {
        if (mRawPtr != aRhs) {
            if (mRawPtr) {
                mRawPtr->Release();
            }
            mRawPtr = aRhs;
            if (mRawPtr) {
                mRawPtr->AddRef();
            }
        }
        return *this;
    }
    
    // Equality operators
    bool operator==(const nsCOMPtr<T>& aRhs) const {
        return mRawPtr == aRhs.mRawPtr;
    }
    
    bool operator==(const T* aRhs) const {
        return mRawPtr == aRhs;
    }
    
    bool operator!=(const nsCOMPtr<T>& aRhs) const {
        return mRawPtr != aRhs.mRawPtr;
    }
    
    bool operator!=(const T* aRhs) const {
        return mRawPtr != aRhs;
    }
    
    // Dereference operators
    T* operator->() const {
        return mRawPtr;
    }
    
    T& operator*() const {
        return *mRawPtr;
    }
    
    // Conversion to raw pointer
    operator T*() const {
        return mRawPtr;
    }
    
    // Get the raw pointer
    T* get() const {
        return mRawPtr;
    }
    
    // Forget the reference (return raw pointer without AddRef)
    T* forget() {
        T* tmp = mRawPtr;
        mRawPtr = nullptr;
        return tmp;
    }
    
private:
    T* mRawPtr;
};

// Helper function for nsCOMPtr with do_QueryInterface
template <class T>
inline nsCOMPtr<T>
do_QueryInterface(nsISupports* aRawPtr)
{
    T* ptr = nullptr;
    if (aRawPtr) {
        aRawPtr->QueryInterface(/* IID for T */, (void**)&ptr);
    }
    return nsCOMPtr<T>(ptr);
}

#endif // nsCOMPtr_h 