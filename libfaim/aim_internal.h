/*
 * aim_internal.h -- prototypes/structs for the guts of libfaim
 *
 */

#ifdef FAIM_INTERNAL
#ifndef __AIM_INTERNAL_H__
#define __AIM_INTERNAL_H__ 1

typedef struct {
	fu16_t family;
	fu16_t subtype;
	fu16_t flags;
	fu32_t id;
} aim_modsnac_t;

#define AIM_MODULENAME_MAXLEN 16
#define AIM_MODFLAG_MULTIFAMILY 0x0001
typedef struct aim_module_s {
	fu16_t family;
	fu16_t version;
	fu16_t toolid;
	fu16_t toolversion;
	fu16_t flags;
	char name[AIM_MODULENAME_MAXLEN+1];
	int (*snachandler)(aim_session_t *sess, struct aim_module_s *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs);
	int (*snacdestructor)(aim_session_t *sess, aim_conn_t *conn, aim_modsnac_t *snac, void *data);

	void (*shutdown)(aim_session_t *sess, struct aim_module_s *mod);
	void *priv;
	struct aim_module_s *next;
} aim_module_t;

faim_internal int aim__registermodule(aim_session_t *sess, int (*modfirst)(aim_session_t *, aim_module_t *));
faim_internal void aim__shutdownmodules(aim_session_t *sess);
faim_internal aim_module_t *aim__findmodulebygroup(aim_session_t *sess, fu16_t group);
faim_internal aim_module_t *aim__findmodule(aim_session_t *sess, const char *name);

faim_internal int buddylist_modfirst(aim_session_t *sess, aim_module_t *mod);
faim_internal int admin_modfirst(aim_session_t *sess, aim_module_t *mod);
faim_internal int bos_modfirst(aim_session_t *sess, aim_module_t *mod);
faim_internal int search_modfirst(aim_session_t *sess, aim_module_t *mod);
faim_internal int stats_modfirst(aim_session_t *sess, aim_module_t *mod);
faim_internal int auth_modfirst(aim_session_t *sess, aim_module_t *mod);
faim_internal int msg_modfirst(aim_session_t *sess, aim_module_t *mod);
faim_internal int misc_modfirst(aim_session_t *sess, aim_module_t *mod);
faim_internal int chatnav_modfirst(aim_session_t *sess, aim_module_t *mod);
faim_internal int chat_modfirst(aim_session_t *sess, aim_module_t *mod);
faim_internal int locate_modfirst(aim_session_t *sess, aim_module_t *mod);
faim_internal int general_modfirst(aim_session_t *sess, aim_module_t *mod);
faim_internal int ssi_modfirst(aim_session_t *sess, aim_module_t *mod);
faim_internal int invite_modfirst(aim_session_t *sess, aim_module_t *mod);
faim_internal int translate_modfirst(aim_session_t *sess, aim_module_t *mod);
faim_internal int popups_modfirst(aim_session_t *sess, aim_module_t *mod);
faim_internal int adverts_modfirst(aim_session_t *sess, aim_module_t *mod);
faim_internal int icq_modfirst(aim_session_t *sess, aim_module_t *mod);
faim_internal int email_modfirst(aim_session_t *sess, aim_module_t *mod);
faim_internal int newsearch_modfirst(aim_session_t *sess, aim_module_t *mod);

faim_internal int aim_genericreq_n(aim_session_t *, aim_conn_t *conn, fu16_t family, fu16_t subtype);
faim_internal int aim_genericreq_n_snacid(aim_session_t *, aim_conn_t *conn, fu16_t family, fu16_t subtype);
faim_internal int aim_genericreq_l(aim_session_t *, aim_conn_t *conn, fu16_t family, fu16_t subtype, fu32_t *);
faim_internal int aim_genericreq_s(aim_session_t *, aim_conn_t *conn, fu16_t family, fu16_t subtype, fu16_t *);

#define AIMBS_CURPOSPAIR(x) ((x)->data + (x)->offset), ((x)->len - (x)->offset)

/* bstream.c */
faim_internal int aim_bstream_init(aim_bstream_t *bs, fu8_t *data, int len);
faim_internal int aim_bstream_empty(aim_bstream_t *bs);
faim_internal int aim_bstream_curpos(aim_bstream_t *bs);
faim_internal int aim_bstream_setpos(aim_bstream_t *bs, int off);
faim_internal void aim_bstream_rewind(aim_bstream_t *bs);
faim_internal int aim_bstream_advance(aim_bstream_t *bs, int n);
faim_internal fu8_t aimbs_get8(aim_bstream_t *bs);
faim_internal fu16_t aimbs_get16(aim_bstream_t *bs);
faim_internal fu32_t aimbs_get32(aim_bstream_t *bs);
faim_internal fu8_t aimbs_getle8(aim_bstream_t *bs);
faim_internal fu16_t aimbs_getle16(aim_bstream_t *bs);
faim_internal fu32_t aimbs_getle32(aim_bstream_t *bs);
faim_internal int aimbs_put8(aim_bstream_t *bs, fu8_t v);
faim_internal int aimbs_put16(aim_bstream_t *bs, fu16_t v);
faim_internal int aimbs_put32(aim_bstream_t *bs, fu32_t v);
faim_internal int aimbs_putle8(aim_bstream_t *bs, fu8_t v);
faim_internal int aimbs_putle16(aim_bstream_t *bs, fu16_t v);
faim_internal int aimbs_putle32(aim_bstream_t *bs, fu32_t v);
faim_internal int aimbs_getrawbuf(aim_bstream_t *bs, fu8_t *buf, int len);
faim_internal fu8_t *aimbs_getraw(aim_bstream_t *bs, int len);
faim_internal char *aimbs_getstr(aim_bstream_t *bs, int len);
faim_internal int aimbs_putraw(aim_bstream_t *bs, const fu8_t *v, int len);
faim_internal int aimbs_putbs(aim_bstream_t *bs, aim_bstream_t *srcbs, int len);

/* conn.c */
faim_internal aim_conn_t *aim_cloneconn(aim_session_t *sess, aim_conn_t *src);

/* ft.c */
faim_internal int aim_rxdispatch_rendezvous(aim_session_t *sess, aim_frame_t *fr);

/* rxhandlers.c */
faim_internal aim_rxcallback_t aim_callhandler(aim_session_t *sess, aim_conn_t *conn, u_short family, u_short type);
faim_internal int aim_callhandler_noparam(aim_session_t *sess, aim_conn_t *conn, fu16_t family, fu16_t type, aim_frame_t *ptr);
faim_internal int aim_parse_unknown(aim_session_t *, aim_frame_t *, ...);
faim_internal void aim_clonehandlers(aim_session_t *sess, aim_conn_t *dest, aim_conn_t *src);

/* rxqueue.c */
faim_internal int aim_recv(int fd, void *buf, size_t count);
faim_internal int aim_bstream_recv(aim_bstream_t *bs, int fd, size_t count);
faim_internal void aim_rxqueue_cleanbyconn(aim_session_t *sess, aim_conn_t *conn);
faim_internal void aim_frame_destroy(aim_frame_t *);

/* txqueue.c */
faim_internal aim_frame_t *aim_tx_new(aim_session_t *sess, aim_conn_t *conn, fu8_t framing, fu16_t chan, int datalen);
faim_internal int aim_tx_enqueue(aim_session_t *, aim_frame_t *);
faim_internal flap_seqnum_t aim_get_next_txseqnum(aim_conn_t *);
faim_internal int aim_tx_sendframe(aim_session_t *sess, aim_frame_t *cur);
faim_internal void aim_tx_cleanqueue(aim_session_t *, aim_conn_t *);

/* XXX - What is this?   faim_internal int aim_tx_printqueue(aim_session_t *); */

/*
 * Generic SNAC structure.  Rarely if ever used.
 */
typedef struct aim_snac_s {
	aim_snacid_t id;
	fu16_t family;
	fu16_t type;
	fu16_t flags;
	void *data;
	time_t issuetime;
	struct aim_snac_s *next;
} aim_snac_t;

struct aim_snac_destructor {
	aim_conn_t *conn;
	void *data;
};

/* snac.c */
faim_internal void aim_initsnachash(aim_session_t *sess);
faim_internal aim_snacid_t aim_newsnac(aim_session_t *, aim_snac_t *newsnac);
faim_internal aim_snacid_t aim_cachesnac(aim_session_t *sess, const fu16_t family, const fu16_t type, const fu16_t flags, const void *data, const int datalen);
faim_internal aim_snac_t *aim_remsnac(aim_session_t *, aim_snacid_t id);
faim_internal void aim_cleansnacs(aim_session_t *, int maxage);
faim_internal int aim_putsnac(aim_bstream_t *, fu16_t family, fu16_t type, fu16_t flags, aim_snacid_t id);

/* Stored in ->priv of the service request SNAC for chats. */
struct chatsnacinfo {
	fu16_t exchange;
	char name[128];
	fu16_t instance;
};

/* these are used by aim_*_clientready */
#define AIM_TOOL_JAVA   0x0001
#define AIM_TOOL_MAC    0x0002
#define AIM_TOOL_WIN16  0x0003
#define AIM_TOOL_WIN32  0x0004
#define AIM_TOOL_MAC68K 0x0005
#define AIM_TOOL_MACPPC 0x0006
#define AIM_TOOL_NEWWIN 0x0010
struct aim_tool_version {
	fu16_t group;
	fu16_t version;
	fu16_t tool;
	fu16_t toolversion;
};

/* 
 * In SNACland, the terms 'family' and 'group' are synonymous -- the former
 * is my term, the latter is AOL's.
 */
struct snacgroup {
	fu16_t group;
	struct snacgroup *next;
};

#ifdef FAIM_NEED_CONN_INTERNAL
struct snacpair {
	fu16_t group;
	fu16_t subtype;
	struct snacpair *next;
};

struct rateclass {
	fu16_t classid;
	fu32_t windowsize;
	fu32_t clear;
	fu32_t alert;
	fu32_t limit;
	fu32_t disconnect;
	fu32_t current;
	fu32_t max;
	fu8_t unknown[5]; /* only present in versions >= 3 */
	struct snacpair *members;
	struct rateclass *next;
};
#endif /* FAIM_NEED_CONN_INTERNAL */

/*
 * This is inside every connection.  But it is a void * to anything
 * outside of libfaim.  It should remain that way.  It's called data
 * abstraction.  Maybe you've heard of it.  (Probably not if you're a 
 * libfaim user.)
 * 
 */
typedef struct aim_conn_inside_s {
	struct snacgroup *groups;
	struct rateclass *rates;
} aim_conn_inside_t;

faim_internal void aim_conn_addgroup(aim_conn_t *conn, fu16_t group);

faim_internal fu32_t aim_getcap(aim_session_t *sess, aim_bstream_t *bs, int len);
faim_internal int aim_putcap(aim_bstream_t *bs, fu32_t caps);

faim_internal int aim_cachecookie(aim_session_t *sess, aim_msgcookie_t *cookie);
faim_internal aim_msgcookie_t *aim_uncachecookie(aim_session_t *sess, fu8_t *cookie, int type);
faim_internal aim_msgcookie_t *aim_mkcookie(fu8_t *, int, void *);
faim_internal aim_msgcookie_t *aim_checkcookie(aim_session_t *, const unsigned char *, const int);
faim_internal int aim_freecookie(aim_session_t *sess, aim_msgcookie_t *cookie);
faim_internal int aim_msgcookie_gettype(int reqclass);
faim_internal int aim_cookie_free(aim_session_t *sess, aim_msgcookie_t *cookie);

faim_internal int aim_extractuserinfo(aim_session_t *sess, aim_bstream_t *bs, aim_userinfo_t *);
faim_internal int aim_putuserinfo(aim_bstream_t *bs, aim_userinfo_t *info);

faim_internal int aim_chat_readroominfo(aim_bstream_t *bs, struct aim_chat_roominfo *outinfo);

faim_internal void faimdprintf(aim_session_t *sess, int dlevel, const char *format, ...);

faim_internal int aim_request_directim(aim_session_t *sess, const char *destsn, fu8_t *ip, fu16_t port, fu8_t *ckret);
faim_internal int aim_request_sendfile(aim_session_t *sess, const char *sn, const char *filename, fu16_t numfiles, fu32_t totsize, fu8_t *ip, fu16_t port, fu8_t *ckret);
faim_internal void aim_conn_close_rend(aim_session_t *sess, aim_conn_t *conn);
faim_internal void aim_conn_kill_rend(aim_session_t *sess, aim_conn_t *conn);

faim_internal void aim_conn_kill_chat(aim_session_t *sess, aim_conn_t *conn);

/* These are all handled internally now. */
faim_internal int aim_setversions(aim_session_t *sess, aim_conn_t *conn);
faim_internal int aim_reqrates(aim_session_t *, aim_conn_t *);
faim_internal int aim_rates_addparam(aim_session_t *, aim_conn_t *);
faim_internal int aim_rates_delparam(aim_session_t *, aim_conn_t *);

#ifndef FAIM_INTERNAL_INSANE
#define printf() printf called inside libfaim
#define sprintf() unbounded sprintf used inside libfaim
#endif

#endif /* __AIM_INTERNAL_H__ */
#endif /* FAIM_INTERNAL */
