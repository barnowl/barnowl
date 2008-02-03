#include "owl.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/param.h>

static const char fileIdent[] = "$Id$";

/* This is now the one function that should be called to log a
 * message.  It will do all the work necessary by calling the other
 * functions in this file as necessary.
 */
void owl_log_message(owl_message *m) {
  owl_function_debugmsg("owl_log_message: entering");

  if (m == NULL) {
    owl_function_debugmsg("owl_log_message: passed null message");
    return;
  }

  /* should we be logging this message? */
  if (!owl_log_shouldlog_message(m)) {
    owl_function_debugmsg("owl_log_message: not logging message");
    return;
  }

  /* handle incmoing messages */
  if (owl_message_is_direction_in(m)) {
    owl_log_incoming(m);
    owl_function_debugmsg("owl_log_message: leaving");
    return;
  }

  /* handle outgoing messages */
  owl_log_outgoing(m);

  owl_function_debugmsg("owl_log_message: leaving");
}

/* Return 1 if we should log the given message, otherwise return 0 */
int owl_log_shouldlog_message(owl_message *m) {
  owl_filter *f;

  /* If there's a logfilter and this message matches it, log */
  f=owl_global_get_filter(&g, owl_global_get_logfilter(&g));
  if (f && owl_filter_message_match(f, m)) return(1);

  /* otherwise we do things based on the logging variables */

  /* skip login/logout messages if appropriate */
  if (!owl_global_is_loglogins(&g) && owl_message_is_loginout(m)) return(0);
      
  /* check for nolog */
  if (!strcasecmp(owl_message_get_opcode(m), "nolog") || !strcasecmp(owl_message_get_instance(m), "nolog")) return(0);

  /* check direction */
  if ((owl_global_get_loggingdirection(&g)==OWL_LOGGING_DIRECTION_IN) && owl_message_is_direction_out(m)) {
    return(0);
  }
  if ((owl_global_get_loggingdirection(&g)==OWL_LOGGING_DIRECTION_OUT) && owl_message_is_direction_in(m)) {
    return(0);
  }

  if (owl_message_is_type_zephyr(m)) {
    if (owl_message_is_personal(m) && !owl_global_is_logging(&g)) return(0);
    if (!owl_message_is_personal(m) && !owl_global_is_classlogging(&g)) return(0);
  } else {
    if (owl_message_is_private(m) || owl_message_is_loginout(m)) {
      if (!owl_global_is_logging(&g)) return(0);
    } else {
      if (!owl_global_is_classlogging(&g)) return(0);
    }
  }
  return(1);
}

void owl_log_zephyr(owl_message *m, FILE *file) {
    char *tmp;
    tmp=short_zuser(owl_message_get_sender(m));
    fprintf(file, "Class: %s Instance: %s", owl_message_get_class(m), owl_message_get_instance(m));
    if (strcmp(owl_message_get_opcode(m), "")) fprintf(file, " Opcode: %s", owl_message_get_opcode(m));
    fprintf(file, "\n");
    fprintf(file, "Time: %s Host: %s\n", owl_message_get_timestr(m), owl_message_get_hostname(m));
    fprintf(file, "From: %s <%s>\n\n", owl_message_get_zsig(m), tmp);
    fprintf(file, "%s\n\n", owl_message_get_body(m));
    owl_free(tmp);
}

void owl_log_aim(owl_message *m, FILE *file) {
    fprintf(file, "From: <%s> To: <%s>\n", owl_message_get_sender(m), owl_message_get_recipient(m));
    fprintf(file, "Time: %s\n\n", owl_message_get_timestr(m));
    if (owl_message_is_login(m))
        fprintf(file, "LOGIN\n\n");
    else if (owl_message_is_logout(m))
        fprintf(file, "LOGOUT\n\n");
    else
        fprintf(file, "%s\n\n", owl_message_get_body(m));
}

void owl_log_jabber(owl_message *m, FILE *file) {
    fprintf(file, "From: <%s> To: <%s>\n",owl_message_get_sender(m), owl_message_get_recipient(m));
    fprintf(file, "Time: %s\n\n", owl_message_get_timestr(m));
    fprintf(file, "%s\n\n",owl_message_get_body(m));
}

void owl_log_generic(owl_message *m, FILE *file) {
    fprintf(file, "From: <%s> To: <%s>\n", owl_message_get_sender(m), owl_message_get_recipient(m));
    fprintf(file, "Time: %s\n\n", owl_message_get_timestr(m));
    fprintf(file, "%s\n\n", owl_message_get_body(m));
}

void owl_log_append(owl_message *m, char *filename) {
    FILE *file;
    file=fopen(filename, "a");
    if (!file) {
        owl_function_error("Unable to open file for logging");
        return;
    }
    if (owl_message_is_type_zephyr(m)) {
        owl_log_zephyr(m, file);
    } else if (owl_message_is_type_jabber(m)) {
        owl_log_jabber(m, file);
    } else if (owl_message_is_type_aim(m)) {
        owl_log_aim(m, file);
    } else {
        owl_log_generic(m, file);
    }
    fclose(file);
}

void owl_log_outgoing(owl_message *m)
{
  char filename[MAXPATHLEN], *logpath;
  char *to, *temp;

  /* expand ~ in path names */
  logpath = owl_text_substitute(owl_global_get_logpath(&g), "~", owl_global_get_homedir(&g));

  /* Figure out what path to log to */
  if (owl_message_is_type_zephyr(m)) {
    /* If this has CC's, do all but the "recipient" which we'll do below */
    to = owl_message_get_cc_without_recipient(m);
    if (to != NULL) {
      temp = strtok(to, " ");
      while (temp != NULL) {
          temp = short_zuser(temp);
          snprintf(filename, MAXPATHLEN, "%s/%s", logpath, temp);
          owl_log_append(m, filename);
          temp = strtok(NULL, " ");
      }
      owl_free(to);
    }
    to = short_zuser(owl_message_get_recipient(m));
  } else if (owl_message_is_type_jabber(m)) {
    to = owl_sprintf("jabber:%s", owl_message_get_recipient(m));
  } else if (owl_message_is_type_aim(m)) {
    temp = owl_aim_normalize_screenname(owl_message_get_recipient(m));
    downstr(temp);
    to = owl_sprintf("aim:%s", temp);
    owl_free(temp);
  } else {
    to = owl_sprintf("loopback");
  }

  snprintf(filename, MAXPATHLEN, "%s/%s", logpath, to);
  owl_log_append(m, filename);
  owl_free(to);

  snprintf(filename, MAXPATHLEN, "%s/all", logpath);
  owl_log_append(m, filename);
  owl_free(logpath);
}


void owl_log_outgoing_zephyr_error(char *to, char *text)
{
  FILE *file;
  char filename[MAXPATHLEN], *logpath;
  char *tobuff, *zwriteline;
  owl_message *m;

  /* create a present message so we can pass it to
   * owl_log_shouldlog_message()
   */
  zwriteline=owl_sprintf("zwrite %s", to);
  m=owl_function_make_outgoing_zephyr(text, zwriteline, "");
  owl_free(zwriteline);
  if (!owl_log_shouldlog_message(m)) {
    owl_message_free(m);
    return;
  }
  owl_message_free(m);

  /* chop off a local realm */
  tobuff=short_zuser(to);

  /* expand ~ in path names */
  logpath = owl_text_substitute(owl_global_get_logpath(&g), "~", owl_global_get_homedir(&g));

  snprintf(filename, MAXPATHLEN, "%s/%s", logpath, tobuff);
  file=fopen(filename, "a");
  if (!file) {
    owl_function_error("Unable to open file for outgoing logging");
    owl_free(logpath);
    owl_free(tobuff);
    return;
  }
  fprintf(file, "ERROR (owl): %s\n%s\n", tobuff, text);
  if (text[strlen(text)-1]!='\n') {
    fprintf(file, "\n");
  }
  fclose(file);

  snprintf(filename, MAXPATHLEN, "%s/all", logpath);
  owl_free(logpath);
  file=fopen(filename, "a");
  if (!file) {
    owl_function_error("Unable to open file for outgoing logging");
    owl_free(tobuff);
    return;
  }
  fprintf(file, "ERROR (owl): %s\n%s\n", tobuff, text);
  if (text[strlen(text)-1]!='\n') {
    fprintf(file, "\n");
  }
  fclose(file);

  owl_free(tobuff);
}

void owl_log_incoming(owl_message *m)
{
  char filename[MAXPATHLEN], allfilename[MAXPATHLEN], *logpath;
  char *frombuff=NULL, *from=NULL;
  int len, ch, i, personal;

  /* figure out if it's a "personal" message or not */
  if (owl_message_is_type_zephyr(m)) {
    if (owl_message_is_personal(m)) {
      personal = 1;
    } else {
      personal = 0;
    }
  } else if (owl_message_is_type_jabber(m)) {
    /* This needs to be fixed to handle groupchat */
    char* msgtype = owl_message_get_attribute_value(m,"jtype");
    if (msgtype && !strcmp(msgtype,"groupchat")) {
      personal = 0;
    } else {
      personal = 1;
    }
  } else {
    if (owl_message_is_private(m) || owl_message_is_loginout(m)) {
      personal = 1;
    } else {
      personal = 0;
    }
  }


  if (owl_message_is_type_zephyr(m)) {
    if (personal) {
      from=frombuff=short_zuser(owl_message_get_sender(m));
    } else {
      from=frombuff=owl_strdup(owl_message_get_class(m));
    }
  } else if (owl_message_is_type_aim(m)) {
    /* we do not yet handle chat rooms */
    char *normalto;
    normalto=owl_aim_normalize_screenname(owl_message_get_sender(m));
    downstr(normalto);
    from=frombuff=owl_sprintf("aim:%s", normalto);
    owl_free(normalto);
  } else if (owl_message_is_type_loopback(m)) {
    from=frombuff=owl_strdup("loopback");
  } else if (owl_message_is_type_jabber(m)) {
    if (personal) {
      from=frombuff=owl_sprintf("jabber:%s",owl_message_get_sender(m));
    } else {
      from=frombuff=owl_sprintf("jabber:%s",owl_message_get_recipient(m));
    }
  } else {
    from=frombuff=owl_strdup("unknown");
  }
  
  /* check for malicious sender formats */
  len=strlen(frombuff);
  if (len<1 || len>35) from="weird";
  if (strchr(frombuff, '/')) from="weird";

  ch=frombuff[0];
  if (!isalnum(ch)) from="weird";

  for (i=0; i<len; i++) {
    if (frombuff[i]<'!' || frombuff[i]>='~') from="weird";
  }

  if (!strcmp(frombuff, ".") || !strcasecmp(frombuff, "..")) from="weird";

  if (!personal) {
    if (strcmp(from, "weird")) downstr(from);
  }

  /* create the filename (expanding ~ in path names) */
  if (personal) {
    logpath = owl_text_substitute(owl_global_get_logpath(&g), "~", owl_global_get_homedir(&g));
    snprintf(filename, MAXPATHLEN, "%s/%s", logpath, from);
    snprintf(allfilename, MAXPATHLEN, "%s/all", logpath);
    owl_log_append(m, allfilename);

  } else {
    logpath = owl_text_substitute(owl_global_get_classlogpath(&g), "~", owl_global_get_homedir(&g));
    snprintf(filename, MAXPATHLEN, "%s/%s", logpath, from);
  }

  owl_log_append(m, filename);

  if (personal && owl_message_is_type_zephyr(m)) {
    /* We want to log to all of the CC'd people who were not us, or
     * the sender, as well.
     */
    char *cc, *temp;
    cc = owl_message_get_cc_without_recipient(m);
    if (cc != NULL) {
      temp = strtok(cc, " ");
      while (temp != NULL) {
        temp = short_zuser(temp);
        if (strcasecmp(temp, frombuff) != 0) {
          snprintf(filename, MAXPATHLEN, "%s/%s", logpath, temp);
          owl_log_append(m, filename);
        }
        temp = strtok(NULL, " ");
      }
      owl_free(cc);
    }
  }

  owl_free(frombuff);
  owl_free(logpath);
}
