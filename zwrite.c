#include <string.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";

int owl_zwrite_create_from_line(owl_zwrite *z, char *line) {
  int argc, badargs, myargc;
  char **argv, **myargv;

  badargs=0;
  
  /* set the defaults */
  strcpy(z->realm, "");
  strcpy(z->class, "message");
  strcpy(z->inst, "personal");
  strcpy(z->opcode, "");
  z->cc=0;
  z->noping=0;
  owl_list_create(&(z->recips));

  /* parse the command line for options */
  argv=myargv=owl_parseline(line, &argc);
  if (argc<0) {
    owl_function_makemsg("Unbalanced quotes");
    return(-1);
  }
  myargc=argc;
  myargc--;
  myargv++;
  while (myargc) {
    if (!strcmp(myargv[0], "-c")) {
      if (myargc<2) {
	badargs=1;
	break;
      }
      strcpy(z->class, myargv[1]);
      myargv+=2;
      myargc-=2;
    } else if (!strcmp(myargv[0], "-i")) {
      if (myargc<2) {
	badargs=1;
	break;
      }
      strcpy(z->inst, myargv[1]);
      myargv+=2;
      myargc-=2;
    } else if (!strcmp(myargv[0], "-r")) {
      if (myargc<2) {
	badargs=1;
	break;
      }
      strcpy(z->realm, myargv[1]);
      myargv+=2;
      myargc-=2;
    } else if (!strcmp(myargv[0], "-O")) {
      if (myargc<2) {
	badargs=1;
	break;
      }
      strcpy(z->opcode, myargv[1]);
      myargv+=2;
      myargc-=2;
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

  if (!strcasecmp(z->class, "message") &&
      !strcasecmp(z->inst, "personal") &&
      owl_list_get_size(&(z->recips))==0) {
    /* do the makemsg somewhere else */
    owl_function_makemsg("You must specify a recipient");
    return(-1);
  }

  return(0);
}

void owl_zwrite_send_ping(owl_zwrite *z) {
  int i, j;
  char to[LINE];

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
      sprintf(to, "%s@%s", (char *) owl_list_get_element(&(z->recips), i), z->realm);
    } else {
      strcpy(to, owl_list_get_element(&(z->recips), i));
    }
    send_ping(to);
  }

}

void owl_zwrite_send_message(owl_zwrite *z, char *msg) {
  int i, j;
  char to[LINE];

  j=owl_list_get_size(&(z->recips));
  if (j>0) {
    char *tmpmsg=NULL;
    char toline[LINE];

    if (z->cc) {
      strcpy(toline, "CC: ");
      for (i=0; i<j; i++) {
	if (strcmp(z->realm, "")) {
	  sprintf(toline, "%s%s@%s ", toline, (char *) owl_list_get_element(&(z->recips), i), z->realm);
	} else {
	  sprintf(toline, "%s%s ", toline, (char *) owl_list_get_element(&(z->recips), i));
	}
      }
      tmpmsg=owl_malloc(strlen(msg)+strlen(toline)+30);
      sprintf(tmpmsg, "%s\n%s", toline, msg);
    }

    for (i=0; i<j; i++) {
      if (strcmp(z->realm, "")) {
	sprintf(to, "%s@%s", (char *) owl_list_get_element(&(z->recips), i), z->realm);
      } else {
	strcpy(to, owl_list_get_element(&(z->recips), i));
      }
      if (z->cc) {
	send_zephyr(z->opcode, NULL, z->class, z->inst, to, tmpmsg);
      } else {
	send_zephyr(z->opcode, NULL, z->class, z->inst, to, msg);
      }
    }
    if (z->cc) {
      owl_free(tmpmsg);
    }
  } else {
    sprintf(to, "@%s", z->realm);
    send_zephyr(z->opcode, NULL, z->class, z->inst, to, msg);
  }
}

char *owl_zwrite_get_class(owl_zwrite *z) {
  return(z->class);
}

char *owl_zwrite_get_instance(owl_zwrite *z) {
  return(z->inst);
}

char *owl_zwrite_get_realm(owl_zwrite *z) {
  return(z->realm);
}

void owl_zwrite_get_recipstr(owl_zwrite *z, char *buff) {
  int i, j;

  strcpy(buff, "");
  j=owl_list_get_size(&(z->recips));
  for (i=0; i<j; i++) {
    strcat(buff, owl_list_get_element(&(z->recips), i));
    strcat(buff, " ");
  }
  buff[strlen(buff)-1]='\0';
}

int owl_zwrite_get_numrecips(owl_zwrite *z) {
  return(owl_list_get_size(&(z->recips)));
}

char *owl_zwrite_get_recip_n(owl_zwrite *z, int n) {
  return(owl_list_get_element(&(z->recips), n));
}

int owl_zwrite_is_personal(owl_zwrite *z) {
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
  

void owl_zwrite_free(owl_zwrite *z) {
  owl_list_free_all(&(z->recips), &owl_free);
}
