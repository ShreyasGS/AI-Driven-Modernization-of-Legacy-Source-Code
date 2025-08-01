/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Ryan Cassin.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Ryan Cassin <rcassin@supernova.org>
 *    Kathleen Brade <brade@netscape.com>
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
 
function checkPipelining()
{
  try {
    var browserEnableHTTP11 = document.getElementById("httpVersion11");
    var browserEnableKeepAlive = document.getElementById("browserEnableKeepAlive");
    var browserEnablePipelining = document.getElementById("browserEnablePipelining");

    if (browserEnableHTTP11.selected && browserEnableKeepAlive.checked) {
      browserEnablePipelining.removeAttribute("disabled");
    } else {
      browserEnablePipelining.setAttribute("disabled", "true");
      browserEnablePipelining.setAttribute("checked", "false");
    }
  } catch(e) {}
}

/* Function to restore pref values to application defaults */
function restoreAcceptEncoding()
{
  try {
    var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                                .getService(Components.interfaces.nsIPrefService);
    var prefs = prefService.getDefaultBranch(null);

    /* get the current pref setting */
    var prefValue = prefs.getCharPref("network.http.accept-encoding");
    var editfield = document.getElementById("acceptEncodingString");
    if (editfield)
      editfield.value = prefValue;
  } catch (e) {}
}

