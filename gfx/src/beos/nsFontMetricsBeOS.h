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
 *   Paul Ashford <arougthopher@lizardland.net>
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

#ifndef nsFontMetricsBeOS_h__
#define nsFontMetricsBeOS_h__

#include "nsDeviceContextBeOS.h" 
#include "nsIFontMetrics.h" 
#include "nsIFontEnumerator.h" 
#include "nsFont.h"
#include "nsString.h"
#include "nsUnitConversion.h"
#include "nsIDeviceContext.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "nsRenderingContextBeOS.h" 
#include "nsICharRepresentable.h" 

#include <Font.h>

class nsFontMetricsBeOS : public nsIFontMetrics
{
public:
  nsFontMetricsBeOS();
  virtual ~nsFontMetricsBeOS();

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
 
  static nsresult FamilyExists(const nsString& aFontName); 
  inline PRBool   GetEmulateBold() { return mEmulateBold; }
 
  nsCOMPtr<nsIAtom>   mLangGroup; 
 
protected:
  void RealizeFont(nsIDeviceContext* aContext);

  nsIDeviceContext    *mDeviceContext;
  nsFont              *mFont;
  BFont               mFontHandle;

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
  
  PRBool              mEmulateBold;
}; 
 
class nsFontEnumeratorBeOS : public nsIFontEnumerator 
{ 
public: 
  nsFontEnumeratorBeOS(); 
  NS_DECL_ISUPPORTS 
  NS_DECL_NSIFONTENUMERATOR 
};

#endif
