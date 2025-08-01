/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Roland Mainz <Roland.Mainz@informatik.med.uni-giessen.de>
 *   IBM Corp.
 */

#include "nsDeviceContext.h"
#include "nsFont.h"
#include "nsIView.h"
#include "nsGfxCIID.h"
#include "nsVoidArray.h"
#include "nsIFontMetrics.h"
#include "nsHashtable.h"
#include "nsILanguageAtomService.h"
#include "nsIServiceManager.h"
#include "nsUnicharUtils.h"
#include "nsCRT.h"


NS_IMPL_ISUPPORTS3(DeviceContextImpl, nsIDeviceContext, nsIObserver, nsISupportsWeakReference)

DeviceContextImpl::DeviceContextImpl()
{
  NS_INIT_REFCNT();
  mFontCache = nsnull;
  mDevUnitsToAppUnits = 1.0f;
  mAppUnitsToDevUnits = 1.0f;
  mGammaValue = 1.0f;
  mCPixelScale = 1.0f;
  mGammaTable = new PRUint8[256];
  mZoom = 1.0f;
  mTextZoom = 1.0f;
  mWidget = nsnull;
  mFontAliasTable = nsnull;

#ifdef NS_PRINT_PREVIEW
  mUseAltDC = kUseAltDCFor_NONE;
#endif
#ifdef NS_DEBUG
  mInitialized = PR_FALSE;
#endif
}

static PRBool PR_CALLBACK DeleteValue(nsHashKey* aKey, void* aValue, void* closure)
{
  delete ((nsString*)aValue);
  return PR_TRUE;
}

DeviceContextImpl::~DeviceContextImpl()
{
  nsCOMPtr<nsIObserverService> obs(do_GetService("@mozilla.org/observer-service;1"));
  if (obs)
    obs->RemoveObserver(this, "memory-pressure");

  if (nsnull != mFontCache)
  {
    delete mFontCache;
    mFontCache = nsnull;
  }

  if (nsnull != mGammaTable)
  {
    delete[] mGammaTable;
    mGammaTable = nsnull;
  }

  if (nsnull != mFontAliasTable) {
    mFontAliasTable->Enumerate(DeleteValue);
    delete mFontAliasTable;
  }

}

NS_IMETHODIMP
DeviceContextImpl::Observe(nsISupports* aSubject, const char* aTopic, const PRUnichar* aSomeData)
{
  if (mFontCache && !nsCRT::strcmp(aTopic, "memory-pressure")) {
    mFontCache->Compact();
  }
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl::Init(nsNativeWidget aWidget)
{
  mWidget = aWidget;

  CommonInit();

  return NS_OK;
}

void DeviceContextImpl::CommonInit(void)
{
#ifdef NS_DEBUG
  NS_ASSERTION(!mInitialized, "device context is initialized twice!");
  mInitialized = PR_TRUE;
#endif

  for (PRInt32 cnt = 0; cnt < 256; cnt++)
    mGammaTable[cnt] = cnt;

  // register as a memory-pressure observer to free font resources
  // in low-memory situations.
  nsCOMPtr<nsIObserverService> obs(do_GetService("@mozilla.org/observer-service;1"));
  if (obs)
    obs->AddObserver(this, "memory-pressure", PR_TRUE);
}

NS_IMETHODIMP DeviceContextImpl::GetTwipsToDevUnits(float &aTwipsToDevUnits) const
{
  aTwipsToDevUnits = mTwipsToPixels;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl::GetDevUnitsToTwips(float &aDevUnitsToTwips) const
{
  aDevUnitsToTwips = mPixelsToTwips;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl::SetAppUnitsToDevUnits(float aAppUnits)
{
  mAppUnitsToDevUnits = aAppUnits;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl::SetDevUnitsToAppUnits(float aDevUnits)
{
  mDevUnitsToAppUnits = aDevUnits;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl::GetAppUnitsToDevUnits(float &aAppUnits) const
{
  aAppUnits = mAppUnitsToDevUnits;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl::GetDevUnitsToAppUnits(float &aDevUnits) const
{
  aDevUnits = mDevUnitsToAppUnits;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl::GetCanonicalPixelScale(float &aScale) const
{
  aScale = mCPixelScale;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl::SetCanonicalPixelScale(float aScale)
{
  mCPixelScale = aScale;
  return NS_OK;
}

static NS_DEFINE_CID(kRenderingContextCID, NS_RENDERING_CONTEXT_CID);

NS_IMETHODIMP DeviceContextImpl::CreateRenderingContext(nsIView *aView, nsIRenderingContext *&aContext)
{
#ifdef NS_PRINT_PREVIEW
  // AltDC NEVER use widgets to create their DCs
  if (mAltDC && (mUseAltDC & kUseAltDCFor_CREATERC_PAINT)) {
    return mAltDC->CreateRenderingContext(aContext);
  }
#endif

  nsresult   rv;
  nsIWidget *win;
  aView->GetWidget(win);

  aContext = nsnull;
  nsCOMPtr<nsIRenderingContext> pContext = do_CreateInstance(kRenderingContextCID, &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = InitRenderingContext(pContext, win);
    if (NS_SUCCEEDED(rv)) {
      aContext = pContext;
      NS_ADDREF(aContext);
    }
  }
  
  NS_IF_RELEASE(win);
  return rv;
}

NS_IMETHODIMP DeviceContextImpl::CreateRenderingContext(nsIWidget *aWidget, nsIRenderingContext *&aContext)
{
  nsresult rv;

#ifdef NS_PRINT_PREVIEW
  // AltDC NEVER use widgets to create their DCs
  // NOTE: The mAltDC will call it;s own init
  // so we can return here
  if (mAltDC && (mUseAltDC & kUseAltDCFor_CREATERC_REFLOW)) {
    return mAltDC->CreateRenderingContext(aContext);
  }
#endif

  aContext = nsnull;
  nsCOMPtr<nsIRenderingContext> pContext = do_CreateInstance(kRenderingContextCID, &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = InitRenderingContext(pContext, aWidget);
    if (NS_SUCCEEDED(rv)) {
      aContext = pContext;
      NS_ADDREF(aContext);
    }
  }    
  
  return rv;
}

NS_IMETHODIMP DeviceContextImpl::InitRenderingContext(nsIRenderingContext *aContext, nsIWidget *aWin)
{
#ifdef NS_PRINT_PREVIEW
  // there are a couple of cases where the kUseAltDCFor_CREATERC_xxx flag has been turned off
  // but we still need to initialize with the Alt DC
  if (mAltDC) {
    return aContext->Init(mAltDC, aWin);
  } else {
    return aContext->Init(this, aWin);
  }
#else
  return aContext->Init(this, aWin);
#endif
}

NS_IMETHODIMP DeviceContextImpl::CreateFontCache()
{
  mFontCache = new nsFontCache();
  if (!mFontCache) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return mFontCache->Init(this);
}

NS_IMETHODIMP DeviceContextImpl::FontMetricsDeleted(const nsIFontMetrics* aFontMetrics)
{
  if (mFontCache) {
    mFontCache->FontMetricsDeleted(aFontMetrics);
  }
  return NS_OK;
}

void
DeviceContextImpl::GetLocaleLangGroup(void)
{
  if (!mLocaleLangGroup) {
    nsCOMPtr<nsILanguageAtomService> langService;
    langService = do_GetService(NS_LANGUAGEATOMSERVICE_CONTRACTID);
    if (langService) {
      langService->GetLocaleLanguageGroup(getter_AddRefs(mLocaleLangGroup));
    }
    if (!mLocaleLangGroup) {
      mLocaleLangGroup = getter_AddRefs(NS_NewAtom("x-western"));
    }
  }
}

NS_IMETHODIMP DeviceContextImpl::GetMetricsFor(const nsFont& aFont,
  nsIAtom* aLangGroup, nsIFontMetrics*& aMetrics)
{
#ifdef NS_PRINT_PREVIEW
  // Defer to Alt when there is one
  if (mAltDC != nsnull && (mUseAltDC & kUseAltDCFor_FONTMETRICS)) {
    return mAltDC->GetMetricsFor(aFont, aLangGroup, aMetrics);
  }
#endif

  if (nsnull == mFontCache) {
    nsresult  rv = CreateFontCache();
    if (NS_FAILED(rv)) {
      aMetrics = nsnull;
      return rv;
    }
    // XXX temporary fix for performance problem -- erik
    GetLocaleLangGroup();
  }

  // XXX figure out why aLangGroup is NULL sometimes
  if (!aLangGroup) {
    aLangGroup = mLocaleLangGroup;
  }

  return mFontCache->GetMetricsFor(aFont, aLangGroup, aMetrics);
}

NS_IMETHODIMP DeviceContextImpl::GetMetricsFor(const nsFont& aFont, nsIFontMetrics*& aMetrics)
{
#ifdef NS_PRINT_PREVIEW
  // Defer to Alt when there is one
  if (mAltDC != nsnull && (mUseAltDC & kUseAltDCFor_FONTMETRICS)) {
    return mAltDC->GetMetricsFor(aFont, aMetrics);
  }
#endif

  if (nsnull == mFontCache) {
    nsresult  rv = CreateFontCache();
    if (NS_FAILED(rv)) {
      aMetrics = nsnull;
      return rv;
    }
    // XXX temporary fix for performance problem -- erik
    GetLocaleLangGroup();
  }
  return mFontCache->GetMetricsFor(aFont, mLocaleLangGroup, aMetrics);
}

NS_IMETHODIMP DeviceContextImpl::SetZoom(float aZoom)
{
  mZoom = aZoom;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl::GetZoom(float &aZoom) const
{
  aZoom = mZoom;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl::SetTextZoom(float aTextZoom)
{
  mTextZoom = aTextZoom;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl::GetTextZoom(float &aTextZoom) const
{
  aTextZoom = mTextZoom;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl::GetGamma(float &aGamma)
{
  aGamma = mGammaValue;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl::SetGamma(float aGamma)
{
  if (aGamma != mGammaValue)
  {
    //we don't need to-recorrect existing images for this case
    //so pass in 1.0 for the current gamma regardless of what it
    //really happens to be. existing images will get a one time
    //re-correction when they're rendered the next time. MMP

    SetGammaTable(mGammaTable, 1.0f, aGamma);

    mGammaValue = aGamma;
  }
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl::GetGammaTable(PRUint8 *&aGammaTable)
{
  //XXX we really need to ref count this somehow. MMP
  aGammaTable = mGammaTable;
  return NS_OK;
}

void DeviceContextImpl::SetGammaTable(PRUint8 * aTable, float aCurrentGamma, float aNewGamma)
{
  double fgval = (1.0f / aCurrentGamma) * (1.0f / aNewGamma);

  for (PRInt32 cnt = 0; cnt < 256; cnt++)
    aTable[cnt] = (PRUint8)(pow((double)cnt * (1. / 256.), fgval) * 255.99999999);
}

NS_IMETHODIMP DeviceContextImpl::GetDepth(PRUint32& aDepth)
{
  aDepth = 24;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl::GetPaletteInfo(nsPaletteInfo& aPaletteInfo)
{
  aPaletteInfo.isPaletteDevice = PR_FALSE;
  aPaletteInfo.sizePalette = 0;
  aPaletteInfo.numReserved = 0;
  aPaletteInfo.palette = nsnull;
  return NS_OK;
}

struct FontEnumData {
  FontEnumData(nsIDeviceContext* aDC, nsString& aFaceName)
    : mDC(aDC), mFaceName(aFaceName)
  {}
  nsIDeviceContext* mDC;
  nsString&         mFaceName;
};

static PRBool FontEnumCallback(const nsString& aFamily, PRBool aGeneric, void *aData)
{
  FontEnumData* data = (FontEnumData*)aData;
  // XXX for now, all generic fonts are presumed to exist
  //     we may want to actually check if there's an installed conversion
  if (aGeneric) {
    data->mFaceName = aFamily;
    return PR_FALSE; // found one, stop.
  }
  else {
    nsAutoString  local;
    PRBool        aliased;
    data->mDC->GetLocalFontName(aFamily, local, aliased);
    if (aliased || (NS_SUCCEEDED(data->mDC->CheckFontExistence(local)))) {
      data->mFaceName = local;
      return PR_FALSE; // found one, stop.
    }
  }
  return PR_TRUE; // didn't exist, continue looking
}

NS_IMETHODIMP DeviceContextImpl::FirstExistingFont(const nsFont& aFont, nsString& aFaceName)
{
  FontEnumData  data(this, aFaceName);
  if (aFont.EnumerateFamilies(FontEnumCallback, &data)) {
    return NS_ERROR_FAILURE;  // ran out
  }
  return NS_OK;
}

class FontAliasKey: public nsHashKey 
{
public:
  FontAliasKey(const nsString& aString)
  {mString.Assign(aString);}

  virtual PRUint32 HashCode(void) const;
  virtual PRBool Equals(const nsHashKey *aKey) const;
  virtual nsHashKey *Clone(void) const;

  nsString  mString;
};

PRUint32 FontAliasKey::HashCode(void) const
{
  PRUint32 hash = 0;
  const PRUnichar* string = mString.get();
  PRUnichar ch;
  while ((ch = *string++) != 0) {
    // FYI: hash = hash*37 + ch
    ch = ToUpperCase(ch);
    hash = ((hash << 5) + (hash << 2) + hash) + ch;
  }
  return hash;
}

PRBool FontAliasKey::Equals(const nsHashKey *aKey) const
{
  return mString.Equals(((FontAliasKey*)aKey)->mString, nsCaseInsensitiveStringComparator());
}

nsHashKey* FontAliasKey::Clone(void) const
{
  return new FontAliasKey(mString);
}
nsresult DeviceContextImpl::CreateFontAliasTable()
{
  nsresult result = NS_OK;

  if (nsnull == mFontAliasTable) {
    mFontAliasTable = new nsHashtable();
    if (nsnull != mFontAliasTable) {

      nsAutoString  times;              times.Assign(NS_LITERAL_STRING("Times"));
      nsAutoString  timesNewRoman;      timesNewRoman.Assign(NS_LITERAL_STRING("Times New Roman"));
      nsAutoString  timesRoman;         timesRoman.Assign(NS_LITERAL_STRING("Times Roman"));
      nsAutoString  arial;              arial.Assign(NS_LITERAL_STRING("Arial"));
      nsAutoString  helvetica;          helvetica.Assign(NS_LITERAL_STRING("Helvetica"));
      nsAutoString  courier;            courier.Assign(NS_LITERAL_STRING("Courier"));
      nsAutoString  courierNew;         courierNew.Assign(NS_LITERAL_STRING("Courier New"));
      nsAutoString  nullStr;

      AliasFont(times, timesNewRoman, timesRoman, PR_FALSE);
      AliasFont(timesRoman, timesNewRoman, times, PR_FALSE);
      AliasFont(timesNewRoman, timesRoman, times, PR_FALSE);
      AliasFont(arial, helvetica, nullStr, PR_FALSE);
      AliasFont(helvetica, arial, nullStr, PR_FALSE);
      AliasFont(courier, courierNew, nullStr, PR_TRUE);
      AliasFont(courierNew, courier, nullStr, PR_FALSE);
    }
    else {
      result = NS_ERROR_OUT_OF_MEMORY;
    }
  }
  return result;
}

nsresult DeviceContextImpl::AliasFont(const nsString& aFont, 
                                      const nsString& aAlias, const nsString& aAltAlias,
                                      PRBool aForceAlias)
{
  nsresult result = NS_OK;

  if (nsnull != mFontAliasTable) {
    if (aForceAlias || NS_FAILED(CheckFontExistence(aFont))) {
      if (NS_SUCCEEDED(CheckFontExistence(aAlias))) {
        nsString* entry = new nsString(aAlias);
        if (nsnull != entry) {
          FontAliasKey key(aFont);
          mFontAliasTable->Put(&key, entry);
        }
        else {
          result = NS_ERROR_OUT_OF_MEMORY;
        }
      }
      else if ((0 < aAltAlias.Length()) && NS_SUCCEEDED(CheckFontExistence(aAltAlias))) {
        nsString* entry = new nsString(aAltAlias);
        if (nsnull != entry) {
          FontAliasKey key(aFont);
          mFontAliasTable->Put(&key, entry);
        }
        else {
          result = NS_ERROR_OUT_OF_MEMORY;
        }
      }
    }
  }
  else {
    result = NS_ERROR_FAILURE;
  }
  return result;
}

NS_IMETHODIMP DeviceContextImpl::GetLocalFontName(const nsString& aFaceName, nsString& aLocalName,
                                                  PRBool& aAliased)
{
  nsresult result = NS_OK;

  if (nsnull == mFontAliasTable) {
    result = CreateFontAliasTable();
  }

  if (nsnull != mFontAliasTable) {
    FontAliasKey key(aFaceName);
    const nsString* alias = (const nsString*)mFontAliasTable->Get(&key);
    if (nsnull != alias) {
      aLocalName = *alias;
      aAliased = PR_TRUE;
    }
    else {
      aLocalName = aFaceName;
      aAliased = PR_FALSE;
    }
  }
  return result;
}

NS_IMETHODIMP DeviceContextImpl::FlushFontCache(void)
{
  if (nsnull != mFontCache)
    mFontCache->Flush();

  return NS_OK;
}

#ifdef NS_PRINT_PREVIEW
NS_IMETHODIMP DeviceContextImpl::SetAltDevice(nsIDeviceContext* aAltDC)
{
  mAltDC = aAltDC;

  // Can't use it if it isn't there
  if (aAltDC == nsnull) {
    mUseAltDC = kUseAltDCFor_NONE;
  }
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl::SetUseAltDC(PRUint8 aValue, PRBool aOn)
{
  if (aOn) {
    mUseAltDC |= aValue;
  } else {
    mUseAltDC &= ~aValue;
  }
  return NS_OK;
}
#endif

/////////////////////////////////////////////////////////////

MOZ_DECL_CTOR_COUNTER(nsFontCache)

nsFontCache::nsFontCache()
{
  MOZ_COUNT_CTOR(nsFontCache);
  mContext = nsnull;
}

nsFontCache::~nsFontCache()
{
  MOZ_COUNT_DTOR(nsFontCache);
  Flush();
}

NS_IMETHODIMP 
nsFontCache::Init(nsIDeviceContext* aContext)
{
  NS_PRECONDITION(nsnull != aContext, "null ptr");
  // Note: we don't hold a reference to the device context, because it
  // holds a reference to us and we don't want circular references
  mContext = aContext;
  return NS_OK;
}

NS_IMETHODIMP 
nsFontCache::GetDeviceContext(nsIDeviceContext *&aContext) const
{
  aContext = mContext;
  NS_IF_ADDREF(aContext);
  return NS_OK;
}

NS_IMETHODIMP 
nsFontCache::GetMetricsFor(const nsFont& aFont, nsIAtom* aLangGroup,
  nsIFontMetrics *&aMetrics)
{
  // First check our cache
  // start from the end, which is where we put the most-recent-used element

  nsIFontMetrics* fm;
  PRInt32 n = mFontMetrics.Count() - 1;
  for (PRInt32 i = n; i >= 0; --i) {
    fm = NS_STATIC_CAST(nsIFontMetrics*, mFontMetrics[i]);
    const nsFont* font;
    fm->GetFont(font);
    if (font->Equals(aFont)) {
      nsCOMPtr<nsIAtom> langGroup;
      fm->GetLangGroup(getter_AddRefs(langGroup));
      if (aLangGroup == langGroup.get()) {
        if (i != n) {
          // promote it to the end of the cache
          mFontMetrics.MoveElement(i, n);
        }
        NS_ADDREF(aMetrics = fm);
        return NS_OK;
      }
    }
  }

  // It's not in the cache. Get font metrics and then cache them.

  aMetrics = nsnull;
  nsresult rv = CreateFontMetricsInstance(&fm);
  if (NS_FAILED(rv)) return rv;
  rv = fm->Init(aFont, aLangGroup, mContext);
  if (NS_SUCCEEDED(rv)) {
    // the mFontMetrics list has the "head" at the end, because append is
    // cheaper than insert
    mFontMetrics.AppendElement(fm);
    aMetrics = fm;
    NS_ADDREF(aMetrics);
    return NS_OK;
  }
  fm->Destroy();
  NS_RELEASE(fm);

  // One reason why Init() fails is because the system is running out of resources. 
  // e.g., on Win95/98 only a very limited number of GDI objects are available.
  // Compact the cache and try again.

  Compact();
  rv = CreateFontMetricsInstance(&fm);
  if (NS_FAILED(rv)) return rv;
  rv = fm->Init(aFont, aLangGroup, mContext);
  if (NS_SUCCEEDED(rv)) {
    mFontMetrics.AppendElement(fm);
    aMetrics = fm;
    NS_ADDREF(aMetrics);
    return NS_OK;
  }
  fm->Destroy();
  NS_RELEASE(fm);

  // could not setup a new one, send an old one (XXX search a "best match"?)

  n = mFontMetrics.Count() - 1; // could have changed in Compact()
  if (n >= 0) {
    aMetrics = NS_STATIC_CAST(nsIFontMetrics*, mFontMetrics[n]);
    NS_ADDREF(aMetrics);
    return NS_OK;
  }

  NS_POSTCONDITION(NS_SUCCEEDED(rv), "font metrics should not be null - bug 136248");
  return rv;
}

/* PostScript and Xprint module may override this method to create 
 * nsIFontMetrics objects with their own classes 
 */
NS_IMETHODIMP
nsFontCache::CreateFontMetricsInstance(nsIFontMetrics** fm)
{
  static NS_DEFINE_CID(kFontMetricsCID, NS_FONT_METRICS_CID);
  return CallCreateInstance(kFontMetricsCID, fm);
}

nsresult nsFontCache::FontMetricsDeleted(const nsIFontMetrics* aFontMetrics)
{
  mFontMetrics.RemoveElement((void*)aFontMetrics);
  return NS_OK;
}

nsresult nsFontCache::Compact()
{
  // Need to loop backward because the running element can be removed on the way
  for (PRInt32 i = mFontMetrics.Count()-1; i >= 0; --i) {
    nsIFontMetrics* fm = NS_STATIC_CAST(nsIFontMetrics*, mFontMetrics[i]);
    nsIFontMetrics* oldfm = fm;
    // Destroy() isn't here because we want our device context to be notified
    NS_RELEASE(fm); // this will reset fm to nsnull
    // if the font is really gone, it would have called back in
    // FontMetricsDeleted() and would have removed itself
    if (mFontMetrics.IndexOf(oldfm) >= 0) {
      // nope, the font is still there, so let's hold onto it too
      NS_ADDREF(oldfm);
    }
  }
  return NS_OK;
}

nsresult nsFontCache::Flush()
{
  for (PRInt32 i = mFontMetrics.Count()-1; i >= 0; --i) {
    nsIFontMetrics* fm = NS_STATIC_CAST(nsIFontMetrics*, mFontMetrics[i]);
    // Destroy() will unhook our device context from the fm so that we won't
    // waste time in triggering the notification of FontMetricsDeleted()
    // in the subsequent release
    fm->Destroy();
    NS_RELEASE(fm);
  }

  mFontMetrics.Clear();

  return NS_OK;
}

