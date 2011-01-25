#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include "owl.h"

owl_zwrite *owl_zwrite_new(const char *line)
{
  owl_zwrite *z = owl_malloc(sizeof *z);
  if (owl_zwrite_create_from_line(z, line) < 0) {
    owl_zwrite_delete(z);
    return NULL;
  }
  return z;
}

int owl_zwrite_create_from_line(owl_zwrite *z, const char *line)
{
  int argc, badargs, myargc;
  char **argv;
  const char *const *myargv;
  char *msg = NULL;

  badargs=0;
  
  /* start with null entries */
  z->cmd=NULL;
  z->realm=NULL;
  z->class=NULL;
  z->inst=NULL;
  z->opcode=NULL;
  z->zsig=NULL;
  z->message=NULL;
  z->cc=0;
  z->noping=0;
  owl_list_create(&(z->recips));
  z->zwriteline = owl_strdup(line);

  /* parse the command line for options */
  argv=owl_parseline(line, &argc);
  myargv=strs(argv);
  if (argc<0) {
    owl_function_error("Unbalanced quotes in zwrite");
    return(-1);
  }
  myargc=argc;
  if (myargc && *(myargv[0])!='-') {
    z->cmd=owl_strdup(myargv[0]);
    myargc--;
    myargv++;
  }
  while (myargc) {
    if (!strcmp(myargv[0], "-c")) {
      if (myargc<2) {
	badargs=1;
	break;
      }
      z->class=owl_validate_utf8(myargv[1]);
      myargv+=2;
      myargc-=2;
    } else if (!strcmp(myargv[0], "-i")) {
      if (myargc<2) {
	badargs=1;
	break;
      }
      z->inst=owl_validate_utf8(myargv[1]);
      myargv+=2;
      myargc-=2;
    } else if (!strcmp(myargv[0], "-r")) {
      if (myargc<2) {
	badargs=1;
	break;
      }
      z->realm=owl_validate_utf8(myargv[1]);
      myargv+=2;
      myargc-=2;
    } else if (!strcmp(myargv[0], "-s")) {
      if (myargc<2) {
	badargs=1;
	break;
      }
      z->zsig=owl_validate_utf8(myargv[1]);
      myargv+=2;
      myargc-=2;
    } else if (!strcmp(myargv[0], "-O")) {
      if (myargc<2) {
	badargs=1;
	break;
      }
      z->opcode=owl_validate_utf8(myargv[1]);
      myargv+=2;
      myargc-=2;
    } else if (!strcmp(myargv[0], "-m")) {
      if (myargc<2) {
	badargs=1;
	break;
      }
      /* we must already have users or a class or an instance */
      if (owl_list_get_size(&(z->recips))<1 && (!z->class) && (!z->inst)) {
	badargs=1;
	break;
      }

      /* Once we have -m, gobble up everything else on the line */
      myargv++;
      myargc--;
      msg = g_strjoinv(" ", (char**)myargv);
      break;
    } else if (!strcmp(myargv[0], "-C")) {
      z->cc=1;
      myargv++;
      myargc--;
    } else if (!strcmp(myargv[0], "-n")) {
      z->noping=1;
      myargv++;
      myargc--;
    } else {
      /* anything unattached is a recipient */
      owl_list_append_element(&(z->recips), owl_validate_utf8(myargv[0]));
      myargv++;
      myargc--;
    }
  }

  owl_parse_delete(argv, argc);

  if (badargs) {
    return(-1);
  }

  if (z->class == NULL &&
      z->inst == NULL &&
      owl_list_get_size(&(z->recips))==0) {
    owl_function_error("You must specify a recipient for zwrite");
    return(-1);
  }

  /* now deal with defaults */
  if (z->class==NULL) z->class=owl_strdup("message");
  if (z->inst==NULL) z->inst=owl_strdup("personal");
  if (z->realm==NULL) z->realm=owl_strdup("");
  if (z->opcode==NULL) z->opcode=owl_strdup("");
  /* z->message is allowed to stay NULL */

  if(msg) {
    owl_zwrite_set_message(z, msg);
    owl_free(msg);
  }

  return(0);
}

void owl_zwrite_populate_zsig(owl_zwrite *z)
{
  /* get a zsig, if not given */
  if (z->zsig != NULL)
    return;

  z->zsig = owl_perlconfig_execute(owl_global_get_zsigfunc(&g));
}

void owl_zwrite_send_ping(const owl_zwrite *z)
{
  int i, j;
  char *to;

  if (z->noping) return;
  
  if (strcasecmp(z->class, "message")) {
    return;
  }

  /* if there are no recipients we won't send a ping, which
     is what we want */
  j=owl_list_get_size(&(z->recips));
  for (i=0; i<j; i++) {
    if (strcmp(z->realm, "")) {
      to = owl_sprintf("%s@%s", (const char *) owl_list_get_element(&(z->recips), i), z->realm);
    } else {
      to = owl_strdup(owl_list_get_element(&(z->recips), i));
    }
    send_ping(to, z->class, z->inst);
    owl_free(to);
  }

}

/* Set the message with no post-processing*/
void owl_zwrite_set_message_raw(owl_zwrite *z, const char *msg)
{
  if (z->message) owl_free(z->message);
  z->message = owl_validate_utf8(msg);
}

void owl_zwrite_set_message(owl_zwrite *z, const char *msg)
{
  int i, j;
  char *toline = NULL;
  char *tmp = NULL, *tmp2;

  if (z->message) owl_free(z->message);

  j=owl_list_get_size(&(z->recips));
  if (j>0 && z->cc) {
    toline = owl_strdup( "CC: ");
    for (i=0; i<j; i++) {
      tmp = toline;
      if (strcmp(z->realm, "")) {
        toline = owl_sprintf( "%s%s@%s ", toline, (const char *) owl_list_get_element(&(z->recips), i), z->realm);
      } else {
        toline = owl_sprintf( "%s%s ", toline, (const char *) owl_list_get_element(&(z->recips), i));
      }
      owl_free(tmp);
      tmp = NULL;
    }
    tmp = owl_validate_utf8(msg);
    tmp2 = owl_text_expand_tabs(tmp);
    z->message=owl_sprintf("%s\n%s", toline, tmp2);
    owl_free(toline);
    owl_free(tmp);
    owl_free(tmp2);
  } else {
    tmp=owl_validate_utf8(msg);
    z->message=owl_text_expand_tabs(tmp);
    owl_free(tmp);
  }
}

const char *owl_zwrite_get_message(const owl_zwrite *z)
{
  if (z->message) return(z->message);
  return("");
}

int owl_zwrite_is_message_set(const owl_zwrite *z)
{
  if (z->message) return(1);
  return(0);
}

int owl_zwrite_send_message(const owl_zwrite *z)
{
  int i, j, ret = 0;
  char *to = NULL;

  if (z->message==NULL) return(-1);

  j=owl_list_get_size(&(z->recips));
  if (j>0) {
    for (i=0; i<j; i++) {
      if (strcmp(z->realm, "")) {
        to = owl_sprintf("%s@%s", (const char *) owl_list_get_element(&(z->recips), i), z->realm);
      } else {
        to = owl_strdup( owl_list_get_element(&(z->recips), i));
      }
      ret = send_zephyr(z->opcode, z->zsig, z->class, z->inst, to, z->message);
      /* Abort on the first error, to match the zwrite binary. */
      if (ret != 0)
        break;
      owl_free(to);
      to = NULL;
    }
  } else {
    to = owl_sprintf( "@%s", z->realm);
    ret = send_zephyr(z->opcode, z->zsig, z->class, z->inst, to, z->message);
  }
  owl_free(to);
  return ret;
}

int owl_zwrite_create_and_send_from_line(const char *cmd, const char *msg)
{
  owl_zwrite z;
  int rv;
  rv=owl_zwrite_create_from_line(&z, cmd);
  if (rv) return(rv);
  if (!owl_zwrite_is_message_set(&z)) {
    owl_zwrite_set_message(&z, msg);
  }
  owl_zwrite_populate_zsig(&z);
  owl_zwrite_send_message(&z);
  owl_zwrite_cleanup(&z);
  return(0);
}

const char *owl_zwrite_get_class(const owl_zwrite *z)
{
  return(z->class);
}

const char *owl_zwrite_get_instance(const owl_zwrite *z)
{
  return(z->inst);
}

const char *owl_zwrite_get_opcode(const owl_zwrite *z)
{
  return(z->opcode);
}

void owl_zwrite_set_opcode(owl_zwrite *z, const char *opcode)
{
  if (z->opcode) owl_free(z->opcode);
  z->opcode=owl_validate_utf8(opcode);
}

const char *owl_zwrite_get_realm(const owl_zwrite *z)
{
  return(z->realm);
}

const char *owl_zwrite_get_zsig(const owl_zwrite *z)
{
  if (z->zsig) return(z->zsig);
  return("");
}

void owl_zwrite_set_zsig(owl_zwrite *z, const char *zsig)
{
  if(z->zsig) owl_free(z->zsig);
  z->zsig = owl_strdup(zsig);
}

void owl_zwrite_get_recipstr(const owl_zwrite *z, char *buff)
{
  int i, j;

  strcpy(buff, "");
  j=owl_list_get_size(&(z->recips));
  for (i=0; i<j; i++) {
    strcat(buff, owl_list_get_element(&(z->recips), i));
    strcat(buff, " ");
  }
  buff[strlen(buff)-1]='\0';
}

int owl_zwrite_get_numrecips(const owl_zwrite *z)
{
  return(owl_list_get_size(&(z->recips)));
}

const char *owl_zwrite_get_recip_n(const owl_zwrite *z, int n)
{
  return(owl_list_get_element(&(z->recips), n));
}

int owl_zwrite_is_personal(const owl_zwrite *z)
{
  /* return true if at least one of the recipients is personal */
  int i, j;
  char *foo;

  j=owl_list_get_size(&(z->recips));
  for (i=0; i<j; i++) {
    foo=owl_list_get_element(&(z->recips), i);
    if (foo[0]!='@') return(1);
  }
  return(0);
}

void owl_zwrite_delete(owl_zwrite *z)
{
  owl_zwrite_cleanup(z);
  owl_free(z);
}

void owl_zwrite_cleanup(owl_zwrite *z)
{
  owl_list_cleanup(&(z->recips), &owl_free);
  if (z->cmd) owl_free(z->cmd);
  if (z->zwriteline) owl_free(z->zwriteline);
  if (z->class) owl_free(z->class);
  if (z->inst) owl_free(z->inst);
  if (z->opcode) owl_free(z->opcode);
  if (z->realm) owl_free(z->realm);
  if (z->message) owl_free(z->message);
  if (z->zsig) owl_free(z->zsig);
}

/*
 * Returns a zwrite line suitable for replying, specifically the
 * message field is stripped out. Result should be freed with
 * owl_free.
 */
char *owl_zwrite_get_replyline(const owl_zwrite *z)
{
  /* Match ordering in zwrite help. */
  GString *buf = g_string_new("");
  int i;

  /* Disturbingly, it is apparently possible to z->cmd to be null if
   * owl_zwrite_create_from_line got something starting with -. And we
   * can't kill it because this is exported to perl. */
  owl_string_append_quoted_arg(buf, z->cmd ? z->cmd : "zwrite");
  if (z->noping) {
    g_string_append(buf, " -n");
  }
  if (z->cc) {
    g_string_append(buf, " -C");
  }
  if (strcmp(z->class, "message")) {
    g_string_append(buf, " -c ");
    owl_string_append_quoted_arg(buf, z->class);
  }
  if (strcmp(z->inst, "personal")) {
    g_string_append(buf, " -i ");
    owl_string_append_quoted_arg(buf, z->inst);
  }
  if (z->realm && z->realm[0] != '\0') {
    g_string_append(buf, " -r ");
    owl_string_append_quoted_arg(buf, z->realm);
  }
  if (z->opcode && z->opcode[0] != '\0') {
    g_string_append(buf, " -O ");
    owl_string_append_quoted_arg(buf, z->opcode);
  }
  for (i = 0; i < owl_list_get_size(&(z->recips)); i++) {
    g_string_append_c(buf, ' ');
    owl_string_append_quoted_arg(buf, owl_list_get_element(&(z->recips), i));
  }

  return g_string_free(buf, false);
}
