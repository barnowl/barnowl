/*
 * Family 0x0007 - Account Administration.
 *
 * Used for stuff like changing the formating of your screen name, changing your 
 * email address, requesting an account confirmation email, getting account info, 
 *
 */

#define FAIM_INTERNAL
#include <aim.h>

/*
 * Subtype 0x0002 - Request a bit of account info.
 *
 * Info should be one of the following:
 * 0x0001 - Screen name formatting
 * 0x0011 - Email address
 * 0x0013 - Unknown
 *
 */ 
faim_export int aim_admin_getinfo(aim_session_t *sess, aim_conn_t *conn, fu16_t info)
{
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 14)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0007, 0x0002, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0007, 0x0002, 0x0000, snacid);

	aimbs_put16(&fr->data, info);
	aimbs_put16(&fr->data, 0x0000);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtypes 0x0003 and 0x0005 - Parse account info.
 *
 * Called in reply to both an information request (subtype 0x0002) and 
 * an information change (subtype 0x0004).
 *
 */
static int infochange(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	aim_rxcallback_t userfunc;
	char *url=NULL, *sn=NULL, *email=NULL;
	fu16_t perms, tlvcount, err=0;

	perms = aimbs_get16(bs);
	tlvcount = aimbs_get16(bs);

	while (tlvcount && aim_bstream_empty(bs)) {
		fu16_t type, length;

		type = aimbs_get16(bs);
		length = aimbs_get16(bs);

		switch (type) {
			case 0x0001: {
				sn = aimbs_getstr(bs, length);
			} break;

			case 0x0004: {
				url = aimbs_getstr(bs, length);
			} break;

			case 0x0008: {
				err = aimbs_get16(bs);
			} break;

			case 0x0011: {
				if (length == 0) {
					email = (char*)malloc(13*sizeof(char));
					strcpy(email, "*suppressed*");
				} else
					email = aimbs_getstr(bs, length);
			} break;
		}

		tlvcount--;
	}

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		userfunc(sess, rx, (snac->subtype == 0x0005) ? 1 : 0, perms, err, url, sn, email);

	if (sn) free(sn);
	if (url) free(url);
	if (email) free(email);

	return 1;
}

/*
 * Subtype 0x0004 - Set screenname formatting.
 *
 */
faim_export int aim_admin_setnick(aim_session_t *sess, aim_conn_t *conn, const char *newnick)
{
	aim_frame_t *fr;
	aim_snacid_t snacid;
	aim_tlvlist_t *tl = NULL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+2+2+strlen(newnick))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0007, 0x0004, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0007, 0x0004, 0x0000, snacid);

	aim_addtlvtochain_raw(&tl, 0x0001, strlen(newnick), newnick);
	
	aim_writetlvchain(&fr->data, &tl);
	aim_freetlvchain(&tl);
	
	aim_tx_enqueue(sess, fr);


	return 0;
}

/*
 * Subtype 0x0004 - Change password.
 *
 */
faim_export int aim_admin_changepasswd(aim_session_t *sess, aim_conn_t *conn, const char *newpw, const char *curpw)
{
	aim_frame_t *fr;
	aim_tlvlist_t *tl = NULL;
	aim_snacid_t snacid;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+4+strlen(curpw)+4+strlen(newpw))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0007, 0x0004, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0007, 0x0004, 0x0000, snacid);

	/* new password TLV t(0002) */
	aim_addtlvtochain_raw(&tl, 0x0002, strlen(newpw), newpw);

	/* current password TLV t(0012) */
	aim_addtlvtochain_raw(&tl, 0x0012, strlen(curpw), curpw);

	aim_writetlvchain(&fr->data, &tl);
	aim_freetlvchain(&tl);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x0004 - Change email address.
 *
 */
faim_export int aim_admin_setemail(aim_session_t *sess, aim_conn_t *conn, const char *newemail)
{
	aim_frame_t *fr;
	aim_snacid_t snacid;
	aim_tlvlist_t *tl = NULL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+2+2+strlen(newemail))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0007, 0x0004, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0007, 0x0004, 0x0000, snacid);

	aim_addtlvtochain_raw(&tl, 0x0011, strlen(newemail), newemail);
	
	aim_writetlvchain(&fr->data, &tl);
	aim_freetlvchain(&tl);
	
	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x0006 - Request account confirmation.
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
 * Subtype 0x0007 - Account confirmation request acknowledgement.
 *
 */
static int accountconfirm(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	fu16_t status;
	aim_tlvlist_t *tl;

	status = aimbs_get16(bs);
	/* This is 0x0013 if unable to confirm at this time */

	tl = aim_readtlvchain(bs);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, status);

	return ret;
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
	mod->toolid = 0x0010;
	mod->toolversion = 0x0629;
	mod->flags = 0;
	strncpy(mod->name, "admin", sizeof(mod->name));
	mod->snachandler = snachandler;

	return 0;
}
