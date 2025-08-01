/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 *   Alec Flett <alecf@netscape.com>
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

/**
 * Communicator Shared Utility Library
 * for shared application glue for the Communicator suite of applications
 **/

/*
  Note: All Editor/Composer-related methods have been moved to editorApplicationOverlay.js,
  so app windows that require those must include editorNavigatorOverlay.xul
*/

/**
 * Go into online/offline mode
 **/

const kIOServiceProgID = "@mozilla.org/network/io-service;1";
const kObserverServiceProgID = "@mozilla.org/observer-service;1";

function toggleOfflineStatus()
{
  var checkfunc;
  try {
    checkfunc = document.getElementById("offline-status").getAttribute('checkfunc');
  }
  catch (ex) {
    checkfunc = null;
  }

  var ioService = Components.classes[kIOServiceProgID]
                            .getService(Components.interfaces.nsIIOService);
  if (checkfunc) {
    if (!eval(checkfunc)) {
      // the pre-offline check function returned false, so don't go offline
      return;
    }
  }
  ioService.offline = !ioService.offline;
}

function setOfflineUI(offline)
{
  var broadcaster = document.getElementById("Communicator:WorkMode");
  var panel = document.getElementById("offline-status");
  if (!broadcaster || !panel) return;

  //Checking for a preference "network.online", if it's locked, disabling 
  // network icon and menu item
  var prefService = Components.classes["@mozilla.org/preferences-service;1"];
  prefService = prefService.getService();
  prefService = prefService.QueryInterface(Components.interfaces.nsIPrefService);
  
  var prefBranch = prefService.getBranch(null);
  
  var offlineLocked = prefBranch.prefIsLocked("network.online"); 
  
  if (offlineLocked ) {
      broadcaster.setAttribute("disabled","true");
  }

  var bundle = srGetStrBundle("chrome://communicator/locale/utilityOverlay.properties");

  if (offline)
    {
      broadcaster.setAttribute("offline", "true");
      panel.setAttribute("tooltiptext", bundle.GetStringFromName("offlineTooltip"));
      broadcaster.setAttribute("label", bundle.GetStringFromName("goonline"));
    }
  else
    {
      broadcaster.removeAttribute("offline");
      panel.setAttribute("tooltiptext", bundle.GetStringFromName("onlineTooltip"));
      broadcaster.setAttribute("label", bundle.GetStringFromName("gooffline"));
    }
}

var goPrefWindow = 0;

function getBrowserURL() {

  try {
    var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                         .getService(Components.interfaces.nsIPrefBranch);
    var url = prefs.getCharPref("browser.chromeURL");
    if (url)
      return url;
  } catch(e) {
  }
  return "chrome://navigator/content/navigator.xul";
}

function goPageSetup(domwin, printSettings)
{
  try {
    if (printSettings == null) {
      alert("PrintSettings arg is null!");
    }

    // This code calls the printoptions service to bring up the printoptions
    // dialog.  This will be an xp dialog if the platform did not override
    // the ShowPrintSetupDialog method.
    var printOptionsService = Components.classes["@mozilla.org/gfx/printoptions;1"]
                                             .getService(Components.interfaces.nsIPrintOptions);
    printOptionsService.ShowPrintSetupDialog(printSettings);
  } catch(e) {
    return false;
  }
  return true;
}

function goPreferences(containerID, paneURL, itemID)
{
  var resizable;
  var pref = Components.classes["@mozilla.org/preferences-service;1"]
                       .getService(Components.interfaces.nsIPrefBranch);
  try {
    // We are resizable ONLY if in box debugging mode, because in
    // this special debug mode it is often impossible to see the 
    // content of the debug panel in order to disable debug mode.
    resizable = pref.getBoolPref("xul.debug.box");
  }
  catch (e) {
    resizable = false;
  }

  //check for an existing pref window and focus it; it's not application modal
  const kWindowMediatorContractID = "@mozilla.org/rdf/datasource;1?name=window-mediator";
  const kWindowMediatorIID = Components.interfaces.nsIWindowMediator;
  const kWindowMediator = Components.classes[kWindowMediatorContractID].getService(kWindowMediatorIID);
  var lastPrefWindow = kWindowMediator.getMostRecentWindow("mozilla:preferences");
  if (lastPrefWindow)
    lastPrefWindow.focus();
  else {
    var resizability = resizable ? "yes" : "no";
    var features = "chrome,titlebar,resizable=" + resizability;
    openDialog("chrome://communicator/content/pref/pref.xul","PrefWindow", 
               features, paneURL, containerID, itemID);
  }
}

function goToggleToolbar( id, elementID )
{
  var toolbar = document.getElementById( id );
  var element = document.getElementById( elementID );
  if ( toolbar )
  {
    var attribValue = toolbar.getAttribute("hidden") ;

    if ( attribValue == "true" )
    {
      toolbar.setAttribute("hidden", "false" );
      if ( element )
        element.setAttribute("checked","true")
    }
    else
    {
      toolbar.setAttribute("hidden", true );
      if ( element )
        element.setAttribute("checked","false")
    }
    document.persist(id, 'hidden');
    document.persist(elementID, 'checked');
  }
}


function goClickThrobber( urlPref )
{
  var url;
  try {
    var pref = Components.classes["@mozilla.org/preferences-service;1"]
                         .getService(Components.interfaces.nsIPrefBranch);
    url = pref.getComplexValue(urlPref, Components.interfaces.nsIPrefLocalizedString).data;
  }

  catch(e) {
    url = null;
  }

  if ( url )
    openTopWin(url);
}


//No longer needed.  Rip this out since we are using openTopWin
function goHelpMenu( url )
{
  /* note that this chrome url should probably change to not have all of the navigator controls */
  /* also, do we want to limit the number of help windows that can be spawned? */
  window.openDialog( getBrowserURL(), "_blank", "chrome,all,dialog=no", url );
}


function openTopWin( url )
{
    /* note that this chrome url should probably change to not have
       all of the navigator controls, but if we do this we need to have
       the option for chrome controls because goClickThrobber() needs to
       use this function with chrome controls */
    /* also, do we want to
       limit the number of help windows that can be spawned? */
    if ((url == null) || (url == "")) return;

    // xlate the URL if necessary
    if (url.indexOf("urn:") == 0)
    {
        url = xlateURL(url);        // does RDF urn expansion
    }

    // avoid loading "", since this loads a directory listing
    if (url == "") {
        url = "about:blank";
    }

    var windowManager = Components.classes['@mozilla.org/rdf/datasource;1?name=window-mediator'].getService();
    var windowManagerInterface = windowManager.QueryInterface( Components.interfaces.nsIWindowMediator);

    var topWindowOfType = windowManagerInterface.getMostRecentWindow( "navigator:browser" );
    if ( topWindowOfType )
    {
        topWindowOfType.focus();
        topWindowOfType.loadURI(url);
        return topWindowOfType;
    }
    else
    {
        return window.openDialog( getBrowserURL(), "_blank", "chrome,all,dialog=no", url );
    }
}

function goAboutDialog()
{
  var defaultAboutState = false;
  try {
    var pref = Components.classes["@mozilla.org/preferences-service;1"]
                         .getService(Components.interfaces.nsIPrefBranch);
    defaultAboutState = pref.getBoolPref("browser.show_about_as_stupid_modal_window");
  }
  catch(e) {
    defaultAboutState = false;
  }
  if( defaultAboutState )
    window.openDialog("chrome://global/content/about.xul", "About", "modal,chrome,resizable=yes,height=450,width=550");
  else
    window.openDialog( getBrowserURL(), "_blank", "chrome,all,dialog=no", 'about:' );
}

// update menu items that rely on focus
function goUpdateGlobalEditMenuItems()
{
  goUpdateCommand('cmd_undo');
  goUpdateCommand('cmd_redo');
  goUpdateCommand('cmd_cut');
  goUpdateCommand('cmd_copy');
  goUpdateCommand('cmd_paste');
  goUpdateCommand('cmd_selectAll');
  goUpdateCommand('cmd_delete');
}

// update menu items that rely on the current selection
function goUpdateSelectEditMenuItems()
{
  goUpdateCommand('cmd_cut');
  goUpdateCommand('cmd_copy');
  goUpdateCommand('cmd_delete');
  goUpdateCommand('cmd_selectAll');
}

// update menu items that relate to undo/redo
function goUpdateUndoEditMenuItems()
{
  goUpdateCommand('cmd_undo');
  goUpdateCommand('cmd_redo');
}

// update menu items that depend on clipboard contents
function goUpdatePasteMenuItems()
{
  goUpdateCommand('cmd_paste');
}

// function that extracts the filename from a url
function extractFileNameFromUrl(urlstr)
{
  if (!urlstr) return null;

  // For "http://foo/bar/cheese.jpg", return "cheese.jpg".
  // For "imap://user@host.com:143/fetch>UID>/INBOX>951?part=1.2&type=image/gif&filename=foo.jpeg", return "foo.jpeg".
  // The 2nd url (ie, "imap://...") is generated for inline images by MimeInlineImage_parse_begin() in mimeiimg.cpp.
  var lastSlash = urlstr.slice(urlstr.lastIndexOf( "/" )+1);
  if (lastSlash)
  { 
    var nameIndex = lastSlash.lastIndexOf( "filename=" );
    if (nameIndex != -1)
      return (lastSlash.slice(nameIndex+9));
    else
      return lastSlash;
  }
  return null; 
}

// Gather all descendent text under given document node.
function gatherTextUnder ( root ) 
{
  var text = "";
  var node = root.firstChild;
  var depth = 1;
  while ( node && depth > 0 ) {
    // See if this node is text.
    if ( node.nodeName == "#text" ) {
      // Add this text to our collection.
      text += " " + node.data;
    } else if ( node.nodeType == Node.ELEMENT_NODE 
                && node.localName.toUpperCase() == "IMG" ) {
      // If it has an alt= attribute, use that.
      var altText = node.getAttribute( "alt" );
      if ( altText && altText != "" ) {
        text = altText;
        break;
      }
    }
    // Find next node to test.
    // First, see if this node has children.
    if ( node.hasChildNodes() ) {
      // Go to first child.
      node = node.firstChild;
      depth++;
    } else {
      // No children, try next sibling.
      if ( node.nextSibling ) {
        node = node.nextSibling;
      } else {
        // Last resort is our next oldest uncle/aunt.
        node = node.parentNode.nextSibling;
        depth--;
      }
    }
  }
  // Strip leading whitespace.
  text = text.replace( /^\s+/, "" );
  // Strip trailing whitespace.
  text = text.replace( /\s+$/, "" );
  // Compress remaining whitespace.
  text = text.replace( /\s+/g, " " );
  return text;
}

var offlineObserver = {
  observe: function(subject, topic, state) {
    // sanity checks
    if (topic != "network:offline-status-changed") return;
    setOfflineUI(state == "offline");
  }
}

function utilityOnLoad(aEvent)
{
  var broadcaster = document.getElementById("Communicator:WorkMode");
  if (!broadcaster) return;

  var observerService = Components.classes[kObserverServiceProgID]
		          .getService(Components.interfaces.nsIObserverService);

  // crude way to prevent registering twice.
  try {
    observerService.removeObserver(offlineObserver, "network:offline-status-changed");
  }
  catch (ex) {
  }
  observerService.addObserver(offlineObserver, "network:offline-status-changed", false);
  // make sure we remove this observer later
  addEventListener("unload",utilityOnUnload,false);

  // set the initial state
  var ioService = Components.classes[kIOServiceProgID]
		      .getService(Components.interfaces.nsIIOService);
  setOfflineUI(ioService.offline);
}

function utilityOnUnload(aEvent) 
{
  var observerService = Components.classes[kObserverServiceProgID]
			  .getService(Components.interfaces.nsIObserverService);
  observerService.removeObserver(offlineObserver, "network:offline-status-changed");
}

addEventListener("load",utilityOnLoad,true);
