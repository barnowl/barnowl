/*
 * Family 0x0002 - Locate.
 *
 * The functions here are responsible for requesting and parsing information-
 * gathering SNACs.  Or something like that.  This family contains the SNACs 
 * for getting and setting info, away messages, directory profile thingy, etc.
 */

#define FAIM_INTERNAL
#include <aim.h>
#ifdef _WIN32
#include "win32dep.h"
#endif

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
	 * These are in ascending numerical order.
	 */

	/*
	 * Perhaps better called AIM_CAPS_SHORTCAPS
	 */
	{AIM_CAPS_ICHAT,
	 {0x09, 0x46, 0x00, 0x00, 0x4c, 0x7f, 0x11, 0xd1, 
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	{AIM_CAPS_SECUREIM,
	 {0x09, 0x46, 0x00, 0x01, 0x4c, 0x7f, 0x11, 0xd1, 
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	{AIM_CAPS_VIDEO,
	 {0x09, 0x46, 0x01, 0x00, 0x4c, 0x7f, 0x11, 0xd1, 
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	/*
	 * Not really sure about this one.  In an email from 
	 * 26 Sep 2003, Matthew Sachs suggested that, "this 
	 * is probably the capability for the SMS features."
	 */
	{AIM_CAPS_SMS,
	 {0x09, 0x46, 0x01, 0xff, 0x4c, 0x7f, 0x11, 0xd1, 
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	{AIM_CAPS_GENERICUNKNOWN,
	 {0x09, 0x46, 0xf0, 0x03, 0x4c, 0x7f, 0x11, 0xd1, 
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	{AIM_CAPS_GENERICUNKNOWN,
	 {0x09, 0x46, 0xf0, 0x04, 0x4c, 0x7f, 0x11, 0xd1, 
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	{AIM_CAPS_GENERICUNKNOWN,
	 {0x09, 0x46, 0xf0, 0x05, 0x4c, 0x7f, 0x11, 0xd1, 
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	{AIM_CAPS_HIPTOP,
	 {0x09, 0x46, 0x13, 0x23, 0x4c, 0x7f, 0x11, 0xd1, 
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	{AIM_CAPS_VOICE,
	 {0x09, 0x46, 0x13, 0x41, 0x4c, 0x7f, 0x11, 0xd1, 
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	{AIM_CAPS_SENDFILE,
	 {0x09, 0x46, 0x13, 0x43, 0x4c, 0x7f, 0x11, 0xd1, 
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	{AIM_CAPS_ICQ_DIRECT,
	 {0x09, 0x46, 0x13, 0x44, 0x4c, 0x7f, 0x11, 0xd1, 
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	{AIM_CAPS_DIRECTIM,
	 {0x09, 0x46, 0x13, 0x45, 0x4c, 0x7f, 0x11, 0xd1, 
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	{AIM_CAPS_BUDDYICON,
	 {0x09, 0x46, 0x13, 0x46, 0x4c, 0x7f, 0x11, 0xd1, 
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	/*
	 * Windows AIM calls this "Add-ins," which is probably more accurate
	 */
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

	/*
	 * Setting this lets AIM users receive messages from ICQ users, and ICQ 
	 * users receive messages from AIM users.  It also lets ICQ users show 
	 * up in buddy lists for AIM users, and AIM users show up in buddy lists 
	 * for ICQ users.  And ICQ privacy/invisibility acts like AIM privacy, 
	 * in that if you add a user to your deny list, you will not be able to 
	 * see them as online (previous you could still see them, but they 
	 * couldn't see you.
	 */
	{AIM_CAPS_INTEROPERATE,
	 {0x09, 0x46, 0x13, 0x4d, 0x4c, 0x7f, 0x11, 0xd1,
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	{AIM_CAPS_ICQUTF8,
	 {0x09, 0x46, 0x13, 0x4e, 0x4c, 0x7f, 0x11, 0xd1,
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	{AIM_CAPS_ICQUTF8OLD,
	 {0x2e, 0x7a, 0x64, 0x75, 0xfa, 0xdf, 0x4d, 0xc8,
	  0x88, 0x6f, 0xea, 0x35, 0x95, 0xfd, 0xb6, 0xdf}},

	/*
	 * Chat is oddball.
	 */
	{AIM_CAPS_CHAT,
	 {0x74, 0x8f, 0x24, 0x20, 0x62, 0x87, 0x11, 0xd1, 
	  0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},

	/*
	{AIM_CAPS_ICQ2GO,
	 {0x56, 0x3f, 0xc8, 0x09, 0x0b, 0x6f, 0x41, 0xbd,
	  0x9f, 0x79, 0x42, 0x26, 0x09, 0xdf, 0xa2, 0xf3}},
	*/

	{AIM_CAPS_ICQRTF,
	 {0x97, 0xb1, 0x27, 0x51, 0x24, 0x3c, 0x43, 0x34,
	  0xad, 0x22, 0xd6, 0xab, 0xf7, 0x3f, 0x14, 0x92}},

	/* This is added by the servers and it only shows up for ourselves... */
	{AIM_CAPS_GENERICUNKNOWN,
	 {0x97, 0xb1, 0x27, 0x51, 0x24, 0x3c, 0x43, 0x34, 
	  0xad, 0x22, 0xd6, 0xab, 0xf7, 0x3f, 0x14, 0x09}},

	{AIM_CAPS_APINFO, 
	 {0xaa, 0x4a, 0x32, 0xb5, 0xf8, 0x84, 0x48, 0xc6,
	  0xa3, 0xd7, 0x8c, 0x50, 0x97, 0x19, 0xfd, 0x5b}},

	{AIM_CAPS_TRILLIANCRYPT,
	 {0xf2, 0xe7, 0xc7, 0xf4, 0xfe, 0xad, 0x4d, 0xfb,
	  0xb2, 0x35, 0x36, 0x79, 0x8b, 0xdf, 0x00, 0x00}},

	{AIM_CAPS_EMPTY,
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},

	{AIM_CAPS_LAST}
};

/*
 * Add the userinfo to our linked list.  If we already have userinfo 
 * for this buddy, then just overwrite parts of the old data.
 * @param userinfo Contains the new information for the buddy.
 */
static void aim_locate_adduserinfo(aim_session_t *sess, aim_userinfo_t *userinfo) {
	aim_userinfo_t *cur;

	cur = aim_locate_finduserinfo(sess, userinfo->sn);

	if (cur == NULL) {
		cur = (aim_userinfo_t *)calloc(1, sizeof(aim_userinfo_t));
		cur->sn = strdup(userinfo->sn);
		cur->next = sess->locate.userinfo;
		sess->locate.userinfo = cur;
	}

	cur->warnlevel = userinfo->warnlevel;
	cur->idletime = userinfo->idletime;
	if (userinfo->flags != 0)
		cur->flags = userinfo->flags;
	if (userinfo->createtime != 0)
		cur->createtime = userinfo->createtime;
	if (userinfo->membersince != 0)
		cur->membersince = userinfo->membersince;
	if (userinfo->onlinesince != 0)
		cur->onlinesince = userinfo->onlinesince;
	if (userinfo->sessionlen != 0)
		cur->sessionlen = userinfo->sessionlen;
	if (userinfo->capabilities != 0)
		cur->capabilities = userinfo->capabilities;
	cur->present |= userinfo->present;

	if (userinfo->iconcsumlen > 0) {
		free(cur->iconcsum);
		cur->iconcsum = (fu8_t *)malloc(userinfo->iconcsumlen);
		memcpy(cur->iconcsum, userinfo->iconcsum, userinfo->iconcsumlen);
		cur->iconcsumlen = userinfo->iconcsumlen;
	}

	if (userinfo->info != NULL) {
		free(cur->info);
		free(cur->info_encoding);
		cur->info = (char *)malloc(userinfo->info_len);
		memcpy(cur->info, userinfo->info, userinfo->info_len);
		cur->info_encoding = strdup(userinfo->info_encoding);
		cur->info_len = userinfo->info_len;
	}

	if (userinfo->away != NULL) {
		free(cur->away);
		free(cur->away_encoding);
		cur->away = (char *)malloc(userinfo->away_len);
		memcpy(cur->away, userinfo->away, userinfo->away_len);
		cur->away_encoding = strdup(userinfo->away_encoding);
		cur->away_len = userinfo->away_len;
	}
}

static void aim_locate_dorequest(aim_session_t *sess) {
	struct userinfo_node *cur = sess->locate.torequest;

	if (cur == NULL)
		return;

	if (sess->locate.waiting_for_response == TRUE)
		return;

	sess->locate.waiting_for_response = TRUE;
	aim_locate_getinfoshort(sess, cur->sn, 0x00000003);

	/* Move this node to the "requested" queue */
	sess->locate.torequest = cur->next;
	cur->next = sess->locate.requested;
	sess->locate.requested = cur;
}

/**
 * Remove this screen name from our queue.  If this info was resquested 
 * by our info request queue, then pop the next element off of the queue.
 *
 * @param sess The aim session.
 * @param sn Screen name of the info we just received.
 * @return True if the request was explicit (client requested the info), 
 *         false if the request was implicit (libfaim request the info).
 */
static int aim_locate_gotuserinfo(aim_session_t *sess, const char *sn) {
	struct userinfo_node *cur, *del;
	int was_explicit = TRUE;

	while ((sess->locate.requested != NULL) && (aim_sncmp(sn, sess->locate.requested->sn) == 0)) {
		del = sess->locate.requested;
		sess->locate.requested = del->next;
		was_explicit = FALSE;
		free(del->sn);
		free(del);
	}

	cur = sess->locate.requested;
	while ((cur != NULL) && (cur->next != NULL)) {
		if (aim_sncmp(sn, cur->next->sn) == 0) {
			del = cur->next;
			cur->next = del->next;
			was_explicit = FALSE;
			free(del->sn);
			free(del);
		} else
			cur = cur->next;
	}

	if (!was_explicit) {
		sess->locate.waiting_for_response = FALSE;
		aim_locate_dorequest(sess);
	}

	return was_explicit;
}

faim_internal void aim_locate_requestuserinfo(aim_session_t *sess, const char *sn) {
	struct userinfo_node *cur;

	/* Make sure we aren't already requesting info for this buddy */
	cur = sess->locate.torequest;
	while (cur != NULL) {
		if (aim_sncmp(sn, cur->sn) == 0)
			return;
		cur = cur->next;
	}

	/* Add a new node to our request queue */
	cur = (struct userinfo_node *)malloc(sizeof(struct userinfo_node));
	cur->sn = strdup(sn);
	cur->next = sess->locate.torequest;
	sess->locate.torequest = cur;

	/* Actually request some info up in this piece */
	aim_locate_dorequest(sess);
}

faim_export aim_userinfo_t *aim_locate_finduserinfo(aim_session_t *sess, const char *sn) {
	aim_userinfo_t *cur = sess->locate.userinfo;

	while (cur != NULL) {
		if (aim_sncmp(cur->sn, sn) == 0)
			return cur;
		cur = cur->next;
	}

	return NULL;
}

faim_internal fu32_t aim_locate_getcaps(aim_session_t *sess, aim_bstream_t *bs, int len)
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

		if (!identified)
			faimdprintf(sess, 0, "unknown capability: {%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
					cap[0], cap[1], cap[2], cap[3],
					cap[4], cap[5],
					cap[6], cap[7],
					cap[8], cap[9],
					cap[10], cap[11], cap[12], cap[13],
					cap[14], cap[15]);

		free(cap);
	}

	return flags;
}

faim_internal fu32_t aim_locate_getcaps_short(aim_session_t *sess, aim_bstream_t *bs, int len)
{
	fu32_t flags = 0;
	int offset;

	for (offset = 0; aim_bstream_empty(bs) && (offset < len); offset += 0x02) {
		fu8_t *cap;
		int i, identified;

		cap = aimbs_getraw(bs, 0x02);

		for (i = 0, identified = 0; !(aim_caps[i].flag & AIM_CAPS_LAST); i++) {
			if (memcmp(&aim_caps[i].data[2], cap, 0x02) == 0) {
				flags |= aim_caps[i].flag;
				identified++;
				break; /* should only match once... */
			}
		}

		if (!identified)
			faimdprintf(sess, 0, "unknown short capability: {%02x%02x}\n", cap[0], cap[1]);

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

faim_internal void aim_info_free(aim_userinfo_t *info)
{
	free(info->sn);
	free(info->iconcsum);
	free(info->info);
	free(info->info_encoding);
	free(info->avail);
	free(info->avail_encoding);
	free(info->away);
	free(info->away_encoding);
}

/*
 * AIM is fairly regular about providing user info.  This is a generic 
 * routine to extract it in its standard form.
 */
faim_internal int aim_info_extract(aim_session_t *sess, aim_bstream_t *bs, aim_userinfo_t *outinfo)
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
	outinfo->sn = aimbs_getstr(bs, snlen);

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
			 * Number of minutes since the user actively used the 
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

		} else if (type == 0x0008) {
			/*
			 * Type = 0x0008
			 *
			 * Client type, or some such.
			 */

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
			 * Format is:
			 * 4 bytes - Our IP address, 0xc0 a8 01 2b for 192.168.1.43
			 * 
			 *
			 */
			aimbs_getrawbuf(bs, outinfo->icqinfo.crap, 0x25);
			outinfo->present |= AIM_USERINFO_PRESENT_ICQDATA;

		} else if (type == 0x000d) {
			/*
			 * Type = 0x000d
			 *
			 * OSCAR Capability information.
			 *
			 */
			outinfo->capabilities |= aim_locate_getcaps(sess, bs, length);
			outinfo->present |= AIM_USERINFO_PRESENT_CAPABILITIES;

		} else if (type == 0x000e) {
			/*
			 * Type = 0x000e
			 *
			 * AOL capability information.
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

		} else if (type == 0x0019) {
			/*
			 * Type = 0x0019
			 *
			 * OSCAR short capability information.  A shortened 
			 * form of the normal capabilities.
			 */
			outinfo->capabilities |= aim_locate_getcaps_short(sess, bs, length);
			outinfo->present |= AIM_USERINFO_PRESENT_CAPABILITIES;

		} else if (type == 0x001b) {
			/*
			 * Type = 0x001a
			 *
			 * AOL short capability information.  A shortened 
			 * form of the normal capabilities.
			 */

		} else if (type == 0x001b) {
			/*
			 * Type = 0x0019
			 *
			 * Encryption certification MD5 checksum.
			 */

		} else if (type == 0x001d) {
			/*
			 * Type = 0x001d
			 *
			 * Buddy icon information and available messages.
			 *
			 * This almost seems like the AIM protocol guys gave 
			 * the iChat guys a Type, and the iChat guys tried to 
			 * cram as much cool shit into it as possible.  Then 
			 * the Windows AIM guys were like, "hey, that's 
			 * pretty neat, let's copy those prawns."
			 *
			 * In that spirit, this can contain a custom message, 
			 * kind of like an away message, but you're not away 
			 * (it's called an "available" message).  Or it can 
			 * contain information about the buddy icon the user 
			 * has stored on the server.
			 */
			int type2, number, length2;

			while (aim_bstream_curpos(bs) < endpos) {
				type2 = aimbs_get16(bs);
				number = aimbs_get8(bs);
				length2 = aimbs_get8(bs);

				switch (type2) {
					case 0x0000: { /* This is an official buddy icon? */
						/* This is always 5 bytes of "0x02 01 d2 04 72"? */
						aim_bstream_advance(bs, length2);
					} break;

					case 0x0001: { /* A buddy icon checksum */
						if ((length2 > 0) && (number == 0x01)) {
							free(outinfo->iconcsum);
							outinfo->iconcsum = aimbs_getraw(bs, length2);
							outinfo->iconcsumlen = length2;
						} else
							aim_bstream_advance(bs, length2);
					} break;

					case 0x0002: { /* An available message */
						if (length2 > 4) {
							free(outinfo->avail);
							outinfo->avail_len = aimbs_get16(bs);
							outinfo->avail = aimbs_getstr(bs, outinfo->avail_len);
							if (aimbs_get16(bs) == 0x0001) { /* We have an encoding */
								aimbs_get16(bs);
								outinfo->avail_encoding = aimbs_getstr(bs, aimbs_get16(bs));
							} else {
								/* No explicit encoding, client should use UTF-8 */
								outinfo->avail_encoding = NULL;
							}
						} else
							aim_bstream_advance(bs, length2);
					} break;

					default: {
						aim_bstream_advance(bs, length2);
					} break;
				}
			}

		} else if (type == 0x001e) {
			/*
			 * Type 30: Unknown.
			 *
			 * Always four bytes, but it doesn't look like an int.
			 */

		} else if (type == 0x001f) {
			/*
			 * Type 31: Unknown.
			 *
			 * Seen on a buddy using DeadAIM.  Data was 4 bytes:
			 * 0x00 00 00 10
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

	aim_locate_adduserinfo(sess, outinfo);

	return 0;
}

/*
 * Inverse of aim_info_extract()
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
		aim_tlvlist_add_16(&tlvlist, 0x0001, info->flags);
	if (info->present & AIM_USERINFO_PRESENT_MEMBERSINCE)
		aim_tlvlist_add_32(&tlvlist, 0x0002, info->membersince);
	if (info->present & AIM_USERINFO_PRESENT_ONLINESINCE)
		aim_tlvlist_add_32(&tlvlist, 0x0003, info->onlinesince);
	if (info->present & AIM_USERINFO_PRESENT_IDLE)
		aim_tlvlist_add_16(&tlvlist, 0x0004, info->idletime);

/* XXX - So, ICQ_OSCAR_SUPPORT is never defined anywhere... */
#if ICQ_OSCAR_SUPPORT
	if (atoi(info->sn) != 0) {
		if (info->present & AIM_USERINFO_PRESENT_ICQEXTSTATUS)
			aim_tlvlist_add_16(&tlvlist, 0x0006, info->icqinfo.status);
		if (info->present & AIM_USERINFO_PRESENT_ICQIPADDR)
			aim_tlvlist_add_32(&tlvlist, 0x000a, info->icqinfo.ipaddr);
	}
#endif

	if (info->present & AIM_USERINFO_PRESENT_CAPABILITIES)
		aim_tlvlist_add_caps(&tlvlist, 0x000d, info->capabilities);
 
	if (info->present & AIM_USERINFO_PRESENT_SESSIONLEN)
		aim_tlvlist_add_32(&tlvlist, (fu16_t)((info->flags & AIM_FLAG_AOL) ? 0x0010 : 0x000f), info->sessionlen);

	aimbs_put16(bs, aim_tlvlist_count(&tlvlist));
	aim_tlvlist_write(bs, &tlvlist);
	aim_tlvlist_free(&tlvlist);

	return 0;
}

/*
 * Subtype 0x0001
 */
static int error(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	aim_snac_t *snac2;
	fu16_t reason;
	char *sn;
	int was_explicit;

	if (!(snac2 = aim_remsnac(sess, snac->id))) {
		faimdprintf(sess, 0, "faim: locate.c, error(): received response from unknown request!\n");
		return 0;
	}

	if ((snac2->family != 0x0002) && (snac2->type != 0x0015)) {
		faimdprintf(sess, 0, "faim: locate.c, error(): received response from invalid request! %d\n", snac2->family);
		return 0;
	}

	if (!(sn = snac2->data)) {
		faimdprintf(sess, 0, "faim: locate.c, error(): received response from request without a screen name!\n");
		return 0;
	}

	reason = aimbs_get16(bs);

	/*
	 * Remove this screen name from our queue.  If the client requested 
	 * this buddy's info explicitly, then notify them that we do not have 
	 * info for this buddy.
	 */
	was_explicit = aim_locate_gotuserinfo(sess, sn);
	if (was_explicit == TRUE)
		if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
			ret = userfunc(sess, rx, reason, sn);

	if (snac2)
		free(snac2->data);
	free(snac2);

	return ret;
}

/*
 * Subtype 0x0002
 *
 * Request Location services rights.
 *
 */
faim_export int aim_locate_reqrights(aim_session_t *sess)
{
	aim_conn_t *conn;

	if (!sess || !(conn = aim_conn_findbygroup(sess, AIM_CB_FAM_LOC)))
		return -EINVAL;

	return aim_genericreq_n_snacid(sess, conn, AIM_CB_FAM_LOC, AIM_CB_LOC_REQRIGHTS);
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

	tlvlist = aim_tlvlist_read(bs);

	if (aim_tlv_gettlv(tlvlist, 0x0001, 1))
		maxsiglen = aim_tlv_get16(tlvlist, 0x0001, 1);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, maxsiglen);

	aim_tlvlist_free(&tlvlist);

	return ret;
}

/*
 * Subtype 0x0004
 *
 * Gives BOS your profile.
 *
 * profile_encoding and awaymsg_encoding MUST be set if profile or
 * away are set, respectively, and their value may or may not be
 * restricted to a few choices.  I am currently aware of:
 * 
 * us-ascii		Just that
 * unicode-2-0		UCS2-BE
 * 
 * profile_len and awaymsg_len MUST be set similarly, and they MUST
 * be the length of their respective strings in bytes.
 *
 * To get the previous behavior of awaymsg == "" un-setting the away
 * message, set awaymsg non-NULL and awaymsg_len to 0 (this is the
 * obvious equivalent).
 * 
 */
faim_export int aim_locate_setprofile(aim_session_t *sess,
				  const char *profile_encoding, const char *profile, const int profile_len,
				  const char *awaymsg_encoding, const char *awaymsg, const int awaymsg_len)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	aim_tlvlist_t *tl = NULL;
	char *encoding;
	static const char defencoding[] = {"text/aolrtf; charset=\"%s\""};

	if (!sess || !(conn = aim_conn_findbygroup(sess, AIM_CB_FAM_LOC)))
		return -EINVAL;

	if (!profile && !awaymsg)
		return -EINVAL;

	if ((profile && profile_encoding == NULL) || (awaymsg && awaymsg_len && awaymsg_encoding == NULL)) {
		return -EINVAL;
	}

	/* Build the packet first to get real length */
	if (profile) {
		/* no + 1 here because of %s */
		encoding = malloc(strlen(defencoding) + strlen(profile_encoding));
		if (encoding == NULL) {
			return -ENOMEM;
		}
		snprintf(encoding, strlen(defencoding) + strlen(profile_encoding), defencoding, profile_encoding);
		aim_tlvlist_add_raw(&tl, 0x0001, strlen(encoding), encoding);
		aim_tlvlist_add_raw(&tl, 0x0002, profile_len, profile);
		free(encoding);
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
		if (awaymsg_len) {
			encoding = malloc(strlen(defencoding) + strlen(awaymsg_encoding));
			if (encoding == NULL) {
				return -ENOMEM;
			}
			snprintf(encoding, strlen(defencoding) + strlen(awaymsg_encoding), defencoding, awaymsg_encoding);
			aim_tlvlist_add_raw(&tl, 0x0003, strlen(encoding), encoding);
			aim_tlvlist_add_raw(&tl, 0x0004, awaymsg_len, awaymsg);
			free(encoding);
		} else
			aim_tlvlist_add_noval(&tl, 0x0004);
	}

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10 + aim_tlvlist_size(&tl))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0002, 0x0004, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0002, 0x004, 0x0000, snacid);

	aim_tlvlist_write(&fr->data, &tl);
	aim_tlvlist_free(&tl);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x0004 - Set your client's capabilities.
 */
faim_export int aim_locate_setcaps(aim_session_t *sess, fu32_t caps)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	aim_tlvlist_t *tl = NULL;

	if (!sess || !(conn = aim_conn_findbygroup(sess, AIM_CB_FAM_LOC)))
		return -EINVAL;

	aim_tlvlist_add_caps(&tl, 0x0005, caps);

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10 + aim_tlvlist_size(&tl))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0002, 0x0004, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0002, 0x004, 0x0000, snacid);

	aim_tlvlist_write(&fr->data, &tl);
	aim_tlvlist_free(&tl);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x0005 - Request info of another AIM user.
 *
 * @param sn The screenname whose info you wish to request.
 * @param infotype The type of info you wish to request.
 *        0x0001 - Info/profile
 *        0x0003 - Away message
 *        0x0004 - Capabilities
 */
faim_export int aim_locate_getinfo(aim_session_t *sess, const char *sn, fu16_t infotype)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sess || !(conn = aim_conn_findbygroup(sess, AIM_CB_FAM_LOC)) || !sn)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 12+1+strlen(sn))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0002, 0x0005, 0x0000, NULL, 0);
	
	aim_putsnac(&fr->data, 0x0002, 0x0005, 0x0000, snacid);
	aimbs_put16(&fr->data, infotype);
	aimbs_put8(&fr->data, strlen(sn));
	aimbs_putraw(&fr->data, sn, strlen(sn));

	aim_tx_enqueue(sess, fr);

	return 0;
}

/* Subtype 0x0006 */
static int userinfo(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	aim_userinfo_t *userinfo, *userinfo2;
	aim_tlvlist_t *tlvlist;
	aim_tlv_t *tlv = NULL;
	int was_explicit;

	userinfo = (aim_userinfo_t *)malloc(sizeof(aim_userinfo_t));
	aim_info_extract(sess, bs, userinfo);
	tlvlist = aim_tlvlist_read(bs);

	/* Profile will be 1 and 2 */
	userinfo->info_encoding = aim_tlv_getstr(tlvlist, 0x0001, 1);
	if ((tlv = aim_tlv_gettlv(tlvlist, 0x0002, 1))) {
		userinfo->info = (char *)malloc(tlv->length);
		memcpy(userinfo->info, tlv->value, tlv->length);
		userinfo->info_len = tlv->length;
	}

	/* Away message will be 3 and 4 */
	userinfo->away_encoding = aim_tlv_getstr(tlvlist, 0x0003, 1);
	if ((tlv = aim_tlv_gettlv(tlvlist, 0x0004, 1))) {
		userinfo->away = (char *)malloc(tlv->length);
		memcpy(userinfo->away, tlv->value, tlv->length);
		userinfo->away_len = tlv->length;
	}

	/* Caps will be 5 */
	if ((tlv = aim_tlv_gettlv(tlvlist, 0x0005, 1))) {
		aim_bstream_t cbs;
		aim_bstream_init(&cbs, tlv->value, tlv->length);
		userinfo->capabilities = aim_locate_getcaps(sess, &cbs, tlv->length);
		userinfo->present = AIM_USERINFO_PRESENT_CAPABILITIES;
	}
	aim_tlvlist_free(&tlvlist);

	aim_locate_adduserinfo(sess, userinfo);
	userinfo2 = aim_locate_finduserinfo(sess, userinfo->sn);
	aim_info_free(userinfo);
	free(userinfo);

	/*
	 * Remove this screen name from our queue.  If the client requested 
	 * this buddy's info explicitly, then notify them that we have info 
	 * for this buddy.
	 */
	was_explicit = aim_locate_gotuserinfo(sess, userinfo2->sn);
	if (was_explicit == TRUE)
		if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
			ret = userfunc(sess, rx, userinfo2);

	return ret;
}

/* 
 * Subtype 0x0009 - Set directory profile data.
 *
 * This is not the same as aim_location_setprofile!
 * privacy: 1 to allow searching, 0 to disallow.
 *
 */
faim_export int aim_locate_setdirinfo(aim_session_t *sess, const char *first, const char *middle, const char *last, const char *maiden, const char *nickname, const char *street, const char *city, const char *state, const char *zip, int country, fu16_t privacy) 
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	aim_tlvlist_t *tl = NULL;

	if (!sess || !(conn = aim_conn_findbygroup(sess, AIM_CB_FAM_LOC)))
		return -EINVAL;

	aim_tlvlist_add_16(&tl, 0x000a, privacy);

	if (first)
		aim_tlvlist_add_raw(&tl, 0x0001, strlen(first), first);
	if (last)
		aim_tlvlist_add_raw(&tl, 0x0002, strlen(last), last);
	if (middle)
		aim_tlvlist_add_raw(&tl, 0x0003, strlen(middle), middle);
	if (maiden)
		aim_tlvlist_add_raw(&tl, 0x0004, strlen(maiden), maiden);

	if (state)
		aim_tlvlist_add_raw(&tl, 0x0007, strlen(state), state);
	if (city)
		aim_tlvlist_add_raw(&tl, 0x0008, strlen(city), city);

	if (nickname)
		aim_tlvlist_add_raw(&tl, 0x000c, strlen(nickname), nickname);
	if (zip)
		aim_tlvlist_add_raw(&tl, 0x000d, strlen(zip), zip);

	if (street)
		aim_tlvlist_add_raw(&tl, 0x0021, strlen(street), street);

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+aim_tlvlist_size(&tl))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0002, 0x0009, 0x0000, NULL, 0);

	aim_putsnac(&fr->data, 0x0002, 0x0009, 0x0000, snacid);
	aim_tlvlist_write(&fr->data, &tl);
	aim_tlvlist_free(&tl);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x000b - Huh? What is this?
 */
faim_export int aim_locate_000b(aim_session_t *sess, const char *sn)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;

		return -EINVAL;

	if (!sess || !(conn = aim_conn_findbygroup(sess, AIM_CB_FAM_LOC)) || !sn)
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
 * Subtype 0x000f
 * 
 * XXX pass these in better
 *
 */
faim_export int aim_locate_setinterests(aim_session_t *sess, const char *interest1, const char *interest2, const char *interest3, const char *interest4, const char *interest5, fu16_t privacy)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	aim_tlvlist_t *tl = NULL;

	if (!sess || !(conn = aim_conn_findbygroup(sess, AIM_CB_FAM_LOC)))
		return -EINVAL;

	/* ?? privacy ?? */
	aim_tlvlist_add_16(&tl, 0x000a, privacy);

	if (interest1)
		aim_tlvlist_add_raw(&tl, 0x0000b, strlen(interest1), interest1);
	if (interest2)
		aim_tlvlist_add_raw(&tl, 0x0000b, strlen(interest2), interest2);
	if (interest3)
		aim_tlvlist_add_raw(&tl, 0x0000b, strlen(interest3), interest3);
	if (interest4)
		aim_tlvlist_add_raw(&tl, 0x0000b, strlen(interest4), interest4);
	if (interest5)
		aim_tlvlist_add_raw(&tl, 0x0000b, strlen(interest5), interest5);

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+aim_tlvlist_size(&tl))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0002, 0x000f, 0x0000, NULL, 0);

	aim_putsnac(&fr->data, 0x0002, 0x000f, 0x0000, 0);
	aim_tlvlist_write(&fr->data, &tl);
	aim_tlvlist_free(&tl);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x0015 - Request the info a user using the short method.  This is 
 * what iChat uses.  It normally is VERY leniently rate limited.
 *
 * @param sn The screen name whose info you wish to request.
 * @param flags The bitmask which specifies the type of info you wish to request.
 *        0x00000001 - Info/profile.
 *        0x00000002 - Away message.
 *        0x00000004 - Capabilities.
 *        0x00000008 - Certification.
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_locate_getinfoshort(aim_session_t *sess, const char *sn, fu32_t flags)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sess || !(conn = aim_conn_findbygroup(sess, AIM_CB_FAM_LOC)) || !sn)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+4+1+strlen(sn))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0002, 0x0015, 0x0000, sn, strlen(sn)+1);

	aim_putsnac(&fr->data, 0x0002, 0x0015, 0x0000, snacid);
	aimbs_put32(&fr->data, flags);
	aimbs_put8(&fr->data, strlen(sn));
	aimbs_putraw(&fr->data, sn, strlen(sn));

	aim_tx_enqueue(sess, fr);

	return 0;
}

static int snachandler(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{

	if (snac->subtype == 0x0001)
		return error(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x0003)
		return rights(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x0006)
		return userinfo(sess, mod, rx, snac, bs);

	return 0;
}

static void locate_shutdown(aim_session_t *sess, aim_module_t *mod)
{
	aim_userinfo_t *del;

	while (sess->locate.userinfo) {
		del = sess->locate.userinfo;
		sess->locate.userinfo = sess->locate.userinfo->next;
		aim_info_free(del);
		free(del);
	}
}

faim_internal int locate_modfirst(aim_session_t *sess, aim_module_t *mod)
{

	mod->family = AIM_CB_FAM_LOC;
	mod->version = 0x0001;
	mod->toolid = 0x0110;
	mod->toolversion = 0x0629;
	mod->flags = 0;
	strncpy(mod->name, "locate", sizeof(mod->name));
	mod->snachandler = snachandler;
	mod->shutdown = locate_shutdown;

	return 0;
}
