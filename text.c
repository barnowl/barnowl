#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";

/* start with line aline (where the first line is 1) and print 'lines'
 *  lines
 */
int owl_text_truncate_lines(char *out, char *in, int aline, int lines)
{
  char *ptr1, *ptr2;
  int i;

  strcpy(out, "");
  
  if (aline==0) aline=1; /* really illegal use */

  /* find the starting line */
  ptr1=in;
  if (aline!=1) {
     for (i=0; i<aline-1; i++) {
      ptr1=strchr(ptr1, '\n');
      if (!ptr1) return(-1);
      ptr1++;
    }
  }
  /* ptr1 now holds the starting point */

  /* copy in the next 'lines' lines */
  if (lines<1) return(-1);
  
  for (i=0; i<lines; i++) {
    ptr2=strchr(ptr1, '\n');
    if (!ptr2) {
      strcat(out, ptr1);
      return(-1);
    }
    strncat(out, ptr1, ptr2-ptr1+1);
    ptr1=ptr2+1;
  }
  return(0);
}

  
/* the first column is column 0.  The message is expected to end in a
 * new line for now */
void owl_text_truncate_cols(char *out, char *in, int acol, int bcol)
{
  char *ptr1, *ptr2, *tmpbuff, *last;
  int len;

  tmpbuff=owl_malloc(strlen(in)+20);

  strcpy(tmpbuff, "");
  last=in+strlen(in)-1;
  ptr1=in;
  while (ptr1<last) {
    ptr2=strchr(ptr1, '\n');
    if (!ptr2) {
      /* but this shouldn't happen if we end in a \n */
      break;
    }
    
    if (ptr2==ptr1) {
      strcat(tmpbuff, "\n");
      ptr1++;
      continue;
    }

    /* we need to check that we won't run over here */
    if ( (ptr2-ptr1) < (bcol-acol) ) {
      len=ptr2-(ptr1+acol);
    } else {
      len=bcol-acol;
    }
    if ((ptr1+len)>=last) {
      len-=last-(ptr1+len);
    }

    strncat(tmpbuff, ptr1+acol, len);
    strcat(tmpbuff, "\n");

    ptr1=ptr2+1;
  }
  strcpy(out, tmpbuff);
  owl_free(tmpbuff);
}


void owl_text_indent(char *out, char *in, int n)
{
  char *ptr1, *ptr2, *last;
  int i;

  strcpy(out, "");

  last=in+strlen(in)-1;
  ptr1=in;
  while (ptr1<=last) {
    for (i=0; i<n; i++) {
      strcat(out, " ");
    }
    ptr2=strchr(ptr1, '\n');
    if (!ptr2) {
      strcat(out, ptr1);
      break;
    } else {
      strncat(out, ptr1, ptr2-ptr1+1);
    }
    ptr1=ptr2+1;
  }
}

int owl_text_num_lines(char *in)
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
char *owl_text_htmlstrip(char *in)
{
  char *ptr1, *end, *ptr2, *ptr3, *out, *out2;

  out=owl_malloc(strlen(in)+30);
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
  owl_free(out);
  out=owl_text_substitute(out2, "&gt;", ">");
  owl_free(out2);
  out2=owl_text_substitute(out, "&amp;", "&");
  owl_free(out);
  out=owl_text_substitute(out2, "&quot;", "\"");
  owl_free(out2);
  out2=owl_text_substitute(out, "&nbsp;", " ");
  owl_free(out);
  out=owl_text_substitute(out2, "&ensp;", "  ");
  owl_free(out2);
  out2=owl_text_substitute(out, "&emsp;", "   ");
  owl_free(out);
  out=owl_text_substitute(out2, "&endash;", "--");
  owl_free(out2);
  out2=owl_text_substitute(out, "&emdash;", "---");
  owl_free(out);

  return(out2);
}

/* caller must free the return */
char *owl_text_wordwrap(char *in, int col)
{
  char *out;
  int cur, lastspace, len, lastnewline;

  out=owl_strdup(in);
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

/* exactly like strstr but case insensitive */
char *stristr(char *a, char *b)
{
  char *x, *y, *ret;

  if ((x=owl_strdup(a))==NULL) return(NULL);
  if ((y=owl_strdup(b))==NULL) return(NULL);
  downstr(x);
  downstr(y);
  ret=strstr(x, y);
  if (ret==NULL) {
    owl_free(x);
    owl_free(y);
    return(NULL);
  }
  ret=ret-x+a;
  owl_free(x);
  owl_free(y);
  return(ret);
}

/* return 1 if a string is only whitespace, otherwise 0 */
int only_whitespace(char *s)
{
  int i;
  for (i=0; s[i]; i++) {
    if (!isspace((int) s[i])) return(0);
  }
  return(1);
}

char *owl_getquoting(char *line)
{
  if (line[0]=='\0') return("'");
  if (strchr(line, '\'')) return("\"");
  if (strchr(line, '"')) return("'");
  if (strchr(line, ' ')) return("'");
  return("");
}

/* Return a string with any occurances of 'from' replaced with 'to'.
 * Does not currently handle backslash quoting, but may in the future.
 * Caller must free returned string.
 */
char *owl_text_substitute(char *in, char *from, char *to)
{
  
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

/* replace all instances of character a in buff with the character
 * b.  buff must be null terminated.
 */
void owl_text_tr(char *buff, char a, char b)
{
  int i;

  owl_function_debugmsg("In: %s", buff);
  for (i=0; buff[i]!='\0'; i++) {
    if (buff[i]==a) buff[i]=b;
  }
  owl_function_debugmsg("Out: %s", buff);
}

/* Return a string which is like 'in' except that every instance of
 * any character in 'toquote' found in 'in' is preceeded by the string
 * 'quotestr'.  For example, owl_text_quote(in, "+*.", "\") would
 * place a backslash before every '+', '*' or '.' in 'in'.  It is
 * permissable for a character in 'quotestr' to be in 'toquote'.
 * On success returns the string, on error returns NULL.
 */
char *owl_text_quote(char *in, char *toquote, char *quotestr)
{
  int i, x, r, place, escape;
  int in_len, toquote_len, quotestr_len;
  char *out;

  in_len=strlen(in);
  toquote_len=strlen(toquote);
  quotestr_len=strlen(quotestr);
  out=owl_malloc((in_len*quotestr_len)+30);
  place=0;
  escape = 0;
  for (i=0; i<in_len; i++) {
    if(strchr(toquote, in[i]) != NULL)
      escape++;
  }
  out = owl_malloc(in_len + quotestr_len*escape+1);
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
