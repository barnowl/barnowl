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
  if (owl_message_is_type_aim(m)) {
    owl_log_outgoing_aim(m);
  } else if (owl_message_is_type_zephyr(m)) {
    owl_log_outgoing_zephyr(m);
  } else if (owl_message_is_type_loopback(m)) {
    owl_log_outgoing_loopback(m);
  } else {
    owl_function_error("Unknown message type for logging");
  }
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

void owl_log_outgoing_zephyr(owl_message *m)
{
  FILE *file;
  char filename[MAXPATHLEN], *logpath;
  char *to, *text;

  /* strip local realm */
  to=short_zuser(owl_message_get_recipient(m));

  /* expand ~ in path names */
  logpath=owl_text_substitute(owl_global_get_logpath(&g), "~", owl_global_get_homedir(&g));

  text=owl_message_get_body(m);

  snprintf(filename, MAXPATHLEN, "%s/%s", logpath, to);
  file=fopen(filename, "a");
  if (!file) {
    owl_function_error("Unable to open file for outgoing logging");
    owl_free(logpath);
    owl_free(to);
    return;
  }
  fprintf(file, "OUTGOING (owl): %s\n%s\n", to, text);
  if (text[strlen(text)-1]!='\n') {
    fprintf(file, "\n");
  }
  fclose(file);

  snprintf(filename, MAXPATHLEN, "%s/all", logpath);
  owl_free(logpath);
  file=fopen(filename, "a");
  if (!file) {
    owl_function_error("Unable to open file for outgoing logging");
    owl_free(to);
    return;
  }
  fprintf(file, "OUTGOING (owl): %s\n%s\n", to, text);
  if (text[strlen(text)-1]!='\n') {
    fprintf(file, "\n");
  }
  fclose(file);

  owl_free(to);
}

void owl_log_outgoing_zephyr_error(char *to, char *text)
{
  FILE *file;
  char filename[MAXPATHLEN], *logpath;
  char *tobuff;

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

void owl_log_outgoing_aim(owl_message *m)
{
  FILE *file;
  char filename[MAXPATHLEN], *logpath;
  char *tobuff, *normalto, *text;

  owl_function_debugmsg("owl_log_outgoing_aim: entering");

  /* normalize and downcase the screenname */
  normalto=owl_aim_normalize_screenname(owl_message_get_recipient(m));
  downstr(normalto);
  tobuff=owl_sprintf("aim:%s", normalto);
  owl_free(normalto);

  /* expand ~ in path names */
  logpath = owl_text_substitute(owl_global_get_logpath(&g), "~", owl_global_get_homedir(&g));

  text=owl_message_get_body(m);
  
  snprintf(filename, MAXPATHLEN, "%s/%s", logpath, tobuff);
  file=fopen(filename, "a");
  if (!file) {
    owl_function_error("Unable to open file for outgoing logging");
    owl_free(logpath);
    owl_free(tobuff);
    return;
  }
  fprintf(file, "OUTGOING (owl): %s\n%s\n", tobuff, text);
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
  fprintf(file, "OUTGOING (owl): %s\n%s\n", tobuff, text);
  if (text[strlen(text)-1]!='\n') {
    fprintf(file, "\n");
  }
  fclose(file);

  owl_free(tobuff);
}

void owl_log_outgoing_loopback(owl_message *m)
{
  FILE *file;
  char filename[MAXPATHLEN], *logpath;
  char *tobuff, *text;

  tobuff=owl_sprintf("loopback");

  /* expand ~ in path names */
  logpath = owl_text_substitute(owl_global_get_logpath(&g), "~", owl_global_get_homedir(&g));

  text=owl_message_get_body(m);

  snprintf(filename, MAXPATHLEN, "%s/%s", logpath, tobuff);
  file=fopen(filename, "a");
  if (!file) {
    owl_function_error("Unable to open file for outgoing logging");
    owl_free(logpath);
    owl_free(tobuff);
    return;
  }
  fprintf(file, "OUTGOING (owl): %s\n%s\n", tobuff, text);
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
  fprintf(file, "OUTGOING (owl): %s\n%s\n", tobuff, text);
  if (text[strlen(text)-1]!='\n') {
    fprintf(file, "\n");
  }
  fclose(file);

  owl_free(tobuff);
}

void owl_log_incoming(owl_message *m)
{
  FILE *file, *allfile;
  char filename[MAXPATHLEN], allfilename[MAXPATHLEN], *logpath;
  char *frombuff=NULL, *from=NULL, *buff=NULL, *ptr;
  int len, ch, i, personal;

  /* figure out if it's a "personal" message or not */
  if (owl_message_is_type_zephyr(m)) {
    if (owl_message_is_personal(m)) {
      personal=1;
    } else {
      personal=0;
    }
  } else {
    if (owl_message_is_private(m) || owl_message_is_loginout(m)) {
      personal=1;
    } else {
      personal=0;
    }
  }

  if (owl_message_is_type_zephyr(m)) {
    if (personal) {
      if (owl_message_is_type_zephyr(m)) {
	from=frombuff=short_zuser(owl_message_get_sender(m));
      }
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

  } else {
    logpath = owl_text_substitute(owl_global_get_classlogpath(&g), "~", owl_global_get_homedir(&g));
    snprintf(filename, MAXPATHLEN, "%s/%s", logpath, from);
  }
  owl_free(logpath);
  
  file=fopen(filename, "a");
  if (!file) {
    owl_function_error("Unable to open file for incoming logging");
    owl_free(frombuff);
    return;
  }

  allfile=NULL;
  if (personal) {
    allfile=fopen(allfilename, "a");
    if (!allfile) {
      owl_function_error("Unable to open file for incoming logging");
      owl_free(frombuff);
      fclose(file);
      return;
    }
  }

  /* write to the main file */
  if (owl_message_is_type_zephyr(m)) {
    char *tmp;
    
    tmp=short_zuser(owl_message_get_sender(m));
    fprintf(file, "Class: %s Instance: %s", owl_message_get_class(m), owl_message_get_instance(m));
    if (strcmp(owl_message_get_opcode(m), "")) fprintf(file, " Opcode: %s", owl_message_get_opcode(m));
    fprintf(file, "\n");
    fprintf(file, "Time: %s Host: %s\n", owl_message_get_timestr(m), owl_message_get_hostname(m));
    ptr=owl_zephyr_get_zsig(owl_message_get_notice(m), &i);
    buff=owl_malloc(i+10);
    memcpy(buff, ptr, i);
    buff[i]='\0';
    fprintf(file, "From: %s <%s>\n\n", buff, tmp);
    fprintf(file, "%s\n\n", owl_message_get_body(m));
    owl_free(tmp);
  } else if (owl_message_is_type_aim(m) && !owl_message_is_loginout(m)) {
    fprintf(file, "From: <%s> To: <%s>\n", owl_message_get_sender(m), owl_message_get_recipient(m));
    fprintf(file, "Time: %s\n\n", owl_message_get_timestr(m));
    fprintf(file, "%s\n\n", owl_message_get_body(m));
  } else if (owl_message_is_type_aim(m) && owl_message_is_loginout(m)) {
    fprintf(file, "From: <%s> To: <%s>\n", owl_message_get_sender(m), owl_message_get_recipient(m));
    fprintf(file, "Time: %s\n\n", owl_message_get_timestr(m));
    if (owl_message_is_login(m)) fprintf(file, "LOGIN\n\n");
    if (owl_message_is_logout(m)) fprintf(file, "LOGOUT\n\n");
  } else {
    fprintf(file, "From: <%s> To: <%s>\n", owl_message_get_sender(m), owl_message_get_recipient(m));
    fprintf(file, "Time: %s\n\n", owl_message_get_timestr(m));
    fprintf(file, "%s\n\n", owl_message_get_body(m));
  }

  fclose(file);

  /* if it's a personal message, also write to the 'all' file */
  if (personal) {
    if (owl_message_is_type_zephyr(m)) {
      char *tmp;

      tmp=short_zuser(owl_message_get_sender(m));
      fprintf(allfile, "Class: %s Instance: %s", owl_message_get_class(m), owl_message_get_instance(m));
      if (strcmp(owl_message_get_opcode(m), "")) fprintf(allfile, " Opcode: %s", owl_message_get_opcode(m));
      fprintf(allfile, "\n");
      fprintf(allfile, "Time: %s Host: %s\n", owl_message_get_timestr(m), owl_message_get_hostname(m));
      fprintf(allfile, "From: %s <%s>\n\n", buff, tmp);
      fprintf(allfile, "%s\n\n", owl_message_get_body(m));
      owl_free(tmp);
    } else if (owl_message_is_type_aim(m) && !owl_message_is_loginout(m)) {
      fprintf(allfile, "From: <%s> To: <%s>\n", owl_message_get_sender(m), owl_message_get_recipient(m));
      fprintf(allfile, "Time: %s\n\n", owl_message_get_timestr(m));
      fprintf(allfile, "%s\n\n", owl_message_get_body(m));
    } else if (owl_message_is_type_aim(m) && owl_message_is_loginout(m)) {
      fprintf(allfile, "From: <%s> To: <%s>\n", owl_message_get_sender(m), owl_message_get_recipient(m));
      fprintf(allfile, "Time: %s\n\n", owl_message_get_timestr(m));
      if (owl_message_is_login(m)) fprintf(allfile, "LOGIN\n\n");
      if (owl_message_is_logout(m)) fprintf(allfile, "LOGOUT\n\n");
    } else {
      fprintf(allfile, "From: <%s> To: <%s>\n", owl_message_get_sender(m), owl_message_get_recipient(m));
      fprintf(allfile, "Time: %s\n\n", owl_message_get_timestr(m));
      fprintf(allfile, "%s\n\n", owl_message_get_body(m));
    }
    fclose(allfile);
  }

  if (owl_message_is_type_zephyr(m)) {
    owl_free(buff);
  }
  owl_free(frombuff);
}
