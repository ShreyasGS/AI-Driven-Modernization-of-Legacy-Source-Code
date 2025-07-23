#ifndef mock_nsTypedSelection_h
#define mock_nsTypedSelection_h

#include "nsIDOMNode.h"
#include "nsIDOMRange.h"
#include "nsCOMPtr.h"

// Forward declarations for XPCOM interfaces
class nsIPresContext;
class nsISupports;

// Mock implementation of nsIDOMNode for testing
class MockDOMNode : public nsIDOMNode {
public:
    MockDOMNode() : mRefCnt(0), mParent(nullptr) {}
    
    // nsISupports methods
    NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr) override {
        *aInstancePtr = this;
        return NS_OK;
    }
    
    NS_IMETHOD_(nsrefcnt) AddRef(void) override {
        return ++mRefCnt;
    }
    
    NS_IMETHOD_(nsrefcnt) Release(void) override {
        nsrefcnt count = --mRefCnt;
        if (count == 0) {
            delete this;
        }
        return count;
    }
    
    // nsIDOMNode methods (minimal implementation)
    NS_IMETHOD GetNodeName(nsAString& aNodeName) override { return NS_OK; }
    NS_IMETHOD GetNodeValue(nsAString& aNodeValue) override { return NS_OK; }
    NS_IMETHOD SetNodeValue(const nsAString& aNodeValue) override { return NS_OK; }
    NS_IMETHOD GetNodeType(uint16_t* aNodeType) override { return NS_OK; }
    NS_IMETHOD GetParentNode(nsIDOMNode** aParentNode) override {
        if (mParent) {
            *aParentNode = mParent;
            mParent->AddRef();
            return NS_OK;
        }
        *aParentNode = nullptr;
        return NS_OK;
    }
    
    // Set parent node for testing
    void SetParentNode(nsIDOMNode* aParent) {
        mParent = aParent;
    }
    
private:
    nsrefcnt mRefCnt;
    nsIDOMNode* mParent;
    
    // Add other required methods as needed
    NS_IMETHOD GetChildNodes(nsIDOMNodeList** aChildNodes) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD GetFirstChild(nsIDOMNode** aFirstChild) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD GetLastChild(nsIDOMNode** aLastChild) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD GetPreviousSibling(nsIDOMNode** aPreviousSibling) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD GetNextSibling(nsIDOMNode** aNextSibling) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD GetAttributes(nsIDOMNamedNodeMap** aAttributes) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD GetOwnerDocument(nsIDOMDocument** aOwnerDocument) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild, nsIDOMNode** aReturn) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild, nsIDOMNode** aReturn) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD HasChildNodes(bool* aReturn) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD CloneNode(bool aDeep, nsIDOMNode** aReturn) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD Normalize() override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD IsSupported(const nsAString& aFeature, const nsAString& aVersion, bool* aReturn) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD GetNamespaceURI(nsAString& aNamespaceURI) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD GetPrefix(nsAString& aPrefix) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD SetPrefix(const nsAString& aPrefix) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD GetLocalName(nsAString& aLocalName) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD HasAttributes(bool* aReturn) override { return NS_ERROR_NOT_IMPLEMENTED; }
};

// Mock implementation of nsIDOMRange for testing
class MockDOMRange : public nsIDOMRange {
public:
    MockDOMRange() : mRefCnt(0) {}
    
    // nsISupports methods
    NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr) override {
        *aInstancePtr = this;
        return NS_OK;
    }
    
    NS_IMETHOD_(nsrefcnt) AddRef(void) override {
        return ++mRefCnt;
    }
    
    NS_IMETHOD_(nsrefcnt) Release(void) override {
        nsrefcnt count = --mRefCnt;
        if (count == 0) {
            delete this;
        }
        return count;
    }
    
    // Minimal implementation of nsIDOMRange methods
    // Add methods as needed for testing
    
private:
    nsrefcnt mRefCnt;
    
    // Add other required methods as needed
    NS_IMETHOD GetStartContainer(nsIDOMNode** aStartContainer) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD GetStartOffset(int32_t* aStartOffset) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD GetEndContainer(nsIDOMNode** aEndContainer) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD GetEndOffset(int32_t* aEndOffset) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD GetCollapsed(bool* aCollapsed) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD GetCommonAncestorContainer(nsIDOMNode** aCommonAncestorContainer) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD SetStart(nsIDOMNode* aStartNode, int32_t aStartOffset) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD SetEnd(nsIDOMNode* aEndNode, int32_t aEndOffset) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD SetStartBefore(nsIDOMNode* aStartNode) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD SetStartAfter(nsIDOMNode* aStartNode) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD SetEndBefore(nsIDOMNode* aEndNode) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD SetEndAfter(nsIDOMNode* aEndNode) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD Collapse(bool aToStart) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD SelectNode(nsIDOMNode* aRefNode) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD SelectNodeContents(nsIDOMNode* aRefNode) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD CompareBoundaryPoints(uint16_t aHow, nsIDOMRange* aSourceRange, int16_t* aReturn) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD DeleteContents() override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD ExtractContents(nsIDOMDocumentFragment** aReturn) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD CloneContents(nsIDOMDocumentFragment** aReturn) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD InsertNode(nsIDOMNode* aNewNode) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD SurroundContents(nsIDOMNode* aNewParent) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD CloneRange(nsIDOMRange** aReturn) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD ToString(nsAString& aReturn) override { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD Detach() override { return NS_ERROR_NOT_IMPLEMENTED; }
};

// Simple array implementation for testing
class MockArray : public nsIArray {
public:
    MockArray() : mRefCnt(0), mElements{nullptr, nullptr, nullptr, nullptr, nullptr}, mCount(0) {}
    
    // nsISupports methods
    NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr) override {
        *aInstancePtr = this;
        return NS_OK;
    }
    
    NS_IMETHOD_(nsrefcnt) AddRef(void) override {
        return ++mRefCnt;
    }
    
    NS_IMETHOD_(nsrefcnt) Release(void) override {
        nsrefcnt count = --mRefCnt;
        if (count == 0) {
            delete this;
        }
        return count;
    }
    
    // nsIArray methods
    NS_IMETHOD GetLength(uint32_t* aLength) override {
        *aLength = mCount;
        return NS_OK;
    }
    
    NS_IMETHOD QueryElementAt(uint32_t aIndex, const nsIID& aIID, void** aResult) override {
        if (aIndex >= mCount) {
            return NS_ERROR_INVALID_ARG;
        }
        return mElements[aIndex]->QueryInterface(aIID, aResult);
    }
    
    NS_IMETHOD IndexOf(uint32_t aStartIndex, nsISupports* aElement, uint32_t* aResult) override {
        for (uint32_t i = aStartIndex; i < mCount; i++) {
            if (mElements[i] == aElement) {
                *aResult = i;
                return NS_OK;
            }
        }
        return NS_ERROR_FAILURE;
    }
    
    NS_IMETHOD Enumerate(nsIEnumerator** aResult) override {
        return NS_ERROR_NOT_IMPLEMENTED;
    }
    
    // Helper methods for testing
    void AppendElement(nsISupports* aElement) {
        if (mCount < 5) {
            mElements[mCount++] = aElement;
        }
    }
    
    nsISupports* ElementAt(uint32_t aIndex) {
        if (aIndex >= mCount) {
            return nullptr;
        }
        return mElements[aIndex];
    }
    
    uint32_t Count(uint32_t* aCount) {
        *aCount = mCount;
        return NS_OK;
    }
    
private:
    nsrefcnt mRefCnt;
    nsISupports* mElements[5]; // Fixed-size array for simplicity
    uint32_t mCount;
};

// Mock implementation of nsTypedSelection for testing
class nsTypedSelection {
public:
    nsTypedSelection() : mAnchorNode(nullptr), mAnchorOffset(0), mFocusNode(nullptr), mFocusOffset(0), mRangeArray(nullptr) {
        // Initialize with default values
    }
    
    ~nsTypedSelection() {
        // Clean up
    }
    
    // Set up test data
    void SetupTestData(nsIDOMNode* aAnchorNode, int32_t aAnchorOffset, nsIDOMNode* aFocusNode, int32_t aFocusOffset) {
        mAnchorNode = aAnchorNode;
        mAnchorOffset = aAnchorOffset;
        mFocusNode = aFocusNode;
        mFocusOffset = aFocusOffset;
    }
    
    void SetupRangeArray(nsIArray* aRangeArray) {
        mRangeArray = aRangeArray;
    }
    
    // Original methods to be tested
    NS_IMETHOD FetchAnchorParent(nsIDOMNode** _retval);
    NS_IMETHOD FetchAnchorOffset(int32_t* _retval);
    
    // Public members for testing
    nsCOMPtr<nsIDOMNode> mAnchorNode;
    int32_t mAnchorOffset;
    nsCOMPtr<nsIDOMNode> mFocusNode;
    int32_t mFocusOffset;
    nsCOMPtr<nsIArray> mRangeArray;
};

#endif // mock_nsTypedSelection_h 