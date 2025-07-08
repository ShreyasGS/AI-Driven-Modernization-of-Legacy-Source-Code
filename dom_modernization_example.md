# DOM Modernization Example: Text Node Implementation

This document demonstrates how to modernize a DOM component from the Mozilla 1.0 codebase using modern C++ techniques. We'll focus on the `nsTextNode` implementation as an example.

## Original Implementation

The original text node implementation has several characteristics typical of legacy C++ code:

1. Manual reference counting
2. Raw pointer usage
3. Out parameters for return values
4. Error codes for error handling

Here's a simplified version of the original implementation:

```cpp
// nsTextNode.h
class nsTextNode : public nsGenericDOMDataNode,
                   public nsIDOMText
{
public:
  nsTextNode();
  virtual ~nsTextNode();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_IMPL_NSIDOMNODE_USING_GENERIC_DOM_DATA

  // nsIDOMCharacterData
  NS_FORWARD_NSIDOMCHARACTERDATA(nsGenericDOMDataNode::)

  // nsIDOMText
  NS_FORWARD_NSIDOMTEXT(nsGenericDOMDataNode::)

  // nsIContent
  NS_IMETHOD GetTag(nsIAtom*& aResult) const;
  NS_IMETHOD_(PRBool) IsContentOfType(PRUint32 aFlags);
};

// nsTextNode.cpp
nsresult
NS_NewTextNode(nsIContent** aInstancePtrResult)
{
  *aInstancePtrResult = new nsTextNode();
  NS_ENSURE_TRUE(*aInstancePtrResult, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aInstancePtrResult);

  return NS_OK;
}

nsTextNode::nsTextNode()
{
}

nsTextNode::~nsTextNode()
{
}

NS_IMPL_ADDREF_INHERITED(nsTextNode, nsGenericDOMDataNode)
NS_IMPL_RELEASE_INHERITED(nsTextNode, nsGenericDOMDataNode)

NS_IMETHODIMP
nsTextNode::SplitText(PRUint32 aOffset, nsIDOMText** aReturn)
{
  nsresult rv;
  nsAutoString cutText;
  PRUint32 length = 0;

  rv = GetLength(&length);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (aOffset > length) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  rv = SubstringData(aOffset, length - aOffset, cutText);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = DeleteData(aOffset, length - aOffset);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsIContent> parent = GetParent();
  if (!parent) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  nsCOMPtr<nsIContent> newContent;
  rv = NS_NewTextNode(getter_AddRefs(newContent));
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsITextContent> tc = do_QueryInterface(newContent);
  rv = tc->SetText(cutText, PR_FALSE);
  if (NS_FAILED(rv)) {
    return rv;
  }

  PRInt32 offsetInParent = -1;
  parent->IndexOf(this, offsetInParent);
  if (offsetInParent < 0) {
    return NS_ERROR_FAILURE;
  }

  rv = parent->InsertChildAt(newContent, offsetInParent + 1, PR_TRUE, PR_TRUE);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return CallQueryInterface(newContent, aReturn);
}
```

## Modernized Implementation

Here's how we can modernize the same component using modern C++ techniques:

```cpp
// nsTextNode.h
class nsTextNode : public nsGenericDOMDataNode,
                   public nsIDOMText
{
public:
  // Factory method instead of global creation function
  static RefPtr<nsTextNode> Create();
  
  nsTextNode();
  virtual ~nsTextNode();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_IMPL_NSIDOMNODE_USING_GENERIC_DOM_DATA

  // nsIDOMCharacterData
  NS_FORWARD_NSIDOMCHARACTERDATA(nsGenericDOMDataNode::)

  // nsIDOMText
  NS_FORWARD_NSIDOMTEXT(nsGenericDOMDataNode::)

  // nsIContent
  NS_IMETHOD GetTag(nsIAtom*& aResult) const;
  NS_IMETHOD_(PRBool) IsContentOfType(PRUint32 aFlags);
  
  // Modern version of SplitText that returns an expected/result type
  Result<RefPtr<nsIDOMText>> SplitTextModern(PRUint32 aOffset);
  
  // Legacy compatibility method
  NS_IMETHOD SplitText(PRUint32 aOffset, nsIDOMText** aReturn) override;
};

// nsTextNode.cpp
RefPtr<nsTextNode> 
nsTextNode::Create()
{
  RefPtr<nsTextNode> textNode = new nsTextNode();
  return textNode;
}

// Legacy compatibility function
nsresult
NS_NewTextNode(nsIContent** aInstancePtrResult)
{
  RefPtr<nsTextNode> node = nsTextNode::Create();
  if (!node) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  node.forget(aInstancePtrResult);
  return NS_OK;
}

nsTextNode::nsTextNode() = default;
nsTextNode::~nsTextNode() = default;

NS_IMPL_ADDREF_INHERITED(nsTextNode, nsGenericDOMDataNode)
NS_IMPL_RELEASE_INHERITED(nsTextNode, nsGenericDOMDataNode)

Result<RefPtr<nsIDOMText>>
nsTextNode::SplitTextModern(PRUint32 aOffset)
{
  // Use structured bindings and auto for cleaner variable declarations
  auto [rv, length] = GetLength();
  if (NS_FAILED(rv)) {
    return std::unexpected(rv);
  }

  if (aOffset > length) {
    return std::unexpected(NS_ERROR_DOM_INDEX_SIZE_ERR);
  }

  // Use std::string_view or similar for string operations
  nsAutoString cutText;
  rv = SubstringData(aOffset, length - aOffset, cutText);
  if (NS_FAILED(rv)) {
    return std::unexpected(rv);
  }

  rv = DeleteData(aOffset, length - aOffset);
  if (NS_FAILED(rv)) {
    return std::unexpected(rv);
  }

  // Use RefPtr instead of nsCOMPtr where appropriate
  RefPtr<nsIContent> parent = GetParent();
  if (!parent) {
    return std::unexpected(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
  }

  // Use factory method instead of global creation function
  RefPtr<nsTextNode> newNode = nsTextNode::Create();
  if (!newNode) {
    return std::unexpected(NS_ERROR_OUT_OF_MEMORY);
  }

  // Use QueryInterface helper that returns RefPtr
  RefPtr<nsITextContent> tc = newNode.as<nsITextContent>();
  rv = tc->SetText(cutText, false);
  if (NS_FAILED(rv)) {
    return std::unexpected(rv);
  }

  // Use std::optional for values that might not exist
  std::optional<PRInt32> offsetInParent;
  rv = parent->IndexOf(this, offsetInParent);
  if (!offsetInParent.has_value() || offsetInParent.value() < 0) {
    return std::unexpected(NS_ERROR_FAILURE);
  }

  rv = parent->InsertChildAt(newNode, offsetInParent.value() + 1, true, true);
  if (NS_FAILED(rv)) {
    return std::unexpected(rv);
  }

  // Return the RefPtr directly
  return RefPtr<nsIDOMText>(newNode.as<nsIDOMText>());
}

// Legacy compatibility method implementation
NS_IMETHODIMP
nsTextNode::SplitText(PRUint32 aOffset, nsIDOMText** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nullptr;
  
  auto result = SplitTextModern(aOffset);
  if (!result) {
    return result.error();
  }
  
  RefPtr<nsIDOMText> newText = result.value();
  newText.forget(aReturn);
  return NS_OK;
}
```

## Key Modernization Techniques

1. **Factory Methods**: Replace global creation functions with static factory methods.

2. **Smart Pointers**: Use `RefPtr` instead of raw pointers and manual reference counting.

3. **Result Types**: Use `std::expected` or a similar pattern for returning values that might fail, rather than out parameters.

4. **Modern C++ Features**:
   - Default constructors/destructors
   - `std::optional` for values that might not exist
   - Structured bindings for cleaner return value handling
   - `nullptr` instead of `NULL` or `nsnull`

5. **Dual API Approach**: Provide both modern and legacy APIs for backward compatibility.

## Benefits of Modernization

1. **Improved Safety**: Smart pointers prevent memory leaks and dangling pointers.

2. **Better Error Handling**: Result types make error conditions explicit and prevent error code propagation mistakes.

3. **Cleaner Code**: Modern C++ features result in more readable and maintainable code.

4. **Compatibility**: The dual API approach allows gradual migration without breaking existing code.

## Implementation Strategy

To modernize DOM components:

1. **Identify Core Classes**: Focus on foundational classes like nodes, elements, and documents.

2. **Add Modern APIs**: Implement new APIs alongside existing ones for backward compatibility.

3. **Update Internal Usage**: Gradually migrate internal code to use the new APIs.

4. **Maintain Tests**: Ensure all functionality works correctly throughout the modernization process.

5. **Document Changes**: Provide clear documentation about new patterns and migration paths. 