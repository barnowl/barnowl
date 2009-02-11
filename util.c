#include "owl.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <pwd.h>

static const char fileIdent[] = "$Id$";

void sepbar(char *in)
{
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
  if (owl_global_is_fancylines(&g)) {
    whline(sepwin, ACS_HLINE, owl_global_get_cols(&g));
  } else {
    whline(sepwin, '-', owl_global_get_cols(&g));
  }

  if (owl_global_is_sepbar_disable(&g)) {
    getyx(sepwin, y, x);
    wmove(sepwin, y, owl_global_get_cols(&g)-1);
    return;
  }

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
  if (strcmp(foo, owl_global_get_view_home(&g))) wattroff(sepwin, A_REVERSE);
  waddstr(sepwin, " ");
  waddstr(sepwin, owl_view_get_filtname(v));
  waddstr(sepwin, " ");
  if (strcmp(foo, owl_global_get_view_home(&g))) wattron(sepwin, A_REVERSE);

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

  if (owl_global_is_zaway(&g) || owl_global_is_aaway(&g)) {
    getyx(sepwin, y, x);
    wmove(sepwin, y, x+2);
    wattron(sepwin, A_BOLD);
    wattroff(sepwin, A_REVERSE);
    if (owl_global_is_zaway(&g) && owl_global_is_aaway(&g)) {
      waddstr(sepwin, " AWAY ");
    } else if (owl_global_is_zaway(&g)) {
      waddstr(sepwin, " Z-AWAY ");
    } else if (owl_global_is_aaway(&g)) {
      waddstr(sepwin, " A-AWAY ");
    }
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

char **atokenize(char *buffer, char *sep, int *i)
{
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

char *skiptokens(char *buff, int n) {
  /* skips n tokens and returns where that would be.
   * TODO: handle quotes more sanely. */
  
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

/* Return a "nice" version of the path.  Tilde expansion is done, and
 * duplicate slashes are removed.  Caller must free the return.
 */
char *owl_util_makepath(char *in)
{
  int i, j, x;
  char *out, user[MAXPATHLEN];
  struct passwd *pw;

  out=owl_malloc(MAXPATHLEN+1);
  out[0]='\0';
  j=strlen(in);
  x=0;
  for (i=0; i<j; i++) {
    if (in[i]=='~') {
      if ( (i==(j-1)) ||          /* last character */
	   (in[i+1]=='/') ) {     /* ~/ */
	/* use my homedir */
	pw=getpwuid(getuid());
	if (!pw) {
	  out[x]=in[i];
	} else {
	  out[x]='\0';
	  strcat(out, pw->pw_dir);
	  x+=strlen(pw->pw_dir);
	}
      } else {
	/* another user homedir */
	int a, b;
	b=0;
	for (a=i+1; i<j; a++) {
	  if (in[a]==' ' || in[a]=='/') {
	    break;
	  } else {
	    user[b]=in[a];
	    i++;
	    b++;
	  }
	}
	user[b]='\0';
	pw=getpwnam(user);
	if (!pw) {
	  out[x]=in[i];
	} else {
	  out[x]='\0';
	  strcat(out, pw->pw_dir);
	  x+=strlen(pw->pw_dir);
	}
      }
    } else if (in[i]=='/') {
      /* check for a double / */
      if (i<(j-1) && (in[i+1]=='/')) {
	/* do nothing */
      } else {
	out[x]=in[i];
	x++;
      }
    } else {
      out[x]=in[i];
      x++;
    }
  }
  out[x]='\0';
  return(out);
}

void atokenize_free(char **tok, int nels)
{
  int i;
  for (i=0; i<nels; i++) {
    owl_free(tok[i]);
  }
  owl_free(tok);
}


void owl_parsefree(char **argv, int argc)
{
  int i;

  if (!argv) return;
  
  for (i=0; i<argc; i++) {
    if (argv[i]) owl_free(argv[i]);
  }
  owl_free(argv);
}

char **owl_parseline(char *line, int *argc)
{
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

  owl_free(curarg);

  /* check for unbalanced quotes */
  if (quote!='\0') {
    owl_parsefree(argv, *argc);
    *argc=-1;
    return(NULL);
  }

  return(argv);
}

/* caller must free the return */
char *owl_util_minutes_to_timestr(int in)
{
  int days, hours;
  long run;
  char *out;

  run=in;

  days=run/1440;
  run-=days*1440;
  hours=run/60;
  run-=hours*60;

  if (days>0) {
    out=owl_sprintf("%i d %2.2i:%2.2li", days, hours, run);
  } else {
    out=owl_sprintf("    %2.2i:%2.2li", hours, run);
  }
  return(out);
}

/* hooks for doing memory allocation et. al. in owl */

void *owl_malloc(size_t size)
{
  return(g_malloc(size));
}

void owl_free(void *ptr)
{
  g_free(ptr);
}

char *owl_strdup(const char *s1)
{
  return(g_strdup(s1));
}

void *owl_realloc(void *ptr, size_t size)
{
  return(g_realloc(ptr, size));
}

/* allocates memory and returns the string or null.
 * caller must free the string. 
 */
char *owl_sprintf(const char *fmt, ...)
{
  va_list ap;
  char *ret = NULL;
  va_start(ap, fmt);
  ret = g_strdup_vprintf(fmt, ap);
  va_end(ap);
  return ret;
}


/* Return the owl color associated with the named color.  Return -1
 * if the named color is not available
 */
int owl_util_string_to_color(char *color)
{
  int c;
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
  c = atoi(color);
  if (c >= -1 && c < COLORS) {
    return(c);
  }
  return(OWL_COLOR_INVALID);
}

/* Return a string name of the given owl color */
char *owl_util_color_to_string(int color)
{
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

/* Get the default tty name.  Caller must free the return */
char *owl_util_get_default_tty()
{
  char *out, *tmp;

  if (getenv("DISPLAY")) {
    out=owl_strdup(getenv("DISPLAY"));
  } else if ((tmp=ttyname(fileno(stdout)))!=NULL) {
    out=owl_strdup(tmp);
    if (!strncmp(out, "/dev/", 5)) {
      owl_free(out);
      out=owl_strdup(tmp+5);
    }
  } else {
    out=owl_strdup("unknown");
  }
  return(out);
}


/* Animation hack */
void owl_hack_animate()
{
  owl_messagelist *ml;
  owl_message *m;
  owl_fmtext *fm;
  char *text, *ptr;
  int place;

  /* grab the first message and make sure its id is 0 */
  ml=owl_global_get_msglist(&g);
  m=owl_messagelist_get_element(ml, 0);
  if (!m) return;
  if (owl_message_get_id(m)!=0) return;

  fm=owl_message_get_fmtext(m);
  text=owl_fmtext_get_text(fm);

  ptr=strstr(text, "OvO");
  if (ptr) {
    place=ptr-text;
    owl_fmtext_set_char(fm, place, '-');
    owl_fmtext_set_char(fm, place+2, '-');

    owl_mainwin_redisplay(owl_global_get_mainwin(&g));
    if (owl_popwin_is_active(owl_global_get_popwin(&g))) {
      owl_popwin_refresh(owl_global_get_popwin(&g));
      /* TODO: this is a broken kludge */
      if (owl_global_get_viewwin(&g)) {
	owl_viewwin_redisplay(owl_global_get_viewwin(&g), 0);
      }
    }
    owl_global_set_needrefresh(&g);
    return;
  }

  ptr=strstr(text, "-v-");
  if (ptr) {
    place=ptr-text;
    owl_fmtext_set_char(fm, place, 'O');
    owl_fmtext_set_char(fm, place+2, 'O');

    owl_mainwin_redisplay(owl_global_get_mainwin(&g));
    if (owl_popwin_is_active(owl_global_get_popwin(&g))) {
      owl_popwin_refresh(owl_global_get_popwin(&g));
      /* TODO: this is a broken kludge */
      if (owl_global_get_viewwin(&g)) {
	owl_viewwin_redisplay(owl_global_get_viewwin(&g), 0);
      }
    }
    owl_global_set_needrefresh(&g);
    return;
  }
}

/* strip leading and trailing new lines.  Caller must free the
 * return.
 */
char *owl_util_stripnewlines(char *in)
{
  
  char  *tmp, *ptr1, *ptr2, *out;

  ptr1=tmp=owl_strdup(in);
  while (ptr1[0]=='\n') {
    ptr1++;
  }
  ptr2=ptr1+strlen(ptr1)-1;
  while (ptr2>ptr1 && ptr2[0]=='\n') {
    ptr2[0]='\0';
    ptr2--;
  }

  out=owl_strdup(ptr1);
  owl_free(tmp);
  return(out);
}

/* Delete the line matching "line" from the named file.  If no such
 * line is found the file is left intact.  If backup==1 then create a
 * backupfile containing the original contents.  This is an
 * inefficient impelementation which reads the entire file into
 * memory.
 */
void owl_util_file_deleteline(char *filename, char *line, int backup)
{
  char buff[LINE], *text;
  char *backupfilename="";
  FILE *file, *backupfile=NULL;
  int size, newline;

  /* open the file for reading */
  file=fopen(filename, "r");
  if (!file) {
    owl_function_error("Error opening file %s", filename);
    return;
  }

  /* open the backup file for writing */
  if (backup) {
    backupfilename=owl_sprintf("%s.backup", filename);
    backupfile=fopen(backupfilename, "w");
    if (!backupfile) {
      owl_function_error("Error opening file %s for writing", backupfilename);
      owl_free(backupfilename);
      fclose(file);
      return;
    }
  }

  /* we'll read the entire file into memory, minus the line we don't want and
   * and at the same time create the backup file if necessary
   */
  text=owl_malloc(LINE);
  strcpy(text, "");
  size=LINE;
  while (fgets(buff, LINE, file)!=NULL) {
    /* strip the newline */
    newline=0;
    if (buff[strlen(buff)-1]=='\n') {
      buff[strlen(buff)-1]='\0';
      newline=1;
    }
    
    /* if we don't match the line, add to saved text in memory */
    if (strcasecmp(buff, line)) {
      size+=LINE;
      text=owl_realloc(text, size);
      strcat(text, buff);
      if (newline) strcat(text, "\n");
    }

    /* write to backupfile if necessary */
    if (backup) {
      fputs(buff, backupfile);
      if (newline) fputs("\n", backupfile);
    }
  }
  if (backup) fclose(backupfile);
  fclose(file);

  /* now rewrite the original file from memory */
  file=fopen(filename, "w");
  if (!file) {
    owl_function_error("WARNING: Error opening %s for writing.  Use %s to restore.", filename, backupfilename);
    owl_function_beep();
  } else {
    fputs(text, file);
    fclose(file);
  }

  if (backup)
    owl_free(backupfilename);
  owl_free(text);
}

int owl_util_max(int a, int b)
{
  if (a>b) return(a);
  return(b);
}

int owl_util_min(int a, int b)
{
  if (a<b) return(a);
  return(b);
}

/* Return the base class or instance from a zephyr class, by removing
   leading `un' or trailing `.d'.
   The caller is responsible for freeing the allocated string.
*/
char * owl_util_baseclass(char * class)
{
  char *start, *end;

  start = class;
  while(!strncmp(start, "un", 2)) {
    start += 2;
  }

  start = owl_strdup(start);
  end = start + strlen(start) - 1;
  while(end > start && *end == 'd' && *(end-1) == '.') {
    end -= 2;
  }
  *(end + 1) = 0;

  return start;
}

char * owl_get_datadir()
{
  char * datadir = getenv("BARNOWL_DATA_DIR");
  if(datadir != NULL)
    return strchr(datadir, '=') + 1;
  return DATADIR;
}

/* Strips format characters from a valid utf-8 string. Returns the
   empty string if 'in' does not validate. */
char * owl_strip_format_chars(char *in)
{
  char *r;
  if (g_utf8_validate(in, -1, NULL)) {
    char *s, *p;
    r = owl_malloc(strlen(in)+1);
    r[0] = '\0';
    s = in;
    p = strchr(s, OWL_FMTEXT_UC_STARTBYTE_UTF8);
    while(p) {
      /* If it's a format character, copy up to it, and skip all
	 immediately following format characters. */
      if (owl_fmtext_is_format_char(g_utf8_get_char(p))) {
	strncat(r, s, p-s);
	p = g_utf8_next_char(p);
	while (p && owl_fmtext_is_format_char(g_utf8_get_char(p))) {
	  p = g_utf8_next_char(p);
	}
	s = p;
	p = strchr(s, OWL_FMTEXT_UC_STARTBYTE_UTF8);
      }
      else {
	p = strchr(p+1, OWL_FMTEXT_UC_STARTBYTE_UTF8);
      }
    }
    if (s) strcat(r,s);
  }
  else {
    r = owl_strdup("");
  }
  return r;
}

/* If in is not UTF-8, convert from ISO-8859-1. We may want to allow
 * the caller to specify an alternative in the future. We also strip
 * out characters in Unicode Plane 16, as we use that plane internally
 * for formatting.
 */
char * owl_validate_or_convert(char *in)
{
  if (g_utf8_validate(in, -1, NULL)) {
    return owl_strip_format_chars(in);
  }
  else {
    return g_convert(in, -1,
		     "UTF-8", "ISO-8859-1",
		     NULL, NULL, NULL);
  }
}
/* Attempts to convert 'in' to ISO-8859-1. Returns that if possible,
   else returns UTF-8.
 */
char * owl_get_iso_8859_1_if_possible(char *in)
{
  char *out;
  if (g_utf8_validate(in, -1, NULL)) {
    out = g_convert(in, -1,
		    "ISO-8859-1", "UTF-8",
		    NULL, NULL, NULL);
    if (!out) {
      out = owl_strdup(in);
    }
  }
  else {
    out = owl_strdup("");
  }
  return out;
}

/* This is based on _extract() and _isCJ() from perl's Text::WrapI18N */
int owl_util_can_break_after(gunichar c)
{
  
  if (c == ' ') return 1;
  if (c >= 0x3000 && c <= 0x312f) {
    /* CJK punctuations, Hiragana, Katakana, Bopomofo */
    if (c == 0x300a || c == 0x300c || c == 0x300e ||
        c == 0x3010 || c == 0x3014 || c == 0x3016 ||
        c == 0x3018 || c == 0x301a)
      return 0;
    return 1;
  }
  if (c >= 0x31a0 && c <= 0x31bf) {return 1;}  /* Bopomofo */
  if (c >= 0x31f0 && c <= 0x31ff) {return 1;}  /* Katakana extension */
  if (c >= 0x3400 && c <= 0x9fff) {return 1;}  /* Han Ideogram */
  if (c >= 0xf900 && c <= 0xfaff) {return 1;}  /* Han Ideogram */
  if (c >= 0x20000 && c <= 0x2ffff) {return 1;}  /* Han Ideogram */
  return 0;
}

/**************************************************************************/
/************************* REGRESSION TESTS *******************************/
/**************************************************************************/

#ifdef OWL_INCLUDE_REG_TESTS

#include "test.h"

int owl_util_regtest(void)
{
  int numfailed=0;

  printf("# BEGIN testing owl_util\n");

  FAIL_UNLESS("owl_util_substitute 1",
	      !strcmp("foo", owl_text_substitute("foo", "", "Y")));
  FAIL_UNLESS("owl_text_substitute 2",
	      !strcmp("fYZYZ", owl_text_substitute("foo", "o", "YZ")));
  FAIL_UNLESS("owl_text_substitute 3",
	      !strcmp("foo", owl_text_substitute("fYZYZ", "YZ", "o")));
  FAIL_UNLESS("owl_text_substitute 4",
	      !strcmp("/u/foo/meep", owl_text_substitute("~/meep", "~", "/u/foo")));

  FAIL_UNLESS("skiptokens 1", 
	      !strcmp("bar quux", skiptokens("foo bar quux", 1)));
  FAIL_UNLESS("skiptokens 2", 
	      !strcmp("meep", skiptokens("foo 'bar quux' meep", 2)));

  /* if (numfailed) printf("*** WARNING: failures encountered with owl_util\n"); */
  printf("# END testing owl_util (%d failures)\n", numfailed);
  return(numfailed);
}

#endif /* OWL_INCLUDE_REG_TESTS */
