#include "owl.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/param.h>

static const char fileIdent[] = "$Id$";

void owl_log_outgoing_zephyr(char *to, char *text) {
  FILE *file;
  char filename[MAXPATHLEN], *logpath;
  char *tobuff, *ptr="";

  tobuff=owl_malloc(strlen(to)+20);
  strcpy(tobuff, to);

  /* chop off a local realm */
  ptr=strchr(tobuff, '@');
  if (ptr && !strncmp(ptr+1, owl_zephyr_get_realm(), strlen(owl_zephyr_get_realm()))) {
    *ptr='\0';
  }

  /* expand ~ in path names */
  logpath = owl_util_substitute(owl_global_get_logpath(&g), "~", 
				owl_global_get_homedir(&g));

  snprintf(filename, MAXPATHLEN, "%s/%s", logpath, tobuff);
  file=fopen(filename, "a");
  if (!file) {
    owl_function_makemsg("Unable to open file for outgoing logging");
    owl_free(logpath);
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
    owl_function_makemsg("Unable to open file for outgoing logging");
    return;
  }
  fprintf(file, "OUTGOING (owl): %s\n%s\n", tobuff, text);
  if (text[strlen(text)-1]!='\n') {
    fprintf(file, "\n");
  }
  fclose(file);

  owl_free(tobuff);
}

void owl_log_outgoing_aim(char *to, char *text) {
  FILE *file;
  char filename[MAXPATHLEN], *logpath;
  char *tobuff;

  tobuff=owl_sprintf("aim:%s", to);

  /* expand ~ in path names */
  logpath = owl_util_substitute(owl_global_get_logpath(&g), "~", 
				owl_global_get_homedir(&g));

  snprintf(filename, MAXPATHLEN, "%s/%s", logpath, tobuff);
  file=fopen(filename, "a");
  if (!file) {
    owl_function_makemsg("Unable to open file for outgoing logging");
    owl_free(logpath);
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
    owl_function_makemsg("Unable to open file for outgoing logging");
    return;
  }
  fprintf(file, "OUTGOING (owl): %s\n%s\n", tobuff, text);
  if (text[strlen(text)-1]!='\n') {
    fprintf(file, "\n");
  }
  fclose(file);

  owl_free(tobuff);
}

void owl_log_incoming(owl_message *m) {
  FILE *file, *allfile;
  char filename[MAXPATHLEN], allfilename[MAXPATHLEN], *logpath;
  char *frombuff=NULL, *from=NULL, *buff=NULL, *ptr;
  int len, ch, i, personal;
      
  /* check for nolog */
  if (!strcasecmp(owl_message_get_opcode(m), "nolog") ||
      !strcasecmp(owl_message_get_instance(m), "nolog")) return;

  /* this is kind of a mess */
  if (owl_message_is_type_zephyr(m)) {
    if (owl_message_is_personal(m)) {
      personal=1;
      if (!owl_global_is_logging(&g)) return;
    } else {
      personal=0;
      if (!owl_global_is_classlogging(&g)) return;
    }
  } else {
    if (owl_message_is_private(m) || owl_message_is_loginout(m)) {
      personal=1;
      if (!owl_global_is_logging(&g)) return;
    } else {
      personal=0;
      if (!owl_global_is_classlogging(&g)) return;
    }
  }

  if (owl_message_is_type_zephyr(m)) {
    /* chop off a local realm for personal zephyr messages */
    if (personal) {
      if (owl_message_is_type_zephyr(m)) {
	from=frombuff=owl_strdup(owl_message_get_sender(m));
	ptr=strchr(frombuff, '@');
	if (ptr && !strncmp(ptr+1, owl_zephyr_get_realm(), strlen(owl_zephyr_get_realm()))) {
	  *ptr='\0';
	}
      }
    } else {
      /* we assume zephyr for now */
      from=frombuff=owl_strdup(owl_message_get_class(m));
    }
  } else if (owl_message_is_type_aim(m)) {
    /* we do not yet handle chat rooms */
    from=frombuff=owl_sprintf("aim:%s", owl_message_get_sender(m));
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
    logpath = owl_util_substitute(owl_global_get_logpath(&g), "~", 
				  owl_global_get_homedir(&g));
    snprintf(filename, MAXPATHLEN, "%s/%s", logpath, from);
    snprintf(allfilename, MAXPATHLEN, "%s/all", logpath);

  } else {
    logpath = owl_util_substitute(owl_global_get_classlogpath(&g), "~", 
				owl_global_get_homedir(&g));

    snprintf(filename, MAXPATHLEN, "%s/%s", logpath, from);
  }
  owl_free(logpath);
  
  file=fopen(filename, "a");
  if (!file) {
    owl_function_makemsg("Unable to open file for incoming logging");
    return;
  }

  allfile=NULL;
  if (personal) {
    allfile=fopen(allfilename, "a");
    if (!allfile) {
      owl_function_makemsg("Unable to open file for incoming logging");
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
    fprintf(file, "%s\n", owl_message_get_body(m));
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
      fprintf(allfile, "%s\n", owl_message_get_body(m));
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
    }
    fclose(allfile);
  }

  if (owl_message_is_type_zephyr(m)) {
    owl_free(buff);
  }
  owl_free(frombuff);
}
