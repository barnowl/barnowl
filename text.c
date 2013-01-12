#include "owl.h"

/* Returns a copy of 'in' with each line indented 'n'
 * characters. Result must be freed with g_free. */
CALLER_OWN char *owl_text_indent(const char *in, int n, bool indent_first_line)
{
  const char *ptr1, *ptr2, *last;
  GString *out = g_string_new("");
  int i;
  bool indent_this_line = indent_first_line;

  last=in+strlen(in)-1;
  ptr1=in;
  while (ptr1<=last) {
    if (indent_this_line) {
      for (i = 0; i < n; i++) {
        g_string_append_c(out, ' ');
      }
    }
    indent_this_line = true;
    ptr2=strchr(ptr1, '\n');
    if (!ptr2) {
      g_string_append(out, ptr1);
      break;
    } else {
      g_string_append_len(out, ptr1, ptr2-ptr1+1);
    }
    ptr1=ptr2+1;
  }
  return g_string_free(out, false);
}

int owl_text_num_lines(const char *in)
{
  int lines, i;

  lines=0;
  for (i=0; in[i]!='\0'; i++) {
    if (in[i]=='\n') lines++;
  }

  /* if the last char wasn't a \n there's one more line */
  if (i>0 && in[i-1]!='\n') lines++;

  return(lines);
}


/* caller must free the return */
CALLER_OWN char *owl_text_htmlstrip(const char *in)
{
  const char *ptr1, *end, *ptr2, *ptr3;
  char *out, *out2;

  out=g_new(char, strlen(in)+30);
  strcpy(out, "");

  ptr1=in;
  end=in+strlen(in);

  while(ptr1<end) {
    /* look for an open bracket */
    ptr2=strchr(ptr1, '<');

    /* if none, copy in from here to end and exit */
    if (ptr2==NULL) {
      strcat(out, ptr1);
      break;
    }

    /* otherwise copy in everything before the open bracket */
    if (ptr2>ptr1) {
      strncat(out, ptr1, ptr2-ptr1);
    }

    /* find the close bracket */
    ptr3=strchr(ptr2, '>');

    /* if there is no close, copy as you are and exit */
    if (!ptr3) {
      strcat(out, ptr2);
      break;
    }

    /* look for things we know */
    if (!strncasecmp(ptr2, "<BODY", 5) ||
	!strncasecmp(ptr2, "<FONT", 5) ||
	!strncasecmp(ptr2, "<HTML", 5) ||
	!strncasecmp(ptr2, "</FONT", 6) ||
	!strncasecmp(ptr2, "</HTML", 6) ||
	!strncasecmp(ptr2, "</BODY", 6)) {

      /* advance to beyond the angle brakcet and go again */
      ptr1=ptr3+1;
      continue;
    }
    if (!strncasecmp(ptr2, "<BR>", 4)) {
      strcat(out, "\n");
      ptr1=ptr3+1;
      continue;
    }

    /* if it wasn't something we know, copy to the > and  go again */
    strncat(out, ptr2, ptr3-ptr2+1);
    ptr1=ptr3+1;
  }

  out2=owl_text_substitute(out, "&lt;", "<");
  g_free(out);
  out=owl_text_substitute(out2, "&gt;", ">");
  g_free(out2);
  out2=owl_text_substitute(out, "&amp;", "&");
  g_free(out);
  out=owl_text_substitute(out2, "&quot;", "\"");
  g_free(out2);
  out2=owl_text_substitute(out, "&nbsp;", " ");
  g_free(out);
  out=owl_text_substitute(out2, "&ensp;", "  ");
  g_free(out2);
  out2=owl_text_substitute(out, "&emsp;", "   ");
  g_free(out);
  out=owl_text_substitute(out2, "&endash;", "--");
  g_free(out2);
  out2=owl_text_substitute(out, "&emdash;", "---");
  g_free(out);

  return(out2);
}

/* Caller must free return */
CALLER_OWN char *owl_text_expand_tabs(const char *in)
{
  int len = 0;
  const char *p = in;
  char *ret, *out;
  int col;

  col = 0;
  while(*p) {
    gunichar c = g_utf8_get_char(p);
    const char *q = g_utf8_next_char(p);
    switch (c) {
    case '\t':
      do { len++; col++; } while (col % OWL_TAB_WIDTH);
      p = q;
      continue;
    case '\n':
      col = 0;
      break;
    default:
      col += mk_wcwidth(c);
      break;
    }
    len += q - p;
    p = q;
  }

  ret = g_new(char, len + 1);

  p = in;
  out = ret;

  col = 0;
  while(*p) {
    gunichar c = g_utf8_get_char(p);
    const char *q = g_utf8_next_char(p);
    switch (c) {
    case '\t':
      do {*(out++) = ' '; col++; } while (col % OWL_TAB_WIDTH);
      p = q;
      continue;
    case '\n':
      col = 0;
      break;
    default:
      col += mk_wcwidth(c);
      break;
    }
    memcpy(out, p, q - p);
    out += q - p;
    p = q;
  }

  *out = 0;

  return ret;
}

/* caller must free the return */
CALLER_OWN char *owl_text_wordwrap(const char *in, int col)
{
  char *out;
  int cur, lastspace, len, lastnewline;

  out=g_strdup(in);
  len=strlen(in);
  cur=0;
  lastspace=-1;
  lastnewline=-1;

  while (cur<(len-1)) {
    if (out[cur]==' ') {
      lastspace=cur;
      cur++;
      continue;
    } else if (out[cur]=='\n') {
      lastnewline=cur;
      cur++;
      continue;
    }

    /* do we need to wrap? */
    if ( (cur-(lastnewline+1)) > col ) {
      if (lastspace==-1 ||
	  (lastnewline>0 && (lastspace<=lastnewline))) {
	/* we can't help, sorry */
	cur++;
	continue;
      }

      /* turn the last space into a newline */
      out[lastspace]='\n';
      lastnewline=lastspace;
      lastspace=-1;
      cur++;
      continue;
    }

    cur++;
    continue;
  }
  return(out);
}

/* this modifies 'in' */
void owl_text_wordunwrap(char *in)
{
  int i, j;

  j=strlen(in);
  for (i=0; i<j; i++) {
    if ( (in[i]=='\n') &&
	 ((i>0) && (i<(j-1))) &&
	 (in[i-1]!='\n') &&
	 (in[i+1]!='\n') )
      in[i]=' ';
  }
}

/* return 1 if a string is only whitespace, otherwise 0 */
int only_whitespace(const char *s)
{
  if (g_utf8_validate(s,-1,NULL)) {
    const char *p;
    for(p = s; p[0]; p=g_utf8_next_char(p)) {
      if (!g_unichar_isspace(g_utf8_get_char(p))) return 0;
    }
  }
  else {
    int i;
    for (i=0; s[i]; i++) {
      if (!isspace((int) s[i])) return(0);
    }
  }
  return(1);
}

/* Return a string with any occurances of 'from' replaced with 'to'.
 * Caller must free returned string.
 */
CALLER_OWN char *owl_text_substitute(const char *in, const char *from, const char *to)
{
  char **split = g_strsplit(in, from, 0), *out;
  out = g_strjoinv(to, split);
  g_strfreev(split);
  return out;
}

/* Return a string which is like 'in' except that every instance of
 * any character in 'toquote' found in 'in' is preceeded by the string
 * 'quotestr'.  For example, owl_text_quote(in, "+*.", "\") would
 * place a backslash before every '+', '*' or '.' in 'in'.  It is
 * permissable for a character in 'quotestr' to be in 'toquote'.
 * On success returns the string, on error returns NULL.
 */
CALLER_OWN char *owl_text_quote(const char *in, const char *toquote, const char *quotestr)
{
  int i, x, r, place, escape;
  int in_len, toquote_len, quotestr_len;
  char *out;

  in_len=strlen(in);
  toquote_len=strlen(toquote);
  quotestr_len=strlen(quotestr);
  place=0;
  escape = 0;
  for (i=0; i<in_len; i++) {
    if(strchr(toquote, in[i]) != NULL)
      escape++;
  }
  out = g_new(char, in_len + quotestr_len*escape+1);
  for (i=0; i<in_len; i++) {

    /* check if it's a character that needs quoting */
    for (x=0; x<toquote_len; x++) {
      if (in[i]==toquote[x]) {
	/* quote it */
	for (r=0; r<quotestr_len; r++) {
	  out[place+r]=quotestr[r];
	}
	place+=quotestr_len;
	break;
      }
    }

    /* either way, we now copy over the character */
    out[place]=in[i];
    place++;
  }
  out[place]='\0';
  return(out);
}
