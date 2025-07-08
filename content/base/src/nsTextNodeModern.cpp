#include "nsTextNodeModern.h"
#include "nsIAtom.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsError.h"
#include "nsContentUtils.h"
#include <algorithm>

namespace mozilla {

//----------------------------------------------------------------------

already_AddRefed<TextNode>
TextNode::Create()
{
  RefPtr<TextNode> textNode = new TextNode();
  return textNode.forget();
}

TextNode::TextNode() = default;
TextNode::~TextNode() = default;

NS_IMPL_ADDREF_INHERITED(TextNode, nsGenericDOMDataNode)
NS_IMPL_RELEASE_INHERITED(TextNode, nsGenericDOMDataNode)

// QueryInterface implementation for TextNode
NS_INTERFACE_MAP_BEGIN(TextNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMText)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCharacterData)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsITextContent)
NS_INTERFACE_MAP_END_INHERITING(nsGenericDOMDataNode)

NS_IMETHODIMP
TextNode::GetTag(nsIAtom*& aResult) const
{
  aResult = nsnull;
  return NS_OK;
}

NS_IMETHODIMP_(PRBool)
TextNode::IsContentOfType(PRUint32 aFlags)
{
  return !(aFlags & ~(nsIContent::eDATA_NODE | nsIContent::eTEXT));
}

#ifdef DEBUG
NS_IMETHODIMP
TextNode::List(FILE* out, PRInt32 aIndent) const
{
  return nsGenericDOMDataNode::List(out, aIndent);
}

NS_IMETHODIMP
TextNode::DumpContent(FILE* out, PRInt32 aIndent, PRBool aDumpAll) const
{
  return nsGenericDOMDataNode::DumpContent(out, aIndent, aDumpAll);
}
#endif

NS_IMETHODIMP
TextNode::CloneContent(PRBool aCloneText, nsITextContent** aClone)
{
  *aClone = nsnull;

  RefPtr<TextNode> newNode = TextNode::Create();
  if (!newNode) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (aCloneText) {
    // Get our text
    nsAutoString text;
    nsresult rv = GetText(text);
    if (NS_FAILED(rv)) {
      return rv;
    }

    // Set the text
    rv = newNode->SetText(text, PR_FALSE);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  newNode.forget(aClone);
  return NS_OK;
}

TextNode::SplitResult
TextNode::SplitTextModern(PRUint32 aOffset)
{
  SplitResult result{nullptr, NS_OK};
  
  // Get the current length
  PRUint32 length = 0;
  result.rv = GetLength(&length);
  if (NS_FAILED(result.rv)) {
    return result;
  }

  // Check if the offset is valid
  if (aOffset > length) {
    result.rv = NS_ERROR_DOM_INDEX_SIZE_ERR;
    return result;
  }

  // Extract the text to cut
  nsAutoString cutText;
  result.rv = SubstringData(aOffset, length - aOffset, cutText);
  if (NS_FAILED(result.rv)) {
    return result;
  }

  // Delete the text from this node
  result.rv = DeleteData(aOffset, length - aOffset);
  if (NS_FAILED(result.rv)) {
    return result;
  }

  // Get the parent
  RefPtr<nsIContent> parent = GetParent();
  if (!parent) {
    result.rv = NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
    return result;
  }

  // Create a new text node
  RefPtr<TextNode> newNode = TextNode::Create();
  if (!newNode) {
    result.rv = NS_ERROR_OUT_OF_MEMORY;
    return result;
  }

  // Set the text in the new node
  RefPtr<nsITextContent> tc = do_QueryObject(newNode);
  result.rv = tc->SetText(cutText, PR_FALSE);
  if (NS_FAILED(result.rv)) {
    return result;
  }

  // Find our position in the parent
  PRInt32 offsetInParent = -1;
  parent->IndexOf(this, offsetInParent);
  if (offsetInParent < 0) {
    result.rv = NS_ERROR_FAILURE;
    return result;
  }

  // Insert the new node after this one
  result.rv = parent->InsertChildAt(newNode, offsetInParent + 1, PR_TRUE, PR_TRUE);
  if (NS_FAILED(result.rv)) {
    return result;
  }

  // Return the new node
  result.newNode = do_QueryObject(newNode);
  return result;
}

NS_IMETHODIMP
TextNode::SplitText(PRUint32 aOffset, nsIDOMText** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nullptr;
  
  // Use the modern implementation
  SplitResult result = SplitTextModern(aOffset);
  if (!result) {
    return result.rv;
  }
  
  // Transfer ownership to the out parameter
  result.newNode.forget(aReturn);
  return NS_OK;
}

} // namespace mozilla

//----------------------------------------------------------------------
// Legacy compatibility function

nsresult
NS_NewTextNodeModern(nsIContent** aInstancePtrResult)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);
  *aInstancePtrResult = nullptr;
  
  RefPtr<mozilla::TextNode> node = mozilla::TextNode::Create();
  if (!node) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  node.forget(aInstancePtrResult);
  return NS_OK;
} 