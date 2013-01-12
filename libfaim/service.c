/*
 * Family 0x0001 - This is a very special group.  All connections support
 * this group, as it does some particularly good things (like rate limiting).
 */

#define FAIM_INTERNAL
#define FAIM_NEED_CONN_INTERNAL
#include <aim.h>

#include "md5.h"

/* Subtype 0x0002 - Client Online */
faim_export int aim_clientready(aim_session_t *sess, aim_conn_t *conn)
{
	aim_conn_inside_t *ins = (aim_conn_inside_t *)conn->inside;
	struct snacgroup *sg;
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!ins)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 1152)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0001, 0x0002, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0001, 0x0002, 0x0000, snacid);

	/*
	 * Send only the tool versions that the server cares about (that it
	 * marked as supporting in the server ready SNAC).
	 */
	for (sg = ins->groups; sg; sg = sg->next) {
		aim_module_t *mod;

		if ((mod = aim__findmodulebygroup(sess, sg->group))) {
			aimbs_put16(&fr->data, mod->family);
			aimbs_put16(&fr->data, mod->version);
			aimbs_put16(&fr->data, mod->toolid);
			aimbs_put16(&fr->data, mod->toolversion);
		} else
			faimdprintf(sess, 1, "aim_clientready: server supports group 0x%04x but we don't!\n", sg->group);
	}

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x0003 - Host Online
 *
 * See comments in conn.c about how the group associations are supposed
 * to work, and how they really work.
 *
 * This info probably doesn't even need to make it to the client.
 *
 * We don't actually call the client here.  This starts off the connection
 * initialization routine required by all AIM connections.  The next time
 * the client is called is the CONNINITDONE callback, which should be
 * shortly after the rate information is acknowledged.
 *
 */
static int hostonline(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	fu16_t *families;
	int famcount;


	if (!(families = malloc(aim_bstream_empty(bs))))
		return 0;

	for (famcount = 0; aim_bstream_empty(bs); famcount++) {
		families[famcount] = aimbs_get16(bs);
		aim_conn_addgroup(rx->conn, families[famcount]);
	}

	free(families);


	/*
	 * Next step is in the Host Versions handler.
	 *
	 * Note that we must send this before we request rates, since
	 * the format of the rate information depends on the versions we
	 * give it.
	 *
	 */
	aim_setversions(sess, rx->conn);

	return 1;
}

/* Subtype 0x0004 - Service request */
faim_export int aim_reqservice(aim_session_t *sess, aim_conn_t *conn, fu16_t serviceid)
{
	return aim_genericreq_s(sess, conn, 0x0001, 0x0004, &serviceid);
}

/* Subtype 0x0005 - Redirect */
static int redirect(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	struct aim_redirect_data redir;
	aim_rxcallback_t userfunc;
	aim_tlvlist_t *tlvlist;
	aim_snac_t *origsnac = NULL;
	int ret = 0;

	memset(&redir, 0, sizeof(redir));

	tlvlist = aim_tlvlist_read(bs);

	if (!aim_tlv_gettlv(tlvlist, 0x000d, 1) ||
			!aim_tlv_gettlv(tlvlist, 0x0005, 1) ||
			!aim_tlv_gettlv(tlvlist, 0x0006, 1)) {
		aim_tlvlist_free(&tlvlist);
		return 0;
	}

	redir.group = aim_tlv_get16(tlvlist, 0x000d, 1);
	redir.ip = aim_tlv_getstr(tlvlist, 0x0005, 1);
	redir.cookielen = aim_tlv_gettlv(tlvlist, 0x0006, 1)->length;
	redir.cookie = aim_tlv_getstr(tlvlist, 0x0006, 1);

	/* Fetch original SNAC so we can get csi if needed */
	origsnac = aim_remsnac(sess, snac->id);

	if ((redir.group == AIM_CONN_TYPE_CHAT) && origsnac) {
		struct chatsnacinfo *csi = (struct chatsnacinfo *)origsnac->data;

		redir.chat.exchange = csi->exchange;
		redir.chat.room = csi->name;
		redir.chat.instance = csi->instance;
	}

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, &redir);

	free((void *)redir.ip);
	free((void *)redir.cookie);

	if (origsnac)
		free(origsnac->data);
	free(origsnac);

	aim_tlvlist_free(&tlvlist);

	return ret;
}

/* Subtype 0x0006 - Request Rate Information. */
faim_internal int aim_reqrates(aim_session_t *sess, aim_conn_t *conn)
{
	return aim_genericreq_n_snacid(sess, conn, 0x0001, 0x0006);
}

/*
 * OSCAR defines several 'rate classes'.  Each class has seperate
 * rate limiting properties (limit level, alert level, disconnect
 * level, etc), and a set of SNAC family/type pairs associated with
 * it.  The rate classes, their limiting properties, and the definitions
 * of which SNACs are belong to which class, are defined in the
 * Rate Response packet at login to each host.
 *
 * Logically, all rate offenses within one class count against further
 * offenses for other SNACs in the same class (ie, sending messages
 * too fast will limit the number of user info requests you can send,
 * since those two SNACs are in the same rate class).
 *
 * Since the rate classes are defined dynamically at login, the values
 * below may change. But they seem to be fairly constant.
 *
 * Currently, BOS defines five rate classes, with the commonly used
 * members as follows...
 *
 *  Rate class 0x0001:
 *  	- Everything thats not in any of the other classes
 *
 *  Rate class 0x0002:
 * 	- Buddy list add/remove
 *	- Permit list add/remove
 *	- Deny list add/remove
 *
 *  Rate class 0x0003:
 *	- User information requests
 *	- Outgoing ICBMs
 *
 *  Rate class 0x0004:
 *	- A few unknowns: 2/9, 2/b, and f/2
 *
 *  Rate class 0x0005:
 *	- Chat room create
 *	- Outgoing chat ICBMs
 *
 * The only other thing of note is that class 5 (chat) has slightly looser
 * limiting properties than class 3 (normal messages).  But thats just a
 * small bit of trivia for you.
 *
 * The last thing that needs to be learned about the rate limiting
 * system is how the actual numbers relate to the passing of time.  This
 * seems to be a big mystery.
 *
 */

static void rc_addclass(struct rateclass **head, struct rateclass *inrc)
{
	struct rateclass *rc, *rc2;

	if (!(rc = malloc(sizeof(struct rateclass))))
		return;

	memcpy(rc, inrc, sizeof(struct rateclass));
	rc->next = NULL;

	for (rc2 = *head; rc2 && rc2->next; rc2 = rc2->next)
		;

	if (!rc2)
		*head = rc;
	else
		rc2->next = rc;

	return;
}

static struct rateclass *rc_findclass(struct rateclass **head, fu16_t id)
{
	struct rateclass *rc;

	for (rc = *head; rc; rc = rc->next) {
		if (rc->classid == id)
			return rc;
	}

	return NULL;
}

static void rc_addpair(struct rateclass *rc, fu16_t group, fu16_t type)
{
	struct snacpair *sp, *sp2;

	if (!(sp = malloc(sizeof(struct snacpair))))
		return;
	memset(sp, 0, sizeof(struct snacpair));

	sp->group = group;
	sp->subtype = type;
	sp->next = NULL;

	for (sp2 = rc->members; sp2 && sp2->next; sp2 = sp2->next)
		;

	if (!sp2)
		rc->members = sp;
	else
		sp2->next = sp;

	return;
}

/* Subtype 0x0007 - Rate Parameters */
static int rateresp(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	aim_conn_inside_t *ins = (aim_conn_inside_t *)rx->conn->inside;
	fu16_t numclasses, i;
	aim_rxcallback_t userfunc;


	/*
	 * First are the parameters for each rate class.
	 */
	numclasses = aimbs_get16(bs);
	for (i = 0; i < numclasses; i++) {
		struct rateclass rc;

		memset(&rc, 0, sizeof(struct rateclass));

		rc.classid = aimbs_get16(bs);
		rc.windowsize = aimbs_get32(bs);
		rc.clear = aimbs_get32(bs);
		rc.alert = aimbs_get32(bs);
		rc.limit = aimbs_get32(bs);
		rc.disconnect = aimbs_get32(bs);
		rc.current = aimbs_get32(bs);
		rc.max = aimbs_get32(bs);

		/*
		 * The server will send an extra five bytes of parameters
		 * depending on the version we advertised in 1/17.  If we
		 * didn't send 1/17 (evil!), then this will crash and you
		 * die, as it will default to the old version but we have
		 * the new version hardcoded here.
		 */
		if (mod->version >= 3)
			aimbs_getrawbuf(bs, rc.unknown, sizeof(rc.unknown));

		faimdprintf(sess, 1, "--- Adding rate class %d to connection type %d: window size = %ld, clear = %ld, alert = %ld, limit = %ld, disconnect = %ld, current = %ld, max = %ld\n", rx->conn->type, rc.classid, rc.windowsize, rc.clear, rc.alert, rc.limit, rc.disconnect, rc.current, rc.max);

		rc_addclass(&ins->rates, &rc);
	}

	/*
	 * Then the members of each class.
	 */
	for (i = 0; i < numclasses; i++) {
		fu16_t classid, count;
		struct rateclass *rc;
		int j;

		classid = aimbs_get16(bs);
		count = aimbs_get16(bs);

		rc = rc_findclass(&ins->rates, classid);

		for (j = 0; j < count; j++) {
			fu16_t group, subtype;

			group = aimbs_get16(bs);
			subtype = aimbs_get16(bs);

			if (rc)
				rc_addpair(rc, group, subtype);
		}
	}

	/*
	 * We don't pass the rate information up to the client, as it really
	 * doesn't care.  The information is stored in the connection, however
	 * so that we can do more fun stuff later (not really).
	 */

	/*
	 * Last step in the conn init procedure is to acknowledge that we
	 * agree to these draconian limitations.
	 */
	aim_rates_addparam(sess, rx->conn);

	/*
	 * Finally, tell the client it's ready to go...
	 */
	if ((userfunc = aim_callhandler(sess, rx->conn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE)))
		userfunc(sess, rx);


	return 1;
}

/* Subtype 0x0008 - Add Rate Parameter */
faim_internal int aim_rates_addparam(aim_session_t *sess, aim_conn_t *conn)
{
	aim_conn_inside_t *ins = (aim_conn_inside_t *)conn->inside;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	struct rateclass *rc;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 512)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0001, 0x0008, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0001, 0x0008, 0x0000, snacid);

	for (rc = ins->rates; rc; rc = rc->next)
		aimbs_put16(&fr->data, rc->classid);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/* Subtype 0x0009 - Delete Rate Parameter */
faim_internal int aim_rates_delparam(aim_session_t *sess, aim_conn_t *conn)
{
	aim_conn_inside_t *ins = (aim_conn_inside_t *)conn->inside;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	struct rateclass *rc;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 512)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0001, 0x0009, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0001, 0x0009, 0x0000, snacid);

	for (rc = ins->rates; rc; rc = rc->next)
		aimbs_put16(&fr->data, rc->classid);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/* Subtype 0x000a - Rate Change */
static int ratechange(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	fu16_t code, rateclass;
	fu32_t currentavg, maxavg, windowsize, clear, alert, limit, disconnect;

	code = aimbs_get16(bs);
	rateclass = aimbs_get16(bs);

	windowsize = aimbs_get32(bs);
	clear = aimbs_get32(bs);
	alert = aimbs_get32(bs);
	limit = aimbs_get32(bs);
	disconnect = aimbs_get32(bs);
	currentavg = aimbs_get32(bs);
	maxavg = aimbs_get32(bs);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, code, rateclass, windowsize, clear, alert, limit, disconnect, currentavg, maxavg);

	return ret;
}

/*
 * How Migrations work.
 *
 * The server sends a Server Pause message, which the client should respond to
 * with a Server Pause Ack, which contains the families it needs on this
 * connection. The server will send a Migration Notice with an IP address, and
 * then disconnect. Next the client should open the connection and send the
 * cookie.  Repeat the normal login process and pretend this never happened.
 *
 * The Server Pause contains no data.
 *
 */

/* Subtype 0x000b - Service Pause */
static int serverpause(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx);

	return ret;
}

/*
 * Subtype 0x000c - Service Pause Acknowledgement
 *
 * It is rather important that aim_sendpauseack() gets called for the exact
 * same connection that the Server Pause callback was called for, since
 * libfaim extracts the data for the SNAC from the connection structure.
 *
 * Of course, if you don't do that, more bad things happen than just what
 * libfaim can cause.
 *
 */
faim_export int aim_sendpauseack(aim_session_t *sess, aim_conn_t *conn)
{
	aim_frame_t *fr;
	aim_snacid_t snacid;
	aim_conn_inside_t *ins = (aim_conn_inside_t *)conn->inside;
	struct snacgroup *sg;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 1024)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0001, 0x000c, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0001, 0x000c, 0x0000, snacid);

	/*
	 * This list should have all the groups that the original
	 * Host Online / Server Ready said this host supports.  And
	 * we want them all back after the migration.
	 */
	for (sg = ins->groups; sg; sg = sg->next)
		aimbs_put16(&fr->data, sg->group);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/* Subtype 0x000d - Service Resume */
static int serverresume(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx);

	return ret;
}

/* Subtype 0x000e - Request self-info */
faim_export int aim_reqpersonalinfo(aim_session_t *sess, aim_conn_t *conn)
{
	return aim_genericreq_n_snacid(sess, conn, 0x0001, 0x000e);
}

/* Subtype 0x000f - Self User Info */
static int selfinfo(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	aim_userinfo_t userinfo;

	aim_info_extract(sess, bs, &userinfo);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, &userinfo);

	aim_info_free(&userinfo);

	return ret;
}

/* Subtype 0x0010 - Evil Notification */
static int evilnotify(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	fu16_t newevil;
	aim_userinfo_t userinfo;

	memset(&userinfo, 0, sizeof(aim_userinfo_t));

	newevil = aimbs_get16(bs);

	if (aim_bstream_empty(bs))
		aim_info_extract(sess, bs, &userinfo);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, newevil, &userinfo);

	aim_info_free(&userinfo);

	return ret;
}

/*
 * Subtype 0x0011 - Idle Notification
 *
 * Should set your current idle time in seconds.  Note that this should
 * never be called consecutively with a non-zero idle time.  That makes
 * OSCAR do funny things.  Instead, just set it once you go idle, and then
 * call it again with zero when you're back.
 *
 */
faim_export int aim_srv_setidle(aim_session_t *sess, fu32_t idletime)
{
	aim_conn_t *conn;

	if (!sess || !(conn = aim_conn_findbygroup(sess, AIM_CB_FAM_BOS)))
		return -EINVAL;

	return aim_genericreq_l(sess, conn, 0x0001, 0x0011, &idletime);
}

/*
 * Subtype 0x0012 - Service Migrate
 *
 * This is the final SNAC sent on the original connection during a migration.
 * It contains the IP and cookie used to connect to the new server, and
 * optionally a list of the SNAC groups being migrated.
 *
 */
static int migrate(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	aim_rxcallback_t userfunc;
	int ret = 0;
	fu16_t groupcount, i;
	aim_tlvlist_t *tl;
	char *ip = NULL;
	aim_tlv_t *cktlv;

	/*
	 * Apparently there's some fun stuff that can happen right here. The
	 * migration can actually be quite selective about what groups it
	 * moves to the new server.  When not all the groups for a connection
	 * are migrated, or they are all migrated but some groups are moved
	 * to a different server than others, it is called a bifurcated
	 * migration.
	 *
	 * Let's play dumb and not support that.
	 *
	 */
	groupcount = aimbs_get16(bs);
	for (i = 0; i < groupcount; i++) {
		fu16_t group;

		group = aimbs_get16(bs);

		faimdprintf(sess, 0, "bifurcated migration unsupported -- group 0x%04x\n", group);
	}

	tl = aim_tlvlist_read(bs);

	if (aim_tlv_gettlv(tl, 0x0005, 1))
		ip = aim_tlv_getstr(tl, 0x0005, 1);

	cktlv = aim_tlv_gettlv(tl, 0x0006, 1);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, ip, cktlv ? cktlv->value : NULL);

	aim_tlvlist_free(&tl);
	free(ip);

	return ret;
}

/* Subtype 0x0013 - Message of the Day */
static int motd(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	aim_rxcallback_t userfunc;
	char *msg = NULL;
	int ret = 0;
	aim_tlvlist_t *tlvlist;
	fu16_t id;

	/*
	 * Code.
	 *
	 * Valid values:
	 *   1 Mandatory upgrade
	 *   2 Advisory upgrade
	 *   3 System bulletin
	 *   4 Nothing's wrong ("top o the world" -- normal)
	 *   5 Lets-break-something.
	 *
	 */
	id = aimbs_get16(bs);

	/*
	 * TLVs follow
	 */
	tlvlist = aim_tlvlist_read(bs);

	msg = aim_tlv_getstr(tlvlist, 0x000b, 1);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, id, msg);

	free(msg);

	aim_tlvlist_free(&tlvlist);

	return ret;
}

/*
 * Subtype 0x0014 - Set privacy flags
 *
 * Normally 0x03.
 *
 *  Bit 1:  Allows other AIM users to see how long you've been idle.
 *  Bit 2:  Allows other AIM users to see how long you've been a member.
 *
 */
faim_export int aim_bos_setprivacyflags(aim_session_t *sess, aim_conn_t *conn, fu32_t flags)
{
	return aim_genericreq_l(sess, conn, 0x0001, 0x0014, &flags);
}

/*
 * Subtype 0x0016 - No-op
 *
 * WinAIM sends these every 4min or so to keep the connection alive.  Its not
 * really necessary.
 *
 * Wha?  No?  Since when?  I think WinAIM sends an empty channel 3
 * SNAC as a no-op...
 */
faim_export int aim_nop(aim_session_t *sess, aim_conn_t *conn)
{
	return aim_genericreq_n(sess, conn, 0x0001, 0x0016);
}

/*
 * Subtype 0x0017 - Set client versions
 *
 * If you've seen the clientonline/clientready SNAC you're probably
 * wondering what the point of this one is.  And that point seems to be
 * that the versions in the client online SNAC are sent too late for the
 * server to be able to use them to change the protocol for the earlier
 * login packets (client versions are sent right after Host Online is
 * received, but client online versions aren't sent until quite a bit later).
 * We can see them already making use of this by changing the format of
 * the rate information based on what version of group 1 we advertise here.
 *
 */
faim_internal int aim_setversions(aim_session_t *sess, aim_conn_t *conn)
{
	aim_conn_inside_t *ins = (aim_conn_inside_t *)conn->inside;
	struct snacgroup *sg;
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!ins)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 1152)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0001, 0x0017, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0001, 0x0017, 0x0000, snacid);

	/*
	 * Send only the versions that the server cares about (that it
	 * marked as supporting in the server ready SNAC).
	 */
	for (sg = ins->groups; sg; sg = sg->next) {
		aim_module_t *mod;

		if ((mod = aim__findmodulebygroup(sess, sg->group))) {
			aimbs_put16(&fr->data, mod->family);
			aimbs_put16(&fr->data, mod->version);
		} else
			faimdprintf(sess, 1, "aim_setversions: server supports group 0x%04x but we don't!\n", sg->group);
	}

	aim_tx_enqueue(sess, fr);

	return 0;
}

/* Subtype 0x0018 - Host versions */
static int hostversions(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int vercount;
	fu8_t *versions;

	/* This is frivolous. (Thank you SmarterChild.) */
	vercount = aim_bstream_empty(bs)/4;
	versions = aimbs_getraw(bs, aim_bstream_empty(bs));
	free(versions);

	/*
	 * Now request rates.
	 */
	aim_reqrates(sess, rx->conn);

	return 1;
}

/*
 * Subtype 0x001e - Extended Status
 *
 * Sets your ICQ status (available, away, do not disturb, etc.)
 *
 * These are the same TLVs seen in user info.  You can
 * also set 0x0008 and 0x000c.
 */
faim_export int aim_setextstatus(aim_session_t *sess, fu32_t status)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	aim_tlvlist_t *tl = NULL;
	fu32_t data;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0004)))
		return -EINVAL;

	data = AIM_ICQ_STATE_HIDEIP | AIM_ICQ_STATE_WEBAWARE | status; /* yay for error checking ;^) */

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10 + 8)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0001, 0x001e, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0001, 0x001e, 0x0000, snacid);

	aim_tlvlist_add_32(&tl, 0x0006, data);
	aim_tlvlist_write(&fr->data, &tl);
	aim_tlvlist_free(&tl);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x001e - Extended Status.
 *
 * Sets your "available" message.  This is currently only supported by iChat
 * and Gaim.
 *
 * These are the same TLVs seen in user info.  You can
 * also set 0x0008 and 0x000c.
 */
faim_export int aim_srv_setavailmsg(aim_session_t *sess, char *msg)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0001)))
		return -EINVAL;

	if (msg != NULL) {
		if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10 + 4 + strlen(msg) + 8)))
			return -ENOMEM;

		snacid = aim_cachesnac(sess, 0x0001, 0x001e, 0x0000, NULL, 0);
		aim_putsnac(&fr->data, 0x0001, 0x001e, 0x0000, snacid);

		aimbs_put16(&fr->data, 0x001d); /* userinfo TLV type */
		aimbs_put16(&fr->data, strlen(msg)+8); /* total length of userinfo TLV data */
		aimbs_put16(&fr->data, 0x0002);
		aimbs_put8(&fr->data, 0x04);
		aimbs_put8(&fr->data, strlen(msg)+4);
		aimbs_put16(&fr->data, strlen(msg));
		aimbs_putraw(&fr->data, msg, strlen(msg));
		aimbs_put16(&fr->data, 0x0000);
	} else {
		if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10 + 4 + 8)))
			return -ENOMEM;

		snacid = aim_cachesnac(sess, 0x0001, 0x001e, 0x0000, NULL, 0);
		aim_putsnac(&fr->data, 0x0001, 0x001e, 0x0000, snacid);

		aimbs_put16(&fr->data, 0x001d);
		aimbs_put16(&fr->data, 0x0008);
		aimbs_put16(&fr->data, 0x0002);
		aimbs_put16(&fr->data, 0x0404);
		aimbs_put16(&fr->data, 0x0000);
		aimbs_put16(&fr->data, 0x0000);
	}

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Starting this past week (26 Mar 2001, say), AOL has started sending
 * this nice little extra SNAC.  AFAIK, it has never been used until now.
 *
 * The request contains eight bytes.  The first four are an offset, the
 * second four are a length.
 *
 * The offset is an offset into aim.exe when it is mapped during execution
 * on Win32.  So far, AOL has only been requesting bytes in static regions
 * of memory.  (I won't put it past them to start requesting data in
 * less static regions -- regions that are initialized at run time, but still
 * before the client recieves this request.)
 *
 * When the client recieves the request, it adds it to the current ds
 * (0x00400000) and dereferences it, copying the data into a buffer which
 * it then runs directly through the MD5 hasher.  The 16 byte output of
 * the hash is then sent back to the server.
 *
 * If the client does not send any data back, or the data does not match
 * the data that the specific client should have, the client will get the
 * following message from "AOL Instant Messenger":
 *    "You have been disconnected from the AOL Instant Message Service (SM)
 *     for accessing the AOL network using unauthorized software.  You can
 *     download a FREE, fully featured, and authorized client, here
 *     http://www.aol.com/aim/download2.html"
 * The connection is then closed, recieving disconnect code 1, URL
 * http://www.aim.aol.com/errors/USER_LOGGED_OFF_NEW_LOGIN.html.
 *
 * Note, however, that numerous inconsistencies can cause the above error,
 * not just sending back a bad hash.  Do not immediatly suspect this code
 * if you get disconnected.  AOL and the open/free software community have
 * played this game for a couple years now, generating the above message
 * on numerous ocassions.
 *
 * Anyway, neener.  We win again.
 *
 */
/* Subtype 0x001f - Client verification */
static int memrequest(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	fu32_t offset, len;
	aim_tlvlist_t *list;
	char *modname;

	offset = aimbs_get32(bs);
	len = aimbs_get32(bs);
	list = aim_tlvlist_read(bs);

	modname = aim_tlv_getstr(list, 0x0001, 1);

	faimdprintf(sess, 1, "data at 0x%08lx (%d bytes) of requested\n", offset, len, modname ? modname : "aim.exe");

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, offset, len, modname);

	free(modname);
	aim_tlvlist_free(&list);

	return ret;
}

#if 0
static void dumpbox(aim_session_t *sess, unsigned char *buf, int len)
{
	int i;

	if (!sess || !buf || !len)
		return;

	faimdprintf(sess, 1, "\nDump of %d bytes at %p:", len, buf);

	for (i = 0; i < len; i++) {
		if ((i % 8) == 0)
			faimdprintf(sess, 1, "\n\t");

		faimdprintf(sess, 1, "0x%2x ", buf[i]);
	}

	faimdprintf(sess, 1, "\n\n");

	return;
}
#endif

/* Subtype 0x0020 - Client verification reply */
faim_export int aim_sendmemblock(aim_session_t *sess, aim_conn_t *conn, fu32_t offset, fu32_t len, const fu8_t *buf, fu8_t flag)
{
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sess || !conn)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+2+16)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0001, 0x0020, 0x0000, NULL, 0);

	aim_putsnac(&fr->data, 0x0001, 0x0020, 0x0000, snacid);
	aimbs_put16(&fr->data, 0x0010); /* md5 is always 16 bytes */

	if ((flag == AIM_SENDMEMBLOCK_FLAG_ISHASH) && buf && (len == 0x10)) { /* we're getting a hash */

		aimbs_putraw(&fr->data, buf, 0x10);

	} else if (buf && (len > 0)) { /* use input buffer */
		md5_state_t state;
		md5_byte_t digest[0x10];

		md5_init(&state);
		md5_append(&state, (const md5_byte_t *)buf, len);
		md5_finish(&state, digest);

		aimbs_putraw(&fr->data, (fu8_t *)digest, 0x10);

	} else if (len == 0) { /* no length, just hash NULL (buf is optional) */
		md5_state_t state;
		fu8_t nil = '\0';
		md5_byte_t digest[0x10];

		/*
		 * These MD5 routines are stupid in that you have to have
		 * at least one append.  So thats why this doesn't look
		 * real logical.
		 */
		md5_init(&state);
		md5_append(&state, (const md5_byte_t *)&nil, 0);
		md5_finish(&state, digest);

		aimbs_putraw(&fr->data, (fu8_t *)digest, 0x10);

	} else {

		/*
		 * This data is correct for AIM 3.5.1670.
		 *
		 * Using these blocks is as close to "legal" as you can get
		 * without using an AIM binary.
		 *
		 */
		if ((offset == 0x03ffffff) && (len == 0x03ffffff)) {

#if 1 /* with "AnrbnrAqhfzcd" */
			aimbs_put32(&fr->data, 0x44a95d26);
			aimbs_put32(&fr->data, 0xd2490423);
			aimbs_put32(&fr->data, 0x93b8821f);
			aimbs_put32(&fr->data, 0x51c54b01);
#else /* no filename */
			aimbs_put32(&fr->data, 0x1df8cbae);
			aimbs_put32(&fr->data, 0x5523b839);
			aimbs_put32(&fr->data, 0xa0e10db3);
			aimbs_put32(&fr->data, 0xa46d3b39);
#endif

		} else if ((offset == 0x00001000) && (len == 0x00000000)) {

			aimbs_put32(&fr->data, 0xd41d8cd9);
			aimbs_put32(&fr->data, 0x8f00b204);
			aimbs_put32(&fr->data, 0xe9800998);
			aimbs_put32(&fr->data, 0xecf8427e);

		} else
			faimdprintf(sess, 0, "sendmemblock: WARNING: unknown hash request\n");

	}

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x0021 - Receive our extended status
 *
 * This is used for iChat's "available" messages, and maybe ICQ extended
 * status messages?  It's also used to tell the client whether or not it
 * needs to upload an SSI buddy icon... who engineers this stuff, anyway?
 */
static int aim_parse_extstatus(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	fu16_t type;
	fu8_t flags, length;

	type = aimbs_get16(bs);
	flags = aimbs_get8(bs);
	length = aimbs_get8(bs);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype))) {
		switch (type) {
		case 0x0000:
		case 0x0001: { /* buddy icon checksum */
			/* not sure what the difference between 1 and 0 is */
			fu8_t *md5 = aimbs_getraw(bs, length);
			ret = userfunc(sess, rx, type, flags, length, md5);
			free(md5);
			} break;
		case 0x0002: { /* available message */
			/* there is a second length that is just for the message */
			char *msg = aimbs_getstr(bs, aimbs_get16(bs));
			ret = userfunc(sess, rx, msg);
			free(msg);
			} break;
		}
	}

	return ret;
}

static int snachandler(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{

	if (snac->subtype == 0x0003)
		return hostonline(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x0005)
		return redirect(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x0007)
		return rateresp(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x000a)
		return ratechange(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x000b)
		return serverpause(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x000d)
		return serverresume(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x000f)
		return selfinfo(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x0010)
		return evilnotify(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x0012)
		return migrate(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x0013)
		return motd(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x0018)
		return hostversions(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x001f)
		return memrequest(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x0021)
		return aim_parse_extstatus(sess, mod, rx, snac, bs);

	return 0;
}

faim_internal int service_modfirst(aim_session_t *sess, aim_module_t *mod)
{

	mod->family = 0x0001;
	mod->version = 0x0003;
	mod->toolid = 0x0110;
	mod->toolversion = 0x0629;
	mod->flags = 0;
	strncpy(mod->name, "service", sizeof(mod->name));
	mod->snachandler = snachandler;

	return 0;
}
