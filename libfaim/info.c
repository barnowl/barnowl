/*
 * Family 0x0002 - Information.
 *
 * The functions here are responsible for requesting and parsing information-
 * gathering SNACs.  Or something like that. 
 *
 */

#define FAIM_INTERNAL
#include <aim.h>

struct aim_priv_inforeq {
	char sn[MAXSNLEN+1];
	fu16_t infotype;
};

/*
 * Subtype 0x0002
 *
 * Request Location services rights.
 *
 */
faim_export int aim_bos_reqlocaterights(aim_session_t *sess, aim_conn_t *conn)
{
        return aim_genericreq_n(sess, conn, 0x0002, 0x0002);
}

/*
 * Subtype 0x0004
 *
 * Gives BOS your profile.
 * 
 */
faim_export int aim_bos_setprofile(aim_session_t *sess, aim_conn_t *conn, const char *profile, const char *awaymsg, fu32_t caps)
{
	static const char defencoding[] = {"text/aolrtf; charset=\"us-ascii\""};
	aim_frame_t *fr;
	aim_tlvlist_t *tl = NULL;
	aim_snacid_t snacid;

	/* Build to packet first to get real length */
	if (profile) {
		aim_addtlvtochain_raw(&tl, 0x0001, strlen(defencoding), defencoding);
		aim_addtlvtochain_raw(&tl, 0x0002, strlen(profile), profile);
	}

	/*
	 * So here's how this works:
	 *   - You are away when you have a non-zero-length type 4 TLV stored.
	 *   - You become unaway when you clear the TLV with a zero-length
	 *       type 4 TLV.
	 *   - If you do not send the type 4 TLV, your status does not change
	 *       (that is, if you were away, you'll remain away).
	 */
	if (awaymsg) {
		if (strlen(awaymsg)) {
			aim_addtlvtochain_raw(&tl, 0x0003, strlen(defencoding), defencoding);
			aim_addtlvtochain_raw(&tl, 0x0004, strlen(awaymsg), awaymsg);
		} else
			aim_addtlvtochain_noval(&tl, 0x0004);
	}

	aim_addtlvtochain_caps(&tl, 0x0005, caps);

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10 + aim_sizetlvchain(&tl))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0002, 0x0004, 0x0000, NULL, 0);

	aim_putsnac(&fr->data, 0x0002, 0x004, 0x0000, snacid);
	aim_writetlvchain(&fr->data, &tl);
	aim_freetlvchain(&tl);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x0005 - Request info of another AIM user.
 *
 */
faim_export int aim_getinfo(aim_session_t *sess, aim_conn_t *conn, const char *sn, fu16_t infotype)
{
	struct aim_priv_inforeq privdata;
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sess || !conn || !sn)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 12+1+strlen(sn))))
		return -ENOMEM;

	strncpy(privdata.sn, sn, sizeof(privdata.sn));
	privdata.infotype = infotype;
	snacid = aim_cachesnac(sess, 0x0002, 0x0005, 0x0000, &privdata, sizeof(struct aim_priv_inforeq));
	
	aim_putsnac(&fr->data, 0x0002, 0x0005, 0x0000, snacid);
	aimbs_put16(&fr->data, infotype);
	aimbs_put8(&fr->data, strlen(sn));
	aimbs_putraw(&fr->data, sn, strlen(sn));

	aim_tx_enqueue(sess, fr);

	return 0;
}

faim_export const char *aim_userinfo_sn(aim_userinfo_t *ui)
{

	if (!ui)
		return NULL;

	return ui->sn;
}

faim_export fu16_t aim_userinfo_flags(aim_userinfo_t *ui)
{

	if (!ui)
		return 0;

	return ui->flags;
}

faim_export fu16_t aim_userinfo_idle(aim_userinfo_t *ui)
{

	if (!ui)
		return 0;

	return ui->idletime;
}

faim_export float aim_userinfo_warnlevel(aim_userinfo_t *ui)
{

	if (!ui)
		return 0.00;

	return (ui->warnlevel / 10);
}

faim_export time_t aim_userinfo_createtime(aim_userinfo_t *ui)
{

	if (!ui)
		return 0;

	return (time_t)ui->createtime;
}

faim_export time_t aim_userinfo_membersince(aim_userinfo_t *ui)
{

	if (!ui)
		return 0;

	return (time_t)ui->membersince;
}

faim_export time_t aim_userinfo_onlinesince(aim_userinfo_t *ui)
{

	if (!ui)
		return 0;

	return (time_t)ui->onlinesince;
}

faim_export fu32_t aim_userinfo_sessionlen(aim_userinfo_t *ui)
{

	if (!ui)
		return 0;

	return ui->sessionlen;
}

faim_export int aim_userinfo_hascap(aim_userinfo_t *ui, fu32_t cap)
{

	if (!ui || !(ui->present & AIM_USERINFO_PRESENT_CAPABILITIES))
		return -1;

	return !!(ui->capabilities & cap);
}


/*
 * Capability blocks. 
 *
 * These are CLSIDs. They should actually be of the form:
 *
 * {0x0946134b, 0x4c7f, 0x11d1,
 *  {0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}}},
 *
 * But, eh.
 */
static const struct {
	fu32_t flag;
	fu8_t data[16];
} aim_caps[] = {

	/*
	 * Chat is oddball.
	 */
	{AIM_CAPS_CHAT,
	 {0x74, 0x8f, 0x24, 0x20, 0x62, 0x87, 0x11, 0xd1, 
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	/*
	 * These are mostly in order.
	 */
	{AIM_CAPS_VOICE,
	 {0x09, 0x46, 0x13, 0x41, 0x4c, 0x7f, 0x11, 0xd1, 
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	{AIM_CAPS_SENDFILE,
	 {0x09, 0x46, 0x13, 0x43, 0x4c, 0x7f, 0x11, 0xd1, 
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	/*
	 * Advertised by the EveryBuddy client.
	 */
	{AIM_CAPS_ICQ,
	 {0x09, 0x46, 0x13, 0x44, 0x4c, 0x7f, 0x11, 0xd1, 
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	{AIM_CAPS_IMIMAGE,
	 {0x09, 0x46, 0x13, 0x45, 0x4c, 0x7f, 0x11, 0xd1, 
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	{AIM_CAPS_BUDDYICON,
	 {0x09, 0x46, 0x13, 0x46, 0x4c, 0x7f, 0x11, 0xd1, 
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	{AIM_CAPS_SAVESTOCKS,
	 {0x09, 0x46, 0x13, 0x47, 0x4c, 0x7f, 0x11, 0xd1,
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	{AIM_CAPS_GETFILE,
	 {0x09, 0x46, 0x13, 0x48, 0x4c, 0x7f, 0x11, 0xd1,
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	{AIM_CAPS_ICQSERVERRELAY,
	 {0x09, 0x46, 0x13, 0x49, 0x4c, 0x7f, 0x11, 0xd1,
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	/*
	 * Indeed, there are two of these.  The former appears to be correct, 
	 * but in some versions of winaim, the second one is set.  Either they 
	 * forgot to fix endianness, or they made a typo. It really doesn't 
	 * matter which.
	 */
	{AIM_CAPS_GAMES,
	 {0x09, 0x46, 0x13, 0x4a, 0x4c, 0x7f, 0x11, 0xd1,
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},
	{AIM_CAPS_GAMES2,
	 {0x09, 0x46, 0x13, 0x4a, 0x4c, 0x7f, 0x11, 0xd1,
	  0x22, 0x82, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	{AIM_CAPS_SENDBUDDYLIST,
	 {0x09, 0x46, 0x13, 0x4b, 0x4c, 0x7f, 0x11, 0xd1,
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	/* from ICQ2002a
	{AIM_CAPS_ICQUNKNOWN2,
	 {0x09, 0x46, 0x13, 0x4e, 0x4c, 0x7f, 0x11, 0xd1,
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}}, */

	{AIM_CAPS_ICQRTF,
	 {0x97, 0xb1, 0x27, 0x51, 0x24, 0x3c, 0x43, 0x34, 
	  0xad, 0x22, 0xd6, 0xab, 0xf7, 0x3f, 0x14, 0x92}},

	/* supposed to be ICQRTF?
	{AIM_CAPS_TRILLUNKNOWN,
	 {0x97, 0xb1, 0x27, 0x51, 0x24, 0x3c, 0x43, 0x34, 
	  0xad, 0x22, 0xd6, 0xab, 0xf7, 0x3f, 0x14, 0x09}}, */

	{AIM_CAPS_ICQUNKNOWN,
	 {0x2e, 0x7a, 0x64, 0x75, 0xfa, 0xdf, 0x4d, 0xc8,
	  0x88, 0x6f, 0xea, 0x35, 0x95, 0xfd, 0xb6, 0xdf}},

	{AIM_CAPS_EMPTY,
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},

	{AIM_CAPS_TRILLIANCRYPT,
	 {0xf2, 0xe7, 0xc7, 0xf4, 0xfe, 0xad, 0x4d, 0xfb,
	  0xb2, 0x35, 0x36, 0x79, 0x8b, 0xdf, 0x00, 0x00}},

	{AIM_CAPS_APINFO, 
         {0xAA, 0x4A, 0x32, 0xB5, 0xF8, 0x84, 0x48, 0xc6,
	  0xA3, 0xD7, 0x8C, 0x50, 0x97, 0x19, 0xFD, 0x5B}},

	{AIM_CAPS_LAST}
};

/*
 * This still takes a length parameter even with a bstream because capabilities
 * are not naturally bounded.
 * 
 */
faim_internal fu32_t aim_getcap(aim_session_t *sess, aim_bstream_t *bs, int len)
{
	fu32_t flags = 0;
	int offset;

	for (offset = 0; aim_bstream_empty(bs) && (offset < len); offset += 0x10) {
		fu8_t *cap;
		int i, identified;

		cap = aimbs_getraw(bs, 0x10);

		for (i = 0, identified = 0; !(aim_caps[i].flag & AIM_CAPS_LAST); i++) {

			if (memcmp(&aim_caps[i].data, cap, 0x10) == 0) {
				flags |= aim_caps[i].flag;
				identified++;
				break; /* should only match once... */

			}
		}

		if (!identified) {
			faimdprintf(sess, 0, "unknown capability: {%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
					cap[0], cap[1], cap[2], cap[3],
					cap[4], cap[5],
					cap[6], cap[7],
					cap[8], cap[9],
					cap[10], cap[11], cap[12], cap[13],
					cap[14], cap[15]);
		}

		free(cap);
	}

	return flags;
}

faim_internal int aim_putcap(aim_bstream_t *bs, fu32_t caps)
{
	int i;

	if (!bs)
		return -EINVAL;

	for (i = 0; aim_bstream_empty(bs); i++) {

		if (aim_caps[i].flag == AIM_CAPS_LAST)
			break;

		if (caps & aim_caps[i].flag)
			aimbs_putraw(bs, aim_caps[i].data, 0x10);

	}

	return 0;
}

static void dumptlv(aim_session_t *sess, fu16_t type, aim_bstream_t *bs, fu8_t len)
{
	int i;

	if (!sess || !bs || !len)
		return;

	faimdprintf(sess, 0, "userinfo:   type  =0x%04x\n", type);
	faimdprintf(sess, 0, "userinfo:   length=0x%04x\n", len);

	faimdprintf(sess, 0, "userinfo:   value:\n");

	for (i = 0; i < len; i++) {
		if ((i % 8) == 0)
			faimdprintf(sess, 0, "\nuserinfo:        ");

		faimdprintf(sess, 0, "0x%2x ", aimbs_get8(bs));
	}

	faimdprintf(sess, 0, "\n");

	return;
}

/*
 * AIM is fairly regular about providing user info.  This is a generic 
 * routine to extract it in its standard form.
 */
faim_internal int aim_extractuserinfo(aim_session_t *sess, aim_bstream_t *bs, aim_userinfo_t *outinfo)
{
	int curtlv, tlvcnt;
	fu8_t snlen;

	if (!bs || !outinfo)
		return -EINVAL;

	/* Clear out old data first */
	memset(outinfo, 0x00, sizeof(aim_userinfo_t));

	/*
	 * Screen name.  Stored as an unterminated string prepended with a 
	 * byte containing its length.
	 */
	snlen = aimbs_get8(bs);
	aimbs_getrawbuf(bs, outinfo->sn, snlen);

	/*
	 * Warning Level.  Stored as an unsigned short.
	 */
	outinfo->warnlevel = aimbs_get16(bs);

	/*
	 * TLV Count. Unsigned short representing the number of 
	 * Type-Length-Value triples that follow.
	 */
	tlvcnt = aimbs_get16(bs);

	/* 
	 * Parse out the Type-Length-Value triples as they're found.
	 */
	for (curtlv = 0; curtlv < tlvcnt; curtlv++) {
		int endpos;
		fu16_t type, length;

		type = aimbs_get16(bs);
		length = aimbs_get16(bs);

		endpos = aim_bstream_curpos(bs) + length;

		if (type == 0x0001) {
			/*
			 * Type = 0x0001: User flags
			 * 
			 * Specified as any of the following ORed together:
			 *      0x0001  Trial (user less than 60days)
			 *      0x0002  Unknown bit 2
			 *      0x0004  AOL Main Service user
			 *      0x0008  Unknown bit 4
			 *      0x0010  Free (AIM) user 
			 *      0x0020  Away
			 *      0x0400  ActiveBuddy
			 *
			 */
			outinfo->flags = aimbs_get16(bs);
			outinfo->present |= AIM_USERINFO_PRESENT_FLAGS;

		} else if (type == 0x0002) {
			/*
			 * Type = 0x0002: Account creation time. 
			 *
			 * The time/date that the user originally registered for
			 * the service, stored in time_t format.
			 *
			 * I'm not sure how this differs from type 5 ("member
			 * since").
			 *
			 * Note: This is the field formerly known as "member
			 * since".  All these years and I finally found out
			 * that I got the name wrong.
			 */
			outinfo->createtime = aimbs_get32(bs);
			outinfo->present |= AIM_USERINFO_PRESENT_CREATETIME;

		} else if (type == 0x0003) {
			/*
			 * Type = 0x0003: On-Since date.
			 *
			 * The time/date that the user started their current 
			 * session, stored in time_t format.
			 */
			outinfo->onlinesince = aimbs_get32(bs);
			outinfo->present |= AIM_USERINFO_PRESENT_ONLINESINCE;

		} else if (type == 0x0004) {
			/*
			 * Type = 0x0004: Idle time.
			 *
			 * Number of seconds since the user actively used the 
			 * service.
			 *
			 * Note that the client tells the server when to start
			 * counting idle times, so this may or may not be 
			 * related to reality.
			 */
			outinfo->idletime = aimbs_get16(bs);
			outinfo->present |= AIM_USERINFO_PRESENT_IDLE;

		} else if (type == 0x0005) {
			/*
			 * Type = 0x0005: Member since date. 
			 *
			 * The time/date that the user originally registered for
			 * the service, stored in time_t format.
			 *
			 * This is sometimes sent instead of type 2 ("account
			 * creation time"), particularly in the self-info.
			 * And particularly for ICQ?
			 */
			outinfo->membersince = aimbs_get32(bs);
			outinfo->present |= AIM_USERINFO_PRESENT_MEMBERSINCE;

		} else if (type == 0x0006) {
			/*
			 * Type = 0x0006: ICQ Online Status
			 *
			 * ICQ's Away/DND/etc "enriched" status. Some decoding 
			 * of values done by Scott <darkagl@pcnet.com>
			 */
			aimbs_get16(bs);
			outinfo->icqinfo.status = aimbs_get16(bs);
			outinfo->present |= AIM_USERINFO_PRESENT_ICQEXTSTATUS;

		} else if (type == 0x000a) {
			/*
			 * Type = 0x000a
			 *
			 * ICQ User IP Address.
			 * Ahh, the joy of ICQ security.
			 */
			outinfo->icqinfo.ipaddr = aimbs_get32(bs);
			outinfo->present |= AIM_USERINFO_PRESENT_ICQIPADDR;

		} else if (type == 0x000c) {
			/* 
			 * Type = 0x000c
			 *
			 * random crap containing the IP address,
			 * apparently a port number, and some Other Stuff.
			 *
			 */
			aimbs_getrawbuf(bs, outinfo->icqinfo.crap, 0x25);
			outinfo->present |= AIM_USERINFO_PRESENT_ICQDATA;

		} else if (type == 0x000d) {
			/*
			 * Type = 0x000d
			 *
			 * Capability information.
			 *
			 */
			outinfo->capabilities = aim_getcap(sess, bs, length);
			outinfo->present |= AIM_USERINFO_PRESENT_CAPABILITIES;

		} else if (type == 0x000e) {
			/*
			 * Type = 0x000e
			 *
			 * Unknown.  Always of zero length, and always only
			 * on AOL users.
			 *
			 * Ignore.
			 *
			 */

		} else if ((type == 0x000f) || (type == 0x0010)) {
			/*
			 * Type = 0x000f: Session Length. (AIM)
			 * Type = 0x0010: Session Length. (AOL)
			 *
			 * The duration, in seconds, of the user's current 
			 * session.
			 *
			 * Which TLV type this comes in depends on the
			 * service the user is using (AIM or AOL).
			 *
			 */
			outinfo->sessionlen = aimbs_get32(bs);
			outinfo->present |= AIM_USERINFO_PRESENT_SESSIONLEN;

		} else if (type == 0x001d) {
			/*
			 * Type 29: Unknown.
			 *
			 * Currently very rare. Always 18 bytes of mostly zero.
			 */

		} else if (type == 0x001e) {
			/*
			 * Type 30: Unknown.
			 *
			 * Always four bytes, but it doesn't look like an int.
			 */
		} else {

			/*
			 * Reaching here indicates that either AOL has
			 * added yet another TLV for us to deal with, 
			 * or the parsing has gone Terribly Wrong.
			 *
			 * Either way, inform the owner and attempt
			 * recovery.
			 *
			 */
			faimdprintf(sess, 0, "userinfo: **warning: unexpected TLV:\n");
			faimdprintf(sess, 0, "userinfo:   sn    =%s\n", outinfo->sn);
			dumptlv(sess, type, bs, length);
		}

		/* Save ourselves. */
		aim_bstream_setpos(bs, endpos);
	}

	return 0;
}

/*
 * Inverse of aim_extractuserinfo()
 */
faim_internal int aim_putuserinfo(aim_bstream_t *bs, aim_userinfo_t *info)
{
	aim_tlvlist_t *tlvlist = NULL;

	if (!bs || !info)
		return -EINVAL;

	aimbs_put8(bs, strlen(info->sn));
	aimbs_putraw(bs, info->sn, strlen(info->sn));

	aimbs_put16(bs, info->warnlevel);


	if (info->present & AIM_USERINFO_PRESENT_FLAGS)
		aim_addtlvtochain16(&tlvlist, 0x0001, info->flags);
	if (info->present & AIM_USERINFO_PRESENT_MEMBERSINCE)
		aim_addtlvtochain32(&tlvlist, 0x0002, info->membersince);
	if (info->present & AIM_USERINFO_PRESENT_ONLINESINCE)
		aim_addtlvtochain32(&tlvlist, 0x0003, info->onlinesince);
	if (info->present & AIM_USERINFO_PRESENT_IDLE)
		aim_addtlvtochain16(&tlvlist, 0x0004, info->idletime);

/* XXX - So, ICQ_OSCAR_SUPPORT is never defined anywhere... */
#if ICQ_OSCAR_SUPPORT
	if (atoi(info->sn) != 0) {
		if (info->present & AIM_USERINFO_PRESENT_ICQEXTSTATUS)
			aim_addtlvtochain16(&tlvlist, 0x0006, info->icqinfo.status);
		if (info->present & AIM_USERINFO_PRESENT_ICQIPADDR)
			aim_addtlvtochain32(&tlvlist, 0x000a, info->icqinfo.ipaddr);
	}
#endif

	if (info->present & AIM_USERINFO_PRESENT_CAPABILITIES)
		aim_addtlvtochain_caps(&tlvlist, 0x000d, info->capabilities);
 
	if (info->present & AIM_USERINFO_PRESENT_SESSIONLEN)
		aim_addtlvtochain32(&tlvlist, (fu16_t)((info->flags & AIM_FLAG_AOL) ? 0x0010 : 0x000f), info->sessionlen);

	aimbs_put16(bs, aim_counttlvchain(&tlvlist));
	aim_writetlvchain(bs, &tlvlist);
	aim_freetlvchain(&tlvlist);

	return 0;
}

/*
 * Subtype 0x000b - Huh? What is this?
 */
faim_export int aim_0002_000b(aim_session_t *sess, aim_conn_t *conn, const char *sn)
{
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sess || !conn || !sn)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+1+strlen(sn))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0002, 0x000b, 0x0000, NULL, 0);
	
	aim_putsnac(&fr->data, 0x0002, 0x000b, 0x0000, snacid);
	aimbs_put8(&fr->data, strlen(sn));
	aimbs_putraw(&fr->data, sn, strlen(sn));

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x0003
 *
 * Normally contains:
 *   t(0001)  - short containing max profile length (value = 1024)
 *   t(0002)  - short - unknown (value = 16) [max MIME type length?]
 *   t(0003)  - short - unknown (value = 10)
 *   t(0004)  - short - unknown (value = 2048) [ICQ only?]
 */
static int rights(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	aim_tlvlist_t *tlvlist;
	aim_rxcallback_t userfunc;
	int ret = 0;
	fu16_t maxsiglen = 0;

	tlvlist = aim_readtlvchain(bs);

	if (aim_gettlv(tlvlist, 0x0001, 1))
		maxsiglen = aim_gettlv16(tlvlist, 0x0001, 1);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, maxsiglen);

	aim_freetlvchain(&tlvlist);

	return ret;
}

/* Subtype 0x0006 */
static int userinfo(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	aim_userinfo_t userinfo;
	char *text_encoding = NULL, *text = NULL;
	aim_rxcallback_t userfunc;
	aim_tlvlist_t *tlvlist;
	aim_snac_t *origsnac = NULL;
	struct aim_priv_inforeq *inforeq;
	int ret = 0;

	origsnac = aim_remsnac(sess, snac->id);

	if (!origsnac || !origsnac->data) {
		faimdprintf(sess, 0, "parse_userinfo_middle: major problem: no snac stored!\n");
		return 0;
	}

	inforeq = (struct aim_priv_inforeq *)origsnac->data;

	if ((inforeq->infotype != AIM_GETINFO_GENERALINFO) &&
			(inforeq->infotype != AIM_GETINFO_AWAYMESSAGE) &&
			(inforeq->infotype != AIM_GETINFO_CAPABILITIES)) {
		faimdprintf(sess, 0, "parse_userinfo_middle: unknown infotype in request! (0x%04x)\n", inforeq->infotype);
		return 0;
	}

	aim_extractuserinfo(sess, bs, &userinfo);

	tlvlist = aim_readtlvchain(bs);

	/* 
	 * Depending on what informational text was requested, different
	 * TLVs will appear here.
	 *
	 * Profile will be 1 and 2, away message will be 3 and 4, caps
	 * will be 5.
	 */
	if (inforeq->infotype == AIM_GETINFO_GENERALINFO) {
		text_encoding = aim_gettlv_str(tlvlist, 0x0001, 1);
		text = aim_gettlv_str(tlvlist, 0x0002, 1);
	} else if (inforeq->infotype == AIM_GETINFO_AWAYMESSAGE) {
		text_encoding = aim_gettlv_str(tlvlist, 0x0003, 1);
		text = aim_gettlv_str(tlvlist, 0x0004, 1);
	} else if (inforeq->infotype == AIM_GETINFO_CAPABILITIES) {
		aim_tlv_t *ct;

		if ((ct = aim_gettlv(tlvlist, 0x0005, 1))) {
			aim_bstream_t cbs;

			aim_bstream_init(&cbs, ct->value, ct->length);

			userinfo.capabilities = aim_getcap(sess, &cbs, ct->length);
			userinfo.present = AIM_USERINFO_PRESENT_CAPABILITIES;
		}
	}

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, &userinfo, inforeq->infotype, text_encoding, text);

	free(text_encoding);
	free(text);

	aim_freetlvchain(&tlvlist);

	if (origsnac)
		free(origsnac->data);
	free(origsnac);

	return ret;
}

/* 
 * Subtype 0x0009 - Set directory profile data.
 *
 * This is not the same as aim_bos_setprofile!
 * privacy: 1 to allow searching, 0 to disallow.
 *
 */
faim_export int aim_setdirectoryinfo(aim_session_t *sess, aim_conn_t *conn, const char *first, const char *middle, const char *last, const char *maiden, const char *nickname, const char *street, const char *city, const char *state, const char *zip, int country, fu16_t privacy) 
{
	aim_frame_t *fr;
	aim_snacid_t snacid;
	aim_tlvlist_t *tl = NULL;

	aim_addtlvtochain16(&tl, 0x000a, privacy);

	if (first)
		aim_addtlvtochain_raw(&tl, 0x0001, strlen(first), first);
	if (last)
		aim_addtlvtochain_raw(&tl, 0x0002, strlen(last), last);
	if (middle)
		aim_addtlvtochain_raw(&tl, 0x0003, strlen(middle), middle);
	if (maiden)
		aim_addtlvtochain_raw(&tl, 0x0004, strlen(maiden), maiden);

	if (state)
		aim_addtlvtochain_raw(&tl, 0x0007, strlen(state), state);
	if (city)
		aim_addtlvtochain_raw(&tl, 0x0008, strlen(city), city);

	if (nickname)
		aim_addtlvtochain_raw(&tl, 0x000c, strlen(nickname), nickname);
	if (zip)
		aim_addtlvtochain_raw(&tl, 0x000d, strlen(zip), zip);

	if (street)
		aim_addtlvtochain_raw(&tl, 0x0021, strlen(street), street);

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+aim_sizetlvchain(&tl))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0002, 0x0009, 0x0000, NULL, 0);

	aim_putsnac(&fr->data, 0x0002, 0x0009, 0x0000, snacid);
	aim_writetlvchain(&fr->data, &tl);
	aim_freetlvchain(&tl);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x000f
 * 
 * XXX pass these in better
 *
 */
faim_export int aim_setuserinterests(aim_session_t *sess, aim_conn_t *conn, const char *interest1, const char *interest2, const char *interest3, const char *interest4, const char *interest5, fu16_t privacy)
{
	aim_frame_t *fr;
	aim_snacid_t snacid;
	aim_tlvlist_t *tl = NULL;

	/* ?? privacy ?? */
	aim_addtlvtochain16(&tl, 0x000a, privacy);

	if (interest1)
		aim_addtlvtochain_raw(&tl, 0x0000b, strlen(interest1), interest1);
	if (interest2)
		aim_addtlvtochain_raw(&tl, 0x0000b, strlen(interest2), interest2);
	if (interest3)
		aim_addtlvtochain_raw(&tl, 0x0000b, strlen(interest3), interest3);
	if (interest4)
		aim_addtlvtochain_raw(&tl, 0x0000b, strlen(interest4), interest4);
	if (interest5)
		aim_addtlvtochain_raw(&tl, 0x0000b, strlen(interest5), interest5);

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+aim_sizetlvchain(&tl))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0002, 0x000f, 0x0000, NULL, 0);

	aim_putsnac(&fr->data, 0x0002, 0x000f, 0x0000, 0);
	aim_writetlvchain(&fr->data, &tl);
	aim_freetlvchain(&tl);

	aim_tx_enqueue(sess, fr);

	return 0;
}

static int snachandler(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{

	if (snac->subtype == 0x0003)
		return rights(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x0006)
		return userinfo(sess, mod, rx, snac, bs);

	return 0;
}

faim_internal int locate_modfirst(aim_session_t *sess, aim_module_t *mod)
{

	mod->family = 0x0002;
	mod->version = 0x0001;
	mod->toolid = 0x0101;
	mod->toolversion = 0x047b;
	mod->flags = 0;
	strncpy(mod->name, "locate", sizeof(mod->name));
	mod->snachandler = snachandler;

	return 0;
}
