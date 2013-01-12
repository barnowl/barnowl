/*
 * Family 0x0004 - Routines for sending/receiving Instant Messages.
 *
 * Note the term ICBM (Inter-Client Basic Message) which blankets
 * all types of genericly routed through-server messages.  Within
 * the ICBM types (family 4), a channel is defined.  Each channel
 * represents a different type of message.  Channel 1 is used for
 * what would commonly be called an "instant message".  Channel 2
 * is used for negotiating "rendezvous".  These transactions end in
 * something more complex happening, such as a chat invitation, or
 * a file transfer.  Channel 3 is used for chat messages (not in
 * the same family as these channels).  Channel 4 is used for
 * various ICQ messages.  Examples are normal messages, URLs, and
 * old-style authorization.
 *
 * In addition to the channel, every ICBM contains a cookie.  For
 * standard IMs, these are only used for error messages.  However,
 * the more complex rendezvous messages make suitably more complex
 * use of this field.
 *
 * TODO: Split this up into an im.c file an an icbm.c file.  It
 *       will be beautiful, you'll see.
 *
 *       Need to rename all the mpmsg messages to aim_im_bleh.
 *
 *       Make sure aim_conn_findbygroup is used by all functions.
 */

#define FAIM_INTERNAL
#include <aim.h>

#ifdef _WIN32
#include "win32dep.h"
#endif

/**
 * Add a standard ICBM header to the given bstream with the given
 * information.
 *
 * @param bs The bstream to write the ICBM header to.
 * @param c c is for cookie, and cookie is for me.
 * @param ch The ICBM channel (1 through 4).
 * @param sn Null-terminated scrizeen nizame.
 * @return The number of bytes written.  It's really not useful.
 */
static int aim_im_puticbm(aim_bstream_t *bs, const fu8_t *c, fu16_t ch, const char *sn)
{
	aimbs_putraw(bs, c, 8);
	aimbs_put16(bs, ch);
	aimbs_put8(bs, strlen(sn));
	aimbs_putraw(bs, sn, strlen(sn));
	return 8+2+1+strlen(sn);
}

/*
 * Takes a msghdr (and a length) and returns a client type
 * code.  Note that this is *only a guess* and has a low likelihood
 * of actually being accurate.
 *
 * Its based on experimental data, with the help of Eric Warmenhoven
 * who seems to have collected a wide variety of different AIM clients.
 *
 *
 * Heres the current collection:
 *  0501 0003 0101 0101 01		AOL Mobile Communicator, WinAIM 1.0.414
 *  0501 0003 0101 0201 01		WinAIM 2.0.847, 2.1.1187, 3.0.1464,
 *					4.3.2229, 4.4.2286
 *  0501 0004 0101 0102 0101		WinAIM 4.1.2010, libfaim (right here)
 *  0501 0003 0101 02			WinAIM 5
 *  0501 0001 01			iChat x.x, mobile buddies
 *  0501 0001 0101 01			AOL v6.0, CompuServe 2000 v6.0, any TOC client
 *  0501 0002 0106			WinICQ 5.45.1.3777.85
 *
 * Note that in this function, only the feature bytes are tested, since
 * the rest will always be the same.
 *
 */
faim_export fu16_t aim_im_fingerprint(const fu8_t *msghdr, int len)
{
	static const struct {
		fu16_t clientid;
		int len;
		fu8_t data[10];
	} fingerprints[] = {
		/* AOL Mobile Communicator, WinAIM 1.0.414 */
		{ AIM_CLIENTTYPE_MC,
		  3, {0x01, 0x01, 0x01}},

		/* WinAIM 2.0.847, 2.1.1187, 3.0.1464, 4.3.2229, 4.4.2286 */
		{ AIM_CLIENTTYPE_WINAIM,
		  3, {0x01, 0x01, 0x02}},

		/* WinAIM 4.1.2010, libfaim */
		{ AIM_CLIENTTYPE_WINAIM41,
		  4, {0x01, 0x01, 0x01, 0x02}},

		/* AOL v6.0, CompuServe 2000 v6.0, any TOC client */
		{ AIM_CLIENTTYPE_AOL_TOC,
		  1, {0x01}},

		{ 0, 0}
	};
	int i;

	if (!msghdr || (len <= 0))
		return AIM_CLIENTTYPE_UNKNOWN;

	for (i = 0; fingerprints[i].len; i++) {
		if (fingerprints[i].len != len)
			continue;
		if (memcmp(fingerprints[i].data, msghdr, fingerprints[i].len) == 0)
			return fingerprints[i].clientid;
	}

	return AIM_CLIENTTYPE_UNKNOWN;
}

/**
 * Subtype 0x0002 - Set ICBM parameters.
 *
 * I definitly recommend sending this.  If you don't, you'll be stuck
 * with the rather unreasonable defaults.
 *
 */
faim_export int aim_im_setparams(aim_session_t *sess, struct aim_icbmparameters *params)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0004)))
		return -EINVAL;

	if (!params)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+16)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0004, 0x0002, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0004, 0x0002, 0x0000, snacid);

	/* This is read-only (see Parameter Reply). Must be set to zero here. */
	aimbs_put16(&fr->data, 0x0000);

	/* These are all read-write */
	aimbs_put32(&fr->data, params->flags);
	aimbs_put16(&fr->data, params->maxmsglen);
	aimbs_put16(&fr->data, params->maxsenderwarn);
	aimbs_put16(&fr->data, params->maxrecverwarn);
	aimbs_put32(&fr->data, params->minmsginterval);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/**
 * Subtype 0x0004 - Request ICBM parameter information.
 *
 */
faim_export int aim_im_reqparams(aim_session_t *sess)
{
	aim_conn_t *conn;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0004)))
		return -EINVAL;

	return aim_genericreq_n_snacid(sess, conn, 0x0004, 0x0004);
}

/**
 * Subtype 0x0005 - Receive parameter information.
 *
 */
static int aim_im_paraminfo(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	aim_rxcallback_t userfunc;
	struct aim_icbmparameters params;

	params.maxchan = aimbs_get16(bs);
	params.flags = aimbs_get32(bs);
	params.maxmsglen = aimbs_get16(bs);
	params.maxsenderwarn = aimbs_get16(bs);
	params.maxrecverwarn = aimbs_get16(bs);
	params.minmsginterval = aimbs_get32(bs);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		return userfunc(sess, rx, &params);

	return 0;
}

/**
 * Subtype 0x0006 - Send an ICBM (instant message).
 *
 *
 * Possible flags:
 *   AIM_IMFLAGS_AWAY  -- Marks the message as an autoresponse
 *   AIM_IMFLAGS_ACK   -- Requests that the server send an ack
 *                        when the message is received (of type 0x0004/0x000c)
 *   AIM_IMFLAGS_OFFLINE--If destination is offline, store it until they are
 *                        online (probably ICQ only).
 *   AIM_IMFLAGS_UNICODE--Instead of ASCII7, the passed message is
 *                        made up of UNICODE duples.  If you set
 *                        this, you'd better be damn sure you know
 *                        what you're doing.
 *   AIM_IMFLAGS_ISO_8859_1 -- The message contains the ASCII8 subset
 *                        known as ISO-8859-1.
 *
 * Generally, you should use the lowest encoding possible to send
 * your message.  If you only use basic punctuation and the generic
 * Latin alphabet, use ASCII7 (no flags).  If you happen to use non-ASCII7
 * characters, but they are all clearly defined in ISO-8859-1, then
 * use that.  Keep in mind that not all characters in the PC ASCII8
 * character set are defined in the ISO standard. For those cases (most
 * notably when the (r) symbol is used), you must use the full UNICODE
 * encoding for your message.  In UNICODE mode, _all_ characters must
 * occupy 16bits, including ones that are not special.  (Remember that
 * the first 128 UNICODE symbols are equivelent to ASCII7, however they
 * must be prefixed with a zero high order byte.)
 *
 * I strongly discourage the use of UNICODE mode, mainly because none
 * of the clients I use can parse those messages (and besides that,
 * wchars are difficult and non-portable to handle in most UNIX environments).
 * If you really need to include special characters, use the HTML UNICODE
 * entities.  These are of the form &#2026; where 2026 is the hex
 * representation of the UNICODE index (in this case, UNICODE
 * "Horizontal Ellipsis", or 133 in in ASCII8).
 *
 * Implementation note:  Since this is one of the most-used functions
 * in all of libfaim, it is written with performance in mind.  As such,
 * it is not as clear as it could be in respect to how this message is
 * supposed to be layed out. Most obviously, tlvlists should be used
 * instead of writing out the bytes manually.
 *
 * XXX - more precise verification that we never send SNACs larger than 8192
 * XXX - check SNAC size for multipart
 *
 */
faim_export int aim_im_sendch1_ext(aim_session_t *sess, struct aim_sendimext_args *args)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	fu8_t ck[8];
	int i, msgtlvlen;
	static const fu8_t deffeatures[] = { 0x01, 0x01, 0x01, 0x02 };

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0004)))
		return -EINVAL;

	if (!args)
		return -EINVAL;

	if (args->flags & AIM_IMFLAGS_MULTIPART) {
		if (args->mpmsg->numparts <= 0)
			return -EINVAL;
	} else {
		if (!args->msg || (args->msglen <= 0))
			return -EINVAL;

		if (args->msglen >= MAXMSGLEN)
			return -E2BIG;
	}

	/* Painfully calculate the size of the message TLV */
	msgtlvlen = 1 + 1; /* 0501 */

	if (args->flags & AIM_IMFLAGS_CUSTOMFEATURES)
		msgtlvlen += 2 + args->featureslen;
	else
		msgtlvlen += 2 + sizeof(deffeatures);

	if (args->flags & AIM_IMFLAGS_MULTIPART) {
		aim_mpmsg_section_t *sec;

		for (sec = args->mpmsg->parts; sec; sec = sec->next) {
			msgtlvlen += 2 /* 0101 */ + 2 /* block len */;
			msgtlvlen += 4 /* charset */ + sec->datalen;
		}

	} else {
		msgtlvlen += 2 /* 0101 */ + 2 /* block len */;
		msgtlvlen += 4 /* charset */ + args->msglen;
	}

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, msgtlvlen+128)))
		return -ENOMEM;

	/* XXX - should be optional */
	snacid = aim_cachesnac(sess, 0x0004, 0x0006, 0x0000, args->destsn, strlen(args->destsn)+1);
	aim_putsnac(&fr->data, 0x0004, 0x0006, 0x0000, snacid);

	/*
	 * Generate a random message cookie
	 *
	 * We could cache these like we do SNAC IDs.  (In fact, it
	 * might be a good idea.)  In the message error functions,
	 * the 8byte message cookie is returned as well as the
	 * SNAC ID.
	 *
	 */
	for (i = 0; i < 8; i++)
		ck[i] = (fu8_t)rand();

	/* ICBM header */
	aim_im_puticbm(&fr->data, ck, 0x0001, args->destsn);

	/* Message TLV (type 0x0002) */
	aimbs_put16(&fr->data, 0x0002);
	aimbs_put16(&fr->data, msgtlvlen);

	/* Features TLV (type 0x0501) */
	aimbs_put16(&fr->data, 0x0501);
	if (args->flags & AIM_IMFLAGS_CUSTOMFEATURES) {
		aimbs_put16(&fr->data, args->featureslen);
		aimbs_putraw(&fr->data, args->features, args->featureslen);
	} else {
		aimbs_put16(&fr->data, sizeof(deffeatures));
		aimbs_putraw(&fr->data, deffeatures, sizeof(deffeatures));
	}

	if (args->flags & AIM_IMFLAGS_MULTIPART) {
		aim_mpmsg_section_t *sec;

		/* Insert each message part in a TLV (type 0x0101) */
		for (sec = args->mpmsg->parts; sec; sec = sec->next) {
			aimbs_put16(&fr->data, 0x0101);
			aimbs_put16(&fr->data, sec->datalen + 4);
			aimbs_put16(&fr->data, sec->charset);
			aimbs_put16(&fr->data, sec->charsubset);
			aimbs_putraw(&fr->data, sec->data, sec->datalen);
		}

	} else {

		/* Insert message text in a TLV (type 0x0101) */
		aimbs_put16(&fr->data, 0x0101);

		/* Message block length */
		aimbs_put16(&fr->data, args->msglen + 0x04);

		/* Character set */
		aimbs_put16(&fr->data, args->charset);
		aimbs_put16(&fr->data, args->charsubset);

		/* Message.  Not terminated */
		aimbs_putraw(&fr->data, args->msg, args->msglen);
	}

	/* Set the Autoresponse flag */
	if (args->flags & AIM_IMFLAGS_AWAY) {
		aimbs_put16(&fr->data, 0x0004);
		aimbs_put16(&fr->data, 0x0000);
	} else if (args->flags & AIM_IMFLAGS_ACK) {
		/* Set the Request Acknowledge flag */
		aimbs_put16(&fr->data, 0x0003);
		aimbs_put16(&fr->data, 0x0000);
	}

	if (args->flags & AIM_IMFLAGS_OFFLINE) {
		aimbs_put16(&fr->data, 0x0006);
		aimbs_put16(&fr->data, 0x0000);
	}

	/*
	 * Set the I HAVE A REALLY PURTY ICON flag.
	 * XXX - This should really only be sent on initial
	 * IMs and when you change your icon.
	 */
	if (args->flags & AIM_IMFLAGS_HASICON) {
		aimbs_put16(&fr->data, 0x0008);
		aimbs_put16(&fr->data, 0x000c);
		aimbs_put32(&fr->data, args->iconlen);
		aimbs_put16(&fr->data, 0x0001);
		aimbs_put16(&fr->data, args->iconsum);
		aimbs_put32(&fr->data, args->iconstamp);
	}

	/*
	 * Set the Buddy Icon Requested flag.
	 * XXX - Everytime?  Surely not...
	 */
	if (args->flags & AIM_IMFLAGS_BUDDYREQ) {
		aimbs_put16(&fr->data, 0x0009);
		aimbs_put16(&fr->data, 0x0000);
	}

	aim_tx_enqueue(sess, fr);

	/* clean out SNACs over 60sec old */
	aim_cleansnacs(sess, 60);

	return 0;
}

/*
 * Simple wrapper for aim_im_sendch1_ext()
 *
 * You cannot use aim_send_im if you need the HASICON flag.  You must
 * use aim_im_sendch1_ext directly for that.
 *
 * aim_send_im also cannot be used if you require UNICODE messages, because
 * that requires an explicit message length.  Use aim_im_sendch1_ext().
 *
 */
faim_export int aim_im_sendch1(aim_session_t *sess, const char *sn, fu16_t flags, const char *msg)
{
	struct aim_sendimext_args args;

	args.destsn = sn;
	args.flags = flags;
	args.msg = msg;
	args.msglen = strlen(msg);
	args.charset = 0x0000;
	args.charsubset = 0x0000;

	/* Make these don't get set by accident -- they need aim_im_sendch1_ext */
	args.flags &= ~(AIM_IMFLAGS_CUSTOMFEATURES | AIM_IMFLAGS_HASICON | AIM_IMFLAGS_MULTIPART);

	return aim_im_sendch1_ext(sess, &args);
}

/**
 * Subtype 0x0006 - Send your icon to a given user.
 *
 * This is also performance sensitive. (If you can believe it...)
 *
 */
faim_export int aim_im_sendch2_icon(aim_session_t *sess, const char *sn, const fu8_t *icon, int iconlen, time_t stamp, fu16_t iconsum)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	fu8_t ck[8];
	int i;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0004)))
		return -EINVAL;

	if (!sn || !icon || (iconlen <= 0) || (iconlen >= MAXICONLEN))
		return -EINVAL;

	for (i = 0; i < 8; i++)
		ck[i] = (fu8_t)rand();

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+8+2+1+strlen(sn)+2+2+2+8+16+2+2+2+2+2+2+2+4+4+4+iconlen+strlen(AIM_ICONIDENT)+2+2)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0004, 0x0006, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0004, 0x0006, 0x0000, snacid);

	/* ICBM header */
	aim_im_puticbm(&fr->data, ck, 0x0002, sn);

	/*
	 * TLV t(0005)
	 *
	 * Encompasses everything below.
	 */
	aimbs_put16(&fr->data, 0x0005);
	aimbs_put16(&fr->data, 2+8+16+6+4+4+iconlen+4+4+4+strlen(AIM_ICONIDENT));

	aimbs_put16(&fr->data, 0x0000);
	aimbs_putraw(&fr->data, ck, 8);
	aim_putcap(&fr->data, AIM_CAPS_BUDDYICON);

	/* TLV t(000a) */
	aimbs_put16(&fr->data, 0x000a);
	aimbs_put16(&fr->data, 0x0002);
	aimbs_put16(&fr->data, 0x0001);

	/* TLV t(000f) */
	aimbs_put16(&fr->data, 0x000f);
	aimbs_put16(&fr->data, 0x0000);

	/* TLV t(2711) */
	aimbs_put16(&fr->data, 0x2711);
	aimbs_put16(&fr->data, 4+4+4+iconlen+strlen(AIM_ICONIDENT));
	aimbs_put16(&fr->data, 0x0000);
	aimbs_put16(&fr->data, iconsum);
	aimbs_put32(&fr->data, iconlen);
	aimbs_put32(&fr->data, stamp);
	aimbs_putraw(&fr->data, icon, iconlen);
	aimbs_putraw(&fr->data, AIM_ICONIDENT, strlen(AIM_ICONIDENT));

	/* TLV t(0003) */
	aimbs_put16(&fr->data, 0x0003);
	aimbs_put16(&fr->data, 0x0000);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x0006 - Send a rich text message.
 *
 * This only works for ICQ 2001b (thats 2001 not 2000).  Better, only
 * send it to clients advertising the RTF capability.  In fact, if you send
 * it to a client that doesn't support that capability, the server will gladly
 * bounce it back to you.
 *
 * You'd think this would be in icq.c, but, well, I'm trying to stick with
 * the one-group-per-file scheme as much as possible.  This could easily
 * be an exception, since Rendezvous IMs are external of the Oscar core,
 * and therefore are undefined.  Really I just need to think of a good way to
 * make an interface similar to what AOL actually uses.  But I'm not using COM.
 *
 */
faim_export int aim_im_sendch2_rtfmsg(aim_session_t *sess, struct aim_sendrtfmsg_args *args)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	fu8_t ck[8];
	const char rtfcap[] = {"{97B12751-243C-4334-AD22-D6ABF73F1492}"}; /* AIM_CAPS_ICQRTF capability in string form */
	int i, servdatalen;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0004)))
		return -EINVAL;

	if (!args || !args->destsn || !args->rtfmsg)
		return -EINVAL;

	servdatalen = 2+2+16+2+4+1+2  +  2+2+4+4+4  +  2+4+2+strlen(args->rtfmsg)+1  +  4+4+4+strlen(rtfcap)+1;

	for (i = 0; i < 8; i++)
		ck[i] = (fu8_t)rand();

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+128+servdatalen)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0004, 0x0006, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0004, 0x0006, 0x0000, snacid);

	/* ICBM header */
	aim_im_puticbm(&fr->data, ck, 0x0002, args->destsn);

	/* TLV t(0005) - Encompasses everything below. */
	aimbs_put16(&fr->data, 0x0005);
	aimbs_put16(&fr->data, 2+8+16  +  2+2+2  +  2+2  +  2+2+servdatalen);

	aimbs_put16(&fr->data, 0x0000);
	aimbs_putraw(&fr->data, ck, 8);
	aim_putcap(&fr->data, AIM_CAPS_ICQSERVERRELAY);

	/* t(000a) l(0002) v(0001) */
	aimbs_put16(&fr->data, 0x000a);
	aimbs_put16(&fr->data, 0x0002);
	aimbs_put16(&fr->data, 0x0001);

	/* t(000f) l(0000) v() */
	aimbs_put16(&fr->data, 0x000f);
	aimbs_put16(&fr->data, 0x0000);

	/* Service Data TLV */
	aimbs_put16(&fr->data, 0x2711);
	aimbs_put16(&fr->data, servdatalen);

	aimbs_putle16(&fr->data, 11 + 16 /* 11 + (sizeof CLSID) */);
	aimbs_putle16(&fr->data, 9);
	aim_putcap(&fr->data, AIM_CAPS_EMPTY);
	aimbs_putle16(&fr->data, 0);
	aimbs_putle32(&fr->data, 0);
	aimbs_putle8(&fr->data, 0);
	aimbs_putle16(&fr->data, 0x03ea); /* trid1 */

	aimbs_putle16(&fr->data, 14);
	aimbs_putle16(&fr->data, 0x03eb); /* trid2 */
	aimbs_putle32(&fr->data, 0);
	aimbs_putle32(&fr->data, 0);
	aimbs_putle32(&fr->data, 0);

	aimbs_putle16(&fr->data, 0x0001);
	aimbs_putle32(&fr->data, 0);
	aimbs_putle16(&fr->data, strlen(args->rtfmsg)+1);
	aimbs_putraw(&fr->data, args->rtfmsg, strlen(args->rtfmsg)+1);

	aimbs_putle32(&fr->data, args->fgcolor);
	aimbs_putle32(&fr->data, args->bgcolor);
	aimbs_putle32(&fr->data, strlen(rtfcap)+1);
	aimbs_putraw(&fr->data, rtfcap, strlen(rtfcap)+1);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/**
 * Subtype 0x0006 - Send an "I want to directly connect to you" message
 *
 */
faim_export int aim_im_sendch2_odcrequest(aim_session_t *sess, fu8_t *cookie, const char *sn, const fu8_t *ip, fu16_t port)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	fu8_t ck[8];
	aim_tlvlist_t *tl = NULL, *itl = NULL;
	int hdrlen, i;
	fu8_t *hdr;
	aim_bstream_t hdrbs;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0004)))
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 256+strlen(sn))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0004, 0x0006, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0004, 0x0006, 0x0000, snacid);

	/*
	 * Generate a random message cookie
	 *
	 * This cookie needs to be alphanumeric and NULL-terminated to be
	 * TOC-compatible.
	 *
	 * XXX - have I mentioned these should be generated in msgcookie.c?
	 *
	 */
	for (i = 0; i < 7; i++)
	       	ck[i] = 0x30 + ((fu8_t) rand() % 10);
	ck[7] = '\0';

	if (cookie)
		memcpy(cookie, ck, 8);

	/* ICBM header */
	aim_im_puticbm(&fr->data, ck, 0x0002, sn);

	aim_tlvlist_add_noval(&tl, 0x0003);

	hdrlen = 2+8+16+6+8+6+4;
	hdr = malloc(hdrlen);
	aim_bstream_init(&hdrbs, hdr, hdrlen);

	aimbs_put16(&hdrbs, 0x0000);
	aimbs_putraw(&hdrbs, ck, 8);
	aim_putcap(&hdrbs, AIM_CAPS_DIRECTIM);

	aim_tlvlist_add_16(&itl, 0x000a, 0x0001);
	aim_tlvlist_add_raw(&itl, 0x0003, 4, ip);
	aim_tlvlist_add_16(&itl, 0x0005, port);
	aim_tlvlist_add_noval(&itl, 0x000f);

	aim_tlvlist_write(&hdrbs, &itl);

	aim_tlvlist_add_raw(&tl, 0x0005, aim_bstream_curpos(&hdrbs), hdr);

	aim_tlvlist_write(&fr->data, &tl);

	free(hdr);
	aim_tlvlist_free(&itl);
	aim_tlvlist_free(&tl);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/**
 * Subtype 0x0006 - Send an "I want to send you this file" message
 *
 */
faim_export int aim_im_sendch2_sendfile_ask(aim_session_t *sess, struct aim_oft_info *oft_info)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	aim_tlvlist_t *tl=NULL, *subtl=NULL;
	int i;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0004)) || !oft_info)
		return -EINVAL;

	/* XXX - Should be like "21CBF95" and null terminated */
	for (i = 0; i < 7; i++)
		oft_info->cookie[i] = 0x30 + ((fu8_t)rand() % 10);
	oft_info->cookie[7] = '\0';

	{ /* Create the subTLV chain */
		fu8_t *buf;
		int buflen;
		aim_bstream_t bs;

		aim_tlvlist_add_16(&subtl, 0x000a, 0x0001);
		aim_tlvlist_add_noval(&subtl, 0x000f);
/*		aim_tlvlist_add_raw(&subtl, 0x000e, 2, "en");
		aim_tlvlist_add_raw(&subtl, 0x000d, 8, "us-ascii");
		aim_tlvlist_add_raw(&subtl, 0x000c, 24, "Please accept this file."); */
		if (oft_info->clientip) {
			fu8_t ip[4];
			char *nexttoken;
			int i = 0;
			nexttoken = strtok(oft_info->clientip, ".");
			while (nexttoken && i<4) {
				ip[i] = atoi(nexttoken);
				nexttoken = strtok(NULL, ".");
				i++;
			}
			aim_tlvlist_add_raw(&subtl, 0x0003, 4, ip);
		}
		aim_tlvlist_add_16(&subtl, 0x0005, oft_info->port);

		/* TLV t(2711) */
		buflen = 2+2+4+strlen(oft_info->fh.name)+1;
		buf = malloc(buflen);
		aim_bstream_init(&bs, buf, buflen);
		aimbs_put16(&bs, (oft_info->fh.totfiles > 1) ? 0x0002 : 0x0001);
		aimbs_put16(&bs, oft_info->fh.totfiles);
		aimbs_put32(&bs, oft_info->fh.totsize);

		/* Filename - NULL terminated, for some odd reason */
		aimbs_putraw(&bs, oft_info->fh.name, strlen(oft_info->fh.name));
		aimbs_put8(&bs, 0x00);

		aim_tlvlist_add_raw(&subtl, 0x2711, bs.len, bs.data);
		free(buf);
	}

	{ /* Create the main TLV chain */
		fu8_t *buf;
		int buflen;
		aim_bstream_t bs;

		/* TLV t(0005) - Encompasses everything from above. Gee. */
		buflen = 2+8+16+aim_tlvlist_size(&subtl);
		buf = malloc(buflen);
		aim_bstream_init(&bs, buf, buflen);
		aimbs_put16(&bs, AIM_RENDEZVOUS_PROPOSE);
		aimbs_putraw(&bs, oft_info->cookie, 8);
		aim_putcap(&bs, AIM_CAPS_SENDFILE);
		aim_tlvlist_write(&bs, &subtl);
		aim_tlvlist_free(&subtl);
		aim_tlvlist_add_raw(&tl, 0x0005, bs.len, bs.data);
		free(buf);

		/* TLV t(0003) - Request an ack */
		aim_tlvlist_add_noval(&tl, 0x0003);
	}

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10 + 11+strlen(oft_info->sn) + aim_tlvlist_size(&tl))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0004, 0x0006, AIM_SNACFLAGS_DESTRUCTOR, oft_info->cookie, sizeof(oft_info->cookie));
	aim_putsnac(&fr->data, 0x0004, 0x0006, 0x0000, snacid);

	/* ICBM header */
	aim_im_puticbm(&fr->data, oft_info->cookie, 0x0002, oft_info->sn);

	/* All that crap from above (the 0x0005 TLV and the 0x0003 TLV) */
	aim_tlvlist_write(&fr->data, &tl);
	aim_tlvlist_free(&tl);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/**
 * Subtype 0x0006 - Send an "I will accept this file" message?
 *
 * @param rendid Capability type (AIM_CAPS_GETFILE or AIM_CAPS_SENDFILE)
 */
faim_export int aim_im_sendch2_sendfile_accept(aim_session_t *sess, struct aim_oft_info *oft_info)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0004)) || !oft_info)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10 + 11+strlen(oft_info->sn) + 4+2+8+16)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0004, 0x0006, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0004, 0x0006, 0x0000, snacid);

	/* ICBM header */
	aim_im_puticbm(&fr->data, oft_info->cookie, 0x0002, oft_info->sn);

	aimbs_put16(&fr->data, 0x0005);
	aimbs_put16(&fr->data, 0x001a);
	aimbs_put16(&fr->data, AIM_RENDEZVOUS_ACCEPT);
	aimbs_putraw(&fr->data, oft_info->cookie, 8);
	aim_putcap(&fr->data, AIM_CAPS_SENDFILE);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/**
 * Subtype 0x0006 - Send a "cancel this file transfer" message?
 *
 */
faim_export int aim_im_sendch2_sendfile_cancel(aim_session_t *sess, struct aim_oft_info *oft_info)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0004)) || !oft_info)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10 + 11+strlen(oft_info->sn) + 4+2+8+16)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0004, 0x0006, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0004, 0x0006, 0x0000, snacid);

	/* ICBM header */
	aim_im_puticbm(&fr->data, oft_info->cookie, 0x0002, oft_info->sn);

	aimbs_put16(&fr->data, 0x0005);
	aimbs_put16(&fr->data, 0x001a);
	aimbs_put16(&fr->data, AIM_RENDEZVOUS_CANCEL);
	aimbs_putraw(&fr->data, oft_info->cookie, 8);
	aim_putcap(&fr->data, AIM_CAPS_SENDFILE);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/**
 * Subtype 0x0006 - Request the status message of the given ICQ user.
 *
 * @param sess The oscar session.
 * @param sn The UIN of the user of whom you wish to request info.
 * @param type The type of info you wish to request.  This should be the current
 *        state of the user, as one of the AIM_ICQ_STATE_* defines.
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_im_sendch2_geticqaway(aim_session_t *sess, const char *sn, int type)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	int i;
	fu8_t ck[8];

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0004)) || !sn)
		return -EINVAL;

	for (i = 0; i < 8; i++)
		ck[i] = (fu8_t)rand();

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+8+2+1+strlen(sn) + 4+0x5e + 4)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0004, 0x0006, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0004, 0x0006, 0x0000, snacid);

	/* ICBM header */
	aim_im_puticbm(&fr->data, ck, 0x0002, sn);

	/* TLV t(0005) - Encompasses almost everything below. */
	aimbs_put16(&fr->data, 0x0005); /* T */
	aimbs_put16(&fr->data, 0x005e); /* L */
	{ /* V */
		aimbs_put16(&fr->data, 0x0000);

		/* Cookie */
		aimbs_putraw(&fr->data, ck, 8);

		/* Put the 16 byte server relay capability */
		aim_putcap(&fr->data, AIM_CAPS_ICQSERVERRELAY);

		/* TLV t(000a) */
		aimbs_put16(&fr->data, 0x000a);
		aimbs_put16(&fr->data, 0x0002);
		aimbs_put16(&fr->data, 0x0001);

		/* TLV t(000f) */
		aimbs_put16(&fr->data, 0x000f);
		aimbs_put16(&fr->data, 0x0000);

		/* TLV t(2711) */
		aimbs_put16(&fr->data, 0x2711);
		aimbs_put16(&fr->data, 0x0036);
		{ /* V */
			aimbs_putle16(&fr->data, 0x001b); /* L */
			aimbs_putle16(&fr->data, 0x0008); /* XXX - Protocol version */
			aim_putcap(&fr->data, AIM_CAPS_EMPTY);
			aimbs_putle16(&fr->data, 0x0000); /* Unknown */
			aimbs_putle16(&fr->data, 0x0003); /* Client features? */
			aimbs_putle16(&fr->data, 0x0000); /* Unknown */
			aimbs_putle8(&fr->data, 0x00); /* Unkizown */
			aimbs_putle16(&fr->data, 0xffff); /* Sequence number?  XXX - This should decrement by 1 with each request */

			aimbs_putle16(&fr->data, 0x000e); /* L */
			aimbs_putle16(&fr->data, 0xffff); /* Sequence number?  XXX - This should decrement by 1 with each request */
			aimbs_putle32(&fr->data, 0x00000000); /* Unknown */
			aimbs_putle32(&fr->data, 0x00000000); /* Unknown */
			aimbs_putle32(&fr->data, 0x00000000); /* Unknown */

			/* The type of status message being requested */
			if (type & AIM_ICQ_STATE_CHAT)
				aimbs_putle16(&fr->data, 0x03ec);
			else if(type & AIM_ICQ_STATE_DND)
				aimbs_putle16(&fr->data, 0x03eb);
			else if(type & AIM_ICQ_STATE_OUT)
				aimbs_putle16(&fr->data, 0x03ea);
			else if(type & AIM_ICQ_STATE_BUSY)
				aimbs_putle16(&fr->data, 0x03e9);
			else if(type & AIM_ICQ_STATE_AWAY)
				aimbs_putle16(&fr->data, 0x03e8);

			aimbs_putle16(&fr->data, 0x0000); /* Status? */
			aimbs_putle16(&fr->data, 0x0001); /* Priority of this message? */
			aimbs_putle16(&fr->data, 0x0001); /* L */
			aimbs_putle8(&fr->data, 0x00); /* String of length L */
		} /* End TLV t(2711) */
	} /* End TLV t(0005) */

	/* TLV t(0003) */
	aimbs_put16(&fr->data, 0x0003);
	aimbs_put16(&fr->data, 0x0000);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/**
 * Subtype 0x0006 - Send an ICQ-esque ICBM.
 *
 * This can be used to send an ICQ authorization reply (deny or grant).  It is the "old way."
 * The new way is to use SSI.  I like the new way a lot better.  This seems like such a hack,
 * mostly because it's in network byte order.  Figuring this stuff out sometimes takes a while,
 * but thats ok, because it gives me time to try to figure out what kind of drugs the AOL people
 * were taking when they merged the two protocols.
 *
 * @param sn The destination screen name.
 * @param type The type of message.  0x0007 for authorization denied.  0x0008 for authorization granted.
 * @param message The message you want to send, it should be null terminated.
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_im_sendch4(aim_session_t *sess, char *sn, fu16_t type, fu8_t *message)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	int i;
	char ck[8];

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0002)))
		return -EINVAL;

	if (!sn || !type || !message)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+8+3+strlen(sn)+12+strlen(message)+1+4)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0004, 0x0006, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0004, 0x0006, 0x0000, snacid);

	/* Cookie */
	for (i=0; i<8; i++)
		ck[i] = (fu8_t)rand();

	/* ICBM header */
	aim_im_puticbm(&fr->data, ck, 0x0004, sn);

	/*
	 * TLV t(0005)
	 *
	 * ICQ data (the UIN and the message).
	 */
	aimbs_put16(&fr->data, 0x0005);
	aimbs_put16(&fr->data, 4 + 2+2+strlen(message)+1);

	/*
	 * Your UIN
	 */
	aimbs_putle32(&fr->data, atoi(sess->sn));

	/*
	 * TLV t(type) l(strlen(message)+1) v(message+NULL)
	 */
	aimbs_putle16(&fr->data, type);
	aimbs_putle16(&fr->data, strlen(message)+1);
	aimbs_putraw(&fr->data, message, strlen(message)+1);

	/*
	 * TLV t(0006) l(0000) v()
	 */
	aimbs_put16(&fr->data, 0x0006);
	aimbs_put16(&fr->data, 0x0000);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * XXX - I don't see when this would ever get called...
 */
static int outgoingim(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int i, ret = 0;
	aim_rxcallback_t userfunc;
	fu8_t cookie[8];
	fu16_t channel;
	aim_tlvlist_t *tlvlist;
	char *sn;
	int snlen;
	fu16_t icbmflags = 0;
	fu8_t flag1 = 0, flag2 = 0;
	fu8_t *msg = NULL;
	aim_tlv_t *msgblock;

	/* ICBM Cookie. */
	for (i = 0; i < 8; i++)
		cookie[i] = aimbs_get8(bs);

	/* Channel ID */
	channel = aimbs_get16(bs);

	if (channel != 0x01) {
		faimdprintf(sess, 0, "icbm: ICBM received on unsupported channel.  Ignoring. (chan = %04x)\n", channel);
		return 0;
	}

	snlen = aimbs_get8(bs);
	sn = aimbs_getstr(bs, snlen);

	tlvlist = aim_tlvlist_read(bs);

	if (aim_tlv_gettlv(tlvlist, 0x0003, 1))
		icbmflags |= AIM_IMFLAGS_ACK;
	if (aim_tlv_gettlv(tlvlist, 0x0004, 1))
		icbmflags |= AIM_IMFLAGS_AWAY;

	if ((msgblock = aim_tlv_gettlv(tlvlist, 0x0002, 1))) {
		aim_bstream_t mbs;
		int featurelen, msglen;

		aim_bstream_init(&mbs, msgblock->value, msgblock->length);

		aimbs_get8(&mbs);
		aimbs_get8(&mbs);
		for (featurelen = aimbs_get16(&mbs); featurelen; featurelen--)
			aimbs_get8(&mbs);
		aimbs_get8(&mbs);
		aimbs_get8(&mbs);

		msglen = aimbs_get16(&mbs) - 4; /* final block length */

		flag1 = aimbs_get16(&mbs);
		flag2 = aimbs_get16(&mbs);

		msg = aimbs_getstr(&mbs, msglen);
	}

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, channel, sn, msg, icbmflags, flag1, flag2);

	free(sn);
	aim_tlvlist_free(&tlvlist);

	return ret;
}

/*
 * Ahh, the joys of nearly ridiculous over-engineering.
 *
 * Not only do AIM ICBM's support multiple channels.  Not only do they
 * support multiple character sets.  But they support multiple character
 * sets / encodings within the same ICBM.
 *
 * These multipart messages allow for complex space savings techniques, which
 * seem utterly unnecessary by today's standards.  In fact, there is only
 * one client still in popular use that still uses this method: AOL for the
 * Macintosh, Version 5.0.  Obscure, yes, I know.
 *
 * In modern (non-"legacy") clients, if the user tries to send a character
 * that is not ISO-8859-1 or ASCII, the client will send the entire message
 * as UNICODE, meaning that every character in the message will occupy the
 * full 16 bit UNICODE field, even if the high order byte would be zero.
 * Multipart messages prevent this wasted space by allowing the client to
 * only send the characters in UNICODE that need to be sent that way, and
 * the rest of the message can be sent in whatever the native character
 * set is (probably ASCII).
 *
 * An important note is that sections will be displayed in the order that
 * they appear in the ICBM.  There is no facility for merging or rearranging
 * sections at run time.  So if you have, say, ASCII then UNICODE then ASCII,
 * you must supply two ASCII sections with a UNICODE in the middle, and incur
 * the associated overhead.
 *
 * Normally I would have laughed and given a firm 'no' to supporting this
 * seldom-used feature, but something is attracting me to it.  In the future,
 * it may be possible to abuse this to send mixed-media messages to other
 * open source clients (like encryption or something) -- see faimtest for
 * examples of how to do this.
 *
 * I would definitly recommend avoiding this feature unless you really
 * know what you are doing, and/or you have something neat to do with it.
 *
 */
faim_export int aim_mpmsg_init(aim_session_t *sess, aim_mpmsg_t *mpm)
{

	memset(mpm, 0, sizeof(aim_mpmsg_t));

	return 0;
}

static int mpmsg_addsection(aim_session_t *sess, aim_mpmsg_t *mpm, fu16_t charset, fu16_t charsubset, fu8_t *data, fu16_t datalen)
{
	aim_mpmsg_section_t *sec;

	if (!(sec = malloc(sizeof(aim_mpmsg_section_t))))
		return -1;

	sec->charset = charset;
	sec->charsubset = charsubset;
	sec->data = data;
	sec->datalen = datalen;
	sec->next = NULL;

	if (!mpm->parts)
		mpm->parts = sec;
	else {
		aim_mpmsg_section_t *cur;

		for (cur = mpm->parts; cur->next; cur = cur->next)
			;
		cur->next = sec;
	}

	mpm->numparts++;

	return 0;
}

faim_export int aim_mpmsg_addraw(aim_session_t *sess, aim_mpmsg_t *mpm, fu16_t charset, fu16_t charsubset, const fu8_t *data, fu16_t datalen)
{
	fu8_t *dup;

	if (!(dup = malloc(datalen)))
		return -1;
	memcpy(dup, data, datalen);

	if (mpmsg_addsection(sess, mpm, charset, charsubset, dup, datalen) == -1) {
		free(dup);
		return -1;
	}

	return 0;
}

/* XXX - should provide a way of saying ISO-8859-1 specifically */
faim_export int aim_mpmsg_addascii(aim_session_t *sess, aim_mpmsg_t *mpm, const char *ascii)
{
	fu8_t *dup;

	if (!(dup = strdup(ascii)))
		return -1;

	if (mpmsg_addsection(sess, mpm, 0x0000, 0x0000, dup, strlen(ascii)) == -1) {
		free(dup);
		return -1;
	}

	return 0;
}

faim_export int aim_mpmsg_addunicode(aim_session_t *sess, aim_mpmsg_t *mpm, const fu16_t *unicode, fu16_t unicodelen)
{
	fu8_t *buf;
	aim_bstream_t bs;
	int i;

	if (!(buf = malloc(unicodelen * 2)))
		return -1;

	aim_bstream_init(&bs, buf, unicodelen * 2);

	/* We assume unicode is in /host/ byte order -- convert to network */
	for (i = 0; i < unicodelen; i++)
		aimbs_put16(&bs, unicode[i]);

	if (mpmsg_addsection(sess, mpm, 0x0002, 0x0000, buf, aim_bstream_curpos(&bs)) == -1) {
		free(buf);
		return -1;
	}

	return 0;
}

faim_export void aim_mpmsg_free(aim_session_t *sess, aim_mpmsg_t *mpm)
{
	aim_mpmsg_section_t *cur;

	for (cur = mpm->parts; cur; ) {
		aim_mpmsg_section_t *tmp;

		tmp = cur->next;
		free(cur->data);
		free(cur);
		cur = tmp;
	}

	mpm->numparts = 0;
	mpm->parts = NULL;

	return;
}

/*
 * Start by building the multipart structures, then pick the first
 * human-readable section and stuff it into args->msg so no one gets
 * suspicious.
 *
 */
static int incomingim_ch1_parsemsgs(aim_session_t *sess, fu8_t *data, int len, struct aim_incomingim_ch1_args *args)
{
	static const fu16_t charsetpri[] = {
		0x0000, /* ASCII first */
		0x0003, /* then ISO-8859-1 */
		0x0002, /* UNICODE as last resort */
	};
	static const int charsetpricount = 3;
	int i;
	aim_bstream_t mbs;
	aim_mpmsg_section_t *sec;

	aim_bstream_init(&mbs, data, len);

	while (aim_bstream_empty(&mbs)) {
		fu16_t msglen, flag1, flag2;
		fu8_t *msgbuf;

		aimbs_get8(&mbs); /* 01 */
		aimbs_get8(&mbs); /* 01 */

		/* Message string length, including character set info. */
		msglen = aimbs_get16(&mbs);

		/* Character set info */
		flag1 = aimbs_get16(&mbs);
		flag2 = aimbs_get16(&mbs);

		/* Message. */
		msglen -= 4;

		/*
		 * For now, we don't care what the encoding is.  Just copy
		 * it into a multipart struct and deal with it later. However,
		 * always pad the ending with a NULL.  This makes it easier
		 * to treat ASCII sections as strings.  It won't matter for
		 * UNICODE or binary data, as you should never read past
		 * the specified data length, which will not include the pad.
		 *
		 * XXX - There's an API bug here.  For sending, the UNICODE is
		 * given in host byte order (aim_mpmsg_addunicode), but here
		 * the received messages are given in network byte order.
		 *
		 */
		msgbuf = aimbs_getstr(&mbs, msglen);
		mpmsg_addsection(sess, &args->mpmsg, flag1, flag2, msgbuf, msglen);

	} /* while */

	args->icbmflags |= AIM_IMFLAGS_MULTIPART; /* always set */

	/*
	 * Clients that support multiparts should never use args->msg, as it
	 * will point to an arbitrary section.
	 *
	 * Here, we attempt to provide clients that do not support multipart
	 * messages with something to look at -- hopefully a human-readable
	 * string.  But, failing that, a UNICODE message, or nothing at all.
	 *
	 * Which means that even if args->msg is NULL, it does not mean the
	 * message was blank.
	 *
	 */
	for (i = 0; i < charsetpricount; i++) {
		for (sec = args->mpmsg.parts; sec; sec = sec->next) {

			if (sec->charset != charsetpri[i])
				continue;

			/* Great. We found one.  Fill it in. */
			args->charset = sec->charset;
			args->charsubset = sec->charsubset;

			/* Set up the simple flags */
			if (args->charset == 0x0000)
				; /* ASCII */
			else if (args->charset == 0x0002)
				args->icbmflags |= AIM_IMFLAGS_UNICODE;
			else if (args->charset == 0x0003)
				args->icbmflags |= AIM_IMFLAGS_ISO_8859_1;
			else if (args->charset == 0xffff)
				; /* no encoding (yeep!) */

			if (args->charsubset == 0x0000)
				; /* standard subencoding? */
			else if (args->charsubset == 0x000b)
				args->icbmflags |= AIM_IMFLAGS_SUBENC_MACINTOSH;
			else if (args->charsubset == 0xffff)
				; /* no subencoding */

			args->msg = sec->data;
			args->msglen = sec->datalen;

			return 0;
		}
	}

	/* No human-readable sections found.  Oh well. */
	args->charset = args->charsubset = 0xffff;
	args->msg = NULL;
	args->msglen = 0;

	return 0;
}

static int incomingim_ch1(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, fu16_t channel, aim_userinfo_t *userinfo, aim_bstream_t *bs, fu8_t *cookie)
{
	fu16_t type, length;
	aim_rxcallback_t userfunc;
	int ret = 0;
	struct aim_incomingim_ch1_args args;
	int endpos;

	memset(&args, 0, sizeof(args));

	aim_mpmsg_init(sess, &args.mpmsg);

	/*
	 * This used to be done using tlvchains.  For performance reasons,
	 * I've changed it to process the TLVs in-place.  This avoids lots
	 * of per-IM memory allocations.
	 */
	while (aim_bstream_empty(bs)) {

		type = aimbs_get16(bs);
		length = aimbs_get16(bs);

		endpos = aim_bstream_curpos(bs) + length;

		if (type == 0x0002) { /* Message Block */

			/*
			 * This TLV consists of the following:
			 *   - 0501 -- Unknown
			 *   - Features: Don't know how to interpret these
			 *   - 0101 -- Unknown
			 *   - Message
			 *
			 */

			aimbs_get8(bs); /* 05 */
			aimbs_get8(bs); /* 01 */

			args.featureslen = aimbs_get16(bs);
			/* XXX XXX this is all evil! */
			args.features = bs->data + bs->offset;
			aim_bstream_advance(bs, args.featureslen);
			args.icbmflags |= AIM_IMFLAGS_CUSTOMFEATURES;

			/*
			 * The rest of the TLV contains one or more message
			 * blocks...
			 */
			incomingim_ch1_parsemsgs(sess, bs->data + bs->offset /* XXX evil!!! */, length - 2 - 2 - args.featureslen, &args);

		} else if (type == 0x0003) { /* Server Ack Requested */

			args.icbmflags |= AIM_IMFLAGS_ACK;

		} else if (type == 0x0004) { /* Message is Auto Response */

			args.icbmflags |= AIM_IMFLAGS_AWAY;

		} else if (type == 0x0006) { /* Message was received offline. */

			/* XXX - not sure if this actually gets sent. */
			args.icbmflags |= AIM_IMFLAGS_OFFLINE;

		} else if (type == 0x0008) { /* I-HAVE-A-REALLY-PURTY-ICON Flag */

			args.iconlen = aimbs_get32(bs);
			aimbs_get16(bs); /* 0x0001 */
			args.iconsum = aimbs_get16(bs);
			args.iconstamp = aimbs_get32(bs);

			/*
			 * This looks to be a client bug.  MacAIM 4.3 will
			 * send this tag, but with all zero values, in the
			 * first message of a conversation. This makes no
			 * sense whatsoever, so I'm going to say its a bug.
			 *
			 * You really shouldn't advertise a zero-length icon
			 * anyway.
			 *
			 */
			if (args.iconlen)
				args.icbmflags |= AIM_IMFLAGS_HASICON;

		} else if (type == 0x0009) {

			args.icbmflags |= AIM_IMFLAGS_BUDDYREQ;

		} else if (type == 0x000b) { /* Non-direct connect typing notification */

			args.icbmflags |= AIM_IMFLAGS_TYPINGNOT;

		} else if (type == 0x0017) {

			args.extdatalen = length;
			args.extdata = aimbs_getraw(bs, args.extdatalen);

		} else {
			faimdprintf(sess, 0, "incomingim_ch1: unknown TLV 0x%04x (len %d)\n", type, length);
		}

		/*
		 * This is here to protect ourselves from ourselves.  That
		 * is, if something above doesn't completly parse its value
		 * section, or, worse, overparses it, this will set the
		 * stream where it needs to be in order to land on the next
		 * TLV when the loop continues.
		 *
		 */
		aim_bstream_setpos(bs, endpos);
	}


	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, channel, userinfo, &args);

	aim_mpmsg_free(sess, &args.mpmsg);
	free(args.extdata);

	return ret;
}

static void incomingim_ch2_buddylist(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_userinfo_t *userinfo, struct aim_incomingim_ch2_args *args, aim_bstream_t *servdata)
{

	/*
	 * This goes like this...
	 *
	 *   group name length
	 *   group name
	 *     num of buddies in group
	 *     buddy name length
	 *     buddy name
	 *     buddy name length
	 *     buddy name
	 *     ...
	 *   group name length
	 *   group name
	 *     num of buddies in group
	 *     buddy name length
	 *     buddy name
	 *     ...
	 *   ...
	 */
	while (servdata && aim_bstream_empty(servdata)) {
		fu16_t gnlen, numb;
		int i;
		char *gn;

		gnlen = aimbs_get16(servdata);
		gn = aimbs_getstr(servdata, gnlen);
		numb = aimbs_get16(servdata);

		for (i = 0; i < numb; i++) {
			fu16_t bnlen;
			char *bn;

			bnlen = aimbs_get16(servdata);
			bn = aimbs_getstr(servdata, bnlen);

			faimdprintf(sess, 0, "got a buddy list from %s: group %s, buddy %s\n", userinfo->sn, gn, bn);

			free(bn);
		}

		free(gn);
	}

	return;
}

static void incomingim_ch2_buddyicon_free(aim_session_t *sess, struct aim_incomingim_ch2_args *args)
{

	free(args->info.icon.icon);

	return;
}

static void incomingim_ch2_buddyicon(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_userinfo_t *userinfo, struct aim_incomingim_ch2_args *args, aim_bstream_t *servdata)
{

	if (servdata) {
		args->info.icon.checksum = aimbs_get32(servdata);
		args->info.icon.length = aimbs_get32(servdata);
		args->info.icon.timestamp = aimbs_get32(servdata);
		args->info.icon.icon = aimbs_getraw(servdata, args->info.icon.length);
	}

	args->destructor = (void *)incomingim_ch2_buddyicon_free;

	return;
}

static void incomingim_ch2_chat_free(aim_session_t *sess, struct aim_incomingim_ch2_args *args)
{

	/* XXX - aim_chat_roominfo_free() */
	free(args->info.chat.roominfo.name);

	return;
}

static void incomingim_ch2_chat(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_userinfo_t *userinfo, struct aim_incomingim_ch2_args *args, aim_bstream_t *servdata)
{

	/*
	 * Chat room info.
	 */
	if (servdata)
		aim_chat_readroominfo(servdata, &args->info.chat.roominfo);

	args->destructor = (void *)incomingim_ch2_chat_free;

	return;
}

static void incomingim_ch2_icqserverrelay_free(aim_session_t *sess, struct aim_incomingim_ch2_args *args)
{

	free((char *)args->info.rtfmsg.rtfmsg);

	return;
}

/*
 * The relationship between AIM_CAPS_ICQSERVERRELAY and AIM_CAPS_ICQRTF is
 * kind of odd. This sends the client ICQRTF since that is all that I've seen
 * SERVERRELAY used for.
 *
 * Note that this is all little-endian.  Cringe.
 *
 */
static void incomingim_ch2_icqserverrelay(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_userinfo_t *userinfo, struct aim_incomingim_ch2_args *args, aim_bstream_t *servdata)
{
	fu16_t hdrlen, anslen, msglen;
	fu16_t msgtype;

	hdrlen = aimbs_getle16(servdata);
	aim_bstream_advance(servdata, hdrlen);

	hdrlen = aimbs_getle16(servdata);
	aim_bstream_advance(servdata, hdrlen);

	msgtype = aimbs_getle16(servdata);

	anslen = aimbs_getle32(servdata);
	aim_bstream_advance(servdata, anslen);

	msglen = aimbs_getle16(servdata);
	args->info.rtfmsg.rtfmsg = aimbs_getstr(servdata, msglen);

	args->info.rtfmsg.fgcolor = aimbs_getle32(servdata);
	args->info.rtfmsg.bgcolor = aimbs_getle32(servdata);

	hdrlen = aimbs_getle32(servdata);
	aim_bstream_advance(servdata, hdrlen);

	/* XXX - This is such a hack. */
	args->reqclass = AIM_CAPS_ICQRTF;

	args->destructor = (void *)incomingim_ch2_icqserverrelay_free;

	return;
}

static void incomingim_ch2_sendfile_free(aim_session_t *sess, struct aim_incomingim_ch2_args *args)
{
	free(args->info.sendfile.filename);
}

static void incomingim_ch2_sendfile(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_userinfo_t *userinfo, struct aim_incomingim_ch2_args *args, aim_bstream_t *servdata)
{

	args->destructor = (void *)incomingim_ch2_sendfile_free;

	/* Maybe there is a better way to tell what kind of sendfile
	 * this is?  Maybe TLV t(000a)? */
	if (servdata) { /* Someone is sending us a file */
		int flen;

		/* subtype is one of AIM_OFT_SUBTYPE_* */
		args->info.sendfile.subtype = aimbs_get16(servdata);
		args->info.sendfile.totfiles = aimbs_get16(servdata);
		args->info.sendfile.totsize = aimbs_get32(servdata);

		/*
		 * I hope to God I'm right when I guess that there is a
		 * 32 char max filename length for single files.  I think
		 * OFT tends to do that.  Gotta love inconsistency.  I saw
		 * a 26 byte filename?
		 */
		/* AAA - create an aimbs_getnullstr function (don't anymore)(maybe) */
		/* Use an inelegant way of getting the null-terminated filename,
		 * since there's no easy bstream routine. */
		for (flen = 0; aimbs_get8(servdata); flen++);
		aim_bstream_advance(servdata, -flen -1);
		args->info.sendfile.filename = aimbs_getstr(servdata, flen);

		/* There is sometimes more after the null-terminated filename,
		 * but I'm unsure of its format. */
		/* I don't believe him. */
	}

	return;
}

typedef void (*ch2_args_destructor_t)(aim_session_t *sess, struct aim_incomingim_ch2_args *args);

static int incomingim_ch2(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, fu16_t channel, aim_userinfo_t *userinfo, aim_tlvlist_t *tlvlist, fu8_t *cookie)
{
	aim_rxcallback_t userfunc;
	aim_tlv_t *block1, *servdatatlv;
	aim_tlvlist_t *list2;
	struct aim_incomingim_ch2_args args;
	aim_bstream_t bbs, sdbs, *sdbsptr = NULL;
	fu8_t *cookie2;
	int ret = 0;

	char proxyip[30] = {""};
	char clientip[30] = {""};
	char verifiedip[30] = {""};

	memset(&args, 0, sizeof(args));

	/*
	 * There's another block of TLVs embedded in the type 5 here.
	 */
	block1 = aim_tlv_gettlv(tlvlist, 0x0005, 1);
	aim_bstream_init(&bbs, block1->value, block1->length);

	/*
	 * First two bytes represent the status of the connection.
	 *
	 * 0 is a request, 1 is a cancel, 2 is an accept
	 */
	args.status = aimbs_get16(&bbs);

	/*
	 * Next comes the cookie.  Should match the ICBM cookie.
	 */
	cookie2 = aimbs_getraw(&bbs, 8);
	if (memcmp(cookie, cookie2, 8) != 0)
		faimdprintf(sess, 0, "rend: warning cookies don't match!\n");
	memcpy(args.cookie, cookie2, 8);
	free(cookie2);

	/*
	 * The next 16bytes are a capability block so we can
	 * identify what type of rendezvous this is.
	 */
	args.reqclass = aim_locate_getcaps(sess, &bbs, 0x10);

	/*
	 * What follows may be TLVs or nothing, depending on the
	 * purpose of the message.
	 *
	 * Ack packets for instance have nothing more to them.
	 */
	list2 = aim_tlvlist_read(&bbs);

	/*
	 * IP address to proxy the file transfer through.
	 *
	 * XXX - I don't like this.  Maybe just read in an int?  Or inet_ntoa...
	 */
	if (aim_tlv_gettlv(list2, 0x0002, 1)) {
		aim_tlv_t *iptlv;

		iptlv = aim_tlv_gettlv(list2, 0x0002, 1);
		if (iptlv->length == 4)
			snprintf(proxyip, sizeof(proxyip), "%hhu.%hhu.%hhu.%hhu",
				iptlv->value[0], iptlv->value[1],
				iptlv->value[2], iptlv->value[3]);
	}

	/*
	 * IP address from the perspective of the client.
	 */
	if (aim_tlv_gettlv(list2, 0x0003, 1)) {
		aim_tlv_t *iptlv;

		iptlv = aim_tlv_gettlv(list2, 0x0003, 1);
		if (iptlv->length == 4)
			snprintf(clientip, sizeof(clientip), "%hhu.%hhu.%hhu.%hhu",
				iptlv->value[0], iptlv->value[1],
				iptlv->value[2], iptlv->value[3]);
	}

	/*
	 * Verified IP address (from the perspective of Oscar).
	 *
	 * This is added by the server.
	 */
	if (aim_tlv_gettlv(list2, 0x0004, 1)) {
		aim_tlv_t *iptlv;

		iptlv = aim_tlv_gettlv(list2, 0x0004, 1);
		if (iptlv->length == 4)
			snprintf(verifiedip, sizeof(verifiedip), "%hhu.%hhu.%hhu.%hhu",
				iptlv->value[0], iptlv->value[1],
				iptlv->value[2], iptlv->value[3]);
	}

	/*
	 * Port number for something.
	 */
	if (aim_tlv_gettlv(list2, 0x0005, 1))
		args.port = aim_tlv_get16(list2, 0x0005, 1);

	/*
	 * Something to do with ft -- two bytes
	 * 0x0001 - "I want to send you this file"
	 * 0x0002 - "I will accept this file from you"
	 */
	if (aim_tlv_gettlv(list2, 0x000a, 1))
		;

	/*
	 * Error code.
	 */
	if (aim_tlv_gettlv(list2, 0x000b, 1))
		args.errorcode = aim_tlv_get16(list2, 0x000b, 1);

	/*
	 * Invitation message / chat description.
	 */
	if (aim_tlv_gettlv(list2, 0x000c, 1))
		args.msg = aim_tlv_getstr(list2, 0x000c, 1);

	/*
	 * Character set.
	 */
	if (aim_tlv_gettlv(list2, 0x000d, 1))
		args.encoding = aim_tlv_getstr(list2, 0x000d, 1);

	/*
	 * Language.
	 */
	if (aim_tlv_gettlv(list2, 0x000e, 1))
		args.language = aim_tlv_getstr(list2, 0x000e, 1);

	/*
	 * Unknown -- no value
	 *
	 * Maybe means we should connect directly to transfer the file?
	 */
	if (aim_tlv_gettlv(list2, 0x000f, 1))
		;

	/*
	 * Unknown -- no value
	 *
	 * Maybe means we should proxy the file transfer through an AIM server?
	 */
	if (aim_tlv_gettlv(list2, 0x0010, 1))
		;

	if (strlen(proxyip))
		args.proxyip = (char *)proxyip;
	if (strlen(clientip))
		args.clientip = (char *)clientip;
	if (strlen(verifiedip))
		args.verifiedip = (char *)verifiedip;

	/*
	 * This must be present in PROPOSALs, but will probably not
	 * exist in CANCELs and ACCEPTs.
	 *
	 * Service Data blocks are module-specific in format.
	 */
	if ((servdatatlv = aim_tlv_gettlv(list2, 0x2711 /* 10001 */, 1))) {

		aim_bstream_init(&sdbs, servdatatlv->value, servdatatlv->length);
		sdbsptr = &sdbs;
	}

	/*
	 * The rest of the handling depends on what type it is.
	 *
	 * Not all of them have special handling (yet).
	 */
	if (args.reqclass & AIM_CAPS_BUDDYICON)
		incomingim_ch2_buddyicon(sess, mod, rx, snac, userinfo, &args, sdbsptr);
	else if (args.reqclass & AIM_CAPS_SENDBUDDYLIST)
		incomingim_ch2_buddylist(sess, mod, rx, snac, userinfo, &args, sdbsptr);
	else if (args.reqclass & AIM_CAPS_CHAT)
		incomingim_ch2_chat(sess, mod, rx, snac, userinfo, &args, sdbsptr);
	else if (args.reqclass & AIM_CAPS_ICQSERVERRELAY)
		incomingim_ch2_icqserverrelay(sess, mod, rx, snac, userinfo, &args, sdbsptr);
	else if (args.reqclass & AIM_CAPS_SENDFILE)
		incomingim_ch2_sendfile(sess, mod, rx, snac, userinfo, &args, sdbsptr);


	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, channel, userinfo, &args);


	if (args.destructor)
		((ch2_args_destructor_t)args.destructor)(sess, &args);

	free((char *)args.msg);
	free((char *)args.encoding);
	free((char *)args.language);

	aim_tlvlist_free(&list2);

	return ret;
}

static int incomingim_ch4(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, fu16_t channel, aim_userinfo_t *userinfo, aim_tlvlist_t *tlvlist, fu8_t *cookie)
{
	aim_bstream_t meat;
	aim_rxcallback_t userfunc;
	aim_tlv_t *block;
	struct aim_incomingim_ch4_args args;
	int ret = 0;

	/*
	 * Make a bstream for the meaty part.  Yum.  Meat.
	 */
	if (!(block = aim_tlv_gettlv(tlvlist, 0x0005, 1)))
		return -1;
	aim_bstream_init(&meat, block->value, block->length);

	args.uin = aimbs_getle32(&meat);
	args.type = aimbs_getle8(&meat);
	args.flags = aimbs_getle8(&meat);
	args.msglen = aimbs_getle16(&meat);
	args.msg = aimbs_getraw(&meat, args.msglen);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, channel, userinfo, &args);

	free(args.msg);

	return ret;
}

/*
 * Subtype 0x0007
 *
 * It can easily be said that parsing ICBMs is THE single
 * most difficult thing to do in the in AIM protocol.  In
 * fact, I think I just did say that.
 *
 * Below is the best damned solution I've come up with
 * over the past sixteen months of battling with it. This
 * can parse both away and normal messages from every client
 * I have access to.  Its not fast, its not clean.  But it works.
 *
 */
static int incomingim(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int i, ret = 0;
	fu8_t cookie[8];
	fu16_t channel;
	aim_userinfo_t userinfo;

	memset(&userinfo, 0x00, sizeof(aim_userinfo_t));

	/*
	 * Read ICBM Cookie.  And throw away.
	 */
	for (i = 0; i < 8; i++)
		cookie[i] = aimbs_get8(bs);

	/*
	 * Channel ID.
	 *
	 * Channel 0x0001 is the message channel.  It is
	 * used to send basic ICBMs.
	 *
	 * Channel 0x0002 is the Rendevous channel, which
	 * is where Chat Invitiations and various client-client
	 * connection negotiations come from.
	 *
	 * Channel 0x0003 is used for chat messages.
	 *
	 * Channel 0x0004 is used for ICQ authorization, or
	 * possibly any system notice.
	 *
	 */
	channel = aimbs_get16(bs);

	/*
	 * Extract the standard user info block.
	 *
	 * Note that although this contains TLVs that appear contiguous
	 * with the TLVs read below, they are two different pieces.  The
	 * userinfo block contains the number of TLVs that contain user
	 * information, the rest are not even though there is no seperation.
	 * You can start reading the message TLVs after aim_info_extract()
	 * parses out the standard userinfo block.
	 *
	 * That also means that TLV types can be duplicated between the
	 * userinfo block and the rest of the message, however there should
	 * never be two TLVs of the same type in one block.
	 *
	 */
	aim_info_extract(sess, bs, &userinfo);

	/*
	 * From here on, its depends on what channel we're on.
	 *
	 * Technically all channels have a TLV list have this, however,
	 * for the common channel 1 case, in-place parsing is used for
	 * performance reasons (less memory allocation).
	 */
	if (channel == 1) {

		ret = incomingim_ch1(sess, mod, rx, snac, channel, &userinfo, bs, cookie);

	} else if (channel == 2) {
		aim_tlvlist_t *tlvlist;

		/*
		 * Read block of TLVs (not including the userinfo data).  All
		 * further data is derived from what is parsed here.
		 */
		tlvlist = aim_tlvlist_read(bs);

		ret = incomingim_ch2(sess, mod, rx, snac, channel, &userinfo, tlvlist, cookie);

		aim_tlvlist_free(&tlvlist);

	} else if (channel == 4) {
		aim_tlvlist_t *tlvlist;

		tlvlist = aim_tlvlist_read(bs);
		ret = incomingim_ch4(sess, mod, rx, snac, channel, &userinfo, tlvlist, cookie);
		aim_tlvlist_free(&tlvlist);

	} else {
		faimdprintf(sess, 0, "icbm: ICBM received on an unsupported channel.  Ignoring.  (chan = %04x)\n", channel);
	}

	aim_info_free(&userinfo);

	return ret;
}

/*
 * Subtype 0x0008 - Send a warning to sn.
 *
 * Flags:
 *  AIM_WARN_ANON  Send as an anonymous (doesn't count as much)
 *
 * returns -1 on error (couldn't alloc packet), 0 on success.
 *
 */
faim_export int aim_im_warn(aim_session_t *sess, aim_conn_t *conn, const char *sn, fu32_t flags)
{
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sess || !conn || !sn)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, strlen(sn)+13)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0004, 0x0008, 0x0000, sn, strlen(sn)+1);
	aim_putsnac(&fr->data, 0x0004, 0x0008, 0x0000, snacid);

	aimbs_put16(&fr->data, (flags & AIM_WARN_ANON) ? 0x0001 : 0x0000);
	aimbs_put8(&fr->data, strlen(sn));
	aimbs_putraw(&fr->data, sn, strlen(sn));

	aim_tx_enqueue(sess, fr);

	return 0;
}

/* Subtype 0x000a */
static int missedcall(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	fu16_t channel, nummissed, reason;
	aim_userinfo_t userinfo;

	while (aim_bstream_empty(bs)) {

		channel = aimbs_get16(bs);
		aim_info_extract(sess, bs, &userinfo);
		nummissed = aimbs_get16(bs);
		reason = aimbs_get16(bs);

		if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
			 ret = userfunc(sess, rx, channel, &userinfo, nummissed, reason);

		aim_info_free(&userinfo);
	}

	return ret;
}

/*
 * Subtype 0x000b
 *
 * Possible codes:
 *    AIM_TRANSFER_DENY_NOTSUPPORTED -- "client does not support"
 *    AIM_TRANSFER_DENY_DECLINE -- "client has declined transfer"
 *    AIM_TRANSFER_DENY_NOTACCEPTING -- "client is not accepting transfers"
 *
 */
faim_export int aim_im_denytransfer(aim_session_t *sess, const char *sender, const char *cookie, fu16_t code)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	aim_tlvlist_t *tl = NULL;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0004)))
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+8+2+1+strlen(sender)+6)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0004, 0x000b, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0004, 0x000b, 0x0000, snacid);

	aimbs_putraw(&fr->data, cookie, 8);

	aimbs_put16(&fr->data, 0x0002); /* channel */
	aimbs_put8(&fr->data, strlen(sender));
	aimbs_putraw(&fr->data, sender, strlen(sender));

	aim_tlvlist_add_16(&tl, 0x0003, code);
	aim_tlvlist_write(&fr->data, &tl);
	aim_tlvlist_free(&tl);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x000b - Receive the response from an ICQ status message request.
 *
 * This contains the ICQ status message.  Go figure.
 *
 */
static int clientautoresp(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	fu16_t channel, reason;
	char *sn;
	fu8_t *ck, snlen;

	ck = aimbs_getraw(bs, 8);
	channel = aimbs_get16(bs);
	snlen = aimbs_get8(bs);
	sn = aimbs_getstr(bs, snlen);
	reason = aimbs_get16(bs);

	if (channel == 0x0002) { /* File transfer declined */
		aimbs_get16(bs); /* Unknown */
		aimbs_get16(bs); /* Unknown */
		if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
			ret = userfunc(sess, rx, channel, sn, reason, ck);
	} else if (channel == 0x0004) { /* ICQ message */
		switch (reason) {
			case 0x0003: { /* ICQ status message.  Maybe other stuff too, you never know with these people. */
				fu8_t statusmsgtype, *msg;
				fu16_t len;
				fu32_t state;

				len = aimbs_getle16(bs); /* Should be 0x001b */
				aim_bstream_advance(bs, len); /* Unknown */

				len = aimbs_getle16(bs); /* Should be 0x000e */
				aim_bstream_advance(bs, len); /* Unknown */

				statusmsgtype = aimbs_getle8(bs);
				switch (statusmsgtype) {
					case 0xe8:
						state = AIM_ICQ_STATE_AWAY;
						break;
					case 0xe9:
						state = AIM_ICQ_STATE_AWAY | AIM_ICQ_STATE_BUSY;
						break;
					case 0xea:
						state = AIM_ICQ_STATE_AWAY | AIM_ICQ_STATE_OUT;
						break;
					case 0xeb:
						state = AIM_ICQ_STATE_AWAY | AIM_ICQ_STATE_DND | AIM_ICQ_STATE_BUSY;
						break;
					case 0xec:
						state = AIM_ICQ_STATE_CHAT;
						break;
					default:
						state = 0;
						break;
				}

				aimbs_getle8(bs); /* Unknown - 0x03 Maybe this means this is an auto-reply */
				aimbs_getle16(bs); /* Unknown - 0x0000 */
				aimbs_getle16(bs); /* Unknown - 0x0000 */

				len = aimbs_getle16(bs);
				msg = aimbs_getraw(bs, len);

				if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
					ret = userfunc(sess, rx, channel, sn, reason, state, msg);

				free(msg);
			} break;

			default: {
				if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
					ret = userfunc(sess, rx, channel, sn, reason);
			} break;
		} /* end switch */
	}

	free(ck);
	free(sn);

	return ret;
}

/*
 * Subtype 0x000c - Receive an ack after sending an ICBM.
 *
 * You have to have send the message with the AIM_IMFLAGS_ACK flag set
 * (TLV t(0003)).  The ack contains the ICBM header of the message you
 * sent.
 *
 */
static int msgack(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	aim_rxcallback_t userfunc;
	fu16_t ch;
	fu8_t *ck;
	char *sn;
	int ret = 0;

	ck = aimbs_getraw(bs, 8);
	ch = aimbs_get16(bs);
	sn = aimbs_getstr(bs, aimbs_get8(bs));

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, ch, sn);

	free(sn);
	free(ck);

	return ret;
}

/*
 * Subtype 0x0014 - Send a mini typing notification (mtn) packet.
 *
 * This is supported by winaim5 and newer, MacAIM bleh and newer, iChat bleh and newer,
 * and Gaim 0.60 and newer.
 *
 */
faim_export int aim_im_sendmtn(aim_session_t *sess, fu16_t type1, const char *sn, fu16_t type2)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0002)))
		return -EINVAL;

	if (!sn)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+11+strlen(sn)+2)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0004, 0x0014, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0004, 0x0014, 0x0000, snacid);

	/*
	 * 8 days of light
	 * Er, that is to say, 8 bytes of 0's
	 */
	aimbs_put16(&fr->data, 0x0000);
	aimbs_put16(&fr->data, 0x0000);
	aimbs_put16(&fr->data, 0x0000);
	aimbs_put16(&fr->data, 0x0000);

	/*
	 * Type 1 (should be 0x0001 for mtn)
	 */
	aimbs_put16(&fr->data, type1);

	/*
	 * Dest sn
	 */
	aimbs_put8(&fr->data, strlen(sn));
	aimbs_putraw(&fr->data, sn, strlen(sn));

	/*
	 * Type 2 (should be 0x0000, 0x0001, or 0x0002 for mtn)
	 */
	aimbs_put16(&fr->data, type2);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x0014 - Receive a mini typing notification (mtn) packet.
 *
 * This is supported by winaim5 and newer, MacAIM bleh and newer, iChat bleh and newer,
 * and Gaim 0.60 and newer.
 *
 */
static int mtn_receive(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	char *sn;
	fu8_t snlen;
	fu16_t type1, type2;

	aim_bstream_advance(bs, 8); /* Unknown - All 0's */
	type1 = aimbs_get16(bs);
	snlen = aimbs_get8(bs);
	sn = aimbs_getstr(bs, snlen);
	type2 = aimbs_get16(bs);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, type1, sn, type2);

	free(sn);

	return ret;
}

static int snachandler(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{

	if (snac->subtype == 0x0005)
		return aim_im_paraminfo(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x0006)
		return outgoingim(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x0007)
		return incomingim(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x000a)
		return missedcall(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x000b)
		return clientautoresp(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x000c)
		return msgack(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x0014)
		return mtn_receive(sess, mod, rx, snac, bs);

	return 0;
}

faim_internal int msg_modfirst(aim_session_t *sess, aim_module_t *mod)
{

	mod->family = 0x0004;
	mod->version = 0x0001;
	mod->toolid = 0x0110;
	mod->toolversion = 0x0629;
	mod->flags = 0;
	strncpy(mod->name, "messaging", sizeof(mod->name));
	mod->snachandler = snachandler;

	return 0;
}
