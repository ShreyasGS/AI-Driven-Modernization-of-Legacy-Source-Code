/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 *   Darin Fisher <darin@netscape.com> (original author)
 *   Andreas Otte <andreas.otte@debitel.net>
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

#include "nsStandardURL.h"
#include "nsURLHelper.h"
#include "nsDependentSubstring.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "nsEscape.h"
#include "nsILocalFile.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsICharsetConverterManager2.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranchInternal.h"
#include "nsIIDNService.h"
#include "nsNetUtil.h"
#include "prlog.h"

static NS_DEFINE_CID(kThisImplCID, NS_THIS_STANDARDURL_IMPL_CID);

nsIURLParser *nsStandardURL::gNoAuthParser = nsnull;
nsIURLParser *nsStandardURL::gAuthParser = nsnull;
nsIURLParser *nsStandardURL::gStdParser = nsnull;
nsIIDNService *nsStandardURL::gIDNService = nsnull;
nsICharsetConverterManager2 *nsStandardURL::gCharsetMgr = nsnull;
PRBool nsStandardURL::gInitialized = PR_FALSE;
PRBool nsStandardURL::gEscapeUTF8 = PR_TRUE;

#if defined(PR_LOGGING)
//
// setenv NSPR_LOG_MODULES nsStandardURL:5
//
static PRLogModuleInfo *gStandardURLLog;
#endif
#define LOG(args)     PR_LOG(gStandardURLLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(gStandardURLLog, PR_LOG_DEBUG)

//----------------------------------------------------------------------------

#define ENSURE_MUTABLE() \
  PR_BEGIN_MACRO \
    if (!mMutable) { \
        NS_ERROR("attempt to modify an immutable nsStandardURL"); \
        return NS_ERROR_ABORT; \
    } \
  PR_END_MACRO

//----------------------------------------------------------------------------

static nsresult
EncodeString(nsIUnicodeEncoder *encoder, const nsAFlatString &str, nsACString &result)
{
    nsresult rv;
    PRInt32 len = str.Length();
    PRInt32 maxlen;

    rv = encoder->GetMaxLength(str.get(), len, &maxlen);
    if (NS_FAILED(rv))
        return rv;

    char buf[256], *p = buf;
    if (PRUint32(maxlen) > sizeof(buf) - 1) {
        p = (char *) malloc(maxlen + 1);
        if (!p)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    rv = encoder->Convert(str.get(), &len, p, &maxlen);
    if (NS_FAILED(rv))
        goto end;
    if (rv == NS_ERROR_UENC_NOMAPPING) {
        NS_WARNING("unicode conversion failed");
        rv = NS_ERROR_UNEXPECTED;
        goto end;
    }
    p[maxlen] = 0;
    result = p;

    rv = encoder->Finish(p, &len);
    if (NS_FAILED(rv))
        goto end;
    p[len] = 0;
    result += p;

end:
    encoder->Reset();

    if (p != buf)
        free(p);
    return rv;
}

// filter out \t\r\n
static const char *
FilterString(const char *str, nsCString &result)
{
    PRBool writing = PR_FALSE;
    result.Truncate();
    const char *p = str;

    // Remove leading spaces, tabs, CR, LF if any.
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
        writing = PR_TRUE;
        str = p + 1;
        p++;
    }

    for (; *p; ++p) {
        if (*p == '\t' || *p == '\r' || *p == '\n') {
            writing = PR_TRUE;
            // append chars up to but not including *p
            if (p > str)
                result.Append(str, p - str);
            str = p + 1;
        }
    }

    // Remove trailing spaces if any
    while (((p-1) >= str) && (*(p-1) == ' ')) {
        writing = PR_TRUE;
        p--;
    }

    if (writing && p > str)
        result.Append(str, p - str);

    return writing ? result.get() : str;
}

//----------------------------------------------------------------------------
// nsStandardURL::nsPrefObserver
//----------------------------------------------------------------------------

#define NS_NET_PREF_ESCAPEUTF8 "network.standard-url.escape-utf8"
#define NS_NET_PREF_ENABLEIDN  "network.enableIDN"

NS_IMPL_ISUPPORTS1(nsStandardURL::nsPrefObserver, nsIObserver)

NS_IMETHODIMP nsStandardURL::
nsPrefObserver::Observe(nsISupports *subject,
                        const char *topic,
                        const PRUnichar *data)
{
    if (!strcmp(topic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
        nsCOMPtr<nsIPrefBranch> prefBranch( do_QueryInterface(subject) );
        if (prefBranch) {
            if (!nsCRT::strcmp(data, NS_LITERAL_STRING(NS_NET_PREF_ESCAPEUTF8).get())) {
                PRBool val;
                if (NS_SUCCEEDED(prefBranch->GetBoolPref(NS_NET_PREF_ESCAPEUTF8, &val)))
                    gEscapeUTF8 = val;
                printf("escape UTF-8 %s\n", gEscapeUTF8 ? "enabled" : "disabled");
            }
            else if (!nsCRT::strcmp(data, NS_LITERAL_STRING(NS_NET_PREF_ENABLEIDN).get())) {
                PRBool val;
                NS_IF_RELEASE(gIDNService);
                if (NS_SUCCEEDED(prefBranch->GetBoolPref(NS_NET_PREF_ENABLEIDN, &val)) && val) {
                    nsCOMPtr<nsIIDNService> serv(do_GetService(NS_IDNSERVICE_CONTRACTID));
                    if (serv)
                        NS_ADDREF(gIDNService = serv.get());
                }
                printf("IDN support %s\n", gIDNService ? "enabled" : "disabled");
            }
        } 
    }
    return NS_OK;
}

//----------------------------------------------------------------------------
// nsStandardURL::nsSegmentEncoder
//----------------------------------------------------------------------------

nsStandardURL::
nsSegmentEncoder::nsSegmentEncoder(const char *charset)
{
    if (!charset || !*charset)
        return;

    // get unicode encoder (XXX cache this someplace)
    nsresult rv;
    if (!gCharsetMgr) {
        nsCOMPtr<nsICharsetConverterManager2> convMgr(
                do_GetService("@mozilla.org/charset-converter-manager;1", &rv));
        if (NS_FAILED(rv)) {
            NS_ERROR("failed to get charset-converter-manager");
            return;
        }
        NS_ADDREF(gCharsetMgr = convMgr);
    }

    nsCOMPtr<nsIAtom> charsetAtom;
    rv = gCharsetMgr->GetCharsetAtom2(charset, getter_AddRefs(charsetAtom));
    if (NS_FAILED(rv)) {
        NS_ERROR("failed to get charset atom");
        return;
    }

    rv = gCharsetMgr->GetUnicodeEncoder(charsetAtom, getter_AddRefs(mEncoder));
    if (NS_FAILED(rv)) {
        NS_ERROR("failed to get unicode encoder");
        mEncoder = 0; // just in case
    }
}

PRInt32 nsStandardURL::
nsSegmentEncoder::EncodeSegmentCount(const char *str,
                                     const URLSegment &seg,
                                     PRInt16 mask,
                                     nsAFlatCString &result)
{
    if (!str)
        return 0;
    PRInt32 len = 0;
    if (seg.mLen > 0) {
        PRUint32 pos = seg.mPos;
        len = seg.mLen;

        // first honor the origin charset if appropriate. as an optimization,
        // only do this if |str| is non-ASCII.
        nsCAutoString encBuf;
        if (mEncoder && !nsCRT::IsAscii(str)) {
            NS_ConvertUTF8toUCS2 ucsBuf(Substring(str + pos, str + pos + len));
            if (NS_SUCCEEDED(EncodeString(mEncoder, ucsBuf, encBuf))) {
                str = encBuf.get();
                pos = 0;
                len = encBuf.Length();
            }
            // else some failure occured... assume UTF-8 is ok.
        }

        // escape per RFC2396 unless UTF-8 and allowed by preferences
        PRInt16 escapeFlags = (gEscapeUTF8 || mEncoder) ? 0 : esc_OnlyASCII;

        PRUint32 initLen = result.Length();

        // now perform any required escaping
        if (NS_EscapeURL(str + pos, len, mask | escapeFlags, result))
            len = result.Length() - initLen;
        else if (str == encBuf.get()) {
            result += encBuf; // append only!!
            len = encBuf.Length();
        }
    }
    return len;
}

const nsACString &nsStandardURL::
nsSegmentEncoder::EncodeSegment(const nsASingleFragmentCString &str,
                                PRInt16 mask,
                                nsAFlatCString &result)
{
    const char *text;
    PRUint32 resultLen = result.Length();
    EncodeSegmentCount(str.BeginReading(text), URLSegment(0, str.Length()), mask, result);
    // since EncodeSegmentCount appends to result, we must check whether its length grew
    if (result.Length() > resultLen)
        return result;
    else
        return str;
}

#define GET_SEGMENT_ENCODER(name) \
    nsSegmentEncoder name(mOriginCharset.get())

//----------------------------------------------------------------------------
// nsStandardURL <public>
//----------------------------------------------------------------------------

nsStandardURL::nsStandardURL()
    : mDefaultPort(-1)
    , mPort(-1)
    , mURLType(URLTYPE_STANDARD)
    , mHostA(nsnull)
    , mHostEncoding(eEncoding_Unknown)
    , mSpecEncoding(eEncoding_Unknown)
    , mMutable(PR_TRUE)
{
#if defined(PR_LOGGING)
    if (!gStandardURLLog)
        gStandardURLLog = PR_NewLogModule("nsStandardURL");
#endif

    LOG(("Creating nsStandardURL @%p\n", this));

    NS_INIT_ISUPPORTS();

    if (!gInitialized) {
        gInitialized = PR_TRUE;
        InitGlobalObjects();
    }

    // default parser in case nsIStandardURL::Init is never called
    mParser = gStdParser;
}

nsStandardURL::~nsStandardURL()
{
    LOG(("Destroying nsStandardURL @%p\n", this));

    CRTFREEIF(mHostA);
}

void
nsStandardURL::InitGlobalObjects()
{
    nsCOMPtr<nsIURLParser> parser;

    parser = do_GetService(NS_NOAUTHURLPARSER_CONTRACTID);
    NS_ASSERTION(parser, "failed getting 'noauth' url parser");
    if (parser) {
        gNoAuthParser = parser.get();
        NS_ADDREF(gNoAuthParser);
    }

    parser = do_GetService(NS_AUTHURLPARSER_CONTRACTID);
    NS_ASSERTION(parser, "failed getting 'auth' url parser");
    if (parser) {
        gAuthParser = parser.get();
        NS_ADDREF(gAuthParser);
    }

    parser = do_GetService(NS_STDURLPARSER_CONTRACTID);
    NS_ASSERTION(parser, "failed getting 'std' url parser");
    if (parser) {
        gStdParser = parser.get();
        NS_ADDREF(gStdParser);
    }

    nsCOMPtr<nsIPrefService> prefService( do_GetService(NS_PREFSERVICE_CONTRACTID) );
    if (prefService) {
        nsCOMPtr<nsIPrefBranch> prefBranch;
        prefService->GetBranch(nsnull, getter_AddRefs(prefBranch));
        if (prefBranch) {
            nsCOMPtr<nsIPrefBranchInternal> pbi( do_QueryInterface(prefBranch) );
            if (pbi) {
                nsCOMPtr<nsIObserver> obs( new nsPrefObserver() );
                pbi->AddObserver(NS_NET_PREF_ESCAPEUTF8, obs.get(), PR_FALSE); 
                pbi->AddObserver(NS_NET_PREF_ENABLEIDN, obs.get(), PR_FALSE); 
            }
        }
    }
}

void
nsStandardURL::ShutdownGlobalObjects()
{
    NS_IF_RELEASE(gNoAuthParser);
    NS_IF_RELEASE(gAuthParser);
    NS_IF_RELEASE(gStdParser);
    NS_IF_RELEASE(gIDNService);
    NS_IF_RELEASE(gCharsetMgr);
}

//----------------------------------------------------------------------------
// nsStandardURL <private>
//----------------------------------------------------------------------------

void
nsStandardURL::Clear()
{
    mSpec.Truncate();

    mPort = -1;

    mAuthority.Reset();
    mUsername.Reset();
    mPassword.Reset();
    mHost.Reset();

    mPath.Reset();
    mFilepath.Reset();
    mDirectory.Reset();
    mBasename.Reset();

    mExtension.Reset();
    mParam.Reset();
    mQuery.Reset();
    mRef.Reset();

    InvalidateCache();
}

void
nsStandardURL::InvalidateCache(PRBool invalidateCachedFile)
{
    if (invalidateCachedFile)
        mFile = 0;
    CRTFREEIF(mHostA);
    mSpecEncoding = eEncoding_Unknown;
    mHostEncoding = eEncoding_Unknown;
}

PRBool
nsStandardURL::EncodeHost(const char *host, nsCString &result)
{
    // Escape IPv6 address literal by surrounding it with []'s
    if (host && (host[0] != '[') && PL_strchr(host, ':')) {
        result.Assign('[');
        result.Append(host);
        result.Append(']');
        return PR_TRUE;
    }
    return PR_FALSE;
}

void
nsStandardURL::CoalescePath(char *path)
{
    CoalesceDirsAbs(path);
    PRInt32 newLen = strlen(path);
    if (newLen < mPath.mLen) {
        PRInt32 diff = newLen - mPath.mLen;
        mPath.mLen = newLen;
        mDirectory.mLen += diff;
        mFilepath.mLen += diff;
        ShiftFromBasename(diff);
    }
}

PRUint32
nsStandardURL::AppendSegmentToBuf(char *buf, PRUint32 i, const char *str, URLSegment &seg, const nsCString *escapedStr)
{
    if (seg.mLen > 0) {
        if (escapedStr && !escapedStr->IsEmpty()) {
            seg.mLen = escapedStr->Length();
            memcpy(buf + i, escapedStr->get(), seg.mLen);
        }
        else
            memcpy(buf + i, str + seg.mPos, seg.mLen);
        seg.mPos = i;
        i += seg.mLen;
    }
    return i;
}

PRUint32
nsStandardURL::AppendToBuf(char *buf, PRUint32 i, const char *str, PRUint32 len)
{
    memcpy(buf + i, str, len);
    return i + len;
}

// basic algorithm:
//  1- escape url segments (for improved GetSpec efficiency)
//  2- allocate spec buffer
//  3- write url segments
//  4- update url segment positions and lengths
nsresult
nsStandardURL::BuildNormalizedSpec(const char *spec)
{
    // Assumptions: all member URLSegments must be relative the |spec| argument
    // passed to this function.

    // buffers for holding escaped url segments (these will remain empty unless
    // escaping is required).
    nsCAutoString encUsername;
    nsCAutoString encPassword;
    nsCAutoString encDirectory;
    nsCAutoString encBasename;
    nsCAutoString encExtension;
    nsCAutoString encParam;
    nsCAutoString encQuery;
    nsCAutoString encRef;

    //
    // escape each URL segment, if necessary, and calculate approximate normalized
    // spec length.
    //
    PRInt32 approxLen = 3; // includes room for "://"

    // the scheme is already ASCII
    if (mScheme.mLen > 0)
        approxLen += mScheme.mLen;

    // encode URL segments; convert UTF-8 to origin charset and possibly escape.
    // results written to encXXX variables only if |spec| is not already in the
    // appropriate encoding.
    {
        GET_SEGMENT_ENCODER(encoder);
        approxLen += encoder.EncodeSegmentCount(spec, mUsername,  esc_Username,      encUsername);
        approxLen += encoder.EncodeSegmentCount(spec, mPassword,  esc_Password,      encPassword);
        approxLen += encoder.EncodeSegmentCount(spec, mDirectory, esc_Directory,     encDirectory);
        approxLen += encoder.EncodeSegmentCount(spec, mBasename,  esc_FileBaseName,  encBasename);
        approxLen += encoder.EncodeSegmentCount(spec, mExtension, esc_FileExtension, encExtension);
        approxLen += encoder.EncodeSegmentCount(spec, mParam,     esc_Param,         encParam);
        approxLen += encoder.EncodeSegmentCount(spec, mQuery,     esc_Query,         encQuery);
        approxLen += encoder.EncodeSegmentCount(spec, mRef,       esc_Ref,           encRef);
    }

    // do not escape the hostname, if IPv6 address literal, mHost will
    // already point to a [ ] delimited IPv6 address literal.
    if (mHost.mLen > 0)
        approxLen += mHost.mLen;

    //
    // generate the normalized URL string
    //
    char *buf = (char *) nsMemory::Alloc(approxLen + 32);
    if (!buf)
        return NS_ERROR_OUT_OF_MEMORY;
    PRUint32 i = 0;

    if (mScheme.mLen > 0) {
        i = AppendSegmentToBuf(buf, i, spec, mScheme);
        ToLowerCase(buf + mScheme.mPos, mScheme.mLen);
        i = AppendToBuf(buf, i, "://", 3);
    }

    // record authority starting position
    mAuthority.mPos = i;

    // append authority
    if (mUsername.mLen > 0) {
        i = AppendSegmentToBuf(buf, i, spec, mUsername, &encUsername);
        if (mPassword.mLen >= 0) {
            buf[i++] = ':';
            i = AppendSegmentToBuf(buf, i, spec, mPassword, &encPassword);
        }
        buf[i++] = '@';
    }
    if (mHost.mLen > 0) {
        i = AppendSegmentToBuf(buf, i, spec, mHost);
        ToLowerCase(buf + mHost.mPos, mHost.mLen);
        if (mPort != -1 && mPort != mDefaultPort) {
            nsCAutoString portbuf;
            portbuf.AppendInt(mPort);
            buf[i++] = ':';
            i = AppendToBuf(buf, i, portbuf.get(), portbuf.Length());
        }
    }

    // record authority length
    mAuthority.mLen = i - mAuthority.mPos;

    // path must always start with a "/"
    if (mPath.mLen <= 0) {
        LOG(("setting path=/"));
        mDirectory.mPos = mFilepath.mPos = mPath.mPos = i;
        mDirectory.mLen = mFilepath.mLen = mPath.mLen = 1;
        // basename must exist, even if empty (bug 113508)
        mBasename.mPos = i+1;
        mBasename.mLen = 0;
        buf[i++] = '/';
    }
    else {
        PRUint32 leadingSlash = 0;
        if (spec[mPath.mPos] != '/') {
            LOG(("adding leading slash to path\n"));
            leadingSlash = 1;
            buf[i++] = '/';
        }

        // record corrected (file)path starting position
        mPath.mPos = mFilepath.mPos = i - leadingSlash;

        i = AppendSegmentToBuf(buf, i, spec, mDirectory, &encDirectory);

        // the directory must end with a '/'
        if (buf[i-1] != '/') {
            buf[i++] = '/';
            mDirectory.mLen++;
        }

        i = AppendSegmentToBuf(buf, i, spec, mBasename, &encBasename);

        // make corrections to directory segment if leadingSlash
        if (leadingSlash) {
            mDirectory.mPos = mPath.mPos;
            if (mDirectory.mLen >= 0)
                mDirectory.mLen += leadingSlash;
            else
                mDirectory.mLen = 1;
        }

        if (mExtension.mLen >= 0) {
            buf[i++] = '.';
            i = AppendSegmentToBuf(buf, i, spec, mExtension, &encExtension);
        }
        // calculate corrected filepath length
        mFilepath.mLen = i - mFilepath.mPos;

        if (mParam.mLen >= 0) {
            buf[i++] = ';';
            i = AppendSegmentToBuf(buf, i, spec, mParam, &encParam);
        }
        if (mQuery.mLen >= 0) {
            buf[i++] = '?';
            i = AppendSegmentToBuf(buf, i, spec, mQuery, &encQuery);
        }
        if (mRef.mLen >= 0) {
            buf[i++] = '#';
            i = AppendSegmentToBuf(buf, i, spec, mRef, &encRef);
        }
        // calculate corrected path length
        mPath.mLen = i - mPath.mPos;
    }

    buf[i] = '\0';

    if (mDirectory.mLen > 1)
        CoalescePath(buf + mDirectory.mPos);

    mSpec.Adopt(buf);
    return NS_OK;
}

PRBool
nsStandardURL::HostsAreEquivalent(nsStandardURL *that)
{
    // optimize for the non-IDN case...
    if ((this->mHostEncoding == eEncoding_ASCII) &&
        (that->mHostEncoding == eEncoding_ASCII))
        return SegmentIs(mHost, that->mSpec.get(), that->mHost);

    nsCAutoString thisHost, thatHost;
    this->GetAsciiHost(thisHost);
    that->GetAsciiHost(thatHost);
    
    return !nsCRT::strcasecmp(thisHost.get(), thatHost.get());
}

PRBool
nsStandardURL::SegmentIs(const URLSegment &seg, const char *val)
{
    // one or both may be null
    if (!val || mSpec.IsEmpty())
        return (!val && (mSpec.IsEmpty() || seg.mLen < 0));
    if (seg.mLen < 0)
        return PR_FALSE;
    // if the first |seg.mLen| chars of |val| match, then |val| must
    // also be null terminated at |seg.mLen|.
    return !nsCRT::strncasecmp(mSpec.get() + seg.mPos, val, seg.mLen)
        && (val[seg.mLen] == '\0');
}

PRBool
nsStandardURL::SegmentIs(const URLSegment &seg1, const char *val, const URLSegment &seg2)
{
    if (seg1.mLen != seg2.mLen)
        return PR_FALSE;
    if (seg1.mLen == -1 || (!val && mSpec.IsEmpty()))
        return PR_TRUE; // both are empty
    return !nsCRT::strncasecmp(mSpec.get() + seg1.mPos, val + seg2.mPos, seg1.mLen); 
}

PRInt32
nsStandardURL::ReplaceSegment(PRUint32 pos, PRUint32 len, const char *val, PRUint32 valLen)
{
    if (val && valLen) {
        if (len == 0)
            mSpec.Insert(val, pos, valLen);
        else
            mSpec.Replace(pos, len, nsDependentCString(val, valLen));
        return valLen - len;
    }

    // else remove the specified segment
    mSpec.Cut(pos, len);
    return -PRInt32(len);
}

PRInt32
nsStandardURL::ReplaceSegment(PRUint32 pos, PRUint32 len, const nsACString &val)
{
    if (len == 0)
        mSpec.Insert(val, pos);
    else
        mSpec.Replace(pos, len, val);
    return val.Length() - len;
}

nsresult
nsStandardURL::ParseURL(const char *spec)
{
    nsresult rv;

    //
    // parse given URL string
    //
    rv = mParser->ParseURL(spec, -1,
                           &mScheme.mPos, &mScheme.mLen,
                           &mAuthority.mPos, &mAuthority.mLen,
                           &mPath.mPos, &mPath.mLen);
    if (NS_FAILED(rv)) return rv;

#ifdef DEBUG
    if (mScheme.mLen <= 0) {
        printf("spec=%s\n", spec);
        NS_WARNING("malformed url: no scheme");
    }
#endif
     
    if (mAuthority.mLen > 0) {
        rv = mParser->ParseAuthority(spec + mAuthority.mPos, mAuthority.mLen,
                                     &mUsername.mPos, &mUsername.mLen,
                                     &mPassword.mPos, &mPassword.mLen,
                                     &mHost.mPos, &mHost.mLen,
                                     &mPort);
        if (NS_FAILED(rv)) return rv;

        mUsername.mPos += mAuthority.mPos;
        mPassword.mPos += mAuthority.mPos;
        mHost.mPos += mAuthority.mPos;
    }

    if (mPath.mLen > 0)
        rv = ParsePath(spec, mPath.mPos, mPath.mLen);

    return rv;
}

nsresult
nsStandardURL::ParsePath(const char *spec, PRUint32 pathPos, PRInt32 pathLen)
{
    nsresult rv = mParser->ParsePath(spec + pathPos, pathLen,
                                     &mFilepath.mPos, &mFilepath.mLen,
                                     &mParam.mPos, &mParam.mLen,
                                     &mQuery.mPos, &mQuery.mLen,
                                     &mRef.mPos, &mRef.mLen);
    if (NS_FAILED(rv)) return rv;

    mFilepath.mPos += pathPos;
    mParam.mPos += pathPos;
    mQuery.mPos += pathPos;
    mRef.mPos += pathPos;

    if (mFilepath.mLen > 0) {
        rv = mParser->ParseFilePath(spec + mFilepath.mPos, mFilepath.mLen,
                                    &mDirectory.mPos, &mDirectory.mLen,
                                    &mBasename.mPos, &mBasename.mLen,
                                    &mExtension.mPos, &mExtension.mLen);
        if (NS_FAILED(rv)) return rv;

        mDirectory.mPos += mFilepath.mPos;
        mBasename.mPos += mFilepath.mPos;
        mExtension.mPos += mFilepath.mPos;
    }
    return NS_OK;
}

char *
nsStandardURL::AppendToSubstring(PRUint32 pos,
                                 PRInt32 len,
                                 const char *tail,
                                 PRInt32 tailLen)
{
    if (tailLen < 0)
        tailLen = strlen(tail);

    char *result = (char *) malloc(len + tailLen + 1);
    if (result) {
        memcpy(result, mSpec.get() + pos, len);
        memcpy(result + len, tail, tailLen);
        result[len + tailLen] = '\0';
    }
    return result;
}

nsresult
nsStandardURL::ReadSegment(nsIBinaryInputStream *stream, URLSegment &seg)
{
    nsresult rv;

    rv = stream->Read32(&seg.mPos);
    if (NS_FAILED(rv)) return rv;

    rv = stream->Read32((PRUint32 *) &seg.mLen);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

nsresult
nsStandardURL::WriteSegment(nsIBinaryOutputStream *stream, const URLSegment &seg)
{
    nsresult rv;

    rv = stream->Write32(seg.mPos);
    if (NS_FAILED(rv)) return rv;

    rv = stream->Write32(PRUint32(seg.mLen));
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

//----------------------------------------------------------------------------
// nsStandardURL::nsISupports
//----------------------------------------------------------------------------

NS_IMPL_ADDREF(nsStandardURL)
NS_IMPL_RELEASE(nsStandardURL)

NS_INTERFACE_MAP_BEGIN(nsStandardURL)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStandardURL)
    NS_INTERFACE_MAP_ENTRY(nsIURI)
    NS_INTERFACE_MAP_ENTRY(nsIURL)
    NS_INTERFACE_MAP_ENTRY(nsIFileURL)
    NS_INTERFACE_MAP_ENTRY(nsIStandardURL)
    NS_INTERFACE_MAP_ENTRY(nsISerializable)
    // see nsStandardURL::Equals
    if (aIID.Equals(kThisImplCID))
        foundInterface = NS_STATIC_CAST(nsIURI *, this);
    else
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------------
// nsStandardURL::nsIURI
//----------------------------------------------------------------------------

// result may contain unescaped UTF-8 characters
NS_IMETHODIMP
nsStandardURL::GetSpec(nsACString &result)
{
    result = mSpec;
    return NS_OK;
}

// result may contain unescaped UTF-8 characters
NS_IMETHODIMP
nsStandardURL::GetPrePath(nsACString &result)
{
    result = Prepath();
    return NS_OK;
}

// result is strictly US-ASCII
NS_IMETHODIMP
nsStandardURL::GetScheme(nsACString &result)
{
    result = Scheme();
    return NS_OK;
}

// result may contain unescaped UTF-8 characters
NS_IMETHODIMP
nsStandardURL::GetUserPass(nsACString &result)
{
    result = Userpass();
    return NS_OK;
}

// result may contain unescaped UTF-8 characters
NS_IMETHODIMP
nsStandardURL::GetUsername(nsACString &result)
{
    result = Username();
    return NS_OK;
}

// result may contain unescaped UTF-8 characters
NS_IMETHODIMP
nsStandardURL::GetPassword(nsACString &result)
{
    result = Password();
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::GetHostPort(nsACString &result)
{
    result = Hostport();
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::GetHost(nsACString &result)
{
    result = Host();
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::GetPort(PRInt32 *result)
{
    *result = mPort;
    return NS_OK;
}

// result may contain unescaped UTF-8 characters
NS_IMETHODIMP
nsStandardURL::GetPath(nsACString &result)
{
    result = Path();
    return NS_OK;
}

// result is ASCII
NS_IMETHODIMP
nsStandardURL::GetAsciiSpec(nsACString &result)
{
    if (mSpecEncoding == eEncoding_Unknown) {
        if (IsASCII(mSpec))
            mSpecEncoding = eEncoding_ASCII;
        else
            mSpecEncoding = eEncoding_UTF8;
    }

    if (mSpecEncoding == eEncoding_ASCII) {
        result = mSpec;
        return NS_OK;
    }

    // try to guess the capacity required for result...
    result.SetCapacity(mSpec.Length() + PR_MIN(32, mSpec.Length()/10));

    result = Substring(mSpec, 0, mScheme.mLen + 3);

    NS_EscapeURL(Userpass(PR_TRUE), esc_OnlyNonASCII | esc_AlwaysCopy, result);

    // get escaped host
    nsCAutoString escHostport;
    if (mHost.mLen > 0) {
        // this doesn't fail
        (void) GetAsciiHost(escHostport);

        // escHostport = "hostA" + ":port"
        PRUint32 pos = mHost.mPos + mHost.mLen;
        if (pos < mPath.mPos)
            escHostport += Substring(mSpec, pos, mPath.mPos - pos);
    }
    result += escHostport;

    NS_EscapeURL(Path(), esc_OnlyNonASCII | esc_AlwaysCopy, result);
    return NS_OK;
}

// result is ASCII
NS_IMETHODIMP
nsStandardURL::GetAsciiHost(nsACString &result)
{
    if (mHostEncoding == eEncoding_Unknown) {
        if (IsASCII(Host()))
            mHostEncoding = eEncoding_ASCII;
        else
            mHostEncoding = eEncoding_UTF8;
    }

    if (mHostEncoding == eEncoding_ASCII) {
        result = Host();
        return NS_OK;
    }

    // perhaps we have it cached...
    if (mHostA) {
        result = mHostA;
        return NS_OK;
    }

    if (gIDNService) {
        nsresult rv;
        rv = gIDNService->ConvertUTF8toACE(PromiseFlatCString(Host()).get(), &mHostA);
        if (NS_SUCCEEDED(rv)) {
            result = mHostA;
            return NS_OK;
        }
        NS_WARNING("UTF8ToIDNHostName failed");
    }

    // something went wrong... guess all we can do is URL escape :-/
    NS_EscapeURL(Host(), esc_AlwaysCopy, result);
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::GetOriginCharset(nsACString &result)
{
    if (mOriginCharset.IsEmpty())
        result = NS_LITERAL_CSTRING("UTF-8");
    else
        result = mOriginCharset;
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetSpec(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &flat = PromiseFlatCString(input);
    const char *spec = flat.get();

    LOG(("nsStandardURL::SetSpec [spec=%s]\n", spec));

    Clear();

    if (!spec)
        return NS_OK;

    // filter out unexpected chars "\r\n\t" if necessary
    nsCAutoString buf1;
    spec = FilterString(spec, buf1);

    // parse the given URL...
    nsresult rv = ParseURL(spec);
    if (NS_FAILED(rv)) return rv;

    // finally, use the URLSegment member variables to build a normalized
    // copy of |spec|
    rv = BuildNormalizedSpec(spec);

#if defined(PR_LOGGING)
    if (LOG_ENABLED()) {
        LOG((" spec      = %s\n", mSpec.get()));
        LOG((" port      = %d\n", mPort));
        LOG((" scheme    = (%u,%d)\n", mScheme.mPos,    mScheme.mLen));
        LOG((" authority = (%u,%d)\n", mAuthority.mPos, mAuthority.mLen));
        LOG((" username  = (%u,%d)\n", mUsername.mPos,  mUsername.mLen));
        LOG((" password  = (%u,%d)\n", mPassword.mPos,  mPassword.mLen));
        LOG((" hostname  = (%u,%d)\n", mHost.mPos,      mHost.mLen));
        LOG((" path      = (%u,%d)\n", mPath.mPos,      mPath.mLen));
        LOG((" filepath  = (%u,%d)\n", mFilepath.mPos,  mFilepath.mLen));
        LOG((" directory = (%u,%d)\n", mDirectory.mPos, mDirectory.mLen));
        LOG((" basename  = (%u,%d)\n", mBasename.mPos,  mBasename.mLen));
        LOG((" extension = (%u,%d)\n", mExtension.mPos, mExtension.mLen));
        LOG((" param     = (%u,%d)\n", mParam.mPos,     mParam.mLen));
        LOG((" query     = (%u,%d)\n", mQuery.mPos,     mQuery.mLen));
        LOG((" ref       = (%u,%d)\n", mRef.mPos,       mRef.mLen));
    }
#endif
    return rv;
}

NS_IMETHODIMP
nsStandardURL::SetScheme(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &scheme = PromiseFlatCString(input);

    LOG(("nsStandardURL::SetScheme [scheme=%s]\n", scheme.get()));

    if (scheme.IsEmpty()) {
        NS_ERROR("cannot remove the scheme from an url");
        return NS_ERROR_UNEXPECTED;
    }
    if (mScheme.mLen < 0) {
        NS_ERROR("uninitialized");
        return NS_ERROR_NOT_INITIALIZED;
    }

    if (!IsValidScheme(scheme)) {
        NS_ERROR("the given url scheme contains invalid characters");
        return NS_ERROR_UNEXPECTED;
    }

    InvalidateCache();

    PRInt32 shift = ReplaceSegment(mScheme.mPos, mScheme.mLen, scheme);

    if (shift) {
        mScheme.mLen = scheme.Length();
        ShiftFromAuthority(shift);
    }

    // ensure new scheme is lowercase
    //
    // XXX the string code unfortunately doesn't provide a ToLowerCase
    //     that operates on a substring.
    ToLowerCase((char *) mSpec.get(), mScheme.mLen);
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetUserPass(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &userpass = PromiseFlatCString(input);

    LOG(("nsStandardURL::SetUserPass [userpass=%s]\n", userpass.get()));

    if (mURLType == URLTYPE_NO_AUTHORITY) {
        NS_ERROR("cannot set user:pass on no-auth url");
        return NS_ERROR_UNEXPECTED;
    }
    if (mAuthority.mLen < 0) {
        NS_ERROR("uninitialized");
        return NS_ERROR_NOT_INITIALIZED;
    }
    NS_ASSERTION(mHost.mLen >= 0, "uninitialized");

    InvalidateCache();

    if (userpass.IsEmpty()) {
        // remove user:pass
        if (mUsername.mLen >= 0) {
            if (mPassword.mLen > 0)
                mUsername.mLen += (mPassword.mLen + 1);
            mUsername.mLen++;
            mSpec.Cut(mUsername.mPos, mUsername.mLen);
            mAuthority.mLen -= mUsername.mLen;
            ShiftFromHost(-mUsername.mLen);
            mUsername.mLen = -1;
            mPassword.mLen = -1;
        }
        return NS_OK;
    }

    nsresult rv;
    PRUint32 usernamePos, passwordPos;
    PRInt32 usernameLen, passwordLen;

    rv = mParser->ParseUserInfo(userpass.get(), userpass.Length(),
                                &usernamePos, &usernameLen,
                                &passwordPos, &passwordLen);
    if (NS_FAILED(rv)) return rv;

    // build new user:pass in |buf|
    nsCAutoString buf;
    if (usernameLen > 0) {
        GET_SEGMENT_ENCODER(encoder);
        usernameLen = encoder.EncodeSegmentCount(userpass.get(),
                                                 URLSegment(usernamePos,
                                                            usernameLen),
                                                 esc_Username | esc_AlwaysCopy,
                                                 buf);
        if (passwordLen >= 0) {
            buf.Append(':');
            passwordLen = encoder.EncodeSegmentCount(userpass.get(),
                                                     URLSegment(passwordPos,
                                                                passwordLen),
                                                     esc_Password |
                                                     esc_AlwaysCopy, buf);
        }
        if (mUsername.mLen < 0)
            buf.Append('@');
    }

    PRUint32 shift = 0;

    if (mUsername.mLen < 0) {
        // no existing user:pass
        if (!buf.IsEmpty()) {
            mSpec.Insert(buf, mHost.mPos);
            mUsername.mPos = mHost.mPos;
            shift = buf.Length();
        }
    }
    else {
        // replace existing user:pass
        PRUint32 userpassLen = mUsername.mLen;
        if (mPassword.mLen >= 0)
            userpassLen += (mPassword.mLen + 1);
        mSpec.Replace(mUsername.mPos, userpassLen, buf);
        shift = buf.Length() - userpassLen;
    }
    if (shift) {
        ShiftFromHost(shift);
        mAuthority.mLen += shift;
    }
    // update positions and lengths
    mUsername.mLen = usernameLen;
    mPassword.mLen = passwordLen;
    if (passwordLen)
        mPassword.mPos = mUsername.mPos + mUsername.mLen + 1;
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetUsername(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &username = PromiseFlatCString(input);

    LOG(("nsStandardURL::SetUsername [username=%s]\n", username.get()));

    if (mURLType == URLTYPE_NO_AUTHORITY) {
        NS_ERROR("cannot set username on no-auth url");
        return NS_ERROR_UNEXPECTED;
    }

    if (username.IsEmpty())
        return SetUserPass(username);

    InvalidateCache();

    // escape username if necessary
    nsCAutoString buf;
    GET_SEGMENT_ENCODER(encoder);
    const nsACString &escUsername =
        encoder.EncodeSegment(username, esc_Username, buf);

    PRInt32 shift;

    if (mUsername.mLen < 0) {
        mUsername.mPos = mAuthority.mPos;
        mSpec.Insert(escUsername + NS_LITERAL_CSTRING("@"), mUsername.mPos);
        shift = escUsername.Length() + 1;
    }
    else
        shift = ReplaceSegment(mUsername.mPos, mUsername.mLen, escUsername);

    if (shift) {
        mUsername.mLen = escUsername.Length();
        mAuthority.mLen += shift;
        ShiftFromPassword(shift);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetPassword(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &password = PromiseFlatCString(input);

    LOG(("nsStandardURL::SetPassword [password=%s]\n", password.get()));

    if (mURLType == URLTYPE_NO_AUTHORITY) {
        NS_ERROR("cannot set password on no-auth url");
        return NS_ERROR_UNEXPECTED;
    }
    if (mUsername.mLen < 0) {
        NS_ERROR("cannot set password without existing username");
        return NS_ERROR_FAILURE;
    }

    InvalidateCache();

    if (password.IsEmpty()) {
        if (mPassword.mLen >= 0) {
            // cut(":password")
            mSpec.Cut(mPassword.mPos - 1, mPassword.mLen + 1);
            ShiftFromHost(-(mPassword.mLen + 1));
            mAuthority.mLen -= (mPassword.mLen + 1);
            mPassword.mLen = -1;
        }
        return NS_OK;
    }

    // escape password if necessary
    nsCAutoString buf;
    GET_SEGMENT_ENCODER(encoder);
    const nsACString &escPassword =
        encoder.EncodeSegment(password, esc_Password, buf);

    PRInt32 shift;

    if (mPassword.mLen < 0) {
        mPassword.mPos = mUsername.mPos + mUsername.mLen + 1;
        mSpec.Insert(NS_LITERAL_CSTRING(":") + escPassword, mPassword.mPos - 1);
        shift = escPassword.Length() + 1;
    }
    else
        shift = ReplaceSegment(mPassword.mPos, mPassword.mLen, escPassword);

    if (shift) {
        mPassword.mLen = escPassword.Length();
        mAuthority.mLen += shift;
        ShiftFromHost(shift);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetHostPort(const nsACString &value)
{
    ENSURE_MUTABLE();

    // XXX needs implementation!!
    NS_NOTREACHED("not implemented");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsStandardURL::SetHost(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &flat = PromiseFlatCString(input);
    const char *host = flat.get();

    LOG(("nsStandardURL::SetHost [host=%s]\n", host));

    if (mURLType == URLTYPE_NO_AUTHORITY) {
        NS_ERROR("cannot set host on no-auth url");
        return NS_ERROR_UNEXPECTED;
    }

    InvalidateCache();

    if (!(host && *host)) {
        // remove existing hostname
        if (mHost.mLen > 0) {
            // remove entire authority
            mSpec.Cut(mAuthority.mPos, mAuthority.mLen);
            ShiftFromPath(-mAuthority.mLen);
            mAuthority.mLen = 0;
            mUsername.mLen = -1;
            mPassword.mLen = -1;
            mHost.mLen = -1;
            mPort = -1;
        }
        return NS_OK;
    }

    // handle IPv6 unescaped address literal
    PRInt32 len;
    nsCAutoString escapedHost;
    if (EncodeHost(host, escapedHost)) {
        host = escapedHost.get();
        len = escapedHost.Length();
    }
    else
        len = strlen(host);

    if (mHost.mLen < 0) {
        mHost.mPos = mAuthority.mPos;
        mHost.mLen = 0;
    }

    PRInt32 shift = ReplaceSegment(mHost.mPos, mHost.mLen, host, len);

    if (shift) {
        mHost.mLen = len;
        mAuthority.mLen += shift;
        ShiftFromPath(shift);
    }
    return NS_OK;
}
             
NS_IMETHODIMP
nsStandardURL::SetPort(PRInt32 port)
{
    ENSURE_MUTABLE();

    LOG(("nsStandardURL::SetPort [port=%d]\n", port));

    if ((port == mPort) || (mPort == -1 && port == mDefaultPort))
        return NS_OK;

    InvalidateCache();

    if (mPort == -1) {
        // need to insert the port number in the URL spec
        nsCAutoString buf;
        buf.Assign(':');
        buf.AppendInt(port);
        mSpec.Insert(buf, mHost.mPos + mHost.mLen);
        ShiftFromPath(buf.Length());
    }
    else if (port == -1) {
        // need to remove the port number from the URL spec
        PRUint32 start = mHost.mPos + mHost.mLen;
        mSpec.Cut(start, mPath.mPos - start);
        ShiftFromPath(start - mPath.mPos);
    }
    else {
        // need to replace the existing port
        nsCAutoString buf;
        buf.AppendInt(port);
        PRUint32 start = mHost.mPos + mHost.mLen + 1;
        PRUint32 length = mPath.mPos - start;
        mSpec.Replace(start, length, buf);
        if (buf.Length() != length)
            ShiftFromPath(buf.Length() - length);
    }

    mPort = port;
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetPath(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &path = PromiseFlatCString(input);

    LOG(("nsStandardURL::SetPath [path=%s]\n", path.get()));

    InvalidateCache();

    if (!path.IsEmpty()) {
        nsCAutoString spec;

        spec.Assign(mSpec.get(), mPath.mPos);
        if (path.First() != '/')
            spec.Append('/');
        spec.Append(path);

        return SetSpec(spec);
    }
    else if (mPath.mLen > 1) {
        mSpec.Cut(mPath.mPos + 1, mPath.mLen - 1);
        // these contain only a '/'
        mPath.mLen = 1;
        mDirectory.mLen = 1;
        mFilepath.mLen = 1;
        // these are no longer defined
        mBasename.mLen = -1;
        mExtension.mLen = -1;
        mParam.mLen = -1;
        mQuery.mLen = -1;
        mRef.mLen = -1;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::Equals(nsIURI *unknownOther, PRBool *result)
{
    NS_ENSURE_ARG_POINTER(unknownOther);
    NS_PRECONDITION(result, "null pointer");

    nsStandardURL *other;
    nsresult rv = unknownOther->QueryInterface(kThisImplCID, (void **) &other);
    if (NS_FAILED(rv)) {
        *result = PR_FALSE;
        return NS_OK;
    }

    *result = 
        SegmentIs(mScheme, other->mSpec.get(), other->mScheme) &&
        SegmentIs(mDirectory, other->mSpec.get(), other->mDirectory) &&
        SegmentIs(mBasename, other->mSpec.get(), other->mBasename) &&
        SegmentIs(mExtension, other->mSpec.get(), other->mExtension) &&
        HostsAreEquivalent(other) &&
        SegmentIs(mQuery, other->mSpec.get(), other->mQuery) &&
        SegmentIs(mRef, other->mSpec.get(), other->mRef) &&
        SegmentIs(mUsername, other->mSpec.get(), other->mUsername) &&
        SegmentIs(mPassword, other->mSpec.get(), other->mPassword) &&
        SegmentIs(mParam, other->mSpec.get(), other->mParam) &&
        (Port() == other->Port());

    NS_RELEASE(other);
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SchemeIs(const char *scheme, PRBool *result)
{
    NS_PRECONDITION(result, "null pointer");

    *result = SegmentIs(mScheme, scheme);
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::Clone(nsIURI **result)
{
    nsStandardURL *clone;
    NS_NEWXPCOM(clone, nsStandardURL);
    if (!clone)
        return NS_ERROR_OUT_OF_MEMORY;

    // XXX a copy-on-write string would be very nice here
    clone->mSpec = mSpec;
    clone->mDefaultPort = mDefaultPort;
    clone->mPort = mPort;
    clone->mScheme = mScheme;
    clone->mAuthority = mAuthority;
    clone->mUsername = mUsername;
    clone->mPassword = mPassword;
    clone->mHost = mHost;
    clone->mPath = mPath;
    clone->mFilepath = mFilepath;
    clone->mDirectory = mDirectory;
    clone->mBasename = mBasename;
    clone->mExtension = mExtension;
    clone->mParam = mParam;
    clone->mQuery = mQuery;
    clone->mRef = mRef;
    clone->mOriginCharset = mOriginCharset;
    clone->mURLType = mURLType;
    clone->mParser = mParser;
    clone->mFile = mFile;
    clone->mHostA = mHostA ? nsCRT::strdup(mHostA) : nsnull;
    clone->mMutable = PR_TRUE;
    clone->mHostEncoding = mHostEncoding;
    clone->mSpecEncoding = mSpecEncoding;

    NS_ADDREF(*result = clone);
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::Resolve(const nsACString &in, nsACString &out)
{
    const nsPromiseFlatCString &flat = PromiseFlatCString(in);
    const char *relpath = flat.get();

    // XXX hack hack hack
    char *p = nsnull;
    char **result = &p;

    LOG(("nsStandardURL::Resolve [this=%p spec=%s relpath=%s]\n",
        this, mSpec.get(), relpath));

    NS_ASSERTION(gNoAuthParser, "no parser: unitialized");

    // NOTE: there is no need for this function to produce normalized
    // output.  normalization will occur when the result is used to 
    // initialize a nsStandardURL object.

    if (mScheme.mLen < 0) {
        NS_ERROR("unable to Resolve URL: this URL not initialized");
        return NS_ERROR_NOT_INITIALIZED;
    }

    nsresult rv;
    URLSegment scheme;
    char *resultPath = nsnull;

    // relative urls should never contain a host, so we always want to use
    // the noauth url parser.
    rv = gNoAuthParser->ParseURL(relpath, flat.Length(),
                                 &scheme.mPos, &scheme.mLen,
                                 nsnull, nsnull,
                                 nsnull, nsnull);

    // if the parser fails (for example because there is no valid scheme)
    // reset the scheme and assume a relative url
    if (NS_FAILED(rv)) 
        scheme.Reset();

    if (scheme.mLen >= 0) {
        // this URL appears to be absolute
        *result = nsCRT::strdup(relpath);
    }
    else if (relpath[0] == '/' && relpath[1] == '/') {
        // this URL is almost absolute
        *result = AppendToSubstring(mScheme.mPos, mScheme.mLen + 1, relpath);
    }
    else {
        PRUint32 len = 0;
        switch (*relpath) {
        case '/':
            // overwrite everything after the authority
            len = mAuthority.mPos + mAuthority.mLen;
            break;
        case '?':
            // overwrite the existing ?query and #ref
            if (mQuery.mLen >= 0)
                len = mQuery.mPos - 1;
            else if (mRef.mLen >= 0)
                len = mRef.mPos - 1;
            else
                len = mPath.mPos + mPath.mLen;
            break;
        case '#':
        case '\0':
            // overwrite the existing #ref
            if (mRef.mLen < 0)
                len = mPath.mPos + mPath.mLen;
            else
                len = mRef.mPos - 1;
            break;
        default:
            // overwrite everything after the directory 
            len = mDirectory.mPos + mDirectory.mLen;
        }
        *result = AppendToSubstring(0, len, relpath);

        // locate result path
        resultPath = *result + mPath.mPos;
    }
    if (!*result)
        return NS_ERROR_OUT_OF_MEMORY;

    if (resultPath)
        CoalesceDirsRel(resultPath);
    else {
        // locate result path
        resultPath = PL_strstr(*result, "://");
        if (resultPath) {
            resultPath = PL_strchr(resultPath + 3, '/');
            if (resultPath)
                CoalesceDirsRel(resultPath);
        }
    }
    // XXX avoid extra copy
    out = *result;
    free(*result);
    return NS_OK;
}

// result may contain unescaped UTF-8 characters
NS_IMETHODIMP
nsStandardURL::GetCommonBaseSpec(nsIURI *uri2, nsACString &aResult)
{
    NS_ENSURE_ARG_POINTER(uri2);

    // if uri's are equal, then return uri as is
    PRBool isEquals = PR_FALSE;
    if (NS_SUCCEEDED(Equals(uri2, &isEquals)) && isEquals)
        return GetSpec(aResult);

    aResult.Truncate();

    // check pre-path; if they don't match, then return empty string
    nsStandardURL *stdurl2;
    nsresult rv = uri2->QueryInterface(kThisImplCID, (void **) &stdurl2);
    isEquals = NS_SUCCEEDED(rv)
            && SegmentIs(mScheme, stdurl2->mSpec.get(), stdurl2->mScheme)    
            && HostsAreEquivalent(stdurl2)
            && SegmentIs(mUsername, stdurl2->mSpec.get(), stdurl2->mUsername)
            && SegmentIs(mPassword, stdurl2->mSpec.get(), stdurl2->mPassword)
            && (Port() == stdurl2->Port());
    if (!isEquals)
    {
        if (NS_SUCCEEDED(rv))
            NS_RELEASE(stdurl2);
        return NS_OK;
    }

    // scan for first mismatched character
    const char *thisIndex, *thatIndex, *startCharPos;
    startCharPos = mSpec.get() + mDirectory.mPos;
    thisIndex = startCharPos;
    thatIndex = stdurl2->mSpec.get() + mDirectory.mPos;
    while ((*thisIndex == *thatIndex) && *thisIndex)
    {
        thisIndex++;
        thatIndex++;
    }

    // backup to just after previous slash so we grab an appropriate path
    // segment such as a directory (not partial segments)
    // todo:  also check for file matches which include '?', '#', and ';'
    while ((*(thisIndex-1) != '/') && (thisIndex != startCharPos))
        thisIndex--;

    // grab spec from beginning to thisIndex
    aResult = Substring(mSpec, mScheme.mPos, thisIndex - mSpec.get());

    NS_RELEASE(stdurl2);
    return rv;
}

NS_IMETHODIMP
nsStandardURL::GetRelativeSpec(nsIURI *uri2, nsACString &aResult)
{
    NS_ENSURE_ARG_POINTER(uri2);

    aResult.Truncate();

    // if uri's are equal, then return empty string
    PRBool isEquals = PR_FALSE;
    if (NS_SUCCEEDED(Equals(uri2, &isEquals)) && isEquals)
        return NS_OK;

    nsStandardURL *stdurl2;
    nsresult rv = uri2->QueryInterface(kThisImplCID, (void **) &stdurl2);
    isEquals = NS_SUCCEEDED(rv)
            && SegmentIs(mScheme, stdurl2->mSpec.get(), stdurl2->mScheme)    
            && SegmentIs(mHost, stdurl2->mSpec.get(), stdurl2->mHost)
            && SegmentIs(mUsername, stdurl2->mSpec.get(), stdurl2->mUsername)
            && SegmentIs(mPassword, stdurl2->mSpec.get(), stdurl2->mPassword)
            && (Port() == stdurl2->Port());
    if (!isEquals)
    {
        if (NS_SUCCEEDED(rv))
            NS_RELEASE(stdurl2);

        return uri2->GetSpec(aResult);
    }

    // scan for first mismatched character
    const char *thisIndex, *thatIndex, *startCharPos;
    startCharPos = mSpec.get() + mDirectory.mPos;
    thisIndex = startCharPos;
    thatIndex = stdurl2->mSpec.get() + mDirectory.mPos;

#ifdef XP_WIN
    PRBool isFileScheme = SegmentIs(mScheme, "file");
    if (isFileScheme)
    {
        // on windows, we need to match the first segment of the path
        // if these don't match then we need to return an absolute path
        // skip over any leading '/' in path
        while ((*thisIndex == *thatIndex) && (*thisIndex == '/'))
        {
            thisIndex++;
            thatIndex++;
        }
        // look for end of first segment
        while ((*thisIndex == *thatIndex) && *thisIndex && (*thisIndex != '/'))
        {
            thisIndex++;
            thatIndex++;
        }

        // if we didn't match through the first segment, return absolute path
        if ((*thisIndex != '/') || (*thatIndex != '/'))
        {
            NS_RELEASE(stdurl2);
            return uri2->GetSpec(aResult);
        }
    }
#endif

    while ((*thisIndex == *thatIndex) && *thisIndex)
    {
        thisIndex++;
        thatIndex++;
    }

    // backup to just after previous slash so we grab an appropriate path
    // segment such as a directory (not partial segments)
    // todo:  also check for file matches with '#', '?' and ';'
    while ((*(thatIndex-1) != '/') && (thatIndex != startCharPos))
        thatIndex--;

    // need to account for slashes and add corresponding "../"
    while (*thisIndex)
    {
        if (*thisIndex == '/')
            aResult.Append("../");

        thisIndex++;
    }

    // grab spec from thisIndex to end
    PRUint32 startPos = stdurl2->mScheme.mPos + thatIndex - stdurl2->mSpec.get();
    aResult.Append(Substring(stdurl2->mSpec, startPos, 
                             stdurl2->mSpec.Length() - startPos));

    NS_RELEASE(stdurl2);
    return rv;
}

//----------------------------------------------------------------------------
// nsStandardURL::nsIURL
//----------------------------------------------------------------------------

// result may contain unescaped UTF-8 characters
NS_IMETHODIMP
nsStandardURL::GetFilePath(nsACString &result)
{
    result = Filepath();
    return NS_OK;
}

// result may contain unescaped UTF-8 characters
NS_IMETHODIMP
nsStandardURL::GetParam(nsACString &result)
{
    result = Param();
    return NS_OK;
}

// result may contain unescaped UTF-8 characters
NS_IMETHODIMP
nsStandardURL::GetQuery(nsACString &result)
{
    result = Query();
    return NS_OK;
}

// result may contain unescaped UTF-8 characters
NS_IMETHODIMP
nsStandardURL::GetRef(nsACString &result)
{
    result = Ref();
    return NS_OK;
}

// result may contain unescaped UTF-8 characters
NS_IMETHODIMP
nsStandardURL::GetDirectory(nsACString &result)
{
    result = Directory();
    return NS_OK;
}

// result may contain unescaped UTF-8 characters
NS_IMETHODIMP
nsStandardURL::GetFileName(nsACString &result)
{
    result = Filename();
    return NS_OK;
}

// result may contain unescaped UTF-8 characters
NS_IMETHODIMP
nsStandardURL::GetFileBaseName(nsACString &result)
{
    result = Basename();
    return NS_OK;
}

// result may contain unescaped UTF-8 characters
NS_IMETHODIMP
nsStandardURL::GetFileExtension(nsACString &result)
{
    result = Extension();
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetFilePath(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &flat = PromiseFlatCString(input);
    const char *filepath = flat.get();

    LOG(("nsStandardURL::SetFilePath [filepath=%s]\n", filepath));

    // if there isn't a filepath, then there can't be anything
    // after the path either.  this url is likely uninitialized.
    if (mFilepath.mLen < 0)
        return SetPath(flat);

    if (filepath && *filepath) {
        nsCAutoString spec;
        PRUint32 dirPos, basePos, extPos;
        PRInt32 dirLen, baseLen, extLen;
        nsresult rv;

        rv = gNoAuthParser->ParseFilePath(filepath, -1,
                                          &dirPos, &dirLen,
                                          &basePos, &baseLen,
                                          &extPos, &extLen);
        if (NS_FAILED(rv)) return rv;

        // build up new candidate spec
        spec.Assign(mSpec.get(), mPath.mPos);

        // ensure leading '/'
        if (filepath[dirPos] != '/')
            spec.Append('/');

        GET_SEGMENT_ENCODER(encoder);

        // append encoded filepath components
        if (dirLen > 0)
            encoder.EncodeSegment(Substring(filepath + dirPos,
                                            filepath + dirPos + dirLen),
                                  esc_Directory | esc_AlwaysCopy, spec);
        if (baseLen > 0)
            encoder.EncodeSegment(Substring(filepath + basePos,
                                            filepath + basePos + baseLen),
                                  esc_FileBaseName | esc_AlwaysCopy, spec);
        if (extLen >= 0) {
            spec.Append('.');
            if (extLen > 0)
                encoder.EncodeSegment(Substring(filepath + extPos,
                                                filepath + extPos + extLen),
                                      esc_FileExtension | esc_AlwaysCopy,
                                      spec);
        }

        // compute the ending position of the current filepath
        if (mFilepath.mLen >= 0) {
            PRUint32 end = mFilepath.mPos + mFilepath.mLen;
            if (mSpec.Length() > end)
                spec.Append(mSpec.get() + end, mSpec.Length() - end);
        }

        return SetSpec(spec);
    }
    else if (mPath.mLen > 1) {
        mSpec.Cut(mPath.mPos + 1, mFilepath.mLen - 1);
        // left shift param, query, and ref
        ShiftFromParam(1 - mFilepath.mLen);
        // these contain only a '/'
        mPath.mLen = 1;
        mDirectory.mLen = 1;
        mFilepath.mLen = 1;
        // these are no longer defined
        mBasename.mLen = -1;
        mExtension.mLen = -1;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetParam(const nsACString &input)
{
    NS_NOTYETIMPLEMENTED("");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsStandardURL::SetQuery(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &flat = PromiseFlatCString(input);
    const char *query = flat.get();

    LOG(("nsStandardURL::SetQuery [query=%s]\n", query));

    if (mPath.mLen < 0)
        return SetPath(flat);

    InvalidateCache();

    if (!query || !*query) {
        // remove existing query
        if (mQuery.mLen >= 0) {
            // remove query and leading '?'
            mSpec.Cut(mQuery.mPos - 1, mQuery.mLen + 1);
            ShiftFromRef(-(mQuery.mLen + 1));
            mPath.mLen -= (mQuery.mLen + 1);
            mQuery.mPos = 0;
            mQuery.mLen = -1;
        }
        return NS_OK;
    }

    PRInt32 queryLen = strlen(query);
    if (query[0] == '?') {
        query++;
        queryLen--;
    }

    if (mQuery.mLen < 0) {
        if (mRef.mLen < 0)
            mQuery.mPos = mSpec.Length();
        else
            mQuery.mPos = mRef.mPos - 1;
        mSpec.Insert('?', mQuery.mPos);
        mQuery.mPos++;
        mQuery.mLen = 0;
        // the insertion pushes these out by 1
        mPath.mLen++;
        mRef.mPos++;
    }

    // encode query if necessary
    nsCAutoString buf;
    GET_SEGMENT_ENCODER(encoder);
    encoder.EncodeSegmentCount(query, URLSegment(0, queryLen), esc_Query, buf);
    if (!buf.IsEmpty()) {
        query = buf.get();
        queryLen = buf.Length();
    }

    PRInt32 shift = ReplaceSegment(mQuery.mPos, mQuery.mLen, query, queryLen);

    if (shift) {
        mQuery.mLen = queryLen;
        mPath.mLen += shift;
        ShiftFromRef(queryLen - mQuery.mLen);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetRef(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &flat = PromiseFlatCString(input);
    const char *ref = flat.get();

    LOG(("nsStandardURL::SetRef [ref=%s]\n", ref));

    if (mPath.mLen < 0)
        return SetPath(flat);

    InvalidateCache();

    if (!ref || !*ref) {
        // remove existing ref
        if (mRef.mLen >= 0) {
            // remove ref and leading '#'
            mSpec.Cut(mRef.mPos - 1, mRef.mLen + 1);
            mPath.mLen -= (mRef.mLen + 1);
            mRef.mPos = 0;
            mRef.mLen = -1;
        }
        return NS_OK;
    }
            
    PRInt32 refLen = strlen(ref);
    if (ref[0] == '#') {
        ref++;
        refLen--;
    }
    
    if (mRef.mLen < 0) {
        mSpec.Append('#');
        mRef.mPos = mSpec.Length();
        mRef.mLen = 0;
    }

    // encode ref if necessary
    nsCAutoString buf;
    GET_SEGMENT_ENCODER(encoder);
    encoder.EncodeSegmentCount(ref, URLSegment(0, refLen), esc_Ref, buf);
    if (!buf.IsEmpty()) {
        ref = buf.get();
        refLen = buf.Length();
    }

    ReplaceSegment(mRef.mPos, mRef.mLen, ref, refLen);
    mPath.mLen += (refLen - mRef.mLen);
    mRef.mLen = refLen;
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetDirectory(const nsACString &input)
{
    NS_NOTYETIMPLEMENTED("");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsStandardURL::SetFileName(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &flat = PromiseFlatCString(input);
    const char *filename = flat.get();

    LOG(("nsStandardURL::SetFileName [filename=%s]\n", filename));

    if (mPath.mLen < 0)
        return SetPath(flat);

    PRInt32 shift = 0;

    if (!(filename && *filename)) {
        // remove the filename
        if (mBasename.mLen > 0) {
            if (mExtension.mLen >= 0)
                mBasename.mLen += (mExtension.mLen + 1);
            mSpec.Cut(mBasename.mPos, mBasename.mLen);
            shift = -mBasename.mLen;
            mBasename.mLen = 0;
            mExtension.mLen = -1;
        }
    }
    else {
	    nsresult rv;
	    URLSegment basename, extension;

	    // let the parser locate the basename and extension
	    rv = gNoAuthParser->ParseFileName(filename, -1,
	                                      &basename.mPos, &basename.mLen,
	                                      &extension.mPos, &extension.mLen);
	    if (NS_FAILED(rv)) return rv;

	    if (basename.mLen < 0) {
	        // remove existing filename
	        if (mBasename.mLen >= 0) {
	            PRUint32 len = mBasename.mLen;
	            if (mExtension.mLen >= 0)
	                len += (mExtension.mLen + 1);
	            mSpec.Cut(mBasename.mPos, len);
	            shift = -PRInt32(len);
	            mBasename.mLen = 0;
	            mExtension.mLen = -1;
	        }
	    }
	    else {
	        nsCAutoString newFilename;
            GET_SEGMENT_ENCODER(encoder);
            basename.mLen = encoder.EncodeSegmentCount(filename, basename,
                                                       esc_FileBaseName |
                                                       esc_AlwaysCopy,
                                                       newFilename);
	        if (extension.mLen >= 0) {
	            newFilename.Append('.');
                extension.mLen = encoder.EncodeSegmentCount(filename, extension,
                                                            esc_FileExtension|esc_AlwaysCopy,
                                                            newFilename);
	        }

	        if (mBasename.mLen < 0) {
	            // insert new filename
	            mBasename.mPos = mDirectory.mPos + mDirectory.mLen;
	            mSpec.Insert(newFilename, mBasename.mPos);
	            shift = newFilename.Length();
	        }
	        else {
	            // replace existing filename
	            PRUint32 oldLen = PRUint32(mBasename.mLen);
	            if (mExtension.mLen >= 0)
	                oldLen += (mExtension.mLen + 1);
	            mSpec.Replace(mBasename.mPos, oldLen, newFilename);
	            shift = newFilename.Length() - oldLen;
	        }
	        
	        mBasename.mLen = basename.mLen;
	        mExtension.mLen = extension.mLen;
	        if (mExtension.mLen >= 0)
	            mExtension.mPos = mBasename.mPos + mBasename.mLen + 1;
	    }
	}
    if (shift) {
        ShiftFromParam(shift);
        mFilepath.mLen += shift;
        mPath.mLen += shift;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetFileBaseName(const nsACString &input)
{
    NS_NOTYETIMPLEMENTED("");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsStandardURL::SetFileExtension(const nsACString &input)
{
    NS_NOTYETIMPLEMENTED("");
    return NS_ERROR_NOT_IMPLEMENTED;
}

//----------------------------------------------------------------------------
// nsStandardURL::nsIFileURL
//----------------------------------------------------------------------------

NS_IMETHODIMP
nsStandardURL::GetFile(nsIFile **result)
{
    // use cached result if present
    if (mFile) {
        NS_ADDREF(*result = mFile);
        return NS_OK;
    }

    if (mSpec.IsEmpty()) {
        NS_ERROR("url not initialized");
        return NS_ERROR_NOT_INITIALIZED;
    }

    if (!SegmentIs(mScheme, "file")) {
        NS_ERROR("not a file URL");
        return NS_ERROR_FAILURE;
    }

    nsresult rv;
    nsCOMPtr<nsILocalFile> localFile(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv));
    if (NS_FAILED(rv)) return rv;

    rv = NS_InitFileFromURLSpec(localFile, mSpec);
    if (NS_FAILED(rv)) return rv;

#if defined(PR_LOGGING)
    if (LOG_ENABLED()) {
        nsCAutoString path;
        localFile->GetNativePath(path);
        LOG(("nsStandardURL::GetFile [this=%p spec=%s resulting_path=%s]\n",
            this, mSpec.get(), path.get()));
    }
#endif

    return CallQueryInterface(localFile, result);
}

NS_IMETHODIMP
nsStandardURL::SetFile(nsIFile *file)
{
    ENSURE_MUTABLE();

    NS_PRECONDITION(file, "null pointer");

    nsresult rv;
    nsCAutoString url;

    rv = NS_GetURLSpecFromFile(file, url);
    if (NS_FAILED(rv)) return rv;

    rv = SetSpec(url);

    // must clone |file| since its value is not guaranteed to remain constant
    if (NS_SUCCEEDED(rv)) {
        InvalidateCache();
        if (NS_FAILED(file->Clone(getter_AddRefs(mFile)))) {
            NS_WARNING("nsIFile::Clone failed");
            // failure to clone is not fatal (GetFile will generate mFile)
            mFile = 0;
        }
    }
    return rv;
}

//----------------------------------------------------------------------------
// nsStandardURL::nsIStandardURL
//----------------------------------------------------------------------------

NS_IMETHODIMP
nsStandardURL::Init(PRUint32 urlType,
                    PRInt32 defaultPort,
                    const nsACString &spec,
                    const char *charset,
                    nsIURI *baseURI)
{
    ENSURE_MUTABLE();

    InvalidateCache();

    switch (urlType) {
    case URLTYPE_STANDARD:
        mParser = gStdParser;
        break;
    case URLTYPE_AUTHORITY:
        mParser = gAuthParser;
        break;
    case URLTYPE_NO_AUTHORITY:
        mParser = gNoAuthParser;
        break;
    default:
        NS_NOTREACHED("bad urlType");
        return NS_ERROR_INVALID_ARG;
    }
    mDefaultPort = defaultPort;

    if (charset == nsnull || *charset == '\0') {
        mOriginCharset.Truncate();
        // check if baseURI provides an origin charset and use that.
        if (baseURI)
            baseURI->GetOriginCharset(mOriginCharset);
    }
    else
        mOriginCharset = charset;

    // an empty charset implies UTF-8
    if (mOriginCharset.EqualsIgnoreCase("UTF-8"))
        mOriginCharset.Truncate();

    if (spec.IsEmpty()) {
        Clear();
        return NS_OK;
    }

    if (!baseURI)
        return SetSpec(spec);

    nsCAutoString buf;
    nsresult rv = baseURI->Resolve(spec, buf);
    if (NS_FAILED(rv)) return rv;

    return SetSpec(buf);
}

NS_IMETHODIMP
nsStandardURL::GetMutable(PRBool *value)
{
    *value = mMutable;
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetMutable(PRBool value)
{
    mMutable = value;
    return NS_OK;
}

//----------------------------------------------------------------------------
// nsStandardURL::nsISerializable
//----------------------------------------------------------------------------

NS_IMETHODIMP
nsStandardURL::Read(nsIObjectInputStream *stream)
{
    nsresult rv;
    nsXPIDLCString buf;
    
    rv = stream->Read32(&mURLType);
    if (NS_FAILED(rv)) return rv;
    switch (mURLType) {
      case URLTYPE_STANDARD:
        mParser = gStdParser;
        break;
      case URLTYPE_AUTHORITY:
        mParser = gAuthParser;
        break;
      case URLTYPE_NO_AUTHORITY:
        mParser = gNoAuthParser;
        break;
      default:
        NS_NOTREACHED("bad urlType");
        return NS_ERROR_FAILURE;
    }

    rv = stream->Read32((PRUint32 *) &mPort);
    if (NS_FAILED(rv)) return rv;

    rv = stream->Read32((PRUint32 *) &mDefaultPort);
    if (NS_FAILED(rv)) return rv;

    rv = NS_ReadOptionalStringZ(stream, getter_Copies(buf));
    if (NS_FAILED(rv)) return rv;
    mSpec = buf;

    rv = ReadSegment(stream, mScheme);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mAuthority);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mUsername);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mPassword);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mHost);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mPath);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mFilepath);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mDirectory);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mBasename);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mExtension);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mParam);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mQuery);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mRef);
    if (NS_FAILED(rv)) return rv;

    rv = NS_ReadOptionalStringZ(stream, getter_Copies(buf));
    if (NS_FAILED(rv)) return rv;
    mOriginCharset = buf;

    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::Write(nsIObjectOutputStream *stream)
{
    nsresult rv;

    rv = stream->Write32(mURLType);
    if (NS_FAILED(rv)) return rv;

    rv = stream->Write32(PRUint32(mPort));
    if (NS_FAILED(rv)) return rv;

    rv = stream->Write32(PRUint32(mDefaultPort));
    if (NS_FAILED(rv)) return rv;

    rv = NS_WriteOptionalStringZ(stream, mSpec.get());
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mScheme);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mAuthority);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mUsername);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mPassword);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mHost);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mPath);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mFilepath);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mDirectory);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mBasename);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mExtension);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mParam);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mQuery);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mRef);
    if (NS_FAILED(rv)) return rv;

    rv = NS_WriteOptionalStringZ(stream, mOriginCharset.get());
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}
