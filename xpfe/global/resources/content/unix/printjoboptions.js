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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s): 
 *   Masaki Katakai <katakai@japan.sun.com>
 *   Roland Mainz <roland.mainz@informatik.med.uni-giessen.de>
 *   Asko Tontti <atontti@cc.hut.fi>
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

var dialog;
var gPrintSettings = null;
var gStringBundle  = null;
var gPrintSettingsInterface  = Components.interfaces.nsIPrintSettings;
var gPaperArray;
var gPrefs;

var default_command    = "lpr";
var gPrintSetInterface = Components.interfaces.nsIPrintSettings;
var doDebug            = true;

//---------------------------------------------------
function checkDouble(element, maxVal)
{
  var value = element.value;
  if (value && value.length > 0) {
    value = value.replace(/[^\.|^0-9]/g,"");
    if (!value) {
      element.value = "";
    } else {
      if (value > maxVal) {
        element.value = maxVal;
      } else {
        element.value = value;
      }
    }
  }
}

//---------------------------------------------------
function getDoubleStr(val, dec)
{
  var str = val.toString();
  var inx = str.indexOf(".");
  return str.substring(0, inx+dec+1);
}

//---------------------------------------------------
function initDialog()
{
  dialog = new Object;

  dialog.paperList       = document.getElementById("paperList");
  dialog.paperGroup      = document.getElementById("paperGroup");
  
  dialog.cmdLabel        = document.getElementById("cmdLabel");
  dialog.cmdInput        = document.getElementById("cmdInput");

  dialog.colorGroup      = document.getElementById("colorGroup");
  dialog.colorRadio      = document.getElementById("colorRadio");
  dialog.grayRadio       = document.getElementById("grayRadio");

  dialog.topInput        = document.getElementById("topInput");
  dialog.bottomInput     = document.getElementById("bottomInput");
  dialog.leftInput       = document.getElementById("leftInput");
  dialog.rightInput      = document.getElementById("rightInput");
}

//---------------------------------------------------
function round10(val)
{
  return Math.round(val * 10) / 10;
}


//---------------------------------------------------
function listElement(aListElement)
  {
    this.listElement = aListElement;
  }

listElement.prototype =
  {
    clearList:
      function ()
        {
          // remove the menupopup node child of the menulist.
          this.listElement.removeChild(this.listElement.firstChild);
        },

    appendPaperNames: 
      function (aDataObject) 
        { 
          var popupNode = document.createElement("menupopup"); 
          for (var i=0;i<aDataObject.length;i++)  {
            var paperObj = aDataObject[i];
            var itemNode = document.createElement("menuitem");
            var label;
            try {
              label = gStringBundle.GetStringFromName(paperObj.name)
            } 
            catch (e) {
              /* No name in string bundle ? Then build one manually (this
               * usually happens when gPaperArray was build by createPaperArrayFromPrinterFeatures() ...) */              
              if (paperObj.inches) {
                label = paperObj.name + " (" + round10(paperObj.width) + "x" + round10(paperObj.height) + " inch)";
              }  
              else {
                label = paperObj.name + " (" + paperObj.width + "x" + paperObj.height + " mm)";
              }
            }
            itemNode.setAttribute("label", label);
            itemNode.setAttribute("value", i);
            popupNode.appendChild(itemNode);            
          }
          this.listElement.appendChild(popupNode); 
        } 
  };

//---------------------------------------------------
function createPaperArrayFromDefaults()
{
  var paperNames   = ["letterSize", "legalSize", "exectiveSize", "a5Size", "a4Size", "a3Size", "a2Size", "a1Size", "a0Size"];
  //var paperNames   = ["&letterRadio.label;", "&legalRadio.label;", "&exectiveRadio.label;", "&a4Radio.label;", "&a3Radio.label;"];
  var paperWidths  = [ 8.5,  8.5,  7.25, 148.0, 210.0, 287.0, 420.0, 594.0,  841.0];
  var paperHeights = [11.0, 14.0, 10.50, 210.0, 297.0, 420.0, 594.0, 841.0, 1189.0];
  var paperInches  = [true, true, true,  false, false, false, false, false, false];
  // this is deprecated
  var paperEnums  = [0, 1, 2, 3, 4, 5, 6, 7, 8];

  gPaperArray = new Array();

  for (var i=0;i<paperNames.length;i++) {
    var obj    = new Object();
    obj.name   = paperNames[i];
    obj.width  = paperWidths[i];
    obj.height = paperHeights[i];
    obj.inches = paperInches[i];
    obj.paperSize = paperEnums[i]; // deprecated
    
    /* Calculate the width/height in millimeters */
    if (paperInches[i]) {
      obj.width_mm  = paperWidths[i]  * 25.4;
      obj.height_mm = paperHeights[i] * 25.4; 
    }
    else {
      obj.width_mm  = paperWidths[i];
      obj.height_mm = paperHeights[i];        
    }
    gPaperArray[i] = obj;
  }
}

//---------------------------------------------------
function createPaperArrayFromPrinterFeatures()
{
  var printername = gPrintSettings.printerName;
  if (doDebug) {
    dump("createPaperArrayFromPrinterFeatures for " + printername + ".\n");
  }

  gPaperArray = new Array();
  
  var numPapers = gPrefs.getIntPref("print.tmp.printerfeatures." + printername + ".paper.count");
  
  if (doDebug) {
    dump("processing " + numPapers + " entries...\n");
  }    

  for (var i=0;i<numPapers;i++) {
    var obj    = new Object();
    obj.name      = gPrefs.getCharPref("print.tmp.printerfeatures." + printername + ".paper." + i + ".name");
    obj.width_mm  = gPrefs.getIntPref("print.tmp.printerfeatures."  + printername + ".paper." + i + ".width_mm");
    obj.height_mm = gPrefs.getIntPref("print.tmp.printerfeatures."  + printername + ".paper." + i + ".height_mm");
    obj.inches    = gPrefs.getBoolPref("print.tmp.printerfeatures." + printername + ".paper." + i + ".is_inch");
    obj.paperSize = 666; // deprecated
    
    /* Calculate the width/height in paper's native units (either inches or millimeters) */
    if (obj.inches) {
      obj.width  = obj.width_mm  / 25.4;
      obj.height = obj.height_mm / 25.4; 
    }
    else {
      obj.width  = obj.width_mm;
      obj.height = obj.height_mm;
    }

    gPaperArray[i] = obj;

    if (doDebug) {
      dump("paper index=" + i + ", name=" + obj.name + ", width=" + obj.width + ", height=" + obj.height + ".\n");
    }
  }  
}

//---------------------------------------------------
function createPaperArray()
{
  var use_paper_array_from_printerfeatures = false;

  try {
    use_paper_array_from_printerfeatures = gPrefs.getBoolPref("print.tmp.printerfeatures." + gPrintSettings.printerName + ".has_special_printerfeatures");
  }
  catch (e) {
  }
  
  if (use_paper_array_from_printerfeatures) {
    createPaperArrayFromPrinterFeatures();    
  }
  else {
    createPaperArrayFromDefaults();
  }
}

//---------------------------------------------------
function createPaperSizeList(selectedInx)
{
  gStringBundle = srGetStrBundle("chrome://communicator/locale/printPageSetup.properties");

  var selectElement = new listElement(dialog.paperList);
  selectElement.clearList();

  selectElement.appendPaperNames(gPaperArray);

  if (selectedInx > -1) {
    selectElement.listElement.selectedIndex = selectedInx;
  }

  //dialog.paperList = selectElement;
}   

//---------------------------------------------------
function loadDialog()
{
  var print_paper_type       = 0;
  var print_paper_unit       = 0;
  var print_paper_width      = 0.0;
  var print_paper_height     = 0.0;
  var print_paper_name       = "";
  var print_color            = true;
  var print_command          = default_command;

  gPrefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);

  if (gPrintSettings) {
    print_paper_type   = gPrintSettings.paperSizeType;
    print_paper_unit   = gPrintSettings.paperSizeUnit;
    print_paper_width  = gPrintSettings.paperWidth;
    print_paper_height = gPrintSettings.paperHeight;
    print_paper_name   = gPrintSettings.paperName;
    print_color        = gPrintSettings.printInColor;
    print_command      = gPrintSettings.printCommand;
  }

  if (doDebug) {
    dump("loadDialog******************************\n");
    dump("paperSizeType "+print_paper_unit+"\n");
    dump("paperWidth    "+print_paper_width+"\n");
    dump("paperHeight   "+print_paper_height+"\n");
    dump("paperName     "+print_paper_name+"\n");
    dump("printInColor  "+print_color+"\n");
    dump("printCommand  "+print_command+"\n");
  }

  createPaperArray();

  var selectedInx = 0;
  for (var i=0;i<gPaperArray.length;i++) { 
    if (print_paper_name == gPaperArray[i].name) {
      selectedInx = i;
      break;
    }
  }
  
  if (doDebug) {
    if (i == gPaperArray.length)
      dump("loadDialog: No paper found.\n");
    else
      dump("loadDialog: found paper '"+gPaperArray[selectedInx].name+"'.\n");
  }

  createPaperSizeList(selectedInx);

  if (print_command == "") {
    print_command = default_command;
  }

  if (print_color) {
    dialog.colorGroup.selectedItem = dialog.colorRadio;
  } else {
    dialog.colorGroup.selectedItem = dialog.grayRadio;
  }

  dialog.cmdInput.value = print_command;

  /* First initalize with the hardcoded defaults... */
  dialog.topInput.value    = "0.04";
  dialog.bottomInput.value = "0.04";
  dialog.leftInput.value   = "0.04";
  dialog.rightInput.value  = "0.04";

  try {
    /* ... then try to get the generic settings ... */
    dialog.topInput.value    = gPrefs.getIntPref("print.print_edge_top") / 100.0;
    dialog.bottomInput.value = gPrefs.getIntPref("print.print_edge_bottom") / 100.0;
    dialog.leftInput.value   = gPrefs.getIntPref("print.print_edge_left") / 100.0;
    dialog.rightInput.value  = gPrefs.getIntPref("print.print_edge_right") / 100.0;

    /* ... and then the printer specific settings. */
    var printername = gPrintSettings.printerName;
    dialog.topInput.value    = gPrefs.getIntPref("print.printer_"+printername+".print_edge_top") / 100.0;
    dialog.bottomInput.value = gPrefs.getIntPref("print.printer_"+printername+".print_edge_bottom") / 100.0;
    dialog.leftInput.value   = gPrefs.getIntPref("print.printer_"+printername+".print_edge_left") / 100.0;
    dialog.rightInput.value  = gPrefs.getIntPref("print.printer_"+printername+".print_edge_right") / 100.0;
  } catch (e) {  }
}

//---------------------------------------------------
function onLoad()
{
  // Init dialog.
  initDialog();

  // window.arguments[0]: container for return value (1 = ok, 0 = cancel)
  var ps = window.arguments[0].QueryInterface(gPrintSetInterface);
  if (ps != null) {
    gPrintSettings = ps;
    paramBlock = window.arguments[1].QueryInterface(Components.interfaces.nsIDialogParamBlock);
  }

  if (doDebug) {
    if (ps == null) alert("PrintSettings is null!");
    if (paramBlock == null) alert("nsIDialogParam is null!");
  }

  // default return value is "cancel"
  paramBlock.SetInt(0, 0);

  loadDialog();
}

//---------------------------------------------------
function onAccept()
{
  var print_paper_type   = gPrintSettingsInterface.kPaperSizeDefined;
  var print_paper_unit   = gPrintSettingsInterface.kPaperSizeInches;
  var print_paper_width  = 0.0;
  var print_paper_height = 0.0;
  var print_paper_name   = "";

  if (gPrintSettings != null) {
    var selectedInx = dialog.paperList.selectedIndex;
    if (gPaperArray[selectedInx].inches) {
      print_paper_unit = gPrintSettingsInterface.kPaperSizeInches;
    } else {
      print_paper_unit = gPrintSettingsInterface.kPaperSizeMillimeters;
    }
    print_paper_width  = gPaperArray[selectedInx].width;
    print_paper_height = gPaperArray[selectedInx].height;
    print_paper_name   = gPaperArray[selectedInx].name;
    gPrintSettings.paperSize = gPaperArray[selectedInx].paperSize; // deprecated

    gPrintSettings.paperSizeType = print_paper_type;
    gPrintSettings.paperSizeUnit = print_paper_unit;
    gPrintSettings.paperWidth    = print_paper_width;
    gPrintSettings.paperHeight   = print_paper_height;
    gPrintSettings.paperName     = print_paper_name;

    // save these out so they can be picked up by the device spec
    gPrintSettings.printInColor = dialog.colorRadio.selected;
    gPrintSettings.printCommand = dialog.cmdInput.value;

    // 
    try {
      var printerName = gPrintSettings.printerName;
      var i = dialog.topInput.value * 100;
      gPrefs.setIntPref("print.printer_"+printerName+".print_edge_top", i);

      i = dialog.bottomInput.value * 100;
      gPrefs.setIntPref("print.printer_"+printerName+".print_edge_bottom", i);

      i = dialog.leftInput.value * 100;
      gPrefs.setIntPref("print.printer_"+printerName+".print_edge_left", i);

      i = dialog.rightInput.value * 100;
      gPrefs.setIntPref("print.printer_"+printerName+".print_edge_right", i);
    } catch (e) {
    }

    if (doDebug) {
      dump("onAccept******************************\n");
      dump("paperSize     "+gPrintSettings.paperSize+" (deprecated)\n");
      dump("paperSizeType "+print_paper_type+" (should be 1)\n");
      dump("paperSizeUnit "+print_paper_unit+"\n");
      dump("paperWidth    "+print_paper_width+"\n");
      dump("paperHeight   "+print_paper_height+"\n");
      dump("paperName     '"+print_paper_name+"'\n");

      dump("printInColor  "+gPrintSettings.printInColor+"\n");
      dump("printCommand  '"+gPrintSettings.printCommand+"'\n");
    }
  } else {
    dump("************ onAccept gPrintSettings: "+gPrintSettings+"\n");
  }

  if (paramBlock) {
    // set return value to "ok"
    paramBlock.SetInt(0, 1);
  } else {
    dump("*** FATAL ERROR: paramBlock missing\n");
  }

  return true;
}

