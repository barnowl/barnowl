#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <zephyr/zephyr.h>
#include <com_err.h>
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
      tmp=short_zuser(retnotice->z_recipient);
      owl_function_makemsg("Message sent to %s.", tmp);
      free(tmp);
    } else {
      owl_function_makemsg("Message sent to -c %s -i %s\n", retnotice->z_class, retnotice->z_class_inst);
    }
  } else if (!strcmp(retnotice->z_message, ZSRVACK_NOTSENT)) {
    if (strcasecmp(retnotice->z_class, "message")) {
      owl_function_makemsg("Not logged in or not subscribing to class %s, instance %s",
	      retnotice->z_class, retnotice->z_class_inst);
    } else {
      tmp = short_zuser(retnotice->z_recipient);
      owl_function_makemsg("%s: Not logged in or subscribing to messages.", 
			   tmp);
      owl_free(tmp);
    }
  } else {
    owl_function_makemsg("Internal error on ack (%s)", retnotice->z_message);
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
  if (!strcasecmp(owl_message_get_zsig(m), "Automated reply:")) return;
  if (!strcasecmp(owl_message_get_sender(m), ZGetSender())) return;

  send_zephyr("",
	      "Automated reply:",
	      owl_message_get_class(m),
	      owl_message_get_instance(m),
	      owl_message_get_sender(m),
	      owl_global_get_zaway_msg(&g));

  /* display the message as an admin message in the receive window */
  tmpbuff = owl_sprintf("zwrite -c %s -i %s %s",
			owl_message_get_class(m),
			owl_message_get_instance(m),
			owl_message_get_sender(m));
  owl_function_make_outgoing_zephyr(owl_global_get_zaway_msg(&g), tmpbuff, "Automated reply:");
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
    myuser=short_zuser(user);
    sprintf(out, "%s: Hidden or not logged-in\n", myuser);
    owl_free(myuser);
    return;
  }
    
  for (;numlocs;numlocs--) {
    ZGetLocations(&locations,&one);
    myuser=short_zuser(user);
    sprintf(out, "%s%s: %s\t%s\t%s\n", out, myuser, locations.host, locations.tty, locations.time);
    owl_free(myuser);
  }
}

void owl_zephyr_addsub(char *filename, char *class, char *inst, char *recip) {
  char *line, subsfile[LINE], buff[LINE];
  FILE *file;

  line=owl_zephyr_makesubline(class, inst, recip);

  if (filename==NULL) {
    sprintf(subsfile, "%s/%s", owl_global_get_homedir(&g), ".zephyr.subs");
  } else {
    strcpy(subsfile, filename);
  }

  /* first check if it exists already */
  file=fopen(subsfile, "r");
  if (!file) {
    owl_function_makemsg("Error opening file %s", subsfile);
    owl_free(line);
    return;
  }
  while (fgets(buff, LINE, file)!=NULL) {
    if (!strcasecmp(buff, line)) {
      owl_function_makemsg("Subscription already present in %s", subsfile);
      owl_free(line);
      return;
    }
  }

  /* if we get here then we didn't find it */
  fclose(file);
  file=fopen(subsfile, "a");
  if (!file) {
    owl_function_makemsg("Error opening file %s for writing", subsfile);
    owl_free(line);
    return;
  }
  fputs(line, file);
  fclose(file);
  owl_function_makemsg("Subscription added");
  
  owl_free(line);
}

void owl_zephyr_delsub(char *filename, char *class, char *inst, char *recip) {
  char *line, subsfile[LINE], buff[LINE], *text;
  char backupfilename[LINE];
  FILE *file, *backupfile;
  int size;
  
  line=owl_zephyr_makesubline(class, inst, recip);

  /* open the subsfile for reading */
  if (filename==NULL) {
    sprintf(subsfile, "%s/%s", owl_global_get_homedir(&g), ".zephyr.subs");
  } else {
    strcpy(subsfile, filename);
  }
  file=fopen(subsfile, "r");
  if (!file) {
    owl_function_makemsg("Error opening file %s", subsfile);
    owl_free(line);
    return;
  }

  /* open the backup file for writing */
  sprintf(backupfilename, "%s.backup", subsfile);
  backupfile=fopen(backupfilename, "w");
  if (!backupfile) {
    owl_function_makemsg("Error opening file %s for writing", backupfilename);
    owl_free(line);
    return;
  }

  /* we'll read the entire file into memory, minus the line we don't want and
   * and at the same time create a backup file */
  text=malloc(LINE);
  size=LINE;
  while (fgets(buff, LINE, file)!=NULL) {
    /* if we don't match the line, add to text */
    if (strcasecmp(buff, line)) {
      size+=LINE;
      text=realloc(text, size);
      strcat(text, buff);
    }

    /* write to backupfile */
    fputs(buff, backupfile);
  }
  fclose(backupfile);
  fclose(file);

  /* now open the original subs file for writing and write out the
   * subs */
  file=fopen(subsfile, "w");
  if (!file) {
    owl_function_makemsg("WARNING: Error opening %s to rewrite subscriptions.  Use %s to restore", subsfile, backupfilename);
    owl_function_beep();
    owl_free(line);
    return;
  }

  fputs(text, file);
  fclose(file);
  owl_free(line);

  owl_function_makemsg("Subscription removed");
}

char *owl_zephyr_makesubline(char *class, char *inst, char *recip) {
  /* caller must free the return */
  char *out;

  out=owl_malloc(strlen(class)+strlen(inst)+strlen(recip)+30);
  sprintf(out, "%s,%s,%s\n", class, inst, !strcmp(recip, "") ? "*" : recip);
  return(out);
}
