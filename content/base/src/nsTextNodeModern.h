#ifndef nsTextNodeModern_h__
#define nsTextNodeModern_h__

#include "nsGenericDOMDataNode.h"
#include "nsIDOMText.h"
#include "nsITextContent.h"
#include <memory>
#include <optional>
#include <string>

namespace mozilla {

/**
 * Modern implementation of a text node using C++11/14/17 features
 * This demonstrates modernization patterns that can be applied to the Mozilla codebase
 */
class TextNode : public nsGenericDOMDataNode,
                 public nsIDOMText
{
public:
  /**
   * Factory method for creating a new TextNode
   * @return RefPtr to the newly created TextNode
   */
  static already_AddRefed<TextNode> Create();
  
  TextNode();
  virtual ~TextNode();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_IMPL_NSIDOMNODE_USING_GENERIC_DOM_DATA

  // nsIDOMCharacterData
  NS_FORWARD_NSIDOMCHARACTERDATA(nsGenericDOMDataNode::)

  // nsIDOMText
  NS_IMETHOD SplitText(PRUint32 aOffset, nsIDOMText** aReturn) override;

  // nsIContent
  NS_IMETHOD GetTag(nsIAtom*& aResult) const override;
  NS_IMETHOD_(PRBool) IsContentOfType(PRUint32 aFlags) override;
  
#ifdef DEBUG
  NS_IMETHOD List(FILE* out, PRInt32 aIndent) const override;
  NS_IMETHOD DumpContent(FILE* out, PRInt32 aIndent, PRBool aDumpAll) const override;
#endif

  // nsITextContent
  NS_IMETHOD CloneContent(PRBool aCloneText, nsITextContent** aClone) override;
  
  /**
   * Modern version of SplitText that returns a Result type
   * @param aOffset Offset at which to split the text node
   * @return Result containing the new text node or an error code
   */
  struct SplitResult {
    RefPtr<nsIDOMText> newNode;
    nsresult rv;
    
    explicit operator bool() const { return NS_SUCCEEDED(rv); }
  };
  
  SplitResult SplitTextModern(PRUint32 aOffset);

private:
  // Prevent copying
  TextNode(const TextNode&) = delete;
  TextNode& operator=(const TextNode&) = delete;
};

} // namespace mozilla

// Legacy compatibility function
nsresult NS_NewTextNodeModern(nsIContent** aInstancePtrResult);

#endif // nsTextNodeModern_h__ 