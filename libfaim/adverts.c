/*
 * Family 0x0005 - Advertisements.
 *
 */

#define FAIM_INTERNAL
#include <aim.h>

faim_export int aim_ads_requestads(aim_session_t *sess, aim_conn_t *conn)
{
	return aim_genericreq_n(sess, conn, 0x0005, 0x0002);
}

static int snachandler(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	return 0;
}

faim_internal int adverts_modfirst(aim_session_t *sess, aim_module_t *mod)
{

	mod->family = 0x0005;
	mod->version = 0x0001;
	mod->toolid = 0x0001;
	mod->toolversion = 0x0001;
	mod->flags = 0;
	strncpy(mod->name, "adverts", sizeof(mod->name));
	mod->snachandler = snachandler;

	return 0;
}
