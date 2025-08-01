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
 * Original Author: Daniel Glazman <glazman@netscape.com>
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

#include "nsHTMLEditor.h"
#include "nsCOMPtr.h"
#include "nsHTMLEditUtils.h"
#include "nsIPref.h"
#include "nsIServiceManager.h"
#include "nsIEditProperty.h"
#include "ChangeCSSInlineStyleTxn.h"
#include "nsIDOMElement.h"
#include "TransactionFactory.h"
#include "nsIDOMElementCSSInlineStyle.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentView.h"
#include "nsIContent.h"
#include "nsTextEditUtils.h"
#include "nsUnicharUtils.h"
#include "nsHTMLCSSUtils.h"

static NS_DEFINE_CID(kPrefServiceCID,       NS_PREF_CID);

static
void ProcessBValue(const nsAString * aInputString, nsAString & aOutputString,
                   const char * aDefaultValueString,
                   const char * aPrependString, const char* aAppendString)
{
  if (aInputString) {
    if (aInputString->Equals(NS_LITERAL_STRING("-moz-editor-invert-value"))) {
      aOutputString.Assign(NS_LITERAL_STRING("normal"));
    }
    else {
      aOutputString.Assign(NS_LITERAL_STRING("bold"));
    }
  }
  else {
    aOutputString.Assign(NS_LITERAL_STRING("bold"));
  }
}

static
void ProcessDefaultValue(const nsAString * aInputString, nsAString & aOutputString,
                         const char * aDefaultValueString,
                         const char * aPrependString, const char* aAppendString)
{
  aOutputString.Truncate();
  if (aDefaultValueString) {
    aOutputString.Assign(NS_ConvertASCIItoUCS2(aDefaultValueString));
  }
}

static
void ProcessSameValue(const nsAString * aInputString, nsAString & aOutputString,
                      const char * aDefaultValueString,
                      const char * aPrependString, const char* aAppendString)
{
  aOutputString.Truncate();
  if (aInputString) {
    aOutputString.Assign(*aInputString);
  }
}

static
void ProcessExtendedValue(const nsAString * aInputString, nsAString & aOutputString,
                          const char * aDefaultValueString,
                          const char * aPrependString, const char* aAppendString)
{
  aOutputString.Truncate();
  if (aInputString) {
    if (aPrependString) {
      aOutputString.Append(NS_ConvertASCIItoUCS2(aPrependString));
    }
    aOutputString.Append(*aInputString);
    if (aAppendString) {
      aOutputString.Append(NS_ConvertASCIItoUCS2(aAppendString));
    }
  }
}

static
void ProcessLengthValue(const nsAString * aInputString, nsAString & aOutputString,
                        const char * aDefaultValueString,
                        const char * aPrependString, const char* aAppendString)
{
  aOutputString.Truncate();
  if (aInputString) {
    aOutputString.Append(*aInputString);
    if (-1 == aOutputString.FindChar(PRUnichar('%'))) {
      aOutputString.Append(NS_LITERAL_STRING("px"));
    }
  }
}

static
void ProcessListStyleTypeValue(const nsAString * aInputString, nsAString & aOutputString,
                               const char * aDefaultValueString,
                               const char * aPrependString, const char* aAppendString)
{
  aOutputString.Truncate();
  if (aInputString) {
    if (aInputString->Equals(NS_LITERAL_STRING("1"))) {
      aOutputString.Append(NS_LITERAL_STRING("decimal"));
    }
    else if (aInputString->Equals(NS_LITERAL_STRING("a"))) {
      aOutputString.Append(NS_LITERAL_STRING("lower-alpha"));
    }
    else if (aInputString->Equals(NS_LITERAL_STRING("A"))) {
      aOutputString.Append(NS_LITERAL_STRING("upper-alpha"));
    }
    else if (aInputString->Equals(NS_LITERAL_STRING("i"))) {
      aOutputString.Append(NS_LITERAL_STRING("lower-roman"));
    }
    else if (aInputString->Equals(NS_LITERAL_STRING("I"))) {
      aOutputString.Append(NS_LITERAL_STRING("upper-roman"));
    }
    else if (aInputString->Equals(NS_LITERAL_STRING("square"))
             || aInputString->Equals(NS_LITERAL_STRING("circle"))
             || aInputString->Equals(NS_LITERAL_STRING("disc"))) {
      aOutputString.Append(*aInputString);
    }
  }
}

static
void ProcessMarginLeftValue(const nsAString * aInputString, nsAString & aOutputString,
                            const char * aDefaultValueString,
                            const char * aPrependString, const char* aAppendString)
{
  aOutputString.Truncate();
  if (aInputString) {
    if (aInputString->Equals(NS_LITERAL_STRING("center")) ||
        aInputString->Equals(NS_LITERAL_STRING("-moz-center"))) {
      aOutputString.Append(NS_LITERAL_STRING("auto")); 
    }
    else if (aInputString->Equals(NS_LITERAL_STRING("right")) ||
             aInputString->Equals(NS_LITERAL_STRING("-moz-right"))) {
      aOutputString.Append(NS_LITERAL_STRING("auto")); 
    }
    else {
      aOutputString.Append(NS_LITERAL_STRING("0px")); 
    }
  }
}

static
void ProcessMarginRightValue(const nsAString * aInputString, nsAString & aOutputString,
                             const char * aDefaultValueString,
                             const char * aPrependString, const char* aAppendString)
{
  aOutputString.Truncate();
  if (aInputString) {
    if (aInputString->Equals(NS_LITERAL_STRING("center")) ||
        aInputString->Equals(NS_LITERAL_STRING("-moz-center"))) {
      aOutputString.Append(NS_LITERAL_STRING("auto")); 
    }
    else if (aInputString->Equals(NS_LITERAL_STRING("left")) ||
             aInputString->Equals(NS_LITERAL_STRING("-moz-left"))) {
      aOutputString.Append(NS_LITERAL_STRING("auto")); 
    }
    else {
      aOutputString.Append(NS_LITERAL_STRING("0px")); 
    }
  }
}

const nsHTMLCSSUtils::CSSEquivTable boldEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_font_weight, ProcessBValue, nsnull, nsnull, nsnull, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable italicEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_font_style, ProcessDefaultValue, "italic", nsnull, nsnull, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable underlineEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_text_decoration, ProcessDefaultValue, "underline", nsnull, nsnull, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable strikeEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_text_decoration, ProcessDefaultValue, "line-through", nsnull, nsnull, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable ttEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_font_family, ProcessDefaultValue, "monospace", nsnull, nsnull, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable fontColorEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_color, ProcessSameValue, nsnull, nsnull, nsnull, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable fontFaceEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_font_family, ProcessSameValue, nsnull, nsnull, nsnull, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable bgcolorEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_background_color, ProcessSameValue, nsnull, nsnull, nsnull, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable backgroundImageEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_background_image, ProcessExtendedValue, nsnull, "url(", ")", PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable textColorEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_color, ProcessSameValue, nsnull, nsnull, nsnull, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable borderEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_border, ProcessExtendedValue, nsnull, nsnull, "px solid", PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable textAlignEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_text_align, ProcessSameValue, nsnull, nsnull, nsnull, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable captionAlignEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_caption_side, ProcessSameValue, nsnull, nsnull, nsnull, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable verticalAlignEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_vertical_align, ProcessSameValue, nsnull, nsnull, nsnull, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable nowrapEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_whitespace, ProcessDefaultValue, "nowrap", nsnull, nsnull, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable widthEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_width, ProcessLengthValue, nsnull, nsnull, nsnull, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable heightEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_height, ProcessLengthValue, nsnull, nsnull, nsnull, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable listStyleTypeEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_list_style_type, ProcessListStyleTypeValue, nsnull, nsnull, nsnull, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable tableAlignEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_text_align, ProcessDefaultValue, "left", nsnull, nsnull, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_margin_left, ProcessMarginLeftValue, nsnull, nsnull, nsnull, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_margin_right, ProcessMarginRightValue, nsnull, nsnull, nsnull, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable hrAlignEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_margin_left, ProcessMarginLeftValue, nsnull, nsnull, nsnull, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_margin_right, ProcessMarginRightValue, nsnull, nsnull, nsnull, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

nsHTMLCSSUtils::nsHTMLCSSUtils()
: mIsCSSPrefChecked(PR_FALSE)
{
}

nsresult
nsHTMLCSSUtils::Init(nsHTMLEditor *aEditor)
{
  nsresult result = NS_OK;
  mHTMLEditor = NS_STATIC_CAST(nsHTMLEditor*, aEditor);

  // let's retrieve the value of the "CSS editing" pref
  nsCOMPtr<nsIPref> prefService = do_GetService(kPrefServiceCID, &result);
  if (NS_SUCCEEDED(result) && prefService) {
    result = prefService->GetBoolPref("editor.use_css", &mIsCSSPrefChecked);
    if (NS_FAILED(result)) return result;
  }
  return result;
}

// Answers true if we have some CSS equivalence for the HTML style defined
// by aProperty and/or aAttribute for the node aNode
PRBool
nsHTMLCSSUtils::IsCSSEditableProperty(nsIDOMNode * aNode,
                                      nsIAtom * aProperty,
                                      const nsAString * aAttribute)
{
  nsCOMPtr<nsIDOMNode> node = aNode;
  // we need an element node here
  if (mHTMLEditor->IsTextNode(aNode)) {
    aNode->GetParentNode(getter_AddRefs(node));
  }
  nsCOMPtr<nsIContent> content = do_QueryInterface(node);
  nsCOMPtr<nsIAtom> tagName;
  content->GetTag(*getter_AddRefs(tagName));

  // html inline styles B I TT U STRIKE and COLOR/FACE on FONT
  if (nsIEditProperty::b == aProperty
      || nsIEditProperty::i == aProperty
      || nsIEditProperty::tt == aProperty
      || nsIEditProperty::u == aProperty
      || nsIEditProperty::strike == aProperty
      || ((nsIEditProperty::font == aProperty) && aAttribute &&
           (aAttribute->Equals(NS_LITERAL_STRING("color")) ||
            aAttribute->Equals(NS_LITERAL_STRING("face"))))) {
    return PR_TRUE;
  }

  // ALIGN attribute on elements supporting it
  if (aAttribute && (aAttribute->Equals(NS_LITERAL_STRING("align"))) &&
      (nsIEditProperty::div == tagName
       || nsIEditProperty::p   == tagName
       || nsIEditProperty::h1  == tagName
       || nsIEditProperty::h2  == tagName
       || nsIEditProperty::h3  == tagName
       || nsIEditProperty::h4  == tagName
       || nsIEditProperty::h5  == tagName
       || nsIEditProperty::h6  == tagName
       || nsIEditProperty::td  == tagName
       || nsIEditProperty::th  == tagName
       || nsIEditProperty::table  == tagName
       || nsIEditProperty::hr  == tagName
       // Let's add the following elements here even if ALIGN has not
       // the same meaning for them
       || nsIEditProperty::legend  == tagName
       || nsIEditProperty::caption == tagName)) {
    return PR_TRUE;
  }

  // attributes TEXT, BACKGROUND and BGCOLOR on BODY
  if (aAttribute && (nsIEditProperty::body == tagName) &&
      (aAttribute->Equals(NS_LITERAL_STRING("text"))
       || aAttribute->Equals(NS_LITERAL_STRING("background"))
       || aAttribute->Equals(NS_LITERAL_STRING("bgcolor")))) {
    return PR_TRUE;
  }

  // attribute BGCOLOR on other elements
  if (aAttribute && aAttribute->Equals(NS_LITERAL_STRING("bgcolor"))) {
    return PR_TRUE;
  }

  // attributes HEIGHT, WIDTH and NOWRAP on TD and TH
  if (aAttribute && ((nsIEditProperty::td == tagName)
                      || (nsIEditProperty::th == tagName)) &&
      (aAttribute->Equals(NS_LITERAL_STRING("height"))
       || aAttribute->Equals(NS_LITERAL_STRING("width"))
       || aAttribute->Equals(NS_LITERAL_STRING("nowrap")))) {
    return PR_TRUE;
  }

  // attributes HEIGHT and WIDTH on TABLE
  if (aAttribute && (nsIEditProperty::table == tagName) &&
      (aAttribute->Equals(NS_LITERAL_STRING("height"))
       || aAttribute->Equals(NS_LITERAL_STRING("width")))) {
    return PR_TRUE;
  }

  // attributes SIZE and WIDTH on HR
  if (aAttribute && (nsIEditProperty::hr == tagName) &&
      (aAttribute->Equals(NS_LITERAL_STRING("size"))
       || aAttribute->Equals(NS_LITERAL_STRING("width")))) {
    return PR_TRUE;
  }

  // attribute TYPE on OL UL LI
  if (aAttribute && (nsIEditProperty::ol == tagName
                     || nsIEditProperty::ul == tagName
                     || nsIEditProperty::li == tagName) &&
      aAttribute->Equals(NS_LITERAL_STRING("type"))) {
    return PR_TRUE;
  }

  if (aAttribute && nsIEditProperty::img == tagName &&
      (aAttribute->Equals(NS_LITERAL_STRING("border"))
       || aAttribute->Equals(NS_LITERAL_STRING("width"))
       || aAttribute->Equals(NS_LITERAL_STRING("height")))) {
    return PR_TRUE;
  }

  // other elements that we can align using CSS even if they
  // can't carry the html ALIGN attribute
  if (aAttribute && aAttribute->Equals(NS_LITERAL_STRING("align")) &&
      (nsIEditProperty::ul == tagName
       || nsIEditProperty::ol == tagName
       || nsIEditProperty::dl == tagName
       || nsIEditProperty::li == tagName
       || nsIEditProperty::dd == tagName
       || nsIEditProperty::dt == tagName
       || nsIEditProperty::address == tagName
       || nsIEditProperty::pre == tagName
       || nsIEditProperty::ul == tagName)) {
    return PR_TRUE;
  }

  return PR_FALSE;
}

// the lowest level above the transaction; adds the css declaration "aProperty : aValue" to
// the inline styles carried by aElement
nsresult
nsHTMLCSSUtils::SetCSSProperty(nsIDOMElement *aElement, nsIAtom * aProperty, const nsAString & aValue)
{
  ChangeCSSInlineStyleTxn *txn;
  nsresult result = CreateCSSPropertyTxn(aElement, aProperty, aValue, &txn, PR_FALSE);
  if (NS_SUCCEEDED(result))  {
    result = mHTMLEditor->Do(txn);
  }
  // The transaction system (if any) has taken ownwership of txn
  NS_IF_RELEASE(txn);
  return result;
}

// the lowest level above the transaction; removes the value aValue from the list of values
// specified for the CSS property aProperty, or totally remove the declaration if this
// property accepts only one value
nsresult
nsHTMLCSSUtils::RemoveCSSProperty(nsIDOMElement *aElement, nsIAtom * aProperty, const nsAString & aValue)
{
  ChangeCSSInlineStyleTxn *txn;
  nsresult result = CreateCSSPropertyTxn(aElement, aProperty, aValue, &txn, PR_TRUE);
  if (NS_SUCCEEDED(result))  {
    result = mHTMLEditor->Do(txn);
  }
  // The transaction system (if any) has taken ownwership of txn
  NS_IF_RELEASE(txn);
  return result;
  return NS_OK;
}

nsresult 
nsHTMLCSSUtils::CreateCSSPropertyTxn(nsIDOMElement *aElement, 
                                     nsIAtom * aAttribute,
                                     const nsAString& aValue,
                                     ChangeCSSInlineStyleTxn ** aTxn,
                                     PRBool aRemoveProperty)
{
  nsresult result = NS_ERROR_NULL_POINTER;
  if (nsnull != aElement)
  {
    result = TransactionFactory::GetNewTransaction(ChangeCSSInlineStyleTxn::GetCID(), (EditTxn **)aTxn);
    if (NS_SUCCEEDED(result))  {
      result = (*aTxn)->Init(mHTMLEditor, aElement, aAttribute, aValue, aRemoveProperty);
    }
  }
  return result;
}

nsresult
nsHTMLCSSUtils::GetSpecifiedProperty(nsIDOMNode *aNode, nsIAtom *aProperty,
                                     nsAString & aValue)
{
  return GetCSSInlinePropertyBase(aNode, aProperty, aValue, nsnull, SPECIFIED_STYLE_TYPE);
}

nsresult
nsHTMLCSSUtils::GetComputedProperty(nsIDOMNode *aNode, nsIAtom *aProperty,
                                    nsAString & aValue)
{
  nsCOMPtr<nsIDOMViewCSS> viewCSS = nsnull;
  nsresult res = GetDefaultViewCSS(aNode, getter_AddRefs(viewCSS));
  if (NS_FAILED(res)) return res;

  return GetCSSInlinePropertyBase(aNode, aProperty, aValue, viewCSS, COMPUTED_STYLE_TYPE);
}

nsresult
nsHTMLCSSUtils::GetCSSInlinePropertyBase(nsIDOMNode *aNode, nsIAtom *aProperty,
                                        nsAString &aValue,
                                        nsIDOMViewCSS *aViewCSS,
                                        PRUint8 aStyleType)
{
  aValue.Truncate();
  NS_ENSURE_TRUE(aProperty, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMElement>element;
  nsresult res = GetElementContainerOrSelf(aNode, getter_AddRefs(element));
  if (NS_FAILED(res)) return res;

  switch (aStyleType) {
    case COMPUTED_STYLE_TYPE:
      if (element && aViewCSS) {
        nsAutoString empty, value, propString;
        nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
        aProperty->ToString(propString);
        // Get the all the computed css styles attached to the element node
        res = aViewCSS->GetComputedStyle(element, empty, getter_AddRefs(cssDecl));
        if (NS_FAILED(res)) return res;
        // from these declarations, get the one we want and that one only
        res = cssDecl->GetPropertyValue(propString, value);
        if (NS_FAILED(res)) return res;
        aValue.Assign(value);
      }
      break;
    case SPECIFIED_STYLE_TYPE:
      if (element) {
        nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
        PRUint32 length = 0;
        res = GetInlineStyles(element, getter_AddRefs(cssDecl), &length);
        if (NS_FAILED(res)) return res;
        nsAutoString value, propString;
        aProperty->ToString(propString);
        res = cssDecl->GetPropertyValue(propString, value);
        if (NS_FAILED(res)) return res;
        aValue.Assign(value);
      }
      break;
  }
  return NS_OK;
}

nsresult
nsHTMLCSSUtils::GetDefaultViewCSS(nsIDOMNode *aNode, nsIDOMViewCSS **aViewCSS)
{
  nsCOMPtr<nsIDOMElement>element;
  nsresult res = GetElementContainerOrSelf(aNode, getter_AddRefs(element));
  if (NS_FAILED(res)) return res;

  // if we have an element node
  if (element) {
    // find the owner document
    nsCOMPtr<nsIDOMDocument> doc;
    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(element);
    res = node->GetOwnerDocument(getter_AddRefs(doc));
    if (NS_FAILED(res)) return res;
    if (doc) {
      nsCOMPtr<nsIDOMDocumentView> documentView;
      nsCOMPtr<nsIDOMAbstractView> abstractView;
      // from the document, get the abtractView
      documentView = do_QueryInterface(doc);
      res = documentView->GetDefaultView(getter_AddRefs(abstractView));
      if (NS_FAILED(res)) return res;
      // from the abstractView, get the CSS view
      CallQueryInterface(abstractView, aViewCSS);
      return NS_OK;
    }
  }
  *aViewCSS = nsnull;
  return NS_OK;
}

nsresult
NS_NewHTMLCSSUtils(nsHTMLCSSUtils** aInstancePtrResult)
{
  nsHTMLCSSUtils * rules = new nsHTMLCSSUtils();
  if (rules) {
    *aInstancePtrResult = rules;
    return NS_OK;
  }
  else {
    *aInstancePtrResult = nsnull;
  }
  return NS_ERROR_OUT_OF_MEMORY;
}

// remove the CSS style "aProperty : aPropertyValue" and possibly remove the whole node
// if it is a span and if its only attribute is _moz_dirty
nsresult
nsHTMLCSSUtils::RemoveCSSInlineStyle(nsIDOMNode *aNode, nsIAtom *aProperty, const nsAString & aPropertyValue)
{
  nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(aNode);

  // remove the property from the style attribute
  nsresult res = RemoveCSSProperty(elem, aProperty, aPropertyValue);
  if (NS_FAILED(res)) return res;

  if (mHTMLEditor->NodeIsType(aNode, nsIEditProperty::span)) {
    PRInt32 attrCount;
    nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
    res = content->GetAttrCount(attrCount);
    if (NS_FAILED(res)) return res;
    if (0 == attrCount) {
      // no more attributes on this span, let's remove the element
      res = mHTMLEditor->RemoveContainer(aNode);
      if (NS_FAILED(res)) return res;
    }
    else if (1 == attrCount) {
      // incredible hack in case the only remaining attribute is a _moz_dirty...
      PRInt32 nameSpaceID;
      nsCOMPtr<nsIAtom> attrName, prefix;
      res = content->GetAttrNameAt(0, nameSpaceID, *getter_AddRefs(attrName),
                                   *getter_AddRefs(prefix));
      if (NS_FAILED(res)) return res;
      nsAutoString attrString, tmp;
      attrName->ToString(attrString);
      if (attrString.Equals(NS_LITERAL_STRING("_moz_dirty"))) {
        res = mHTMLEditor->RemoveContainer(aNode);
        if (NS_FAILED(res)) return res;
      }
    }
  }
  return NS_OK;
}

// Answers true is the property can be removed by setting a "none" CSS value
// on a node
PRBool
nsHTMLCSSUtils::IsCSSInvertable(nsIAtom *aProperty, const nsAString *aAttribute)
{
  return PRBool(nsIEditProperty::b == aProperty);
}

// Get the default browser background color if we need it for GetCSSBackgroundColorState
nsresult
nsHTMLCSSUtils::GetDefaultBackgroundColor(nsAString & aColor)
{
  nsresult result;
  nsCOMPtr<nsIPref> prefService = do_GetService(kPrefServiceCID, &result);
  if (NS_FAILED(result)) return result;
  aColor.Assign(NS_LITERAL_STRING("#ffffff"));
  nsXPIDLCString returnColor;
  if (prefService) {
    PRBool useCustomColors;
    result = prefService->GetBoolPref("editor.use_custom_colors", &useCustomColors);
    if (NS_FAILED(result)) return result;
    if (useCustomColors) {
      result = prefService->CopyCharPref("editor.background_color",
                                         getter_Copies(returnColor));
      if (NS_FAILED(result)) return result;
    }
    else {
      PRBool useSystemColors;
      result = prefService->GetBoolPref("browser.display.use_system_colors", &useSystemColors);
      if (NS_FAILED(result)) return result;
      if (!useSystemColors) {
        result = prefService->CopyCharPref("browser.display.background_color",
                                           getter_Copies(returnColor));
        if (NS_FAILED(result)) return result;
      }
    }
  }
  if (returnColor) {
    aColor.Assign(NS_ConvertASCIItoUCS2(returnColor));
  }
  return NS_OK;
}

// Get the default length unit used for CSS Indent/Outdent
nsresult
nsHTMLCSSUtils::GetDefaultLengthUnit(nsAString & aLengthUnit)
{
  nsresult result;
  nsCOMPtr<nsIPref> prefService = do_GetService(kPrefServiceCID, &result);
  if (NS_FAILED(result)) return result;
  aLengthUnit.Assign(NS_LITERAL_STRING("px"));
  if (NS_SUCCEEDED(result) && prefService) {
    nsXPIDLCString returnLengthUnit;
    result = prefService->CopyCharPref("editor.css.default_length_unit",
                                       getter_Copies(returnLengthUnit));
    if (NS_FAILED(result)) return result;
    if (returnLengthUnit) {
      aLengthUnit.Assign(NS_ConvertASCIItoUCS2(returnLengthUnit));
    }
  }
  return NS_OK;
}

// Unfortunately, CSSStyleDeclaration::GetPropertyCSSValue is not yet implemented...
// We need then a way to determine the number part and the unit from aString, aString
// being the result of a GetPropertyValue query...
void
nsHTMLCSSUtils::ParseLength(const nsAString & aString, float * aValue, nsIAtom ** aUnit)
{
  nsAString::const_iterator iter;
  aString.BeginReading(iter);

  float a = 10.0f , b = 1.0f, value = 0;
  PRInt8 sign = 1;
  PRInt32 i = 0, j = aString.Length();
  PRUnichar c;
  nsAutoString unit;
  PRBool floatingPointFound = PR_FALSE;
  c = *iter;
  if (PRUnichar('-') == c) { sign = -1; iter++; i++; }
  else if (PRUnichar('+') == c) { iter++; i++; }
  while (i < j) {
    c = *iter;
    if ((PRUnichar('0') == c) ||
        (PRUnichar('1') == c) ||
        (PRUnichar('2') == c) ||
        (PRUnichar('3') == c) ||
        (PRUnichar('4') == c) ||
        (PRUnichar('5') == c) ||
        (PRUnichar('6') == c) ||
        (PRUnichar('7') == c) ||
        (PRUnichar('8') == c) ||
        (PRUnichar('9') == c)) {
      value = (value * a) + (b * (c - PRUnichar('0')));
      b = b / 10 * a;
    }
    else if (!floatingPointFound && (PRUnichar('.') == c)) {
      floatingPointFound = PR_TRUE;
      a = 1.0f; b = 0.1f;
    }
    else break;
    iter++;
    i++;
  }
  unit = Substring(aString, aString.Length() - (j-i), j-i);
  *aValue = value * sign;
  *aUnit = NS_NewAtom(unit); 
}

void
nsHTMLCSSUtils::GetCSSPropertyAtom(nsCSSEditableProperty aProperty, nsIAtom ** aAtom)
{
  *aAtom = nsnull;
  if (0 < aProperty) {
    switch (aProperty) {
      case eCSSEditableProperty_background_color:
        *aAtom = nsIEditProperty::cssBackgroundColor;
        break;
      case eCSSEditableProperty_background_image:
        *aAtom = nsIEditProperty::cssBackgroundImage;
        break;
      case eCSSEditableProperty_border:
        *aAtom = nsIEditProperty::cssBorder;
        break;
      case eCSSEditableProperty_caption_side:
        *aAtom = nsIEditProperty::cssCaptionSide;
        break;
      case eCSSEditableProperty_color:
        *aAtom = nsIEditProperty::cssColor;
        break;
      case eCSSEditableProperty_float:
        *aAtom = nsIEditProperty::cssFloat;
        break;
      case eCSSEditableProperty_font_family:
        *aAtom = nsIEditProperty::cssFontFamily;
        break;
      case eCSSEditableProperty_font_size:
        *aAtom = nsIEditProperty::cssFontSize;
        break;
      case eCSSEditableProperty_font_style:
        *aAtom = nsIEditProperty::cssFontStyle;
        break;
      case eCSSEditableProperty_font_weight:
        *aAtom = nsIEditProperty::cssFontWeight;
        break;
      case eCSSEditableProperty_height:
        *aAtom = nsIEditProperty::cssHeight;
        break;
      case eCSSEditableProperty_list_style_type:
        *aAtom = nsIEditProperty::cssListStyleType;
        break;
      case eCSSEditableProperty_margin_left:
        *aAtom = nsIEditProperty::cssMarginLeft;
        break;
      case eCSSEditableProperty_margin_right:
        *aAtom = nsIEditProperty::cssMarginRight;
        break;
      case eCSSEditableProperty_text_align:
        *aAtom = nsIEditProperty::cssTextAlign;
        break;
      case eCSSEditableProperty_text_decoration:
        *aAtom = nsIEditProperty::cssTextDecoration;
        break;
      case eCSSEditableProperty_vertical_align:
        *aAtom = nsIEditProperty::cssVerticalAlign;
        break;
      case eCSSEditableProperty_whitespace:
        *aAtom = nsIEditProperty::cssWhitespace;
        break;
      case eCSSEditableProperty_width:
        *aAtom = nsIEditProperty::cssWidth;
        break;
    }
  }
}

// Populate aProperty and aValueArray with the CSS declarations equivalent to the
// value aValue according to the equivalence table aEquivTable
void
nsHTMLCSSUtils::BuildCSSDeclarations(nsVoidArray & aPropertyArray,
                                     nsStringArray & aValueArray,
                                     const CSSEquivTable * aEquivTable,
                                     const nsAString * aValue,
                                     PRBool aGetOrRemoveRequest)
{
  // clear arrays
  aPropertyArray.Clear();
  aValueArray.Clear();

  // if we have an input value, let's use it
  nsAutoString value;
  if (aValue) {
    value.Assign(*aValue);
    ToLowerCase(value);
  }

  PRInt8 index = 0;
  nsCSSEditableProperty cssProperty = aEquivTable[index].cssProperty;
  while (cssProperty) {
    if (!aGetOrRemoveRequest|| aEquivTable[index].gettable) {
      nsAutoString cssValue, cssPropertyString;
      nsIAtom * cssPropertyAtom;
      // find the equivalent css value for the index-th property in
      // the equivalence table
      (*aEquivTable[index].processValueFunctor) (&value, cssValue,
                                                 aEquivTable[index].defaultValue,
                                                 aEquivTable[index].prependValue,
                                                 aEquivTable[index].appendValue);
      GetCSSPropertyAtom(cssProperty, &cssPropertyAtom);
      aPropertyArray.AppendElement(cssPropertyAtom);
      aValueArray.AppendString(cssValue);
    }
    index++;
    cssProperty = aEquivTable[index].cssProperty;
  }
}

// Populate cssPropertyArray and cssValueArray with the declarations equivalent
// to aHTMLProperty/aAttribute/aValue for the node aNode
void
nsHTMLCSSUtils::GenerateCSSDeclarationsFromHTMLStyle(nsIDOMNode * aNode,
                                                     nsIAtom *aHTMLProperty,
                                                     const nsAString * aAttribute,
                                                     const nsAString * aValue,
                                                     nsVoidArray & cssPropertyArray,
                                                     nsStringArray & cssValueArray,
                                                     PRBool aGetOrRemoveRequest)
{
  nsCOMPtr<nsIDOMNode> node = aNode;
  if (mHTMLEditor->IsTextNode(aNode)) {
    aNode->GetParentNode(getter_AddRefs(node));
  }
  if (!node) return;

  nsCOMPtr<nsIContent> content = do_QueryInterface(node);
  nsCOMPtr<nsIAtom> tagName;
  content->GetTag(*getter_AddRefs(tagName));

  if (nsIEditProperty::b == aHTMLProperty) {
    BuildCSSDeclarations(cssPropertyArray, cssValueArray, boldEquivTable, aValue, aGetOrRemoveRequest);
  }
  else if (nsIEditProperty::i == aHTMLProperty) {
    BuildCSSDeclarations(cssPropertyArray, cssValueArray, italicEquivTable, aValue, aGetOrRemoveRequest);
  }
  else if (nsIEditProperty::u == aHTMLProperty) {
    BuildCSSDeclarations(cssPropertyArray, cssValueArray, underlineEquivTable, aValue, aGetOrRemoveRequest);
  }
  else if (nsIEditProperty::strike == aHTMLProperty) {
    BuildCSSDeclarations(cssPropertyArray, cssValueArray, strikeEquivTable, aValue, aGetOrRemoveRequest);
  }
  else if (nsIEditProperty::tt == aHTMLProperty) {
    BuildCSSDeclarations(cssPropertyArray, cssValueArray, ttEquivTable, aValue, aGetOrRemoveRequest);
  }
  else if (aAttribute) {
    if (nsIEditProperty::font == aHTMLProperty &&
        aAttribute->Equals(NS_LITERAL_STRING("color"))) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, fontColorEquivTable, aValue, aGetOrRemoveRequest);
    }
    else if (nsIEditProperty::font == aHTMLProperty &&
             aAttribute->Equals(NS_LITERAL_STRING("face"))) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, fontFaceEquivTable, aValue, aGetOrRemoveRequest);
    }
    else if (aAttribute->Equals(NS_LITERAL_STRING("bgcolor"))) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, bgcolorEquivTable, aValue, aGetOrRemoveRequest);
    }
    else if (aAttribute->Equals(NS_LITERAL_STRING("background"))) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, backgroundImageEquivTable, aValue, aGetOrRemoveRequest);
    }
    else if (aAttribute->Equals(NS_LITERAL_STRING("text"))) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, textColorEquivTable, aValue, aGetOrRemoveRequest);
    }
    else if (aAttribute->Equals(NS_LITERAL_STRING("border"))) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, borderEquivTable, aValue, aGetOrRemoveRequest);
    }
    else if (aAttribute->Equals(NS_LITERAL_STRING("align"))) {
      if (nsIEditProperty::table  == tagName) {
        BuildCSSDeclarations(cssPropertyArray, cssValueArray, tableAlignEquivTable, aValue, aGetOrRemoveRequest);
      }
      else if (nsIEditProperty::hr  == tagName) {
        BuildCSSDeclarations(cssPropertyArray, cssValueArray, hrAlignEquivTable, aValue, aGetOrRemoveRequest);
      }
      else if (nsIEditProperty::legend  == tagName ||
               nsIEditProperty::caption == tagName) {
        BuildCSSDeclarations(cssPropertyArray, cssValueArray, captionAlignEquivTable, aValue, aGetOrRemoveRequest);
      }
      else {
        BuildCSSDeclarations(cssPropertyArray, cssValueArray, textAlignEquivTable, aValue, aGetOrRemoveRequest);
      }
    }
    else if (aAttribute->Equals(NS_LITERAL_STRING("valign"))) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, verticalAlignEquivTable, aValue, aGetOrRemoveRequest);
    }
    else if (aAttribute->Equals(NS_LITERAL_STRING("nowrap"))) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, nowrapEquivTable, aValue, aGetOrRemoveRequest);
    }
    else if (aAttribute->Equals(NS_LITERAL_STRING("width"))) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, widthEquivTable, aValue, aGetOrRemoveRequest);
    }
    else if (aAttribute->Equals(NS_LITERAL_STRING("height")) ||
             (nsIEditProperty::hr == tagName && aAttribute->Equals(NS_LITERAL_STRING("size")))) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, heightEquivTable, aValue, aGetOrRemoveRequest);
    }
    else if (aAttribute->Equals(NS_LITERAL_STRING("type")) &&
             (nsIEditProperty::ol == tagName
              || nsIEditProperty::ul == tagName
              || nsIEditProperty::li == tagName)) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, listStyleTypeEquivTable, aValue, aGetOrRemoveRequest);
    }
  }
}

// Add to aNode the CSS inline style equivalent to HTMLProperty/aAttribute/aValue for the node,
// and return in aCount the number of CSS properties set by the call
nsresult
nsHTMLCSSUtils::SetCSSEquivalentToHTMLStyle(nsIDOMNode * aNode,
                                            nsIAtom *aHTMLProperty,
                                            const nsAString *aAttribute,
                                            const nsAString *aValue,
                                            PRInt32 * aCount)
{
  nsCOMPtr<nsIDOMElement> theElement = do_QueryInterface(aNode);
  nsresult res = NS_OK;
  *aCount = 0;
  if (theElement && IsCSSEditableProperty(aNode, aHTMLProperty, aAttribute)) {
    // we can apply the styles only if the node is an element and if we have
    // an equivalence for the requested HTML style in this implementation

    // Find the CSS equivalence to the HTML style
    nsVoidArray cssPropertyArray;
    nsStringArray cssValueArray;
    GenerateCSSDeclarationsFromHTMLStyle(aNode, aHTMLProperty, aAttribute, aValue,
                                         cssPropertyArray, cssValueArray, PR_FALSE);

    // set the individual CSS inline styles
    *aCount = cssPropertyArray.Count();
    PRInt32 index;
    for (index = 0; index < *aCount; index++) {
      nsAutoString valueString;
      cssValueArray.StringAt(index, valueString);
      nsCOMPtr<nsIDOMElement> theElement = do_QueryInterface(aNode);
      res = SetCSSProperty(theElement, (nsIAtom *)cssPropertyArray.ElementAt(index),
                           valueString);
      if (NS_FAILED(res)) return res;
    }
  }
  return NS_OK;
}

// Remove from aNode the CSS inline style equivalent to HTMLProperty/aAttribute/aValue for the node
nsresult
nsHTMLCSSUtils::RemoveCSSEquivalentToHTMLStyle(nsIDOMNode * aNode,
                                            nsIAtom *aHTMLProperty,
                                            const nsAString *aAttribute,
                                            const nsAString *aValue)
{
  nsCOMPtr<nsIDOMElement> theElement = do_QueryInterface(aNode);
  nsresult res = NS_OK;
  PRInt32 count = 0;
  if (theElement && IsCSSEditableProperty(aNode, aHTMLProperty, aAttribute)) {
    // we can apply the styles only if the node is an element and if we have
    // an equivalence for the requested HTML style in this implementation

    // Find the CSS equivalence to the HTML style
    nsVoidArray cssPropertyArray;
    nsStringArray cssValueArray;
    GenerateCSSDeclarationsFromHTMLStyle(aNode, aHTMLProperty, aAttribute, aValue,
                                         cssPropertyArray, cssValueArray, PR_TRUE);

    // remove the individual CSS inline styles
    count = cssPropertyArray.Count();
    PRInt32 index;
    for (index = 0; index < count; index++) {
      nsAutoString valueString;
      cssValueArray.StringAt(index, valueString);
      nsCOMPtr<nsIDOMElement> theElement = do_QueryInterface(aNode);
      res = RemoveCSSProperty(theElement, (nsIAtom *)cssPropertyArray.ElementAt(index), valueString);
      if (NS_FAILED(res)) return res;
    }
  }
  return NS_OK;
}

// aReturn is true if the element aElement carries an ID or a class.
nsresult
nsHTMLCSSUtils::HasClassOrID(nsIDOMElement * aElement, PRBool & aReturn)
{
  nsAutoString classVal, idVal;
  PRBool isClassSet, isIdSet;
  aReturn = PR_FALSE;

  nsresult res = mHTMLEditor->GetAttributeValue(aElement,  NS_LITERAL_STRING("class"), classVal, &isClassSet);
  if (NS_FAILED(res)) return res;
  res = mHTMLEditor->GetAttributeValue(aElement,  NS_LITERAL_STRING("id"), idVal, &isIdSet);
  if (NS_FAILED(res)) return res;

  // we need to make sure that if the element has an id or a class attribute,
  // the attribute is not the empty string
  aReturn = ((isClassSet && (0 != classVal.Length())) ||
             (isIdSet    && (0 != idVal.Length())));
  return NS_OK;
}

// returns in aValueString the list of values for the CSS equivalences to
// the HTML style aHTMLProperty/aAttribute/aValueString for the node aNode;
// the value of aStyleType controls the styles we retrieve : specified or
// computed.
nsresult
nsHTMLCSSUtils::GetCSSEquivalentToHTMLInlineStyleSet(nsIDOMNode * aNode,
                                                     nsIAtom *aHTMLProperty,
                                                     const nsAString *aAttribute,
                                                     nsAString & aValueString,
                                                     PRUint8 aStyleType)
{
  nsCOMPtr<nsIDOMElement> theElement;
  nsresult res = GetElementContainerOrSelf(aNode, getter_AddRefs(theElement));
  if (NS_FAILED(res)) return res;
  nsCOMPtr<nsIDOMNode> theNode = do_QueryInterface(theElement);

  if (theElement && IsCSSEditableProperty(theNode, aHTMLProperty, aAttribute)) {
    // Yes, the requested HTML style has a CSS equivalence in this implementation
    // Retrieve the default ViewCSS if we are asked for computed styles
    nsCOMPtr<nsIDOMViewCSS> viewCSS = nsnull;
    if (COMPUTED_STYLE_TYPE == aStyleType) {
      res = GetDefaultViewCSS(theNode, getter_AddRefs(viewCSS));
      if (NS_FAILED(res)) return res;
    }
    nsVoidArray cssPropertyArray;
    nsStringArray cssValueArray;
    // get the CSS equivalence with last param PR_TRUE indicating we want only the
    // "gettable" properties
    GenerateCSSDeclarationsFromHTMLStyle(theNode, aHTMLProperty, aAttribute, nsnull,
                                         cssPropertyArray, cssValueArray, PR_TRUE);
    PRInt32 count = cssPropertyArray.Count();
    PRInt32 index;
    for (index = 0; index < count; index++) {
      nsAutoString valueString;
      // retrieve the specified/computed value of the property
      res = GetCSSInlinePropertyBase(theNode, (nsIAtom *)cssPropertyArray.ElementAt(index),
                                     valueString, viewCSS, aStyleType);
      if (NS_FAILED(res)) return res;
      // append the value to aValueString (possibly with a leading whitespace)
      if (index) aValueString.Append(PRUnichar(' '));
      aValueString.Append(valueString);
    }
  }
  return NS_OK;
}

// Does the node aNode (or his parent if it is not an element node) carries
// the CSS equivalent styles to the HTML style aHTMLProperty/aAttribute/
// aValueString for this node ?
// The value of aStyleType controls the styles we retrieve : specified or
// computed. The return value aIsSet is true is the CSS styles are set.
nsresult
nsHTMLCSSUtils::IsCSSEquivalentToHTMLInlineStyleSet(nsIDOMNode * aNode,
                                                    nsIAtom *aHTMLProperty,
                                                    const nsAString * aHTMLAttribute,
                                                    PRBool & aIsSet,
                                                    nsAString & valueString,
                                                    PRUint8 aStyleType)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);

  nsAutoString htmlValueString(valueString);
  aIsSet = PR_FALSE;
  nsCOMPtr<nsIDOMNode> node = aNode;
  do {
    valueString.Assign(htmlValueString);
    // get the value of the CSS equivalent styles
    nsresult res = GetCSSEquivalentToHTMLInlineStyleSet(node, aHTMLProperty, aHTMLAttribute,
                                                        valueString, aStyleType);
    if (NS_FAILED(res)) return res;

    // early way out if we can
    if (valueString.Equals(NS_LITERAL_STRING(""))) return NS_OK;

    if (nsIEditProperty::b == aHTMLProperty) {
      if (valueString.Equals(NS_LITERAL_STRING("bold"))) {
        aIsSet = PR_TRUE;
      }
      else if (valueString.Equals(NS_LITERAL_STRING("normal"))) {
        aIsSet = PR_FALSE;
      }
      else if (valueString.Equals(NS_LITERAL_STRING("bolder"))) {
        aIsSet = PR_TRUE;
        valueString.Assign(NS_LITERAL_STRING("bold"));
      }
      else {
        PRInt32 weight = 0;
        PRInt32 errorCode;
        nsAutoString value(valueString);
        weight = value.ToInteger(&errorCode, 10);
        if (400 < weight) {
          aIsSet = PR_TRUE;
          valueString.Assign(NS_LITERAL_STRING("bold"));
        }
        else {
          aIsSet = PR_FALSE;
          valueString.Assign(NS_LITERAL_STRING("normal"));
        }
      }
    }
    
    else if (nsIEditProperty::i == aHTMLProperty) {
      if (valueString.Equals(NS_LITERAL_STRING("italic")) ||
          valueString.Equals(NS_LITERAL_STRING("oblique"))) {
        aIsSet= PR_TRUE;
      }
    }

    else if (nsIEditProperty::u == aHTMLProperty) {
      nsAutoString val;
      val.Assign(NS_LITERAL_STRING("underline"));
      aIsSet = PRBool(ChangeCSSInlineStyleTxn::ValueIncludes(valueString, val, PR_FALSE));
    }

    else if (nsIEditProperty::strike == aHTMLProperty) {
      nsAutoString val;
      val.Assign(NS_LITERAL_STRING("line-through"));
      aIsSet = PRBool(ChangeCSSInlineStyleTxn::ValueIncludes(valueString, val, PR_FALSE));
    }

    else if ((nsIEditProperty::font == aHTMLProperty) && aHTMLAttribute
             && aHTMLAttribute->Equals(NS_LITERAL_STRING("color"))) {
      aIsSet = PR_TRUE;
    }

    else if (nsIEditProperty::tt == aHTMLProperty) {
      aIsSet = Substring(valueString, 0, 9).Equals(NS_LITERAL_STRING("monospace"));
    }
    
    else if ((nsIEditProperty::font == aHTMLProperty) && aHTMLAttribute
             && aHTMLAttribute->Equals(NS_LITERAL_STRING("face"))) {
      nsAutoString leftCSSValue;
      leftCSSValue = Substring(valueString, 0, 5);
      ToLowerCase(leftCSSValue);
      if (!htmlValueString.Equals(NS_LITERAL_STRING(""))) {
        nsAutoString leftHTMLValue;
        htmlValueString.Left(leftHTMLValue, 5);
        aIsSet = leftHTMLValue.Equals(leftCSSValue,
                                      nsCaseInsensitiveStringComparator());
      }
      else {
        aIsSet = (leftCSSValue.Equals(NS_LITERAL_STRING("times")) ||
                  leftCSSValue.Equals(NS_LITERAL_STRING("helve")) ||
                  leftCSSValue.Equals(NS_LITERAL_STRING("couri")));
      }
      return NS_OK;
    }
    else if (aHTMLAttribute && aHTMLAttribute
             && aHTMLAttribute->Equals(NS_LITERAL_STRING("bgcolor"))) {
      aIsSet = PR_TRUE;
    }
    else if (aHTMLAttribute && aHTMLAttribute
             && aHTMLAttribute->Equals(NS_LITERAL_STRING("align"))) {
      aIsSet = PR_TRUE;
    }
    else {
      aIsSet = PR_FALSE;
      return NS_OK;
    }

    if (!htmlValueString.Equals(NS_LITERAL_STRING(""))) {
      if (htmlValueString.Equals(valueString,
                                 nsCaseInsensitiveStringComparator())) {
        aIsSet = PR_TRUE;
      }
    }

    if (nsIEditProperty::u == aHTMLProperty || nsIEditProperty::strike == aHTMLProperty) {
      // unfortunately, the value of the text-decoration property is not inherited.
      // that means that we have to look at ancestors of node to see if they are underlined
      nsCOMPtr<nsIDOMNode> tmp;
      res = node->GetParentNode(getter_AddRefs(tmp));
      if (NS_FAILED(res)) return res;
      nsCOMPtr<nsIDOMElement> element = do_QueryInterface(tmp);
      if (element) {
        node = tmp;
      }
      else {
        node = nsnull;
      }
    }
  } while ((nsIEditProperty::u == aHTMLProperty || nsIEditProperty::strike == aHTMLProperty) &&
           !aIsSet && node);
  return NS_OK;
}

nsresult
nsHTMLCSSUtils::SetCSSEnabled(PRBool aIsCSSPrefChecked)
{
  mIsCSSPrefChecked = aIsCSSPrefChecked;
  return NS_OK;
}

PRBool
nsHTMLCSSUtils::IsCSSPrefChecked()
{
  return mIsCSSPrefChecked ;
}

// ElementsSameStyle compares two elements and checks if they have the same
// specified CSS declarations in the STYLE attribute 
// The answer is always negative if at least one of them carries an ID or a class
PRBool
nsHTMLCSSUtils::ElementsSameStyle(nsIDOMNode *aFirstNode, nsIDOMNode *aSecondNode)
{
  nsresult res;
  nsCOMPtr<nsIDOMElement> firstElement  = do_QueryInterface(aFirstNode);
  nsCOMPtr<nsIDOMElement> secondElement = do_QueryInterface(aSecondNode);

  NS_ASSERTION((firstElement && secondElement), "Non element nodes passed to ElementsSameStyle.");

  nsAutoString firstID, secondID;
  PRBool isFirstIDSet, isSecondIDSet;
  res = mHTMLEditor->GetAttributeValue(firstElement,  NS_LITERAL_STRING("id"), firstID,  &isFirstIDSet);
  res = mHTMLEditor->GetAttributeValue(secondElement, NS_LITERAL_STRING("id"), secondID, &isSecondIDSet);
  if (isFirstIDSet || isSecondIDSet) {
    // at least one of the spans carries an ID ; suspect a CSS rule applies to it and
    // refuse to merge the nodes
    return PR_FALSE;
  }

  nsAutoString firstClass, secondClass;
  PRBool isFirstClassSet, isSecondClassSet;
  res = mHTMLEditor->GetAttributeValue(firstElement,  NS_LITERAL_STRING("class"), firstClass,  &isFirstClassSet);
  res = mHTMLEditor->GetAttributeValue(secondElement, NS_LITERAL_STRING("class"), secondClass, &isSecondClassSet);
  if (isFirstClassSet && isSecondClassSet) {
    // both spans carry a class, let's compare them
    if (!firstClass.Equals(secondClass)) {
      // WARNING : technically, the comparison just above is questionable :
      // from a pure HTML/CSS point of view class="a b" is NOT the same than
      // class="b a" because a CSS rule could test the exact value of the class
      // attribute to be "a b" for instance ; from a user's point of view, a
      // wysiwyg editor should probably NOT make any difference. CSS people
      // need to discuss this issue before any modification.
      return PR_FALSE;
    }
  }
  else if (isFirstClassSet || isSecondClassSet) {
    // one span only carries a class, early way out
    return PR_FALSE;
  }

  nsCOMPtr<nsIDOMCSSStyleDeclaration> firstCSSDecl, secondCSSDecl;
  PRUint32 firstLength, secondLength;
  res = GetInlineStyles(firstElement,  getter_AddRefs(firstCSSDecl),  &firstLength);
  if (NS_FAILED(res) || !firstCSSDecl) return PR_FALSE;
  res = GetInlineStyles(secondElement, getter_AddRefs(secondCSSDecl), &secondLength);
  if (NS_FAILED(res) || !secondCSSDecl) return PR_FALSE;

  if (firstLength != secondLength) {
    // early way out if we can
    return PR_FALSE;
  }
  else if (0 == firstLength) {
    // no inline style !
    return PR_TRUE;
  }

  PRUint32 i;
  nsAutoString propertyNameString;
  nsAutoString firstValue, secondValue;
  for (i=0; i<firstLength; i++) {
    firstCSSDecl->Item(i, propertyNameString);
    firstCSSDecl->GetPropertyValue(propertyNameString, firstValue);
    secondCSSDecl->GetPropertyValue(propertyNameString, secondValue);
    if (!firstValue.Equals(secondValue)) {
      return PR_FALSE;
    }
  }
  for (i=0; i<secondLength; i++) {
    secondCSSDecl->Item(i, propertyNameString);
    secondCSSDecl->GetPropertyValue(propertyNameString, secondValue);
    firstCSSDecl->GetPropertyValue(propertyNameString, firstValue);
    if (!firstValue.Equals(secondValue)) {
      return PR_FALSE;
    }
  }

  return PR_TRUE;
}

nsresult
nsHTMLCSSUtils::GetInlineStyles(nsIDOMElement *aElement,
                                nsIDOMCSSStyleDeclaration **aCssDecl,
                                PRUint32 *aLength)
{
  if (!aElement || !aLength) return NS_ERROR_NULL_POINTER;
  *aLength = 0;
  nsCOMPtr<nsIDOMElementCSSInlineStyle> inlineStyles = do_QueryInterface(aElement);
  if (!inlineStyles) return NS_ERROR_NULL_POINTER;
  nsresult res = inlineStyles->GetStyle(aCssDecl);
  if (NS_FAILED(res) || !aCssDecl) return NS_ERROR_NULL_POINTER;
  (*aCssDecl)->GetLength(aLength);
  return NS_OK;
}

nsresult
nsHTMLCSSUtils::GetElementContainerOrSelf(nsIDOMNode * aNode, nsIDOMElement ** aElement)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMNode> node=aNode, parentNode=aNode;
  PRUint16 type;
  nsresult res;
  res = node->GetNodeType(&type);
  if (NS_FAILED(res)) return res;
  // loop until we find an element
  while (node && nsIDOMNode::ELEMENT_NODE != type) {
    parentNode = node;
    parentNode->GetParentNode(getter_AddRefs(node));
    if (NS_FAILED(res)) return res;
    node->GetNodeType(&type);
    if (NS_FAILED(res)) return res;
  }
  NS_ENSURE_TRUE(node, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMElement> element = do_QueryInterface(node);
  (*aElement) = element;
  NS_ADDREF(*aElement);
  return NS_OK;
}
