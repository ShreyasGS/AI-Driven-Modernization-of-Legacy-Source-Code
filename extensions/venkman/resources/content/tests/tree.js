/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/ 
 * 
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License. 
 *
 * The Original Code is The JavaScript Debugger
 * 
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation
 * Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the MPL or the GPL.
 *
 * Contributor(s):
 *  Robert Ginda, <rginda@netscape.com>, original author
 *
 */

var sampleShare = new Object();
var sampleTree = new TreeOView(sampleShare);

function SampleRecord (name, gender) 
{
    this.setColumnPropertyName ("sample-name", "name");
    this.setColumnPropertyName ("sample-gender", "gender");    
    this.name = name;
    this.gender = gender;
}

SampleRecord.prototype = new TreeOViewRecord(sampleShare);

sampleTree.childData.appendChild (new SampleRecord ("vinnie", "male"));
var betty = new SampleRecord ("betty", "female");
betty.reserveChildren();
betty.open()
betty.appendChild (new SampleRecord ("kid1", "male"));
var kid2 = new SampleRecord ("kid2", "male");
kid2.reserveChildren();
var kid22 = new SampleRecord ("kid22", "female");
kid2.appendChild (kid22);
betty.appendChild (kid2);
sampleTree.childData.appendChild (betty);
sampleTree.childData.appendChild (new SampleRecord ("joey", "male"));

function onload ()
{

    var tree = document.getElementById("sample-tree");
    tree.treeBoxObject.view = sampleTree;
    dt();
    //    debugger;
}

function toggleBetty ()
{
    if (betty.isHidden)
        betty.unHide();
    else
        betty.hide();
}

function formatRecord (rec, indent)
{
    var str = "";
    
    for (var i in rec._colValues)
        str += rec._colValues[i] + ", ";
    
    str += "[";
    
    str += rec.calculateVisualRow() + ", ";
    str += rec.childIndex + ", ";
    str += rec.level + ", ";
    str += rec.visualFootprint + ", ";
    str += rec.isHidden + "]";
    
    dd (indent + str);
}

function formatBranch (rec, indent)
{
    for (var i = 0; i < rec.childData.length; ++i)
    {
        formatRecord (rec.childData[i], indent);
        if ("childData" in rec.childData[i])
            formatBranch(rec.childData[i], indent + "  ");
    }
}

function nativeFrameTest()
{
    function compare(a, b)
    {
        debugger;
        if (a > b)
            return 1;

        if (a < b)
            return -1;

        return 0;
    };
    
    var ary = [2, 1];
    ary.sort(compare);
}

function dbg()
{
    var a = 0;
    dbg2();
    var c = 0;
}

function dbg2()
{
    dd("dbg2");
    var nothere;
    var nope = null;
    var f = false;
    var i = 4;
    var d = Number.MAX_VALUE;
    var s = "hello world";
    var fun = dbg;
    var obj = new Object();
    debugger;
    guessThis();
}

function dt()
{
    formatBranch(sampleTree.childData, "");
}

var guessThis = 
function () 
{
    var x = 1;
}

var observer = {
    onFoo: function (){}
}

function switchTest ()
{
    var x = 1;
    
    switch (x)
    {
        case 1:
            ++x;
            break;
            
        case 2:
            --x;
            break;
            
        default:
            x += 3;
            break;
    }
}

