/*
 * Family 0x0003 - Old-style Buddylist Management (non-SSI).
 *
 */

#define FAIM_INTERNAL
#include <aim.h>

/*
 * Subtype 0x0002 - Request rights.
 *
 * Request Buddy List rights.
 *
 */
faim_export int aim_bos_reqbuddyrights(aim_session_t *sess, aim_conn_t *conn)
{
	return aim_genericreq_n(sess, conn, 0x0003, 0x0002);
}

/*
 * Subtype 0x0003 - Rights.
 *
 */
static int rights(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	aim_rxcallback_t userfunc;
	aim_tlvlist_t *tlvlist;
	fu16_t maxbuddies = 0, maxwatchers = 0;
	int ret = 0;

	/* 
	 * TLVs follow 
	 */
	tlvlist = aim_readtlvchain(bs);

	/*
	 * TLV type 0x0001: Maximum number of buddies.
	 */
	if (aim_gettlv(tlvlist, 0x0001, 1))
		maxbuddies = aim_gettlv16(tlvlist, 0x0001, 1);

	/*
	 * TLV type 0x0002: Maximum number of watchers.
	 *
	 * Watchers are other users who have you on their buddy
	 * list.  (This is called the "reverse list" by a certain
	 * other IM protocol.)
	 * 
	 */
	if (aim_gettlv(tlvlist, 0x0002, 1))
		maxwatchers = aim_gettlv16(tlvlist, 0x0002, 1);

	/*
	 * TLV type 0x0003: Unknown.
	 *
	 * ICQ only?
	 */

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, maxbuddies, maxwatchers);

	aim_freetlvchain(&tlvlist);

	return ret;  
}

/*
 * Subtype 0x0004 - Add buddy to list.
 *
 * Adds a single buddy to your buddy list after login.
 * XXX This should just be an extension of setbuddylist()
 *
 */
faim_export int aim_add_buddy(aim_session_t *sess, aim_conn_t *conn, const char *sn)
{
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sn || !strlen(sn))
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+1+strlen(sn))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0003, 0x0004, 0x0000, sn, strlen(sn)+1);
	aim_putsnac(&fr->data, 0x0003, 0x0004, 0x0000, snacid);

	aimbs_put8(&fr->data, strlen(sn));
	aimbs_putraw(&fr->data, sn, strlen(sn));

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x0004 - Add multiple buddies to your buddy list.
 *
 * This just builds the "set buddy list" command then queues it.
 *
 * buddy_list = "Screen Name One&ScreenNameTwo&";
 *
 * XXX Clean this up.  
 *
 */
faim_export int aim_bos_setbuddylist(aim_session_t *sess, aim_conn_t *conn, const char *buddy_list)
{
	aim_frame_t *fr;
	aim_snacid_t snacid;
	int len = 0;
	char *localcpy = NULL;
	char *tmpptr = NULL;

	if (!buddy_list || !(localcpy = strdup(buddy_list))) 
		return -EINVAL;

	for (tmpptr = strtok(localcpy, "&"); tmpptr; ) {
		faimdprintf(sess, 2, "---adding: %s (%d)\n", tmpptr, strlen(tmpptr));
		len += 1 + strlen(tmpptr);
		tmpptr = strtok(NULL, "&");
	}

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+len)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0003, 0x0004, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0003, 0x0004, 0x0000, snacid);

	strncpy(localcpy, buddy_list, strlen(buddy_list) + 1);

	for (tmpptr = strtok(localcpy, "&"); tmpptr; ) {

		faimdprintf(sess, 2, "---adding: %s (%d)\n", tmpptr, strlen(tmpptr));

		aimbs_put8(&fr->data, strlen(tmpptr));
		aimbs_putraw(&fr->data, tmpptr, strlen(tmpptr));
		tmpptr = strtok(NULL, "&");
	}

	aim_tx_enqueue(sess, fr);

	free(localcpy);

	return 0;
}

/*
 * Subtype 0x0005 - Remove buddy from list.
 *
 * XXX generalise to support removing multiple buddies (basically, its
 * the same as setbuddylist() but with a different snac subtype).
 *
 */
faim_export int aim_remove_buddy(aim_session_t *sess, aim_conn_t *conn, const char *sn)
{
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sn || !strlen(sn))
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+1+strlen(sn))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0003, 0x0005, 0x0000, sn, strlen(sn)+1);
	aim_putsnac(&fr->data, 0x0003, 0x0005, 0x0000, snacid);

	aimbs_put8(&fr->data, strlen(sn));
	aimbs_putraw(&fr->data, sn, strlen(sn));

	aim_tx_enqueue(sess, fr);

	return 0;
}

/* 
 * Subtype 0x000b
 *
 * XXX Why would we send this?
 *
 */
faim_export int aim_sendbuddyoncoming(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *info)
{
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sess || !conn || !info)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 1152)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0003, 0x000b, 0x0000, NULL, 0);
	
	aim_putsnac(&fr->data, 0x0003, 0x000b, 0x0000, snacid);
	aim_putuserinfo(&fr->data, info);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/* 
 * Subtype 0x000c
 *
 * XXX Why would we send this?
 *
 */
faim_export int aim_sendbuddyoffgoing(aim_session_t *sess, aim_conn_t *conn, const char *sn)
{
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sess || !conn || !sn)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+1+strlen(sn))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0003, 0x000c, 0x0000, NULL, 0);
	
	aim_putsnac(&fr->data, 0x0003, 0x000c, 0x0000, snacid);
	aimbs_put8(&fr->data, strlen(sn));
	aimbs_putraw(&fr->data, sn, strlen(sn));

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtypes 0x000b and 0x000c - Change in buddy status
 *
 * Oncoming Buddy notifications contain a subset of the
 * user information structure.  Its close enough to run
 * through aim_extractuserinfo() however.
 *
 * Although the offgoing notification contains no information,
 * it is still in a format parsable by extractuserinfo.
 *
 */
static int buddychange(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	aim_userinfo_t userinfo;
	aim_rxcallback_t userfunc;

	aim_extractuserinfo(sess, bs, &userinfo);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		return userfunc(sess, rx, &userinfo);

	return 0;
}

static int snachandler(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{

	if (snac->subtype == 0x0003)
		return rights(sess, mod, rx, snac, bs);
	else if ((snac->subtype == 0x000b) || (snac->subtype == 0x000c))
		return buddychange(sess, mod, rx, snac, bs);

	return 0;
}

faim_internal int buddylist_modfirst(aim_session_t *sess, aim_module_t *mod)
{

	mod->family = 0x0003;
	mod->version = 0x0001;
	mod->toolid = 0x0110;
	mod->toolversion = 0x047b;
	mod->flags = 0;
	strncpy(mod->name, "buddylist", sizeof(mod->name));
	mod->snachandler = snachandler;

	return 0;
}
