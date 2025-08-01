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

#ifndef nsFontMetricsGTK_h__
#define nsFontMetricsGTK_h__

#include "nsDeviceContextGTK.h"
#include "nsIFontMetrics.h"
#include "nsIFontEnumerator.h"
#include "nsFont.h"
#include "nsString.h"
#include "nsUnitConversion.h"
#include "nsIDeviceContext.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "nsRenderingContextGTK.h"
#include "nsICharRepresentable.h"
#include "nsCompressedCharMap.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#undef FONT_HAS_GLYPH
#define FONT_HAS_GLYPH(map, char) IS_REPRESENTABLE(map, char)
#define WEIGHT_INDEX(weight) (((weight) / 100) - 1)

#define NS_FONT_DEBUG_LOAD_FONT   0x01
#define NS_FONT_DEBUG_CALL_TRACE  0x02
#define NS_FONT_DEBUG_FIND_FONT   0x04
#define NS_FONT_DEBUG_SIZE_FONT   0x08
#define NS_FONT_DEBUG_SCALED_FONT       0x10
#define NS_FONT_DEBUG_BANNED_FONT       0x20
#define NS_FONT_DEBUG_FONT_CATALOG      0x100
#define NS_FONT_DEBUG_FONT_SCAN         0x200
#define NS_FONT_DEBUG_FREETYPE_FONT     0x400
#define NS_FONT_DEBUG_FREETYPE_GRAPHICS 0x800

#undef NS_FONT_DEBUG
#define NS_FONT_DEBUG 1
#ifdef NS_FONT_DEBUG
extern PRUint32 gFontDebug;

# define DEBUG_PRINTF(x) \
         DEBUG_PRINTF_MACRO(x, 0xFFFF)

# define DEBUG_PRINTF_MACRO(x, type) \
            PR_BEGIN_MACRO \
              if (gFontDebug & (type)) { \
                printf x ; \
                printf(", %s %d\n", __FILE__, __LINE__); \
              } \
            PR_END_MACRO
#else
# define DEBUG_PRINTF_MACRO(x, type) \
            PR_BEGIN_MACRO \
            PR_END_MACRO
#endif

#define FIND_FONT_PRINTF(x) \
         DEBUG_PRINTF_MACRO(x, NS_FONT_DEBUG_FIND_FONT)

#define SIZE_FONT_PRINTF(x) \
         DEBUG_PRINTF_MACRO(x, NS_FONT_DEBUG_SIZE_FONT)

#define SCALED_FONT_PRINTF(x) \
         DEBUG_PRINTF_MACRO(x, NS_FONT_DEBUG_SCALED_FONT)

#define BANNED_FONT_PRINTF(x) \
         DEBUG_PRINTF_MACRO(x, NS_FONT_DEBUG_BANNED_FONT)

#define FONT_CATALOG_PRINTF(x) \
         DEBUG_PRINTF_MACRO(x, NS_FONT_DEBUG_FONT_CATALOG)

#define FONT_SCAN_PRINTF(x) \
            PR_BEGIN_MACRO \
              if (gFontDebug & NS_FONT_DEBUG_FONT_SCAN) { \
                printf x ; \
                fflush(stdout); \
              } \
            PR_END_MACRO

#define FREETYPE_FONT_PRINTF(x) \
         DEBUG_PRINTF_MACRO(x, NS_FONT_DEBUG_FREETYPE_FONT)

typedef struct nsFontCharSetInfo nsFontCharSetInfo;

typedef gint (*nsFontCharSetConverter)(nsFontCharSetInfo* aSelf,
  XFontStruct* aFont, const PRUnichar* aSrcBuf, PRInt32 aSrcLen,
  char* aDestBuf, PRInt32 aDestLen);

struct nsFontCharSet;
struct nsFontFamily;
struct nsFontNode;
struct nsFontStretch;
struct nsFontWeight;
class nsXFont;

class nsFontGTKUserDefined;
class nsFontMetricsGTK;
class nsFreeTypeFace;

struct nsFontStretch
{
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  void SortSizes(void);

  nsFontGTK**        mSizes;
  PRUint16           mSizesAlloc;
  PRUint16           mSizesCount;

  char*              mScalable;
  PRBool             mOutlineScaled;
  nsVoidArray        mScaledFonts;
  nsFreeTypeFace *   mFreeTypeFaceID;
};

struct nsFontStyle
{
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  void FillWeightHoles(void);

  nsFontWeight* mWeights[9];
};

struct nsFontWeight
{
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  void FillStretchHoles(void);

  nsFontStretch* mStretches[9];
};

struct nsFontNode
{
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  void FillStyleHoles(void);

  nsCAutoString      mName;
  nsFontCharSetInfo* mCharSetInfo;
  nsFontStyle*       mStyles[3];
  PRUint8            mHolesFilled;
  PRUint8            mDummy;
};

class nsFontNodeArray : public nsAutoVoidArray
{
public:
  nsFontNodeArray() {};

  nsFontNode* GetElement(PRInt32 aIndex)
  {
    return (nsFontNode*) ElementAt(aIndex);
  };
};

/*
 * Font Language Groups
 *
 * These Font Language Groups (FLG) indicate other related
 * encodings to look at when searching for glyphs
 *
 */
typedef struct nsFontLangGroup {
  const char *mFontLangGroupName;
  nsIAtom*    mFontLangGroupAtom;
} nsFontLangGroup;

struct nsFontCharSetMap
{
  const char*        mName;
  nsFontLangGroup*   mFontLangGroup;
  nsFontCharSetInfo* mInfo;
};

class nsFontGTK
{
public:
  nsFontGTK();
  nsFontGTK(nsFontGTK*);
  virtual ~nsFontGTK();
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  void LoadFont(void);
  PRBool IsEmptyFont(XFontStruct*);

  inline int SupportsChar(PRUnichar aChar)
    { return mCCMap && CCMAP_HAS_CHAR(mCCMap, aChar); };

  virtual GdkFont* GetGDKFont(void);
  virtual nsXFont* GetXFont(void);
  virtual PRBool   GetXFontIs10646(void);
  virtual PRBool   IsFreeTypeFont(void);
  virtual gint GetWidth(const PRUnichar* aString, PRUint32 aLength) = 0;
  virtual gint DrawString(nsRenderingContextGTK* aContext,
                          nsDrawingSurfaceGTK* aSurface, nscoord aX,
                          nscoord aY, const PRUnichar* aString,
                          PRUint32 aLength) = 0;
#ifdef MOZ_MATHML
  // bounding metrics for a string 
  // remember returned values are not in app units 
  // - to emulate GetWidth () above
  virtual nsresult
  GetBoundingMetrics(const PRUnichar*   aString,
                     PRUint32           aLength,
                     nsBoundingMetrics& aBoundingMetrics) = 0;
#endif

  PRUint16*              mCCMap;
  nsFontCharSetInfo*     mCharSetInfo;
  char*                  mName;
  nsFontGTKUserDefined*  mUserDefinedFont;
  PRUint16               mSize;
  PRUint16               mAABaseSize;
  PRInt16                mBaselineAdjust;

  // these values are not in app units, they need to be scaled with 
  // nsIDeviceContext::GetDevUnitsToAppUnits()
  PRInt16                mMaxAscent;
  PRInt16                mMaxDescent;

protected:
  GdkFont*               mFont;
  GdkFont*               mFontHolder;
  nsXFont*               mXFont;
  PRBool                 mAlreadyCalledLoadFont;
};

class nsFontMetricsGTK : public nsIFontMetrics
{
public:
  nsFontMetricsGTK();
  virtual ~nsFontMetricsGTK();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  NS_DECL_ISUPPORTS

  NS_IMETHOD  Init(const nsFont& aFont, nsIAtom* aLangGroup,
                   nsIDeviceContext* aContext);
  NS_IMETHOD  Destroy();

  NS_IMETHOD  GetXHeight(nscoord& aResult);
  NS_IMETHOD  GetSuperscriptOffset(nscoord& aResult);
  NS_IMETHOD  GetSubscriptOffset(nscoord& aResult);
  NS_IMETHOD  GetStrikeout(nscoord& aOffset, nscoord& aSize);
  NS_IMETHOD  GetUnderline(nscoord& aOffset, nscoord& aSize);

  NS_IMETHOD  GetHeight(nscoord &aHeight);
  NS_IMETHOD  GetNormalLineHeight(nscoord &aHeight);
  NS_IMETHOD  GetLeading(nscoord &aLeading);
  NS_IMETHOD  GetEmHeight(nscoord &aHeight);
  NS_IMETHOD  GetEmAscent(nscoord &aAscent);
  NS_IMETHOD  GetEmDescent(nscoord &aDescent);
  NS_IMETHOD  GetMaxHeight(nscoord &aHeight);
  NS_IMETHOD  GetMaxAscent(nscoord &aAscent);
  NS_IMETHOD  GetMaxDescent(nscoord &aDescent);
  NS_IMETHOD  GetMaxAdvance(nscoord &aAdvance);
  NS_IMETHOD  GetFont(const nsFont *&aFont);
  NS_IMETHOD  GetLangGroup(nsIAtom** aLangGroup);
  NS_IMETHOD  GetFontHandle(nsFontHandle &aHandle);
  
  NS_IMETHOD  GetSpaceWidth(nscoord &aSpaceWidth);

  nsFontGTK*  FindFont(PRUnichar aChar);
  nsFontGTK*  FindUserDefinedFont(PRUnichar aChar);
  nsFontGTK*  FindStyleSheetSpecificFont(PRUnichar aChar);
  nsFontGTK*  FindStyleSheetGenericFont(PRUnichar aChar);
  nsFontGTK*  FindLangGroupPrefFont(nsIAtom* aLangGroup, PRUnichar aChar);
  nsFontGTK*  FindLangGroupFont(nsIAtom* aLangGroup, PRUnichar aChar, nsCString* aName);
  nsFontGTK*  FindAnyFont(PRUnichar aChar);
  nsFontGTK*  FindSubstituteFont(PRUnichar aChar);

  nsFontGTK*  SearchNode(nsFontNode* aNode, PRUnichar aChar);
  nsFontGTK*  TryAliases(nsCString* aName, PRUnichar aChar);
  nsFontGTK*  TryFamily(nsCString* aName, PRUnichar aChar);
  nsFontGTK*  TryNode(nsCString* aName, PRUnichar aChar);
  nsFontGTK*  TryNodes(nsACString &aFFREName, PRUnichar aChar);
  nsFontGTK*  TryLangGroup(nsIAtom* aLangGroup, nsCString* aName, PRUnichar aChar);

  nsFontGTK*  AddToLoadedFontsList(nsFontGTK* aFont);
  nsFontGTK*  FindNearestSize(nsFontStretch* aStretch, PRUint16 aSize);
  nsFontGTK*  GetAASBBaseFont(nsFontStretch* aStretch, 
                              nsFontCharSetInfo* aCharSet);
  nsFontGTK*  PickASizeAndLoad(nsFontStretch* aStretch,
                               nsFontCharSetInfo* aCharSet, 
                               PRUnichar aChar,
                               const char *aName);

  static nsresult FamilyExists(const nsString& aFontName);

  //friend struct nsFontGTK;

  nsFontGTK   **mLoadedFonts;
  PRUint16    mLoadedFontsAlloc;
  PRUint16    mLoadedFontsCount;

  nsFontGTK   *mSubstituteFont;

  nsCStringArray mFonts;
  PRUint16       mFontsIndex;
  nsAutoVoidArray   mFontIsGeneric;

  nsCAutoString     mDefaultFont;
  nsCString         *mGeneric;
  nsCOMPtr<nsIAtom> mLangGroup;
  nsCAutoString     mUserDefined;

  PRUint8 mTriedAllGenerics;
  PRUint8 mIsUserDefined;

protected:
  void RealizeFont();

  nsIDeviceContext    *mDeviceContext;
  nsFont              *mFont;
  nsFontGTK           *mWesternFont;

  nscoord             mLeading;
  nscoord             mEmHeight;
  nscoord             mEmAscent;
  nscoord             mEmDescent;
  nscoord             mMaxHeight;
  nscoord             mMaxAscent;
  nscoord             mMaxDescent;
  nscoord             mMaxAdvance;
  nscoord             mXHeight;
  nscoord             mSuperscriptOffset;
  nscoord             mSubscriptOffset;
  nscoord             mStrikeoutSize;
  nscoord             mStrikeoutOffset;
  nscoord             mUnderlineSize;
  nscoord             mUnderlineOffset;
  nscoord             mSpaceWidth;

  PRUint16            mPixelSize;
  PRUint8             mStretchIndex;
  PRUint8             mStyleIndex;
  nsFontCharSetConverter mDocConverterType;
};

class nsFontEnumeratorGTK : public nsIFontEnumerator
{
public:
  nsFontEnumeratorGTK();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFONTENUMERATOR
};

class nsHashKey;
PRBool FreeNode(nsHashKey* aKey, void* aData, void* aClosure);
nsFontCharSetInfo *GetCharSetInfo(const char *aCharSetName);
void CharSetNameToCodeRangeBits(const char*, PRUint32*, PRUint32*);
nsFontCharSetMap *GetCharSetMap(const char *aCharSetName);




#endif
