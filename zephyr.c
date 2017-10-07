#include "owl.h"
#include <stdio.h>
#include <sys/stat.h>

#ifdef HAVE_LIBZEPHYR
static GSource *owl_zephyr_event_source_new(int fd);

static gboolean owl_zephyr_event_prepare(GSource *source, int *timeout);
static gboolean owl_zephyr_event_check(GSource *source);
static gboolean owl_zephyr_event_dispatch(GSource *source, GSourceFunc callback, gpointer user_data);

static GList *deferred_subs = NULL;

typedef struct _owl_sub_list {                            /* noproto */
  ZSubscription_t *subs;
  int nsubs;
} owl_sub_list;

Code_t ZResetAuthentication(void);

static GSourceFuncs zephyr_event_funcs = {
  owl_zephyr_event_prepare,
  owl_zephyr_event_check,
  owl_zephyr_event_dispatch,
  NULL
};
#endif

#define HM_SVC_FALLBACK		htons((unsigned short) 2104)

static CALLER_OWN char *owl_zephyr_dotfile(const char *name, const char *input)
{
  if (input != NULL)
    return g_strdup(input);
  else
    return g_build_filename(owl_global_get_homedir(&g), name, NULL);
}

#ifdef HAVE_LIBZEPHYR
void owl_zephyr_initialize(void)
{
  Code_t ret;
  struct servent *sp;
  struct sockaddr_in sin;
  ZNotice_t req;
  GIOChannel *channel;

  /*
   * Code modified from libzephyr's ZhmStat.c
   *
   * Modified to add the fd to our select loop, rather than hanging
   * until we get an ack.
   */

  if ((ret = ZOpenPort(NULL)) != ZERR_NONE) {
    owl_function_error("Error opening Zephyr port: %s", error_message(ret));
    return;
  }

  (void) memset(&sin, 0, sizeof(struct sockaddr_in));

  sp = getservbyname(HM_SVCNAME, "udp");

  sin.sin_port = (sp) ? sp->s_port : HM_SVC_FALLBACK;
  sin.sin_family = AF_INET;

  sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  (void) memset(&req, 0, sizeof(req));
  req.z_kind = STAT;
  req.z_port = 0;
  req.z_class = zstr(HM_STAT_CLASS);
  req.z_class_inst = zstr(HM_STAT_CLIENT);
  req.z_opcode = zstr(HM_GIMMESTATS);
  req.z_sender = zstr("");
  req.z_recipient = zstr("");
  req.z_default_format = zstr("");
  req.z_message_len = 0;

  if ((ret = ZSetDestAddr(&sin)) != ZERR_NONE) {
    owl_function_error("Initializing Zephyr: %s", error_message(ret));
    return;
  }

  if ((ret = ZSendNotice(&req, ZNOAUTH)) != ZERR_NONE) {
    owl_function_error("Initializing Zephyr: %s", error_message(ret));
    return;
  }

  channel = g_io_channel_unix_new(ZGetFD());
  g_io_add_watch(channel, G_IO_IN | G_IO_ERR | G_IO_HUP,
		 &owl_zephyr_finish_initialization, NULL);
  g_io_channel_unref(channel);
}

gboolean owl_zephyr_finish_initialization(GIOChannel *source, GIOCondition condition, void *data) {
  Code_t code;
  char *perl;
  GSource *event_source;

  ZClosePort();

  if ((code = ZInitialize()) != ZERR_NONE) {
    owl_function_error("Initializing Zephyr: %s", error_message(code));
    return FALSE;
  }

  if ((code = ZOpenPort(NULL)) != ZERR_NONE) {
    owl_function_error("Initializing Zephyr: %s", error_message(code));
    return FALSE;
  }

  event_source = owl_zephyr_event_source_new(ZGetFD());
  g_source_attach(event_source, NULL);
  g_source_unref(event_source);

  owl_global_set_havezephyr(&g);

  if(g.load_initial_subs) {
    owl_zephyr_load_initial_subs();
  }
  while(deferred_subs != NULL) {
    owl_sub_list *subs = deferred_subs->data;
    owl_function_debugmsg("Loading %d deferred subs.", subs->nsubs);
    owl_zephyr_loadsubs_helper(subs->subs, subs->nsubs);
    deferred_subs = g_list_delete_link(deferred_subs, deferred_subs);
    g_slice_free(owl_sub_list, subs);
  }

  /* zlog in if we need to */
  if (owl_global_is_startuplogin(&g)) {
    owl_function_debugmsg("startup: doing zlog in");
    owl_zephyr_zlog_in();
  }
  /* check pseudo-logins if we need to */
  if (owl_global_is_pseudologins(&g)) {
    owl_function_debugmsg("startup: checking pseudo-logins");
    owl_function_zephyr_buddy_check(0);
  }

  perl = owl_perlconfig_execute("BarnOwl::Zephyr::_zephyr_startup()");
  g_free(perl);
  return FALSE;
}

void owl_zephyr_load_initial_subs(void) {
  int ret_sd, ret_bd, ret_u;

  owl_function_debugmsg("startup: loading initial zephyr subs");

  /* load default subscriptions */
  ret_sd = owl_zephyr_loaddefaultsubs();

  /* load BarnOwl default subscriptions */
  ret_bd = owl_zephyr_loadbarnowldefaultsubs();

  /* load subscriptions from subs file */
  ret_u = owl_zephyr_loadsubs(NULL, 0);

  if (ret_sd || ret_bd || ret_u) {
    owl_function_error("Error loading zephyr subscriptions");
  }

  /* load login subscriptions */
  if (owl_global_is_loginsubs(&g)) {
    owl_function_debugmsg("startup: loading login subs");
    owl_function_loadloginsubs(NULL);
  }
}
#else
void owl_zephyr_initialize(void)
{
}
#endif


int owl_zephyr_shutdown(void)
{
#ifdef HAVE_LIBZEPHYR
  if(owl_global_is_havezephyr(&g)) {
    unsuball();
    ZClosePort();
  }
#endif
  return 0;
}

int owl_zephyr_zpending(void)
{
#ifdef HAVE_LIBZEPHYR
  Code_t code;
  if(owl_global_is_havezephyr(&g)) {
    if((code = ZPending()) < 0) {
      owl_function_debugmsg("Error (%s) in ZPending()\n",
                            error_message(code));
      return 0;
    }
    return code;
  }
#endif
  return 0;
}

int owl_zephyr_zqlength(void)
{
#ifdef HAVE_LIBZEPHYR
  Code_t code;
  if(owl_global_is_havezephyr(&g)) {
    if((code = ZQLength()) < 0) {
      owl_function_debugmsg("Error (%s) in ZQLength()\n",
                            error_message(code));
      return 0;
    }
    return code;
  }
#endif
  return 0;
}

const char *owl_zephyr_get_realm(void)
{
#ifdef HAVE_LIBZEPHYR
  if (owl_global_is_havezephyr(&g))
    return(ZGetRealm());
#endif
  return "";
}

const char *owl_zephyr_get_sender(void)
{
#ifdef HAVE_LIBZEPHYR
  if (owl_global_is_havezephyr(&g))
    return(ZGetSender());
#endif
  return "";
}

#ifdef HAVE_LIBZEPHYR
int owl_zephyr_loadsubs_helper(ZSubscription_t subs[], int count)
{
  int ret = 0;
  Code_t code;

  if (owl_global_is_havezephyr(&g)) {
    int i;
    /* sub without defaults */
    code = ZSubscribeToSansDefaults(subs, count, 0);
    if (code != ZERR_NONE) {
      owl_function_error("Error subscribing to zephyr notifications: %s",
                         error_message(code));
      ret=-2;
    }

    /* free stuff */
    for (i=0; i<count; i++) {
      g_free(subs[i].zsub_class);
      g_free(subs[i].zsub_classinst);
      g_free(subs[i].zsub_recipient);
    }

    g_free(subs);
  } else {
    owl_sub_list *s = g_slice_new(owl_sub_list);
    s->subs = subs;
    s->nsubs = count;
    deferred_subs = g_list_append(deferred_subs, s);
  }

  return ret;
}
#endif

/* Load zephyr subscriptions from 'filename'.  If 'filename' is NULL,
 * the default file $HOME/.zephyr.subs will be used.
 *
 * Returns 0 on success.  If the file does not exist, return -1 if
 * 'error_on_nofile' is 1, otherwise return 0.  Return -1 if the file
 * exists but can not be read.  Return -2 if there is a failure from
 * zephyr to load the subscriptions.
 */
int owl_zephyr_loadsubs(const char *filename, int error_on_nofile)
{
#ifdef HAVE_LIBZEPHYR
  FILE *file;
  int fopen_errno;
  char *tmp, *start, *saveptr;
  char *buffer = NULL;
  char *subsfile;
  ZSubscription_t *subs;
  int subSize = 1024;
  int count;

  subsfile = owl_zephyr_dotfile(".zephyr.subs", filename);

  count = 0;
  file = fopen(subsfile, "r");
  fopen_errno = errno;
  g_free(subsfile);
  if (!file) {
    if (error_on_nofile == 1 || fopen_errno != ENOENT)
      return -1;
    return 0;
  }

  subs = g_new(ZSubscription_t, subSize);
  while (owl_getline(&buffer, file)) {
    if (buffer[0] == '#' || buffer[0] == '\n')
	continue;

    if (buffer[0] == '-')
      start = buffer + 1;
    else
      start = buffer;

    if (count >= subSize) {
      subSize *= 2;
      subs = g_renew(ZSubscription_t, subs, subSize);
    }
    
    /* add it to the list of subs */
    if ((tmp = strtok_r(start, ",\n\r", &saveptr)) == NULL)
      continue;
    subs[count].zsub_class = g_strdup(tmp);
    if ((tmp = strtok_r(NULL, ",\n\r", &saveptr)) == NULL)
      continue;
    subs[count].zsub_classinst = g_strdup(tmp);
    if ((tmp = strtok_r(NULL, " \t\n\r", &saveptr)) == NULL)
      continue;
    subs[count].zsub_recipient = g_strdup(tmp);

    /* if it started with '-' then add it to the global punt list, and
     * remove it from the list of subs. */
    if (buffer[0] == '-') {
      owl_function_zpunt(subs[count].zsub_class, subs[count].zsub_classinst, subs[count].zsub_recipient, 0);
      g_free(subs[count].zsub_class);
      g_free(subs[count].zsub_classinst);
      g_free(subs[count].zsub_recipient);
    } else {
      count++;
    }
  }
  fclose(file);
  if (buffer)
    g_free(buffer);

  ZResetAuthentication();
  return owl_zephyr_loadsubs_helper(subs, count);
#else
  return 0;
#endif
}

/* Load default BarnOwl subscriptions
 *
 * Returns 0 on success.
 * Return -2 if there is a failure from zephyr to load the subscriptions.
 */
int owl_zephyr_loadbarnowldefaultsubs(void)
{
#ifdef HAVE_LIBZEPHYR
  ZSubscription_t *subs;
  int subSize = 10; /* Max BarnOwl default subs we allow */
  int count, ret;

  subs = g_new(ZSubscription_t, subSize);
  ZResetAuthentication();
  count=0;

  subs[count].zsub_class=g_strdup("message");
  subs[count].zsub_classinst=g_strdup("*");
  subs[count].zsub_recipient=g_strdup("%me%");
  count++;

  ret = owl_zephyr_loadsubs_helper(subs, count);
  return(ret);
#else
  return(0);
#endif
}

int owl_zephyr_loaddefaultsubs(void)
{
#ifdef HAVE_LIBZEPHYR
  Code_t ret;

  if (owl_global_is_havezephyr(&g)) {
    ZSubscription_t subs[10];

    ret = ZSubscribeTo(subs, 0, 0);
    if (ret != ZERR_NONE) {
      owl_function_error("Error subscribing to default zephyr notifications: %s.",
                           error_message(ret));
      return(-1);
    }
  }
  return(0);
#else
  return(0);
#endif
}

int owl_zephyr_loadloginsubs(const char *filename)
{
#ifdef HAVE_LIBZEPHYR
  FILE *file;
  ZSubscription_t *subs;
  int numSubs = 100;
  char *subsfile;
  char *buffer = NULL;
  int count;

  subs = g_new(ZSubscription_t, numSubs);
  subsfile = owl_zephyr_dotfile(".anyone", filename);

  count = 0;
  file = fopen(subsfile, "r");
  g_free(subsfile);
  if (file) {
    while (owl_getline_chomp(&buffer, file)) {
      if (buffer[0] == '\0' || buffer[0] == '#')
	continue;

      if (count == numSubs) {
        numSubs *= 2;
        subs = g_renew(ZSubscription_t, subs, numSubs);
      }

      subs[count].zsub_class = g_strdup("login");
      subs[count].zsub_recipient = g_strdup("*");
      subs[count].zsub_classinst = long_zuser(buffer);

      count++;
    }
    fclose(file);
  } else {
    g_free(subs);
    return 0;
  }
  g_free(buffer);

  ZResetAuthentication();
  return owl_zephyr_loadsubs_helper(subs, count);
#else
  return 0;
#endif
}

bool unsuball(void)
{
#if HAVE_LIBZEPHYR
  Code_t ret;

  ZResetAuthentication();
  ret = ZCancelSubscriptions(0);
  if (ret != ZERR_NONE)
    owl_function_error("Zephyr: Cancelling subscriptions: %s",
                       error_message(ret));
  return (ret == ZERR_NONE);
#endif
  return true;
}

int owl_zephyr_sub(const char *class, const char *inst, const char *recip)
{
#ifdef HAVE_LIBZEPHYR
  ZSubscription_t subs[5];
  Code_t ret;

  subs[0].zsub_class=zstr(class);
  subs[0].zsub_classinst=zstr(inst);
  subs[0].zsub_recipient=zstr(recip);

  ZResetAuthentication();
  ret = ZSubscribeTo(subs, 1, 0);
  if (ret != ZERR_NONE) {
    owl_function_error("Error subbing to <%s,%s,%s>: %s",
                       class, inst, recip,
                       error_message(ret));
    return(-2);
  }
  return(0);
#else
  return(0);
#endif
}


int owl_zephyr_unsub(const char *class, const char *inst, const char *recip)
{
#ifdef HAVE_LIBZEPHYR
  ZSubscription_t subs[5];
  Code_t ret;

  subs[0].zsub_class=zstr(class);
  subs[0].zsub_classinst=zstr(inst);
  subs[0].zsub_recipient=zstr(recip);

  ZResetAuthentication();
  ret = ZUnsubscribeTo(subs, 1, 0);
  if (ret != ZERR_NONE) {
    owl_function_error("Error unsubbing from <%s,%s,%s>: %s",
                       class, inst, recip,
                       error_message(ret));
    return(-2);
  }
  return(0);
#else
  return(0);
#endif
}

#ifdef HAVE_LIBZEPHYR
const char *owl_zephyr_first_raw_field(const ZNotice_t *n)
{
  if (n->z_message_len == 0)
    return NULL;
  return n->z_message;
}

const char *owl_zephyr_next_raw_field(const ZNotice_t *n, const char *f)
{
  const char *end = n->z_message + n->z_message_len;
  f = memchr(f, '\0', end - f);
  if (f == NULL)
    return NULL;
  return f + 1;
}

const char *owl_zephyr_get_raw_field(const ZNotice_t *n, int j)
{
  int i;
  const char *f;
  for (i = 1, f = owl_zephyr_first_raw_field(n); i < j && f != NULL;
       i++, f = owl_zephyr_next_raw_field(n, f))
    ;
  return f;
}

CALLER_OWN char *owl_zephyr_field(const ZNotice_t *n, const char *f)
{
  if (f == NULL)
    return g_strdup("");
  return g_strndup(f, n->z_message + n->z_message_len - f);
}

CALLER_OWN char *owl_zephyr_field_as_utf8(const ZNotice_t *n, const char *f)
{
  char *tmp = owl_zephyr_field(n, f);
  char *out = owl_validate_or_convert(tmp);
  g_free(tmp);
  return out;
}

CALLER_OWN char *owl_zephyr_get_field(const ZNotice_t *n, int j)
{
  return owl_zephyr_field(n, owl_zephyr_get_raw_field(n, j));
}

CALLER_OWN char *owl_zephyr_get_field_as_utf8(const ZNotice_t *n, int j)
{
  return owl_zephyr_field_as_utf8(n, owl_zephyr_get_raw_field(n, j));
}
#else
const char *owl_zephyr_first_raw_field(const void *n)
{
  return NULL;
}

const char *owl_zephyr_next_raw_field(const void *n, const char *f)
{
  return NULL;
}

const char *owl_zephyr_get_raw_field(const void *n, int j)
{
  return NULL;
}

CALLER_OWN char *owl_zephyr_field(const void *n, const char *f)
{
  return g_strdup("");
}

CALLER_OWN char *owl_zephyr_field_as_utf8(const void *n, const char *f)
{
  return g_strdup("");
}

CALLER_OWN char *owl_zephyr_get_field(const void *n, int j)
{
  return g_strdup("");
}

CALLER_OWN char *owl_zephyr_get_field_as_utf8(const void *n, int j)
{
  return owl_zephyr_field(n, owl_zephyr_get_raw_field(n, j));
}
#endif


#ifdef HAVE_LIBZEPHYR
int owl_zephyr_get_num_fields(const ZNotice_t *n)
{
  int i;
  const char *f;
  for (i = 0, f = owl_zephyr_first_raw_field(n); f != NULL;
       i++, f = owl_zephyr_next_raw_field(n, f))
    ;
  return i;
}
#else
int owl_zephyr_get_num_fields(const void *n)
{
  return(0);
}
#endif

#ifdef HAVE_LIBZEPHYR
/* return a pointer to the message, place the message length in k
 * caller must free the return
 */
CALLER_OWN char *owl_zephyr_get_message(const ZNotice_t *n, const owl_message *m)
{
#define OWL_NFIELDS	5
  int i;
  char *fields[OWL_NFIELDS + 1];
  char *msg = NULL;

  /* don't let ping messages have a body */
  if (!strcasecmp(n->z_opcode, "ping")) {
    return(g_strdup(""));
  }

  for(i = 0; i < OWL_NFIELDS; i++)
    fields[i + 1] = owl_zephyr_get_field(n, i + 1);

  /* deal with MIT NOC messages */
  if (!strcasecmp(n->z_default_format, "@center(@bold(NOC Message))\n\n@bold(Sender:) $1 <$sender>\n@bold(Time:  ) $time\n\n@italic($opcode service on $instance $3.) $4\n")) {

    msg = g_strdup_printf("%s service on %s %s\n%s", n->z_opcode, n->z_class_inst, fields[3], fields[4]);
  }
  /* deal with MIT Discuss messages */
  else if (!strcasecmp(n->z_default_format, "New transaction [$1] entered in $2\nFrom: $3 ($5)\nSubject: $4") ||
           !strcasecmp(n->z_default_format, "New transaction [$1] entered in $2\nFrom: $3\nSubject: $4")) {
    
    msg = g_strdup_printf("New transaction [%s] entered in %s\nFrom: %s (%s)\nSubject: %s",
                          fields[1], fields[2], fields[3], fields[5], fields[4]);
  }
  /* deal with MIT Moira messages */
  else if (!strcasecmp(n->z_default_format, "MOIRA $instance on $fromhost:\n $message\n")) {
    msg = g_strdup_printf("MOIRA %s on %s: %s",
                          n->z_class_inst,
                          owl_message_get_hostname(m),
                          fields[1]);
  } else {
    if (owl_zephyr_get_num_fields(n) == 1)
      msg = g_strdup(fields[1]);
    else
      msg = g_strdup(fields[2]);
  }

  for (i = 0; i < OWL_NFIELDS; i++)
    g_free(fields[i + 1]);

  return msg;
}
#endif

#ifdef HAVE_LIBZEPHYR
const char *owl_zephyr_get_zsig(const ZNotice_t *n, int *k)
{
  /* return a pointer to the zsig if there is one */

  /* message length 0? No zsig */
  if (n->z_message_len==0) {
    *k=0;
    return("");
  }

  /* If there's only one field, no zsig */
  if (owl_zephyr_get_num_fields(n) == 1) {
    *k=0;
    return("");
  }

  /* Everything else is field 1 */
  *k=strlen(n->z_message);
  return(n->z_message);
}
#else
const char *owl_zephyr_get_zsig(const void *n, int *k)
{
  return("");
}
#endif

int send_zephyr(const char *opcode, const char *zsig, const char *class, const char *instance, const char *recipient, const char *message)
{
#ifdef HAVE_LIBZEPHYR
  Code_t ret;
  ZNotice_t notice;
  char *zsender = NULL;
    
  memset(&notice, 0, sizeof(notice));

  ZResetAuthentication();

  if (!zsig) zsig="";
  
  notice.z_kind=ACKED;
  notice.z_port=0;
#ifdef ZCHARSET_UTF_8
  notice.z_charset = ZCHARSET_UTF_8;
#endif
  notice.z_class=zstr(class);
  notice.z_class_inst=zstr(instance);
  if (!strcmp(recipient, "@")) {
    notice.z_recipient=zstr("");
  } else {
    notice.z_recipient=zstr(recipient);
  }
  if (!owl_zwrite_recip_is_personal(recipient) && *owl_global_get_zsender(&g))
    notice.z_sender = zsender = long_zuser(owl_global_get_zsender(&g));
  notice.z_default_format=zstr(ZEPHYR_DEFAULT_FORMAT);
  if (opcode) notice.z_opcode=zstr(opcode);

  notice.z_message_len=strlen(zsig)+1+strlen(message);
  notice.z_message=g_new(char, notice.z_message_len+10);
  strcpy(notice.z_message, zsig);
  memcpy(notice.z_message+strlen(zsig)+1, message, strlen(message));

  /* ret=ZSendNotice(&notice, ZAUTH); */
  ret=ZSrvSendNotice(&notice, ZAUTH, send_zephyr_helper);
  
  /* free then check the return */
  g_free(notice.z_message);
  g_free(zsender);
  if (ret != ZERR_NONE) {
    owl_function_error("Error sending zephyr: %s", error_message(ret));
    return(ret);
  }
  return(0);
#else
  return(0);
#endif
}

#ifdef HAVE_LIBZEPHYR
Code_t send_zephyr_helper(ZNotice_t *notice, char *buf, int len, int wait)
{
  return(ZSendPacket(buf, len, 0));
}
#endif

void send_ping(const char *to, const char *zclass, const char *zinstance)
{
#ifdef HAVE_LIBZEPHYR
  send_zephyr("PING", "", zclass, zinstance, to, "");
#endif
}

#ifdef HAVE_LIBZEPHYR
void owl_zephyr_handle_ack(const ZNotice_t *retnotice)
{
  char *tmp;
  
  /* if it's an HMACK ignore it */
  if (retnotice->z_kind == HMACK) return;

  if (retnotice->z_kind == SERVNAK) {
    owl_function_error("Authorization failure sending zephyr");
  } else if ((retnotice->z_kind != SERVACK) || !retnotice->z_message_len) {
    owl_function_error("Detected server failure while receiving acknowledgement");
  } else if (!strcmp(retnotice->z_message, ZSRVACK_SENT)) {
    if (!strcasecmp(retnotice->z_opcode, "ping")) {
      return;
    } else {
      if (strcasecmp(retnotice->z_recipient, ""))
      { /* personal */
        tmp=short_zuser(retnotice->z_recipient);
        if(!strcasecmp(retnotice->z_class, "message") &&
           !strcasecmp(retnotice->z_class_inst, "personal")) {
          owl_function_makemsg("Message sent to %s.", tmp);
        } else if(!strcasecmp(retnotice->z_class, "message")) { /* instanced, but not classed, personal */
          owl_function_makemsg("Message sent to %s on -i %s\n", tmp, retnotice->z_class_inst);
        } else { /* classed personal */
          owl_function_makemsg("Message sent to %s on -c %s -i %s\n", tmp, retnotice->z_class, retnotice->z_class_inst);
        }
        g_free(tmp);
      } else {
        /* class / instance message */
          owl_function_makemsg("Message sent to -c %s -i %s\n", retnotice->z_class, retnotice->z_class_inst);
      }
    }
  } else if (!strcmp(retnotice->z_message, ZSRVACK_NOTSENT)) {
    if (retnotice->z_recipient == NULL
        || !owl_zwrite_recip_is_personal(retnotice->z_recipient)) {
      char *buff;
      owl_function_error("No one subscribed to class %s", retnotice->z_class);
      buff = g_strdup_printf("Could not send message to class %s: no one subscribed.\n", retnotice->z_class);
      owl_function_adminmsg("", buff);
      g_free(buff);
    } else {
      char *buff;
      owl_zwrite zw;

      tmp = short_zuser(retnotice->z_recipient);
      owl_function_error("%s: Not logged in or subscribing.", tmp);
      /*
       * These error messages are often over 80 chars, but users who want to
       * see the whole thing can scroll to the side, and for those with wide
       * terminals or who don't care, not splitting saves a line in the UI
       */
      if(strcasecmp(retnotice->z_class, "message")) {
        buff = g_strdup_printf(
                 "Could not send message to %s: "
                 "not logged in or subscribing to class %s, instance %s.\n",
                 tmp,
                 retnotice->z_class,
                 retnotice->z_class_inst);
      } else if(strcasecmp(retnotice->z_class_inst, "personal")) {
        buff = g_strdup_printf(
                 "Could not send message to %s: "
                 "not logged in or subscribing to instance %s.\n",
                 tmp,
                 retnotice->z_class_inst);
      } else {
        buff = g_strdup_printf(
                 "Could not send message to %s: "
                 "not logged in or subscribing to messages.\n",
                 tmp);
      }
      owl_function_adminmsg("", buff);

      memset(&zw, 0, sizeof(zw));
      zw.class = g_strdup(retnotice->z_class);
      zw.inst  = g_strdup(retnotice->z_class_inst);
      zw.realm = g_strdup("");
      zw.opcode = g_strdup(retnotice->z_opcode);
      zw.zsig   = g_strdup("");
      zw.recips = g_ptr_array_new();
      g_ptr_array_add(zw.recips, g_strdup(retnotice->z_recipient));

      owl_log_outgoing_zephyr_error(&zw, buff);

      owl_zwrite_cleanup(&zw);
      g_free(buff);
      g_free(tmp);
    }
  } else {
    owl_function_error("Internal error on ack (%s)", retnotice->z_message);
  }
}
#else
void owl_zephyr_handle_ack(const void *retnotice)
{
}
#endif

#ifdef HAVE_LIBZEPHYR
int owl_zephyr_notice_is_ack(const ZNotice_t *n)
{
  if (n->z_kind == SERVNAK || n->z_kind == SERVACK || n->z_kind == HMACK) {
    if (!strcasecmp(n->z_class, LOGIN_CLASS)) return(0);
    return(1);
  }
  return(0);
}
#else
int owl_zephyr_notice_is_ack(const void *n)
{
  return(0);
}
#endif
  
void owl_zephyr_zaway(const owl_message *m)
{
#ifdef HAVE_LIBZEPHYR
  char *tmpbuff, *myuser, *to;
  owl_zwrite *z;
  
  /* bail if it doesn't look like a message we should reply to.  Some
   * of this defined by the way zaway(1) works
   */
  if (strcasecmp(owl_message_get_class(m), "message")) return;
  if (strcasecmp(owl_message_get_recipient(m), ZGetSender())) return;
  if (!strcasecmp(owl_message_get_sender(m), "")) return;
  if (!strcasecmp(owl_message_get_opcode(m), "ping")) return;
  if (!strcasecmp(owl_message_get_opcode(m), "auto")) return;
  if (!strcasecmp(owl_message_get_zsig(m), "Automated reply:")) return;
  if (!strcasecmp(owl_message_get_sender(m), ZGetSender())) return;
  if (owl_message_get_attribute_value(m, "isauto")) return;

  if (owl_global_is_smartstrip(&g)) {
    to=owl_zephyr_smartstripped_user(owl_message_get_sender(m));
  } else {
    to=g_strdup(owl_message_get_sender(m));
  }

  send_zephyr("",
	      "Automated reply:",
	      owl_message_get_class(m),
	      owl_message_get_instance(m),
	      to,
	      owl_global_get_zaway_msg(&g));

  myuser=short_zuser(to);
  if (!strcasecmp(owl_message_get_instance(m), "personal")) {
    tmpbuff = owl_string_build_quoted("zwrite %q", myuser);
  } else {
    tmpbuff = owl_string_build_quoted("zwrite -i %q %q", owl_message_get_instance(m), myuser);
  }
  g_free(myuser);
  g_free(to);

  z = owl_zwrite_new_from_line(tmpbuff);
  g_free(tmpbuff);
  if (z == NULL) {
    owl_function_error("Error creating outgoing zephyr.");
    return;
  }
  owl_zwrite_set_message(z, owl_global_get_zaway_msg(&g));
  owl_zwrite_set_zsig(z, "Automated reply:");

  /* display the message as an admin message in the receive window */
  owl_function_add_outgoing_zephyrs(z);
  owl_zwrite_delete(z);
#endif
}

#ifdef HAVE_LIBZEPHYR
void owl_zephyr_hackaway_cr(ZNotice_t *n)
{
  /* replace \r's with ' '.  Gross-ish */
  int i;

  for (i=0; i<n->z_message_len; i++) {
    if (n->z_message[i]=='\r') {
      n->z_message[i]=' ';
    }
  }
}
#endif

CALLER_OWN char *owl_zephyr_zlocate(const char *user, int auth)
{
#ifdef HAVE_LIBZEPHYR
  int ret, numlocs;
  int one = 1;
  ZLocations_t locations;
  char *myuser;
  char *p, *result;

  ZResetAuthentication();
  ret = ZLocateUser(zstr(user), &numlocs, auth ? ZAUTH : ZNOAUTH);
  if (ret != ZERR_NONE)
    return g_strdup_printf("Error locating user %s: %s\n",
			   user, error_message(ret));

  myuser = short_zuser(user);
  if (numlocs == 0) {
    result = g_strdup_printf("%s: Hidden or not logged in\n", myuser);
  } else {
    result = g_strdup("");
    for (; numlocs; numlocs--) {
      ZGetLocations(&locations, &one);
      p = g_strdup_printf("%s%s: %s\t%s\t%s\n",
			  result, myuser,
			  locations.host ? locations.host : "?",
			  locations.tty ? locations.tty : "?",
			  locations.time ? locations.time : "?");
      g_free(result);
      result = p;
    }
  }
  g_free(myuser);

  return result;
#else
  return g_strdup("");
#endif
}

void owl_zephyr_addsub(const char *filename, const char *class, const char *inst, const char *recip)
{
#ifdef HAVE_LIBZEPHYR
  char *line, *subsfile, *s = NULL;
  FILE *file;
  int duplicate = 0;

  line = owl_zephyr_makesubline(class, inst, recip);
  subsfile = owl_zephyr_dotfile(".zephyr.subs", filename);

  /* if the file already exists, check to see if the sub is already there */
  file = fopen(subsfile, "r");
  if (file) {
    while (owl_getline(&s, file)) {
      if (strcasecmp(s, line) == 0) {
	owl_function_error("Subscription already present in %s", subsfile);
	duplicate++;
      }
    }
    fclose(file);
    g_free(s);
  }

  if (!duplicate) {
    file = fopen(subsfile, "a");
    if (file) {
      fputs(line, file);
      fclose(file);
      owl_function_makemsg("Subscription added");
    } else {
      owl_function_error("Error opening file %s for writing", subsfile);
    }
  }

  g_free(subsfile);
  g_free(line);
#endif
}

void owl_zephyr_delsub(const char *filename, const char *class, const char *inst, const char *recip)
{
#ifdef HAVE_LIBZEPHYR
  char *line, *subsfile;
  int linesdeleted;
  
  line=owl_zephyr_makesubline(class, inst, recip);
  line[strlen(line)-1]='\0';

  subsfile = owl_zephyr_dotfile(".zephyr.subs", filename);
  
  linesdeleted = owl_util_file_deleteline(subsfile, line, 1);
  if (linesdeleted > 0) {
    owl_function_makemsg("Subscription removed");
  } else if (linesdeleted == 0) {
    owl_function_error("No subscription present in %s", subsfile);
  }
  g_free(subsfile);
  g_free(line);
#endif
}

/* caller must free the return */
CALLER_OWN char *owl_zephyr_makesubline(const char *class, const char *inst, const char *recip)
{
  return g_strdup_printf("%s,%s,%s\n", class, inst, !strcmp(recip, "") ? "*" : recip);
}


void owl_zephyr_zlog_in(void)
{
#ifdef HAVE_LIBZEPHYR
  ZResetAuthentication();

  /* ZSetLocation, and store the default value as the current value */
  owl_global_set_exposure(&g, owl_global_get_default_exposure(&g));
#endif
}

void owl_zephyr_zlog_out(void)
{
#ifdef HAVE_LIBZEPHYR
  Code_t ret;

  ZResetAuthentication();
  ret = ZUnsetLocation();
  if (ret != ZERR_NONE)
    owl_function_error("Error unsetting location: %s", error_message(ret));
#endif
}

void owl_zephyr_addbuddy(const char *name)
{
  char *filename;
  FILE *file;
  
  filename = owl_zephyr_dotfile(".anyone", NULL);
  file = fopen(filename, "a");
  g_free(filename);
  if (!file) {
    owl_function_error("Error opening zephyr buddy file for append");
    return;
  }
  fprintf(file, "%s\n", name);
  fclose(file);
}

void owl_zephyr_delbuddy(const char *name)
{
  char *filename;

  filename = owl_zephyr_dotfile(".anyone", NULL);
  owl_util_file_deleteline(filename, name, 0);
  g_free(filename);
}

#ifdef HAVE_LIBZEPHYR
const char *owl_zephyr_get_charsetstr(const ZNotice_t *n)
{
#ifdef ZCHARSET_UTF_8
  return ZCharsetToString(n->z_charset);
#else
  return "";
#endif
}
#else
const char *owl_zephyr_get_charsetstr(const void *n)
{
  return "";
}
#endif

/* return auth string */
#ifdef HAVE_LIBZEPHYR
const char *owl_zephyr_get_authstr(const ZNotice_t *n)
{

  if (!n) return("UNKNOWN");

  if (n->z_auth == ZAUTH_FAILED) {
    return ("FAILED");
  } else if (n->z_auth == ZAUTH_NO) {
    return ("NO");
  } else if (n->z_auth == ZAUTH_YES) {
    return ("YES");
  } else {
    return ("UNKNOWN");
  }           
}
#else
const char *owl_zephyr_get_authstr(const void *n)
{
  return("");
}
#endif

/* Returns a buffer of subscriptions or an error message.  Caller must
 * free the return.
 */
CALLER_OWN char *owl_zephyr_getsubs(void)
{
#ifdef HAVE_LIBZEPHYR
  Code_t ret;
  int num, i, one;
  ZSubscription_t sub;
  GString *buf;

  ret = ZRetrieveSubscriptions(0, &num);
  if (ret != ZERR_NONE)
    return g_strdup_printf("Zephyr: Requesting subscriptions: %s\n", error_message(ret));
  if (num == 0)
    return g_strdup("Zephyr: No subscriptions retrieved\n");

  buf = g_string_new("");
  for (i=0; i<num; i++) {
    one = 1;
    if ((ret = ZGetSubscriptions(&sub, &one)) != ZERR_NONE) {
      ZFlushSubscriptions();
      g_string_free(buf, true);
      return g_strdup_printf("Zephyr: Getting subscriptions: %s\n", error_message(ret));
    } else {
      /* g_string_append_printf would be backwards. */
      char *tmp = g_strdup_printf("<%s,%s,%s>\n",
                                  sub.zsub_class,
                                  sub.zsub_classinst,
                                  sub.zsub_recipient);
      g_string_prepend(buf, tmp);
      g_free(tmp);
    }
  }

  ZFlushSubscriptions();
  return g_string_free(buf, false);
#else
  return(g_strdup("Zephyr not available"));
#endif
}

const char *owl_zephyr_get_variable(const char *var)
{
#ifdef HAVE_LIBZEPHYR
  return(ZGetVariable(zstr(var)));
#else
  return("");
#endif
}

void owl_zephyr_set_locationinfo(const char *host, const char *val)
{
#ifdef HAVE_LIBZEPHYR
  ZInitLocationInfo(zstr(host), zstr(val));
#endif
}

const char *owl_zephyr_normalize_exposure(const char *exposure)
{
  if (exposure == NULL)
    return NULL;
#ifdef HAVE_LIBZEPHYR
  return ZParseExposureLevel(zstr(exposure));
#else
  return exposure;
#endif
}

int owl_zephyr_set_default_exposure(const char *exposure)
{
#ifdef HAVE_LIBZEPHYR
  Code_t ret;
  if (exposure == NULL)
    return -1;
  exposure = ZParseExposureLevel(zstr(exposure));
  if (exposure == NULL)
    return -1;
  ret = ZSetVariable(zstr("exposure"), zstr(exposure)); /* ZSetVariable does file I/O */
  if (ret != ZERR_NONE) {
    owl_function_error("Unable to set default exposure location: %s", error_message(ret));
    return -1;
  }
#endif
  return 0;
}

const char *owl_zephyr_get_default_exposure(void)
{
#ifdef HAVE_LIBZEPHYR
  const char *exposure = ZGetVariable(zstr("exposure")); /* ZGetVariable does file I/O */
  if (exposure == NULL)
    return EXPOSE_REALMVIS;
  exposure = ZParseExposureLevel(zstr(exposure));
  if (exposure == NULL) /* The user manually entered an invalid value in ~/.zephyr.vars, or something weird happened. */
    return EXPOSE_REALMVIS;
  return exposure;
#else
  return "";
#endif
}

int owl_zephyr_set_exposure(const char *exposure)
{
#ifdef HAVE_LIBZEPHYR
  Code_t ret;
  if (exposure == NULL)
    return -1;
  exposure = ZParseExposureLevel(zstr(exposure));
  if (exposure == NULL)
    return -1;
  ret = ZSetLocation(zstr(exposure));
  if (ret != ZERR_NONE
#ifdef ZCONST
      /* Before zephyr 3.0, ZSetLocation had a bug where, if you were subscribed
       * to your own logins, it could wait for the wrong notice and return
       * ZERR_INTERNAL when found neither SERVACK nor SERVNAK. Suppress it when
       * building against the old ABI. */
      && ret != ZERR_INTERNAL
#endif
     ) {
    owl_function_error("Unable to set exposure level: %s.", error_message(ret));
    return -1;
  }
#endif
  return 0;
}
  
/* Strip a local realm fron the zephyr user name.
 * The caller must free the return
 */
CALLER_OWN char *short_zuser(const char *in)
{
  char *ptr = strrchr(in, '@');
  if (ptr && (ptr[1] == '\0' || !strcasecmp(ptr+1, owl_zephyr_get_realm()))) {
    return g_strndup(in, ptr - in);
  }
  return g_strdup(in);
}

/* Append a local realm to the zephyr user name if necessary.
 * The caller must free the return.
 */
CALLER_OWN char *long_zuser(const char *in)
{
  char *ptr = strrchr(in, '@');
  if (ptr) {
    if (ptr[1])
      return g_strdup(in);
    /* Ends in @, so assume default realm. */
    return g_strdup_printf("%s%s", in, owl_zephyr_get_realm());
  }
  return g_strdup_printf("%s@%s", in, owl_zephyr_get_realm());
}

/* Return the realm of the zephyr user name. Caller does /not/ free the return.
 * The string is valid at least as long as the input is.
 */
const char *zuser_realm(const char *in)
{
  char *ptr = strrchr(in, '@');
  /* If the name has an @ and does not end with @, use that. Otherwise, take
   * the default realm. */
  return (ptr && ptr[1]) ? (ptr+1) : owl_zephyr_get_realm();
}

/* strip out the instance from a zsender's principal.  Preserves the
 * realm if present.  Leave host/ and daemon/ krb5 principals
 * alone. Also leave rcmd. and daemon. krb4 principals alone. The
 * caller must free the return.
 */
CALLER_OWN char *owl_zephyr_smartstripped_user(const char *in)
{
  int n = strcspn(in, "./");
  const char *realm = strchr(in, '@');
  if (realm == NULL)
    realm = in + strlen(in);

  if (in + n >= realm ||
      g_str_has_prefix(in, OWL_ZEPHYR_NOSTRIP_HOST) ||
      g_str_has_prefix(in, OWL_ZEPHYR_NOSTRIP_RCMD) ||
      g_str_has_prefix(in, OWL_ZEPHYR_NOSTRIP_DAEMON5) ||
      g_str_has_prefix(in, OWL_ZEPHYR_NOSTRIP_DAEMON4))
    return g_strdup(in);
  else
    return g_strdup_printf("%.*s%s", n, in, realm);
}

/* Read the list of users in 'filename' as a .anyone file, and return as a
 * GPtrArray of strings.  If 'filename' is NULL, use the default .anyone file
 * in the users home directory.  Returns NULL on failure.
 */
GPtrArray *owl_zephyr_get_anyone_list(const char *filename)
{
#ifdef HAVE_LIBZEPHYR
  char *ourfile, *tmp, *s = NULL;
  FILE *f;
  GPtrArray *list;

  ourfile = owl_zephyr_dotfile(".anyone", filename);

  f = fopen(ourfile, "r");
  if (!f) {
    owl_function_error("Error opening file %s: %s", ourfile, strerror(errno) ? strerror(errno) : "");
    g_free(ourfile);
    return NULL;
  }
  g_free(ourfile);

  list = g_ptr_array_new();
  while (owl_getline_chomp(&s, f)) {
    /* ignore comments, blank lines etc. */
    if (s[0] == '#' || s[0] == '\0')
      continue;

    /* ignore from # on */
    tmp = strchr(s, '#');
    if (tmp)
      tmp[0] = '\0';

    /* ignore from SPC */
    tmp = strchr(s, ' ');
    if (tmp)
      tmp[0] = '\0';

    g_ptr_array_add(list, long_zuser(s));
  }
  g_free(s);
  fclose(f);
  return list;
#else
  return NULL;
#endif
}

#ifdef HAVE_LIBZEPHYR
void owl_zephyr_process_pseudologin(ZNotice_t *n)
{
  owl_message *m;
  owl_zbuddylist *zbl;
  GList **zaldlist;
  GList *zaldptr;
  ZAsyncLocateData_t *zald = NULL;
  ZLocations_t location;
  int numlocs, ret, notify;

  /* Find a ZALD to match this notice. */
  zaldlist = owl_global_get_zaldlist(&g);
  zaldptr = g_list_first(*zaldlist);
  while (zaldptr) {
    if (ZCompareALDPred(n, zaldptr->data)) {
      zald = zaldptr->data;
      *zaldlist = g_list_remove(*zaldlist, zaldptr->data);
      break;
    }
    zaldptr = g_list_next(zaldptr);
  }
  if (zald) {
    /* Deal with notice. */
    notify = owl_global_get_pseudologin_notify(&g);
    zbl = owl_global_get_zephyr_buddylist(&g);
    ret = ZParseLocations(n, zald, &numlocs, NULL);
    if (ret == ZERR_NONE) {
      if (numlocs > 0 && !owl_zbuddylist_contains_user(zbl, zald->user)) {
        if (notify) {
          numlocs = 1;
          ret = ZGetLocations(&location, &numlocs);
          if (ret == ZERR_NONE) {
            /* Send a PSEUDO LOGIN! */
            m = g_slice_new(owl_message);
            owl_message_create_pseudo_zlogin(m, 0, zald->user,
                                             location.host,
                                             location.time,
                                             location.tty);
            owl_global_messagequeue_addmsg(&g, m);
          }
          owl_zbuddylist_adduser(zbl, zald->user);
          owl_function_debugmsg("owl_function_zephyr_buddy_check: login for %s ", zald->user);
        }
      } else if (numlocs == 0 && owl_zbuddylist_contains_user(zbl, zald->user)) {
        /* Send a PSEUDO LOGOUT! */
        if (notify) {
          m = g_slice_new(owl_message);
          owl_message_create_pseudo_zlogin(m, 1, zald->user, "", "", "");
          owl_global_messagequeue_addmsg(&g, m);
        }
        owl_zbuddylist_deluser(zbl, zald->user);
        owl_function_debugmsg("owl_function_zephyr_buddy_check: logout for %s ", zald->user);
      }
    }
    ZFreeALD(zald);
    g_slice_free(ZAsyncLocateData_t, zald);
  }
}
#else
void owl_zephyr_process_pseudologin(void *n)
{
}
#endif

gboolean owl_zephyr_buddycheck_timer(void *data)
{
  if (owl_global_is_pseudologins(&g)) {
    owl_function_debugmsg("Doing zephyr buddy check");
    owl_function_zephyr_buddy_check(1);
  } else {
    owl_function_debugmsg("Warning: owl_zephyr_buddycheck_timer call pointless; timer should have been disabled");
  }
  return TRUE;
}

/*
 * Process zephyrgrams from libzephyr's queue. To prevent starvation,
 * process a maximum of OWL_MAX_ZEPHYRGRAMS_TO_PROCESS.
 *
 * Returns the number of zephyrgrams processed.
 */

#define OWL_MAX_ZEPHYRGRAMS_TO_PROCESS 20

#ifdef HAVE_LIBZEPHYR
static int _owl_zephyr_process_events(void)
{
  int zpendcount=0;
  ZNotice_t notice;
  Code_t code;
  owl_message *m=NULL;

  while(owl_zephyr_zpending() && zpendcount < OWL_MAX_ZEPHYRGRAMS_TO_PROCESS) {
    if (owl_zephyr_zpending()) {
      if ((code = ZReceiveNotice(&notice, NULL)) != ZERR_NONE) {
        owl_function_debugmsg("Error: %s while calling ZReceiveNotice\n",
                              error_message(code));
        continue;
      }
      zpendcount++;

      /* is this an ack from a zephyr we sent? */
      if (owl_zephyr_notice_is_ack(&notice)) {
        owl_zephyr_handle_ack(&notice);
        ZFreeNotice(&notice);
        continue;
      }

      /* if it's a ping and we're not viewing pings then skip it */
      if (!owl_global_is_rxping(&g) && !strcasecmp(notice.z_opcode, "ping")) {
        ZFreeNotice(&notice);
        continue;
      }

      /* if it is a LOCATE message, it's for pseudologins. */
      if (strcmp(notice.z_opcode, LOCATE_LOCATE) == 0) {
        owl_zephyr_process_pseudologin(&notice);
        ZFreeNotice(&notice);
        continue;
      }

      /* create the new message */
      m=g_slice_new(owl_message);
      owl_message_create_from_znotice(m, &notice);

      owl_global_messagequeue_addmsg(&g, m);
    }
  }
  return zpendcount;
}

typedef struct { /*noproto*/
  GSource source;
  GPollFD poll_fd;
} owl_zephyr_event_source;

static GSource *owl_zephyr_event_source_new(int fd) {
  GSource *source;
  owl_zephyr_event_source *event_source;

  source = g_source_new(&zephyr_event_funcs, sizeof(owl_zephyr_event_source));
  event_source = (owl_zephyr_event_source*) source;
  event_source->poll_fd.fd = fd;
  event_source->poll_fd.events = G_IO_IN | G_IO_HUP | G_IO_ERR;
  g_source_add_poll(source, &event_source->poll_fd);

  return source;
}

static gboolean owl_zephyr_event_prepare(GSource *source, int *timeout) {
  *timeout = -1;
  return owl_zephyr_zqlength() > 0;
}

static gboolean owl_zephyr_event_check(GSource *source) {
  owl_zephyr_event_source *event_source = (owl_zephyr_event_source*)source;
  if (event_source->poll_fd.revents & event_source->poll_fd.events)
    return owl_zephyr_zpending() > 0;
  return FALSE;
}

static gboolean owl_zephyr_event_dispatch(GSource *source, GSourceFunc callback, gpointer user_data) {
  _owl_zephyr_process_events();
  return TRUE;
}
#endif
