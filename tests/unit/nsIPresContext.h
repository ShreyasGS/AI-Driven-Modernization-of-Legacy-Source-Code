#ifndef nsIPresContext_h
#define nsIPresContext_h

#include "nsIDOMNode.h"

// Define nsIPresContext interface
class nsIPresContext {
public:
    // nsISupports methods
    NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr) = 0;
    NS_IMETHOD_(nsrefcnt) AddRef(void) = 0;
    NS_IMETHOD_(nsrefcnt) Release(void) = 0;
    
    // Minimal interface for testing
};

#endif // nsIPresContext_h 