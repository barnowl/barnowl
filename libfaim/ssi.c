/*
 * Family 0x0013 - Server-Side/Stored Information.
 *
 * Relatively new facility that allows certain types of information, such as 
 * a user's buddy list, permit/deny list, and permit/deny preferences, to be 
 * stored on the server, so that they can be accessed from any client.
 *
 * We keep 2 copies of SSI data:
 * 1) An exact copy of what is stored on the AIM servers.
 * 2) A local copy that we make changes to, and then send diffs 
 *    between this and the exact copy to keep them in sync.
 *
 * All the "aim_ssi_itemlist_bleh" functions near the top just modify the list 
 * that is given to them (i.e. they don't send SNACs).
 *
 * The SNAC sending and receiving functions are lower down in the file, and 
 * they're simpler.  They are in the order of the subtypes they deal with, 
 * starting with the request rights function (subtype 0x0002), then parse 
 * rights (subtype 0x0003), then--well, you get the idea.
 *
 * This is entirely too complicated.
 * You don't know the half of it.
 *
 */

#define FAIM_INTERNAL
#include <aim.h>

/**
 * Locally rebuild the 0x00c8 TLV in the additional data of the given group.
 *
 * @param list A pointer to a pointer to the current list of items.
 * @param name A null terminated string containing the group name, or NULL 
 *        if you want to modify the master group.
 * @return Return a pointer to the modified item.
 */
static struct aim_ssi_item *aim_ssi_itemlist_rebuildgroup(struct aim_ssi_item *list, const char *name)
{
	int newlen;
	struct aim_ssi_item *cur, *group;

	if (!list)
		return NULL;

	/* Find the group */
	if (!(group = aim_ssi_itemlist_finditem(list, name, NULL, AIM_SSI_TYPE_GROUP)))
		return NULL;

	/* Find the length for the new additional data */
	newlen = 0;
	if (group->gid == 0x0000) {
		for (cur=list; cur; cur=cur->next)
			if ((cur->type == AIM_SSI_TYPE_GROUP) && (cur->gid != 0x0000))
				newlen += 2;
	} else {
		for (cur=list; cur; cur=cur->next)
			if ((cur->gid == group->gid) && (cur->type == AIM_SSI_TYPE_BUDDY))
				newlen += 2;
	}

	/* Build the new TLV list */
	if (newlen > 0) {
		fu8_t *newdata;

		if (!(newdata = (fu8_t *)malloc((newlen)*sizeof(fu8_t))))
			return NULL;
		newlen = 0;
		if (group->gid == 0x0000) {
			for (cur=list; cur; cur=cur->next)
				if ((cur->type == AIM_SSI_TYPE_GROUP) && (cur->gid != 0x0000))
						newlen += aimutil_put16(newdata+newlen, cur->gid);
		} else {
			for (cur=list; cur; cur=cur->next)
				if ((cur->gid == group->gid) && (cur->type == AIM_SSI_TYPE_BUDDY))
						newlen += aimutil_put16(newdata+newlen, cur->bid);
		}
		aim_tlvlist_replace_raw(&group->data, 0x00c8, newlen, newdata);

		free(newdata);
	}

	return group;
}

/**
 * Locally add a new item to the given item list.
 *
 * @param list A pointer to a pointer to the current list of items.
 * @param name A null terminated string of the name of the new item, or NULL if the 
 *        item should have no name.
 * @param gid The group ID# you want the new item to have, or 0xFFFF if we should pick something.
 * @param bid The buddy ID# you want the new item to have, or 0xFFFF if we should pick something.
 * @param type The type of the item, 0x0000 for a contact, 0x0001 for a group, etc.
 * @param data The additional data for the new item.
 * @return A pointer to the newly created item.
 */
static struct aim_ssi_item *aim_ssi_itemlist_add(struct aim_ssi_item **list, const char *name, fu16_t gid, fu16_t bid, fu16_t type, aim_tlvlist_t *data)
{
	int i;
	struct aim_ssi_item *cur, *new;

	if (!list)
		return NULL;

	if (!(new = (struct aim_ssi_item *)malloc(sizeof(struct aim_ssi_item))))
		return NULL;

	/* Set the name */
	if (name) {
		new->name = (char *)malloc((strlen(name)+1)*sizeof(char));
		strcpy(new->name, name);
	} else
		new->name = NULL;

	/* Set the group ID# and buddy ID# */
	new->gid = gid;
	new->bid = bid;
	if (type == AIM_SSI_TYPE_GROUP) {
		if ((new->gid == 0xFFFF) && name) {
			do {
				new->gid += 0x0001;
				for (cur=*list, i=0; ((cur) && (!i)); cur=cur->next)
					if ((cur->type == AIM_SSI_TYPE_GROUP) && (cur->gid == new->gid))
						i=1;
			} while (i);
		}
	} else {
		if (new->bid == 0xFFFF) {
			do {
				new->bid += 0x0001;
				for (cur=*list, i=0; ((cur) && (!i)); cur=cur->next)
					if ((cur->bid == new->bid) && (cur->gid == new->gid))
						i=1;
			} while (i);
		}
	}

	/* Set the type */
	new->type = type;

	/* Set the TLV list */
	new->data = aim_tlvlist_copy(data);

	/* Add the item to the list in the correct numerical position.  Fancy, eh? */
	if (*list) {
		if ((new->gid < (*list)->gid) || ((new->gid == (*list)->gid) && (new->bid < (*list)->bid))) {
			new->next = *list;
			*list = new;
		} else {
			struct aim_ssi_item *prev;
			for ((prev=*list, cur=(*list)->next); (cur && ((new->gid > cur->gid) || ((new->gid == cur->gid) && (new->bid > cur->bid)))); prev=cur, cur=cur->next);
			new->next = prev->next;
			prev->next = new;
		}
	} else {
		new->next = *list;
		*list = new;
	}

	return new;
}

/**
 * Locally delete an item from the given item list.
 *
 * @param list A pointer to a pointer to the current list of items.
 * @param del A pointer to the item you want to remove from the list.
 * @return Return 0 if no errors, otherwise return the error number.
 */
static int aim_ssi_itemlist_del(struct aim_ssi_item **list, struct aim_ssi_item *del)
{
	if (!list || !(*list) || !del)
		return -EINVAL;

	/* Remove the item from the list */
	if (*list == del) {
		*list = (*list)->next;
	} else {
		struct aim_ssi_item *cur;
		for (cur=*list; (cur->next && (cur->next!=del)); cur=cur->next);
		if (cur->next)
			cur->next = del->next;
	}

	/* Free the removed item */
	free(del->name);
	aim_tlvlist_free(&del->data);
	free(del);

	return 0;
}

/**
 * Compare two items to see if they have the same data.
 *
 * @param cur1 A pointer to a pointer to the first item.
 * @param cur2 A pointer to a pointer to the second item.
 * @return Return 0 if no differences, or a number if there are differences.
 */
static int aim_ssi_itemlist_cmp(struct aim_ssi_item *cur1, struct aim_ssi_item *cur2)
{
	if (!cur1 || !cur2)
		return 1;

	if (cur1->data && !cur2->data)
		return 2;

	if (!cur1->data && cur2->data)
		return 3;

	if ((cur1->data && cur2->data) && (aim_tlvlist_cmp(cur1->data, cur2->data)))
			return 4;

	if (cur1->name && !cur2->name)
		return 5;

	if (!cur1->name && cur2->name)
		return 6;

	if (cur1->name && cur2->name && aim_sncmp(cur1->name, cur2->name))
		return 7;

	if (cur1->gid != cur2->gid)
		return 8;

	if (cur1->bid != cur2->bid)
		return 9;

	if (cur1->type != cur2->type)
		return 10;

	return 0;
}

faim_export int aim_ssi_itemlist_valid(struct aim_ssi_item *list, struct aim_ssi_item *item)
{
	struct aim_ssi_item *cur;
	for (cur=list; cur; cur=cur->next)
		if (cur == item)
			return 1;
	return 0;
}

/**
 * Locally find an item given a group ID# and a buddy ID#.
 *
 * @param list A pointer to the current list of items.
 * @param gid The group ID# of the desired item.
 * @param bid The buddy ID# of the desired item.
 * @return Return a pointer to the item if found, else return NULL;
 */
faim_export struct aim_ssi_item *aim_ssi_itemlist_find(struct aim_ssi_item *list, fu16_t gid, fu16_t bid)
{
	struct aim_ssi_item *cur;
	for (cur=list; cur; cur=cur->next)
		if ((cur->gid == gid) && (cur->bid == bid))
			return cur;
	return NULL;
}

/**
 * Locally find an item given a group name, screen name, and type.  If group name 
 * and screen name are null, then just return the first item of the given type.
 *
 * @param list A pointer to the current list of items.
 * @param gn The group name of the desired item.
 * @param bn The buddy name of the desired item.
 * @param type The type of the desired item.
 * @return Return a pointer to the item if found, else return NULL;
 */
faim_export struct aim_ssi_item *aim_ssi_itemlist_finditem(struct aim_ssi_item *list, const char *gn, const char *sn, fu16_t type)
{
	struct aim_ssi_item *cur;
	if (!list)
		return NULL;

	if (gn && sn) { /* For finding buddies in groups */
		for (cur=list; cur; cur=cur->next)
			if ((cur->type == type) && (cur->name) && !(aim_sncmp(cur->name, sn))) {
				struct aim_ssi_item *curg;
				for (curg=list; curg; curg=curg->next)
					if ((curg->type == AIM_SSI_TYPE_GROUP) && (curg->gid == cur->gid) && (curg->name) && !(aim_sncmp(curg->name, gn)))
						return cur;
			}

	} else if (gn) { /* For finding groups */
		for (cur=list; cur; cur=cur->next) {
			if ((cur->type == type) && (cur->bid == 0x0000) && (cur->name) && !(aim_sncmp(cur->name, gn))) {
				return cur;
			}
		}

	} else if (sn) { /* For finding permits, denies, and ignores */
		for (cur=list; cur; cur=cur->next) {
			if ((cur->type == type) && (cur->name) && !(aim_sncmp(cur->name, sn))) {
				return cur;
			}
		}

	/* For stuff without names--permit deny setting, visibility mask, etc. */
	} else for (cur=list; cur; cur=cur->next) {
		if ((cur->type == type) && (!cur->name))
			return cur;
	}

	return NULL;
}

/**
 * Check if the given buddy exists in any group in the buddy list.
 *
 * @param list A pointer to the current list of items.
 * @param sn The group name of the desired item.
 * @return Return a pointer to the name of the item if found, else return NULL;
 */
faim_export struct aim_ssi_item *aim_ssi_itemlist_exists(struct aim_ssi_item *list, const char *sn)
{
	struct aim_ssi_item *cur;
	if (!list || !sn)
		return NULL;
	for (cur=list; cur; cur=cur->next)
		if ((cur->type == AIM_SSI_TYPE_BUDDY) && (cur->name) && (!aim_sncmp(cur->name, sn)))
			return cur;
	return NULL;
}

/**
 * Locally find the parent item of the given buddy name.
 *
 * @param list A pointer to the current list of items.
 * @param bn The buddy name of the desired item.
 * @return Return a pointer to the name of the item if found, else return NULL;
 */
faim_export char *aim_ssi_itemlist_findparentname(struct aim_ssi_item *list, const char *sn)
{
	struct aim_ssi_item *cur, *curg;
	if (!list || !sn)
		return NULL;
	if (!(cur = aim_ssi_itemlist_exists(list, sn)))
		return NULL;
	if (!(curg = aim_ssi_itemlist_find(list, cur->gid, 0x0000)))
		return NULL;
	return curg->name;
}

/**
 * Locally find the permit/deny setting item, and return the setting.
 *
 * @param list A pointer to the current list of items.
 * @return Return the current SSI permit deny setting, or 0 if no setting was found.
 */
faim_export int aim_ssi_getpermdeny(struct aim_ssi_item *list)
{
	struct aim_ssi_item *cur = aim_ssi_itemlist_finditem(list, NULL, NULL, AIM_SSI_TYPE_PDINFO);
	if (cur) {
		aim_tlv_t *tlv = aim_tlv_gettlv(cur->data, 0x00ca, 1);
		if (tlv && tlv->value)
			return aimutil_get8(tlv->value);
	}
	return 0;
}

/**
 * Locally find the presence flag item, and return the setting.  The returned setting is a 
 * bitmask of the user flags that you are visible to.  See the AIM_FLAG_* #defines 
 * in aim.h
 *
 * @param list A pointer to the current list of items.
 * @return Return the current visibility mask.
 */
faim_export fu32_t aim_ssi_getpresence(struct aim_ssi_item *list)
{
	struct aim_ssi_item *cur = aim_ssi_itemlist_finditem(list, NULL, NULL, AIM_SSI_TYPE_PRESENCEPREFS);
	if (cur) {
		aim_tlv_t *tlv = aim_tlv_gettlv(cur->data, 0x00c9, 1);
		if (tlv && tlv->length)
			return aimutil_get32(tlv->value);
	}
	return 0xFFFFFFFF;
}

/**
 * Locally find the alias of the given buddy.
 *
 * @param list A pointer to the current list of items.
 * @param gn The group of the buddy.
 * @param sn The name of the buddy.
 * @return A pointer to a NULL terminated string that is the buddy's 
 *         alias, or NULL if the buddy has no alias.  You should free
 *         this returned value!
 */
faim_export char *aim_ssi_getalias(struct aim_ssi_item *list, const char *gn, const char *sn)
{
	struct aim_ssi_item *cur = aim_ssi_itemlist_finditem(list, gn, sn, AIM_SSI_TYPE_BUDDY);
	if (cur) {
		aim_tlv_t *tlv = aim_tlv_gettlv(cur->data, 0x0131, 1);
		if (tlv && tlv->length) {
			char *alias = (char *)malloc((tlv->length+1)*sizeof(char));
			strncpy(alias, tlv->value, tlv->length);
			alias[tlv->length] = 0;
			return alias;
		}
	}
	return NULL;
}

/**
 * Locally find the comment of the given buddy.
 *
 * @param list A pointer to the current list of items.
 * @param gn The group of the buddy.
 * @param sn The name of the buddy.
 * @return A pointer to a NULL terminated string that is the buddy's 
 *         comment, or NULL if the buddy has no comment.  You should free
 *         this returned value!
 */
faim_export char *aim_ssi_getcomment(struct aim_ssi_item *list, const char *gn, const char *sn)
{
	struct aim_ssi_item *cur = aim_ssi_itemlist_finditem(list, gn, sn, AIM_SSI_TYPE_BUDDY);
	if (cur) {
		aim_tlv_t *tlv = aim_tlv_gettlv(cur->data, 0x013c, 1);
		if (tlv && tlv->length) {
			char *alias = (char *)malloc((tlv->length+1)*sizeof(char));
			strncpy(alias, tlv->value, tlv->length);
			alias[tlv->length] = 0;
			return alias;
		}
	}
	return NULL;
}

/**
 * Locally find if you are waiting for authorization for a buddy.
 *
 * @param list A pointer to the current list of items.
 * @param gn The group of the buddy.
 * @param sn The name of the buddy.
 * @return A pointer to a NULL terminated string that is the buddies 
 *         alias, or NULL if the buddy has no alias.  You should free
 *         this returned value!
 */
faim_export int aim_ssi_waitingforauth(struct aim_ssi_item *list, const char *gn, const char *sn)
{
	struct aim_ssi_item *cur = aim_ssi_itemlist_finditem(list, gn, sn, AIM_SSI_TYPE_BUDDY);
	if (cur) {
		if (aim_tlv_gettlv(cur->data, 0x0066, 1))
			return 1;
	}
	return 0;
}

/**
 * If there are changes, then create temporary items and 
 * call addmoddel.
 *
 * @param sess The oscar session.
 * @return Return 0 if no errors, otherwise return the error number.
 */
static int aim_ssi_sync(aim_session_t *sess)
{
	struct aim_ssi_item *cur1, *cur2;
	struct aim_ssi_tmp *cur, *new;

	if (!sess)
		return -EINVAL;

	/* If we're waiting for an ack, we shouldn't do anything else */
	if (sess->ssi.waiting_for_ack)
		return 0;

	/*
	 * Compare the 2 lists and create an aim_ssi_tmp for each difference.  
	 * We should only send either additions, modifications, or deletions 
	 * before waiting for an acknowledgement.  So first do deletions, then 
	 * additions, then modifications.  Also, both the official and the local 
	 * list should be in ascending numerical order for the group ID#s and the 
	 * buddy ID#s, which makes things more efficient.  I think.
	 */

	/* Additions */
	if (!sess->ssi.pending) {
		for (cur1=sess->ssi.local; cur1; cur1=cur1->next) {
			if (!aim_ssi_itemlist_find(sess->ssi.official, cur1->gid, cur1->bid)) {
				new = (struct aim_ssi_tmp *)malloc(sizeof(struct aim_ssi_tmp));
				new->action = AIM_CB_SSI_ADD;
				new->ack = 0xffff;
				new->name = NULL;
				new->item = cur1;
				new->next = NULL;
				if (sess->ssi.pending) {
					for (cur=sess->ssi.pending; cur->next; cur=cur->next);
					cur->next = new;
				} else
					sess->ssi.pending = new;
			}
		}
	}

	/* Deletions */
	if (!sess->ssi.pending) {
		for (cur1=sess->ssi.official; cur1; cur1=cur1->next) {
			if (!aim_ssi_itemlist_find(sess->ssi.local, cur1->gid, cur1->bid)) {
				new = (struct aim_ssi_tmp *)malloc(sizeof(struct aim_ssi_tmp));
				new->action = AIM_CB_SSI_DEL;
				new->ack = 0xffff;
				new->name = NULL;
				new->item = cur1;
				new->next = NULL;
				if (sess->ssi.pending) {
					for (cur=sess->ssi.pending; cur->next; cur=cur->next);
					cur->next = new;
				} else
					sess->ssi.pending = new;
			}
		}
	}

	/* Modifications */
	if (!sess->ssi.pending) {
		for (cur1=sess->ssi.local; cur1; cur1=cur1->next) {
			cur2 = aim_ssi_itemlist_find(sess->ssi.official, cur1->gid, cur1->bid);
			if (cur2 && (aim_ssi_itemlist_cmp(cur1, cur2))) {
				new = (struct aim_ssi_tmp *)malloc(sizeof(struct aim_ssi_tmp));
				new->action = AIM_CB_SSI_MOD;
				new->ack = 0xffff;
				new->name = NULL;
				new->item = cur1;
				new->next = NULL;
				if (sess->ssi.pending) {
					for (cur=sess->ssi.pending; cur->next; cur=cur->next);
					cur->next = new;
				} else
					sess->ssi.pending = new;
			}
		}
	}

	/* We're out of stuff to do, so tell the AIM servers we're done and exit */
	if (!sess->ssi.pending) {
		aim_ssi_modend(sess);
		return 0;
	}

	/* Make sure we don't send anything else between now 
	 * and when we receive the ack for the following operation */
	sess->ssi.waiting_for_ack = 1;

	/* Now go mail off our data and wait 4 to 6 weeks */
	aim_ssi_addmoddel(sess);

	return 0;
}

/**
 * Free all SSI data.
 *
 * This doesn't remove it from the server, that's different.
 *
 * @param sess The oscar session.
 * @return Return 0 if no errors, otherwise return the error number.
 */
static int aim_ssi_freelist(aim_session_t *sess)
{
	struct aim_ssi_item *cur, *del;
	struct aim_ssi_tmp *curtmp, *deltmp;

	cur = sess->ssi.official;
	while (cur) {
		del = cur;
		cur = cur->next;
		free(del->name);
		aim_tlvlist_free(&del->data);
		free(del);
	}

	cur = sess->ssi.local;
	while (cur) {
		del = cur;
		cur = cur->next;
		free(del->name);
		aim_tlvlist_free(&del->data);
		free(del);
	}

	curtmp = sess->ssi.pending;
	while (curtmp) {
		deltmp = curtmp;
		curtmp = curtmp->next;
		free(deltmp);
	}

	sess->ssi.numitems = 0;
	sess->ssi.official = NULL;
	sess->ssi.local = NULL;
	sess->ssi.pending = NULL;
	sess->ssi.timestamp = (time_t)0;

	return 0;
}

/**
 * Delete all SSI data.
 *
 * @param sess The oscar session.
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_ssi_deletelist(aim_session_t *sess)
{
	struct aim_ssi_item *cur, *del;

	if (!sess)
		return -EINVAL;

	/* Free the local list */
	cur = sess->ssi.local;
	while (cur) {
		del = cur;
		cur = cur->next;
		free(del->name);
		aim_tlvlist_free(&del->data);
		free(del);
	}
	sess->ssi.local = NULL;

	/* Sync our local list with the server list */
	aim_ssi_sync(sess);

	return 0;
}

/**
 * This "cleans" the ssi list.  It does the following:
 * 1) Makes sure all buddies, permits, and denies have names.
 * 2) Makes sure that all buddies are in a group that exist.
 * 3) Deletes any empty groups
 *
 * @param sess The oscar session.
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_ssi_cleanlist(aim_session_t *sess)
{
	struct aim_ssi_item *cur, *next;

	if (!sess)
		return -EINVAL;

	/* Delete any buddies, permits, or denies with empty names. */
	/* If there are any buddies directly in the master group, add them to a real group. */
	/* DESTROY any buddies that are directly in the master group. */
	/* Do the same for buddies that are in a non-existant group. */
	/* This will kind of mess up if you hit the item limit, but this function isn't too critical */
	cur = sess->ssi.local;
	while (cur) {
		next = cur->next;
		if (!cur->name) {
			if (cur->type == AIM_SSI_TYPE_BUDDY)
				aim_ssi_delbuddy(sess, NULL, NULL);
			else if (cur->type == AIM_SSI_TYPE_PERMIT)
				aim_ssi_delpermit(sess, NULL);
			else if (cur->type == AIM_SSI_TYPE_DENY)
				aim_ssi_deldeny(sess, NULL);
		} else if ((cur->type == AIM_SSI_TYPE_BUDDY) && ((cur->gid == 0x0000) || (!aim_ssi_itemlist_find(sess->ssi.local, cur->gid, 0x0000)))) {
			aim_ssi_addbuddy(sess, cur->name, "orphans", NULL, NULL, NULL, 0);
			aim_ssi_delbuddy(sess, cur->name, NULL);
		}
		cur = next;
	}

	/* Check if there are empty groups and delete them */
	cur = sess->ssi.local;
	while (cur) {
		next = cur->next;
		if (cur->type == AIM_SSI_TYPE_GROUP) {
			aim_tlv_t *tlv = aim_tlv_gettlv(cur->data, 0x00c8, 1);
			if (!tlv || !tlv->length)
				aim_ssi_itemlist_del(&sess->ssi.local, cur);
		}
		cur = next;
	}

	/* Check if the master group is empty */
	if ((cur = aim_ssi_itemlist_find(sess->ssi.local, 0x0000, 0x0000)) && (!cur->data))
		aim_ssi_itemlist_del(&sess->ssi.local, cur);

	return 0;
}

/**
 * Add a buddy to the list.
 *
 * @param sess The oscar session.
 * @param name The name of the item.
 * @param group The group of the item.
 * @param alias The alias/nickname of the item, or NULL.
 * @param comment The buddy comment for the item, or NULL.
 * @param smsnum The locally assigned SMS number, or NULL.
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_ssi_addbuddy(aim_session_t *sess, const char *name, const char *group, const char *alias, const char *comment, const char *smsnum, int needauth)
{
	struct aim_ssi_item *parent;
	aim_tlvlist_t *data = NULL;

	if (!sess || !name || !group)
		return -EINVAL;

	/* Find the parent */
	if (!(parent = aim_ssi_itemlist_finditem(sess->ssi.local, group, NULL, AIM_SSI_TYPE_GROUP))) {
		/* Find the parent's parent (the master group) */
		if (!(parent = aim_ssi_itemlist_find(sess->ssi.local, 0x0000, 0x0000)))
			if (!(parent = aim_ssi_itemlist_add(&sess->ssi.local, NULL, 0x0000, 0x0000, AIM_SSI_TYPE_GROUP, NULL)))
				return -ENOMEM;
		/* Add the parent */
		if (!(parent = aim_ssi_itemlist_add(&sess->ssi.local, group, 0xFFFF, 0x0000, AIM_SSI_TYPE_GROUP, NULL)))
			return -ENOMEM;

		/* Modify the parent's parent (the master group) */
		aim_ssi_itemlist_rebuildgroup(sess->ssi.local, NULL);
	}

	/* Create a TLV list for the new buddy */
	if (needauth)
		aim_tlvlist_add_noval(&data, 0x0066);
	if (alias)
		aim_tlvlist_add_raw(&data, 0x0131, strlen(alias), alias);
	if (smsnum)
		aim_tlvlist_add_raw(&data, 0x013a, strlen(smsnum), smsnum);
	if (comment)
		aim_tlvlist_add_raw(&data, 0x013c, strlen(comment), comment);

	/* Add that bad boy */
	aim_ssi_itemlist_add(&sess->ssi.local, name, parent->gid, 0xFFFF, AIM_SSI_TYPE_BUDDY, data);
	aim_tlvlist_free(&data);

	/* Modify the parent group */
	aim_ssi_itemlist_rebuildgroup(sess->ssi.local, group);

	/* Sync our local list with the server list */
	aim_ssi_sync(sess);

	return 0;
}

/**
 * Add a permit buddy to the list.
 *
 * @param sess The oscar session.
 * @param name The name of the item..
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_ssi_addpermit(aim_session_t *sess, const char *name)
{

	if (!sess || !name)
		return -EINVAL;

	/* Add that bad boy */
	aim_ssi_itemlist_add(&sess->ssi.local, name, 0x0000, 0xFFFF, AIM_SSI_TYPE_PERMIT, NULL);

	/* Sync our local list with the server list */
	aim_ssi_sync(sess);

	return 0;
}

/**
 * Add a deny buddy to the list.
 *
 * @param sess The oscar session.
 * @param name The name of the item..
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_ssi_adddeny(aim_session_t *sess, const char *name)
{

	if (!sess || !name)
		return -EINVAL;

	/* Add that bad boy */
	aim_ssi_itemlist_add(&sess->ssi.local, name, 0x0000, 0xFFFF, AIM_SSI_TYPE_DENY, NULL);

	/* Sync our local list with the server list */
	aim_ssi_sync(sess);

	return 0;
}

/**
 * Deletes a buddy from the list.
 *
 * @param sess The oscar session.
 * @param name The name of the item, or NULL.
 * @param group The group of the item, or NULL.
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_ssi_delbuddy(aim_session_t *sess, const char *name, const char *group)
{
	struct aim_ssi_item *del;

	if (!sess)
		return -EINVAL;

	/* Find the buddy */
	if (!(del = aim_ssi_itemlist_finditem(sess->ssi.local, group, name, AIM_SSI_TYPE_BUDDY)))
		return -EINVAL;

	/* Remove the item from the list */
	aim_ssi_itemlist_del(&sess->ssi.local, del);

	/* Modify the parent group */
	aim_ssi_itemlist_rebuildgroup(sess->ssi.local, group);

	/* Check if we should delete the parent group */
	if ((del = aim_ssi_itemlist_finditem(sess->ssi.local, group, NULL, AIM_SSI_TYPE_GROUP)) && (!del->data)) {
		aim_ssi_itemlist_del(&sess->ssi.local, del);

		/* Modify the parent group */
		aim_ssi_itemlist_rebuildgroup(sess->ssi.local, NULL);

		/* Check if we should delete the parent's parent (the master group) */
		if ((del = aim_ssi_itemlist_find(sess->ssi.local, 0x0000, 0x0000)) && (!del->data)) {
			aim_ssi_itemlist_del(&sess->ssi.local, del);
		}
	}

	/* Sync our local list with the server list */
	aim_ssi_sync(sess);

	return 0;
}

/**
 * Deletes a permit buddy from the list.
 *
 * @param sess The oscar session.
 * @param name The name of the item, or NULL.
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_ssi_delpermit(aim_session_t *sess, const char *name)
{
	struct aim_ssi_item *del;

	if (!sess)
		return -EINVAL;

	/* Find the item */
	if (!(del = aim_ssi_itemlist_finditem(sess->ssi.local, NULL, name, AIM_SSI_TYPE_PERMIT)))
		return -EINVAL;

	/* Remove the item from the list */
	aim_ssi_itemlist_del(&sess->ssi.local, del);

	/* Sync our local list with the server list */
	aim_ssi_sync(sess);

	return 0;
}

/**
 * Deletes a deny buddy from the list.
 *
 * @param sess The oscar session.
 * @param name The name of the item, or NULL.
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_ssi_deldeny(aim_session_t *sess, const char *name)
{
	struct aim_ssi_item *del;

	if (!sess)
		return -EINVAL;

	/* Find the item */
	if (!(del = aim_ssi_itemlist_finditem(sess->ssi.local, NULL, name, AIM_SSI_TYPE_DENY)))
		return -EINVAL;

	/* Remove the item from the list */
	aim_ssi_itemlist_del(&sess->ssi.local, del);

	/* Sync our local list with the server list */
	aim_ssi_sync(sess);

	return 0;
}

/**
 * Move a buddy from one group to another group.  This basically just deletes the 
 * buddy and re-adds it.
 *
 * @param sess The oscar session.
 * @param oldgn The group that the buddy is currently in.
 * @param newgn The group that the buddy should be moved in to.
 * @param sn The name of the buddy to be moved.
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_ssi_movebuddy(aim_session_t *sess, const char *oldgn, const char *newgn, const char *sn)
{
	aim_ssi_addbuddy(sess, sn, newgn, aim_ssi_getalias(sess->ssi.local, oldgn, sn), NULL, NULL, aim_ssi_waitingforauth(sess->ssi.local, oldgn, sn));
	aim_ssi_delbuddy(sess, sn, oldgn);
	return 0;
}

/**
 * Change the alias stored on the server for a given buddy.
 *
 * @param sess The oscar session.
 * @param gn The group that the buddy is currently in.
 * @param sn The screen name of the buddy.
 * @param alias The new alias for the buddy, or NULL if you want to remove 
 *        a buddy's comment.
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_ssi_aliasbuddy(aim_session_t *sess, const char *gn, const char *sn, const char *alias)
{
	struct aim_ssi_item *tmp;

	if (!sess || !gn || !sn)
		return -EINVAL;

	if (!(tmp = aim_ssi_itemlist_finditem(sess->ssi.local, gn, sn, AIM_SSI_TYPE_BUDDY)))
		return -EINVAL;

	/* Either add or remove the 0x0131 TLV from the TLV chain */
	if ((alias != NULL) && (strlen(alias) > 0))
		aim_tlvlist_replace_raw(&tmp->data, 0x0131, strlen(alias), alias);
	else
		aim_tlvlist_remove(&tmp->data, 0x0131);

	/* Sync our local list with the server list */
	aim_ssi_sync(sess);

	return 0;
}

/**
 * Change the comment stored on the server for a given buddy.
 *
 * @param sess The oscar session.
 * @param gn The group that the buddy is currently in.
 * @param sn The screen name of the buddy.
 * @param alias The new comment for the buddy, or NULL if you want to remove 
 *        a buddy's comment.
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_ssi_editcomment(aim_session_t *sess, const char *gn, const char *sn, const char *comment)
{
	struct aim_ssi_item *tmp;

	if (!sess || !gn || !sn)
		return -EINVAL;

	if (!(tmp = aim_ssi_itemlist_finditem(sess->ssi.local, gn, sn, AIM_SSI_TYPE_BUDDY)))
		return -EINVAL;

	/* Either add or remove the 0x0131 TLV from the TLV chain */
	if ((comment != NULL) && (strlen(comment) > 0))
		aim_tlvlist_replace_raw(&tmp->data, 0x013c, strlen(comment), comment);
	else
		aim_tlvlist_remove(&tmp->data, 0x013c);

	/* Sync our local list with the server list */
	aim_ssi_sync(sess);

	return 0;
}

/**
 * Rename a group.
 *
 * @param sess The oscar session.
 * @param oldgn The old group name.
 * @param newgn The new group name.
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_ssi_rename_group(aim_session_t *sess, const char *oldgn, const char *newgn)
{
	struct aim_ssi_item *group;

	if (!sess || !oldgn || !newgn)
		return -EINVAL;

	if (!(group = aim_ssi_itemlist_finditem(sess->ssi.local, oldgn, NULL, AIM_SSI_TYPE_GROUP)))
		return -EINVAL;

	free(group->name);
	group->name = (char *)malloc((strlen(newgn)+1)*sizeof(char));
	strcpy(group->name, newgn);

	/* Sync our local list with the server list */
	aim_ssi_sync(sess);

	return 0;
}

/**
 * Stores your permit/deny setting on the server, and starts using it.
 *
 * @param sess The oscar session.
 * @param permdeny Your permit/deny setting.  Can be one of the following:
 *        1 - Allow all users
 *        2 - Block all users
 *        3 - Allow only the users below
 *        4 - Block only the users below
 *        5 - Allow only users on my buddy list
 * @param vismask A bitmask of the class of users to whom you want to be 
 *        visible.  See the AIM_FLAG_BLEH #defines in aim.h
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_ssi_setpermdeny(aim_session_t *sess, fu8_t permdeny, fu32_t vismask)
{
	struct aim_ssi_item *tmp;

	if (!sess)
		return -EINVAL;

	/* Find the PDINFO item, or add it if it does not exist */
	if (!(tmp = aim_ssi_itemlist_finditem(sess->ssi.local, NULL, NULL, AIM_SSI_TYPE_PDINFO)))
		tmp = aim_ssi_itemlist_add(&sess->ssi.local, NULL, 0x0000, 0xFFFF, AIM_SSI_TYPE_PDINFO, NULL);

	/* Need to add the 0x00ca TLV to the TLV chain */
	aim_tlvlist_replace_8(&tmp->data, 0x00ca, permdeny);

	/* Need to add the 0x00cb TLV to the TLV chain */
	aim_tlvlist_replace_32(&tmp->data, 0x00cb, vismask);

	/* Sync our local list with the server list */
	aim_ssi_sync(sess);

	return 0;
}

/**
 * Set buddy icon information
 *
 * @param sess The oscar session.
 * @param iconcsum The MD5 checksum of the icon you are using.
 * @param iconcsumlen Length of the MD5 checksum given above.  Should be 0x10 bytes.
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_ssi_seticon(aim_session_t *sess, fu8_t *iconsum, fu16_t iconsumlen)
{
	struct aim_ssi_item *tmp;
	fu8_t *csumdata;

	if (!sess || !iconsum || !iconsumlen)
		return -EINVAL;

	/* Find the ICONINFO item, or add it if it does not exist */
	if (!(tmp = aim_ssi_itemlist_finditem(sess->ssi.local, NULL, "1", AIM_SSI_TYPE_ICONINFO))) {
		tmp = aim_ssi_itemlist_add(&sess->ssi.local, "1", 0x0000, 0x51F4, AIM_SSI_TYPE_ICONINFO, NULL);
	}

	/* Need to add the 0x00d5 TLV to the TLV chain */
	if (!(csumdata = (fu8_t *)malloc((iconsumlen+2)*sizeof(fu8_t))))
		return -ENOMEM;
	csumdata[0] = 0x00;
	csumdata[1] = 0x10;
	memcpy(&csumdata[2], iconsum, iconsumlen);
	aim_tlvlist_replace_raw(&tmp->data, 0x00d5, (iconsumlen+2) * sizeof(fu8_t), csumdata);
	free(csumdata);

	/* Need to add the 0x0131 TLV to the TLV chain, used to cache the icon */
	aim_tlvlist_replace_noval(&tmp->data, 0x0131);

	/* Sync our local list with the server list */
	aim_ssi_sync(sess);
	return 0;
}

/**
 * Remove a reference to a server stored buddy icon.  This will make your 
 * icon stop showing up to other people.
 *
 * @param sess The oscar session.
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_ssi_delicon(aim_session_t *sess)
{
	struct aim_ssi_item *tmp;

	if (!sess)
		return -EINVAL;

	/* Find the ICONINFO item and delete it if it exists*/
	if ((tmp = aim_ssi_itemlist_finditem(sess->ssi.local, NULL, "1", AIM_SSI_TYPE_ICONINFO)))
		aim_ssi_itemlist_del(&sess->ssi.local, tmp);

	/* Sync our local list with the server list */
	aim_ssi_sync(sess);
	return 0;
}

/**
 * Stores your setting for whether you should show up as idle or not.
 *
 * @param sess The oscar session.
 * @param presence I think it's a bitmask, but I only know what one of the bits is:
 *        0x00000002 - Hide wireless?
 *        0x00000400 - Allow others to see your idle time
 * @return Return 0 if no errors, otherwise return the error number.
 */
faim_export int aim_ssi_setpresence(aim_session_t *sess, fu32_t presence) {
	struct aim_ssi_item *tmp;

	if (!sess)
		return -EINVAL;

	/* Find the PRESENCEPREFS item, or add it if it does not exist */
	if (!(tmp = aim_ssi_itemlist_finditem(sess->ssi.local, NULL, NULL, AIM_SSI_TYPE_PRESENCEPREFS)))
		tmp = aim_ssi_itemlist_add(&sess->ssi.local, NULL, 0x0000, 0xFFFF, AIM_SSI_TYPE_PRESENCEPREFS, NULL);

	/* Need to add the x00c9 TLV to the TLV chain */
	aim_tlvlist_replace_32(&tmp->data, 0x00c9, presence);

	/* Sync our local list with the server list */
	aim_ssi_sync(sess);

	return 0;
}

/*
 * Subtype 0x0002 - Request SSI Rights.
 */
faim_export int aim_ssi_reqrights(aim_session_t *sess)
{
	aim_conn_t *conn;

	if (!sess || !(conn = aim_conn_findbygroup(sess, AIM_CB_FAM_SSI)))
		return -EINVAL;

	return aim_genericreq_n_snacid(sess, conn, AIM_CB_FAM_SSI, AIM_CB_SSI_REQRIGHTS);
}

/*
 * Subtype 0x0003 - SSI Rights Information.
 */
static int parserights(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0, i;
	aim_rxcallback_t userfunc;
	aim_tlvlist_t *tlvlist;
	aim_tlv_t *tlv;
	aim_bstream_t bstream;
	fu16_t *maxitems;

	/* This SNAC is made up of a bunch of TLVs */
	tlvlist = aim_tlvlist_read(bs);

	/* TLV 0x0004 contains the maximum number of each item */
	if (!(tlv = aim_tlv_gettlv(tlvlist, 0x0004, 1))) {
		aim_tlvlist_free(&tlvlist);
		return 0;
	}

	aim_bstream_init(&bstream, tlv->value, tlv->length);

	if (!(maxitems = (fu16_t *)malloc((tlv->length/2)*sizeof(fu16_t)))) {
		aim_tlvlist_free(&tlvlist);
		return 0;
	}

	for (i=0; i<(tlv->length/2); i++)
		maxitems[i] = aimbs_get16(&bstream);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, tlv->length/2, maxitems);

	aim_tlvlist_free(&tlvlist);
	free(maxitems);

	return ret;
}

/*
 * Subtype 0x0004 - Request SSI Data when you don't have a timestamp and 
 * revision number.
 * 
 */
faim_export int aim_ssi_reqdata(aim_session_t *sess)
{
	aim_conn_t *conn;

	if (!sess || !(conn = aim_conn_findbygroup(sess, AIM_CB_FAM_SSI)))
		return -EINVAL;

	/* Free any current data, just in case */
	aim_ssi_freelist(sess);

	return aim_genericreq_n_snacid(sess, conn, AIM_CB_FAM_SSI, AIM_CB_SSI_REQDATA);
}

/*
 * Subtype 0x0005 - Request SSI Data when you have a timestamp and revision 
 * number.
 *
 * The data will only be sent if it is newer than the posted local
 * timestamp and revision.
 * 
 * Note that the client should never increment the revision, only the server.
 * 
 */
faim_export int aim_ssi_reqifchanged(aim_session_t *sess, time_t timestamp, fu16_t numitems)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sess || !(conn = aim_conn_findbygroup(sess, AIM_CB_FAM_SSI)))
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+4+2)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, AIM_CB_FAM_SSI, AIM_CB_SSI_REQIFCHANGED, 0x0000, NULL, 0);

	aim_putsnac(&fr->data, AIM_CB_FAM_SSI, AIM_CB_SSI_REQIFCHANGED, 0x0000, snacid);
	aimbs_put32(&fr->data, timestamp);
	aimbs_put16(&fr->data, numitems);

	aim_tx_enqueue(sess, fr);

	/* Free any current data, just in case */
	aim_ssi_freelist(sess);

	return 0;
}

/*
 * Subtype 0x0006 - SSI Data.
 */
static int parsedata(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	fu8_t fmtver; /* guess */
	fu16_t namelen, gid, bid, type;
	char *name;
	aim_tlvlist_t *data;

	fmtver = aimbs_get8(bs); /* Version of ssi data.  Should be 0x00 */
	sess->ssi.numitems += aimbs_get16(bs); /* # of items in this SSI SNAC */

	/* Read in the list */
	while (aim_bstream_empty(bs) > 4) { /* last four bytes are timestamp */
		if ((namelen = aimbs_get16(bs)))
			name = aimbs_getstr(bs, namelen);
		else
			name = NULL;
		gid = aimbs_get16(bs);
		bid = aimbs_get16(bs);
		type = aimbs_get16(bs);
		data = aim_tlvlist_readlen(bs, aimbs_get16(bs));
		aim_ssi_itemlist_add(&sess->ssi.official, name, gid, bid, type, data);
		free(name);
		aim_tlvlist_free(&data);
	}

	/* Read in the timestamp */
	sess->ssi.timestamp = aimbs_get32(bs);

	if (!(snac->flags & 0x0001)) {
		/* Make a copy of the list */
		struct aim_ssi_item *cur;
		for (cur=sess->ssi.official; cur; cur=cur->next)
			aim_ssi_itemlist_add(&sess->ssi.local, cur->name, cur->gid, cur->bid, cur->type, cur->data);

		sess->ssi.received_data = 1;

		if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
			ret = userfunc(sess, rx, fmtver, sess->ssi.numitems, sess->ssi.official, sess->ssi.timestamp);
	}

	return ret;
}

/*
 * Subtype 0x0007 - SSI Activate Data.
 *
 * Should be sent after receiving 13/6 or 13/f to tell the server you
 * are ready to begin using the list.  It will promptly give you the
 * presence information for everyone in your list and put your permit/deny
 * settings into effect.
 * 
 */
faim_export int aim_ssi_enable(aim_session_t *sess)
{
	aim_conn_t *conn;

	if (!sess || !(conn = aim_conn_findbygroup(sess, AIM_CB_FAM_SSI)))
		return -EINVAL;

	return aim_genericreq_n(sess, conn, AIM_CB_FAM_SSI, 0x0007);
}

/*
 * Subtype 0x0008/0x0009/0x000a - SSI Add/Mod/Del Item(s).
 *
 * Sends the SNAC to add, modify, or delete an item from the server-stored
 * information.  These 3 SNACs all have an identical structure.  The only
 * difference is the subtype that is set for the SNAC.
 * 
 */
faim_export int aim_ssi_addmoddel(aim_session_t *sess)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;
	int snaclen;
	struct aim_ssi_tmp *cur;

	if (!sess || !(conn = aim_conn_findbygroup(sess, AIM_CB_FAM_SSI)) || !sess->ssi.pending || !sess->ssi.pending->item)
		return -EINVAL;

	/* Calculate total SNAC size */
	snaclen = 10; /* For family, subtype, flags, and SNAC ID */
	for (cur=sess->ssi.pending; cur; cur=cur->next) {
		snaclen += 10; /* For length, GID, BID, type, and length */
		if (cur->item->name)
			snaclen += strlen(cur->item->name);
		if (cur->item->data)
			snaclen += aim_tlvlist_size(&cur->item->data);
	}

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, snaclen)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, AIM_CB_FAM_SSI, sess->ssi.pending->action, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, AIM_CB_FAM_SSI, sess->ssi.pending->action, 0x0000, snacid);

	for (cur=sess->ssi.pending; cur; cur=cur->next) {
		aimbs_put16(&fr->data, cur->item->name ? strlen(cur->item->name) : 0);
		if (cur->item->name)
			aimbs_putraw(&fr->data, cur->item->name, strlen(cur->item->name));
		aimbs_put16(&fr->data, cur->item->gid);
		aimbs_put16(&fr->data, cur->item->bid);
		aimbs_put16(&fr->data, cur->item->type);
		aimbs_put16(&fr->data, cur->item->data ? aim_tlvlist_size(&cur->item->data) : 0);
		if (cur->item->data)
			aim_tlvlist_write(&fr->data, &cur->item->data);
	}

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x0008 - Incoming SSI add.
 *
 * XXX - It would probably be good for the client to actually do something when it gets this.
 */
static int parseadd(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	char *name;
	fu16_t len, gid, bid, type;
	aim_tlvlist_t *data;

	while (aim_bstream_empty(bs)) {
		if ((len = aimbs_get16(bs)))
			name = aimbs_getstr(bs, len);
		else
			name = NULL;
		gid = aimbs_get16(bs);
		bid = aimbs_get16(bs);
		type = aimbs_get16(bs);
		if ((len = aimbs_get16(bs)))
			data = aim_tlvlist_readlen(bs, len);
		else
			data = NULL;

		aim_ssi_itemlist_add(&sess->ssi.local, name, gid, bid, type, data);
		aim_ssi_itemlist_add(&sess->ssi.official, name, gid, bid, type, data);
		free(name);
		aim_tlvlist_free(&data);

		if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
			ret = userfunc(sess, rx);

		free(name);
	}

	return ret;
}

/*
 * Subtype 0x0009 - Incoming SSI mod.
 *
 * XXX - It would probably be good for the client to actually do something when it gets this.
 */
static int parsemod(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	char *name;
	fu16_t len, gid, bid, type;
	aim_tlvlist_t *data;
	struct aim_ssi_item *item;

	while (aim_bstream_empty(bs)) {
		if ((len = aimbs_get16(bs)))
			name = aimbs_getstr(bs, len);
		else
			name = NULL;
		gid = aimbs_get16(bs);
		bid = aimbs_get16(bs);
		type = aimbs_get16(bs);
		if ((len = aimbs_get16(bs)))
			data = aim_tlvlist_readlen(bs, len);
		else
			data = NULL;

		/* Replace the 2 local items with the given one */
		if ((item = aim_ssi_itemlist_find(sess->ssi.local, gid, bid))) {
			item->type = type;
			free(item->name);
			if (name) {
				item->name = (char *)malloc((strlen(name)+1)*sizeof(char));
				strcpy(item->name, name);
			} else
				item->name = NULL;
			aim_tlvlist_free(&item->data);
			item->data = aim_tlvlist_copy(data);
		}

		if ((item = aim_ssi_itemlist_find(sess->ssi.official, gid, bid))) {
			item->type = type;
			free(item->name);
			if (name) {
				item->name = (char *)malloc((strlen(name)+1)*sizeof(char));
				strcpy(item->name, name);
			} else
				item->name = NULL;
			aim_tlvlist_free(&item->data);
			item->data = aim_tlvlist_copy(data);
		}

		if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
			ret = userfunc(sess, rx);

		free(name);
		aim_tlvlist_free(&data);
	}

	return ret;
}

/*
 * Subtype 0x000a - Incoming SSI del.
 *
 * XXX - It would probably be good for the client to actually do something when it gets this.
 */
static int parsedel(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	fu16_t gid, bid;
	struct aim_ssi_item *del;

	while (aim_bstream_empty(bs)) {
		aim_bstream_advance(bs, aimbs_get16(bs));
		gid = aimbs_get16(bs);
		bid = aimbs_get16(bs);
		aimbs_get16(bs);
		aim_bstream_advance(bs, aimbs_get16(bs));

		if ((del = aim_ssi_itemlist_find(sess->ssi.local, gid, bid)))
			aim_ssi_itemlist_del(&sess->ssi.local, del);
		if ((del = aim_ssi_itemlist_find(sess->ssi.official, gid, bid)))
			aim_ssi_itemlist_del(&sess->ssi.official, del);

		if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
			ret = userfunc(sess, rx);
	}

	return ret;
}

/*
 * Subtype 0x000e - SSI Add/Mod/Del Ack.
 *
 * Response to add, modify, or delete SNAC (sent with aim_ssi_addmoddel).
 *
 */
static int parseack(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	struct aim_ssi_tmp *cur, *del;

	/* Read in the success/failure flags from the ack SNAC */
	cur = sess->ssi.pending;
	while (cur && (aim_bstream_empty(bs)>0)) {
		cur->ack = aimbs_get16(bs);
		cur = cur->next;
	}

	/*
	 * If outcome is 0, then add the item to the item list, or replace the other item, 
	 * or remove the old item.  If outcome is non-zero, then remove the item from the 
	 * local list, or unmodify it, or add it.
	 */
	for (cur=sess->ssi.pending; (cur && (cur->ack != 0xffff)); cur=cur->next) {
	if (cur->item) {
		if (cur->ack) {
			/* Our action was unsuccessful, so change the local list back to how it was */
			if (cur->action == AIM_CB_SSI_ADD) {
				/* Remove the item from the local list */
				/* Make sure cur->item is still valid memory */
				if (aim_ssi_itemlist_valid(sess->ssi.local, cur->item)) {
					if (cur->item->name) {
						cur->name = (char *)malloc((strlen(cur->item->name)+1)*sizeof(char));
						strcpy(cur->name, cur->item->name);
					}
					aim_ssi_itemlist_del(&sess->ssi.local, cur->item);
				}
				cur->item = NULL;

			} else if (cur->action == AIM_CB_SSI_MOD) {
				/* Replace the local item with the item from the official list */
				if (aim_ssi_itemlist_valid(sess->ssi.local, cur->item)) {
					struct aim_ssi_item *cur1;
					if ((cur1 = aim_ssi_itemlist_find(sess->ssi.official, cur->item->gid, cur->item->bid))) {
						free(cur->item->name);
						if (cur1->name) {
							cur->item->name = (char *)malloc((strlen(cur1->name)+1)*sizeof(char));
							strcpy(cur->item->name, cur1->name);
						} else
							cur->item->name = NULL;
						aim_tlvlist_free(&cur->item->data);
						cur->item->data = aim_tlvlist_copy(cur1->data);
					}
				} else
					cur->item = NULL;

			} else if (cur->action == AIM_CB_SSI_DEL) {
				/* Add the item back into the local list */
				if (aim_ssi_itemlist_valid(sess->ssi.official, cur->item)) {
					aim_ssi_itemlist_add(&sess->ssi.local, cur->item->name, cur->item->gid, cur->item->bid, cur->item->type, cur->item->data);
				} else
					cur->item = NULL;
			}

		} else {
			/* Do the exact opposite */
			if (cur->action == AIM_CB_SSI_ADD) {
			/* Add the local item to the official list */
				if (aim_ssi_itemlist_valid(sess->ssi.local, cur->item)) {
					aim_ssi_itemlist_add(&sess->ssi.official, cur->item->name, cur->item->gid, cur->item->bid, cur->item->type, cur->item->data);
				} else
					cur->item = NULL;

			} else if (cur->action == AIM_CB_SSI_MOD) {
				/* Replace the official item with the item from the local list */
				if (aim_ssi_itemlist_valid(sess->ssi.local, cur->item)) {
					struct aim_ssi_item *cur1;
					if ((cur1 = aim_ssi_itemlist_find(sess->ssi.official, cur->item->gid, cur->item->bid))) {
						free(cur1->name);
						if (cur->item->name) {
							cur1->name = (char *)malloc((strlen(cur->item->name)+1)*sizeof(char));
							strcpy(cur1->name, cur->item->name);
						} else
							cur1->name = NULL;
						aim_tlvlist_free(&cur1->data);
						cur1->data = aim_tlvlist_copy(cur->item->data);
					}
				} else
					cur->item = NULL;

			} else if (cur->action == AIM_CB_SSI_DEL) {
				/* Remove the item from the official list */
				if (aim_ssi_itemlist_valid(sess->ssi.official, cur->item))
					aim_ssi_itemlist_del(&sess->ssi.official, cur->item);
				cur->item = NULL;
			}

		}
	} /* End if (cur->item) */
	} /* End for loop */

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, sess->ssi.pending);

	/* Free all aim_ssi_tmp's with an outcome */
	cur = sess->ssi.pending;
	while (cur && (cur->ack != 0xffff)) {
		del = cur;
		cur = cur->next;
		free(del->name);
		free(del);
	}
	sess->ssi.pending = cur;

	/* If we're not waiting for any more acks, then send more SNACs */
	if (!sess->ssi.pending) {
		sess->ssi.pending = NULL;
		sess->ssi.waiting_for_ack = 0;
		aim_ssi_sync(sess);
	}

	return ret;
}

/*
 * Subtype 0x000f - SSI Data Unchanged.
 *
 * Response to aim_ssi_reqifchanged() if the server-side data is not newer than
 * posted local stamp/revision.
 *
 */
static int parsedataunchanged(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;

	sess->ssi.received_data = 1;

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx);

	return ret;
}

/*
 * Subtype 0x0011 - SSI Begin Data Modification.
 *
 * Tells the server you're going to start modifying data.
 * 
 */
faim_export int aim_ssi_modbegin(aim_session_t *sess)
{
	aim_conn_t *conn;

	if (!sess || !(conn = aim_conn_findbygroup(sess, AIM_CB_FAM_SSI)))
		return -EINVAL;

	return aim_genericreq_n(sess, conn, AIM_CB_FAM_SSI, AIM_CB_SSI_EDITSTART);
}

/*
 * Subtype 0x0012 - SSI End Data Modification.
 *
 * Tells the server you're finished modifying data.
 *
 */
faim_export int aim_ssi_modend(aim_session_t *sess)
{
	aim_conn_t *conn;

	if (!sess || !(conn = aim_conn_findbygroup(sess, AIM_CB_FAM_SSI)))
		return -EINVAL;

	return aim_genericreq_n(sess, conn, AIM_CB_FAM_SSI, AIM_CB_SSI_EDITSTOP);
}

/*
 * Subtype 0x0014 - Grant authorization
 *
 * Authorizes a contact so they can add you to their contact list.
 *
 */
faim_export int aim_ssi_sendauth(aim_session_t *sess, char *sn, char *msg)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sess || !(conn = aim_conn_findbygroup(sess, AIM_CB_FAM_SSI)) || !sn)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+1+strlen(sn)+2+(msg ? strlen(msg)+1 : 0)+2)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, AIM_CB_FAM_SSI, AIM_CB_SSI_SENDAUTH, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, AIM_CB_FAM_SSI, AIM_CB_SSI_SENDAUTH, 0x0000, snacid);

	/* Screen name */
	aimbs_put8(&fr->data, strlen(sn));
	aimbs_putraw(&fr->data, sn, strlen(sn));

	/* Message (null terminated) */
	aimbs_put16(&fr->data, msg ? strlen(msg) : 0);
	if (msg) {
		aimbs_putraw(&fr->data, msg, strlen(msg));
		aimbs_put8(&fr->data, 0x00);
	}

	/* Unknown */
	aimbs_put16(&fr->data, 0x0000);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x0015 - Receive an authorization grant
 */
static int receiveauthgrant(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	fu16_t tmp;
	char *sn, *msg;

	/* Read screen name */
	if ((tmp = aimbs_get8(bs)))
		sn = aimbs_getstr(bs, tmp);
	else
		sn = NULL;

	/* Read message (null terminated) */
	if ((tmp = aimbs_get16(bs)))
		msg = aimbs_getstr(bs, tmp);
	else
		msg = NULL;

	/* Unknown */
	tmp = aimbs_get16(bs);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, sn, msg);

	free(sn);
	free(msg);

	return ret;
}

/*
 * Subtype 0x0018 - Send authorization request
 *
 * Sends a request for authorization to the given contact.  The request will either be
 * granted, denied, or dropped.
 *
 */
faim_export int aim_ssi_sendauthrequest(aim_session_t *sess, char *sn, char *msg)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sess || !(conn = aim_conn_findbygroup(sess, AIM_CB_FAM_SSI)) || !sn)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+1+strlen(sn)+2+(msg ? strlen(msg)+1 : 0)+2)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, AIM_CB_FAM_SSI, AIM_CB_SSI_SENDAUTHREQ, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, AIM_CB_FAM_SSI, AIM_CB_SSI_SENDAUTHREQ, 0x0000, snacid);

	/* Screen name */
	aimbs_put8(&fr->data, strlen(sn));
	aimbs_putraw(&fr->data, sn, strlen(sn));

	/* Message (null terminated) */
	aimbs_put16(&fr->data, msg ? strlen(msg) : 0);
	if (msg) {
		aimbs_putraw(&fr->data, msg, strlen(msg));
		aimbs_put8(&fr->data, 0x00);
	}

	/* Unknown */
	aimbs_put16(&fr->data, 0x0000);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x0019 - Receive an authorization request
 */
static int receiveauthrequest(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	fu16_t tmp;
	char *sn, *msg;

	/* Read screen name */
	if ((tmp = aimbs_get8(bs)))
		sn = aimbs_getstr(bs, tmp);
	else
		sn = NULL;

	/* Read message (null terminated) */
	if ((tmp = aimbs_get16(bs)))
		msg = aimbs_getstr(bs, tmp);
	else
		msg = NULL;

	/* Unknown */
	tmp = aimbs_get16(bs);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, sn, msg);

	free(sn);
	free(msg);

	return ret;
}

/*
 * Subtype 0x001a - Send authorization reply
 *
 * Sends a reply to a request for authorization.  The reply can either 
 * grant authorization or deny authorization.
 *
 * if reply=0x00 then deny
 * if reply=0x01 then grant
 *
 */
faim_export int aim_ssi_sendauthreply(aim_session_t *sess, char *sn, fu8_t reply, char *msg)
{
	aim_conn_t *conn;
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sess || !(conn = aim_conn_findbygroup(sess, AIM_CB_FAM_SSI)) || !sn)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10 + 1+strlen(sn) + 1 + 2+(msg ? strlen(msg)+1 : 0) + 2)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, AIM_CB_FAM_SSI, AIM_CB_SSI_SENDAUTHREP, 0x0000, NULL, 0);
	aim_putsnac(&fr->data, AIM_CB_FAM_SSI, AIM_CB_SSI_SENDAUTHREP, 0x0000, snacid);

	/* Screen name */
	aimbs_put8(&fr->data, strlen(sn));
	aimbs_putraw(&fr->data, sn, strlen(sn));

	/* Grant or deny */
	aimbs_put8(&fr->data, reply);

	/* Message (null terminated) */
	aimbs_put16(&fr->data, msg ? (strlen(msg)+1) : 0);
	if (msg) {
		aimbs_putraw(&fr->data, msg, strlen(msg));
		aimbs_put8(&fr->data, 0x00);
	}

	/* Unknown */
	aimbs_put16(&fr->data, 0x0000);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * Subtype 0x001b - Receive an authorization reply
 * You get this bad boy when other people respond to the authorization 
 * request that you have previously sent them.
 */
static int receiveauthreply(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	fu16_t tmp;
	fu8_t reply;
	char *sn, *msg;

	/* Read screen name */
	if ((tmp = aimbs_get8(bs)))
		sn = aimbs_getstr(bs, tmp);
	else
		sn = NULL;

	/* Read reply */
	reply = aimbs_get8(bs);

	/* Read message (null terminated) */
	if ((tmp = aimbs_get16(bs)))
		msg = aimbs_getstr(bs, tmp);
	else
		msg = NULL;

	/* Unknown */
	tmp = aimbs_get16(bs);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, sn, reply, msg);

	free(sn);
	free(msg);

	return ret;
}

/*
 * Subtype 0x001c - Receive a message telling you someone added you to their list.
 */
static int receiveadded(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	fu16_t tmp;
	char *sn;

	/* Read screen name */
	if ((tmp = aimbs_get8(bs)))
		sn = aimbs_getstr(bs, tmp);
	else
		sn = NULL;

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, sn);

	free(sn);

	return ret;
}

static int snachandler(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{

	if (snac->subtype == AIM_CB_SSI_RIGHTSINFO)
		return parserights(sess, mod, rx, snac, bs);
	else if (snac->subtype == AIM_CB_SSI_LIST)
		return parsedata(sess, mod, rx, snac, bs);
	else if (snac->subtype == AIM_CB_SSI_ADD)
		return parseadd(sess, mod, rx, snac, bs);
	else if (snac->subtype == AIM_CB_SSI_MOD)
		return parsemod(sess, mod, rx, snac, bs);
	else if (snac->subtype == AIM_CB_SSI_DEL)
		return parsedel(sess, mod, rx, snac, bs);
	else if (snac->subtype == AIM_CB_SSI_SRVACK)
		return parseack(sess, mod, rx, snac, bs);
	else if (snac->subtype == AIM_CB_SSI_NOLIST)
		return parsedataunchanged(sess, mod, rx, snac, bs);
	else if (snac->subtype == AIM_CB_SSI_RECVAUTH)
		return receiveauthgrant(sess, mod, rx, snac, bs);
	else if (snac->subtype == AIM_CB_SSI_RECVAUTHREQ)
		return receiveauthrequest(sess, mod, rx, snac, bs);
	else if (snac->subtype == AIM_CB_SSI_RECVAUTHREP)
		return receiveauthreply(sess, mod, rx, snac, bs);
	else if (snac->subtype == AIM_CB_SSI_ADDED)
		return receiveadded(sess, mod, rx, snac, bs);

	return 0;
}

static void ssi_shutdown(aim_session_t *sess, aim_module_t *mod)
{
	aim_ssi_freelist(sess);
}

faim_internal int ssi_modfirst(aim_session_t *sess, aim_module_t *mod)
{

	mod->family = AIM_CB_FAM_SSI;
	mod->version = 0x0004;
	mod->toolid = 0x0110;
	mod->toolversion = 0x0629;
	mod->flags = 0;
	strncpy(mod->name, "ssi", sizeof(mod->name));
	mod->snachandler = snachandler;
	mod->shutdown = ssi_shutdown;

	return 0;
}
