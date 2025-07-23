#ifndef nsIDOMRange_h
#define nsIDOMRange_h

#include "nsIDOMNode.h"

// Define nsIDOMRange interface
class nsIDOMRange {
public:
    // nsISupports methods
    NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr) = 0;
    NS_IMETHOD_(nsrefcnt) AddRef(void) = 0;
    NS_IMETHOD_(nsrefcnt) Release(void) = 0;
    
    // nsIDOMRange methods
    NS_IMETHOD GetStartContainer(nsIDOMNode** aStartContainer) = 0;
    NS_IMETHOD GetStartOffset(int32_t* aStartOffset) = 0;
    NS_IMETHOD GetEndContainer(nsIDOMNode** aEndContainer) = 0;
    NS_IMETHOD GetEndOffset(int32_t* aEndOffset) = 0;
    NS_IMETHOD GetCollapsed(bool* aCollapsed) = 0;
    NS_IMETHOD GetCommonAncestorContainer(nsIDOMNode** aCommonAncestorContainer) = 0;
    NS_IMETHOD SetStart(nsIDOMNode* aStartNode, int32_t aStartOffset) = 0;
    NS_IMETHOD SetEnd(nsIDOMNode* aEndNode, int32_t aEndOffset) = 0;
    NS_IMETHOD SetStartBefore(nsIDOMNode* aStartNode) = 0;
    NS_IMETHOD SetStartAfter(nsIDOMNode* aStartNode) = 0;
    NS_IMETHOD SetEndBefore(nsIDOMNode* aEndNode) = 0;
    NS_IMETHOD SetEndAfter(nsIDOMNode* aEndNode) = 0;
    NS_IMETHOD Collapse(bool aToStart) = 0;
    NS_IMETHOD SelectNode(nsIDOMNode* aRefNode) = 0;
    NS_IMETHOD SelectNodeContents(nsIDOMNode* aRefNode) = 0;
    NS_IMETHOD CompareBoundaryPoints(uint16_t aHow, nsIDOMRange* aSourceRange, int16_t* aReturn) = 0;
    NS_IMETHOD DeleteContents() = 0;
    NS_IMETHOD ExtractContents(nsIDOMDocumentFragment** aReturn) = 0;
    NS_IMETHOD CloneContents(nsIDOMDocumentFragment** aReturn) = 0;
    NS_IMETHOD InsertNode(nsIDOMNode* aNewNode) = 0;
    NS_IMETHOD SurroundContents(nsIDOMNode* aNewParent) = 0;
    NS_IMETHOD CloneRange(nsIDOMRange** aReturn) = 0;
    NS_IMETHOD ToString(nsAString& aReturn) = 0;
    NS_IMETHOD Detach() = 0;
};

#endif // nsIDOMRange_h 