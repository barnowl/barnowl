#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include "owl.h"

#ifdef HAVE_LIBZEPHYR
static GList *deferred_subs = NULL;

typedef struct _owl_sub_list {                            /* noproto */
  ZSubscription_t *subs;
  int nsubs;
} owl_sub_list;

Code_t ZResetAuthentication(void);
#endif

#define HM_SVC_FALLBACK		htons((unsigned short) 2104)

static char *owl_zephyr_dotfile(const char *name, const char *input)
{
  if (input != NULL)
    return owl_strdup(input);
  else
    return owl_sprintf("%s/%s", owl_global_get_homedir(&g), name);
}

#ifdef HAVE_LIBZEPHYR
void owl_zephyr_initialize(void)
{
  int ret;
  struct servent *sp;
  struct sockaddr_in sin;
  ZNotice_t req;
  Code_t code;

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

  if ((code = ZSetDestAddr(&sin)) != ZERR_NONE) {
    owl_function_error("Initializing Zephyr: %s", error_message(code));
    return;
  }

  if ((code = ZSendNotice(&req, ZNOAUTH)) != ZERR_NONE) {
    owl_function_error("Initializing Zephyr: %s", error_message(code));
    return;
  }

  owl_select_add_io_dispatch(ZGetFD(), OWL_IO_READ|OWL_IO_EXCEPT, &owl_zephyr_finish_initialization, NULL, NULL);
}

void owl_zephyr_finish_initialization(const owl_io_dispatch *d, void *data) {
  Code_t code;
  char *perl;

  owl_select_remove_io_dispatch(d);

  ZClosePort();

  if ((code = ZInitialize()) != ZERR_NONE) {
    owl_function_error("Initializing Zephyr: %s", error_message(code));
    return;
  }

  if ((code = ZOpenPort(NULL)) != ZERR_NONE) {
    owl_function_error("Initializing Zephyr: %s", error_message(code));
    return;
  }

  owl_select_add_io_dispatch(ZGetFD(), OWL_IO_READ|OWL_IO_EXCEPT, &owl_zephyr_process_events, NULL, NULL);

  owl_global_set_havezephyr(&g);

  if(g.load_initial_subs) {
    owl_zephyr_load_initial_subs();
  }
  while(deferred_subs != NULL) {
    owl_sub_list *subs = deferred_subs->data;
    owl_function_debugmsg("Loading %d deferred subs.", subs->nsubs);
    owl_zephyr_loadsubs_helper(subs->subs, subs->nsubs);
    deferred_subs = g_list_delete_link(deferred_subs, deferred_subs);
    owl_free(subs);
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
  owl_free(perl);

  owl_select_add_pre_select_action(owl_zephyr_pre_select_action, NULL, NULL);
}

void owl_zephyr_load_initial_subs(void) {
  int ret_sd, ret_bd, ret_u;

  owl_function_debugmsg("startup: loading initial zephyr subs");

  /* load default subscriptions */
  ret_sd = owl_zephyr_loaddefaultsubs();

  /* load Barnowl default subscriptions */
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
  if (owl_global_is_havezephyr(&g)) {
    int i;
    /* sub without defaults */
    if (ZSubscribeToSansDefaults(subs,count,0) != ZERR_NONE) {
      owl_function_error("Error subscribing to zephyr notifications.");
      ret=-2;
    }

    /* free stuff */
    for (i=0; i<count; i++) {
      owl_free(subs[i].zsub_class);
      owl_free(subs[i].zsub_classinst);
      owl_free(subs[i].zsub_recipient);
    }

    owl_free(subs);
  } else {
    owl_sub_list *s = owl_malloc(sizeof(owl_sub_list));
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
  char *tmp, *start;
  char *buffer = NULL;
  char *subsfile;
  ZSubscription_t *subs;
  int subSize = 1024;
  int count;
  struct stat statbuff;

  subs = owl_malloc(sizeof(ZSubscription_t) * subSize);
  subsfile = owl_zephyr_dotfile(".zephyr.subs", filename);

  if (stat(subsfile, &statbuff) != 0) {
    owl_free(subsfile);
    if (error_on_nofile == 1)
      return -1;
    return 0;
  }

  ZResetAuthentication();
  count = 0;
  file = fopen(subsfile, "r");
  owl_free(subsfile);
  if (!file)
    return -1;
  while (owl_getline(&buffer, file)) {
    if (buffer[0] == '#' || buffer[0] == '\n')
	continue;

    if (buffer[0] == '-')
      start = buffer + 1;
    else
      start = buffer;

    if (count >= subSize) {
      subSize *= 2;
      subs = owl_realloc(subs, sizeof(ZSubscription_t) * subSize);
    }
    
    /* add it to the list of subs */
    if ((tmp = strtok(start, ",\n\r")) == NULL)
      continue;
    subs[count].zsub_class = owl_strdup(tmp);
    if ((tmp=strtok(NULL, ",\n\r")) == NULL)
      continue;
    subs[count].zsub_classinst = owl_strdup(tmp);
    if ((tmp = strtok(NULL, " \t\n\r")) == NULL)
      continue;
    subs[count].zsub_recipient = owl_strdup(tmp);

    /* if it started with '-' then add it to the global punt list, and
     * remove it from the list of subs. */
    if (buffer[0] == '-') {
      owl_function_zpunt(subs[count].zsub_class, subs[count].zsub_classinst, subs[count].zsub_recipient, 0);
      owl_free(subs[count].zsub_class);
      owl_free(subs[count].zsub_classinst);
      owl_free(subs[count].zsub_recipient);
    } else {
      count++;
    }
  }
  fclose(file);
  if (buffer)
    owl_free(buffer);

  return owl_zephyr_loadsubs_helper(subs, count);
#else
  return 0;
#endif
}

/* Load default Barnowl subscriptions
 *
 * Returns 0 on success.
 * Return -2 if there is a failure from zephyr to load the subscriptions.
 */
int owl_zephyr_loadbarnowldefaultsubs(void)
{
#ifdef HAVE_LIBZEPHYR
  ZSubscription_t *subs;
  int subSize = 10; /* Max Barnowl default subs we allow */
  int count, ret;

  subs = owl_malloc(sizeof(ZSubscription_t) * subSize);
  ZResetAuthentication();
  count=0;

  subs[count].zsub_class=owl_strdup("message");
  subs[count].zsub_classinst=owl_strdup("*");
  subs[count].zsub_recipient=owl_strdup("%me%");
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
  if (owl_global_is_havezephyr(&g)) {
    ZSubscription_t subs[10];
    
    if (ZSubscribeTo(subs,0,0) != ZERR_NONE) {
      owl_function_error("Error subscribing to default zephyr notifications.");
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
  struct stat statbuff;

  subs = owl_malloc(numSubs * sizeof(ZSubscription_t));
  subsfile = owl_zephyr_dotfile(".anyone", filename);

  if (stat(subsfile, &statbuff) == -1) {
    owl_free(subs);
    owl_free(subsfile);
    return 0;
  }

  ZResetAuthentication();
  count = 0;
  file = fopen(subsfile, "r");
  owl_free(subsfile);
  if (file) {
    while (owl_getline_chomp(&buffer, file)) {
      if (buffer[0] == '\0' || buffer[0] == '#')
	continue;

      if (count == numSubs) {
        numSubs *= 2;
        subs = owl_realloc(subs, numSubs * sizeof(ZSubscription_t));
      }

      subs[count].zsub_class = owl_strdup("login");
      subs[count].zsub_recipient = owl_strdup("*");
      subs[count].zsub_classinst = long_zuser(buffer);

      count++;
    }
    fclose(file);
  } else {
    return 0;
  }
  if (buffer)
    owl_free(buffer);

  return owl_zephyr_loadsubs_helper(subs, count);
#else
  return 0;
#endif
}

void unsuball(void)
{
#if HAVE_LIBZEPHYR
  int ret;

  ZResetAuthentication();
  ret=ZCancelSubscriptions(0);
  if (ret != ZERR_NONE) {
    com_err("owl",ret,"while unsubscribing");
  }
#endif
}

int owl_zephyr_sub(const char *class, const char *inst, const char *recip)
{
#ifdef HAVE_LIBZEPHYR
  ZSubscription_t subs[5];

  subs[0].zsub_class=zstr(class);
  subs[0].zsub_classinst=zstr(inst);
  subs[0].zsub_recipient=zstr(recip);

  ZResetAuthentication();
  if (ZSubscribeTo(subs,1,0) != ZERR_NONE) {
    owl_function_error("Error subbing to <%s,%s,%s>", class, inst, recip);
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

  subs[0].zsub_class=zstr(class);
  subs[0].zsub_classinst=zstr(inst);
  subs[0].zsub_recipient=zstr(recip);

  ZResetAuthentication();
  if (ZUnsubscribeTo(subs,1,0) != ZERR_NONE) {
    owl_function_error("Error unsubbing from <%s,%s,%s>", class, inst, recip);
    return(-2);
  }
  return(0);
#else
  return(0);
#endif
}

/* return a pointer to the data in the Jth field, (NULL terminated by
 * definition).  Caller must free the return.
 */
#ifdef HAVE_LIBZEPHYR
char *owl_zephyr_get_field(const ZNotice_t *n, int j)
{
  int i, count, save;

  /* If there's no message here, just run along now */
  if (n->z_message_len == 0)
    return(owl_strdup(""));

  count=save=0;
  for (i=0; i<n->z_message_len; i++) {
    if (n->z_message[i]=='\0') {
      count++;
      if (count==j) {
	/* just found the end of the field we're looking for */
	return(owl_strdup(n->z_message+save));
      } else {
	save=i+1;
      }
    }
  }
  /* catch the last field, which might not be null terminated */
  if (count==j-1) {
    return g_strndup(n->z_message + save, n->z_message_len - save);
  }

  return(owl_strdup(""));
}

char *owl_zephyr_get_field_as_utf8(const ZNotice_t *n, int j)
{
  int i, count, save;

  /* If there's no message here, just run along now */
  if (n->z_message_len == 0)
    return(owl_strdup(""));

  count=save=0;
  for (i = 0; i < n->z_message_len; i++) {
    if (n->z_message[i]=='\0') {
      count++;
      if (count == j) {
	/* just found the end of the field we're looking for */
	return(owl_validate_or_convert(n->z_message + save));
      } else {
	save = i + 1;
      }
    }
  }
  /* catch the last field, which might not be null terminated */
  if (count == j - 1) {
    char *tmp, *out;
    tmp = g_strndup(n->z_message + save, n->z_message_len - save);
    out = owl_validate_or_convert(tmp);
    owl_free(tmp);
    return out;
  }

  return(owl_strdup(""));
}
#else
char *owl_zephyr_get_field(void *n, int j)
{
  return(owl_strdup(""));
}
char *owl_zephyr_get_field_as_utf8(void *n, int j)
{
  return owl_zephyr_get_field(n, j);
}
#endif


#ifdef HAVE_LIBZEPHYR
int owl_zephyr_get_num_fields(const ZNotice_t *n)
{
  int i, fields;

  if(n->z_message_len == 0)
    return 0;

  fields=1;
  for (i=0; i<n->z_message_len; i++) {
    if (n->z_message[i]=='\0') fields++;
  }
  
  return(fields);
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
char *owl_zephyr_get_message(const ZNotice_t *n, const owl_message *m)
{
  /* don't let ping messages have a body */
  if (!strcasecmp(n->z_opcode, "ping")) {
    return(owl_strdup(""));
  }

  /* deal with MIT NOC messages */
  if (!strcasecmp(n->z_default_format, "@center(@bold(NOC Message))\n\n@bold(Sender:) $1 <$sender>\n@bold(Time:  ) $time\n\n@italic($opcode service on $instance $3.) $4\n")) {
    char *msg, *field3, *field4;

    field3 = owl_zephyr_get_field(n, 3);
    field4 = owl_zephyr_get_field(n, 4);

    msg = owl_sprintf("%s service on %s %s\n%s", n->z_opcode, n->z_class_inst, field3, field4);
    owl_free(field3);
    owl_free(field4);
    if (msg) {
      return msg;
    }
  }
  /* deal with MIT Discuss messages */
  else if (!strcasecmp(n->z_default_format, "New transaction [$1] entered in $2\nFrom: $3 ($5)\nSubject: $4") ||
           !strcasecmp(n->z_default_format, "New transaction [$1] entered in $2\nFrom: $3\nSubject: $4")) {
    char *msg, *field1, *field2, *field3, *field4, *field5;
    
    field1 = owl_zephyr_get_field(n, 1);
    field2 = owl_zephyr_get_field(n, 2);
    field3 = owl_zephyr_get_field(n, 3);
    field4 = owl_zephyr_get_field(n, 4);
    field5 = owl_zephyr_get_field(n, 5);
    
    msg = owl_sprintf("New transaction [%s] entered in %s\nFrom: %s (%s)\nSubject: %s", field1, field2, field3, field5, field4);
    owl_free(field1);
    owl_free(field2);
    owl_free(field3);
    owl_free(field4);
    owl_free(field5);
    if (msg) {
      return msg;
    }
  }
  /* deal with MIT Moira messages */
  else if (!strcasecmp(n->z_default_format, "MOIRA $instance on $fromhost:\n $message\n")) {
    char *msg, *field1;
    
    field1 = owl_zephyr_get_field(n, 1);
    
    msg = owl_sprintf("MOIRA %s on %s: %s", n->z_class_inst, owl_message_get_hostname(m), field1);
    owl_free(field1);
    if (msg) {
      return msg;
    }
  }

  if (owl_zephyr_get_num_fields(n) == 1) {
    return(owl_zephyr_get_field(n, 1));
  }
  else {
    return(owl_zephyr_get_field(n, 2));
  }
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
  int ret;
  ZNotice_t notice;
    
  memset(&notice, 0, sizeof(notice));

  ZResetAuthentication();

  if (!zsig) zsig="";
  
  notice.z_kind=ACKED;
  notice.z_port=0;
  notice.z_class=zstr(class);
  notice.z_class_inst=zstr(instance);
  notice.z_sender=NULL;
  if (!strcmp(recipient, "*") || !strcmp(recipient, "@")) {
    notice.z_recipient=zstr("");
    if (*owl_global_get_zsender(&g))
        notice.z_sender=zstr(owl_global_get_zsender(&g));
  } else {
    notice.z_recipient=zstr(recipient);
  }
  notice.z_default_format=zstr("Class $class, Instance $instance:\nTo: @bold($recipient) at $time $date\nFrom: @bold{$1 <$sender>}\n\n$2");
  if (opcode) notice.z_opcode=zstr(opcode);

  notice.z_message_len=strlen(zsig)+1+strlen(message);
  notice.z_message=owl_malloc(notice.z_message_len+10);
  strcpy(notice.z_message, zsig);
  memcpy(notice.z_message+strlen(zsig)+1, message, strlen(message));

  /* ret=ZSendNotice(&notice, ZAUTH); */
  ret=ZSrvSendNotice(&notice, ZAUTH, send_zephyr_helper);
  
  /* free then check the return */
  owl_free(notice.z_message);
  ZFreeNotice(&notice);
  if (ret!=ZERR_NONE) {
    owl_function_error("Error sending zephyr");
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
        owl_free(tmp);
      } else {
        /* class / instance message */
          owl_function_makemsg("Message sent to -c %s -i %s\n", retnotice->z_class, retnotice->z_class_inst);
      }
    }
  } else if (!strcmp(retnotice->z_message, ZSRVACK_NOTSENT)) {
    #define BUFFLEN 1024
    if (retnotice->z_recipient == NULL
        || *retnotice->z_recipient == 0
        || *retnotice->z_recipient == '@') {
      char buff[BUFFLEN];
      owl_function_error("No one subscribed to class %s", retnotice->z_class);
      snprintf(buff, BUFFLEN, "Could not send message to class %s: no one subscribed.\n", retnotice->z_class);
      owl_function_adminmsg("", buff);
    } else {
      char buff[BUFFLEN];
      owl_zwrite zw;
      char *realm;

      tmp = short_zuser(retnotice->z_recipient);
      owl_function_error("%s: Not logged in or subscribing.", tmp);
      /*
       * These error messages are often over 80 chars, but users who want to
       * see the whole thing can scroll to the side, and for those with wide
       * terminals or who don't care, not splitting saves a line in the UI
       */
      if(strcasecmp(retnotice->z_class, "message")) {
        snprintf(buff, BUFFLEN,
                 "Could not send message to %s: "
                 "not logged in or subscribing to class %s, instance %s.\n",
                 tmp,
                 retnotice->z_class,
                 retnotice->z_class_inst);
      } else if(strcasecmp(retnotice->z_class_inst, "personal")) {
        snprintf(buff, BUFFLEN,
                 "Could not send message to %s: "
                 "not logged in or subscribing to instance %s.\n",
                 tmp,
                 retnotice->z_class_inst);
      } else {
        snprintf(buff, BUFFLEN,
                 "Could not send message to %s: "
                 "not logged in or subscribing to messages.\n",
                 tmp);
      }
      owl_function_adminmsg("", buff);

      memset(&zw, 0, sizeof(zw));
      zw.class = owl_strdup(retnotice->z_class);
      zw.inst  = owl_strdup(retnotice->z_class_inst);
      realm = strchr(retnotice->z_recipient, '@');
      if(realm) {
        zw.realm = owl_strdup(realm + 1);
      } else {
        zw.realm = owl_strdup(owl_zephyr_get_realm());
      }
      zw.opcode = owl_strdup(retnotice->z_opcode);
      zw.zsig   = owl_strdup("");
      owl_list_create(&(zw.recips));
      owl_list_append_element(&(zw.recips), owl_strdup(tmp));

      owl_log_outgoing_zephyr_error(&zw, buff);

      owl_zwrite_cleanup(&zw);
      owl_free(tmp);
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
  owl_message *mout;
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
    to=owl_strdup(owl_message_get_sender(m));
  }

  send_zephyr("",
	      "Automated reply:",
	      owl_message_get_class(m),
	      owl_message_get_instance(m),
	      to,
	      owl_global_get_zaway_msg(&g));

  myuser=short_zuser(to);
  if (!strcasecmp(owl_message_get_instance(m), "personal")) {
    tmpbuff = owl_sprintf("zwrite %s", myuser);
  } else {
    tmpbuff = owl_sprintf("zwrite -i %s %s", owl_message_get_instance(m), myuser);
  }
  owl_free(myuser);
  owl_free(to);

  z = owl_zwrite_new(tmpbuff);
  owl_zwrite_set_message(z, owl_global_get_zaway_msg(&g));
  owl_zwrite_set_zsig(z, "Automated reply:");

  /* display the message as an admin message in the receive window */
  mout=owl_function_make_outgoing_zephyr(z);
  owl_global_messagequeue_addmsg(&g, mout);
  owl_free(tmpbuff);
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

char *owl_zephyr_zlocate(const char *user, int auth)
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
    return owl_sprintf("Error locating user %s: %s\n",
		       user, error_message(ret));

  myuser = short_zuser(user);
  if (numlocs == 0) {
    result = owl_sprintf("%s: Hidden or not logged in\n", myuser);
  } else {
    result = owl_strdup("");
    for (; numlocs; numlocs--) {
      ZGetLocations(&locations, &one);
      p = owl_sprintf("%s%s: %s\t%s\t%s\n",
			  result, myuser,
			  locations.host ? locations.host : "?",
			  locations.tty ? locations.tty : "?",
			  locations.time ? locations.time : "?");
      owl_free(result);
      result = p;
    }
  }
  owl_free(myuser);

  return result;
#else
  return owl_strdup("");
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
    owl_free(s);
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

  owl_free(line);
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
  owl_free(subsfile);
  owl_free(line);
#endif
}

/* caller must free the return */
char *owl_zephyr_makesubline(const char *class, const char *inst, const char *recip)
{
  return owl_sprintf("%s,%s,%s\n", class, inst, !strcmp(recip, "") ? "*" : recip);
}


void owl_zephyr_zlog_in(void)
{
#ifdef HAVE_LIBZEPHYR
  const char *exposure, *eset;
  int ret;

  ZResetAuthentication();
    
  eset=EXPOSE_REALMVIS;
  exposure=ZGetVariable(zstr("exposure"));
  if (exposure==NULL) {
    eset=EXPOSE_REALMVIS;
  } else if (!strcasecmp(exposure,EXPOSE_NONE)) {
    eset = EXPOSE_NONE;
  } else if (!strcasecmp(exposure,EXPOSE_OPSTAFF)) {
    eset = EXPOSE_OPSTAFF;
  } else if (!strcasecmp(exposure,EXPOSE_REALMVIS)) {
    eset = EXPOSE_REALMVIS;
  } else if (!strcasecmp(exposure,EXPOSE_REALMANN)) {
    eset = EXPOSE_REALMANN;
  } else if (!strcasecmp(exposure,EXPOSE_NETVIS)) {
    eset = EXPOSE_NETVIS;
  } else if (!strcasecmp(exposure,EXPOSE_NETANN)) {
    eset = EXPOSE_NETANN;
  }
   
  ret=ZSetLocation(zstr(eset));
  if (ret != ZERR_NONE) {
    /*
      owl_function_makemsg("Error setting location: %s", error_message(ret));
    */
  }
#endif
}

void owl_zephyr_zlog_out(void)
{
#ifdef HAVE_LIBZEPHYR
  int ret;

  ZResetAuthentication();
  ret=ZUnsetLocation();
  if (ret != ZERR_NONE) {
    /*
      owl_function_makemsg("Error unsetting location: %s", error_message(ret));
    */
  }
#endif
}

void owl_zephyr_addbuddy(const char *name)
{
  char *filename;
  FILE *file;
  
  filename = owl_zephyr_dotfile(".anyone", NULL);
  file = fopen(filename, "a");
  owl_free(filename);
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
  owl_free(filename);
}

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
char *owl_zephyr_getsubs(void)
{
#ifdef HAVE_LIBZEPHYR
  int ret, num, i, one;
  int buffsize;
  ZSubscription_t sub;
  char *out;
  one=1;

  ret=ZRetrieveSubscriptions(0, &num);
  if (ret==ZERR_TOOMANYSUBS) {
    return(owl_strdup("Zephyr: too many subscriptions\n"));
  } else if (ret || (num <= 0)) {
    return(owl_strdup("Zephyr: error retriving subscriptions\n"));
  }

  buffsize = (num + 1) * 50;
  out=owl_malloc(buffsize);
  strcpy(out, "");
  for (i=0; i<num; i++) {
    if ((ret = ZGetSubscriptions(&sub, &one)) != ZERR_NONE) {
      owl_free(out);
      ZFlushSubscriptions();
      out=owl_strdup("Error while getting subscriptions\n");
      return(out);
    } else {
      int tmpbufflen;
      char *tmpbuff;
      tmpbuff = owl_sprintf("<%s,%s,%s>\n%s", sub.zsub_class, sub.zsub_classinst, sub.zsub_recipient, out);
      tmpbufflen = strlen(tmpbuff) + 1;
      if (tmpbufflen > buffsize) {
        char *out2;
        buffsize = tmpbufflen * 2;
        out2 = owl_realloc(out, buffsize);
        if (out2 == NULL) {
          owl_free(out);
          owl_free(tmpbuff);
          ZFlushSubscriptions();
          out=owl_strdup("Realloc error while getting subscriptions\n");
          return(out);    
        }
        out = out2;
      }
      strcpy(out, tmpbuff);
      owl_free(tmpbuff);
    }
  }

  ZFlushSubscriptions();
  return(out);
#else
  return(owl_strdup("Zephyr not available"));
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
  
/* Strip a local realm fron the zephyr user name.
 * The caller must free the return
 */
char *short_zuser(const char *in)
{
  char *out, *ptr;

  out=owl_strdup(in);
  ptr=strchr(out, '@');
  if (ptr) {
    if (!strcasecmp(ptr+1, owl_zephyr_get_realm())) {
      *ptr='\0';
    }
  }
  return(out);
}

/* Append a local realm to the zephyr user name if necessary.
 * The caller must free the return.
 */
char *long_zuser(const char *in)
{
  if (strchr(in, '@')) {
    return(owl_strdup(in));
  }
  return(owl_sprintf("%s@%s", in, owl_zephyr_get_realm()));
}

/* strip out the instance from a zsender's principal.  Preserves the
 * realm if present.  Leave host/ and daemon/ krb5 principals
 * alone. Also leave rcmd. and daemon. krb4 principals alone. The
 * caller must free the return.
 */
char *owl_zephyr_smartstripped_user(const char *in)
{
  char *slash, *dot, *realm, *out;

  out = owl_strdup(in);

  /* bail immeaditly if we don't have to do any work */
  slash = strchr(out, '/');
  dot = strchr(out, '.');
  if (!slash && !dot) {
    return(out);
  }

  if (!strncasecmp(out, OWL_ZEPHYR_NOSTRIP_HOST, strlen(OWL_ZEPHYR_NOSTRIP_HOST)) ||
      !strncasecmp(out, OWL_ZEPHYR_NOSTRIP_RCMD, strlen(OWL_ZEPHYR_NOSTRIP_RCMD)) ||
      !strncasecmp(out, OWL_ZEPHYR_NOSTRIP_DAEMON5, strlen(OWL_ZEPHYR_NOSTRIP_DAEMON5)) ||
      !strncasecmp(out, OWL_ZEPHYR_NOSTRIP_DAEMON4, strlen(OWL_ZEPHYR_NOSTRIP_DAEMON4))) {
    return(out);
  }

  realm = strchr(out, '@');
  if (!slash && dot && realm && (dot > realm)) {
    /* There's no '/', and the first '.' is in the realm */
    return(out);
  }

  /* remove the realm from out, but hold on to it */
  if (realm) realm[0]='\0';

  /* strip */
  if (slash) slash[0] = '\0';  /* krb5 style user/instance */
  else if (dot) dot[0] = '\0'; /* krb4 style user.instance */

  /* reattach the realm if we had one */
  if (realm) {
    strcat(out, "@");
    strcat(out, realm+1);
  }

  return(out);
}

/* read the list of users in 'filename' as a .anyone file, and put the
 * names of the zephyr users in the list 'in'.  If 'filename' is NULL,
 * use the default .anyone file in the users home directory.  Returns
 * -1 on failure, 0 on success.
 */
int owl_zephyr_get_anyone_list(owl_list *in, const char *filename)
{
#ifdef HAVE_LIBZEPHYR
  char *ourfile, *tmp, *s = NULL;
  FILE *f;

  ourfile = owl_zephyr_dotfile(".anyone", filename);

  f = fopen(ourfile, "r");
  if (!f) {
    owl_function_error("Error opening file %s: %s", ourfile, strerror(errno) ? strerror(errno) : "");
    owl_free(ourfile);
    return -1;
  }
  owl_free(ourfile);

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

    owl_list_append_element(in, long_zuser(s));
  }
  owl_free(s);
  fclose(f);
  return 0;
#else
  return -1;
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
            m = owl_malloc(sizeof(owl_message));
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
          m = owl_malloc(sizeof(owl_message));
          owl_message_create_pseudo_zlogin(m, 1, zald->user, "", "", "");
          owl_global_messagequeue_addmsg(&g, m);
        }
        owl_zbuddylist_deluser(zbl, zald->user);
        owl_function_debugmsg("owl_function_zephyr_buddy_check: logout for %s ", zald->user);
      }
    }
    ZFreeALD(zald);
    owl_free(zald);
  }
}
#else
void owl_zephyr_process_pseudologin(void *n)
{
}
#endif

void owl_zephyr_buddycheck_timer(owl_timer *t, void *data)
{
  if (owl_global_is_pseudologins(&g)) {
    owl_function_debugmsg("Doing zephyr buddy check");
    owl_function_zephyr_buddy_check(1);
  } else {
    owl_function_debugmsg("Warning: owl_zephyr_buddycheck_timer call pointless; timer should have been disabled");
  }
}

/*
 * Process zephyrgrams from libzephyr's queue. To prevent starvation,
 * process a maximum of OWL_MAX_ZEPHYRGRAMS_TO_PROCESS.
 *
 * Returns the number of zephyrgrams processed.
 */

#define OWL_MAX_ZEPHYRGRAMS_TO_PROCESS 20

static int _owl_zephyr_process_events(void)
{
  int zpendcount=0;
#ifdef HAVE_LIBZEPHYR
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
      m=owl_malloc(sizeof(owl_message));
      owl_message_create_from_znotice(m, &notice);

      owl_global_messagequeue_addmsg(&g, m);
    }
  }
#endif
  return zpendcount;
}

void owl_zephyr_process_events(const owl_io_dispatch *d, void *data)
{
  _owl_zephyr_process_events();
}

int owl_zephyr_pre_select_action(owl_ps_action *a, void *p)
{
  return _owl_zephyr_process_events();
}
