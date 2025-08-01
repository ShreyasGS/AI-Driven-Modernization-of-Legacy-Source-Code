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
 *   Roy Yokoyama <yokoyama@netscape.com>
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
#include "nsCRT.h"
#include "pratom.h"
#include "nsCOMPtr.h"
#include "nsIFactory.h"
#include "nsIRegistry.h"
#include "nsIGenericFactory.h"
#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsIModule.h"
#include "nsUCvTW2CID.h"
#include "nsUCvTW2Dll.h"

#include "nsEUCTWToUnicode.h"
#include "nsUnicodeToEUCTW.h"
#include "nsUnicodeToCNS11643p1.h"
#include "nsUnicodeToCNS11643p2.h"
#include "nsUnicodeToCNS11643p3.h"
#include "nsUnicodeToCNS11643p4.h"
#include "nsUnicodeToCNS11643p5.h"
#include "nsUnicodeToCNS11643p6.h"
#include "nsUnicodeToCNS11643p7.h"

//----------------------------------------------------------------------------
// Global functions and data [declaration]

static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);

#define DECODER_NAME_BASE "Unicode Decoder-"
#define ENCODER_NAME_BASE "Unicode Encoder-"

PRUint16 g_ufCNS1MappingTable[] = {
#include "cns_1.uf"
};

PRUint16 g_ufCNS2MappingTable[] = {
#include "cns_2.uf"
};

PRUint16 g_ufCNS3MappingTable[] = {
#include "cns3.uf"
};

PRUint16 g_ufCNS4MappingTable[] = {
#include "cns4.uf"
};

PRUint16 g_ufCNS5MappingTable[] = {
#include "cns5.uf"
};

PRUint16 g_ufCNS6MappingTable[] = {
#include "cns6.uf"
};

PRUint16 g_ufCNS7MappingTable[] = {
#include "cns7.uf"
};

PRUint16 g_utCNS1MappingTable[] = {
#include "cns_1.ut"
};

PRUint16 g_utCNS2MappingTable[] = {
#include "cns_2.ut"
};

PRUint16 g_utCNS3MappingTable[] = {
#include "cns3.ut"
};

PRUint16 g_utCNS4MappingTable[] = {
#include "cns4.ut"
};

PRUint16 g_utCNS5MappingTable[] = {
#include "cns5.ut"
};

PRUint16 g_utCNS6MappingTable[] = {
#include "cns6.ut"
};

PRUint16 g_utCNS7MappingTable[] = {
#include "cns7.ut"
};

PRUint16 g_ASCIIMappingTable[] = {
  0x0001, 0x0004, 0x0005, 0x0008, 0x0000, 0x0000, 0x007F, 0x0000
};

NS_CONVERTER_REGISTRY_START
NS_UCONV_REG_UNREG("x-euc-tw", "Unicode" , NS_EUCTWTOUNICODE_CID)
NS_UCONV_REG_UNREG("Unicode", "x-euc-tw" , NS_UNICODETOEUCTW_CID)
NS_UCONV_REG_UNREG("Unicode", "x-cns-11643-1" , NS_UNICODETOCNS11643P1_CID)
NS_UCONV_REG_UNREG("Unicode", "x-cns-11643-2" , NS_UNICODETOCNS11643P2_CID)
NS_UCONV_REG_UNREG("Unicode", "x-cns-11643-3" , NS_UNICODETOCNS11643P3_CID)
NS_UCONV_REG_UNREG("Unicode", "x-cns-11643-4" , NS_UNICODETOCNS11643P4_CID)
NS_UCONV_REG_UNREG("Unicode", "x-cns-11643-5" , NS_UNICODETOCNS11643P5_CID)
NS_UCONV_REG_UNREG("Unicode", "x-cns-11643-6" , NS_UNICODETOCNS11643P6_CID)
NS_UCONV_REG_UNREG("Unicode", "x-cns-11643-7" , NS_UNICODETOCNS11643P7_CID)
NS_CONVERTER_REGISTRY_END

NS_IMPL_NSUCONVERTERREGSELF

NS_GENERIC_FACTORY_CONSTRUCTOR(nsEUCTWToUnicode);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToEUCTW);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToCNS11643p1);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToCNS11643p2);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToCNS11643p3);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToCNS11643p4);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToCNS11643p5);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToCNS11643p6);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToCNS11643p7);

static const nsModuleComponentInfo components[] = 
{
  { 
    DECODER_NAME_BASE "x-euc-tw" , NS_EUCTWTOUNICODE_CID, 
    NS_UNICODEDECODER_CONTRACTID_BASE "x-euc-tw",
    nsEUCTWToUnicodeConstructor,
    // global converter registration
    nsUConverterRegSelf, nsUConverterUnregSelf
  },
  { 
    ENCODER_NAME_BASE "x-euc-tw" , NS_UNICODETOEUCTW_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-euc-tw",
    nsUnicodeToEUCTWConstructor,
  },
  { 
    ENCODER_NAME_BASE "x-cns-11643-1" , NS_UNICODETOCNS11643P1_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-cns-11643-1",
    nsUnicodeToCNS11643p1Constructor,
  },
  { 
    ENCODER_NAME_BASE "x-cns-11643-2" , NS_UNICODETOCNS11643P2_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-cns-11643-2",
    nsUnicodeToCNS11643p2Constructor,
  },
  { 
    ENCODER_NAME_BASE "x-cns-11643-3" , NS_UNICODETOCNS11643P3_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-cns-11643-3",
    nsUnicodeToCNS11643p3Constructor,
  },
  { 
    ENCODER_NAME_BASE "x-cns-11643-4" , NS_UNICODETOCNS11643P4_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-cns-11643-4",
    nsUnicodeToCNS11643p4Constructor,
  },
  { 
    ENCODER_NAME_BASE "x-cns-11643-5" , NS_UNICODETOCNS11643P5_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-cns-11643-5",
    nsUnicodeToCNS11643p5Constructor,
  },
  { 
    ENCODER_NAME_BASE "x-cns-11643-6" , NS_UNICODETOCNS11643P6_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-cns-11643-6",
    nsUnicodeToCNS11643p6Constructor,
  },
  { 
    ENCODER_NAME_BASE "x-cns-11643-7" , NS_UNICODETOCNS11643P7_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-cns-11643-7",
    nsUnicodeToCNS11643p7Constructor,
  }
};

NS_IMPL_NSGETMODULE(nsUCvTW2Module, components);
