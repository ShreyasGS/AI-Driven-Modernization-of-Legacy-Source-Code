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
 *   Pierre Phaneuf <pp@ludusdesign.com>
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

#include "nspr.h"
#include "nsString.h"
#include "pratom.h"
#include "nsCOMPtr.h"
#include "nsIFactory.h"
#include "nsIRegistry.h"
#include "nsIGenericFactory.h"
#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsIModule.h"
#include "nsUCvCnCID.h"
#include "nsUCvCnDll.h"
#include "nsCRT.h"

#include "nsHZToUnicode.h"
#include "nsUnicodeToHZ.h"
#include "nsGBKToUnicode.h"
#include "nsUnicodeToGBK.h"
#include "nsUnicodeToGBKNoAscii.h"
#include "nsCP936ToUnicode.h"
#include "nsUnicodeToCP936.h"
#include "nsGB2312ToUnicodeV2.h"
#include "nsUnicodeToGB2312V2.h"
#include "nsUnicodeToGB2312GL.h"
#include "gbku.h"

//----------------------------------------------------------------------------
// Global functions and data [declaration]

static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);

#define DECODER_NAME_BASE "Unicode Decoder-"
#define ENCODER_NAME_BASE "Unicode Encoder-"

//----------------------------------------------------------------------------

NS_CONVERTER_REGISTRY_START
NS_UCONV_REG_UNREG("GB2312", "Unicode" , NS_GB2312TOUNICODE_CID)
NS_UCONV_REG_UNREG("Unicode", "GB2312",  NS_UNICODETOGB2312_CID)
NS_UCONV_REG_UNREG("windows-936", "Unicode" , NS_CP936TOUNICODE_CID)
NS_UCONV_REG_UNREG("Unicode", "windows-936",  NS_UNICODETOCP936_CID)
NS_UCONV_REG_UNREG("x-gbk", "Unicode" , NS_GBKTOUNICODE_CID)
NS_UCONV_REG_UNREG("Unicode", "x-gbk",  NS_UNICODETOGBK_CID)
NS_UCONV_REG_UNREG("Unicode", "x-gbk-noascii",  NS_UNICODETOGBKNOASCII_CID)
NS_UCONV_REG_UNREG("HZ-GB-2312", "Unicode" , NS_HZTOUNICODE_CID)
NS_UCONV_REG_UNREG("Unicode", "HZ-GB-2312",  NS_UNICODETOHZ_CID)
NS_UCONV_REG_UNREG("Unicode", "gb_2312-80",  NS_UNICODETOGB2312GL_CID)
NS_UCONV_REG_UNREG("gb18030", "Unicode" , NS_GB18030TOUNICODE_CID)
NS_UCONV_REG_UNREG("Unicode", "gb18030",  NS_UNICODETOGB18030_CID)
NS_UCONV_REG_UNREG("Unicode", "gb18030.2000-0",  NS_UNICODETOGB18030Font0_CID)
NS_UCONV_REG_UNREG("Unicode", "gb18030.2000-1",  NS_UNICODETOGB18030Font1_CID)
NS_CONVERTER_REGISTRY_END

NS_IMPL_NSUCONVERTERREGSELF

NS_GENERIC_FACTORY_CONSTRUCTOR(nsGB2312ToUnicodeV2);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToGB2312V2);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCP936ToUnicode);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToCP936);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsGBKToUnicode);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToGBK);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToGBKNoAscii);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsHZToUnicode);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToHZ);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToGB2312GL);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsGB18030ToUnicode);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToGB18030);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToGB18030Font0);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToGB18030Font1);


static const nsModuleComponentInfo components[] = 
{
  { 
    DECODER_NAME_BASE "GB2312" , NS_GB2312TOUNICODE_CID, 
    NS_UNICODEDECODER_CONTRACTID_BASE "GB2312",
    nsGB2312ToUnicodeV2Constructor ,
    // global converter registration
    nsUConverterRegSelf, nsUConverterUnregSelf,
  },
  { 
    ENCODER_NAME_BASE "GB2312" , NS_UNICODETOGB2312_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "GB2312",
    nsUnicodeToGB2312V2Constructor, 
  },
  { 
    DECODER_NAME_BASE "windows-936" , NS_CP936TOUNICODE_CID, 
    NS_UNICODEDECODER_CONTRACTID_BASE "windows-936",
    nsCP936ToUnicodeConstructor ,
  },
  { 
    ENCODER_NAME_BASE "windows-936" , NS_UNICODETOCP936_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "windows-936",
    nsUnicodeToCP936Constructor, 
  },
  { 
    DECODER_NAME_BASE "x-gbk" , NS_GBKTOUNICODE_CID, 
    NS_UNICODEDECODER_CONTRACTID_BASE "x-gbk",
    nsGBKToUnicodeConstructor ,
  },
  { 
    ENCODER_NAME_BASE "x-gbk" , NS_UNICODETOGBK_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-gbk",
    nsUnicodeToGBKConstructor, 
  },  
  { 
    ENCODER_NAME_BASE "x-gbk-noascii" , NS_UNICODETOGBKNOASCII_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-gbk-noascii",
    nsUnicodeToGBKNoAsciiConstructor, 
  },  
  { 
    DECODER_NAME_BASE "HZ-GB-2312" , NS_HZTOUNICODE_CID, 
    NS_UNICODEDECODER_CONTRACTID_BASE "HZ-GB-2312",
    nsHZToUnicodeConstructor ,
  },  
  { 
    ENCODER_NAME_BASE "HZ-GB-2312" , NS_UNICODETOHZ_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "HZ-GB-2312",
    nsUnicodeToHZConstructor, 
  },  
  { 
    ENCODER_NAME_BASE "gb_2312-80" , NS_UNICODETOGB2312GL_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "gb_2312-80",
    nsUnicodeToGB2312GLConstructor, 
  },
  { 
    DECODER_NAME_BASE "gb18030" , NS_GB18030TOUNICODE_CID, 
    NS_UNICODEDECODER_CONTRACTID_BASE "gb18030",
    nsGB18030ToUnicodeConstructor ,
  },  
  { 
    ENCODER_NAME_BASE "gb18030.2000-0" , NS_UNICODETOGB18030Font0_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "gb18030.2000-0",
    nsUnicodeToGB18030Font0Constructor, 
  },  
  { 
    ENCODER_NAME_BASE "gb18030.2000-1" , NS_UNICODETOGB18030Font1_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "gb18030.2000-1",
    nsUnicodeToGB18030Font1Constructor, 
  },  
  { 
    ENCODER_NAME_BASE "gb18030" , NS_UNICODETOGB18030_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "gb18030",
    nsUnicodeToGB18030Constructor, 
  }
};

NS_IMPL_NSGETMODULE(nsUCvCnModule, components);
