/*
 * Family 0x0008 - Popups.
 *
 * Popups are just what it sounds like.  They're a way for the server to
 * open up an informative box on the client's screen.
 */

#define FAIM_INTERNAL
#include <aim.h>

/*
 * This is all there is to it.
 *
 * The message is probably HTML.
 * 
 */
static int parsepopup(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	aim_rxcallback_t userfunc;
	aim_tlvlist_t *tl;
	int ret = 0;
	char *msg, *url;
	fu16_t width, height, delay;

	tl = aim_readtlvchain(bs);

	msg = aim_gettlv_str(tl, 0x0001, 1);
	url = aim_gettlv_str(tl, 0x0002, 1);
	width = aim_gettlv16(tl, 0x0003, 1);
	height = aim_gettlv16(tl, 0x0004, 1);
	delay = aim_gettlv16(tl, 0x0005, 1);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, msg, url, width, height, delay);

	aim_freetlvchain(&tl);
	free(msg);
	free(url);

	return ret;
}

static int snachandler(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{

	if (snac->subtype == 0x0002)
		return parsepopup(sess, mod, rx, snac, bs);

	return 0;
}

faim_internal int popups_modfirst(aim_session_t *sess, aim_module_t *mod)
{

	mod->family = 0x0008;
	mod->version = 0x0001;
	mod->toolid = 0x0104;
	mod->toolversion = 0x0001;
	mod->flags = 0;
	strncpy(mod->name, "popups", sizeof(mod->name));
	mod->snachandler = snachandler;

	return 0;
}
