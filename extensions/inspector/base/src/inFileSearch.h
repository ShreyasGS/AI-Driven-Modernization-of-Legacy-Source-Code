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
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Joe Hewitt <hewitt@netscape.com> (original author)
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

#ifndef __inFileSearch_h__
#define __inFileSearch_h__

#include "inIFileSearch.h"

#include "nsString.h"
#include "nsVoidArray.h"
#include "nsISupportsArray.h"
#include "inISearchObserver.h"
#include "nsIFile.h"
#include "inSearchLoop.h"

class inFileSearch : public inIFileSearch
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_INISEARCHPROCESS
  NS_DECL_INIFILESEARCH

  inFileSearch();
  virtual ~inFileSearch();

protected:
  // inISearchProcess related
  PRBool mIsActive;
  PRInt32 mResultCount;
  nsCOMPtr<nsIFile> mLastResult;
  nsCOMPtr<nsISupportsArray> mResults;
  PRBool mHoldResults;
  nsAutoString* mBasePath;
  PRBool mReturnRelativePaths;
  nsCOMPtr<inISearchObserver> mObserver;

  // inIFileSearch related
  nsCOMPtr<nsIFile> mSearchPath;
  nsAutoString* mTextCriteria;
  PRUnichar** mFilenameCriteria;
  PRUint32 mFilenameCriteriaCount;
  PRBool mSearchRecursive;
  PRUint32 mDirsSearched;

  // asynchronous search related
  nsCOMPtr<nsISupportsArray> mDirStack;
  inSearchLoop* mSearchLoop;

  // life cycle of search
  nsresult InitSearch();
  nsresult KillSearch(PRInt16 aResult);
  nsresult SearchDirectory(nsIFile* aDir, PRBool aIsSync);
  nsresult PrepareResult(nsIFile* aFile, PRBool aIsSync);

  // asynchronous search helpers
  nsresult InitSearchLoop();
  nsresult InitSubDirectoryStack();
  PRBool GetNextSubDirectory(nsIFile** aDir);
  nsresult PushSubDirectoryOnStack(nsIFile* aDir);
  nsIFile* GetNextDirectory(nsISimpleEnumerator* aEnum);

  // pattern matching
  PRBool MatchFile(nsIFile* aFile);
  static PRBool MatchPattern(PRUnichar* aPattern, PRUnichar* aString);
  static PRBool AdvanceWildcard(PRUnichar** aString, PRUnichar* aNextChar);
  
  // misc
  nsresult MakePathRelative(nsAString& aPath);
  nsresult CountDirectoryDepth(nsIFile* aDir, PRUint32* aDepth);

};

#endif // __inFileSearch_h__
