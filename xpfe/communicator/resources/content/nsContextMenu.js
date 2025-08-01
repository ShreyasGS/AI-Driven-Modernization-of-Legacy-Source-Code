/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
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
 * The Original Code is Mozilla Communicator client code,
 * released March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *     William A. ("PowerGUI") Law <law@netscape.com>
 *     Blake Ross <blakeross@telocity.com>
 *     Gervase Markham <gerv@gerv.net>
 */

/*------------------------------ nsContextMenu ---------------------------------
|   This JavaScript "class" is used to implement the browser's content-area    |
|   context menu.                                                              |
|                                                                              |
|   For usage, see references to this class in navigator.xul.                  |
|                                                                              |
|   Currently, this code is relatively useless for any other purpose.  In the  |
|   longer term, this code will be restructured to make it more reusable.      |
------------------------------------------------------------------------------*/
function nsContextMenu( xulMenu ) {
    this.target         = null;
    this.menu           = null;
    this.onTextInput    = false;
    this.onImage        = false;
    this.onLink         = false;
    this.onMailtoLink   = false;
    this.onSaveableLink = false;
    this.link           = false;
    this.inFrame        = false;
    this.hasBGImage     = false;
    this.isTextSelected = false;
    this.inDirList      = false;
    this.shouldDisplay  = true;

    // Initialize new menu.
    this.initMenu( xulMenu );
}

// Prototype for nsContextMenu "class."
nsContextMenu.prototype = {
    // onDestroy is a no-op at this point.
    onDestroy : function () {
    },
    // Initialize context menu.
    initMenu : function ( popup ) {
        // Save menu.
        this.menu = popup;

        // Get contextual info.
        this.setTarget( document.popupNode );
        
        this.isTextSelected = this.isTextSelection();

        // Initialize (disable/remove) menu items.
        this.initItems();
    },
    initItems : function () {
        this.initOpenItems();
        this.initNavigationItems();
        this.initViewItems();
        this.initMiscItems();
        this.initSaveItems();
        this.initClipboardItems();
        this.initMetadataItems();
    },
    initOpenItems : function () {
        this.showItem( "context-openlink", this.onSaveableLink || ( this.inDirList && this.onLink ) );
        this.showItem( "context-openlinkintab", this.onSaveableLink || ( this.inDirList && this.onLink ) );

        this.showItem( "context-sep-open", this.onSaveableLink || ( this.inDirList && this.onLink ) );
    },
    initNavigationItems : function () {
        // Back determined by canGoBack broadcaster.
        this.setItemAttrFromNode( "context-back", "disabled", "canGoBack" );

        // Forward determined by canGoForward broadcaster.
        this.setItemAttrFromNode( "context-forward", "disabled", "canGoForward" );
        
        this.showItem( "context-back", !( this.isTextSelected || this.onLink || this.onImage || this.onTextInput ) );
        this.showItem( "context-forward", !( this.isTextSelected || this.onLink || this.onImage || this.onTextInput ) );

        this.showItem( "context-reload", !( this.isTextSelected || this.onLink || this.onImage || this.onTextInput ) );
        
        this.showItem( "context-stop", !( this.isTextSelected || this.onLink || this.onImage || this.onTextInput ) );
        this.showItem( "context-sep-stop", !( this.isTextSelected || this.onLink || this.onImage || this.onTextInput ) );

        // XXX: Stop is determined in navigator.js; the canStop broadcaster is broken
        //this.setItemAttrFromNode( "context-stop", "disabled", "canStop" );
    },
    initSaveItems : function () {
        this.showItem( "context-savepage", !( this.inDirList || this.isTextSelected || this.onTextInput ) && !( this.onLink && this.onImage ) );

        // Save link depends on whether we're in a link.
        this.showItem( "context-savelink", this.onSaveableLink );

        // Save image depends on whether there is one.
        this.showItem( "context-saveimage", this.onImage );
        
        this.showItem( "context-sendimage", this.onImage );
    },
    initViewItems : function () {
        // View source is always OK, unless in directory listing.
        this.showItem( "context-viewsource", !( this.inDirList || this.onImage || this.isTextSelected || this.onLink || this.onTextInput ) );
        this.showItem( "context-viewinfo", !( this.inDirList || this.onImage || this.isTextSelected || this.onLink || this.onTextInput ) );

        this.showItem( "context-sep-properties", !( this.inDirList || this.isTextSelected || this.onTextInput ) );
        // Set As Wallpaper depends on whether an image was clicked on, and only works on Windows.
        this.showItem( "context-setWallpaper", this.onImage && navigator.appVersion.indexOf("Windows") != -1);
        
        this.showItem( "context-sep-setWallpaper", this.onImage && navigator.appVersion.indexOf("Windows") != -1);

        // View Image depends on whether an image was clicked on.
        this.showItem( "context-viewimage", this.onImage );

        // View background image depends on whether there is one.
        this.showItem( "context-viewbgimage", !( this.inDirList || this.onImage || this.isTextSelected || this.onLink || this.onTextInput ) );
        this.showItem( "context-sep-viewbgimage", !( this.inDirList || this.onImage || this.isTextSelected || this.onLink || this.onTextInput ) );
        var menuitem = document.getElementById("context-viewbgimage");

        if (this.hasBGImage)
          menuitem.removeAttribute("disabled");
        else
          menuitem.setAttribute("disabled", "true");
    },
    initMiscItems : function () {
        // Use "Bookmark This Link" if on a link.
        this.showItem( "context-bookmarkpage", !( this.isTextSelected || this.onTextInput ) );
        this.showItem( "context-bookmarklink", this.onLink && !this.onMailtoLink );
        this.showItem( "context-searchselect", this.isTextSelected );
        this.showItem( "frame", this.inFrame );
        this.showItem( "frame-sep", this.inFrame );
    },
    initClipboardItems : function () {

        // Copy depends on whether there is selected text.
        // Enabling this context menu item is now done through the global
        // command updating system
        // this.setItemAttr( "context-copy", "disabled", !this.isTextSelected() );

        goUpdateGlobalEditMenuItems();

        this.showItem( "context-undo", this.isTextSelected || this.onTextInput );
        this.showItem( "context-sep-undo", this.isTextSelected || this.onTextInput );
        this.showItem( "context-cut", this.isTextSelected || this.onTextInput );
        this.showItem( "context-copy", this.isTextSelected || this.onTextInput );
        this.showItem( "context-paste", this.isTextSelected || this.onTextInput );
        this.showItem( "context-delete", this.isTextSelected || this.onTextInput );
        this.showItem( "context-sep-paste", this.isTextSelected || this.onTextInput );
        this.showItem( "context-selectall", this.isTextSelected || this.onTextInput );
        this.showItem( "context-sep-selectall", this.isTextSelected );

        // XXX dr
        // ------
        // nsDocumentViewer.cpp has code to determine whether we're
        // on a link or an image. we really ought to be using that...

        // Copy email link depends on whether we're on an email link.
        this.showItem( "context-copyemail", this.onMailtoLink );

        // Copy link location depends on whether we're on a link.
        this.showItem( "context-copylink", this.onLink );
        this.showItem( "context-sep-copylink", this.onLink );

        // Copy image location depends on whether we're on an image.
        this.showItem( "context-copyimage", this.onImage );
        this.showItem( "context-sep-copyimage", this.onImage );
    },
    initMetadataItems : function () {
        // Show if user clicked on something which has metadata.
        this.showItem( "context-metadata", this.onMetaDataItem );
    },
    // Set various context menu attributes based on the state of the world.
    setTarget : function ( node ) {
        const xulNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
        if ( node.namespaceURI == xulNS ) {
          this.shouldDisplay = false;
          return;
        }
        // Initialize contextual info.
        this.onImage    = false;
        this.onMetaDataItem = false;
        this.onTextInput = false;
        this.imageURL   = "";
        this.onLink     = false;
        this.inFrame    = false;
        this.hasBGImage = false;
        this.bgImageURL = "";

        // Remember the node that was clicked.
        this.target = node;
        // See if the user clicked on an image.
        if ( this.target.nodeType == Node.ELEMENT_NODE ) {
             if ( this.target.localName.toUpperCase() == "IMG" ) {
                this.onImage = true;
                this.imageURL = this.target.src;
                // Look for image map.
                var mapName = this.target.getAttribute( "usemap" );
                if ( mapName ) {
                    // Find map.
                    var map = this.target.ownerDocument.getElementById( mapName.substr(1) );
                    if ( map ) {
                        // Search child <area>s for a match.
                        var areas = map.childNodes;
                        //XXX Client side image maps are too hard for now!
                        areas.length = 0;
                        for ( var i = 0; i < areas.length && !this.onLink; i++ ) {
                            var area = areas[i];
                            if ( area.nodeType == Node.ELEMENT_NODE
                                 &&
                                 area.localName.toUpperCase() == "AREA" ) {
                                // Get type (rect/circle/polygon/default).
                                var type = area.getAttribute( "type" );
                                var coords = this.parseCoords( area );
                                switch ( type.toUpperCase() ) {
                                    case "RECT":
                                    case "RECTANGLE":
                                        break;
                                    case "CIRC":
                                    case "CIRCLE":
                                        break;
                                    case "POLY":
                                    case "POLYGON":
                                        break;
                                    case "DEFAULT":
                                        // Default matches entire image.
                                        this.onLink = true;
                                        this.link = area;
                                        this.onSaveableLink = this.isLinkSaveable( this.link );
                                        break;
                                }
                            }
                        }
                    }
                }
             } else if ( this.target.localName.toUpperCase() == "OBJECT"
                         &&
                         // See if object tag is for an image.
                         this.objectIsImage( this.target ) ) {
                // This is an image.
                this.onImage = true;
                // URL must be constructed.
                this.imageURL = this.objectImageURL( this.target );
             } else if ( this.target.localName.toUpperCase() == "INPUT") {
               type = this.target.getAttribute("type");
               if(type && type.toUpperCase() == "IMAGE") {
                 this.onImage = true;
                 // Convert src attribute to absolute URL.
                 this.imageURL = this.makeURLAbsolute( this.target.baseURI,
                                                       this.target.src );
               } else /* if (this.target.getAttribute( "type" ).toUpperCase() == "TEXT") */ {
                 this.onTextInput = this.isTargetATextBox(this.target);
               }
            } else if ( this.target.localName.toUpperCase() == "TEXTAREA" ) {
                 this.onTextInput = true;
            } else if ( this.target.localName.toUpperCase() == "HTML" ) {
               // pages with multiple <body>s are lame. we'll teach them a lesson.
               var bodyElt = this.target.ownerDocument.getElementsByTagName("body")[0];
               if ( bodyElt ) {
                 var attr = bodyElt.getAttribute( "background" );
                 if ( attr ||
                      ( attr = this.getComputedURL( bodyElt, "background-image" ) ) ) {
                   this.hasBGImage = true;
                   this.bgImageURL = this.makeURLAbsolute( bodyElt.baseURI,
                                                           attr );
                 }
               }
            } else if ( "HTTPIndex" in _content &&
                        _content.HTTPIndex instanceof Components.interfaces.nsIHTTPIndex ) {
                this.inDirList = true;
                // Bubble outward till we get to an element with URL attribute
                // (which should be the href).
                var root = this.target;
                while ( root && !this.link ) {
                    if ( root.tagName == "tree" ) {
                        // Hit root of tree; must have clicked in empty space;
                        // thus, no link.
                        break;
                    }
                    if ( root.getAttribute( "URL" ) ) {
                        // Build pseudo link object so link-related functions work.
                        this.onLink = true;
                        this.link = { href : root.getAttribute("URL"),
                                      getAttribute: function (attr) {
                                          if (attr == "title") {
                                              return root.firstChild.firstChild.getAttribute("label");
                                          } else {
                                              return "";
                                          }
                                      }
                                    };
                        // If element is a directory, then you can't save it.
                        if ( root.getAttribute( "container" ) == "true" ) {
                            this.onSaveableLink = false;
                        } else {
                            this.onSaveableLink = true;
                        }
                    } else {
                        root = root.parentNode;
                    }
                }
            }
        }

        // We have meta data on images.
        this.onMetaDataItem = this.onImage;
        
        // See if the user clicked in a frame.
        if ( this.target.ownerDocument != window._content.document ) {
            this.inFrame = true;
        }
        
        // Bubble out, looking for items of interest
        var elem = this.target;
        while ( elem ) {
            if ( elem.nodeType == Node.ELEMENT_NODE ) {
                var localname = elem.localName.toUpperCase();
                
                // Link?
                if ( !this.onLink && 
                    ( (localname === "A" && elem.href) ||
                      localname === "AREA" ||
                      localname === "LINK" ||
                      elem.getAttributeNS( "http://www.w3.org/1999/xlink", "type") == "simple" ) ) {
                    // Clicked on a link.
                    this.onLink = true;
                    this.onMetaDataItem = true;
                    // Remember corresponding element.
                    this.link = elem;
                    this.onMailtoLink = this.isLinkType( "mailto:", this.link );
                    // Remember if it is saveable.
                    this.onSaveableLink = this.isLinkSaveable( this.link );
                }
                
                // Text input?
                if ( !this.onTextInput ) {
                    // Clicked on a link.
                    this.onTextInput = this.isTargetATextBox(elem);
                }
                
                // Metadata item?
                if ( !this.onMetaDataItem ) {
                    // We currently display metadata on anything which fits
                    // the below test.
                    if ( ( localname === "BLOCKQUOTE" && 'cite' in elem && elem.cite)  ||
                         ( localname === "Q" && 'cite' in elem && elem.cite)           ||
                         ( localname === "TABLE" && 'summary' in elem && elem.summary) ||
                         ( ( localname === "INS" || localname === "DEL" ) &&
                           ( ( 'cite' in elem && elem.cite ) ||
                             ( 'dateTime' in elem && elem.dateTime ) ) )               ||
                         ( 'title' in elem && elem.title )                             ||
                         ( 'lang' in elem && elem.lang ) ) {
                        dump("On metadata item.\n");
                        this.onMetaDataItem = true;
                    }
                }

                // Background image?  We don't bother if we've already found a background
                // image further down the hierarchy.  Otherwise, we look for background=
                // attribute on html elements that support that, or, background-image style.
                var bgImgUrl = null;
                if ( !this.hasBGImage &&
                     ( ( localname.search( /^(?:TD|TH|TABLE|BODY)$/ ) != -1 &&
                         ( bgImgUrl = elem.getAttribute( "background" ) ) ) ||
                       ( bgImgUrl = this.getComputedURL( elem, "background-image" ) ) ) ) {
                    this.hasBGImage = true;
                    this.bgImageURL = this.makeURLAbsolute( elem.baseURI,
                                                            bgImgUrl );
                }
            }
            elem = elem.parentNode;    
        }
    },
    // Returns the computed style attribute for the given element.
    getComputedStyle: function( elem, prop ) {
         return elem.ownerDocument.defaultView.getComputedStyle( elem, '' ).getPropertyValue( prop );
    },
    // Returns a "url"-type computed style attribute value, with the url() stripped.
    getComputedURL: function( elem, prop ) {
         var url = elem.ownerDocument.defaultView.getComputedStyle( elem, '' ).getPropertyCSSValue( prop );
         return ( url.primitiveType == CSSPrimitiveValue.CSS_URI ) ? url.getStringValue() : null;
    },
    // Returns true iff clicked on link is saveable.
    isLinkSaveable : function ( link ) {
        // We don't do the Right Thing for news/snews yet, so turn them off
        // until we do.
        return !(this.isLinkType( "mailto:" , link )     ||
                 this.isLinkType( "javascript:" , link ) ||
                 this.isLinkType( "news:", link )        || 
                 this.isLinkType( "snews:", link ) ); 
    },
    // Returns true iff clicked on link is of type given.
    isLinkType : function ( linktype, link ) {        
        try {
            // Test for missing protocol property.
            if ( !link.protocol ) {
                // We must resort to testing the URL string :-(.
                var protocol;
                if ( link.href ) {
                    protocol = link.href.substr( 0, linktype.length );
                } else {
                    protocol = link.getAttributeNS("http://www.w3.org/1999/xlink","href");
                    if ( protocol ) {
                        protocol = protocol.substr( 0, linktype.length );
                    }
                }
                return protocol.toLowerCase() === linktype;        
            } else {
                // Presume all but javascript: urls are saveable.
                return link.protocol.toLowerCase() === linktype;
            }
        } catch (e) {
            // something was wrong with the link,
            // so we won't be able to save it anyway
            return false;
        }
    },
    // Open linked-to URL in a new window.
    openLink : function () {
        // Determine linked-to URL.
        openNewWindowWith( this.linkURL() );
    },
    // Open linked-to URL in a new tab.
    openLinkInTab : function () {
        // Determine linked-to URL.
        openNewTabWith( this.linkURL() );
    },
    // Open frame in a new tab.
    openFrameInTab : function () {
        // Determine linked-to URL.
        openNewTabWith( this.target.ownerDocument.location.href );
    },
    // Reload clicked-in frame.
    reloadFrame : function () {
        this.target.ownerDocument.location.reload();
    },
    // Open clicked-in frame in its own window.
    openFrame : function () {
        openNewWindowWith( this.target.ownerDocument.location.href );
    },
    // Open clicked-in frame in the same window
    showOnlyThisFrame : function () {
        window.loadURI(this.target.ownerDocument.location.href);
    },
    // Open new "view source" window with the frame's URL.
    viewFrameSource : function () {
        BrowserViewSourceOfDocument(this.target.ownerDocument);
    },
    viewInfo : function () {
      BrowserPageInfo();
    },
    viewFrameInfo : function () {
      BrowserPageInfo(this.target.ownerDocument);
    },
    // Change current window to the URL of the image.
    viewImage : function () {
        openTopWin( this.imageURL );
    },
    // Change current window to the URL of the background image.
    viewBGImage : function () {
        openTopWin( this.bgImageURL );
    },
    setWallpaper: function() {
      var winhooks = Components.classes[ "@mozilla.org/winhooks;1" ].
                       getService(Components.interfaces.nsIWindowsHooks);
      
      winhooks.setImageAsWallpaper(this.target, false);
    },    
    // Save URL of clicked-on frame.
    saveFrame : function () {
        saveDocument( this.target.ownerDocument );
    },
    // Save URL of clicked-on link.
    saveLink : function () {
        saveURL( this.linkURL(), this.linkText(), null, true );
    },
    // Save URL of clicked-on image.
    saveImage : function () {
        saveURL( this.imageURL, null, "SaveImageTitle", false );
    },
    // Generate email address and put it on clipboard.
    copyEmail : function () {
        // Copy the comma-separated list of email addresses only.
        // There are other ways of embedding email addresses in a mailto:
        // link, but such complex parsing is beyond us.
        var url = this.linkURL();
        var qmark = url.indexOf( "?" );
        var addresses;
        
        if ( qmark > 7 ) {                   // 7 == length of "mailto:"
            addresses = url.substring( 7, qmark );
        } else {
            addresses = url.substr( 7 );
        }

        var clipboard = this.getService( "@mozilla.org/widget/clipboardhelper;1",
                                         Components.interfaces.nsIClipboardHelper );
        clipboard.copyString(addresses);
    },    
    addBookmark : function() {
      var docshell = document.getElementById( "content" ).webNavigation;
      BookmarksUtils.addBookmark( docshell.currentURI.spec,
                                  docshell.document.title,
                                  docshell.document.charset,
                                  false );
    },
    addBookmarkForFrame : function() {
      var doc = this.target.ownerDocument;
      var uri = doc.location.href;
      var title = doc.title;
      if ( !title )
        title = uri;
      BookmarksUtils.addBookmark( uri,
                                  title,
                                  doc.charset,
                                  false );
    },
    // Open Metadata window for node
    showMetadata : function () {
        window.openDialog(  "chrome://navigator/content/metadata.xul",
                            "_blank",
                            "scrollbars,resizable,chrome,dialog=no",
                            this.target);
    },

    ///////////////
    // Utilities //
    ///////////////

    // Create instance of component given contractId and iid (as string).
    createInstance : function ( contractId, iidName ) {
        var iid = Components.interfaces[ iidName ];
        return Components.classes[ contractId ].createInstance( iid );
    },
    // Get service given contractId and iid (as string).
    getService : function ( contractId, iidName ) {
        var iid = Components.interfaces[ iidName ];
        return Components.classes[ contractId ].getService( iid );
    },
    // Show/hide one item (specified via name or the item element itself).
    showItem : function ( itemOrId, show ) {
        var item = itemOrId.constructor == String ? document.getElementById(itemOrId) : itemOrId;
        if (item) 
          item.hidden = !show;
    },
    // Set given attribute of specified context-menu item.  If the
    // value is null, then it removes the attribute (which works
    // nicely for the disabled attribute).
    setItemAttr : function ( id, attr, val ) {
        var elem = document.getElementById( id );
        if ( elem ) {
            if ( val == null ) {
                // null indicates attr should be removed.
                elem.removeAttribute( attr );
            } else {
                // Set attr=val.
                elem.setAttribute( attr, val );
            }
        }
    },
    // Set context menu attribute according to like attribute of another node
    // (such as a broadcaster).
    setItemAttrFromNode : function ( item_id, attr, other_id ) {
        var elem = document.getElementById( other_id );
        if ( elem && elem.getAttribute( attr ) == "true" ) {
            this.setItemAttr( item_id, attr, "true" );
        } else {
            this.setItemAttr( item_id, attr, null );
        }
    },
    // Temporary workaround for DOM api not yet implemented by XUL nodes.
    cloneNode : function ( item ) {
        // Create another element like the one we're cloning.
        var node = document.createElement( item.tagName );

        // Copy attributes from argument item to the new one.
        var attrs = item.attributes;
        for ( var i = 0; i < attrs.length; i++ ) {
            var attr = attrs.item( i );
            node.setAttribute( attr.nodeName, attr.nodeValue );
        }

        // Voila!
        return node;
    },
    // Generate fully-qualified URL for clicked-on link.
    linkURL : function () {
        if (this.link.href) {
          return this.link.href;
        }
        var href = this.link.getAttributeNS("http://www.w3.org/1999/xlink","href");
        if (!href || !href.match(/\S/)) {
          throw "Empty href"; // Without this we try to save as the current doc, for example, HTML case also throws if empty
        }
        href = this.makeURLAbsolute(this.link.baseURI,href);
        return href;
    },
    // Get text of link.
    linkText : function () {
        var text = gatherTextUnder( this.link );
        if (!text || !text.match(/\S/)) {
          text = this.link.getAttribute("title");
          if (!text || !text.match(/\S/)) {
            text = this.link.getAttribute("alt");
            if (!text || !text.match(/\S/)) {
              if (this.link.href) {                
                text = this.link.href;
              } else {
                text = getAttributeNS("http://www.w3.org/1999/xlink", "href");
                if (text && text.match(/\S/)) {
                  text = this.makeURLAbsolute(this.link.baseURI, text);
                }
              }
            }
          }
        }

        return text;
    },

    //Get selected object and convert it to a string to get
    //selected text.   Only use the first 15 chars.
    isTextSelection : function() {
        var result = false;
        var selection = this.searchSelected();
        var searchSelect = document.getElementById('context-searchselect');

        var bundle = srGetStrBundle("chrome://communicator/locale/contentAreaCommands.properties");

        var searchSelectText;
        if (selection != "") {
            searchSelectText = selection.toString();
            if (searchSelectText.length > 15)
                searchSelectText = searchSelectText.substr(0,15) + "...";
            result = true;

          // format "Search for <selection>" string to show in menu
          searchSelectText = bundle.formatStringFromName("searchText",
                                                         [searchSelectText], 1);
          searchSelect.setAttribute("label", searchSelectText);
        } 
        return result;
    },
    
    searchSelected : function() {
        var focusedWindow = document.commandDispatcher.focusedWindow;
        var searchStr = focusedWindow.getSelection();
        searchStr = searchStr.toString();
        searchStr = searchStr.replace( /^\s+/, "" );
        searchStr = searchStr.replace(/(\n|\r|\t)+/g, " ");
        searchStr = searchStr.replace(/\s+$/,"");
        return searchStr;
    },
    
    // Determine if target <object> is an image.
    objectIsImage : function ( objElem ) {
        var result = false;
        // Get type and data attributes.
        var type = objElem.getAttribute( "type" );
        var data = objElem.getAttribute( "data" );
        // Presume any mime type of the form "image/..." is an image.
        // There must be a data= attribute with an URL, also.
        if ( type.substring( 0, 6 ) == "image/" && data && data != "" ) {
            result = true;
        }
        return result;
    },
    // Extract image URL from <object> tag.
    objectImageURL : function ( objElem ) {
        // Extract url from data= attribute.
        var data = objElem.getAttribute( "data" );
        // Make it absolute.
        return this.makeURLAbsolute( objElem.baseURI, data );
    },
    // Convert relative URL to absolute, using document's <base>.
    makeURLAbsolute : function ( base, url ) {
        // Construct nsIURL.
        var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                      .getService(Components.interfaces.nsIIOService);
        var baseURI  = ioService.newURI(base, null, null);
        
        return ioService.newURI(baseURI.resolve(url), null, null).spec;
    },
    // Parse coords= attribute and return array.
    parseCoords : function ( area ) {
        return [];
    },
    toString : function () {
        return "contextMenu.target     = " + this.target + "\n" +
               "contextMenu.onImage    = " + this.onImage + "\n" +
               "contextMenu.onLink     = " + this.onLink + "\n" +
               "contextMenu.link       = " + this.link + "\n" +
               "contextMenu.inFrame    = " + this.inFrame + "\n" +
               "contextMenu.hasBGImage = " + this.hasBGImage + "\n";
    },
    isTargetATextBox : function ( node )
    {
      if (node.nodeType != Node.ELEMENT_NODE)
        return false;

      if (node.localName.toUpperCase() == "INPUT") {
        var attrib = "";
        var type = node.getAttribute("type");

        if (type)
          attrib = type.toUpperCase();

        return( (attrib != "IMAGE") &&
                (attrib != "CHECKBOX") &&
                (attrib != "RADIO") &&
                (attrib != "SUBMIT") &&
                (attrib != "RESET") &&
                (attrib != "FILE") &&
                (attrib != "HIDDEN") &&
                (attrib != "RESET") &&
                (attrib != "BUTTON") );
      } else  {
        return(node.localName.toUpperCase() == "TEXTAREA");
      }
    },
    
    // Determines whether or not the separator with the specified ID should be 
    // shown or not by determining if there are any non-hidden items between it
    // and the previous separator. 
    shouldShowSeparator : function ( aSeparatorID )
    {
      var separator = document.getElementById(aSeparatorID);
      if (separator) {
        var sibling = separator.previousSibling;
        while (sibling && sibling.localName != "menuseparator") {
          if (sibling.getAttribute("hidden") != "true")
            return true;
          sibling = sibling.previousSibling;
        }
      }
      return false;  
    }
};

/*************************************************************************
 *
 *   nsDefaultEngine : nsIObserver
 *
 *************************************************************************/
function nsDefaultEngine()
{
    try
    {
        var pb = Components.classes["@mozilla.org/preferences-service;1"].
                   getService(Components.interfaces.nsIPrefBranch);
        var pbi = pb.QueryInterface(
                    Components.interfaces.nsIPrefBranchInternal);
        pbi.addObserver(this.domain, this, false);

        // reuse code by explicitly invoking initial |observe| call
        // to initialize the |icon| and |name| member variables
        this.observe(pb, "", this.domain);
    }
    catch (ex)
    {
    }
}

nsDefaultEngine.prototype = 
{
    name: "",
    icon: "",
    domain: "browser.search.defaultengine",

    // nsIObserver implementation
    observe: function(aPrefBranch, aTopic, aPrefName)
    {
        try
        {
            var rdf = Components.
                        classes["@mozilla.org/rdf/rdf-service;1"].
                        getService(Components.interfaces.nsIRDFService);
            var ds = rdf.GetDataSource("rdf:internetsearch");
            var defaultEngine = aPrefBranch.getCharPref(aPrefName);
            var res = rdf.GetResource(defaultEngine);

            // get engine ``pretty'' name
            const kNC_Name = rdf.GetResource(
                               "http://home.netscape.com/NC-rdf#Name");
            var engineName = ds.GetTarget(res, kNC_Name, true);
            if (engineName)
            {
                this.name = engineName.QueryInterface(
                              Components.interfaces.nsIRDFLiteral).Value;
            }

            // get URL to engine vendor icon
            const kNC_Icon = rdf.GetResource(
                               "http://home.netscape.com/NC-rdf#Icon");
            var iconURL = ds.GetTarget(res, kNC_Icon, true);
            if (iconURL)
            {
                this.icon = iconURL.QueryInterface(
                  Components.interfaces.nsIRDFLiteral).Value;
            }
        }
        catch (ex)
        {
        }
    }
}
