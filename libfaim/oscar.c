/*
 * gaim
 *
 * Some code copyright (C) 1998-1999, Mark Spencer <markster@marko.net>
 * Some code copyright (C) 1999-2001, Eric Warmenhoven
 * Some code copyright (C) 2001-2003, Sean Egan
 * Some code copyright (C) 2001-2003, Mark Doliner <thekingant@users.sourceforge.net>
 *
 * Most libfaim code copyright (C) 1998-2001 Adam Fritzler <afritz@auk.cx>
 * Some libfaim code copyright (C) 2001-2003 Mark Doliner <thekingant@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include "internal.h"

#include "account.h"
#include "accountopt.h"
#include "buddyicon.h"
#include "conversation.h"
#include "debug.h"
#include "ft.h"
#include "imgstore.h"
#include "multi.h"
#include "notify.h"
#include "privacy.h"
#include "prpl.h"
#include "proxy.h"
#include "request.h"
#include "util.h"
#include "html.h"

#include "aim.h"
#include "md5.h"

/* XXX CORE/UI */
#include "gtkinternal.h"
#include "gaim.h"


#define UC_AOL		0x02
#define UC_ADMIN	0x04
#define UC_UNCONFIRMED	0x08
#define UC_NORMAL	0x10
#define UC_AB		0x20
#define UC_WIRELESS	0x40
#define UC_HIPTOP	0x80

#define AIMHASHDATA "http://gaim.sourceforge.net/aim_data.php3"

static GaimPlugin *my_protocol = NULL;

static int caps_aim = AIM_CAPS_CHAT | AIM_CAPS_BUDDYICON | AIM_CAPS_DIRECTIM | AIM_CAPS_SENDFILE | AIM_CAPS_INTEROPERATE;
static int caps_icq = AIM_CAPS_BUDDYICON | AIM_CAPS_DIRECTIM | AIM_CAPS_SENDFILE | AIM_CAPS_ICQUTF8 | AIM_CAPS_INTEROPERATE;

static fu8_t features_aim[] = {0x01, 0x01, 0x01, 0x02};
static fu8_t features_icq[] = {0x01, 0x06};

struct oscar_data {
	aim_session_t *sess;
	aim_conn_t *conn;

	guint cnpa;
	guint paspa;
	guint emlpa;
	guint icopa;

	gboolean iconconnecting;
	gboolean set_icon;

	GSList *create_rooms;

	gboolean conf;
	gboolean reqemail;
	gboolean setemail;
	char *email;
	gboolean setnick;
	char *newsn;
	gboolean chpass;
	char *oldp;
	char *newp;
	
	GSList *oscar_chats;
	GSList *direct_ims;
	GSList *file_transfers;
	GHashTable *buddyinfo;
	GSList *requesticon;

	gboolean killme;
	gboolean icq;
	GSList *evilhack;
	guint icontimer;
	guint getblisttimer;

	struct {
		guint maxwatchers; /* max users who can watch you */
		guint maxbuddies; /* max users you can watch */
		guint maxgroups; /* max groups in server list */
		guint maxpermits; /* max users on permit list */
		guint maxdenies; /* max users on deny list */
		guint maxsiglen; /* max size (bytes) of profile */
		guint maxawaymsglen; /* max size (bytes) of posted away message */
	} rights;
};

struct create_room {
	char *name;
	int exchange;
};

struct chat_connection {
	char *name;
	char *show; /* AOL did something funny to us */
	fu16_t exchange;
	fu16_t instance;
	int fd; /* this is redundant since we have the conn below */
	aim_conn_t *conn;
	int inpa;
	int id;
	GaimConnection *gc; /* i hate this. */
	GaimConversation *cnv; /* bah. */
	int maxlen;
	int maxvis;
};

struct direct_im {
	GaimConnection *gc;
	char name[80];
	int watcher;
	aim_conn_t *conn;
	gboolean connected;
};

struct ask_direct {
	GaimConnection *gc;
	char *sn;
	char ip[64];
	fu8_t cookie[8];
};

/* Various PRPL-specific buddy info that we want to keep track of */
struct buddyinfo {
	time_t signon;
	int caps;
	gboolean typingnot;
	gchar *availmsg;
	fu32_t ipaddr;

	unsigned long ico_me_len;
	unsigned long ico_me_csum;
	time_t ico_me_time;
	gboolean ico_informed;

	unsigned long ico_len;
	unsigned long ico_csum;
	time_t ico_time;
	gboolean ico_need;

	fu16_t iconcsumlen;
	fu8_t *iconcsum;
};

struct name_data {
	GaimConnection *gc;
	gchar *name;
	gchar *nick;
};

static char *msgerrreason[] = {
	N_("Invalid error"),
	N_("Invalid SNAC"),
	N_("Rate to host"),
	N_("Rate to client"),
	N_("Not logged in"),
	N_("Service unavailable"),
	N_("Service not defined"),
	N_("Obsolete SNAC"),
	N_("Not supported by host"),
	N_("Not supported by client"),
	N_("Refused by client"),
	N_("Reply too big"),
	N_("Responses lost"),
	N_("Request denied"),
	N_("Busted SNAC payload"),
	N_("Insufficient rights"),
	N_("In local permit/deny"),
	N_("Too evil (sender)"),
	N_("Too evil (receiver)"),
	N_("User temporarily unavailable"),
	N_("No match"),
	N_("List overflow"),
	N_("Request ambiguous"),
	N_("Queue full"),
	N_("Not while on AOL")
};
static int msgerrreasonlen = 25;

/* All the libfaim->gaim callback functions */
static int gaim_parse_auth_resp  (aim_session_t *, aim_frame_t *, ...);
static int gaim_parse_login      (aim_session_t *, aim_frame_t *, ...);
static int gaim_handle_redirect  (aim_session_t *, aim_frame_t *, ...);
static int gaim_info_change      (aim_session_t *, aim_frame_t *, ...);
static int gaim_account_confirm  (aim_session_t *, aim_frame_t *, ...);
static int gaim_parse_oncoming   (aim_session_t *, aim_frame_t *, ...);
static int gaim_parse_offgoing   (aim_session_t *, aim_frame_t *, ...);
static int gaim_parse_incoming_im(aim_session_t *, aim_frame_t *, ...);
static int gaim_parse_misses     (aim_session_t *, aim_frame_t *, ...);
static int gaim_parse_clientauto (aim_session_t *, aim_frame_t *, ...);
static int gaim_parse_user_info  (aim_session_t *, aim_frame_t *, ...);
static int gaim_parse_motd       (aim_session_t *, aim_frame_t *, ...);
static int gaim_chatnav_info     (aim_session_t *, aim_frame_t *, ...);
static int gaim_chat_join        (aim_session_t *, aim_frame_t *, ...);
static int gaim_chat_leave       (aim_session_t *, aim_frame_t *, ...);
static int gaim_chat_info_update (aim_session_t *, aim_frame_t *, ...);
static int gaim_chat_incoming_msg(aim_session_t *, aim_frame_t *, ...);
static int gaim_email_parseupdate(aim_session_t *, aim_frame_t *, ...);
static int gaim_icon_error       (aim_session_t *, aim_frame_t *, ...);
static int gaim_icon_parseicon   (aim_session_t *, aim_frame_t *, ...);
static int oscar_icon_req        (aim_session_t *, aim_frame_t *, ...);
static int gaim_parse_msgack     (aim_session_t *, aim_frame_t *, ...);
static int gaim_parse_ratechange (aim_session_t *, aim_frame_t *, ...);
static int gaim_parse_evilnotify (aim_session_t *, aim_frame_t *, ...);
static int gaim_parse_searcherror(aim_session_t *, aim_frame_t *, ...);
static int gaim_parse_searchreply(aim_session_t *, aim_frame_t *, ...);
static int gaim_bosrights        (aim_session_t *, aim_frame_t *, ...);
static int gaim_connerr          (aim_session_t *, aim_frame_t *, ...);
static int conninitdone_admin    (aim_session_t *, aim_frame_t *, ...);
static int conninitdone_bos      (aim_session_t *, aim_frame_t *, ...);
static int conninitdone_chatnav  (aim_session_t *, aim_frame_t *, ...);
static int conninitdone_chat     (aim_session_t *, aim_frame_t *, ...);
static int conninitdone_email    (aim_session_t *, aim_frame_t *, ...);
static int conninitdone_icon     (aim_session_t *, aim_frame_t *, ...);
static int gaim_parse_msgerr     (aim_session_t *, aim_frame_t *, ...);
static int gaim_parse_mtn        (aim_session_t *, aim_frame_t *, ...);
static int gaim_parse_locaterights(aim_session_t *, aim_frame_t *, ...);
static int gaim_parse_buddyrights(aim_session_t *, aim_frame_t *, ...);
static int gaim_parse_locerr     (aim_session_t *, aim_frame_t *, ...);
static int gaim_icbm_param_info  (aim_session_t *, aim_frame_t *, ...);
static int gaim_parse_genericerr (aim_session_t *, aim_frame_t *, ...);
static int gaim_memrequest       (aim_session_t *, aim_frame_t *, ...);
static int gaim_selfinfo         (aim_session_t *, aim_frame_t *, ...);
static int gaim_offlinemsg       (aim_session_t *, aim_frame_t *, ...);
static int gaim_offlinemsgdone   (aim_session_t *, aim_frame_t *, ...);
static int gaim_icqalias         (aim_session_t *, aim_frame_t *, ...);
static int gaim_icqinfo          (aim_session_t *, aim_frame_t *, ...);
static int gaim_popup            (aim_session_t *, aim_frame_t *, ...);
#ifndef NOSSI
static int gaim_ssi_parseerr     (aim_session_t *, aim_frame_t *, ...);
static int gaim_ssi_parserights  (aim_session_t *, aim_frame_t *, ...);
static int gaim_ssi_parselist    (aim_session_t *, aim_frame_t *, ...);
static int gaim_ssi_parseack     (aim_session_t *, aim_frame_t *, ...);
static int gaim_ssi_authgiven    (aim_session_t *, aim_frame_t *, ...);
static int gaim_ssi_authrequest  (aim_session_t *, aim_frame_t *, ...);
static int gaim_ssi_authreply    (aim_session_t *, aim_frame_t *, ...);
static int gaim_ssi_gotadded     (aim_session_t *, aim_frame_t *, ...);
#endif

/* for DirectIM/image transfer */
static int gaim_odc_initiate     (aim_session_t *, aim_frame_t *, ...);
static int gaim_odc_incoming     (aim_session_t *, aim_frame_t *, ...);
static int gaim_odc_typing       (aim_session_t *, aim_frame_t *, ...);
static int gaim_odc_update_ui    (aim_session_t *, aim_frame_t *, ...);

/* for file transfer */
static int oscar_sendfile_estblsh(aim_session_t *, aim_frame_t *, ...);
static int oscar_sendfile_prompt (aim_session_t *, aim_frame_t *, ...);
static int oscar_sendfile_ack    (aim_session_t *, aim_frame_t *, ...);
static int oscar_sendfile_done   (aim_session_t *, aim_frame_t *, ...);

/* for icons */
static gboolean gaim_icon_timerfunc(gpointer data);

/* prpl actions - remove this at some point */
static void oscar_set_info(GaimConnection *gc, const char *text);

static void oscar_free_name_data(struct name_data *data) {
	g_free(data->name);
	g_free(data->nick);
	g_free(data);
}

static void oscar_free_buddyinfo(void *data) {
	struct buddyinfo *bi = data;
	g_free(bi->availmsg);
	g_free(bi->iconcsum);
	g_free(bi);
}

static fu32_t oscar_encoding_check(const char *utf8)
{
	int i = 0;
	fu32_t encodingflag = 0;

	/* Determine how we can send this message.  Per the warnings elsewhere 
	 * in this file, these little checks determine the simplest encoding 
	 * we can use for a given message send using it. */
	while (utf8[i]) {
		if ((unsigned char)utf8[i] > 0x7f) {
			/* not ASCII! */
			encodingflag = AIM_IMFLAGS_ISO_8859_1;
			break;
		}
		i++;
	}
	while (utf8[i]) {
		/* ISO-8859-1 is 0x00-0xbf in the first byte
		 * followed by 0xc0-0xc3 in the second */
		if ((unsigned char)utf8[i] < 0x80) {
			i++;
			continue;
		} else if (((unsigned char)utf8[i] & 0xfc) == 0xc0 &&
			   ((unsigned char)utf8[i + 1] & 0xc0) == 0x80) {
			i += 2;
			continue;
		}
		encodingflag = AIM_IMFLAGS_UNICODE;
		break;
	}

	return encodingflag;
}

static fu32_t oscar_encoding_parse(const char *enc)
{
	char *charset;

	/* If anything goes wrong, fall back on ASCII and print a message */
	if (enc == NULL) {
		gaim_debug(GAIM_DEBUG_WARNING, "oscar",
				   "Encoding was null, that's odd\n");
		return 0;
	}
	charset = strstr(enc, "charset=");
	if (charset == NULL) {
		gaim_debug(GAIM_DEBUG_WARNING, "oscar",
				   "No charset specified for info, assuming ASCII\n");
		return 0;
	}
	charset += 8;
	if (!strcmp(charset, "\"us-ascii\"") || !strcmp(charset, "\"utf-8\"")) {
		/* UTF-8 is our native charset, ASCII is a proper subset */
		return 0;
	} else if (!strcmp(charset, "\"iso-8859-1\"")) {
		return AIM_IMFLAGS_ISO_8859_1;
	} else if (!strcmp(charset, "\"unicode-2-0\"")) {
		return AIM_IMFLAGS_UNICODE;
	} else {
		gaim_debug(GAIM_DEBUG_WARNING, "oscar",
				   "Unrecognized character set '%s', using ASCII\n", charset);
		return 0;
	}
}

gchar *oscar_encoding_to_utf8(const char *encoding, char *text, int textlen)
{
	gchar *utf8 = NULL;
	int flags = oscar_encoding_parse(encoding);

	switch (flags) {
	case 0:
		utf8 = g_strndup(text, textlen);
		break;
	case AIM_IMFLAGS_ISO_8859_1:
		utf8 = g_convert(text, textlen, "UTF-8", "ISO-8859-1", NULL, NULL, NULL);
		break;
	case AIM_IMFLAGS_UNICODE:
		utf8 = g_convert(text, textlen, "UTF-8", "UCS-2BE", NULL, NULL, NULL);
		break;
	}

	return utf8;
}

static struct direct_im *find_direct_im(struct oscar_data *od, const char *who) {
	GSList *d = od->direct_ims;
	struct direct_im *m = NULL;

	while (d) {
		m = (struct direct_im *)d->data;
		if (!aim_sncmp(who, m->name))
			return m;
		d = d->next;
	}

	return NULL;
}

static char *extract_name(const char *name) {
	char *tmp, *x;
	int i, j;

	if (!name)
		return NULL;
	
	x = strchr(name, '-');

	if (!x) return NULL;
	x = strchr(++x, '-');
	if (!x) return NULL;
	tmp = g_strdup(++x);

	for (i = 0, j = 0; x[i]; i++) {
		char hex[3];
		if (x[i] != '%') {
			tmp[j++] = x[i];
			continue;
		}
		strncpy(hex, x + ++i, 2); hex[2] = 0;
		i++;
		tmp[j++] = strtol(hex, NULL, 16);
	}

	tmp[j] = 0;
	return tmp;
}

static struct chat_connection *find_oscar_chat(GaimConnection *gc, int id) {
	GSList *g = ((struct oscar_data *)gc->proto_data)->oscar_chats;
	struct chat_connection *c = NULL;

	while (g) {
		c = (struct chat_connection *)g->data;
		if (c->id == id)
			break;
		g = g->next;
		c = NULL;
	}

	return c;
}

static struct chat_connection *find_oscar_chat_by_conn(GaimConnection *gc,
							aim_conn_t *conn) {
	GSList *g = ((struct oscar_data *)gc->proto_data)->oscar_chats;
	struct chat_connection *c = NULL;

	while (g) {
		c = (struct chat_connection *)g->data;
		if (c->conn == conn)
			break;
		g = g->next;
		c = NULL;
	}

	return c;
}

static void gaim_odc_disconnect(aim_session_t *sess, aim_conn_t *conn) {
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
	GaimConversation *cnv;
	struct direct_im *dim;
	char *sn;
	char buf[256];

	sn = g_strdup(aim_odc_getsn(conn));

	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "%s disconnected Direct IM.\n", sn);

	dim = find_direct_im(od, sn);
	od->direct_ims = g_slist_remove(od->direct_ims, dim);
	gaim_input_remove(dim->watcher);

	if (dim->connected)
		g_snprintf(buf, sizeof buf, _("Direct IM with %s closed"), sn);
	else 
		g_snprintf(buf, sizeof buf, _("Direct IM with %s failed"), sn);

	cnv = gaim_find_conversation_with_account(sn, gaim_connection_get_account(gc));
	if (cnv)
		gaim_conversation_write(cnv, NULL, buf, GAIM_MESSAGE_SYSTEM, time(NULL));

	gaim_conversation_update_progress(cnv, 0);

	g_free(dim); /* I guess? I don't see it anywhere else... -- mid */
	g_free(sn);

	return;
}

static void oscar_callback(gpointer data, gint source, GaimInputCondition condition) {
	aim_conn_t *conn = (aim_conn_t *)data;
	aim_session_t *sess = aim_conn_getsess(conn);
	GaimConnection *gc = sess ? sess->aux_data : NULL;
	struct oscar_data *od;

	if (!gc) {
		/* gc is null. we return, else we seg SIGSEG on next line. */
		gaim_debug(GAIM_DEBUG_INFO, "oscar",
				   "oscar callback for closed connection (1).\n");
		return;
	}
      
	od = (struct oscar_data *)gc->proto_data;

	if (!g_list_find(gaim_connections_get_all(), gc)) {
		/* oh boy. this is probably bad. i guess the only thing we 
		 * can really do is return? */
		gaim_debug(GAIM_DEBUG_INFO, "oscar",
				   "oscar callback for closed connection (2).\n");
		gaim_debug(GAIM_DEBUG_MISC, "oscar", "gc = %p\n", gc);
		return;
	}

	if (condition & GAIM_INPUT_READ) {
		if (conn->type == AIM_CONN_TYPE_LISTENER) {
			gaim_debug(GAIM_DEBUG_INFO, "oscar",
					   "got information on rendezvous listener\n");
			if (aim_handlerendconnect(od->sess, conn) < 0) {
				gaim_debug(GAIM_DEBUG_ERROR, "oscar",
						   "connection error (rendezvous listener)\n");
				aim_conn_kill(od->sess, &conn);
			}
		} else {
			if (aim_get_command(od->sess, conn) >= 0) {
				aim_rxdispatch(od->sess);
				if (od->killme) {
					gaim_debug(GAIM_DEBUG_ERROR, "oscar", "Waiting to be destroyed\n");
					return;
				}
			} else {
				if ((conn->type == AIM_CONN_TYPE_BOS) ||
					   !(aim_getconn_type(od->sess, AIM_CONN_TYPE_BOS))) {
					gaim_debug(GAIM_DEBUG_ERROR, "oscar",
							   "major connection error\n");
					gaim_connection_error(gc, _("Disconnected."));
				} else if (conn->type == AIM_CONN_TYPE_CHAT) {
					struct chat_connection *c = find_oscar_chat_by_conn(gc, conn);
					char *buf;
					gaim_debug(GAIM_DEBUG_INFO, "oscar",
							   "disconnected from chat room %s\n", c->name);
					c->conn = NULL;
					if (c->inpa > 0)
						gaim_input_remove(c->inpa);
					c->inpa = 0;
					c->fd = -1;
					aim_conn_kill(od->sess, &conn);
					buf = g_strdup_printf(_("You have been disconnected from chat room %s."), c->name);
					gaim_notify_error(gc, NULL, buf, NULL);
					g_free(buf);
				} else if (conn->type == AIM_CONN_TYPE_CHATNAV) {
					if (od->cnpa > 0)
						gaim_input_remove(od->cnpa);
					od->cnpa = 0;
					gaim_debug(GAIM_DEBUG_INFO, "oscar",
							   "removing chatnav input watcher\n");
					while (od->create_rooms) {
						struct create_room *cr = od->create_rooms->data;
						g_free(cr->name);
						od->create_rooms =
							g_slist_remove(od->create_rooms, cr);
						g_free(cr);
						gaim_notify_error(gc, NULL,
										  _("Chat is currently unavailable"),
										  NULL);
					}
					aim_conn_kill(od->sess, &conn);
				} else if (conn->type == AIM_CONN_TYPE_AUTH) {
					if (od->paspa > 0)
						gaim_input_remove(od->paspa);
					od->paspa = 0;
					gaim_debug(GAIM_DEBUG_INFO, "oscar",
							   "removing authconn input watcher\n");
					aim_conn_kill(od->sess, &conn);
				} else if (conn->type == AIM_CONN_TYPE_EMAIL) {
					if (od->emlpa > 0)
						gaim_input_remove(od->emlpa);
					od->emlpa = 0;
					gaim_debug(GAIM_DEBUG_INFO, "oscar",
							   "removing email input watcher\n");
					aim_conn_kill(od->sess, &conn);
				} else if (conn->type == AIM_CONN_TYPE_ICON) {
					if (od->icopa > 0)
						gaim_input_remove(od->icopa);
					od->icopa = 0;
					gaim_debug(GAIM_DEBUG_INFO, "oscar",
							   "removing icon input watcher\n");
					aim_conn_kill(od->sess, &conn);
				} else if (conn->type == AIM_CONN_TYPE_RENDEZVOUS) {
					if (conn->subtype == AIM_CONN_SUBTYPE_OFT_DIRECTIM)
						gaim_odc_disconnect(od->sess, conn);
					aim_conn_kill(od->sess, &conn);
				} else {
					gaim_debug(GAIM_DEBUG_ERROR, "oscar",
							   "holy crap! generic connection error! %hu\n",
							   conn->type);
					aim_conn_kill(od->sess, &conn);
				}
			}
		}
	}
}

static void oscar_debug(aim_session_t *sess, int level, const char *format, va_list va) {
	char *s = g_strdup_vprintf(format, va);
	char buf[256];
	char *t;
	GaimConnection *gc = sess->aux_data;

	g_snprintf(buf, sizeof(buf), "%s %d: ", gaim_account_get_username(gaim_connection_get_account(gc)), level);
	t = g_strconcat(buf, s, NULL);
	gaim_debug(GAIM_DEBUG_INFO, "oscar", t);
	if (t[strlen(t)-1] != '\n')
		gaim_debug(GAIM_DEBUG_INFO, NULL, "\n");
	g_free(t);
	g_free(s);
}

static void oscar_login_connect(gpointer data, gint source, GaimInputCondition cond)
{
	GaimConnection *gc = data;
	struct oscar_data *od;
	aim_session_t *sess;
	aim_conn_t *conn;

	if (!g_list_find(gaim_connections_get_all(), gc)) {
		close(source);
		return;
	}

	od = gc->proto_data;
	sess = od->sess;
	conn = aim_getconn_type_all(sess, AIM_CONN_TYPE_AUTH);
	
	conn->fd = source;

	if (source < 0) {
		gaim_connection_error(gc, _("Couldn't connect to host"));
		return;
	}

	aim_conn_completeconnect(sess, conn);
	gc->inpa = gaim_input_add(conn->fd, GAIM_INPUT_READ, oscar_callback, conn);
	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "Password sent, waiting for response\n");
}

static void oscar_login(GaimAccount *account) {
	aim_session_t *sess;
	aim_conn_t *conn;
	char buf[256];
	GaimConnection *gc = gaim_account_get_connection(account);
	struct oscar_data *od = gc->proto_data = g_new0(struct oscar_data, 1);

	gaim_debug(GAIM_DEBUG_MISC, "oscar", "oscar_login: gc = %p\n", gc);

	if (isdigit(*(gaim_account_get_username(account)))) {
		od->icq = TRUE;
	} else {
		gc->flags |= GAIM_CONNECTION_HTML;
		gc->flags |= GAIM_CONNECTION_AUTO_RESP;
	}
	od->buddyinfo = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, oscar_free_buddyinfo);

	sess = g_new0(aim_session_t, 1);

	aim_session_init(sess, AIM_SESS_FLAGS_NONBLOCKCONNECT, 0);
	aim_setdebuggingcb(sess, oscar_debug);

	/* we need an immediate queue because we don't use a while-loop to
	 * see if things need to be sent. */
	aim_tx_setenqueue(sess, AIM_TX_IMMEDIATE, NULL);
	od->sess = sess;
	sess->aux_data = gc;

	conn = aim_newconn(sess, AIM_CONN_TYPE_AUTH, NULL);
	if (conn == NULL) {
		gaim_debug(GAIM_DEBUG_ERROR, "oscar",
				   "internal connection error\n");
		gaim_connection_error(gc, _("Unable to login to AIM"));
		return;
	}

	g_snprintf(buf, sizeof(buf), _("Signon: %s"), gaim_account_get_username(account));
	gaim_connection_update_progress(gc, buf, 2, 5);

	aim_conn_addhandler(sess, conn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNERR, gaim_connerr, 0);
	aim_conn_addhandler(sess, conn, 0x0017, 0x0007, gaim_parse_login, 0);
	aim_conn_addhandler(sess, conn, 0x0017, 0x0003, gaim_parse_auth_resp, 0);

	conn->status |= AIM_CONN_STATUS_INPROGRESS;
	if (gaim_proxy_connect(account, gaim_account_get_string(account, "server", FAIM_LOGIN_SERVER),
			  gaim_account_get_int(account, "port", FAIM_LOGIN_PORT),
			  oscar_login_connect, gc) < 0) {
		gaim_connection_error(gc, _("Couldn't connect to host"));
		return;
	}
	aim_request_login(sess, conn, gaim_account_get_username(account));
}

static void oscar_close(GaimConnection *gc) {
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;

	while (od->oscar_chats) {
		struct chat_connection *n = od->oscar_chats->data;
		if (n->inpa > 0)
			gaim_input_remove(n->inpa);
		g_free(n->name);
		g_free(n->show);
		od->oscar_chats = g_slist_remove(od->oscar_chats, n);
		g_free(n);
	}
	while (od->direct_ims) {
		struct direct_im *n = od->direct_ims->data;
		if (n->watcher > 0)
			gaim_input_remove(n->watcher);
		od->direct_ims = g_slist_remove(od->direct_ims, n);
		g_free(n);
	}
/* BBB */
	while (od->file_transfers) {
		GaimXfer *xfer;
		xfer = (GaimXfer *)od->file_transfers->data;
		gaim_xfer_destroy(xfer);
	}
	while (od->requesticon) {
		char *sn = od->requesticon->data;
		od->requesticon = g_slist_remove(od->requesticon, sn);
		free(sn);
	}
	g_hash_table_destroy(od->buddyinfo);
	while (od->evilhack) {
		g_free(od->evilhack->data);
		od->evilhack = g_slist_remove(od->evilhack, od->evilhack->data);
	}
	while (od->create_rooms) {
		struct create_room *cr = od->create_rooms->data;
		g_free(cr->name);
		od->create_rooms = g_slist_remove(od->create_rooms, cr);
		g_free(cr);
	}
	if (od->email)
		g_free(od->email);
	if (od->newp)
		g_free(od->newp);
	if (od->oldp)
		g_free(od->oldp);
	if (gc->inpa > 0)
		gaim_input_remove(gc->inpa);
	if (od->cnpa > 0)
		gaim_input_remove(od->cnpa);
	if (od->paspa > 0)
		gaim_input_remove(od->paspa);
	if (od->emlpa > 0)
		gaim_input_remove(od->emlpa);
	if (od->icopa > 0)
		gaim_input_remove(od->icopa);
	if (od->icontimer > 0)
		g_source_remove(od->icontimer);
	if (od->getblisttimer)
		g_source_remove(od->getblisttimer);
	aim_session_kill(od->sess);
	g_free(od->sess);
	od->sess = NULL;
	g_free(gc->proto_data);
	gc->proto_data = NULL;
	gaim_debug(GAIM_DEBUG_INFO, "oscar", "Signed off.\n");
}

static void oscar_bos_connect(gpointer data, gint source, GaimInputCondition cond) {
	GaimConnection *gc = data;
	struct oscar_data *od;
	aim_session_t *sess;
	aim_conn_t *bosconn;

	if (!g_list_find(gaim_connections_get_all(), gc)) {
		close(source);
		return;
	}

	od = gc->proto_data;
	sess = od->sess;
	bosconn = od->conn;	
	bosconn->fd = source;

	if (source < 0) {
		gaim_connection_error(gc, _("Could Not Connect"));
		return;
	}

	aim_conn_completeconnect(sess, bosconn);
	gc->inpa = gaim_input_add(bosconn->fd, GAIM_INPUT_READ, oscar_callback, bosconn);
	gaim_connection_update_progress(gc,
			_("Connection established, cookie sent"), 4, 5);
}

/* BBB */
/*
 * This little area in oscar.c is the nexus of file transfer code, 
 * so I wrote a little explanation of what happens.  I am such a 
 * ninja.
 *
 * The series of events for a file send is:
 *  -Create xfer and call gaim_xfer_request (this happens in oscar_ask_sendfile)
 *  -User chooses a file and oscar_xfer_init is called.  It establishs a 
 *   listening socket, then asks the remote user to connect to us (and 
 *   gives them the file name, port, IP, etc.)
 *  -They connect to us and we send them an AIM_CB_OFT_PROMPT (this happens 
 *   in oscar_sendfile_estblsh)
 *  -They send us an AIM_CB_OFT_ACK and then we start sending data
 *  -When we finish, they send us an AIM_CB_OFT_DONE and they close the 
 *   connection.
 *  -We get drunk because file transfer kicks ass.
 *
 * The series of events for a file receive is:
 *  -Create xfer and call gaim_xfer request (this happens in incomingim_chan2)
 *  -Gaim user selects file to name and location to save file to and 
 *   oscar_xfer_init is called
 *  -It connects to the remote user using the IP they gave us earlier
 *  -After connecting, they send us an AIM_CB_OFT_PROMPT.  In reply, we send 
 *   them an AIM_CB_OFT_ACK.
 *  -They begin to send us lots of raw data.
 *  -When they finish sending data we send an AIM_CB_OFT_DONE and then close 
 *   the connectionn.
 */
static void oscar_sendfile_connected(gpointer data, gint source, GaimInputCondition condition);

/* XXX - This function is pretty ugly */
static void oscar_xfer_init(GaimXfer *xfer)
{
	struct aim_oft_info *oft_info = xfer->data;
	GaimConnection *gc = oft_info->sess->aux_data;
	struct oscar_data *od = gc->proto_data;

	if (gaim_xfer_get_type(xfer) == GAIM_XFER_SEND) {
		int i;

		xfer->filename = g_path_get_basename(xfer->local_filename);
		strncpy(oft_info->fh.name, xfer->filename, 64);
		oft_info->fh.totsize = gaim_xfer_get_size(xfer);
		oft_info->fh.size = gaim_xfer_get_size(xfer);
		oft_info->fh.checksum = aim_oft_checksum_file(xfer->local_filename);

		/*
		 * First try the port specified earlier (5190).  If that fails, 
		 * increment by 1 and try again.
		 */
		aim_sendfile_listen(od->sess, oft_info);
		for (i=0; (i<5 && !oft_info->conn); i++) {
			xfer->local_port = oft_info->port = oft_info->port + 1;
			aim_sendfile_listen(od->sess, oft_info);
		}
		gaim_debug(GAIM_DEBUG_MISC, "oscar",
				   "port is %d, ip is %s\n",
				   xfer->local_port, oft_info->clientip);
		if (oft_info->conn) {
			xfer->watcher = gaim_input_add(oft_info->conn->fd, GAIM_INPUT_READ, oscar_callback, oft_info->conn);
			aim_im_sendch2_sendfile_ask(od->sess, oft_info);
			aim_conn_addhandler(od->sess, oft_info->conn, AIM_CB_FAM_OFT, AIM_CB_OFT_ESTABLISHED, oscar_sendfile_estblsh, 0);
		} else {
			gaim_notify_error(gc, NULL, _("File Transfer Aborted"),
							  _("Unable to establish listener socket."));
			/* XXX - The below line causes a crash because the transfer is canceled before the "Ok" callback on the file selection thing exists, I think */
			/* gaim_xfer_cancel_remote(xfer); */
		}
	} else if (gaim_xfer_get_type(xfer) == GAIM_XFER_RECEIVE) {
		oft_info->conn = aim_newconn(od->sess, AIM_CONN_TYPE_RENDEZVOUS, NULL);
		if (oft_info->conn) {
			oft_info->conn->subtype = AIM_CONN_SUBTYPE_OFT_SENDFILE;
			aim_conn_addhandler(od->sess, oft_info->conn, AIM_CB_FAM_OFT, AIM_CB_OFT_PROMPT, oscar_sendfile_prompt, 0);
			oft_info->conn->fd = xfer->fd = gaim_proxy_connect(gaim_connection_get_account(gc), xfer->remote_ip, xfer->remote_port, 
								      oscar_sendfile_connected, xfer);
			if (xfer->fd == -1) {
				gaim_notify_error(gc, NULL, _("File Transfer Aborted"),
								  _("Unable to establish file descriptor."));
				/* gaim_xfer_cancel_remote(xfer); */
			}
		} else {
			gaim_notify_error(gc, NULL, _("File Transfer Aborted"),
							  _("Unable to create new connection."));
			/* gaim_xfer_cancel_remote(xfer); */
			/* Try a different port? Ask them to connect to us? */
		}

	}
}

static void oscar_xfer_start(GaimXfer *xfer)
{

	gaim_debug(GAIM_DEBUG_INFO, "oscar", "AAA - in oscar_xfer_start\n");
	/* I'm pretty sure we don't need to do jack here.  Nor Jill. */
}

static void oscar_xfer_end(GaimXfer *xfer)
{
	struct aim_oft_info *oft_info = xfer->data;
	GaimConnection *gc = oft_info->sess->aux_data;
	struct oscar_data *od = gc->proto_data;

	gaim_debug(GAIM_DEBUG_INFO, "oscar", "AAA - in oscar_xfer_end\n");

	if (gaim_xfer_get_type(xfer) == GAIM_XFER_RECEIVE) {
		oft_info->fh.nrecvd = gaim_xfer_get_bytes_sent(xfer);
		aim_oft_sendheader(oft_info->sess, AIM_CB_OFT_DONE, oft_info);
	}

	aim_conn_kill(oft_info->sess, &oft_info->conn);
	aim_oft_destroyinfo(oft_info);
	xfer->data = NULL;
	od->file_transfers = g_slist_remove(od->file_transfers, xfer);
}

static void oscar_xfer_cancel_send(GaimXfer *xfer)
{
	struct aim_oft_info *oft_info = xfer->data;
	GaimConnection *gc = oft_info->sess->aux_data;
	struct oscar_data *od = gc->proto_data;

	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "AAA - in oscar_xfer_cancel_send\n");

	aim_im_sendch2_sendfile_cancel(oft_info->sess, oft_info);

	aim_conn_kill(oft_info->sess, &oft_info->conn);
	aim_oft_destroyinfo(oft_info);
	xfer->data = NULL;
	od->file_transfers = g_slist_remove(od->file_transfers, xfer);
}

static void oscar_xfer_cancel_recv(GaimXfer *xfer)
{
	struct aim_oft_info *oft_info = xfer->data;
	GaimConnection *gc = oft_info->sess->aux_data;
	struct oscar_data *od = gc->proto_data;

	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "AAA - in oscar_xfer_cancel_recv\n");

	aim_im_sendch2_sendfile_cancel(oft_info->sess, oft_info);

	aim_conn_kill(oft_info->sess, &oft_info->conn);
	aim_oft_destroyinfo(oft_info);
	xfer->data = NULL;
	od->file_transfers = g_slist_remove(od->file_transfers, xfer);
}

static void oscar_xfer_ack(GaimXfer *xfer, const char *buffer, size_t size)
{
	struct aim_oft_info *oft_info = xfer->data;

	if (gaim_xfer_get_type(xfer) == GAIM_XFER_SEND) {
		/*
		 * If we're done sending, intercept the socket from the core ft code
		 * and wait for the other guy to send the "done" OFT packet.
		 */
		if (gaim_xfer_get_bytes_remaining(xfer) <= 0) {
			gaim_input_remove(xfer->watcher);
			xfer->watcher = gaim_input_add(xfer->fd, GAIM_INPUT_READ, oscar_callback, oft_info->conn);
			xfer->fd = 0;
			gaim_xfer_set_completed(xfer, TRUE);
		}
	} else if (gaim_xfer_get_type(xfer) == GAIM_XFER_RECEIVE) {
		/* Update our rolling checksum.  Like Walmart, yo. */
		oft_info->fh.recvcsum = aim_oft_checksum_chunk(buffer, size, oft_info->fh.recvcsum);
	}
}

static GaimXfer *oscar_find_xfer_by_cookie(GSList *fts, const char *ck)
{
	GaimXfer *xfer;
	struct aim_oft_info *oft_info;

	while (fts) {
		xfer = fts->data;
		oft_info = xfer->data;

		if (oft_info && !strcmp(ck, oft_info->cookie))
			return xfer;

		fts = g_slist_next(fts);
	}

	return NULL;
}

static GaimXfer *oscar_find_xfer_by_conn(GSList *fts, aim_conn_t *conn)
{
	GaimXfer *xfer;
	struct aim_oft_info *oft_info;

	while (fts) {
		xfer = fts->data;
		oft_info = xfer->data;

		if (oft_info && (conn == oft_info->conn))
			return xfer;

		fts = g_slist_next(fts);
	}

	return NULL;
}

static void oscar_ask_sendfile(GaimConnection *gc, const char *destsn) {
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
	GaimXfer *xfer;
	struct aim_oft_info *oft_info;

	/* You want to send a file to someone else, you're so generous */

	/* Build the file transfer handle */
	xfer = gaim_xfer_new(gaim_connection_get_account(gc), GAIM_XFER_SEND, destsn);
	xfer->local_port = 5190;

	/* Create the oscar-specific data */
	oft_info = aim_oft_createinfo(od->sess, NULL, destsn, xfer->local_ip, xfer->local_port, 0, 0, NULL);
	xfer->data = oft_info;

	 /* Setup our I/O op functions */
	gaim_xfer_set_init_fnc(xfer, oscar_xfer_init);
	gaim_xfer_set_start_fnc(xfer, oscar_xfer_start);
	gaim_xfer_set_end_fnc(xfer, oscar_xfer_end);
	gaim_xfer_set_cancel_send_fnc(xfer, oscar_xfer_cancel_send);
	gaim_xfer_set_cancel_recv_fnc(xfer, oscar_xfer_cancel_recv);
	gaim_xfer_set_ack_fnc(xfer, oscar_xfer_ack);

	/* Keep track of this transfer for later */
	od->file_transfers = g_slist_append(od->file_transfers, xfer);

	/* Now perform the request */
	gaim_xfer_request(xfer);
}

static int gaim_parse_auth_resp(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = gc->proto_data;
	GaimAccount *account = gc->account;
	aim_conn_t *bosconn;
	char *host; int port;
	int i, rc;
	va_list ap;
	struct aim_authresp_info *info;

	port = gaim_account_get_int(account, "port", FAIM_LOGIN_PORT);

	va_start(ap, fr);
	info = va_arg(ap, struct aim_authresp_info *);
	va_end(ap);

	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "inside auth_resp (Screen name: %s)\n", info->sn);

	if (info->errorcode || !info->bosip || !info->cookielen || !info->cookie) {
		char buf[256];
		switch (info->errorcode) {
		case 0x05:
			/* Incorrect nick/password */
			gc->wants_to_die = TRUE;
			gaim_connection_error(gc, _("Incorrect nickname or password."));
			break;
		case 0x11:
			/* Suspended account */
			gc->wants_to_die = TRUE;
			gaim_connection_error(gc, _("Your account is currently suspended."));
			break;
		case 0x14:
			/* service temporarily unavailable */
			gaim_connection_error(gc, _("The AOL Instant Messenger service is temporarily unavailable."));
			break;
		case 0x18:
			/* connecting too frequently */
			gc->wants_to_die = TRUE;
			gaim_connection_error(gc, _("You have been connecting and disconnecting too frequently. Wait ten minutes and try again. If you continue to try, you will need to wait even longer."));
			break;
		case 0x1c:
			/* client too old */
			gc->wants_to_die = TRUE;
			g_snprintf(buf, sizeof(buf), _("The client version you are using is too old. Please upgrade at %s"), GAIM_WEBSITE);
			gaim_connection_error(gc, buf);
			break;
		default:
			gaim_connection_error(gc, _("Authentication failed"));
			break;
		}
		gaim_debug(GAIM_DEBUG_ERROR, "oscar",
				   "Login Error Code 0x%04hx\n", info->errorcode);
		gaim_debug(GAIM_DEBUG_ERROR, "oscar",
				   "Error URL: %s\n", info->errorurl);
		od->killme = TRUE;
		return 1;
	}


	gaim_debug(GAIM_DEBUG_MISC, "oscar",
			   "Reg status: %hu\n", info->regstatus);

	if (info->email) {
		gaim_debug(GAIM_DEBUG_MISC, "oscar", "Email: %s\n", info->email);
	} else {
		gaim_debug(GAIM_DEBUG_MISC, "oscar", "Email is NULL\n");
	}
	
	gaim_debug(GAIM_DEBUG_MISC, "oscar", "BOSIP: %s\n", info->bosip);
	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "Closing auth connection...\n");
	aim_conn_kill(sess, &fr->conn);

	bosconn = aim_newconn(sess, AIM_CONN_TYPE_BOS, NULL);
	if (bosconn == NULL) {
		gaim_connection_error(gc, _("Internal Error"));
		od->killme = TRUE;
		return 0;
	}

	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNERR, gaim_connerr, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, conninitdone_bos, 0);
	aim_conn_addhandler(sess, bosconn, 0x0009, 0x0003, gaim_bosrights, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_ACK, AIM_CB_ACK_ACK, NULL, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_REDIRECT, gaim_handle_redirect, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_LOC, AIM_CB_LOC_RIGHTSINFO, gaim_parse_locaterights, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_BUD, AIM_CB_BUD_RIGHTSINFO, gaim_parse_buddyrights, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_BUD, AIM_CB_BUD_ONCOMING, gaim_parse_oncoming, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_BUD, AIM_CB_BUD_OFFGOING, gaim_parse_offgoing, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_INCOMING, gaim_parse_incoming_im, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_LOC, AIM_CB_LOC_ERROR, gaim_parse_locerr, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_MISSEDCALL, gaim_parse_misses, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_CLIENTAUTORESP, gaim_parse_clientauto, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_RATECHANGE, gaim_parse_ratechange, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_EVIL, gaim_parse_evilnotify, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_LOK, AIM_CB_LOK_ERROR, gaim_parse_searcherror, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_LOK, 0x0003, gaim_parse_searchreply, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_ERROR, gaim_parse_msgerr, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_MTN, gaim_parse_mtn, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_LOC, AIM_CB_LOC_USERINFO, gaim_parse_user_info, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_ACK, gaim_parse_msgack, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_MOTD, gaim_parse_motd, 0);
	aim_conn_addhandler(sess, bosconn, 0x0004, 0x0005, gaim_icbm_param_info, 0);
	aim_conn_addhandler(sess, bosconn, 0x0001, 0x0001, gaim_parse_genericerr, 0);
	aim_conn_addhandler(sess, bosconn, 0x0003, 0x0001, gaim_parse_genericerr, 0);
	aim_conn_addhandler(sess, bosconn, 0x0009, 0x0001, gaim_parse_genericerr, 0);
	aim_conn_addhandler(sess, bosconn, 0x0001, 0x001f, gaim_memrequest, 0);
	aim_conn_addhandler(sess, bosconn, 0x0001, 0x000f, gaim_selfinfo, 0);
	aim_conn_addhandler(sess, bosconn, 0x0001, 0x0021, oscar_icon_req,0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_ICQ, AIM_CB_ICQ_OFFLINEMSG, gaim_offlinemsg, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_ICQ, AIM_CB_ICQ_OFFLINEMSGCOMPLETE, gaim_offlinemsgdone, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_POP, 0x0002, gaim_popup, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_ICQ, AIM_CB_ICQ_ALIAS, gaim_icqalias, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_ICQ, AIM_CB_ICQ_INFO, gaim_icqinfo, 0);
#ifndef NOSSI
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SSI, AIM_CB_SSI_ERROR, gaim_ssi_parseerr, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SSI, AIM_CB_SSI_RIGHTSINFO, gaim_ssi_parserights, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SSI, AIM_CB_SSI_LIST, gaim_ssi_parselist, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SSI, AIM_CB_SSI_NOLIST, gaim_ssi_parselist, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SSI, AIM_CB_SSI_SRVACK, gaim_ssi_parseack, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SSI, AIM_CB_SSI_RECVAUTH, gaim_ssi_authgiven, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SSI, AIM_CB_SSI_RECVAUTHREQ, gaim_ssi_authrequest, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SSI, AIM_CB_SSI_RECVAUTHREP, gaim_ssi_authreply, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SSI, AIM_CB_SSI_ADDED, gaim_ssi_gotadded, 0);
#endif

	((struct oscar_data *)gc->proto_data)->conn = bosconn;
	for (i = 0; i < (int)strlen(info->bosip); i++) {
		if (info->bosip[i] == ':') {
			port = atoi(&(info->bosip[i+1]));
			break;
		}
	}
	host = g_strndup(info->bosip, i);
	bosconn->status |= AIM_CONN_STATUS_INPROGRESS;
	rc = gaim_proxy_connect(gc->account, host, port, oscar_bos_connect, gc);
	g_free(host);
	if (rc < 0) {
		gaim_connection_error(gc, _("Could Not Connect"));
		od->killme = TRUE;
		return 0;
	}
	aim_sendcookie(sess, bosconn, info->cookielen, info->cookie);
	gaim_input_remove(gc->inpa);

	return 1;
}

struct pieceofcrap {
	GaimConnection *gc;
	unsigned long offset;
	unsigned long len;
	char *modname;
	int fd;
	aim_conn_t *conn;
	unsigned int inpa;
};

static void damn_you(gpointer data, gint source, GaimInputCondition c)
{
	struct pieceofcrap *pos = data;
	struct oscar_data *od = pos->gc->proto_data;
	char in = '\0';
	int x = 0;
	unsigned char m[17];

	while (read(pos->fd, &in, 1) == 1) {
		if (in == '\n')
			x++;
		else if (in != '\r')
			x = 0;
		if (x == 2)
			break;
		in = '\0';
	}
	if (in != '\n') {
		char buf[256];
		g_snprintf(buf, sizeof(buf), _("You may be disconnected shortly.  You may want to use TOC until "
			"this is fixed.  Check %s for updates."), GAIM_WEBSITE);
		gaim_notify_warning(pos->gc, NULL,
							_("Gaim was Unable to get a valid AIM login hash."),
							buf);
		gaim_input_remove(pos->inpa);
		close(pos->fd);
		g_free(pos);
		return;
	}
	read(pos->fd, m, 16);
	m[16] = '\0';
	gaim_debug(GAIM_DEBUG_MISC, "oscar", "Sending hash: ");
	for (x = 0; x < 16; x++)
		gaim_debug(GAIM_DEBUG_MISC, NULL, "%02hhx ", (unsigned char)m[x]);

	gaim_debug(GAIM_DEBUG_MISC, NULL, "\n");
	gaim_input_remove(pos->inpa);
	close(pos->fd);
	aim_sendmemblock(od->sess, pos->conn, 0, 16, m, AIM_SENDMEMBLOCK_FLAG_ISHASH);
	g_free(pos);
}

static void straight_to_hell(gpointer data, gint source, GaimInputCondition cond) {
	struct pieceofcrap *pos = data;
	gchar *buf;

	pos->fd = source;

	if (source < 0) {
		buf = g_strdup_printf(_("You may be disconnected shortly.  You may want to use TOC until "
			"this is fixed.  Check %s for updates."), GAIM_WEBSITE);
		gaim_notify_warning(pos->gc, NULL,
							_("Gaim was Unable to get a valid AIM login hash."),
							buf);
		g_free(buf);
		if (pos->modname)
			g_free(pos->modname);
		g_free(pos);
		return;
	}

	buf = g_strdup_printf("GET " AIMHASHDATA "?offset=%ld&len=%ld&modname=%s HTTP/1.0\n\n",
			pos->offset, pos->len, pos->modname ? pos->modname : "");
	write(pos->fd, buf, strlen(buf));
	g_free(buf);
	if (pos->modname)
		g_free(pos->modname);
	pos->inpa = gaim_input_add(pos->fd, GAIM_INPUT_READ, damn_you, pos);
	return;
}

/* size of icbmui.ocm, the largest module in AIM 3.5 */
#define AIM_MAX_FILE_SIZE 98304

int gaim_memrequest(aim_session_t *sess, aim_frame_t *fr, ...) {
	va_list ap;
	struct pieceofcrap *pos;
	fu32_t offset, len;
	char *modname;

	va_start(ap, fr);
	offset = va_arg(ap, fu32_t);
	len = va_arg(ap, fu32_t);
	modname = va_arg(ap, char *);
	va_end(ap);

	gaim_debug(GAIM_DEBUG_MISC, "oscar",
			   "offset: %u, len: %u, file: %s\n",
			   offset, len, (modname ? modname : "aim.exe"));

	if (len == 0) {
		gaim_debug(GAIM_DEBUG_MISC, "oscar", "len is 0, hashing NULL\n");
		aim_sendmemblock(sess, fr->conn, offset, len, NULL,
				AIM_SENDMEMBLOCK_FLAG_ISREQUEST);
		return 1;
	}
	/* uncomment this when you're convinced it's right. remember, it's been wrong before.
	if (offset > AIM_MAX_FILE_SIZE || len > AIM_MAX_FILE_SIZE) {
		char *buf;
		int i = 8;
		if (modname)
			i += strlen(modname);
		buf = g_malloc(i);
		i = 0;
		if (modname) {
			memcpy(buf, modname, strlen(modname));
			i += strlen(modname);
		}
		buf[i++] = offset & 0xff;
		buf[i++] = (offset >> 8) & 0xff;
		buf[i++] = (offset >> 16) & 0xff;
		buf[i++] = (offset >> 24) & 0xff;
		buf[i++] = len & 0xff;
		buf[i++] = (len >> 8) & 0xff;
		buf[i++] = (len >> 16) & 0xff;
		buf[i++] = (len >> 24) & 0xff;
		gaim_debug(GAIM_DEBUG_MISC, "oscar", "len + offset is invalid, "
		           "hashing request\n");
		aim_sendmemblock(sess, command->conn, offset, i, buf, AIM_SENDMEMBLOCK_FLAG_ISREQUEST);
		g_free(buf);
		return 1;
	}
	*/

	pos = g_new0(struct pieceofcrap, 1);
	pos->gc = sess->aux_data;
	pos->conn = fr->conn;

	pos->offset = offset;
	pos->len = len;
	pos->modname = modname ? g_strdup(modname) : NULL;

	if (gaim_proxy_connect(pos->gc->account, "gaim.sourceforge.net", 80, straight_to_hell, pos) != 0) {
		char buf[256];
		if (pos->modname)
			g_free(pos->modname);
		g_free(pos);
		g_snprintf(buf, sizeof(buf), _("You may be disconnected shortly.  You may want to use TOC until "
			"this is fixed.  Check %s for updates."), GAIM_WEBSITE);
		gaim_notify_warning(pos->gc, NULL,
							_("Gaim was Unable to get a valid login hash."),
							buf);
	}

	return 1;
}

static int gaim_parse_login(aim_session_t *sess, aim_frame_t *fr, ...) {
	char *key;
	va_list ap;
	GaimConnection *gc = sess->aux_data;
	GaimAccount *account = gaim_connection_get_account(gc);
	GaimAccount *ac = gaim_connection_get_account(gc);
	struct oscar_data *od = gc->proto_data;

	va_start(ap, fr);
	key = va_arg(ap, char *);
	va_end(ap);

	if (od->icq) {
		struct client_info_s info = CLIENTINFO_ICQ_KNOWNGOOD;
		aim_send_login(sess, fr->conn, gaim_account_get_username(ac),
					   gaim_account_get_password(account), &info, key);
	} else {
#if 0
		struct client_info_s info = {"gaim", 4, 1, 2010, "us", "en", 0x0004, 0x0000, 0x04b};
#endif
		struct client_info_s info = CLIENTINFO_AIM_KNOWNGOOD;
		aim_send_login(sess, fr->conn, gaim_account_get_username(ac),
					   gaim_account_get_password(account), &info, key);
	}

	return 1;
}

static int conninitdone_chat(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	struct chat_connection *chatcon;
	static int id = 1;

	aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CHT, 0x0001, gaim_parse_genericerr, 0);
	aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_USERJOIN, gaim_chat_join, 0);
	aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_USERLEAVE, gaim_chat_leave, 0);
	aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_ROOMINFOUPDATE, gaim_chat_info_update, 0);
	aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_INCOMINGMSG, gaim_chat_incoming_msg, 0);

	aim_clientready(sess, fr->conn);

	chatcon = find_oscar_chat_by_conn(gc, fr->conn);
	chatcon->id = id;
	chatcon->cnv = serv_got_joined_chat(gc, id++, chatcon->show);

	return 1;
}

static int conninitdone_chatnav(aim_session_t *sess, aim_frame_t *fr, ...) {

	aim_conn_addhandler(sess, fr->conn, 0x000d, 0x0001, gaim_parse_genericerr, 0);
	aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CTN, AIM_CB_CTN_INFO, gaim_chatnav_info, 0);

	aim_clientready(sess, fr->conn);

	aim_chatnav_reqrights(sess, fr->conn);

	return 1;
}

static int conninitdone_email(aim_session_t *sess, aim_frame_t *fr, ...) {

	aim_conn_addhandler(sess, fr->conn, 0x0018, 0x0001, gaim_parse_genericerr, 0);
	aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_EML, AIM_CB_EML_MAILSTATUS, gaim_email_parseupdate, 0);

	aim_email_sendcookies(sess, fr->conn);
	aim_email_activate(sess, fr->conn);
	aim_clientready(sess, fr->conn);

	return 1;
}

static int conninitdone_icon(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = gc->proto_data;

	aim_conn_addhandler(sess, fr->conn, 0x0018, 0x0001, gaim_parse_genericerr, 0);
	aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_ICO, AIM_CB_ICO_ERROR, gaim_icon_error, 0);
	aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_ICO, AIM_CB_ICO_RESPONSE, gaim_icon_parseicon, 0);

	aim_clientready(sess, fr->conn);

	od->iconconnecting = FALSE;

	if (od->icontimer)
		g_source_remove(od->icontimer);
	od->icontimer = g_timeout_add(100, gaim_icon_timerfunc, gc);

	return 1;
}

static void oscar_chatnav_connect(gpointer data, gint source, GaimInputCondition cond) {
	GaimConnection *gc = data;
	struct oscar_data *od;
	aim_session_t *sess;
	aim_conn_t *tstconn;

	if (!g_list_find(gaim_connections_get_all(), gc)) {
		close(source);
		return;
	}

	od = gc->proto_data;
	sess = od->sess;
	tstconn = aim_getconn_type_all(sess, AIM_CONN_TYPE_CHATNAV);
	tstconn->fd = source;

	if (source < 0) {
		aim_conn_kill(sess, &tstconn);
		gaim_debug(GAIM_DEBUG_ERROR, "oscar",
				   "unable to connect to chatnav server\n");
		return;
	}

	aim_conn_completeconnect(sess, tstconn);
	od->cnpa = gaim_input_add(tstconn->fd, GAIM_INPUT_READ, oscar_callback, tstconn);
	gaim_debug(GAIM_DEBUG_INFO, "oscar", "chatnav: connected\n");
}

static void oscar_auth_connect(gpointer data, gint source, GaimInputCondition cond)
{
	GaimConnection *gc = data;
	struct oscar_data *od;
	aim_session_t *sess;
	aim_conn_t *tstconn;

	if (!g_list_find(gaim_connections_get_all(), gc)) {
		close(source);
		return;
	}

	od = gc->proto_data;
	sess = od->sess;
	tstconn = aim_getconn_type_all(sess, AIM_CONN_TYPE_AUTH);
	tstconn->fd = source;

	if (source < 0) {
		aim_conn_kill(sess, &tstconn);
		gaim_debug(GAIM_DEBUG_ERROR, "oscar",
				   "unable to connect to authorizer\n");
		return;
	}

	aim_conn_completeconnect(sess, tstconn);
	od->paspa = gaim_input_add(tstconn->fd, GAIM_INPUT_READ, oscar_callback, tstconn);
	gaim_debug(GAIM_DEBUG_INFO, "oscar", "admin: connected\n");
}

static void oscar_chat_connect(gpointer data, gint source, GaimInputCondition cond)
{
	struct chat_connection *ccon = data;
	GaimConnection *gc = ccon->gc;
	struct oscar_data *od;
	aim_session_t *sess;
	aim_conn_t *tstconn;

	if (!g_list_find(gaim_connections_get_all(), gc)) {
		close(source);
		g_free(ccon->show);
		g_free(ccon->name);
		g_free(ccon);
		return;
	}

	od = gc->proto_data;
	sess = od->sess;
	tstconn = ccon->conn;
	tstconn->fd = source;

	if (source < 0) {
		aim_conn_kill(sess, &tstconn);
		g_free(ccon->show);
		g_free(ccon->name);
		g_free(ccon);
		return;
	}

	aim_conn_completeconnect(sess, ccon->conn);
	ccon->inpa = gaim_input_add(tstconn->fd, GAIM_INPUT_READ, oscar_callback, tstconn);
	od->oscar_chats = g_slist_append(od->oscar_chats, ccon);
}

static void oscar_email_connect(gpointer data, gint source, GaimInputCondition cond) {
	GaimConnection *gc = data;
	struct oscar_data *od;
	aim_session_t *sess;
	aim_conn_t *tstconn;

	if (!g_list_find(gaim_connections_get_all(), gc)) {
		close(source);
		return;
	}

	od = gc->proto_data;
	sess = od->sess;
	tstconn = aim_getconn_type_all(sess, AIM_CONN_TYPE_EMAIL);
	tstconn->fd = source;

	if (source < 0) {
		aim_conn_kill(sess, &tstconn);
		gaim_debug(GAIM_DEBUG_ERROR, "oscar",
				   "unable to connect to email server\n");
		return;
	}

	aim_conn_completeconnect(sess, tstconn);
	od->emlpa = gaim_input_add(tstconn->fd, GAIM_INPUT_READ, oscar_callback, tstconn);
	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "email: connected\n");
}

static void oscar_icon_connect(gpointer data, gint source, GaimInputCondition cond) {
	GaimConnection *gc = data;
	struct oscar_data *od;
	aim_session_t *sess;
	aim_conn_t *tstconn;

	if (!g_list_find(gaim_connections_get_all(), gc)) {
		close(source);
		return;
	}

	od = gc->proto_data;
	sess = od->sess;
	tstconn = aim_getconn_type_all(sess, AIM_CONN_TYPE_ICON);
	tstconn->fd = source;

	if (source < 0) {
		aim_conn_kill(sess, &tstconn);
		gaim_debug(GAIM_DEBUG_ERROR, "oscar",
				   "unable to connect to icon server\n");
		return;
	}

	aim_conn_completeconnect(sess, tstconn);
	od->icopa = gaim_input_add(tstconn->fd, GAIM_INPUT_READ, oscar_callback, tstconn);
	gaim_debug(GAIM_DEBUG_INFO, "oscar", "icon: connected\n");
}

/* Hrmph. I don't know how to make this look better. --mid */
static int gaim_handle_redirect(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	GaimAccount *account = gaim_connection_get_account(gc);
	aim_conn_t *tstconn;
	int i;
	char *host;
	int port;
	va_list ap;
	struct aim_redirect_data *redir;

	port = gaim_account_get_int(account, "port", FAIM_LOGIN_PORT);

	va_start(ap, fr);
	redir = va_arg(ap, struct aim_redirect_data *);
	va_end(ap);

	for (i = 0; i < (int)strlen(redir->ip); i++) {
		if (redir->ip[i] == ':') {
			port = atoi(&(redir->ip[i+1]));
			break;
		}
	}
	host = g_strndup(redir->ip, i);

	switch(redir->group) {
	case 0x7: /* Authorizer */
		gaim_debug(GAIM_DEBUG_INFO, "oscar",
				   "Reconnecting with authorizor...\n");
		tstconn = aim_newconn(sess, AIM_CONN_TYPE_AUTH, NULL);
		if (tstconn == NULL) {
			gaim_debug(GAIM_DEBUG_ERROR, "oscar",
					   "unable to reconnect with authorizer\n");
			g_free(host);
			return 1;
		}
		aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNERR, gaim_connerr, 0);
		aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, conninitdone_admin, 0);

		tstconn->status |= AIM_CONN_STATUS_INPROGRESS;
		if (gaim_proxy_connect(account, host, port, oscar_auth_connect, gc) != 0) {
			aim_conn_kill(sess, &tstconn);
			gaim_debug(GAIM_DEBUG_ERROR, "oscar",
					   "unable to reconnect with authorizer\n");
			g_free(host);
			return 1;
		}
		aim_sendcookie(sess, tstconn, redir->cookielen, redir->cookie);
	break;

	case 0xd: /* ChatNav */
		tstconn = aim_newconn(sess, AIM_CONN_TYPE_CHATNAV, NULL);
		if (tstconn == NULL) {
			gaim_debug(GAIM_DEBUG_ERROR, "oscar",
					   "unable to connect to chatnav server\n");
			g_free(host);
			return 1;
		}
		aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNERR, gaim_connerr, 0);
		aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, conninitdone_chatnav, 0);

		tstconn->status |= AIM_CONN_STATUS_INPROGRESS;
		if (gaim_proxy_connect(account, host, port, oscar_chatnav_connect, gc) != 0) {
			aim_conn_kill(sess, &tstconn);
			gaim_debug(GAIM_DEBUG_ERROR, "oscar",
					   "unable to connect to chatnav server\n");
			g_free(host);
			return 1;
		}
		aim_sendcookie(sess, tstconn, redir->cookielen, redir->cookie);
	break;

	case 0xe: { /* Chat */
		struct chat_connection *ccon;

		tstconn = aim_newconn(sess, AIM_CONN_TYPE_CHAT, NULL);
		if (tstconn == NULL) {
			gaim_debug(GAIM_DEBUG_ERROR, "oscar",
					   "unable to connect to chat server\n");
			g_free(host);
			return 1;
		}

		aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNERR, gaim_connerr, 0);
		aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, conninitdone_chat, 0);

		ccon = g_new0(struct chat_connection, 1);
		ccon->conn = tstconn;
		ccon->gc = gc;
		ccon->fd = -1;
		ccon->name = g_strdup(redir->chat.room);
		ccon->exchange = redir->chat.exchange;
		ccon->instance = redir->chat.instance;
		ccon->show = extract_name(redir->chat.room);

		ccon->conn->status |= AIM_CONN_STATUS_INPROGRESS;
		if (gaim_proxy_connect(account, host, port, oscar_chat_connect, ccon) != 0) {
			aim_conn_kill(sess, &tstconn);
			gaim_debug(GAIM_DEBUG_ERROR, "oscar",
					   "unable to connect to chat server\n");
			g_free(host);
			g_free(ccon->show);
			g_free(ccon->name);
			g_free(ccon);
			return 1;
		}
		aim_sendcookie(sess, tstconn, redir->cookielen, redir->cookie);
		gaim_debug(GAIM_DEBUG_INFO, "oscar",
				   "Connected to chat room %s exchange %hu\n",
				   ccon->name, ccon->exchange);
	} break;

	case 0x0010: { /* icon */
		if (!(tstconn = aim_newconn(sess, AIM_CONN_TYPE_ICON, NULL))) {
			gaim_debug(GAIM_DEBUG_ERROR, "oscar",
					   "unable to connect to icon server\n");
			g_free(host);
			return 1;
		}
		aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNERR, gaim_connerr, 0);
		aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, conninitdone_icon, 0);

		tstconn->status |= AIM_CONN_STATUS_INPROGRESS;
		if (gaim_proxy_connect(account, host, port, oscar_icon_connect, gc) != 0) {
			aim_conn_kill(sess, &tstconn);
			gaim_debug(GAIM_DEBUG_ERROR, "oscar",
					   "unable to connect to icon server\n");
			g_free(host);
			return 1;
		}
		aim_sendcookie(sess, tstconn, redir->cookielen, redir->cookie);
	} break;

	case 0x0018: { /* email */
		if (!(tstconn = aim_newconn(sess, AIM_CONN_TYPE_EMAIL, NULL))) {
			gaim_debug(GAIM_DEBUG_ERROR, "oscar",
					   "unable to connect to email server\n");
			g_free(host);
			return 1;
		}
		aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNERR, gaim_connerr, 0);
		aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, conninitdone_email, 0);

		tstconn->status |= AIM_CONN_STATUS_INPROGRESS;
		if (gaim_proxy_connect(account, host, port, oscar_email_connect, gc) != 0) {
			aim_conn_kill(sess, &tstconn);
			gaim_debug(GAIM_DEBUG_ERROR, "oscar",
					   "unable to connect to email server\n");
			g_free(host);
			return 1;
		}
		aim_sendcookie(sess, tstconn, redir->cookielen, redir->cookie);
	} break;

	default: /* huh? */
		gaim_debug(GAIM_DEBUG_WARNING, "oscar",
				   "got redirect for unknown service 0x%04hx\n",
				   redir->group);
		break;
	}

	g_free(host);
	return 1;
}

static int gaim_parse_oncoming(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = gc->proto_data;
	struct buddyinfo *bi;
	time_t time_idle = 0, signon = 0;
	int type = 0;
	int caps = 0;
	va_list ap;
	aim_userinfo_t *info;

	va_start(ap, fr);
	info = va_arg(ap, aim_userinfo_t *);
	va_end(ap);

	if (info->present & AIM_USERINFO_PRESENT_CAPABILITIES)
		caps = info->capabilities;
	if (info->flags & AIM_FLAG_ACTIVEBUDDY)
		type |= UC_AB;
	if (caps & AIM_CAPS_HIPTOP)
		type |= UC_HIPTOP;

	if (info->present & AIM_USERINFO_PRESENT_FLAGS) {
		if (info->flags & AIM_FLAG_UNCONFIRMED)
			type |= UC_UNCONFIRMED;
		if (info->flags & AIM_FLAG_ADMINISTRATOR)
			type |= UC_ADMIN;
		if (info->flags & AIM_FLAG_AOL)
			type |= UC_AOL;
		if (info->flags & AIM_FLAG_FREE)
			type |= UC_NORMAL;
		if (info->flags & AIM_FLAG_AWAY)
			type |= UC_UNAVAILABLE;
		if (info->flags & AIM_FLAG_WIRELESS)
			type |= UC_WIRELESS;
	}
	if (info->present & AIM_USERINFO_PRESENT_ICQEXTSTATUS) {
		type = (info->icqinfo.status << 16);
		if (!(info->icqinfo.status & AIM_ICQ_STATE_CHAT) &&
		      (info->icqinfo.status != AIM_ICQ_STATE_NORMAL)) {
			type |= UC_UNAVAILABLE;
		}
	}

	if (caps & AIM_CAPS_ICQ)
		caps ^= AIM_CAPS_ICQ;

	if (info->present & AIM_USERINFO_PRESENT_IDLE) {
		time(&time_idle);
		time_idle -= info->idletime*60;
	}

	if (info->present & AIM_USERINFO_PRESENT_ONLINESINCE)
		signon = info->onlinesince;
	else if (info->present & AIM_USERINFO_PRESENT_SESSIONLEN)
		signon = time(NULL) - info->sessionlen;

	if (!aim_sncmp(gaim_account_get_username(gaim_connection_get_account(gc)), info->sn))
		gaim_connection_set_display_name(gc, info->sn);

	bi = g_hash_table_lookup(od->buddyinfo, normalize(info->sn));
	if (!bi) {
		bi = g_new0(struct buddyinfo, 1);
		g_hash_table_insert(od->buddyinfo, g_strdup(normalize(info->sn)), bi);
	}
	bi->signon = info->onlinesince ? info->onlinesince : (info->sessionlen + time(NULL));
	if (caps)
		bi->caps = caps;
	bi->typingnot = FALSE;
	bi->ico_informed = FALSE;
	bi->ipaddr = info->icqinfo.ipaddr;

	/* Available message stuff */
	free(bi->availmsg);
	if (info->availmsg)
		if (info->availmsg_encoding) {
			gchar *enc = g_strdup_printf("charset=\"%s\"", info->availmsg_encoding);
			bi->availmsg = oscar_encoding_to_utf8(enc, info->availmsg, info->availmsg_len);
			g_free(enc);
		} else {
			/* No explicit encoding means utf8.  Yay. */
			bi->availmsg = g_strdup(info->availmsg);
		}
	else
		bi->availmsg = NULL;

	/* Server stored icon stuff */
	if (info->iconcsumlen) {
		char *b16, *saved_b16;
		GaimBuddy *b;

		free(bi->iconcsum);
		bi->iconcsum = malloc(info->iconcsumlen);
		memcpy(bi->iconcsum, info->iconcsum, info->iconcsumlen);
		bi->iconcsumlen = info->iconcsumlen;
		b16 = tobase16(bi->iconcsum, bi->iconcsumlen);
		b = gaim_find_buddy(gc->account, info->sn);
		saved_b16 = gaim_buddy_get_setting(b, "icon_checksum");
		if (!b16 || !saved_b16 || strcmp(b16, saved_b16)) {
			GSList *cur = od->requesticon;
			while (cur && aim_sncmp((char *)cur->data, info->sn))
				cur = cur->next;
			if (!cur) {
				od->requesticon = g_slist_append(od->requesticon, strdup(normalize(info->sn)));
				if (od->icontimer)
					g_source_remove(od->icontimer);
				od->icontimer = g_timeout_add(500, gaim_icon_timerfunc, gc);
			}
		}
		g_free(saved_b16);
		g_free(b16);
	}

	serv_got_update(gc, info->sn, 1, (info->warnlevel/10.0) + 0.5, signon, time_idle, type);

	return 1;
}

static int gaim_parse_offgoing(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = gc->proto_data;
	va_list ap;
	aim_userinfo_t *info;

	va_start(ap, fr);
	info = va_arg(ap, aim_userinfo_t *);
	va_end(ap);

	serv_got_update(gc, info->sn, 0, 0, 0, 0, 0);

	g_hash_table_remove(od->buddyinfo, normalize(info->sn));

	return 1;
}

static void cancel_direct_im(struct ask_direct *d) {
	gaim_debug(GAIM_DEBUG_INFO, "oscar", "Freeing DirectIM prompts.\n");

	g_free(d->sn);
	g_free(d);
}

static void oscar_odc_callback(gpointer data, gint source, GaimInputCondition condition) {
	struct direct_im *dim = data;
	GaimConnection *gc = dim->gc;
	struct oscar_data *od = gc->proto_data;
	GaimConversation *cnv;
	char buf[256];
	struct sockaddr name;
	socklen_t name_len = 1;
	
	if (!g_list_find(gaim_connections_get_all(), gc)) {
		g_free(dim);
		return;
	}

	if (source < 0) {
		g_free(dim);
		return;
	}

	dim->conn->fd = source;
	aim_conn_completeconnect(od->sess, dim->conn);
	cnv = gaim_conversation_new(GAIM_CONV_IM, dim->gc->account, dim->name);

	/* This is the best way to see if we're connected or not */
	if (getpeername(source, &name, &name_len) == 0) {
		g_snprintf(buf, sizeof buf, _("Direct IM with %s established"), dim->name);
		dim->connected = TRUE;
		gaim_conversation_write(cnv, NULL, buf, GAIM_MESSAGE_SYSTEM, time(NULL));
	}
	od->direct_ims = g_slist_append(od->direct_ims, dim);
	
	dim->watcher = gaim_input_add(dim->conn->fd, GAIM_INPUT_READ, oscar_callback, dim->conn);
}

/* BBB */
/*
 * This is called after a remote AIM user has connected to us.  We 
 * want to do some voodoo with the socket file descriptors, add a 
 * callback or two, and then send the AIM_CB_OFT_PROMPT.
 */
static int oscar_sendfile_estblsh(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
	GaimXfer *xfer;
	struct aim_oft_info *oft_info;
	va_list ap;
	aim_conn_t *conn, *listenerconn;

	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "AAA - in oscar_sendfile_estblsh\n");
	va_start(ap, fr);
	conn = va_arg(ap, aim_conn_t *);
	listenerconn = va_arg(ap, aim_conn_t *);
	va_end(ap);

	if (!(xfer = oscar_find_xfer_by_conn(od->file_transfers, listenerconn)))
		return 1;

	if (!(oft_info = xfer->data))
		return 1;

	/* Stop watching listener conn; watch transfer conn instead */
	gaim_input_remove(xfer->watcher);
	aim_conn_kill(sess, &listenerconn);

	oft_info->conn = conn;
	xfer->fd = oft_info->conn->fd;

	aim_conn_addhandler(sess, oft_info->conn, AIM_CB_FAM_OFT, AIM_CB_OFT_ACK, oscar_sendfile_ack, 0);
	aim_conn_addhandler(sess, oft_info->conn, AIM_CB_FAM_OFT, AIM_CB_OFT_DONE, oscar_sendfile_done, 0);
	xfer->watcher = gaim_input_add(oft_info->conn->fd, GAIM_INPUT_READ, oscar_callback, oft_info->conn);

	/* Inform the other user that we are connected and ready to transfer */
	aim_oft_sendheader(sess, AIM_CB_OFT_PROMPT, oft_info);

	return 0;
}

/*
 * This is the gaim callback passed to gaim_proxy_connect when connecting to another AIM 
 * user in order to transfer a file.
 */
static void oscar_sendfile_connected(gpointer data, gint source, GaimInputCondition condition) {
	GaimXfer *xfer;
	struct aim_oft_info *oft_info;

	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "AAA - in oscar_sendfile_connected\n");
	if (!(xfer = data))
		return;
	if (!(oft_info = xfer->data))
		return;
	if (source < 0)
		return;

	xfer->fd = source;
	oft_info->conn->fd = source;

	aim_conn_completeconnect(oft_info->sess, oft_info->conn);
	xfer->watcher = gaim_input_add(xfer->fd, GAIM_INPUT_READ, oscar_callback, oft_info->conn);

	/* Inform the other user that we are connected and ready to transfer */
	aim_im_sendch2_sendfile_accept(oft_info->sess, oft_info);

	return;
}

/*
 * This is called when a buddy sends us some file info.  This happens when they 
 * are sending a file to you, and you have just established a connection to them.
 * You should send them the exact same info except use the real cookie.  We also 
 * get like totally ready to like, receive the file, kay?
 */
static int oscar_sendfile_prompt(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = gc->proto_data;
	GaimXfer *xfer;
	struct aim_oft_info *oft_info;
	va_list ap;
	aim_conn_t *conn;
	fu8_t *cookie;
	struct aim_fileheader_t *fh;

	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "AAA - in oscar_sendfile_prompt\n");
	va_start(ap, fr);
	conn = va_arg(ap, aim_conn_t *);
	cookie = va_arg(ap, fu8_t *);
	fh = va_arg(ap, struct aim_fileheader_t *);
	va_end(ap);

	if (!(xfer = oscar_find_xfer_by_conn(od->file_transfers, conn)))
		return 1;

	if (!(oft_info = xfer->data))
		return 1;

	/* We want to stop listening with a normal thingy */
	gaim_input_remove(xfer->watcher);
	xfer->watcher = 0;

	/* They sent us some information about the file they're sending */
	memcpy(&oft_info->fh, fh, sizeof(*fh));

	/* Fill in the cookie */
	memcpy(&oft_info->fh.bcookie, oft_info->cookie, 8);

	/* XXX - convert the name from UTF-8 to UCS-2 if necessary, and pass the encoding to the call below */
	aim_oft_sendheader(oft_info->sess, AIM_CB_OFT_ACK, oft_info);
	gaim_xfer_start(xfer, xfer->fd, NULL, 0);

	return 0;
}

/*
 * We are sending a file to someone else.  They have just acknowledged our 
 * prompt, so we want to start sending data like there's no tomorrow.
 */
static int oscar_sendfile_ack(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = gc->proto_data;
	GaimXfer *xfer;
	va_list ap;
	aim_conn_t *conn;
	fu8_t *cookie;
	struct aim_fileheader_t *fh;

	gaim_debug(GAIM_DEBUG_INFO, "oscar", "AAA - in oscar_sendfile_ack\n");
	va_start(ap, fr);
	conn = va_arg(ap, aim_conn_t *);
	cookie = va_arg(ap, fu8_t *);
	fh = va_arg(ap, struct aim_fileheader_t *);
	va_end(ap);

	if (!(xfer = oscar_find_xfer_by_cookie(od->file_transfers, cookie)))
		return 1;

	/* We want to stop listening with a normal thingy */
	gaim_input_remove(xfer->watcher);
	xfer->watcher = 0;

	gaim_xfer_start(xfer, xfer->fd, NULL, 0);

	return 0;
}

/*
 * We just sent a file to someone.  They said they got it and everything, 
 * so we can close our direct connection and what not.
 */
static int oscar_sendfile_done(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = gc->proto_data;
	GaimXfer *xfer;
	va_list ap;
	aim_conn_t *conn;
	fu8_t *cookie;
	struct aim_fileheader_t *fh;

	gaim_debug(GAIM_DEBUG_INFO, "oscar", "AAA - in oscar_sendfile_done\n");
	va_start(ap, fr);
	conn = va_arg(ap, aim_conn_t *);
	cookie = va_arg(ap, fu8_t *);
	fh = va_arg(ap, struct aim_fileheader_t *);
	va_end(ap);

	if (!(xfer = oscar_find_xfer_by_conn(od->file_transfers, conn)))
		return 1;

	xfer->fd = conn->fd;
	gaim_xfer_end(xfer);

	return 0;
}

static void accept_direct_im(struct ask_direct *d) {
	GaimConnection *gc = d->gc;
	struct oscar_data *od;
	struct direct_im *dim;
	char *host; int port = 4443;
	int i, rc;

	if (!g_list_find(gaim_connections_get_all(), gc)) {
		cancel_direct_im(d);
		return;
	}

	od = (struct oscar_data *)gc->proto_data;
	gaim_debug(GAIM_DEBUG_INFO, "oscar", "Accepted DirectIM.\n");

	dim = find_direct_im(od, d->sn);
	if (dim) {
		cancel_direct_im(d); /* 40 */
		return;
	}
	dim = g_new0(struct direct_im, 1);
	dim->gc = d->gc;
	g_snprintf(dim->name, sizeof dim->name, "%s", d->sn);

	dim->conn = aim_odc_connect(od->sess, d->sn, NULL, d->cookie);
	if (!dim->conn) {
		g_free(dim);
		cancel_direct_im(d);
		return;
	}

	aim_conn_addhandler(od->sess, dim->conn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMINCOMING,
				gaim_odc_incoming, 0);
	aim_conn_addhandler(od->sess, dim->conn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMTYPING,
				gaim_odc_typing, 0);
	aim_conn_addhandler(od->sess, dim->conn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_IMAGETRANSFER,
			        gaim_odc_update_ui, 0);
	for (i = 0; i < (int)strlen(d->ip); i++) {
		if (d->ip[i] == ':') {
			port = atoi(&(d->ip[i+1]));
			break;
		}
	}
	host = g_strndup(d->ip, i);
	dim->conn->status |= AIM_CONN_STATUS_INPROGRESS;
	rc = gaim_proxy_connect(gc->account, host, port, oscar_odc_callback, dim);
	g_free(host);
	if (rc < 0) {
		aim_conn_kill(od->sess, &dim->conn);
		g_free(dim);
		cancel_direct_im(d);
		return;
	}

	cancel_direct_im(d);

	return;
}

static int incomingim_chan1(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *userinfo, struct aim_incomingim_ch1_args *args) {
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = gc->proto_data;
	char *tmp;
	GaimImFlags flags = 0;
	gsize convlen;
	GError *err = NULL;
	struct buddyinfo *bi;
	const char *iconfile;

	bi = g_hash_table_lookup(od->buddyinfo, normalize(userinfo->sn));
	if (!bi) {
		bi = g_new0(struct buddyinfo, 1);
		g_hash_table_insert(od->buddyinfo, g_strdup(normalize(userinfo->sn)), bi);
	}

	if (args->icbmflags & AIM_IMFLAGS_AWAY)
		flags |= GAIM_IM_AUTO_RESP;

	if (args->icbmflags & AIM_IMFLAGS_TYPINGNOT)
		bi->typingnot = TRUE;
	else
		bi->typingnot = FALSE;

	if ((args->icbmflags & AIM_IMFLAGS_HASICON) && (args->iconlen) && (args->iconsum) && (args->iconstamp)) {
		gaim_debug(GAIM_DEBUG_MISC, "oscar",
				   "%s has an icon\n", userinfo->sn);
		if ((args->iconlen != bi->ico_len) || (args->iconsum != bi->ico_csum) || (args->iconstamp != bi->ico_time)) {
			bi->ico_need = TRUE;
			bi->ico_len = args->iconlen;
			bi->ico_csum = args->iconsum;
			bi->ico_time = args->iconstamp;
		}
	}

	if ((iconfile = gaim_account_get_buddy_icon(gaim_connection_get_account(gc))) && 
	    (args->icbmflags & AIM_IMFLAGS_BUDDYREQ)) {
		FILE *file;
		struct stat st;

		if (!stat(iconfile, &st)) {
			char *buf = g_malloc(st.st_size);
			file = fopen(iconfile, "rb");
			if (file) {
				int len = fread(buf, 1, st.st_size, file);
				gaim_debug(GAIM_DEBUG_INFO, "oscar",
						   "Sending buddy icon to %s (%d bytes, "
						   "%lu reported)\n",
						   userinfo->sn, len, st.st_size);
				aim_im_sendch2_icon(sess, userinfo->sn, buf, st.st_size,
					st.st_mtime, aimutil_iconsum(buf, st.st_size));
				fclose(file);
			} else
				gaim_debug(GAIM_DEBUG_ERROR, "oscar",
						   "Can't open buddy icon file!\n");
			g_free(buf);
		} else
			gaim_debug(GAIM_DEBUG_ERROR, "oscar",
					   "Can't stat buddy icon file!\n");
	}

	gaim_debug(GAIM_DEBUG_MISC, "oscar",
			   "Character set is %hu %hu\n", args->charset, args->charsubset);
	if (args->icbmflags & AIM_IMFLAGS_UNICODE) {
		/* This message is marked as UNICODE, so we have to
		 * convert it to utf-8 before handing it to the gaim core.
		 * This conversion should *never* fail, if it does it
		 * means that either the incoming ICBM is corrupted or
		 * there is something we don't understand about it.
		 * For the record, AIM Unicode is big-endian UCS-2 */

		gaim_debug(GAIM_DEBUG_INFO, "oscar", "Received UNICODE IM\n");

		if (!args->msg || !args->msglen)
			return 1;

		tmp = g_convert(args->msg, args->msglen, "UTF-8", "UCS-2BE", NULL, &convlen, &err);
		if (err) {
			gaim_debug(GAIM_DEBUG_INFO, "oscar",
					   "Unicode IM conversion: %s\n", err->message);
			tmp = strdup(_("(There was an error receiving this message)"));
			g_error_free(err);
		}
	} else {
		/* This will get executed for both AIM_IMFLAGS_ISO_8859_1 and
		 * unflagged messages, which are ASCII.  That's OK because
		 * ASCII is a strict subset of ISO-8859-1; this should
		 * help with compatibility with old, broken versions of
		 * gaim (everything before 0.60) and other broken clients
		 * that will happily send ISO-8859-1 without marking it as
		 * such */
		if (args->icbmflags & AIM_IMFLAGS_ISO_8859_1)
			gaim_debug(GAIM_DEBUG_INFO, "oscar",
					   "Received ISO-8859-1 IM\n");

		if (!args->msg || !args->msglen)
			return 1;

		tmp = g_convert(args->msg, args->msglen, "UTF-8", "ISO-8859-1", NULL, &convlen, &err);
		if (err) {
			gaim_debug(GAIM_DEBUG_INFO, "oscar",
					   "ISO-8859-1 IM conversion: %s\n", err->message);
			tmp = strdup(_("(There was an error receiving this message)"));
			g_error_free(err);
		}
	}

	/* strip_linefeed(tmp); */
	serv_got_im(gc, userinfo->sn, tmp, flags, time(NULL));
	g_free(tmp);

	return 1;
}

static int incomingim_chan2(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *userinfo, struct aim_incomingim_ch2_args *args) {
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = gc->proto_data;
	const char *username = gaim_account_get_username(gaim_connection_get_account(gc));

	if (!args)
		return 0;

	gaim_debug(GAIM_DEBUG_MISC, "oscar",
			   "rendezvous with %s, status is %hu\n",
			   userinfo->sn, args->status);

	if (args->reqclass & AIM_CAPS_CHAT) {
		char *name;
		GHashTable *components;

		if (!args->info.chat.roominfo.name || !args->info.chat.roominfo.exchange || !args->msg)
			return 1;
		components = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
				g_free);
		name = extract_name(args->info.chat.roominfo.name);
		g_hash_table_replace(components, g_strdup("room"), g_strdup(name ? name : args->info.chat.roominfo.name));
		g_hash_table_replace(components, g_strdup("exchange"), g_strdup_printf("%d", args->info.chat.roominfo.exchange));
		serv_got_chat_invite(gc,
				     name ? name : args->info.chat.roominfo.name,
				     userinfo->sn,
				     args->msg,
				     components);
		if (name)
			g_free(name);
	} else if (args->reqclass & AIM_CAPS_SENDFILE) {
/* BBB */
		if (args->status == AIM_RENDEZVOUS_PROPOSE) {
			/* Someone wants to send a file (or files) to us */
			GaimXfer *xfer;
			struct aim_oft_info *oft_info;

			if (!args->cookie || !args->port || !args->verifiedip || 
			    !args->info.sendfile.filename || !args->info.sendfile.totsize || 
			    !args->info.sendfile.totfiles || !args->reqclass) {
				gaim_debug(GAIM_DEBUG_WARNING, "oscar",
						   "%s tried to send you a file with incomplete "
						   "information.\n", userinfo->sn);
				if (args->proxyip)
					gaim_debug(GAIM_DEBUG_WARNING, "oscar",
							   "IP for a proxy server was given.  Gaim "
							   "does not support this yet.\n");
				return 1;
			}

			if (args->info.sendfile.subtype == AIM_OFT_SUBTYPE_SEND_DIR) {
				/* last char of the ft req is a star, they are sending us a
				 * directory -- remove the star and trailing slash so we dont save
				 * directories that look like 'dirname\*'  -- arl */
				char *tmp = strrchr(args->info.sendfile.filename, '\\');
				if (tmp && (tmp[1] == '*')) {
					tmp[0] = '\0';
				}
			}

			/* Build the file transfer handle */
			xfer = gaim_xfer_new(gc->account, GAIM_XFER_RECEIVE, userinfo->sn);
			xfer->remote_ip = g_strdup(args->verifiedip);
			xfer->remote_port = args->port;
			gaim_xfer_set_filename(xfer, args->info.sendfile.filename);
			gaim_xfer_set_size(xfer, args->info.sendfile.totsize);

			/* Create the oscar-specific data */
			oft_info = aim_oft_createinfo(od->sess, args->cookie, userinfo->sn, args->clientip, xfer->remote_port, 0, 0, NULL);
			if (args->proxyip)
				oft_info->proxyip = g_strdup(args->proxyip);
			if (args->verifiedip)
				oft_info->verifiedip = g_strdup(args->verifiedip);
			xfer->data = oft_info;

			 /* Setup our I/O op functions */
			gaim_xfer_set_init_fnc(xfer, oscar_xfer_init);
			gaim_xfer_set_start_fnc(xfer, oscar_xfer_start);
			gaim_xfer_set_end_fnc(xfer, oscar_xfer_end);
			gaim_xfer_set_cancel_send_fnc(xfer, oscar_xfer_cancel_send);
			gaim_xfer_set_cancel_recv_fnc(xfer, oscar_xfer_cancel_recv);
			gaim_xfer_set_ack_fnc(xfer, oscar_xfer_ack);

			/*
			 * XXX - Should do something with args->msg, args->encoding, and args->language
			 *       probably make it apply to all ch2 messages.
			 */

			/* Keep track of this transfer for later */
			od->file_transfers = g_slist_append(od->file_transfers, xfer);

			/* Now perform the request */
			gaim_xfer_request(xfer);
		} else if (args->status == AIM_RENDEZVOUS_CANCEL) {
			/* The other user wants to cancel a file transfer */
			GaimXfer *xfer;
			gaim_debug(GAIM_DEBUG_INFO, "oscar",
					   "AAA - File transfer canceled by remote user\n");
			if ((xfer = oscar_find_xfer_by_cookie(od->file_transfers, args->cookie)))
				gaim_xfer_cancel_remote(xfer);
		} else if (args->status == AIM_RENDEZVOUS_ACCEPT) {
			/*
			 * This gets sent by the receiver of a file 
			 * as they connect directly to us.  If we don't 
			 * get this, then maybe a third party connected 
			 * to us, and we shouldn't send them anything.
			 */
		} else {
			gaim_debug(GAIM_DEBUG_ERROR, "oscar",
					   "unknown rendezvous status!\n");
		}
	} else if (args->reqclass & AIM_CAPS_GETFILE) {
	} else if (args->reqclass & AIM_CAPS_VOICE) {
	} else if (args->reqclass & AIM_CAPS_BUDDYICON) {
		gaim_buddy_icons_set_for_user(gaim_connection_get_account(gc),
									  userinfo->sn, args->info.icon.icon,
									  args->info.icon.length);
	} else if (args->reqclass & AIM_CAPS_DIRECTIM) {
		struct ask_direct *d = g_new0(struct ask_direct, 1);
		char buf[256];

		if (!args->verifiedip) {
			gaim_debug(GAIM_DEBUG_INFO, "oscar",
					   "directim kill blocked (%s)\n", userinfo->sn);
			return 1;
		}

		gaim_debug(GAIM_DEBUG_INFO, "oscar",
				   "%s received direct im request from %s (%s)\n",
				   username, userinfo->sn, args->verifiedip);

		d->gc = gc;
		d->sn = g_strdup(userinfo->sn);
		strncpy(d->ip, args->verifiedip, sizeof(d->ip));
		memcpy(d->cookie, args->cookie, 8);
		g_snprintf(buf, sizeof buf, _("%s has just asked to directly connect to %s"), userinfo->sn, username);

		gaim_request_action(gc, NULL, buf,
							_("This requires a direct connection between "
							  "the two computers and is necessary for IM "
							  "Images.  Because your IP address will be "
							  "revealed, this may be considered a privacy "
							  "risk."), 0, d, 2,
							_("Connect"), G_CALLBACK(accept_direct_im),
							_("Cancel"), G_CALLBACK(cancel_direct_im));
	} else {
		gaim_debug(GAIM_DEBUG_ERROR, "oscar",
				   "Unknown reqclass %hu\n", args->reqclass);
	}

	return 1;
}

/*
 * Authorization Functions
 * Most of these are callbacks from dialogs.  They're used by both 
 * methods of authorization (SSI and old-school channel 4 ICBM)
 */
/* When you ask other people for authorization */
static void gaim_auth_request(struct name_data *data, char *msg) {
	GaimConnection *gc = data->gc;

	if (g_list_find(gaim_connections_get_all(), gc)) {
		struct oscar_data *od = gc->proto_data;
		GaimBuddy *buddy = gaim_find_buddy(gc->account, data->name);
		GaimGroup *group = gaim_find_buddys_group(buddy);
		if (buddy && group) {
			gaim_debug(GAIM_DEBUG_INFO, "oscar",
					   "ssi: adding buddy %s to group %s\n",
					   buddy->name, group->name);
			aim_ssi_sendauthrequest(od->sess, data->name, msg ? msg : _("Please authorize me so I can add you to my buddy list."));
			if (!aim_ssi_itemlist_finditem(od->sess->ssi.local, group->name, buddy->name, AIM_SSI_TYPE_BUDDY))
				aim_ssi_addbuddy(od->sess, buddy->name, group->name, gaim_get_buddy_alias_only(buddy), NULL, NULL, 1);
		}
	}
}

static void gaim_auth_request_msgprompt(struct name_data *data) {
	gaim_request_input(data->gc, NULL, _("Authorization Request Message:"),
					   NULL, _("Please authorize me!"), TRUE, FALSE,
					   _("OK"), G_CALLBACK(gaim_auth_request),
					   _("Cancel"), G_CALLBACK(oscar_free_name_data),
					   data);
}

static void gaim_auth_dontrequest(struct name_data *data) {
	GaimConnection *gc = data->gc;

	if (g_list_find(gaim_connections_get_all(), gc)) {
		/* struct oscar_data *od = gc->proto_data; */
		/* XXX - Take the buddy out of our buddy list */
	}

	oscar_free_name_data(data);
}

static void gaim_auth_sendrequest(GaimConnection *gc, const char *name) {
	struct name_data *data = g_new(struct name_data, 1);
	GaimBuddy *buddy;
	gchar *dialog_msg, *nombre;

	buddy = gaim_find_buddy(gc->account, name);
	if (buddy && (gaim_get_buddy_alias_only(buddy)))
		nombre = g_strdup_printf("%s (%s)", name, gaim_get_buddy_alias_only(buddy));
	else
		nombre = NULL;

	dialog_msg = g_strdup_printf(_("The user %s requires authorization before being added to a buddy list.  Do you want to send an authorization request?"), (nombre ? nombre : name));
	data->gc = gc;
	data->name = g_strdup(name);
	data->nick = NULL;

	gaim_request_action(gc, NULL, _("Request Authorization"), dialog_msg,
						0, data, 2,
						_("Request Authorization"),
						G_CALLBACK(gaim_auth_request_msgprompt),
						_("Cancel"), G_CALLBACK(gaim_auth_dontrequest));

	g_free(dialog_msg);
	g_free(nombre);
}

/* When other people ask you for authorization */
static void gaim_auth_grant(struct name_data *data) {
	GaimConnection *gc = data->gc;

	if (g_list_find(gaim_connections_get_all(), gc)) {
		struct oscar_data *od = gc->proto_data;
#ifdef NOSSI
		GaimBuddy *buddy;
		gchar message;
		message = 0;
		buddy = gaim_find_buddy(gc->account, data->name);
		aim_im_sendch4(od->sess, data->name, AIM_ICQMSG_AUTHGRANTED, &message);
		show_got_added(gc, NULL, data->name, (buddy ? gaim_get_buddy_alias_only(buddy) : NULL), NULL);
#else
		aim_ssi_sendauthreply(od->sess, data->name, 0x01, NULL);
#endif
	}

	oscar_free_name_data(data);
}

/* When other people ask you for authorization */
static void gaim_auth_dontgrant(struct name_data *data, char *msg) {
	GaimConnection *gc = data->gc;

	if (g_list_find(gaim_connections_get_all(), gc)) {
		struct oscar_data *od = gc->proto_data;
#ifdef NOSSI
		aim_im_sendch4(od->sess, data->name, AIM_ICQMSG_AUTHDENIED, msg ? msg : _("No reason given."));
#else
		aim_ssi_sendauthreply(od->sess, data->name, 0x00, msg ? msg : _("No reason given."));
#endif
	}
}

static void gaim_auth_dontgrant_msgprompt(struct name_data *data) {
	gaim_request_input(data->gc, NULL, _("Authorization Denied Message:"),
					   NULL, _("No reason given."), TRUE, FALSE,
					   _("OK"), G_CALLBACK(gaim_auth_dontgrant),
					   _("Cancel"), G_CALLBACK(oscar_free_name_data),
					   data);
}

/* When someone sends you contacts  */
static void gaim_icq_contactadd(struct name_data *data) {
	GaimConnection *gc = data->gc;

	if (g_list_find(gaim_connections_get_all(), gc)) {
		show_add_buddy(gc, data->name, NULL, data->nick);
	}

	oscar_free_name_data(data);
}

static int incomingim_chan4(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *userinfo, struct aim_incomingim_ch4_args *args, time_t t) {
	GaimConnection *gc = sess->aux_data;
	gchar **msg1, **msg2;
	GError *err = NULL;
	int i, numtoks;

	if (!args->type || !args->msg || !args->uin)
		return 1;

	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "Received a channel 4 message of type 0x%02hhx.\n", args->type);

	/* Split up the message at the delimeter character, then convert each string to UTF-8 */
	msg1 = g_strsplit(args->msg, "\376", 0);
	for (numtoks=0; msg1[numtoks]; numtoks++);
	msg2 = (gchar **)g_malloc((numtoks+1)*sizeof(gchar *));
	for (i=0; msg1[i]; i++) {
		strip_linefeed(msg1[i]);
		msg2[i] = g_convert(msg1[i], strlen(msg1[i]), "UTF-8", "ISO-8859-1", NULL, NULL, &err);
		if (err) {
			gaim_debug(GAIM_DEBUG_ERROR, "oscar",
					   "Error converting a string from ISO-8859-1 to "
					   "UTF-8 in oscar ICBM channel 4 parsing\n");
			g_error_free(err);
		}
	}
	msg2[i] = NULL;

	switch (args->type) {
		case 0x01: { /* MacICQ message or basic offline message */
			if (i >= 1) {
				gchar *uin = g_strdup_printf("%u", args->uin);
				if (t) { /* This is an offline message */
					/* I think this timestamp is in UTC, or something */
					serv_got_im(gc, uin, msg2[0], 0, t);
				} else { /* This is a message from MacICQ/Miranda */
					serv_got_im(gc, uin, msg2[0], 0, time(NULL));
				}
				g_free(uin);
			}
		} break;

		case 0x04: { /* Someone sent you a URL */
			if (i >= 2) {
				gchar *uin = g_strdup_printf("%u", args->uin);
				gchar *message = g_strdup_printf("<A HREF=\"%s\">%s</A>", msg2[1], msg2[0]);
				serv_got_im(gc, uin, message, 0, time(NULL));
				g_free(uin);
				g_free(message);
			}
		} break;

		case 0x06: { /* Someone requested authorization */
			if (i >= 6) {
				struct name_data *data = g_new(struct name_data, 1);
				gchar *dialog_msg = g_strdup_printf(_("The user %u wants to add you to their buddy list for the following reason:\n%s"), args->uin, msg2[5] ? msg2[5] : _("No reason given."));
				gaim_debug(GAIM_DEBUG_INFO, "oscar",
						   "Received an authorization request from UIN %u\n",
						   args->uin);
				data->gc = gc;
				data->name = g_strdup_printf("%u", args->uin);
				data->nick = NULL;

				gaim_request_action(gc, NULL, _("Authorization Request"),
									dialog_msg, 0, data, 2,
									_("Authorize"),
									G_CALLBACK(gaim_auth_grant),
									_("Deny"),
									G_CALLBACK(gaim_auth_dontgrant_msgprompt));
				g_free(dialog_msg);
			}
		} break;

		case 0x07: { /* Someone has denied you authorization */
			if (i >= 1) {
				gchar *dialog_msg = g_strdup_printf(_("The user %u has denied your request to add them to your contact list for the following reason:\n%s"), args->uin, msg2[0] ? msg2[0] : _("No reason given."));
				gaim_notify_info(gc, NULL, _("ICQ authorization denied."),
								 dialog_msg);
				g_free(dialog_msg);
			}
		} break;

		case 0x08: { /* Someone has granted you authorization */
			gchar *dialog_msg = g_strdup_printf(_("The user %u has granted your request to add them to your contact list."), args->uin);
			gaim_notify_info(gc, NULL, "ICQ authorization accepted.",
							 dialog_msg);
			g_free(dialog_msg);
		} break;

		case 0x09: { /* Message from the Godly ICQ server itself, I think */
			if (i >= 5) {
				gchar *dialog_msg = g_strdup_printf(_("You have received a special message\n\nFrom: %s [%s]\n%s"), msg2[0], msg2[3], msg2[5]);
				gaim_notify_info(gc, NULL, "ICQ Server Message", dialog_msg);
				g_free(dialog_msg);
			}
		} break;

		case 0x0d: { /* Someone has sent you a pager message from http://www.icq.com/your_uin */
			if (i >= 6) {
				gchar *dialog_msg = g_strdup_printf(_("You have received an ICQ page\n\nFrom: %s [%s]\n%s"), msg2[0], msg2[3], msg2[5]);
				gaim_notify_info(gc, NULL, "ICQ Page", dialog_msg);
				g_free(dialog_msg);
			}
		} break;

		case 0x0e: { /* Someone has emailed you at your_uin@pager.icq.com */
			if (i >= 6) {
				gchar *dialog_msg = g_strdup_printf(_("You have received an ICQ email from %s [%s]\n\nMessage is:\n%s"), msg2[0], msg2[3], msg2[5]);
				gaim_notify_info(gc, NULL, "ICQ Email", dialog_msg);
				g_free(dialog_msg);
			}
		} break;

		case 0x12: {
			/* Ack for authorizing/denying someone.  Or possibly an ack for sending any system notice */
			/* Someone added you to their contact list? */
		} break;

		case 0x13: { /* Someone has sent you some ICQ contacts */
			int i, num;
			gchar **text;
			text = g_strsplit(args->msg, "\376", 0);
			if (text) {
				num = 0;
				for (i=0; i<strlen(text[0]); i++)
					num = num*10 + text[0][i]-48;
				for (i=0; i<num; i++) {
					struct name_data *data = g_new(struct name_data, 1);
					gchar *message = g_strdup_printf(_("ICQ user %u has sent you a contact: %s (%s)"), args->uin, text[i*2+2], text[i*2+1]);
					data->gc = gc;
					data->name = g_strdup(text[i*2+1]);
					data->nick = g_strdup(text[i*2+2]);

					gaim_request_action(gc, NULL, message,
										_("Do you want to add this contact "
										  "to your Buddy List?"),
										0, data, 2,
										_("Add"), G_CALLBACK(gaim_icq_contactadd),
										_("Decline"), G_CALLBACK(oscar_free_name_data));
					g_free(message);
				}
				g_strfreev(text);
			}
		} break;

		case 0x1a: { /* Someone has sent you a greeting card or requested contacts? */
			/* This is boring and silly. */
		} break;

		default: {
			gaim_debug(GAIM_DEBUG_INFO, "oscar",
					   "Received a channel 4 message of unknown type "
					   "(type 0x%02hhx).\n", args->type);
		} break;
	}

	g_strfreev(msg1);
	g_strfreev(msg2);

	return 1;
}

static int gaim_parse_incoming_im(aim_session_t *sess, aim_frame_t *fr, ...) {
	fu16_t channel;
	int ret = 0;
	aim_userinfo_t *userinfo;
	va_list ap;

	va_start(ap, fr);
	channel = (fu16_t)va_arg(ap, unsigned int);
	userinfo = va_arg(ap, aim_userinfo_t *);

	switch (channel) {
		case 1: { /* standard message */
			struct aim_incomingim_ch1_args *args;
			args = va_arg(ap, struct aim_incomingim_ch1_args *);
			ret = incomingim_chan1(sess, fr->conn, userinfo, args);
		} break;

		case 2: { /* rendevous */
			struct aim_incomingim_ch2_args *args;
			args = va_arg(ap, struct aim_incomingim_ch2_args *);
			ret = incomingim_chan2(sess, fr->conn, userinfo, args);
		} break;

		case 4: { /* ICQ */
			struct aim_incomingim_ch4_args *args;
			args = va_arg(ap, struct aim_incomingim_ch4_args *);
			ret = incomingim_chan4(sess, fr->conn, userinfo, args, 0);
		} break;

		default: {
			gaim_debug(GAIM_DEBUG_WARNING, "oscar",
					   "ICBM received on unsupported channel (channel "
					   "0x%04hx).", channel);
		} break;
	}

	va_end(ap);

	return ret;
}

static int gaim_parse_misses(aim_session_t *sess, aim_frame_t *fr, ...) {
	char *buf;
	va_list ap;
	fu16_t chan, nummissed, reason;
	aim_userinfo_t *userinfo;

	va_start(ap, fr);
	chan = (fu16_t)va_arg(ap, unsigned int);
	userinfo = va_arg(ap, aim_userinfo_t *);
	nummissed = (fu16_t)va_arg(ap, unsigned int);
	reason = (fu16_t)va_arg(ap, unsigned int);
	va_end(ap);

	switch(reason) {
		case 0: /* Invalid (0) */
			buf = g_strdup_printf(
				   ngettext(
				   "You missed %hu message from %s because it was invalid.",
				   "You missed %hu messages from %s because they were invalid.",
				   nummissed),
				   nummissed,
				   userinfo->sn);
			break;
		case 1: /* Message too large */
			buf = g_strdup_printf(
				   ngettext(
				   "You missed %hu message from %s because it was too large.",
				   "You missed %hu messages from %s because they were too large.",
				   nummissed),
				   nummissed,
				   userinfo->sn);
			break;
		case 2: /* Rate exceeded */
			buf = g_strdup_printf(
				   ngettext(
				   "You missed %hu message from %s because the rate limit has been exceeded.",
				   "You missed %hu messages from %s because the rate limit has been exceeded.",
				   nummissed),
				   nummissed,
				   userinfo->sn);
			break;
		case 3: /* Evil Sender */
			buf = g_strdup_printf(
				   ngettext(
				   "You missed %hu message from %s because he/she was too evil.",
				   "You missed %hu messages from %s because he/she was too evil.",
				   nummissed),
				   nummissed,
				   userinfo->sn);
			break;
		case 4: /* Evil Receiver */
			buf = g_strdup_printf(
				   ngettext(
				   "You missed %hu message from %s because you are too evil.",
				   "You missed %hu messages from %s because you are too evil.",
				   nummissed),
				   nummissed,
				   userinfo->sn);
			break;
		default:
			buf = g_strdup_printf(
				   ngettext(
				   "You missed %hu message from %s for an unknown reason.",
				   "You missed %hu messages from %s for an unknown reason.",
				   nummissed),
				   nummissed,
				   userinfo->sn);
			break;
	}
	gaim_notify_error(sess->aux_data, NULL, buf, NULL);
	g_free(buf);

	return 1;
}

static char *gaim_icq_status(int state) {
	/* Make a cute little string that shows the status of the dude or dudet */
	if (state & AIM_ICQ_STATE_CHAT)
		return g_strdup_printf(_("Free For Chat"));
	else if (state & AIM_ICQ_STATE_DND)
		return g_strdup_printf(_("Do Not Disturb"));
	else if (state & AIM_ICQ_STATE_OUT)
		return g_strdup_printf(_("Not Available"));
	else if (state & AIM_ICQ_STATE_BUSY)
		return g_strdup_printf(_("Occupied"));
	else if (state & AIM_ICQ_STATE_AWAY)
		return g_strdup_printf(_("Away"));
	else if (state & AIM_ICQ_STATE_WEBAWARE)
		return g_strdup_printf(_("Web Aware"));
	else if (state & AIM_ICQ_STATE_INVISIBLE)
		return g_strdup_printf(_("Invisible"));
	else
		return g_strdup_printf(_("Online"));
}

static int gaim_parse_clientauto_ch2(aim_session_t *sess, const char *who, fu16_t reason, const char *cookie) {
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = gc->proto_data;

/* BBB */
	switch (reason) {
		case 3: { /* Decline sendfile. */
			GaimXfer *xfer;
			gaim_debug(GAIM_DEBUG_INFO, "oscar",
					   "AAA - Other user declined file transfer\n");
			if ((xfer = oscar_find_xfer_by_cookie(od->file_transfers, cookie)))
				gaim_xfer_cancel_remote(xfer);
		} break;

		default: {
			gaim_debug(GAIM_DEBUG_WARNING, "oscar",
					   "Received an unknown rendezvous client auto-response "
					   "from %s.  Type 0x%04hx\n", who, reason);
		}

	}

	return 0;
}

static int gaim_parse_clientauto_ch4(aim_session_t *sess, char *who, fu16_t reason, fu32_t state, char *msg) {
	GaimConnection *gc = sess->aux_data;

	switch(reason) {
		case 0x0003: { /* Reply from an ICQ status message request */
			char *status_msg = gaim_icq_status(state);
			char *dialog_msg, **splitmsg;
			struct oscar_data *od = gc->proto_data;
			GSList *l = od->evilhack;
			gboolean evilhack = FALSE;

			/* Split at (carriage return/newline)'s, then rejoin later with BRs between. */
			splitmsg = g_strsplit(msg, "\r\n", 0);

			/* If who is in od->evilhack, then we're just getting the away message, otherwise this 
			 * will just get appended to the info box (which is already showing). */
			while (l) {
				char *x = l->data;
				if (!strcmp(x, normalize(who))) {
					evilhack = TRUE;
					g_free(x);
					od->evilhack = g_slist_remove(od->evilhack, x);
					break;
				}
				l = l->next;
			}

			if (evilhack)
				dialog_msg = g_strdup_printf(_("<B>UIN:</B> %s<BR><B>Status:</B> %s<HR>%s"), who, status_msg, g_strjoinv("<BR>", splitmsg));
			else
				dialog_msg = g_strdup_printf(_("<B>Status:</B> %s<HR>%s"), status_msg, g_strjoinv("<BR>", splitmsg));
			g_show_info_text(gc, who, 2, dialog_msg, NULL);

			g_free(status_msg);
			g_free(dialog_msg);
			g_strfreev(splitmsg);
		} break;

		default: {
			gaim_debug(GAIM_DEBUG_WARNING, "oscar",
					   "Received an unknown client auto-response from %s.  "
					   "Type 0x%04hx\n", who, reason);
		} break;
	} /* end of switch */

	return 0;
}

static int gaim_parse_clientauto(aim_session_t *sess, aim_frame_t *fr, ...) {
	va_list ap;
	fu16_t chan, reason;
	char *who;

	va_start(ap, fr);
	chan = (fu16_t)va_arg(ap, unsigned int);
	who = va_arg(ap, char *);
	reason = (fu16_t)va_arg(ap, unsigned int);

	if (chan == 0x0002) { /* File transfer declined */
		char *cookie = va_arg(ap, char *);
		return gaim_parse_clientauto_ch2(sess, who, reason, cookie);
	} else if (chan == 0x0004) { /* ICQ message */
		fu32_t state = 0;
		char *msg = NULL;
		if (reason == 0x0003) {
			state = va_arg(ap, fu32_t);
			msg = va_arg(ap, char *);
		}
		return gaim_parse_clientauto_ch4(sess, who, reason, state, msg);
	}

	va_end(ap);

	return 1;
}

static int gaim_parse_genericerr(aim_session_t *sess, aim_frame_t *fr, ...) {
	va_list ap;
	fu16_t reason;
	char *m;

	va_start(ap, fr);
	reason = (fu16_t) va_arg(ap, unsigned int);
	va_end(ap);

	gaim_debug(GAIM_DEBUG_ERROR, "oscar",
			   "snac threw error (reason 0x%04hx: %s)\n", reason,
			   (reason < msgerrreasonlen) ? msgerrreason[reason] : "unknown");

	m = g_strdup_printf(_("SNAC threw error: %s\n"),
			reason < msgerrreasonlen ? _(msgerrreason[reason]) : _("Unknown error"));
	gaim_notify_error(sess->aux_data, NULL, m, NULL);
	g_free(m);

	return 1;
}

static int gaim_parse_msgerr(aim_session_t *sess, aim_frame_t *fr, ...) {
#if 0
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = gc->proto_data;
	GaimXfer *xfer;
#endif
	va_list ap;
	fu16_t reason;
	char *data, *buf;

	va_start(ap, fr);
	reason = (fu16_t)va_arg(ap, unsigned int);
	data = va_arg(ap, char *);
	va_end(ap);

	gaim_debug(GAIM_DEBUG_ERROR, "oscar",
			   "Message error with data %s and reason %hu\n", data, reason);

/* BBB */
#if 0
	/* If this was a file transfer request, data is a cookie */
	if ((xfer = oscar_find_xfer_by_cookie(od->file_transfers, data))) {
		gaim_xfer_cancel_remote(xfer);
		return 1;
	}
#endif

	/* Data is assumed to be the destination sn */
	buf = g_strdup_printf(_("Your message to %s did not get sent:"), data);
	gaim_notify_error(sess->aux_data, NULL, buf,
					  (reason < msgerrreasonlen) ? _(msgerrreason[reason]) : _("No reason given."));
	g_free(buf);

	return 1;
}

static int gaim_parse_mtn(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	va_list ap;
	fu16_t type1, type2;
	char *sn;

	va_start(ap, fr);
	type1 = (fu16_t) va_arg(ap, unsigned int);
	sn = va_arg(ap, char *);
	type2 = (fu16_t) va_arg(ap, unsigned int);
	va_end(ap);

	switch (type2) {
		case 0x0000: { /* Text has been cleared */
			serv_got_typing_stopped(gc, sn);
		} break;

		case 0x0001: { /* Paused typing */
			serv_got_typing(gc, sn, 0, GAIM_TYPED);
		} break;

		case 0x0002: { /* Typing */
			serv_got_typing(gc, sn, 0, GAIM_TYPING);
		} break;

		default: {
			gaim_debug(GAIM_DEBUG_ERROR, "oscar", "Received unknown typing notification message from %s.  Type1 is 0x%04x and type2 is 0x%04hx.\n", sn, type1, type2);
		} break;
	}

	return 1;
}

static int gaim_parse_locerr(aim_session_t *sess, aim_frame_t *fr, ...) {
	char *buf;
	va_list ap;
	fu16_t reason;
	char *destn;

	va_start(ap, fr);
	reason = (fu16_t) va_arg(ap, unsigned int);
	destn = va_arg(ap, char *);
	va_end(ap);

	buf = g_strdup_printf(_("User information for %s unavailable:"), destn);
	gaim_notify_error(sess->aux_data, NULL, buf,
					  (reason < msgerrreasonlen) ? _(msgerrreason[reason]) : _("No reason given."));
	g_free(buf);

	return 1;
}

#if 0
static char *images(int flags) {
	static char buf[1024];
	g_snprintf(buf, sizeof(buf), "%s%s%s%s%s%s%s",
			(flags & AIM_FLAG_ACTIVEBUDDY) ? "<IMG SRC=\"ab_icon.gif\">" : "",
			(flags & AIM_FLAG_UNCONFIRMED) ? "<IMG SRC=\"dt_icon.gif\">" : "",
			(flags & AIM_FLAG_AOL) ? "<IMG SRC=\"aol_icon.gif\">" : "",
			(flags & AIM_FLAG_ICQ) ? "<IMG SRC=\"icq_icon.gif\">" : "",
			(flags & AIM_FLAG_ADMINISTRATOR) ? "<IMG SRC=\"admin_icon.gif\">" : "",
			(flags & AIM_FLAG_FREE) ? "<IMG SRC=\"free_icon.gif\">" : "",
			(flags & AIM_FLAG_WIRELESS) ? "<IMG SRC=\"wireless_icon.gif\">" : "");
	return buf;
}
#endif

static char *caps_string(guint caps)
{
	static char buf[512], *tmp;
	int count = 0, i = 0;
	guint bit = 1;

	if (!caps) {
		return NULL;
	} else while (bit <= AIM_CAPS_LAST) {
		if (bit & caps) {
			switch (bit) {
			case AIM_CAPS_BUDDYICON:
				tmp = _("Buddy Icon");
				break;
			case AIM_CAPS_VOICE:
				tmp = _("Voice");
				break;
			case AIM_CAPS_DIRECTIM:
				tmp = _("Direct IM");
				break;
			case AIM_CAPS_CHAT:
				tmp = _("Chat");
				break;
			case AIM_CAPS_GETFILE:
				tmp = _("Get File");
				break;
			case AIM_CAPS_SENDFILE:
				tmp = _("Send File");
				break;
			case AIM_CAPS_GAMES:
			case AIM_CAPS_GAMES2:
				tmp = _("Games");
				break;
			case AIM_CAPS_SAVESTOCKS:
				tmp = _("Add-Ins");
				break;
			case AIM_CAPS_SENDBUDDYLIST:
				tmp = _("Send Buddy List");
				break;
			case AIM_CAPS_ICQ:
				tmp = _("EveryBuddy Bug");
				break;
			case AIM_CAPS_APINFO:
				tmp = _("AP User");
				break;
			case AIM_CAPS_ICQRTF:
				tmp = _("ICQ RTF");
				break;
			case AIM_CAPS_EMPTY:
				tmp = _("Nihilist");
				break;
			case AIM_CAPS_ICQSERVERRELAY:
				tmp = _("ICQ Server Relay");
				break;
			case AIM_CAPS_ICQUTF8OLD:
				tmp = _("Old ICQ UTF8");
				break;
			case AIM_CAPS_TRILLIANCRYPT:
				tmp = _("Trillian Encryption");
				break;
			case AIM_CAPS_ICQUTF8:
				tmp = _("ICQ UTF8");
				break;
			case AIM_CAPS_HIPTOP:
				tmp = _("Hiptop");
				break;
			case AIM_CAPS_SECUREIM:
				tmp = _("Secure IM");
				break;
			default:
				tmp = NULL;
				break;
			}
			if (tmp)
				i += g_snprintf(buf + i, sizeof(buf) - i, "%s%s", (count ? ", " : ""),
						tmp);
			count++;
		}
		bit <<= 1;
	}
	return buf; 
}

static int gaim_parse_user_info(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = gc->proto_data;
	gchar *header;
	GSList *l = od->evilhack;
	gboolean evilhack = FALSE;
	gchar *membersince = NULL, *onlinesince = NULL, *idle = NULL;
	va_list ap;
	aim_userinfo_t *info;
	fu16_t infotype;
	char *text_enc = NULL, *text = NULL, *utf8 = NULL;
	int text_len;
	const char *username = gaim_account_get_username(gaim_connection_get_account(gc));

	va_start(ap, fr);
	info = va_arg(ap, aim_userinfo_t *);
	infotype = (fu16_t) va_arg(ap, unsigned int);
	text_enc = va_arg(ap, char *);
	text = va_arg(ap, char *);
	text_len = va_arg(ap, int);
	va_end(ap);

	if (text_len > 0) {
		if (!(utf8 = oscar_encoding_to_utf8(text_enc, text, text_len))) {
			utf8 = g_strdup(_("<i>Unable to display information because it was sent in an unknown encoding.</i>"));
			gaim_debug(GAIM_DEBUG_ERROR, "oscar",
					   "Encountered an unknown encoding while parsing userinfo\n");
		}
	}

	if (info->present & AIM_USERINFO_PRESENT_ONLINESINCE) {
		onlinesince = g_strdup_printf(_("Online Since : <b>%s</b><br>\n"),
					asctime(localtime((time_t *)&info->onlinesince)));
	}

	if (info->present & AIM_USERINFO_PRESENT_MEMBERSINCE) {
		membersince = g_strdup_printf(_("Member Since : <b>%s</b><br>\n"),
					asctime(localtime((time_t *)&info->membersince)));
	}

	if (info->present & AIM_USERINFO_PRESENT_IDLE) {
		gchar *itime = sec_to_text(info->idletime*60);
		idle = g_strdup_printf(_("Idle : <b>%s</b>"), itime);
		g_free(itime);
	} else
		idle = g_strdup(_("Idle: <b>Active</b>"));

	header = g_strdup_printf(_("Username : <b>%s</b>  %s <br>\n"
			"Warning Level : <b>%d %%</b><br>\n"
			"%s"
			"%s"
			"%s\n"
			"<hr>\n"),
			info->sn,
			/* images(info->flags), */
			"",
			(int)((info->warnlevel/10.0) + 0.5),
			onlinesince ? onlinesince : "",
			membersince ? membersince : "",
			idle ? idle : "");

	g_free(onlinesince);
	g_free(membersince);
	g_free(idle);

	while (l) {
		char *x = l->data;
		if (!strcmp(x, normalize(info->sn))) {
			evilhack = TRUE;
			g_free(x);
			od->evilhack = g_slist_remove(od->evilhack, x);
			break;
		}
		l = l->next;
	}

	if (infotype == AIM_GETINFO_AWAYMESSAGE) {
		if (evilhack) {
			g_show_info_text(gc, info->sn, 2,
					 header,
					 (utf8 && *utf8) ? away_subs(utf8, username) :
					 _("<i>User has no away message</i>"), NULL);
		} else {
			g_show_info_text(gc, info->sn, 0,
					 header,
					 (utf8 && *utf8) ? away_subs(utf8, username) : NULL,
					 (utf8 && *utf8) ? "<hr>" : NULL,
					 NULL);
		}
	} else if (infotype == AIM_GETINFO_CAPABILITIES) {
		g_show_info_text(gc, info->sn, 2,
				header,
				"<i>", _("Client Capabilities: "),
				caps_string(info->capabilities),
				"</i>",
				NULL);
	} else {
		g_show_info_text(gc, info->sn, 1,
				 (utf8 && *utf8) ? away_subs(utf8, username) : _("<i>No Information Provided</i>"),
				 NULL);
	}

	g_free(header);
	g_free(utf8);

	return 1;
}

static int gaim_parse_motd(aim_session_t *sess, aim_frame_t *fr, ...) {
	char *msg;
	fu16_t id;
	va_list ap;

	va_start(ap, fr);
	id  = (fu16_t) va_arg(ap, unsigned int);
	msg = va_arg(ap, char *);
	va_end(ap);

	gaim_debug(GAIM_DEBUG_MISC, "oscar",
			   "MOTD: %s (%hu)\n", msg ? msg : "Unknown", id);
	if (id < 4)
		gaim_notify_warning(sess->aux_data, NULL,
							_("Your AIM connection may be lost."), NULL);

	return 1;
}

static int gaim_chatnav_info(aim_session_t *sess, aim_frame_t *fr, ...) {
	va_list ap;
	fu16_t type;
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;

	va_start(ap, fr);
	type = (fu16_t) va_arg(ap, unsigned int);

	switch(type) {
		case 0x0002: {
			fu8_t maxrooms;
			struct aim_chat_exchangeinfo *exchanges;
			int exchangecount, i;

			maxrooms = (fu8_t) va_arg(ap, unsigned int);
			exchangecount = va_arg(ap, int);
			exchanges = va_arg(ap, struct aim_chat_exchangeinfo *);

			gaim_debug(GAIM_DEBUG_MISC, "oscar",
					   "chat info: Chat Rights:\n");
			gaim_debug(GAIM_DEBUG_MISC, "oscar",
					   "chat info: \tMax Concurrent Rooms: %hhd\n", maxrooms);
			gaim_debug(GAIM_DEBUG_MISC, "oscar",
					   "chat info: \tExchange List: (%d total)\n", exchangecount);
			for (i = 0; i < exchangecount; i++)
				gaim_debug(GAIM_DEBUG_MISC, "oscar",
						   "chat info: \t\t%hu    %s\n",
						   exchanges[i].number, exchanges[i].name ? exchanges[i].name : "");
			while (od->create_rooms) {
				struct create_room *cr = od->create_rooms->data;
				gaim_debug(GAIM_DEBUG_INFO, "oscar",
						   "creating room %s\n", cr->name);
				aim_chatnav_createroom(sess, fr->conn, cr->name, cr->exchange);
				g_free(cr->name);
				od->create_rooms = g_slist_remove(od->create_rooms, cr);
				g_free(cr);
			}
			}
			break;
		case 0x0008: {
			char *fqcn, *name, *ck;
			fu16_t instance, flags, maxmsglen, maxoccupancy, unknown, exchange;
			fu8_t createperms;
			fu32_t createtime;

			fqcn = va_arg(ap, char *);
			instance = (fu16_t)va_arg(ap, unsigned int);
			exchange = (fu16_t)va_arg(ap, unsigned int);
			flags = (fu16_t)va_arg(ap, unsigned int);
			createtime = va_arg(ap, fu32_t);
			maxmsglen = (fu16_t)va_arg(ap, unsigned int);
			maxoccupancy = (fu16_t)va_arg(ap, unsigned int);
			createperms = (fu8_t)va_arg(ap, unsigned int);
			unknown = (fu16_t)va_arg(ap, unsigned int);
			name = va_arg(ap, char *);
			ck = va_arg(ap, char *);

			gaim_debug(GAIM_DEBUG_MISC, "oscar",
					   "created room: %s %hu %hu %hu %u %hu %hu %hhu %hu %s %s\n",
					fqcn,
					exchange, instance, flags,
					createtime,
					maxmsglen, maxoccupancy, createperms, unknown,
					name, ck);
			aim_chat_join(od->sess, od->conn, exchange, ck, instance);
			}
			break;
		default:
			gaim_debug(GAIM_DEBUG_WARNING, "oscar",
					   "chatnav info: unknown type (%04hx)\n", type);
			break;
	}

	va_end(ap);

	return 1;
}

static int gaim_chat_join(aim_session_t *sess, aim_frame_t *fr, ...) {
	va_list ap;
	int count, i;
	aim_userinfo_t *info;
	GaimConnection *g = sess->aux_data;

	struct chat_connection *c = NULL;

	va_start(ap, fr);
	count = va_arg(ap, int);
	info  = va_arg(ap, aim_userinfo_t *);
	va_end(ap);

	c = find_oscar_chat_by_conn(g, fr->conn);
	if (!c)
		return 1;

	for (i = 0; i < count; i++)
		gaim_chat_add_user(GAIM_CHAT(c->cnv), info[i].sn, NULL);

	return 1;
}

static int gaim_chat_leave(aim_session_t *sess, aim_frame_t *fr, ...) {
	va_list ap;
	int count, i;
	aim_userinfo_t *info;
	GaimConnection *g = sess->aux_data;

	struct chat_connection *c = NULL;

	va_start(ap, fr);
	count = va_arg(ap, int);
	info  = va_arg(ap, aim_userinfo_t *);
	va_end(ap);

	c = find_oscar_chat_by_conn(g, fr->conn);
	if (!c)
		return 1;

	for (i = 0; i < count; i++)
		gaim_chat_remove_user(GAIM_CHAT(c->cnv), info[i].sn, NULL);

	return 1;
}

static int gaim_chat_info_update(aim_session_t *sess, aim_frame_t *fr, ...) {
	va_list ap;
	aim_userinfo_t *userinfo;
	struct aim_chat_roominfo *roominfo;
	char *roomname;
	int usercount;
	char *roomdesc;
	fu16_t unknown_c9, unknown_d2, unknown_d5, maxmsglen, maxvisiblemsglen;
	fu32_t creationtime;
	GaimConnection *gc = sess->aux_data;
	struct chat_connection *ccon = find_oscar_chat_by_conn(gc, fr->conn);

	va_start(ap, fr);
	roominfo = va_arg(ap, struct aim_chat_roominfo *);
	roomname = va_arg(ap, char *);
	usercount= va_arg(ap, int);
	userinfo = va_arg(ap, aim_userinfo_t *);
	roomdesc = va_arg(ap, char *);
	unknown_c9 = (fu16_t)va_arg(ap, unsigned int);
	creationtime = va_arg(ap, fu32_t);
	maxmsglen = (fu16_t)va_arg(ap, unsigned int);
	unknown_d2 = (fu16_t)va_arg(ap, unsigned int);
	unknown_d5 = (fu16_t)va_arg(ap, unsigned int);
	maxvisiblemsglen = (fu16_t)va_arg(ap, unsigned int);
	va_end(ap);

	gaim_debug(GAIM_DEBUG_MISC, "oscar",
			   "inside chat_info_update (maxmsglen = %hu, maxvislen = %hu)\n",
			   maxmsglen, maxvisiblemsglen);

	ccon->maxlen = maxmsglen;
	ccon->maxvis = maxvisiblemsglen;

	return 1;
}

static int gaim_chat_incoming_msg(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	va_list ap;
	aim_userinfo_t *info;
	char *msg;
	struct chat_connection *ccon = find_oscar_chat_by_conn(gc, fr->conn);

	va_start(ap, fr);
	info = va_arg(ap, aim_userinfo_t *);
	msg = va_arg(ap, char *);
	va_end(ap);

	serv_got_chat_in(gc, ccon->id, info->sn, 0, msg, time((time_t)NULL));

	return 1;
}

static int gaim_email_parseupdate(aim_session_t *sess, aim_frame_t *fr, ...) {
	va_list ap;
	GaimConnection *gc = sess->aux_data;
	struct aim_emailinfo *emailinfo;
	int havenewmail;

	va_start(ap, fr);
	emailinfo = va_arg(ap, struct aim_emailinfo *);
	havenewmail = va_arg(ap, int);
	va_end(ap);

	if (emailinfo && gaim_account_get_check_mail(gc->account)) {
		gchar *to = g_strdup_printf("%s@%s", gaim_account_get_username(gaim_connection_get_account(gc)), emailinfo->domain);
		if (emailinfo->unread && havenewmail)
			gaim_notify_emails(gc, emailinfo->nummsgs, FALSE, NULL, NULL, (const char **)&to, (const char **)&emailinfo->url, NULL, NULL);
		g_free(to);
	}

	return 1;
}

static int gaim_icon_error(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = gc->proto_data;
	char *sn;

	sn = od->requesticon->data;
	gaim_debug(GAIM_DEBUG_MISC, "oscar",
			   "removing %s from hash table\n", sn);
	od->requesticon = g_slist_remove(od->requesticon, sn);
	free(sn);

	if (od->icontimer)
		g_source_remove(od->icontimer);
	od->icontimer = g_timeout_add(500, gaim_icon_timerfunc, gc);

	return 1;
}

static int gaim_icon_parseicon(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = gc->proto_data;
	GSList *cur;
	va_list ap;
	char *sn;
	fu8_t *iconcsum, *icon;
	fu16_t iconcsumlen, iconlen;

	va_start(ap, fr);
	sn = va_arg(ap, char *);
	iconcsum = va_arg(ap, fu8_t *);
	iconcsumlen = va_arg(ap, int);
	icon = va_arg(ap, fu8_t *);
	iconlen = va_arg(ap, int);
	va_end(ap);

	if (iconlen > 0) {
		char *b16;
		GaimBuddy *b = gaim_find_buddy(gc->account, sn);
		gaim_buddy_icons_set_for_user(gaim_connection_get_account(gc),
									  sn, icon, iconlen);
		b16 = tobase16(iconcsum, iconcsumlen);
		if (b16) {
			gaim_buddy_set_setting(b, "icon_checksum", b16);
			gaim_blist_save();
			free(b16);
		}
	}

	cur = od->requesticon;
	while (cur) {
		char *cursn = cur->data;
		if (!aim_sncmp(cursn, sn)) {
			od->requesticon = g_slist_remove(od->requesticon, cursn);
			free(cursn);
			cur = od->requesticon;
		} else
			cur = cur->next;
	}

	if (od->icontimer)
		g_source_remove(od->icontimer);
	od->icontimer = g_timeout_add(250, gaim_icon_timerfunc, gc);

	return 1;
}

static gboolean gaim_icon_timerfunc(gpointer data) {
	GaimConnection *gc = data;
	struct oscar_data *od = gc->proto_data;
	struct buddyinfo *bi;
	aim_conn_t *conn;

	conn = aim_getconn_type(od->sess, AIM_CONN_TYPE_ICON);
	if (!conn) {
		if (!od->iconconnecting) {
			aim_reqservice(od->sess, od->conn, AIM_CONN_TYPE_ICON);
			od->iconconnecting = TRUE;
		}
		return FALSE;
	}

	if (od->set_icon) {
		struct stat st;
		const char *iconfile = gaim_account_get_buddy_icon(gaim_connection_get_account(gc));
		if (iconfile == NULL) {
			/* Set an empty icon, or something */
		} else if (!stat(iconfile, &st)) {
			char *buf = g_malloc(st.st_size);
			FILE *file = fopen(iconfile, "rb");
			if (file) {
				fread(buf, 1, st.st_size, file);
				fclose(file);
				gaim_debug(GAIM_DEBUG_INFO, "oscar",
					   "Uploading icon to icon server\n");
				aim_bart_upload(od->sess, buf, st.st_size);
			} else
				gaim_debug(GAIM_DEBUG_ERROR, "oscar",
					   "Can't open buddy icon file!\n");
			g_free(buf);
		} else {
			gaim_debug(GAIM_DEBUG_ERROR, "oscar",
				   "Can't stat buddy icon file!\n");
		}
		od->set_icon = FALSE;
	}

	if (!od->requesticon) {
		gaim_debug(GAIM_DEBUG_MISC, "oscar",
				   "no more icons to request\n");
		return FALSE;
	}

	bi = g_hash_table_lookup(od->buddyinfo, (char *)od->requesticon->data);
	if (bi && (bi->iconcsumlen > 0)) {
		aim_bart_request(od->sess, od->requesticon->data, bi->iconcsum, bi->iconcsumlen);
		return FALSE;
	} else {
		char *sn = od->requesticon->data;
		od->requesticon = g_slist_remove(od->requesticon, sn);
		free(sn);
	}

	return TRUE;
}

/*
 * Recieved in response to an IM sent with the AIM_IMFLAGS_ACK option.
 */
static int gaim_parse_msgack(aim_session_t *sess, aim_frame_t *fr, ...) {
	va_list ap;
	fu16_t type;
	char *sn;

	va_start(ap, fr);
	type = (fu16_t) va_arg(ap, unsigned int);
	sn = va_arg(ap, char *);
	va_end(ap);

	gaim_debug(GAIM_DEBUG_INFO, "oscar", "Sent message to %s.\n", sn);

	return 1;
}

static int gaim_parse_ratechange(aim_session_t *sess, aim_frame_t *fr, ...) {
	static const char *codes[5] = {
		"invalid",
		"change",
		"warning",
		"limit",
		"limit cleared",
	};
	va_list ap;
	fu16_t code, rateclass;
	fu32_t windowsize, clear, alert, limit, disconnect, currentavg, maxavg;

	va_start(ap, fr); 
	code = (fu16_t)va_arg(ap, unsigned int);
	rateclass= (fu16_t)va_arg(ap, unsigned int);
	windowsize = va_arg(ap, fu32_t);
	clear = va_arg(ap, fu32_t);
	alert = va_arg(ap, fu32_t);
	limit = va_arg(ap, fu32_t);
	disconnect = va_arg(ap, fu32_t);
	currentavg = va_arg(ap, fu32_t);
	maxavg = va_arg(ap, fu32_t);
	va_end(ap);

	gaim_debug(GAIM_DEBUG_MISC, "oscar",
			   "rate %s (param ID 0x%04hx): curavg = %u, maxavg = %u, alert at %u, "
		     "clear warning at %u, limit at %u, disconnect at %u (window size = %u)\n",
		     (code < 5) ? codes[code] : codes[0],
		     rateclass,
		     currentavg, maxavg,
		     alert, clear,
		     limit, disconnect,
		     windowsize);

	/* XXX fix these values */
	if (code == AIM_RATE_CODE_CHANGE) {
		if (currentavg >= clear)
			aim_conn_setlatency(fr->conn, 0);
	} else if (code == AIM_RATE_CODE_WARNING) {
		aim_conn_setlatency(fr->conn, windowsize/4);
	} else if (code == AIM_RATE_CODE_LIMIT) {
		gaim_notify_error(sess->aux_data, NULL, _("Rate limiting error."),
						  _("The last action you attempted could not be "
							"performed because you are over the rate limit. "
							"Please wait 10 seconds and try again."));
		aim_conn_setlatency(fr->conn, windowsize/2);
	} else if (code == AIM_RATE_CODE_CLEARLIMIT) {
		aim_conn_setlatency(fr->conn, 0);
	}

	return 1;
}

static int gaim_parse_evilnotify(aim_session_t *sess, aim_frame_t *fr, ...) {
	va_list ap;
	fu16_t newevil;
	aim_userinfo_t *userinfo;
	GaimConnection *gc = sess->aux_data;

	va_start(ap, fr);
	newevil = (fu16_t) va_arg(ap, unsigned int);
	userinfo = va_arg(ap, aim_userinfo_t *);
	va_end(ap);

	serv_got_eviled(gc, (userinfo && userinfo->sn[0]) ? userinfo->sn : NULL, (newevil/10.0) + 0.5);

	return 1;
}

static int gaim_selfinfo(aim_session_t *sess, aim_frame_t *fr, ...) {
	va_list ap;
	aim_userinfo_t *info;
	GaimConnection *gc = sess->aux_data;

	va_start(ap, fr);
	info = va_arg(ap, aim_userinfo_t *);
	va_end(ap);

	gc->evil = (info->warnlevel/10.0) + 0.5;

	if (info->onlinesince)
		gc->login_time_official = info->onlinesince;

	return 1;
}

static int gaim_connerr(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = gc->proto_data;
	va_list ap;
	fu16_t code;
	char *msg;

	va_start(ap, fr);
	code = (fu16_t)va_arg(ap, int);
	msg = va_arg(ap, char *);
	va_end(ap);

	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "Disconnected.  Code is 0x%04x and msg is %s\n", code, msg);
	if ((fr) && (fr->conn) && (fr->conn->type == AIM_CONN_TYPE_BOS)) {
		if (code == 0x0001) {
			gc->wants_to_die = TRUE;
			gaim_connection_error(gc, _("You have been disconnected because you have signed on with this screen name at another location."));
		} else {
			gaim_connection_error(gc, _("You have been signed off for an unknown reason."));
		}
		od->killme = TRUE;
	}

	return 1;
}

static int conninitdone_bos(aim_session_t *sess, aim_frame_t *fr, ...) {

	aim_reqpersonalinfo(sess, fr->conn);

#ifndef NOSSI
	gaim_debug(GAIM_DEBUG_INFO, "oscar", "ssi: requesting ssi list\n");
	aim_ssi_reqrights(sess);
	aim_ssi_reqdata(sess);
#endif

	aim_bos_reqlocaterights(sess, fr->conn);
	aim_bos_reqbuddyrights(sess, fr->conn);
	aim_im_reqparams(sess);
	aim_bos_reqrights(sess, fr->conn); /* XXX - Don't call this with ssi? */

#ifdef NOSSI
	aim_bos_setgroupperm(sess, fr->conn, AIM_FLAG_ALLUSERS);
	aim_bos_setprivacyflags(sess, fr->conn, AIM_PRIVFLAGS_ALLOWIDLE | AIM_PRIVFLAGS_ALLOWMEMBERSINCE);
#endif

	return 1;
}

static int conninitdone_admin(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = gc->proto_data;

	aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_ADM, 0x0003, gaim_info_change, 0);
	aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_ADM, 0x0005, gaim_info_change, 0);
	aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_ADM, 0x0007, gaim_account_confirm, 0);

	aim_clientready(sess, fr->conn);
	gaim_debug(GAIM_DEBUG_INFO, "oscar", "connected to admin\n");

	if (od->chpass) {
		gaim_debug(GAIM_DEBUG_INFO, "oscar", "changing password\n");
		aim_admin_changepasswd(sess, fr->conn, od->newp, od->oldp);
		g_free(od->oldp);
		od->oldp = NULL;
		g_free(od->newp);
		od->newp = NULL;
		od->chpass = FALSE;
	}
	if (od->setnick) {
		gaim_debug(GAIM_DEBUG_INFO, "oscar", "formatting screenname\n");
		aim_admin_setnick(sess, fr->conn, od->newsn);
		g_free(od->newsn);
		od->newsn = NULL;
		od->setnick = FALSE;
	}
	if (od->conf) {
		gaim_debug(GAIM_DEBUG_INFO, "oscar", "confirming account\n");
		aim_admin_reqconfirm(sess, fr->conn);
		od->conf = FALSE;
	}
	if (od->reqemail) {
		gaim_debug(GAIM_DEBUG_INFO, "oscar", "requesting email\n");
		aim_admin_getinfo(sess, fr->conn, 0x0011);
		od->reqemail = FALSE;
	}
	if (od->setemail) {
		gaim_debug(GAIM_DEBUG_INFO, "oscar", "setting email\n");
		aim_admin_setemail(sess, fr->conn, od->email);
		g_free(od->email);
		od->email = NULL;
		od->setemail = FALSE;
	}

	return 1;
}

static int gaim_icbm_param_info(aim_session_t *sess, aim_frame_t *fr, ...) {
	struct aim_icbmparameters *params;
	va_list ap;

	va_start(ap, fr);
	params = va_arg(ap, struct aim_icbmparameters *);
	va_end(ap);

	/* XXX - evidently this crashes on solaris. i have no clue why
	gaim_debug(GAIM_DEBUG_MISC, "oscar", "ICBM Parameters: maxchannel = %hu, default flags = 0x%08lx, max msg len = %hu, "
			"max sender evil = %f, max receiver evil = %f, min msg interval = %u\n",
			params->maxchan, params->flags, params->maxmsglen,
			((float)params->maxsenderwarn)/10.0, ((float)params->maxrecverwarn)/10.0,
			params->minmsginterval);
	*/

	/* Maybe senderwarn and recverwarn should be user preferences... */
	params->flags = 0x0000000b;
	params->maxmsglen = 8000;
	params->minmsginterval = 0;

	aim_im_setparams(sess, params);

	return 1;
}

static int gaim_parse_locaterights(aim_session_t *sess, aim_frame_t *fr, ...)
{
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
	va_list ap;
	fu16_t maxsiglen;

	va_start(ap, fr);
	maxsiglen = (fu16_t) va_arg(ap, int);
	va_end(ap);

	gaim_debug(GAIM_DEBUG_MISC, "oscar",
			   "locate rights: max sig len = %d\n", maxsiglen);

	od->rights.maxsiglen = od->rights.maxawaymsglen = (guint)maxsiglen;

	if (od->icq)
		aim_bos_setprofile(sess, fr->conn, NULL, NULL, 0, NULL, NULL, 0, caps_icq);
	else
		oscar_set_info(gc, gc->account->user_info);

	return 1;
}

static int gaim_parse_buddyrights(aim_session_t *sess, aim_frame_t *fr, ...) {
	va_list ap;
	fu16_t maxbuddies, maxwatchers;
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;

	va_start(ap, fr);
	maxbuddies = (fu16_t) va_arg(ap, unsigned int);
	maxwatchers = (fu16_t) va_arg(ap, unsigned int);
	va_end(ap);

	gaim_debug(GAIM_DEBUG_MISC, "oscar",
			   "buddy list rights: Max buddies = %hu / Max watchers = %hu\n", maxbuddies, maxwatchers);

	od->rights.maxbuddies = (guint)maxbuddies;
	od->rights.maxwatchers = (guint)maxwatchers;

	return 1;
}

static int gaim_bosrights(aim_session_t *sess, aim_frame_t *fr, ...) {
	fu16_t maxpermits, maxdenies;
	va_list ap;
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;

	va_start(ap, fr);
	maxpermits = (fu16_t) va_arg(ap, unsigned int);
	maxdenies = (fu16_t) va_arg(ap, unsigned int);
	va_end(ap);

	gaim_debug(GAIM_DEBUG_MISC, "oscar",
			   "BOS rights: Max permit = %hu / Max deny = %hu\n", maxpermits, maxdenies);

	od->rights.maxpermits = (guint)maxpermits;
	od->rights.maxdenies = (guint)maxdenies;

	gaim_connection_set_state(gc, GAIM_CONNECTED);
	serv_finish_login(gc);

	gaim_debug(GAIM_DEBUG_INFO, "oscar", "buddy list loaded\n");

	aim_clientready(sess, fr->conn);
	aim_srv_setavailmsg(sess, NULL);
	aim_bos_setidle(sess, fr->conn, 0);

	if (od->icq) {
		aim_icq_reqofflinemsgs(sess);
		aim_icq_hideip(sess);
	}

	aim_reqservice(sess, fr->conn, AIM_CONN_TYPE_CHATNAV);
	if (sess->authinfo->email)
		aim_reqservice(sess, fr->conn, AIM_CONN_TYPE_EMAIL);

	return 1;
}

static int gaim_offlinemsg(aim_session_t *sess, aim_frame_t *fr, ...) {
	va_list ap;
	struct aim_icq_offlinemsg *msg;
	struct aim_incomingim_ch4_args args;
	time_t t;

	va_start(ap, fr);
	msg = va_arg(ap, struct aim_icq_offlinemsg *);
	va_end(ap);

	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "Received offline message.  Converting to channel 4 ICBM...\n");
	args.uin = msg->sender;
	args.type = msg->type;
	args.flags = msg->flags;
	args.msglen = msg->msglen;
	args.msg = msg->msg;
	t = get_time(msg->year, msg->month, msg->day, msg->hour, msg->minute, 0);
	incomingim_chan4(sess, fr->conn, NULL, &args, t);

	return 1;
}

static int gaim_offlinemsgdone(aim_session_t *sess, aim_frame_t *fr, ...)
{
	aim_icq_ackofflinemsgs(sess);
	return 1;
}

static int gaim_icqinfo(aim_session_t *sess, aim_frame_t *fr, ...)
{
	GaimConnection *gc = sess->aux_data;
	gchar *buf, *tmp, *utf8;
	gchar who[16];
	GaimBuddy *buddy;
	gchar *primary;
	va_list ap;
	struct aim_icq_info *info;

	va_start(ap, fr);
	info = va_arg(ap, struct aim_icq_info *);
	va_end(ap);

	if (!info->uin)
		return 0;

	g_snprintf(who, sizeof(who), "%u", info->uin);
	buf = g_strdup_printf("<b>%s:</b> %s", _("UIN"), who);
	if (info->nick && info->nick[0] && (utf8 = gaim_try_conv_to_utf8(info->nick))) {
		tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("Nick"), ":</b> ", utf8, NULL);  g_free(tmp); g_free(utf8);
	}
	if (info->first && info->first[0] && (utf8 = gaim_try_conv_to_utf8(info->first))) {
		tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("First Name"), ":</b> ", utf8, NULL);  g_free(tmp); g_free(utf8);
	}
	if (info->last && info->last[0] && (utf8 = gaim_try_conv_to_utf8(info->last))) {
		tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("Last Name"), ":</b> ", utf8, NULL);  g_free(tmp); g_free(utf8);
	}
	if (info->email && info->email[0] && (utf8 = gaim_try_conv_to_utf8(info->email))) {
		tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("Email Address"), ":</b> <a href=\"mailto:", utf8, "\">", utf8, "</a>", NULL);  g_free(tmp); g_free(utf8);
	}
	if (info->numaddresses && info->email2) {
		int i;
		for (i = 0; i < info->numaddresses; i++) {
			if (info->email2[i] && info->email2[i][0] && (utf8 = gaim_try_conv_to_utf8(info->email2[i]))) {
			tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("Email Address"), ":</b> <a href=\"mailto:", utf8, "\">", utf8, "</a>", NULL);  g_free(tmp); g_free(utf8);
			}
		}
	}
	if (info->mobile && info->mobile[0] && (utf8 = gaim_try_conv_to_utf8(info->mobile))) {
		tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("Mobile Phone"), ":</b> ", utf8, NULL);  g_free(tmp); g_free(utf8);
	}
	if (info->gender) {
		tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("Gender"), ":</b> ", info->gender==1 ? _("Female") : _("Male"), NULL);  g_free(tmp);
	}
	if (info->birthyear || info->birthmonth || info->birthday) {
		char date[30];
		struct tm tm;
		tm.tm_mday = (int)info->birthday;
		tm.tm_mon = (int)info->birthmonth-1;
		tm.tm_year = (int)info->birthyear-1900;
		strftime(date, sizeof(date), "%x", &tm);
		tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("Birthday"), ":</b> ", date, NULL);  g_free(tmp);
	}
	if (info->age) {
		char age[5];
		snprintf(age, sizeof(age), "%hhd", info->age);
		tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("Age"), ":</b> ", age, NULL);  g_free(tmp);
	}
	if (info->personalwebpage && info->personalwebpage[0] && (utf8 = gaim_try_conv_to_utf8(info->personalwebpage))) {
		tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("Personal Web Page"), ":</b> <a href=\"", utf8, "\">", utf8, "</a>", NULL);  g_free(tmp); g_free(utf8);
	}
	if (info->info && info->info[0] && (utf8 = gaim_try_conv_to_utf8(info->info))) {
		tmp = buf;  buf = g_strconcat(tmp, "<hr><b>", _("Additional Information"), ":</b><br>", utf8, NULL);  g_free(tmp); g_free(utf8);
	}
	tmp = buf;  buf = g_strconcat(tmp, "<hr>\n", NULL);  g_free(tmp);
	if ((info->homeaddr && (info->homeaddr[0])) || (info->homecity && info->homecity[0]) || (info->homestate && info->homestate[0]) || (info->homezip && info->homezip[0])) {
		tmp = buf;  buf = g_strconcat(tmp, "<b>", _("Home Address"), ":</b>", NULL);  g_free(tmp);
		if (info->homeaddr && info->homeaddr[0] && (utf8 = gaim_try_conv_to_utf8(info->homeaddr))) {
			tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("Address"), ":</b> ", utf8, NULL);  g_free(tmp); g_free(utf8);
		}
		if (info->homecity && info->homecity[0] && (utf8 = gaim_try_conv_to_utf8(info->homecity))) {
			tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("City"), ":</b> ", utf8,  NULL);  g_free(tmp); g_free(utf8);
		}
		if (info->homestate && info->homestate[0] && (utf8 = gaim_try_conv_to_utf8(info->homestate))) {
			tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("State"), ":</b> ", utf8, NULL);  g_free(tmp); g_free(utf8);
		}
		if (info->homezip && info->homezip[0] && (utf8 = gaim_try_conv_to_utf8(info->homezip))) {
			tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("Zip Code"), ":</b> ", utf8, NULL);  g_free(tmp); g_free(utf8);
		}
		tmp = buf; buf = g_strconcat(tmp, "\n<hr>\n", NULL); g_free(tmp);
	}
	if ((info->workaddr && info->workaddr[0]) || (info->workcity && info->workcity[0]) || (info->workstate && info->workstate[0]) || (info->workzip && info->workzip[0])) {
		tmp = buf;  buf = g_strconcat(tmp, "<b>", _("Work Address"), ":</b>", NULL);  g_free(tmp);
		if (info->workaddr && info->workaddr[0] && (utf8 = gaim_try_conv_to_utf8(info->workaddr))) {
			tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("Address"), ":</b> ", utf8, NULL);  g_free(tmp); g_free(utf8);
		}
		if (info->workcity && info->workcity[0] && (utf8 = gaim_try_conv_to_utf8(info->workcity))) {
			tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("City"), ":</b> ", utf8, NULL);  g_free(tmp); g_free(utf8);
		}
		if (info->workstate && info->workstate[0] && (utf8 = gaim_try_conv_to_utf8(info->workstate))) {
			tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("State"), ":</b> ", utf8, NULL);  g_free(tmp); g_free(utf8);
		}
		if (info->workzip && info->workzip[0] && (utf8 = gaim_try_conv_to_utf8(info->workzip))) {
			tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("Zip Code"), ":</b> ", utf8, NULL);  g_free(tmp); g_free(utf8);
		}
		tmp = buf; buf = g_strconcat(tmp, "\n<hr>\n", NULL); g_free(tmp);
	}
	if ((info->workcompany && info->workcompany[0]) || (info->workdivision && info->workdivision[0]) || (info->workposition && info->workposition[0]) || (info->workwebpage && info->workwebpage[0])) {
		tmp = buf;  buf = g_strconcat(tmp, "<b>", _("Work Information"), ":</b>", NULL);  g_free(tmp);
		if (info->workcompany && info->workcompany[0] && (utf8 = gaim_try_conv_to_utf8(info->workcompany))) {
			tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("Company"), ":</b> ", utf8, NULL);  g_free(tmp); g_free(utf8);
		}
		if (info->workdivision && info->workdivision[0] && (utf8 = gaim_try_conv_to_utf8(info->workdivision))) {
			tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("Division"), ":</b> ", utf8, NULL);  g_free(tmp); g_free(utf8);
		}
		if (info->workposition && info->workposition[0] && (utf8 = gaim_try_conv_to_utf8(info->workposition))) {
			tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("Position"), ":</b> ", utf8, NULL);  g_free(tmp); g_free(utf8);
		}
		if (info->workwebpage && info->workwebpage[0] && (utf8 = gaim_try_conv_to_utf8(info->workwebpage))) {
			tmp = buf;  buf = g_strconcat(tmp, "\n<br><b>", _("Web Page"), ":</b> <a href=\"", utf8, "\">", utf8, "</a>", NULL);  g_free(tmp); g_free(utf8);
		}
		tmp = buf; buf = g_strconcat(tmp, "\n<hr>\n", NULL); g_free(tmp);
	}

	buddy = gaim_find_buddy(gaim_connection_get_account(gc), who);
	primary = g_strdup_printf(_("ICQ Info for %s"), gaim_get_buddy_alias(buddy));
	gaim_notify_formatted(gc, NULL, primary, NULL, buf, NULL, NULL);
	g_free(primary);
	g_free(buf);

	return 1;
}

static int gaim_icqalias(aim_session_t *sess, aim_frame_t *fr, ...)
{
	GaimConnection *gc = sess->aux_data;
	gchar who[16], *utf8;
	GaimBuddy *b;
	va_list ap;
	struct aim_icq_info *info;

	va_start(ap, fr);
	info = va_arg(ap, struct aim_icq_info *);
	va_end(ap);

	if (info->uin && info->nick && info->nick[0] && (utf8 = gaim_try_conv_to_utf8(info->nick))) {
		g_snprintf(who, sizeof(who), "%u", info->uin);
		serv_got_alias(gc, who, utf8);
		if ((b = gaim_find_buddy(gc->account, who))) {
			gaim_buddy_set_setting(b, "servernick", utf8);
			gaim_blist_save();
		}
		g_free(utf8);
	}

	return 1;
}

static int gaim_popup(aim_session_t *sess, aim_frame_t *fr, ...)
{
	char *msg, *url;
	fu16_t wid, hei, delay;
	va_list ap;

	va_start(ap, fr);
	msg = va_arg(ap, char *);
	url = va_arg(ap, char *);
	wid = (fu16_t) va_arg(ap, int);
	hei = (fu16_t) va_arg(ap, int);
	delay = (fu16_t) va_arg(ap, int);
	va_end(ap);

	serv_got_popup(msg, url, wid, hei);

	return 1;
}

static int gaim_parse_searchreply(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	gchar *secondary;
	GString *text;
	int i, num;
	va_list ap;
	char *email, *SNs;

	va_start(ap, fr);
	email = va_arg(ap, char *);
	num = va_arg(ap, int);
	SNs = va_arg(ap, char *);
	va_end(ap);

	secondary = g_strdup_printf(_("The following screennames are associated with %s"), email);
	text = g_string_new("");
	for (i = 0; i < num; i++)
		g_string_append_printf(text, "%s<br>", &SNs[i * (MAXSNLEN + 1)]);
	gaim_notify_formatted(gc, NULL, _("Search Results"), secondary, text->str, NULL, NULL);

	g_free(secondary);
	g_string_free(text, TRUE);

	return 1;
}

static int gaim_parse_searcherror(aim_session_t *sess, aim_frame_t *fr, ...) {
	va_list ap;
	char *email;
	char *buf;

	va_start(ap, fr);
	email = va_arg(ap, char *);
	va_end(ap);

	buf = g_strdup_printf(_("No results found for email address %s"), email);
	gaim_notify_error(sess->aux_data, NULL, buf, NULL);
	g_free(buf);

	return 1;
}

static int gaim_account_confirm(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	fu16_t status;
	va_list ap;
	char msg[256];

	va_start(ap, fr);
	status = (fu16_t) va_arg(ap, unsigned int); /* status code of confirmation request */
	va_end(ap);

	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "account confirmation returned status 0x%04x (%s)\n", status,
			status ? "unknown" : "email sent");
	if (!status) {
		g_snprintf(msg, sizeof(msg), _("You should receive an email asking to confirm %s."),
				gaim_account_get_username(gaim_connection_get_account(gc)));
		gaim_notify_info(gc, NULL, _("Account Confirmation Requested"), msg);
	}

	return 1;
}

static int gaim_info_change(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	va_list ap;
	fu16_t perms, err;
	char *url, *sn, *email;
	int change;

	va_start(ap, fr);
	change = va_arg(ap, int);
	perms = (fu16_t) va_arg(ap, unsigned int);
	err = (fu16_t) va_arg(ap, unsigned int);
	url = va_arg(ap, char *);
	sn = va_arg(ap, char *);
	email = va_arg(ap, char *);
	va_end(ap);

	gaim_debug(GAIM_DEBUG_MISC, "oscar",
			   "account info: because of %s, perms=0x%04x, err=0x%04x, url=%s, sn=%s, email=%s\n",
		change ? "change" : "request", perms, err, url, sn, email);

	if (err && url) {
		char *dialog_msg;
		char *dialog_top = g_strdup_printf(_("Error Changing Account Info"));
		switch (err) {
			case 0x0001: {
				dialog_msg = g_strdup_printf(_("Error 0x%04x: Unable to format screen name because the requested screen name differs from the original."), err);
			} break;
			case 0x0006: {
				dialog_msg = g_strdup_printf(_("Error 0x%04x: Unable to format screen name because the requested screen name ends in a space."), err);
			} break;
			case 0x000b: {
				dialog_msg = g_strdup_printf(_("Error 0x%04x: Unable to format screen name because the requested screen name is too long."), err);
			} break;
			case 0x001d: {
				dialog_msg = g_strdup_printf(_("Error 0x%04x: Unable to change email address because there is already a request pending for this screen name."), err);
			} break;
			case 0x0021: {
				dialog_msg = g_strdup_printf(_("Error 0x%04x: Unable to change email address because the given address has too many screen names associated with it."), err);
			} break;
			case 0x0023: {
				dialog_msg = g_strdup_printf(_("Error 0x%04x: Unable to change email address because the given address is invalid."), err);
			} break;
			default: {
				dialog_msg = g_strdup_printf(_("Error 0x%04x: Unknown error."), err);
			} break;
		}
		gaim_notify_error(gc, NULL, dialog_top, dialog_msg);
		g_free(dialog_top);
		g_free(dialog_msg);
		return 1;
	}

	if (sn) {
		char *dialog_msg = g_strdup_printf(_("Your screen name is currently formatted as follows:\n%s"), sn);
		gaim_notify_info(gc, NULL, _("Account Info"), dialog_msg);
		g_free(dialog_msg);
	}

	if (email) {
		char *dialog_msg = g_strdup_printf(_("The email address for %s is %s"), 
						   gaim_account_get_username(gaim_connection_get_account(gc)), email);
		gaim_notify_info(gc, NULL, _("Account Info"), dialog_msg);
		g_free(dialog_msg);
	}

	return 1;
}

static void oscar_keepalive(GaimConnection *gc) {
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
	aim_flap_nop(od->sess, od->conn);
}

static int oscar_send_typing(GaimConnection *gc, const char *name, int typing) {
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
	struct direct_im *dim = find_direct_im(od, name);
	if (dim)
		if (typing == GAIM_TYPING)
			aim_odc_send_typing(od->sess, dim->conn, 0x0002);
		else if (typing == GAIM_TYPED)
			aim_odc_send_typing(od->sess, dim->conn, 0x0001);
		else
			aim_odc_send_typing(od->sess, dim->conn, 0x0000);
	else {
		/* Don't send if this turkey is in our deny list */
		GSList *list;
		for (list=gc->account->deny; (list && aim_sncmp(name, list->data)); list=list->next);
		if (!list) {
			struct buddyinfo *bi = g_hash_table_lookup(od->buddyinfo, normalize(name));
			if (bi && bi->typingnot) {
				if (typing == GAIM_TYPING)
					aim_im_sendmtn(od->sess, 0x0001, name, 0x0002);
				else if (typing == GAIM_TYPED)
					aim_im_sendmtn(od->sess, 0x0001, name, 0x0001);
				else
					aim_im_sendmtn(od->sess, 0x0001, name, 0x0000);
			}
		}
	}
	return 0;
}
static void oscar_ask_direct_im(GaimConnection *gc, const char *name);
static int gaim_odc_send_im(aim_session_t *, aim_conn_t *, const char *, GaimImFlags);

static int oscar_send_im(GaimConnection *gc, const char *name, const char *message, GaimImFlags imflags) {
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
	struct direct_im *dim = find_direct_im(od, name);
	int ret = 0;
	GError *err = NULL;
	const char *iconfile = gaim_account_get_buddy_icon(gaim_connection_get_account(gc));
	char *tmpmsg = NULL;

	if (dim && dim->connected) {
		/* If we're directly connected, send a direct IM */
		ret = gaim_odc_send_im(od->sess, dim->conn, message, imflags);
	} else if (imflags & GAIM_IM_IMAGES) {
		/* Trying to send an IM image outside of a direct connection. */
		oscar_ask_direct_im(gc, name);
		ret = -ENOTCONN;
	} else {
		struct buddyinfo *bi;
		struct aim_sendimext_args args;
		struct stat st;
		gsize len;

		bi = g_hash_table_lookup(od->buddyinfo, normalize(name));
		if (!bi) {
			bi = g_new0(struct buddyinfo, 1);
			g_hash_table_insert(od->buddyinfo, g_strdup(normalize(name)), bi);
		}

		args.flags = AIM_IMFLAGS_ACK | AIM_IMFLAGS_CUSTOMFEATURES;
		if (od->icq) {
			args.features = features_icq;
			args.featureslen = sizeof(features_icq);
			args.flags |= AIM_IMFLAGS_OFFLINE;
		} else {
			args.features = features_aim;
			args.featureslen = sizeof(features_aim);

			if (imflags & GAIM_IM_AUTO_RESP)
				args.flags |= AIM_IMFLAGS_AWAY;
		}

		if (bi->ico_need) {
			gaim_debug(GAIM_DEBUG_INFO, "oscar",
					   "Sending buddy icon request with message\n");
			args.flags |= AIM_IMFLAGS_BUDDYREQ;
			bi->ico_need = FALSE;
		}

		if (iconfile && !stat(iconfile, &st)) {
			FILE *file = fopen(iconfile, "r");
			if (file) {
				char *buf = g_malloc(st.st_size);
				fread(buf, 1, st.st_size, file);
				fclose(file);

				args.iconlen   = st.st_size;
				args.iconsum   = aimutil_iconsum(buf, st.st_size);
				args.iconstamp = st.st_mtime;

				if ((args.iconlen != bi->ico_me_len) || (args.iconsum != bi->ico_me_csum) || (args.iconstamp != bi->ico_me_time))
					bi->ico_informed = FALSE;

				if (!bi->ico_informed) {
					gaim_debug(GAIM_DEBUG_INFO, "oscar",
							   "Claiming to have a buddy icon\n");
					args.flags |= AIM_IMFLAGS_HASICON;
					bi->ico_me_len = args.iconlen;
					bi->ico_me_csum = args.iconsum;
					bi->ico_me_time = args.iconstamp;
					bi->ico_informed = TRUE;
				}

				g_free(buf);
			}
		}

		args.destsn = name;

		/* For ICQ send newlines as CR/LF, for AIM send newlines as <BR> */
		if (isdigit(name[0]))
			tmpmsg = add_cr(message);
		else
			tmpmsg = strdup_withhtml(message);
		len = strlen(tmpmsg);

		args.flags |= oscar_encoding_check(tmpmsg);
		if (args.flags & AIM_IMFLAGS_UNICODE) {
			gaim_debug(GAIM_DEBUG_INFO, "oscar", "Sending Unicode IM\n");
			args.charset = 0x0002;
			args.charsubset = 0x0000;
			args.msg = g_convert(tmpmsg, len, "UCS-2BE", "UTF-8", NULL, &len, &err);
			if (err) {
				gaim_debug(GAIM_DEBUG_ERROR, "oscar",
						   "Error converting a unicode message: %s\n", err->message);
				gaim_debug(GAIM_DEBUG_ERROR, "oscar",
						   "This really shouldn't happen!\n");
				/* We really shouldn't try to send the
				 * IM now, but I'm not sure what to do */
				g_error_free(err);
			}
		} else if (args.flags & AIM_IMFLAGS_ISO_8859_1) {
			gaim_debug(GAIM_DEBUG_INFO, "oscar",
					   "Sending ISO-8859-1 IM\n");
			args.charset = 0x0003;
			args.charsubset = 0x0000;
			args.msg = g_convert(tmpmsg, len, "ISO-8859-1", "UTF-8", NULL, &len, &err);
			if (err) {
				gaim_debug(GAIM_DEBUG_ERROR, "oscar",
						   "conversion error: %s\n", err->message);
				gaim_debug(GAIM_DEBUG_ERROR, "oscar",
						   "Someone tell Ethan his 8859-1 detection is wrong\n");
				args.flags ^= AIM_IMFLAGS_ISO_8859_1 | AIM_IMFLAGS_UNICODE;
				len = strlen(tmpmsg);
				g_error_free(err);
				args.msg = g_convert(tmpmsg, len, "UCS-2BE", "UTF8", NULL, &len, &err);
				if (err) {
					gaim_debug(GAIM_DEBUG_ERROR, "oscar",
							   "Error in unicode fallback: %s\n", err->message);
					g_error_free(err);
				}
			}
		} else {
			args.charset = 0x0000;
			args.charsubset = 0x0000;
			args.msg = tmpmsg;
		}
		args.msglen = len;

		ret = aim_im_sendch1_ext(od->sess, &args);
	}

	g_free(tmpmsg);

	if (ret >= 0)
		return 1;

	return ret;
}

static void oscar_get_info(GaimConnection *g, const char *name) {
	struct oscar_data *od = (struct oscar_data *)g->proto_data;
	if (od->icq)
		aim_icq_getallinfo(od->sess, name);
	else
		/* people want the away message on the top, so we get the away message
		 * first and then get the regular info, since it's too difficult to
		 * insert in the middle. i hate people. */
		aim_getinfo(od->sess, od->conn, name, AIM_GETINFO_AWAYMESSAGE);
}

static void oscar_get_away(GaimConnection *g, const char *who) {
	struct oscar_data *od = (struct oscar_data *)g->proto_data;
	if (od->icq) {
		GaimBuddy *budlight = gaim_find_buddy(g->account, who);
		if (budlight)
			if ((budlight->uc & 0xffff0000) >> 16)
				aim_im_sendch2_geticqaway(od->sess, who, (budlight->uc & 0xffff0000) >> 16);
			else
				gaim_debug(GAIM_DEBUG_ERROR, "oscar",
						   "Error: The user %s has no status message, therefore not requesting.\n", who);
		else
			gaim_debug(GAIM_DEBUG_ERROR, "oscar",
					   "Error: Could not find %s in local contact list, therefore unable to request status message.\n", who);
	} else
		aim_getinfo(od->sess, od->conn, who, AIM_GETINFO_GENERALINFO);
}

static void oscar_set_dir(GaimConnection *g, const char *first, const char *middle, const char *last,
			  const char *maiden, const char *city, const char *state, const char *country, int web) {
	/* XXX - some of these things are wrong, but i'm lazy */
	struct oscar_data *od = (struct oscar_data *)g->proto_data;
	aim_setdirectoryinfo(od->sess, od->conn, first, middle, last,
				maiden, NULL, NULL, city, state, NULL, 0, web);
}

static void oscar_set_idle(GaimConnection *gc, int time) {
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
	aim_bos_setidle(od->sess, od->conn, time);
}

static void oscar_set_info(GaimConnection *gc, const char *text) {
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
	fu32_t flags = 0;
	char *text_html = NULL;
	char *msg = NULL;
	gsize msglen = 0;

	if (od->rights.maxsiglen == 0)
		gaim_notify_warning(gc, NULL, _("Unable to set AIM profile."),
							_("You have probably requested to set your "
							  "profile before the login procedure completed.  "
							  "Your profile remains unset; try setting it "
							  "again when you are fully connected."));

	if (od->icq)
		aim_bos_setprofile(od->sess, od->conn, NULL, NULL, 0, NULL, NULL, 0, caps_icq);
	else {
		if (!text) {
			aim_bos_setprofile(od->sess, od->conn, NULL, NULL, 0, NULL, NULL, 0, caps_aim);
			return;
		}
		
		text_html = strdup_withhtml(text);
		flags = oscar_encoding_check(text_html);
		if (flags & AIM_IMFLAGS_UNICODE) {
			msg = g_convert(text_html, strlen(text_html), "UCS-2BE", "UTF-8", NULL, &msglen, NULL);
			aim_bos_setprofile(od->sess, od->conn, "unicode-2-0", msg, (msglen > od->rights.maxsiglen ? od->rights.maxsiglen : msglen), NULL, NULL, 0, caps_aim);
			g_free(msg);
		} else if (flags & AIM_IMFLAGS_ISO_8859_1) {
			msg = g_convert(text_html, strlen(text_html), "ISO-8859-1", "UTF-8", NULL, &msglen, NULL);
			aim_bos_setprofile(od->sess, od->conn, "iso-8859-1", msg, (msglen > od->rights.maxsiglen ? od->rights.maxsiglen : msglen), NULL, NULL, 0, caps_aim);
			g_free(msg);
		} else {
			msglen = strlen(text_html);
			aim_bos_setprofile(od->sess, od->conn, "us-ascii", text_html, (msglen > od->rights.maxsiglen ? od->rights.maxsiglen : msglen), NULL, NULL, 0, caps_aim);
		}

		if (msglen > od->rights.maxsiglen) {
			gchar *errstr;
			errstr = g_strdup_printf(ngettext("The maximum profile length of %d byte "
									 "has been exceeded.  Gaim has truncated it for you.",
									 "The maximum profile length of %d bytes "
									 "has been exceeded.  Gaim has truncated it for you.",
									 od->rights.maxsiglen), od->rights.maxsiglen);
			gaim_notify_warning(gc, NULL, _("Profile too long."), errstr);
			g_free(errstr);
		}

		g_free(text_html);

	}

	return;
}

static void oscar_set_away_aim(GaimConnection *gc, struct oscar_data *od, const char *text)
{
	fu32_t flags = 0;
	gchar *text_html = NULL;
	char *msg = NULL;
	gsize msglen = 0;

	if (od->rights.maxawaymsglen == 0)
		gaim_notify_warning(gc, NULL, _("Unable to set AIM away message."),
							_("You have probably requested to set your "
							  "away message before the login procedure "
							  "completed.  You remain in a \"present\" "
							  "state; try setting it again when you are "
							  "fully connected."));

	if (gc->away) {
		g_free(gc->away);
		gc->away = NULL;
	}

	if (!text) {
		aim_bos_setprofile(od->sess, od->conn, NULL, NULL, 0, NULL, "", 0, caps_aim);
		return;
	}

	text_html = strdup_withhtml(text);
	flags = oscar_encoding_check(text_html);
	if (flags & AIM_IMFLAGS_UNICODE) {
		msg = g_convert(text_html, strlen(text_html), "UCS-2BE", "UTF-8", NULL, &msglen, NULL);
		aim_bos_setprofile(od->sess, od->conn, NULL, NULL, 0, "unicode-2-0", msg, 
			(msglen > od->rights.maxawaymsglen ? od->rights.maxawaymsglen : msglen), caps_aim);
		g_free(msg);
		gc->away = g_strndup(text, od->rights.maxawaymsglen/2);
	} else if (flags & AIM_IMFLAGS_ISO_8859_1) {
		msg = g_convert(text_html, strlen(text_html), "ISO-8859-1", "UTF-8", NULL, &msglen, NULL);
		aim_bos_setprofile(od->sess, od->conn, NULL, NULL, 0, "iso-8859-1", msg, 
			(msglen > od->rights.maxawaymsglen ? od->rights.maxawaymsglen : msglen), caps_aim);
		g_free(msg);
		gc->away = g_strndup(text_html, od->rights.maxawaymsglen);
	} else {
		msglen = strlen(text_html);
		aim_bos_setprofile(od->sess, od->conn, NULL, NULL, 0, "us-ascii", text_html, 
			(msglen > od->rights.maxawaymsglen ? od->rights.maxawaymsglen : msglen), caps_aim);
		gc->away = g_strndup(text_html, od->rights.maxawaymsglen);
	}

	if (msglen > od->rights.maxawaymsglen) {
		gchar *errstr;

		errstr = g_strdup_printf(ngettext("The maximum away message length of %d byte "
								 "has been exceeded.  Gaim has truncated it for you.",
								 "The maximum away message length of %d bytes "
								 "has been exceeded.  Gaim has truncated it for you.",
								 od->rights.maxawaymsglen), od->rights.maxawaymsglen);
		gaim_notify_warning(gc, NULL, _("Away message too long."), errstr);
		g_free(errstr);
	}
	
	g_free(text_html);
	return;
}

static void oscar_set_away_icq(GaimConnection *gc, struct oscar_data *od, const char *state, const char *message)
{
	GaimAccount *account = gaim_connection_get_account(gc);
	if (gc->away) {
		g_free(gc->away);
		gc->away = NULL;
	}

	if (strcmp(state, _("Invisible"))) {
		if ((od->sess->ssi.received_data) && (aim_ssi_getpermdeny(od->sess->ssi.local) != account->perm_deny))
			aim_ssi_setpermdeny(od->sess, account->perm_deny, 0xffffffff);
		account->perm_deny = 4;
	} else {
		if ((od->sess->ssi.received_data) && (aim_ssi_getpermdeny(od->sess->ssi.local) != 0x03))
			aim_ssi_setpermdeny(od->sess, 0x03, 0xffffffff);
		account->perm_deny = 3;
	}

	if (!strcmp(state, _("Online")))
		aim_setextstatus(od->sess, AIM_ICQ_STATE_NORMAL);
	else if (!strcmp(state, _("Away"))) {
		aim_setextstatus(od->sess, AIM_ICQ_STATE_AWAY);
		gc->away = g_strdup("");
	} else if (!strcmp(state, _("Do Not Disturb"))) {
		aim_setextstatus(od->sess, AIM_ICQ_STATE_AWAY | AIM_ICQ_STATE_DND | AIM_ICQ_STATE_BUSY);
		gc->away = g_strdup("");
	} else if (!strcmp(state, _("Not Available"))) {
		aim_setextstatus(od->sess, AIM_ICQ_STATE_OUT | AIM_ICQ_STATE_AWAY);
		gc->away = g_strdup("");
	} else if (!strcmp(state, _("Occupied"))) {
		aim_setextstatus(od->sess, AIM_ICQ_STATE_AWAY | AIM_ICQ_STATE_BUSY);
		gc->away = g_strdup("");
	} else if (!strcmp(state, _("Free For Chat"))) {
		aim_setextstatus(od->sess, AIM_ICQ_STATE_CHAT);
		gc->away = g_strdup("");
	} else if (!strcmp(state, _("Invisible"))) {
		aim_setextstatus(od->sess, AIM_ICQ_STATE_INVISIBLE);
		gc->away = g_strdup("");
	} else if (!strcmp(state, GAIM_AWAY_CUSTOM)) {
	 	if (message) {
			aim_setextstatus(od->sess, AIM_ICQ_STATE_OUT | AIM_ICQ_STATE_AWAY);
			gc->away = g_strdup("");
		} else {
			aim_setextstatus(od->sess, AIM_ICQ_STATE_NORMAL);
		}
	}

	return;
}

static void oscar_set_away(GaimConnection *gc, const char *state, const char *message)
{
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;

	if (od->icq)
		oscar_set_away_icq(gc, od, state, message);
	else
		oscar_set_away_aim(gc, od, message);

	return;
}

static void oscar_warn(GaimConnection *gc, const char *name, int anon) {
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
	aim_im_warn(od->sess, od->conn, name, anon ? AIM_WARN_ANON : 0);
}

static void oscar_dir_search(GaimConnection *gc, const char *first, const char *middle, const char *last,
			     const char *maiden, const char *city, const char *state, const char *country, const char *email) {
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
	if (strlen(email))
		aim_search_address(od->sess, od->conn, email);
}

static void oscar_add_buddy(GaimConnection *gc, const char *name, GaimGroup *g) {
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
#ifdef NOSSI
	aim_add_buddy(od->sess, od->conn, name);
#else
	if ((od->sess->ssi.received_data) && !(aim_ssi_itemlist_exists(od->sess->ssi.local, name))) {
		GaimBuddy *buddy = gaim_find_buddy(gc->account, name);
		GaimGroup *group = gaim_find_buddys_group(buddy);
		if (buddy && group) {
			gaim_debug(GAIM_DEBUG_INFO, "oscar",
					   "ssi: adding buddy %s to group %s\n", name, group->name);
			aim_ssi_addbuddy(od->sess, buddy->name, group->name, gaim_get_buddy_alias_only(buddy), NULL, NULL, 0);
		}
	}
#endif
	if (od->icq)
		aim_icq_getalias(od->sess, name);
}

static void oscar_add_buddies(GaimConnection *gc, GList *buddies) {
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
#ifdef NOSSI
	char buf[MSG_LEN];
	int n=0;
	while (buddies) {
		if (n > MSG_LEN - 18) {
			aim_bos_setbuddylist(od->sess, od->conn, buf);
			n = 0;
		}
		n += g_snprintf(buf + n, sizeof(buf) - n, "%s&", (char *)buddies->data);
		buddies = buddies->next;
	}
	aim_bos_setbuddylist(od->sess, od->conn, buf);
#else
	if (od->sess->ssi.received_data) {
		while (buddies) {
			GaimBuddy *buddy = gaim_find_buddy(gc->account, (const char *)buddies->data);
			GaimGroup *group = gaim_find_buddys_group(buddy);
			if (buddy && group) {
				gaim_debug(GAIM_DEBUG_INFO, "oscar",
						   "ssi: adding buddy %s to group %s\n", (const char *)buddies->data, group->name);
				aim_ssi_addbuddy(od->sess, buddy->name, group->name, gaim_get_buddy_alias_only(buddy), NULL, NULL, 0);
			}
			buddies = buddies->next;
		}
	}
#endif
}

static void oscar_remove_buddy(GaimConnection *gc, const char *name, const char *group) {
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
#ifdef NOSSI
	aim_remove_buddy(od->sess, od->conn, name);
#else
	if (od->sess->ssi.received_data) {
		gaim_debug(GAIM_DEBUG_INFO, "oscar",
				   "ssi: deleting buddy %s from group %s\n", name, group);
		aim_ssi_delbuddy(od->sess, name, group);
	}
#endif
}

static void oscar_remove_buddies(GaimConnection *gc, GList *buddies, const char *group) {
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
#ifdef NOSSI
	GList *cur;
	for (cur=buddies; cur; cur=cur->next)
		aim_remove_buddy(od->sess, od->conn, cur->data);
#else
	if (od->sess->ssi.received_data) {
		while (buddies) {
			gaim_debug(GAIM_DEBUG_INFO, "oscar",
					   "ssi: deleting buddy %s from group %s\n", (char *)buddies->data, group);
			aim_ssi_delbuddy(od->sess, buddies->data, group);
			buddies = buddies->next;
		}
	}
#endif
}

#ifndef NOSSI
static void oscar_move_buddy(GaimConnection *gc, const char *name, const char *old_group, const char *new_group) {
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
	if (od->sess->ssi.received_data && strcmp(old_group, new_group)) {
		gaim_debug(GAIM_DEBUG_INFO, "oscar",
				   "ssi: moving buddy %s from group %s to group %s\n", name, old_group, new_group);
		aim_ssi_movebuddy(od->sess, old_group, new_group, name);
	}
}

static void oscar_alias_buddy(GaimConnection *gc, const char *name, const char *alias) {
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
	if (od->sess->ssi.received_data) {
		char *gname = aim_ssi_itemlist_findparentname(od->sess->ssi.local, name);
		if (gname) {
			gaim_debug(GAIM_DEBUG_INFO, "oscar",
					   "ssi: changing the alias for buddy %s to %s\n", name, alias);
			aim_ssi_aliasbuddy(od->sess, gname, name, alias);
		}
	}
}

static void oscar_rename_group(GaimConnection *g, const char *old_group, const char *new_group, GList *members) {
	struct oscar_data *od = (struct oscar_data *)g->proto_data;

	if (od->sess->ssi.received_data) {
		if (aim_ssi_itemlist_finditem(od->sess->ssi.local, new_group, NULL, AIM_SSI_TYPE_GROUP)) {
			oscar_remove_buddies(g, members, old_group);
			oscar_add_buddies(g, members);
			gaim_debug(GAIM_DEBUG_INFO, "oscar",
					   "ssi: moved all buddies from group %s to %s\n", old_group, new_group);
		} else {
			aim_ssi_rename_group(od->sess, old_group, new_group);
			gaim_debug(GAIM_DEBUG_INFO, "oscar",
					   "ssi: renamed group %s to %s\n", old_group, new_group);
		}
	}
}

static gboolean gaim_ssi_rerequestdata(gpointer data) {
	aim_session_t *sess = data;
	aim_ssi_reqdata(sess);
	return FALSE;
}

static int gaim_ssi_parseerr(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = gc->proto_data;
	va_list ap;
	fu16_t reason;

	va_start(ap, fr);
	reason = (fu16_t)va_arg(ap, unsigned int);
	va_end(ap);

	gaim_debug(GAIM_DEBUG_ERROR, "oscar", "ssi: SNAC error %hu\n", reason);

	if (reason == 0x0005) {
		gaim_notify_error(gc, NULL, _("Unable To Retrieve Buddy List"),
						  _("Gaim was temporarily unable to retrieve your buddy list from the AIM servers.  Your buddy list is not lost, and will probably become available in a few hours."));
		od->getblisttimer = g_timeout_add(300000, gaim_ssi_rerequestdata, od->sess);
	}

	/* Activate SSI */
	/* Sending the enable causes other people to be able to see you, and you to see them */
	/* Make sure your privacy setting/invisibility is set how you want it before this! */
	gaim_debug(GAIM_DEBUG_INFO, "oscar", "ssi: activating server-stored buddy list\n");
	aim_ssi_enable(od->sess);

	return 1;
}

static int gaim_ssi_parserights(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
	int numtypes, i;
	fu16_t *maxitems;
	va_list ap;

	va_start(ap, fr);
	numtypes = va_arg(ap, int);
	maxitems = va_arg(ap, fu16_t *);
	va_end(ap);

	gaim_debug(GAIM_DEBUG_MISC, "oscar", "ssi rights:");

	for (i=0; i<numtypes; i++)
		gaim_debug(GAIM_DEBUG_MISC, NULL, " max type 0x%04x=%hd,",
				   i, maxitems[i]);

	gaim_debug(GAIM_DEBUG_MISC, NULL, "\n");

	if (numtypes >= 0)
		od->rights.maxbuddies = maxitems[0];
	if (numtypes >= 1)
		od->rights.maxgroups = maxitems[1];
	if (numtypes >= 2)
		od->rights.maxpermits = maxitems[2];
	if (numtypes >= 3)
		od->rights.maxdenies = maxitems[3];

	return 1;
}

static int gaim_ssi_parselist(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	GaimAccount *account = gaim_connection_get_account(gc);
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
	struct aim_ssi_item *curitem;
	int tmp;
	gboolean export = FALSE;
	/* XXX - use these?
	va_list ap;

	va_start(ap, fr);
	fmtver = (fu16_t)va_arg(ap, int);
	numitems = (fu16_t)va_arg(ap, int);
	items = va_arg(ap, struct aim_ssi_item);
	timestamp = va_arg(ap, fu32_t);
	va_end(ap); */

	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "ssi: syncing local list and server list\n");

	/* Clean the buddy list */
	aim_ssi_cleanlist(sess);

	/* Add from server list to local list */
	for (curitem=sess->ssi.local; curitem; curitem=curitem->next) {
		switch (curitem->type) {
			case 0x0000: { /* Buddy */
				if (curitem->name) {
					char *gname = aim_ssi_itemlist_findparentname(sess->ssi.local, curitem->name);
					char *gname_utf8 = gaim_try_conv_to_utf8(gname);
					char *alias = aim_ssi_getalias(sess->ssi.local, gname, curitem->name);
					char *alias_utf8 = gaim_try_conv_to_utf8(alias);
					GaimBuddy *buddy = gaim_find_buddy(gc->account, curitem->name);
					/* Should gname be freed here? -- elb */
					/* Not with the current code, but that might be cleaner -- med */
					free(alias);
					if (buddy) {
						/* Get server stored alias */
						if (alias_utf8) {
							g_free(buddy->alias);
							buddy->alias = g_strdup(alias_utf8);
						}
					} else {
						GaimGroup *g;
						buddy = gaim_buddy_new(gc->account, curitem->name, alias_utf8);

						if (!(g = gaim_find_group(gname_utf8 ? gname_utf8 : _("Orphans")))) {
							g = gaim_group_new(gname_utf8 ? gname_utf8 : _("Orphans"));
							gaim_blist_add_group(g, NULL);
						}

						gaim_debug(GAIM_DEBUG_INFO, "oscar",
								   "ssi: adding buddy %s to group %s to local list\n", curitem->name, gname_utf8 ? gname_utf8 : _("Orphans"));
						gaim_blist_add_buddy(buddy, NULL, g, NULL);
						export = TRUE;
					}
					free(gname_utf8);
					free(alias_utf8);
				}
			} break;

			case 0x0001: { /* Group */
				/* Shouldn't add empty groups */
			} break;

			case 0x0002: { /* Permit buddy */
				if (curitem->name) {
					/* if (!find_permdeny_by_name(gc->permit, curitem->name)) { AAA */
					GSList *list;
					for (list=account->permit; (list && aim_sncmp(curitem->name, list->data)); list=list->next);
					if (!list) {
						gaim_debug(GAIM_DEBUG_INFO, "oscar",
								   "ssi: adding permit buddy %s to local list\n", curitem->name);
						gaim_privacy_permit_add(account, curitem->name, TRUE);
						export = TRUE;
					}
				}
			} break;

			case 0x0003: { /* Deny buddy */
				if (curitem->name) {
					GSList *list;
					for (list=account->deny; (list && aim_sncmp(curitem->name, list->data)); list=list->next);
					if (!list) {
						gaim_debug(GAIM_DEBUG_INFO, "oscar",
								   "ssi: adding deny buddy %s to local list\n", curitem->name);
						gaim_privacy_deny_add(account, curitem->name, TRUE);
						export = TRUE;
					}
				}
			} break;

			case 0x0004: { /* Permit/deny setting */
				if (curitem->data) {
					fu8_t permdeny;
					if ((permdeny = aim_ssi_getpermdeny(sess->ssi.local)) && (permdeny != account->perm_deny)) {
						gaim_debug(GAIM_DEBUG_INFO, "oscar",
								   "ssi: changing permdeny from %d to %hhu\n", account->perm_deny, permdeny);
						account->perm_deny = permdeny;
						if (od->icq && account->perm_deny == 0x03) {
							serv_set_away(gc, "Invisible", "");
						}
						export = TRUE;
					}
				}
			} break;

			case 0x0005: { /* Presence setting */
				/* We don't want to change Gaim's setting because it applies to all accounts */
			} break;
		} /* End of switch on curitem->type */
	} /* End of for loop */

	/* If changes were made, then flush buddy list to file */
	if (export)
		gaim_blist_save();

	{ /* Add from local list to server list */
		GaimBlistNode *gnode, *cnode, *bnode;
		GaimGroup *group;
		GaimBuddy *buddy;
		struct gaim_buddy_list *blist;
		GSList *cur;

		/* Buddies */
		if ((blist = gaim_get_blist()))
			for (gnode = blist->root; gnode; gnode = gnode->next) {
				if(!GAIM_BLIST_NODE_IS_GROUP(gnode))
					continue;
				group = (GaimGroup *)gnode;
				for (cnode = gnode->child; cnode; cnode = cnode->next) {
					if(!GAIM_BLIST_NODE_IS_CONTACT(cnode))
						continue;
					for (bnode = cnode->child; bnode; bnode = bnode->next) {
						if(!GAIM_BLIST_NODE_IS_BUDDY(bnode))
							continue;
						buddy = (GaimBuddy *)bnode;
						if (buddy->account == gc->account) {
							gchar *servernick = gaim_buddy_get_setting(buddy, "servernick");
							if (servernick) {
								serv_got_alias(gc, buddy->name, servernick);
								g_free(servernick);
							}
							if (aim_ssi_itemlist_exists(sess->ssi.local, buddy->name)) {
								/* Store local alias on server */
								char *alias = aim_ssi_getalias(sess->ssi.local, group->name, buddy->name);
								if (!alias && buddy->alias && strlen(buddy->alias))
									aim_ssi_aliasbuddy(sess, group->name, buddy->name, buddy->alias);
								free(alias);
							} else {
								gaim_debug(GAIM_DEBUG_INFO, "oscar",
										"ssi: adding buddy %s from local list to server list\n", buddy->name);
								aim_ssi_addbuddy(sess, buddy->name, group->name, gaim_get_buddy_alias_only(buddy), NULL, NULL, 0);
							}
						}
					}
				}
			}

		/* Permit list */
		if (gc->account->permit) {
			for (cur=gc->account->permit; cur; cur=cur->next)
				if (!aim_ssi_itemlist_finditem(sess->ssi.local, NULL, cur->data, AIM_SSI_TYPE_PERMIT)) {
					gaim_debug(GAIM_DEBUG_INFO, "oscar",
							"ssi: adding permit %s from local list to server list\n", (char *)cur->data);
					aim_ssi_addpermit(sess, cur->data);
				}
		}

		/* Deny list */
		if (gc->account->deny) {
			for (cur=gc->account->deny; cur; cur=cur->next)
				if (!aim_ssi_itemlist_finditem(sess->ssi.local, NULL, cur->data, AIM_SSI_TYPE_DENY)) {
					gaim_debug(GAIM_DEBUG_INFO, "oscar",
							"ssi: adding deny %s from local list to server list\n", (char *)cur->data);
					aim_ssi_adddeny(sess, cur->data);
				}
		}
		/* Presence settings (idle time visibility) */
		if ((tmp = aim_ssi_getpresence(sess->ssi.local)) != 0xFFFFFFFF)
			if (!(tmp & 0x400))
				aim_ssi_setpresence(sess, tmp | 0x400);
	} /* end adding buddies from local list to server list */

	/* Set our ICQ status */
	if  (od->icq && !gc->away) {
		aim_setextstatus(sess, AIM_ICQ_STATE_NORMAL);
	}

	/* Activate SSI */
	/* Sending the enable causes other people to be able to see you, and you to see them */
	/* Make sure your privacy setting/invisibility is set how you want it before this! */
	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "ssi: activating server-stored buddy list\n");
	aim_ssi_enable(sess);

	return 1;
}

static int gaim_ssi_parseack(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	va_list ap;
	struct aim_ssi_tmp *retval;

	va_start(ap, fr);
	retval = va_arg(ap, struct aim_ssi_tmp *);
	va_end(ap);

	while (retval) {
		gaim_debug(GAIM_DEBUG_MISC, "oscar",
				   "ssi: status is 0x%04hx for a 0x%04hx action with name %s\n", retval->ack,  retval->action, retval->item ? (retval->item->name ? retval->item->name : "no name") : "no item");

		if (retval->ack != 0xffff)
		switch (retval->ack) {
			case 0x0000: { /* added successfully */
			} break;

			case 0x000c: { /* you are over the limit, the cheat is to the limit, come on fhqwhgads */
				gchar *buf;
				buf = g_strdup_printf(_("Could not add the buddy %s because you have too many buddies in your buddy list.  Please remove one and try again."), (retval->name ? retval->name : _("(no name)")));
				gaim_notify_error(gc, NULL, _("Unable To Add"), buf);
				g_free(buf);
			}

			case 0x000e: { /* contact requires authorization */
				if ((retval->action == AIM_CB_SSI_ADD) && (retval->name))
					gaim_auth_sendrequest(gc, retval->name);
			} break;

			default: { /* La la la */
				gchar *buf;
				gaim_debug(GAIM_DEBUG_ERROR, "oscar", "ssi: Action 0x%04hx was unsuccessful with error 0x%04hx\n", retval->action, retval->ack);
				buf = g_strdup_printf(_("Could not add the buddy %s for an unknown reason.  The most common reason for this is that you have the maximum number of allowed buddies in your buddy list."), (retval->name ? retval->name : _("(no name)")));
				gaim_notify_error(gc, NULL, _("Unable To Add"), buf);
				g_free(buf);
				/* XXX - Should remove buddy from local list */
			} break;
		}

		retval = retval->next;
	}

	return 1;
}

static int gaim_ssi_authgiven(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	va_list ap;
	char *sn, *msg;
	gchar *dialog_msg, *nombre;
	struct name_data *data;
	GaimBuddy *buddy;

	va_start(ap, fr);
	sn = va_arg(ap, char *);
	msg = va_arg(ap, char *);
	va_end(ap);

	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "ssi: %s has given you permission to add him to your buddy list\n", sn);

	buddy = gaim_find_buddy(gc->account, sn);
	if (buddy && (gaim_get_buddy_alias_only(buddy)))
		nombre = g_strdup_printf("%s (%s)", sn, gaim_get_buddy_alias_only(buddy));
	else
		nombre = g_strdup(sn);

	dialog_msg = g_strdup_printf(_("The user %s has given you permission to add you to their buddy list.  Do you want to add them?"), nombre);
	data = g_new(struct name_data, 1);
	data->gc = gc;
	data->name = g_strdup(sn);
	data->nick = NULL;

	gaim_request_yes_no(gc, NULL, _("Authorization Given"), dialog_msg,
						0, data,
						G_CALLBACK(gaim_icq_contactadd),
						G_CALLBACK(oscar_free_name_data));

	g_free(dialog_msg);
	g_free(nombre);

	return 1;
}

static int gaim_ssi_authrequest(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	va_list ap;
	char *sn, *msg;
	gchar *dialog_msg, *nombre;
	struct name_data *data;
	GaimBuddy *buddy;

	va_start(ap, fr);
	sn = va_arg(ap, char *);
	msg = va_arg(ap, char *);
	va_end(ap);

	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "ssi: received authorization request from %s\n", sn);

	buddy = gaim_find_buddy(gc->account, sn);
	if (buddy && (gaim_get_buddy_alias_only(buddy)))
		nombre = g_strdup_printf("%s (%s)", sn, gaim_get_buddy_alias_only(buddy));
	else
		nombre = g_strdup(sn);

	dialog_msg = g_strdup_printf(_("The user %s wants to add you to their buddy list for the following reason:\n%s"), nombre, msg ? msg : _("No reason given."));
	data = g_new(struct name_data, 1);
	data->gc = gc;
	data->name = g_strdup(sn);
	data->nick = NULL;

	gaim_request_action(gc, NULL, _("Authorization Request"), dialog_msg,
						0, data, 2,
						_("Authorize"), G_CALLBACK(gaim_auth_grant),
						_("Deny"), G_CALLBACK(gaim_auth_dontgrant_msgprompt));

	g_free(dialog_msg);
	g_free(nombre);

	return 1;
}

static int gaim_ssi_authreply(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	va_list ap;
	char *sn, *msg;
	gchar *dialog_msg, *nombre;
	fu8_t reply;
	GaimBuddy *buddy;

	va_start(ap, fr);
	sn = va_arg(ap, char *);
	reply = (fu8_t)va_arg(ap, int);
	msg = va_arg(ap, char *);
	va_end(ap);

	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "ssi: received authorization reply from %s.  Reply is 0x%04hhx\n", sn, reply);

	buddy = gaim_find_buddy(gc->account, sn);
	if (buddy && (gaim_get_buddy_alias_only(buddy)))
		nombre = g_strdup_printf("%s (%s)", sn, gaim_get_buddy_alias_only(buddy));
	else
		nombre = g_strdup(sn);

	if (reply) {
		/* Granted */
		dialog_msg = g_strdup_printf(_("The user %s has granted your request to add them to your contact list."), nombre);
		gaim_notify_info(gc, NULL, _("Authorization Granted"), dialog_msg);
	} else {
		/* Denied */
		dialog_msg = g_strdup_printf(_("The user %s has denied your request to add them to your contact list for the following reason:\n%s"), nombre, msg ? msg : _("No reason given."));
		gaim_notify_info(gc, NULL, _("Authorization Denied"), dialog_msg);
	}
	g_free(dialog_msg);
	g_free(nombre);

	return 1;
}

static int gaim_ssi_gotadded(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	va_list ap;
	char *sn;
	GaimBuddy *buddy;

	va_start(ap, fr);
	sn = va_arg(ap, char *);
	va_end(ap);

	buddy = gaim_find_buddy(gc->account, sn);
	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "ssi: %s added you to their buddy list\n", sn);
	show_got_added(gc, NULL, sn, (buddy ? gaim_get_buddy_alias_only(buddy) : NULL), NULL);

	return 1;
}
#endif

static GList *oscar_chat_info(GaimConnection *gc) {
	GList *m = NULL;
	struct proto_chat_entry *pce;

	pce = g_new0(struct proto_chat_entry, 1);
	pce->label = _("Join what group:");
	pce->identifier = "room";
	m = g_list_append(m, pce);

	pce = g_new0(struct proto_chat_entry, 1);
	pce->label = _("Exchange:");
	pce->identifier = "exchange";
	pce->is_int = TRUE;
	pce->min = 4;
	pce->max = 20;
	m = g_list_append(m, pce);

	return m;
}

static void oscar_join_chat(GaimConnection *g, GHashTable *data) {
	struct oscar_data *od = (struct oscar_data *)g->proto_data;
	aim_conn_t *cur;
	char *name, *exchange;

	name = g_hash_table_lookup(data, "room");
	exchange = g_hash_table_lookup(data, "exchange");

	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "Attempting to join chat room %s.\n", name);
	if ((cur = aim_getconn_type(od->sess, AIM_CONN_TYPE_CHATNAV))) {
		gaim_debug(GAIM_DEBUG_INFO, "oscar",
				   "chatnav exists, creating room\n");
		aim_chatnav_createroom(od->sess, cur, name, atoi(exchange));
	} else {
		/* this gets tricky */
		struct create_room *cr = g_new0(struct create_room, 1);
		gaim_debug(GAIM_DEBUG_INFO, "oscar",
				   "chatnav does not exist, opening chatnav\n");
		cr->exchange = atoi(exchange);
		cr->name = g_strdup(name);
		od->create_rooms = g_slist_append(od->create_rooms, cr);
		aim_reqservice(od->sess, od->conn, AIM_CONN_TYPE_CHATNAV);
	}
}

static void oscar_chat_invite(GaimConnection *g, int id, const char *message, const char *name) {
	struct oscar_data *od = (struct oscar_data *)g->proto_data;
	struct chat_connection *ccon = find_oscar_chat(g, id);
	
	if (!ccon)
		return;
	
	aim_chat_invite(od->sess, od->conn, name, message ? message : "",
			ccon->exchange, ccon->name, 0x0);
}

static void oscar_chat_leave(GaimConnection *g, int id) {
	struct oscar_data *od = g ? (struct oscar_data *)g->proto_data : NULL;
	GSList *bcs = g->buddy_chats;
	GaimConversation *b = NULL;
	struct chat_connection *c = NULL;
	int count = 0;

	while (bcs) {
		count++;
		b = (GaimConversation *)bcs->data;
		if (id == gaim_chat_get_id(GAIM_CHAT(b)))
			break;
		bcs = bcs->next;
		b = NULL;
	}

	if (!b)
		return;

	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "Attempting to leave room %s (currently in %d rooms)\n", b->name, count);
	
	c = find_oscar_chat(g, gaim_chat_get_id(GAIM_CHAT(b)));
	if (c != NULL) {
		if (od)
			od->oscar_chats = g_slist_remove(od->oscar_chats, c);
		if (c->inpa > 0)
			gaim_input_remove(c->inpa);
		if (g && od->sess)
			aim_conn_kill(od->sess, &c->conn);
		g_free(c->name);
		g_free(c->show);
		g_free(c);
	}
	/* we do this because with Oscar it doesn't tell us we left */
	serv_got_chat_left(g, gaim_chat_get_id(GAIM_CHAT(b)));
}

static int oscar_chat_send(GaimConnection *g, int id, const char *message) {
	struct oscar_data *od = (struct oscar_data *)g->proto_data;
	GSList *bcs = g->buddy_chats;
	GaimConversation *b = NULL;
	struct chat_connection *c = NULL;
	char *buf, *buf2;
	int i, j;

	while (bcs) {
		b = (GaimConversation *)bcs->data;
		if (id == gaim_chat_get_id(GAIM_CHAT(b)))
			break;
		bcs = bcs->next;
		b = NULL;
	}
	if (!b)
		return -EINVAL;

	bcs = od->oscar_chats;
	while (bcs) {
		c = (struct chat_connection *)bcs->data;
		if (b == c->cnv)
			break;
		bcs = bcs->next;
		c = NULL;
	}
	if (!c)
		return -EINVAL;

	buf = g_malloc(strlen(message) * 4 + 1);
	for (i = 0, j = 0; i < strlen(message); i++) {
		if (message[i] == '\n') {
			buf[j++] = '<';
			buf[j++] = 'B';
			buf[j++] = 'R';
			buf[j++] = '>';
		} else {
			buf[j++] = message[i];
		}
	}
	buf[j] = '\0';

	if (strlen(buf) > c->maxlen)
		return -E2BIG;

	buf2 = strip_html(buf);
	if (strlen(buf2) > c->maxvis) {
		g_free(buf2);
		return -E2BIG;
	}
	g_free(buf2);

	aim_chat_send_im(od->sess, c->conn, 0, buf, strlen(buf));
	g_free(buf);
	return 0;
}

static const char *oscar_list_icon(GaimAccount *a, GaimBuddy *b) {
	if (!b || (b && b->name && b->name[0] == '+')) {
		if (a != NULL && isdigit(*gaim_account_get_username(a)))
			return "icq";
		else
			return "aim";
	}

	if (b != NULL && isdigit(b->name[0]))
		return "icq";
	return "aim";
}

static void oscar_list_emblems(GaimBuddy *b, char **se, char **sw, char **nw, char **ne)
{
	char *emblems[4] = {NULL,NULL,NULL,NULL};
	int i = 0;

	if (!GAIM_BUDDY_IS_ONLINE(b)) {
		GaimAccount *account;
		GaimConnection *gc;
		struct oscar_data *od;
		char *gname;
		if ((b->name) && (account = b->account) && (gc = account->gc) && 
			(od = gc->proto_data) && (od->sess->ssi.received_data) && 
			(gname = aim_ssi_itemlist_findparentname(od->sess->ssi.local, b->name)) && 
			(aim_ssi_waitingforauth(od->sess->ssi.local, gname, b->name))) {
			emblems[i++] = "notauthorized";
		} else {
			emblems[i++] = "offline";
		}
	}

	if (b->name && (b->uc & 0xffff0000) && isdigit(b->name[0])) {
		int uc = b->uc >> 16;
		if (uc & AIM_ICQ_STATE_INVISIBLE)
			emblems[i++] = "invisible";
		else if (uc & AIM_ICQ_STATE_CHAT)
			emblems[i++] = "freeforchat";
		else if (uc & AIM_ICQ_STATE_DND)
			emblems[i++] = "dnd";
		else if (uc & AIM_ICQ_STATE_OUT)
			emblems[i++] = "na";
		else if (uc & AIM_ICQ_STATE_BUSY)
			emblems[i++] = "occupied";
		else if (uc & AIM_ICQ_STATE_AWAY)
			emblems[i++] = "away";
	} else {
		if (b->uc & UC_UNAVAILABLE) 
			emblems[i++] = "away";
	}
	if (b->uc & UC_WIRELESS)
		emblems[i++] = "wireless";
	if (b->uc & UC_AOL)
		emblems[i++] = "aol";
	if (b->uc & UC_ADMIN)
		emblems[i++] = "admin";
	if (b->uc & UC_AB && i < 4)
		emblems[i++] = "activebuddy";
	if (b->uc & UC_HIPTOP && i < 4)
		emblems[i++] = "hiptop";
/*	if (b->uc & UC_UNCONFIRMED && i < 4)
		emblems[i++] = "unconfirmed"; */
	*se = emblems[0];
	*sw = emblems[1];
	*nw = emblems[2];
	*ne = emblems[3];
}

static char *oscar_tooltip_text(GaimBuddy *b) {
	GaimConnection *gc = b->account->gc;
	struct oscar_data *od = gc->proto_data;
	struct buddyinfo *bi = g_hash_table_lookup(od->buddyinfo, normalize(b->name));
	gchar *tmp, *yay = g_strdup("");

	if (GAIM_BUDDY_IS_ONLINE(b)) {
		if (isdigit(b->name[0])) {
			char *tmp, *status;
			status = gaim_icq_status((b->uc & 0xffff0000) >> 16);
			tmp = yay;
			yay = g_strconcat(tmp, _("<b>Status:</b> "), status, "\n", NULL);
			g_free(tmp);
			g_free(status);
		}

		if (bi) {
			char *tstr = sec_to_text(time(NULL) - bi->signon + 
				(gc->login_time_official ? gc->login_time_official - gc->login_time : 0));
			tmp = yay;
			yay = g_strconcat(tmp, _("<b>Logged In:</b> "), tstr, "\n", NULL);
			free(tmp);
			free(tstr);

			if (bi->ipaddr) {
				char *tstr =  g_strdup_printf("%hhd.%hhd.%hhd.%hhd",
								(bi->ipaddr & 0xff000000) >> 24,
								(bi->ipaddr & 0x00ff0000) >> 16,
								(bi->ipaddr & 0x0000ff00) >> 8,
								(bi->ipaddr & 0x000000ff));
				tmp = yay;
				yay = g_strconcat(tmp, _("<b>IP Address:</b> "), tstr, "\n", NULL);
				free(tmp);
				free(tstr);
			}

			if (bi->caps) {
				char *caps = caps_string(bi->caps);
				tmp = yay;
				yay = g_strconcat(tmp, _("<b>Capabilities:</b> "), caps, "\n", NULL);
				free(tmp);
			}

			if (bi->availmsg && !(b->uc & UC_UNAVAILABLE)) {
				gchar *escaped = g_markup_escape_text(bi->availmsg, strlen(bi->availmsg));
				tmp = yay;
				yay = g_strconcat(tmp, _("<b>Available:</b> "), escaped, "\n", NULL);
				free(tmp);
				g_free(escaped);
			}
		}
	} else {
		char *gname = aim_ssi_itemlist_findparentname(od->sess->ssi.local, b->name);
		if (aim_ssi_waitingforauth(od->sess->ssi.local, gname, b->name)) {
			tmp = yay;
			yay = g_strconcat(tmp, _("<b>Status:</b> Not Authorized"), "\n", NULL);
			g_free(tmp);
		} else {
			tmp = yay;
			yay = g_strconcat(tmp, _("<b>Status:</b> Offline"), "\n", NULL);
			g_free(tmp);
		}
	}

	/* remove the trailing newline character */
	if (yay)
		yay[strlen(yay)-1] = '\0';
	return yay;
}

static char *oscar_status_text(GaimBuddy *b) {
	GaimConnection *gc = b->account->gc;
	struct oscar_data *od = gc->proto_data;
	gchar *ret = NULL;

	if ((b->uc & UC_UNAVAILABLE) || (((b->uc & 0xffff0000) >> 16) & AIM_ICQ_STATE_CHAT)) {
		if (isdigit(b->name[0]))
			ret = gaim_icq_status((b->uc & 0xffff0000) >> 16);
		else
			ret = g_strdup(_("Away"));
	} else if (GAIM_BUDDY_IS_ONLINE(b)) {
		struct buddyinfo *bi = g_hash_table_lookup(od->buddyinfo, normalize(b->name));
		if (bi->availmsg)
			ret = g_markup_escape_text(bi->availmsg, strlen(bi->availmsg));
	} else {
		char *gname = aim_ssi_itemlist_findparentname(od->sess->ssi.local, b->name);
		if (aim_ssi_waitingforauth(od->sess->ssi.local, gname, b->name))
			ret = g_strdup(_("Not Authorized"));
		else
			ret = g_strdup(_("Offline"));
	}

	return ret;
}


static int oscar_icon_req(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = gc->proto_data;
	va_list ap;
	fu16_t type;
	fu8_t flags = 0, length = 0;
	char *md5 = NULL;

	va_start(ap, fr);
	type = va_arg(ap, int);

	switch(type) {
		case 0x0000:
		case 0x0001: {
			flags = va_arg(ap, int);
			length = va_arg(ap, int);
			md5 = va_arg(ap, char *);

			if (flags == 0x41) {
				if (!aim_getconn_type(od->sess, AIM_CONN_TYPE_ICON) && !od->iconconnecting) {
					od->iconconnecting = TRUE;
					od->set_icon = TRUE;
					aim_reqservice(od->sess, od->conn, AIM_CONN_TYPE_ICON);
				} else {
					struct stat st;
					const char *iconfile = gaim_account_get_buddy_icon(gaim_connection_get_account(gc));
					if (iconfile == NULL) {
						/* Set an empty icon, or something */
					} else if (!stat(iconfile, &st)) {
						char *buf = g_malloc(st.st_size);
						FILE *file = fopen(iconfile, "rb");
						if (file) {
							fread(buf, 1, st.st_size, file);
							fclose(file);
							gaim_debug(GAIM_DEBUG_INFO, "oscar",
								   "Uploading icon to icon server\n");
							aim_bart_upload(od->sess, buf, st.st_size);
						} else
							gaim_debug(GAIM_DEBUG_ERROR, "oscar",
								   "Can't open buddy icon file!\n");
						g_free(buf);
					} else {
						gaim_debug(GAIM_DEBUG_ERROR, "oscar",
							   "Can't stat buddy icon file!\n");
					}
				}
			} else if (flags == 0x81)
				aim_ssi_seticon(od->sess, md5, length); 
		} break;

		case 0x0002: { /* We just set an "available" message? */
		} break;
	}

	va_end(ap);

	return 0;
}

/*
 * We have just established a socket with the other dude, so set up some handlers.
 */
static int gaim_odc_initiate(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
	GaimConversation *cnv;
	struct direct_im *dim;
	char buf[256];
	char *sn;
	va_list ap;
	aim_conn_t *newconn, *listenerconn;

	va_start(ap, fr);
	newconn = va_arg(ap, aim_conn_t *);
	listenerconn = va_arg(ap, aim_conn_t *);
	va_end(ap);

	aim_conn_close(listenerconn);
	aim_conn_kill(sess, &listenerconn);

	sn = g_strdup(aim_odc_getsn(newconn));

	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "DirectIM: initiate success to %s\n", sn);
	dim = find_direct_im(od, sn);

	cnv = gaim_conversation_new(GAIM_CONV_IM, dim->gc->account, sn);
	gaim_input_remove(dim->watcher);
	dim->conn = newconn;
	dim->watcher = gaim_input_add(dim->conn->fd, GAIM_INPUT_READ, oscar_callback, dim->conn);
	dim->connected = TRUE;
	g_snprintf(buf, sizeof buf, _("Direct IM with %s established"), sn);
	g_free(sn);
	gaim_conversation_write(cnv, NULL, buf, GAIM_MESSAGE_SYSTEM, time(NULL));

	aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMINCOMING, gaim_odc_incoming, 0);
	aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMTYPING, gaim_odc_typing, 0);
	aim_conn_addhandler(sess, newconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_IMAGETRANSFER, gaim_odc_update_ui, 0);

	return 1;
}

/*
 * This is called when each chunk of an image is received.  It can be used to 
 * update a progress bar, or to eat lots of dry cat food.  Wet cat food is 
 * nasty, you sicko.
 */
static int gaim_odc_update_ui(aim_session_t *sess, aim_frame_t *fr, ...) {
	va_list ap;
	char *sn;
	double percent;
	GaimConnection *gc = sess->aux_data;
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
	GaimConversation *c;
	struct direct_im *dim;

	va_start(ap, fr);
	sn = va_arg(ap, char *);
	percent = va_arg(ap, double);
	va_end(ap);

	if (!(dim = find_direct_im(od, sn)))
		return 1;
	if (dim->watcher) {
		gaim_input_remove(dim->watcher);   /* Otherwise, the callback will callback */
		dim->watcher = 0;
	}
	/* XXX is this really necessary? */
	while (gtk_events_pending())
		gtk_main_iteration();

	c = gaim_find_conversation_with_account(sn, gaim_connection_get_account(gc));
	if (c != NULL)
		gaim_conversation_update_progress(c, percent);
	dim->watcher = gaim_input_add(dim->conn->fd, GAIM_INPUT_READ,
				      oscar_callback, dim->conn);

	return 1;
}

/*
 * This is called after a direct IM has been received in its entirety.  This 
 * function is passed a long chunk of data which contains the IM with any 
 * data chunks (images) appended to it.
 *
 * This function rips out all the data chunks and creates an imgstore for 
 * each one.  In order to do this, it first goes through the IM and takes 
 * out all the IMG tags.  When doing so, it rewrites the original IMG tag 
 * with one compatable with the imgstore Gaim core code. For each one, we 
 * then read in chunks of data from the end of the message and actually 
 * create the img store using the given data.
 *
 * For somewhat easy reference, here's a sample message
 * (without the whitespace and asterisks):
 *
 * <HTML><BODY BGCOLOR="#ffffff">
 *     <FONT LANG="0">
 *     This is a really stupid picture:<BR>
 *     <IMG SRC="Sample.jpg" ID="1" WIDTH="283" HEIGHT="212" DATASIZE="9894"><BR>
 *     Yeah it is<BR>
 *     Here is another one:<BR>
 *     <IMG SRC="Soap Bubbles.bmp" ID="2" WIDTH="256" HEIGHT="256" DATASIZE="65978">   
 *     </FONT>
 * </BODY></HTML>
 * <BINARY>
 *     <DATA ID="1" SIZE="9894">datadatadatadata</DATA>
 *     <DATA ID="2" SIZE="65978">datadatadatadata</DATA>
 * </BINARY>
 */
static int gaim_odc_incoming(aim_session_t *sess, aim_frame_t *fr, ...) {
	GaimConnection *gc = sess->aux_data;
	GaimImFlags imflags = 0;
	GString *newmsg = g_string_new("");
	GSList *images = NULL;
	va_list ap;
	const char *sn, *msg, *msgend, *binary;
	size_t len;
	int encoding, isawaymsg;

	va_start(ap, fr);
	sn = va_arg(ap, const char *);
	msg = va_arg(ap, const char *);
	len = va_arg(ap, size_t);
	encoding = va_arg(ap, int);
	isawaymsg = va_arg(ap, int);
	va_end(ap);
	msgend = msg + len;

	gaim_debug(GAIM_DEBUG_INFO, "oscar",
			   "Got DirectIM message from %s\n", sn);

	if (isawaymsg)
		imflags |= GAIM_IM_AUTO_RESP;

	/* message has a binary trailer */
	if ((binary = gaim_strcasestr(msg, "<binary>"))) {
		GData *attribs;
		const char *tmp, *start, *end, *last = NULL;

		tmp = msg;

		/* for each valid image tag... */
		while (gaim_markup_find_tag("img", tmp, &start, &end, &attribs)) {
			const char *id, *src, *datasize;
			const char *tag = NULL, *data = NULL;
			size_t size;
			int imgid = 0;

			/* update the location of the last img tag */
			last = end;

			/* grab attributes */
			id       = g_datalist_get_data(&attribs, "id");
			src      = g_datalist_get_data(&attribs, "src");
			datasize = g_datalist_get_data(&attribs, "datasize");

			/* if we have id & datasize, build the data tag */
			if (id && datasize)
				tag = g_strdup_printf("<data id=\"%s\" size=\"%s\">", id, datasize);

			/* if we have a tag, find the start of the data */
			if (tag && (data = gaim_strcasestr(binary, tag)))
				data += strlen(tag);

			/* check the data is here and store it */
			if (data + (size = atoi(datasize)) <= msgend)
				imgid = gaim_imgstore_add(data, size, src);

			/* if we have a stored image... */
			if (imgid) {
				/* append the message up to the tag */
				newmsg = g_string_append_len(newmsg, tmp, start - tmp);

				/* write the new image tag */
				g_string_append_printf(newmsg, "<IMG ID=\"%d\">", imgid);

				/* and record the image number */
				images = g_slist_append(images, GINT_TO_POINTER(imgid));
			} else {
				/* otherwise, copy up to the end of the tag */
				newmsg = g_string_append_len(newmsg, tmp, (end + 1) - tmp);
			}

			/* clear the attribute list */
			g_datalist_clear(&attribs);

			/* continue from the end of the tag */
			tmp = end + 1;
		}

		/* append any remaining message data (without the > :-) */
		if (last++ && (last < binary))
			newmsg = g_string_append_len(newmsg, last, binary - last);

		/* set the flag if we caught any images */
		if (images)
			imflags |= GAIM_IM_IMAGES;
	} else {
		g_string_append_len(newmsg, msg, len);
	}

	/* XXX - I imagine Paco-Paco will want to do some voodoo with the encoding here */
	serv_got_im(gc, sn, newmsg->str, imflags, time(NULL));

	/* free up the message */
	g_string_free(newmsg, TRUE);

	/* unref any images we allocated */
	if (images) {
		GSList *tmp;
		int id;

		for (tmp = images; tmp != NULL; tmp = tmp->next) {
			id = GPOINTER_TO_INT(tmp->data);
			gaim_imgstore_unref(id);
		}

		g_slist_free(images);
	}

	return 1;
}

static int gaim_odc_typing(aim_session_t *sess, aim_frame_t *fr, ...) {
	va_list ap;
	char *sn;
	int typing;
	GaimConnection *gc = sess->aux_data;

	va_start(ap, fr);
	sn = va_arg(ap, char *);
	typing = va_arg(ap, int);
	va_end(ap);

	if (typing == 0x0002) {
		/* I had to leave this. It's just too funny. It reminds me of my sister. */
		gaim_debug(GAIM_DEBUG_INFO, "oscar",
				   "ohmigod! %s has started typing (DirectIM). He's going to send you a message! *squeal*\n", sn);
		serv_got_typing(gc, sn, 0, GAIM_TYPING);
	} else if (typing == 0x0001)
		serv_got_typing(gc, sn, 0, GAIM_TYPED);
	else
		serv_got_typing_stopped(gc, sn);
	return 1;
}

static int gaim_odc_send_im(aim_session_t *sess, aim_conn_t *conn, const char *message, GaimImFlags imflags) {
	char *buf;
	size_t len;
	int ret;

	if (imflags & GAIM_IM_IMAGES) {
		GString *msg = g_string_new("");
		GString *data = g_string_new("<BINARY>");
		GData *attribs;
		const char *tmp, *start, *end, *last = NULL;
		int oscar_id = 0;

		tmp = message;

		/* for each valid IMG tag... */
		while (gaim_markup_find_tag("img", tmp, &start, &end, &attribs)) {
			GaimStoredImage *image = NULL;
			const char *id;

			last = end;
			id = g_datalist_get_data(&attribs, "id");

			/* ... if it refers to a valid gaim image ... */
			if (id && (image = gaim_imgstore_get(atoi(id)))) {
				/* ... append the message from start to the tag ... */
				msg = g_string_append_len(msg, tmp, start - tmp);
				oscar_id++;

				/* ... insert a new img tag with the oscar id ... */
				if (image->filename)
					g_string_append_printf(msg,
						"<IMG SRC=\"file://%s\" ID=\"%d\" DATASIZE=\"%d\">",
						image->filename, oscar_id, image->size);
				else
					g_string_append_printf(msg,
						"<IMG ID=\"%d\" DATASIZE=\"%d\">",
						oscar_id, image->size);

				/* ... and append the data to the binary section ... */
				g_string_append_printf(data, "<DATA ID=\"%d\" SIZE=\"%d\">",
					oscar_id, image->size);
				data = g_string_append_len(data, image->data, image->size);
				data = g_string_append(data, "</DATA>");
			} else {
				/* ... otherwise, allow the possibly invalid img tag through. */
				/* should we do something else? */
				msg = g_string_append_len(msg, tmp, (end + 1) - tmp);
			}

			g_datalist_clear(&attribs);

			/* continue from the end of the tag */
			tmp = end + 1;
		}

		/* append any remaining message data (without the > :-) */
		if (last++ && *last)
			msg = g_string_append(msg, last);

		/* if we inserted any images in the binary section, append it */
		if (oscar_id) {
			msg = g_string_append_len(msg, data->str, data->len);
			msg = g_string_append(msg, "</BINARY>");
		}

		len = msg->len;
		buf = msg->str;
		g_string_free(msg, FALSE);
		g_string_free(data, TRUE);
	} else {
		len = strlen(message);
		buf = g_memdup(message, len+1);
	}

	/* XXX - The last parameter below is the encoding.  Let Paco-Paco do something with it. */
	if (imflags & GAIM_IM_AUTO_RESP)
		ret =  aim_odc_send_im(sess, conn, buf, len, 0, 1);
	else
		ret =  aim_odc_send_im(sess, conn, buf, len, 0, 0);

	g_free(buf);

	return ret;
}

struct ask_do_dir_im {
	char *who;
	GaimConnection *gc;
};

static void oscar_cancel_direct_im(struct ask_do_dir_im *data) {
	g_free(data->who);
	g_free(data);
}

static void oscar_direct_im(struct ask_do_dir_im *data) {
	GaimConnection *gc = data->gc;
	struct oscar_data *od;
	struct direct_im *dim;

	if (!g_list_find(gaim_connections_get_all(), gc)) {
		g_free(data->who);
		g_free(data);
		return;
	}

	od = (struct oscar_data *)gc->proto_data;

	dim = find_direct_im(od, data->who);
	if (dim) {
		if (!(dim->connected)) {  /* We'll free the old, unconnected dim, and start over */
			od->direct_ims = g_slist_remove(od->direct_ims, dim);
			gaim_input_remove(dim->watcher);
			g_free(dim);
			gaim_debug(GAIM_DEBUG_INFO, "oscar",
					   "Gave up on old direct IM, trying again\n");
		} else {
			gaim_notify_error(gc, NULL, "DirectIM already open.", NULL);
			g_free(data->who);
			g_free(data);
			return;
		}
	}
	dim = g_new0(struct direct_im, 1);
	dim->gc = gc;
	g_snprintf(dim->name, sizeof dim->name, "%s", data->who);

	dim->conn = aim_odc_initiate(od->sess, data->who);
	if (dim->conn != NULL) {
		od->direct_ims = g_slist_append(od->direct_ims, dim);
		dim->watcher = gaim_input_add(dim->conn->fd, GAIM_INPUT_READ,
						oscar_callback, dim->conn);
		aim_conn_addhandler(od->sess, dim->conn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIM_ESTABLISHED,
					gaim_odc_initiate, 0);
	} else {
		gaim_notify_error(gc, NULL, _("Unable to open Direct IM"), NULL);
		g_free(dim);
	}

	g_free(data->who);
	g_free(data);
}

static void oscar_ask_direct_im(GaimConnection *gc, const char *who) {
	gchar *buf;
	struct ask_do_dir_im *data = g_new0(struct ask_do_dir_im, 1);
	data->who = g_strdup(who);
	data->gc = gc;
	buf = g_strdup_printf(_("You have selected to open a Direct IM connection with %s."), who);

	gaim_request_action(gc, NULL, buf,
						_("Because this reveals your IP address, it "
						  "may be considered a privacy risk.  Do you "
						  "wish to continue?"),
						0, data, 2,
						_("Connect"), G_CALLBACK(oscar_direct_im),
						_("Cancel"), G_CALLBACK(oscar_cancel_direct_im));
	g_free(buf);
}

static void oscar_set_permit_deny(GaimConnection *gc) {
	GaimAccount *account = gaim_connection_get_account(gc);
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
#ifdef NOSSI
	GSList *list, *g = gaim_blist_groups(), *g1;
	char buf[MAXMSGLEN];
	int at;

	switch(account->perm_deny) {
	case 1: 
		aim_bos_changevisibility(od->sess, od->conn, AIM_VISIBILITYCHANGE_DENYADD, gaim_account_get_username(account));
		break;
	case 2:
		aim_bos_changevisibility(od->sess, od->conn, AIM_VISIBILITYCHANGE_PERMITADD, gaim_account_get_username(account));
		break;
	case 3:
		list = account->permit;
		at = 0;
		while (list) {
			at += g_snprintf(buf + at, sizeof(buf) - at, "%s&", (char *)list->data);
			list = list->next;
		}
		aim_bos_changevisibility(od->sess, od->conn, AIM_VISIBILITYCHANGE_PERMITADD, buf);
		break;
	case 4:
		list = account->deny;
		at = 0;
		while (list) {
			at += g_snprintf(buf + at, sizeof(buf) - at, "%s&", (char *)list->data);
			list = list->next;
		}
		aim_bos_changevisibility(od->sess, od->conn, AIM_VISIBILITYCHANGE_DENYADD, buf);
		break;
	case 5:
		g1 = g;
		at = 0;
		while (g1) {
			list = gaim_blist_members((struct group *)g->data);
			GSList list1 = list;
			while (list1) {
				struct buddy *b = list1->data;
				if(b->account == account)
					at += g_snprintf(buf + at, sizeof(buf) - at, "%s&", b->name);
				list1 = list1->next;
			}
			g_slist_free(list);
			g1 = g1->next;
		}
		g_slist_free(g);
		aim_bos_changevisibility(od->sess, od->conn, AIM_VISIBILITYCHANGE_PERMITADD, buf);
		break;
	default:
		break;
	}
	signoff_blocked(gc);
#else
	if (od->sess->ssi.received_data)
		aim_ssi_setpermdeny(od->sess, account->perm_deny, 0xffffffff);
#endif
}

static void oscar_add_permit(GaimConnection *gc, const char *who) {
#ifdef NOSSI
	if (gc->account->permdeny == 3)
		oscar_set_permit_deny(gc);
#else
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
	gaim_debug(GAIM_DEBUG_INFO, "oscar", "ssi: About to add a permit\n");
	if (od->sess->ssi.received_data)
		aim_ssi_addpermit(od->sess, who);
#endif
}

static void oscar_add_deny(GaimConnection *gc, const char *who) {
#ifdef NOSSI
	if (gc->account->permdeny == 4)
		oscar_set_permit_deny(gc);
#else
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
	gaim_debug(GAIM_DEBUG_INFO, "oscar", "ssi: About to add a deny\n");
	if (od->sess->ssi.received_data)
		aim_ssi_adddeny(od->sess, who);
#endif
}

static void oscar_rem_permit(GaimConnection *gc, const char *who) {
#ifdef NOSSI
	if (gc->account->permdeny == 3)
		oscar_set_permit_deny(gc);
#else
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
	gaim_debug(GAIM_DEBUG_INFO, "oscar", "ssi: About to delete a permit\n");
	if (od->sess->ssi.received_data)
		aim_ssi_delpermit(od->sess, who);
#endif
}

static void oscar_rem_deny(GaimConnection *gc, const char *who) {
#ifdef NOSSI
	if (gc->account->permdeny == 4)
		oscar_set_permit_deny(gc);
#else
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;
	gaim_debug(GAIM_DEBUG_INFO, "oscar", "ssi: About to delete a deny\n");
	if (od->sess->ssi.received_data)
		aim_ssi_deldeny(od->sess, who);
#endif
}

static GList *oscar_away_states(GaimConnection *gc)
{
	struct oscar_data *od = gc->proto_data;
	GList *m = NULL;

	if (!od->icq)
		return g_list_append(m, GAIM_AWAY_CUSTOM);

	m = g_list_append(m, _("Online"));
	m = g_list_append(m, _("Away"));
	m = g_list_append(m, _("Do Not Disturb"));
	m = g_list_append(m, _("Not Available"));
	m = g_list_append(m, _("Occupied"));
	m = g_list_append(m, _("Free For Chat"));
	m = g_list_append(m, _("Invisible"));

	return m;
}

static GList *oscar_buddy_menu(GaimConnection *gc, const char *who) {
	struct oscar_data *od = gc->proto_data;
	GList *m = NULL;
	struct proto_buddy_menu *pbm;

	if (od->icq) {
#if 0
		pbm = g_new0(struct proto_buddy_menu, 1);
		pbm->label = _("Get Status Msg");
		pbm->callback = oscar_get_icqstatusmsg;
		pbm->gc = gc;
		m = g_list_append(m, pbm);
#endif
	} else {
		GaimBuddy *b = gaim_find_buddy(gc->account, who);
		struct buddyinfo *bi;

		if (b)
			bi = g_hash_table_lookup(od->buddyinfo, normalize(b->name));

		if (b && bi && aim_sncmp(gaim_account_get_username(gaim_connection_get_account(gc)), who) && GAIM_BUDDY_IS_ONLINE(b)) {
			if (bi->caps & AIM_CAPS_DIRECTIM) {
				pbm = g_new0(struct proto_buddy_menu, 1);
				pbm->label = _("Direct IM");
				pbm->callback = oscar_ask_direct_im;
				pbm->gc = gc;
				m = g_list_append(m, pbm);
			}

			if (bi->caps & AIM_CAPS_SENDFILE) {
				pbm = g_new0(struct proto_buddy_menu, 1);
				pbm->label = _("Send File");
				pbm->callback = oscar_ask_sendfile;
				pbm->gc = gc;
				m = g_list_append(m, pbm);
			}
#if 0
			if (bi->caps & AIM_CAPS_GETFILE) {
				pbm = g_new0(struct proto_buddy_menu, 1);
				pbm->label = _("Get File");
				pbm->callback = oscar_ask_getfile;
				pbm->gc = gc;
				m = g_list_append(m, pbm);
			}
#endif
		}
	}

	if (od->sess->ssi.received_data) {
		char *gname = aim_ssi_itemlist_findparentname(od->sess->ssi.local, who);
		if (gname && aim_ssi_waitingforauth(od->sess->ssi.local, gname, who)) {
			pbm = g_new0(struct proto_buddy_menu, 1);
			pbm->label = _("Re-request Authorization");
			pbm->callback = gaim_auth_sendrequest;
			pbm->gc = gc;
			m = g_list_append(m, pbm);
		}
	}
	
	return m;
}

static void oscar_format_screenname(GaimConnection *gc, const char *nick) {
	struct oscar_data *od = gc->proto_data;
	if (!aim_sncmp(gaim_account_get_username(gaim_connection_get_account(gc)), nick)) {
		if (!aim_getconn_type(od->sess, AIM_CONN_TYPE_AUTH)) {
			od->setnick = TRUE;
			od->newsn = g_strdup(nick);
			aim_reqservice(od->sess, od->conn, AIM_CONN_TYPE_AUTH);
		} else {
			aim_admin_setnick(od->sess, aim_getconn_type(od->sess, AIM_CONN_TYPE_AUTH), nick);
		}
	} else {
		gaim_notify_error(gc, NULL, _("The new formatting is invalid."),
						  _("Screenname formatting can change only capitalization and whitespace."));
	}
}

static void oscar_show_format_screenname(GaimConnection *gc)
{
	gaim_request_input(gc, NULL, _("New screenname formatting:"), NULL,
					   gaim_connection_get_display_name(gc), FALSE, FALSE,
					   _("OK"), G_CALLBACK(oscar_format_screenname),
					   _("Cancel"), NULL,
					   gc);
}

static void oscar_confirm_account(GaimConnection *gc)
{
	struct oscar_data *od = gc->proto_data;
	aim_conn_t *conn = aim_getconn_type(od->sess, AIM_CONN_TYPE_AUTH);

	if (conn) {
		aim_admin_reqconfirm(od->sess, conn);
	} else {
		od->conf = TRUE;
		aim_reqservice(od->sess, od->conn, AIM_CONN_TYPE_AUTH);
	}
}

static void oscar_show_email(GaimConnection *gc)
{
	struct oscar_data *od = gc->proto_data;
	aim_conn_t *conn = aim_getconn_type(od->sess, AIM_CONN_TYPE_AUTH);

	if (conn) {
		aim_admin_getinfo(od->sess, conn, 0x11);
	} else {
		od->reqemail = TRUE;
		aim_reqservice(od->sess, od->conn, AIM_CONN_TYPE_AUTH);
	}
}

static void oscar_change_email(GaimConnection *gc, const char *email)
{
	struct oscar_data *od = gc->proto_data;
	aim_conn_t *conn = aim_getconn_type(od->sess, AIM_CONN_TYPE_AUTH);

	if (conn) {
		aim_admin_setemail(od->sess, conn, email);
	} else {
		od->setemail = TRUE;
		od->email = g_strdup(email);
		aim_reqservice(od->sess, od->conn, AIM_CONN_TYPE_AUTH);
	}
}

static void oscar_show_change_email(GaimConnection *gc)
{
	gaim_request_input(gc, NULL, _("Change Address To:"), NULL, NULL,
					   FALSE, FALSE,
					   _("OK"), G_CALLBACK(oscar_change_email),
					   _("Cancel"), NULL,
					   gc);
}

static void oscar_show_awaitingauth(GaimConnection *gc)
{
	struct oscar_data *od = gc->proto_data;
	gchar *nombre, *text, *tmp;
	GaimBlistNode *gnode, *cnode, *bnode;
	int num=0;

	text = g_strdup("");

	for (gnode = gaim_get_blist()->root; gnode; gnode = gnode->next) {
		GaimGroup *group = (GaimGroup *)gnode;
		if(!GAIM_BLIST_NODE_IS_GROUP(gnode))
			continue;
		for (cnode = gnode->child; cnode; cnode = cnode->next) {
			if(!GAIM_BLIST_NODE_IS_CONTACT(cnode))
				continue;
			for (bnode = cnode->child; bnode; bnode = bnode->next) {
				GaimBuddy *buddy = (GaimBuddy *)bnode;
				if(!GAIM_BLIST_NODE_IS_BUDDY(bnode))
					continue;
				if (buddy->account == gc->account && aim_ssi_waitingforauth(od->sess->ssi.local, group->name, buddy->name)) {
					if (gaim_get_buddy_alias_only(buddy))
						nombre = g_strdup_printf(" %s (%s)", buddy->name, gaim_get_buddy_alias_only(buddy));
					else
						nombre = g_strdup_printf(" %s", buddy->name);
					tmp = g_strdup_printf("%s%s<br>", text, nombre);
					g_free(text);
					text = tmp;
					g_free(nombre);
					num++;
				}
			}
		}
	}

	if (!num) {
		g_free(text);
		text = g_strdup(_("<i>you are not waiting for authorization</i>"));
	}

	gaim_notify_formatted(gc, NULL, _("You are awaiting authorization from "
						  "the following buddies"),	_("You can re-request "
						  "authorization from these buddies by "
						  "right-clicking on them and selecting "
						  "\"Re-request Authorization.\""), text, NULL, NULL);
	g_free(text);
}

#if 0
static void oscar_setavailmsg(GaimConnection *gc, char *text) {
	struct oscar_data *od = (struct oscar_data *)gc->proto_data;

	aim_srv_setavailmsg(od->sess, text);
}

static void oscar_show_setavailmsg(GaimConnection *gc)
{
	gaim_request_input(gc, NULL, _("Available Message:"),
					   NULL, _("Please talk to me, I'm lonely! (and single)"), TRUE, FALSE,
					   _("OK"), G_CALLBACK(oscar_setavailmsg),
					   _("Cancel"), NULL,
					   gc);
}
#endif

static void oscar_show_chpassurl(GaimConnection *gc)
{
	struct oscar_data *od = gc->proto_data;
	gchar *substituted = gaim_strreplace(od->sess->authinfo->chpassurl, "%s", gaim_account_get_username(gaim_connection_get_account(gc)));
	gaim_notify_uri(gc, substituted);
	g_free(substituted);
}

static void oscar_set_icon(GaimConnection *gc, const char *iconfile)
{
	struct oscar_data *od = gc->proto_data;
	aim_session_t *sess = od->sess;
	FILE *file;
	struct stat st;

	if (iconfile == NULL) {
		/* Set an empty icon, or something */
	} else if (!stat(iconfile, &st)) {
		char *buf = g_malloc(st.st_size);
		file = fopen(iconfile, "rb");
		if (file) {
			md5_state_t *state;
			char md5[16];
			int len = fread(buf, 1, st.st_size, file);
			fclose(file);
			state = g_malloc(sizeof(md5_state_t));
			md5_init(state);
			md5_append(state, buf, len);
			md5_finish(state, md5);
			g_free(state);
			aim_ssi_seticon(sess, md5, 16);
		} else
			gaim_debug(GAIM_DEBUG_ERROR, "oscar",
				   "Can't open buddy icon file!\n");
		g_free(buf);
	} else
		gaim_debug(GAIM_DEBUG_ERROR, "oscar",
			   "Can't stat buddy icon file!\n");
}


static GList *oscar_actions(GaimConnection *gc)
{
	struct oscar_data *od = gc->proto_data;
	struct proto_actions_menu *pam;
	GList *m = NULL;

	pam = g_new0(struct proto_actions_menu, 1);
	pam->label = _("Set User Info");
	pam->callback = show_set_info;
	pam->gc = gc;
	m = g_list_append(m, pam);

#if 0
	pam = g_new0(struct proto_actions_menu, 1);
	pam->label = _("Set Available Message");
	pam->callback = oscar_show_setavailmsg;
	pam->gc = gc;
	m = g_list_append(m, pam);
#endif

	pam = g_new0(struct proto_actions_menu, 1);
	pam->label = _("Change Password");
	pam->callback = show_change_passwd;
	pam->gc = gc;
	m = g_list_append(m, pam);

	if (od->sess->authinfo->chpassurl) {
		pam = g_new0(struct proto_actions_menu, 1);
		pam->label = _("Change Password (URL)");
		pam->callback = oscar_show_chpassurl;
		pam->gc = gc;
		m = g_list_append(m, pam);
	}

	if (!od->icq) {
		/* AIM actions */
		m = g_list_append(m, NULL);

		pam = g_new0(struct proto_actions_menu, 1);
		pam->label = _("Format Screenname");
		pam->callback = oscar_show_format_screenname;
		pam->gc = gc;
		m = g_list_append(m, pam);

		pam = g_new0(struct proto_actions_menu, 1);
		pam->label = _("Confirm Account");
		pam->callback = oscar_confirm_account;
		pam->gc = gc;
		m = g_list_append(m, pam);

		pam = g_new0(struct proto_actions_menu, 1);
		pam->label = _("Display Current Registered Address");
		pam->callback = oscar_show_email;
		pam->gc = gc;
		m = g_list_append(m, pam);

		pam = g_new0(struct proto_actions_menu, 1);
		pam->label = _("Change Current Registered Address");
		pam->callback = oscar_show_change_email;
		pam->gc = gc;
		m = g_list_append(m, pam);
	}

	m = g_list_append(m, NULL);

	pam = g_new0(struct proto_actions_menu, 1);
	pam->label = _("Show Buddies Awaiting Authorization");
	pam->callback = oscar_show_awaitingauth;
	pam->gc = gc;
	m = g_list_append(m, pam);

	m = g_list_append(m, NULL);

	pam = g_new0(struct proto_actions_menu, 1);
	pam->label = _("Search for Buddy by Email");
	pam->callback = show_find_email;
	pam->gc = gc;
	m = g_list_append(m, pam);

/*	pam = g_new0(struct proto_actions_menu, 1);
	pam->label = _("Search for Buddy by Information");
	pam->callback = show_find_info;
	pam->gc = gc;
	m = g_list_append(m, pam); */

	return m;
}

static void oscar_change_passwd(GaimConnection *gc, const char *old, const char *new)
{
	struct oscar_data *od = gc->proto_data;

	if (od->icq) {
		aim_icq_changepasswd(od->sess, new);
	} else {
		aim_conn_t *conn = aim_getconn_type(od->sess, AIM_CONN_TYPE_AUTH);
		if (conn) {
			aim_admin_changepasswd(od->sess, conn, new, old);
		} else {
			od->chpass = TRUE;
			od->oldp = g_strdup(old);
			od->newp = g_strdup(new);
			aim_reqservice(od->sess, od->conn, AIM_CONN_TYPE_AUTH);
		}
	}
}

static void oscar_convo_closed(GaimConnection *gc, const char *who)
{
	struct oscar_data *od = gc->proto_data;
	struct direct_im *dim = find_direct_im(od, who);

	if (!dim)
		return;

	od->direct_ims = g_slist_remove(od->direct_ims, dim);
	gaim_input_remove(dim->watcher);
	aim_conn_kill(od->sess, &dim->conn);
	g_free(dim);
}

static GaimPluginProtocolInfo prpl_info =
{
	GAIM_PROTO_OSCAR,
	OPT_PROTO_MAIL_CHECK | OPT_PROTO_BUDDY_ICON | OPT_PROTO_IM_IMAGE,
	NULL,
	NULL,
	oscar_list_icon,
	oscar_list_emblems,
	oscar_status_text,
	oscar_tooltip_text,
	oscar_away_states,
	oscar_actions,
	oscar_buddy_menu,
	oscar_chat_info,
	oscar_login,
	oscar_close,
	oscar_send_im,
	oscar_set_info,
	oscar_send_typing,
	oscar_get_info,
	oscar_set_away,
	oscar_get_away,
	oscar_set_dir,
	NULL,
	oscar_dir_search,
	oscar_set_idle,
	oscar_change_passwd,
	oscar_add_buddy,
	oscar_add_buddies,
	oscar_remove_buddy,
	oscar_remove_buddies,
	oscar_add_permit,
	oscar_add_deny,
	oscar_rem_permit,
	oscar_rem_deny,
	oscar_set_permit_deny,
	oscar_warn,
	oscar_join_chat,
	oscar_chat_invite,
	oscar_chat_leave,
	NULL,
	oscar_chat_send,
	oscar_keepalive,
	NULL,
	NULL,
	NULL,
#ifndef NOSSI
	oscar_alias_buddy,
	oscar_move_buddy,
	oscar_rename_group,
#else
	NULL,
	NULL,
	NULL,
#endif
	NULL,
	oscar_convo_closed,
	NULL,
	oscar_set_icon
};

static GaimPluginInfo info =
{
	2,                                                /**< api_version    */
	GAIM_PLUGIN_PROTOCOL,                             /**< type           */
	NULL,                                             /**< ui_requirement */
	0,                                                /**< flags          */
	NULL,                                             /**< dependencies   */
	GAIM_PRIORITY_DEFAULT,                            /**< priority       */

	"prpl-oscar",                                     /**< id             */
	"AIM/ICQ",                                        /**< name           */
	VERSION,                                          /**< version        */
	                                                  /**  summary        */
	N_("AIM/ICQ Protocol Plugin"),
	                                                  /**  description    */
	N_("AIM/ICQ Protocol Plugin"),
	NULL,                                             /**< author         */
	GAIM_WEBSITE,                                     /**< homepage       */

	NULL,                                             /**< load           */
	NULL,                                             /**< unload         */
	NULL,                                             /**< destroy        */

	NULL,                                             /**< ui_info        */
	&prpl_info                                        /**< extra_info     */
};

static void
init_plugin(GaimPlugin *plugin)
{
	GaimAccountOption *option;

	option = gaim_account_option_string_new(_("Auth host"), "server",
											"login.oscar.aol.com");
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,
											   option);

	option = gaim_account_option_int_new(_("Auth port"), "port", 5190);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,
											   option);

	my_protocol = plugin;
}

GAIM_INIT_PLUGIN(oscar, init_plugin, info);
