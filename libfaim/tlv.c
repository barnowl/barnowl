
#define FAIM_INTERNAL
#include <aim.h>

static aim_tlv_t *createtlv(fu16_t type, fu16_t length, fu8_t *value)
{
	aim_tlv_t *ret;

	if (!(ret = (aim_tlv_t *)malloc(sizeof(aim_tlv_t))))
		return NULL;
	ret->type = type;
	ret->length = length;
	ret->value = value;

	return ret;
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
 * Read a TLV chain from a buffer.
 *
 * Reads and parses a series of TLV patterns from a data buffer; the
 * returned structure is manipulatable with the rest of the TLV
 * routines.  When done with a TLV chain, aim_tlvlist_free() should
 * be called to free the dynamic substructures.
 *
 * XXX There should be a flag setable here to have the tlvlist contain
 * bstream references, so that at least the ->value portion of each 
 * element doesn't need to be malloc/memcpy'd.  This could prove to be
 * just as effecient as the in-place TLV parsing used in a couple places
 * in libfaim.
 *
 * @param bs Input bstream
 */
faim_internal aim_tlvlist_t *aim_tlvlist_read(aim_bstream_t *bs)
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
				aim_tlvlist_free(&list);
				return NULL;
			}

			cur = (aim_tlvlist_t *)malloc(sizeof(aim_tlvlist_t));
			if (!cur) {
				aim_tlvlist_free(&list);
				return NULL;
			}

			memset(cur, 0, sizeof(aim_tlvlist_t));

			cur->tlv = createtlv(type, length, NULL);
			if (!cur->tlv) {
				free(cur);
				aim_tlvlist_free(&list);
				return NULL;
			}
			if (cur->tlv->length > 0) {
				cur->tlv->value = aimbs_getraw(bs, length);	
				if (!cur->tlv->value) {
					freetlv(&cur->tlv);
					free(cur);
					aim_tlvlist_free(&list);
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
 * Read a TLV chain from a buffer.
 *
 * Reads and parses a series of TLV patterns from a data buffer; the
 * returned structure is manipulatable with the rest of the TLV
 * routines.  When done with a TLV chain, aim_tlvlist_free() should
 * be called to free the dynamic substructures.
 *
 * XXX There should be a flag setable here to have the tlvlist contain
 * bstream references, so that at least the ->value portion of each 
 * element doesn't need to be malloc/memcpy'd.  This could prove to be
 * just as effecient as the in-place TLV parsing used in a couple places
 * in libfaim.
 *
 * @param bs Input bstream
 * @param num The max number of TLVs that will be read, or -1 if unlimited.  
 *        There are a number of places where you want to read in a tlvchain, 
 *        but the chain is not at the end of the SNAC, and the chain is 
 *        preceeded by the number of TLVs.  So you can limit that with this.
 */
faim_internal aim_tlvlist_t *aim_tlvlist_readnum(aim_bstream_t *bs, fu16_t num)
{
	aim_tlvlist_t *list = NULL, *cur;

	while ((aim_bstream_empty(bs) > 0) && (num != 0)) {
		fu16_t type, length;

		type = aimbs_get16(bs);
		length = aimbs_get16(bs);

		if (length > aim_bstream_empty(bs)) {
			aim_tlvlist_free(&list);
			return NULL;
		}

		cur = (aim_tlvlist_t *)malloc(sizeof(aim_tlvlist_t));
		if (!cur) {
			aim_tlvlist_free(&list);
			return NULL;
		}

		memset(cur, 0, sizeof(aim_tlvlist_t));

		cur->tlv = createtlv(type, length, NULL);
		if (!cur->tlv) {
			free(cur);
			aim_tlvlist_free(&list);
			return NULL;
		}
		if (cur->tlv->length > 0) {
			cur->tlv->value = aimbs_getraw(bs, length);
			if (!cur->tlv->value) {
				freetlv(&cur->tlv);
				free(cur);
				aim_tlvlist_free(&list);
				return NULL;
			}
		}

		if (num > 0)
			num--;
		cur->next = list;
		list = cur;
	}

	return list;
}

/**
 * Read a TLV chain from a buffer.
 *
 * Reads and parses a series of TLV patterns from a data buffer; the
 * returned structure is manipulatable with the rest of the TLV
 * routines.  When done with a TLV chain, aim_tlvlist_free() should
 * be called to free the dynamic substructures.
 *
 * XXX There should be a flag setable here to have the tlvlist contain
 * bstream references, so that at least the ->value portion of each 
 * element doesn't need to be malloc/memcpy'd.  This could prove to be
 * just as effecient as the in-place TLV parsing used in a couple places
 * in libfaim.
 *
 * @param bs Input bstream
 * @param len The max length in bytes that will be read.
 *        There are a number of places where you want to read in a tlvchain, 
 *        but the chain is not at the end of the SNAC, and the chain is 
 *        preceeded by the length of the TLVs.  So you can limit that with this.
 */
faim_internal aim_tlvlist_t *aim_tlvlist_readlen(aim_bstream_t *bs, fu16_t len)
{
	aim_tlvlist_t *list = NULL, *cur;

	while ((aim_bstream_empty(bs) > 0) && (len > 0)) {
		fu16_t type, length;

		type = aimbs_get16(bs);
		length = aimbs_get16(bs);

		if (length > aim_bstream_empty(bs)) {
			aim_tlvlist_free(&list);
			return NULL;
		}

		cur = (aim_tlvlist_t *)malloc(sizeof(aim_tlvlist_t));
		if (!cur) {
			aim_tlvlist_free(&list);
			return NULL;
		}

		memset(cur, 0, sizeof(aim_tlvlist_t));

		cur->tlv = createtlv(type, length, NULL);
		if (!cur->tlv) {
			free(cur);
			aim_tlvlist_free(&list);
			return NULL;
		}
		if (cur->tlv->length > 0) {
			cur->tlv->value = aimbs_getraw(bs, length);
			if (!cur->tlv->value) {
				freetlv(&cur->tlv);
				free(cur);
				aim_tlvlist_free(&list);
				return NULL;
			}
		}

		len -= aim_tlvlist_size(&cur);
		cur->next = list;
		list = cur;
	}

	return list;
}

/**
 * Duplicate a TLV chain.
 * This is pretty pelf exslanatory.
 *
 * @param orig The TLV chain you want to make a copy of.
 * @return A newly allocated TLV chain.
 */
faim_internal aim_tlvlist_t *aim_tlvlist_copy(aim_tlvlist_t *orig)
{
	aim_tlvlist_t *new = NULL;

	while (orig) {
		aim_tlvlist_add_raw(&new, orig->tlv->type, orig->tlv->length, orig->tlv->value);
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

	if (aim_tlvlist_size(&one) != aim_tlvlist_size(&two))
		return 1;

	aim_bstream_init(&bs1, ((fu8_t *)malloc(aim_tlvlist_size(&one)*sizeof(fu8_t))), aim_tlvlist_size(&one));
	aim_bstream_init(&bs2, ((fu8_t *)malloc(aim_tlvlist_size(&two)*sizeof(fu8_t))), aim_tlvlist_size(&two));

	aim_tlvlist_write(&bs1, &one);
	aim_tlvlist_write(&bs2, &two);

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
 * Free a TLV chain structure
 * @list: Chain to be freed
 *
 * Walks the list of TLVs in the passed TLV chain and
 * frees each one. Note that any references to this data
 * should be removed before calling this.
 *
 */
faim_internal void aim_tlvlist_free(aim_tlvlist_t **list)
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
 * Count the number of TLVs in a chain.
 *
 * @param list Chain to be counted.
 * @return The number of TLVs stored in the passed chain.
 */
faim_internal int aim_tlvlist_count(aim_tlvlist_t **list)
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
 * Count the number of bytes in a TLV chain.
 *
 * @param list Chain to be sized
 * @return The number of bytes that would be needed to 
 *         write the passed TLV chain to a data buffer.
 */
faim_internal int aim_tlvlist_size(aim_tlvlist_t **list)
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
 * Adds the passed string as a TLV element of the passed type
 * to the TLV chain.
 *
 * @param list Desination chain (%NULL pointer if empty).
 * @param type TLV type.
 * @length Length of string to add (not including %NULL).
 * @value String to add.
 * @retun The size of the value added.
 */
faim_internal int aim_tlvlist_add_raw(aim_tlvlist_t **list, const fu16_t type, const fu16_t length, const fu8_t *value)
{
	aim_tlvlist_t *newtlv, *cur;

	if (list == NULL)
		return 0;

	if (!(newtlv = (aim_tlvlist_t *)malloc(sizeof(aim_tlvlist_t))))
		return 0;
	memset(newtlv, 0x00, sizeof(aim_tlvlist_t));

	if (!(newtlv->tlv = createtlv(type, length, NULL))) {
		free(newtlv);
		return 0;
	}
	if (newtlv->tlv->length > 0) {
		newtlv->tlv->value = (fu8_t *)malloc(newtlv->tlv->length);
		memcpy(newtlv->tlv->value, value, newtlv->tlv->length);
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
 * Add a one byte integer to a TLV chain.
 *
 * @param list Destination chain.
 * @param type TLV type to add.
 * @param value Value to add.
 * @retun The size of the value added.
 */
faim_internal int aim_tlvlist_add_8(aim_tlvlist_t **list, const fu16_t type, const fu8_t value)
{
	fu8_t v8[1];

	aimutil_put8(v8, value);

	return aim_tlvlist_add_raw(list, type, 1, v8);
}

/**
 * Add a two byte integer to a TLV chain.
 *
 * @param list Destination chain.
 * @param type TLV type to add.
 * @param value Value to add.
 * @retun The size of the value added.
 */
faim_internal int aim_tlvlist_add_16(aim_tlvlist_t **list, const fu16_t type, const fu16_t value)
{
	fu8_t v16[2];

	aimutil_put16(v16, value);

	return aim_tlvlist_add_raw(list, type, 2, v16);
}

/**
 * Add a four byte integer to a TLV chain.
 *
 * @param list Destination chain.
 * @param type TLV type to add.
 * @param value Value to add.
 * @retun The size of the value added.
 */
faim_internal int aim_tlvlist_add_32(aim_tlvlist_t **list, const fu16_t type, const fu32_t value)
{
	fu8_t v32[4];

	aimutil_put32(v32, value);

	return aim_tlvlist_add_raw(list, type, 4, v32);
}

/**
 * Adds a block of capability blocks to a TLV chain. The bitfield
 * passed in should be a bitwise %OR of any of the %AIM_CAPS constants:
 *
 *     %AIM_CAPS_BUDDYICON   Supports Buddy Icons
 *     %AIM_CAPS_VOICE       Supports Voice Chat
 *     %AIM_CAPS_IMIMAGE     Supports DirectIM/IMImage
 *     %AIM_CAPS_CHAT        Supports Chat
 *     %AIM_CAPS_GETFILE     Supports Get File functions
 *     %AIM_CAPS_SENDFILE    Supports Send File functions
 *
 * @param list Destination chain
 * @param type TLV type to add
 * @param caps Bitfield of capability flags to send
 * @retun The size of the value added.
 */
faim_internal int aim_tlvlist_add_caps(aim_tlvlist_t **list, const fu16_t type, const fu32_t caps)
{
	fu8_t buf[16*16]; /* XXX icky fixed length buffer */
	aim_bstream_t bs;

	if (!caps)
		return 0; /* nothing there anyway */

	aim_bstream_init(&bs, buf, sizeof(buf));

	aim_putcap(&bs, caps);

	return aim_tlvlist_add_raw(list, type, aim_bstream_curpos(&bs), buf);
}

/**
 * Adds the given userinfo struct to a TLV chain.
 *
 * @param list Destination chain.
 * @param type TLV type to add.
 * @retun The size of the value added.
 */
faim_internal int aim_tlvlist_add_userinfo(aim_tlvlist_t **list, fu16_t type, aim_userinfo_t *userinfo)
{
	fu8_t buf[1024]; /* bleh */
	aim_bstream_t bs;

	aim_bstream_init(&bs, buf, sizeof(buf));

	aim_putuserinfo(&bs, userinfo);

	return aim_tlvlist_add_raw(list, type, aim_bstream_curpos(&bs), buf);
}

/**
 * Adds a TLV with a zero length to a TLV chain.
 *
 * @param list Destination chain.
 * @param type TLV type to add.
 * @retun The size of the value added.
 */
faim_internal int aim_tlvlist_add_noval(aim_tlvlist_t **list, const fu16_t type)
{
	return aim_tlvlist_add_raw(list, type, 0, NULL);
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
faim_internal int aim_tlvlist_add_frozentlvlist(aim_tlvlist_t **list, fu16_t type, aim_tlvlist_t **tl)
{
	fu8_t *buf;
	int buflen;
	aim_bstream_t bs;

	buflen = aim_tlvlist_size(tl);

	if (buflen <= 0)
		return 0;

	if (!(buf = malloc(buflen)))
		return 0;

	aim_bstream_init(&bs, buf, buflen);

	aim_tlvlist_write(&bs, tl);

	aim_tlvlist_add_raw(list, type, aim_bstream_curpos(&bs), buf);

	free(buf);

	return buflen;
}

/**
 * Substitute a TLV of a given type with a new TLV of the same type.  If 
 * you attempt to replace a TLV that does not exist, this function will 
 * just add a new TLV as if you called aim_tlvlist_add_raw().
 *
 * @param list Desination chain (%NULL pointer if empty).
 * @param type TLV type.
 * @param length Length of string to add (not including %NULL).
 * @param value String to add.
 * @return The length of the TLV.
 */
faim_internal int aim_tlvlist_replace_raw(aim_tlvlist_t **list, const fu16_t type, const fu16_t length, const fu8_t *value)
{
	aim_tlvlist_t *cur;

	if (list == NULL)
		return 0;

	for (cur = *list; ((cur != NULL) && (cur->tlv->type != type)); cur = cur->next);
	if (cur == NULL)
		return aim_tlvlist_add_raw(list, type, length, value);

	free(cur->tlv->value);
	cur->tlv->length = length;
	if (cur->tlv->length > 0) {
		cur->tlv->value = (fu8_t *)malloc(cur->tlv->length);
		memcpy(cur->tlv->value, value, cur->tlv->length);
	} else
		cur->tlv->value = NULL;

	return cur->tlv->length;
}

/**
 * Substitute a TLV of a given type with a new TLV of the same type.  If 
 * you attempt to replace a TLV that does not exist, this function will 
 * just add a new TLV as if you called aim_tlvlist_add_raw().
 *
 * @param list Desination chain (%NULL pointer if empty).
 * @param type TLV type.
 * @return The length of the TLV.
 */
faim_internal int aim_tlvlist_replace_noval(aim_tlvlist_t **list, const fu16_t type)
{
	return aim_tlvlist_replace_raw(list, type, 0, NULL);
}

/**
 * Substitute a TLV of a given type with a new TLV of the same type.  If 
 * you attempt to replace a TLV that does not exist, this function will 
 * just add a new TLV as if you called aim_tlvlist_add_raw().
 *
 * @param list Desination chain (%NULL pointer if empty).
 * @param type TLV type.
 * @param value 8 bit value to add.
 * @return The length of the TLV.
 */
faim_internal int aim_tlvlist_replace_8(aim_tlvlist_t **list, const fu16_t type, const fu8_t value)
{
	fu8_t v8[1];

	aimutil_put8(v8, value);

	return aim_tlvlist_replace_raw(list, type, 1, v8);
}

/**
 * Substitute a TLV of a given type with a new TLV of the same type.  If 
 * you attempt to replace a TLV that does not exist, this function will 
 * just add a new TLV as if you called aim_tlvlist_add_raw().
 *
 * @param list Desination chain (%NULL pointer if empty).
 * @param type TLV type.
 * @param value 32 bit value to add.
 * @return The length of the TLV.
 */
faim_internal int aim_tlvlist_replace_32(aim_tlvlist_t **list, const fu16_t type, const fu32_t value)
{
	fu8_t v32[4];

	aimutil_put32(v32, value);

	return aim_tlvlist_replace_raw(list, type, 4, v32);
}

/**
 * Remove a TLV of a given type.  If you attempt to remove a TLV that 
 * does not exist, nothing happens.
 *
 * @param list Desination chain (%NULL pointer if empty).
 * @param type TLV type.
 */
faim_internal void aim_tlvlist_remove(aim_tlvlist_t **list, const fu16_t type)
{
	aim_tlvlist_t *del;

	if (!list || !(*list))
		return;

	/* Remove the item from the list */
	if ((*list)->tlv->type == type) {
		del = *list;
		*list = (*list)->next;
	} else {
		aim_tlvlist_t *cur;
		for (cur=*list; (cur->next && (cur->next->tlv->type!=type)); cur=cur->next);
		if (!cur->next)
			return;
		del = cur->next;
		cur->next = del->next;
	}

	/* Free the removed item */
	free(del->tlv->value);
	free(del->tlv);
	free(del);
}

/**
 * aim_tlvlist_write - Write a TLV chain into a data buffer.
 * @buf: Destination buffer
 * @buflen: Maximum number of bytes that will be written to buffer
 * @list: Source TLV chain
 *
 * Copies a TLV chain into a raw data buffer, writing only the number
 * of bytes specified. This operation does not free the chain; 
 * aim_tlvlist_free() must still be called to free up the memory used
 * by the chain structures.
 *
 * XXX clean this up, make better use of bstreams 
 */
faim_internal int aim_tlvlist_write(aim_bstream_t *bs, aim_tlvlist_t **list)
{
	int goodbuflen;
	aim_tlvlist_t *cur;

	/* do an initial run to test total length */
	goodbuflen = aim_tlvlist_size(list);

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
 * Grab the Nth TLV of type type in the TLV list list.
 *
 * Returns a pointer to an aim_tlv_t of the specified type; 
 * %NULL on error.  The @nth parameter is specified starting at %1.
 * In most cases, there will be no more than one TLV of any type
 * in a chain.
 *
 * @param list Source chain.
 * @param type Requested TLV type.
 * @param nth Index of TLV of type to get.
 * @return The TLV you were looking for, or NULL if one could not be found.
 */
faim_internal aim_tlv_t *aim_tlv_gettlv(aim_tlvlist_t *list, const fu16_t type, const int nth)
{
	aim_tlvlist_t *cur;
	int i;

	for (cur = list, i = 0; cur; cur = cur->next) {
		if (cur && cur->tlv) {
			if (cur->tlv->type == type)
				i++;
			if (i >= nth)
				return cur->tlv;
		}
	}

	return NULL;
}

/**
 * Retrieve the data from the nth TLV in the given TLV chain as a string.
 *
 * @param list Source TLV chain.
 * @param type TLV type to search for.
 * @param nth Index of TLV to return.
 * @return The value of the TLV you were looking for, or NULL if one could 
 *         not be found.  This is a dynamic buffer and must be freed by the 
 *         caller.
 */
faim_internal char *aim_tlv_getstr(aim_tlvlist_t *list, const fu16_t type, const int nth)
{
	aim_tlv_t *tlv;
	char *newstr;

	if (!(tlv = aim_tlv_gettlv(list, type, nth)))
		return NULL;

	newstr = (char *) malloc(tlv->length + 1);
	memcpy(newstr, tlv->value, tlv->length);
	newstr[tlv->length] = '\0';

	return newstr;
}

/**
 * Retrieve the data from the nth TLV in the given TLV chain as an 8bit 
 * integer.
 *
 * @param list Source TLV chain.
 * @param type TLV type to search for.
 * @param nth Index of TLV to return.
 * @return The value the TLV you were looking for, or 0 if one could 
 *         not be found.
 */
faim_internal fu8_t aim_tlv_get8(aim_tlvlist_t *list, const fu16_t type, const int nth)
{
	aim_tlv_t *tlv;

	if (!(tlv = aim_tlv_gettlv(list, type, nth)))
		return 0; /* erm */
	return aimutil_get8(tlv->value);
}

/**
 * Retrieve the data from the nth TLV in the given TLV chain as a 16bit 
 * integer.
 *
 * @param list Source TLV chain.
 * @param type TLV type to search for.
 * @param nth Index of TLV to return.
 * @return The value the TLV you were looking for, or 0 if one could 
 *         not be found.
 */
faim_internal fu16_t aim_tlv_get16(aim_tlvlist_t *list, const fu16_t type, const int nth)
{
	aim_tlv_t *tlv;

	if (!(tlv = aim_tlv_gettlv(list, type, nth)))
		return 0; /* erm */
	return aimutil_get16(tlv->value);
}

/**
 * Retrieve the data from the nth TLV in the given TLV chain as a 32bit 
 * integer.
 *
 * @param list Source TLV chain.
 * @param type TLV type to search for.
 * @param nth Index of TLV to return.
 * @return The value the TLV you were looking for, or 0 if one could 
 *         not be found.
 */
faim_internal fu32_t aim_tlv_get32(aim_tlvlist_t *list, const fu16_t type, const int nth)
{
	aim_tlv_t *tlv;

	if (!(tlv = aim_tlv_gettlv(list, type, nth)))
		return 0; /* erm */
	return aimutil_get32(tlv->value);
}
