/*
 * Family 0x0015 - Encapsulated ICQ.
 *
 */

#define FAIM_INTERNAL
#include <aim.h>

faim_export int aim_icq_reqofflinemsgs(aim_session_t *sess)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	int bslen;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0015)))
		return -EINVAL;

	bslen = 2 + 4 + 2 + 2;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10 + 4 + bslen)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0015, 0x0002, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0015, 0x0002, 0x0000, snacid);

	/* For simplicity, don't bother using a tlvlist */
	aimbs_put16(&fr->data, 0x0001);
	aimbs_put16(&fr->data, bslen);

	aimbs_putle16(&fr->data, bslen - 2);
	aimbs_putle32(&fr->data, atoi(sess->sn));
	aimbs_putle16(&fr->data, 0x003c); /* I command thee. */
	aimbs_putle16(&fr->data, snacid); /* eh. */

	aim_tx_enqueue(sess, fr);

	return 0;
}

faim_export int aim_icq_ackofflinemsgs(aim_session_t *sess)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	int bslen;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0015)))
		return -EINVAL;

	bslen = 2 + 4 + 2 + 2;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10 + 4 + bslen)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0015, 0x0002, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0015, 0x0002, 0x0000, snacid);

	/* For simplicity, don't bother using a tlvlist */
	aimbs_put16(&fr->data, 0x0001);
	aimbs_put16(&fr->data, bslen);

	aimbs_putle16(&fr->data, bslen - 2);
	aimbs_putle32(&fr->data, atoi(sess->sn));
	aimbs_putle16(&fr->data, 0x003e); /* I command thee. */
	aimbs_putle16(&fr->data, snacid); /* eh. */

	aim_tx_enqueue(sess, fr);

	return 0;
}

faim_export int aim_icq_hideip(aim_session_t *sess)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	int bslen;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0015)))
		return -EINVAL;

	bslen = 2+4+2+2+2+4;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10 + 4 + bslen)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0015, 0x0002, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0015, 0x0002, 0x0000, snacid);

	/* For simplicity, don't bother using a tlvlist */
	aimbs_put16(&fr->data, 0x0001);
	aimbs_put16(&fr->data, bslen);

	aimbs_putle16(&fr->data, bslen - 2);
	aimbs_putle32(&fr->data, atoi(sess->sn));
	aimbs_putle16(&fr->data, 0x07d0); /* I command thee. */
	aimbs_putle16(&fr->data, snacid); /* eh. */
	aimbs_putle16(&fr->data, 0x0424); /* shrug. */
	aimbs_putle16(&fr->data, 0x0001);
	aimbs_putle16(&fr->data, 0x0001);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/**
 * Change your ICQ password.
 *
 * @param sess The oscar session
 * @param passwd The new password.  If this is longer than 8 characters it 
 *        will be truncated.
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_icq_changepasswd(aim_session_t *sess, const char *passwd)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	int bslen, passwdlen;

	if (!passwd)
		return -EINVAL;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0015)))
		return -EINVAL;

	passwdlen = strlen(passwd);
	if (passwdlen > MAXICQPASSLEN)
		passwdlen = MAXICQPASSLEN;
	bslen = 2+4+2+2+2+2+passwdlen+1;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10 + 4 + bslen)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0015, 0x0002, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0015, 0x0002, 0x0000, snacid);

	/* For simplicity, don't bother using a tlvlist */
	aimbs_put16(&fr->data, 0x0001);
	aimbs_put16(&fr->data, bslen);

	aimbs_putle16(&fr->data, bslen - 2);
	aimbs_putle32(&fr->data, atoi(sess->sn));
	aimbs_putle16(&fr->data, 0x07d0); /* I command thee. */
	aimbs_putle16(&fr->data, snacid); /* eh. */
	aimbs_putle16(&fr->data, 0x042e); /* shrug. */
	aimbs_putle16(&fr->data, passwdlen+1);
	aimbs_putraw(&fr->data, passwd, passwdlen);
	aimbs_putle8(&fr->data, '\0');

	aim_tx_enqueue(sess, fr);

	return 0;
}

faim_export int aim_icq_getallinfo(aim_session_t *sess, const char *uin)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	int bslen;
	struct aim_icq_info *info;

	if (!uin || uin[0] < '0' || uin[0] > '9')
		return -EINVAL;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0015)))
		return -EINVAL;

	bslen = 2 + 4 + 2 + 2 + 2 + 4;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10 + 4 + bslen)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0015, 0x0002, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0015, 0x0002, 0x0000, snacid);

	/* For simplicity, don't bother using a tlvlist */
	aimbs_put16(&fr->data, 0x0001);
	aimbs_put16(&fr->data, bslen);

	aimbs_putle16(&fr->data, bslen - 2);
	aimbs_putle32(&fr->data, atoi(sess->sn));
	aimbs_putle16(&fr->data, 0x07d0); /* I command thee. */
	aimbs_putle16(&fr->data, snacid); /* eh. */
	aimbs_putle16(&fr->data, 0x04b2); /* shrug. */
	aimbs_putle32(&fr->data, atoi(uin));

	aim_tx_enqueue(sess, fr);

	/* Keep track of this request and the ICQ number and request ID */
	info = (struct aim_icq_info *)calloc(1, sizeof(struct aim_icq_info));
	info->reqid = snacid;
	info->uin = atoi(uin);
	info->next = sess->icq_info;
	sess->icq_info = info;

	return 0;
}

faim_export int aim_icq_getalias(aim_session_t *sess, const char *uin)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	int bslen;
	struct aim_icq_info *info;

	if (!uin || uin[0] < '0' || uin[0] > '9')
		return -EINVAL;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0015)))
		return -EINVAL;

	bslen = 2 + 4 + 2 + 2 + 2 + 4;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10 + 4 + bslen)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0015, 0x0002, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0015, 0x0002, 0x0000, snacid);

	/* For simplicity, don't bother using a tlvlist */
	aimbs_put16(&fr->data, 0x0001);
	aimbs_put16(&fr->data, bslen);

	aimbs_putle16(&fr->data, bslen - 2);
	aimbs_putle32(&fr->data, atoi(sess->sn));
	aimbs_putle16(&fr->data, 0x07d0); /* I command thee. */
	aimbs_putle16(&fr->data, snacid); /* eh. */
	aimbs_putle16(&fr->data, 0x04ba); /* shrug. */
	aimbs_putle32(&fr->data, atoi(uin));

	aim_tx_enqueue(sess, fr);

	/* Keep track of this request and the ICQ number and request ID */
	info = (struct aim_icq_info *)calloc(1, sizeof(struct aim_icq_info));
	info->reqid = snacid;
	info->uin = atoi(uin);
	info->next = sess->icq_info;
	sess->icq_info = info;

	return 0;
}

faim_export int aim_icq_getsimpleinfo(aim_session_t *sess, const char *uin)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	int bslen;

	if (!uin || uin[0] < '0' || uin[0] > '9')
		return -EINVAL;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0015)))
		return -EINVAL;

	bslen = 2 + 4 + 2 + 2 + 2 + 4;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10 + 4 + bslen)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0015, 0x0002, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0015, 0x0002, 0x0000, snacid);

	/* For simplicity, don't bother using a tlvlist */
	aimbs_put16(&fr->data, 0x0001);
	aimbs_put16(&fr->data, bslen);

	aimbs_putle16(&fr->data, bslen - 2);
	aimbs_putle32(&fr->data, atoi(sess->sn));
	aimbs_putle16(&fr->data, 0x07d0); /* I command thee. */
	aimbs_putle16(&fr->data, snacid); /* eh. */
	aimbs_putle16(&fr->data, 0x051f); /* shrug. */
	aimbs_putle32(&fr->data, atoi(uin));

	aim_tx_enqueue(sess, fr);

	return 0;
}

faim_export int aim_icq_sendxmlreq(aim_session_t *sess, const char *xml)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	int bslen;

	if (!xml || !strlen(xml))
		return -EINVAL;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x0015)))
		return -EINVAL;

	bslen = 2 + 10 + 2 + strlen(xml) + 1;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10 + 4 + bslen)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0015, 0x0002, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x0015, 0x0002, 0x0000, snacid);

	/* For simplicity, don't bother using a tlvlist */
	aimbs_put16(&fr->data, 0x0001);
	aimbs_put16(&fr->data, bslen);

	aimbs_putle16(&fr->data, bslen - 2);
	aimbs_putle32(&fr->data, atoi(sess->sn));
	aimbs_putle16(&fr->data, 0x07d0); /* I command thee. */
	aimbs_putle16(&fr->data, snacid); /* eh. */
	aimbs_putle16(&fr->data, 0x0998); /* shrug. */
	aimbs_putle16(&fr->data, strlen(xml) + 1);
	aimbs_putraw(&fr->data, xml, strlen(xml) + 1);

	aim_tx_enqueue(sess, fr);

	return 0;
}

static void aim_icq_freeinfo(struct aim_icq_info *info) {
	int i;

	if (!info)
		return;
	free(info->nick);
	free(info->first);
	free(info->last);
	free(info->email);
	free(info->homecity);
	free(info->homestate);
	free(info->homephone);
	free(info->homefax);
	free(info->homeaddr);
	free(info->mobile);
	free(info->homezip);
	free(info->personalwebpage);
	if (info->email2)
		for (i = 0; i < info->numaddresses; i++)
			free(info->email2[i]);
	free(info->email2);
	free(info->workcity);
	free(info->workstate);
	free(info->workphone);
	free(info->workfax);
	free(info->workaddr);
	free(info->workzip);
	free(info->workcompany);
	free(info->workdivision);
	free(info->workposition);
	free(info->workwebpage);
	free(info->info);
	free(info);
}

/**
 * Subtype 0x0003 - Response to 0x0015/0x002, contains an ICQesque packet.
 */
static int icqresponse(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_tlvlist_t *tl;
	aim_tlv_t *datatlv;
	aim_bstream_t qbs;
	fu32_t ouruin;
	fu16_t cmdlen, cmd, reqid;

	if (!(tl = aim_readtlvchain(bs)) || !(datatlv = aim_gettlv(tl, 0x0001, 1))) {
		aim_freetlvchain(&tl);
		faimdprintf(sess, 0, "corrupt ICQ response\n");
		return 0;
	}

	aim_bstream_init(&qbs, datatlv->value, datatlv->length);

	cmdlen = aimbs_getle16(&qbs);
	ouruin = aimbs_getle32(&qbs);
	cmd = aimbs_getle16(&qbs);
	reqid = aimbs_getle16(&qbs);

	faimdprintf(sess, 1, "icq response: %d bytes, %ld, 0x%04x, 0x%04x\n", cmdlen, ouruin, cmd, reqid);

	if (cmd == 0x0041) { /* offline message */
		struct aim_icq_offlinemsg msg;
		aim_rxcallback_t userfunc;

		memset(&msg, 0, sizeof(msg));

		msg.sender = aimbs_getle32(&qbs);
		msg.year = aimbs_getle16(&qbs);
		msg.month = aimbs_getle8(&qbs);
		msg.day = aimbs_getle8(&qbs);
		msg.hour = aimbs_getle8(&qbs);
		msg.minute = aimbs_getle8(&qbs);
		msg.type = aimbs_getle8(&qbs);
		msg.flags = aimbs_getle8(&qbs);
		msg.msglen = aimbs_getle16(&qbs);
		msg.msg = aimbs_getstr(&qbs, msg.msglen);

		if ((userfunc = aim_callhandler(sess, rx->conn, AIM_CB_FAM_ICQ, AIM_CB_ICQ_OFFLINEMSG)))
			ret = userfunc(sess, rx, &msg);

		free(msg.msg);

	} else if (cmd == 0x0042) {
		aim_rxcallback_t userfunc;

		if ((userfunc = aim_callhandler(sess, rx->conn, AIM_CB_FAM_ICQ, AIM_CB_ICQ_OFFLINEMSGCOMPLETE)))
			ret = userfunc(sess, rx);

	} else if (cmd == 0x07da) { /* information */
		fu16_t subtype;
		struct aim_icq_info *info;
		aim_rxcallback_t userfunc;

		subtype = aimbs_getle16(&qbs);
		aim_bstream_advance(&qbs, 1); /* 0x0a */

		/* find other data from the same request */
		for (info = sess->icq_info; info && (info->reqid != reqid); info = info->next);
		if (!info) {
			info = (struct aim_icq_info *)calloc(1, sizeof(struct aim_icq_info));
			info->reqid = reqid;
			info->next = sess->icq_info;
			sess->icq_info = info;
		}

		switch (subtype) {
		case 0x00a0: { /* hide ip status */
			/* nothing */
		} break;

		case 0x00aa: { /* password change status */
			/* nothing */
		} break;

		case 0x00c8: { /* general and "home" information */
			info->nick = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->first = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->last = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->email = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->homecity = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->homestate = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->homephone = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->homefax = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->homeaddr = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->mobile = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->homezip = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->homecountry = aimbs_getle16(&qbs);
			/* 0x0a 00 02 00 */
			/* 1 byte timezone? */
			/* 1 byte hide email flag? */
		} break;

		case 0x00dc: { /* personal information */
			info->age = aimbs_getle8(&qbs);
			info->unknown = aimbs_getle8(&qbs);
			info->gender = aimbs_getle8(&qbs);
			info->personalwebpage = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->birthyear = aimbs_getle16(&qbs);
			info->birthmonth = aimbs_getle8(&qbs);
			info->birthday = aimbs_getle8(&qbs);
			info->language1 = aimbs_getle8(&qbs);
			info->language2 = aimbs_getle8(&qbs);
			info->language3 = aimbs_getle8(&qbs);
			/* 0x00 00 01 00 00 01 00 00 00 00 00 */
		} break;

		case 0x00d2: { /* work information */
			info->workcity = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->workstate = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->workphone = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->workfax = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->workaddr = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->workzip = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->workcountry = aimbs_getle16(&qbs);
			info->workcompany = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->workdivision = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->workposition = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			aim_bstream_advance(&qbs, 2); /* 0x01 00 */
			info->workwebpage = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
		} break;

		case 0x00e6: { /* additional personal information */
			info->info = aimbs_getstr(&qbs, aimbs_getle16(&qbs)-1);
		} break;

		case 0x00eb: { /* email address(es) */
			int i;
			info->numaddresses = aimbs_getle16(&qbs);
			info->email2 = (char **)calloc(info->numaddresses, sizeof(char *));
			for (i = 0; i < info->numaddresses; i++) {
				info->email2[i] = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
				if (i+1 != info->numaddresses)
					aim_bstream_advance(&qbs, 1); /* 0x00 */
			}
		} break;

		case 0x00f0: { /* personal interests */
		} break;

		case 0x00fa: { /* past background and current organizations */
		} break;

		case 0x0104: { /* alias info */
			info->nick = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->first = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->last = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			aim_bstream_advance(&qbs, aimbs_getle16(&qbs)); /* email address? */
			/* Then 0x00 02 00 */
		} break;

		case 0x010e: { /* unknown */
			/* 0x00 00 */
		} break;

		case 0x019a: { /* simple info */
			aim_bstream_advance(&qbs, 2);
			info->uin = aimbs_getle32(&qbs);
			info->nick = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->first = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->last = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			info->email = aimbs_getstr(&qbs, aimbs_getle16(&qbs));
			/* Then 0x00 02 00 00 00 00 00 */
		} break;
		} /* End switch statement */

		if (!(snac->flags & 0x0001)) {
			if (subtype != 0x0104)
				if ((userfunc = aim_callhandler(sess, rx->conn, AIM_CB_FAM_ICQ, AIM_CB_ICQ_INFO)))
					ret = userfunc(sess, rx, info);

			if (info->uin && info->nick)
				if ((userfunc = aim_callhandler(sess, rx->conn, AIM_CB_FAM_ICQ, AIM_CB_ICQ_ALIAS)))
					ret = userfunc(sess, rx, info);

			if (sess->icq_info == info) {
				sess->icq_info = info->next;
			} else {
				struct aim_icq_info *cur;
				for (cur=sess->icq_info; (cur->next && (cur->next!=info)); cur=cur->next);
				if (cur->next)
					cur->next = cur->next->next;
			}
			aim_icq_freeinfo(info);
		}
	}

	aim_freetlvchain(&tl);

	return ret;
}

static int snachandler(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{

	if (snac->subtype == 0x0003)
		return icqresponse(sess, mod, rx, snac, bs);

	return 0;
}

static void icq_shutdown(aim_session_t *sess, aim_module_t *mod)
{
	struct aim_icq_info *del;

	while (sess->icq_info) {
		del = sess->icq_info;
		sess->icq_info = sess->icq_info->next;
		aim_icq_freeinfo(del);
	}

	return;
}

faim_internal int icq_modfirst(aim_session_t *sess, aim_module_t *mod)
{

	mod->family = 0x0015;
	mod->version = 0x0001;
	mod->toolid = 0x0110;
	mod->toolversion = 0x047c;
	mod->flags = 0;
	strncpy(mod->name, "icq", sizeof(mod->name));
	mod->snachandler = snachandler;
	mod->shutdown = icq_shutdown;

	return 0;
}
