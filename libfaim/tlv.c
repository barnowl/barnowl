
#define FAIM_INTERNAL
#include <aim.h>

static aim_tlv_t *createtlv(void)
{
	aim_tlv_t *newtlv;

	if (!(newtlv = (aim_tlv_t *)malloc(sizeof(aim_tlv_t))))
		return NULL;
	memset(newtlv, 0, sizeof(aim_tlv_t));

	return newtlv;
}

static void freetlv(aim_tlv_t **oldtlv)
{

	if (!oldtlv || !*oldtlv)
		return;
	
	free((*oldtlv)->value);
	free(*oldtlv);
	*oldtlv = NULL;

	return;
}

/**
 * aim_readtlvchain - Read a TLV chain from a buffer.
 * @param bs Input bstream
 *
 * Reads and parses a series of TLV patterns from a data buffer; the
 * returned structure is manipulatable with the rest of the TLV
 * routines.  When done with a TLV chain, aim_freetlvchain() should
 * be called to free the dynamic substructures.
 *
 * XXX There should be a flag setable here to have the tlvlist contain
 * bstream references, so that at least the ->value portion of each 
 * element doesn't need to be malloc/memcpy'd.  This could prove to be
 * just as effecient as the in-place TLV parsing used in a couple places
 * in libfaim.
 *
 */
faim_internal aim_tlvlist_t *aim_readtlvchain(aim_bstream_t *bs)
{
	aim_tlvlist_t *list = NULL, *cur;
	
	while (aim_bstream_empty(bs) > 0) {
		fu16_t type, length;

		type = aimbs_get16(bs);
		length = aimbs_get16(bs);

#if 0 /* temporarily disabled until I know if they're still doing it or not */
		/*
		 * Okay, so now AOL has decided that any TLV of
		 * type 0x0013 can only be two bytes, despite
		 * what the actual given length is.  So here 
		 * we dump any invalid TLVs of that sort.  Hopefully
		 * theres no special cases to this special case.
		 *   - mid (30jun2000)
		 */
		if ((type == 0x0013) && (length != 0x0002))
			length = 0x0002;
#else
		if (0)
			;
#endif
		else {

			if (length > aim_bstream_empty(bs)) {
				aim_freetlvchain(&list);
				return NULL;
			}

			cur = (aim_tlvlist_t *)malloc(sizeof(aim_tlvlist_t));
			if (!cur) {
				aim_freetlvchain(&list);
				return NULL;
			}

			memset(cur, 0, sizeof(aim_tlvlist_t));

			cur->tlv = createtlv();
			if (!cur->tlv) {
				free(cur);
				aim_freetlvchain(&list);
				return NULL;
			}
			cur->tlv->type = type;
			if ((cur->tlv->length = length)) {
			       cur->tlv->value = aimbs_getraw(bs, length);	
			       if (!cur->tlv->value) {
				       freetlv(&cur->tlv);
				       free(cur);
				       aim_freetlvchain(&list);
				       return NULL;
			       }
			}

			cur->next = list;
			list = cur;
		}
	}

	return list;
}

/**
 * aim_readtlvchain_num - Read a TLV chain from a buffer.
 * @param bs Input bstream
 * @param num The max number of TLVs that will be read, or -1 if unlimited.  
 *        There are a number of places where you want to read in a tlvchain, 
 *        but the chain is not at the end of the SNAC, and the chain is 
 *        preceeded by the number of TLVs.  So you can limit that with this.
 *
 * Reads and parses a series of TLV patterns from a data buffer; the
 * returned structure is manipulatable with the rest of the TLV
 * routines.  When done with a TLV chain, aim_freetlvchain() should
 * be called to free the dynamic substructures.
 *
 * XXX There should be a flag setable here to have the tlvlist contain
 * bstream references, so that at least the ->value portion of each 
 * element doesn't need to be malloc/memcpy'd.  This could prove to be
 * just as effecient as the in-place TLV parsing used in a couple places
 * in libfaim.
 *
 */
faim_internal aim_tlvlist_t *aim_readtlvchain_num(aim_bstream_t *bs, fu16_t num)
{
	aim_tlvlist_t *list = NULL, *cur;

	while ((aim_bstream_empty(bs) > 0) && (num != 0)) {
		fu16_t type, length;

		type = aimbs_get16(bs);
		length = aimbs_get16(bs);

		if (length > aim_bstream_empty(bs)) {
			aim_freetlvchain(&list);
			return NULL;
		}

		cur = (aim_tlvlist_t *)malloc(sizeof(aim_tlvlist_t));
		if (!cur) {
			aim_freetlvchain(&list);
			return NULL;
		}

		memset(cur, 0, sizeof(aim_tlvlist_t));

		cur->tlv = createtlv();
		if (!cur->tlv) {
			free(cur);
			aim_freetlvchain(&list);
			return NULL;
		}
		cur->tlv->type = type;
		if ((cur->tlv->length = length)) {
		       cur->tlv->value = aimbs_getraw(bs, length);
		       if (!cur->tlv->value) {
			       freetlv(&cur->tlv);
			       free(cur);
			       aim_freetlvchain(&list);
			       return NULL;
		       }
		}

		num--;
		cur->next = list;
		list = cur;
	}

	return list;
}

/**
 * aim_readtlvchain_len - Read a TLV chain from a buffer.
 * @param bs Input bstream
 * @param len The max length in bytes that will be read.
 *        There are a number of places where you want to read in a tlvchain, 
 *        but the chain is not at the end of the SNAC, and the chain is 
 *        preceeded by the length of the TLVs.  So you can limit that with this.
 *
 * Reads and parses a series of TLV patterns from a data buffer; the
 * returned structure is manipulatable with the rest of the TLV
 * routines.  When done with a TLV chain, aim_freetlvchain() should
 * be called to free the dynamic substructures.
 *
 * XXX There should be a flag setable here to have the tlvlist contain
 * bstream references, so that at least the ->value portion of each 
 * element doesn't need to be malloc/memcpy'd.  This could prove to be
 * just as effecient as the in-place TLV parsing used in a couple places
 * in libfaim.
 *
 */
faim_internal aim_tlvlist_t *aim_readtlvchain_len(aim_bstream_t *bs, fu16_t len)
{
	aim_tlvlist_t *list = NULL, *cur;

	while ((aim_bstream_empty(bs) > 0) && (len > 0)) {
		fu16_t type, length;

		type = aimbs_get16(bs);
		length = aimbs_get16(bs);

		if (length > aim_bstream_empty(bs)) {
			aim_freetlvchain(&list);
			return NULL;
		}

		cur = (aim_tlvlist_t *)malloc(sizeof(aim_tlvlist_t));
		if (!cur) {
			aim_freetlvchain(&list);
			return NULL;
		}

		memset(cur, 0, sizeof(aim_tlvlist_t));

		cur->tlv = createtlv();
		if (!cur->tlv) {
			free(cur);
			aim_freetlvchain(&list);
			return NULL;
		}
		cur->tlv->type = type;
		if ((cur->tlv->length = length)) {
		       cur->tlv->value = aimbs_getraw(bs, length);
		       if (!cur->tlv->value) {
			       freetlv(&cur->tlv);
			       free(cur);
			       aim_freetlvchain(&list);
			       return NULL;
		       }
		}

		len -= aim_sizetlvchain(&cur);
		cur->next = list;
		list = cur;
	}

	return list;
}

/**
 * aim_tlvlist_copy - Duplicate a TLV chain.
 * @param orig
 *
 * This is pretty pelf exslanatory.
 *
 */
faim_internal aim_tlvlist_t *aim_tlvlist_copy(aim_tlvlist_t *orig)
{
	aim_tlvlist_t *new = NULL;

	while (orig) {
		aim_addtlvtochain_raw(&new, orig->tlv->type, orig->tlv->length, orig->tlv->value);
		orig = orig->next;
	}

	return new;
}

/*
 * Compare two TLV lists for equality.  This probably is not the most 
 * efficient way to do this.
 *
 * @param one One of the TLV chains to compare.
 * @param two The other TLV chain to compare.
 * @preturn Retrun 0 if the lists are the same, return 1 if they are different.
 */
faim_internal int aim_tlvlist_cmp(aim_tlvlist_t *one, aim_tlvlist_t *two)
{
	aim_bstream_t bs1, bs2;

	if (aim_sizetlvchain(&one) != aim_sizetlvchain(&two))
		return 1;

	aim_bstream_init(&bs1, ((fu8_t *)malloc(aim_sizetlvchain(&one)*sizeof(fu8_t))), aim_sizetlvchain(&one));
	aim_bstream_init(&bs2, ((fu8_t *)malloc(aim_sizetlvchain(&two)*sizeof(fu8_t))), aim_sizetlvchain(&two));

	aim_writetlvchain(&bs1, &one);
	aim_writetlvchain(&bs2, &two);

	if (memcmp(bs1.data, bs2.data, bs1.len)) {
		free(bs1.data);
		free(bs2.data);
		return 1;
	}

	free(bs1.data);
	free(bs2.data);

	return 0;
}

/**
 * aim_freetlvchain - Free a TLV chain structure
 * @list: Chain to be freed
 *
 * Walks the list of TLVs in the passed TLV chain and
 * frees each one. Note that any references to this data
 * should be removed before calling this.
 *
 */
faim_internal void aim_freetlvchain(aim_tlvlist_t **list)
{
	aim_tlvlist_t *cur;

	if (!list || !*list)
		return;

	for (cur = *list; cur; ) {
		aim_tlvlist_t *tmp;
		
		freetlv(&cur->tlv);

		tmp = cur->next;
		free(cur);
		cur = tmp;
	}

	list = NULL;

	return;
}

/**
 * aim_counttlvchain - Count the number of TLVs in a chain
 * @list: Chain to be counted
 *
 * Returns the number of TLVs stored in the passed chain.
 *
 */
faim_internal int aim_counttlvchain(aim_tlvlist_t **list)
{
	aim_tlvlist_t *cur;
	int count;

	if (!list || !*list)
		return 0;

	for (cur = *list, count = 0; cur; cur = cur->next)
		count++;

	return count;
}

/**
 * aim_sizetlvchain - Count the number of bytes in a TLV chain
 * @list: Chain to be sized
 *
 * Returns the number of bytes that would be needed to 
 * write the passed TLV chain to a data buffer.
 *
 */
faim_internal int aim_sizetlvchain(aim_tlvlist_t **list)
{
	aim_tlvlist_t *cur;
	int size;

	if (!list || !*list)
		return 0;

	for (cur = *list, size = 0; cur; cur = cur->next)
		size += (4 + cur->tlv->length);

	return size;
}

/**
 * aim_addtlvtochain_raw - Add a string to a TLV chain
 * @list: Desination chain (%NULL pointer if empty)
 * @t: TLV type
 * @l: Length of string to add (not including %NULL)
 * @v: String to add
 *
 * Adds the passed string as a TLV element of the passed type
 * to the TLV chain.
 *
 */
faim_internal int aim_addtlvtochain_raw(aim_tlvlist_t **list, const fu16_t t, const fu16_t l, const fu8_t *v)
{
	aim_tlvlist_t *newtlv, *cur;

	if (!list)
		return 0;

	if (!(newtlv = (aim_tlvlist_t *)malloc(sizeof(aim_tlvlist_t))))
		return 0;
	memset(newtlv, 0x00, sizeof(aim_tlvlist_t));

	if (!(newtlv->tlv = createtlv())) {
		free(newtlv);
		return 0;
	}
	newtlv->tlv->type = t;
	if ((newtlv->tlv->length = l)) {
		newtlv->tlv->value = (fu8_t *)malloc(newtlv->tlv->length);
		memcpy(newtlv->tlv->value, v, newtlv->tlv->length);
	}

	if (!*list)
		*list = newtlv;
	else {
		for(cur = *list; cur->next; cur = cur->next)
			;
		cur->next = newtlv;
	}

	return newtlv->tlv->length;
}

/**
 * aim_addtlvtochain8 - Add a 8bit integer to a TLV chain
 * @list: Destination chain
 * @type: TLV type to add
 * @val: Value to add
 *
 * Adds a one-byte unsigned integer to a TLV chain.
 *
 */
faim_internal int aim_addtlvtochain8(aim_tlvlist_t **list, const fu16_t t, const fu8_t v)
{
	fu8_t v8[1];

	aimutil_put8(v8, v);

	return aim_addtlvtochain_raw(list, t, 1, v8);
}

/**
 * aim_addtlvtochain16 - Add a 16bit integer to a TLV chain
 * @list: Destination chain
 * @t: TLV type to add
 * @v: Value to add
 *
 * Adds a two-byte unsigned integer to a TLV chain.
 *
 */
faim_internal int aim_addtlvtochain16(aim_tlvlist_t **list, const fu16_t t, const fu16_t v)
{
	fu8_t v16[2];

	aimutil_put16(v16, v);

	return aim_addtlvtochain_raw(list, t, 2, v16);
}

/**
 * aim_addtlvtochain32 - Add a 32bit integer to a TLV chain
 * @list: Destination chain
 * @type: TLV type to add
 * @val: Value to add
 *
 * Adds a four-byte unsigned integer to a TLV chain.
 *
 */
faim_internal int aim_addtlvtochain32(aim_tlvlist_t **list, const fu16_t t, const fu32_t v)
{
	fu8_t v32[4];

	aimutil_put32(v32, v);

	return aim_addtlvtochain_raw(list, t, 4, v32);
}

/**
 * aim_addtlvtochain_caps - Add a capability block to a TLV chain
 * @list: Destination chain
 * @type: TLV type to add
 * @caps: Bitfield of capability flags to send
 *
 * Adds a block of capability blocks to a TLV chain. The bitfield
 * passed in should be a bitwise %OR of any of the %AIM_CAPS constants:
 *
 *      %AIM_CAPS_BUDDYICON   Supports Buddy Icons
 *
 *      %AIM_CAPS_VOICE       Supports Voice Chat
 *
 *      %AIM_CAPS_IMIMAGE     Supports DirectIM/IMImage
 *
 *      %AIM_CAPS_CHAT        Supports Chat
 *
 *      %AIM_CAPS_GETFILE     Supports Get File functions
 *
 *      %AIM_CAPS_SENDFILE    Supports Send File functions
 *
 */
faim_internal int aim_addtlvtochain_caps(aim_tlvlist_t **list, const fu16_t t, const fu32_t caps)
{
	fu8_t buf[16*16]; /* XXX icky fixed length buffer */
	aim_bstream_t bs;

	if (!caps)
		return 0; /* nothing there anyway */

	aim_bstream_init(&bs, buf, sizeof(buf));

	aim_putcap(&bs, caps);

	return aim_addtlvtochain_raw(list, t, aim_bstream_curpos(&bs), buf);
}

faim_internal int aim_addtlvtochain_userinfo(aim_tlvlist_t **list, fu16_t type, aim_userinfo_t *ui)
{
	fu8_t buf[1024]; /* bleh */
	aim_bstream_t bs;

	aim_bstream_init(&bs, buf, sizeof(buf));

	aim_putuserinfo(&bs, ui);

	return aim_addtlvtochain_raw(list, type, aim_bstream_curpos(&bs), buf);
}

/**
 * aim_addtlvtochain_noval - Add a blank TLV to a TLV chain
 * @list: Destination chain
 * @type: TLV type to add
 *
 * Adds a TLV with a zero length to a TLV chain.
 *
 */
faim_internal int aim_addtlvtochain_noval(aim_tlvlist_t **list, const fu16_t t)
{
	return aim_addtlvtochain_raw(list, t, 0, NULL);
}

/*
 * Note that the inner TLV chain will not be modifiable as a tlvchain once
 * it is written using this.  Or rather, it can be, but updates won't be
 * made to this.
 *
 * XXX should probably support sublists for real.
 * 
 * This is so neat.
 *
 */
faim_internal int aim_addtlvtochain_frozentlvlist(aim_tlvlist_t **list, fu16_t type, aim_tlvlist_t **tl)
{
	fu8_t *buf;
	int buflen;
	aim_bstream_t bs;

	buflen = aim_sizetlvchain(tl);

	if (buflen <= 0)
		return 0;

	if (!(buf = malloc(buflen)))
		return 0;

	aim_bstream_init(&bs, buf, buflen);

	aim_writetlvchain(&bs, tl);

	aim_addtlvtochain_raw(list, type, aim_bstream_curpos(&bs), buf);

	free(buf);

	return buflen;
}

/**
 * aim_writetlvchain - Write a TLV chain into a data buffer.
 * @buf: Destination buffer
 * @buflen: Maximum number of bytes that will be written to buffer
 * @list: Source TLV chain
 *
 * Copies a TLV chain into a raw data buffer, writing only the number
 * of bytes specified. This operation does not free the chain; 
 * aim_freetlvchain() must still be called to free up the memory used
 * by the chain structures.
 *
 * XXX clean this up, make better use of bstreams 
 */
faim_internal int aim_writetlvchain(aim_bstream_t *bs, aim_tlvlist_t **list)
{
	int goodbuflen;
	aim_tlvlist_t *cur;

	/* do an initial run to test total length */
	goodbuflen = aim_sizetlvchain(list);

	if (goodbuflen > aim_bstream_empty(bs))
		return 0; /* not enough buffer */

	/* do the real write-out */
	for (cur = *list; cur; cur = cur->next) {
		aimbs_put16(bs, cur->tlv->type);
		aimbs_put16(bs, cur->tlv->length);
		if (cur->tlv->length)
			aimbs_putraw(bs, cur->tlv->value, cur->tlv->length);
	}

	return 1; /* XXX this is a nonsensical return */
}


/**
 * aim_gettlv - Grab the Nth TLV of type type in the TLV list list.
 * @list: Source chain
 * @type: Requested TLV type
 * @nth: Index of TLV of type to get
 *
 * Returns a pointer to an aim_tlv_t of the specified type; 
 * %NULL on error.  The @nth parameter is specified starting at %1.
 * In most cases, there will be no more than one TLV of any type
 * in a chain.
 *
 */
faim_internal aim_tlv_t *aim_gettlv(aim_tlvlist_t *list, const fu16_t t, const int n)
{
	aim_tlvlist_t *cur;
	int i;

	for (cur = list, i = 0; cur; cur = cur->next) {
		if (cur && cur->tlv) {
			if (cur->tlv->type == t)
				i++;
			if (i >= n)
				return cur->tlv;
		}
	}

	return NULL;
}

/**
 * aim_gettlv_str - Retrieve the Nth TLV in chain as a string.
 * @list: Source TLV chain
 * @type: TLV type to search for
 * @nth: Index of TLV to return
 *
 * Same as aim_gettlv(), except that the return value is a %NULL-
 * terminated string instead of an aim_tlv_t.  This is a 
 * dynamic buffer and must be freed by the caller.
 *
 */
faim_internal char *aim_gettlv_str(aim_tlvlist_t *list, const fu16_t t, const int n)
{
	aim_tlv_t *tlv;
	char *newstr;

	if (!(tlv = aim_gettlv(list, t, n)))
		return NULL;

	newstr = (char *) malloc(tlv->length + 1);
	memcpy(newstr, tlv->value, tlv->length);
	*(newstr + tlv->length) = '\0';

	return newstr;
}

/**
 * aim_gettlv8 - Retrieve the Nth TLV in chain as a 8bit integer.
 * @list: Source TLV chain
 * @type: TLV type to search for
 * @nth: Index of TLV to return
 *
 * Same as aim_gettlv(), except that the return value is a 
 * 8bit integer instead of an aim_tlv_t. 
 *
 */
faim_internal fu8_t aim_gettlv8(aim_tlvlist_t *list, const fu16_t t, const int n)
{
	aim_tlv_t *tlv;

	if (!(tlv = aim_gettlv(list, t, n)))
		return 0; /* erm */
	return aimutil_get8(tlv->value);
}

/**
 * aim_gettlv16 - Retrieve the Nth TLV in chain as a 16bit integer.
 * @list: Source TLV chain
 * @type: TLV type to search for
 * @nth: Index of TLV to return
 *
 * Same as aim_gettlv(), except that the return value is a 
 * 16bit integer instead of an aim_tlv_t. 
 *
 */
faim_internal fu16_t aim_gettlv16(aim_tlvlist_t *list, const fu16_t t, const int n)
{
	aim_tlv_t *tlv;

	if (!(tlv = aim_gettlv(list, t, n)))
		return 0; /* erm */
	return aimutil_get16(tlv->value);
}

/**
 * aim_gettlv32 - Retrieve the Nth TLV in chain as a 32bit integer.
 * @list: Source TLV chain
 * @type: TLV type to search for
 * @nth: Index of TLV to return
 *
 * Same as aim_gettlv(), except that the return value is a 
 * 32bit integer instead of an aim_tlv_t. 
 *
 */
faim_internal fu32_t aim_gettlv32(aim_tlvlist_t *list, const fu16_t t, const int n)
{
	aim_tlv_t *tlv;

	if (!(tlv = aim_gettlv(list, t, n)))
		return 0; /* erm */
	return aimutil_get32(tlv->value);
}

#if 0
/**
 * aim_puttlv_8 - Write a one-byte TLV.
 * @buf: Destination buffer
 * @t: TLV type
 * @v: Value
 *
 * Writes a TLV with a one-byte integer value portion.
 *
 */
faim_export int aim_puttlv_8(fu8_t *buf, const fu16_t t, const fu8_t v)
{
	fu8_t v8[1];

	aimutil_put8(v8, v);

	return aim_puttlv_raw(buf, t, 1, v8);
}

/**
 * aim_puttlv_16 - Write a two-byte TLV.
 * @buf: Destination buffer
 * @t: TLV type
 * @v: Value
 *
 * Writes a TLV with a two-byte integer value portion.
 *
 */
faim_export int aim_puttlv_16(fu8_t *buf, const fu16_t t, const fu16_t v)
{
	fu8_t v16[2];

	aimutil_put16(v16, v);

	return aim_puttlv_raw(buf, t, 2, v16);
}


/**
 * aim_puttlv_32 - Write a four-byte TLV.
 * @buf: Destination buffer
 * @t: TLV type
 * @v: Value
 *
 * Writes a TLV with a four-byte integer value portion.
 *
 */
faim_export int aim_puttlv_32(fu8_t *buf, const fu16_t t, const fu32_t v)
{
	fu8_t v32[4];

	aimutil_put32(v32, v);

	return aim_puttlv_raw(buf, t, 4, v32);
}

/**
 * aim_puttlv_raw - Write a raw TLV.
 * @buf: Destination buffer
 * @t: TLV type
 * @l: Length of string
 * @v: String to write
 *
 * Writes a TLV with a raw value portion.  (Only the first @l
 * bytes of the passed buffer will be written, which should not
 * include a terminating NULL.)
 *
 */
faim_export int aim_puttlv_raw(fu8_t *buf, const fu16_t t, const fu16_t l, const fu8_t *v)
{
	int i;

	i = aimutil_put16(buf, t);
	i += aimutil_put16(buf+i, l);
	if (l)
		memcpy(buf+i, v, l);
	i += l;

	return i;
}
#endif
