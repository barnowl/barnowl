#include "owl.h"
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>

static const char fileIdent[] = "$Id$";

void sepbar(char *in) {
  char buff[1024];
  WINDOW *sepwin;
  owl_messagelist *ml;
  owl_view *v;
  int x, y, i;
  char *foo, *appendtosepbar;

  sepwin=owl_global_get_curs_sepwin(&g);
  ml=owl_global_get_msglist(&g);
  v=owl_global_get_current_view(&g);

  werase(sepwin);
  wattron(sepwin, A_REVERSE);
  whline(sepwin, ACS_HLINE, owl_global_get_cols(&g));

  wmove(sepwin, 0, 2);  

  if (owl_messagelist_get_size(ml)==0) {
    strcpy(buff, " (-/-) ");
  } else {
    snprintf(buff, 1024, " (%i/%i/%i) ", owl_global_get_curmsg(&g)+1,
	    owl_view_get_size(v),
	    owl_messagelist_get_size(ml));
  }
  waddstr(sepwin, buff);

  foo=owl_view_get_filtname(v);
  if (strcmp(foo, "all")) wattroff(sepwin, A_REVERSE);
  waddstr(sepwin, " ");
  waddstr(sepwin, owl_view_get_filtname(v));
  waddstr(sepwin, " ");
  if (strcmp(foo, "all")) wattron(sepwin, A_REVERSE);

  if (owl_mainwin_is_curmsg_truncated(owl_global_get_mainwin(&g))) {
    getyx(sepwin, y, x);
    wmove(sepwin, y, x+2);
    wattron(sepwin, A_BOLD);
    waddstr(sepwin, " <truncated> ");
    wattroff(sepwin, A_BOLD);
  }

  i=owl_mainwin_get_last_msg(owl_global_get_mainwin(&g));
  if ((i != -1) &&
      (i < owl_view_get_size(v)-1)) {
    getyx(sepwin, y, x);
    wmove(sepwin, y, x+2);
    wattron(sepwin, A_BOLD);
    waddstr(sepwin, " <more> ");
    wattroff(sepwin, A_BOLD);
  }

  if (owl_global_get_rightshift(&g)>0) {
    getyx(sepwin, y, x);
    wmove(sepwin, y, x+2);
    snprintf(buff, 1024, " right: %i ", owl_global_get_rightshift(&g));
    waddstr(sepwin, buff);
  }

  if (owl_global_is_zaway(&g)) {
    getyx(sepwin, y, x);
    wmove(sepwin, y, x+2);
    wattron(sepwin, A_BOLD);
    wattroff(sepwin, A_REVERSE);
    waddstr(sepwin, " ZAWAY ");
    wattron(sepwin, A_REVERSE);
    wattroff(sepwin, A_BOLD);
  }

  if (owl_global_get_curmsg_vert_offset(&g)) {
    getyx(sepwin, y, x);
    wmove(sepwin, y, x+2);
    wattron(sepwin, A_BOLD);
    wattroff(sepwin, A_REVERSE);
    waddstr(sepwin, " SCROLL ");
    wattron(sepwin, A_REVERSE);
    wattroff(sepwin, A_BOLD);
  }
  
  if (in) {
    getyx(sepwin, y, x);
    wmove(sepwin, y, x+2);
    waddstr(sepwin, in);
  }

  appendtosepbar = owl_global_get_appendtosepbar(&g);
  if (appendtosepbar && *appendtosepbar) {
    getyx(sepwin, y, x);
    wmove(sepwin, y, x+2);
    waddstr(sepwin, " ");
    waddstr(sepwin, owl_global_get_appendtosepbar(&g));
    waddstr(sepwin, " ");
  }

  getyx(sepwin, y, x);
  wmove(sepwin, y, owl_global_get_cols(&g)-1);
    
  wattroff(sepwin, A_BOLD);
  wattroff(sepwin, A_REVERSE);
  wnoutrefresh(sepwin);
}


void pophandler_quit(int ch) {
  if (ch=='q') {
    owl_popwin_close(owl_global_get_popwin(&g));
  }
}

char **atokenize(char *buffer, char *sep, int *i) {
  /* each element of return must be freed by user */
  char **args;
  char *workbuff, *foo;
  int done=0, first=1, count=0;

  workbuff=owl_malloc(strlen(buffer)+1);
  memcpy(workbuff, buffer, strlen(buffer)+1);

  args=NULL;
  while (!done) {
    if (first) {
      first=0;
      foo=(char *)strtok(workbuff, sep);
    } else {
      foo=(char *)strtok(NULL, sep);
    }
    if (foo==NULL) {
      done=1;
    } else {
      args=(char **)owl_realloc(args, sizeof(char *) * (count+1));
      args[count]=owl_malloc(strlen(foo)+1);
      strcpy(args[count], foo);
      count++;
    }
  }
  *i=count;
  owl_free(workbuff);
  return(args);
}

/* skips n tokens and returns where that would be.
 * TODO: handle quotes more sanely. */
char *skiptokens(char *buff, int n) {
  int inquotes=0;
  while (*buff && n>0) {
      while (*buff == ' ') buff++;
      while (*buff && (inquotes || *buff != ' ')) { 
	if (*buff == '"' || *buff == '\'') inquotes=!inquotes;
	buff++;
      }
      while (*buff == ' ') buff++;
      n--;
  }
  return buff;
}

void atokenize_free(char **tok, int nels) {
  int i;
  for (i=0; i<nels; i++) {
    owl_free(tok[i]);
  }
  owl_free(tok);
}


void owl_parsefree(char **argv, int argc) {
  int i;

  if (!argv) return;
  
  for (i=0; i<argc; i++) {
    if (argv[i]) owl_free(argv[i]);
  }
  owl_free(argv);
}

char **owl_parseline(char *line, int *argc) {
  /* break a command line up into argv, argc.  The caller must free
     the returned values.  If there is an error argc will be set to
     -1, argv will be NULL and the caller does not need to free
     anything */

  char **argv;
  int i, len, between=1;
  char *curarg;
  char quote;

  argv=owl_malloc(sizeof(char *));
  len=strlen(line);
  curarg=owl_malloc(len+10);
  strcpy(curarg, "");
  quote='\0';
  *argc=0;
  for (i=0; i<len+1; i++) {
    /* find the first real character */
    if (between) {
      if (line[i]==' ' || line[i]=='\t' || line[i]=='\0') {
	continue;
      } else {
	between=0;
	i--;
	continue;
      }
    }

    /* deal with a quote character */
    if (line[i]=='"' || line[i]=="'"[0]) {
      /* if this type of quote is open, close it */
      if (quote==line[i]) {
	quote='\0';
	continue;
      }

      /* if no quoting is open then open with this */
      if (quote=='\0') {
	quote=line[i];
	continue;
      }

      /* if another type of quote is open then treat this as a literal */
      curarg[strlen(curarg)+1]='\0';
      curarg[strlen(curarg)]=line[i];
      continue;
    }

    /* if it's not a space or end of command, then use it */
    if (line[i]!=' ' && line[i]!='\t' && line[i]!='\n' && line[i]!='\0') {
      curarg[strlen(curarg)+1]='\0';
      curarg[strlen(curarg)]=line[i];
      continue;
    }

    /* otherwise, if we're not in quotes, add the whole argument */
    if (quote=='\0') {
      /* add the argument */
      argv=owl_realloc(argv, sizeof(char *)*((*argc)+1));
      argv[*argc]=owl_malloc(strlen(curarg)+2);
      strcpy(argv[*argc], curarg);
      *argc=*argc+1;
      strcpy(curarg, "");
      between=1;
      continue;
    }

    /* if it is a space and we're in quotes, then use it */
    curarg[strlen(curarg)+1]='\0';
    curarg[strlen(curarg)]=line[i];
  }

  /* check for unbalanced quotes */
  if (quote!='\0') {
    owl_parsefree(argv, *argc);
    *argc=-1;
    return(NULL);
  }

  return(argv);
}



int owl_util_find_trans(char *in, int len) {
  /* return the index of the last char before a change from the first
     one */
  int i;
  for (i=1; i<len; i++) {
    if (in[i] != in[0]) return(i-1);
  }
  return(i);
}


void downstr(char *foo) {
  int i;
  for (i=0; foo[i]!='\0'; i++) {
    foo[i]=tolower(foo[i]);
  }
}

char *stristr(char *a, char *b) {
  char *x, *y, *ret;

  if ((x=owl_strdup(a))==NULL) return(NULL);
  if ((y=owl_strdup(b))==NULL) return(NULL);
  downstr(x);
  downstr(y);
  ret=strstr(x, y);
  owl_free(x);
  owl_free(y);
  return(ret);
}

/* returns if a string is only whitespace */
int only_whitespace(char *s) {
  int i;
  for (i=0; s[i]; i++) {
    if (!isspace(s[i])) return(0);
  }
  return(1);
}

void *owl_malloc(size_t size) {
  return(malloc(size));
}

void owl_free(void *ptr) {
  free(ptr);
}

char *owl_strdup(const char *s1) {
  return(strdup(s1));
}

void *owl_realloc(void *ptr, size_t size) {
  return(realloc(ptr, size));
}

/* allocates memory and returns the string or null.
 * caller must free the string. 
 * from Linux sprintf man page. 
 */
char *owl_sprintf(const char *fmt, ...) {
  int n, size = 100;
  char *p;
  va_list ap;
  if ((p = owl_malloc (size)) == NULL)
    return NULL;
  while (1) {
    /* Try to print in the allocated space. */
    va_start(ap, fmt);
    n = vsnprintf (p, size, fmt, ap);
    va_end(ap);
    /* If that worked, return the string. */
    if (n > -1 && n < size)
      return p;
    /* Else try again with more space. */
    if (n > -1)    /* glibc 2.1 */
      size = n+1; /* precisely what is needed */
    else           /* glibc 2.0 */
      size *= 2;  /* twice the old size */
    if ((p = owl_realloc (p, size)) == NULL)
      return NULL;
  }
}

char *pretty_sender(char *in) {
  char *out, *ptr;
  
  /* the caller must free the return */
  out=owl_strdup(in);
  ptr=strchr(out, '@');
  if (ptr) {
    if (!strcasecmp(ptr+1, ZGetRealm())) {
      *ptr='\0';
    }
  }
  return(out);
}

char *long_sender(char *in) {
  char *ptr;

  /* the caller must free the return */
  if (NULL != (ptr=strchr(in, '@'))) {
    return owl_strdup(in);
  } else {
    return owl_sprintf("%s@%s", in, ZGetRealm());
  }
}
		  

char *owl_getquoting(char *line) {
  if (line[0]=='\0') return("'");
  if (strchr(line, '\'')) return("\"");
  if (strchr(line, '"')) return("'");
  if (strchr(line, ' ')) return("'");
  return("");
}

/* Caller must free returned string.
 * Returns a string with any occurances of 'from' replaced with 'to'.
 * Does not currently handle backslash quoting, but may in the future.
 */
char *owl_util_substitute(char *in, char *from, char *to) {
  char *out;
  int   outlen, tolen, fromlen, inpos=0, outpos=0;

  if (!*from) return owl_strdup(in);

  outlen = strlen(in)+1;
  tolen  = strlen(to);
  fromlen  = strlen(from);
  out = malloc(outlen);

  while (in[inpos]) {
    if (!strncmp(in+inpos, from, fromlen)) {
      outlen += tolen;
      out = owl_realloc(out, outlen);
      strcpy(out+outpos, to);
      inpos += fromlen;
      outpos += tolen;
    } else {
      out[outpos] = in[inpos];
      inpos++; outpos++;
    }
  }
  out[outpos] = '\0';
  return(out);
}


int owl_util_string_to_color(char *color) {
  if (!strcasecmp(color, "black")) {
    return(OWL_COLOR_BLACK);
  } else if (!strcasecmp(color, "red")) {
    return(OWL_COLOR_RED);
  } else if (!strcasecmp(color, "green")) {
    return(OWL_COLOR_GREEN);
  } else if (!strcasecmp(color, "yellow")) {
    return(OWL_COLOR_YELLOW);
  } else if (!strcasecmp(color, "blue")) {
    return(OWL_COLOR_BLUE);
  } else if (!strcasecmp(color, "magenta")) {
    return(OWL_COLOR_MAGENTA);
  } else if (!strcasecmp(color, "cyan")) {
    return(OWL_COLOR_CYAN);
  } else if (!strcasecmp(color, "white")) {
    return(OWL_COLOR_WHITE);
  } else if (!strcasecmp(color, "default")) {
    return(OWL_COLOR_DEFAULT);
  }
  return(OWL_COLOR_DEFAULT);
}

char *owl_util_color_to_string(int color) {
  if (color==OWL_COLOR_BLACK)   return("black");
  if (color==OWL_COLOR_RED)     return("red");
  if (color==OWL_COLOR_GREEN)   return("green");
  if (color==OWL_COLOR_YELLOW)  return("yellow");
  if (color==OWL_COLOR_BLUE)    return("blue");
  if (color==OWL_COLOR_MAGENTA) return("magenta");
  if (color==OWL_COLOR_CYAN)    return("cyan");
  if (color==OWL_COLOR_WHITE)   return("white");
  if (color==OWL_COLOR_DEFAULT) return("default");
  return("Unknown color");
}


/**************************************************************************/
/************************* REGRESSION TESTS *******************************/
/**************************************************************************/

#ifdef OWL_INCLUDE_REG_TESTS

#define FAIL_UNLESS(desc,pred) printf("\t%-4s: %s\n", (pred)?"ok":(numfailed++,"FAIL"), desc)

int owl_util_regtest(void) {
  int numfailed=0;

  printf("BEGIN testing owl_util\n");

  FAIL_UNLESS("owl_util_substitute 1",
	      !strcmp("foo", owl_util_substitute("foo", "", "Y")));
  FAIL_UNLESS("owl_util_substitute 2",
	      !strcmp("fYZYZ", owl_util_substitute("foo", "o", "YZ")));
  FAIL_UNLESS("owl_util_substitute 3",
	      !strcmp("foo", owl_util_substitute("fYZYZ", "YZ", "o")));
  FAIL_UNLESS("owl_util_substitute 4",
	      !strcmp("/u/foo/meep", owl_util_substitute("~/meep", "~", "/u/foo")));

  FAIL_UNLESS("skiptokens 1", 
	      !strcmp("bar quux", skiptokens("foo bar quux", 1)));
  FAIL_UNLESS("skiptokens 2", 
	      !strcmp("meep", skiptokens("foo 'bar quux' meep", 2)));

  if (numfailed) printf("*** WARNING: failures encountered with owl_util\n");
  printf("END testing owl_util (%d failures)\n", numfailed);
  return(numfailed);
}

#endif /* OWL_INCLUDE_REG_TESTS */
