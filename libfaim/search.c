/*
 * Family 0x000a - User Search.
 *
 * TODO: Add aim_usersearch_name()
 *
 */

#define FAIM_INTERNAL
#include <aim.h>

/*
 * Subtype 0x0001
 *
 * XXX can this be integrated with the rest of the error handling?
 */
static int error(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	aim_snac_t *snac2;

	/* XXX the modules interface should have already retrieved this for us */
	if (!(snac2 = aim_remsnac(sess, snac->id))) {
		faimdprintf(sess, 2, "search error: couldn't get a snac for 0x%08lx\n", snac->id);
		return 0;
	}

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, snac2->data /* address */);

	/* XXX freesnac()? */
	if (snac2)
		free(snac2->data);
	free(snac2);

	return ret;
}

/*
 * Subtype 0x0002
 *
 */
faim_export int aim_search_address(aim_session_t *sess, aim_conn_t *conn, const char *address)
{
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sess || !conn || !address)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+strlen(address))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x000a, 0x0002, 0x0000, strdup(address), strlen(address)+1);
	aim_putsnac(&fr->data, 0x000a, 0x0002, 0x0000, snacid);

	aimbs_putraw(&fr->data, address, strlen(address));

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x0003
 *
 */
static int reply(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int j = 0, m, ret = 0;
	aim_tlvlist_t *tlvlist;
	char *cur = NULL, *buf = NULL;
	aim_rxcallback_t userfunc;
	aim_snac_t *snac2;
	char *searchaddr = NULL;

	if ((snac2 = aim_remsnac(sess, snac->id)))
		searchaddr = (char *)snac2->data;

	tlvlist = aim_tlvlist_read(bs);
	m = aim_tlvlist_count(&tlvlist);

	/* XXX uhm.
	 * This is the only place that uses something other than 1 for the 3rd
	 * parameter to aim_tlv_gettlv_whatever().
	 */
	while ((cur = aim_tlv_getstr(tlvlist, 0x0001, j+1)) && j < m) {
		buf = realloc(buf, (j+1) * (MAXSNLEN+1));

		strncpy(&buf[j * (MAXSNLEN+1)], cur, MAXSNLEN);
		free(cur);

		j++;
	}

	aim_tlvlist_free(&tlvlist);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, searchaddr, j, buf);

	/* XXX freesnac()? */
	if (snac2)
		free(snac2->data);
	free(snac2);

	free(buf);

	return ret;
}

static int snachandler(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{

	if (snac->subtype == 0x0001)
		return error(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x0003)
		return reply(sess, mod, rx, snac, bs);

	return 0;
}

faim_internal int search_modfirst(aim_session_t *sess, aim_module_t *mod)
{

	mod->family = 0x000a;
	mod->version = 0x0001;
	mod->toolid = 0x0110;
	mod->toolversion = 0x0629;
	mod->flags = 0;
	strncpy(mod->name, "search", sizeof(mod->name));
	mod->snachandler = snachandler;

	return 0;
}
