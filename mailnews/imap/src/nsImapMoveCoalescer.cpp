/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * Portions created by the Initial Developer are Copyright (C) 1999
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#include "msgCore.h"
#include "nsMsgImapCID.h"
#include "nsImapMailFolder.h"
#include "nsImapMoveCoalescer.h"
#include "nsMsgKeyArray.h"
#include "nsImapService.h"
#include "nsIMsgFolder.h" // TO include biffState enum. Change to bool later...

static NS_DEFINE_CID(kCImapService, NS_IMAPSERVICE_CID);

nsImapMoveCoalescer::nsImapMoveCoalescer(nsIMsgFolder *sourceFolder, nsIMsgWindow *msgWindow)
{
  m_sourceFolder = sourceFolder; 
  m_msgWindow = msgWindow;
}

nsImapMoveCoalescer::~nsImapMoveCoalescer()
{
  for (PRInt32 i = 0; i < m_sourceKeyArrays.Count(); i++)
  {
    nsMsgKeyArray *keys = (nsMsgKeyArray *) m_sourceKeyArrays.ElementAt(i);
    delete keys;
  }
}

nsresult nsImapMoveCoalescer::AddMove(nsIMsgFolder *folder, nsMsgKey key)
{
  if (!m_destFolders)
    NS_NewISupportsArray(getter_AddRefs(m_destFolders));
  if (m_destFolders)
  {
    nsCOMPtr <nsISupports> supports = do_QueryInterface(folder);
    if (supports)
    {
      PRInt32 folderIndex = m_destFolders->IndexOf(supports);
      nsMsgKeyArray *keysToAdd=nsnull;
      if (folderIndex >= 0)
      {
        keysToAdd = (nsMsgKeyArray *) m_sourceKeyArrays.ElementAt(folderIndex);
      }
      else
      {
        m_destFolders->AppendElement(supports);
        keysToAdd = new nsMsgKeyArray;
        if (!keysToAdd)
          return NS_ERROR_OUT_OF_MEMORY;
        
        m_sourceKeyArrays.AppendElement(keysToAdd);
      }
      if (keysToAdd)
        keysToAdd->Add(key);
      return NS_OK;
    }
    else
      return NS_ERROR_NULL_POINTER;
  }
  else
    return NS_ERROR_OUT_OF_MEMORY;
  
}

nsresult nsImapMoveCoalescer::PlaybackMoves(nsIEventQueue *eventQueue)
{
  PRUint32 numFolders;
  nsresult rv = NS_OK;
  
  if (!m_destFolders)
    return NS_OK;	// nothing to do.
  
  m_destFolders->Count(&numFolders);
  for (PRUint32 i = 0; i < numFolders; i++)
  {
    nsCOMPtr <nsISupports> destSupports = getter_AddRefs(m_destFolders->ElementAt(i));
    nsCOMPtr <nsIMsgFolder> destFolder(do_QueryInterface(destSupports));
    nsCOMPtr<nsIImapService> imapService = 
      do_GetService(kCImapService, &rv);
    if (NS_SUCCEEDED(rv) && imapService)
    {
      nsMsgKeyArray *keysToAdd = (nsMsgKeyArray *) m_sourceKeyArrays.ElementAt(i);
      if (keysToAdd)
      {
        nsCString messageIds;
        
        nsImapMailFolder::AllocateUidStringFromKeys(keysToAdd->GetArray(), keysToAdd->GetSize(), messageIds);
        PRUint32 numKeysToAdd = keysToAdd->GetSize();
        
        destFolder->SetNumNewMessages(numKeysToAdd);
        destFolder->SetHasNewMessages(PR_TRUE);

        // adjust the new message count on the source folder
        PRInt32 oldNewMessageCount = 0;
        m_sourceFolder->GetNumNewMessages(PR_FALSE, &oldNewMessageCount);
        if (oldNewMessageCount >= numKeysToAdd)
          oldNewMessageCount -= numKeysToAdd;
        else
          oldNewMessageCount = 0;

        m_sourceFolder->SetNumNewMessages(oldNewMessageCount);
        
        nsCOMPtr <nsISupports> sourceSupports = do_QueryInterface(m_sourceFolder, &rv);
        nsCOMPtr <nsIUrlListener> urlListener(do_QueryInterface(sourceSupports));
        
        nsCOMPtr<nsISupportsArray> messages;
        NS_NewISupportsArray(getter_AddRefs(messages));
        for (PRUint32 keyIndex = 0; keyIndex < keysToAdd->GetSize(); keyIndex++)
        {
          nsCOMPtr<nsIMsgDBHdr> mailHdr = nsnull;
          rv = m_sourceFolder->GetMessageHeader(keysToAdd->ElementAt(keyIndex), getter_AddRefs(mailHdr));
          if (NS_SUCCEEDED(rv) && mailHdr)
          {
            nsCOMPtr<nsISupports> iSupports = do_QueryInterface(mailHdr);
            messages->AppendElement(iSupports);
          }
        }
        rv = destFolder->CopyMessages(m_sourceFolder,
          messages, PR_TRUE, m_msgWindow, 
          /*nsIMsgCopyServiceListener* listener*/ nsnull, PR_FALSE, PR_FALSE /*allowUndo*/);
          //			   rv = imapService->OnlineMessageCopy(eventQueue,
          //						m_sourceFolder, messageIds.get(),
          //						destFolder, PR_TRUE, PR_TRUE,
          //						urlListener, nsnull, nsnull);
      }
    }
  }
  return rv;
}

