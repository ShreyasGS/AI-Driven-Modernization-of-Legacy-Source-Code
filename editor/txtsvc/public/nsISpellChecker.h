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

#ifndef nsISpellChecker_h__
#define nsISpellChecker_h__

#include "nsISupports.h"

#define NS_SPELLCHECKER_CONTRACTID "@mozilla.org/spellchecker;1"

#define NS_ISPELLCHECKER_IID                    \
{ /* F72A52F1-F83B-11d2-8D54-000064657374 */    \
0xf72a52f1, 0xf83b, 0x11d2,                     \
{ 0x8d, 0x54, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74 } }

class nsITextServicesDocument;
class nsString;
class nsStringArray;

/**
 * A generic interface for a spelling checker.
 */
class nsISpellChecker  : public nsISupports{
public:

  static const nsIID& GetIID() { static nsIID iid = NS_ISPELLCHECKER_IID; return iid; }

  /**
   * Tells the spellchecker what document to check.
   * @param aDoc is the document to check.
   * @param aFromStartOfDoc If true, start check from beginning of document,
   * if false, start check from current cursor position.
   */
  NS_IMETHOD SetDocument(nsITextServicesDocument *aDoc, PRBool aFromStartofDoc) = 0;

  /**
   * Selects (hilites) the next misspelled word in the document.
   * @param aWord will contain the misspelled word.
   * @param aSuggestions is an array of nsStrings, that represent the
   * suggested replacements for the misspelled word.
   */
  NS_IMETHOD NextMisspelledWord(nsString *aWord, nsStringArray *aSuggestions) = 0;

  /**
   * Checks if a word is misspelled. No document is required to use this method.
   * @param aWord is the word to check.
   * @param aIsMisspelled will be set to true if the word is misspelled.
   * @param aSuggestions is an array of nsStrings which represent the
   * suggested replacements for the misspelled word. The array will be empty
   * if there aren't any suggestions.
   */
  NS_IMETHOD CheckWord(const nsString *aWord, PRBool *aIsMisspelled, nsStringArray *aSuggestions) = 0;

  /**
   * Replaces the old word with the specified new word.
   * @param aOldWord is the word to be replaced.
   * @param aNewWord is the word that is to replace old word.
   * @param aAllOccurrences will replace all occurrences of old
   * word, in the document, with new word when it is true. If
   * false, it will replace the 1st occurrence only!
   */
  NS_IMETHOD Replace(const nsString *aOldWord, const nsString *aNewWord, PRBool aAllOccurrences) = 0;

  /**
   * Ignores all occurrences of the specified word in the document.
   * @param aWord is the word to ignore.
   */
  NS_IMETHOD IgnoreAll(const nsString *aWord) = 0;

  /**
   * Add a word to the user's personal dictionary.
   * @param aWord is the word to add.
   */
  NS_IMETHOD AddWordToPersonalDictionary(const nsString *aWord) = 0;

  /**
   * Remove a word from the user's personal dictionary.
   * @param aWord is the word to remove.
   */
  NS_IMETHOD RemoveWordFromPersonalDictionary(const nsString *aWord) = 0;

  /**
   * Returns the list of words in the user's personal dictionary.
   * @param aWordList is an array of nsStrings that represent the
   * list of words in the user's personal dictionary.
   */
  NS_IMETHOD GetPersonalDictionary(nsStringArray *aWordList) = 0;

  /**
   * Returns the list of strings representing the dictionaries
   * the spellchecker supports. It was suggested that the strings
   * returned be in the RFC 1766 format. This format looks something
   * like <ISO 639 language code>-<ISO 3166 country code>.
   * For example: en-US
   * @param aDictionaryList is an array of nsStrings that represent the
   * dictionaries supported by the spellchecker.
   */
  NS_IMETHOD GetDictionaryList(nsStringArray *aDictionaryList) = 0;

  /**
   * Returns a string representing the current dictionary.
   * @param aDictionary will contain the name of the dictionary.
   * This name is the same string that is in the list returned
   * by GetDictionaryList().
   */
  NS_IMETHOD GetCurrentDictionary(nsString *aDictionary) = 0;

  /**
   * Tells the spellchecker to use a specific dictionary.
   * @param aDictionary a string that is in the list returned
   * by GetDictionaryList().
   */
  NS_IMETHOD SetCurrentDictionary(const nsString *aDictionary) = 0;
};

#endif // nsISpellChecker_h__

