
#define FAIM_INTERNAL
#include <aim.h>

/* called for both reply and change-reply */
static int infochange(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{

	/*
	 * struct {
	 *    unsigned short perms;
	 *    unsigned short tlvcount;
	 *    aim_tlv_t tlvs[tlvcount];
	 *  } admin_info[n];
	 */
	while (aim_bstream_empty(bs)) {
		fu16_t perms, tlvcount;

		perms = aimbs_get16(bs);
		tlvcount = aimbs_get16(bs);

		while (tlvcount && aim_bstream_empty(bs)) {
			aim_rxcallback_t userfunc;
			fu16_t type, len;
			fu8_t *val;
			int str = 0;

			type = aimbs_get16(bs);
			len = aimbs_get16(bs);

			if ((type == 0x0011) || (type == 0x0004))
				str = 1;

			if (str)
				val = aimbs_getstr(bs, len);
			else
				val = aimbs_getraw(bs, len);

			/* XXX fix so its only called once for the entire packet */
			if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
				userfunc(sess, rx, (snac->subtype == 0x0005) ? 1 : 0, perms, type, len, val, str);

			free(val);

			tlvcount--;
		}
	}

	return 1;
}

static int accountconfirm(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	aim_rxcallback_t userfunc;
	fu16_t status;

	status = aimbs_get16(bs);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		return userfunc(sess, rx, status);

	return 0;
}

static int snachandler(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{

	if ((snac->subtype == 0x0003) || (snac->subtype == 0x0005))
		return infochange(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x0007)
		return accountconfirm(sess, mod, rx, snac, bs);

	return 0;
}

faim_internal int admin_modfirst(aim_session_t *sess, aim_module_t *mod)
{

	mod->family = 0x0007;
	mod->version = 0x0001;
	mod->toolid = AIM_TOOL_NEWWIN;
	mod->toolversion = 0x0361; /* XXX this and above aren't right */
	mod->flags = 0;
	strncpy(mod->name, "admin", sizeof(mod->name));
	mod->snachandler = snachandler;

	return 0;
}

faim_export int aim_admin_changepasswd(aim_session_t *sess, aim_conn_t *conn, const char *newpw, const char *curpw)
{
	aim_frame_t *tx;
	aim_tlvlist_t *tl = NULL;
	aim_snacid_t snacid;

	if (!(tx = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+4+strlen(curpw)+4+strlen(newpw))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0007, 0x0004, 0x0000, NULL, 0);
	aim_putsnac(&tx->data, 0x0007, 0x0004, 0x0000, snacid);

	/* new password TLV t(0002) */
	aim_addtlvtochain_raw(&tl, 0x0002, strlen(newpw), newpw);

	/* current password TLV t(0012) */
	aim_addtlvtochain_raw(&tl, 0x0012, strlen(curpw), curpw);

	aim_writetlvchain(&tx->data, &tl);
	aim_freetlvchain(&tl);

	aim_tx_enqueue(sess, tx);

	return 0;
}

/*
 * Request account confirmation. 
 *
 * This will cause an email to be sent to the address associated with
 * the account.  By following the instructions in the mail, you can
 * get the TRIAL flag removed from your account.
 *
 */
faim_export int aim_admin_reqconfirm(aim_session_t *sess, aim_conn_t *conn)
{
	return aim_genericreq_n(sess, conn, 0x0007, 0x0006);
}

/*
 * Request a bit of account info.
 *
 * The only known valid tag is 0x0011 (email address).
 *
 */ 
faim_export int aim_admin_getinfo(aim_session_t *sess, aim_conn_t *conn, fu16_t info)
{
	aim_frame_t *tx;
	aim_snacid_t snacid;

	if (!(tx = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 14)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0002, 0x0002, 0x0000, NULL, 0);
	aim_putsnac(&tx->data, 0x0007, 0x0002, 0x0000, snacid);

	aimbs_put16(&tx->data, info);
	aimbs_put16(&tx->data, 0x0000);

	aim_tx_enqueue(sess, tx);

	return 0;
}

faim_export int aim_admin_setemail(aim_session_t *sess, aim_conn_t *conn, const char *newemail)
{
	aim_frame_t *tx;
	aim_snacid_t snacid;
	aim_tlvlist_t *tl = NULL;

	if (!(tx = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+2+2+strlen(newemail))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0007, 0x0004, 0x0000, NULL, 0);
	aim_putsnac(&tx->data, 0x0007, 0x0004, 0x0000, snacid);

	aim_addtlvtochain_raw(&tl, 0x0011, strlen(newemail), newemail);
	
	aim_writetlvchain(&tx->data, &tl);
	aim_freetlvchain(&tl);
	
	aim_tx_enqueue(sess, tx);

	return 0;
}

faim_export int aim_admin_setnick(aim_session_t *sess, aim_conn_t *conn, const char *newnick)
{
	aim_frame_t *tx;
	aim_snacid_t snacid;
	aim_tlvlist_t *tl = NULL;

	if (!(tx = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+2+2+strlen(newnick))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0007, 0x0004, 0x0000, NULL, 0);
	aim_putsnac(&tx->data, 0x0007, 0x0004, 0x0000, snacid);

	aim_addtlvtochain_raw(&tl, 0x0001, strlen(newnick), newnick);
	
	aim_writetlvchain(&tx->data, &tl);
	aim_freetlvchain(&tl);
	
	aim_tx_enqueue(sess, tx);

	return 0;
}

