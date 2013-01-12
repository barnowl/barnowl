/*
 * Family 0x000d - Handle ChatNav.
 *
 * The ChatNav(igation) service does various things to keep chat
 * alive.  It provides room information, room searching and creating,
 * as well as giving users the right ("permission") to use chat.
 *
 */

#define FAIM_INTERNAL
#include <aim.h>

/*
 * Subtype 0x0002
 *
 * conn must be a chatnav connection!
 *
 */
faim_export int aim_chatnav_reqrights(aim_session_t *sess, aim_conn_t *conn)
{
	return aim_genericreq_n_snacid(sess, conn, 0x000d, 0x0002);
}

/*
 * Subtype 0x0008
 */
faim_export int aim_chatnav_createroom(aim_session_t *sess, aim_conn_t *conn, const char *name, fu16_t exchange)
{
	static const char ck[] = {"create"};
	static const char lang[] = {"en"};
	static const char charset[] = {"us-ascii"};
	aim_frame_t *fr;
	aim_snacid_t snacid;
	aim_tlvlist_t *tl = NULL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 1152)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x000d, 0x0008, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x000d, 0x0008, 0x0000, snacid);

	/* exchange */
	aimbs_put16(&fr->data, exchange);

	/*
	 * This looks to be a big hack.  You'll note that this entire
	 * SNAC is just a room info structure, but the hard room name,
	 * here, is set to "create".
	 *
	 * Either this goes on the "list of questions concerning
	 * why-the-hell-did-you-do-that", or this value is completly
	 * ignored.  Without experimental evidence, but a good knowledge of
	 * AOL style, I'm going to guess that it is the latter, and that
	 * the value of the room name in create requests is ignored.
	 */
	aimbs_put8(&fr->data, strlen(ck));
	aimbs_putraw(&fr->data, ck, strlen(ck));

	/*
	 * instance
	 *
	 * Setting this to 0xffff apparently assigns the last instance.
	 *
	 */
	aimbs_put16(&fr->data, 0xffff);

	/* detail level */
	aimbs_put8(&fr->data, 0x01);

	aim_tlvlist_add_raw(&tl, 0x00d3, strlen(name), name);
	aim_tlvlist_add_raw(&tl, 0x00d6, strlen(charset), charset);
	aim_tlvlist_add_raw(&tl, 0x00d7, strlen(lang), lang);

	/* tlvcount */
	aimbs_put16(&fr->data, aim_tlvlist_count(&tl));
	aim_tlvlist_write(&fr->data, &tl);

	aim_tlvlist_free(&tl);

	aim_tx_enqueue(sess, fr);

	return 0;
}

static int parseinfo_perms(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs, aim_snac_t *snac2)
{
	aim_rxcallback_t userfunc;
	int ret = 0;
	struct aim_chat_exchangeinfo *exchanges = NULL;
	int curexchange;
	aim_tlv_t *exchangetlv;
	fu8_t maxrooms = 0;
	aim_tlvlist_t *tlvlist, *innerlist;

	tlvlist = aim_tlvlist_read(bs);

	/*
	 * Type 0x0002: Maximum concurrent rooms.
	 */
	if (aim_tlv_gettlv(tlvlist, 0x0002, 1))
		maxrooms = aim_tlv_get8(tlvlist, 0x0002, 1);

	/*
	 * Type 0x0003: Exchange information
	 *
	 * There can be any number of these, each one
	 * representing another exchange.
	 *
	 */
	for (curexchange = 0; ((exchangetlv = aim_tlv_gettlv(tlvlist, 0x0003, curexchange+1))); ) {
		aim_bstream_t tbs;

		aim_bstream_init(&tbs, exchangetlv->value, exchangetlv->length);

		curexchange++;

		exchanges = realloc(exchanges, curexchange * sizeof(struct aim_chat_exchangeinfo));

		/* exchange number */
		exchanges[curexchange-1].number = aimbs_get16(&tbs);
		innerlist = aim_tlvlist_read(&tbs);

		/*
		 * Type 0x000a: Unknown.
		 *
		 * Usually three bytes: 0x0114 (exchange 1) or 0x010f (others).
		 *
		 */
		if (aim_tlv_gettlv(innerlist, 0x000a, 1))
			;

		/*
		 * Type 0x000d: Unknown.
		 */
		if (aim_tlv_gettlv(innerlist, 0x000d, 1))
			;

		/*
		 * Type 0x0004: Unknown
		 */
		if (aim_tlv_gettlv(innerlist, 0x0004, 1))
			;

		/*
		 * Type 0x0002: Unknown
		 */
		if (aim_tlv_gettlv(innerlist, 0x0002, 1)) {
			fu16_t classperms;

			classperms = aim_tlv_get16(innerlist, 0x0002, 1);

			faimdprintf(sess, 1, "faim: class permissions %x\n", classperms);
		}

		/*
		 * Type 0x00c9: Flags
		 *
		 * 1 Evilable
		 * 2 Nav Only
		 * 4 Instancing Allowed
		 * 8 Occupant Peek Allowed
		 *
		 */
		if (aim_tlv_gettlv(innerlist, 0x00c9, 1))
			exchanges[curexchange-1].flags = aim_tlv_get16(innerlist, 0x00c9, 1);

		/*
		 * Type 0x00ca: Creation Date
		 */
		if (aim_tlv_gettlv(innerlist, 0x00ca, 1))
			;

		/*
		 * Type 0x00d0: Mandatory Channels?
		 */
		if (aim_tlv_gettlv(innerlist, 0x00d0, 1))
			;

		/*
		 * Type 0x00d1: Maximum Message length
		 */
		if (aim_tlv_gettlv(innerlist, 0x00d1, 1))
			;

		/*
		 * Type 0x00d2: Maximum Occupancy?
		 */
		if (aim_tlv_gettlv(innerlist, 0x00d2, 1))
			;

		/*
		 * Type 0x00d3: Exchange Description
		 */
		if (aim_tlv_gettlv(innerlist, 0x00d3, 1))
			exchanges[curexchange-1].name = aim_tlv_getstr(innerlist, 0x00d3, 1);
		else
			exchanges[curexchange-1].name = NULL;

		/*
		 * Type 0x00d4: Exchange Description URL
		 */
		if (aim_tlv_gettlv(innerlist, 0x00d4, 1))
			;

		/*
		 * Type 0x00d5: Creation Permissions
		 *
		 * 0  Creation not allowed
		 * 1  Room creation allowed
		 * 2  Exchange creation allowed
		 *
		 */
		if (aim_tlv_gettlv(innerlist, 0x00d5, 1)) {
			fu8_t createperms;

			createperms = aim_tlv_get8(innerlist, 0x00d5, 1);
		}

		/*
		 * Type 0x00d6: Character Set (First Time)
		 */
		if (aim_tlv_gettlv(innerlist, 0x00d6, 1))
			exchanges[curexchange-1].charset1 = aim_tlv_getstr(innerlist, 0x00d6, 1);
		else
			exchanges[curexchange-1].charset1 = NULL;

		/*
		 * Type 0x00d7: Language (First Time)
		 */
		if (aim_tlv_gettlv(innerlist, 0x00d7, 1))
			exchanges[curexchange-1].lang1 = aim_tlv_getstr(innerlist, 0x00d7, 1);
		else
			exchanges[curexchange-1].lang1 = NULL;

		/*
		 * Type 0x00d8: Character Set (Second Time)
		 */
		if (aim_tlv_gettlv(innerlist, 0x00d8, 1))
			exchanges[curexchange-1].charset2 = aim_tlv_getstr(innerlist, 0x00d8, 1);
		else
			exchanges[curexchange-1].charset2 = NULL;

		/*
		 * Type 0x00d9: Language (Second Time)
		 */
		if (aim_tlv_gettlv(innerlist, 0x00d9, 1))
			exchanges[curexchange-1].lang2 = aim_tlv_getstr(innerlist, 0x00d9, 1);
		else
			exchanges[curexchange-1].lang2 = NULL;

		/*
		 * Type 0x00da: Unknown
		 */
		if (aim_tlv_gettlv(innerlist, 0x00da, 1))
			;

		aim_tlvlist_free(&innerlist);
	}

	/*
	 * Call client.
	 */
	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, snac2->type, maxrooms, curexchange, exchanges);

	for (curexchange--; curexchange >= 0; curexchange--) {
		free(exchanges[curexchange].name);
		free(exchanges[curexchange].charset1);
		free(exchanges[curexchange].lang1);
		free(exchanges[curexchange].charset2);
		free(exchanges[curexchange].lang2);
	}
	free(exchanges);
	aim_tlvlist_free(&tlvlist);

	return ret;
}

static int parseinfo_create(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs, aim_snac_t *snac2)
{
	aim_rxcallback_t userfunc;
	aim_tlvlist_t *tlvlist, *innerlist;
	char *ck = NULL, *fqcn = NULL, *name = NULL;
	fu16_t exchange = 0, instance = 0, unknown = 0, flags = 0, maxmsglen = 0, maxoccupancy = 0;
	fu32_t createtime = 0;
	fu8_t createperms = 0, detaillevel;
	int cklen;
	aim_tlv_t *bigblock;
	int ret = 0;
	aim_bstream_t bbbs;

	tlvlist = aim_tlvlist_read(bs);

	if (!(bigblock = aim_tlv_gettlv(tlvlist, 0x0004, 1))) {
		faimdprintf(sess, 0, "no bigblock in top tlv in create room response\n");
		aim_tlvlist_free(&tlvlist);
		return 0;
	}

	aim_bstream_init(&bbbs, bigblock->value, bigblock->length);

	exchange = aimbs_get16(&bbbs);
	cklen = aimbs_get8(&bbbs);
	ck = aimbs_getstr(&bbbs, cklen);
	instance = aimbs_get16(&bbbs);
	detaillevel = aimbs_get8(&bbbs);

	if (detaillevel != 0x02) {
		faimdprintf(sess, 0, "unknown detaillevel in create room response (0x%02x)\n", detaillevel);
		aim_tlvlist_free(&tlvlist);
		free(ck);
		return 0;
	}

	unknown = aimbs_get16(&bbbs);

	innerlist = aim_tlvlist_read(&bbbs);

	if (aim_tlv_gettlv(innerlist, 0x006a, 1))
		fqcn = aim_tlv_getstr(innerlist, 0x006a, 1);

	if (aim_tlv_gettlv(innerlist, 0x00c9, 1))
		flags = aim_tlv_get16(innerlist, 0x00c9, 1);

	if (aim_tlv_gettlv(innerlist, 0x00ca, 1))
		createtime = aim_tlv_get32(innerlist, 0x00ca, 1);

	if (aim_tlv_gettlv(innerlist, 0x00d1, 1))
		maxmsglen = aim_tlv_get16(innerlist, 0x00d1, 1);

	if (aim_tlv_gettlv(innerlist, 0x00d2, 1))
		maxoccupancy = aim_tlv_get16(innerlist, 0x00d2, 1);

	if (aim_tlv_gettlv(innerlist, 0x00d3, 1))
		name = aim_tlv_getstr(innerlist, 0x00d3, 1);

	if (aim_tlv_gettlv(innerlist, 0x00d5, 1))
		createperms = aim_tlv_get8(innerlist, 0x00d5, 1);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype))) {
		ret = userfunc(sess, rx, snac2->type, fqcn, instance, exchange, flags, createtime, maxmsglen, maxoccupancy, createperms, unknown, name, ck);
	}

	free(ck);
	free(name);
	free(fqcn);
	aim_tlvlist_free(&innerlist);
	aim_tlvlist_free(&tlvlist);

	return ret;
}

/*
 * Subtype 0x0009
 *
 * Since multiple things can trigger this callback, we must lookup the
 * snacid to determine the original snac subtype that was called.
 *
 * XXX This isn't really how this works.  But this is:  Every d/9 response
 * has a 16bit value at the beginning. That matches to:
 *    Short Desc = 1
 *    Full Desc = 2
 *    Instance Info = 4
 *    Nav Short Desc = 8
 *    Nav Instance Info = 16
 * And then everything is really asynchronous.  There is no specific
 * attachment of a response to a create room request, for example.  Creating
 * the room yields no different a response than requesting the room's info.
 *
 */
static int parseinfo(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	aim_snac_t *snac2;
	int ret = 0;

	if (!(snac2 = aim_remsnac(sess, snac->id))) {
		faimdprintf(sess, 0, "faim: chatnav_parse_info: received response to unknown request! (%08lx)\n", snac->id);
		return 0;
	}

	if (snac2->family != 0x000d) {
		faimdprintf(sess, 0, "faim: chatnav_parse_info: recieved response that maps to corrupt request! (fam=%04x)\n", snac2->family);
		return 0;
	}

	/*
	 * We now know what the original SNAC subtype was.
	 */
	if (snac2->type == 0x0002) /* request chat rights */
		ret = parseinfo_perms(sess, mod, rx, snac, bs, snac2);
	else if (snac2->type == 0x0003) /* request exchange info */
		faimdprintf(sess, 0, "chatnav_parse_info: resposne to exchange info\n");
	else if (snac2->type == 0x0004) /* request room info */
		faimdprintf(sess, 0, "chatnav_parse_info: response to room info\n");
	else if (snac2->type == 0x0005) /* request more room info */
		faimdprintf(sess, 0, "chatnav_parse_info: response to more room info\n");
	else if (snac2->type == 0x0006) /* request occupant list */
		faimdprintf(sess, 0, "chatnav_parse_info: response to occupant info\n");
	else if (snac2->type == 0x0007) /* search for a room */
		faimdprintf(sess, 0, "chatnav_parse_info: search results\n");
	else if (snac2->type == 0x0008) /* create room */
		ret = parseinfo_create(sess, mod, rx, snac, bs, snac2);
	else
		faimdprintf(sess, 0, "chatnav_parse_info: unknown request subtype (%04x)\n", snac2->type);

	if (snac2)
		free(snac2->data);
	free(snac2);

	return ret;
}

static int snachandler(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{

	if (snac->subtype == 0x0009)
		return parseinfo(sess, mod, rx, snac, bs);

	return 0;
}

faim_internal int chatnav_modfirst(aim_session_t *sess, aim_module_t *mod)
{

	mod->family = 0x000d;
	mod->version = 0x0001;
	mod->toolid = 0x0010;
	mod->toolversion = 0x0629;
	mod->flags = 0;
	strncpy(mod->name, "chatnav", sizeof(mod->name));
	mod->snachandler = snachandler;

	return 0;
}
