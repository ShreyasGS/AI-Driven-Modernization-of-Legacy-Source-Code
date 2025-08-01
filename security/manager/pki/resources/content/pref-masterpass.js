/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
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
 *  Javier Delgadillo <javi@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

const nsIPK11Token = Components.interfaces.nsIPK11Token;
const nsPK11TokenDB = "@mozilla.org/security/pk11tokendb;1";
const nsIPK11TokenDB = Components.interfaces.nsIPK11TokenDB;
const nsIDialogParamBlock = Components.interfaces.nsIDialogParamBlock;
const nsDialogParamBlock = "@mozilla.org/embedcomp/dialogparam;1";

var internal_token;

function onMasterPasswordLoad()
{
  var tokendb = Components.classes[nsPK11TokenDB].getService(nsIPK11TokenDB);
  internal_token = tokendb.getInternalKeyToken();
  var askTimes = internal_token.getAskPasswordTimes();
  switch (askTimes) {
  case nsIPK11Token.ASK_FIRST_TIME:  askTimes = 0; break;
  case nsIPK11Token.ASK_EVERY_TIME:  askTimes = 1; break;
  case nsIPK11Token.ASK_EXPIRE_TIME: askTimes = 2; break;
  }
  var radiogroup = document.getElementById("passwordAskTimes");
  var radioitem;
  switch (askTimes) {
  case 0: radioitem = document.getElementById("askFirstTime"); break;
  case 1: radioitem = document.getElementById("askEveryTime"); break;
  case 2: radioitem = document.getElementById("askTimeout"); break;
  }
  radiogroup.selectedItem = radioitem;
  var timeout = internal_token.getAskPasswordTimeout();
  var timeoutField = document.getElementById("passwordTimeout");
  timeoutField.setAttribute("value", timeout);
  
  changePasswordSettings();
}

function changePasswordSettings()
{
  var askTimes = 0;
  var timeout = internal_token.getAskPasswordTimeout();
  var timeoutField = document.getElementById("passwordTimeout");
  var radiogroup = document.getElementById("passwordAskTimes");
  switch ( radiogroup.value ) {
  case "0": 
    timeoutField.setAttribute("disabled", true);
    askTimes = nsIPK11Token.ASK_FIRST_TIME;
    break;
  case "1": 
    timeoutField.setAttribute("disabled", true);
    askTimes = nsIPK11Token.ASK_EVERY_TIME;
    break;
  case "2":
    timeoutField.removeAttribute("disabled");
    timeoutField.focus();
    timeout = timeoutField.value;
    var re = new RegExp("^[0-9]+$");
    if (!re.test(timeout)) {
      timeout = "1";
    }
    askTimes = nsIPK11Token.ASK_EXPIRE_TIME;
    break;
  }
  internal_token.setAskPasswordDefaults(askTimes, timeout);

  var askEveryTimeHidden = document.getElementById("askEveryTimeHidden");
  askEveryTimeHidden.checked = (radiogroup.value == 1) ? true : false;
}

function ChangePW()
{
  var params = Components.classes[nsDialogParamBlock].createInstance(nsIDialogParamBlock);
  params.SetString(1,"_blank");
  window.openDialog("chrome://pippki/content/changepassword.xul","",
              "chrome,resizable=1,modal=1,dialog=1",params);
}

function ResetPW()
{
  window.open("chrome://pippki/content/resetpassword.xul",
              internal_token.tokenName,
              "centerscreen,chrome,resizable=1,modal=1,dialog=1");
}
