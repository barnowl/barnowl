/*
 * Family 0x0018 - Email notification
 *
 * Used for being alerted when the email address(es) associated with 
 * your screen name get new electronic-m.  For normal AIM accounts, you 
 * get the email address screenname@netscape.net.  AOL accounts have 
 * screenname@aol.com, and can also activate a netscape.net account.
 *
 */

#define FAIM_INTERNAL
#include <aim.h>

/**
 * Subtype 0x0006 - Request information about your email account
 *
 * @param sess The oscar session.
 * @param conn The email connection for this session.
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_email_sendcookies(aim_session_t *sess, aim_conn_t *conn)
{
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sess || !conn)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+2+16+16)))
		return -ENOMEM;
	snacid = aim_cachesnac(sess, 0x0018, 0x0006, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0018, 0x0006, 0x0000, snacid);

	/* Number of cookies to follow */
	aimbs_put16(&fr->data, 0x0002);

	/* Cookie */
	aimbs_put16(&fr->data, 0x5d5e);
	aimbs_put16(&fr->data, 0x1708);
	aimbs_put16(&fr->data, 0x55aa);
	aimbs_put16(&fr->data, 0x11d3);
	aimbs_put16(&fr->data, 0xb143);
	aimbs_put16(&fr->data, 0x0060);
	aimbs_put16(&fr->data, 0xb0fb);
	aimbs_put16(&fr->data, 0x1ecb);

	/* Cookie */
	aimbs_put16(&fr->data, 0xb380);
	aimbs_put16(&fr->data, 0x9ad8);
	aimbs_put16(&fr->data, 0x0dba);
	aimbs_put16(&fr->data, 0x11d5);
	aimbs_put16(&fr->data, 0x9f8a);
	aimbs_put16(&fr->data, 0x0060);
	aimbs_put16(&fr->data, 0xb0ee);
	aimbs_put16(&fr->data, 0x0631);

	aim_tx_enqueue(sess, fr);

	return 0;
}


/**
 * Subtype 0x0007 - Receive information about your email account
 * So I don't even know if you can have multiple 16 byte keys, 
 * but this is coded so it will handle that, and handle it well.
 * This tells you if you have unread mail or not, the URL you 
 * should use to access that mail, and the domain name for the 
 * email account (screenname@domainname.com).  If this is the 
 * first 0x0007 SNAC you've received since you signed on, or if 
 * this is just a periodic status update, this will also contain 
 * the number of unread emails that you have.
 */
static int parseinfo(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	aim_rxcallback_t userfunc;
	struct aim_emailinfo *new;
	aim_tlvlist_t *tlvlist;
	fu8_t *cookie8, *cookie16;
	int tmp, havenewmail = 0; /* Used to tell the client we have _new_ mail */

	cookie8 = aimbs_getraw(bs, 8); /* Possibly the code used to log you in to mail? */
	cookie16 = aimbs_getraw(bs, 16); /* Mail cookie sent above */

	/* See if we already have some info associated with this cookie */
	for (new=sess->emailinfo; (new && strncmp(cookie16, new->cookie16, 16)); new=new->next);
	if (new) {
		/* Free some of the old info, if existant */
		free(new->cookie8);
		free(new->cookie16);
		free(new->url);
		free(new->domain);
	} else {
		/* We don't already have info, so create a new struct for it */
		if (!(new = malloc(sizeof(struct aim_emailinfo))))
			return -ENOMEM;
		memset(new, 0, sizeof(struct aim_emailinfo));
		new->next = sess->emailinfo;
		sess->emailinfo = new;
	}

	new->cookie8 = cookie8;
	new->cookie16 = cookie16;

	tlvlist = aim_readtlvchain_num(bs, aimbs_get16(bs));

	tmp = aim_gettlv16(tlvlist, 0x0080, 1);
	if (tmp) {
		if (new->nummsgs < tmp)
			havenewmail = 1;
		new->nummsgs = tmp;
	} else {
		/* If they don't send a 0x0080 TLV, it means we definately have new mail */
		/* (ie. this is not just another status update) */
		havenewmail = 1;
		new->nummsgs++; /* We know we have at least 1 new email */
	}
	new->url = aim_gettlv_str(tlvlist, 0x0007, 1);
	if (!(new->unread = aim_gettlv8(tlvlist, 0x0081, 1))) {
		havenewmail = 0;
		new->nummsgs = 0;
	}
	new->domain = aim_gettlv_str(tlvlist, 0x0082, 1);
	new->flag = aim_gettlv16(tlvlist, 0x0084, 1);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		return userfunc(sess, rx, new, havenewmail);

	return 0;
}

/**
 * Subtype 0x0016 - Send something or other
 *
 * @param sess The oscar session.
 * @param conn The email connection for this session.
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_email_activate(aim_session_t *sess, aim_conn_t *conn)
{
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sess || !conn)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+1+16)))
		return -ENOMEM;
	snacid = aim_cachesnac(sess, 0x0018, 0x0016, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0018, 0x0016, 0x0000, snacid);

	/* I would guess this tells AIM that you want updates for your mail accounts */
	/* ...but I really have no idea */
	aimbs_put8(&fr->data, 0x02);
	aimbs_put32(&fr->data, 0x04000000);
	aimbs_put32(&fr->data, 0x04000000);
	aimbs_put32(&fr->data, 0x04000000);
	aimbs_put32(&fr->data, 0x00000000);

	aim_tx_enqueue(sess, fr);

	return 0;
}

static int snachandler(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{

	if (snac->subtype == 0x0007)
		return parseinfo(sess, mod, rx, snac, bs);

	return 0;
}

static void email_shutdown(aim_session_t *sess, aim_module_t *mod)
{
	while (sess->emailinfo) {
		struct aim_emailinfo *tmp = sess->emailinfo;
		sess->emailinfo = sess->emailinfo->next;
		free(tmp->cookie16);
		free(tmp->cookie8);
		free(tmp->url);
		free(tmp->domain);
		free(tmp);
	}

	return;
}

faim_internal int email_modfirst(aim_session_t *sess, aim_module_t *mod)
{

	mod->family = 0x0018;
	mod->version = 0x0001;
	mod->toolid = 0x0010;
	mod->toolversion = 0x0629;
	mod->flags = 0;
	strncpy(mod->name, "email", sizeof(mod->name));
	mod->snachandler = snachandler;
	mod->shutdown = email_shutdown;

	return 0;
}
