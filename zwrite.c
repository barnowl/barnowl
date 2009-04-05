/* Copyright (c) 2002,2003,2004,2009 James M. Kretchmar
 *
 * This file is part of Owl.
 *
 * Owl is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Owl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Owl.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ---------------------------------------------------------------
 * 
 * As of Owl version 2.1.12 there are patches contributed by
 * developers of the branched BarnOwl project, Copyright (c)
 * 2006-2009 The BarnOwl Developers. All rights reserved.
 */

#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";

int owl_zwrite_create_from_line(owl_zwrite *z, char *line)
{
  int argc, badargs, myargc;
  char **argv, **myargv;
  char *zsigproc, *zsigowlvar, *zsigzvar, *ptr;
  struct passwd *pw;

  badargs=0;
  
  /* start with null entires */
  z->realm=NULL;
  z->class=NULL;
  z->inst=NULL;
  z->opcode=NULL;
  z->zsig=NULL;
  z->message=NULL;
  z->cc=0;
  z->noping=0;
  owl_list_create(&(z->recips));

  /* parse the command line for options */
  argv=myargv=owl_parseline(line, &argc);
  if (argc<0) {
    owl_function_error("Unbalanced quotes in zwrite");
    return(-1);
  }
  myargc=argc;
  if (myargc && *(myargv[0])!='-') {
    myargc--;
    myargv++;
  }
  while (myargc) {
    if (!strcmp(myargv[0], "-c")) {
      if (myargc<2) {
	badargs=1;
	break;
      }
      z->class=owl_strdup(myargv[1]);
      myargv+=2;
      myargc-=2;
    } else if (!strcmp(myargv[0], "-i")) {
      if (myargc<2) {
	badargs=1;
	break;
      }
      z->inst=owl_strdup(myargv[1]);
      myargv+=2;
      myargc-=2;
    } else if (!strcmp(myargv[0], "-r")) {
      if (myargc<2) {
	badargs=1;
	break;
      }
      z->realm=owl_strdup(myargv[1]);
      myargv+=2;
      myargc-=2;
    } else if (!strcmp(myargv[0], "-s")) {
      if (myargc<2) {
	badargs=1;
	break;
      }
      z->zsig=owl_strdup(myargv[1]);
      myargv+=2;
      myargc-=2;
    } else if (!strcmp(myargv[0], "-O")) {
      if (myargc<2) {
	badargs=1;
	break;
      }
      z->opcode=owl_strdup(myargv[1]);
      myargv+=2;
      myargc-=2;
    } else if (!strcmp(myargv[0], "-m")) {
      if (myargc<2) {
	badargs=1;
	break;
      }
      /* we must already have users or a class or an instance*/
      if (owl_list_get_size(&(z->recips))<1 && (!z->class) && (!z->inst)) {
	badargs=1;
	break;
      }

      /* Once we have -m, gobble up everything else on the line */
      myargv++;
      myargc--;
      z->message=owl_strdup("");
      while (myargc) {
	z->message=realloc(z->message, strlen(z->message)+strlen(myargv[0])+5);
	strcat(z->message, myargv[0]);
	strcat(z->message, " ");
	myargc--;
	myargv++;
      }
      z->message[strlen(z->message)-1]='\0'; /* remove last space */
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
      owl_list_append_element(&(z->recips), strdup(myargv[0]));
      myargv++;
      myargc--;
    }
  }

  owl_parsefree(argv, argc);

  if (badargs) {
    return(-1);
  }

  /* now deal with defaults */
  if (z->class==NULL) z->class=owl_strdup("message");
  if (z->inst==NULL) z->inst=owl_strdup("personal");
  if (z->realm==NULL) z->realm=owl_strdup("");
  if (z->opcode==NULL) z->opcode=owl_strdup("");
  /* z->message is allowed to stay NULL */
  
  if (!strcasecmp(z->class, "message") &&
      !strcasecmp(z->inst, "personal") &&
      owl_list_get_size(&(z->recips))==0) {
    owl_function_error("You must specify a recipient for zwrite");
    return(-1);
  }

  /* get a zsig, if not given */
  if (z->zsig==NULL) {
    zsigproc = owl_global_get_zsigproc(&g);
    zsigowlvar = owl_global_get_zsig(&g);
    zsigzvar = owl_zephyr_get_variable("zwrite-signature");

    if (zsigowlvar && *zsigowlvar) {
      z->zsig=owl_strdup(zsigowlvar);
    } else if (zsigproc && *zsigproc) {
      FILE *file;
      char buff[LINE], *openline;
      
      /* simple hack for now to nuke stderr */
      openline=owl_malloc(strlen(zsigproc)+40);
      strcpy(openline, zsigproc);
#if !(OWL_STDERR_REDIR)
      strcat(openline, " 2> /dev/null");
#endif
      file=popen(openline, "r");
      owl_free(openline);
      if (!file) {
	if (zsigzvar && *zsigzvar) {
	  z->zsig=owl_strdup(zsigzvar);
	}
      } else {
	z->zsig=owl_malloc(LINE*5);
	strcpy(z->zsig, "");
	while (fgets(buff, LINE, file)) { /* wrong sizing */
	  strcat(z->zsig, buff);
	}
	pclose(file);
	if (z->zsig[strlen(z->zsig)-1]=='\n') {
	  z->zsig[strlen(z->zsig)-1]='\0';
	}
      }
    } else if (zsigzvar) {
      z->zsig=owl_strdup(zsigzvar);
    } else if (((pw=getpwuid(getuid()))!=NULL) && (pw->pw_gecos)) {
      z->zsig=strdup(pw->pw_gecos);
      ptr=strchr(z->zsig, ',');
      if (ptr) {
	ptr[0]='\0';
      }
    }
  }

  return(0);
}

void owl_zwrite_send_ping(owl_zwrite *z)
{
  int i, j;
  char *to;

  if (z->noping) return;
  
  if (strcasecmp(z->class, "message") ||
      strcasecmp(z->inst, "personal")) {
    return;
  }

  /* if there are no recipients we won't send a ping, which
     is what we want */
  j=owl_list_get_size(&(z->recips));
  for (i=0; i<j; i++) {
    if (strcmp(z->realm, "")) {
      to = owl_sprintf("%s@%s", (char *) owl_list_get_element(&(z->recips), i), z->realm);
    } else {
      to = owl_strdup(owl_list_get_element(&(z->recips), i));
    }
    send_ping(to);
    owl_free(to);
  }

}

void owl_zwrite_set_message(owl_zwrite *z, char *msg)
{
  int i, j;
  char *toline = NULL;
  char *tmp = NULL;
  
  if (z->message) owl_free(z->message);

  j=owl_list_get_size(&(z->recips));
  if (j>0 && z->cc) {
    toline = owl_strdup( "CC: ");
    for (i=0; i<j; i++) {
      tmp = toline;
      if (strcmp(z->realm, "")) {
	toline = owl_sprintf( "%s%s@%s ", toline, (char *) owl_list_get_element(&(z->recips), i), z->realm);
      } else {
	toline = owl_sprintf( "%s%s ", toline, (char *) owl_list_get_element(&(z->recips), i));
      }
      owl_free(tmp);
      tmp=NULL;
    }
    z->message=owl_sprintf("%s\n%s", toline, msg);
    owl_free(toline);
  } else {
    z->message=owl_strdup(msg);
  }
}

char *owl_zwrite_get_message(owl_zwrite *z)
{
  if (z->message) return(z->message);
  return("");
}

int owl_zwrite_is_message_set(owl_zwrite *z)
{
  if (z->message) return(1);
  return(0);
}

int owl_zwrite_send_message(owl_zwrite *z)
{
  int i, j;
  char *to = NULL;

  if (z->message==NULL) return(-1);

  j=owl_list_get_size(&(z->recips));
  if (j>0) {
    for (i=0; i<j; i++) {
      if (strcmp(z->realm, "")) {
	to = owl_sprintf("%s@%s", (char *) owl_list_get_element(&(z->recips), i), z->realm);
      } else {
to = owl_strdup( owl_list_get_element(&(z->recips), i));
      }
      send_zephyr(z->opcode, z->zsig, z->class, z->inst, to, z->message);
      owl_free(to);
      to = NULL;
    }
  } else {
    to = owl_sprintf( "@%s", z->realm);
    send_zephyr(z->opcode, z->zsig, z->class, z->inst, to, z->message);
  }
  owl_free(to);
  return(0);
}

int owl_zwrite_create_and_send_from_line(char *cmd, char *msg)
{
  owl_zwrite z;
  int rv;
  rv=owl_zwrite_create_from_line(&z, cmd);
  if (rv) return(rv);
  if (!owl_zwrite_is_message_set(&z)) {
    owl_zwrite_set_message(&z, msg);
  }
  owl_zwrite_send_message(&z);
  owl_zwrite_free(&z);
  return(0);
}

char *owl_zwrite_get_class(owl_zwrite *z)
{
  return(z->class);
}

char *owl_zwrite_get_instance(owl_zwrite *z)
{
  return(z->inst);
}

char *owl_zwrite_get_opcode(owl_zwrite *z)
{
  return(z->opcode);
}

void owl_zwrite_set_opcode(owl_zwrite *z, char *opcode)
{
  if (z->opcode) owl_free(z->opcode);
  z->opcode=owl_strdup(opcode);
}

char *owl_zwrite_get_realm(owl_zwrite *z)
{
  return(z->realm);
}

char *owl_zwrite_get_zsig(owl_zwrite *z)
{
  if (z->zsig) return(z->zsig);
  return("");
}

int owl_zwrite_get_numrecips(owl_zwrite *z)
{
  return(owl_list_get_size(&(z->recips)));
}

char *owl_zwrite_get_recip_n(owl_zwrite *z, int n)
{
  return(owl_list_get_element(&(z->recips), n));
}

int owl_zwrite_is_personal(owl_zwrite *z)
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
  
void owl_zwrite_free(owl_zwrite *z)
{
  owl_list_free_all(&(z->recips), &owl_free);
  if (z->class) owl_free(z->class);
  if (z->inst) owl_free(z->inst);
  if (z->opcode) owl_free(z->opcode);
  if (z->realm) owl_free(z->realm);
  if (z->message) owl_free(z->message);
  if (z->zsig) owl_free(z->zsig);
}
