/*
 * rxhandlers.c
 *
 * This file contains most all of the incoming packet handlers, along
 * with aim_rxdispatch(), the Rx dispatcher.  Queue/list management is
 * actually done in aim_rxqueue.c.
 *
 */

#define FAIM_INTERNAL
#include <aim.h>

struct aim_rxcblist_s {
	fu16_t family;
	fu16_t type;
	aim_rxcallback_t handler;
	u_short flags;
	struct aim_rxcblist_s *next;
};

faim_internal aim_module_t *aim__findmodulebygroup(aim_session_t *sess, fu16_t group)
{
	aim_module_t *cur;

	for (cur = (aim_module_t *)sess->modlistv; cur; cur = cur->next) {
		if (cur->family == group)
			return cur;
	}

	return NULL;
}

faim_internal aim_module_t *aim__findmodule(aim_session_t *sess, const char *name)
{
	aim_module_t *cur;

	for (cur = (aim_module_t *)sess->modlistv; cur; cur = cur->next) {
		if (strcmp(name, cur->name) == 0)
			return cur;
	}

	return NULL;
}

faim_internal int aim__registermodule(aim_session_t *sess, int (*modfirst)(aim_session_t *, aim_module_t *))
{
	aim_module_t *mod;

	if (!sess || !modfirst)
		return -1;

	if (!(mod = malloc(sizeof(aim_module_t))))
		return -1;
	memset(mod, 0, sizeof(aim_module_t));

	if (modfirst(sess, mod) == -1) {
		free(mod);
		return -1;
	}

	if (aim__findmodule(sess, mod->name)) {
		if (mod->shutdown)
			mod->shutdown(sess, mod);
		free(mod);
		return -1;
	}

	mod->next = (aim_module_t *)sess->modlistv;
	(aim_module_t *)sess->modlistv = mod;

	faimdprintf(sess, 1, "registered module %s (family 0x%04x, version = 0x%04x, tool 0x%04x, tool version 0x%04x)\n", mod->name, mod->family, mod->version, mod->toolid, mod->toolversion);

	return 0;
}

faim_internal void aim__shutdownmodules(aim_session_t *sess)
{
	aim_module_t *cur;

	for (cur = (aim_module_t *)sess->modlistv; cur; ) {
		aim_module_t *tmp;

		tmp = cur->next;

		if (cur->shutdown)
			cur->shutdown(sess, cur);

		free(cur);

		cur = tmp;
	}

	sess->modlistv = NULL;

	return;
}

static int consumesnac(aim_session_t *sess, aim_frame_t *rx)
{
	aim_module_t *cur;
	aim_modsnac_t snac;

	if (aim_bstream_empty(&rx->data) < 10)
		return 0;

	snac.family = aimbs_get16(&rx->data);
	snac.subtype = aimbs_get16(&rx->data);
	snac.flags = aimbs_get16(&rx->data);
	snac.id = aimbs_get32(&rx->data);

	for (cur = (aim_module_t *)sess->modlistv; cur; cur = cur->next) {

		if (!(cur->flags & AIM_MODFLAG_MULTIFAMILY) && 
				(cur->family != snac.family))
			continue;

		if (cur->snachandler(sess, cur, rx, &snac, &rx->data))
			return 1;

	}

	return 0;
}

static int consumenonsnac(aim_session_t *sess, aim_frame_t *rx, fu16_t family, fu16_t subtype)
{
	aim_module_t *cur;
	aim_modsnac_t snac;

	snac.family = family;
	snac.subtype = subtype;
	snac.flags = snac.id = 0;

	for (cur = (aim_module_t *)sess->modlistv; cur; cur = cur->next) {

		if (!(cur->flags & AIM_MODFLAG_MULTIFAMILY) && 
				(cur->family != snac.family))
			continue;

		if (cur->snachandler(sess, cur, rx, &snac, &rx->data))
			return 1;

	}

	return 0;
}

static int negchan_middle(aim_session_t *sess, aim_frame_t *fr)
{
	aim_tlvlist_t *tlvlist;
	char *msg = NULL;
	fu16_t code = 0;
	aim_rxcallback_t userfunc;
	int ret = 1;

	if (aim_bstream_empty(&fr->data) == 0) {
		/* XXX should do something with this */
		return 1;
	}

	/* Used only by the older login protocol */
	/* XXX remove this special case? */
	if (fr->conn->type == AIM_CONN_TYPE_AUTH)
		return consumenonsnac(sess, fr, 0x0017, 0x0003);

	tlvlist = aim_readtlvchain(&fr->data);

	if (aim_gettlv(tlvlist, 0x0009, 1))
		code = aim_gettlv16(tlvlist, 0x0009, 1);

	if (aim_gettlv(tlvlist, 0x000b, 1))
		msg = aim_gettlv_str(tlvlist, 0x000b, 1);

	if ((userfunc = aim_callhandler(sess, fr->conn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNERR))) 
		ret = userfunc(sess, fr, code, msg);

	aim_freetlvchain(&tlvlist);

	free(msg);

	return ret;
}

/*
 * Bleck functions get called when there's no non-bleck functions
 * around to cleanup the mess...
 */
faim_internal int bleck(aim_session_t *sess, aim_frame_t *frame, ...)
{
	fu16_t family, subtype;
	fu16_t maxf, maxs;

	static const char *channels[6] = {
		"Invalid (0)",
		"FLAP Version",
		"SNAC",
		"Invalid (3)",
		"Negotiation",
		"FLAP NOP"
	};
	static const int maxchannels = 5;
	
	/* XXX: this is ugly. and big just for debugging. */
	static const char *literals[14][25] = {
		{"Invalid", 
		 NULL
		},
		{"General", 
		 "Invalid",
		 "Error",
		 "Client Ready",
		 "Server Ready",
		 "Service Request",
		 "Redirect",
		 "Rate Information Request",
		 "Rate Information",
		 "Rate Information Ack",
		 NULL,
		 "Rate Information Change",
		 "Server Pause",
		 NULL,
		 "Server Resume",
		 "Request Personal User Information",
		 "Personal User Information",
		 "Evil Notification",
		 NULL,
		 "Migration notice",
		 "Message of the Day",
		 "Set Privacy Flags",
		 "Well Known URL",
		 "NOP"
		},
		{"Location", 
		 "Invalid",
		 "Error",
		 "Request Rights",
		 "Rights Information", 
		 "Set user information", 
		 "Request User Information", 
		 "User Information", 
		 "Watcher Sub Request",
		 "Watcher Notification"
		},
		{"Buddy List Management", 
		 "Invalid", 
		 "Error", 
		 "Request Rights",
		 "Rights Information",
		 "Add Buddy", 
		 "Remove Buddy", 
		 "Watcher List Query", 
		 "Watcher List Response", 
		 "Watcher SubRequest", 
		 "Watcher Notification", 
		 "Reject Notification", 
		 "Oncoming Buddy", 
		 "Offgoing Buddy"
		},
		{"Messeging", 
		 "Invalid",
		 "Error", 
		 "Add ICBM Parameter",
		 "Remove ICBM Parameter", 
		 "Request Parameter Information",
		 "Parameter Information",
		 "Outgoing Message", 
		 "Incoming Message",
		 "Evil Request",
		 "Evil Reply", 
		 "Missed Calls",
		 "Message Error", 
		 "Host Ack"
		},
		{"Advertisements", 
		 "Invalid", 
		 "Error", 
		 "Request Ad",
		 "Ad Data (GIFs)"
		},
		{"Invitation / Client-to-Client", 
		 "Invalid",
		 "Error",
		 "Invite a Friend",
		 "Invitation Ack"
		},
		{"Administrative", 
		 "Invalid",
		 "Error",
		 "Information Request",
		 "Information Reply",
		 "Information Change Request",
		 "Information Chat Reply",
		 "Account Confirm Request",
		 "Account Confirm Reply",
		 "Account Delete Request",
		 "Account Delete Reply"
		},
		{"Popups", 
		 "Invalid",
		 "Error",
		 "Display Popup"
		},
		{"BOS", 
		 "Invalid",
		 "Error",
		 "Request Rights",
		 "Rights Response",
		 "Set group permission mask",
		 "Add permission list entries",
		 "Delete permission list entries",
		 "Add deny list entries",
		 "Delete deny list entries",
		 "Server Error"
		},
		{"User Lookup", 
		 "Invalid",
		 "Error",
		 "Search Request",
		 "Search Response"
		},
		{"Stats", 
		 "Invalid",
		 "Error",
		 "Set minimum report interval",
		 "Report Events"
		},
		{"Translate", 
		 "Invalid",
		 "Error",
		 "Translate Request",
		 "Translate Reply",
		},
		{"Chat Navigation", 
		 "Invalid",
		 "Error",
		 "Request rights",
		 "Request Exchange Information",
		 "Request Room Information",
		 "Request Occupant List",
		 "Search for Room",
		 "Outgoing Message", 
		 "Incoming Message",
		 "Evil Request", 
		 "Evil Reply", 
		 "Chat Error",
		}
	};

	maxf = sizeof(literals) / sizeof(literals[0]);
	maxs = sizeof(literals[0]) / sizeof(literals[0][0]);

	if (frame->hdr.flap.type == 0x02) {

		family = aimbs_get16(&frame->data);
		subtype = aimbs_get16(&frame->data);
		
		if ((family < maxf) && (subtype+1 < maxs) && (literals[family][subtype] != NULL))
			faimdprintf(sess, 0, "bleck: channel %s: null handler for %04x/%04x (%s)\n", channels[frame->hdr.flap.type], family, subtype, literals[family][subtype+1]);
		else
			faimdprintf(sess, 0, "bleck: channel %s: null handler for %04x/%04x (no literal)\n", channels[frame->hdr.flap.type], family, subtype);
	} else {

		if (frame->hdr.flap.type <= maxchannels)
			faimdprintf(sess, 0, "bleck: channel %s (0x%02x)\n", channels[frame->hdr.flap.type], frame->hdr.flap.type);
		else
			faimdprintf(sess, 0, "bleck: unknown channel 0x%02x\n", frame->hdr.flap.type);

	}
		
	return 1;
}

/*
 * Some SNACs we do not allow to be hooked, for good reason.
 */
static int checkdisallowed(fu16_t group, fu16_t type)
{
	static const struct {
		fu16_t group;
		fu16_t type;
	} dontuse[] = {
		{0x0001, 0x0002},
		{0x0001, 0x0003},
		{0x0001, 0x0006},
		{0x0001, 0x0007},
		{0x0001, 0x0008},
		{0x0001, 0x0017},
		{0x0001, 0x0018},
		{0x0000, 0x0000}
	};
	int i;

	for (i = 0; dontuse[i].group != 0x0000; i++) {
		if ((dontuse[i].group == group) && (dontuse[i].type == type))
			return 1;
	}

	return 0;
}

faim_export int aim_conn_addhandler(aim_session_t *sess, aim_conn_t *conn, fu16_t family, fu16_t type, aim_rxcallback_t newhandler, fu16_t flags)
{
	struct aim_rxcblist_s *newcb;

	if (!conn)
		return -1;

	faimdprintf(sess, 1, "aim_conn_addhandler: adding for %04x/%04x\n", family, type);

	if (checkdisallowed(family, type)) {
		faimdprintf(sess, 0, "aim_conn_addhandler: client tried to hook %x/%x -- BUG!!!\n", family, type);
		return -1;
	}

	if (!(newcb = (struct aim_rxcblist_s *)calloc(1, sizeof(struct aim_rxcblist_s))))
		return -1;

	newcb->family = family;
	newcb->type = type;
	newcb->flags = flags;
	newcb->handler = newhandler ? newhandler : bleck;
	newcb->next = NULL;

	if (!conn->handlerlist)
		conn->handlerlist = (void *)newcb;
	else {
		struct aim_rxcblist_s *cur;

		for (cur = (struct aim_rxcblist_s *)conn->handlerlist; cur->next; cur = cur->next)
			;
		cur->next = newcb;
	}

	return 0;
}

faim_export int aim_clearhandlers(aim_conn_t *conn)
{
	struct aim_rxcblist_s *cur;

	if (!conn)
		return -1;

	for (cur = (struct aim_rxcblist_s *)conn->handlerlist; cur; ) {
		struct aim_rxcblist_s *tmp;

		tmp = cur->next;
		free(cur);
		cur = tmp;
	}
	conn->handlerlist = NULL;

	return 0;
}

faim_internal aim_rxcallback_t aim_callhandler(aim_session_t *sess, aim_conn_t *conn, fu16_t family, fu16_t type)
{
	struct aim_rxcblist_s *cur;

	if (!conn)
		return NULL;

	faimdprintf(sess, 1, "aim_callhandler: calling for %04x/%04x\n", family, type);

	for (cur = (struct aim_rxcblist_s *)conn->handlerlist; cur; cur = cur->next) {
		if ((cur->family == family) && (cur->type == type))
			return cur->handler;
	}

	if (type == AIM_CB_SPECIAL_DEFAULT) {
		faimdprintf(sess, 1, "aim_callhandler: no default handler for family 0x%04x\n", family);
		return NULL; /* prevent infinite recursion */
	}

	faimdprintf(sess, 1, "aim_callhandler: no handler for  0x%04x/0x%04x\n", family, type);

	return aim_callhandler(sess, conn, family, AIM_CB_SPECIAL_DEFAULT);
}

faim_internal void aim_clonehandlers(aim_session_t *sess, aim_conn_t *dest, aim_conn_t *src)
{
	struct aim_rxcblist_s *cur;

	for (cur = (struct aim_rxcblist_s *)src->handlerlist; cur; cur = cur->next) {
		aim_conn_addhandler(sess, dest, cur->family, cur->type, 
						cur->handler, cur->flags);
	}

	return;
}

faim_internal int aim_callhandler_noparam(aim_session_t *sess, aim_conn_t *conn,fu16_t family, fu16_t type, aim_frame_t *ptr)
{
	aim_rxcallback_t userfunc;

	if ((userfunc = aim_callhandler(sess, conn, family, type)))
		return userfunc(sess, ptr);

	return 1; /* XXX */
}

/*
 * aim_rxdispatch()
 *
 * Basically, heres what this should do:
 *   1) Determine correct packet handler for this packet
 *   2) Mark the packet handled (so it can be dequeued in purge_queue())
 *   3) Send the packet to the packet handler
 *   4) Go to next packet in the queue and start over
 *   5) When done, run purge_queue() to purge handled commands
 *
 * TODO: Clean up.
 * TODO: More support for mid-level handlers.
 * TODO: Allow for NULL handlers.
 *
 */
faim_export void aim_rxdispatch(aim_session_t *sess)
{
	int i;
	aim_frame_t *cur;

	for (cur = sess->queue_incoming, i = 0; cur; cur = cur->next, i++) {

		/*
		 * XXX: This is still fairly ugly.
		 */

		if (cur->handled)
			continue;

		/*
		 * This is a debugging/sanity check only and probably 
		 * could/should be removed for stable code.
		 */
		if (((cur->hdrtype == AIM_FRAMETYPE_OFT) && 
		   (cur->conn->type != AIM_CONN_TYPE_RENDEZVOUS)) || 
		  ((cur->hdrtype == AIM_FRAMETYPE_FLAP) && 
		   (cur->conn->type == AIM_CONN_TYPE_RENDEZVOUS))) {
			faimdprintf(sess, 0, "rxhandlers: incompatible frame type %d on connection type 0x%04x\n", cur->hdrtype, cur->conn->type);
			cur->handled = 1;
			continue;
		}

		if (cur->conn->type == AIM_CONN_TYPE_RENDEZVOUS) {
			if (cur->hdrtype != AIM_FRAMETYPE_OFT) {
				faimdprintf(sess, 0, "internal error: non-OFT frames on OFT connection\n");
				cur->handled = 1; /* get rid of it */
			} else {
				aim_rxdispatch_rendezvous(sess, cur);
				cur->handled = 1;
			}
			continue;
		}

		if (cur->conn->type == AIM_CONN_TYPE_RENDEZVOUS_OUT) {
			/* not possible */
			faimdprintf(sess, 0, "rxdispatch called on RENDEZVOUS_OUT connection!\n");
			cur->handled = 1;
			continue;
		}

		if (cur->hdr.flap.type == 0x01) {
			
			cur->handled = aim_callhandler_noparam(sess, cur->conn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_FLAPVER, cur); /* XXX use consumenonsnac */
			
			continue;
			
		} else if (cur->hdr.flap.type == 0x02) {

			if ((cur->handled = consumesnac(sess, cur)))
				continue;

		} else if (cur->hdr.flap.type == 0x04) {

			cur->handled = negchan_middle(sess, cur);
			continue;

		} else if (cur->hdr.flap.type == 0x05)
			;
		
		if (!cur->handled) {
			consumenonsnac(sess, cur, 0xffff, 0xffff); /* last chance! */
			cur->handled = 1;
		}
	}

	/* 
	 * This doesn't have to be called here.  It could easily be done
	 * by a seperate thread or something. It's an administrative operation,
	 * and can take a while. Though the less you call it the less memory
	 * you'll have :)
	 */
	aim_purge_rxqueue(sess);

	return;
}

faim_internal int aim_parse_unknown(aim_session_t *sess, aim_frame_t *frame, ...)
{
	int i;

	faimdprintf(sess, 1, "\nRecieved unknown packet:");

	for (i = 0; aim_bstream_empty(&frame->data); i++) {
		if ((i % 8) == 0)
			faimdprintf(sess, 1, "\n\t");

		faimdprintf(sess, 1, "0x%2x ", aimbs_get8(&frame->data));
	}

	faimdprintf(sess, 1, "\n\n");

	return 1;
}
