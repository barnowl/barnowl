#include <stdio.h>
#include <stdio.h>
#include <sys/stat.h>
#include "owl.h"

/**********************************************************************/

struct owlfaim_priv {
  char *aimbinarypath;
  char *screenname;
  char *password;
  char *server;
  char *proxy;
  char *proxyusername;
  char *proxypass;
  char *ohcaptainmycaptain;
  int connected;

  FILE *listingfile;
  char *listingpath;

  fu8_t *buddyicon;
  int buddyiconlen;
  time_t buddyiconstamp;
  fu16_t buddyiconsum;
};

/*
static char *msgerrreasons[] = {
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
*/

static void faimtest_debugcb(aim_session_t *sess, int level, const char *format, va_list va);
static int faimtest_parse_login(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_parse_authresp(aim_session_t *sess, aim_frame_t *fr, ...);
int faimtest_flapversion(aim_session_t *sess, aim_frame_t *fr, ...);
int faimtest_conncomplete(aim_session_t *sess, aim_frame_t *fr, ...);
void addcb_bos(aim_session_t *sess, aim_conn_t *bosconn);
static int conninitdone_bos(aim_session_t *sess, aim_frame_t *fr, ...);
int login(aim_session_t *sess, const char *sn, const char *passwd);
static int conninitdone_admin(aim_session_t *sess, aim_frame_t *fr, ...);
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
static int faimtest_parse_motd(aim_session_t *sess, aim_frame_t *fr, ...);
static int getaimdata(aim_session_t *sess, unsigned char **bufret, int *buflenret, unsigned long offset, unsigned long len, const char *modname);
static int faimtest_memrequest(aim_session_t *sess, aim_frame_t *fr, ...);
static void printuserflags(fu16_t flags);
static int faimtest_parse_userinfo(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_handlecmd(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *userinfo, const char *tmpstr);
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

static int directim_incoming(aim_session_t *sess, aim_frame_t *fr, ...);
static int directim_typing(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_directim_initiate(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_getfile_filereq(aim_session_t *ses, aim_frame_t *fr, ...);
static int faimtest_getfile_filesend(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_getfile_complete(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_getfile_disconnect(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_getfile_listing(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_getfile_listingreq(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_getfile_receive(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_getfile_state4(aim_session_t *sess, aim_frame_t *fr, ...);
static int faimtest_getfile_initiate(aim_session_t *sess, aim_frame_t *fr, ...);
void getfile_start(aim_session_t *sess, aim_conn_t *conn, const char *sn);
void getfile_requested(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *userinfo, struct aim_incomingim_ch2_args *args);
void directim_start(aim_session_t *sess, const char *sn);
void directim_requested(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *userinfo, struct aim_incomingim_ch2_args *args);

void chatnav_redirect(aim_session_t *sess, struct aim_redirect_data *redir);
void chat_redirect(aim_session_t *sess, struct aim_redirect_data *redir);

/*****************************************************************/

void owl_aim_init(void)
{
  aim_session_init(owl_global_get_aimsess(&g), AIM_SESS_FLAGS_NONBLOCKCONNECT, 1);
  aim_setdebuggingcb(owl_global_get_aimsess(&g), faimtest_debugcb);
}

int owl_aim_login(char *screenname, char *password)
{
  int ret;
  struct owlfaim_priv *priv;

  /* this will leak, I know and just don't care right now */
  priv=owl_malloc(sizeof(struct owlfaim_priv));
  memset(priv, 0, sizeof(struct owlfaim_priv));

  priv->screenname = owl_strdup(screenname);
  priv->password = owl_strdup(password);
  priv->server = "login.oscar.aol.com";
  owl_global_get_aimsess(&g)->aux_data = priv;

  
  /* login */
  ret=login(owl_global_get_aimsess(&g), priv->screenname, priv->password);
  if (ret) {
    owl_global_set_aimnologgedin(&g);
    /* printf("login returned %i\n"); */
    return(-1);
  }
  owl_global_set_aimloggedin(&g, screenname);
  return(0);
}

void owl_aim_logout(void)
{
  /* need to check if it's connected first, I think */
  logout(owl_global_get_aimsess(&g));
  owl_global_set_aimnologgedin(&g);
}

int owl_aim_send_im(char *to, char *msg)
{
  aim_send_im(owl_global_get_aimsess(&g), to, AIM_IMFLAGS_ACK, msg);

  /* I don't know how to check for an error yet */
  return(0);
}

void owl_aim_addbuddy(char *screenname)
{
  const char **foo;
  
  /*
  aim_bos_setbuddylist(owl_global_get_aimsess(&g),
		       aim_getconn_type(owl_global_get_aimsess(&g), AIM_CONN_TYPE_BOS),
		       foo);
  */

  /*
  aim_add_buddy(owl_global_get_aimsess(&g),
		aim_getconn_type(owl_global_get_aimsess(&g), AIM_CONN_TYPE_BOS),
		screenname);
  */

  foo=(char *) owl_malloc(30);
  foo[0]=screenname;
  aim_ssi_addbuddies(owl_global_get_aimsess(&g),
		     aim_getconn_type(owl_global_get_aimsess(&g), AIM_CONN_TYPE_BOS),
		     "Buddies", foo, 1);
  owl_free(foo);

}

void owl_aim_delbuddy(char *screenname)
{
  /*
  aim_remove_buddy(owl_global_get_aimsess(&g),
		   aim_getconn_type(owl_global_get_aimsess(&g), AIM_CONN_TYPE_BOS),
		   screenname);
  */

  aim_ssi_delbuddies(owl_global_get_aimsess(&g),
		     aim_getconn_type(owl_global_get_aimsess(&g), AIM_CONN_TYPE_BOS),
		     "Buddies", &screenname, 1);

  /* for some reason we don't get an offgoing notice, so we'll remove
   * manually
   */

  owl_buddylist_offgoing(owl_global_get_buddylist(&g), screenname);


}

void owl_aim_chat_join(char *chatroom)
{
}

void owl_aim_chat_leave(char *chatroom)
{
}

int owl_aim_chat_sendmsg(char *chatroom, char *msg)
{
  return(0);
}

/* caller must free the return */
char *owl_aim_normalize_screenname(char *in) {
  char *out;
  int i, j, k;

  j=strlen(in);
  out=owl_malloc(j+30);
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

int owl_aim_process_events() {
  aim_session_t *aimsess;
  aim_conn_t *waitingconn = NULL;
  struct timeval tv;
  int selstat = 0;
  struct owlfaim_priv *priv;

  aimsess=owl_global_get_aimsess(&g);
  priv = (struct owlfaim_priv *) &(aimsess->aux_data);

  /* do a select without blocking */
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  waitingconn = aim_select(aimsess, &tv, &selstat);

  if (owl_global_is_aimnop_time(&g)) {
    aim_flap_nop(aimsess, aim_getconn_type(aimsess, AIM_CONN_TYPE_BOS));
    owl_global_aimnop_sent(&g);
  }

  if (selstat == -1) { /* error */
    /* keepgoing = 0; */ /* fall through */
  } else if (selstat == 0) { /* no events pending */
    /* printf("selstat == 0\n"); */
  } else if (selstat == 1) { /* outgoing data pending */
    /* printf("selstat == 1\n"); */
    aim_tx_flushqueue(aimsess);
  } else if (selstat == 2) { /* incoming data pending */
    /* printf("selstat == 2\n"); */
    if (waitingconn->type == AIM_CONN_TYPE_RENDEZVOUS_OUT) {
      if (aim_handlerendconnect(aimsess, waitingconn) < 0) {
	/* printf("connection error (rend out)\n"); */
	aim_conn_kill(aimsess, &waitingconn);
      }
    } else {
      if (aim_get_command(aimsess, waitingconn) >= 0) {
	aim_rxdispatch(aimsess);
      } else {
	/* printf("connection error (type 0x%04x:0x%04x)\n", waitingconn->type, waitingconn->subtype); */
	/* we should have callbacks for all these, else the library will do the conn_kill for us. */
	if (waitingconn->type == AIM_CONN_TYPE_RENDEZVOUS) {
	  if (waitingconn->subtype == AIM_CONN_SUBTYPE_OFT_DIRECTIM) {
	    /* printf("disconnected from %s\n", aim_directim_getsn(waitingconn)); */
	    aim_conn_kill(aimsess, &waitingconn);
	  }
	} else {
	  aim_conn_kill(aimsess, &waitingconn);
	}
	if (!aim_getconn_type(aimsess, AIM_CONN_TYPE_BOS)) {
	  /* printf("major connection error\n"); */
	  /* break; */
	  /* we should probably kill the session */
	}
      }
    }
  }
  /* free(priv->buddyicon); */
  /* exit(0); */
  return(0);
}

static void faimtest_debugcb(aim_session_t *sess, int level, const char *format, va_list va)
{
  return;
}


static int faimtest_parse_login(aim_session_t *sess, aim_frame_t *fr, ...)
{
  struct owlfaim_priv *priv = (struct owlfaim_priv *)sess->aux_data;
  struct client_info_s info = CLIENTINFO_AIM_KNOWNGOOD;
    
  char *key;
  va_list ap;

  va_start(ap, fr);
  key = va_arg(ap, char *);
  va_end(ap);

  aim_send_login(sess, fr->conn, priv->screenname, priv->password, &info, key);
  
  return 1;
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

  /*
   * Check for error.
   */
  if (info->errorcode || !info->bosip || !info->cookie) {
    /*
    printf("Login Error Code 0x%04x\n", info->errorcode);
    printf("Error URL: %s\n", info->errorurl);
    */
    aim_conn_kill(sess, &fr->conn);
    return 1;
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
    return 1;
  } else if (bosconn->status & AIM_CONN_STATUS_CONNERR) {
    /* printf("could not connect to BOS\n"); */
    aim_conn_kill(sess, &bosconn);
    return 1;
  }
  addcb_bos(sess, bosconn);
  aim_sendcookie(sess, bosconn, info->cookie);
  return 1;
}

int faimtest_flapversion(aim_session_t *sess, aim_frame_t *fr, ...)
{
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
  return 1;
}

void addcb_bos(aim_session_t *sess, aim_conn_t *bosconn)
{

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
  aim_conn_addhandler(sess, bosconn, 0x0001,         0x001f,                        faimtest_memrequest, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_BUD, AIM_CB_BUD_ONCOMING,           faimtest_parse_oncoming, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_BUD, AIM_CB_BUD_OFFGOING,           faimtest_parse_offgoing, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_INCOMING,           faimtest_parse_incoming_im, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_LOC, AIM_CB_LOC_ERROR,              faimtest_parse_locerr, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_MISSEDCALL,         faimtest_parse_misses, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_RATECHANGE,         faimtest_parse_ratechange, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_EVIL,               faimtest_parse_evilnotify, 0);
  aim_conn_addhandler(sess, bosconn, 0x000a,         0x0001,                        faimtest_parse_searcherror, 0);
  aim_conn_addhandler(sess, bosconn, 0x000a,         0x0003,                        faimtest_parse_searchreply, 0);
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

  #ifdef MID_REWROTE_ALL_THE_CRAP
  aim_conn_addhandler(sess, bosconn, 0xffff,         0xffff,                        faimtest_parse_unknown, 0);
  #endif

  return;
}

static int conninitdone_bos(aim_session_t *sess, aim_frame_t *fr, ...)
{
  char buddies[128]; /* this is the new buddy list */
  char profile[256]; /* this is the new profile */
  /* char awaymsg[] = {"blah blah blah Ole! blah blah blah"}; */

  /* Caution: Buddy1 and Buddy2 are real people! (who I don't know) */
  /*  snprintf(buddies, sizeof(buddies), "Buddy1&Buddy2&%s&", priv->ohcaptainmycaptain ? priv->ohcaptainmycaptain : "blah");
  snprintf(profile, sizeof(profile), "Hello.<br>My captain is %s.  They were dumb enough to leave this message in their client, or they are using faimtest.  Shame
on them.", priv->ohcaptainmycaptain);
  */

  /*
  snprintf(buddies, sizeof(buddies), "Buddy1&Buddy2&");
  snprintf(profile, sizeof(profile), "Hello.<br>This is a test");
  */
  strcpy(profile, "");

  aim_reqpersonalinfo(sess, fr->conn);
  aim_bos_reqlocaterights(sess, fr->conn);

  /* aim_bos_setprofile(sess, fr->conn, profile, awaymsg, AIM_CAPS_BUDDYICON | AIM_CAPS_CHAT | AIM_CAPS_GETFILE | AIM_CAPS_SENDFILE | AIM_CAPS_IMIMAGE | AIM_CAPS_GAMES | AIM_CAPS_SAVESTOCKS | AIM_CAPS_SENDBUDDYLIST | AIM_CAPS_ICQ | AIM_CAPS_ICQUNKNOWN | AIM_CAPS_ICQRTF | AIM_CAPS_ICQSERVERRELAY | AIM_CAPS_TRILLIANCRYPT); */
  aim_bos_setprofile(sess, fr->conn, "", "", AIM_CAPS_SENDBUDDYLIST );
  aim_bos_reqbuddyrights(sess, fr->conn);

  /* send the buddy list and profile (required, even if empty) */
  aim_bos_setbuddylist(sess, fr->conn, buddies);

  aim_reqicbmparams(sess);

  aim_bos_reqrights(sess, fr->conn);
  /* set group permissions -- all user classes */
  aim_bos_setgroupperm(sess, fr->conn, AIM_FLAG_ALLUSERS);
  aim_bos_setprivacyflags(sess, fr->conn, AIM_PRIVFLAGS_ALLOWIDLE);

  return 1;
}

int login(aim_session_t *sess, const char *sn, const char *passwd)
{
  struct owlfaim_priv *priv = (struct owlfaim_priv *)sess->aux_data;
  aim_conn_t *authconn;

  if (sn) priv->screenname = strdup(sn);
  if (passwd) priv->password = strdup(passwd);
  if (priv->proxy) aim_setupproxy(sess, priv->proxy, priv->proxyusername, priv->proxypass);

  if (!priv->screenname || !priv->password) {
    /* printf("need SN and password\n"); */
    return(-1);
  }

  if (!(authconn = aim_newconn(sess, AIM_CONN_TYPE_AUTH, priv->server ? priv->server : FAIM_LOGIN_SERVER))) {
    /* printf("internal connection error during login\n"); */
    return (-1);
  } else if (authconn->fd == -1) {
    if (authconn->status & AIM_CONN_STATUS_RESOLVERR) {
      /* printf("could not resolve authorizer name\n");*/
    } else if (authconn->status & AIM_CONN_STATUS_CONNERR) {
      /* printf("could not connect to authorizer\n"); */
    }
    aim_conn_kill(sess, &authconn);
    return(-1);
  }

  aim_conn_addhandler(sess, authconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_FLAPVER, faimtest_flapversion, 0);
  aim_conn_addhandler(sess, authconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNCOMPLETE, faimtest_conncomplete, 0);
  aim_conn_addhandler(sess, authconn, 0x0017, 0x0007, faimtest_parse_login, 0);
  aim_conn_addhandler(sess, authconn, 0x0017, 0x0003, faimtest_parse_authresp, 0);

  /* If the connection is in progress, this will just be queued */
  aim_request_login(sess, authconn, priv->screenname);

  /* printf("login request sent\n"); */
  return(0);
}


static int conninitdone_admin(aim_session_t *sess, aim_frame_t *fr, ...)
{
  aim_clientready(sess, fr->conn);
  /* printf("initialization done for admin connection\n"); */
  return(1);
}

int logout(aim_session_t *sess)
{
  aim_session_kill(sess);
  owl_aim_init();
  
  /* kretch
  if (faimtest_init() == -1)
    printf("faimtest_init failed\n");
  */

  return(0);
}

/**************************************************************************************************/

static int faimtest_parse_connerr(aim_session_t *sess, aim_frame_t *fr, ...)
{
  struct owlfaim_priv *priv = (struct owlfaim_priv *)sess->aux_data;
  va_list ap;
  fu16_t code;
  char *msg;
  
  va_start(ap, fr);
  code = va_arg(ap, int);
  msg = va_arg(ap, char *);
  va_end(ap);
  
  /* printf("connerr: Code 0x%04x: %s\n", code, msg); */
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
  
  /* printf("account confirmation returned status 0x%04x (%s)\n", status, (status==0x0000)?"email sent":"unknown"); */
  
  return 1;
}

static int faimtest_infochange(aim_session_t *sess, aim_frame_t *fr, ...)
{
  fu16_t change = 0, perms, type;
  int length, str;
  char *val;
  va_list ap;
  
  va_start(ap, fr);
  change = va_arg(ap, int);
  perms = (fu16_t)va_arg(ap, unsigned int);
  type = (fu16_t)va_arg(ap, unsigned int);
  length = va_arg(ap, int);
  val = va_arg(ap, char *);
  str = va_arg(ap, int);
  va_end(ap);
  
  /* printf("info%s: perms = %d, type = %x, length = %d, val = %s\n", change?" change":"", perms, type, length, str?val:"(not string)"); */
  
  return 1;
}


static int faimtest_handleredirect(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  struct aim_redirect_data *redir;
  
  va_start(ap, fr);
  redir = va_arg(ap, struct aim_redirect_data *);
  
  if (redir->group == 0x0005) {  /* Adverts */
    
  } else if (redir->group == 0x0007) {  /* Authorizer */
    aim_conn_t *tstconn;
    
    tstconn = aim_newconn(sess, AIM_CONN_TYPE_AUTH, redir->ip);
    if (!tstconn || (tstconn->status & AIM_CONN_STATUS_RESOLVERR)) {
      /* printf("unable to reconnect with authorizer\n"); */
    } else {
      aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_FLAPVER, faimtest_flapversion, 0);
      aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNCOMPLETE, faimtest_conncomplete, 0);
      aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, conninitdone_admin, 0);
      aim_conn_addhandler(sess, tstconn, 0x0007, 0x0007, faimtest_accountconfirm, 0);
      aim_conn_addhandler(sess, tstconn, 0x0007, 0x0003, faimtest_infochange, 0);
      aim_conn_addhandler(sess, tstconn, 0x0007, 0x0005, faimtest_infochange, 0);
      /* Send the cookie to the Auth */
      aim_sendcookie(sess, tstconn, redir->cookie);
      /* printf("sent cookie to authorizer host\n"); */
    }
  } else if (redir->group == 0x000d) {  /* ChatNav */
    chatnav_redirect(sess, redir);
  } else if (redir->group == 0x000e) { /* Chat */
    chat_redirect(sess, redir);
  } else {
    /* printf("uh oh... got redirect for unknown service 0x%04x!!\n", redir->group); */
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
  
  /* printf("ICBM Parameters: maxchannel = %d, default flags = 0x%08lx, max msg len = %d, max sender evil = %f, max reciever evil = %f, min msg interval = %ld\n", params->maxchan, params->flags, params->maxmsglen, ((float)params->maxsenderwarn)/10.0, ((float)params->maxrecverwarn)/10.0, params->minmsginterval); */
     
  /*
  * Set these to your taste, or client medium.  Setting minmsginterval
  * higher is good for keeping yourself from getting flooded (esp
  * if you're on a slow connection or something where that would be
  * useful).
  */
  params->maxmsglen = 8000;
  params->minmsginterval = 0; /* in milliseconds */
  aim_seticbmparam(sess, params);
  
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
  
  /* printf("buddy list rights: Max buddies = %d / Max watchers = %d\n", maxbuddies, maxwatchers); */
  
  aim_ssi_reqrights(sess, fr->conn);
  
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
  
  /* printf("BOS rights: Max permit = %d / Max deny = %d\n", maxpermits, maxdenies); */
  aim_clientready(sess, fr->conn);
  /* printf("officially connected to BOS.\n"); */
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
  
  /* printf("locate rights: max signature length = %d\n", maxsiglen); */
  
  return 1;
}

static int faimtest_reportinterval(aim_session_t *sess, aim_frame_t *fr, ...)
{
  struct owlfaim_priv *priv = (struct owlfaim_priv *)sess->aux_data;
  va_list ap;
  fu16_t interval;
  
  va_start(ap, fr);
  interval = va_arg(ap, int);
  va_end(ap);
  
  /* printf("minimum report interval: %d (seconds?)\n", interval); */
  
  if (!priv->connected) {
    priv->connected++;
  }
  aim_reqicbmparams(sess);
  return 1;
}

static int faimtest_parse_motd(aim_session_t *sess, aim_frame_t *fr, ...)
{
  char *msg;
  fu16_t id;
  va_list ap;
  /* static int codeslen = 5;
  static char *codes[] = {
    "Unknown",
    "Mandatory upgrade",
    "Advisory upgrade",
    "System bulletin",
    "Top o' the world!"
  };
  */

  return 1;
  
  va_start(ap, fr);
  id = va_arg(ap, int);
  msg = va_arg(ap, char *);
  va_end(ap);
  
  /* printf("motd: %s (%d / %s)\n", msg, id, (id < codeslen)?codes[id]:"unknown"); */
  
  return 1;
}

/*
 * This is a little more complicated than it looks.  The module
 * name (proto, boscore, etc) may or may not be given.  If it is
 * not given, then use aim.exe.  If it is given, put ".ocm" on the
 * end of it.
 *
 * Now, if the offset or length requested would cause a read past
 * the end of the file, then the request is considered invalid.  Invalid
 * requests are processed specially.  The value hashed is the
 * the request, put into little-endian (eight bytes: offset followed
 * by length).  
 *
 * Additionally, if the request is valid, the length is mod 4096.  It is
 * important that the length is checked for validity first before doing
 * the mod.
 *
 * Note to Bosco's Brigade: if you'd like to break this, put the 
 * module name on an invalid request.
 *
 */
static int getaimdata(aim_session_t *sess, unsigned char **bufret, int *buflenret, unsigned long offset, unsigned long len, const char *modname)
{
  struct owlfaim_priv *priv = (struct owlfaim_priv *)sess->aux_data;
  FILE *f;
  static const char defaultmod[] = "aim.exe";
  char *filename = NULL;
  struct stat st;
  unsigned char *buf;
  int invalid = 0;
  
  if (!bufret || !buflenret)
    return -1;
  
  if (modname) {
    if (!(filename = malloc(strlen(priv->aimbinarypath)+1+strlen(modname)+4+1))) {
      /* perror("memrequest: malloc"); */
      return -1;
    }
    sprintf(filename, "%s/%s.ocm", priv->aimbinarypath, modname);
  } else {
    if (!(filename = malloc(strlen(priv->aimbinarypath)+1+strlen(defaultmod)+1))) {
      /* perror("memrequest: malloc"); */
      return -1;
    }
    sprintf(filename, "%s/%s", priv->aimbinarypath, defaultmod);
  }
  
  if (stat(filename, &st) == -1) {
    if (!modname) {
      /* perror("memrequest: stat"); */
      free(filename);
      return -1;
    }
    invalid = 1;
  }
  
  if (!invalid) {
    if ((offset > st.st_size) || (len > st.st_size))
      invalid = 1;
    else if ((st.st_size - offset) < len)
      len = st.st_size - offset;
    else if ((st.st_size - len) < len)
      len = st.st_size - len;
  }
  
  if (!invalid && len) {
    len %= 4096;
  }
  
  if (invalid) {
    int i;
    
    free(filename); /* not needed */
    printf("memrequest: recieved invalid request for 0x%08lx bytes at 0x%08lx (file %s)\n", len, offset, modname);
    i = 8;
    if (modname) {
      i+=strlen(modname);
    }
    
    if (!(buf = malloc(i))) {
      return -1;
    }
    
    i=0;
    
    if (modname) {
      memcpy(buf, modname, strlen(modname));
      i+=strlen(modname);
    }
    
    /* Damn endianness. This must be little (LSB first) endian. */
    buf[i++] = offset & 0xff;
    buf[i++] = (offset >> 8) & 0xff;
    buf[i++] = (offset >> 16) & 0xff;
    buf[i++] = (offset >> 24) & 0xff;
    buf[i++] = len & 0xff;
    buf[i++] = (len >> 8) & 0xff;
    buf[i++] = (len >> 16) & 0xff;
    buf[i++] = (len >> 24) & 0xff;
    
    *bufret = buf;
    *buflenret = i;
  } else {
    if (!(buf = malloc(len))) {
      free(filename);
      return -1;
    }
    /* printf("memrequest: loading %ld bytes from 0x%08lx in \"%s\"...\n", len, offset, filename); */
    if (!(f = fopen(filename, "r"))) {
      /* perror("memrequest: fopen"); */
      free(filename);
      free(buf);
      return -1;
    }
    
    free(filename);
    
    if (fseek(f, offset, SEEK_SET) == -1) {
      /* perror("memrequest: fseek"); */
      fclose(f);
      free(buf);
      return -1;
    }
    
    if (fread(buf, len, 1, f) != 1) {
      /* perror("memrequest: fread"); */
      fclose(f);
      free(buf);
      return -1;
    }
    
    fclose(f);
    *bufret = buf;
    *buflenret = len;
  }
  return 0; /* success! */
}

/*
 * This will get an offset and a length.  The client should read this
 * data out of whatever AIM.EXE binary the user has provided (hopefully
 * it matches the client information thats sent at login) and pass a
 * buffer back to libfaim so it can hash the data and send it to AOL for
 * inspection by the client police.
 */
static int faimtest_memrequest(aim_session_t *sess, aim_frame_t *fr, ...)
{
  struct owlfaim_priv *priv = (struct owlfaim_priv *)sess->aux_data;
  va_list ap;
  fu32_t offset, len;
  char *modname;
  unsigned char *buf;
  int buflen;
  
  va_start(ap, fr);
  offset = va_arg(ap, fu32_t);
  len = va_arg(ap, fu32_t);
  modname = va_arg(ap, char *);
  va_end(ap);
  
  if (priv->aimbinarypath && (getaimdata(sess, &buf, &buflen, offset, len, modname) == 0)) {
    aim_sendmemblock(sess, fr->conn, offset, buflen, buf, AIM_SENDMEMBLOCK_FLAG_ISREQUEST);
    free(buf);
  } else {
    /* printf("memrequest: unable to use AIM binary (\"%s/%s\"), sending defaults...\n", priv->aimbinarypath, modname); */
    aim_sendmemblock(sess, fr->conn, offset, len, NULL, AIM_SENDMEMBLOCK_FLAG_ISREQUEST);
  }
  return 1;
}

static void printuserflags(fu16_t flags)
{

  if (flags & AIM_FLAG_UNCONFIRMED)
    printf("UNCONFIRMED ");
  if (flags & AIM_FLAG_ADMINISTRATOR)
    printf("ADMINISTRATOR ");
  if (flags & AIM_FLAG_AOL)
    printf("AOL ");
  if (flags & AIM_FLAG_OSCAR_PAY)
    printf("OSCAR_PAY ");
  if (flags & AIM_FLAG_FREE)
    printf("FREE ");
  if (flags & AIM_FLAG_AWAY)
    printf("AWAY ");
  if (flags & AIM_FLAG_ICQ)
    printf("ICQ ");
  if (flags & AIM_FLAG_WIRELESS)
    printf("WIRELESS ");
  if (flags & AIM_FLAG_ACTIVEBUDDY)
    printf("ACTIVEBUDDY ");
  
  return;
}

static int faimtest_parse_userinfo(aim_session_t *sess, aim_frame_t *fr, ...)
{
  aim_userinfo_t *userinfo;
  char *prof_encoding = NULL;
  char *prof = NULL;
  fu16_t inforeq = 0;

  va_list ap;
  va_start(ap, fr);
  userinfo = va_arg(ap, aim_userinfo_t *);
  inforeq = (fu16_t)va_arg(ap, unsigned int);
  prof_encoding = va_arg(ap, char *);
  prof = va_arg(ap, char *);
  va_end(ap);

  return (1);

  printf("userinfo: sn: %s\n", userinfo->sn);
  printf("userinfo: warnlevel: %f\n", aim_userinfo_warnlevel(userinfo));
  printf("userinfo: flags: 0x%04x = ", userinfo->flags);
  printuserflags(userinfo->flags);
  printf("\n");
  
  printf("userinfo: membersince: %lu\n", userinfo->membersince);
  printf("userinfo: onlinesince: %lu\n", userinfo->onlinesince);
  printf("userinfo: idletime: 0x%04x\n", userinfo->idletime);
  printf("userinfo: capabilities = %s = 0x%08lx\n", (userinfo->present & AIM_USERINFO_PRESENT_CAPABILITIES) ? "present" : "not present", userinfo->capabilities);
  
  
  if (inforeq == AIM_GETINFO_GENERALINFO) {
    printf("userinfo: profile_encoding: %s\n", prof_encoding ? prof_encoding : "[none]");
    printf("userinfo: prof: %s\n", prof ? prof : "[none]");
  } else if (inforeq == AIM_GETINFO_AWAYMESSAGE) {
    printf("userinfo: awaymsg_encoding: %s\n", prof_encoding ? prof_encoding : "[none]");
    printf("userinfo: awaymsg: %s\n", prof ? prof : "[none]");
  } else if (inforeq == AIM_GETINFO_CAPABILITIES) {
    printf("userinfo: capabilities: see above\n");
  } else {
    printf("userinfo: unknown info request\n");
  }
  
  return 1;
}

static int faimtest_handlecmd(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *userinfo, const char *tmpstr)
{
  struct owlfaim_priv *priv = (struct owlfaim_priv *)sess->aux_data;
  
  if (!strncmp(tmpstr, "disconnect", 10)) {
    logout(sess);
  } else if (strstr(tmpstr, "goodday")) {
    aim_send_im(sess, userinfo->sn, AIM_IMFLAGS_ACK, "Good day to you too.");
  } else if (strstr(tmpstr, "haveicon") && priv->buddyicon) {
    struct aim_sendimext_args args;
    /* static const char iconmsg[] = {"I have an icon"}; */
    static const char iconmsg[] = {""};
    
    args.destsn = userinfo->sn;
    args.flags = AIM_IMFLAGS_HASICON;
    args.msg = iconmsg;
    args.msglen = strlen(iconmsg);
    args.iconlen = priv->buddyiconlen;
    args.iconstamp = priv->buddyiconstamp;
    args.iconsum = priv->buddyiconsum;
    
    /* aim_send_im_ext(sess, &args); */
    
  } else if (strstr(tmpstr, "sendbin")) {
    struct aim_sendimext_args args;
    static const unsigned char data[] = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
      0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
      0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
      0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    };
    
    /*
     * I put this here as a demonstration of how to send 
     * arbitrary binary data via OSCAR ICBM's without the need
     * for escape or baseN encoding of any sort.  
     *
     * Apparently if you set the charset to something WinAIM
     * doesn't recognize, it will completly ignore the message.
     * That is, it will not display anything in the conversation
     * window for the user that recieved it.
     *
     * HOWEVER, if they do not have a conversation window open
     * for you, a new one will be created, but it will not have
     * any messages in it.  Therefore sending these things could
     * be a great way to seemingly subliminally convince people
     * to talk to you...
     *
     */
    args.destsn = userinfo->sn;
    args.flags = AIM_IMFLAGS_CUSTOMCHARSET;
    args.charset = args.charsubset = 0x4242;
    args.msg = data;
    args.msglen = sizeof(data);
    aim_send_im_ext(sess, &args);
   } else if (strstr(tmpstr, "sendmulti")) {
    struct aim_sendimext_args args;
    aim_mpmsg_t mpm;
    static const fu16_t unidata[] = { /* "UNICODE." */
      0x0055, 0x004e, 0x0049, 0x0043,
      0x004f, 0x0044, 0x0045, 0x002e,
    };
    static const int unidatalen = 8;
    
    /*
     * This is how multipart messages should be sent.
     *
     * This should render as:
     *        "Part 1, ASCII.  UNICODE.Part 3, ASCII.  "
     */
    
    aim_mpmsg_init(sess, &mpm);
    aim_mpmsg_addascii(sess, &mpm, "Part 1, ASCII.  ");
    aim_mpmsg_addunicode(sess, &mpm, unidata, unidatalen);
    aim_mpmsg_addascii(sess, &mpm, "Part 3, ASCII.  ");
    
    args.destsn = userinfo->sn;
    args.flags = AIM_IMFLAGS_MULTIPART;
    args.mpmsg = &mpm;
    
    aim_send_im_ext(sess, &args);
    
    aim_mpmsg_free(sess, &mpm);
    
  } else if (strstr(tmpstr, "sendprebin")) {
    static const unsigned char data[] = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
      0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
      0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
      0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    };
    struct aim_sendimext_args args;
    aim_mpmsg_t mpm;
    
    /*
     * This demonstrates sending a human-readable preamble,
     * and then arbitrary binary data.
     *
     * This means that you can very inconspicuously send binary
     * attachments to other users.  In WinAIM, this appears as
     * though it only had the ASCII portion.
     *
     */
    
    aim_mpmsg_init(sess, &mpm);
    aim_mpmsg_addascii(sess, &mpm, "This message has binary data.");
    aim_mpmsg_addraw(sess, &mpm, 0x4242, 0x4242, data, sizeof(data));
    
    args.destsn = userinfo->sn;
    args.flags = AIM_IMFLAGS_MULTIPART;
    args.mpmsg = &mpm;
    
    aim_send_im_ext(sess, &args);
    aim_mpmsg_free(sess, &mpm);
    
  } else if (strstr(tmpstr, "havefeat")) {
    struct aim_sendimext_args args;
    static const char featmsg[] = {"I have nifty features."};
    fu8_t features[] = {0x01, 0x01, 0x01, 0x02, 0x42, 0x43, 0x44, 0x45};
    
    args.destsn = userinfo->sn;
    args.flags = AIM_IMFLAGS_CUSTOMFEATURES;
    args.msg = featmsg;
    args.msglen = strlen(featmsg);
    args.features = features;
    args.featureslen = sizeof(features);
    
    aim_send_im_ext(sess, &args);
  } else if (strstr(tmpstr, "sendicon") && priv->buddyicon) {
    aim_send_icon(sess, userinfo->sn, priv->buddyicon, priv->buddyiconlen, priv->buddyiconstamp, priv->buddyiconsum);
  } else if (strstr(tmpstr, "warnme")) {
    /* printf("icbm: sending non-anon warning\n"); */
    aim_send_warning(sess, conn, userinfo->sn, 0);
  } else if (strstr(tmpstr, "anonwarn")) {
    /* printf("icbm: sending anon warning\n"); */
    aim_send_warning(sess, conn, userinfo->sn, AIM_WARN_ANON);
  } else if (strstr(tmpstr, "setdirectoryinfo")) {
    /* printf("icbm: sending backwards profile data\n"); */
    aim_setdirectoryinfo(sess, conn, "tsrif", "elddim", "tsal", "nediam", "emankcin", "teerts", "ytic", "etats", "piz", 0, 1);
  } else if (strstr(tmpstr, "setinterests")) {
    /* printf("icbm: setting fun interests\n"); */
    aim_setuserinterests(sess, conn, "interest1", "interest2", "interest3", "interest4", "interest5", 1);
  } else if (!strncmp(tmpstr, "getfile", 7)) {
    if (!priv->ohcaptainmycaptain) {
      aim_send_im(sess, userinfo->sn, AIM_IMFLAGS_ACK, "I have no owner!");
    } else {
      getfile_start(sess, conn, (strlen(tmpstr) < 8)?priv->ohcaptainmycaptain:tmpstr+8);
    }
  } else if (!strncmp(tmpstr, "open chatnav", 12)) {
    aim_reqservice(sess, conn, AIM_CONN_TYPE_CHATNAV);
  } else if (!strncmp(tmpstr, "create", 6)) {
    aim_chatnav_createroom(sess,aim_getconn_type(sess, AIM_CONN_TYPE_CHATNAV), (strlen(tmpstr) < 7)?"WorldDomination":tmpstr+7, 0x0004);
  } else if (!strncmp(tmpstr, "close chatnav", 13)) {
    aim_conn_t *chatnavconn;
    if ((chatnavconn = aim_getconn_type(sess, AIM_CONN_TYPE_CHATNAV)))
      aim_conn_kill(sess, &chatnavconn);
  } else if (!strncmp(tmpstr, "join", 4)) {
    aim_chat_join(sess, conn, 0x0004, "worlddomination", 0x0000);
  } else if (!strncmp(tmpstr, "leave", 5)) {
    aim_chat_leaveroom(sess, "worlddomination");
  } else if (!strncmp(tmpstr, "getinfo", 7)) {
    aim_getinfo(sess, conn, "midendian", AIM_GETINFO_GENERALINFO);
    aim_getinfo(sess, conn, "midendian", AIM_GETINFO_AWAYMESSAGE);
    aim_getinfo(sess, conn, "midendian", AIM_GETINFO_CAPABILITIES);
  } else if (strstr(tmpstr, "open directim")) {
    directim_start(sess, userinfo->sn);
  } else if(strstr(tmpstr, "lookup")) {
    aim_usersearch_address(sess, conn, "mid@auk.cx");
  } else if (!strncmp(tmpstr, "reqsendmsg", 10)) {
    aim_send_im(sess, priv->ohcaptainmycaptain, 0, "sendmsg 7900");
  } else if (!strncmp(tmpstr, "reqadmin", 8)) {
    aim_reqservice(sess, conn, AIM_CONN_TYPE_AUTH);
  } else if (!strncmp(tmpstr, "changenick", 10)) {
    aim_admin_setnick(sess, aim_getconn_type(sess, AIM_CONN_TYPE_AUTH), "diputs8  1");
  } else if (!strncmp(tmpstr, "reqconfirm", 10)) {
    aim_admin_reqconfirm(sess, aim_getconn_type(sess, AIM_CONN_TYPE_AUTH));
  } else if (!strncmp(tmpstr, "reqemail", 8)) {
    aim_admin_getinfo(sess, aim_getconn_type(sess, AIM_CONN_TYPE_AUTH), 0x0011);
  } else if (!strncmp(tmpstr, "changepass", 8)) {
    aim_admin_changepasswd(sess, aim_getconn_type(sess, AIM_CONN_TYPE_AUTH), "NEWPASSWORD", "OLDPASSWORD");
  } else if (!strncmp(tmpstr, "setemail", 8)) {
    aim_admin_setemail(sess, aim_getconn_type(sess, AIM_CONN_TYPE_AUTH), "NEWEMAILADDRESS");
  } else if (!strncmp(tmpstr, "sendmsg", 7)) {
    int i;
    
    i = atoi(tmpstr+8);
    if (i < 10000) {
      char *newbuf;
      int z;
      
      newbuf = malloc(i+1);
      for (z = 0; z < i; z++)
	newbuf[z] = (z % 10)+0x30;
      newbuf[i] = '\0';
      aim_send_im(sess, userinfo->sn, AIM_IMFLAGS_ACK | AIM_IMFLAGS_AWAY, newbuf);
      free(newbuf);
    }
  } else if (strstr(tmpstr, "seticqstatus")) {
    aim_setextstatus(sess, conn, AIM_ICQ_STATE_DND);
  } else if (strstr(tmpstr, "rtfmsg")) {
    static const char rtfmsg[] = {"{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1033{\\fonttbl{\\f0\\fswiss\\fcharset0 Arial;}{\\f1\\froman\\fprq2\\fcharset0 Georgia;}{\\f2\\fmodern\\fprq1\\fcharset0 MS Mincho;}{\\f3\\froman\\fprq2\\fcharset2 Symbol;}}\\viewkind4\\uc1\\pard\\f0\\fs20 Test\\f1 test\\f2\fs44 test\\f3\\fs20 test\\f0\\par}"};
    struct aim_sendrtfmsg_args rtfargs;
    
    memset(&rtfargs, 0, sizeof(rtfargs));
    rtfargs.destsn = userinfo->sn;
    rtfargs.fgcolor = 0xffffffff;
    rtfargs.bgcolor = 0x00000000;
    rtfargs.rtfmsg = rtfmsg;
    aim_send_rtfmsg(sess, &rtfargs);
  } else {
    /* printf("unknown command.\n"); */
    aim_add_buddy(sess, conn, userinfo->sn);
  }  
  
  return 0;
}

/*
 * Channel 1: Standard Message
 */
static int faimtest_parse_incoming_im_chan1(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *userinfo, struct aim_incomingim_ch1_args *args)
{
  struct owlfaim_priv *priv = (struct owlfaim_priv *)sess->aux_data;
  int clienttype = AIM_CLIENTTYPE_UNKNOWN;
  owl_message *m;
  char *stripmsg, *nz_screenname, *wrapmsg;
  char realmsg[8192+1] = {""};
  clienttype = aim_fingerprintclient(args->features, args->featureslen);

  /*
  printf("icbm: sn = \"%s\"\n", userinfo->sn);
  printf("icbm: probable client type: %d\n", clienttype);
  printf("icbm: warnlevel = %f\n", aim_userinfo_warnlevel(userinfo));
  printf("icbm: flags = 0x%04x = ", userinfo->flags);
  printuserflags(userinfo->flags);
  printf("\n");
  */

  /*
  printf("icbm: membersince = %lu\n", userinfo->membersince);
  printf("icbm: onlinesince = %lu\n", userinfo->onlinesince);
  printf("icbm: idletime = 0x%04x\n", userinfo->idletime);
  printf("icbm: capabilities = %s = 0x%08lx\n", (userinfo->present & AIM_USERINFO_PRESENT_CAPABILITIES) ? "present" : "not present", userinfo->capabilities);
  */

  /*
  printf("icbm: icbmflags = ");
  if (args->icbmflags & AIM_IMFLAGS_AWAY) printf("away ");
  if (args->icbmflags & AIM_IMFLAGS_ACK) printf("ackrequest ");
  if (args->icbmflags & AIM_IMFLAGS_OFFLINE) printf("offline ");
  if (args->icbmflags & AIM_IMFLAGS_BUDDYREQ) printf("buddyreq ");
  if (args->icbmflags & AIM_IMFLAGS_HASICON) printf("hasicon ");
  printf("\n");
  */

  if (args->icbmflags & AIM_IMFLAGS_CUSTOMCHARSET) {
    /* printf("icbm: encoding flags = {%04x, %04x}\n", args->charset, args->charsubset); */
  }
  
  /*
   * Quickly convert it to eight bit format, replacing non-ASCII UNICODE 
   * characters with their equivelent HTML entity.
   */
  if (args->icbmflags & AIM_IMFLAGS_UNICODE) {
    int i;
    
    for (i=0; i<args->msglen; i+=2) {
      fu16_t uni;

      uni = ((args->msg[i] & 0xff) << 8) | (args->msg[i+1] & 0xff);
      if ((uni < 128) || ((uni >= 160) && (uni <= 255))) { /* ISO 8859-1 */
	snprintf(realmsg+strlen(realmsg), sizeof(realmsg)-strlen(realmsg), "%c", uni);
      } else { /* something else, do UNICODE entity */
	snprintf(realmsg+strlen(realmsg), sizeof(realmsg)-strlen(realmsg), "&#%04x;", uni);
      }
    }
  } else {
    /*
     * For non-UNICODE encodings (ASCII and ISO 8859-1), there is 
     * no need to do anything special here.  Most 
     * terminals/whatever will be able to display such characters 
     * unmodified.
     *
     * Beware that PC-ASCII 128 through 159 are _not_ actually 
     * defined in ASCII or ISO 8859-1, and you should send them as 
     * UNICODE.  WinAIM will send these characters in a UNICODE 
     * message, so you need to do so as well.
     *
     * You may not think it necessary to handle UNICODE messages.  
     * You're probably wrong.  For one thing, Microsoft "Smart
     * Quotes" will be sent by WinAIM as UNICODE (not HTML UNICODE,
     * but real UNICODE). If you don't parse UNICODE at all, your 
     * users will get a blank message instead of the message 
     * containing Smart Quotes.
     *
     */
    strncpy(realmsg, args->msg, sizeof(realmsg));
  }

  /* create a message, and put it on the message queue */
  stripmsg=owl_text_htmlstrip(realmsg);
  wrapmsg=owl_text_wordwrap(stripmsg, 70);
  nz_screenname=owl_aim_normalize_screenname(userinfo->sn);
  m=owl_malloc(sizeof(owl_message));
  owl_message_create_aim(m,
			 nz_screenname,
			 owl_global_get_aim_screenname(&g),
			 wrapmsg,
			 OWL_MESSAGE_DIRECTION_IN,
			 0);
  owl_global_messagequeue_addmsg(&g, m);
  owl_free(stripmsg);
  owl_free(wrapmsg);
  owl_free(nz_screenname);

  return(1);
  
  /* printf("icbm: message: %s\n", realmsg); */
  
  if (args->icbmflags & AIM_IMFLAGS_MULTIPART) {
    aim_mpmsg_section_t *sec;
    int z;

    /* printf("icbm: multipart: this message has %d parts\n", args->mpmsg.numparts); */
    
    for (sec = args->mpmsg.parts, z = 0; sec; sec = sec->next, z++) {
      if ((sec->charset == 0x0000) || (sec->charset == 0x0003) || (sec->charset == 0xffff)) {
	/* printf("icbm: multipart:   part %d: charset 0x%04x, subset 0x%04x, msg = %s\n", z, sec->charset, sec->charsubset, sec->data); */
      } else {
	/* printf("icbm: multipart:   part %d: charset 0x%04x, subset 0x%04x, binary or UNICODE data\n", z, sec->charset, sec->charsubset); */
      }
    }
  }
  
  if (args->icbmflags & AIM_IMFLAGS_HASICON) {
    /* aim_send_im(sess, userinfo->sn, AIM_IMFLAGS_BUDDYREQ, "You have an icon"); */
    /* printf("icbm: their icon: iconstamp = %ld, iconlen = 0x%08lx, iconsum = 0x%04x\n", args->iconstamp, args->iconlen, args->iconsum); */
  }

  /*
  if (realmsg) {
    int i = 0;
    while (realmsg[i] == '<') {
      if (realmsg[i] == '<') {
	while (realmsg[i] != '>')
	  i++;
	i++;
      }
    }
    tmpstr = realmsg+i;
    faimtest_handlecmd(sess, conn, userinfo, tmpstr);
  }
  */
  
  if (priv->buddyicon && (args->icbmflags & AIM_IMFLAGS_BUDDYREQ)) {
    aim_send_icon(sess, userinfo->sn, priv->buddyicon, priv->buddyiconlen, priv->buddyiconstamp, priv->buddyiconsum);
  }
  
  return 1;
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
  
  if (args->reqclass == AIM_CAPS_GETFILE) {
    getfile_requested(sess, conn, userinfo, args);
  } else if (args->reqclass == AIM_CAPS_SENDFILE) {
    /* printf("send file!\n"); */
  } else if (args->reqclass == AIM_CAPS_CHAT) {
    /*
    printf("chat invitation: room name = %s\n", args->info.chat.roominfo.name);
    printf("chat invitation: exchange = 0x%04x\n", args->info.chat.roominfo.exchange);
    printf("chat invitation: instance = 0x%04x\n", args->info.chat.roominfo.instance);
    */
    /* Automatically join room... */
    /* printf("chat invitiation: autojoining %s...\n", args->info.chat.roominfo.name); */

    /* aim_chat_join(sess, conn, args->info.chat.roominfo.exchange, args->info.chat.roominfo.name, args->info.chat.roominfo.instance); */
  } else if (args->reqclass == AIM_CAPS_IMIMAGE) {
    directim_requested(sess, conn, userinfo, args);
  } else if (args->reqclass == AIM_CAPS_BUDDYICON) {
    /* printf("Buddy Icon from %s, length = %lu\n", userinfo->sn, args->info.icon.length); */
  } else if (args->reqclass == AIM_CAPS_ICQRTF) {
    /* printf("RTF message from %s: (fgcolor = 0x%08lx, bgcolor = 0x%08lx) %s\n", userinfo->sn, args->info.rtfmsg.fgcolor, args->info.rtfmsg.bgcolor, args->info.rtfmsg.rtfmsg); */
  } else {
    /* printf("icbm: unknown reqclass (%d)\n", args->reqclass); */
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
    /* printf("unsupported channel 0x%04x\n", channel); */
  }
  va_end(ap);
  /* printf("icbm: done with ICBM handling (ret = %d)\n", ret); */
  return 1;
}

static int faimtest_parse_oncoming(aim_session_t *sess, aim_frame_t *fr, ...)
{
  aim_userinfo_t *userinfo;
  char *nz_screenname;

  va_list ap;
  va_start(ap, fr);
  userinfo = va_arg(ap, aim_userinfo_t *);
  va_end(ap);

  nz_screenname=owl_aim_normalize_screenname(userinfo->sn);
  owl_buddylist_oncoming(owl_global_get_buddylist(&g), nz_screenname);
  owl_free(nz_screenname);
  
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
  owl_free(nz_screenname);

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
  
  return 1;
}

static int faimtest_parse_msgerr(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  char *destsn;
  fu16_t reason;
  
  va_start(ap, fr);
  reason = (fu16_t)va_arg(ap, unsigned int);
  destsn = va_arg(ap, char *);
  va_end(ap);
  
  /* printf("message to %s bounced (reason 0x%04x: %s)\n", destsn, reason, (reason<msgerrreasonslen)?msgerrreasons[reason]:"unknown"); */
  
  return 1;
}

static int faimtest_parse_locerr(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  char *destsn;
  fu16_t reason;
  
  va_start(ap, fr);
  reason = (fu16_t)va_arg(ap, unsigned int);
  destsn = va_arg(ap, char *);
  va_end(ap);
  
  /* printf("user information for %s unavailable (reason 0x%04x: %s)\n", destsn, reason, (reason<msgerrreasonslen)?msgerrreasons[reason]:"unknown"); */
  
  return 1;
}

static int faimtest_parse_misses(aim_session_t *sess, aim_frame_t *fr, ...)
{
  /*
  static char *missedreasons[] = {
    "Invalid (0)",
    "Message too large",
    "Rate exceeded",
    "Evil Sender",
    "Evil Receiver"
  };
  static int missedreasonslen = 5; */
  
  va_list ap;
  fu16_t chan, nummissed, reason;
  aim_userinfo_t *userinfo;
  
  va_start(ap, fr);
  chan = (fu16_t)va_arg(ap, unsigned int);
  userinfo = va_arg(ap, aim_userinfo_t *);
  nummissed = (fu16_t)va_arg(ap, unsigned int);
  reason = (fu16_t)va_arg(ap, unsigned int);
  va_end(ap);
  
  /* printf("missed %d messages from %s on channel %d (reason %d: %s)\n", nummissed, userinfo->sn, chan, reason, (reason<missedreasonslen)?missedreasons[reason]:"unknown"); */
  
  return 1;
}

/*
 * Received in response to an IM sent with the AIM_IMFLAGS_ACK option.
 */
static int faimtest_parse_msgack(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  fu16_t type;
  char *sn = NULL;
  
  va_start(ap, fr);
  type = (fu16_t)va_arg(ap, unsigned int);
  sn = va_arg(ap, char *);
  va_end(ap);
  
  /* printf("msgack: 0x%04x / %s\n", type, sn); */
  
  return 1;
}

static int faimtest_parse_ratechange(aim_session_t *sess, aim_frame_t *fr, ...)
{
  /*
  static char *codes[5] = {
    "invalid",
    "change",
    "warning",
    "limit",
    "limit cleared"
  };
  */
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
  
  /*
  printf("rate %s (rate class 0x%04x): curavg = %ld, maxavg = %ld, alert at %ld, clear warning at %ld, limit at %ld, disconnect at %ld (window size = %ld)\n",
	 (code < 5)?codes[code]:"invalid",
	 rateclass,
	 currentavg, maxavg,
	 alert, clear,
	 limit, disconnect,
	 windowsize);
  */
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
  /* printf("evil level change: new value = %2.1f%% (caused by %s)\n", ((float)newevil)/10, (userinfo && strlen(userinfo->sn))?userinfo->sn:"anonymous"); */
  
  return 1;
}

static int faimtest_parse_searchreply(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  char *address, *SNs;
  int num;
  /* int i; */
  
  va_start(ap, fr);
  address = va_arg(ap, char *);
  num = va_arg(ap, int);
  SNs = va_arg(ap, char *);
  va_end(ap);
  
  /* printf("E-Mail Search Results for %s: ", address);
  
  for(i = 0; i < num; i++)
    printf("%s, ", &SNs[i*(MAXSNLEN+1)]);
  printf("\n");
  */
  
  return 1;
}

static int faimtest_parse_searcherror(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  char *address;
  
  va_start(ap, fr);
  address = va_arg(ap, char *);
  va_end(ap);
  
  /* printf("E-Mail Search Results for %s: No Results or Invalid Email\n", address); */
  
  return 1;
}

static int handlepopup(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  char *msg, *url;
  fu16_t width, height, delay;
  
  va_start(ap, fr);
  msg = va_arg(ap, char *);
  url = va_arg(ap, char *);
  width = va_arg(ap, unsigned int);
  height = va_arg(ap, unsigned int);
  delay = va_arg(ap, unsigned int);
  va_end(ap);
  
  /* printf("popup: (%dx%x:%d) %s (%s)\n", width, height, delay, msg, url); */
  
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
  char *bosip;
  fu8_t *cookie;
  
  va_start(ap, fr);
  bosip = va_arg(ap, char *);
  cookie = va_arg(ap, fu8_t *);
  va_end(ap);
  
  /* printf("migration in progress -- new BOS is %s -- disconnecting\n", bosip); */
  aim_conn_kill(sess, &fr->conn);
  
  if (!(bosconn = aim_newconn(sess, AIM_CONN_TYPE_BOS, bosip))) {
    /* printf("migrate: could not connect to BOS: internal error\n"); */
    return 1;
  } else if (bosconn->status & AIM_CONN_STATUS_CONNERR) {	
    /* printf("migrate: could not connect to BOS\n"); */
    aim_conn_kill(sess, &bosconn);
    return 1;
  }
  
  /* Login will happen all over again. */
  addcb_bos(sess, bosconn);
  aim_sendcookie(sess, bosconn, cookie);
  return 1;
}

static int ssirights(aim_session_t *sess, aim_frame_t *fr, ...)
{
  
  /* printf("got SSI rights, requesting data\n"); */
  aim_ssi_reqdata(sess, fr->conn, 0, 0x0000);
  
  return 1;
}

static int ssidata(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  fu8_t fmtver;
  fu16_t itemcount;
  fu32_t stamp;
  struct aim_ssi_item *list, *l;
  
  va_start(ap, fr);
  fmtver = va_arg(ap, unsigned int);
  itemcount = va_arg(ap, unsigned int);
  stamp = va_arg(ap, fu32_t);
  list = va_arg(ap, struct aim_ssi_item *);
  va_end(ap);
  
  /* printf("got SSI data: (0x%02x, %d items, %ld)\n", fmtver, itemcount, stamp); */
  for (l = list; l; l = l->next) {
    /* printf("\t0x%04x (%s) - 0x%04x/0x%04x\n", l->type, l->name, l->gid, l->bid); */
    /* printf("I would have printed data here!\n"); */
  }
  
  aim_ssi_enable(sess, fr->conn);
  
  return 1;
}

static int ssidatanochange(aim_session_t *sess, aim_frame_t *fr, ...)
{
  /* printf("server says we have the latest SSI data already\n"); */
  aim_ssi_enable(sess, fr->conn);
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
    /* printf("offline message from %ld at %d/%d/%d %02d:%02d : %s\n", msg->sender, msg->year, msg->month, msg->day, msg->hour, msg->minute, msg->msg); */
  } else {
    /* printf("unknown offline message type 0x%04x\n", msg->type); */
  }
  return 1;
}

static int offlinemsgdone(aim_session_t *sess, aim_frame_t *fr, ...)
{
  /* Tell the server to delete them. */
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
  char *roomname;
  int usercount;
  char *roomdesc;
  fu16_t flags, unknown_d2, unknown_d5, maxmsglen, maxvisiblemsglen;
  fu32_t creationtime;
  const char *croomname;
  /* int i; */
  
  croomname = aim_chat_getname(fr->conn);
  
  va_start(ap, fr);
  roominfo = va_arg(ap, struct aim_chat_roominfo *);
  roomname = va_arg(ap, char *);
  usercount = va_arg(ap, int);
  userinfo = va_arg(ap, aim_userinfo_t *);
  roomdesc = va_arg(ap, char *);
  flags = (fu16_t)va_arg(ap, unsigned int);
  creationtime = va_arg(ap, fu32_t);
  maxmsglen = (fu16_t)va_arg(ap, unsigned int);
  unknown_d2 = (fu16_t)va_arg(ap, unsigned int);
  unknown_d5 = (fu16_t)va_arg(ap, unsigned int);
  maxvisiblemsglen = (fu16_t)va_arg(ap, unsigned int);
  va_end(ap);

  /*
  printf("chat: %s:  info update:\n", croomname);
  printf("chat: %s:  \tRoominfo: {%04x, %s, %04x}\n", croomname, roominfo->exchange, roominfo->name, roominfo->instance);
  printf("chat: %s:  \tRoomname: %s\n", croomname, roomname);
  printf("chat: %s:  \tRoomdesc: %s\n", croomname, roomdesc);
  printf("chat: %s:  \tOccupants: (%d)\n", croomname, usercount);

  for (i = 0; i < usercount; i++)
    printf("chat: %s:  \t\t%s\n", croomname, userinfo[i].sn);
  
  printf("chat: %s:  \tRoom flags: 0x%04x (%s%s%s%s)\n", 
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
  
  return 1;
}

static int faimtest_chat_incomingmsg(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  aim_userinfo_t *userinfo;
  char *msg;
  char tmpbuf[1152];
  
  va_start(ap, fr);
  userinfo = va_arg(ap, aim_userinfo_t *);	
  msg = va_arg(ap, char *);
  va_end(ap);

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

static int faimtest_chatnav_info(aim_session_t *sess, aim_frame_t *fr, ...)
{
  fu16_t type;
  va_list ap;
  
  va_start(ap, fr);
  type = (fu16_t)va_arg(ap, unsigned int);
  
  if (type == 0x0002) {
    int maxrooms;
    struct aim_chat_exchangeinfo *exchanges;
    int exchangecount;
    /* int i; */
    
    maxrooms = va_arg(ap, int);
    exchangecount = va_arg(ap, int);
    exchanges = va_arg(ap, struct aim_chat_exchangeinfo *);
    va_end(ap);

    /*
    printf("chat info: Chat Rights:\n");
    printf("chat info: \tMax Concurrent Rooms: %d\n", maxrooms);
    printf("chat info: \tExchange List: (%d total)\n", exchangecount);
    for (i = 0; i < exchangecount; i++) {
      printf("chat info: \t\t%x: %s (%s/%s) (0x%04x = %s%s%s%s)\n", 
	     exchanges[i].number,
	     exchanges[i].name,
	     exchanges[i].charset1,
	     exchanges[i].lang1,
	     exchanges[i].flags,
	     (exchanges[i].flags & AIM_CHATROOM_FLAG_EVILABLE) ? "Evilable, " : "",
	     (exchanges[i].flags & AIM_CHATROOM_FLAG_NAV_ONLY) ? "Nav Only, " : "",
	     (exchanges[i].flags & AIM_CHATROOM_FLAG_INSTANCING_ALLOWED) ? "Instancing allowed, " : "",
	     (exchanges[i].flags & AIM_CHATROOM_FLAG_OCCUPANT_PEEK_ALLOWED) ? "Occupant peek allowed, " : "");
    }
    */
  } else if (type == 0x0008) {
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
    va_end(ap);
    
    /* printf("received room create reply for %s/0x%04x\n", fqcn, exchange); */
    
  } else {
    va_end(ap);
    /* printf("chatnav info: unknown type (%04x)\n", type); */
  }
  
  return 1;
}

static int conninitdone_chat(aim_session_t *sess, aim_frame_t *fr, ...)
{

  aim_clientready(sess, fr->conn);
  
  if (fr->conn->type == AIM_CONN_TYPE_CHATNAV) {
    /* printf("chatnav ready\n"); */
    aim_conn_addhandler(sess, fr->conn, 0x000d, 0x0001, faimtest_parse_genericerr, 0);
    aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CTN, AIM_CB_CTN_INFO, faimtest_chatnav_info, 0);
    aim_chatnav_reqrights(sess, fr->conn);
  } else if (fr->conn->type == AIM_CONN_TYPE_CHAT) {
    /* printf("chat ready\n"); */
    aim_conn_addhandler(sess, fr->conn, 0x000e, 0x0001, faimtest_parse_genericerr, 0);
    aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_USERJOIN, faimtest_chat_join, 0);
    aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_USERLEAVE, faimtest_chat_leave, 0);
    aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_ROOMINFOUPDATE, faimtest_chat_infoupdate, 0);
    aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_INCOMINGMSG, faimtest_chat_incomingmsg, 0);
  }
  return 1;
}

void chatnav_redirect(aim_session_t *sess, struct aim_redirect_data *redir)
{
  aim_conn_t *tstconn;
  
  tstconn = aim_newconn(sess, AIM_CONN_TYPE_CHATNAV, redir->ip);
  if (!tstconn || (tstconn->status & AIM_CONN_STATUS_RESOLVERR)) {
    /* printf("unable to connect to chat(nav) server\n"); */
    if (tstconn)
      aim_conn_kill(sess, &tstconn);
    return;
  }
  
  aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNCOMPLETE, faimtest_conncomplete, 0);
  aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, conninitdone_chat, 0);
  aim_sendcookie(sess, tstconn, redir->cookie);
  /* printf("chatnav: connected\n"); */
  return;
}

/* XXX this needs instance too */
void chat_redirect(aim_session_t *sess, struct aim_redirect_data *redir)
{
  aim_conn_t *tstconn;
  
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
  aim_sendcookie(sess, tstconn, redir->cookie);
  return;	
}




/******************************************* ft.c ******************************************/


static int directim_incoming(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  char *sn, *msg;
  
  va_start(ap, fr);
  sn = va_arg(ap, char *);
  msg = va_arg(ap, char *);
  va_end(ap);
  
  /* printf("Directim from %s: %s\n", sn, msg); */
  if (strstr(msg, "sendmsg")) {
    int i;
    
    i = atoi(strstr(msg, "sendmsg")+8);
    if (i < 10000) {
      char *newbuf;
      int z;
      
      newbuf = malloc(i+1);
      for (z = 0; z < i; z++)
	newbuf[z] = (z % 10)+0x30;
      newbuf[i] = '\0';
      aim_send_im_direct(sess, fr->conn, newbuf, strlen(newbuf), 0);
      free(newbuf);
    }
  } else if (strstr(msg, "goodday")) {
    aim_send_im_direct(sess, fr->conn, "Good day to you, too", 18, 0);
  } else {
    char newmsg[1024];
    snprintf(newmsg, sizeof(newmsg), "unknown (%s)\n", msg);
    aim_send_im_direct(sess, fr->conn, newmsg, strlen(newmsg), 0);
  }

  return 1;
}

static int directim_typing(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  char *sn;
  
  va_start(ap, fr);
  sn = va_arg(ap, char *);
  va_end(ap);
  /* printf("ohmigod! %s has started typing (DirectIM). He's going to send you a message! *squeal*\n", sn); */
  return 1;
}

static int faimtest_directim_initiate(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  aim_conn_t *newconn, *listenerconn;
  
  va_start(ap, fr);
  newconn = va_arg(ap, aim_conn_t *);
  listenerconn = va_arg(ap, aim_conn_t *);
  va_end(ap);
  
  aim_conn_close(listenerconn);
  aim_conn_kill(sess, &listenerconn);
  
  aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMINCOMING, directim_incoming, 0);
  aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMTYPING, directim_typing, 0);
  
  aim_send_im_direct(sess, newconn, "goodday", 8, 0);
  
  /* printf("OFT: DirectIM: connected to %s\n", aim_directim_getsn(newconn)); */
  
  return 1;
}

static int faimtest_getfile_filereq(aim_session_t *ses, aim_frame_t *fr, ...)
{
  va_list ap;
  aim_conn_t *oftconn;
  struct aim_fileheader_t *fh;
  fu8_t *cookie;
  
  va_start(ap, fr);
  oftconn = va_arg(ap, aim_conn_t *);
  fh = va_arg(ap, struct aim_fileheader_t *);
  cookie = va_arg(ap, fu8_t *);
  va_end(ap);
  
  /* printf("request for file %s.\n", fh->name); */
  
  return 1;
}

static int faimtest_getfile_filesend(aim_session_t *sess, aim_frame_t *fr, ...)
{
  struct owlfaim_priv *priv = (struct owlfaim_priv *)sess->aux_data;
  va_list ap;
  aim_conn_t *oftconn;
  struct aim_fileheader_t *fh;
  char *path;
  fu8_t *cookie;
  int pos, bufpos = 0, bufsize = 2048, i;
  char *buf;
  FILE *file;
  
  va_start(ap, fr);
  oftconn = va_arg(ap, aim_conn_t *);
  fh = va_arg(ap, struct aim_fileheader_t *);
  cookie = va_arg(ap, fu8_t *);
  va_end(ap);
  
  /* printf("sending file %s(%ld).\n", fh->name, fh->size); */

  if (!(buf = malloc(2048)))
    return -1;
  
  if (!(path = (char *)calloc(1, strlen(priv->listingpath) +strlen(fh->name)+2))) {
    /* perror("calloc"); */
    /* printf("error in calloc of path\n"); */
    
    return 0; /* XXX: no idea what winaim expects here =) */
  }

  snprintf(path, strlen(priv->listingpath)+strlen(fh->name)+2, "%s/%s", priv->listingpath, fh->name);
  
  if (!(file = fopen(path, "r"))) {
    /* printf("getfile_send fopen failed for %s. damn.\n", path); */
    return 0;
  }
  
  /* 
   * This is a mess. Remember that faimtest is demonstration code
   * only and for the sake of the gods, don't use this code in any
   * of your clients. --mid
   */
  for(pos = 0; pos < fh->size; pos++) {
    
    bufpos = pos % bufsize;
    
    if (bufpos == 0 && pos > 0) { /* filled our buffer. spit it across the wire */
      if ( (i = send(oftconn->fd, buf, bufsize, 0)) != bufsize ) {
	/* perror("faim: getfile_send: write1"); */
	/* printf("faim: getfile_send: whoopsy, didn't write it all...\n"); */
	free(buf);   
	return 0;
      }
    }
    
    if( (buf[bufpos] = fgetc(file)) == EOF) {
      if(pos != fh->size) {
	/* printf("faim: getfile_send: hrm... apparent early EOF at pos 0x%x of 0x%lx\n", pos, fh->size); */
	free(buf);   
	return 0;
      }
    }      
    /* printf("%c(0x%02x) ", buf[pos], buf[pos]); */
  }
  
  if( (i = send(oftconn->fd, buf, bufpos+1, 0)) != (bufpos+1)) {
    /* perror("faim: getfile_send: write2"); */
    /* printf("faim: getfile_send cleanup: whoopsy, didn't write it all...\n"); */
    free(buf);   
    return 0;
  }
  
  free(buf);
  free(fh);
  
  return 1;
}

static int faimtest_getfile_complete(aim_session_t *sess, aim_frame_t *fr, ...) 
{
  va_list ap;
  aim_conn_t *conn;
  struct aim_fileheader_t *fh;
  
  va_start(ap, fr);
  conn = va_arg(ap, aim_conn_t *);
  fh = va_arg(ap, struct aim_fileheader_t *);
  va_end(ap);
  
  /* printf("completed file transfer for %s.\n", fh->name); */
  
  aim_conn_close(conn);
  aim_conn_kill(sess, &conn);
  
  return 1;
}

static int faimtest_getfile_disconnect(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  aim_conn_t *conn;
  char *sn;
  
  va_start(ap, fr);
  conn = va_arg(ap, aim_conn_t *);
  sn = va_arg(ap, char *);
  va_end(ap);
  
  aim_conn_kill(sess, &conn);
  
  /* printf("getfile: disconnected from %s\n", sn); */
  
  return 1;
}

static int faimtest_getfile_listing(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  aim_conn_t *conn;
  char *listing;
  struct aim_filetransfer_priv *ft;
  char *filename, *nameend, *sizec;
  int filesize, namelen;
  
  va_start(ap, fr);
  conn = va_arg(ap, aim_conn_t *);
  ft = va_arg(ap, struct aim_filetransfer_priv *);
  listing = va_arg(ap, char *);
  va_end(ap);
  
  /* printf("listing on %d==================\n%s\n===========\n", conn->fd, listing); */
  
  nameend = strstr(listing+0x1a, "\r");
  namelen = nameend - (listing + 0x1a);
  
  filename = malloc(namelen + 1);
  strncpy(filename, listing+0x1a, namelen);
  filename[namelen] = 0x00;
  
  sizec = malloc(8+1);
  memcpy(sizec, listing + 0x11, 8);
  sizec[8] = 0x00;
  
  filesize =  strtol(sizec, (char **)NULL, 10);
  
  /* printf("requesting %d %s(%d long)\n", namelen, filename, filesize); */
  
  /* aim_oft_getfile_request(sess, conn, filename, filesize); */
  
  free(filename);
  free(sizec);
  
  return 0;
}

static int faimtest_getfile_listingreq(aim_session_t *sess, aim_frame_t *fr, ...)
{
  struct owlfaim_priv *priv = (struct owlfaim_priv *)sess->aux_data;
  va_list ap;
  aim_conn_t *oftconn;
  struct aim_fileheader_t *fh;
  int pos, bufpos = 0, bufsize = 2048, i;
  char *buf;
  
  va_start(ap, fr);
  oftconn = va_arg(ap, aim_conn_t *);
  fh = va_arg(ap, struct aim_fileheader_t *);
  va_end(ap);
  
  /* printf("sending listing of size %ld\n", fh->size); */

  if(!(buf = malloc(2048))) return -1;
  
  for(pos = 0; pos < fh->size; pos++) {
    bufpos = pos % bufsize;
    if(bufpos == 0 && pos > 0) { /* filled our buffer. spit it across the wire */
      if ( (i = send(oftconn->fd, buf, bufsize, 0)) != bufsize ) {
	/* perror("faim: getfile_send: write1"); */
	/* printf("faim: getfile_send: whoopsy, didn't write it all...\n"); */
	free(buf);   
	return 0;
      }
    }
    if( (buf[bufpos] = fgetc(priv->listingfile)) == EOF) {
      if(pos != fh->size) {
	/* printf("faim: getfile_send: hrm... apparent early EOF at pos 0x%x of 0x%lx\n", pos, fh->size); */
	free(buf);   
	return 0;
      }
    }      
  }
  
  if( (i = send(oftconn->fd, buf, bufpos+1, 0)) != (bufpos+1)) {
    /* perror("faim: getfile_send: write2"); */
    /* printf("faim: getfile_send cleanup: whoopsy, didn't write it all...\n"); */
    free(buf);   
    return 0;
  }
  
  /* printf("sent listing\n"); */
  free(buf);
  
  return 0;
}

static int faimtest_getfile_receive(aim_session_t *sess, aim_frame_t *fr, ...)
{
  /* needs ot be fixed --kretch */
#if 0
  va_list ap;
  aim_conn_t *conn;
  struct aim_filetransfer_priv *ft;
  unsigned char data;
  int pos;


  va_start(ap, fr);
  conn = va_arg(ap, aim_conn_t *);
  ft = va_arg(ap, struct aim_filetransfer_priv *);
  va_end(ap);

  /* printf("receiving %ld bytes of file data for %s:\n\t", ft->fh.size, ft->fh.name); */

  for(pos = 0; pos < ft->fh.size; pos++) {
    read(conn->fd, &data, 1);
    /* printf("%c(%02x) ", data, data); */
  }

  /* printf("\n"); */

  aim_oft_getfile_end(sess, conn);
#endif
  return 0;
}

static int faimtest_getfile_state4(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  aim_conn_t *conn;
  
  va_start(ap, fr);
  conn = va_arg(ap, aim_conn_t *);
  va_end(ap);
  
  aim_conn_close(conn);
  aim_conn_kill(sess, &conn);
  
  return 0;
}

static int faimtest_getfile_initiate(aim_session_t *sess, aim_frame_t *fr, ...)
{
  va_list ap;
  aim_conn_t *conn, *listenerconn;
  struct aim_filetransfer_priv *priv;
  
  va_start(ap, fr);
  conn = va_arg(ap, aim_conn_t *);
  listenerconn = va_arg(ap, aim_conn_t *);
  va_end(ap);
  
  aim_conn_close(listenerconn);
  aim_conn_kill(sess, &listenerconn);

  /*   This doesn't work with the new libfaim */
  aim_conn_addhandler(sess, conn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILEFILEREQ,  faimtest_getfile_filereq, 0);
  aim_conn_addhandler(sess, conn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILEFILESEND, faimtest_getfile_filesend, 0);
  aim_conn_addhandler(sess, conn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILECOMPLETE, faimtest_getfile_complete, 0);      
  aim_conn_addhandler(sess, conn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILEDISCONNECT, faimtest_getfile_disconnect, 0);      
  aim_conn_addhandler(sess, conn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILELISTING, faimtest_getfile_listing, 0);
  aim_conn_addhandler(sess, conn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILELISTINGREQ, faimtest_getfile_listingreq, 0);
  aim_conn_addhandler(sess, conn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILERECEIVE, faimtest_getfile_receive, 0);
  aim_conn_addhandler(sess, conn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILESTATE4, faimtest_getfile_state4, 0);
  
  priv = (struct aim_filetransfer_priv *)conn->priv;
  
  /* printf("getfile: %s (%s) connected to us on %d\n", priv->sn, priv->ip, conn->fd); */
  
  return 1;
}

void getfile_start(aim_session_t *sess, aim_conn_t *conn, const char *sn)
{
  aim_conn_t *newconn;
  
  newconn = aim_getfile_initiate(sess, conn, sn);
  /* printf("getting file listing from %s\n", sn); */
  aim_conn_addhandler(sess, newconn,  AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILEINITIATE, faimtest_getfile_initiate,0);
  
  return;
}

void getfile_requested(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *userinfo, struct aim_incomingim_ch2_args *args)
{
  struct owlfaim_priv *priv = (struct owlfaim_priv *)sess->aux_data;
  aim_conn_t *newconn;
  struct aim_fileheader_t *fh;
  
  printf("get file request from %s (at %s/%s/%s) %x\n", userinfo->sn, args->clientip, args->clientip2, args->verifiedip, args->reqclass);
  fh = aim_getlisting(sess, priv->listingfile);
  newconn = aim_accepttransfer(sess, conn, userinfo->sn, args->cookie, args->clientip, fh->totfiles, fh->totsize, fh->size, fh->checksum, args->reqclass);
  free(fh);

  if ( (!newconn) || (newconn->fd == -1) ) {
    printf("getfile: requestconn: apparent error in accepttransfer\n");
    if (newconn) aim_conn_kill(sess, &newconn);
    return;
  }
  
  aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILELISTINGREQ, faimtest_getfile_listingreq, 0);
  aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILEFILEREQ,  faimtest_getfile_filereq, 0);
  aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILEFILESEND, faimtest_getfile_filesend, 0);
  aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILECOMPLETE, faimtest_getfile_complete, 0);      
  aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILEDISCONNECT, faimtest_getfile_disconnect, 0);      
  
  /* printf("getfile connect succeeded, handlers added.\n"); */
  
  return;
}

void directim_start(aim_session_t *sess, const char *sn)
{
  aim_conn_t *newconn;
  
  /* printf("opening directim to %s\n", sn); */
  newconn = aim_directim_initiate(sess, sn);
  if (!newconn || (newconn->fd == -1)) {
    /* printf("connection failed!\n"); */
    if (newconn) aim_conn_kill(sess, &newconn);
  } else {
    aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMINITIATE, faimtest_directim_initiate, 0);
  }
  return;
}

void directim_requested(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *userinfo, struct aim_incomingim_ch2_args *args)
{
  aim_conn_t *newconn;
  
  /* printf("OFT: DirectIM: request from %s (%s/%s/%s)\n", userinfo->sn, args->clientip, args->clientip2, args->verifiedip); */
  
  newconn = aim_directim_connect(sess, userinfo->sn, args->clientip, args->cookie);
  if (!newconn || (newconn->fd == -1)) {
    /* printf("icbm: imimage: could not connect\n"); */
    if (newconn) aim_conn_kill(sess, &newconn);
  } else {
    aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMINCOMING, directim_incoming, 0);
    aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMTYPING, directim_typing, 0);
    /* printf("OFT: DirectIM: connected to %s\n", userinfo->sn); */
    aim_send_im_direct(sess, newconn, "goodday", 7, 0);
  }
}


/****************************************/

/*
static int gaim_parse_oncoming(aim_session_t *sess, aim_frame_t *fr, ...) {
        struct gaim_connection *gc = sess->aux_data;
        struct oscar_data *od = gc->proto_data;
        aim_userinfo_t *info;
        time_t time_idle = 0, signon = 0;
        int type = 0;
        int caps = 0;
        char *tmp;

        va_list ap;
        va_start(ap, fr);
        info = va_arg(ap, aim_userinfo_t *);
        va_end(ap);

        if (info->present & AIM_USERINFO_PRESENT_CAPABILITIES)
                caps = info->capabilities;
        if (info->flags & AIM_FLAG_ACTIVEBUDDY)
                type |= UC_AB;

        if ((!od->icq) && (info->present & AIM_USERINFO_PRESENT_FLAGS)) {
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

        if (info->present & AIM_USERINFO_PRESENT_SESSIONLEN)
                signon = time(NULL) - info->sessionlen;

        tmp = g_strdup(normalize(gc->username));
        if (!strcmp(tmp, normalize(info->sn)))
                g_snprintf(gc->displayname, sizeof(gc->displayname), "%s", info->sn);
        g_free(tmp);

        serv_got_update(gc, info->sn, 1, info->warnlevel/10, signon,
                        time_idle, type, caps);

        return 1;
}
*/
