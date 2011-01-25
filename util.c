#include "owl.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <stdarg.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>

const char *skiptokens(const char *buff, int n) {
  /* skips n tokens and returns where that would be. */
  char quote = 0;
  while (*buff && n>0) {
      while (*buff == ' ') buff++;
      while (*buff && (quote || *buff != ' ')) {
        if(quote) {
          if(*buff == quote) quote = 0;
        } else if(*buff == '"' || *buff == '\'') {
          quote = *buff;
        }
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
char *owl_util_makepath(const char *in)
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

void owl_parse_delete(char **argv, int argc)
{
  g_strfreev(argv);
}

char **owl_parseline(const char *line, int *argc)
{
  /* break a command line up into argv, argc.  The caller must free
     the returned values.  If there is an error argc will be set to
     -1, argv will be NULL and the caller does not need to free
     anything. The returned vector is NULL-terminated. */

  GPtrArray *argv;
  int i, len, between=1;
  char *curarg;
  char quote;

  argv = g_ptr_array_new_with_free_func(owl_free);
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
      g_ptr_array_add(argv, owl_strdup(curarg));
      strcpy(curarg, "");
      between=1;
      continue;
    }

    /* if it is a space and we're in quotes, then use it */
    curarg[strlen(curarg)+1]='\0';
    curarg[strlen(curarg)]=line[i];
  }

  *argc = argv->len;
  g_ptr_array_add(argv, NULL);
  owl_free(curarg);

  /* check for unbalanced quotes */
  if (quote!='\0') {
    g_ptr_array_free(argv, true);
    *argc = -1;
    return(NULL);
  }

  return (char**)g_ptr_array_free(argv, false);
}

/* Appends a quoted version of arg suitable for placing in a
 * command-line to a GString. Does not append a space. */
void owl_string_append_quoted_arg(GString *buf, const char *arg)
{
  const char *argp;
  if (arg[0] == '\0') {
    /* Quote the empty string. */
    g_string_append(buf, "''");
  } else if (arg[strcspn(arg, "'\" \n\t")] == '\0') {
    /* If there are no nasty characters, return as-is. */
    g_string_append(buf, arg);
  } else if (!strchr(arg, '\'')) {
    /* Single-quote if possible. */
    g_string_append_c(buf, '\'');
    g_string_append(buf, arg);
    g_string_append_c(buf, '\'');
  } else {
    /* Nasty case: double-quote, but change all internal "s to "'"'"
     * so that they are single-quoted because we're too cool for
     * backslashes.
     */
    g_string_append_c(buf, '"');
    for (argp = arg; *argp; argp++) {
      if (*argp == '"')
	g_string_append(buf, "\"'\"'\"");
      else
	g_string_append_c(buf, *argp);
    }
    g_string_append_c(buf, '"');
  }
}

/*
 * Appends 'tmpl' to 'buf', replacing any instances of '%q' with arguments from
 * the varargs provided, quoting them to be safe for placing in a barnowl
 * command line.
 */
void owl_string_appendf_quoted(GString *buf, const char *tmpl, ...)
{
  va_list ap;
  va_start(ap, tmpl);
  owl_string_vappendf_quoted(buf, tmpl, ap);
  va_end(ap);
}

void owl_string_vappendf_quoted(GString *buf, const char *tmpl, va_list ap)
{
  const char *p = tmpl, *last = tmpl;
  while (true) {
    p = strchr(p, '%');
    if (p == NULL) break;
    if (*(p+1) != 'q') {
      p++;
      if (*p) p++;
      continue;
    }
    g_string_append_len(buf, last, p - last);
    owl_string_append_quoted_arg(buf, va_arg(ap, char *));
    p += 2; last = p;
  }

  g_string_append(buf, last);
}

char *owl_string_build_quoted(const char *tmpl, ...)
{
  GString *buf = g_string_new("");
  va_list ap;
  va_start(ap, tmpl);
  owl_string_vappendf_quoted(buf, tmpl, ap);
  va_end(ap);
  return g_string_free(buf, false);  
}

/* Returns a quoted version of arg suitable for placing in a
 * command-line. Result should be freed with owl_free. */
char *owl_arg_quote(const char *arg)
{
  GString *buf = g_string_new("");;
  owl_string_append_quoted_arg(buf, arg);
  return g_string_free(buf, false);
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

/* These are in order of their value in owl.h */
static const struct {
  int number;
  const char *name;
} color_map[] = {
  {OWL_COLOR_INVALID, "invalid"},
  {OWL_COLOR_DEFAULT, "default"},
  {OWL_COLOR_BLACK, "black"},
  {OWL_COLOR_RED, "red"},
  {OWL_COLOR_GREEN, "green"},
  {OWL_COLOR_YELLOW,"yellow"},
  {OWL_COLOR_BLUE, "blue"},
  {OWL_COLOR_MAGENTA, "magenta"},
  {OWL_COLOR_CYAN, "cyan"},
  {OWL_COLOR_WHITE, "white"},
};

/* Return the owl color associated with the named color.  Return -1
 * if the named color is not available
 */
int owl_util_string_to_color(const char *color)
{
  int c, i;
  char *p;

  for (i = 0; i < (sizeof(color_map)/sizeof(color_map[0])); i++)
    if (strcasecmp(color, color_map[i].name) == 0)
      return color_map[i].number;

  c = strtol(color, &p, 10);
  if (p != color && c >= -1 && c < COLORS) {
    return(c);
  }
  return(OWL_COLOR_INVALID);
}

/* Return a string name of the given owl color */
const char *owl_util_color_to_string(int color)
{
  if (color >= OWL_COLOR_INVALID && color <= OWL_COLOR_WHITE)
    return color_map[color - OWL_COLOR_INVALID].name;
  return("Unknown color");
}

/* Get the default tty name.  Caller must free the return */
char *owl_util_get_default_tty(void)
{
  const char *tmp;
  char *out;

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

/* strip leading and trailing new lines.  Caller must free the
 * return.
 */
char *owl_util_stripnewlines(const char *in)
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


/* If filename is a link, recursively resolve symlinks.  Otherwise, return the filename
 * unchanged.  On error, call owl_function_error and return NULL.
 *
 * This function assumes that filename eventually resolves to an acutal file.
 * If you want to check this, you should stat() the file first.
 *
 * The caller of this function is responsible for freeing the return value.
 *
 * Error conditions are the same as g_file_read_link.
 */
gchar *owl_util_recursive_resolve_link(const char *filename)
{
  gchar *last_path = g_strdup(filename);
  GError *err = NULL;

  while (g_file_test(last_path, G_FILE_TEST_IS_SYMLINK)) {
    gchar *link_path = g_file_read_link(last_path, &err);
    if (link_path == NULL) {
      owl_function_error("Cannot resolve symlink %s: %s",
			 last_path, err->message);
      g_error_free(err);
      g_free(last_path);
      return NULL;
    }

    /* Deal with obnoxious relative paths. If we really care, all this
     * is racy. Whatever. */
    if (!g_path_is_absolute(link_path)) {
      char *last_dir = g_path_get_dirname(last_path);
      char *tmp = g_build_path(G_DIR_SEPARATOR_S,
			       last_dir,
			       link_path,
			       NULL);
      g_free(last_dir);
      g_free(link_path);
      link_path = tmp;
    }

    g_free(last_path);
    last_path = link_path;
  }
  return last_path;
}

/* Delete all lines matching "line" from the named file.  If no such
 * line is found the file is left intact.  If backup==1 then leave a
 * backup file containing the original contents.  The match is
 * case-insensitive.
 *
 * Returns the number of lines removed on success.  Returns -1 on failure.
 */
int owl_util_file_deleteline(const char *filename, const char *line, int backup)
{
  char *backupfile, *newfile, *buf = NULL;
  gchar *actual_filename; /* gchar; we need to g_free it */
  FILE *old, *new;
  struct stat st;
  int numremoved = 0;

  if ((old = fopen(filename, "r")) == NULL) {
    owl_function_error("Cannot open %s (for reading): %s",
		       filename, strerror(errno));
    return -1;
  }

  if (fstat(fileno(old), &st) != 0) {
    owl_function_error("Cannot stat %s: %s", filename, strerror(errno));
    return -1;
  }

  /* resolve symlinks, because link() fails on symlinks, at least on AFS */
  actual_filename = owl_util_recursive_resolve_link(filename);
  if (actual_filename == NULL)
    return -1; /* resolving the symlink failed, but we already logged this error */

  newfile = owl_sprintf("%s.new", actual_filename);
  if ((new = fopen(newfile, "w")) == NULL) {
    owl_function_error("Cannot open %s (for writing): %s",
		       actual_filename, strerror(errno));
    owl_free(newfile);
    fclose(old);
    free(actual_filename);
    return -1;
  }

  if (fchmod(fileno(new), st.st_mode & 0777) != 0) {
    owl_function_error("Cannot set permissions on %s: %s",
		       actual_filename, strerror(errno));
    unlink(newfile);
    fclose(new);
    owl_free(newfile);
    fclose(old);
    free(actual_filename);
    return -1;
  }

  while (owl_getline_chomp(&buf, old))
    if (strcasecmp(buf, line) != 0)
      fprintf(new, "%s\n", buf);
    else
      numremoved++;
  owl_free(buf);

  fclose(new);
  fclose(old);

  if (backup) {
    backupfile = owl_sprintf("%s.backup", actual_filename);
    unlink(backupfile);
    if (link(actual_filename, backupfile) != 0) {
      owl_function_error("Cannot link %s: %s", backupfile, strerror(errno));
      owl_free(backupfile);
      unlink(newfile);
      owl_free(newfile);
      return -1;
    }
    owl_free(backupfile);
  }

  if (rename(newfile, actual_filename) != 0) {
    owl_function_error("Cannot move %s to %s: %s",
		       newfile, actual_filename, strerror(errno));
    numremoved = -1;
  }

  unlink(newfile);
  owl_free(newfile);

  g_free(actual_filename);

  return numremoved;
}

/* Return the base class or instance from a zephyr class, by removing
   leading `un' or trailing `.d'.
   The caller is responsible for freeing the allocated string.
*/
char * owl_util_baseclass(const char * class)
{
  char *start, *end;

  while(!strncmp(class, "un", 2)) {
    class += 2;
  }

  start = owl_strdup(class);
  end = start + strlen(start) - 1;
  while(end > start && *end == 'd' && *(end-1) == '.') {
    end -= 2;
  }
  *(end + 1) = 0;

  return start;
}

const char * owl_get_datadir(void)
{
  const char * datadir = getenv("BARNOWL_DATA_DIR");
  if(datadir != NULL)
    return datadir;
  return DATADIR;
}

const char * owl_get_bindir(void)
{
  const char * bindir = getenv("BARNOWL_BIN_DIR");
  if(bindir != NULL)
    return bindir;
  return BINDIR;
}

/* Strips format characters from a valid utf-8 string. Returns the
   empty string if 'in' does not validate. */
char * owl_strip_format_chars(const char *in)
{
  char *r;
  if (g_utf8_validate(in, -1, NULL)) {
    const char *s, *p;
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
	while (owl_fmtext_is_format_char(g_utf8_get_char(p))) {
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
char * owl_validate_or_convert(const char *in)
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
/*
 * Validate 'in' as UTF-8, and either return a copy of it, or an empty
 * string if it is invalid utf-8.
 */
char * owl_validate_utf8(const char *in)
{
  char *out;
  if (g_utf8_validate(in, -1, NULL)) {
    out = owl_strdup(in);
  } else {
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

char *owl_escape_highbit(const char *str)
{
  GString *out = g_string_new("");
  unsigned char c;
  while((c = (*str++))) {
    if(c == '\\') {
      g_string_append(out, "\\\\");
    } else if(c & 0x80) {
      g_string_append_printf(out, "\\x%02x", (int)c);
    } else {
      g_string_append_c(out, c);
    }
  }
  return g_string_free(out, 0);
}

/* innards of owl_getline{,_chomp} below */
static int owl_getline_internal(char **s, FILE *fp, int newline)
{
  int size = 0;
  int target = 0;
  int count = 0;
  int c;

  while (1) {
    c = getc(fp);
    if ((target + 1) > size) {
      size += BUFSIZ;
      *s = owl_realloc(*s, size);
    }
    if (c == EOF)
      break;
    count++;
    if (c != '\n' || newline)
	(*s)[target++] = c;
    if (c == '\n')
      break;
  }
  (*s)[target] = 0;

  return count;
}

/* Read a line from fp, allocating memory to hold it, returning the number of
 * byte read.  *s should either be NULL or a pointer to memory allocated with
 * owl_malloc; it will be owl_realloc'd as appropriate.  The caller must
 * eventually free it.  (This is roughly the interface of getline in the gnu
 * libc).
 *
 * The final newline will be included if it's there.
 */
int owl_getline(char **s, FILE *fp)
{
  return owl_getline_internal(s, fp, 1);
}

/* As above, but omitting the final newline */
int owl_getline_chomp(char **s, FILE *fp)
{
  return owl_getline_internal(s, fp, 0);
}

/* Read the rest of the input available in fp into a string. */
char *owl_slurp(FILE *fp)
{
  char *buf = NULL;
  char *p;
  int size = 0;
  int count;

  while (1) {
    buf = owl_realloc(buf, size + BUFSIZ);
    p = &buf[size];
    size += BUFSIZ;

    if ((count = fread(p, 1, BUFSIZ, fp)) < BUFSIZ)
      break;
  }
  p[count] = 0;

  return buf;
}

gulong owl_dirty_window_on_signal(owl_window *w, gpointer sender, const gchar *detailed_signal)
{
  return owl_signal_connect_object(sender, detailed_signal, G_CALLBACK(owl_window_dirty), w, G_CONNECT_SWAPPED);
}

typedef struct { /*noproto*/
  GObject  *sender;
  gulong    signal_id;
} SignalData;

static void _closure_invalidated(gpointer data, GClosure *closure);

/*
 * GObject's g_signal_connect_object has a documented bug. This function is
 * identical except it does not leak the signal handler.
 */
gulong owl_signal_connect_object(gpointer sender, const gchar *detailed_signal, GCallback c_handler, gpointer receiver, GConnectFlags connect_flags)
{
  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE (sender), 0);
  g_return_val_if_fail (detailed_signal != NULL, 0);
  g_return_val_if_fail (c_handler != NULL, 0);

  if (receiver) {
    SignalData *sdata;
    GClosure *closure;
    gulong signal_id;

    g_return_val_if_fail (G_IS_OBJECT (receiver), 0);

    closure = ((connect_flags & G_CONNECT_SWAPPED) ? g_cclosure_new_object_swap : g_cclosure_new_object) (c_handler, receiver);
    signal_id = g_signal_connect_closure (sender, detailed_signal, closure, connect_flags & G_CONNECT_AFTER);

    /* Register the missing hooks */
    sdata = g_slice_new0(SignalData);
    sdata->sender = sender;
    sdata->signal_id = signal_id;

    g_closure_add_invalidate_notifier(closure, sdata, _closure_invalidated);

    return signal_id;
  } else {
    return g_signal_connect_data(sender, detailed_signal, c_handler, NULL, NULL, connect_flags);
  }
}

/*
 * There are three ways the signal could come to an end:
 * 
 * 1. The user explicitly disconnects it with the returned signal_id.
 *    - In that case, the disconnection unref's the closure, causing it
 *      to first be invalidated. The handler's already disconnected, so
 *      we have no work to do.
 * 2. The sender gets destroyed.
 *    - GObject will disconnect each signal which then goes into the above
 *      case. Our handler does no work.
 * 3. The receiver gets destroyed.
 *    - The GClosure was created by g_cclosure_new_object_{,swap} which gets
 *      invalidated when the receiver is destroyed. We then follow through case 1
 *      again, but *this* time, the handler has not been disconnected. We then
 *      clean up ourselves.
 *
 * We can't actually hook into this process earlier with weakrefs as GObject
 * will, on object dispose, first disconnect signals, then invalidate closures,
 * and notify weakrefs last.
 */
static void _closure_invalidated(gpointer data, GClosure *closure)
{
  SignalData *sdata = data;
  if (g_signal_handler_is_connected(sdata->sender, sdata->signal_id)) {
    g_signal_handler_disconnect(sdata->sender, sdata->signal_id);
  }
  g_slice_free(SignalData, sdata);
}

