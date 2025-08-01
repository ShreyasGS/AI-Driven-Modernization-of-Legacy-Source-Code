/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */


/**
 * MODULE NOTES:
 * @update  gess 4/1/98
 * 
 * This class is defines the basic interface between the
 * parser and the content sink. The parser will iterate
 * over the collection of tokens that it sees from the
 * tokenizer, coverting each related "group" into one of
 * these. This object gets passed to the sink, and is
 * then immediately reused.
 *
 * If you want to hang onto one of these, you should
 * make your own copy.
 *
 */

#ifndef NS_PARSERNODE__
#define NS_PARSERNODE__

#include "nsIParserNode.h"
#include "nsToken.h"
#include "nsString.h"
#include "nsParserCIID.h"
#include "nsDeque.h"
#include "nsDTDUtils.h"

class nsTokenAllocator;

class nsCParserNode :  public nsIParserNode {

  protected:

    PRInt32 mRefCnt;

  public:

    void AddRef()
    {
      ++mRefCnt;
    }

    void Release(nsFixedSizeAllocator& aPool)
    {
      if (--mRefCnt == 0)
        Destroy(this, aPool);
    }

#ifndef HEAP_ALLOCATED_NODES
  private:

    /**
     * Hide operator new; clients should use Create() instead.
     */
    static void* operator new(size_t) { return 0; }

    /**
     * Hide operator delete; clients should use Destroy() instead.
     */
    static void operator delete(void*,size_t) {}

#endif

  public:
    static nsCParserNode* Create(CToken* aToken,
                                 nsTokenAllocator* aTokenAllocator,
                                 nsNodeAllocator* aNodeAllocator)
    {
#ifdef HEAP_ALLOCATED_NODES
      return new
#else
      nsFixedSizeAllocator& pool = aNodeAllocator->GetArenaPool();
      void* place = pool.Alloc(sizeof(nsCParserNode));
      return ::new (place)
#endif
        nsCParserNode(aToken, aTokenAllocator, aNodeAllocator);
    }

    static void Destroy(nsCParserNode* aNode, nsFixedSizeAllocator& aPool)
    {
#ifdef HEAP_ALLOCATED_NODES
      delete aNode;
#else
      aNode->~nsCParserNode();
      aPool.Free(aNode, sizeof(*aNode));
#endif
    }

    /**
     * Default constructor
     */
    nsCParserNode();

    /**
     * Constructor
     * @update	gess5/11/98
     * @param   aToken is the token this node "refers" to
     */
    nsCParserNode(CToken* aToken,
                  nsTokenAllocator* aTokenAllocator,
                  nsNodeAllocator* aNodeAllocator=0);

    /**
     * Destructor
     * @update	gess5/11/98
     */
    virtual ~nsCParserNode();

    /**
     * Init
     * @update	gess5/11/98
     */
    virtual nsresult Init(CToken* aToken,
                          nsTokenAllocator* aTokenAllocator,
                          nsNodeAllocator* aNodeAllocator=0);

    /**
     * Retrieve the name of the node
     * @update	gess5/11/98
     * @return  string containing node name
     */
    virtual const nsString& GetName() const;

    /**
     * Retrieve the text from the given node
     * @update	gess5/11/98
     * @return  string containing node text
     */
    virtual const nsAString& GetText() const;

    /**
     * Retrieve the type of the parser node.
     * @update	gess5/11/98
     * @return  node type.
     */
    virtual PRInt32 GetNodeType()  const;

    /**
     * Retrieve token type of parser node
     * @update	gess5/11/98
     * @return  token type
     */
    virtual PRInt32 GetTokenType()  const;


    //***************************************
    //methods for accessing key/value pairs
    //***************************************

    /**
     * Retrieve the number of attributes in this node.
     * @update	gess5/11/98
     * @return  count of attributes (may be 0)
     */
    virtual PRInt32 GetAttributeCount(PRBool askToken=PR_FALSE) const;

    /**
     * Retrieve the key (of key/value pair) at given index
     * @update	gess5/11/98
     * @param   anIndex is the index of the key you want
     * @return  string containing key.
     */
    virtual const nsAString& GetKeyAt(PRUint32 anIndex) const;

    /**
     * Retrieve the value (of key/value pair) at given index
     * @update	gess5/11/98
     * @param   anIndex is the index of the value you want
     * @return  string containing value.
     */
    virtual const nsAString& GetValueAt(PRUint32 anIndex) const;

    /**
     * NOTE: When the node is an entity, this will translate the entity
     *       to it's unicode value, and store it in aString.
     * @update	gess5/11/98
     * @param   aString will contain the resulting unicode string value
     * @return  int (unicode char or unicode index from table)
     */
    virtual PRInt32 TranslateToUnicodeStr(nsString& aString) const;

    /**
     * 
     * @update	gess5/11/98
     * @param 
     * @return
     */
    virtual void AddAttribute(CToken* aToken);

    /**
     * This getter retrieves the line number from the input source where
     * the token occured. Lines are interpreted as occuring between \n characters.
     * @update	gess7/24/98
     * @return  int containing the line number the token was found on
     */
    virtual PRInt32 GetSourceLineNumber(void) const;

    /** This method pop the attribute token from the given index
     * @update	harishd 03/25/99
     * @return  token at anIndex
     */
    virtual CToken* PopAttributeToken();

    /** Retrieve a string containing the tag and its attributes in "source" form
     * @update	rickg 06June2000
     * @return  void
     */
    virtual void GetSource(nsString& aString);

    /*
     * Get and set the ID attribute atom for this node.
     * See http://www.w3.org/TR/1998/REC-xml-19980210#sec-attribute-types
     * for the definition of an ID attribute.
     *
     */
    virtual nsresult GetIDAttributeAtom(nsIAtom** aResult) const;
    virtual nsresult SetIDAttributeAtom(nsIAtom* aID);

    /**
     * This pair of methods allows us to set a generic bit (for arbitrary use)
     * on each node stored in the context.
     * @update	gess 11May2000
     */
    virtual PRBool  GetGenericState(void) const {return mGenericState;}
    virtual void    SetGenericState(PRBool aState) {mGenericState=aState;}

    /** Release all the objects you're holding
     * @update	harishd 08/02/00
     * @return  void
     */
    virtual nsresult ReleaseAll();

    CToken*   mToken;
    nsDeque*  mAttributes;
    PRInt32   mUseCount;
    PRBool    mGenericState;
    nsCOMPtr<nsIAtom> mIDAttributeAtom;
    
   nsTokenAllocator* mTokenAllocator;
#ifdef HEAP_ALLOCATED_NODES
   nsNodeAllocator*  mNodeAllocator; // weak 
#endif
};

#endif


