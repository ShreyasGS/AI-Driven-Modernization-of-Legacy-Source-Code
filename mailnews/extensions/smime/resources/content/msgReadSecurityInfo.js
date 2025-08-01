/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corp..
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s): Kai Engert <kaie@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

const nsIPKIParamBlock = Components.interfaces.nsIPKIParamBlock;
const nsIDialogParamBlock = Components.interfaces.nsIDialogParamBlock;
const nsIX509Cert = Components.interfaces.nsIX509Cert;
const nsICMSMessageErrors = Components.interfaces.nsICMSMessageErrors;

var gSignerCert = null;
var gEncryptionCert = null;

var gSignatureStatus = -1;
var gEncryptionStatus = -1;

var params = null;

function setText(id, value) {
  var element = document.getElementById(id);
  if (!element)
    return;
  if (element.hasChildNodes())
    element.removeChild(element.firstChild);
  var textNode = document.createTextNode(value);
  element.appendChild(textNode);
}

function onLoad()
{
  document.getElementById("cancel").setAttribute("collapsed", "true");

  var pkiParams = window.arguments[0].QueryInterface(nsIPKIParamBlock);
  var isupport = pkiParams.getISupportAtIndex(1);
  if (isupport) {
    gSignerCert = isupport.QueryInterface(nsIX509Cert);
  }
  isupport = pkiParams.getISupportAtIndex(2);
  if (isupport) {
    gEncryptionCert = isupport.QueryInterface(nsIX509Cert);
  }
  
  params = pkiParams.QueryInterface(nsIDialogParamBlock);
  
  gSignatureStatus = params.GetInt(1);
  gEncryptionStatus = params.GetInt(2);
  
  var bundle = document.getElementById("bundle_smime_read_info");

  if (bundle) {
    var sigInfoLabel = null;
    var sigInfoHeader = null;
    var sigInfo = null;
    var sigInfo_clueless = false;

    switch (gSignatureStatus) {
      case -1:
      case nsICMSMessageErrors.VERIFY_NOT_SIGNED:
        sigInfoLabel = "SINoneLabel";
        sigInfo = "SINone";
        break;

      case nsICMSMessageErrors.SUCCESS:
        sigInfoLabel = "SIValidLabel";
        sigInfo = "SIValid";
        break;


      case nsICMSMessageErrors.VERIFY_BAD_SIGNATURE:
      case nsICMSMessageErrors.VERIFY_DIGEST_MISMATCH:
        sigInfoLabel = "SIInvalidLabel";
        sigInfoHeader = "SIInvalidHeader";
        sigInfo = "SIContentAltered";
        break;

      case nsICMSMessageErrors.VERIFY_UNKNOWN_ALGO:
      case nsICMSMessageErrors.VERIFY_UNSUPPORTED_ALGO:
        sigInfoLabel = "SIInvalidLabel";
        sigInfoHeader = "SIInvalidHeader";
        sigInfo = "SIInvalidCipher";
        break;

      case nsICMSMessageErrors.VERIFY_HEADER_MISMATCH:
        sigInfoLabel = "SIInvalidLabel";
        sigInfoHeader = "SIInvalidHeader";
        sigInfo = "SIHeaderMismatch";
        break;

      case nsICMSMessageErrors.VERIFY_UNTRUSTED:
        sigInfoLabel = "SIInvalidLabel";
        sigInfoHeader = "SIInvalidHeader";
        sigInfo = "SIUntrustedCA";
        // XXX Need to extend to communicate better errors
        // might also be:
        // SIExpired SIRevoked SINotYetValid SIUnknownCA SIExpiredCA SIRevokedCA SINotYetValidCA
        break;

      case nsICMSMessageErrors.VERIFY_NOT_YET_ATTEMPTED:
      case nsICMSMessageErrors.GENERAL_ERROR:
      case nsICMSMessageErrors.VERIFY_NO_CONTENT_INFO:
      case nsICMSMessageErrors.VERIFY_BAD_DIGEST:
      case nsICMSMessageErrors.VERIFY_NOCERT:
      case nsICMSMessageErrors.VERIFY_ERROR_UNVERIFIED:
      case nsICMSMessageErrors.VERIFY_ERROR_PROCESSING:
      case nsICMSMessageErrors.VERIFY_MALFORMED_SIGNATURE:
        sigInfoLabel = "SIInvalidLabel";
        sigInfoHeader = "SIInvalidHeader";
        sigInfo_clueless = true;
        break;
    }

    
    document.getElementById("signatureLabel").value = 
      bundle.getString(sigInfoLabel);

    if (sigInfoHeader) {
      var l = document.getElementById("signatureHeader");
      l.collapsed = false;
      l.value = bundle.getString(sigInfoHeader);
    }
    
    var str;
    if (sigInfo) {
      str = bundle.getString(sigInfo);
    }
    else if (sigInfo_clueless) {
      str = bundle.getString("SIClueless") + " (" + gSignatureStatus + ")";
    }
    setText("signatureExplanation", str);
    

    var encInfoLabel = null;
    var encInfoHeader = null;
    var encInfo = null;
    var encInfo_clueless = false;

    switch (gEncryptionStatus) {
      case -1:
        encInfoLabel = "EINoneLabel";
        encInfo = "EINone";
        break;

      case nsICMSMessageErrors.SUCCESS:
        encInfoLabel = "EIValidLabel";
        encInfo = "EIValid";
        break;

      case nsICMSMessageErrors.GENERAL_ERROR:
        encInfoLabel = "EIInvalidLabel";
        encInfoHeader = "EIInvalidHeader";
        encInfo_clueless = 1;
        break;
    }


    document.getElementById("encryptionLabel").value = 
      bundle.getString(encInfoLabel);

    if (encInfoHeader) {
      var l = document.getElementById("encryptionHeader");
      l.collapsed = false;
      l.value = bundle.getString(encInfoHeader);
    }
    
    var str;
    if (encInfo) {
      str = bundle.getString(encInfo);
    }
    else if (encInfo_clueless) {
      str = bundle.getString("EIClueless");
    }
    setText("encryptionExplanation", str);
  }
  
  if (gSignerCert) {
    document.getElementById("signatureCert").collapsed = false;
    if (gSignerCert.subjectName) {
      document.getElementById("signedBy").value = gSignerCert.commonName;
    }
    if (gSignerCert.emailAddress) {
      document.getElementById("signerEmail").value = gSignerCert.emailAddress;
    }
    if (gSignerCert.issuerName) {
      document.getElementById("sigCertIssuedBy").value = gSignerCert.issuerCommonName;
    }
  }

  if (gEncryptionCert) {
    document.getElementById("encryptionCert").collapsed = false;
    if (gEncryptionCert.subjectName) {
      document.getElementById("encryptedFor").value = gEncryptionCert.commonName;
    }
    if (gEncryptionCert.emailAddress) {
      document.getElementById("recipientEmail").value = gEncryptionCert.emailAddress;
    }
    if (gEncryptionCert.issuerName) {
      document.getElementById("encCertIssuedBy").value = gEncryptionCert.issuerCommonName;
    }
  }
}

function viewSignatureCert()
{
  if (gSignerCert) {
    gSignerCert.view();
  }
}

function viewEncryptionCert()
{
  if (gEncryptionCert) {
    gEncryptionCert.view();
  }
}

function doHelpButton()
{
  openHelp('received_security');
}
