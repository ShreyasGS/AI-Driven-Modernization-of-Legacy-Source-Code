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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   Daniel Glazman <glazman@netscape.com>
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

#include "nscore.h"

#define PL_ARENA_CONST_ALIGN_MASK 7
#define NS_RULEHASH_ARENA_BLOCK_SIZE (256)
#include "plarena.h"

#include "nsICSSStyleSheet.h"
#include "nsIArena.h"
#include "nsCRT.h"
#include "nsIAtom.h"
#include "nsIURL.h"
#include "nsIServiceManager.h"
#include "nsISupportsArray.h"
#include "nsHashtable.h"
#include "nsICSSPseudoComparator.h"
#include "nsICSSStyleRuleProcessor.h"
#include "nsICSSStyleRule.h"
#include "nsICSSNameSpaceRule.h"
#include "nsICSSMediaRule.h"
#include "nsIMediaList.h"
#include "nsIHTMLContent.h"
#include "nsIDocument.h"
#include "nsIHTMLContentContainer.h"
#include "nsIPresContext.h"
#include "nsIEventStateManager.h"
#include "nsHTMLAtoms.h"
#include "nsLayoutAtoms.h"
#include "nsIFrame.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsVoidArray.h"
#include "nsIUnicharInputStream.h"
#include "nsHTMLIIDs.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLLinkElement.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLOptionElement.h"
#include "nsIDOMStyleSheetList.h"
#include "nsIDOMCSSStyleSheet.h"
#include "nsIDOMCSSStyleRule.h"
#include "nsIDOMCSSRuleList.h"
#include "nsIDOMMediaList.h"
#include "nsIDOMNode.h"
#include "nsDOMError.h"
#include "nsIPresShell.h"
#include "nsICSSParser.h"
#include "nsICSSLoader.h"
#include "nsRuleWalker.h"
#include "nsCSSAtoms.h"
#include "nsINameSpaceManager.h"
#include "nsINameSpace.h"
#include "nsITextContent.h"
#include "prlog.h"
#include "nsCOMPtr.h"
#include "nsIStyleSet.h"
#include "nsISizeOfHandler.h"
#include "nsStyleUtil.h"
#include "nsQuickSort.h"
#ifdef MOZ_XUL
#include "nsIXULContent.h"
#endif

#include "nsContentUtils.h"

//#define DEBUG_RULES
//#define EVENT_DEBUG
//#define DEBUG_HASH
 
#ifdef DEBUG_HASH
static void DebugHashCount(PRBool hash) {
  static gCountHash = 0;
  static gCountCompStr = 0;
  if (hash) {
    if (++gCountHash % 100 == 0) {
      printf("gCountHash   = %ld\n", gCountHash);
    }
  }
  else {
    if (++gCountCompStr % 100 == 0) {
      printf("gCountCompStr = %ld\n", gCountCompStr);
    }
  }
}
#endif //DEBUG_HASH


// ----------------------
// Rule hash keys
//
// An |AtomKey| is to be used for storage in the hashtable, and a
// |DependentAtomKey| should be used on the stack to avoid the performance
// cost of refcounting an atom that one is assured to own for the lifetime
// of the key.

class AtomKey_base : public nsHashKey {
public:
  virtual PRUint32 HashCode(void) const;
  virtual PRBool Equals(const nsHashKey *aKey) const;
  virtual void SetKeyCaseSensitive(PRBool aCaseSensitive);
  nsIAtom*  mAtom;
  PRBool    mCaseSensitive;
};

class AtomKey : public AtomKey_base {
public:
  AtomKey(nsIAtom* aAtom);
  AtomKey(const AtomKey_base& aKey);
  virtual ~AtomKey(void);
  virtual nsHashKey *Clone(void) const;
};


class DependentAtomKey : public AtomKey_base {
public:
  DependentAtomKey(nsIAtom* aAtom)
    {
      mAtom = aAtom;
      SetKeyCaseSensitive(PR_TRUE);
    }
  DependentAtomKey(const DependentAtomKey& aKey);
  virtual ~DependentAtomKey(void)
    {
    }
  virtual nsHashKey *Clone(void) const;
};

void AtomKey_base::SetKeyCaseSensitive(PRBool aCaseSensitive)
{
  mCaseSensitive = aCaseSensitive;
}

PRUint32 AtomKey_base::HashCode(void) const
{
  if (mCaseSensitive) {
    return NS_PTR_TO_INT32(mAtom);
  }
  else {
#ifdef DEBUG_HASH
  DebugHashCount(PR_TRUE);
#endif
    nsAutoString myStr;
    mAtom->ToString(myStr);
    ToUpperCase(myStr);
    return nsCRT::HashCode(myStr.get());
  }
}

PRBool AtomKey_base::Equals(const nsHashKey* aKey) const
{
  if (mCaseSensitive) {
    return PRBool (((AtomKey_base*)aKey)->mAtom == mAtom);
  }

  // first try case-sensitive match
  if (((AtomKey_base*)aKey)->mAtom == mAtom)
    return PR_TRUE;
  
#ifdef DEBUG_HASH
  DebugHashCount(PR_FALSE);
#endif
  const PRUnichar *myStr = nsnull;
  mAtom->GetUnicode(&myStr);

  nsIAtom* theirAtom = ((AtomKey_base*)aKey)->mAtom; 

  const PRUnichar *theirStr = nsnull;
  theirAtom->GetUnicode(&theirStr);

  return nsDependentString(myStr).Equals(nsDependentString(theirStr),
                                         nsCaseInsensitiveStringComparator());
}


AtomKey::AtomKey(nsIAtom* aAtom)
{
  mAtom = aAtom;
  NS_ADDREF(mAtom);
  SetKeyCaseSensitive(PR_TRUE);
}

AtomKey::AtomKey(const AtomKey_base& aKey)
{
  mAtom = aKey.mAtom;
  NS_ADDREF(mAtom);
  SetKeyCaseSensitive(PR_TRUE);
}

AtomKey::~AtomKey(void)
{
  NS_RELEASE(mAtom);
}

nsHashKey* AtomKey::Clone(void) const
{
  return new AtomKey(*this);
}


DependentAtomKey::DependentAtomKey(const DependentAtomKey& aKey)
{
  NS_NOTREACHED("Should never clone to a dependent atom key.");
  mAtom = aKey.mAtom;
  SetKeyCaseSensitive(PR_TRUE);
}

nsHashKey* DependentAtomKey::Clone(void) const
{
  return new AtomKey(*this); // return a non-dependent key
}

// ----------------------
// Rule array hash key
//
class nsInt32Key: public nsHashKey {
public:
  nsInt32Key(PRInt32 aInt);
  nsInt32Key(const nsInt32Key& aKey);
  virtual ~nsInt32Key(void);
  virtual PRUint32 HashCode(void) const;
  virtual PRBool Equals(const nsHashKey *aKey) const;
  virtual nsHashKey *Clone(void) const;
  PRInt32 mInt;
};

nsInt32Key::nsInt32Key(PRInt32 aInt) :
  mInt(aInt)
{
}

nsInt32Key::~nsInt32Key(void)
{
}

PRUint32 nsInt32Key::HashCode(void) const
{
  return (PRUint32)mInt;
}

PRBool nsInt32Key::Equals(const nsHashKey* aKey) const
{
  return PRBool (((nsInt32Key*)aKey)->mInt == mInt);
}

nsHashKey* nsInt32Key::Clone(void) const
{
  return new nsInt32Key(mInt);
}


struct RuleValue {
  RuleValue(nsICSSStyleRule* aRule, PRInt32 aIndex)
  {
    mRule = aRule;
    mIndex = aIndex;
    mNext = nsnull;
  }

  // CAUTION: ~RuleValue will never get called as RuleValues are arena
  // allocated and arena cleanup will take care of deleting memory.
  // Add code to RuleHash::~RuleHash to get it to call the destructor
  // if any more cleanup needs to happen.
  ~RuleValue(void)
  {
    // Rule values are arena allocated. No need for any deletion.
  }

  // Placement new to arena allocate the RuleValues
  void *operator new(size_t aSize, PLArenaPool &aArena) {
    void *mem;
    PL_ARENA_ALLOCATE(mem, &aArena, aSize);
    return mem;
  }

  nsICSSStyleRule*  mRule;
  PRInt32           mIndex;
  RuleValue*        mNext;
};

// ------------------------------
// Rule hash table
//

#undef RULE_HASH_STATS
#undef PRINT_UNIVERSAL_RULES

#ifdef DEBUG_dbaron
#define RULE_HASH_STATS
#define PRINT_UNIVERSAL_RULES
#endif

// Enumerator callback function.
typedef void (*RuleEnumFunc)(nsICSSStyleRule* aRule, void *aData);

class RuleHash {
public:
  RuleHash(void);
  ~RuleHash(void);
  void AppendRule(nsICSSStyleRule* aRule);
  void EnumerateAllRules(PRInt32 aNameSpace, nsIAtom* aTag, nsIAtom* aID,
                         const nsVoidArray& aClassList,
                         RuleEnumFunc aFunc, void* aData);
  void EnumerateTagRules(nsIAtom* aTag,
                         RuleEnumFunc aFunc, void* aData);
  void SetCaseSensitive(PRBool aCaseSensitive) {mCaseSensitive = aCaseSensitive;};

protected:
  void AppendRuleToTable(nsHashtable& aTable, nsIAtom* aAtom, nsICSSStyleRule* aRule, PRBool aCaseSensitive = PR_TRUE);
  void AppendRuleToTable(nsHashtable& aTable, PRInt32 aNameSpace, nsICSSStyleRule* aRule);

  // All rule values in these hashtables are arena allocated
  PRInt32     mRuleCount;
  nsHashtable mIdTable;
  nsHashtable mClassTable;
  nsHashtable mTagTable;
  nsHashtable mNameSpaceTable;

  RuleValue** mEnumList;
  PRInt32     mEnumListSize;
  RuleValue   mEndValue;
  PRBool      mCaseSensitive;

  PLArenaPool mArena;

#ifdef RULE_HASH_STATS
  PRUint32    mUniversalSelectors;
  PRUint32    mNameSpaceSelectors;
  PRUint32    mTagSelectors;
  PRUint32    mClassSelectors;
  PRUint32    mIdSelectors;

  PRUint32    mElementsMatched;
  PRUint32    mPseudosMatched;

  PRUint32    mElementUniversalCalls;
  PRUint32    mElementNameSpaceCalls;
  PRUint32    mElementTagCalls;
  PRUint32    mElementClassCalls;
  PRUint32    mElementIdCalls;

  PRUint32    mPseudoTagCalls;
#endif // RULE_HASH_STATS
};

RuleHash::RuleHash(void)
  : mRuleCount(0),
    mIdTable(), mClassTable(), mTagTable(), mNameSpaceTable(),
    mEnumList(nsnull), mEnumListSize(0),
    mEndValue(nsnull, -1),
    mCaseSensitive(PR_TRUE)
#ifdef RULE_HASH_STATS
    ,
    mUniversalSelectors(0),
    mNameSpaceSelectors(0),
    mTagSelectors(0),
    mClassSelectors(0),
    mIdSelectors(0),
    mElementsMatched(0),
    mPseudosMatched(0),
    mElementUniversalCalls(0),
    mElementNameSpaceCalls(0),
    mElementTagCalls(0),
    mElementClassCalls(0),
    mElementIdCalls(0),
    mPseudoTagCalls(0)
#endif
{
  // Initialize our arena
  PL_INIT_ARENA_POOL(&mArena, "RuleHashArena", NS_RULEHASH_ARENA_BLOCK_SIZE);
}

RuleHash::~RuleHash(void)
{
#ifdef RULE_HASH_STATS
  printf(
"RuleHash(%p):\n"
"  Selectors: Universal (%lu) NameSpace(%lu) Tag(%lu) Class(%lu) Id(%lu)\n"
"  Content Nodes: Elements(%lu) Pseudo-Elements(%lu)\n"
"  Element Calls: Universal(%lu) NameSpace(%lu) Tag(%lu) Class(%lu) Id(%lu)\n"
"  Pseudo-Element Calls: Tag(%lu)\n",
         this,
         mUniversalSelectors, mNameSpaceSelectors, mTagSelectors,
           mClassSelectors, mIdSelectors,
         mElementsMatched,
         mPseudosMatched,
         mElementUniversalCalls, mElementNameSpaceCalls, mElementTagCalls,
           mElementClassCalls, mElementIdCalls,
         mPseudoTagCalls);
#ifdef PRINT_UNIVERSAL_RULES
  {
    DependentAtomKey universalKey(nsCSSAtoms::universalSelector);
    RuleValue*  value = (RuleValue*)mTagTable.Get(&universalKey);
    if (value) {
      printf("  Universal rules:\n");
      do {
        nsAutoString selectorText;
        PRUint32 lineNumber = value->mRule->GetLineNumber();
        value->mRule->GetSourceSelectorText(selectorText);
        printf("    line %d, %s\n",
               lineNumber, NS_ConvertUCS2toUTF8(selectorText).get());
        value = value->mNext;
      } while (value != &mEndValue);
    }
  }
#endif // PRINT_UNIVERSAL_RULES
#endif // RULE_HASH_STATS
  // Rule Values are arena allocated no need to delete them. Their destructor
  // isn't doing any cleanup. So we dont even bother to enumerate through
  // the hash tables and call their destructors.
  if (nsnull != mEnumList) {
    delete [] mEnumList;
  }
  // delete arena for strings and small objects
  PL_FinishArenaPool(&mArena);
}

void RuleHash::AppendRuleToTable(nsHashtable& aTable, nsIAtom* aAtom, nsICSSStyleRule* aRule, PRBool aCaseSensitive)
{
  NS_ASSERTION(nsnull != aAtom, "null hash key");

  DependentAtomKey key(aAtom);
  key.SetKeyCaseSensitive(aCaseSensitive);
  RuleValue*  value = (RuleValue*)aTable.Get(&key);

  if (nsnull == value) {
    value = new (mArena) RuleValue(aRule, mRuleCount++);
    aTable.Put(&key, value);
    value->mNext = &mEndValue;
  }
  else {
    while (&mEndValue != value->mNext) {
      value = value->mNext;
    }
    value->mNext = new (mArena) RuleValue(aRule, mRuleCount++);
    value->mNext->mNext = &mEndValue;
  }
  mEndValue.mIndex = mRuleCount;
}

void RuleHash::AppendRuleToTable(nsHashtable& aTable, PRInt32 aNameSpace, nsICSSStyleRule* aRule)
{
  nsInt32Key key(aNameSpace);
  RuleValue*  value = (RuleValue*)aTable.Get(&key);

  if (nsnull == value) {
    value = new (mArena) RuleValue(aRule, mRuleCount++);
    aTable.Put(&key, value);
    value->mNext = &mEndValue;
  }
  else {
    while (&mEndValue != value->mNext) {
      value = value->mNext;
    }
    value->mNext = new (mArena) RuleValue(aRule, mRuleCount++);
    value->mNext->mNext = &mEndValue;
  }
  mEndValue.mIndex = mRuleCount;
}

void RuleHash::AppendRule(nsICSSStyleRule* aRule)
{
  nsCSSSelector*  selector = aRule->FirstSelector();
  if (nsnull != selector->mIDList) {
    AppendRuleToTable(mIdTable, selector->mIDList->mAtom, aRule, mCaseSensitive);
#ifdef RULE_HASH_STATS
    ++mIdSelectors;
#endif
  }
  else if (nsnull != selector->mClassList) {
    AppendRuleToTable(mClassTable, selector->mClassList->mAtom, aRule, mCaseSensitive);
#ifdef RULE_HASH_STATS
    ++mClassSelectors;
#endif
  }
  else if (nsnull != selector->mTag) {
    AppendRuleToTable(mTagTable, selector->mTag, aRule);
#ifdef RULE_HASH_STATS
    ++mTagSelectors;
#endif
  }
  else if (kNameSpaceID_Unknown != selector->mNameSpace) {
    AppendRuleToTable(mNameSpaceTable, selector->mNameSpace, aRule);
#ifdef RULE_HASH_STATS
    ++mNameSpaceSelectors;
#endif
  }
  else {  // universal tag selector
    AppendRuleToTable(mTagTable, nsCSSAtoms::universalSelector, aRule);
#ifdef RULE_HASH_STATS
    ++mUniversalSelectors;
#endif
  }
}

// this should cover practically all cases so we don't need to reallocate
#define MIN_ENUM_LIST_SIZE 8

void RuleHash::EnumerateAllRules(PRInt32 aNameSpace, nsIAtom* aTag,
                                 nsIAtom* aID, const nsVoidArray& aClassList,
                                 RuleEnumFunc aFunc, void* aData)
{
  PRInt32 classCount = aClassList.Count();
  PRInt32 testCount = classCount + 1; // + 1 for universal selector
  if (nsnull != aTag) {
    testCount++;
  }
  if (nsnull != aID) {
    testCount++;
  }
  if (kNameSpaceID_Unknown != aNameSpace) {
    testCount++;
  }

  if (mEnumListSize < testCount) {
    delete [] mEnumList;
    mEnumListSize = PR_MAX(testCount, MIN_ENUM_LIST_SIZE);
    mEnumList = new RuleValue*[mEnumListSize];
  }

  PRInt32 index;
  PRInt32 valueCount = 0;
#ifdef RULE_HASH_STATS
  ++mElementsMatched;
#endif

  { // universal tag rules
    DependentAtomKey universalKey(nsCSSAtoms::universalSelector);
    RuleValue*  value = (RuleValue*)mTagTable.Get(&universalKey);
    if (nsnull != value) {
      mEnumList[valueCount++] = value;
#ifdef RULE_HASH_STATS
      do {
        ++mElementUniversalCalls;
        value = value->mNext;
      } while (value != &mEndValue);
#endif
    }
  }
  // universal rules within the namespace
  if (kNameSpaceID_Unknown != aNameSpace) {
    nsInt32Key nameSpaceKey(aNameSpace);
    RuleValue* value = (RuleValue*)mNameSpaceTable.Get(&nameSpaceKey);
    if (nsnull != value) {
      mEnumList[valueCount++] = value;
#ifdef RULE_HASH_STATS
      do {
        ++mElementNameSpaceCalls;
        value = value->mNext;
      } while (value != &mEndValue);
#endif
    }
  }
  if (nsnull != aTag) {
    DependentAtomKey tagKey(aTag);
    RuleValue* value = (RuleValue*)mTagTable.Get(&tagKey);
    if (nsnull != value) {
      mEnumList[valueCount++] = value;
#ifdef RULE_HASH_STATS
      do {
        ++mElementTagCalls;
        value = value->mNext;
      } while (value != &mEndValue);
#endif
    }
  }
  if (nsnull != aID) {
    DependentAtomKey idKey(aID);
    idKey.SetKeyCaseSensitive(mCaseSensitive);
    RuleValue* value = (RuleValue*)mIdTable.Get(&idKey);
    if (nsnull != value) {
      mEnumList[valueCount++] = value;
#ifdef RULE_HASH_STATS
      do {
        ++mElementIdCalls;
        value = value->mNext;
      } while (value != &mEndValue);
#endif
    }
  }
  for (index = 0; index < classCount; index++) {
    nsIAtom* classAtom = (nsIAtom*)aClassList.ElementAt(index);
    DependentAtomKey classKey(classAtom);
    classKey.SetKeyCaseSensitive(mCaseSensitive);
    RuleValue* value = (RuleValue*)mClassTable.Get(&classKey);
    if (nsnull != value) {
      mEnumList[valueCount++] = value;
#ifdef RULE_HASH_STATS
      do {
        ++mElementClassCalls;
        value = value->mNext;
      } while (value != &mEndValue);
#endif
    }
  }
  NS_ASSERTION(valueCount <= testCount, "values exceeded list size");

  if (1 < valueCount) {
    PRInt32 ruleIndex = mRuleCount;
    PRInt32 valueIndex = -1;

    for (index = 0; index < valueCount; index++) {
      if (mEnumList[index]->mIndex < ruleIndex) {
        ruleIndex = mEnumList[index]->mIndex;
        valueIndex = index;
      }
    }
    do {
      (*aFunc)(mEnumList[valueIndex]->mRule, aData);
      mEnumList[valueIndex] = mEnumList[valueIndex]->mNext;
      ruleIndex = mEnumList[valueIndex]->mIndex;
      for (index = 0; index < valueCount; index++) {
        if (mEnumList[index]->mIndex < ruleIndex) {
          ruleIndex = mEnumList[index]->mIndex;
          valueIndex = index;
        }
      }
    } while (ruleIndex < mRuleCount);
  }
  else if (0 < valueCount) {  // fast loop over single value
    RuleValue* value = mEnumList[0];
    do {
      (*aFunc)(value->mRule, aData);
      value = value->mNext;
    } while (&mEndValue != value);
  }
}

void RuleHash::EnumerateTagRules(nsIAtom* aTag, RuleEnumFunc aFunc, void* aData)
{
  DependentAtomKey tagKey(aTag);
  RuleValue*  tagValue = (RuleValue*)mTagTable.Get(&tagKey);

#ifdef RULE_HASH_STATS
  ++mPseudosMatched;
#endif
  if (tagValue) {
    do {
#ifdef RULE_HASH_STATS
      ++mPseudoTagCalls;
#endif
      (*aFunc)(tagValue->mRule, aData);
      tagValue = tagValue->mNext;
    } while (&mEndValue != tagValue);
  }
}

//--------------------------------

struct RuleCascadeData {
  RuleCascadeData(nsIAtom *aMedium)
    : mWeightedRules(nsnull),
      mRuleHash(),
      mStateSelectors(),
      mMedium(aMedium),
      mNext(nsnull)
  {
    NS_NewISupportsArray(&mWeightedRules);
  }

  ~RuleCascadeData(void)
  {
    NS_IF_RELEASE(mWeightedRules);
  }
  nsISupportsArray* mWeightedRules;
  RuleHash          mRuleHash;
  nsVoidArray       mStateSelectors;

  nsCOMPtr<nsIAtom> mMedium;
  RuleCascadeData*  mNext; // for a different medium
};

// -------------------------------
// CSS Style Rule processor
//

class CSSRuleProcessor: public nsICSSStyleRuleProcessor {
public:
  CSSRuleProcessor(void);
  virtual ~CSSRuleProcessor(void);

  NS_DECL_ISUPPORTS

public:
  // nsICSSStyleRuleProcessor
  NS_IMETHOD AppendStyleSheet(nsICSSStyleSheet* aStyleSheet);

  NS_IMETHOD ClearRuleCascades(void);

  // nsIStyleRuleProcessor
  NS_IMETHOD RulesMatching(ElementRuleProcessorData* aData,
                           nsIAtom* aMedium);

  NS_IMETHOD RulesMatching(PseudoRuleProcessorData* aData,
                           nsIAtom* aMedium);

  NS_IMETHOD HasStateDependentStyle(StateRuleProcessorData* aData,
                                    nsIAtom* aMedium,
                                    PRBool* aResult);

#ifdef DEBUG
  virtual void SizeOf(nsISizeOfHandler *aSizeofHandler, PRUint32 &aSize);
#endif

protected:
  RuleCascadeData* GetRuleCascade(nsIPresContext* aPresContext, nsIAtom* aMedium);

  static PRBool CascadeSheetRulesInto(nsISupports* aSheet, void* aData);

  nsISupportsArray* mSheets;

  RuleCascadeData*  mRuleCascades;
};

// -------------------------------
// CSS Style Sheet Inner Data Container
//

class CSSStyleSheetInner {
public:
  CSSStyleSheetInner(nsICSSStyleSheet* aParentSheet);
  CSSStyleSheetInner(CSSStyleSheetInner& aCopy, nsICSSStyleSheet* aParentSheet);
  virtual ~CSSStyleSheetInner(void);

  virtual CSSStyleSheetInner* CloneFor(nsICSSStyleSheet* aParentSheet);
  virtual void AddSheet(nsICSSStyleSheet* aParentSheet);
  virtual void RemoveSheet(nsICSSStyleSheet* aParentSheet);

  virtual void RebuildNameSpaces(void);

#ifdef DEBUG
  virtual void SizeOf(nsISizeOfHandler *aSizeofHandler, PRUint32 &aSize);
#endif

  nsAutoVoidArray       mSheets;

  nsIURI*               mURL;

  nsISupportsArray*     mOrderedRules;

  nsCOMPtr<nsINameSpace> mNameSpace;
  PRInt32               mDefaultNameSpaceID;
  nsHashtable           mRelevantAttributes;
};


// -------------------------------
// CSS Style Sheet
//

class CSSImportsCollectionImpl;
class CSSRuleListImpl;
class DOMMediaListImpl;

class CSSStyleSheetImpl : public nsICSSStyleSheet, 
                          public nsIDOMCSSStyleSheet
{
public:
  void* operator new(size_t size);
  void* operator new(size_t size, nsIArena* aArena);
  void operator delete(void* ptr);

  CSSStyleSheetImpl();

  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
  NS_IMETHOD_(nsrefcnt) AddRef();
  NS_IMETHOD_(nsrefcnt) Release();

  // basic style sheet data
  NS_IMETHOD Init(nsIURI* aURL);
  NS_IMETHOD GetURL(nsIURI*& aURL) const;
  NS_IMETHOD GetTitle(nsString& aTitle) const;
  NS_IMETHOD SetTitle(const nsString& aTitle);
  NS_IMETHOD GetType(nsString& aType) const;
  NS_IMETHOD GetMediumCount(PRInt32& aCount) const;
  NS_IMETHOD GetMediumAt(PRInt32 aIndex, nsIAtom*& aMedium) const;
  NS_IMETHOD_(PRBool) UseForMedium(nsIAtom* aMedium) const;
  NS_IMETHOD AppendMedium(nsIAtom* aMedium);
  NS_IMETHOD ClearMedia(void);
  NS_IMETHOD DeleteRuleFromGroup(nsICSSGroupRule* aGroup, PRUint32 aIndex);
  NS_IMETHOD InsertRuleIntoGroup(const nsAString& aRule, nsICSSGroupRule* aGroup, PRUint32 aIndex, PRUint32* _retval);
  
  NS_IMETHOD GetEnabled(PRBool& aEnabled) const;
  NS_IMETHOD SetEnabled(PRBool aEnabled);

  // style sheet owner info
  NS_IMETHOD GetParentSheet(nsIStyleSheet*& aParent) const;  // may be null
  NS_IMETHOD GetOwningDocument(nsIDocument*& aDocument) const;
  NS_IMETHOD SetOwningDocument(nsIDocument* aDocument);
  NS_IMETHOD SetOwningNode(nsIDOMNode* aOwningNode);

  NS_IMETHOD GetStyleRuleProcessor(nsIStyleRuleProcessor*& aProcessor,
                                   nsIStyleRuleProcessor* aPrevProcessor);
  NS_IMETHOD DropRuleProcessorReference(nsICSSStyleRuleProcessor* aProcessor);


  NS_IMETHOD ContainsStyleSheet(nsIURI* aURL, PRBool& aContains, nsIStyleSheet** aTheChild=nsnull);

  NS_IMETHOD AppendStyleSheet(nsICSSStyleSheet* aSheet);
  NS_IMETHOD InsertStyleSheetAt(nsICSSStyleSheet* aSheet, PRInt32 aIndex);


  // If changing the given attribute cannot affect style context, aAffects
  // will be PR_FALSE on return.
  NS_IMETHOD AttributeAffectsStyle(nsIAtom *aAttribute, nsIContent *aContent,
                                   PRBool &aAffects);

  // Find attributes in selector for rule, for use with AttributeAffectsStyle
  NS_IMETHOD CheckRuleForAttributes(nsICSSRule* aRule);

  // XXX do these belong here or are they generic?
  NS_IMETHOD PrependStyleRule(nsICSSRule* aRule);
  NS_IMETHOD AppendStyleRule(nsICSSRule* aRule);

  NS_IMETHOD  StyleRuleCount(PRInt32& aCount) const;
  NS_IMETHOD  GetStyleRuleAt(PRInt32 aIndex, nsICSSRule*& aRule) const;

  NS_IMETHOD  StyleSheetCount(PRInt32& aCount) const;
  NS_IMETHOD  GetStyleSheetAt(PRInt32 aIndex, nsICSSStyleSheet*& aSheet) const;

  NS_IMETHOD  GetNameSpace(nsINameSpace*& aNameSpace) const;
  NS_IMETHOD  SetDefaultNameSpaceID(PRInt32 aDefaultNameSpaceID);

  NS_IMETHOD Clone(nsICSSStyleSheet*& aClone) const;

  NS_IMETHOD IsModified(PRBool* aSheetModified) const;
  NS_IMETHOD SetModified(PRBool aModified);

  nsresult  EnsureUniqueInner(void);

#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;

  virtual void SizeOf(nsISizeOfHandler *aSizeofHandler, PRUint32 &aSize);
#endif

  // nsIDOMStyleSheet interface
  NS_DECL_NSIDOMSTYLESHEET

  // nsIDOMCSSStyleSheet interface
  NS_DECL_NSIDOMCSSSTYLESHEET

private: 
  // These are not supported and are not implemented! 
  CSSStyleSheetImpl(const CSSStyleSheetImpl& aCopy); 
  CSSStyleSheetImpl& operator=(const CSSStyleSheetImpl& aCopy); 

protected:
  virtual ~CSSStyleSheetImpl();

  void ClearRuleCascades(void);

  nsresult WillDirty(void);
  void     DidDirty(void);

protected:
  PRUint32 mInHeap : 1;
  PRUint32 mRefCnt : 31;
  NS_DECL_OWNINGTHREAD // for thread-safety checking

  nsString              mTitle;
  DOMMediaListImpl*     mMedia;
  CSSStyleSheetImpl*    mFirstChild;
  CSSStyleSheetImpl*    mNext;
  nsICSSStyleSheet*     mParent;

  CSSImportsCollectionImpl* mImportsCollection;
  CSSRuleListImpl*      mRuleCollection;
  nsIDocument*          mDocument;
  nsIDOMNode*           mOwningNode;
  PRBool                mDisabled;
  PRBool                mDirty; // has been modified 

  CSSStyleSheetInner*   mInner;

  nsAutoVoidArray*      mRuleProcessors;

  friend class CSSRuleProcessor;
  friend class DOMMediaListImpl;
};


// -------------------------------
// Style Rule List for the DOM
//
class CSSRuleListImpl : public nsIDOMCSSRuleList
{
public:
  CSSRuleListImpl(CSSStyleSheetImpl *aStyleSheet);

  NS_DECL_ISUPPORTS

  // nsIDOMCSSRuleList interface
  NS_IMETHOD    GetLength(PRUint32* aLength); 
  NS_IMETHOD    Item(PRUint32 aIndex, nsIDOMCSSRule** aReturn); 

  void DropReference() { mStyleSheet = nsnull; }

protected:
  virtual ~CSSRuleListImpl();

  CSSStyleSheetImpl*  mStyleSheet;
public:
  PRBool              mRulesAccessed;
};

CSSRuleListImpl::CSSRuleListImpl(CSSStyleSheetImpl *aStyleSheet)
{
  NS_INIT_REFCNT();
  // Not reference counted to avoid circular references.
  // The style sheet will tell us when its going away.
  mStyleSheet = aStyleSheet;
  mRulesAccessed = PR_FALSE;
}

CSSRuleListImpl::~CSSRuleListImpl()
{
}

// QueryInterface implementation for CSSRuleList
NS_INTERFACE_MAP_BEGIN(CSSRuleListImpl)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRuleList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(CSSRuleList)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(CSSRuleListImpl);
NS_IMPL_RELEASE(CSSRuleListImpl);


NS_IMETHODIMP    
CSSRuleListImpl::GetLength(PRUint32* aLength)
{
  if (nsnull != mStyleSheet) {
    PRInt32 count;
    mStyleSheet->StyleRuleCount(count);
    *aLength = (PRUint32)count;
  }
  else {
    *aLength = 0;
  }

  return NS_OK;
}

NS_IMETHODIMP    
CSSRuleListImpl::Item(PRUint32 aIndex, nsIDOMCSSRule** aReturn)
{
  nsresult result = NS_OK;

  *aReturn = nsnull;
  if (mStyleSheet) {
    result = mStyleSheet->EnsureUniqueInner(); // needed to ensure rules have correct parent
    if (NS_SUCCEEDED(result)) {
      nsCOMPtr<nsICSSRule> rule;

      result = mStyleSheet->GetStyleRuleAt(aIndex, *getter_AddRefs(rule));
      if (rule) {
        result = CallQueryInterface(rule, aReturn);
        mRulesAccessed = PR_TRUE; // signal to never share rules again
      } else if (result == NS_ERROR_ILLEGAL_VALUE) {
        result = NS_OK; // per spec: "Return Value ... null if ... not a valid index."
      }
    }
  }
  
  return result;
}

class DOMMediaListImpl : public nsIDOMMediaList,
                         public nsIMediaList
{
  NS_DECL_ISUPPORTS

  NS_DECL_NSIDOMMEDIALIST

  NS_FORWARD_NSISUPPORTSARRAY(mArray->)
  NS_FORWARD_NSICOLLECTION(mArray->);
  NS_FORWARD_NSISERIALIZABLE(mArray->); // XXXbe temporary

  NS_IMETHOD_(PRBool) operator==(const nsISupportsArray& other) {
    return PR_FALSE;
  }

  NS_IMETHOD_(nsISupports*)  operator[](PRUint32 aIndex) {
    return mArray->ElementAt(aIndex);
  }

  // nsIMediaList methods
  NS_DECL_NSIMEDIALIST
  
  DOMMediaListImpl(nsISupportsArray *aArray, CSSStyleSheetImpl *aStyleSheet);
  virtual ~DOMMediaListImpl();

private:
  nsresult BeginMediaChange(void);
  nsresult EndMediaChange(void);
  nsresult Delete(const nsAString & aOldMedium);
  nsresult Append(const nsAString & aOldMedium);

  nsCOMPtr<nsISupportsArray> mArray;
  // not refcounted; sheet will let us know when it goes away
  // mStyleSheet is the sheet that needs to be dirtied when this medialist
  // changes
  CSSStyleSheetImpl*         mStyleSheet;
};

// QueryInterface implementation for DOMMediaListImpl
NS_INTERFACE_MAP_BEGIN(DOMMediaListImpl)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMediaList)
  NS_INTERFACE_MAP_ENTRY(nsIMediaList)
  NS_INTERFACE_MAP_ENTRY(nsISupportsArray)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMMediaList)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(MediaList)
  NS_INTERFACE_MAP_ENTRY(nsISerializable)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(DOMMediaListImpl);
NS_IMPL_RELEASE(DOMMediaListImpl);


DOMMediaListImpl::DOMMediaListImpl(nsISupportsArray *aArray,
                                   CSSStyleSheetImpl *aStyleSheet)
  : mArray(aArray), mStyleSheet(aStyleSheet)
{
  NS_INIT_REFCNT();

  NS_ABORT_IF_FALSE(mArray, "This can't be used without an array!!");
}

DOMMediaListImpl::~DOMMediaListImpl()
{
}

nsresult
NS_NewMediaList(nsIMediaList** aInstancePtrResult) {
  return NS_NewMediaList(NS_LITERAL_STRING(""), aInstancePtrResult);
}

nsresult
NS_NewMediaList(const nsAString& aMediaText, nsIMediaList** aInstancePtrResult) {
  nsresult rv;
  NS_ASSERTION(aInstancePtrResult, "Null out param.");

  nsCOMPtr<nsISupportsArray> array;
  rv = NS_NewISupportsArray(getter_AddRefs(array));
  if (NS_FAILED(rv)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  DOMMediaListImpl* medialist = new DOMMediaListImpl(array, nsnull);
  *aInstancePtrResult = medialist;
  NS_ENSURE_TRUE(medialist, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aInstancePtrResult);
  rv = medialist->SetMediaText(aMediaText);
  if (NS_FAILED(rv)) {
    NS_RELEASE(*aInstancePtrResult);
    *aInstancePtrResult = nsnull;
  }
  return rv;
}

nsresult NS_NewMediaList(nsISupportsArray* aArray, nsICSSStyleSheet* aSheet, nsIMediaList** aInstancePtrResult)
{
  NS_ASSERTION(aInstancePtrResult, "Null out param.");
  DOMMediaListImpl* medialist = new DOMMediaListImpl(aArray, NS_STATIC_CAST(CSSStyleSheetImpl*, aSheet));
  *aInstancePtrResult = medialist;
  NS_ENSURE_TRUE(medialist, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aInstancePtrResult);
  return NS_OK;
}

NS_IMETHODIMP
DOMMediaListImpl::GetText(nsAString& aMediaText)
{
  aMediaText.Truncate();

  PRUint32 cnt;
  nsresult rv = Count(&cnt);
  if (NS_FAILED(rv)) return rv;

  PRInt32 count = cnt, index = 0;

  while (index < count) {
    nsCOMPtr<nsIAtom> medium;
    QueryElementAt(index++, NS_GET_IID(nsIAtom), getter_AddRefs(medium));
    NS_ENSURE_TRUE(medium, NS_ERROR_FAILURE);

    const PRUnichar *buffer;
    medium->GetUnicode(&buffer);
    aMediaText.Append(buffer);
    if (index < count) {
      aMediaText.Append(NS_LITERAL_STRING(", "));
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
DOMMediaListImpl::SetText(const nsAString& aMediaText)
{
  nsresult rv = Clear();
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString buf(aMediaText);
  PRInt32 n = buf.FindChar(',');

  do {
    if (n < 0)
      n = buf.Length();

    nsAutoString tmp;

    buf.Left(tmp, n);

    tmp.CompressWhitespace();

    if (!tmp.IsEmpty()) {
      rv = Append(tmp);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    buf.Cut(0, n + 1);

    n = buf.FindChar(',');
  } while (!buf.IsEmpty());

  return rv;
}

/*
 * aMatch is true when we contain the desired medium or contain the
 * "all" medium or contain no media at all, which is the same as
 * containing "all"
 */
NS_IMETHODIMP
DOMMediaListImpl::MatchesMedium(nsIAtom* aMedium, PRBool* aMatch)
{
  NS_ENSURE_ARG_POINTER(aMatch);
  *aMatch = PR_FALSE;
  *aMatch = (-1 != IndexOf(aMedium)) ||
            (-1 != IndexOf(nsLayoutAtoms::all));
  if (*aMatch)
    return NS_OK;
  PRUint32 count;
  nsresult rv = Count(&count);
  if(NS_FAILED(rv))
    return rv;  
  *aMatch = (count == 0);
  return NS_OK;
}

NS_IMETHODIMP
DOMMediaListImpl::DropReference(void)
{
  mStyleSheet = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
DOMMediaListImpl::GetMediaText(nsAString& aMediaText)
{
  return GetText(aMediaText);
}

NS_IMETHODIMP
DOMMediaListImpl::SetMediaText(const nsAString& aMediaText)
{
  nsresult rv;
  rv = BeginMediaChange();
  if (NS_FAILED(rv))
    return rv;
  
  rv = SetText(aMediaText);
  if (NS_FAILED(rv))
    return rv;
  
  rv = EndMediaChange();
  return rv;
}
                               
NS_IMETHODIMP
DOMMediaListImpl::GetLength(PRUint32* aLength)
{
  NS_ENSURE_ARG_POINTER(aLength);

  PRUint32 cnt;

  nsresult rv = Count(&cnt);
  if (NS_FAILED(rv)) return rv;

  *aLength = cnt;

  return NS_OK;
}

NS_IMETHODIMP
DOMMediaListImpl::Item(PRUint32 aIndex, nsAString& aReturn)
{
  nsCOMPtr<nsISupports> tmp(dont_AddRef(ElementAt(aIndex)));

  if (tmp) {
    nsCOMPtr<nsIAtom> medium(do_QueryInterface(tmp));
    NS_ENSURE_TRUE(medium, NS_ERROR_FAILURE);

    const PRUnichar *buffer;
    medium->GetUnicode(&buffer);
    aReturn.Assign(buffer);
  } else {
    aReturn.Truncate();
  }

  return NS_OK;
}

NS_IMETHODIMP
DOMMediaListImpl::DeleteMedium(const nsAString& aOldMedium)
{
  nsresult rv;
  rv = BeginMediaChange();
  if (NS_FAILED(rv))
    return rv;
  
  rv = Delete(aOldMedium);
  if (NS_FAILED(rv))
    return rv;

  rv = EndMediaChange();
  return rv;
}

NS_IMETHODIMP
DOMMediaListImpl::AppendMedium(const nsAString& aNewMedium)
{
  nsresult rv;
  rv = BeginMediaChange();
  if (NS_FAILED(rv))
    return rv;
  
  rv = Append(aNewMedium);
  if (NS_FAILED(rv))
    return rv;

  rv = EndMediaChange();
  return rv;
}

nsresult
DOMMediaListImpl::Delete(const nsAString& aOldMedium)
{
  if (aOldMedium.IsEmpty())
    return NS_ERROR_DOM_NOT_FOUND_ERR;

  nsCOMPtr<nsIAtom> old(dont_AddRef(NS_NewAtom(aOldMedium)));
  NS_ENSURE_TRUE(old, NS_ERROR_OUT_OF_MEMORY);

  PRInt32 indx = IndexOf(old);

  if (indx < 0) {
    return NS_ERROR_DOM_NOT_FOUND_ERR;
  }

  RemoveElementAt(indx);

  return NS_OK;
}

nsresult
DOMMediaListImpl::Append(const nsAString& aNewMedium)
{
  if (aNewMedium.IsEmpty())
    return NS_ERROR_DOM_NOT_FOUND_ERR;

  nsCOMPtr<nsIAtom> media(dont_AddRef(NS_NewAtom(aNewMedium)));
  NS_ENSURE_TRUE(media, NS_ERROR_OUT_OF_MEMORY);

  PRInt32 indx = IndexOf(media);

  if (indx >= 0) {
    RemoveElementAt(indx);
  }

  AppendElement(media);

  return NS_OK;
}

nsresult
DOMMediaListImpl::BeginMediaChange(void)
{
  nsresult rv;
  nsCOMPtr<nsIDocument> doc;

  if (mStyleSheet) {
    rv = mStyleSheet->GetOwningDocument(*getter_AddRefs(doc));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = doc->BeginUpdate();
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mStyleSheet->WillDirty();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult
DOMMediaListImpl::EndMediaChange(void)
{
  nsresult rv;
  nsCOMPtr<nsIDocument> doc;
  if (mStyleSheet) {
    mStyleSheet->DidDirty();
    rv = mStyleSheet->GetOwningDocument(*getter_AddRefs(doc));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = doc->StyleRuleChanged(mStyleSheet, nsnull, NS_STYLE_HINT_RECONSTRUCT_ALL);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = doc->EndUpdate();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

// -------------------------------
// Imports Collection for the DOM
//
class CSSImportsCollectionImpl : public nsIDOMStyleSheetList
{
public:
  CSSImportsCollectionImpl(nsICSSStyleSheet *aStyleSheet);

  NS_DECL_ISUPPORTS

  // nsIDOMCSSStyleSheetList interface
  NS_IMETHOD    GetLength(PRUint32* aLength); 
  NS_IMETHOD    Item(PRUint32 aIndex, nsIDOMStyleSheet** aReturn); 

  void DropReference() { mStyleSheet = nsnull; }

protected:
  virtual ~CSSImportsCollectionImpl();

  nsICSSStyleSheet*  mStyleSheet;
};

CSSImportsCollectionImpl::CSSImportsCollectionImpl(nsICSSStyleSheet *aStyleSheet)
{
  NS_INIT_REFCNT();
  // Not reference counted to avoid circular references.
  // The style sheet will tell us when its going away.
  mStyleSheet = aStyleSheet;
}

CSSImportsCollectionImpl::~CSSImportsCollectionImpl()
{
}


// QueryInterface implementation for CSSImportsCollectionImpl
NS_INTERFACE_MAP_BEGIN(CSSImportsCollectionImpl)
  NS_INTERFACE_MAP_ENTRY(nsIDOMStyleSheetList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(StyleSheetList)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(CSSImportsCollectionImpl);
NS_IMPL_RELEASE(CSSImportsCollectionImpl);


NS_IMETHODIMP
CSSImportsCollectionImpl::GetLength(PRUint32* aLength)
{
  if (nsnull != mStyleSheet) {
    PRInt32 count;
    mStyleSheet->StyleSheetCount(count);
    *aLength = (PRUint32)count;
  }
  else {
    *aLength = 0;
  }

  return NS_OK;
}

NS_IMETHODIMP    
CSSImportsCollectionImpl::Item(PRUint32 aIndex, nsIDOMStyleSheet** aReturn)
{
  nsresult result = NS_OK;

  *aReturn = nsnull;
  if (nsnull != mStyleSheet) {
    nsICSSStyleSheet *sheet;

    result = mStyleSheet->GetStyleSheetAt(aIndex, sheet);
    if (NS_OK == result) {
      result = sheet->QueryInterface(NS_GET_IID(nsIDOMStyleSheet), (void **)aReturn);
    }
    NS_RELEASE(sheet);
  }
  
  return result;
}

// -------------------------------
// CSS Style Sheet Inner Data Container
//


static PRBool SetStyleSheetReference(nsISupports* aElement, void* aSheet)
{
  nsICSSRule*  rule = (nsICSSRule*)aElement;

  if (nsnull != rule) {
    rule->SetStyleSheet((nsICSSStyleSheet*)aSheet);
  }
  return PR_TRUE;
}

CSSStyleSheetInner::CSSStyleSheetInner(nsICSSStyleSheet* aParentSheet)
  : mSheets(),
    mURL(nsnull),
    mOrderedRules(nsnull),
    mNameSpace(nsnull),
    mDefaultNameSpaceID(kNameSpaceID_None),
    mRelevantAttributes()
{
  MOZ_COUNT_CTOR(CSSStyleSheetInner);
  mSheets.AppendElement(aParentSheet);
}

static PRBool
CloneRuleInto(nsISupports* aRule, void* aArray)
{
  nsICSSRule* rule = (nsICSSRule*)aRule;
  nsICSSRule* clone = nsnull;
  rule->Clone(clone);
  if (clone) {
    nsISupportsArray* array = (nsISupportsArray*)aArray;
    array->AppendElement(clone);
    NS_RELEASE(clone);
  }
  return PR_TRUE;
}

static PRBool PR_CALLBACK
CopyRelevantAttributes(nsHashKey *aAttrKey, void *aAtom, void *aTable)
{
  nsHashtable *table = NS_STATIC_CAST(nsHashtable *, aTable);
  AtomKey *key =  NS_STATIC_CAST(AtomKey *, aAttrKey);
  table->Put(key, key->mAtom);
  // we don't need to addref the atom here, because we're also copying the
  // rules when we clone, and that will add a ref for us
  return PR_TRUE;
}

CSSStyleSheetInner::CSSStyleSheetInner(CSSStyleSheetInner& aCopy,
                                       nsICSSStyleSheet* aParentSheet)
  : mSheets(),
    mURL(aCopy.mURL),
    mNameSpace(nsnull),
    mDefaultNameSpaceID(aCopy.mDefaultNameSpaceID),
    mRelevantAttributes()
{
  MOZ_COUNT_CTOR(CSSStyleSheetInner);
  mSheets.AppendElement(aParentSheet);
  NS_IF_ADDREF(mURL);
  if (aCopy.mOrderedRules) {
    NS_NewISupportsArray(&mOrderedRules);
    if (mOrderedRules) {
      aCopy.mOrderedRules->EnumerateForwards(CloneRuleInto, mOrderedRules);
      mOrderedRules->EnumerateForwards(SetStyleSheetReference, aParentSheet);
    }
  }
  else {
    mOrderedRules = nsnull;
  }
  aCopy.mRelevantAttributes.Enumerate(CopyRelevantAttributes,
                                      &mRelevantAttributes);
  RebuildNameSpaces();
}

CSSStyleSheetInner::~CSSStyleSheetInner(void)
{
  MOZ_COUNT_DTOR(CSSStyleSheetInner);
  NS_IF_RELEASE(mURL);
  if (mOrderedRules) {
    mOrderedRules->EnumerateForwards(SetStyleSheetReference, nsnull);
    NS_RELEASE(mOrderedRules);
  }
}

CSSStyleSheetInner* 
CSSStyleSheetInner::CloneFor(nsICSSStyleSheet* aParentSheet)
{
  return new CSSStyleSheetInner(*this, aParentSheet);
}

void
CSSStyleSheetInner::AddSheet(nsICSSStyleSheet* aParentSheet)
{
  mSheets.AppendElement(aParentSheet);
}

void
CSSStyleSheetInner::RemoveSheet(nsICSSStyleSheet* aParentSheet)
{
  if (1 == mSheets.Count()) {
    NS_ASSERTION(aParentSheet == (nsICSSStyleSheet*)mSheets.ElementAt(0), "bad parent");
    delete this;
    return;
  }
  if (aParentSheet == (nsICSSStyleSheet*)mSheets.ElementAt(0)) {
    mSheets.RemoveElementAt(0);
    NS_ASSERTION(mSheets.Count(), "no parents");
    if (mOrderedRules) {
      mOrderedRules->EnumerateForwards(SetStyleSheetReference, 
                                       (nsICSSStyleSheet*)mSheets.ElementAt(0));
    }
  }
  else {
    mSheets.RemoveElement(aParentSheet);
  }
}

static PRBool
CreateNameSpace(nsISupports* aRule, void* aNameSpacePtr)
{
  nsICSSRule* rule = (nsICSSRule*)aRule;
  PRInt32 type = nsICSSRule::UNKNOWN_RULE;
  rule->GetType(type);
  if (nsICSSRule::NAMESPACE_RULE == type) {
    nsICSSNameSpaceRule*  nameSpaceRule = (nsICSSNameSpaceRule*)rule;
    nsINameSpace** nameSpacePtr = (nsINameSpace**)aNameSpacePtr;
    nsINameSpace* lastNameSpace = *nameSpacePtr;
    nsINameSpace* newNameSpace;

    nsIAtom*      prefix = nsnull;
    nsAutoString  urlSpec;
    nameSpaceRule->GetPrefix(prefix);
    nameSpaceRule->GetURLSpec(urlSpec);
    lastNameSpace->CreateChildNameSpace(prefix, urlSpec, newNameSpace);
    NS_IF_RELEASE(prefix);
    if (newNameSpace) {
      NS_RELEASE(lastNameSpace);
      (*nameSpacePtr) = newNameSpace; // takes ref
    }

    return PR_TRUE;
  }
  // stop if not namespace, import or charset because namespace can't follow anything else
  return (((nsICSSRule::CHARSET_RULE == type) || 
           (nsICSSRule::IMPORT_RULE)) ? PR_TRUE : PR_FALSE); 
}

void 
CSSStyleSheetInner::RebuildNameSpaces(void)
{
  nsCOMPtr<nsINameSpaceManager>  nameSpaceMgr;
  if (mNameSpace) {
    mNameSpace->GetNameSpaceManager(*getter_AddRefs(nameSpaceMgr));
  }
  else {
    NS_NewNameSpaceManager(getter_AddRefs(nameSpaceMgr));
  }
  if (nameSpaceMgr) {
    nameSpaceMgr->CreateRootNameSpace(*getter_AddRefs(mNameSpace));
    if (kNameSpaceID_Unknown != mDefaultNameSpaceID) {
      nsCOMPtr<nsINameSpace> defaultNameSpace;
      mNameSpace->CreateChildNameSpace(nsnull, mDefaultNameSpaceID,
                                       *getter_AddRefs(defaultNameSpace));
      if (defaultNameSpace) {
        mNameSpace = defaultNameSpace;
      }
    }
    if (mOrderedRules) {
      mOrderedRules->EnumerateForwards(CreateNameSpace, address_of(mNameSpace));
    }
  }
}

#ifdef DEBUG
/******************************************************************************
* SizeOf method:
*
*  Self (reported as CSSStyleSheetInner's size): 
*    1) sizeof(*this) + sizeof mSheets array (not contents though)
*       + size of the mOrderedRules array (not the contents though)
*
*  Contained / Aggregated data (not reported as CSSStyleSheetInner's size):
*    1) mSheets: each style sheet is sized seperately
*    2) mOrderedRules: each fule is sized seperately
*
*  Children / siblings / parents:
*    none
*    
******************************************************************************/
void CSSStyleSheetInner::SizeOf(nsISizeOfHandler *aSizeOfHandler, PRUint32 &aSize)
{
  NS_ASSERTION(aSizeOfHandler != nsnull, "SizeOf handler cannot be null");

  // first get the unique items collection
  UNIQUE_STYLE_ITEMS(uniqueItems);
  if(! uniqueItems->AddItem((void*)this)){
    // this style sheet is lared accounted for
    return;
  }

  PRUint32 localSize=0;
  PRBool rulesCounted=PR_FALSE;

  // create a tag for this instance
  nsCOMPtr<nsIAtom> tag;
  tag = getter_AddRefs(NS_NewAtom("CSSStyleSheetInner"));
  // get the size of an empty instance and add to the sizeof handler
  aSize = sizeof(CSSStyleSheetInner);

  // add in the size of the mSheets array itself
  mSheets.SizeOf(aSizeOfHandler,&localSize);
  aSize += localSize;

  // and the mOrderedRules array (if there is one)
  if(mOrderedRules && uniqueItems->AddItem(mOrderedRules)){
    rulesCounted=PR_TRUE;
    // no SizeOf method so we just get the basic object size
    aSize += sizeof(*mOrderedRules);
  }
  aSizeOfHandler->AddSize(tag,aSize);


  // delegate to the contained containers
  // mSheets : nsVoidArray
  {
    PRUint32 sheetCount, sheetCur;
    sheetCount = mSheets.Count();
    for(sheetCur=0; sheetCur < sheetCount; sheetCur++){
      nsICSSStyleSheet* sheet = (nsICSSStyleSheet*)mSheets.ElementAt(sheetCur);
      if(sheet){
        sheet->SizeOf(aSizeOfHandler, localSize);
      }
    }
  }
  // mOrderedRules : nsISupportsArray*
  if(mOrderedRules && rulesCounted){
    PRUint32 ruleCount, ruleCur;
    mOrderedRules->Count(&ruleCount);
    for(ruleCur=0; ruleCur < ruleCount; ruleCur++){
      nsICSSRule* rule = (nsICSSRule*)mOrderedRules->ElementAt(ruleCur);
      if(rule){
        rule->SizeOf(aSizeOfHandler, localSize);
        NS_IF_RELEASE(rule);
      }
    }
  }
}
#endif


// -------------------------------
// CSS Style Sheet
//

void* CSSStyleSheetImpl::operator new(size_t size)
{
  CSSStyleSheetImpl* rv = (CSSStyleSheetImpl*) ::operator new(size);
#ifdef NS_DEBUG
  if (nsnull != rv) {
    memset(rv, 0xEE, size);
  }
#endif
  rv->mInHeap = 1;
  return (void*) rv;
}

void* CSSStyleSheetImpl::operator new(size_t size, nsIArena* aArena)
{
  CSSStyleSheetImpl* rv = (CSSStyleSheetImpl*) aArena->Alloc(PRInt32(size));
#ifdef NS_DEBUG
  if (nsnull != rv) {
    memset(rv, 0xEE, size);
  }
#endif
  rv->mInHeap = 0;
  return (void*) rv;
}

void CSSStyleSheetImpl::operator delete(void* ptr)
{
  CSSStyleSheetImpl* sheet = (CSSStyleSheetImpl*) ptr;
  if (nsnull != sheet) {
    if (sheet->mInHeap) {
      ::operator delete(ptr);
    }
  }
}

MOZ_DECL_CTOR_COUNTER(CSSStyleSheetImpl)

CSSStyleSheetImpl::CSSStyleSheetImpl()
  : nsICSSStyleSheet(),
    mTitle(), 
    mMedia(nsnull),
    mFirstChild(nsnull), 
    mNext(nsnull),
    mParent(nsnull),
    mImportsCollection(nsnull),
    mRuleCollection(nsnull),
    mDocument(nsnull),
    mOwningNode(nsnull),
    mDisabled(PR_FALSE),
    mDirty(PR_FALSE),
    mRuleProcessors(nsnull)
{
  NS_INIT_REFCNT();

  mInner = new CSSStyleSheetInner(this);
}

CSSStyleSheetImpl::CSSStyleSheetImpl(const CSSStyleSheetImpl& aCopy)
  : nsICSSStyleSheet(),
    mTitle(aCopy.mTitle), 
    mMedia(nsnull),
    mFirstChild(nsnull), 
    mNext(nsnull),
    mParent(aCopy.mParent),
    mImportsCollection(nsnull), // re-created lazily
    mRuleCollection(nsnull), // re-created lazily
    mDocument(aCopy.mDocument),
    mOwningNode(aCopy.mOwningNode),
    mDisabled(aCopy.mDisabled),
    mDirty(PR_FALSE),
    mInner(aCopy.mInner),
    mRuleProcessors(nsnull)
{
  NS_INIT_REFCNT();

  mInner->AddSheet(this);

  if (aCopy.mRuleCollection && 
      aCopy.mRuleCollection->mRulesAccessed) {  // CSSOM's been there, force full copy now
    EnsureUniqueInner();
  }

  if (aCopy.mMedia) {
    nsCOMPtr<nsISupportsArray> tmp;
    (NS_STATIC_CAST(nsISupportsArray *, aCopy.mMedia))->Clone(getter_AddRefs(tmp));
    mMedia = new DOMMediaListImpl(tmp, this);
    NS_IF_ADDREF(mMedia);
  }

  if (aCopy.mFirstChild) {
    CSSStyleSheetImpl*  otherChild = aCopy.mFirstChild;
    CSSStyleSheetImpl** ourSlot = &mFirstChild;
    do {
      CSSStyleSheetImpl* child = new CSSStyleSheetImpl(*otherChild);
      if (child) {
        NS_ADDREF(child);
        (*ourSlot) = child;
        ourSlot = &(child->mNext);
      }
      otherChild = otherChild->mNext;
    }
    while (otherChild && ourSlot);
  }
}

CSSStyleSheetImpl::~CSSStyleSheetImpl()
{
  if (mFirstChild) {
    CSSStyleSheetImpl* child = mFirstChild;
    do {
      child->mParent = nsnull;
      child = child->mNext;
    } while (child);
    NS_RELEASE(mFirstChild);
  }
  NS_IF_RELEASE(mNext);
  if (nsnull != mRuleCollection) {
    mRuleCollection->DropReference();
    NS_RELEASE(mRuleCollection);
  }
  if (nsnull != mImportsCollection) {
    mImportsCollection->DropReference();
    NS_RELEASE(mImportsCollection);
  }
  if (mMedia) {
    mMedia->DropReference();
    NS_RELEASE(mMedia);
  }
  mInner->RemoveSheet(this);
  // XXX The document reference is not reference counted and should
  // not be released. The document will let us know when it is going
  // away.
  if (mRuleProcessors) {
    NS_ASSERTION(mRuleProcessors->Count() == 0, "destucting sheet with rule processor reference");
    delete mRuleProcessors; // weak refs, should be empty here anyway
  }
}


// QueryInterface implementation for CSSStyleSheetImpl
NS_INTERFACE_MAP_BEGIN(CSSStyleSheetImpl)
  NS_INTERFACE_MAP_ENTRY(nsICSSStyleSheet)
  NS_INTERFACE_MAP_ENTRY(nsIStyleSheet)
  NS_INTERFACE_MAP_ENTRY(nsIDOMStyleSheet)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSStyleSheet)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsICSSStyleSheet)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(CSSStyleSheet)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(CSSStyleSheetImpl)
NS_IMPL_RELEASE(CSSStyleSheetImpl)


NS_IMETHODIMP
CSSStyleSheetImpl::GetStyleRuleProcessor(nsIStyleRuleProcessor*& aProcessor,
                                         nsIStyleRuleProcessor* aPrevProcessor)
{
  nsresult  result = NS_OK;
  nsICSSStyleRuleProcessor*  cssProcessor = nsnull;

  if (aPrevProcessor) {
    result = aPrevProcessor->QueryInterface(NS_GET_IID(nsICSSStyleRuleProcessor), (void**)&cssProcessor);
  }
  if (! cssProcessor) {
    CSSRuleProcessor* processor = new CSSRuleProcessor();
    if (processor) {
      result = processor->QueryInterface(NS_GET_IID(nsICSSStyleRuleProcessor), (void**)&cssProcessor);
      if (NS_FAILED(result)) {
        delete processor;
        cssProcessor = nsnull;
      }
    }
    else {
      result = NS_ERROR_OUT_OF_MEMORY;
    }
  }
  if (NS_SUCCEEDED(result) && cssProcessor) {
    cssProcessor->AppendStyleSheet(this);
    if (! mRuleProcessors) {
      mRuleProcessors = new nsAutoVoidArray();
    }
    if (mRuleProcessors) {
      NS_ASSERTION(-1 == mRuleProcessors->IndexOf(cssProcessor), "processor already registered");
      mRuleProcessors->AppendElement(cssProcessor); // weak ref
    }
  }
  aProcessor = cssProcessor;
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::DropRuleProcessorReference(nsICSSStyleRuleProcessor* aProcessor)
{
  NS_ASSERTION(mRuleProcessors, "no rule processors registered");
  if (mRuleProcessors) {
    NS_ASSERTION(-1 != mRuleProcessors->IndexOf(aProcessor), "not a registered processor");
    mRuleProcessors->RemoveElement(aProcessor);
  }
  return NS_OK;
}





NS_IMETHODIMP
CSSStyleSheetImpl::Init(nsIURI* aURL)
{
  NS_PRECONDITION(aURL, "null ptr");
  if (! aURL)
    return NS_ERROR_NULL_POINTER;

  if (! mInner) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ASSERTION(! mInner->mURL, "already initialized");
  if (mInner->mURL)
    return NS_ERROR_ALREADY_INITIALIZED;

  if (mInner->mURL) {
#ifdef DEBUG
    PRBool eq;
    nsresult rv = mInner->mURL->Equals(aURL, &eq);
    NS_ASSERTION(NS_SUCCEEDED(rv) && eq, "bad inner");
#endif
  }
  else {
    mInner->mURL = aURL;
    NS_ADDREF(mInner->mURL);
  }
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::GetURL(nsIURI*& aURL) const
{
  const nsIURI* url = ((mInner) ? mInner->mURL : nsnull);
  aURL = (nsIURI*)url;
  NS_IF_ADDREF(aURL);
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::SetTitle(const nsString& aTitle)
{
  mTitle = aTitle;
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::GetType(nsString& aType) const
{
  aType.Assign(NS_LITERAL_STRING("text/css"));
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::GetMediumCount(PRInt32& aCount) const
{
  if (mMedia) {
    PRUint32 cnt;
    nsresult rv = mMedia->Count(&cnt);
    if (NS_FAILED(rv)) return rv;
    aCount = cnt;
  }
  else
    aCount = 0;
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::GetMediumAt(PRInt32 aIndex, nsIAtom*& aMedium) const
{
  nsIAtom* medium = nsnull;
  if (nsnull != mMedia) {
    medium = (nsIAtom*)mMedia->ElementAt(aIndex);
  }
  if (nsnull != medium) {
    aMedium = medium;
    return NS_OK;
  }
  aMedium = nsnull;
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP_(PRBool)
CSSStyleSheetImpl::UseForMedium(nsIAtom* aMedium) const
{
  if (mMedia) {
    PRBool matches = PR_FALSE;
    mMedia->MatchesMedium(aMedium, &matches);
    return matches;
  }
  return PR_TRUE;
}



NS_IMETHODIMP
CSSStyleSheetImpl::AppendMedium(nsIAtom* aMedium)
{
  nsresult result = NS_OK;
  if (!mMedia) {
    nsCOMPtr<nsISupportsArray> tmp;
    result = NS_NewISupportsArray(getter_AddRefs(tmp));
    NS_ENSURE_SUCCESS(result, result);

    mMedia = new DOMMediaListImpl(tmp, this);
    NS_ENSURE_TRUE(mMedia, NS_ERROR_OUT_OF_MEMORY);

    NS_ADDREF(mMedia);
  }

  if (mMedia) {
    mMedia->AppendElement(aMedium);
  }
  return result;
}

NS_IMETHODIMP
CSSStyleSheetImpl::ClearMedia(void)
{
  if (mMedia) {
    mMedia->Clear();
  }
  return NS_OK;
}


NS_IMETHODIMP
CSSStyleSheetImpl::GetEnabled(PRBool& aEnabled) const
{
  aEnabled = ((PR_TRUE == mDisabled) ? PR_FALSE : PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::SetEnabled(PRBool aEnabled)
{
  PRBool oldState = mDisabled;
  mDisabled = ((PR_TRUE == aEnabled) ? PR_FALSE : PR_TRUE);

  if ((nsnull != mDocument) && (mDisabled != oldState)) {
    mDocument->SetStyleSheetDisabledState(this, mDisabled);
  }

  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::GetParentSheet(nsIStyleSheet*& aParent) const
{
  aParent = mParent;
  NS_IF_ADDREF(aParent);
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::GetOwningDocument(nsIDocument*& aDocument) const
{
  nsIDocument*  doc = mDocument;
  CSSStyleSheetImpl* parent = (CSSStyleSheetImpl*)mParent;
  while ((nsnull == doc) && (nsnull != parent)) {
    doc = parent->mDocument;
    parent = (CSSStyleSheetImpl*)(parent->mParent);
  }

  NS_IF_ADDREF(doc);
  aDocument = doc;
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::SetOwningDocument(nsIDocument* aDocument)
{ // not ref counted
  mDocument = aDocument;
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::SetOwningNode(nsIDOMNode* aOwningNode)
{ // not ref counted
  mOwningNode = aOwningNode;
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::ContainsStyleSheet(nsIURI* aURL, PRBool& aContains, nsIStyleSheet** aTheChild /*=nsnull*/)
{
  NS_PRECONDITION(nsnull != aURL, "null arg");

  // first check ourself out
  nsresult rv = mInner->mURL->Equals(aURL, &aContains);
  if (NS_FAILED(rv)) aContains = PR_FALSE;

  if (aContains) {
    // if we found it and the out-param is there, set it and addref
    if (aTheChild) {
      rv = QueryInterface( NS_GET_IID(nsIStyleSheet), (void **)aTheChild);
    }
  } else {
    CSSStyleSheetImpl*  child = mFirstChild;
    // now check the chil'ins out (recursively)
    while ((PR_FALSE == aContains) && (nsnull != child)) {
      child->ContainsStyleSheet(aURL, aContains, aTheChild);
      if (aContains) {
        break;
      } else {
        child = child->mNext;
      }
    }
  }

  // NOTE: if there are errors in the above we are handling them locally 
  //       and not promoting them to the caller
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::AppendStyleSheet(nsICSSStyleSheet* aSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");

  if (NS_SUCCEEDED(WillDirty())) {
    NS_ADDREF(aSheet);
    CSSStyleSheetImpl* sheet = (CSSStyleSheetImpl*)aSheet;

    if (! mFirstChild) {
      mFirstChild = sheet;
    }
    else {
      CSSStyleSheetImpl* child = mFirstChild;
      while (child->mNext) {
        child = child->mNext;
      }
      child->mNext = sheet;
    }
  
    // This is not reference counted. Our parent tells us when
    // it's going away.
    sheet->mParent = this;
    DidDirty();
  }
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::InsertStyleSheetAt(nsICSSStyleSheet* aSheet, PRInt32 aIndex)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");

  nsresult result = WillDirty();

  if (NS_SUCCEEDED(result)) {
    NS_ADDREF(aSheet);
    CSSStyleSheetImpl* sheet = (CSSStyleSheetImpl*)aSheet;
    CSSStyleSheetImpl* child = mFirstChild;

    if (aIndex && child) {
      while ((0 < --aIndex) && child->mNext) {
        child = child->mNext;
      }
      sheet->mNext = child->mNext;
      child->mNext = sheet;
    }
    else {
      sheet->mNext = mFirstChild;
      mFirstChild = sheet; 
    }

    // This is not reference counted. Our parent tells us when
    // it's going away.
    sheet->mParent = this;
    DidDirty();
  }
  return result;
}

NS_IMETHODIMP
CSSStyleSheetImpl::AttributeAffectsStyle(nsIAtom *aAttribute,
                                         nsIContent *aContent,
                                         PRBool &aAffects)
{
  DependentAtomKey key(aAttribute);
  aAffects = !!mInner->mRelevantAttributes.Get(&key);
  for (CSSStyleSheetImpl *child = mFirstChild;
       child && !aAffects;
       child = child->mNext) {
    child->AttributeAffectsStyle(aAttribute, aContent, aAffects);
  }
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::PrependStyleRule(nsICSSRule* aRule)
{
  NS_PRECONDITION(nsnull != aRule, "null arg");

  if (NS_SUCCEEDED(WillDirty())) {
    if (! mInner->mOrderedRules) {
      NS_NewISupportsArray(&(mInner->mOrderedRules));
    }
    if (mInner->mOrderedRules) {
      mInner->mOrderedRules->InsertElementAt(aRule, 0);
      aRule->SetStyleSheet(this);
      DidDirty();

      PRInt32 type = nsICSSRule::UNKNOWN_RULE;
      aRule->GetType(type);
      if (nsICSSRule::NAMESPACE_RULE == type) {
        // no api to prepend a namespace (ugh), release old ones and re-create them all
        mInner->RebuildNameSpaces();
      } else {
        CheckRuleForAttributes(aRule);
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::AppendStyleRule(nsICSSRule* aRule)
{
  NS_PRECONDITION(nsnull != aRule, "null arg");

  if (NS_SUCCEEDED(WillDirty())) {
    if (! mInner->mOrderedRules) {
      NS_NewISupportsArray(&(mInner->mOrderedRules));
    }
    if (mInner->mOrderedRules) {
      mInner->mOrderedRules->AppendElement(aRule);
      aRule->SetStyleSheet(this);
      DidDirty();

      PRInt32 type = nsICSSRule::UNKNOWN_RULE;
      aRule->GetType(type);
      if (nsICSSRule::NAMESPACE_RULE == type) {
        if (! mInner->mNameSpace) {
          nsCOMPtr<nsINameSpaceManager>  nameSpaceMgr;
          NS_NewNameSpaceManager(getter_AddRefs(nameSpaceMgr));
          if (nameSpaceMgr) {
            nameSpaceMgr->CreateRootNameSpace(*getter_AddRefs(mInner->mNameSpace));
          }
        }

        if (mInner->mNameSpace) {
          nsCOMPtr<nsICSSNameSpaceRule> nameSpaceRule(do_QueryInterface(aRule));
          nsCOMPtr<nsINameSpace> newNameSpace;

          nsCOMPtr<nsIAtom> prefix;
          nsAutoString  urlSpec;
          nameSpaceRule->GetPrefix(*getter_AddRefs(prefix));
          nameSpaceRule->GetURLSpec(urlSpec);
          mInner->mNameSpace->CreateChildNameSpace(prefix, urlSpec,
                                                   *getter_AddRefs(newNameSpace));
          if (newNameSpace) {
            mInner->mNameSpace = newNameSpace;
          }
        }
      } else {
        CheckRuleForAttributes(aRule);
      }
        
    }
  }
  return NS_OK;
}

static PRBool
CheckRuleForAttributesEnum(nsISupports *aRule, void *aData)
{
  nsICSSRule *rule = NS_STATIC_CAST(nsICSSRule *, aRule);
  CSSStyleSheetImpl *sheet = NS_STATIC_CAST(CSSStyleSheetImpl *, aData);
  return NS_SUCCEEDED(sheet->CheckRuleForAttributes(rule));
}

NS_IMETHODIMP
CSSStyleSheetImpl::CheckRuleForAttributes(nsICSSRule *aRule)
{
  PRInt32 ruleType = nsICSSRule::UNKNOWN_RULE;
  aRule->GetType(ruleType);
  switch (ruleType) {
    case nsICSSRule::MEDIA_RULE: {
      nsICSSMediaRule *mediaRule = (nsICSSMediaRule *)aRule;
      return mediaRule->EnumerateRulesForwards(CheckRuleForAttributesEnum,
                                               (void *)this);
    }
    case nsICSSRule::STYLE_RULE: {
      nsICSSStyleRule *styleRule = NS_STATIC_CAST(nsICSSStyleRule *, aRule);
      nsCSSSelector *iter;
      for (iter = styleRule->FirstSelector(); iter; iter = iter->mNext) {
        /* P.classname means we have to check the attribute "class" */
        if (iter->mIDList) {
          DependentAtomKey idKey(nsHTMLAtoms::id);
          mInner->mRelevantAttributes.Put(&idKey, nsHTMLAtoms::id);
        }
        if (iter->mClassList) {
          DependentAtomKey classKey(nsHTMLAtoms::kClass);
          mInner->mRelevantAttributes.Put(&classKey, nsHTMLAtoms::kClass);
        }
        for (nsAttrSelector *sel = iter->mAttrList; sel; sel = sel->mNext) {
          /* store it in this sheet's attributes-that-matter table */
          /* XXX store tag name too, but handle collisions */
#ifdef DEBUG_shaver_off
          nsAutoString str;
          sel->mAttr->ToString(str);
          char * chars = ToNewCString(str);
          fprintf(stderr, "[%s@%p]", chars, this);
          nsMemory::Free(chars);
#endif
          DependentAtomKey key(sel->mAttr);
          mInner->mRelevantAttributes.Put(&key, sel->mAttr);
        }
      }
    } /* fall-through */
    default:
      return NS_OK;
  }
}

NS_IMETHODIMP
CSSStyleSheetImpl::StyleRuleCount(PRInt32& aCount) const
{
  aCount = 0;
  if (mInner && mInner->mOrderedRules) {
    PRUint32 cnt;
    nsresult rv = ((CSSStyleSheetImpl*)this)->mInner->mOrderedRules->Count(&cnt);      // XXX bogus cast -- this method should not be const
    aCount = (PRInt32)cnt;
    return rv;
  }
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::GetStyleRuleAt(PRInt32 aIndex, nsICSSRule*& aRule) const
{
  nsresult result = NS_ERROR_ILLEGAL_VALUE;

  if (mInner && mInner->mOrderedRules) {
    aRule = (nsICSSRule*)(mInner->mOrderedRules->ElementAt(aIndex));
    if (nsnull != aRule) {
      result = NS_OK;
    }
  }
  else {
    aRule = nsnull;
  }
  return result;
}

NS_IMETHODIMP
CSSStyleSheetImpl::GetNameSpace(nsINameSpace*& aNameSpace) const
{
  if (mInner) {
    aNameSpace = mInner->mNameSpace;
    NS_IF_ADDREF(aNameSpace);
  }
  else {
    aNameSpace = nsnull;
  }
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::SetDefaultNameSpaceID(PRInt32 aDefaultNameSpaceID)
{
  if (mInner) {
    mInner->mDefaultNameSpaceID = aDefaultNameSpaceID;
    mInner->RebuildNameSpaces();
  }
  return NS_OK;
}


NS_IMETHODIMP
CSSStyleSheetImpl::StyleSheetCount(PRInt32& aCount) const
{
  // XXX Far from an ideal way to do this, but the hope is that
  // it won't be done too often. If it is, we might want to 
  // consider storing the children in an array.
  aCount = 0;

  const CSSStyleSheetImpl* child = mFirstChild;
  while (child) {
    aCount++;
    child = child->mNext;
  }

  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::GetStyleSheetAt(PRInt32 aIndex, nsICSSStyleSheet*& aSheet) const
{
  // XXX Ughh...an O(n^2) method for doing iteration. Again, we hope
  // that this isn't done too often. If it is, we need to change the
  // underlying storage mechanism
  aSheet = nsnull;

  if (mFirstChild) {
    const CSSStyleSheetImpl* child = mFirstChild;
    while ((child) && (0 != aIndex)) {
      --aIndex;
      child = child->mNext;
    }
    
    aSheet = (nsICSSStyleSheet*)child;
    NS_IF_ADDREF(aSheet);
  }

  return NS_OK;
}

nsresult  
CSSStyleSheetImpl::EnsureUniqueInner(void)
{
  if (! mInner) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  if (1 < mInner->mSheets.Count()) {  
    CSSStyleSheetInner* clone = mInner->CloneFor(this);
    if (clone) {
      mInner->RemoveSheet(this);
      mInner = clone;
    }
    else {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::Clone(nsICSSStyleSheet*& aClone) const
{
  // XXX no, really need to clone
  CSSStyleSheetImpl* clone = new CSSStyleSheetImpl(*this);
  if (clone) {
    aClone = (nsICSSStyleSheet*)clone;
    NS_ADDREF(aClone);
  }
  return NS_OK;
}

#ifdef DEBUG
static void
ListRules(nsISupportsArray* aRules, FILE* aOut, PRInt32 aIndent)
{
  PRUint32 count;
  PRUint32 index;
  if (aRules) {
    aRules->Count(&count);
    for (index = 0; index < count; index++) {
      nsCOMPtr<nsICSSRule> rule = dont_AddRef((nsICSSRule*)aRules->ElementAt(index));
      rule->List(aOut, aIndent);
    }
  }
}

struct ListEnumData {
  ListEnumData(FILE* aOut, PRInt32 aIndent)
    : mOut(aOut),
      mIndent(aIndent)
  {
  }
  FILE*   mOut;
  PRInt32 mIndent;
};

#if 0
static PRBool ListCascade(nsHashKey* aKey, void* aValue, void* aClosure)
{
  AtomKey* key = (AtomKey*)aKey;
  RuleCascadeData* cascade = (RuleCascadeData*)aValue;
  ListEnumData* data = (ListEnumData*)aClosure;

  fputs("\nRules in cascade order for medium: \"", data->mOut);
  nsAutoString  buffer;
  key->mAtom->ToString(buffer);
  fputs(NS_LossyConvertUCS2toASCII(buffer).get(), data->mOut);
  fputs("\"\n", data->mOut);

  ListRules(cascade->mWeightedRules, data->mOut, data->mIndent);
  return PR_TRUE;
}
#endif


void CSSStyleSheetImpl::List(FILE* out, PRInt32 aIndent) const
{

  PRInt32 index;

  // Indent
  for (index = aIndent; --index >= 0; ) fputs("  ", out);

  if (! mInner) {
    fputs("CSS Style Sheet - without inner data storage - ERROR\n", out);
    return;
  }

  fputs("CSS Style Sheet: ", out);
  nsCAutoString urlSpec;
  nsresult rv = mInner->mURL->GetSpec(urlSpec);
  if (NS_SUCCEEDED(rv) && !urlSpec.IsEmpty()) {
    fputs(urlSpec.get(), out);
  }

  if (mMedia) {
    fputs(" media: ", out);
    index = 0;
    PRUint32 count;
    mMedia->Count(&count);
    nsAutoString  buffer;
    while (index < PRInt32(count)) {
      nsCOMPtr<nsIAtom> medium = dont_AddRef((nsIAtom*)mMedia->ElementAt(index++));
      medium->ToString(buffer);
      fputs(NS_LossyConvertUCS2toASCII(buffer).get(), out);
      if (index < PRInt32(count)) {
        fputs(", ", out);
      }
    }
  }
  fputs("\n", out);

  const CSSStyleSheetImpl*  child = mFirstChild;
  while (nsnull != child) {
    child->List(out, aIndent + 1);
    child = child->mNext;
  }

  fputs("Rules in source order:\n", out);
  ListRules(mInner->mOrderedRules, out, aIndent);
}

/******************************************************************************
* SizeOf method:
*
*  Self (reported as CSSStyleSheetImpl's size): 
*    1) sizeof(*this) + sizeof the mImportsCollection + sizeof mCuleCollection)
*
*  Contained / Aggregated data (not reported as CSSStyleSheetImpl's size):
*    1) mInner is delegated to be counted seperately
*
*  Children / siblings / parents:
*    1) Recurse to mFirstChild
*    
******************************************************************************/
void CSSStyleSheetImpl::SizeOf(nsISizeOfHandler *aSizeOfHandler, PRUint32 &aSize)
{
  NS_ASSERTION(aSizeOfHandler != nsnull, "SizeOf handler cannot be null");

  // first get the unique items collection
  UNIQUE_STYLE_ITEMS(uniqueItems);
  if(! uniqueItems->AddItem((void*)this)){
    // this style sheet is already accounted for
    return;
  }

  PRUint32 localSize=0;

  // create a tag for this instance
  nsCOMPtr<nsIAtom> tag;
  tag = getter_AddRefs(NS_NewAtom("CSSStyleSheet"));
  // get the size of an empty instance and add to the sizeof handler
  aSize = sizeof(CSSStyleSheetImpl);

  // add up the contained objects we won't delegate to: 
  // NOTE that we just add the sizeof the objects
  // since the style data they contain is accounted for elsewhere
  // - mImportsCollection
  // - mRuleCollection
  aSize += sizeof(mImportsCollection);
  aSize += sizeof(mRuleCollection);
  aSizeOfHandler->AddSize(tag,aSize);

  // size the inner
  if(mInner){
    mInner->SizeOf(aSizeOfHandler, localSize);
  }

  // now travers the children (recursively, I'm sorry to say)
  if(mFirstChild){
    PRUint32 childSize=0;
    mFirstChild->SizeOf(aSizeOfHandler, childSize);
  }
}
#endif

static PRBool PR_CALLBACK
EnumClearRuleCascades(void* aProcessor, void* aData)
{
  nsICSSStyleRuleProcessor* processor = (nsICSSStyleRuleProcessor*)aProcessor;
  processor->ClearRuleCascades();
  return PR_TRUE;
}

void 
CSSStyleSheetImpl::ClearRuleCascades(void)
{
  if (mRuleProcessors) {
    mRuleProcessors->EnumerateForwards(EnumClearRuleCascades, nsnull);
  }
  if (mParent) {
    CSSStyleSheetImpl* parent = (CSSStyleSheetImpl*)mParent;
    parent->ClearRuleCascades();
  }
}

nsresult
CSSStyleSheetImpl::WillDirty(void)
{
  return EnsureUniqueInner();
}

void
CSSStyleSheetImpl::DidDirty(void)
{
  ClearRuleCascades();
  mDirty = PR_TRUE;
}

NS_IMETHODIMP 
CSSStyleSheetImpl::IsModified(PRBool* aSheetModified) const
{
  *aSheetModified = mDirty;
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::SetModified(PRBool aModified)
{
  mDirty = aModified;
  return NS_OK;
}

  // nsIDOMStyleSheet interface
NS_IMETHODIMP    
CSSStyleSheetImpl::GetType(nsAString& aType)
{
  aType.Assign(NS_LITERAL_STRING("text/css"));
  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleSheetImpl::GetDisabled(PRBool* aDisabled)
{
  *aDisabled = mDisabled;
  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleSheetImpl::SetDisabled(PRBool aDisabled)
{
  PRBool oldState = mDisabled;
  mDisabled = aDisabled;

  if ((nsnull != mDocument) && (mDisabled != oldState)) {
    mDocument->SetStyleSheetDisabledState(this, mDisabled);
  }

  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::GetOwnerNode(nsIDOMNode** aOwnerNode)
{
  *aOwnerNode = mOwningNode;
  NS_IF_ADDREF(*aOwnerNode);
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::GetParentStyleSheet(nsIDOMStyleSheet** aParentStyleSheet)
{
  NS_ENSURE_ARG_POINTER(aParentStyleSheet);

  nsresult rv = NS_OK;

  if (mParent) {
    rv =  mParent->QueryInterface(NS_GET_IID(nsIDOMStyleSheet),
                                  (void **)aParentStyleSheet);
  } else {
    *aParentStyleSheet = nsnull;
  }

  return rv;
}

NS_IMETHODIMP
CSSStyleSheetImpl::GetHref(nsAString& aHref)
{
  if (mInner && mInner->mURL) {
    nsCAutoString str;
    mInner->mURL->GetSpec(str);
    aHref.Assign(NS_ConvertUTF8toUCS2(str));
  }
  else {
    aHref.Truncate();
  }

  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::GetTitle(nsString& aTitle) const
{
  aTitle = mTitle;
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::GetTitle(nsAString& aTitle)
{
  aTitle.Assign(mTitle);
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::GetMedia(nsIDOMMediaList** aMedia)
{
  NS_ENSURE_ARG_POINTER(aMedia);
  *aMedia = nsnull;

  if (!mMedia) {
    nsCOMPtr<nsISupportsArray> tmp;
    NS_NewISupportsArray(getter_AddRefs(tmp));
    NS_ENSURE_TRUE(tmp, NS_ERROR_NULL_POINTER);

    mMedia = new DOMMediaListImpl(tmp, this);
    NS_IF_ADDREF(mMedia);
  }

  *aMedia = mMedia;
  NS_IF_ADDREF(*aMedia);

  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleSheetImpl::GetOwnerRule(nsIDOMCSSRule** aOwnerRule)
{
  NS_ENSURE_ARG_POINTER(aOwnerRule);
  *aOwnerRule = nsnull;

  // TBI: This should return the owner rule once the style system knows about
  // owning rules...

  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleSheetImpl::GetCssRules(nsIDOMCSSRuleList** aCssRules)
{
  if (nsnull == mRuleCollection) {
    mRuleCollection = new CSSRuleListImpl(this);
    if (nsnull == mRuleCollection) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mRuleCollection);
  }

  *aCssRules = mRuleCollection;
  NS_ADDREF(mRuleCollection);

  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleSheetImpl::InsertRule(const nsAString& aRule, 
                              PRUint32 aIndex, 
                              PRUint32* aReturn)
{
  NS_ENSURE_TRUE(mInner, NS_ERROR_FAILURE);
  nsresult result;
  result = WillDirty();
  if (NS_FAILED(result))
    return result;
  
  if (! mInner->mOrderedRules) {
    result = NS_NewISupportsArray(&(mInner->mOrderedRules));
  }
  if (NS_FAILED(result))
    return result;

  PRUint32 count;
  mInner->mOrderedRules->Count(&count);
  if (aIndex > count)
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  
  nsCOMPtr<nsICSSLoader> loader;
  nsCOMPtr<nsICSSParser> css;
  nsCOMPtr<nsIHTMLContentContainer> htmlContainer(do_QueryInterface(mDocument));
  if (htmlContainer) {
    htmlContainer->GetCSSLoader(*getter_AddRefs(loader));
  }
  NS_ASSERTION(loader || !mDocument, "Document with no CSS loader!");
  if (loader) {
    result = loader->GetParserFor(this, getter_AddRefs(css));
  }
  else {
    result = NS_NewCSSParser(getter_AddRefs(css));
    if (css) {
      css->SetStyleSheet(this);
    }
  }
  if (NS_FAILED(result))
    return result;

  if (mDocument) {
    result = mDocument->BeginUpdate();
    if (NS_FAILED(result))
      return result;
  }

  nsCOMPtr<nsISupportsArray> rules;
  result = css->ParseRule(aRule, mInner->mURL, getter_AddRefs(rules));
  if (NS_FAILED(result))
    return result;
  
  PRUint32 rulecount = 0;
  rules->Count(&rulecount);  
  if (rulecount == 0 && !aRule.IsEmpty()) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }
  
  // Hierarchy checking.  Just check the first and last rule in the list.
  
  // check that we're not inserting before a charset rule
  nsCOMPtr<nsICSSRule> nextRule;
  PRInt32 nextType = nsICSSRule::UNKNOWN_RULE;
  nextRule = dont_AddRef((nsICSSRule*)mInner->mOrderedRules->ElementAt(aIndex));
  if (nextRule) {
    nextRule->GetType(nextType);
    if (nextType == nsICSSRule::CHARSET_RULE) {
      return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
    }

    // check last rule in list
    nsCOMPtr<nsICSSRule> lastRule = dont_AddRef((nsICSSRule*)rules->ElementAt(rulecount-1));
    PRInt32 lastType = nsICSSRule::UNKNOWN_RULE;
    lastRule->GetType(lastType);
    
    if (nextType == nsICSSRule::IMPORT_RULE &&
        lastType != nsICSSRule::CHARSET_RULE &&
        lastType != nsICSSRule::IMPORT_RULE) {
      return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
    }
    
    if (nextType == nsICSSRule::NAMESPACE_RULE &&
        lastType != nsICSSRule::CHARSET_RULE &&
        lastType != nsICSSRule::IMPORT_RULE &&
        lastType != nsICSSRule::NAMESPACE_RULE) {
      return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
    } 
  }
  
  // check first rule in list
  nsCOMPtr<nsICSSRule> firstRule = dont_AddRef((nsICSSRule*)rules->ElementAt(0));
  PRInt32 firstType = nsICSSRule::UNKNOWN_RULE;
  firstRule->GetType(firstType);
  if (aIndex != 0) {
    if (firstType == nsICSSRule::CHARSET_RULE) { // no inserting charset at nonzero position
      return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
    }
  
    nsCOMPtr<nsICSSRule> prevRule = dont_AddRef((nsICSSRule*)mInner->mOrderedRules->ElementAt(aIndex-1));
    PRInt32 prevType = nsICSSRule::UNKNOWN_RULE;
    prevRule->GetType(prevType);

    if (firstType == nsICSSRule::IMPORT_RULE &&
        prevType != nsICSSRule::CHARSET_RULE &&
        prevType != nsICSSRule::IMPORT_RULE) {
      return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
    }

    if (firstType == nsICSSRule::NAMESPACE_RULE &&
        prevType != nsICSSRule::CHARSET_RULE &&
        prevType != nsICSSRule::IMPORT_RULE &&
        prevType != nsICSSRule::NAMESPACE_RULE) {
      return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
    }
  }
  
  result = mInner->mOrderedRules->InsertElementsAt(rules, aIndex);
  NS_ENSURE_SUCCESS(result, result);
  DidDirty();

  nsCOMPtr<nsICSSRule> cssRule;
  PRUint32 counter;
  for (counter = 0; counter < rulecount; counter++) {
    cssRule = dont_AddRef((nsICSSRule*)rules->ElementAt(counter));
    cssRule->SetStyleSheet(this);
    
    PRInt32 type = nsICSSRule::UNKNOWN_RULE;
    cssRule->GetType(type);
    if (type == nsICSSRule::NAMESPACE_RULE) {
      if (! mInner->mNameSpace) {
        nsCOMPtr<nsINameSpaceManager>  nameSpaceMgr;
        result = NS_NewNameSpaceManager(getter_AddRefs(nameSpaceMgr));
        if (NS_FAILED(result))
          return result;
        nameSpaceMgr->CreateRootNameSpace(*getter_AddRefs(mInner->mNameSpace));
      }

      NS_ENSURE_TRUE(mInner->mNameSpace, NS_ERROR_FAILURE);

      nsCOMPtr<nsICSSNameSpaceRule> nameSpaceRule(do_QueryInterface(cssRule));
      nsCOMPtr<nsINameSpace> newNameSpace;
    
      nsCOMPtr<nsIAtom> prefix;
      nsAutoString urlSpec;
      nameSpaceRule->GetPrefix(*getter_AddRefs(prefix));
      nameSpaceRule->GetURLSpec(urlSpec);
      mInner->mNameSpace->CreateChildNameSpace(prefix, urlSpec,
                                               *getter_AddRefs(newNameSpace));
      if (newNameSpace) {
        mInner->mNameSpace = newNameSpace;
      }
    }
    else {
      CheckRuleForAttributes(cssRule);
    }
    
    if (mDocument) {
      result = mDocument->StyleRuleAdded(this, cssRule);
      NS_ENSURE_SUCCESS(result, result);
    }
  }
  
  if (mDocument) {
    result = mDocument->EndUpdate();
    NS_ENSURE_SUCCESS(result, result);
  }

  if (loader) {
    loader->RecycleParser(css);
  }
  
  *aReturn = aIndex;
  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleSheetImpl::DeleteRule(PRUint32 aIndex)
{
  nsresult result = NS_ERROR_DOM_INDEX_SIZE_ERR;

  // XXX TBI: handle @rule types
  if (mInner && mInner->mOrderedRules) {
    if (mDocument) {
      result = mDocument->BeginUpdate();
      if (NS_FAILED(result))
        return result;
    }
    result = WillDirty();

    if (NS_SUCCEEDED(result)) {
      PRUint32 count;
      mInner->mOrderedRules->Count(&count);
      if (aIndex >= count)
        return NS_ERROR_DOM_INDEX_SIZE_ERR;

      nsCOMPtr<nsICSSRule> rule =
        dont_AddRef((nsICSSRule*)mInner->mOrderedRules->ElementAt(aIndex));
      if (rule) {
        mInner->mOrderedRules->RemoveElementAt(aIndex);
        rule->SetStyleSheet(nsnull);
        DidDirty();

        if (mDocument) {
          result = mDocument->StyleRuleRemoved(this, rule);
          NS_ENSURE_SUCCESS(result, result);
          
          result = mDocument->EndUpdate();
          NS_ENSURE_SUCCESS(result, result);
        }
      }
    }
  }

  return result;
}

NS_IMETHODIMP
CSSStyleSheetImpl::DeleteRuleFromGroup(nsICSSGroupRule* aGroup, PRUint32 aIndex)
{
  NS_ENSURE_ARG_POINTER(aGroup);
  
  nsresult result;
  nsCOMPtr<nsICSSRule> rule;
  result = aGroup->GetStyleRuleAt(aIndex, *getter_AddRefs(rule));
  NS_ENSURE_SUCCESS(result, result);
  
  // check that the rule actually belongs to this sheet!
  nsCOMPtr<nsIDOMCSSRule> domRule(do_QueryInterface(rule));
  nsCOMPtr<nsIDOMCSSStyleSheet> ruleSheet;
  result = domRule->GetParentStyleSheet(getter_AddRefs(ruleSheet));
  NS_ENSURE_SUCCESS(result, result);  
  nsCOMPtr<nsIDOMCSSStyleSheet> thisSheet;
  this->QueryInterface(NS_GET_IID(nsIDOMCSSStyleSheet), getter_AddRefs(thisSheet));

  if (thisSheet != ruleSheet) {
    return NS_ERROR_INVALID_ARG;
  }

  result = mDocument->BeginUpdate();
  NS_ENSURE_SUCCESS(result, result);

  result = WillDirty();
  NS_ENSURE_SUCCESS(result, result);

  result = aGroup->DeleteStyleRuleAt(aIndex);
  NS_ENSURE_SUCCESS(result, result);
  
  rule->SetStyleSheet(nsnull);
  
  DidDirty();

  result = mDocument->StyleRuleRemoved(this, rule);
  NS_ENSURE_SUCCESS(result, result);

  result = mDocument->EndUpdate();
  NS_ENSURE_SUCCESS(result, result);

  return NS_OK;
}

NS_IMETHODIMP
CSSStyleSheetImpl::InsertRuleIntoGroup(const nsAString & aRule, nsICSSGroupRule* aGroup, PRUint32 aIndex, PRUint32* _retval)
{
  nsresult result;
  // check that the group actually belongs to this sheet!
  nsCOMPtr<nsIDOMCSSRule> domGroup(do_QueryInterface(aGroup));
  nsCOMPtr<nsIDOMCSSStyleSheet> groupSheet;
  result = domGroup->GetParentStyleSheet(getter_AddRefs(groupSheet));
  NS_ENSURE_SUCCESS(result, result);  
  nsCOMPtr<nsIDOMCSSStyleSheet> thisSheet;
  this->QueryInterface(NS_GET_IID(nsIDOMCSSStyleSheet), getter_AddRefs(thisSheet));

  if (thisSheet != groupSheet) {
    return NS_ERROR_INVALID_ARG;
  }

  // get the css parser
  nsCOMPtr<nsICSSLoader> loader;
  nsCOMPtr<nsICSSParser> css;
  nsCOMPtr<nsIHTMLContentContainer> htmlContainer(do_QueryInterface(mDocument));
  if (htmlContainer) {
    htmlContainer->GetCSSLoader(*getter_AddRefs(loader));
  }
  NS_ASSERTION(loader || !mDocument, "Document with no CSS loader!");

  if (loader) {
    result = loader->GetParserFor(this, getter_AddRefs(css));
  }
  else {
    result = NS_NewCSSParser(getter_AddRefs(css));
    if (css) {
      css->SetStyleSheet(this);
    }
  }
  NS_ENSURE_SUCCESS(result, result);

  // parse and grab the rule 
  result = mDocument->BeginUpdate();
  NS_ENSURE_SUCCESS(result, result);

  result = WillDirty();
  NS_ENSURE_SUCCESS(result, result);

  nsCOMPtr<nsISupportsArray> rules;
  result = css->ParseRule(aRule, mInner->mURL, getter_AddRefs(rules));
  NS_ENSURE_SUCCESS(result, result);

  PRUint32 rulecount = 0;
  rules->Count(&rulecount);
    if (rulecount == 0 && !aRule.IsEmpty()) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  PRUint32 counter;
  nsCOMPtr<nsICSSRule> rule;
  for (counter = 0; counter < rulecount; counter++) {
    // Only rulesets are allowed in a group as of CSS2
    PRInt32 type = nsICSSRule::UNKNOWN_RULE;
    rule = dont_AddRef((nsICSSRule*)rules->ElementAt(counter));
    rule->GetType(type);
    if (type != nsICSSRule::STYLE_RULE) {
      return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
    }
  }
  
  result = aGroup->InsertStyleRulesAt(aIndex, rules);
  NS_ENSURE_SUCCESS(result, result);
  DidDirty();
  for (counter = 0; counter < rulecount; counter++) {
    rule = dont_AddRef((nsICSSRule*)rules->ElementAt(counter));
    CheckRuleForAttributes(rule);
  
    result = mDocument->StyleRuleAdded(this, rule);
    NS_ENSURE_SUCCESS(result, result);
  }

  result = mDocument->EndUpdate();
  NS_ENSURE_SUCCESS(result, result);

  if (loader) {
    loader->RecycleParser(css);
  }

  *_retval = aIndex;
  return NS_OK;
}

// XXX for backwards compatibility and convenience
NS_EXPORT nsresult
  NS_NewCSSStyleSheet(nsICSSStyleSheet** aInstancePtrResult, nsIURI* aURL)
{
  nsICSSStyleSheet* sheet;
  nsresult rv;
  if (NS_FAILED(rv = NS_NewCSSStyleSheet(&sheet)))
    return rv;

  if (NS_FAILED(rv = sheet->Init(aURL))) {
    NS_RELEASE(sheet);
    return rv;
  }

  *aInstancePtrResult = sheet;
  return NS_OK;
}

NS_EXPORT nsresult
  NS_NewCSSStyleSheet(nsICSSStyleSheet** aInstancePtrResult)
{
  if (aInstancePtrResult == nsnull) {
    return NS_ERROR_NULL_POINTER;
  }

  CSSStyleSheetImpl  *it = new CSSStyleSheetImpl();

  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(it);
  *aInstancePtrResult = it;
  return NS_OK;
}


// -------------------------------
// CSS Style rule processor implementation
//

CSSRuleProcessor::CSSRuleProcessor(void)
  : mSheets(nsnull),
    mRuleCascades(nsnull)
{
  NS_INIT_REFCNT();
}

static PRBool
DropProcessorReference(nsISupports* aSheet, void* aProcessor)
{
  nsICSSStyleSheet* sheet = (nsICSSStyleSheet*)aSheet;
  nsICSSStyleRuleProcessor* processor = (nsICSSStyleRuleProcessor*)aProcessor;
  sheet->DropRuleProcessorReference(processor);
  return PR_TRUE;
}

CSSRuleProcessor::~CSSRuleProcessor(void)
{
  if (mSheets) {
    mSheets->EnumerateForwards(DropProcessorReference, this);
    NS_RELEASE(mSheets);
  }
  ClearRuleCascades();
}

NS_IMPL_ADDREF(CSSRuleProcessor);
NS_IMPL_RELEASE(CSSRuleProcessor);

nsresult 
CSSRuleProcessor::QueryInterface(REFNSIID aIID, void** aInstancePtrResult)
{
  if (NULL == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }

  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
  if (aIID.Equals(NS_GET_IID(nsICSSStyleRuleProcessor))) {
    *aInstancePtrResult = (void*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIStyleRuleProcessor))) {
    *aInstancePtrResult = (void*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtrResult = (void*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}


NS_IMETHODIMP
CSSRuleProcessor::AppendStyleSheet(nsICSSStyleSheet* aStyleSheet)
{
  nsresult result = NS_OK;
  if (! mSheets) {
    result = NS_NewISupportsArray(&mSheets);
  }
  if (mSheets) {
    mSheets->AppendElement(aStyleSheet);
  }
  return result;
}

MOZ_DECL_CTOR_COUNTER(RuleProcessorData)

RuleProcessorData::RuleProcessorData(nsIPresContext* aPresContext,
                                     nsIContent* aContent, 
                                     nsRuleWalker* aRuleWalker,
                                     nsCompatibility* aCompat /*= nsnull*/)
{
  MOZ_COUNT_CTOR(RuleProcessorData);

  NS_PRECONDITION(aPresContext, "null pointer");
  NS_ASSERTION(!aContent || aContent->IsContentOfType(nsIContent::eELEMENT),
               "non-element leaked into SelectorMatches");

  mPresContext = aPresContext;
  mContent = aContent;
  mParentContent = nsnull;
  mRuleWalker = aRuleWalker;
  mScopedRoot = nsnull;

  mContentTag = nsnull;
  mContentID = nsnull;
  mStyledContent = nsnull;
  mIsHTMLContent = PR_FALSE;
  mIsHTMLLink = PR_FALSE;
  mIsSimpleXLink = PR_FALSE;
  mIsChecked = PR_FALSE;
  mLinkState = eLinkState_Unknown;
  mEventState = NS_EVENT_STATE_UNSPECIFIED;
  mNameSpaceID = kNameSpaceID_Unknown;
  mPreviousSiblingData = nsnull;
  mParentData = nsnull;

  if(!aCompat) {
    // get the compat. mode (unless it is provided)
    nsCompatibility quirkMode = eCompatibility_Standard;
    mPresContext->GetCompatibilityMode(&quirkMode);
    mIsQuirkMode = eCompatibility_Standard == quirkMode ? PR_FALSE : PR_TRUE;
  } else {
    mIsQuirkMode = eCompatibility_Standard == *aCompat ? PR_FALSE : PR_TRUE;
  }


  if(aContent){
    // we hold no ref to the content...
    mContent = aContent;

    // get the namespace
    aContent->GetNameSpaceID(mNameSpaceID);

    // get the tag and parent
    aContent->GetTag(mContentTag);
    aContent->GetParent(mParentContent);

    // get the event state
    nsIEventStateManager* eventStateManager = nsnull;
    mPresContext->GetEventStateManager(&eventStateManager);
    if(eventStateManager) {
      eventStateManager->GetContentState(aContent, mEventState);
      NS_RELEASE(eventStateManager);
    }

    // get the styledcontent interface and the ID
    if (NS_SUCCEEDED(aContent->QueryInterface(NS_GET_IID(nsIStyledContent), (void**)&mStyledContent))) {
      NS_ASSERTION(mStyledContent, "Succeeded but returned null");
      mStyledContent->GetID(mContentID);
    }

    // see if there are attributes for the content
    PRInt32 attrCount = 0;
    aContent->GetAttrCount(attrCount);
    mHasAttributes = PRBool(attrCount > 0);

    // check for HTMLContent and Link status
    if (aContent->IsContentOfType(nsIContent::eHTML)) 
      mIsHTMLContent = PR_TRUE;

    // if HTML content and it has some attributes, check for an HTML link
    // NOTE: optimization: cannot be a link if no attributes (since it needs an href)
    if (PR_TRUE == mIsHTMLContent && mHasAttributes) {
      // check if it is an HTML Link
      if(nsStyleUtil::IsHTMLLink(aContent, mContentTag, mPresContext, &mLinkState)) {
        mIsHTMLLink = PR_TRUE;
      }
    } 

    // if not an HTML link, check for a simple xlink (cannot be both HTML link and xlink)
    // NOTE: optimization: cannot be an XLink if no attributes (since it needs an 
    if(PR_FALSE == mIsHTMLLink &&
       mHasAttributes && 
       !(aContent->IsContentOfType(nsIContent::eHTML) || aContent->IsContentOfType(nsIContent::eXUL)) && 
       nsStyleUtil::IsSimpleXlink(aContent, mPresContext, &mLinkState)) {
      mIsSimpleXLink = PR_TRUE;
    } 

    if (mIsHTMLContent) {
      PRBool isChecked = PR_FALSE;
      if (mContentTag == nsHTMLAtoms::option) {
        nsCOMPtr<nsIDOMHTMLOptionElement> optEl = do_QueryInterface(mContent);
        optEl->GetSelected(&isChecked);
      } else if (mContentTag == nsHTMLAtoms::input) {
        nsCOMPtr<nsIDOMHTMLInputElement> inputEl = do_QueryInterface(mContent);
        inputEl->GetChecked(&isChecked);
      }
      mIsChecked = isChecked;
    }
  }
}

RuleProcessorData::~RuleProcessorData()
{
  MOZ_COUNT_DTOR(RuleProcessorData);

  if (mPreviousSiblingData)
    mPreviousSiblingData->Destroy(mPresContext);
  if (mParentData)
    mParentData->Destroy(mPresContext);

  NS_IF_RELEASE(mParentContent);
  NS_IF_RELEASE(mContentTag);
  NS_IF_RELEASE(mContentID);
  NS_IF_RELEASE(mStyledContent);
}

static const PRUnichar kNullCh = PRUnichar('\0');

static PRBool ValueIncludes(const nsString& aValueList, const nsString& aValue, PRBool aCaseSensitive)
{
  nsAutoString  valueList(aValueList);

  valueList.Append(kNullCh);  // put an extra null at the end

  PRUnichar* value = (PRUnichar*)(const PRUnichar*)aValue.get();
  PRUnichar* start = (PRUnichar*)(const PRUnichar*)valueList.get();
  PRUnichar* end   = start;

  while (kNullCh != *start) {
    while ((kNullCh != *start) && nsCRT::IsAsciiSpace(*start)) {  // skip leading space
      start++;
    }
    end = start;

    while ((kNullCh != *end) && (PR_FALSE == nsCRT::IsAsciiSpace(*end))) { // look for space or end
      end++;
    }
    *end = kNullCh; // end string here

    if (start < end) {
      if (aCaseSensitive) {
        if (!nsCRT::strcmp(value, start)) {
          return PR_TRUE;
        }
      }
      else {
        if (nsDependentString(value).Equals(nsDependentString(start),
                                            nsCaseInsensitiveStringComparator())) {
          return PR_TRUE;
        }
      }
    }

    start = ++end;
  }
  return PR_FALSE;
}

inline PRBool IsEventPseudo(nsIAtom* aAtom)
{
  return PRBool ((nsCSSAtoms::activePseudo == aAtom)   || 
                 (nsCSSAtoms::dragOverPseudo == aAtom) || 
                 (nsCSSAtoms::focusPseudo == aAtom)    || 
                 (nsCSSAtoms::hoverPseudo == aAtom)); 
                 // XXX selected, enabled, disabled, selection?
}

inline PRBool IsLinkPseudo(nsIAtom* aAtom)
{
  return PRBool ((nsCSSAtoms::linkPseudo == aAtom) || 
                 (nsCSSAtoms::visitedPseudo == aAtom) ||
                 (nsCSSAtoms::anyLinkPseudo == aAtom));
}

inline PRBool IsEventSensitive(nsIAtom *aPseudo, nsIAtom *aContentTag, PRBool aSelectorIsGlobal)
{
  // if the selector is global, meaning it is not tied to a tag, then
  // we restrict the application of the event pseudo to the following tags
  if (aSelectorIsGlobal) {
    return PRBool ((nsHTMLAtoms::a == aContentTag)      ||
                   (nsHTMLAtoms::button == aContentTag) ||
                   (nsHTMLAtoms::img == aContentTag)    ||
                   (nsHTMLAtoms::input == aContentTag)  ||
                   (nsHTMLAtoms::li == aContentTag)     ||
                   (nsHTMLAtoms::label == aContentTag)  ||
                   (nsHTMLAtoms::select == aContentTag) ||
                   (nsHTMLAtoms::textarea == aContentTag));
  } else {
    // selector is not global, so apply the event pseudo to everything except HTML and BODY
    return PRBool ((nsHTMLAtoms::html != aContentTag) && 
                   (nsHTMLAtoms::body != aContentTag));
  }
}


static PRBool IsSignificantChild(nsIContent* aChild, PRBool aAcceptNonWhitespaceText)
{
  nsIAtom* tag;
  aChild->GetTag(tag); // skip text & comments
  if ((tag != nsLayoutAtoms::textTagName) && 
      (tag != nsLayoutAtoms::commentTagName) &&
      (tag != nsLayoutAtoms::processingInstructionTagName)) {
    NS_IF_RELEASE(tag);
    return PR_TRUE;
  }
  if (aAcceptNonWhitespaceText) {
    if (tag == nsLayoutAtoms::textTagName) {  // skip only whitespace text
      nsITextContent* text = nsnull;
      if (NS_SUCCEEDED(aChild->QueryInterface(NS_GET_IID(nsITextContent), (void**)&text))) {
        PRBool  isWhite;
        text->IsOnlyWhitespace(&isWhite);
        NS_RELEASE(text);
        if (! isWhite) {
          NS_RELEASE(tag);
          return PR_TRUE;
        }
      }
    }
  }
  NS_IF_RELEASE(tag);
  return PR_FALSE;
}


// NOTE:  The |aStateMask| code isn't going to work correctly anymore if
// we start batching style changes, because if multiple states change in
// separate notifications then we might determine the style is not
// state-dependent when it really is (e.g., determining that a
// :hover:active rule no longer matches when both states are unset).
static PRBool SelectorMatches(RuleProcessorData &data,
                              nsCSSSelector* aSelector,
                              PRInt32 aStateMask, // states NOT to test
                              PRInt8 aNegationIndex) 

{
  // if we are dealing with negations, reverse the values of PR_TRUE and PR_FALSE
  PRBool  localFalse = PRBool(0 < aNegationIndex);
  PRBool  localTrue = PRBool(0 == aNegationIndex);
  PRBool  result = localTrue;

  // Do not perform the test if aNegationIndex==1
  // because it then contains only negated IDs, classes, attributes and pseudo-
  // classes
  if (1 != aNegationIndex) {
    if (kNameSpaceID_Unknown != aSelector->mNameSpace) {
      if (data.mNameSpaceID != aSelector->mNameSpace) {
        result = localFalse;
      }
    }
    if (localTrue == result) {
      if ((nsnull != aSelector->mTag) && (aSelector->mTag != data.mContentTag)) {
        result = localFalse;
      }
    }
    // optimization : bail out early if we can
    if (!result) {
      return PR_FALSE;
    }
  }

  result = PR_TRUE;

  if (nsnull != aSelector->mPseudoClassList) {  // test for pseudo class match
    // first-child, root, lang, active, focus, hover, link, visited...
    // XXX disabled, enabled, selected, selection
    nsAtomList* pseudoClass = aSelector->mPseudoClassList;

    while (result && (nsnull != pseudoClass)) {
      if ((nsCSSAtoms::firstChildPseudo == pseudoClass->mAtom) ||
          (nsCSSAtoms::firstNodePseudo == pseudoClass->mAtom) ) {
        nsIContent* firstChild = nsnull;
        nsIContent* parent = data.mParentContent;
        if (parent) {
          PRInt32 index = -1;
          do {
            parent->ChildAt(++index, firstChild);
            if (firstChild) { // stop at first non-comment and non-whitespace node (and non-text node for firstChild)
              if (IsSignificantChild(firstChild, (nsCSSAtoms::firstNodePseudo == pseudoClass->mAtom))) {
                break;
              }
              NS_RELEASE(firstChild);
            }
            else {
              break;
            }
          } while (1 == 1);
        }
        result = PRBool(localTrue == (data.mContent == firstChild));
        NS_IF_RELEASE(firstChild);
      }
      else if ((nsCSSAtoms::lastChildPseudo == pseudoClass->mAtom) ||
               (nsCSSAtoms::lastNodePseudo == pseudoClass->mAtom)) {
        nsIContent* lastChild = nsnull;
        nsIContent* parent = data.mParentContent;
        if (parent) {
          PRInt32 index;
          parent->ChildCount(index);
          do {
            parent->ChildAt(--index, lastChild);
            if (lastChild) { // stop at first non-comment and non-whitespace node (and non-text node for lastChild)
              if (IsSignificantChild(lastChild, (nsCSSAtoms::lastNodePseudo == pseudoClass->mAtom))) {
                break;
              }
              NS_RELEASE(lastChild);
            }
            else {
              break;
            }
          } while (1 == 1);
        }
        result = PRBool(localTrue == (data.mContent == lastChild));
        NS_IF_RELEASE(lastChild);
      }
      else if (nsCSSAtoms::emptyPseudo == pseudoClass->mAtom) {
        nsIContent* child = nsnull;
        nsIContent* element = data.mContent;
        PRInt32 index = -1;
        do {
          element->ChildAt(++index, child);
          if (child) { // stop at first non-comment and non-whitespace node
            if (IsSignificantChild(child, PR_TRUE)) {
              break;
            }
            NS_RELEASE(child);
          }
          else {
            break;
          }
        } while (1 == 1);
        result = PRBool(localTrue == (child == nsnull));
        NS_IF_RELEASE(child);
      }
      else if (nsCSSAtoms::rootPseudo == pseudoClass->mAtom) {
        if (data.mParentContent) {
          result = localFalse;
        }
        else {
          result = localTrue;
        }
      }
      else if (nsCSSAtoms::xblBoundElementPseudo == pseudoClass->mAtom) {
        result = (data.mScopedRoot && data.mScopedRoot == data.mContent)
                   ? localTrue : localFalse;
      }
      else if (nsCSSAtoms::langPseudo == pseudoClass->mAtom) {
        // XXX not yet implemented
        result = PR_FALSE;
      }
      else if (IsEventPseudo(pseudoClass->mAtom)) {
        // check if the element is event-sensitive
        // NOTE: we distinguish between global and subjected selectors so
        //       pass that information on to the determining routine
        // ALSO NOTE: we used to do this only in Quirks mode, but because of
        //            performance problems we do it all the time now (bug 68821)
        //            When style resolution due to state changes is optimized this
        //            should go back to QuirksMode only behavour (see also bug 75559)
        PRBool isSelectorGlobal = aSelector->mTag==nsnull ? PR_TRUE : PR_FALSE;
        if ((data.mIsHTMLContent) &&
            (!IsEventSensitive(pseudoClass->mAtom, data.mContentTag, isSelectorGlobal))){
          result = localFalse;
        } else {
          if (nsCSSAtoms::activePseudo == pseudoClass->mAtom) {
            result = (aStateMask & NS_EVENT_STATE_ACTIVE) ||
                     (localTrue == (0 != (data.mEventState & NS_EVENT_STATE_ACTIVE)));
          }
          else if (nsCSSAtoms::focusPseudo == pseudoClass->mAtom) {
            result = (aStateMask & NS_EVENT_STATE_FOCUS) ||
                     (localTrue == (0 != (data.mEventState & NS_EVENT_STATE_FOCUS)));
          }
          else if (nsCSSAtoms::hoverPseudo == pseudoClass->mAtom) {
            result = (aStateMask & NS_EVENT_STATE_HOVER) ||
                     (localTrue == (0 != (data.mEventState & NS_EVENT_STATE_HOVER)));
          }
          else if (nsCSSAtoms::dragOverPseudo == pseudoClass->mAtom) {
            result = (aStateMask & NS_EVENT_STATE_DRAGOVER) ||
                     (localTrue == (0 != (data.mEventState & NS_EVENT_STATE_DRAGOVER)));
          }
        } 
      }
      else if (IsLinkPseudo(pseudoClass->mAtom)) {
        if (data.mIsHTMLLink || data.mIsSimpleXLink) {
          if (result) {
            if (nsCSSAtoms::anyLinkPseudo == pseudoClass->mAtom) {
              result = localTrue;
            }
            else if (nsCSSAtoms::linkPseudo == pseudoClass->mAtom) {
              result = PRBool(localTrue == (eLinkState_Unvisited == data.mLinkState));
            }
            else if (nsCSSAtoms::visitedPseudo == pseudoClass->mAtom) {
              result = PRBool(localTrue == (eLinkState_Visited == data.mLinkState));
            }
          }
        }
        else {
          result = localFalse;  // not a link
        }
      }
      else if (nsCSSAtoms::checkedPseudo == pseudoClass->mAtom) {
        // This pseudoclass matches the selected state on the following elements:
        //  <option>
        //  <input type=checkbox>
        //  <input type=radio>
        if (!(aStateMask & NS_EVENT_STATE_CHECKED))
          result = data.mIsChecked ? localTrue : localFalse;
      }
      else {
        result = localFalse;  // unknown pseudo class
      }
      pseudoClass = pseudoClass->mNext;
    }
  }

  // namespace/tag match
  if (result && aSelector->mAttrList) { // test for attribute match
    // if no attributes on the content, no match
    if(!data.mHasAttributes) {
      result = localFalse;
    } else {
      result = localTrue;
      nsAttrSelector* attr = aSelector->mAttrList;
      do {
        if (!data.mContent->HasAttr(attr->mNameSpace, attr->mAttr)) {
          result = localFalse;
        }
        else if (attr->mFunction != NS_ATTR_FUNC_SET) {
          nsAutoString value;
          nsresult attrState =
              data.mContent->GetAttr(attr->mNameSpace, attr->mAttr, value);
          NS_ASSERTION(NS_SUCCEEDED(attrState) &&
                       NS_CONTENT_ATTR_NOT_THERE != attrState,
                       "HasAttr lied or GetAttr failed");
          PRBool isCaseSensitive = attr->mCaseSensitive;
          switch (attr->mFunction) {
            case NS_ATTR_FUNC_EQUALS: 
              if (isCaseSensitive) {
                result = PRBool(localTrue == value.Equals(attr->mValue));
              }
              else {
                result = PRBool(localTrue == value.Equals(attr->mValue, nsCaseInsensitiveStringComparator()));
              }
              break;
            case NS_ATTR_FUNC_INCLUDES: 
              result = PRBool(localTrue == ValueIncludes(value, attr->mValue, isCaseSensitive));
              break;
            case NS_ATTR_FUNC_DASHMATCH: 
              {
                PRUint32 selLen = attr->mValue.Length();
                PRUint32 valLen = value.Length();
                if (selLen > valLen) {
                  result = localFalse;
                } else {
                  if (selLen != valLen &&
                      value.CharAt(selLen) != PRUnichar('-')) {
                    // to match, the value must have a dash after the end of
                    // the selector's text (unless the selector and the value
                    // have the same text)
                    result = localFalse;
                    break;
                  }
                  if (isCaseSensitive)
                    result = PRBool(localTrue == Substring(value, 0, selLen).Equals(attr->mValue, nsDefaultStringComparator()));
                  else
                    result = PRBool(localTrue == Substring(value, 0, selLen).Equals(attr->mValue, nsCaseInsensitiveStringComparator()));
                }
              }
              break;
            case NS_ATTR_FUNC_ENDSMATCH:
              {
                PRUint32 selLen = attr->mValue.Length();
                PRUint32 valLen = value.Length();
                if (selLen > valLen) {
                  result = localFalse;
                } else {
                  if (isCaseSensitive)
                    result = PRBool(localTrue == Substring(value, valLen - selLen, selLen).Equals(attr->mValue, nsDefaultStringComparator()));
                  else
                    result = PRBool(localTrue == Substring(value, valLen - selLen, selLen).Equals(attr->mValue, nsCaseInsensitiveStringComparator()));
                }
              }
              break;
            case NS_ATTR_FUNC_BEGINSMATCH:
              {
                PRUint32 selLen = attr->mValue.Length();
                PRUint32 valLen = value.Length();
                if (selLen > valLen) {
                  result = localFalse;
                } else {
                  if (isCaseSensitive)
                    result = PRBool(localTrue == Substring(value, 0, selLen).Equals(attr->mValue, nsDefaultStringComparator()));
                  else
                    result = PRBool(localTrue == Substring(value, 0, selLen).Equals(attr->mValue, nsCaseInsensitiveStringComparator()));
                }
              }
              break;
            case NS_ATTR_FUNC_CONTAINSMATCH:
              result = PRBool(localTrue == (FindInReadable(attr->mValue, value, nsCaseInsensitiveStringComparator())));
              break;
          }
        }
        attr = attr->mNext;
      } while (result && (nsnull != attr));
    }
  }
  if (result &&
      ((nsnull != aSelector->mIDList) || (nsnull != aSelector->mClassList))) {  // test for ID & class match
    result = localFalse;
    if (data.mStyledContent) {
      PRBool isCaseSensitive = !data.mIsQuirkMode; // bug 93371
      nsAtomList* IDList = aSelector->mIDList;
      if (nsnull == IDList) {
        result = PR_TRUE;
      }
      else if (nsnull != data.mContentID) {
        result = PR_TRUE;
        if (isCaseSensitive) {
          do {
            if (localTrue == (IDList->mAtom != data.mContentID)) {
              result = PR_FALSE;
              break;
            }
            IDList = IDList->mNext;
          } while (IDList);
        } else {
          const PRUnichar* id1Str;
          data.mContentID->GetUnicode(&id1Str);
          nsDependentString id1(id1Str);
          do {
            const PRUnichar* id2Str;
            IDList->mAtom->GetUnicode(&id2Str);
            nsDependentString id2(id2Str);
            if (localTrue !=
                id1.Equals(id2, nsCaseInsensitiveStringComparator())) {
              result = PR_FALSE;
              break;
            }
            IDList = IDList->mNext;
          } while (IDList);
        }
      }
      
      if (result) {
        nsAtomList* classList = aSelector->mClassList;
        while (nsnull != classList) {
          if (localTrue == (NS_COMFALSE == data.mStyledContent->HasClass(classList->mAtom, isCaseSensitive))) {
            result = PR_FALSE;
            break;
          }
          classList = classList->mNext;
        }
      }
    }
  }
  
  // apply SelectorMatches to the negated selectors in the chain
  if (result && (nsnull != aSelector->mNegations)) {
    result = SelectorMatches(data, aSelector->mNegations, aStateMask,
                             aNegationIndex+1);
  }
  return result;
}

static PRBool SelectorMatchesTree(RuleProcessorData &data,
                                  nsCSSSelector* aSelector) 
{
  nsCSSSelector* selector = aSelector;

  if (selector) {
    nsIContent* content = nsnull;
    nsIContent* lastContent = data.mContent;
    NS_ADDREF(lastContent);
    RuleProcessorData* curdata = &data;
    while (nsnull != selector) { // check compound selectors
      // Find the appropriate content (whether parent or previous sibling)
      // to check next, and if we don't already have a RuleProcessorData
      // for it, create one.

      // for adjacent sibling combinators, the content to test against the
      // selector is the previous sibling
      nsCompatibility compat = curdata->mIsQuirkMode ? eCompatibility_NavQuirks : eCompatibility_Standard;
      RuleProcessorData* newdata;
      if (PRUnichar('+') == selector->mOperator) {
        newdata = curdata->mPreviousSiblingData;
        if (!newdata) {
          nsIContent* parent;
          PRInt32 index;
          lastContent->GetParent(parent);
          if (parent) {
            parent->IndexOf(lastContent, index);
            while (0 <= --index) {  // skip text & comment nodes
              parent->ChildAt(index, content);
              nsIAtom* tag;
              content->GetTag(tag);
              if ((tag != nsLayoutAtoms::textTagName) && 
                  (tag != nsLayoutAtoms::commentTagName)) {
                NS_IF_RELEASE(tag);
                newdata =
                    new (curdata->mPresContext) RuleProcessorData(curdata->mPresContext, content,
                                                                    curdata->mRuleWalker, &compat);
                curdata->mPreviousSiblingData = newdata;    
                break;
              }
              NS_RELEASE(content);
              NS_IF_RELEASE(tag);
            }
            NS_RELEASE(parent);
          }
        } else {
          content = newdata->mContent;
          NS_ADDREF(content);
        }
      }
      // for descendant combinators and child combinators, the content
      // to test against is the parent
      else {
        newdata = curdata->mParentData;
        if (!newdata) {
          lastContent->GetParent(content);
          if (content) {
            newdata = new (curdata->mPresContext) RuleProcessorData(curdata->mPresContext, content,
                                                                      curdata->mRuleWalker, &compat);
            curdata->mParentData = newdata;    
          }
        } else {
          content = newdata->mContent;
          NS_ADDREF(content);
        }
      }
      if (! newdata) {
        NS_ASSERTION(!content, "content must be null");
        break;
      }
      if (SelectorMatches(*newdata, selector, 0, 0)) {
        // to avoid greedy matching, we need to recurse if this is a
        // descendant combinator and the next combinator is not
        if ((NS_IS_GREEDY_OPERATOR(selector->mOperator)) &&
            (selector->mNext) &&
            (!NS_IS_GREEDY_OPERATOR(selector->mNext->mOperator))) {

          // pretend the selector didn't match, and step through content
          // while testing the same selector

          // This approach is slightly strange is that when it recurses
          // it tests from the top of the content tree, down.  This
          // doesn't matter much for performance since most selectors
          // don't match.  (If most did, it might be faster...)
          if (SelectorMatchesTree(*newdata, selector)) {
            selector = nsnull; // indicate success
            break;
          }
        }
        selector = selector->mNext;
      }
      else {
        // for adjacent sibling and child combinators, if we didn't find
        // a match, we're done
        if (!NS_IS_GREEDY_OPERATOR(selector->mOperator)) {
          NS_RELEASE(content);
          break;  // parent was required to match
        }
      }
      NS_IF_RELEASE(lastContent);
      lastContent = content;  // take refcount
      content = nsnull;
      curdata = newdata;
    }
    NS_IF_RELEASE(lastContent);
  }
  return PRBool(nsnull == selector);  // matches if ran out of selectors
}

static void ContentEnumFunc(nsICSSStyleRule* aRule, void* aData)
{
  ElementRuleProcessorData* data = (ElementRuleProcessorData*)aData;

  nsCSSSelector* selector = aRule->FirstSelector();
  if (SelectorMatches(*data, selector, 0, 0)) {
    selector = selector->mNext;
    if (SelectorMatchesTree(*data, selector)) {
      // for performance, require that every implementation of
      // nsICSSStyleRule return the same pointer for nsIStyleRule (why
      // would anything multiply inherit nsIStyleRule anyway?)
#ifdef DEBUG
      nsCOMPtr<nsIStyleRule> iRule = do_QueryInterface(aRule);
      NS_ASSERTION(NS_STATIC_CAST(nsIStyleRule*, aRule) == iRule.get(),
                   "Please fix QI so this performance optimization is valid");
#endif
      data->mRuleWalker->Forward(NS_STATIC_CAST(nsIStyleRule*, aRule));
      // nsStyleSet will deal with the !important rule
    }
  }
}

#ifdef DEBUG_RULES
static PRBool ContentEnumWrap(nsISupports* aRule, void* aData)
{
  ContentEnumFunc((nsICSSStyleRule*)aRule, aData);
  return PR_TRUE;
}
#endif

NS_IMETHODIMP
CSSRuleProcessor::RulesMatching(ElementRuleProcessorData *aData,
                                nsIAtom* aMedium)
{
  NS_PRECONDITION(aData->mContent->IsContentOfType(nsIContent::eELEMENT),
                  "content must be element");

  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext, aMedium);

  if (cascade) {
    nsAutoVoidArray classArray;

    nsIStyledContent* styledContent = aData->mStyledContent;
    if (styledContent)
      styledContent->GetClasses(classArray);

    cascade->mRuleHash.EnumerateAllRules(aData->mNameSpaceID, aData->mContentTag, aData->mContentID, classArray, ContentEnumFunc, aData);

#ifdef DEBUG_RULES
    nsISupportsArray* list1;
    nsISupportsArray* list2;
    NS_NewISupportsArray(&list1);
    NS_NewISupportsArray(&list2);

    aData->mResults = list1;
    cascade->mRuleHash.EnumerateAllRules(aData->mNameSpaceID, aData->mContentTag, aData->mContentID, classArray, ContentEnumFunc, &data);
    aData->mResults = list2;
    cascade->mWeightedRules->EnumerateBackwards(ContentEnumWrap, &data);
    NS_ASSERTION(list1->Equals(list2), "lists not equal");
    NS_RELEASE(list1);
    NS_RELEASE(list2);
#endif
  }
  return NS_OK;
}

static void PseudoEnumFunc(nsICSSStyleRule* aRule, void* aData)
{
  PseudoRuleProcessorData* data = (PseudoRuleProcessorData*)aData;

  nsCSSSelector* selector = aRule->FirstSelector();

  NS_ASSERTION(selector->mTag == data->mPseudoTag, "RuleHash failure");
  PRBool matches = PR_TRUE;
  if (data->mComparator)
    data->mComparator->PseudoMatches(data->mPseudoTag, selector, &matches);

  if (matches) {
    selector = selector->mNext;

    if (selector) { // test next selector specially
      if (PRUnichar('+') == selector->mOperator) {
        return; // not valid here, can't match
      }
      if (SelectorMatches(*data, selector, 0, 0)) {
        selector = selector->mNext;
      }
      else {
        if (PRUnichar('>') == selector->mOperator) {
          return; // immediate parent didn't match
        }
      }
    }

    if (selector && 
        (! SelectorMatchesTree(*data, selector))) {
      return; // remaining selectors didn't match
    }

    // for performance, require that every implementation of
    // nsICSSStyleRule return the same pointer for nsIStyleRule (why
    // would anything multiply inherit nsIStyleRule anyway?)
#ifdef DEBUG
    nsCOMPtr<nsIStyleRule> iRule = do_QueryInterface(aRule);
    NS_ASSERTION(NS_STATIC_CAST(nsIStyleRule*, aRule) == iRule.get(),
                 "Please fix QI so this performance optimization is valid");
#endif
    data->mRuleWalker->Forward(NS_STATIC_CAST(nsIStyleRule*, aRule));
    // nsStyleSet will deal with the !important rule
  }
}

#ifdef DEBUG_RULES
static PRBool PseudoEnumWrap(nsISupports* aRule, void* aData)
{
  PseudoEnumFunc((nsICSSStyleRule*)aRule, aData);
  return PR_TRUE;
}
#endif

NS_IMETHODIMP
CSSRuleProcessor::RulesMatching(PseudoRuleProcessorData* aData,
                                nsIAtom* aMedium)
{
  NS_PRECONDITION(!aData->mContent ||
                  aData->mContent->IsContentOfType(nsIContent::eELEMENT),
                  "content (if present) must be element");

  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext, aMedium);

  if (cascade) {
    cascade->mRuleHash.EnumerateTagRules(aData->mPseudoTag,
                                         PseudoEnumFunc, aData);

#ifdef DEBUG_RULES
    nsISupportsArray* list1;
    nsISupportsArray* list2;
    NS_NewISupportsArray(&list1);
    NS_NewISupportsArray(&list2);
    aData->mResults = list1;
    cascade->mRuleHash.EnumerateTagRules(aData->mPseudoTag, PseudoEnumFunc, aData);
    aData->mResults = list2;
    cascade->mWeightedRules->EnumerateBackwards(PseudoEnumWrap, aData);
    NS_ASSERTION(list1->Equals(list2), "lists not equal");
    NS_RELEASE(list1);
    NS_RELEASE(list2);
#endif
  }
  return NS_OK;
}

static 
PRBool PR_CALLBACK StateEnumFunc(void* aSelector, void* aData)
{
  StateRuleProcessorData* data = (StateRuleProcessorData*)aData;

  nsCSSSelector* selector = (nsCSSSelector*)aSelector;
  if (SelectorMatches(*data, selector, data->mStateMask, 0)) {
    selector = selector->mNext;
    if (SelectorMatchesTree(*data, selector)) {
      return PR_FALSE;
    }
  }
  return PR_TRUE;
}

NS_IMETHODIMP
CSSRuleProcessor::HasStateDependentStyle(StateRuleProcessorData* aData,
                                         nsIAtom* aMedium,
                                         PRBool* aResult)
{
  NS_PRECONDITION(aData->mContent->IsContentOfType(nsIContent::eELEMENT),
                  "content must be element");

  RuleCascadeData* cascade = GetRuleCascade(aData->mPresContext, aMedium);

  // Look up content in state rule list.  If enumeration stopped, have state.
  *aResult = cascade &&
             !cascade->mStateSelectors.EnumerateForwards(StateEnumFunc, aData);

  return NS_OK;
}


#ifdef DEBUG

struct CascadeSizeEnumData {

  CascadeSizeEnumData(nsISizeOfHandler *aSizeOfHandler, 
                      nsUniqueStyleItems *aUniqueStyleItem,
                      nsIAtom *aTag)
  {
    handler = aSizeOfHandler;
    uniqueItems = aUniqueStyleItem;
    tag = aTag;
  }
    // weak references all 'round

  nsISizeOfHandler    *handler;
  nsUniqueStyleItems  *uniqueItems;
  nsIAtom             *tag;
};

static 
PRBool PR_CALLBACK StateSelectorsSizeEnumFunc( void *aSelector, void *aData )
{
  nsCSSSelector* selector = (nsCSSSelector*)aSelector;
  CascadeSizeEnumData *pData = (CascadeSizeEnumData *)aData;
  NS_ASSERTION(selector && pData, "null arguments not supported");

  if(! pData->uniqueItems->AddItem((void*)selector)){
    return PR_TRUE;
  }

  // pass the call to the selector
  PRUint32 localSize = 0;
  selector->SizeOf(pData->handler, localSize);

  return PR_TRUE;
}

static 
PRBool WeightedRulesSizeEnumFunc( nsISupports *aRule, void *aData )
{
  nsICSSStyleRule* rule = (nsICSSStyleRule*)aRule;
  CascadeSizeEnumData *pData = (CascadeSizeEnumData *)aData;
  NS_ASSERTION(rule && pData, "null arguments not supported");

  if(! pData->uniqueItems->AddItem((void*)rule)){
    return PR_TRUE;
  }

  PRUint32 localSize=0;

  // pass the call to the rule
  rule->SizeOf(pData->handler, localSize);

  return PR_TRUE;
}

static 
void CascadeSizeEnumFunc(RuleCascadeData *cascade, CascadeSizeEnumData *pData)
{
  NS_ASSERTION(cascade && pData, "null arguments not supported");

  // see if the cascade has already been counted
  if(!(pData->uniqueItems->AddItem(cascade))){
    return;
  }
  // record the size of the cascade data itself
  PRUint32 localSize = sizeof(RuleCascadeData);
  pData->handler->AddSize(pData->tag, localSize);

  // next add up the selectors and the weighted rules for the cascade
  nsCOMPtr<nsIAtom> stateSelectorSizeTag;
  stateSelectorSizeTag = getter_AddRefs(NS_NewAtom("CascadeStateSelectors"));
  CascadeSizeEnumData stateData(pData->handler,pData->uniqueItems,stateSelectorSizeTag);
  cascade->mStateSelectors.EnumerateForwards(StateSelectorsSizeEnumFunc, &stateData);
  
  if(cascade->mWeightedRules){
    nsCOMPtr<nsIAtom> weightedRulesSizeTag;
    weightedRulesSizeTag = getter_AddRefs(NS_NewAtom("CascadeWeightedRules"));
    CascadeSizeEnumData stateData2(pData->handler,pData->uniqueItems,weightedRulesSizeTag);
    cascade->mWeightedRules->EnumerateForwards(WeightedRulesSizeEnumFunc, &stateData2);
  }
}

/******************************************************************************
* SizeOf method:
*
*  Self (reported as CSSRuleProcessor's size): 
*    1) sizeof(*this)
*
*  Contained / Aggregated data (not reported as CSSRuleProcessor's size):
*    1) Delegate to the StyleSheets in the mSheets collection
*    2) Delegate to the Rules in the CascadeTable
*
*  Children / siblings / parents:
*    none
*    
******************************************************************************/
void CSSRuleProcessor::SizeOf(nsISizeOfHandler *aSizeOfHandler, PRUint32 &aSize)
{
  NS_ASSERTION(aSizeOfHandler != nsnull, "SizeOf handler cannot be null");

  // first get the unique items collection
  UNIQUE_STYLE_ITEMS(uniqueItems);
  if(! uniqueItems->AddItem((void*)this)){
    return;
  }

  // create a tag for this instance
  nsCOMPtr<nsIAtom> tag;
  tag = getter_AddRefs(NS_NewAtom("CSSRuleProcessor"));
  // get the size of an empty instance and add to the sizeof handler
  aSize = sizeof(CSSRuleProcessor);

  // collect sizes for the data
  // - mSheets
  // - mRuleCascades

  // sheets first
  if(mSheets && uniqueItems->AddItem(mSheets)){
    PRUint32 sheetCount, curSheet, localSize2;
    mSheets->Count(&sheetCount);
    for(curSheet=0; curSheet < sheetCount; curSheet++){
      nsCOMPtr<nsICSSStyleSheet> pSheet =
        dont_AddRef((nsICSSStyleSheet*)mSheets->ElementAt(curSheet));
      if(pSheet && uniqueItems->AddItem((void*)pSheet)){
        pSheet->SizeOf(aSizeOfHandler, localSize2);
        // XXX aSize += localSize2;
      }
    }
  }

  // and for the medium cascade table we account for the hash table overhead,
  // and then compute the sizeof each rule-cascade in the table
  {
    nsCOMPtr<nsIAtom> tag2 = getter_AddRefs(NS_NewAtom("RuleCascade"));
    CascadeSizeEnumData data(aSizeOfHandler, uniqueItems, tag2);
    for (RuleCascadeData *cascadeData = mRuleCascades;
   cascadeData;
   cascadeData = cascadeData->mNext) {
      CascadeSizeEnumFunc(cascadeData, &data);
    }
  }
  
  // now add the size of the RuleProcessor
  aSizeOfHandler->AddSize(tag,aSize);
}
#endif

NS_IMETHODIMP
CSSRuleProcessor::ClearRuleCascades(void)
{
  RuleCascadeData *data = mRuleCascades;
  mRuleCascades = nsnull;
  while (data) {
    RuleCascadeData *next = data->mNext;
    delete data;
    data = next;
  }
  return NS_OK;
}


static
PRBool BuildHashEnum(nsISupports* aRule, void* aHash)
{
  nsICSSStyleRule* rule = (nsICSSStyleRule*)aRule;
  RuleHash* hash = (RuleHash*)aHash;
  hash->AppendRule(rule);
  return PR_TRUE;
}

inline
PRBool IsStateSelector(nsCSSSelector& aSelector)
{
  nsAtomList* pseudoClass = aSelector.mPseudoClassList;
  while (pseudoClass) {
    if ((pseudoClass->mAtom == nsCSSAtoms::activePseudo) ||
        (pseudoClass->mAtom == nsCSSAtoms::anyLinkPseudo) ||
        (pseudoClass->mAtom == nsCSSAtoms::checkedPseudo) ||
        (pseudoClass->mAtom == nsCSSAtoms::disabledPseudo) || 
        (pseudoClass->mAtom == nsCSSAtoms::dragOverPseudo) || 
        (pseudoClass->mAtom == nsCSSAtoms::enabledPseudo) || 
        (pseudoClass->mAtom == nsCSSAtoms::focusPseudo) || 
        (pseudoClass->mAtom == nsCSSAtoms::hoverPseudo) || 
        (pseudoClass->mAtom == nsCSSAtoms::linkPseudo) ||
        (pseudoClass->mAtom == nsCSSAtoms::visitedPseudo)) {
      return PR_TRUE;
    }
    pseudoClass = pseudoClass->mNext;
  }
  return PR_FALSE;
}

static
PRBool BuildStateEnum(nsISupports* aRule, void* aArray)
{
  nsICSSStyleRule* rule = (nsICSSStyleRule*)aRule;
  nsVoidArray* array = (nsVoidArray*)aArray;

  nsCSSSelector* selector = rule->FirstSelector();

  while (selector) {
    if (IsStateSelector(*selector)) {
      array->AppendElement(selector);
    }
    selector = selector->mNext;
  }
  return PR_TRUE;
}

struct CascadeEnumData {
  CascadeEnumData(nsIAtom* aMedium)
    : mMedium(aMedium),
      mRuleArrays(64)
  {
  }

  nsIAtom* mMedium;
  nsSupportsHashtable mRuleArrays; // of nsISupportsArray
};

static PRBool
InsertRuleByWeight(nsISupports* aRule, void* aData)
{
  nsICSSRule* rule = (nsICSSRule*)aRule;
  CascadeEnumData* data = (CascadeEnumData*)aData;
  PRInt32 type = nsICSSRule::UNKNOWN_RULE;
  rule->GetType(type);

  if (nsICSSRule::STYLE_RULE == type) {
    nsICSSStyleRule* styleRule = (nsICSSStyleRule*)rule;

    PRInt32 weight = styleRule->GetWeight();
    nsInt32Key key(weight);
    nsCOMPtr<nsISupportsArray> rules(dont_AddRef(
            NS_STATIC_CAST(nsISupportsArray*, data->mRuleArrays.Get(&key))));
    if (!rules) {
      NS_NewISupportsArray(getter_AddRefs(rules));
      if (!rules) return PR_FALSE; // out of memory
      data->mRuleArrays.Put(&key, rules);
    }
    rules->AppendElement(styleRule);
  }
  else if (nsICSSRule::MEDIA_RULE == type) {
    nsICSSMediaRule* mediaRule = (nsICSSMediaRule*)rule;
    if (mediaRule->UseForMedium(data->mMedium)) {
      mediaRule->EnumerateRulesForwards(InsertRuleByWeight, aData);
    }
  }
  return PR_TRUE;
}


PRBool
CSSRuleProcessor::CascadeSheetRulesInto(nsISupports* aSheet, void* aData)
{
  nsICSSStyleSheet* iSheet = (nsICSSStyleSheet*)aSheet;
  CSSStyleSheetImpl*  sheet = (CSSStyleSheetImpl*)iSheet;
  CascadeEnumData*  data = (CascadeEnumData*)aData;
  PRBool bSheetEnabled = PR_TRUE;
  sheet->GetEnabled(bSheetEnabled);

  if ((bSheetEnabled) && (sheet->UseForMedium(data->mMedium))) {
    CSSStyleSheetImpl*  child = sheet->mFirstChild;
    while (child) {
      CascadeSheetRulesInto((nsICSSStyleSheet*)child, data);
      child = child->mNext;
    }

    if (sheet->mInner && sheet->mInner->mOrderedRules) {
      sheet->mInner->mOrderedRules->EnumerateForwards(InsertRuleByWeight, data);
    }
  }
  return PR_TRUE;
}

struct RuleArrayData {
  PRInt32 mWeight;
  nsISupportsArray* mRuleArray;
};

PR_STATIC_CALLBACK(int) CompareArrayData(const void* aArg1, const void* aArg2,
                                         void* closure)
{
  const RuleArrayData* arg1 = NS_STATIC_CAST(const RuleArrayData*, aArg1);
  const RuleArrayData* arg2 = NS_STATIC_CAST(const RuleArrayData*, aArg2);
  return arg2->mWeight - arg1->mWeight; // put higher weight first
}


struct FillArrayData {
  FillArrayData(RuleArrayData* aArrayData) :
    mIndex(0),
    mArrayData(aArrayData)
  {
  }
  PRInt32 mIndex;
  RuleArrayData* mArrayData;
};

PR_STATIC_CALLBACK(PRBool) FillArray(nsHashKey* aKey, void* aData,
                                     void* aClosure)
{
  nsInt32Key* key = NS_STATIC_CAST(nsInt32Key*, aKey);
  nsISupportsArray* weightArray = NS_STATIC_CAST(nsISupportsArray*, aData);
  FillArrayData* data = NS_STATIC_CAST(FillArrayData*, aClosure);

  RuleArrayData& ruleData = data->mArrayData[data->mIndex++];
  ruleData.mRuleArray = weightArray;
  ruleData.mWeight = key->mInt;

  return PR_TRUE;
}

static PRBool AppendRuleToArray(nsISupports* aElement, void* aData)
{
  nsISupportsArray* ruleArray = NS_STATIC_CAST(nsISupportsArray*, aData);
  ruleArray->AppendElement(aElement);
  return PR_TRUE;
}

/**
 * Takes the hashtable of arrays (keyed by weight, in order sort) and
 * puts them all in one big array which has a primary sort by weight
 * and secondary sort by reverse order.
 */
static void PutRulesInList(nsSupportsHashtable* aRuleArrays,
                           nsISupportsArray* aWeightedRules)
{
  PRInt32 arrayCount = aRuleArrays->Count();
  RuleArrayData* arrayData = new RuleArrayData[arrayCount];
  FillArrayData faData(arrayData);
  aRuleArrays->Enumerate(FillArray, &faData);
  NS_QuickSort(arrayData, arrayCount, sizeof(RuleArrayData),
               CompareArrayData, nsnull);
  PRInt32 i;
  for (i = 0; i < arrayCount; i++) {
    // append the array in reverse
    arrayData[i].mRuleArray->EnumerateBackwards(AppendRuleToArray,
                                                aWeightedRules);
  }

  delete [] arrayData;
}

RuleCascadeData*
CSSRuleProcessor::GetRuleCascade(nsIPresContext* aPresContext, nsIAtom* aMedium)
{
  RuleCascadeData **cascadep = &mRuleCascades;
  RuleCascadeData *cascade;
  while ((cascade = *cascadep)) {
    if (cascade->mMedium == aMedium)
      return cascade;
    cascadep = &cascade->mNext;
  }

  if (mSheets) {
    cascade = new RuleCascadeData(aMedium);
    if (cascade) {
      *cascadep = cascade;

      CascadeEnumData data(aMedium);
      mSheets->EnumerateForwards(CascadeSheetRulesInto, &data);
      PutRulesInList(&data.mRuleArrays, cascade->mWeightedRules);

      nsCompatibility quirkMode = eCompatibility_Standard;
      aPresContext->GetCompatibilityMode(&quirkMode);
      PRBool isQuirksMode = (eCompatibility_Standard == quirkMode ? PR_FALSE : PR_TRUE);

      cascade->mRuleHash.SetCaseSensitive(!isQuirksMode);
      cascade->mWeightedRules->EnumerateBackwards(BuildHashEnum, &(cascade->mRuleHash));
      cascade->mWeightedRules->EnumerateBackwards(BuildStateEnum, &(cascade->mStateSelectors));
    }
  }
  return cascade;
}
