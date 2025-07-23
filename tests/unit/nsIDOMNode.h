#ifndef nsIDOMNode_h
#define nsIDOMNode_h

// Define basic types without cstdint
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef int int16_t;
typedef unsigned int nsrefcnt;
typedef unsigned int nsresult;

// Define error codes
#define NS_OK                     0
#define NS_ERROR_NULL_POINTER     0x80004003
#define NS_ERROR_FAILURE          0x80004005
#define NS_ERROR_INVALID_ARG      0x80070057
#define NS_ERROR_NOT_IMPLEMENTED  0x80004001

// Define success/failure macros
#define NS_SUCCEEDED(rv) (((nsresult)(rv) & 0x80000000) == 0)
#define NS_FAILED(rv)    (((nsresult)(rv) & 0x80000000) != 0)

// Define basic COM interface methods
#define NS_IMETHOD virtual nsresult
#define NS_IMETHOD_(type) virtual type

// Forward declarations
class nsAString;
class nsIID;
class nsIDOMNodeList;
class nsIDOMNamedNodeMap;
class nsIDOMDocument;
class nsIDOMDocumentFragment;

// Define nsIDOMNode interface
class nsIDOMNode {
public:
    // nsISupports methods
    NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr) = 0;
    NS_IMETHOD_(nsrefcnt) AddRef(void) = 0;
    NS_IMETHOD_(nsrefcnt) Release(void) = 0;
    
    // nsIDOMNode methods
    NS_IMETHOD GetNodeName(nsAString& aNodeName) = 0;
    NS_IMETHOD GetNodeValue(nsAString& aNodeValue) = 0;
    NS_IMETHOD SetNodeValue(const nsAString& aNodeValue) = 0;
    NS_IMETHOD GetNodeType(uint16_t* aNodeType) = 0;
    NS_IMETHOD GetParentNode(nsIDOMNode** aParentNode) = 0;
    NS_IMETHOD GetChildNodes(nsIDOMNodeList** aChildNodes) = 0;
    NS_IMETHOD GetFirstChild(nsIDOMNode** aFirstChild) = 0;
    NS_IMETHOD GetLastChild(nsIDOMNode** aLastChild) = 0;
    NS_IMETHOD GetPreviousSibling(nsIDOMNode** aPreviousSibling) = 0;
    NS_IMETHOD GetNextSibling(nsIDOMNode** aNextSibling) = 0;
    NS_IMETHOD GetAttributes(nsIDOMNamedNodeMap** aAttributes) = 0;
    NS_IMETHOD GetOwnerDocument(nsIDOMDocument** aOwnerDocument) = 0;
    NS_IMETHOD InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild, nsIDOMNode** aReturn) = 0;
    NS_IMETHOD ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild, nsIDOMNode** aReturn) = 0;
    NS_IMETHOD RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn) = 0;
    NS_IMETHOD AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn) = 0;
    NS_IMETHOD HasChildNodes(bool* aReturn) = 0;
    NS_IMETHOD CloneNode(bool aDeep, nsIDOMNode** aReturn) = 0;
    NS_IMETHOD Normalize() = 0;
    NS_IMETHOD IsSupported(const nsAString& aFeature, const nsAString& aVersion, bool* aReturn) = 0;
    NS_IMETHOD GetNamespaceURI(nsAString& aNamespaceURI) = 0;
    NS_IMETHOD GetPrefix(nsAString& aPrefix) = 0;
    NS_IMETHOD SetPrefix(const nsAString& aPrefix) = 0;
    NS_IMETHOD GetLocalName(nsAString& aLocalName) = 0;
    NS_IMETHOD HasAttributes(bool* aReturn) = 0;
};

#endif // nsIDOMNode_h 