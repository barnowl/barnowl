#include "owl.h"

/**********************************************************************/

struct owlfaim_priv {
  char *screenname;
  char *password;
  char *server;
  int connected;
};

static const char *msgerrreasons[] = {
	"Invalid error",
	"Invalid SNAC",
	"Rate to host",
	"Rate to client",
	"Not logged on",
	"Service unavailable",
	"Service not defined",
	"Obsolete SNAC",
	"Not supported by host",
	"Not supported by client",
	"Refused by client",
	"Reply too big",
	"Responses lost",
	"Request denied",
	"Busted SNAC payload",
	"Insufficient rights",
	"In local permit/deny",
	"Too evil (sender)",
	"Too evil (receiver)",
	"User temporarily unavailable",
	"No match",
	"List overflow",
	"Request ambiguous",
	"Queue full",
	"Not while on AOL",
};
static int msgerrreasonslen = 25;

static void faimtest_debugcb(aim_session_t *sess, int level, const char *format, va_list va);
static int faimtest_parse_login(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_parse_authresp(aim_session_t *sess, aim_frame_t *fr, ...);
int faimtest_flapversion(aim_session_t *sess, aim_frame_t *fr, ...);
int faimtest_conncomplete(aim_session_t *sess, aim_frame_t *fr, ...);
void addcb_bos(aim_session_t *sess, aim_conn_t *bosconn);
static int conninitdone_bos(aim_session_t *sess, aim_frame_t *fr, ...);
static int conninitdone_admin(aim_session_t *sess, aim_frame_t *fr, ...);
static int conninitdone_chat     (aim_session_t *, aim_frame_t *, ...);
int logout(aim_session_t *sess);

static int faimtest_parse_connerr(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_accountconfirm(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_infochange(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_handleredirect(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_icbmparaminfo(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_parse_buddyrights(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_bosrights(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_locrights(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_reportinterval(aim_session_t *sess, aim_frame_t *fr, ...);
/* static int reportinterval(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs); */
static int faimtest_parse_motd(aim_session_t *sess, aim_frame_t *fr, ...);
/* static void printuserflags(fu16_t flags); */
static int faimtest_parse_userinfo(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_parse_incoming_im_chan1(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *userinfo, struct aim_incomingim_ch1_args *args);
static int faimtest_parse_incoming_im_chan2(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *userinfo, struct aim_incomingim_ch2_args *args);
static int faimtest_parse_incoming_im(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_parse_oncoming(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_parse_offgoing(aim_session_t *sess, aim_frame_t *fr, ...);
int faimtest_parse_genericerr(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_parse_msgerr(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_parse_locerr(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_parse_misses(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_parse_msgack(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_parse_ratechange(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_parse_evilnotify(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_parse_searchreply(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_parse_searcherror(aim_session_t *sess, aim_frame_t *fr, ...);
static int handlepopup(aim_session_t *sess, aim_frame_t *fr, ...);
static int serverpause(aim_session_t *sess, aim_frame_t *fr, ...);
static int migrate(aim_session_t *sess, aim_frame_t *fr, ...);
static int ssirights(aim_session_t *sess, aim_frame_t *fr, ...);
static int ssidata(aim_session_t *sess, aim_frame_t *fr, ...);
static int ssidatanochange(aim_session_t *sess, aim_frame_t *fr, ...);
static int offlinemsg(aim_session_t *sess, aim_frame_t *fr, ...);
static int offlinemsgdone(aim_session_t *sess, aim_frame_t *fr, ...);
/*
static int faimtest_ssi_parseerr     (aim_session_t *, aim_frame_t *, ...);
static int faimtest_ssi_parserights  (aim_session_t *, aim_frame_t *, ...);
static int faimtest_ssi_parselist    (aim_session_t *, aim_frame_t *, ...);
static int faimtest_ssi_parseack     (aim_session_t *, aim_frame_t *, ...);
static int faimtest_ssi_authgiven    (aim_session_t *, aim_frame_t *, ...);
static int faimtest_ssi_authrequest  (aim_session_t *, aim_frame_t *, ...);
static int faimtest_ssi_authreply    (aim_session_t *, aim_frame_t *, ...);
static int faimtest_ssi_gotadded     (aim_session_t *, aim_frame_t *, ...);
*/

void chatnav_redirect(aim_session_t *sess, struct aim_redirect_data *redir);
void chat_redirect(aim_session_t *sess, struct aim_redirect_data *redir);

/*****************************************************************/

void owl_aim_init(void)
{
  /* this has all been moved to owl_aim_login, but we'll leave the
   * function here, in case there's stuff we want to init in the
   * future.  It's still called by Owl.
   */

}

gboolean owl_aim_send_nop(gpointer data) {
  owl_global *g = data;
  if (owl_global_is_doaimevents(g)) {
    aim_session_t *sess = owl_global_get_aimsess(g);
    aim_flap_nop(sess, aim_getconn_type(sess, AIM_CONN_TYPE_BOS));
  }
  return TRUE;
}


int owl_aim_login(const char *screenname, const char *password)
{
  struct owlfaim_priv *priv;
  aim_conn_t *conn;
  aim_session_t *sess;

  sess=owl_global_get_aimsess(&g);

  aim_session_init(sess, TRUE, 0);
  aim_setdebuggingcb(sess, faimtest_debugcb);
  aim_tx_setenqueue(sess, AIM_TX_IMMEDIATE, NULL);

  /* this will leak, I know and just don't care right now */
  priv=g_new0(struct owlfaim_priv, 1);

  priv->screenname = g_strdup(screenname);
  priv->password = g_strdup(password);
  priv->server = g_strdup(FAIM_LOGIN_SERVER);
  sess->aux_data = priv;

  conn=aim_newconn(sess, AIM_CONN_TYPE_AUTH, priv->server ? priv->server : FAIM_LOGIN_SERVER);
  /*  conn=aim_newconn(sess, AIM_CONN_TYPE_AUTH, NULL); */
  if (!conn) {
    owl_function_error("owl_aim_login: connection error during AIM login\n");
    owl_global_set_aimnologgedin(&g);
    owl_global_set_no_doaimevents(&g);
    return (-1);
  }

  /*
  else if (conn->fd == -1) {
    if (conn->status & AIM_CONN_STATUS_RESOLVERR) {
      owl_function_error("owl_aim_login: could not resolve authorize name");
    } else if (conn->status & AIM_CONN_STATUS_CONNERR) {
      owl_function_error("owl_aim_login: could not connect to authorizer");
    } else {
      owl_function_error("owl_aim_login: unknown connection error");
    }
    owl_global_set_aimnologgedin(&g);
    owl_global_set_no_doaimevents(&g);
    aim_conn_kill(sess, &conn);
    return(-1);
  }
  */


  aim_conn_addhandler(sess, conn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_FLAPVER, faimtest_flapversion, 0);
  aim_conn_addhandler(sess, conn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNCOMPLETE, faimtest_conncomplete, 0);

  aim_conn_addhandler(sess, conn, AIM_CB_FAM_ATH, AIM_CB_ATH_AUTHRESPONSE, faimtest_parse_login, 0);
  aim_conn_addhandler(sess, conn, AIM_CB_FAM_ATH, AIM_CB_ATH_LOGINRESPONSE, faimtest_parse_authresp, 0);
  /* aim_conn_addhandler(sess, conn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNERR, gaim_connerr, 0); */
  /* aim_conn_addhandler(sess, conn, 0x0017, 0x0007, gaim_parse_login, 0); */
  /* aim_conn_addhandler(sess, conn, 0x0017, 0x0003, gaim_parse_auth_resp, 0); */

  /* start processing AIM events */
  owl_global_set_doaimevents(&g);
  /* conn->status |= AIM_CONN_STATUS_INPROGRESS; */
  owl_function_debugmsg("owl_aim_login: sending login request for %s", screenname);
  aim_request_login(sess, conn, screenname);
  owl_function_debugmsg("owl_aim_login: connecting");

  g.aim_nop_timer = g_timeout_add_seconds(30, owl_aim_send_nop, &g);

  return(0);
}

static gboolean owl_aim_unset_ignorelogin(void *data)
{
  owl_global *g = data;
  owl_global_unset_ignore_aimlogin(g);
  return FALSE;  /* only run once. */
}

/* stuff to run once login has been successful */
void owl_aim_successful_login(const char *screenname)
{
  char *buff;
  owl_function_debugmsg("doing owl_aim_successful_login");
  owl_global_set_aimloggedin(&g, screenname);
  owl_global_set_doaimevents(&g); /* this should already be on */
  owl_function_makemsg("%s logged in", screenname);
  buff=g_strdup_printf("Logged in to AIM as %s", screenname);
  owl_function_adminmsg("", buff);
  g_free(buff);

  owl_function_debugmsg("Successful AIM login for %s", screenname);

  /* start the ingorelogin timer */
  owl_global_set_ignore_aimlogin(&g);
  g_timeout_add_seconds(owl_global_get_aim_ignorelogin_timer(&g),
                        owl_aim_unset_ignorelogin, &g);

  /* aim_ssi_setpresence(owl_global_get_aimsess(&g), 0x00000400); */
  /* aim_bos_setidle(owl_global_get_aimsess(&g), owl_global_get_bosconn(&g), 5000); */
}

void owl_aim_logout(void)
{
  /* need to check if it's connected first, I think */
  logout(owl_global_get_aimsess(&g));

  if (owl_global_is_aimloggedin(&g)) owl_function_adminmsg("", "Logged out of AIM");
  owl_global_set_aimnologgedin(&g);
  owl_global_set_no_doaimevents(&g);
  if (g.aim_nop_timer) {
    g_source_remove(g.aim_nop_timer);
    g.aim_nop_timer = 0;
  }
}

void owl_aim_logged_out(void)
{
  if (owl_global_is_aimloggedin(&g)) owl_function_adminmsg("", "Logged out of AIM");
  owl_aim_logout();
}

void owl_aim_login_error(const char *message)
{
  if (message) {
    owl_function_error("%s", message);
  } else {
    owl_function_error("Authentication error on login");
  }
  owl_function_beep();
  owl_global_set_aimnologgedin(&g);
  owl_global_set_no_doaimevents(&g);
  if (g.aim_nop_timer) {
    g_source_remove(g.aim_nop_timer);
    g.aim_nop_timer = 0;
  }
}

/*
 * I got these constants by skimming libfaim/im.c
 *
 * "UNICODE" actually means "UCS-2BE".
 */
#define AIM_CHARSET_ISO_8859_1         0x0003
#define AIM_CHARSET_UNICODE            0x0002

static int owl_aim_do_send(const char *to, const char *msg, int flags)
{
  int ret;
  char *encoded;
  struct aim_sendimext_args args;
    gsize len;

  encoded = g_convert(msg, -1, "ISO-8859-1", "UTF-8", NULL, &len, NULL);
  if (encoded) {
    owl_function_debugmsg("Encoded outgoing AIM as ISO-8859-1");
    args.charset = AIM_CHARSET_ISO_8859_1;
    args.charsubset = 0;
    args.flags = AIM_IMFLAGS_ISO_8859_1;
  } else {
    owl_function_debugmsg("Encoding outgoing IM as UCS-2BE");
    encoded = g_convert(msg, -1, "UCS-2BE", "UTF-8", NULL, &len, NULL);
    if (!encoded) {
      /*
       * TODO: Strip or HTML-encode characters, or figure out how to
       * send in a differen charset.
       */
      owl_function_error("Unable to encode outgoing AIM message in UCS-2");
      return 1;
    }

    args.charset = AIM_CHARSET_UNICODE;
    args.charsubset = 0;
    args.flags = AIM_IMFLAGS_UNICODE;
  }

  args.destsn = to;
  args.msg = encoded;
  args.msglen = len;
  args.flags |= flags;

  ret=aim_im_sendch1_ext(owl_global_get_aimsess(&g), &args);

  g_free(encoded);

  return(ret);
}

int owl_aim_send_im(const char *to, const char *msg)
{
  return owl_aim_do_send(to, msg, 0);
}

int owl_aim_send_awaymsg(const char *to, const char *msg)
{
  return owl_aim_do_send(to, msg, AIM_IMFLAGS_AWAY);
}

void owl_aim_addbuddy(const char *name)
{

  aim_ssi_addbuddy(owl_global_get_aimsess(&g), name, "Buddies", NULL, NULL, NULL, 0);

  /*
  aim_ssi_addbuddy(owl_global_get_aimsess(&g),
		   name,
		   "Buddies",
		   NULL, NULL, NULL,
		   aim_ssi_waitingforauth(owl_global_get_aimsess(&g)->ssi.local, "Buddies", name));
  */
}

void owl_aim_delbuddy(const char *name)
{
  aim_ssi_delbuddy(owl_global_get_aimsess(&g), name, "Buddies");
  owl_buddylist_offgoing(owl_global_get_buddylist(&g), name);
}

void owl_aim_search(const char *email)
{
  int ret;

  owl_function_debugmsg("owl_aim_search: doing search for %s", email);
  ret=aim_search_address(owl_global_get_aimsess(&g),
			 aim_getconn_type(owl_global_get_aimsess(&g), AIM_CONN_TYPE_BOS),
			 email);

  if (ret) owl_function_error("owl_aim_search: aim_search_address returned %i", ret);
}


int owl_aim_set_awaymsg(const char *msg)
{
  int len;
  char *foo;
  /* there is a max away message lentgh we should check against */

  foo=g_strdup(msg);
  len=strlen(foo);
  if (len>500) {
    foo[500]='\0';
    len=499;
  }

  aim_locate_setprofile(owl_global_get_aimsess(&g),
			NULL, NULL, 0,
			"us-ascii", foo, len);
  g_free(foo);

  /*
  aim_bos_setprofile(owl_global_get_aimsess(&g),
		     owl_global_get_bosconn(&g),
		     NULL, NULL, 0, "us-ascii", msg,
		     strlen(msg), 0);
  */
  return(0);
}

void owl_aim_chat_join(const char *name, int exchange)
{
  int ret;
  aim_conn_t *cur;
  /*
  OscarData *od = g->proto_data;
  const char *name, *exchange;
  */

  owl_function_debugmsg("Attempting to join chatroom %s exchange %i", name, exchange);

  /*
  name = g_hash_table_lookup(data, "room");
  exchange = g_hash_table_lookup(data, "exchange");
  */
  if ((cur = aim_getconn_type(owl_global_get_aimsess(&g), AIM_CONN_TYPE_CHATNAV))) {
    owl_function_debugmsg("owl_aim_chat_join: chatnav exists, creating room");
    aim_chatnav_createroom(owl_global_get_aimsess(&g), cur, name, exchange);
  } else {
    /*    struct create_room *cr = g_new0(struct create_room, 1); */
    owl_function_debugmsg("owl_aim_chat_join: chatnav does not exist, opening chatnav");
    /*
    cr->exchange = atoi(exchange);
    cr->name = g_strdup(name);
    od->create_rooms = g_slist_append(od->create_rooms, cr);
    */
    aim_reqservice(owl_global_get_aimsess(&g),
		   aim_getconn_type(owl_global_get_aimsess(&g), AIM_CONN_TYPE_CHATNAV),
		   AIM_CONN_TYPE_CHATNAV);
    aim_reqservice(owl_global_get_aimsess(&g), NULL, AIM_CONN_TYPE_CHATNAV);
    aim_chatnav_createroom(owl_global_get_aimsess(&g), cur, name, exchange);
    ret=aim_chat_join(owl_global_get_aimsess(&g), owl_global_get_bosconn(&g), exchange, name, 0x0000);

  }
  return;
  /******/


  /* ret=aim_chat_join(owl_global_get_aimsess(&g), owl_global_get_bosconn(&g), exchange, chatroom, 0x0000); */
  /*
  ret=aim_chat_join(owl_global_get_aimsess(&g),
		    aim_getconn_type(owl_global_get_aimsess(&g), AIM_CONN_TYPE_CHATNAV), exchange, chatroom, 0x0000);
  */

  aim_reqservice(owl_global_get_aimsess(&g), owl_global_get_bosconn(&g), AIM_CONN_TYPE_CHATNAV);
  ret = aim_chatnav_createroom(owl_global_get_aimsess(&g),
			       aim_getconn_type(owl_global_get_aimsess(&g), AIM_CONN_TYPE_CHATNAV), name, exchange);
   ret=aim_chat_join(owl_global_get_aimsess(&g), owl_global_get_bosconn(&g), exchange, name, 0x0000);

}

void owl_aim_chat_leave(const char *chatroom)
{
}

int owl_aim_chat_sendmsg(const char *chatroom, const char *msg)
{
  return(0);
}

/* caller must free the return */
CALLER_OWN char *owl_aim_normalize_screenname(const char *in)
{
  char *out;
  int i, j, k;

  j=strlen(in);
  out=g_new(char, j+30);
  k=0;
  for (i=0; i<j; i++) {
    if (in[i]!=' ') {
      out[k]=in[i];
      k++;
    }
  }
  out[k]='\0';
  return(out);
}

int owl_aim_process_events(aim_session_t *aimsess)
{
  aim_conn_t *waitingconn = NULL;
  struct timeval tv;
  int selstat = 0;
  struct owlfaim_priv *priv;

  priv = aimsess->aux_data;

  /* do a select without blocking */
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  waitingconn = aim_select(aimsess, &tv, &selstat);

  if (selstat == -1) {
    owl_aim_logged_out();
  } else if (selstat == 0) {
    /* no events pending */
  } else if (selstat == 1) { /* outgoing data pending */
    aim_tx_flushqueue(aimsess);
  } else if (selstat == 2) { /* incoming data pending */
    /* printf("selstat == 2\n"); */

    if (aim_get_command(aimsess, waitingconn) >= 0) {
      aim_rxdispatch(aimsess);
    } else {
      /* printf("connection error (type 0x%04x:0x%04x)\n", waitingconn->type, waitingconn->subtype); */
      /* we should have callbacks for all these, else the library will do the conn_kill for us. */
      if (waitingconn->type == AIM_CONN_TYPE_RENDEZVOUS) {
	if (waitingconn->subtype == AIM_CONN_SUBTYPE_OFT_DIRECTIM) {
	  /* printf("disconnected from %s\n", aim_directim_getsn(waitingconn)); */
	  aim_conn_kill(aimsess, &waitingconn);
	  owl_aim_logged_out();
	}
      } else {
	aim_conn_kill(aimsess, &waitingconn);
	owl_aim_logged_out();
      }
      if (!aim_getconn_type(aimsess, AIM_CONN_TYPE_BOS)) {
	/* printf("major connection error\n"); */
	owl_aim_logged_out();
	/* break; */
      }
    }
  }
  /* exit(0); */
  return(0);
}

static void faimtest_debugcb(aim_session_t *sess, int level, const char *format, va_list va)
{
  return;
}

static int faimtest_parse_login(aim_session_t *sess, aim_frame_t *fr, ...)
{
  struct owlfaim_priv *priv = sess->aux_data;
  struct client_info_s info = CLIENTINFO_AIM_KNOWNGOOD;

  const char *key;
  va_list ap;

  va_start(ap, fr);
  key = va_arg(ap, const char *);
  va_end(ap);

  aim_send_login(sess, fr->conn, priv->screenname, priv->password, &info, key);

  return(1);
}


static int faimtest_parse_authresp(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  struct aim_authresp_info *info;
  aim_conn_t *bosconn;

  va_start(ap, fr);
  info = va_arg(ap, struct aim_authresp_info *);
  va_end(ap);

  /* printf("Screen name: %s\n", info->sn); */
  owl_function_debugmsg("doing faimtest_parse_authresp");
  owl_function_debugmsg("faimtest_parse_authresp: %s", info->sn);

  /*
   * Check for error.
   */
  if (info->errorcode || !info->bosip || !info->cookie) {
    /*
    printf("Login Error Code 0x%04x\n", info->errorcode);
    printf("Error URL: %s\n", info->errorurl);
    */
    if (info->errorcode==0x05) {
      owl_aim_login_error("Incorrect nickname or password.");
    } else if (info->errorcode==0x11) {
      owl_aim_login_error("Your account is currently suspended.");
    } else if (info->errorcode==0x14) {
      owl_aim_login_error("The AOL Instant Messenger service is temporarily unavailable.");
    } else if (info->errorcode==0x18) {
      owl_aim_login_error("You have been connecting and disconnecting too frequently. Wait ten minutes and try again. If you continue to try, you will need to wait even longer.");
    } else if (info->errorcode==0x1c) {
      owl_aim_login_error("The client version you are using is too old.");
    } else {
      owl_aim_login_error(NULL);
    }
    aim_conn_kill(sess, &fr->conn);
    return(1);
  }

  /*
  printf("Reg status: %d\n", info->regstatus);
  printf("Email: %s\n", info->email);
  printf("BOS IP: %s\n", info->bosip);
  */

  /* printf("Closing auth connection...\n"); */
  aim_conn_kill(sess, &fr->conn);
  if (!(bosconn = aim_newconn(sess, AIM_CONN_TYPE_BOS, info->bosip))) {
    /* printf("could not connect to BOS: internal error\n"); */
    return(1);
  } else if (bosconn->status & AIM_CONN_STATUS_CONNERR) {
    /* printf("could not connect to BOS\n"); */
    aim_conn_kill(sess, &bosconn);
    return(1);
  }
  owl_global_set_bossconn(&g, bosconn);
  owl_aim_successful_login(info->sn);
  addcb_bos(sess, bosconn);
  aim_sendcookie(sess, bosconn, info->cookielen, info->cookie);
  return(1);
}

int faimtest_flapversion(aim_session_t *sess, aim_frame_t *fr, ...)
{
  owl_function_debugmsg("doing faimtest_flapversion");

#if 0
  /* XXX fix libfaim to support this */
  printf("using FLAP version 0x%08x\n", /* aimutil_get32(fr->data)*/ 0xffffffff);

  /*
   * This is an alternate location for starting the login process.
   */
  /* XXX should do more checking to make sure its really the right AUTH conn */
  if (fr->conn->type == AIM_CONN_TYPE_AUTH) {
    /* do NOT send a flapversion, request_login will send it if needed */
    aim_request_login(sess, fr->conn, priv->screenname);
    /* printf("faimtest: login request sent\n"); */
  }
#endif

  return 1;
}


int faimtest_conncomplete(aim_session_t *sess, aim_frame_t *fr, ...)
{
  owl_function_debugmsg("doing faimtest_conncomplete");
  /* owl_aim_successful_login(info->sn); */
  return 1;
}

void addcb_bos(aim_session_t *sess, aim_conn_t *bosconn)
{
  owl_function_debugmsg("doing addcb_bos");
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNCOMPLETE, faimtest_conncomplete, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, conninitdone_bos, 0);

  aim_conn_addhandler(sess, bosconn, 0x0013,         0x0003,                        ssirights, 0);
  aim_conn_addhandler(sess, bosconn, 0x0013,         0x0006,                        ssidata, 0);
  aim_conn_addhandler(sess, bosconn, 0x0013,         0x000f,                        ssidatanochange, 0);
  aim_conn_addhandler(sess, bosconn, 0x0008,         0x0002,                        handlepopup, 0);
  aim_conn_addhandler(sess, bosconn, 0x0009,         0x0003,                        faimtest_bosrights, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_REDIRECT,           faimtest_handleredirect, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_STS, AIM_CB_STS_SETREPORTINTERVAL,  faimtest_reportinterval, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_BUD, AIM_CB_BUD_RIGHTSINFO,         faimtest_parse_buddyrights, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_MOTD,               faimtest_parse_motd, 0);
  aim_conn_addhandler(sess, bosconn, 0x0004,         0x0005,                        faimtest_icbmparaminfo, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNERR,    faimtest_parse_connerr, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_LOC, AIM_CB_LOC_RIGHTSINFO,         faimtest_locrights, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_BUD, AIM_CB_BUD_ONCOMING,           faimtest_parse_oncoming, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_BUD, AIM_CB_BUD_OFFGOING,           faimtest_parse_offgoing, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_INCOMING,           faimtest_parse_incoming_im, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_LOC, AIM_CB_LOC_ERROR,              faimtest_parse_locerr, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_MISSEDCALL,         faimtest_parse_misses, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_RATECHANGE,         faimtest_parse_ratechange, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_EVIL,               faimtest_parse_evilnotify, 0);

  aim_conn_addhandler(sess, bosconn, 0x000a,         0x0001,                        faimtest_parse_searcherror, 0);
  aim_conn_addhandler(sess, bosconn, 0x000a,         0x0003,                        faimtest_parse_searchreply, 0);

  /*
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_LOK, AIM_CB_LOK_ERROR, faimtest_parse_searcherror, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_LOK, 0x0003, faimtest_parse_searchreply, 0);
  */

  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_ERROR,              faimtest_parse_msgerr, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_LOC, AIM_CB_LOC_USERINFO,           faimtest_parse_userinfo, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_ACK,                faimtest_parse_msgack, 0);

  aim_conn_addhandler(sess, bosconn, 0x0001,         0x0001,                        faimtest_parse_genericerr, 0);
  aim_conn_addhandler(sess, bosconn, 0x0003,         0x0001,                        faimtest_parse_genericerr, 0);
  aim_conn_addhandler(sess, bosconn, 0x0009,         0x0001,                        faimtest_parse_genericerr, 0);
  aim_conn_addhandler(sess, bosconn, 0x0001,         0x000b,                        serverpause, 0);
  aim_conn_addhandler(sess, bosconn, 0x0001,         0x0012,                        migrate, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_ICQ, AIM_CB_ICQ_OFFLINEMSG,         offlinemsg, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_ICQ, AIM_CB_ICQ_OFFLINEMSGCOMPLETE, offlinemsgdone, 0);

  /*
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNERR,     gaim_connerr, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, conninitdone_chatnav, 0);
  */

  /*
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SSI, AIM_CB_SSI_ERROR,              faimtest_ssi_parseerr, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SSI, AIM_CB_SSI_RIGHTSINFO,         faimtest_ssi_parserights, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SSI, AIM_CB_SSI_LIST,               faimtest_ssi_parselist, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SSI, AIM_CB_SSI_NOLIST,             faimtest_ssi_parselist, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SSI, AIM_CB_SSI_SRVACK,             faimtest_ssi_parseack, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SSI, AIM_CB_SSI_RECVAUTH,           faimtest_ssi_authgiven, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SSI, AIM_CB_SSI_RECVAUTHREQ,        faimtest_ssi_authrequest, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SSI, AIM_CB_SSI_RECVAUTHREP,        faimtest_ssi_authreply, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SSI, AIM_CB_SSI_ADDED,              faimtest_ssi_gotadded, 0);
  */

  return;
}

static int conninitdone_bos(aim_session_t *sess, aim_frame_t *fr, ...)
{
  owl_function_debugmsg("doing coninitdone_bos");


  aim_reqpersonalinfo(sess, fr->conn);
  aim_ssi_reqrights(sess);
  aim_ssi_reqdata(sess);
  aim_locate_reqrights(sess);
  aim_buddylist_reqrights(sess, fr->conn);

  aim_im_reqparams(sess);
  /* aim_bos_reqrights(sess, fr->conn); */ /* XXX - Don't call this with ssi */

  owl_function_debugmsg("conninitdone_bos: requesting rights");
  aim_bos_reqrights(sess, fr->conn); /* XXX - Don't call this with ssi */
  aim_bos_setgroupperm(sess, fr->conn, AIM_FLAG_ALLUSERS);
  aim_bos_setprivacyflags(sess, fr->conn, AIM_PRIVFLAGS_ALLOWIDLE | AIM_PRIVFLAGS_ALLOWMEMBERSINCE);

  return(1);
}

static int conninitdone_admin(aim_session_t *sess, aim_frame_t *fr, ...)
{
  aim_clientready(sess, fr->conn);
  owl_function_debugmsg("conninitdone_admin: initialization done for admin connection");
  return(1);
}

int logout(aim_session_t *sess)
{
  aim_session_kill(sess);
  owl_aim_init();

  owl_function_debugmsg("libfaim logout called");
  /*
  if (faimtest_init() == -1)
    printf("faimtest_init failed\n");
  */

  return(0);
}

/**************************************************************************************************/

static int faimtest_parse_connerr(aim_session_t *sess, aim_frame_t *fr, ...)
{
  struct owlfaim_priv *priv = sess->aux_data;
  va_list ap;
  fu16_t code;
  const char *msg;

  va_start(ap, fr);
  code = va_arg(ap, int);
  msg = va_arg(ap, const char *);
  va_end(ap);

  owl_function_error("faimtest_parse_connerr: Code 0x%04x: %s\n", code, msg);
  aim_conn_kill(sess, &fr->conn); /* this will break the main loop */

  priv->connected = 0;

  return 1;
}

static int faimtest_accountconfirm(aim_session_t *sess, aim_frame_t *fr, ...)
{
  int status;
  va_list ap;

  va_start(ap, fr);
  status = va_arg(ap, int); /* status code of confirmation request */
  va_end(ap);

  /* owl_function_debugmsg("faimtest_accountconfirm: Code 0x%04x: %s\n", code, msg); */
  owl_function_debugmsg("faimtest_accountconfirm: account confirmation returned status 0x%04x (%s)\n", status, (status==0x0000)?"email sent":"unknown");

  return 1;
}

static int faimtest_infochange(aim_session_t *sess, aim_frame_t *fr, ...)
{
  fu16_t change = 0, perms, type;
  int length, str;
  const char *val;
  va_list ap;

  va_start(ap, fr);
  change = va_arg(ap, int);
  perms = (fu16_t)va_arg(ap, unsigned int);
  type = (fu16_t)va_arg(ap, unsigned int);
  length = va_arg(ap, int);
  val = va_arg(ap, const char *);
  str = va_arg(ap, int);
  va_end(ap);

  owl_function_debugmsg("faimtest_infochange: info%s: perms = %d, type = %x, length = %d, val = %s", change?" change":"", perms, type, length, str?val:"(not string)");

  return(1);
}


static int faimtest_handleredirect(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  struct aim_redirect_data *redir;

  owl_function_debugmsg("faimtest_handledirect:");

  va_start(ap, fr);
  redir = va_arg(ap, struct aim_redirect_data *);

  if (redir->group == 0x0005) {  /* Adverts */

  } else if (redir->group == 0x0007) {  /* Authorizer */
    aim_conn_t *tstconn;

    owl_function_debugmsg("faimtest_handledirect: autorizer");

    tstconn = aim_newconn(sess, AIM_CONN_TYPE_AUTH, redir->ip);
    if (!tstconn || (tstconn->status & AIM_CONN_STATUS_RESOLVERR)) {
      owl_function_error("faimtest_handleredirect: unable to reconnect with authorizer");
    } else {
      aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_FLAPVER, faimtest_flapversion, 0);
      aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNCOMPLETE, faimtest_conncomplete, 0);
      aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, conninitdone_admin, 0);
      aim_conn_addhandler(sess, tstconn, 0x0007, 0x0007, faimtest_accountconfirm, 0);
      aim_conn_addhandler(sess, tstconn, 0x0007, 0x0003, faimtest_infochange, 0);
      aim_conn_addhandler(sess, tstconn, 0x0007, 0x0005, faimtest_infochange, 0);
      /* Send the cookie to the Auth */
      aim_sendcookie(sess, tstconn, redir->cookielen, redir->cookie);
      owl_function_debugmsg("faimtest_handleredirect: sent cookie to authorizer host");
    }
  } else if (redir->group == 0x000d) {  /* ChatNav */
    owl_function_debugmsg("faimtest_handledirect: chatnav");
    chatnav_redirect(sess, redir);
  } else if (redir->group == 0x000e) { /* Chat */
    owl_function_debugmsg("faimtest_handledirect: chat");
    chat_redirect(sess, redir);
  } else {
    owl_function_debugmsg("faimtest_handleredirect: uh oh... got redirect for unknown service 0x%04x!!", redir->group);
  }
  va_end(ap);
  return 1;
}

static int faimtest_icbmparaminfo(aim_session_t *sess, aim_frame_t *fr, ...)
{
  struct aim_icbmparameters *params;
  va_list ap;

  va_start(ap, fr);
  params = va_arg(ap, struct aim_icbmparameters *);
  va_end(ap);

  owl_function_debugmsg("faimtest_icbmparaminfo: ICBM Parameters: maxchannel = %d, default flags = 0x%08x, max msg len = %d, max sender evil = %f, max receiver evil = %f, min msg interval = %u",
		       params->maxchan, params->flags, params->maxmsglen, ((float)params->maxsenderwarn)/10.0, ((float)params->maxrecverwarn)/10.0, params->minmsginterval);

  /*
  * Set these to your taste, or client medium.  Setting minmsginterval
  * higher is good for keeping yourself from getting flooded (esp
  * if you're on a slow connection or something where that would be
  * useful).
  */
  params->maxmsglen = 8000;
  params->minmsginterval = 0; /* in milliseconds */
  /* aim_seticbmparam(sess, params); */
  aim_im_setparams(sess, params);

  return 1;
}

static int faimtest_parse_buddyrights(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  fu16_t maxbuddies, maxwatchers;

  va_start(ap, fr);
  maxbuddies = va_arg(ap, int);
  maxwatchers = va_arg(ap, int);
  va_end(ap);

  owl_function_debugmsg("faimtest_parse_buddyrights: Max buddies = %d / Max watchers = %d\n", maxbuddies, maxwatchers);

  /* aim_ssi_reqrights(sess, fr->conn); */
  aim_ssi_reqrights(sess);

  return 1;
}

static int faimtest_bosrights(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  fu16_t maxpermits, maxdenies;

  va_start(ap, fr);
  maxpermits = va_arg(ap, int);
  maxdenies = va_arg(ap, int);
  va_end(ap);

  owl_function_debugmsg("faimtest_bosrights: Max permit = %d / Max deny = %d\n", maxpermits, maxdenies);
  aim_clientready(sess, fr->conn);
  owl_function_debugmsg("officially connected to BOS.");
  aim_icq_reqofflinemsgs(sess);
  return 1;
}

static int faimtest_locrights(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  fu16_t maxsiglen;

  va_start(ap, fr);
  maxsiglen = va_arg(ap, int);
  va_end(ap);

  owl_function_debugmsg("faimtest_locrights: rights: max signature length = %d\n", maxsiglen);

  return(1);
}

static int faimtest_reportinterval(aim_session_t *sess, aim_frame_t *fr, ...)
{
  struct owlfaim_priv *priv = sess->aux_data;
  va_list ap;
  fu16_t interval;

  va_start(ap, fr);
  interval = va_arg(ap, int);
  va_end(ap);

  owl_function_debugmsg("faimtest_reportinterval: %d (seconds?)\n", interval);

  if (!priv->connected) {
    priv->connected++;
  }
  /* aim_reqicbmparams(sess); */
  aim_im_reqparams(sess);
  /* kretch */
  return 1;
}

static int faimtest_parse_motd(aim_session_t *sess, aim_frame_t *fr, ...)
{
  const char *msg;
  fu16_t id;
  va_list ap;
  static int codeslen = 5;
  static const char *codes[] = {
    "Unknown",
    "Mandatory upgrade",
    "Advisory upgrade",
    "System bulletin",
    "Top o' the world!"
  };

  va_start(ap, fr);
  id = va_arg(ap, int);
  msg = va_arg(ap, const char *);
  va_end(ap);

  owl_function_debugmsg("faimtest_parse_motd: %s (%d / %s)\n", msg?msg:"nomsg", id, (id < codeslen)?codes[id]:"unknown");

  return 1;
}

/*
static void printuserflags(fu16_t flags)
{
  if (flags & AIM_FLAG_UNCONFIRMED) printf("UNCONFIRMED ");
  if (flags & AIM_FLAG_ADMINISTRATOR) printf("ADMINISTRATOR ");
  if (flags & AIM_FLAG_AOL) printf("AOL ");
  if (flags & AIM_FLAG_OSCAR_PAY) printf("OSCAR_PAY ");
  if (flags & AIM_FLAG_FREE) printf("FREE ");
  if (flags & AIM_FLAG_AWAY) printf("AWAY ");
  if (flags & AIM_FLAG_ICQ) printf("ICQ ");
  if (flags & AIM_FLAG_WIRELESS) printf("WIRELESS ");
  if (flags & AIM_FLAG_ACTIVEBUDDY) printf("ACTIVEBUDDY ");

  return;
}
*/

static int faimtest_parse_userinfo(aim_session_t *sess, aim_frame_t *fr, ...)
{
  aim_userinfo_t *userinfo;
  const char *prof_encoding = NULL;
  const char *prof = NULL;
  fu16_t inforeq = 0;
  owl_buddy *b;
  va_list ap;
  va_start(ap, fr);
  userinfo = va_arg(ap, aim_userinfo_t *);
  inforeq = (fu16_t)va_arg(ap, unsigned int);
  prof_encoding = va_arg(ap, const char *);
  prof = va_arg(ap, const char *);
  va_end(ap);

  /* right now the only reason we call this is for idle times */
  owl_function_debugmsg("parse_userinfo sn: %s idle: %i", userinfo->sn, userinfo->idletime);
  b=owl_buddylist_get_aim_buddy(owl_global_get_buddylist(&g),
				userinfo->sn);
  if (!b) return(1);
  owl_buddy_set_idle_since(b, userinfo->idletime);
  return(1);

  /*
  printf("userinfo: sn: %s\n", userinfo->sn);
  printf("userinfo: warnlevel: %f\n", aim_userinfo_warnlevel(userinfo));
  printf("userinfo: flags: 0x%04x = ", userinfo->flags);
  printuserflags(userinfo->flags);
  printf("\n");
  */

  /*
  printf("userinfo: membersince: %lu\n", userinfo->membersince);
  printf("userinfo: onlinesince: %lu\n", userinfo->onlinesince);
  printf("userinfo: idletime: 0x%04x\n", userinfo->idletime);
  printf("userinfo: capabilities = %s = 0x%08lx\n", (userinfo->present & AIM_USERINFO_PRESENT_CAPABILITIES) ? "present" : "not present", userinfo->capabilities);
  */

  /*
  if (inforeq == AIM_GETINFO_GENERALINFO) {
    owl_function_debugmsg("userinfo: profile_encoding: %s\n", prof_encoding ? prof_encoding : "[none]");
    owl_function_debugmsg("userinfo: prof: %s\n", prof ? prof : "[none]");
  } else if (inforeq == AIM_GETINFO_AWAYMESSAGE) {
    owl_function_debugmsg("userinfo: awaymsg_encoding: %s\n", prof_encoding ? prof_encoding : "[none]");
    owl_function_debugmsg("userinfo: awaymsg: %s\n", prof ? prof : "[none]");
  } else if (inforeq == AIM_GETINFO_CAPABILITIES) {
    owl_function_debugmsg("userinfo: capabilities: see above\n");
  } else {
    owl_function_debugmsg("userinfo: unknown info request\n");
  }
  */
  return(1);
}

/*
 * Channel 1: Standard Message
 */
static int faimtest_parse_incoming_im_chan1(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *userinfo, struct aim_incomingim_ch1_args *args)
{
  owl_message *m;
  char *stripmsg, *nz_screenname, *wrapmsg;
  char *realmsg = NULL;

  if (!args->msg) {
    realmsg = g_strdup("");
  } else if (args->icbmflags & AIM_IMFLAGS_UNICODE) {
    realmsg = g_convert(args->msg, args->msglen, "UTF-8", "UCS-2BE",
                        NULL, NULL, NULL);
  } else if(args->icbmflags & AIM_IMFLAGS_ISO_8859_1) {
    realmsg = g_convert(args->msg, args->msglen, "UTF-8", "ISO-8859-1",
                        NULL, NULL, NULL);
  } else {
    realmsg = g_strdup(args->msg);
  }

  if (!realmsg) {
    realmsg = g_strdup("[Error decoding incoming IM]");
  }

  owl_function_debugmsg("faimtest_parse_incoming_im_chan1: message from: %s", userinfo->sn?userinfo->sn:"");
  /* create a message, and put it on the message queue */
  stripmsg=owl_text_htmlstrip(realmsg);
  wrapmsg=owl_text_wordwrap(stripmsg, 70);
  nz_screenname=owl_aim_normalize_screenname(userinfo->sn);
  m=g_new(owl_message, 1);
  owl_message_create_aim(m,
			 nz_screenname,
			 owl_global_get_aim_screenname(&g),
			 wrapmsg,
			 OWL_MESSAGE_DIRECTION_IN,
			 0);
  if (args->icbmflags & AIM_IMFLAGS_AWAY) owl_message_set_attribute(m, "isauto", "");
  owl_global_messagequeue_addmsg(&g, m);
  g_free(stripmsg);
  g_free(wrapmsg);
  g_free(nz_screenname);
  g_free(realmsg);

  return(1);

  /* TODO: Multipart? See history from before 1.8 release. */
}

/*
 * Channel 2: Rendevous Request
 */
static int faimtest_parse_incoming_im_chan2(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *userinfo, struct aim_incomingim_ch2_args *args)
{
  /*
  printf("rendezvous: source sn = %s\n", userinfo->sn);
  printf("rendezvous: \twarnlevel = %f\n", aim_userinfo_warnlevel(userinfo));
  printf("rendezvous: \tclass = 0x%04x = ", userinfo->flags);
  printuserflags(userinfo->flags);
  printf("\n");

  printf("rendezvous: \tonlinesince = %lu\n", userinfo->onlinesince);
  printf("rendezvous: \tidletime = 0x%04x\n", userinfo->idletime);

  printf("rendezvous: message/description = %s\n", args->msg);
  printf("rendezvous: encoding = %s\n", args->encoding);
  printf("rendezvous: language = %s\n", args->language);
  */

  if (args->reqclass == AIM_CAPS_SENDFILE) {
    owl_function_debugmsg("faimtest_parse_incoming_im_chan2: send file!");
  } else if (args->reqclass == AIM_CAPS_CHAT) {
    owl_function_debugmsg("faimtest_parse_incoming_im_chan2: chat invite: %s, %i, %i", args->info.chat.roominfo.name, args->info.chat.roominfo.exchange, args->info.chat.roominfo.instance);
    /*
    printf("chat invitation: room name = %s\n", args->info.chat.roominfo.name);
    printf("chat invitation: exchange = 0x%04x\n", args->info.chat.roominfo.exchange);
    printf("chat invitation: instance = 0x%04x\n", args->info.chat.roominfo.instance);
    */
    /* Automatically join room... */
    /* printf("chat invitiation: autojoining %s...\n", args->info.chat.roominfo.name); */

    /* aim_chat_join(sess, conn, args->info.chat.roominfo.exchange, args->info.chat.roominfo.name, args->info.chat.roominfo.instance); */
  } else if (args->reqclass == AIM_CAPS_BUDDYICON) {
    owl_function_debugmsg("faimtest_parse_incoming_im_chan2: Buddy Icon from %s, length = %u\n",
			  userinfo->sn, args->info.icon.length);
  } else if (args->reqclass == AIM_CAPS_ICQRTF) {
    owl_function_debugmsg("faimtest_parse_incoming_im_chan2: RTF message from %s: (fgcolor = 0x%08x, bgcolor = 0x%08x) %s\n",
			  userinfo->sn, args->info.rtfmsg.fgcolor, args->info.rtfmsg.bgcolor, args->info.rtfmsg.rtfmsg);
  } else {
    owl_function_debugmsg("faimtest_parse_incoming_im_chan2: icbm: unknown reqclass (%d)\n", args->reqclass);
  }
  return 1;
}

static int faimtest_parse_incoming_im(aim_session_t *sess, aim_frame_t *fr, ...)
{
  fu16_t channel;
  aim_userinfo_t *userinfo;
  va_list ap;
  int ret = 0;

  va_start(ap, fr);
  channel = (fu16_t)va_arg(ap, unsigned int);
  userinfo = va_arg(ap, aim_userinfo_t *);

  if (channel == 1) {
    struct aim_incomingim_ch1_args *args;
    args = va_arg(ap, struct aim_incomingim_ch1_args *);
    ret = faimtest_parse_incoming_im_chan1(sess, fr->conn, userinfo, args);
  } else if (channel == 2) {
    struct aim_incomingim_ch2_args *args;
    args = va_arg(ap, struct aim_incomingim_ch2_args *);
    ret = faimtest_parse_incoming_im_chan2(sess, fr->conn, userinfo, args);
  } else {
    owl_function_debugmsg("faimtest_parse_incoming_im: unsupported channel 0x%04x\n", channel);
  }
  va_end(ap);
  owl_function_debugmsg("faimtest_parse_incoming_im: done with ICBM handling (ret = %d)\n", ret);
  return 1;
}

static int faimtest_parse_oncoming(aim_session_t *sess, aim_frame_t *fr, ...)
{
  aim_userinfo_t *userinfo;
  char *nz_screenname;
  owl_buddy *b;
  va_list ap;
  va_start(ap, fr);
  userinfo = va_arg(ap, aim_userinfo_t *);
  va_end(ap);

  nz_screenname=owl_aim_normalize_screenname(userinfo->sn);

  owl_buddylist_oncoming(owl_global_get_buddylist(&g), nz_screenname);

  if (userinfo->present & AIM_USERINFO_PRESENT_IDLE) {
    owl_function_debugmsg("faimtest_parseoncoming: in empty part of userinfo present and present idle");
  }

  b=owl_buddylist_get_aim_buddy(owl_global_get_buddylist(&g), nz_screenname);
  if (!b) {
    owl_function_debugmsg("Error: parse_oncoming setting idle time with no buddy present.");
    return(1);
  }
  if (userinfo->idletime==0) {
    owl_buddy_set_unidle(b);
  } else {
    owl_buddy_set_idle(b);
    owl_buddy_set_idle_since(b, userinfo->idletime);
  }

  if (userinfo->flags & AIM_FLAG_AWAY) {
    owl_function_debugmsg("parse_oncoming sn: %s away flag!", userinfo->sn);
  }

  owl_function_debugmsg("parse_oncoming sn: %s idle: %i", userinfo->sn, userinfo->idletime);

  g_free(nz_screenname);

  /*
    printf("%ld  %s is now online (flags: %04x = %s%s%s%s%s%s%s%s) (caps = %s = 0x%08lx)\n",
    time(NULL),
    userinfo->sn, userinfo->flags,
    (userinfo->flags&AIM_FLAG_UNCONFIRMED)?" UNCONFIRMED":"",
    (userinfo->flags&AIM_FLAG_ADMINISTRATOR)?" ADMINISTRATOR":"",
    (userinfo->flags&AIM_FLAG_AOL)?" AOL":"",
    (userinfo->flags&AIM_FLAG_OSCAR_PAY)?" OSCAR_PAY":"",
    (userinfo->flags&AIM_FLAG_FREE)?" FREE":"",
    (userinfo->flags&AIM_FLAG_AWAY)?" AWAY":"",
    (userinfo->flags&AIM_FLAG_ICQ)?" ICQ":"",
    (userinfo->flags&AIM_FLAG_WIRELESS)?" WIRELESS":"",
    (userinfo->present & AIM_USERINFO_PRESENT_CAPABILITIES) ? "present" : "not present",
    userinfo->capabilities);
  */
  return(1);
}

static int faimtest_parse_offgoing(aim_session_t *sess, aim_frame_t *fr, ...)
{
  aim_userinfo_t *userinfo;
  char *nz_screenname;
  va_list ap;

  va_start(ap, fr);
  userinfo = va_arg(ap, aim_userinfo_t *);
  va_end(ap);

  nz_screenname=owl_aim_normalize_screenname(userinfo->sn);
  owl_buddylist_offgoing(owl_global_get_buddylist(&g), nz_screenname);
  g_free(nz_screenname);

  if (userinfo->present & AIM_USERINFO_PRESENT_IDLE) {
    owl_function_debugmsg("parse_offgoing sn: %s idle time %i", userinfo->sn, userinfo->idletime);
  }

  /*
  printf("%ld  %s is now offline (flags: %04x = %s%s%s%s%s%s%s%s) (caps = %s = 0x%08lx)\n",
	 time(NULL),
	 userinfo->sn, userinfo->flags,
	 (userinfo->flags&AIM_FLAG_UNCONFIRMED)?" UNCONFIRMED":"",
	 (userinfo->flags&AIM_FLAG_ADMINISTRATOR)?" ADMINISTRATOR":"",
	 (userinfo->flags&AIM_FLAG_AOL)?" AOL":"",
	 (userinfo->flags&AIM_FLAG_OSCAR_PAY)?" OSCAR_PAY":"",
	 (userinfo->flags&AIM_FLAG_FREE)?" FREE":"",
	 (userinfo->flags&AIM_FLAG_AWAY)?" AWAY":"",
	 (userinfo->flags&AIM_FLAG_ICQ)?" ICQ":"",
	 (userinfo->flags&AIM_FLAG_WIRELESS)?" WIRELESS":"",
	 (userinfo->present & AIM_USERINFO_PRESENT_CAPABILITIES) ? "present" : "not present",
	 userinfo->capabilities);
  */

  return 1;
}

/* Used by chat as well. */
int faimtest_parse_genericerr(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  fu16_t reason;

  va_start(ap, fr);
  reason = (fu16_t)va_arg(ap, unsigned int);
  va_end(ap);

  /* printf("snac threw error (reason 0x%04x: %s)\n", reason, (reason<msgerrreasonslen)?msgerrreasons[reason]:"unknown"); */
  if (reason<msgerrreasonslen) owl_function_error("%s", msgerrreasons[reason]);

  return 1;
}

static int faimtest_parse_msgerr(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  const char *destsn;
  fu16_t reason;

  va_start(ap, fr);
  reason = (fu16_t)va_arg(ap, unsigned int);
  destsn = va_arg(ap, const char *);
  va_end(ap);

  /* printf("message to %s bounced (reason 0x%04x: %s)\n", destsn, reason, (reason<msgerrreasonslen)?msgerrreasons[reason]:"unknown"); */
  if (reason<msgerrreasonslen) owl_function_error("%s", msgerrreasons[reason]);

  if (reason==4) {
    owl_function_adminmsg("", "Could not send AIM message, user not logged on");
  }

  return 1;
}

static int faimtest_parse_locerr(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  const char *destsn;
  fu16_t reason;

  va_start(ap, fr);
  reason = (fu16_t)va_arg(ap, unsigned int);
  destsn = va_arg(ap, const char *);
  va_end(ap);

  /* printf("user information for %s unavailable (reason 0x%04x: %s)\n", destsn, reason, (reason<msgerrreasonslen)?msgerrreasons[reason]:"unknown"); */
  if (reason<msgerrreasonslen) owl_function_error("%s", msgerrreasons[reason]);

  return 1;
}

static int faimtest_parse_misses(aim_session_t *sess, aim_frame_t *fr, ...)
{
  static const char *missedreasons[] = {
    "Invalid (0)",
    "Message too large",
    "Rate exceeded",
    "Evil Sender",
    "Evil Receiver"
  };
  static int missedreasonslen = 5;

  va_list ap;
  fu16_t chan, nummissed, reason;
  aim_userinfo_t *userinfo;

  va_start(ap, fr);
  chan = (fu16_t)va_arg(ap, unsigned int);
  userinfo = va_arg(ap, aim_userinfo_t *);
  nummissed = (fu16_t)va_arg(ap, unsigned int);
  reason = (fu16_t)va_arg(ap, unsigned int);
  va_end(ap);

  owl_function_debugmsg("faimtest_parse_misses: missed %d messages from %s on channel %d (reason %d: %s)\n", nummissed, userinfo->sn, chan, reason, (reason<missedreasonslen)?missedreasons[reason]:"unknown");

  return 1;
}

/*
 * Received in response to an IM sent with the AIM_IMFLAGS_ACK option.
 */
static int faimtest_parse_msgack(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  fu16_t type;
  const char *sn = NULL;

  va_start(ap, fr);
  type = (fu16_t)va_arg(ap, unsigned int);
  sn = va_arg(ap, const char *);
  va_end(ap);

  owl_function_debugmsg("faimtest_parse_msgack: 0x%04x / %s\n", type, sn);

  return 1;
}

static int faimtest_parse_ratechange(aim_session_t *sess, aim_frame_t *fr, ...)
{
  static const char *codes[5] = {
    "invalid",
    "change",
    "warning",
    "limit",
    "limit cleared"
  };
  va_list ap;
  fu16_t code, rateclass;
  fu32_t windowsize, clear, alert, limit, disconnect;
  fu32_t currentavg, maxavg;

  va_start(ap, fr);

  /* See code explanations below */
  code = (fu16_t)va_arg(ap, unsigned int);

  /*
   * See comments above aim_parse_ratechange_middle() in aim_rxhandlers.c.
   */
  rateclass = (fu16_t)va_arg(ap, unsigned int);

  /*
   * Not sure what this is exactly.  I think its the temporal
   * relation factor (ie, how to make the rest of the numbers
   * make sense in the real world).
   */
  windowsize = va_arg(ap, fu32_t);

  /* Explained below */
  clear = va_arg(ap, fu32_t);
  alert = va_arg(ap, fu32_t);
  limit = va_arg(ap, fu32_t);
  disconnect = va_arg(ap, fu32_t);
  currentavg = va_arg(ap, fu32_t);
  maxavg = va_arg(ap, fu32_t);

  va_end(ap);

  owl_function_debugmsg("faimtest_parse_ratechange: rate %s (rate class 0x%04x): curavg = %u, maxavg = %u, alert at %u, clear warning at %u, limit at %u, disconnect at %u (window size = %u)",
			(code < 5)?codes[code]:"invalid",
			rateclass,
			currentavg, maxavg,
			alert, clear,
			limit, disconnect,
			windowsize);
  return 1;
}

static int faimtest_parse_evilnotify(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  fu16_t newevil;
  aim_userinfo_t *userinfo;

  va_start(ap, fr);
  newevil = (fu16_t)va_arg(ap, unsigned int);
  userinfo = va_arg(ap, aim_userinfo_t *);
  va_end(ap);

  /*
   * Evil Notifications that are lacking userinfo->sn are anon-warns
   * if they are an evil increases, but are not warnings at all if its
   * a decrease (its the natural backoff happening).
   *
   * newevil is passed as an int representing the new evil value times
   * ten.
   */
  owl_function_debugmsg("faimtest_parse_evilnotify: new value = %2.1f%% (caused by %s)\n", ((float)newevil)/10, (userinfo && strlen(userinfo->sn))?userinfo->sn:"anonymous");

  return 1;
}

static int faimtest_parse_searchreply(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  const char *address, *SNs;
  int num, i;
  GPtrArray *list;

  va_start(ap, fr);
  address = va_arg(ap, const char *);
  num = va_arg(ap, int);
  SNs = va_arg(ap, const char *);
  va_end(ap);

  list = g_ptr_array_new();

  owl_function_debugmsg("faimtest_parse_searchreply: E-Mail Search Results for %s: ", address);
  for (i=0; i<num; i++) {
    owl_function_debugmsg("  %s", &SNs[i*(MAXSNLEN+1)]);
    g_ptr_array_add(list, (void *)&SNs[i*(MAXSNLEN+1)]);
  }
  owl_function_aimsearch_results(address, list);
  g_ptr_array_free(list, true);
  return 1;
}

static int faimtest_parse_searcherror(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  const char *address;

  va_start(ap, fr);
  address = va_arg(ap, const char *);
  va_end(ap);

  owl_function_error("No results searching for %s", address);
  owl_function_debugmsg("faimtest_parse_searcherror: E-Mail Search Results for %s: No Results or Invalid Email\n", address);

  return(1);
}

static int handlepopup(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  const char *msg, *url;
  fu16_t width, height, delay;

  va_start(ap, fr);
  msg = va_arg(ap, const char *);
  url = va_arg(ap, const char *);
  width = va_arg(ap, unsigned int);
  height = va_arg(ap, unsigned int);
  delay = va_arg(ap, unsigned int);
  va_end(ap);

  owl_function_debugmsg("handlepopup: (%dx%x:%d) %s (%s)\n", width, height, delay, msg, url);

  return 1;
}

static int serverpause(aim_session_t *sess, aim_frame_t *fr, ...)
{
  aim_sendpauseack(sess, fr->conn);
  return 1;
}

static int migrate(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  aim_conn_t *bosconn;
  const char *bosip;
  fu8_t *cookie;

  va_start(ap, fr);
  bosip = va_arg(ap, const char *);
  cookie = va_arg(ap, fu8_t *);
  va_end(ap);

  owl_function_debugmsg("migrate: migration in progress -- new BOS is %s -- disconnecting", bosip);
  aim_conn_kill(sess, &fr->conn);

  if (!(bosconn = aim_newconn(sess, AIM_CONN_TYPE_BOS, bosip))) {
    owl_function_debugmsg("migrate: could not connect to BOS: internal error");
    return 1;
  } else if (bosconn->status & AIM_CONN_STATUS_CONNERR) {
    owl_function_debugmsg("migrate: could not connect to BOS");
    aim_conn_kill(sess, &bosconn);
    return 1;
  }

  /* Login will happen all over again. */
  addcb_bos(sess, bosconn);
  /* aim_sendcookie(sess, bosconn, cookie); */ /********/
  return 1;
}

static int ssirights(aim_session_t *sess, aim_frame_t *fr, ...)
{
  owl_function_debugmsg("ssirights: got SSI rights, requesting data\n");
  /* aim_ssi_reqdata(sess, fr->conn, 0, 0x0000); */
  aim_ssi_reqdata(sess);

  return(1);
}

static int ssidata(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  fu8_t fmtver;
  fu16_t itemcount;
  fu32_t stamp;
  struct aim_ssi_item *list;
  /*
  struct aim_ssi_item *curitem;
  struct aim_ssi_item *l;
  */

  va_start(ap, fr);
  fmtver = va_arg(ap, unsigned int);
  itemcount = va_arg(ap, unsigned int);
  stamp = va_arg(ap, fu32_t);
  list = va_arg(ap, struct aim_ssi_item *);
  va_end(ap);

  owl_function_debugmsg("ssiddata: got SSI data (0x%02x, %d items, %u)", fmtver, itemcount, stamp);
  /*
  for (curitem=sess->ssi.local; curitem; curitem=curitem->next) {
    for (l = list; l; l = l->next) {
      owl_function_debugmsg("\t0x%04x (%s) - 0x%04x/0x%04x", l->type, l->name, l->gid, l->bid);
    }
  }
  */
  aim_ssi_enable(sess);

  return 1;
}

static int ssidatanochange(aim_session_t *sess, aim_frame_t *fr, ...)
{
  owl_function_debugmsg("ssidatanochange: server says we have the latest SSI data already");
  /* aim_ssi_enable(sess, fr->conn); */
  aim_ssi_enable(sess);
  return 1;
}

static int offlinemsg(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  struct aim_icq_offlinemsg *msg;

  va_start(ap, fr);
  msg = va_arg(ap, struct aim_icq_offlinemsg *);
  va_end(ap);

  if (msg->type == 0x0001) {
    owl_function_debugmsg("offlinemsg: from %u at %d/%d/%d %02d:%02d : %s", msg->sender, msg->year, msg->month, msg->day, msg->hour, msg->minute, msg->msg);
  } else {
    owl_function_debugmsg("unknown offline message type 0x%04x", msg->type);
  }
  return 1;
}

static int offlinemsgdone(aim_session_t *sess, aim_frame_t *fr, ...)
{
  /* Tell the server to delete them. */
  owl_function_debugmsg("offlinemsg done: ");
  aim_icq_ackofflinemsgs(sess);
  return 1;
}


/******************** chat.c **************************/

static int faimtest_chat_join(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  aim_userinfo_t *userinfo;
  int count;
  /* int i; */

  va_start(ap, fr);
  count = va_arg(ap, int);
  userinfo = va_arg(ap, aim_userinfo_t *);
  va_end(ap);

  owl_function_debugmsg("In faimtest_chat_join");
  /*
  printf("chat: %s:  New occupants have joined:\n", aim_chat_getname(fr->conn));
  for (i = 0; i < count; i++)
    printf("chat: %s: \t%s\n", aim_chat_getname(fr->conn), userinfo[i].sn);
  */
  return 1;
}

static int faimtest_chat_leave(aim_session_t *sess, aim_frame_t *fr, ...)
{
  aim_userinfo_t *userinfo;
  va_list ap;
  int count;
  /* int i; */


  va_start(ap, fr);
  count = va_arg(ap, int);
  userinfo = va_arg(ap, aim_userinfo_t *);
  va_end(ap);

  /*
    printf("chat: %s:  Some occupants have left:\n", aim_chat_getname(fr->conn));

    for (i = 0; i < count; i++)
    printf("chat: %s: \t%s\n", aim_chat_getname(fr->conn), userinfo[i].sn);
  */
  return 1;
}

static int faimtest_chat_infoupdate(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  aim_userinfo_t *userinfo;
  struct aim_chat_roominfo *roominfo;
  const char *roomname;
  int usercount;
  const char *roomdesc;
  fu16_t flags, unknown_d2, unknown_d5, maxmsglen, maxvisiblemsglen;
  fu32_t creationtime;
  const char *croomname;
  /* int i; */

  croomname = aim_chat_getname(fr->conn);

  va_start(ap, fr);
  roominfo = va_arg(ap, struct aim_chat_roominfo *);
  roomname = va_arg(ap, const char *);
  usercount = va_arg(ap, int);
  userinfo = va_arg(ap, aim_userinfo_t *);
  roomdesc = va_arg(ap, const char *);
  flags = (fu16_t)va_arg(ap, unsigned int);
  creationtime = va_arg(ap, fu32_t);
  maxmsglen = (fu16_t)va_arg(ap, unsigned int);
  unknown_d2 = (fu16_t)va_arg(ap, unsigned int);
  unknown_d5 = (fu16_t)va_arg(ap, unsigned int);
  maxvisiblemsglen = (fu16_t)va_arg(ap, unsigned int);
  va_end(ap);

  owl_function_debugmsg("In faimtest_chat_infoupdate");
  /*
  printf("chat: %s:  info update:\n", croomname);
  printf("chat: %s:  \tRoominfo: {%04x, %s, %04x}\n", croomname, roominfo->exchange, roominfo->name, roominfo->instance);
  printf("chat: %s:  \tRoomname: %s\n", croomname, roomname);
  printf("chat: %s:  \tRoomdesc: %s\n", croomname, roomdesc);
  printf("chat: %s:  \tOccupants: (%d)\n", croomname, usercount);

  for (i = 0; i < usercount; i++)
    printf("chat: %s:  \t\t%s\n", croomname, userinfo[i].sn);

  owl_function_debugmsg("chat: %s:  \tRoom flags: 0x%04x (%s%s%s%s)\n",
	 croomname, flags,
	 (flags & AIM_CHATROOM_FLAG_EVILABLE) ? "Evilable, " : "",
	 (flags & AIM_CHATROOM_FLAG_NAV_ONLY) ? "Nav Only, " : "",
	 (flags & AIM_CHATROOM_FLAG_INSTANCING_ALLOWED) ? "Instancing allowed, " : "",
	 (flags & AIM_CHATROOM_FLAG_OCCUPANT_PEEK_ALLOWED) ? "Occupant peek allowed, " : "");
  printf("chat: %s:  \tCreation time: %lu (time_t)\n", croomname, creationtime);
  printf("chat: %s:  \tUnknown_d2: 0x%04x\n", croomname, unknown_d2);
  printf("chat: %s:  \tUnknown_d5: 0x%02x\n", croomname, unknown_d5);
  printf("chat: %s:  \tMax message length: %d bytes\n", croomname, maxmsglen);
  printf("chat: %s:  \tMax visible message length: %d bytes\n", croomname, maxvisiblemsglen);
  */

  return(1);
}

static int faimtest_chat_incomingmsg(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  aim_userinfo_t *userinfo;
  const char *msg;
  char tmpbuf[1152];

  va_start(ap, fr);
  userinfo = va_arg(ap, aim_userinfo_t *);
  msg = va_arg(ap, const char *);
  va_end(ap);

  owl_function_debugmsg("in faimtest_chat_incomingmsg");

  /*
  printf("chat: %s: incoming msg from %s: %s\n", aim_chat_getname(fr->conn), userinfo->sn, msg);
  */

  /*
   * Do an echo for testing purposes.  But not for ourselves ("oops!")
   */
  if (strcmp(userinfo->sn, sess->sn) != 0) {
    /* sprintf(tmpbuf, "(%s said \"%s\")", userinfo->sn, msg); */
    aim_chat_send_im(sess, fr->conn, 0, tmpbuf, strlen(tmpbuf));
  }

  return 1;
}

static int conninitdone_chat(aim_session_t *sess, aim_frame_t *fr, ...)
{
  owl_function_debugmsg("faimtest_conninitdone_chat:");

  aim_conn_addhandler(sess, fr->conn, 0x000e, 0x0001, faimtest_parse_genericerr, 0);
  aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_USERJOIN, faimtest_chat_join, 0);
  aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_USERLEAVE, faimtest_chat_leave, 0);
  aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_ROOMINFOUPDATE, faimtest_chat_infoupdate, 0);
  aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_INCOMINGMSG, faimtest_chat_incomingmsg, 0);

  aim_clientready(sess, fr->conn);

  owl_function_debugmsg("Chat ready");

  /*
    chatcon = find_oscar_chat_by_conn(gc, fr->conn);
    chatcon->id = id;
    chatcon->cnv = serv_got_joined_chat(gc, id++, chatcon->show);
  */
  return(1);
}

void chatnav_redirect(aim_session_t *sess, struct aim_redirect_data *redir)
{
  aim_conn_t *tstconn;

  owl_function_debugmsg("in faimtest_chatnav_redirect");

  tstconn = aim_newconn(sess, AIM_CONN_TYPE_CHATNAV, redir->ip);
  if (!tstconn || (tstconn->status & AIM_CONN_STATUS_RESOLVERR)) {
    /* printf("unable to connect to chat(nav) server\n"); */
    if (tstconn)
      aim_conn_kill(sess, &tstconn);
    return;
  }

  aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNCOMPLETE, faimtest_conncomplete, 0);
  aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, conninitdone_chat, 0);
  aim_sendcookie(sess, tstconn, redir->cookielen, redir->cookie);
  /* printf("chatnav: connected\n"); */
  return;
}

/* XXX this needs instance too */
void chat_redirect(aim_session_t *sess, struct aim_redirect_data *redir)
{
  aim_conn_t *tstconn;

  owl_function_debugmsg("in chat_redirect");

  tstconn = aim_newconn(sess, AIM_CONN_TYPE_CHAT, redir->ip);
  if (!tstconn || (tstconn->status & AIM_CONN_STATUS_RESOLVERR)) {
    /* printf("unable to connect to chat server\n"); */
    if (tstconn) aim_conn_kill(sess, &tstconn);
    return;
  }
  /* printf("chat: connected to %s instance %d on exchange %d\n", redir->chat.room, redir->chat.instance, redir->chat.exchange); */

  /*
   * We must do this to attach the stored name to the connection!
   */
  aim_chat_attachname(tstconn, redir->chat.exchange, redir->chat.room, redir->chat.instance);
  aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNCOMPLETE, faimtest_conncomplete, 0);
  aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, conninitdone_chat, 0);
  aim_sendcookie(sess, tstconn, redir->cookielen, redir->cookie);
  return;
}

typedef struct _owl_aim_event_source { /*noproto*/
  GSource source;
  aim_session_t *sess;
  GPtrArray *fds;
} owl_aim_event_source;

static void truncate_pollfd_list(owl_aim_event_source *event_source, int len)
{
  GPollFD *fd;
  int i;
  if (len < event_source->fds->len) {
    owl_function_debugmsg("Truncating AIM PollFDs to %d, was %d", len, event_source->fds->len);
    for (i = len; i < event_source->fds->len; i++) {
      fd = event_source->fds->pdata[i];
      g_source_remove_poll(&event_source->source, fd);
      g_free(fd);
    }
    g_ptr_array_remove_range(event_source->fds, len, event_source->fds->len - len);
  }
}

static gboolean owl_aim_event_source_prepare(GSource *source, int *timeout)
{
  owl_aim_event_source *event_source = (owl_aim_event_source*)source;
  aim_conn_t *cur;
  GPollFD *fd;
  int i;

  /* AIM HACK:
   *
   *  The problem - I'm not sure where to hook into the owl/faim
   *  interface to keep track of when the AIM socket(s) open and
   *  close. In particular, the bosconn thing throws me off. So,
   *  rather than register particular dispatchers for AIM, I look up
   *  the relevant FDs and add them to select's watch lists, then
   *  check for them individually before moving on to the other
   *  dispatchers. --asedeno
   */
  i = 0;
  for (cur = event_source->sess->connlist; cur; cur = cur->next) {
    if (cur->fd != -1) {
      /* Add new GPollFDs as necessary. */
      if (i == event_source->fds->len) {
	fd = g_new0(GPollFD, 1);
	g_ptr_array_add(event_source->fds, fd);
	g_source_add_poll(source, fd);
        owl_function_debugmsg("Allocated new AIM PollFD, len = %d", event_source->fds->len);
      }
      fd = event_source->fds->pdata[i];
      fd->fd = cur->fd;
      fd->events = G_IO_IN | G_IO_HUP | G_IO_ERR;
      if (cur->status & AIM_CONN_STATUS_INPROGRESS) {
        /* AIM login requires checking writable sockets. See aim_select. */
	fd->events |= G_IO_OUT;
      }
      i++;
    }
  }
  /* If the number of GPollFDs went down, clean up. */
  truncate_pollfd_list(event_source, i);

  *timeout = -1;
  return FALSE;
}

static gboolean owl_aim_event_source_check(GSource *source)
{
  owl_aim_event_source *event_source = (owl_aim_event_source*)source;
  int i;

  for (i = 0; i < event_source->fds->len; i++) {
    GPollFD *fd = event_source->fds->pdata[i];
    if (fd->revents & fd->events)
      return TRUE;
  }
  return FALSE;
}

static gboolean owl_aim_event_source_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
  owl_aim_event_source *event_source = (owl_aim_event_source*)source;
  owl_aim_process_events(event_source->sess);
  return TRUE;
}

static void owl_aim_event_source_finalize(GSource *source)
{
  owl_aim_event_source *event_source = (owl_aim_event_source*)source;
  /* Don't remove the GPollFDs. We are being finalized, so they'll be removed
   * for us. Moreover, glib will fire asserts if g_source_remove_poll is called
   * on a source which has been destroyed (which occurs when g_source_remove is
   * called on it). */
  owl_ptr_array_free(event_source->fds, g_free);
}

static GSourceFuncs aim_event_funcs = {
  owl_aim_event_source_prepare,
  owl_aim_event_source_check,
  owl_aim_event_source_dispatch,
  owl_aim_event_source_finalize,
};

GSource *owl_aim_event_source_new(aim_session_t *sess)
{
  GSource *source;
  owl_aim_event_source *event_source;

  source = g_source_new(&aim_event_funcs, sizeof(owl_aim_event_source));
  event_source = (owl_aim_event_source *)source;
  event_source->sess = sess;
  /* TODO: When we depend on glib 2.22+, use g_ptr_array_new_with_free_func. */
  event_source->fds = g_ptr_array_new();
  return source;
}
