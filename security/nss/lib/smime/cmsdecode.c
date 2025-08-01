/*
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
 * The Original Code is the Netscape security libraries.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1994-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
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

/*
 * CMS decoding.
 *
 * $Id: cmsdecode.c,v 1.4.18.1 2002/04/10 03:29:01 cltbld%netscape.com Exp $
 */

#include "cmslocal.h"

#include "cert.h"
#include "key.h"
#include "secasn1.h"
#include "secitem.h"
#include "secoid.h"
#include "prtime.h"
#include "secerr.h"

struct NSSCMSDecoderContextStr {
    SEC_ASN1DecoderContext *		dcx;		/* ASN.1 decoder context */
    NSSCMSMessage *			cmsg;		/* backpointer to the root message */
    SECOidTag				type;		/* type of message */
    NSSCMSContent			content;	/* pointer to message */
    NSSCMSDecoderContext *		childp7dcx;	/* inner CMS decoder context */
    PRBool				saw_contents;
    int					error;
    NSSCMSContentCallback		cb;
    void *				cb_arg;
};

static void nss_cms_decoder_update_filter (void *arg, const char *data, unsigned long len,
                          int depth, SEC_ASN1EncodingPart data_kind);
static SECStatus nss_cms_before_data(NSSCMSDecoderContext *p7dcx);
static SECStatus nss_cms_after_data(NSSCMSDecoderContext *p7dcx);
static SECStatus nss_cms_after_end(NSSCMSDecoderContext *p7dcx);
static void nss_cms_decoder_work_data(NSSCMSDecoderContext *p7dcx, 
			     const unsigned char *data, unsigned long len, PRBool final);

extern const SEC_ASN1Template NSSCMSMessageTemplate[];

/* 
 * nss_cms_decoder_notify -
 *  this is the driver of the decoding process. It gets called by the ASN.1
 *  decoder before and after an object is decoded.
 *  at various points in the decoding process, we intercept to set up and do
 *  further processing.
 */
static void
nss_cms_decoder_notify(void *arg, PRBool before, void *dest, int depth)
{
    NSSCMSDecoderContext *p7dcx;
    NSSCMSContentInfo *rootcinfo, *cinfo;
    PRBool after = !before;

    p7dcx = (NSSCMSDecoderContext *)arg;
    rootcinfo = &(p7dcx->cmsg->contentInfo);

    /* XXX error handling: need to set p7dcx->error */

#ifdef CMSDEBUG 
    fprintf(stderr, "%6.6s, dest = 0x%08x, depth = %d\n", before ? "before" : "after", dest, depth);
#endif

    /* so what are we working on right now? */
    switch (p7dcx->type) {
    case SEC_OID_UNKNOWN:
	/*
	 * right now, we are still decoding the OUTER (root) cinfo
	 * As soon as we know the inner content type, set up the info,
	 * but NO inner decoder or filter. The root decoder handles the first
	 * level children by itself - only for encapsulated contents (which
	 * are encoded as DER inside of an OCTET STRING) we need to set up a
	 * child decoder...
	 */
	if (after && dest == &(rootcinfo->contentType)) {
	    p7dcx->type = NSS_CMSContentInfo_GetContentTypeTag(rootcinfo);
	    p7dcx->content = rootcinfo->content;	/* is this ready already ? need to alloc? */
	    /* XXX yes we need to alloc -- continue here */
	}
	break;
    case SEC_OID_PKCS7_DATA:
	/* this can only happen if the outermost cinfo has DATA in it */
	/* otherwise, we handle this type implicitely in the inner decoders */

	if (before && dest == &(rootcinfo->content)) {
	    /* fake it to cause the filter to put the data in the right place... */
	    /* we want the ASN.1 decoder to deliver the decoded bytes to us from now on */
	    SEC_ASN1DecoderSetFilterProc(p7dcx->dcx,
					  nss_cms_decoder_update_filter,
					  p7dcx,
					  (PRBool)(p7dcx->cb != NULL));
	    break;
	}

	if (after && dest == &(rootcinfo->content.data)) {
	    /* remove the filter */
	    SEC_ASN1DecoderClearFilterProc(p7dcx->dcx);
	}
	break;

    case SEC_OID_PKCS7_SIGNED_DATA:
    case SEC_OID_PKCS7_ENVELOPED_DATA:
    case SEC_OID_PKCS7_DIGESTED_DATA:
    case SEC_OID_PKCS7_ENCRYPTED_DATA:

	if (before && dest == &(rootcinfo->content))
	    break;					/* we're not there yet */

	if (p7dcx->content.pointer == NULL)
	    p7dcx->content = rootcinfo->content;

	/* get this data type's inner contentInfo */
	cinfo = NSS_CMSContent_GetContentInfo(p7dcx->content.pointer, p7dcx->type);

	if (before && dest == &(cinfo->contentType)) {
	    /* at this point, set up the &%$&$ back pointer */
	    /* we cannot do it later, because the content itself is optional! */
	    /* please give me C++ */
	    switch (p7dcx->type) {
	    case SEC_OID_PKCS7_SIGNED_DATA:
		p7dcx->content.signedData->cmsg = p7dcx->cmsg;
		break;
	    case SEC_OID_PKCS7_DIGESTED_DATA:
		p7dcx->content.digestedData->cmsg = p7dcx->cmsg;
		break;
	    case SEC_OID_PKCS7_ENVELOPED_DATA:
		p7dcx->content.envelopedData->cmsg = p7dcx->cmsg;
		break;
	    case SEC_OID_PKCS7_ENCRYPTED_DATA:
		p7dcx->content.encryptedData->cmsg = p7dcx->cmsg;
		break;
	    default:
		PORT_Assert(0);
		break;
	    }
	}

	if (before && dest == &(cinfo->rawContent)) {
	    /* we want the ASN.1 decoder to deliver the decoded bytes to us from now on */
	    SEC_ASN1DecoderSetFilterProc(p7dcx->dcx, nss_cms_decoder_update_filter,
					  p7dcx, (PRBool)(p7dcx->cb != NULL));


	    /* we're right in front of the data */
	    if (nss_cms_before_data(p7dcx) != SECSuccess) {
		SEC_ASN1DecoderClearFilterProc(p7dcx->dcx);	/* stop all processing */
		p7dcx->error = PORT_GetError();
	    }
	}
	if (after && dest == &(cinfo->rawContent)) {
	    /* we're right after of the data */
	    if (nss_cms_after_data(p7dcx) != SECSuccess)
		p7dcx->error = PORT_GetError();

	    /* we don't need to see the contents anymore */
	    SEC_ASN1DecoderClearFilterProc(p7dcx->dcx);
	}
	break;

#if 0 /* NIH */
    case SEC_OID_PKCS7_AUTHENTICATED_DATA:
#endif
    default:
	/* unsupported or unknown message type - fail (more or less) gracefully */
	p7dcx->error = SEC_ERROR_UNSUPPORTED_MESSAGE_TYPE;
	break;
    }
}

/*
 * nss_cms_before_data - set up the current encoder to receive data
 */
static SECStatus
nss_cms_before_data(NSSCMSDecoderContext *p7dcx)
{
    SECStatus rv;
    SECOidTag childtype;
    PLArenaPool *poolp;
    NSSCMSDecoderContext *childp7dcx;
    NSSCMSContentInfo *cinfo;
    const SEC_ASN1Template *template;
    void *mark = NULL;
    size_t size;
    
    poolp = p7dcx->cmsg->poolp;

    /* call _Decode_BeforeData handlers */
    switch (p7dcx->type) {
    case SEC_OID_PKCS7_SIGNED_DATA:
	/* we're decoding a signedData, so set up the digests */
	rv = NSS_CMSSignedData_Decode_BeforeData(p7dcx->content.signedData);
	if (rv != SECSuccess)
	    return SECFailure;
	break;
    case SEC_OID_PKCS7_DIGESTED_DATA:
	/* we're encoding a digestedData, so set up the digest */
	rv = NSS_CMSDigestedData_Decode_BeforeData(p7dcx->content.digestedData);
	if (rv != SECSuccess)
	    return SECFailure;
	break;
    case SEC_OID_PKCS7_ENVELOPED_DATA:
	rv = NSS_CMSEnvelopedData_Decode_BeforeData(p7dcx->content.envelopedData);
	if (rv != SECSuccess)
	    return SECFailure;
	break;
    case SEC_OID_PKCS7_ENCRYPTED_DATA:
	rv = NSS_CMSEncryptedData_Decode_BeforeData(p7dcx->content.encryptedData);
	if (rv != SECSuccess)
	    return SECFailure;
	break;
    default:
	return SECFailure;
    }

    /* ok, now we have a pointer to cinfo */
    /* find out what kind of data is encapsulated */
    
    cinfo = NSS_CMSContent_GetContentInfo(p7dcx->content.pointer, p7dcx->type);
    childtype = NSS_CMSContentInfo_GetContentTypeTag(cinfo);

    if (childtype == SEC_OID_PKCS7_DATA) {
	cinfo->content.data = SECITEM_AllocItem(poolp, NULL, 0);
	if (cinfo->content.data == NULL)
	    /* set memory error */
	    return SECFailure;

	p7dcx->childp7dcx = NULL;
	return SECSuccess;
    }

    /* set up inner decoder */

    if ((template = NSS_CMSUtil_GetTemplateByTypeTag(childtype)) == NULL)
	return SECFailure;

    childp7dcx = (NSSCMSDecoderContext *)PORT_ZAlloc(sizeof(NSSCMSDecoderContext));
    if (childp7dcx == NULL)
	return SECFailure;

    mark = PORT_ArenaMark(poolp);

    /* allocate space for the stuff we're creating */
    size = NSS_CMSUtil_GetSizeByTypeTag(childtype);
    childp7dcx->content.pointer = (void *)PORT_ArenaZAlloc(poolp, size);
    if (childp7dcx->content.pointer == NULL)
	goto loser;

    /* start the child decoder */
    childp7dcx->dcx = SEC_ASN1DecoderStart(poolp, childp7dcx->content.pointer, template);
    if (childp7dcx->dcx == NULL)
	goto loser;

    /* the new decoder needs to notify, too */
    SEC_ASN1DecoderSetNotifyProc(childp7dcx->dcx, nss_cms_decoder_notify, childp7dcx);

    /* tell the parent decoder that it needs to feed us the content data */
    p7dcx->childp7dcx = childp7dcx;

    childp7dcx->type = childtype;	/* our type */

    childp7dcx->cmsg = p7dcx->cmsg;	/* backpointer to root message */

    /* should the child decoder encounter real data, it needs to give it to the caller */
    childp7dcx->cb = p7dcx->cb;
    childp7dcx->cb_arg = p7dcx->cb_arg;

    /* now set up the parent to hand decoded data to the next level decoder */
    p7dcx->cb = (NSSCMSContentCallback)NSS_CMSDecoder_Update;
    p7dcx->cb_arg = childp7dcx;

    PORT_ArenaUnmark(poolp, mark);

    return SECSuccess;

loser:
    if (mark)
	PORT_ArenaRelease(poolp, mark);
    if (childp7dcx)
	PORT_Free(childp7dcx);
    p7dcx->childp7dcx = NULL;
    return SECFailure;
}

static SECStatus
nss_cms_after_data(NSSCMSDecoderContext *p7dcx)
{
    PLArenaPool *poolp;
    NSSCMSDecoderContext *childp7dcx;
    SECStatus rv = SECFailure;

    poolp = p7dcx->cmsg->poolp;

    /* Handle last block. This is necessary to flush out the last bytes
     * of a possibly incomplete block */
    nss_cms_decoder_work_data(p7dcx, NULL, 0, PR_TRUE);

    /* finish any "inner" decoders - there's no more data coming... */
    if (p7dcx->childp7dcx != NULL) {
	childp7dcx = p7dcx->childp7dcx;
	if (childp7dcx->dcx != NULL) {
	    if (SEC_ASN1DecoderFinish(childp7dcx->dcx) != SECSuccess) {
		/* do what? free content? */
		rv = SECFailure;
	    } else {
		rv = nss_cms_after_end(childp7dcx);
	    }
	    if (rv != SECSuccess)
		goto done;
	}
	PORT_Free(p7dcx->childp7dcx);
	p7dcx->childp7dcx = NULL;
    }

    switch (p7dcx->type) {
    case SEC_OID_PKCS7_SIGNED_DATA:
	/* this will finish the digests and verify */
	rv = NSS_CMSSignedData_Decode_AfterData(p7dcx->content.signedData);
	break;
    case SEC_OID_PKCS7_ENVELOPED_DATA:
	rv = NSS_CMSEnvelopedData_Decode_AfterData(p7dcx->content.envelopedData);
	break;
    case SEC_OID_PKCS7_DIGESTED_DATA:
	rv = NSS_CMSDigestedData_Decode_AfterData(p7dcx->content.digestedData);
	break;
    case SEC_OID_PKCS7_ENCRYPTED_DATA:
	rv = NSS_CMSEncryptedData_Decode_AfterData(p7dcx->content.encryptedData);
	break;
    case SEC_OID_PKCS7_DATA:
	/* do nothing */
	break;
    default:
	rv = SECFailure;
	break;
    }
done:
    return rv;
}

static SECStatus
nss_cms_after_end(NSSCMSDecoderContext *p7dcx)
{
    SECStatus rv;
    PLArenaPool *poolp;

    poolp = p7dcx->cmsg->poolp;

    switch (p7dcx->type) {
    case SEC_OID_PKCS7_SIGNED_DATA:
	rv = NSS_CMSSignedData_Decode_AfterEnd(p7dcx->content.signedData);
	break;
    case SEC_OID_PKCS7_ENVELOPED_DATA:
	rv = NSS_CMSEnvelopedData_Decode_AfterEnd(p7dcx->content.envelopedData);
	break;
    case SEC_OID_PKCS7_DIGESTED_DATA:
	rv = NSS_CMSDigestedData_Decode_AfterEnd(p7dcx->content.digestedData);
	break;
    case SEC_OID_PKCS7_ENCRYPTED_DATA:
	rv = NSS_CMSEncryptedData_Decode_AfterEnd(p7dcx->content.encryptedData);
	break;
    case SEC_OID_PKCS7_DATA:
	rv = SECSuccess;
	break;
    default:
	rv = SECFailure;	/* we should not have got that far... */
	break;
    }
    return rv;
}

/*
 * nss_cms_decoder_work_data - handle decoded data bytes.
 *
 * This function either decrypts the data if needed, and/or calculates digests
 * on it, then either stores it or passes it on to the next level decoder.
 */
static void
nss_cms_decoder_work_data(NSSCMSDecoderContext *p7dcx, 
			     const unsigned char *data, unsigned long len,
			     PRBool final)
{
    NSSCMSContentInfo *cinfo;
    unsigned char *buf = NULL;
    unsigned char *dest;
    unsigned int offset;
    SECStatus rv;
    SECItem *storage;

    /*
     * We should really have data to process, or we should be trying
     * to finish/flush the last block.  (This is an overly paranoid
     * check since all callers are in this file and simple inspection
     * proves they do it right.  But it could find a bug in future
     * modifications/development, that is why it is here.)
     */
    PORT_Assert ((data != NULL && len) || final);

    cinfo = NSS_CMSContent_GetContentInfo(p7dcx->content.pointer, p7dcx->type);

    if (cinfo->ciphcx != NULL) {
	/*
	 * we are decrypting.
	 * 
	 * XXX If we get an error, we do not want to do the digest or callback,
	 * but we want to keep decoding.  Or maybe we want to stop decoding
	 * altogether if there is a callback, because obviously we are not
	 * sending the data back and they want to know that.
	 */

	unsigned int outlen = 0;	/* length of decrypted data */
	unsigned int buflen;		/* length available for decrypted data */

	/* find out about the length of decrypted data */
	buflen = NSS_CMSCipherContext_DecryptLength(cinfo->ciphcx, len, final);

	/*
	 * it might happen that we did not provide enough data for a full
	 * block (decryption unit), and that there is no output available
	 */

	/* no output available, AND no input? */
	if (buflen == 0 && len == 0)
	    goto loser;	/* bail out */

	/*
	 * have inner decoder: pass the data on (means inner content type is NOT data)
	 * no inner decoder: we have DATA in here: either call callback or store
	 */
	if (buflen != 0) {
	    /* there will be some output - need to make room for it */
	    /* allocate buffer from the heap */
	    buf = (unsigned char *)PORT_Alloc(buflen);
	    if (buf == NULL) {
		p7dcx->error = SEC_ERROR_NO_MEMORY;
		goto loser;
	    }
	}

	/*
	 * decrypt incoming data
	 * buf can still be NULL here (and buflen == 0) here if we don't expect
	 * any output (see above), but we still need to call NSS_CMSCipherContext_Decrypt to
	 * keep track of incoming data
	 */
	rv = NSS_CMSCipherContext_Decrypt(cinfo->ciphcx, buf, &outlen, buflen,
			       data, len, final);
	if (rv != SECSuccess) {
	    p7dcx->error = PORT_GetError();
	    goto loser;
	}

	PORT_Assert (final || outlen == buflen);
	
	/* swap decrypted data in */
	data = buf;
	len = outlen;
    }

    if (len == 0)
	goto done;		/* nothing more to do */

    /*
     * Update the running digests with plaintext bytes (if we need to).
     */
    if (cinfo->digcx)
	NSS_CMSDigestContext_Update(cinfo->digcx, data, len);

    /* at this point, we have the plain decoded & decrypted data */
    /* which is either more encoded DER which we need to hand to the child decoder */
    /*              or data we need to hand back to our caller */

    /* pass the content back to our caller or */
    /* feed our freshly decrypted and decoded data into child decoder */
    if (p7dcx->cb != NULL) {
	(*p7dcx->cb)(p7dcx->cb_arg, (const char *)data, len);
    }
#if 1
    else
#endif
    if (NSS_CMSContentInfo_GetContentTypeTag(cinfo) == SEC_OID_PKCS7_DATA) {
	/* store it in "inner" data item as well */
	/* find the DATA item in the encapsulated cinfo and store it there */
	storage = cinfo->content.data;

	offset = storage->len;
	if (storage->len == 0) {
	    dest = (unsigned char *)PORT_ArenaAlloc(p7dcx->cmsg->poolp, len);
	} else {
	    dest = (unsigned char *)PORT_ArenaGrow(p7dcx->cmsg->poolp, 
				  storage->data,
				  storage->len,
				  storage->len + len);
	}
	if (dest == NULL) {
	    p7dcx->error = SEC_ERROR_NO_MEMORY;
	    goto loser;
	}

	storage->data = dest;
	storage->len += len;

	/* copy it in */
	PORT_Memcpy(storage->data + offset, data, len);
    }

done:
loser:
    if (buf)
	PORT_Free (buf);
}

/*
 * nss_cms_decoder_update_filter - process ASN.1 data
 *
 * once we have set up a filter in nss_cms_decoder_notify(),
 * all data processed by the ASN.1 decoder is also passed through here.
 * we pass the content bytes (as opposed to length and tag bytes) on to
 * nss_cms_decoder_work_data().
 */
static void
nss_cms_decoder_update_filter (void *arg, const char *data, unsigned long len,
			  int depth, SEC_ASN1EncodingPart data_kind)
{
    NSSCMSDecoderContext *p7dcx;

    PORT_Assert (len);	/* paranoia */
    if (len == 0)
	return;

    p7dcx = (NSSCMSDecoderContext*)arg;

    p7dcx->saw_contents = PR_TRUE;

    /* pass on the content bytes only */
    if (data_kind == SEC_ASN1_Contents)
	nss_cms_decoder_work_data(p7dcx, (const unsigned char *) data, len, PR_FALSE);
}

/*
 * NSS_CMSDecoder_Start - set up decoding of a DER-encoded CMS message
 *
 * "poolp" - pointer to arena for message, or NULL if new pool should be created
 * "cb", "cb_arg" - callback function and argument for delivery of inner content
 * "pwfn", pwfn_arg" - callback function for getting token password
 * "decrypt_key_cb", "decrypt_key_cb_arg" - callback function for getting bulk key for encryptedData
 */
NSSCMSDecoderContext *
NSS_CMSDecoder_Start(PRArenaPool *poolp,
		      NSSCMSContentCallback cb, void *cb_arg,
		      PK11PasswordFunc pwfn, void *pwfn_arg,
		      NSSCMSGetDecryptKeyCallback decrypt_key_cb, void *decrypt_key_cb_arg)
{
    NSSCMSDecoderContext *p7dcx;
    NSSCMSMessage *cmsg;

    cmsg = NSS_CMSMessage_Create(poolp);
    if (cmsg == NULL)
	return NULL;

    NSS_CMSMessage_SetEncodingParams(cmsg, pwfn, pwfn_arg, decrypt_key_cb, decrypt_key_cb_arg,
					NULL, NULL);

    p7dcx = (NSSCMSDecoderContext*)PORT_ZAlloc(sizeof(NSSCMSDecoderContext));
    if (p7dcx == NULL) {
	NSS_CMSMessage_Destroy(cmsg);
	return NULL;
    }

    p7dcx->dcx = SEC_ASN1DecoderStart(cmsg->poolp, cmsg, NSSCMSMessageTemplate);
    if (p7dcx->dcx == NULL) {
	PORT_Free (p7dcx);
	NSS_CMSMessage_Destroy(cmsg);
	return NULL;
    }

    SEC_ASN1DecoderSetNotifyProc (p7dcx->dcx, nss_cms_decoder_notify, p7dcx);

    p7dcx->cmsg = cmsg;
    p7dcx->type = SEC_OID_UNKNOWN;

    p7dcx->cb = cb;
    p7dcx->cb_arg = cb_arg;

    return p7dcx;
}

/*
 * NSS_CMSDecoder_Update - feed DER-encoded data to decoder
 */
SECStatus
NSS_CMSDecoder_Update(NSSCMSDecoderContext *p7dcx, const char *buf, unsigned long len)
{
    if (p7dcx->dcx != NULL && p7dcx->error == 0) {	/* if error is set already, don't bother */
	if (SEC_ASN1DecoderUpdate (p7dcx->dcx, buf, len) != SECSuccess) {
	    p7dcx->error = PORT_GetError();
	    PORT_Assert (p7dcx->error);
	    if (p7dcx->error == 0)
		p7dcx->error = -1;
	}
    }

    if (p7dcx->error == 0)
	return SECSuccess;

    /* there has been a problem, let's finish the decoder */
    if (p7dcx->dcx != NULL) {
	(void) SEC_ASN1DecoderFinish (p7dcx->dcx);
	p7dcx->dcx = NULL;
    }
    PORT_SetError (p7dcx->error);

    return SECFailure;
}

/*
 * NSS_CMSDecoder_Cancel - stop decoding in case of error
 */
void
NSS_CMSDecoder_Cancel(NSSCMSDecoderContext *p7dcx)
{
    /* XXXX what about inner decoders? running digests? decryption? */
    /* XXXX there's a leak here! */
    NSS_CMSMessage_Destroy(p7dcx->cmsg);
    (void)SEC_ASN1DecoderFinish(p7dcx->dcx);
    PORT_Free(p7dcx);
}

/*
 * NSS_CMSDecoder_Finish - mark the end of inner content and finish decoding
 */
NSSCMSMessage *
NSS_CMSDecoder_Finish(NSSCMSDecoderContext *p7dcx)
{
    NSSCMSMessage *cmsg;

    cmsg = p7dcx->cmsg;

    if (p7dcx->dcx == NULL || SEC_ASN1DecoderFinish(p7dcx->dcx) != SECSuccess ||
	nss_cms_after_end(p7dcx) != SECSuccess)
    {
	NSS_CMSMessage_Destroy(cmsg);	/* needs to get rid of pool if it's ours */
	cmsg = NULL;
    }

    PORT_Free(p7dcx);
    return cmsg;
}

NSSCMSMessage *
NSS_CMSMessage_CreateFromDER(SECItem *DERmessage,
		    NSSCMSContentCallback cb, void *cb_arg,
		    PK11PasswordFunc pwfn, void *pwfn_arg,
		    NSSCMSGetDecryptKeyCallback decrypt_key_cb, void *decrypt_key_cb_arg)
{
    NSSCMSDecoderContext *p7dcx;

    /* first arg(poolp) == NULL => create our own pool */
    p7dcx = NSS_CMSDecoder_Start(NULL, cb, cb_arg, pwfn, pwfn_arg, decrypt_key_cb, decrypt_key_cb_arg);
    (void) NSS_CMSDecoder_Update(p7dcx, (char *)DERmessage->data, DERmessage->len);
    return NSS_CMSDecoder_Finish(p7dcx);
}

