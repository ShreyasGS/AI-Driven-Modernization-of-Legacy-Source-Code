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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    David Hyatt <hyatt@netscape.com> (Original Author)
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

#ifndef nsXBLContentSink_h__
#define nsXBLContentSink_h__

#include "nsXMLContentSink.h"
#include "nsXBLDocumentInfo.h"
#include "nsIXBLPrototypeBinding.h"
#include "nsIXBLPrototypeHandler.h"
#include "nsXBLProtoImpl.h"
#include "nsICSSParser.h"
#include "nsLayoutCID.h"

typedef enum {
  eXBL_InDocument,
  eXBL_InBinding,
  eXBL_InResources,
  eXBL_InImplementation,
  eXBL_InHandlers
} XBLPrimaryState;

typedef enum {
  eXBL_None,
  eXBL_InHandler,
  eXBL_InMethod,
  eXBL_InProperty,
  eXBL_InField,
  eXBL_InBody,
  eXBL_InGetter,
  eXBL_InSetter,
  eXBL_InConstructor,
  eXBL_InDestructor
} XBLSecondaryState;

class nsXULPrototypeElement;
class nsXBLProtoImplMember;
class nsXBLProtoImplProperty;
class nsXBLProtoImplMethod;
class nsXBLProtoImplField;

// The XBL content sink overrides the XML content sink to
// builds its own lightweight data structures for the <resources>,
// <handlers>, <implementation>, and 

class nsXBLContentSink : public nsXMLContentSink {
public:
  nsXBLContentSink();
  ~nsXBLContentSink();

  nsresult Init(nsIDocument* aDoc,
                nsIURI* aURL,
                nsIWebShell* aContainer);

  // nsIContentSink overrides
  NS_IMETHOD HandleStartElement(const PRUnichar *aName, 
                                const PRUnichar **aAtts, 
                                PRUint32 aAttsCount, 
                                PRUint32 aIndex, 
                                PRUint32 aLineNumber);

  NS_IMETHOD HandleEndElement(const PRUnichar *aName);
  
  NS_IMETHOD HandleCDataSection(const PRUnichar *aData, 
                                PRUint32 aLength);

protected:
    // nsXMLContentSink overrides
    PRBool OnOpenContainer(const PRUnichar **aAtts, 
                           PRUint32 aAttsCount, 
                           PRInt32 aNameSpaceID, 
                           nsIAtom* aTagName);
    
    nsresult CreateElement(const PRUnichar** aAtts, 
                           PRUint32 aAttsCount, 
                           PRInt32 aNameSpaceID, 
                           nsINodeInfo* aNodeInfo, 
                           nsIContent** aResult);
    
    nsresult AddAttributes(const PRUnichar** aAtts, 
                           nsIContent* aContent,
                           PRBool aIsHTML);
    
    nsresult AddAttributesToXULPrototype(const PRUnichar **aAtts, 
                                         PRUint32 aAttsCount, 
                                         nsXULPrototypeElement* aElement);
      
    // Our own helpers for constructing XBL prototype objects.
    void ConstructBinding();
    void ConstructHandler(const PRUnichar **aAtts);
    void ConstructResource(const PRUnichar **aAtts, nsIAtom* aResourceType);
    void ConstructImplementation(const PRUnichar **aAtts);
    void ConstructProperty(const PRUnichar **aAtts);
    void ConstructMethod(const PRUnichar **aAtts);
    void ConstructParameter(const PRUnichar **aAtts);
    void ConstructField(const PRUnichar **aAtts);
  

  // nsXMLContentSink overrides
  nsresult FlushText(PRBool aCreateTextNode=PR_TRUE,
                     PRBool* aDidFlush=nsnull);
protected:
  XBLPrimaryState mState;
  XBLSecondaryState mSecondaryState;
  nsIXBLDocumentInfo* mDocInfo;
  PRBool mIsChromeOrResource; // For bug #45989

  nsCOMPtr<nsICSSParser> mCSSParser;            // [OWNER]

  nsCOMPtr<nsIXBLPrototypeBinding> mBinding;
  nsCOMPtr<nsIXBLPrototypeHandler> mHandler;
  nsXBLProtoImpl* mImplementation;
  nsXBLProtoImplMember* mImplMember;
  nsXBLProtoImplProperty* mProperty;
  nsXBLProtoImplMethod* mMethod;
  nsXBLProtoImplField* mField;
};


nsresult
NS_NewXBLContentSink(nsIXMLContentSink** aResult,
                     nsIDocument* aDoc,
                     nsIURI* aURL,
                     nsIWebShell* aWebShell);
#endif // nsXBLContentSink_h__
