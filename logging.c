#include "owl.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/param.h>

static const char fileIdent[] = "$Id$";

void owl_log_outgoing(char *to, char *text) {
  FILE *file;
  char filename[MAXPATHLEN], *logpath;
  char *tobuff, *ptr;

  tobuff=owl_malloc(strlen(to)+20);
  strcpy(tobuff, to);

  /* chop off a local realm */
  ptr=strchr(tobuff, '@');
  if (ptr && !strncmp(ptr+1, ZGetRealm(), strlen(ZGetRealm()))) {
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

void owl_log_incoming(owl_message *m) {
  FILE *file, *allfile;
  char filename[MAXPATHLEN], allfilename[MAXPATHLEN], *logpath;
  char *frombuff, *ptr, *from, *buff, *tmp;
  int len, ch, i, personal;

  /* check for nolog */
  if (!strcasecmp(owl_message_get_opcode(m), "nolog") ||
      !strcasecmp(owl_message_get_instance(m), "nolog")) return;

  if (owl_message_is_personal(m)) {
    personal=1;
    if (!owl_global_is_logging(&g)) return;
  } else {
    personal=0;
    if (!owl_global_is_classlogging(&g)) return;
  }

  if (personal) {
    from=frombuff=owl_strdup(owl_message_get_sender(m));
    /* chop off a local realm */
    ptr=strchr(frombuff, '@');
    if (ptr && !strncmp(ptr+1, ZGetRealm(), strlen(ZGetRealm()))) {
      *ptr='\0';
    }
  } else {
    from=frombuff=owl_strdup(owl_message_get_class(m));
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
      return;
    }
  }

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
  fclose(file);

  if (personal) {
    fprintf(allfile, "Class: %s Instance: %s", owl_message_get_class(m), owl_message_get_instance(m));
    if (strcmp(owl_message_get_opcode(m), "")) fprintf(allfile, " Opcode: %s", owl_message_get_opcode(m));
    fprintf(allfile, "\n");
    fprintf(allfile, "Time: %s Host: %s\n", owl_message_get_timestr(m), owl_message_get_hostname(m));
    fprintf(allfile, "From: %s <%s>\n\n", buff, tmp);
    fprintf(allfile, "%s\n", owl_message_get_body(m));
    fclose(allfile);
  }

  owl_free(tmp);
  owl_free(buff);
  owl_free(frombuff);
}
