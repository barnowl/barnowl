#include "owl.h"
#include <stdlib.h>
#include <string.h>

static const char fileIdent[] = "$Id$";

/* initialize an fmtext with no data */
void owl_fmtext_init_null(owl_fmtext *f)
{
  f->textlen = 0;
  f->bufflen = 5;
  f->textbuff = owl_malloc(5);
  f->textbuff[0] = 0;
  f->default_attrs = OWL_FMTEXT_ATTR_NONE;
  f->default_fgcolor = OWL_COLOR_DEFAULT;
  f->default_bgcolor = OWL_COLOR_DEFAULT;
}

/* Clear the data from an fmtext, but don't deallocate memory. This
   fmtext can then be appended to again. */
void owl_fmtext_clear(owl_fmtext *f)
{
  f->textlen = 0;
  f->textbuff[0] = 0;
  f->default_attrs = OWL_FMTEXT_ATTR_NONE;
  f->default_fgcolor = OWL_COLOR_DEFAULT;
  f->default_bgcolor = OWL_COLOR_DEFAULT;
}

void _owl_fmtext_realloc(owl_fmtext *f, int newlen) /*noproto*/
{
    if(newlen + 1 > f->bufflen) {
      f->textbuff = owl_realloc(f->textbuff, newlen + 1);
      f->bufflen = newlen+1;
  }
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
void owl_fmtext_append_attr(owl_fmtext *f, char *text, char attr, short fgcolor, short bgcolor)
{
  char attrbuff[6];
  int newlen, a = 0, fg = 0, bg = 0;
  
  if (attr != OWL_FMTEXT_ATTR_NONE) a=1;
  if (fgcolor != OWL_COLOR_DEFAULT) fg=1;
  if (bgcolor != OWL_COLOR_DEFAULT) bg=1;

  /* Plane-16 characters in UTF-8 are 4 bytes long. */
  newlen = strlen(f->textbuff) + strlen(text) + (8 * (a + fg + bg));
  _owl_fmtext_realloc(f, newlen);

  /* Set attributes */
  if (a) {
    memset(attrbuff,0,6);
    g_unichar_to_utf8(OWL_FMTEXT_UC_ATTR | attr, attrbuff);
    strcat(f->textbuff, attrbuff);      
  }
  if (fg) {
    memset(attrbuff,0,6);
    g_unichar_to_utf8(OWL_FMTEXT_UC_FGCOLOR | fgcolor, attrbuff);
    strcat(f->textbuff, attrbuff);      
  }
  if (bg) {
    memset(attrbuff,0,6);
    g_unichar_to_utf8(OWL_FMTEXT_UC_BGCOLOR | bgcolor, attrbuff);
    strcat(f->textbuff, attrbuff);      
  }
  
  strcat(f->textbuff, text);

  /* Reset attributes */
  if (bg) strcat(f->textbuff, OWL_FMTEXT_UTF8_BGDEFAULT);
  if (fg) strcat(f->textbuff, OWL_FMTEXT_UTF8_FGDEFAULT);
  if (a)  strcat(f->textbuff, OWL_FMTEXT_UTF8_ATTR_NONE);
  f->textlen=newlen;
}

/* Append normal, uncolored text 'text' to 'f' */
void owl_fmtext_append_normal(owl_fmtext *f, char *text)
{
  owl_fmtext_append_attr(f, text, OWL_FMTEXT_ATTR_NONE, OWL_COLOR_DEFAULT, OWL_COLOR_DEFAULT);
}

/* Append normal text 'text' to 'f' with color 'color' */
void owl_fmtext_append_normal_color(owl_fmtext *f, char *text, int fgcolor, int bgcolor)
{
  owl_fmtext_append_attr(f, text, OWL_FMTEXT_ATTR_NONE, fgcolor, bgcolor);
}

/* Append bold text 'text' to 'f' */
void owl_fmtext_append_bold(owl_fmtext *f, char *text)
{
  owl_fmtext_append_attr(f, text, OWL_FMTEXT_ATTR_BOLD, OWL_COLOR_DEFAULT, OWL_COLOR_DEFAULT);
}

/* Append reverse video text 'text' to 'f' */
void owl_fmtext_append_reverse(owl_fmtext *f, char *text)
{
  owl_fmtext_append_attr(f, text, OWL_FMTEXT_ATTR_REVERSE, OWL_COLOR_DEFAULT, OWL_COLOR_DEFAULT);
}

/* Append reversed and bold, uncolored text 'text' to 'f' */
void owl_fmtext_append_reversebold(owl_fmtext *f, char *text)
{
  owl_fmtext_append_attr(f, text, OWL_FMTEXT_ATTR_REVERSE | OWL_FMTEXT_ATTR_BOLD, OWL_COLOR_DEFAULT, OWL_COLOR_DEFAULT);
}

/* Add the attribute 'attr' to the default atts for the text in 'f' */
void owl_fmtext_addattr(owl_fmtext *f, char attr)
{
  /* add the attribute to all text */
  f->default_attrs |= attr;
}

/* Set the default foreground color for this fmtext to 'color'.
 * Only affects text that is colored default.
 */
void owl_fmtext_colorize(owl_fmtext *f, int color)
{
  f->default_fgcolor = color;
}

/* Set the default foreground color for this fmtext to 'color'.
 * Only affects text that is colored default.
 */
void owl_fmtext_colorizebg(owl_fmtext *f, int color)
{
  f->default_bgcolor = color;
}

/* Internal function. Parse attrbute character. */
void _owl_fmtext_update_attributes(gunichar c, char *attr, short *fgcolor, short *bgcolor) /*noproto*/
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
void _owl_fmtext_scan_attributes(owl_fmtext *f, int start, char *attr, short *fgcolor, short *bgcolor) /*noproto*/
{
  char *p;
  p = strchr(f->textbuff, OWL_FMTEXT_UC_STARTBYTE_UTF8);
  while (p && p < f->textbuff + start) {
    _owl_fmtext_update_attributes(g_utf8_get_char(p), attr, fgcolor, bgcolor);
    p = strchr(p+1, OWL_FMTEXT_UC_STARTBYTE_UTF8);
  }
}  

/* Internal function.  Append text from 'in' between index 'start' and
 * 'stop' to the end of 'f'
 */
void _owl_fmtext_append_fmtext(owl_fmtext *f, owl_fmtext *in, int start, int stop) /*noproto*/
{
  char attrbuff[6];
  int newlen, a = 0, fg = 0, bg = 0;
  char attr = 0;
  short fgcolor = OWL_COLOR_DEFAULT;
  short bgcolor = OWL_COLOR_DEFAULT;

  _owl_fmtext_scan_attributes(in, start, &attr, &fgcolor, &bgcolor);
  if (attr != OWL_FMTEXT_ATTR_NONE) a=1;
  if (fgcolor != OWL_COLOR_DEFAULT) fg=1;
  if (bgcolor != OWL_COLOR_DEFAULT) bg=1;

  /* We will reset to defaults after appending the text. We may need
     to set initial attributes. */
  newlen=strlen(f->textbuff)+(stop-start+1) + (4 * (a + fg + bg)) + 12;
  _owl_fmtext_realloc(f, newlen);

  if (a) {
    memset(attrbuff,0,6);
    g_unichar_to_utf8(OWL_FMTEXT_UC_ATTR | attr, attrbuff);
    strcat(f->textbuff, attrbuff);      
  }
  if (fg) {
    memset(attrbuff,0,6);
    g_unichar_to_utf8(OWL_FMTEXT_UC_FGCOLOR | fgcolor, attrbuff);
    strcat(f->textbuff, attrbuff);      
  }
  if (bg) {
    memset(attrbuff,0,6);
    g_unichar_to_utf8(OWL_FMTEXT_UC_BGCOLOR | bgcolor, attrbuff);
    strcat(f->textbuff, attrbuff);      
  }

  strncat(f->textbuff, in->textbuff+start, stop-start+1);

  /* Reset attributes */
  strcat(f->textbuff, OWL_FMTEXT_UTF8_BGDEFAULT);
  strcat(f->textbuff, OWL_FMTEXT_UTF8_FGDEFAULT);
  strcat(f->textbuff, OWL_FMTEXT_UTF8_ATTR_NONE);

  f->textbuff[newlen]='\0';
  f->textlen=newlen;
}

/* append fmtext 'in' to 'f' */
void owl_fmtext_append_fmtext(owl_fmtext *f, owl_fmtext *in)
{
  _owl_fmtext_append_fmtext(f, in, 0, in->textlen);

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
char *owl_fmtext_print_plain(owl_fmtext *f)
{
  return owl_strip_format_chars(f->textbuff);
}

void _owl_fmtext_wattrset(WINDOW *w, int attrs) /*noproto*/
{
  wattrset(w, A_NORMAL);
  if (attrs & OWL_FMTEXT_ATTR_BOLD) wattron(w, A_BOLD);
  if (attrs & OWL_FMTEXT_ATTR_REVERSE) wattron(w, A_REVERSE);
  if (attrs & OWL_FMTEXT_ATTR_UNDERLINE) wattron(w, A_UNDERLINE);
}

void _owl_fmtext_update_colorpair(short fg, short bg, short *pair) /*noproto*/
{
  if (owl_global_get_hascolors(&g)) {
    *pair = owl_fmtext_get_colorpair(fg, bg);
  }
}

void _owl_fmtext_wcolor_set(WINDOW *w, short pair) /*noproto*/
{
  if (owl_global_get_hascolors(&g)) {
      wcolor_set(w,pair,NULL);
  }
}

/* add the formatted text to the curses window 'w'.  The window 'w'
 * must already be initiatlized with curses
 */
void _owl_fmtext_curs_waddstr(owl_fmtext *f, WINDOW *w, int do_search) /*noproto*/
{
  /* char *tmpbuff; */
  /* int position, trans1, trans2, trans3, len, lastsame; */
  char *s, *p;
  char attr;
  short fg, bg, pair;
  int search_results, search_len;
  
  if (w==NULL) {
    owl_function_debugmsg("Hit a null window in owl_fmtext_curs_waddstr.");
    return;
  }

  search_results = (do_search
		    ? owl_fmtext_search(f, owl_global_get_search_string(&g))
		    : 0);
  search_len = (search_results
		? strlen(owl_global_get_search_string(&g))
		: 0);
  s = f->textbuff;
  /* Set default attributes. */
  attr = f->default_attrs;
  fg = f->default_fgcolor;
  bg = f->default_bgcolor;
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
      if (search_results) {
	/* Search is active, so highlight search results. */
	char tmp2, *ss;
	ss = stristr(s, owl_global_get_search_string(&g));
	while (ss) {
	  /* Found search string, highlight it. */

	  tmp2 = ss[0];
	  ss[0] = '\0';
	  waddstr(w, s);
	  ss[0] = tmp2;

	  _owl_fmtext_wattrset(w, attr ^ OWL_FMTEXT_ATTR_REVERSE);
	  _owl_fmtext_wcolor_set(w, pair);
	  
	  tmp2 = ss[search_len];
	  ss[search_len] = '\0';
	  waddstr(w, ss);
	  ss[search_len] = tmp2;

	  _owl_fmtext_wattrset(w, attr);
	  _owl_fmtext_wcolor_set(w, pair);

	  s = ss + search_len;
	  ss = stristr(s, owl_global_get_search_string(&g));
	}
      }
      /* Deal with remaining part of string. */
      waddstr(w, s);
      p[0] = tmp;

      /* Deal with new attributes. Initialize to defaults, then
	 process all consecutive formatting characters. */
      attr = f->default_attrs;
      fg = f->default_fgcolor;
      bg = f->default_bgcolor;
      while (p && owl_fmtext_is_format_char(g_utf8_get_char(p))) {
	_owl_fmtext_update_attributes(g_utf8_get_char(p), &attr, &fg, &bg);
	p = g_utf8_next_char(p);
      }
      _owl_fmtext_wattrset(w, attr | f->default_attrs);
      if (fg == OWL_COLOR_DEFAULT) fg = f->default_fgcolor;
      if (bg == OWL_COLOR_DEFAULT) bg = f->default_bgcolor;
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
}

void owl_fmtext_curs_waddstr(owl_fmtext *f, WINDOW *w)
{
  _owl_fmtext_curs_waddstr(f, w, owl_global_is_search_active(&g));
}

void owl_fmtext_curs_waddstr_without_search(owl_fmtext *f, WINDOW *w)
{
  _owl_fmtext_curs_waddstr(f, w, 0);
}

/* start with line 'aline' (where the first line is 0) and print
 * 'lines' number of lines into 'out'
 */
int owl_fmtext_truncate_lines(owl_fmtext *in, int aline, int lines, owl_fmtext *out)
{
  char *ptr1, *ptr2;
  int i, offset;
  
  /* find the starting line */
  ptr1 = in->textbuff;
  for (i = 0; i < aline; i++) {
    ptr1 = strchr(ptr1, '\n');
    if (!ptr1) return(-1);
    ptr1++;
  }
  
  /* ptr1 now holds the starting point */

  /* copy the default attributes */
  out->default_attrs = in->default_attrs;
  out->default_fgcolor = in->default_fgcolor;
  out->default_bgcolor = in->default_bgcolor;
    
  /* copy in the next 'lines' lines */
  if (lines < 1) return(-1);

  for (i = 0; i < lines; i++) {
    offset = ptr1 - in->textbuff;
    ptr2 = strchr(ptr1, '\n');
    if (!ptr2) {
      _owl_fmtext_append_fmtext(out, in, offset, (in->textlen) - 1);
      return(-1);
    }
    _owl_fmtext_append_fmtext(out, in, offset, (ptr2 - ptr1) + offset);
    ptr1 = ptr2 + 1;
  }
  return(0);
}

/* Truncate the message so that each line begins at column 'acol' and
 * ends at 'bcol' or sooner.  The first column is number 0.  The new
 * message is placed in 'out'.  The message is * expected to end in a
 * new line for now. NOTE: This needs to be modified to deal with
 * backing up if we find a SPACING COMBINING MARK at the end of a
 * line. If that happens, we should back up to the last non-mark
 * character and stop there.
 */
void owl_fmtext_truncate_cols(owl_fmtext *in, int acol, int bcol, owl_fmtext *out)
{
  char *ptr_s, *ptr_e, *ptr_c, *last;
  int col, st, padding, chwidth;

  /* copy the default attributes */
  out->default_attrs = in->default_attrs;
  out->default_fgcolor = in->default_fgcolor;
  out->default_bgcolor = in->default_bgcolor;

  last=in->textbuff+in->textlen-1;
  ptr_s=in->textbuff;
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
	/* We made it to the newline. */
	_owl_fmtext_append_fmtext(out, in, ptr_s - in->textbuff, ptr_c - in->textbuff);
      }
      else {
	if (chwidth > 1) {
	  /* Last char is wide, truncate. */
	  _owl_fmtext_append_fmtext(out, in, ptr_s - in->textbuff, ptr_c - in->textbuff - 1);
	  owl_fmtext_append_normal(out, "\n");
	}
	else {
	  /* Last char fits perfectly, leave alone.*/
	  _owl_fmtext_append_fmtext(out, in, ptr_s - in->textbuff, ptr_c - in->textbuff);
	}
      }
    }
    else {
      owl_fmtext_append_normal(out, "\n");
    }
    ptr_s = g_utf8_next_char(ptr_e);
  }
}

/* Return the number of lines in 'f' */
int owl_fmtext_num_lines(owl_fmtext *f)
{
  int lines, i;

  if (f->textlen==0) return(0);
  
  lines=0;
  for (i=0; i<f->textlen; i++) {
    if (f->textbuff[i]=='\n') lines++;
  }

  /* if the last char wasn't a \n there's one more line */
  if (f->textbuff[i-1]!='\n') lines++;

  return(lines);
}

char *owl_fmtext_get_text(owl_fmtext *f)
{
  return(f->textbuff);
}

/* set the charater at 'index' to be 'char'.  If index is out of
 * bounds don't do anything. If c or char at index is not ASCII, don't
 * do anything because it's not UTF-8 safe. */
void owl_fmtext_set_char(owl_fmtext *f, int index, char ch)
{
  if ((index < 0) || (index > f->textlen-1)) return;
  /* NOT ASCII*/
  if (f->textbuff[index] & 0x80 || ch & 0x80) return; 
  f->textbuff[index]=ch;
}

/* Make a copy of the fmtext 'src' into 'dst' */
void owl_fmtext_copy(owl_fmtext *dst, owl_fmtext *src)
{
  int mallocsize;

  if (src->textlen==0) {
    mallocsize=5;
  } else {
    mallocsize=src->textlen+2;
  }
  dst->textlen=src->textlen;
  dst->bufflen=mallocsize;
  dst->textbuff=owl_malloc(mallocsize);
  memcpy(dst->textbuff, src->textbuff, src->textlen+1);
  dst->default_attrs = src->default_attrs;
  dst->default_fgcolor = src->default_fgcolor;
  dst->default_bgcolor = src->default_bgcolor;
}

/* return 1 if the string is found, 0 if not.  This is a case
 *  insensitive search.
 */
int owl_fmtext_search(owl_fmtext *f, char *string)
{
  if (stristr(f->textbuff, string)) return(1);
  return(0);
}


/* Append the text 'text' to 'f' and interpret the zephyr style
 * formatting syntax to set appropriate attributes.
 */
void owl_fmtext_append_ztext(owl_fmtext *f, char *text)
{
  int stacksize, curattrs, curcolor;
  char *ptr, *txtptr, *buff, *tmpptr;
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
      buff=owl_malloc(ptr-txtptr+20);
      strncpy(buff, txtptr, ptr-txtptr);
      buff[ptr-txtptr]='\0';
      owl_fmtext_append_attr(f, buff, curattrs, curcolor, OWL_COLOR_DEFAULT);
      owl_free(buff);

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
      buff=owl_malloc(tmpptr-ptr+20);
      strncpy(buff, ptr, tmpptr-ptr);
      buff[tmpptr-ptr]='\0';
      if (!strcasecmp(buff, "@bold")) {
        attrstack[stacksize]=OWL_FMTEXT_ATTR_BOLD;
        chrstack[stacksize]=tmpptr[0];
	colorstack[stacksize]=curcolor;
        stacksize++;
        curattrs|=OWL_FMTEXT_ATTR_BOLD;
        txtptr+=6;
        owl_free(buff);
        continue;
      } else if (!strcasecmp(buff, "@b")) {
        attrstack[stacksize]=OWL_FMTEXT_ATTR_BOLD;
        chrstack[stacksize]=tmpptr[0];
	colorstack[stacksize]=curcolor;
        stacksize++;
        curattrs|=OWL_FMTEXT_ATTR_BOLD;
        txtptr+=3;
        owl_free(buff);
        continue;
      } else if (!strcasecmp(buff, "@i")) {
        attrstack[stacksize]=OWL_FMTEXT_ATTR_UNDERLINE;
        chrstack[stacksize]=tmpptr[0];
	colorstack[stacksize]=curcolor;
        stacksize++;
        curattrs|=OWL_FMTEXT_ATTR_UNDERLINE;
        txtptr+=3;
        owl_free(buff);
        continue;
      } else if (!strcasecmp(buff, "@italic")) {
        attrstack[stacksize]=OWL_FMTEXT_ATTR_UNDERLINE;
        chrstack[stacksize]=tmpptr[0];
	colorstack[stacksize]=curcolor;
        stacksize++;
        curattrs|=OWL_FMTEXT_ATTR_UNDERLINE;
        txtptr+=8;
        owl_free(buff);
        continue;
      } else if (!strcasecmp(buff, "@")) {
	attrstack[stacksize]=OWL_FMTEXT_ATTR_NONE;
	chrstack[stacksize]=tmpptr[0];
	colorstack[stacksize]=curcolor;
        stacksize++;
        txtptr+=2;
        owl_free(buff);
        continue;

        /* if it's a color read the color, set the current color and
           continue */
      } else if (!strcasecmp(buff, "@color") 
                 && owl_global_get_hascolors(&g)
                 && owl_global_is_colorztext(&g)) {
        owl_free(buff);
        txtptr+=7;
        tmpptr=strpbrk(txtptr, "@{[<()>]}");
        if (tmpptr &&
            ((txtptr[-1]=='(' && tmpptr[0]==')') ||
             (txtptr[-1]=='<' && tmpptr[0]=='>') ||
             (txtptr[-1]=='[' && tmpptr[0]==']') ||
             (txtptr[-1]=='{' && tmpptr[0]=='}'))) {

          /* grab the color name */
          buff=owl_malloc(tmpptr-txtptr+20);
          strncpy(buff, txtptr, tmpptr-txtptr);
          buff[tmpptr-txtptr]='\0';

          /* set it as the current color */
          curcolor=owl_util_string_to_color(buff);
          if (curcolor==-1) curcolor=OWL_COLOR_DEFAULT;
          owl_free(buff);
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
      buff=owl_malloc(ptr-txtptr+20);
      strncpy(buff, txtptr, ptr-txtptr);
      buff[ptr-txtptr]='\0';
      owl_fmtext_append_attr(f, buff, curattrs, curcolor, OWL_COLOR_DEFAULT);
      owl_free(buff);

      /* now deal with the closer */
      txtptr=ptr;

      /* first, if the stack is empty we must bail (just print and go) */
      if (stacksize==0) {
        buff=owl_malloc(5);
        buff[0]=ptr[0];
        buff[1]='\0';
        owl_fmtext_append_attr(f, buff, curattrs, curcolor, OWL_COLOR_DEFAULT);
        owl_free(buff);
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
        buff=owl_malloc(5);
        buff[0]=ptr[0];
        buff[1]='\0';
        owl_fmtext_append_attr(f, buff, curattrs, curcolor, OWL_COLOR_DEFAULT);
        owl_free(buff);
        txtptr++;
        continue;
      }
    } else {
      /* we've found an unattached opener, print everything and move on */
      buff=owl_malloc(ptr-txtptr+20);
      strncpy(buff, txtptr, ptr-txtptr+1);
      buff[ptr-txtptr+1]='\0';
      owl_fmtext_append_attr(f, buff, curattrs, curcolor, OWL_COLOR_DEFAULT);
      owl_free(buff);
      txtptr=ptr+1;
      continue;
    }
  }
}

/* requires that the list values are strings or NULL.
 * joins the elements together with join_with. 
 * If format_fn is specified, passes it the list element value
 * and it will return a string which this needs to free. */
void owl_fmtext_append_list(owl_fmtext *f, owl_list *l, char *join_with, char *(format_fn)(void*))
{
  int i, size;
  void *elem;
  char *text;

  size = owl_list_get_size(l);
  for (i=0; i<size; i++) {
    elem = (char*)owl_list_get_element(l,i);
    if (elem && format_fn) {
      text = format_fn(elem);
      if (text) {
        owl_fmtext_append_normal(f, text);
        owl_free(text);
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
void owl_fmtext_free(owl_fmtext *f)
{
  if (f->textbuff) owl_free(f->textbuff);
}

/*** Color Pair manager ***/
void owl_fmtext_init_colorpair_mgr(owl_colorpair_mgr *cpmgr)
{
  /* This could be a bitarray if we wanted to save memory. */
  short i, j;
  cpmgr->next = 8;
  
  /* The test is <= because we allocate COLORS+1 entries. */
  cpmgr->pairs = owl_malloc((COLORS+1) * sizeof(short*));
  for(i = 0; i <= COLORS; i++) {
    cpmgr->pairs[i] = owl_malloc((COLORS+1) * sizeof(short));
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

/* Reset used list */
void owl_fmtext_reset_colorpairs()
{
  if (owl_global_get_hascolors(&g)) {
    short i, j;
    owl_colorpair_mgr *cpmgr = owl_global_get_colorpair_mgr(&g);
    cpmgr->next = 8;
    
    /* The test is <= because we allocated COLORS+1 entries. */
    for(i = 0; i <= COLORS; i++) {
      for(j = 0; j <= COLORS; j++) {
	cpmgr->pairs[i][j] = -1;
      }
    }
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
  short pair, default_bg;

  /* Sanity (Bounds) Check */
  if (fg > COLORS || fg < OWL_COLOR_DEFAULT) fg = OWL_COLOR_DEFAULT;
  if (bg > COLORS || bg < OWL_COLOR_DEFAULT) bg = OWL_COLOR_DEFAULT;
	    
#ifdef HAVE_USE_DEFAULT_COLORS
  if (fg == OWL_COLOR_DEFAULT) fg = -1;
  default_bg = OWL_COLOR_DEFAULT;
#else
  if (fg == OWL_COLOR_DEFAULT) fg = 0;
  if (bg == OWL_COLOR_DEFAULT) bg = 0;
  default_bg = COLOR_BLACK;
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
      pair = owl_fmtext_get_colorpair(fg, OWL_COLOR_DEFAULT);
    }
    else {
      /* We still don't have a pair, defaults all around. */
      owl_function_debugmsg("colorpairs: color shortage - dropping foreground and background color.");
      pair = 0;
    }
  }
  return pair;
}
