/*
 * misc.c
 *
 * Random stuff.  Basically just a few functions for sending 
 * simple SNACs, and then the generic error handler.
 *
 */

#define FAIM_INTERNAL
#include <aim.h> 

/*
 * Generic routine for sending commands.
 *
 *
 * I know I can do this in a smarter way...but I'm not thinking straight
 * right now...
 *
 * I had one big function that handled all three cases, but then it broke
 * and I split it up into three.  But then I fixed it.  I just never went
 * back to the single.  I don't see any advantage to doing it either way.
 *
 */
faim_internal int aim_genericreq_n(aim_session_t *sess, aim_conn_t *conn, fu16_t family, fu16_t subtype)
{
	aim_frame_t *fr;
	aim_snacid_t snacid = 0x00000000;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10)))
		return -ENOMEM;

	aim_putsnac(&fr->data, family, subtype, 0x0000, snacid);

	aim_tx_enqueue(sess, fr);

	return 0;
}

faim_internal int aim_genericreq_n_snacid(aim_session_t *sess, aim_conn_t *conn, fu16_t family, fu16_t subtype)
{
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, family, subtype, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, family, subtype, 0x0000, snacid);

	aim_tx_enqueue(sess, fr);

	return 0;
}

faim_internal int aim_genericreq_l(aim_session_t *sess, aim_conn_t *conn, fu16_t family, fu16_t subtype, fu32_t *longdata)
{
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!longdata)
		return aim_genericreq_n(sess, conn, family, subtype);

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+4)))
		return -ENOMEM; 

	snacid = aim_cachesnac(sess, family, subtype, 0x0000, NULL, 0);

	aim_putsnac(&fr->data, family, subtype, 0x0000, snacid);
	aimbs_put32(&fr->data, *longdata);

	aim_tx_enqueue(sess, fr);

	return 0;
}

faim_internal int aim_genericreq_s(aim_session_t *sess, aim_conn_t *conn, fu16_t family, fu16_t subtype, fu16_t *shortdata)
{
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!shortdata)
		return aim_genericreq_n(sess, conn, family, subtype);

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+2)))
		return -ENOMEM; 

	snacid = aim_cachesnac(sess, family, subtype, 0x0000, NULL, 0);

	aim_putsnac(&fr->data, family, subtype, 0x0000, snacid);
	aimbs_put16(&fr->data, *shortdata);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Should be generic enough to handle the errors for all groups.
 *
 */
static int generror(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	int error = 0;
	aim_rxcallback_t userfunc;
	aim_snac_t *snac2;

	snac2 = aim_remsnac(sess, snac->id);

	if (aim_bstream_empty(bs))
		error = aimbs_get16(bs);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, error, snac2 ? snac2->data : NULL);

	if (snac2)
		free(snac2->data);
	free(snac2);

	return ret;
}

static int snachandler(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{

	if (snac->subtype == 0x0001)
		return generror(sess, mod, rx, snac, bs);
	else if ((snac->family == 0xffff) && (snac->subtype == 0xffff)) {
		aim_rxcallback_t userfunc;

		if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
			return userfunc(sess, rx);
	}

	return 0;
}

faim_internal int misc_modfirst(aim_session_t *sess, aim_module_t *mod)
{

	mod->family = 0xffff;
	mod->version = 0x0000;
	mod->flags = AIM_MODFLAG_MULTIFAMILY;
	strncpy(mod->name, "misc", sizeof(mod->name));
	mod->snachandler = snachandler;

	return 0;
}
