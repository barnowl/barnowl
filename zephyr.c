#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <zephyr/zephyr.h>
#include <com_err.h>
#include <pwd.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";

int owl_zephyr_loadsubs(char *filename) {
  /* return 0  on success
   *        -1 on file error
   *        -2 on subscription error
   */
  FILE *file;
  char *tmp, *start;
  char buffer[1024], subsfile[1024];
  ZSubscription_t subs[3001];
  int count, ret, i;

  ret=0;
  
  if (filename==NULL) {
    sprintf(subsfile, "%s/%s", owl_global_get_homedir(&g), ".zephyr.subs");
  } else {
    strcpy(subsfile, filename);
  }
  
  /* need to redo this to do chunks, not just bail after 3000 */
  count=0;
  file=fopen(subsfile, "r");
  if (file) {
    while ( fgets(buffer, 1024, file)!=NULL ) {
      if (buffer[0]=='#' || buffer[0]=='\n' || buffer[0]=='\n') continue;

      if (buffer[0]=='-') {
	start=buffer+1;
      } else {
	start=buffer;
      }
      
      if (count >= 3000) break; /* also tell the user */

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
  } else {
    count=0;
    ret=-1;
  }

  /* sub with defaults */
  if (ZSubscribeTo(subs,count,0) != ZERR_NONE) {
    fprintf(stderr, "Error subbing\n");
    ret=-2;
  }

  /* free stuff */
  for (i=0; i<count; i++) {
    owl_free(subs[i].zsub_class);
    owl_free(subs[i].zsub_classinst);
    owl_free(subs[i].zsub_recipient);
  }

  return(ret);
}

int loadloginsubs(char *filename) {
  FILE *file;
  ZSubscription_t subs[3001];
  char subsfile[1024], buffer[1024];
  int count, ret, i;

  ret=0;

  if (filename==NULL) {
    sprintf(subsfile, "%s/%s", owl_global_get_homedir(&g), ".anyone");
  } else {
    strcpy(subsfile, filename);
  }

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
	subs[count].zsub_classinst=owl_malloc(1024);
	sprintf(subs[count].zsub_classinst, "%s@%s", buffer, ZGetRealm());
      }

      count++;
    }
    fclose(file);
  } else {
    count=0;
    ret=-1;
  }

  /* sub with defaults */
  if (ZSubscribeTo(subs,count,0) != ZERR_NONE) {
    fprintf(stderr, "Error subbing\n");
    ret=-2;
  }

  /* free stuff */
  for (i=0; i<count; i++) {
    owl_free(subs[i].zsub_classinst);
  }

  return(ret);
}

void unsuball() {
  int ret;
  
  ret=ZCancelSubscriptions(0);
  if (ret != ZERR_NONE) {
    com_err("owl",ret,"while unsubscribing");
  }
}

int owl_zephyr_sub(char *class, char *inst, char *recip) {
  ZSubscription_t subs[5];
  int ret;

  subs[0].zsub_class=class;
  subs[0].zsub_classinst=inst;
  subs[0].zsub_recipient=recip;

  if (ZSubscribeTo(subs,1,0) != ZERR_NONE) {
    fprintf(stderr, "Error subbing\n");
    ret=-2;
  }
  return(0);
}


int owl_zephyr_unsub(char *class, char *inst, char *recip) {
  ZSubscription_t subs[5];
  int ret;

  subs[0].zsub_class=class;
  subs[0].zsub_classinst=inst;
  subs[0].zsub_recipient=recip;

  if (ZUnsubscribeTo(subs,1,0) != ZERR_NONE) {
    fprintf(stderr, "Error unsubbing\n");
    ret=-2;
  }
  return(0);
}


void zlog_in() {
  int ret;

  ret=ZSetLocation(EXPOSE_NETVIS);
  /* do something on failure */
}

void zlog_out() {
  int ret;

  ret=ZUnsetLocation();
  /* do something on failure */
}


char *owl_zephyr_get_field(ZNotice_t *n, int j, int *k) {
  /* return a pointer to the Jth field, place the length in k.  If the
     field doesn't exist return an emtpy string */
  int i, count, save;

  count=save=0;
  for (i=0; i<n->z_message_len; i++) {
    if (n->z_message[i]=='\0') {
      count++;
      if (count==j) {
	/* just found the end of the field we're looking for */
	*k=i-save;
	return(n->z_message+save);
      } else {
	save=i+1;
      }
    }
  }
  /* catch the last field */
  if (count==j-1) {
    *k=n->z_message_len-save;
    return(n->z_message+save);
  }
  
  *k=0;
  return("");
}


int owl_zephyr_get_num_fields(ZNotice_t *n) {
  int i, fields;

  fields=1;
  for (i=0; i<n->z_message_len; i++) {
    if (n->z_message[i]=='\0') fields++;
  }
  
  return(fields);
}


char *owl_zephyr_get_message(ZNotice_t *n, int *k) {
  /* return a pointer to the message, place the message length in k */
  if (!strcasecmp(n->z_opcode, "ping")) {
    *k=0;
    return("");
  }

  return(owl_zephyr_get_field(n, 2, k));
}


char *owl_zephyr_get_zsig(ZNotice_t *n, int *k) {
  /* return a pointer to the zsig if there is one */

  if (n->z_message_len==0) {
    *k=0;
    return("");
  }
  *k=strlen(n->z_message);
  return(n->z_message);
}


int send_zephyr(char *opcode, char *zsig, char *class, char *instance, char *recipient, char *message) {
  int ret;
  ZNotice_t notice;
  char *ptr;
  struct passwd *pw;
  char zsigtmp[LINE];
  char *zsigexec, *zsigowlvar, *zsigzvar;
  
  zsigexec = owl_global_get_zsig_exec(&g);
  zsigowlvar = owl_global_get_zsig(&g);
  zsigzvar = ZGetVariable("zwrite-signature");

  if (zsig) {
    strcpy(zsigtmp, zsig);
  } else if (zsigowlvar && *zsigowlvar) {
    strncpy(zsigtmp, zsigowlvar, LINE);
  } else if (zsigexec && *zsigexec) {
    FILE *file;
    char buff[LINE];
    strcpy(zsigtmp, "");
    file=popen(zsigexec, "r");
    if (!file) {
      if (zsigzvar && *zsigzvar) {
	strncpy(zsigtmp, zsigzvar, LINE);
      }
    } else {
      while (fgets(buff, LINE, file)) { /* wrong sizing */
	strcat(zsigtmp, buff);
      }
      pclose(file);
      if (zsigtmp[strlen(zsigtmp)-1]=='\n') {
	zsigtmp[strlen(zsigtmp)-1]='\0';
      }
    }
  } else if (zsigzvar) {
    strncpy(zsigtmp, zsigzvar, LINE);
  } else if (((pw=getpwuid(getuid()))!=NULL) && (pw->pw_gecos)) {
    strncpy(zsigtmp, pw->pw_gecos, LINE);
    ptr=strchr(zsigtmp, ',');
    if (ptr) {
      ptr[0]='\0';
    }
  } else {
    strcpy(zsigtmp, "");
  }
    
  memset(&notice, 0, sizeof(notice));

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

  notice.z_message_len=strlen(zsigtmp)+1+strlen(message);
  notice.z_message=owl_malloc(notice.z_message_len+10);
  strcpy(notice.z_message, zsigtmp);
  memcpy(notice.z_message+strlen(zsigtmp)+1, message, strlen(message));

  /* ret=ZSendNotice(&notice, ZAUTH); */
  ret=ZSrvSendNotice(&notice, ZAUTH, send_zephyr_helper);
  
  /* free then check the return */
  owl_free(notice.z_message);
  ZFreeNotice(&notice);
  if (ret!=ZERR_NONE) {
    owl_function_makemsg("Error sending zephyr");
    return(ret);
  }

  return(0);
}

Code_t send_zephyr_helper(ZNotice_t *notice, char *buf, int len, int wait) {
  return(ZSendPacket(buf, len, 0));
}

void send_ping(char *to) {
  send_zephyr("PING", "", "MESSAGE", "PERSONAL", to, "");
}

void owl_zephyr_handle_ack(ZNotice_t *retnotice) {
  char buff[LINE];
  char *tmp;
  
  /* if it's an HMACK ignore it */
  if (retnotice->z_kind == HMACK) return;
  
  if (retnotice->z_kind == SERVNAK) {
    owl_function_makemsg("Authorization failure sending zephyr");
  } else if ((retnotice->z_kind != SERVACK) || !retnotice->z_message_len) {
    owl_function_makemsg("Detected server failure while receiving acknowledgement");
  } else if (!strcmp(retnotice->z_message, ZSRVACK_SENT)) {
    if (!strcasecmp(retnotice->z_opcode, "ping")) {
      return;
    } else if (!strcasecmp(retnotice->z_class, "message") &&
	       !strcasecmp(retnotice->z_class_inst, "personal")) {
      tmp=pretty_sender(retnotice->z_recipient);
      sprintf(buff, "Message sent to %s.", tmp);
      free(tmp);
    } else {
      sprintf(buff, "Message sent to -c %s -i %s\n", retnotice->z_class, retnotice->z_class_inst);
    }
    owl_function_makemsg(buff);
  } else if (!strcmp(retnotice->z_message, ZSRVACK_NOTSENT)) {
    if (strcasecmp(retnotice->z_class, "message")) {
      sprintf(buff, "Not logged in or not subscribing to class %s, instance %s",
	      retnotice->z_class, retnotice->z_class_inst);
      owl_function_makemsg(buff);
    } else {
      owl_function_makemsg("Not logged in or subscribing to messages.");
    }
  } else {
    char buff[1024];
    sprintf(buff, "Internal error on ack (%s)", retnotice->z_message);
    owl_function_makemsg(buff);
  }
}

int owl_zephyr_notice_is_ack(ZNotice_t *n) {
  if (n->z_kind == SERVNAK || n->z_kind == SERVACK || n->z_kind == HMACK) {
    if (!strcasecmp(n->z_class, LOGIN_CLASS)) return(0);
    return(1);
  }
  return(0);
}
  
void owl_zephyr_zaway(owl_message *m) {
  char *tmpbuff;
  
  /* bail if it doesn't look like a message we should reply to */
  if (strcasecmp(owl_message_get_class(m), "message")) return;
  if (strcasecmp(owl_message_get_recipient(m), ZGetSender())) return;
  if (!strcasecmp(owl_message_get_sender(m), "")) return;
  if (!strcasecmp(owl_message_get_opcode(m), "ping")) return;
  if (!strcasecmp(owl_message_get_opcode(m), "auto")) return;
  if (!strcasecmp(owl_message_get_notice(m)->z_message, "Automated reply:")) return;
  if (!strcasecmp(owl_message_get_sender(m), ZGetSender())) return;
  /* add one to not send to ourselves */

  send_zephyr("",
	      "Automated reply:",
	      owl_message_get_class(m),
	      owl_message_get_instance(m),
	      owl_message_get_sender(m),
	      owl_global_get_zaway_msg(&g));

  /* display the message as an admin message in the receive window */
  tmpbuff=owl_malloc(strlen(owl_global_get_zaway_msg(&g))+LINE);
  sprintf(tmpbuff, "Message sent to %s", owl_message_get_sender(m));
  owl_function_adminmsg(tmpbuff, owl_global_get_zaway_msg(&g));
  owl_free(tmpbuff);
}


void owl_zephyr_hackaway_cr(ZNotice_t *n) {
  /* replace \r's with ' '.  Gross-ish */
  int i;

  for (i=0; i<n->z_message_len; i++) {
    if (n->z_message[i]=='\r') {
      n->z_message[i]=' ';
    }
  }
}

void owl_zephyr_zlocate(char *user, char *out, int auth) {
  int ret, numlocs;
  int one = 1;
  ZLocations_t locations;
  char *myuser;
  
  strcpy(out, "");

  ret=ZLocateUser(user,&numlocs,auth?ZAUTH:ZNOAUTH);
  if (ret != ZERR_NONE) {
    owl_function_makemsg("Error locating user");
  }

  if (numlocs==0) {
    strcpy(out, "Hidden or not logged-in\n");
    return;
  }
    
  for (;numlocs;numlocs--) {
    ZGetLocations(&locations,&one);
    myuser=pretty_sender(user);
    sprintf(out, "%s%s: %s\t%s\t%s\n", out, myuser, locations.host, locations.tty, locations.time);
    owl_free(myuser);
  }
}
