/*
 * Family 0x000e - Routines for the Chat service.
 *
 */

#define FAIM_INTERNAL
#include <aim.h> 

/* Stored in the ->priv of chat connections */
struct chatconnpriv {
	fu16_t exchange;
	char *name;
	fu16_t instance;
};

faim_internal void aim_conn_kill_chat(aim_session_t *sess, aim_conn_t *conn)
{
	struct chatconnpriv *ccp = (struct chatconnpriv *)conn->priv;

	if (ccp)
		free(ccp->name);
	free(ccp);

	return;
}

faim_export char *aim_chat_getname(aim_conn_t *conn)
{
	struct chatconnpriv *ccp;

	if (!conn)
		return NULL;

	if (conn->type != AIM_CONN_TYPE_CHAT)
		return NULL;

	ccp = (struct chatconnpriv *)conn->priv;

	return ccp->name;
}

/* XXX get this into conn.c -- evil!! */
faim_export aim_conn_t *aim_chat_getconn(aim_session_t *sess, const char *name)
{
	aim_conn_t *cur;

	for (cur = sess->connlist; cur; cur = cur->next) {
		struct chatconnpriv *ccp = (struct chatconnpriv *)cur->priv;

		if (cur->type != AIM_CONN_TYPE_CHAT)
			continue;
		if (!cur->priv) {
			faimdprintf(sess, 0, "faim: chat: chat connection with no name! (fd = %d)\n", cur->fd);
			continue;
		}

		if (strcmp(ccp->name, name) == 0)
			break;
	}

	return cur;
}

faim_export int aim_chat_attachname(aim_conn_t *conn, fu16_t exchange, const char *roomname, fu16_t instance)
{
	struct chatconnpriv *ccp;

	if (!conn || !roomname)
		return -EINVAL;

	if (conn->priv)
		free(conn->priv);

	if (!(ccp = malloc(sizeof(struct chatconnpriv))))
		return -ENOMEM;

	ccp->exchange = exchange;
	ccp->name = strdup(roomname);
	ccp->instance = instance;

	conn->priv = (void *)ccp;

	return 0;
}

static int aim_addtlvtochain_chatroom(aim_tlvlist_t **list, fu16_t type, fu16_t exchange, const char *roomname, fu16_t instance)
{
	fu8_t *buf;
	int buflen;
	aim_bstream_t bs;

	buflen = 2 + 1 + strlen(roomname) + 2;
	
	if (!(buf = malloc(buflen)))
		return 0;

	aim_bstream_init(&bs, buf, buflen);

	aimbs_put16(&bs, exchange);
	aimbs_put8(&bs, strlen(roomname));
	aimbs_putraw(&bs, roomname, strlen(roomname));
	aimbs_put16(&bs, instance);

	aim_addtlvtochain_raw(list, type, aim_bstream_curpos(&bs), buf);

	free(buf);

	return 0;
}

/*
 * Join a room of name roomname.  This is the first step to joining an 
 * already created room.  It's basically a Service Request for 
 * family 0x000e, with a little added on to specify the exchange and room 
 * name.
 */
faim_export int aim_chat_join(aim_session_t *sess, aim_conn_t *conn, fu16_t exchange, const char *roomname, fu16_t instance)
{
	aim_frame_t *fr;
	aim_snacid_t snacid;
	aim_tlvlist_t *tl = NULL;
	struct chatsnacinfo csi;
	
	if (!sess || !conn || !roomname || !strlen(roomname))
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 512)))
		return -ENOMEM;

	memset(&csi, 0, sizeof(csi));
	csi.exchange = exchange;
	strncpy(csi.name, roomname, sizeof(csi.name));
	csi.instance = instance;

	snacid = aim_cachesnac(sess, 0x0001, 0x0004, 0x0000, &csi, sizeof(csi));
	aim_putsnac(&fr->data, 0x0001, 0x0004, 0x0000, snacid);

	/*
	 * Requesting service chat (0x000e)
	 */
	aimbs_put16(&fr->data, 0x000e);

	aim_addtlvtochain_chatroom(&tl, 0x0001, exchange, roomname, instance);
	aim_writetlvchain(&fr->data, &tl);
	aim_freetlvchain(&tl);

	aim_tx_enqueue(sess, fr);

	return 0; 
}

faim_internal int aim_chat_readroominfo(aim_bstream_t *bs, struct aim_chat_roominfo *outinfo)
{
	int namelen;

	if (!bs || !outinfo)
		return 0;

	outinfo->exchange = aimbs_get16(bs);
	namelen = aimbs_get8(bs);
	outinfo->name = aimbs_getstr(bs, namelen);
	outinfo->instance = aimbs_get16(bs);

	return 0;
}

faim_export int aim_chat_leaveroom(aim_session_t *sess, const char *name)
{
	aim_conn_t *conn;

	if (!(conn = aim_chat_getconn(sess, name)))
		return -ENOENT;

	aim_conn_close(conn);

	return 0;
}

/*
 * conn must be a BOS connection!
 */
faim_export int aim_chat_invite(aim_session_t *sess, aim_conn_t *conn, const char *sn, const char *msg, fu16_t exchange, const char *roomname, fu16_t instance)
{
	int i;
	aim_frame_t *fr;
	aim_msgcookie_t *cookie;
	struct aim_invite_priv *priv;
	fu8_t ckstr[8];
	aim_snacid_t snacid;
	aim_tlvlist_t *otl = NULL, *itl = NULL;
	fu8_t *hdr;
	int hdrlen;
	aim_bstream_t hdrbs;
	
	if (!sess || !conn || !sn || !msg || !roomname)
		return -EINVAL;

	if (conn->type != AIM_CONN_TYPE_BOS)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 1152+strlen(sn)+strlen(roomname)+strlen(msg))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0004, 0x0006, 0x0000, sn, strlen(sn)+1);
	aim_putsnac(&fr->data, 0x0004, 0x0006, 0x0000, snacid);


	/*
	 * Cookie
	 */
	for (i = 0; i < sizeof(ckstr); i++)
		aimutil_put8(ckstr, (fu8_t) rand());

	/* XXX should be uncached by an unwritten 'invite accept' handler */
	if ((priv = malloc(sizeof(struct aim_invite_priv)))) {
		priv->sn = strdup(sn);
		priv->roomname = strdup(roomname);
		priv->exchange = exchange;
		priv->instance = instance;
	}

	if ((cookie = aim_mkcookie(ckstr, AIM_COOKIETYPE_INVITE, priv)))
		aim_cachecookie(sess, cookie);
	else
		free(priv);

	for (i = 0; i < sizeof(ckstr); i++)
		aimbs_put8(&fr->data, ckstr[i]);


	/*
	 * Channel (2)
	 */
	aimbs_put16(&fr->data, 0x0002);

	/*
	 * Dest sn
	 */
	aimbs_put8(&fr->data, strlen(sn));
	aimbs_putraw(&fr->data, sn, strlen(sn));

	/*
	 * TLV t(0005)
	 *
	 * Everything else is inside this TLV.
	 *
	 * Sigh.  AOL was rather inconsistent right here.  So we have
	 * to play some minor tricks.  Right inside the type 5 is some
	 * raw data, followed by a series of TLVs.  
	 *
	 */
	hdrlen = 2+8+16+6+4+4+strlen(msg)+4+2+1+strlen(roomname)+2;
	hdr = malloc(hdrlen);
	aim_bstream_init(&hdrbs, hdr, hdrlen);
	
	aimbs_put16(&hdrbs, 0x0000); /* Unknown! */
	aimbs_putraw(&hdrbs, ckstr, sizeof(ckstr)); /* I think... */
	aim_putcap(&hdrbs, AIM_CAPS_CHAT);

	aim_addtlvtochain16(&itl, 0x000a, 0x0001);
	aim_addtlvtochain_noval(&itl, 0x000f);
	aim_addtlvtochain_raw(&itl, 0x000c, strlen(msg), msg);
	aim_addtlvtochain_chatroom(&itl, 0x2711, exchange, roomname, instance);
	aim_writetlvchain(&hdrbs, &itl);
	
	aim_addtlvtochain_raw(&otl, 0x0005, aim_bstream_curpos(&hdrbs), hdr);

	aim_writetlvchain(&fr->data, &otl);

	free(hdr);
	aim_freetlvchain(&itl);
	aim_freetlvchain(&otl);
	
	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x0002 - General room information.  Lots of stuff.
 *
 * Values I know are in here but I havent attached
 * them to any of the 'Unknown's:
 *	- Language (English)
 *
 */
static int infoupdate(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	aim_userinfo_t *userinfo = NULL;
	aim_rxcallback_t userfunc;
	int ret = 0;
	int usercount = 0;
	fu8_t detaillevel = 0;
	char *roomname = NULL;
	struct aim_chat_roominfo roominfo;
	fu16_t tlvcount = 0;
	aim_tlvlist_t *tlvlist;
	char *roomdesc = NULL;
	fu16_t flags = 0;
	fu32_t creationtime = 0;
	fu16_t maxmsglen = 0, maxvisiblemsglen = 0;
	fu16_t unknown_d2 = 0, unknown_d5 = 0;

	aim_chat_readroominfo(bs, &roominfo);

	detaillevel = aimbs_get8(bs);

	if (detaillevel != 0x02) {
		faimdprintf(sess, 0, "faim: chat_roomupdateinfo: detail level %d not supported\n", detaillevel);
		return 1;
	}

	tlvcount = aimbs_get16(bs);

	/*
	 * Everything else are TLVs.
	 */ 
	tlvlist = aim_readtlvchain(bs);

	/*
	 * TLV type 0x006a is the room name in Human Readable Form.
	 */
	if (aim_gettlv(tlvlist, 0x006a, 1))
		roomname = aim_gettlv_str(tlvlist, 0x006a, 1);

	/*
	 * Type 0x006f: Number of occupants.
	 */
	if (aim_gettlv(tlvlist, 0x006f, 1))
		usercount = aim_gettlv16(tlvlist, 0x006f, 1);

	/*
	 * Type 0x0073:  Occupant list.
	 */
	if (aim_gettlv(tlvlist, 0x0073, 1)) {	
		int curoccupant = 0;
		aim_tlv_t *tmptlv;
		aim_bstream_t occbs;

		tmptlv = aim_gettlv(tlvlist, 0x0073, 1);

		/* Allocate enough userinfo structs for all occupants */
		userinfo = calloc(usercount, sizeof(aim_userinfo_t));

		aim_bstream_init(&occbs, tmptlv->value, tmptlv->length);

		while (curoccupant < usercount)
			aim_extractuserinfo(sess, &occbs, &userinfo[curoccupant++]);
	}

	/* 
	 * Type 0x00c9: Flags. (AIM_CHATROOM_FLAG)
	 */
	if (aim_gettlv(tlvlist, 0x00c9, 1))
		flags = aim_gettlv16(tlvlist, 0x00c9, 1);

	/* 
	 * Type 0x00ca: Creation time (4 bytes)
	 */
	if (aim_gettlv(tlvlist, 0x00ca, 1))
		creationtime = aim_gettlv32(tlvlist, 0x00ca, 1);

	/* 
	 * Type 0x00d1: Maximum Message Length
	 */
	if (aim_gettlv(tlvlist, 0x00d1, 1))
		maxmsglen = aim_gettlv16(tlvlist, 0x00d1, 1);

	/* 
	 * Type 0x00d2: Unknown. (2 bytes)
	 */
	if (aim_gettlv(tlvlist, 0x00d2, 1))
		unknown_d2 = aim_gettlv16(tlvlist, 0x00d2, 1);

	/* 
	 * Type 0x00d3: Room Description
	 */
	if (aim_gettlv(tlvlist, 0x00d3, 1))
		roomdesc = aim_gettlv_str(tlvlist, 0x00d3, 1);

	/*
	 * Type 0x000d4: Unknown (flag only)
	 */
	if (aim_gettlv(tlvlist, 0x000d4, 1))
		;

	/* 
	 * Type 0x00d5: Unknown. (1 byte)
	 */
	if (aim_gettlv(tlvlist, 0x00d5, 1))
		unknown_d5 = aim_gettlv8(tlvlist, 0x00d5, 1);


	/*
	 * Type 0x00d6: Encoding 1 ("us-ascii")
	 */
	if (aim_gettlv(tlvlist, 0x000d6, 1))
		;
	
	/*
	 * Type 0x00d7: Language 1 ("en")
	 */
	if (aim_gettlv(tlvlist, 0x000d7, 1))
		;

	/*
	 * Type 0x00d8: Encoding 2 ("us-ascii")
	 */
	if (aim_gettlv(tlvlist, 0x000d8, 1))
		;
	
	/*
	 * Type 0x00d9: Language 2 ("en")
	 */
	if (aim_gettlv(tlvlist, 0x000d9, 1))
		;

	/*
	 * Type 0x00da: Maximum visible message length
	 */
	if (aim_gettlv(tlvlist, 0x000da, 1))
		maxvisiblemsglen = aim_gettlv16(tlvlist, 0x00da, 1);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype))) {
		ret = userfunc(sess,
				rx, 
				&roominfo,
				roomname,
				usercount,
				userinfo,	
				roomdesc,
				flags,
				creationtime,
				maxmsglen,
				unknown_d2,
				unknown_d5,
				maxvisiblemsglen);
	}

	free(roominfo.name);
	free(userinfo);
	free(roomname);
	free(roomdesc);
	aim_freetlvchain(&tlvlist);

	return ret;
}

/* Subtypes 0x0003 and 0x0004 */
static int userlistchange(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	aim_userinfo_t *userinfo = NULL;
	aim_rxcallback_t userfunc;
	int curcount = 0, ret = 0;

	while (aim_bstream_empty(bs)) {
		curcount++;
		userinfo = realloc(userinfo, curcount * sizeof(aim_userinfo_t));
		aim_extractuserinfo(sess, bs, &userinfo[curcount-1]);
	}

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, curcount, userinfo);

	free(userinfo);

	return ret;
}

/*
 * Subtype 0x0005 - Send a Chat Message.
 *
 * Possible flags:
 *   AIM_CHATFLAGS_NOREFLECT   --  Unset the flag that requests messages
 *                                 should be sent to their sender.
 *   AIM_CHATFLAGS_AWAY        --  Mark the message as an autoresponse
 *                                 (Note that WinAIM does not honor this,
 *                                 and displays the message as normal.)
 *
 * XXX convert this to use tlvchains 
 */
faim_export int aim_chat_send_im(aim_session_t *sess, aim_conn_t *conn, fu16_t flags, const char *msg, int msglen)
{   
	int i;
	aim_frame_t *fr;
	aim_msgcookie_t *cookie;
	aim_snacid_t snacid;
	fu8_t ckstr[8];
	aim_tlvlist_t *otl = NULL, *itl = NULL;

	if (!sess || !conn || !msg || (msglen <= 0))
		return 0;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 1152)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x000e, 0x0005, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x000e, 0x0005, 0x0000, snacid);


	/* 
	 * Generate a random message cookie.
	 *
	 * XXX mkcookie should generate the cookie and cache it in one
	 * operation to preserve uniqueness.
	 *
	 */
	for (i = 0; i < sizeof(ckstr); i++)
		aimutil_put8(ckstr+i, (fu8_t) rand());

	cookie = aim_mkcookie(ckstr, AIM_COOKIETYPE_CHAT, NULL);
	cookie->data = NULL; /* XXX store something useful here */

	aim_cachecookie(sess, cookie);

	for (i = 0; i < sizeof(ckstr); i++)
		aimbs_put8(&fr->data, ckstr[i]);


	/*
	 * Channel ID. 
	 */
	aimbs_put16(&fr->data, 0x0003);


	/*
	 * Type 1: Flag meaning this message is destined to the room.
	 */
	aim_addtlvtochain_noval(&otl, 0x0001);

	/*
	 * Type 6: Reflect
	 */
	if (!(flags & AIM_CHATFLAGS_NOREFLECT))
		aim_addtlvtochain_noval(&otl, 0x0006);

	/*
	 * Type 7: Autoresponse
	 */
	if (flags & AIM_CHATFLAGS_AWAY)
		aim_addtlvtochain_noval(&otl, 0x0007);

	/*
	 * SubTLV: Type 1: Message
	 */
	aim_addtlvtochain_raw(&itl, 0x0001, strlen(msg), msg);

	/*
	 * Type 5: Message block.  Contains more TLVs.
	 *
	 * This could include other information... We just
	 * put in a message TLV however.  
	 * 
	 */
	aim_addtlvtochain_frozentlvlist(&otl, 0x0005, &itl);

	aim_writetlvchain(&fr->data, &otl);
	
	aim_freetlvchain(&itl);
	aim_freetlvchain(&otl);
	
	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x0006
 *
 * We could probably include this in the normal ICBM parsing 
 * code as channel 0x0003, however, since only the start
 * would be the same, we might as well do it here.
 *
 * General outline of this SNAC:
 *   snac
 *   cookie
 *   channel id
 *   tlvlist
 *     unknown
 *     source user info
 *       name
 *       evility
 *       userinfo tlvs
 *         online time
 *         etc
 *     message metatlv
 *       message tlv
 *         message string
 *       possibly others
 *  
 */
static int incomingmsg(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	aim_userinfo_t userinfo;
	aim_rxcallback_t userfunc;	
	int ret = 0;
	fu8_t *cookie;
	fu16_t channel;
	aim_tlvlist_t *otl;
	char *msg = NULL;
	aim_msgcookie_t *ck;

	memset(&userinfo, 0, sizeof(aim_userinfo_t));

	/*
	 * ICBM Cookie.  Uncache it.
	 */
	cookie = aimbs_getraw(bs, 8);

	if ((ck = aim_uncachecookie(sess, cookie, AIM_COOKIETYPE_CHAT))) {
		free(ck->data);
		free(ck);
	}

	/*
	 * Channel ID
	 *
	 * Channels 1 and 2 are implemented in the normal ICBM
	 * parser.
	 *
	 * We only do channel 3 here.
	 *
	 */
	channel = aimbs_get16(bs);

	if (channel != 0x0003) {
		faimdprintf(sess, 0, "faim: chat_incoming: unknown channel! (0x%04x)\n", channel);
		return 0;
	}

	/*
	 * Start parsing TLVs right away. 
	 */
	otl = aim_readtlvchain(bs);

	/*
	 * Type 0x0003: Source User Information
	 */
	if (aim_gettlv(otl, 0x0003, 1)) {
		aim_tlv_t *userinfotlv;
		aim_bstream_t tbs;

		userinfotlv = aim_gettlv(otl, 0x0003, 1);

		aim_bstream_init(&tbs, userinfotlv->value, userinfotlv->length);
		aim_extractuserinfo(sess, &tbs, &userinfo);
	}

	/*
	 * Type 0x0001: If present, it means it was a message to the 
	 * room (as opposed to a whisper).
	 */
	if (aim_gettlv(otl, 0x0001, 1))
		;

	/*
	 * Type 0x0005: Message Block.  Conains more TLVs.
	 */
	if (aim_gettlv(otl, 0x0005, 1)) {
		aim_tlvlist_t *itl;
		aim_tlv_t *msgblock;
		aim_bstream_t tbs;

		msgblock = aim_gettlv(otl, 0x0005, 1);
		aim_bstream_init(&tbs, msgblock->value, msgblock->length);
		itl = aim_readtlvchain(&tbs);

		/* 
		 * Type 0x0001: Message.
		 */	
		if (aim_gettlv(itl, 0x0001, 1))
			msg = aim_gettlv_str(itl, 0x0001, 1);

		aim_freetlvchain(&itl); 
	}

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, &userinfo, msg);

	free(cookie);
	free(msg);
	aim_freetlvchain(&otl);

	return ret;
}

static int snachandler(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{

	if (snac->subtype == 0x0002)
		return infoupdate(sess, mod, rx, snac, bs);
	else if ((snac->subtype == 0x0003) || (snac->subtype == 0x0004))
		return userlistchange(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x0006)
		return incomingmsg(sess, mod, rx, snac, bs);

	return 0;
}

faim_internal int chat_modfirst(aim_session_t *sess, aim_module_t *mod)
{

	mod->family = 0x000e;
	mod->version = 0x0001;
	mod->toolid = 0x0004; /* XXX this doesn't look right */
	mod->toolversion = 0x0001; /* nor does this */
	mod->flags = 0;
	strncpy(mod->name, "chat", sizeof(mod->name));
	mod->snachandler = snachandler;

	return 0;
}
