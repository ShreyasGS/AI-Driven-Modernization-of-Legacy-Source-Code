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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#ifndef nsIPrivateCompositionEvent_h__
#define nsIPrivateCompositionEvent_h__

#include "nsEvent.h"
#include "nsISupports.h"

// {ECF6BEF1-5F0C-11d3-9EB3-0060089FE59B}
#define NS_IPRIVATECOMPOSITIONEVENT_IID	\
{ 0xecf6bef1, 0x5f0c, 0x11d3, \
{ 0x9e, 0xb3, 0x0, 0x60, 0x8, 0x9f, 0xe5, 0x9b }}

class nsIPrivateCompositionEvent : public nsISupports {

public:
	static const nsIID& GetIID() { static nsIID iid = NS_IPRIVATECOMPOSITIONEVENT_IID; return iid; }

	NS_IMETHOD GetCompositionReply(struct nsTextEventReply** aReply) = 0;
    NS_IMETHOD GetReconversionReply(nsReconversionEventReply** aReply) = 0;
};

#endif // nsIPrivateCompositionEvent_h__

