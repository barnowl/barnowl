#include "owl.h"
#include <stdlib.h>
#include <string.h>

/* initialize an fmtext with no data */
void owl_fmtext_init_null(owl_fmtext *f)
{
  f->buff = g_string_new("");
}

/* Clear the data from an fmtext, but don't deallocate memory. This
   fmtext can then be appended to again. */
void owl_fmtext_clear(owl_fmtext *f)
{
  g_string_truncate(f->buff, 0);
}

int owl_fmtext_is_format_char(gunichar c)
{
  if ((c & ~OWL_FMTEXT_UC_ATTR_MASK) == OWL_FMTEXT_UC_ATTR) return 1;
  if ((c & ~(OWL_FMTEXT_UC_ALLCOLOR_MASK)) == OWL_FMTEXT_UC_COLOR_BASE) return 1;
  return 0;
}
/* append text to the end of 'f' with attribute 'attr' and color
 * 'color'
 */
void owl_fmtext_append_attr(owl_fmtext *f, const char *text, char attr, short fgcolor, short bgcolor)
{
  int a = 0, fg = 0, bg = 0;
  
  if (attr != OWL_FMTEXT_ATTR_NONE) a=1;
  if (fgcolor != OWL_COLOR_DEFAULT) fg=1;
  if (bgcolor != OWL_COLOR_DEFAULT) bg=1;

  /* Set attributes */
  if (a)
    g_string_append_unichar(f->buff, OWL_FMTEXT_UC_ATTR | attr);
  if (fg)
    g_string_append_unichar(f->buff, OWL_FMTEXT_UC_FGCOLOR | fgcolor);
  if (bg)
    g_string_append_unichar(f->buff, OWL_FMTEXT_UC_BGCOLOR | bgcolor);

  g_string_append(f->buff, text);

  /* Reset attributes */
  if (bg) g_string_append_unichar(f->buff, OWL_FMTEXT_UC_BGDEFAULT);
  if (fg) g_string_append_unichar(f->buff, OWL_FMTEXT_UC_FGDEFAULT);
  if (a)  g_string_append_unichar(f->buff, OWL_FMTEXT_UC_ATTR | OWL_FMTEXT_UC_ATTR);
}

/* Append normal, uncolored text 'text' to 'f' */
void owl_fmtext_append_normal(owl_fmtext *f, const char *text)
{
  owl_fmtext_append_attr(f, text, OWL_FMTEXT_ATTR_NONE, OWL_COLOR_DEFAULT, OWL_COLOR_DEFAULT);
}

/* Append normal, uncolored text specified by format string to 'f' */
void owl_fmtext_appendf_normal(owl_fmtext *f, const char *fmt, ...)
{
  va_list ap;
  char *buff;

  va_start(ap, fmt);
  buff = g_strdup_vprintf(fmt, ap);
  va_end(ap);
  if (!buff)
    return;
  owl_fmtext_append_attr(f, buff, OWL_FMTEXT_ATTR_NONE, OWL_COLOR_DEFAULT, OWL_COLOR_DEFAULT);
  g_free(buff);
}

/* Append normal text 'text' to 'f' with color 'color' */
void owl_fmtext_append_normal_color(owl_fmtext *f, const char *text, int fgcolor, int bgcolor)
{
  owl_fmtext_append_attr(f, text, OWL_FMTEXT_ATTR_NONE, fgcolor, bgcolor);
}

/* Append bold text 'text' to 'f' */
void owl_fmtext_append_bold(owl_fmtext *f, const char *text)
{
  owl_fmtext_append_attr(f, text, OWL_FMTEXT_ATTR_BOLD, OWL_COLOR_DEFAULT, OWL_COLOR_DEFAULT);
}

/* Append reverse video text 'text' to 'f' */
void owl_fmtext_append_reverse(owl_fmtext *f, const char *text)
{
  owl_fmtext_append_attr(f, text, OWL_FMTEXT_ATTR_REVERSE, OWL_COLOR_DEFAULT, OWL_COLOR_DEFAULT);
}

/* Append reversed and bold, uncolored text 'text' to 'f' */
void owl_fmtext_append_reversebold(owl_fmtext *f, const char *text)
{
  owl_fmtext_append_attr(f, text, OWL_FMTEXT_ATTR_REVERSE | OWL_FMTEXT_ATTR_BOLD, OWL_COLOR_DEFAULT, OWL_COLOR_DEFAULT);
}

/* Internal function. Parse attrbute character. */
static void _owl_fmtext_update_attributes(gunichar c, char *attr, short *fgcolor, short *bgcolor)
{
  if ((c & OWL_FMTEXT_UC_ATTR) == OWL_FMTEXT_UC_ATTR) {
    *attr = c & OWL_FMTEXT_UC_ATTR_MASK;
  }
  else if ((c & OWL_FMTEXT_UC_COLOR_BASE) == OWL_FMTEXT_UC_COLOR_BASE) {
    if ((c & OWL_FMTEXT_UC_BGCOLOR) == OWL_FMTEXT_UC_BGCOLOR) {
      *bgcolor = (c == OWL_FMTEXT_UC_BGDEFAULT
                  ? OWL_COLOR_DEFAULT
                  : c & OWL_FMTEXT_UC_COLOR_MASK);
    }
    else if ((c & OWL_FMTEXT_UC_FGCOLOR) == OWL_FMTEXT_UC_FGCOLOR) {
      *fgcolor = (c == OWL_FMTEXT_UC_FGDEFAULT
                  ? OWL_COLOR_DEFAULT
                  : c & OWL_FMTEXT_UC_COLOR_MASK);
    }
  }
}

/* Internal function. Scan for attribute characters. */
static void _owl_fmtext_scan_attributes(const owl_fmtext *f, int start, char *attr, short *fgcolor, short *bgcolor)
{
  const char *p;
  p = strchr(f->buff->str, OWL_FMTEXT_UC_STARTBYTE_UTF8);
  while (p && p < f->buff->str + start) {
    _owl_fmtext_update_attributes(g_utf8_get_char(p), attr, fgcolor, bgcolor);
    p = strchr(p+1, OWL_FMTEXT_UC_STARTBYTE_UTF8);
  }
}  

/* Internal function.  Append text from 'in' between index 'start'
 * inclusive and 'stop' exclusive, to the end of 'f'. This function
 * works with bytes.
 */
static void _owl_fmtext_append_fmtext(owl_fmtext *f, const owl_fmtext *in, int start, int stop)
{
  int a = 0, fg = 0, bg = 0;
  char attr = 0;
  short fgcolor = OWL_COLOR_DEFAULT;
  short bgcolor = OWL_COLOR_DEFAULT;

  _owl_fmtext_scan_attributes(in, start, &attr, &fgcolor, &bgcolor);
  if (attr != OWL_FMTEXT_ATTR_NONE) a=1;
  if (fgcolor != OWL_COLOR_DEFAULT) fg=1;
  if (bgcolor != OWL_COLOR_DEFAULT) bg=1;

  if (a)
    g_string_append_unichar(f->buff, OWL_FMTEXT_UC_ATTR | attr);
  if (fg)
    g_string_append_unichar(f->buff, OWL_FMTEXT_UC_FGCOLOR | fgcolor);
  if (bg)
    g_string_append_unichar(f->buff, OWL_FMTEXT_UC_BGCOLOR | bgcolor);

  g_string_append_len(f->buff, in->buff->str+start, stop-start);

  /* Reset attributes */
  g_string_append_unichar(f->buff, OWL_FMTEXT_UC_BGDEFAULT);
  g_string_append_unichar(f->buff, OWL_FMTEXT_UC_FGDEFAULT);
  g_string_append_unichar(f->buff, OWL_FMTEXT_UC_ATTR | OWL_FMTEXT_UC_ATTR);
}

/* append fmtext 'in' to 'f' */
void owl_fmtext_append_fmtext(owl_fmtext *f, const owl_fmtext *in)
{
  _owl_fmtext_append_fmtext(f, in, 0, in->buff->len);

}

/* Append 'nspaces' number of spaces to the end of 'f' */
void owl_fmtext_append_spaces(owl_fmtext *f, int nspaces)
{
  int i;
  for (i=0; i<nspaces; i++) {
    owl_fmtext_append_normal(f, " ");
  }
}

/* Return a plain version of the fmtext.  Caller is responsible for
 * freeing the return
 */
char *owl_fmtext_print_plain(const owl_fmtext *f)
{
  return owl_strip_format_chars(f->buff->str);
}

static void _owl_fmtext_wattrset(WINDOW *w, int attrs)
{
  wattrset(w, A_NORMAL);
  if (attrs & OWL_FMTEXT_ATTR_BOLD) wattron(w, A_BOLD);
  if (attrs & OWL_FMTEXT_ATTR_REVERSE) wattron(w, A_REVERSE);
  if (attrs & OWL_FMTEXT_ATTR_UNDERLINE) wattron(w, A_UNDERLINE);
}

static void _owl_fmtext_update_colorpair(short fg, short bg, short *pair)
{
  if (owl_global_get_hascolors(&g)) {
    *pair = owl_fmtext_get_colorpair(fg, bg);
  }
}

static void _owl_fmtext_wcolor_set(WINDOW *w, short pair)
{
  if (owl_global_get_hascolors(&g)) {
      wcolor_set(w,pair,NULL);
      wbkgdset(w, COLOR_PAIR(pair));
  }
}

/* add the formatted text to the curses window 'w'.  The window 'w'
 * must already be initiatlized with curses
 */
static void _owl_fmtext_curs_waddstr(const owl_fmtext *f, WINDOW *w, int do_search, char default_attrs, short default_fgcolor, short default_bgcolor)
{
  /* char *tmpbuff; */
  /* int position, trans1, trans2, trans3, len, lastsame; */
  char *s, *p;
  char attr;
  short fg, bg, pair = 0;
  
  if (w==NULL) {
    owl_function_debugmsg("Hit a null window in owl_fmtext_curs_waddstr.");
    return;
  }

  s = f->buff->str;
  /* Set default attributes. */
  attr = default_attrs;
  fg = default_fgcolor;
  bg = default_bgcolor;
  _owl_fmtext_wattrset(w, attr);
  _owl_fmtext_update_colorpair(fg, bg, &pair);
  _owl_fmtext_wcolor_set(w, pair);

  /* Find next possible format character. */
  p = strchr(s, OWL_FMTEXT_UC_STARTBYTE_UTF8);
  while(p) {
    if (owl_fmtext_is_format_char(g_utf8_get_char(p))) {
      /* Deal with all text from last insert to here. */
      char tmp;
   
      tmp = p[0];
      p[0] = '\0';
      if (do_search && owl_global_is_search_active(&g)) {
	/* Search is active, so highlight search results. */
	int start, end;
	while (owl_regex_compare(owl_global_get_search_re(&g), s, &start, &end) == 0) {
	  /* Prevent an infinite loop matching the empty string. */
	  if (end == 0)
	    break;

	  /* Found search string, highlight it. */

	  waddnstr(w, s, start);

	  _owl_fmtext_wattrset(w, attr ^ OWL_FMTEXT_ATTR_REVERSE);
	  _owl_fmtext_wcolor_set(w, pair);
	  
	  waddnstr(w, s + start, end - start);

	  _owl_fmtext_wattrset(w, attr);
	  _owl_fmtext_wcolor_set(w, pair);

	  s += end;
	}
      }
      /* Deal with remaining part of string. */
      waddstr(w, s);
      p[0] = tmp;

      /* Deal with new attributes. Initialize to defaults, then
	 process all consecutive formatting characters. */
      attr = default_attrs;
      fg = default_fgcolor;
      bg = default_bgcolor;
      while (owl_fmtext_is_format_char(g_utf8_get_char(p))) {
	_owl_fmtext_update_attributes(g_utf8_get_char(p), &attr, &fg, &bg);
	p = g_utf8_next_char(p);
      }
      _owl_fmtext_wattrset(w, attr | default_attrs);
      if (fg == OWL_COLOR_DEFAULT) fg = default_fgcolor;
      if (bg == OWL_COLOR_DEFAULT) bg = default_bgcolor;
      _owl_fmtext_update_colorpair(fg, bg, &pair);
      _owl_fmtext_wcolor_set(w, pair);

      /* Advance to next non-formatting character. */
      s = p;
      p = strchr(s, OWL_FMTEXT_UC_STARTBYTE_UTF8);
    }
    else {
      p = strchr(p+1, OWL_FMTEXT_UC_STARTBYTE_UTF8);
    }
  }
  if (s) {
    waddstr(w, s);
  }
  wbkgdset(w, 0);
}

void owl_fmtext_curs_waddstr(const owl_fmtext *f, WINDOW *w, char default_attrs, short default_fgcolor, short default_bgcolor)
{
  _owl_fmtext_curs_waddstr(f, w, 1, default_attrs, default_fgcolor, default_bgcolor);
}

void owl_fmtext_curs_waddstr_without_search(const owl_fmtext *f, WINDOW *w, char default_attrs, short default_fgcolor, short default_bgcolor)
{
  _owl_fmtext_curs_waddstr(f, w, 0, default_attrs, default_fgcolor, default_bgcolor);
}

/* Expands tabs. Tabs are expanded as if given an initial indent of start. */
void owl_fmtext_expand_tabs(const owl_fmtext *in, owl_fmtext *out, int start) {
  int col = start, numcopied = 0;
  char *ptr;

  for (ptr = in->buff->str;
       ptr < in->buff->str + in->buff->len;
       ptr = g_utf8_next_char(ptr)) {
    gunichar c = g_utf8_get_char(ptr);
    int chwidth;
    if (c == '\t') {
      /* Copy up to this tab */
      _owl_fmtext_append_fmtext(out, in, numcopied, ptr - in->buff->str);
      /* and then copy spaces for the tab. */
      chwidth = OWL_TAB_WIDTH - (col % OWL_TAB_WIDTH);
      owl_fmtext_append_spaces(out, chwidth);
      col += chwidth;
      numcopied = g_utf8_next_char(ptr) - in->buff->str;
    } else {
      /* Just update col. We'll append later. */
      if (c == '\n') {
        col = start;
      } else if (!owl_fmtext_is_format_char(c)) {
        col += mk_wcwidth(c);
      }
    }
  }
  /* Append anything we've missed. */
  if (numcopied < in->buff->len)
    _owl_fmtext_append_fmtext(out, in, numcopied, in->buff->len);
}

/* start with line 'aline' (where the first line is 0) and print
 * 'lines' number of lines into 'out'
 */
int owl_fmtext_truncate_lines(const owl_fmtext *in, int aline, int lines, owl_fmtext *out)
{
  const char *ptr1, *ptr2;
  int i, offset;
  
  /* find the starting line */
  ptr1 = in->buff->str;
  for (i = 0; i < aline; i++) {
    ptr1 = strchr(ptr1, '\n');
    if (!ptr1) return(-1);
    ptr1++;
  }
  
  /* ptr1 now holds the starting point */

  /* copy in the next 'lines' lines */
  if (lines < 1) return(-1);

  for (i = 0; i < lines; i++) {
    offset = ptr1 - in->buff->str;
    ptr2 = strchr(ptr1, '\n');
    if (!ptr2) {
      /* Copy to the end of the buffer. */
      _owl_fmtext_append_fmtext(out, in, offset, in->buff->len);
      return(-1);
    }
    /* Copy up to, and including, the new line. */
    _owl_fmtext_append_fmtext(out, in, offset, (ptr2 - ptr1) + offset + 1);
    ptr1 = ptr2 + 1;
  }
  return(0);
}

/* Implementation of owl_fmtext_truncate_cols. Does not support tabs in input. */
void _owl_fmtext_truncate_cols_internal(const owl_fmtext *in, int acol, int bcol, owl_fmtext *out)
{
  const char *ptr_s, *ptr_e, *ptr_c, *last;
  int col, st, padding, chwidth;

  last = in->buff->str + in->buff->len - 1;
  ptr_s = in->buff->str;
  while (ptr_s <= last) {
    ptr_e=strchr(ptr_s, '\n');
    if (!ptr_e) {
      /* but this shouldn't happen if we end in a \n */
      break;
    }
    
    if (ptr_e == ptr_s) {
      owl_fmtext_append_normal(out, "\n");
      ++ptr_s;
      continue;
    }

    col = 0;
    st = 0;
    padding = 0;
    chwidth = 0;
    ptr_c = ptr_s;
    while(ptr_c < ptr_e) {
      gunichar c = g_utf8_get_char(ptr_c);
      if (!owl_fmtext_is_format_char(c)) {
	chwidth = mk_wcwidth(c);
	if (col + chwidth > bcol) break;
	
	if (col >= acol) {
	  if (st == 0) {
	    ptr_s = ptr_c;
	    padding = col - acol;
	    ++st;
	  }
	}
	col += chwidth;
	chwidth = 0;
      }
      ptr_c = g_utf8_next_char(ptr_c);
    }
    if (st) {
      /* lead padding */
      owl_fmtext_append_spaces(out, padding);
      if (ptr_c == ptr_e) {
	/* We made it to the newline. Append up to, and including it. */
	_owl_fmtext_append_fmtext(out, in, ptr_s - in->buff->str, ptr_c - in->buff->str + 1);
      }
      else if (chwidth > 1) {
        /* Last char is wide, truncate. */
        _owl_fmtext_append_fmtext(out, in, ptr_s - in->buff->str, ptr_c - in->buff->str);
        owl_fmtext_append_normal(out, "\n");
      }
      else {
        /* Last char fits perfectly, We stop at the next char to make
	 * sure we get it all. */
        ptr_c = g_utf8_next_char(ptr_c);
        _owl_fmtext_append_fmtext(out, in, ptr_s - in->buff->str, ptr_c - in->buff->str);
      }
    }
    else {
      owl_fmtext_append_normal(out, "\n");
    }
    ptr_s = g_utf8_next_char(ptr_e);
  }
}

/* Truncate the message so that each line begins at column 'acol' and
 * ends at 'bcol' or sooner.  The first column is number 0.  The new
 * message is placed in 'out'.  The message is expected to end in a
 * new line for now.
 *
 * NOTE: This needs to be modified to deal with backing up if we find
 * a SPACING COMBINING MARK at the end of a line. If that happens, we
 * should back up to the last non-mark character and stop there.
 *
 * NOTE: If a line ends at bcol, we omit the newline. This is so printing
 * to ncurses works.
 */
void owl_fmtext_truncate_cols(const owl_fmtext *in, int acol, int bcol, owl_fmtext *out)
{
  owl_fmtext notabs;

  /* _owl_fmtext_truncate_cols_internal cannot handle tabs. */
  if (strchr(in->buff->str, '\t')) {
    owl_fmtext_init_null(&notabs);
    owl_fmtext_expand_tabs(in, &notabs, 0);
    _owl_fmtext_truncate_cols_internal(&notabs, acol, bcol, out);
    owl_fmtext_cleanup(&notabs);
  } else {
    _owl_fmtext_truncate_cols_internal(in, acol, bcol, out);
  }
}

/* Return the number of lines in 'f' */
int owl_fmtext_num_lines(const owl_fmtext *f)
{
  int lines, i;
  char *lastbreak, *p;

  lines=0;
  lastbreak = f->buff->str;
  for (i = 0; i < f->buff->len; i++) {
    if (f->buff->str[i]=='\n') {
      lastbreak = f->buff->str + i;
      lines++;
    }
  }

  /* Check if there's a trailing line; formatting characters don't count. */
  for (p = g_utf8_next_char(lastbreak);
       p < f->buff->str + f->buff->len;
       p = g_utf8_next_char(p)) {
    if (!owl_fmtext_is_format_char(g_utf8_get_char(p))) {
      lines++;
      break;
    }
  }

  return(lines);
}

/* Returns the line number, starting at 0, of the character which
 * contains the byte at 'offset'. Note that a trailing newline is part
 * of the line it ends. Also, while a trailing line of formatting
 * characters does not contribute to owl_fmtext_num_lines, those
 * characters are considered on a new line. */
int owl_fmtext_line_number(const owl_fmtext *f, int offset)
{
  int i, lineno = 0;
  if (offset >= f->buff->len)
    offset = f->buff->len - 1;
  for (i = 0; i < offset; i++) {
    if (f->buff->str[i] == '\n')
      lineno++;
  }
  return lineno;
}

/* Searches for line 'lineno' in 'f'. The returned range, [start,
 * end), forms a half-open interval for the extent of the line. */
void owl_fmtext_line_extents(const owl_fmtext *f, int lineno, int *o_start, int *o_end)
{
  int start, end;
  char *newline;
  for (start = 0; lineno > 0 && start < f->buff->len; start++) {
    if (f->buff->str[start] == '\n')
      lineno--;
  }
  newline = strchr(f->buff->str + start, '\n');
  /* Include the newline, if it is there. */
  end = newline ? newline - f->buff->str + 1 : f->buff->len;
  if (o_start) *o_start = start;
  if (o_end) *o_end = end;
}

const char *owl_fmtext_get_text(const owl_fmtext *f)
{
  return f->buff->str;
}

int owl_fmtext_num_bytes(const owl_fmtext *f)
{
  return f->buff->len;
}

/* Make a copy of the fmtext 'src' into 'dst' */
void owl_fmtext_copy(owl_fmtext *dst, const owl_fmtext *src)
{
  dst->buff = g_string_new(src->buff->str);
}

/* Search 'f' for the regex 're' for matches starting at
 * 'start'. Returns the offset of the first match, -1 if not
 * found. This is a case-insensitive search.
 */
int owl_fmtext_search(const owl_fmtext *f, const owl_regex *re, int start)
{
  int offset;
  if (start > f->buff->len ||
      owl_regex_compare(re, f->buff->str + start, &offset, NULL) != 0)
    return -1;
  return offset + start;
}


/* Append the text 'text' to 'f' and interpret the zephyr style
 * formatting syntax to set appropriate attributes.
 */
void owl_fmtext_append_ztext(owl_fmtext *f, const char *text)
{
  int stacksize, curattrs, curcolor;
  const char *ptr, *txtptr, *tmpptr;
  char *buff;
  int attrstack[32], chrstack[32], colorstack[32];

  curattrs=OWL_FMTEXT_ATTR_NONE;
  curcolor=OWL_COLOR_DEFAULT;
  stacksize=0;
  txtptr=text;
  while (1) {
    ptr=strpbrk(txtptr, "@{[<()>]}");
    if (!ptr) {
      /* add all the rest of the text and exit */
      owl_fmtext_append_attr(f, txtptr, curattrs, curcolor, OWL_COLOR_DEFAULT);
      return;
    } else if (ptr[0]=='@') {
      /* add the text up to this point then deal with the stack */
      buff=g_new(char, ptr-txtptr+20);
      strncpy(buff, txtptr, ptr-txtptr);
      buff[ptr-txtptr]='\0';
      owl_fmtext_append_attr(f, buff, curattrs, curcolor, OWL_COLOR_DEFAULT);
      g_free(buff);

      /* update pointer to point at the @ */
      txtptr=ptr;

      /* now the stack */

      /* if we've hit our max stack depth, print the @ and move on */
      if (stacksize==32) {
        owl_fmtext_append_attr(f, "@", curattrs, curcolor, OWL_COLOR_DEFAULT);
        txtptr++;
        continue;
      }

      /* if it's an @@, print an @ and continue */
      if (txtptr[1]=='@') {
        owl_fmtext_append_attr(f, "@", curattrs, curcolor, OWL_COLOR_DEFAULT);
        txtptr+=2;
        continue;
      }
        
      /* if there's no opener, print the @ and continue */
      tmpptr=strpbrk(txtptr, "(<[{ ");
      if (!tmpptr || tmpptr[0]==' ') {
        owl_fmtext_append_attr(f, "@", curattrs, curcolor, OWL_COLOR_DEFAULT);
        txtptr++;
        continue;
      }

      /* check what command we've got, push it on the stack, start
         using it, and continue ... unless it's a color command */
      buff=g_new(char, tmpptr-ptr+20);
      strncpy(buff, ptr, tmpptr-ptr);
      buff[tmpptr-ptr]='\0';
      if (!strcasecmp(buff, "@bold")) {
        attrstack[stacksize]=OWL_FMTEXT_ATTR_BOLD;
        chrstack[stacksize]=tmpptr[0];
	colorstack[stacksize]=curcolor;
        stacksize++;
        curattrs|=OWL_FMTEXT_ATTR_BOLD;
        txtptr+=6;
        g_free(buff);
        continue;
      } else if (!strcasecmp(buff, "@b")) {
        attrstack[stacksize]=OWL_FMTEXT_ATTR_BOLD;
        chrstack[stacksize]=tmpptr[0];
	colorstack[stacksize]=curcolor;
        stacksize++;
        curattrs|=OWL_FMTEXT_ATTR_BOLD;
        txtptr+=3;
        g_free(buff);
        continue;
      } else if (!strcasecmp(buff, "@i")) {
        attrstack[stacksize]=OWL_FMTEXT_ATTR_UNDERLINE;
        chrstack[stacksize]=tmpptr[0];
	colorstack[stacksize]=curcolor;
        stacksize++;
        curattrs|=OWL_FMTEXT_ATTR_UNDERLINE;
        txtptr+=3;
        g_free(buff);
        continue;
      } else if (!strcasecmp(buff, "@italic")) {
        attrstack[stacksize]=OWL_FMTEXT_ATTR_UNDERLINE;
        chrstack[stacksize]=tmpptr[0];
	colorstack[stacksize]=curcolor;
        stacksize++;
        curattrs|=OWL_FMTEXT_ATTR_UNDERLINE;
        txtptr+=8;
        g_free(buff);
        continue;
      } else if (!strcasecmp(buff, "@")) {
	attrstack[stacksize]=OWL_FMTEXT_ATTR_NONE;
	chrstack[stacksize]=tmpptr[0];
	colorstack[stacksize]=curcolor;
        stacksize++;
        txtptr+=2;
        g_free(buff);
        continue;

        /* if it's a color read the color, set the current color and
           continue */
      } else if (!strcasecmp(buff, "@color") 
                 && owl_global_get_hascolors(&g)
                 && owl_global_is_colorztext(&g)) {
        g_free(buff);
        txtptr+=7;
        tmpptr=strpbrk(txtptr, "@{[<()>]}");
        if (tmpptr &&
            ((txtptr[-1]=='(' && tmpptr[0]==')') ||
             (txtptr[-1]=='<' && tmpptr[0]=='>') ||
             (txtptr[-1]=='[' && tmpptr[0]==']') ||
             (txtptr[-1]=='{' && tmpptr[0]=='}'))) {

          /* grab the color name */
          buff=g_new(char, tmpptr-txtptr+20);
          strncpy(buff, txtptr, tmpptr-txtptr);
          buff[tmpptr-txtptr]='\0';

          /* set it as the current color */
          curcolor=owl_util_string_to_color(buff);
          if (curcolor == OWL_COLOR_INVALID)
	      curcolor = OWL_COLOR_DEFAULT;
          g_free(buff);
          txtptr=tmpptr+1;
          continue;

        } else {

        }

      } else {
        /* if we didn't understand it, we'll print it.  This is different from zwgc
         * but zwgc seems to be smarter about some screw cases than I am
         */
        owl_fmtext_append_attr(f, "@", curattrs, curcolor, OWL_COLOR_DEFAULT);
        txtptr++;
        continue;
      }

    } else if (ptr[0]=='}' || ptr[0]==']' || ptr[0]==')' || ptr[0]=='>') {
      /* add the text up to this point first */
      buff=g_new(char, ptr-txtptr+20);
      strncpy(buff, txtptr, ptr-txtptr);
      buff[ptr-txtptr]='\0';
      owl_fmtext_append_attr(f, buff, curattrs, curcolor, OWL_COLOR_DEFAULT);
      g_free(buff);

      /* now deal with the closer */
      txtptr=ptr;

      /* first, if the stack is empty we must bail (just print and go) */
      if (stacksize==0) {
        buff=g_new(char, 5);
        buff[0]=ptr[0];
        buff[1]='\0';
        owl_fmtext_append_attr(f, buff, curattrs, curcolor, OWL_COLOR_DEFAULT);
        g_free(buff);
        txtptr++;
        continue;
      }

      /* if the closing char is what's on the stack, turn off the
         attribue and pop the stack */
      if ((ptr[0]==')' && chrstack[stacksize-1]=='(') ||
          (ptr[0]=='>' && chrstack[stacksize-1]=='<') ||
          (ptr[0]==']' && chrstack[stacksize-1]=='[') ||
          (ptr[0]=='}' && chrstack[stacksize-1]=='{')) {
        int i;
        stacksize--;
        curattrs=OWL_FMTEXT_ATTR_NONE;
	curcolor = colorstack[stacksize];
        for (i=0; i<stacksize; i++) {
          curattrs|=attrstack[i];
        }
        txtptr+=1;
        continue;
      } else {
        /* otherwise print and continue */
        buff=g_new(char, 5);
        buff[0]=ptr[0];
        buff[1]='\0';
        owl_fmtext_append_attr(f, buff, curattrs, curcolor, OWL_COLOR_DEFAULT);
        g_free(buff);
        txtptr++;
        continue;
      }
    } else {
      /* we've found an unattached opener, print everything and move on */
      buff=g_new(char, ptr-txtptr+20);
      strncpy(buff, txtptr, ptr-txtptr+1);
      buff[ptr-txtptr+1]='\0';
      owl_fmtext_append_attr(f, buff, curattrs, curcolor, OWL_COLOR_DEFAULT);
      g_free(buff);
      txtptr=ptr+1;
      continue;
    }
  }
}

/* requires that the list values are strings or NULL.
 * joins the elements together with join_with. 
 * If format_fn is specified, passes it the list element value
 * and it will return a string which this needs to free. */
void owl_fmtext_append_list(owl_fmtext *f, const owl_list *l, const char *join_with, char *(format_fn)(const char *))
{
  int i, size;
  const char *elem;
  char *text;

  size = owl_list_get_size(l);
  for (i=0; i<size; i++) {
    elem = owl_list_get_element(l,i);
    if (elem && format_fn) {
      text = format_fn(elem);
      if (text) {
        owl_fmtext_append_normal(f, text);
        g_free(text);
      }
    } else if (elem) {
      owl_fmtext_append_normal(f, elem);
    }
    if ((i < size-1) && join_with) {
      owl_fmtext_append_normal(f, join_with);
    }
  }
}

/* Free all memory allocated by the object */
void owl_fmtext_cleanup(owl_fmtext *f)
{
  if (f->buff) g_string_free(f->buff, true);
  f->buff = NULL;
}

/*** Color Pair manager ***/
void owl_fmtext_init_colorpair_mgr(owl_colorpair_mgr *cpmgr)
{
  /* This could be a bitarray if we wanted to save memory. */
  short i;
  /* The test is <= because we allocate COLORS+1 entries. */
  cpmgr->pairs = g_new(short *, COLORS + 1);
  for(i = 0; i <= COLORS; i++) {
    cpmgr->pairs[i] = g_new(short, COLORS + 1);
  }
  owl_fmtext_reset_colorpairs(cpmgr);
}

/* Reset used list */
void owl_fmtext_reset_colorpairs(owl_colorpair_mgr *cpmgr)
{
  short i, j;

  cpmgr->overflow = false;
  cpmgr->next = 8;
  /* The test is <= because we allocated COLORS+1 entries. */
  for(i = 0; i <= COLORS; i++) {
    for(j = 0; j <= COLORS; j++) {
      cpmgr->pairs[i][j] = -1;
    }
  }
  if (owl_global_get_hascolors(&g)) {
    for(i = 0; i < 8; i++) {
      short fg, bg;
      if (i >= COLORS) continue;
      pair_content(i, &fg, &bg);
      cpmgr->pairs[fg+1][bg+1] = i;
    }
  }
}

/* Assign pairs by request */
short owl_fmtext_get_colorpair(int fg, int bg)
{
  owl_colorpair_mgr *cpmgr;
  short pair;

  /* Sanity (Bounds) Check */
  if (fg > COLORS || fg < OWL_COLOR_DEFAULT) fg = OWL_COLOR_DEFAULT;
  if (bg > COLORS || bg < OWL_COLOR_DEFAULT) bg = OWL_COLOR_DEFAULT;
	    
#ifdef HAVE_USE_DEFAULT_COLORS
  if (fg == OWL_COLOR_DEFAULT) fg = -1;
#else
  if (fg == OWL_COLOR_DEFAULT) fg = 0;
  if (bg == OWL_COLOR_DEFAULT) bg = 0;
#endif

  /* looking for a pair we already set up for this draw. */
  cpmgr = owl_global_get_colorpair_mgr(&g);
  pair = cpmgr->pairs[fg+1][bg+1];
  if (!(pair != -1 && pair < cpmgr->next)) {
    /* If we didn't find a pair, search for a free one to assign. */
    pair = (cpmgr->next < COLOR_PAIRS) ? cpmgr->next : -1;
    if (pair != -1) {
      /* We found a free pair, initialize it. */
      init_pair(pair, fg, bg);
      cpmgr->pairs[fg+1][bg+1] = pair;
      cpmgr->next++;
    }
    else if (bg != OWL_COLOR_DEFAULT) {
      /* We still don't have a pair, drop the background color. Too bad. */
      owl_function_debugmsg("colorpairs: color shortage - dropping background color.");
      cpmgr->overflow = true;
      pair = owl_fmtext_get_colorpair(fg, OWL_COLOR_DEFAULT);
    }
    else {
      /* We still don't have a pair, defaults all around. */
      owl_function_debugmsg("colorpairs: color shortage - dropping foreground and background color.");
      cpmgr->overflow = true;
      pair = 0;
    }
  }
  return pair;
}
