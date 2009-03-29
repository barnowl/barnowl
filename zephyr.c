#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";

#ifdef HAVE_LIBZEPHYR
Code_t ZResetAuthentication();
#endif

int owl_zephyr_initialize()
{
#ifdef HAVE_LIBZEPHYR
  int ret;
  
  if ((ret = ZInitialize()) != ZERR_NONE) {
    com_err("owl",ret,"while initializing");
    return(1);
  }
  if ((ret = ZOpenPort(NULL)) != ZERR_NONE) {
    com_err("owl",ret,"while opening port");
    return(1);
  }
  owl_zephyr_process_events(NULL);
#endif
  return(0);
}


int owl_zephyr_shutdown()
{
#ifdef HAVE_LIBZEPHYR
  unsuball();
  owl_zephyr_process_events(NULL);
  ZClosePort();
#endif
  return(0);
}

int owl_zephyr_zpending()
{
#ifdef HAVE_LIBZEPHYR
  return(ZPending());
#else
  return(0);
#endif
}

char *owl_zephyr_get_realm()
{
#ifdef HAVE_LIBZEPHYR
  return(ZGetRealm());
#else
  return("");
#endif
}

char *owl_zephyr_get_sender()
{
#ifdef HAVE_LIBZEPHYR
  return(ZGetSender());
#else
  return("");
#endif
}

#ifdef HAVE_LIBZEPHYR
int owl_zephyr_loadsubs_helper(ZSubscription_t subs[], int count)
{
  int i, ret = 0;
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
  owl_zephyr_process_events(NULL);
  return ret;
}
#endif

/* Load zephyr subscriptions form 'filename'.  If 'filename' is NULL,
 * the default file $HOME/.zephyr.subs will be used.
 *
 * Returns 0 on success.  If the file does not exist, return -1 if
 * 'error_on_nofile' is 1, otherwise return 0.  Return -1 if the file
 * exists but can not be read.  Return -2 if there is a failure from
 * zephyr to load the subscriptions.
 */
int owl_zephyr_loadsubs(char *filename, int error_on_nofile)
{
#ifdef HAVE_LIBZEPHYR
  FILE *file;
  char *tmp, *start;
  char buffer[1024], subsfile[1024];
  ZSubscription_t subs[3001];
  int count, ret;
  struct stat statbuff;

  if (filename==NULL) {
    sprintf(subsfile, "%s/%s", owl_global_get_homedir(&g), ".zephyr.subs");
  } else {
    strcpy(subsfile, filename);
  }

  ret=stat(subsfile, &statbuff);
  if (ret) {
    if (error_on_nofile==1) return(-1);
    return(0);
  }

  ZResetAuthentication();
  count=0;
  file=fopen(subsfile, "r");
  if (!file) return(-1);
  while ( fgets(buffer, 1024, file)!=NULL ) {
    if (buffer[0]=='#' || buffer[0]=='\n' || buffer[0]=='\n') continue;
    
    if (buffer[0]=='-') {
      start=buffer+1;
    } else {
      start=buffer;
    }
    
    if (count >= 3000) {
      ret = owl_zephyr_loadsubs_helper(subs, count);
      if (ret != 0) {
	fclose(file);
	return(ret);
      }
      count=0;
    }
    
    /* add it to the list of subs */
    if ((tmp=(char *) strtok(start, ",\n\r"))==NULL) continue;
    subs[count].zsub_class=owl_strdup(tmp);
    if ((tmp=(char *) strtok(NULL, ",\n\r"))==NULL) continue;
    subs[count].zsub_classinst=owl_strdup(tmp);
    if ((tmp=(char *) strtok(NULL, " \t\n\r"))==NULL) continue;
    subs[count].zsub_recipient=owl_strdup(tmp);
    
    /* if it started with '-' then add it to the global punt list */
    if (buffer[0]=='-') {
      owl_function_zpunt(subs[count].zsub_class, subs[count].zsub_classinst, subs[count].zsub_recipient, 0);
    }
    
    count++;
  }
  fclose(file);

  ret=owl_zephyr_loadsubs_helper(subs, count);

  return(ret);
#else
  return(0);
#endif
}

int owl_zephyr_loaddefaultsubs()
{
#ifdef HAVE_LIBZEPHYR
  ZSubscription_t subs[10];
    
  if (ZSubscribeTo(subs,0,0) != ZERR_NONE) {
    owl_function_error("Error subscribing to default zephyr notifications.");
    return(-1);
  }
  owl_zephyr_process_events(NULL);
  return(0);
#else
  return(0);
#endif
}

int owl_zephyr_loadloginsubs(char *filename)
{
#ifdef HAVE_LIBZEPHYR
  FILE *file;
  ZSubscription_t subs[3001];
  char subsfile[1024], buffer[1024];
  int count, ret, i;
  struct stat statbuff;

  if (filename==NULL) {
    sprintf(subsfile, "%s/%s", owl_global_get_homedir(&g), ".anyone");
  } else {
    strcpy(subsfile, filename);
  }
  
  ret=stat(subsfile, &statbuff);
  if (ret) return(0);

  ret=0;

  ZResetAuthentication();
  /* need to redo this to do chunks, not just bag out after 3000 */
  count=0;
  file=fopen(subsfile, "r");
  if (file) {
    while ( fgets(buffer, 1024, file)!=NULL ) {
      if (buffer[0]=='#' || buffer[0]=='\n' || buffer[0]=='\n') continue;
      
      if (count >= 3000) break; /* also tell the user */

      buffer[strlen(buffer)-1]='\0';
      subs[count].zsub_class="login";
      subs[count].zsub_recipient="*";
      if (strchr(buffer, '@')) {
	subs[count].zsub_classinst=owl_strdup(buffer);
      } else {
	subs[count].zsub_classinst=owl_sprintf("%s@%s", buffer, ZGetRealm());
      }

      count++;
    }
    fclose(file);
  } else {
    count=0;
    ret=-1;
  }

  /* sub with defaults */
  if (ZSubscribeToSansDefaults(subs,count,0) != ZERR_NONE) {
    owl_function_error("Error subscribing to zephyr notifications.");
    ret=-2;
  }

  /* free stuff */
  for (i=0; i<count; i++) {
    owl_free(subs[i].zsub_classinst);
  }

  owl_zephyr_process_events(NULL);

  return(ret);
#else
  return(0);
#endif
}

void unsuball()
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

int owl_zephyr_sub(char *class, char *inst, char *recip)
{
#ifdef HAVE_LIBZEPHYR
  ZSubscription_t subs[5];

  subs[0].zsub_class=class;
  subs[0].zsub_classinst=inst;
  subs[0].zsub_recipient=recip;

  ZResetAuthentication();
  if (ZSubscribeTo(subs,1,0) != ZERR_NONE) {
    owl_function_error("Error subbing to <%s,%s,%s>", class, inst, recip);
    return(-2);
  }
  owl_zephyr_process_events(NULL);
  return(0);
#else
  return(0);
#endif
}


int owl_zephyr_unsub(char *class, char *inst, char *recip)
{
#ifdef HAVE_LIBZEPHYR
  ZSubscription_t subs[5];

  subs[0].zsub_class=class;
  subs[0].zsub_classinst=inst;
  subs[0].zsub_recipient=recip;

  ZResetAuthentication();
  if (ZUnsubscribeTo(subs,1,0) != ZERR_NONE) {
    owl_function_error("Error unsubbing from <%s,%s,%s>", class, inst, recip);
    return(-2);
  }
  owl_zephyr_process_events(NULL);
  return(0);
#else
  return(0);
#endif
}

/* return a pointer to the data in the Jth field, (NULL terminated by
 * definition).  Caller must free the return.
 */
#ifdef HAVE_LIBZEPHYR
char *owl_zephyr_get_field(ZNotice_t *n, int j)
{
  int i, count, save;
  char *out;

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
    out=owl_malloc(n->z_message_len-save+5);
    memcpy(out, n->z_message+save, n->z_message_len-save);
    out[n->z_message_len-save]='\0';
    return(out);
  }

  return(owl_strdup(""));
}
#else
char *owl_zephyr_get_field(void *n, int j)
{
  return(owl_strdup(""));
}
#endif


#ifdef HAVE_LIBZEPHYR
int owl_zephyr_get_num_fields(ZNotice_t *n)
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
int owl_zephyr_get_num_fields(void *n)
{
  return(0);
}
#endif

#ifdef HAVE_LIBZEPHYR
/* return a pointer to the message, place the message length in k
 * caller must free the return
 */
char *owl_zephyr_get_message(ZNotice_t *n)
{
  /* don't let ping messages have a body */
  if (!strcasecmp(n->z_opcode, "ping")) {
    return(owl_strdup(""));
  }

  /* deal with MIT Athena OLC messages */
  if (!strcasecmp(n->z_sender, "olc.matisse@ATHENA.MIT.EDU")) {
    return(owl_zephyr_get_field(n, 1));
  }
  /* deal with MIT NOC messages */
  else if (!strcasecmp(n->z_sender, "rcmd.achilles@ATHENA.MIT.EDU")) {
    /* $opcode service on $instance $3.\n$4 */
    char *msg, *opcode, *instance, *field3, *field4;

    opcode = n->z_opcode;
    instance = n->z_class_inst;
    field3 = owl_zephyr_get_field(n, 3);
    field4 = owl_zephyr_get_field(n, 4);

    msg = owl_sprintf("%s service on %s %s\n%s", opcode, instance, field3, field4);
    if (msg) {
      return msg;
    }
  }

  return(owl_zephyr_get_field(n, 2));
}
#endif

#ifdef HAVE_LIBZEPHYR
char *owl_zephyr_get_zsig(ZNotice_t *n, int *k)
{
  /* return a pointer to the zsig if there is one */

  /* message length 0? No zsig */
  if (n->z_message_len==0) {
    *k=0;
    return("");
  }

  /* No zsig for OLC messages */
  if (!strcasecmp(n->z_sender, "olc.matisse@ATHENA.MIT.EDU")) {
    return("");
  }

  /* Everything else is field 1 */
  *k=strlen(n->z_message);
  return(n->z_message);
}
#else
char *owl_zephyr_get_zsig(void *n, int *k)
{
  return("");
}
#endif

int send_zephyr(char *opcode, char *zsig, char *class, char *instance, char *recipient, char *message)
{
#ifdef HAVE_LIBZEPHYR
  int ret;
  ZNotice_t notice;
    
  memset(&notice, 0, sizeof(notice));

  ZResetAuthentication();

  if (!zsig) zsig="";
  
  notice.z_kind=ACKED;
  notice.z_port=0;
  notice.z_class=class;
  notice.z_class_inst=instance;
  if (!strcmp(recipient, "*") || !strcmp(recipient, "@")) {
    notice.z_recipient="";
  } else {
    notice.z_recipient=recipient;
  }
  notice.z_default_format="Class $class, Instance $instance:\nTo: @bold($recipient) at $time $date\nFrom: @bold{$1 <$sender>}\n\n$2";
  notice.z_sender=NULL;
  if (opcode) notice.z_opcode=opcode;

  notice.z_message_len=strlen(zsig)+1+strlen(message);
  notice.z_message=owl_malloc(notice.z_message_len+10);
  strcpy(notice.z_message, zsig);
  memcpy(notice.z_message+strlen(zsig)+1, message, strlen(message));

  /* ret=ZSendNotice(&notice, ZAUTH); */
  ret=ZSrvSendNotice(&notice, ZAUTH, send_zephyr_helper);
  
  /* free then check the return */
  owl_free(notice.z_message);
  ZFreeNotice(&notice);
  owl_zephyr_process_events(NULL);
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
  int ret;
  ret=ZSendPacket(buf, len, 0);
  owl_zephyr_process_events(NULL);
  return(ret);
}
#endif

void send_ping(char *to)
{
#ifdef HAVE_LIBZEPHYR
  send_zephyr("PING", "", "MESSAGE", "PERSONAL", to, "");
  owl_zephyr_process_events(NULL);
#endif
}

#ifdef HAVE_LIBZEPHYR
void owl_zephyr_handle_ack(ZNotice_t *retnotice)
{
  char *tmp, *buff;
  
  /* if it's an HMACK ignore it */
  if (retnotice->z_kind == HMACK) return;

  if (retnotice->z_kind == SERVNAK) {
    owl_function_error("Authorization failure sending zephyr");
  } else if ((retnotice->z_kind != SERVACK) || !retnotice->z_message_len) {
    owl_function_error("Detected server failure while receiving acknowledgement");
  } else if (!strcmp(retnotice->z_message, ZSRVACK_SENT)) {
    if (!strcasecmp(retnotice->z_opcode, "ping")) {
      return;
    } else if (!strcasecmp(retnotice->z_class, "message") &&
	       !strcasecmp(retnotice->z_class_inst, "personal")) {
      tmp=short_zuser(retnotice->z_recipient);
      owl_function_makemsg("Message sent to %s.", tmp);
      free(tmp);
    } else {
      owl_function_makemsg("Message sent to -c %s -i %s\n", retnotice->z_class, retnotice->z_class_inst);
    }
  } else if (!strcmp(retnotice->z_message, ZSRVACK_NOTSENT)) {
    if (strcasecmp(retnotice->z_class, "message")) {
      owl_function_error("No one subscribed to class class %s", retnotice->z_class);
      buff=owl_sprintf("Could not send message to class %s: no one subscribed.\n",
		       retnotice->z_class);
      owl_function_adminmsg("", buff);
      owl_free(buff);
    } else {
      tmp = short_zuser(retnotice->z_recipient);
      owl_function_error("%s: Not logged in or subscribing to messages.", tmp);
      buff=owl_sprintf("Could not send message to %s: not logged in or subscribing to messages.\n",
		       tmp);
      owl_function_adminmsg("", buff);
      owl_log_outgoing_zephyr_error(tmp, buff);
      owl_free(tmp);
      owl_free(buff);
    }
  } else {
    owl_function_error("Internal error on ack (%s)", retnotice->z_message);
  }
  owl_zephyr_process_events(NULL);
}
#else
void owl_zephyr_handle_ack(void *retnotice)
{
}
#endif

#ifdef HAVE_LIBZEPHYR
int owl_zephyr_notice_is_ack(ZNotice_t *n)
{
  if (n->z_kind == SERVNAK || n->z_kind == SERVACK || n->z_kind == HMACK) {
    if (!strcasecmp(n->z_class, LOGIN_CLASS)) return(0);
    return(1);
  }
  return(0);
}
#else
int owl_zephyr_notice_is_ack(void *n)
{
  return(0);
}
#endif
  
void owl_zephyr_zaway(owl_message *m)
{
#ifdef HAVE_LIBZEPHYR
  char *tmpbuff, *myuser, *to;
  owl_message *mout;
  
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

  /* display the message as an admin message in the receive window */
  mout=owl_function_make_outgoing_zephyr(owl_global_get_zaway_msg(&g), tmpbuff, "Automated reply:");
  owl_function_add_message(mout);
  owl_free(tmpbuff);
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

char *owl_zephyr_zlocate(char *user, int auth)
{
#ifdef HAVE_LIBZEPHYR
  int ret, numlocs;
  int one = 1;
  ZLocations_t locations;
  char *myuser, *out, *tmp;
  
  ZResetAuthentication();
  ret=ZLocateUser(user,&numlocs,auth?ZAUTH:ZNOAUTH);
  owl_zephyr_process_events(NULL);
  if (ret != ZERR_NONE) {
    return(owl_sprintf("Error locating user %s\n", user));
  }

  if (numlocs==0) {
    myuser=short_zuser(user);
    out=owl_sprintf("%s: Hidden or not logged in\n", myuser);
    owl_free(myuser);
    return(out);
  }

  out=strdup("");
  for (;numlocs;numlocs--) {
    ZGetLocations(&locations,&one);
    myuser=short_zuser(user);
    tmp=owl_sprintf("%s%s: %s\t%s\t%s\n",
		    out,
		    myuser,
		    locations.host ? locations.host : "?",
		    locations.tty ? locations.tty : "?",
		    locations.time ? locations.time : "?");
    owl_free(out);
    out=tmp;
    owl_free(myuser);
  }
  owl_zephyr_process_events(NULL);
  return(out);
#else
  return(owl_strdup("Zephyr not available"));
#endif
}

void owl_zephyr_addsub(char *filename, char *class, char *inst, char *recip)
{
#ifdef HAVE_LIBZEPHYR
  char *line, subsfile[LINE], buff[LINE];
  FILE *file;

  line=owl_zephyr_makesubline(class, inst, recip);

  if (filename==NULL) {
    sprintf(subsfile, "%s/%s", owl_global_get_homedir(&g), ".zephyr.subs");
  } else {
    strcpy(subsfile, filename);
  }

  /* if the file already exists, check to see if the sub is already there */
  file=fopen(subsfile, "r");
  if (file) {
    while (fgets(buff, LINE, file)!=NULL) {
      if (!strcasecmp(buff, line)) {
	owl_function_error("Subscription already present in %s", subsfile);
	owl_free(line);
	fclose(file);
	return;
      }
    }
    fclose(file);
  }

  /* if we get here then we didn't find it */
  file=fopen(subsfile, "a");
  if (!file) {
    owl_function_error("Error opening file %s for writing", subsfile);
    owl_free(line);
    return;
  }
  fputs(line, file);
  fclose(file);
  owl_function_makemsg("Subscription added");
  
  owl_free(line);
#endif
}

void owl_zephyr_delsub(char *filename, char *class, char *inst, char *recip)
{
#ifdef HAVE_LIBZEPHYR
  char *line, *subsfile;
  
  line=owl_zephyr_makesubline(class, inst, recip);
  line[strlen(line)-1]='\0';

  if (!filename) {
    subsfile=owl_sprintf("%s/.zephyr.subs", owl_global_get_homedir(&g));
  } else {
    subsfile=owl_strdup(filename);
  }
  
  owl_util_file_deleteline(subsfile, line, 1);
  owl_free(subsfile);
  owl_free(line);
  owl_function_makemsg("Subscription removed");
#endif
}

/* caller must free the return */
char *owl_zephyr_makesubline(char *class, char *inst, char *recip)
{
  return(owl_sprintf("%s,%s,%s\n", class, inst, !strcmp(recip, "") ? "*" : recip));
}

void owl_zephyr_zlog_in(void)
{
#ifdef HAVE_LIBZEPHYR
  char *exposure, *eset;
  int ret;

  ZResetAuthentication();
    
  eset=EXPOSE_REALMVIS;
  exposure=ZGetVariable("exposure");
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
   
  ret=ZSetLocation(eset);
  owl_zephyr_process_events(NULL);
  if (ret != ZERR_NONE) {
    /* owl_function_makemsg("Error setting location: %s", error_message(ret)); */
  }
#endif
}

void owl_zephyr_zlog_out(void)
{
#ifdef HAVE_LIBZEPHYR
  int ret;

  ZResetAuthentication();
  ret=ZUnsetLocation();
  owl_zephyr_process_events(NULL);
  if (ret != ZERR_NONE) {
    /* owl_function_makemsg("Error unsetting location: %s", error_message(ret)); */
  }
#endif
}

void owl_zephyr_addbuddy(char *name)
{
  char *filename;
  FILE *file;
  
  filename=owl_sprintf("%s/.anyone", owl_global_get_homedir(&g));
  file=fopen(filename, "a");
  owl_free(filename);
  if (!file) {
    owl_function_error("Error opening zephyr buddy file for append");
    return;
  }
  fprintf(file, "%s\n", name);
  fclose(file);
}

void owl_zephyr_delbuddy(char *name)
{
  char *filename;

  filename=owl_sprintf("%s/.anyone", owl_global_get_homedir(&g));
  owl_util_file_deleteline(filename, name, 0);
  owl_free(filename);
}

/* return auth string */
#ifdef HAVE_LIBZEPHYR
char *owl_zephyr_get_authstr(ZNotice_t *n)
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
char *owl_zephyr_get_authstr(void *n)
{
  return("");
}
#endif

/* Returns a buffer of subscriptions or an error message.  Caller must
 * free the return.
 */
char *owl_zephyr_getsubs()
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
      owl_zephyr_process_events(NULL);
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
  owl_zephyr_process_events(NULL);
  return(out);
#else
  return(owl_strdup("Zephyr not available"));
#endif
}

char *owl_zephyr_get_variable(char *var)
{
#ifdef HAVE_LIBZEPHYR
  return(ZGetVariable(var));
#else
  return("");
#endif
}

void owl_zephyr_set_locationinfo(char *host, char *val)
{
#ifdef HAVE_LIBZEPHYR
  ZInitLocationInfo(host, val);
  owl_zephyr_process_events(NULL);
#endif
}
  
/* Strip a local realm fron the zephyr user name.
 * The caller must free the return
 */
char *short_zuser(char *in)
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
char *long_zuser(char *in)
{
  if (strchr(in, '@')) {
    return(owl_strdup(in));
  }
  return(owl_sprintf("%s@%s", in, owl_zephyr_get_realm()));
}

/* strip out the instance from a zsender's principal.  Preserves the
 * realm if present.  daemon.webzephyr is a special case.  The
 * caller must free the return
 */
char *owl_zephyr_smartstripped_user(char *in)
{
  char *ptr, *realm, *out;

  out=owl_strdup(in);

  /* bail immeaditly if we don't have to do any work */
  ptr=strchr(in, '.');
  if (!strchr(in, '/') && !ptr) {
    /* no '/' and no '.' */
    return(out);
  }
  if (ptr && strchr(in, '@') && (ptr > strchr(in, '@'))) {
    /* There's a '.' but it's in the realm */
    return(out);
  }
  if (!strncasecmp(in, OWL_WEBZEPHYR_PRINCIPAL, strlen(OWL_WEBZEPHYR_PRINCIPAL))) {
    return(out);
  }

  /* remove the realm from ptr, but hold on to it */
  realm=strchr(out, '@');
  if (realm) realm[0]='\0';

  /* strip */
  ptr=strchr(out, '.');
  if (!ptr) ptr=strchr(out, '/');
  ptr[0]='\0';

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
int owl_zephyr_get_anyone_list(owl_list *in, char *filename)
{
#ifdef HAVE_LIBZEPHYR
  char *ourfile, *tmp, buff[LINE];
  FILE *f;

  if (filename==NULL) {
    tmp=owl_global_get_homedir(&g);
    ourfile=owl_sprintf("%s/.anyone", owl_global_get_homedir(&g));
  } else {
    ourfile=owl_strdup(filename);
  }
  
  f=fopen(ourfile, "r");
  if (!f) {
    owl_function_error("Error opening file %s: %s", ourfile, strerror(errno) ? strerror(errno) : "");
    owl_free(ourfile);
    return(-1);
  }

  while (fgets(buff, LINE, f)!=NULL) {
    /* ignore comments, blank lines etc. */
    if (buff[0]=='#') continue;
    if (buff[0]=='\n') continue;
    if (buff[0]=='\0') continue;
    
    /* strip the \n */
    buff[strlen(buff)-1]='\0';
    
    /* ingore from # on */
    tmp=strchr(buff, '#');
    if (tmp) tmp[0]='\0';
    
    /* ingore from SPC */
    tmp=strchr(buff, ' ');
    if (tmp) tmp[0]='\0';
    
    /* stick on the local realm. */
    if (!strchr(buff, '@')) {
      strcat(buff, "@");
      strcat(buff, ZGetRealm());
    }
    owl_list_append_element(in, owl_strdup(buff));
  }
  fclose(f);
  owl_free(ourfile);
  return(0);
#else
  return(-1);
#endif
}


#ifdef HAVE_LIBZEPHYR
void owl_zephyr_process_events(owl_dispatch *d) {
  int zpendcount=0;
  ZNotice_t notice;
  struct sockaddr_in from;
  owl_message *m=NULL;

  while(owl_zephyr_zpending() && zpendcount < 20) {
    if (owl_zephyr_zpending()) {
      ZReceiveNotice(&notice, &from);
      zpendcount++;

      /* is this an ack from a zephyr we sent? */
      if (owl_zephyr_notice_is_ack(&notice)) {
        owl_zephyr_handle_ack(&notice);
        continue;
      }

      /* if it's a ping and we're not viewing pings then skip it */
      if (!owl_global_is_rxping(&g) && !strcasecmp(notice.z_opcode, "ping")) {
        continue;
      }

      /* create the new message */
      m=owl_malloc(sizeof(owl_message));
      owl_message_create_from_znotice(m, &notice);

      owl_global_messagequeue_addmsg(&g, m);
    }
  }
}

#else
void owl_zephyr_process_events(owl_dispatch *d) {
  
}
#endif
