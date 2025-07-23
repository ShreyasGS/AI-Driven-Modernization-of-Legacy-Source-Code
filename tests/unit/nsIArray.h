#ifndef nsIArray_h
#define nsIArray_h

#include "nsIDOMNode.h"

// Forward declarations
class nsIEnumerator;

// Define nsIArray interface
class nsIArray : public nsISupports {
public:
    NS_IMETHOD GetLength(uint32_t* aLength) = 0;
    NS_IMETHOD QueryElementAt(uint32_t aIndex, const nsIID& aIID, void** aResult) = 0;
    NS_IMETHOD IndexOf(uint32_t aStartIndex, nsISupports* aElement, uint32_t* aResult) = 0;
    NS_IMETHOD Enumerate(nsIEnumerator** aResult) = 0;
};

#endif // nsIArray_h 