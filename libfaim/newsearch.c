/*
 * Family 0x000f - Newer Search Method
 *
 * Used for searching for other AIM users by email address, name, 
 * location, commmon interests, and a few other similar things.
 *
 */

#define FAIM_INTERNAL
#include <aim.h>

/**
 * Subtype 0x0002 - Submit a User Search Request
 *
 * Search for an AIM screen name based on their email address.
 *
 * @param sess The oscar session.
 * @param region Should be "us-ascii" unless you know what you're doing.
 * @param email The email address you want to search for.
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_usersearch_email(aim_session_t *sess, const char *region, const char *email)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	aim_tlvlist_t *tl = NULL;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x000f)) || !region || !email)
		return -EINVAL;

	/* Create a TLV chain, write it to the outgoing frame, then free the chain */
	aim_addtlvtochain_raw(&tl, 0x001c, strlen(region), region);
	aim_addtlvtochain16(&tl, 0x000a, 0x0001); /* Type of search */
	aim_addtlvtochain_raw(&tl, 0x0005, strlen(email), email);

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+aim_sizetlvchain(&tl))))
		return -ENOMEM;
	snacid = aim_cachesnac(sess, 0x000f, 0x0002, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x000f, 0x0002, 0x0000, snacid);

	aim_writetlvchain(&fr->data, &tl);
	aim_freetlvchain(&tl);

	aim_tx_enqueue(sess, fr);

	return 0;
}


/**
 * Subtype 0x0002 - Submit a User Search Request
 *
 * Search for an AIM screen name based on various info 
 * about the person.
 *
 * @param sess The oscar session.
 * @param region Should be "us-ascii" unless you know what you're doing.
 * @param first The first name of the person you want to search for.
 * @param middle The middle name of the person you want to search for.
 * @param last The last name of the person you want to search for.
 * @param maiden The maiden name of the person you want to search for.
 * @param nick The nick name of the person you want to search for.
 * @param city The city where the person you want to search for resides.
 * @param state The state where the person you want to search for resides.
 * @param country The country where the person you want to search for resides.
 * @param zip The zip code where the person you want to search for resides.
 * @param address The street address where the person you want to seach for resides.
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_usersearch_name(aim_session_t *sess, const char *region, const char *first, const char *middle, const char *last, const char *maiden, const char *nick, const char *city, const char *state, const char *country, const char *zip, const char *address)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	aim_tlvlist_t *tl = NULL;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x000f)) || !region)
		return -EINVAL;

	/* Create a TLV chain, write it to the outgoing frame, then free the chain */
	aim_addtlvtochain_raw(&tl, 0x001c, strlen(region), region);
	aim_addtlvtochain16(&tl, 0x000a, 0x0000); /* Type of search */
	if (first)
		aim_addtlvtochain_raw(&tl, 0x0001, strlen(first), first);
	if (last)
		aim_addtlvtochain_raw(&tl, 0x0002, strlen(last), last);
	if (middle)
		aim_addtlvtochain_raw(&tl, 0x0003, strlen(middle), middle);
	if (maiden)
		aim_addtlvtochain_raw(&tl, 0x0004, strlen(maiden), maiden);
	if (country)
		aim_addtlvtochain_raw(&tl, 0x0006, strlen(country), country);
	if (state)
		aim_addtlvtochain_raw(&tl, 0x0007, strlen(state), state);
	if (city)
		aim_addtlvtochain_raw(&tl, 0x0008, strlen(city), city);
	if (nick)
		aim_addtlvtochain_raw(&tl, 0x000c, strlen(nick), nick);
	if (zip)
		aim_addtlvtochain_raw(&tl, 0x000d, strlen(zip), zip);
	if (address)
		aim_addtlvtochain_raw(&tl, 0x0021, strlen(address), address);

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+aim_sizetlvchain(&tl))))
		return -ENOMEM;
	snacid = aim_cachesnac(sess, 0x000f, 0x0002, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x000f, 0x0002, 0x0000, snacid);

	aim_writetlvchain(&fr->data, &tl);
	aim_freetlvchain(&tl);

	aim_tx_enqueue(sess, fr);

	return 0;
}


/**
 * Subtype 0x0002 - Submit a User Search Request
 *
 * @param sess The oscar session.
 * @param interest1 An interest you want to search for.
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_usersearch_interest(aim_session_t *sess, const char *region, const char *interest)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	aim_tlvlist_t *tl = NULL;

	if (!sess || !(conn = aim_conn_findbygroup(sess, 0x000f)) || !region)
		return -EINVAL;

	/* Create a TLV chain, write it to the outgoing frame, then free the chain */
	aim_addtlvtochain_raw(&tl, 0x001c, strlen(region), region);
	aim_addtlvtochain16(&tl, 0x000a, 0x0001); /* Type of search */
	if (interest)
		aim_addtlvtochain_raw(&tl, 0x0001, strlen(interest), interest);

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+aim_sizetlvchain(&tl))))
		return -ENOMEM;
	snacid = aim_cachesnac(sess, 0x000f, 0x0002, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, 0x000f, 0x0002, 0x0000, snacid);

	aim_writetlvchain(&fr->data, &tl);
	aim_freetlvchain(&tl);

	aim_tx_enqueue(sess, fr);

	return 0;
}


/**
 * Subtype 0x0003 - Receive Reply From a User Search
 *
 */
static int parseresults(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	aim_rxcallback_t userfunc;
	fu16_t tmp, numresults;
	struct aim_usersearch *results = NULL;

	tmp = aimbs_get16(bs); /* Unknown */
	tmp = aimbs_get16(bs); /* Unknown */
	aim_bstream_advance(bs, tmp);

	numresults = aimbs_get16(bs); /* Number of results to follow */

	/* Allocate a linked list, 1 node per result */
	while (numresults) {
		struct aim_usersearch *new;
		aim_tlvlist_t *tl = aim_readtlvchain_num(bs, aimbs_get16(bs));
		new = (struct aim_usersearch *)malloc(sizeof(struct aim_usersearch));
		new->first = aim_gettlv_str(tl, 0x0001, 1);
		new->last = aim_gettlv_str(tl, 0x0002, 1);
		new->middle = aim_gettlv_str(tl, 0x0003, 1);
		new->maiden = aim_gettlv_str(tl, 0x0004, 1);
		new->email = aim_gettlv_str(tl, 0x0005, 1);
		new->country = aim_gettlv_str(tl, 0x0006, 1);
		new->state = aim_gettlv_str(tl, 0x0007, 1);
		new->city = aim_gettlv_str(tl, 0x0008, 1);
		new->sn = aim_gettlv_str(tl, 0x0009, 1);
		new->interest = aim_gettlv_str(tl, 0x000b, 1);
		new->nick = aim_gettlv_str(tl, 0x000c, 1);
		new->zip = aim_gettlv_str(tl, 0x000d, 1);
		new->region = aim_gettlv_str(tl, 0x001c, 1);
		new->address = aim_gettlv_str(tl, 0x0021, 1);
		new->next = results;
		results = new;
		numresults--;
	}

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		return userfunc(sess, rx, results);

	/* Now free everything from above */
	while (results) {
		struct aim_usersearch *del = results;
		results = results->next;
		free(del->first);
		free(del->last);
		free(del->middle);
		free(del->maiden);
		free(del->email);
		free(del->country);
		free(del->state);
		free(del->city);
		free(del->sn);
		free(del->interest);
		free(del->nick);
		free(del->zip);
		free(del->region);
		free(del->address);
		free(del);
	}

	return 0;
}

static int snachandler(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{

	if (snac->subtype == 0x0003)
		return parseresults(sess, mod, rx, snac, bs);

	return 0;
}

faim_internal int newsearch_modfirst(aim_session_t *sess, aim_module_t *mod)
{

	mod->family = 0x000f;
	mod->version = 0x0001;
	mod->toolid = 0x0010;
	mod->toolversion = 0x0629;
	mod->flags = 0;
	strncpy(mod->name, "newsearch", sizeof(mod->name));
	mod->snachandler = snachandler;

	return 0;
}
