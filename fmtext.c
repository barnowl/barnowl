#include "owl.h"
#include <stdlib.h>
#include <string.h>

static const char fileIdent[] = "$Id$";

/* initialize an fmtext with no data */
void owl_fmtext_init_null(owl_fmtext *f)
{
  f->textlen=0;
  f->bufflen=5;
  f->textbuff=owl_malloc(5);
  f->fmbuff=owl_malloc(5);
  f->fgcolorbuff=owl_malloc(5 * sizeof(short));
  f->bgcolorbuff=owl_malloc(5 * sizeof(short));
  f->textbuff[0]=0;
  f->fmbuff[0]=OWL_FMTEXT_ATTR_NONE;
  f->fgcolorbuff[0]=OWL_COLOR_DEFAULT;
  f->bgcolorbuff[0]=OWL_COLOR_DEFAULT;
}

/* Clear the data from an fmtext, but don't deallocate memory. This
   fmtext can then be appended to again. */
void owl_fmtext_clear(owl_fmtext *f)
{
    f->textlen = 0;
    f->textbuff[0] = 0;
    f->fmbuff[0]=OWL_FMTEXT_ATTR_NONE;
    f->fgcolorbuff[0]=OWL_COLOR_DEFAULT;
    f->bgcolorbuff[0]=OWL_COLOR_DEFAULT;
}

/* Internal function.  Set the attribute 'attr' from index 'first' to
 * index 'last'
 */
void _owl_fmtext_set_attr(owl_fmtext *f, int attr, int first, int last)
{
  int i;
  for (i=first; i<=last; i++) {
    f->fmbuff[i]=(unsigned char) attr;
  }
}

/* Internal function.  Add the attribute 'attr' to the existing
 * attributes from index 'first' to index 'last'
 */
void _owl_fmtext_add_attr(owl_fmtext *f, int attr, int first, int last)
{
  int i;
  for (i=first; i<=last; i++) {
    f->fmbuff[i]|=(unsigned char) attr;
  }
}

/* Internal function.  Set the color to be 'color' from index 'first'
 * to index 'last
 */
void _owl_fmtext_set_fgcolor(owl_fmtext *f, int color, int first, int last)
{
  int i;
  for (i=first; i<=last; i++) {
    f->fgcolorbuff[i]=(short)color;
  }
}

void _owl_fmtext_set_bgcolor(owl_fmtext *f, int color, int first, int last)
{
  int i;
  for (i=first; i<=last; i++) {
    f->bgcolorbuff[i]=(short)color;
  }
}

void _owl_fmtext_realloc(owl_fmtext *f, int newlen) /*noproto*/
{
    if(newlen + 1 > f->bufflen) {
      f->textbuff=owl_realloc(f->textbuff, newlen+1);
      f->fmbuff=owl_realloc(f->fmbuff, newlen+1);
      f->fgcolorbuff=owl_realloc(f->fgcolorbuff, (newlen+1) * sizeof(short));
      f->bgcolorbuff=owl_realloc(f->bgcolorbuff, (newlen+1) * sizeof(short));
      f->bufflen = newlen+1;
  }
}

/* append text to the end of 'f' with attribute 'attr' and color
 * 'color'
 */
void owl_fmtext_append_attr(owl_fmtext *f, char *text, int attr, int fgcolor, int bgcolor)
{
  int newlen;
  newlen=strlen(f->textbuff)+strlen(text);
  _owl_fmtext_realloc(f, newlen);
  
  strcat(f->textbuff, text);
  _owl_fmtext_set_attr(f, attr, f->textlen, newlen);
  _owl_fmtext_set_fgcolor(f, fgcolor, f->textlen, newlen);
  _owl_fmtext_set_bgcolor(f, bgcolor, f->textlen, newlen);
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

/* Add the attribute 'attr' to all text in 'f' */
void owl_fmtext_addattr(owl_fmtext *f, int attr)
{
  /* add the attribute to all text */
  int i, j;

  j=f->textlen;
  for (i=0; i<j; i++) {
    f->fmbuff[i] |= attr;
  }
}

/* Anywhere the color is NOT ALREDY SET, set the color to 'color'.
 * Other colors are left unchanged
 */
void owl_fmtext_colorize(owl_fmtext *f, int color)
{
  /* everywhere the fgcolor is OWL_COLOR_DEFAULT, change it to be 'color' */
  int i, j;

  j=f->textlen;
  for(i=0; i<j; i++) {
    if (f->fgcolorbuff[i]==OWL_COLOR_DEFAULT) f->fgcolorbuff[i] = (short)color;
  }
}

void owl_fmtext_colorizebg(owl_fmtext *f, int color)
{
  /* everywhere the bgcolor is OWL_COLOR_DEFAULT, change it to be 'color' */
  int i, j;

  j=f->textlen;
  for(i=0; i<j; i++) {
    if (f->bgcolorbuff[i]==OWL_COLOR_DEFAULT) f->bgcolorbuff[i] = (short)color;
  }
}

/* Internal function.  Append text from 'in' between index 'start' and
 * 'stop' to the end of 'f'
 */
void _owl_fmtext_append_fmtext(owl_fmtext *f, owl_fmtext *in, int start, int stop)
{
  int newlen, i;

  newlen=strlen(f->textbuff)+(stop-start+1);
  _owl_fmtext_realloc(f, newlen);

  strncat(f->textbuff, in->textbuff+start, stop-start+1);
  f->textbuff[newlen]='\0';
  for (i=start; i<=stop; i++) {
    f->fmbuff[f->textlen+(i-start)]=in->fmbuff[i];
    f->fgcolorbuff[f->textlen+(i-start)]=in->fgcolorbuff[i];
    f->bgcolorbuff[f->textlen+(i-start)]=in->bgcolorbuff[i];
  }
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
  return(owl_strdup(f->textbuff));
}

/* add the formatted text to the curses window 'w'.  The window 'w'
 * must already be initiatlized with curses
 */
void owl_fmtext_curs_waddstr(owl_fmtext *f, WINDOW *w)
{
  char *tmpbuff;
  int position, trans1, trans2, trans3, len, lastsame;

  if (w==NULL) {
    owl_function_debugmsg("Hit a null window in owl_fmtext_curs_waddstr.");
    return;
  }

  tmpbuff=owl_malloc(f->textlen+10);

  position=0;
  len=f->textlen;
  while (position<=len) {
    /* find the last char with the current format and color */
    trans1=owl_util_find_trans(f->fmbuff+position, len-position);
    trans2=owl_util_find_trans_short(f->fgcolorbuff+position, len-position);
    trans3=owl_util_find_trans_short(f->bgcolorbuff+position, len-position);

    lastsame = (trans1 < trans2) ? trans1 : trans2;
    lastsame = (lastsame < trans3) ? lastsame : trans3;
    lastsame += position;

    /* set the format */
    wattrset(w, A_NORMAL);
    if (f->fmbuff[position] & OWL_FMTEXT_ATTR_BOLD) {
      wattron(w, A_BOLD);
    }
    if (f->fmbuff[position] & OWL_FMTEXT_ATTR_REVERSE) {
      wattron(w, A_REVERSE);
    }
    if (f->fmbuff[position] & OWL_FMTEXT_ATTR_UNDERLINE) {
      wattron(w, A_UNDERLINE);
    }

    /* set the color */
    /* warning, this is sort of a hack */
    if (owl_global_get_hascolors(&g)) {
      short fg, bg, pair;
      fg = f->fgcolorbuff[position];
      bg = f->bgcolorbuff[position];

      pair = owl_fmtext_get_colorpair(fg, bg);
      if (pair != -1) {
        wcolor_set(w,pair,NULL);
      }
    }

    /* add the text */
    strncpy(tmpbuff, f->textbuff + position, lastsame-position+1);
    tmpbuff[lastsame-position+1]='\0';
    waddstr(w, tmpbuff);

    position=lastsame+1;
  }
  owl_free(tmpbuff);
}


/* start with line 'aline' (where the first line is 0) and print
 * 'lines' number of lines into 'out'
 */
int owl_fmtext_truncate_lines(owl_fmtext *in, int aline, int lines, owl_fmtext *out)
{
  char *ptr1, *ptr2;
  int i, offset;
  
  /* find the starting line */
  ptr1=in->textbuff;
  if (aline!=0) {
    for (i=0; i<aline; i++) {
      ptr1=strchr(ptr1, '\n');
      if (!ptr1) return(-1);
      ptr1++;
    }
  }
  /* ptr1 now holds the starting point */

  /* copy in the next 'lines' lines */
  if (lines<1) return(-1);

  for (i=0; i<lines; i++) {
    offset=ptr1-in->textbuff;
    ptr2=strchr(ptr1, '\n');
    if (!ptr2) {
      _owl_fmtext_append_fmtext(out, in, offset, (in->textlen)-1);
      return(-1);
    }
    _owl_fmtext_append_fmtext(out, in, offset, (ptr2-ptr1)+offset);
    ptr1=ptr2+1;
  }
  return(0);
}

/* Truncate the message so that each line begins at column 'acol' and
 * ends at 'bcol' or sooner.  The first column is number 0.  The new
 * message is placed in 'out'.  The message is * expected to end in a
 * new line for now
 */
void owl_fmtext_truncate_cols(owl_fmtext *in, int acol, int bcol, owl_fmtext *out)
{
  char *ptr1, *ptr2, *last;
  int len, offset;

  last=in->textbuff+in->textlen-1;
  ptr1=in->textbuff;
  while (ptr1<=last) {
    ptr2=strchr(ptr1, '\n');
    if (!ptr2) {
      /* but this shouldn't happen if we end in a \n */
      break;
    }
    
    if (ptr2==ptr1) {
      owl_fmtext_append_normal(out, "\n");
      ptr1++;
      continue;
    }

    /* we need to check that we won't run over here */
    len=bcol-acol;
    if (len > (ptr2-(ptr1+acol))) {
      /* the whole line fits with room to spare, don't take a full 'len' */
      len=ptr2-(ptr1+acol);
    }
    if (len>last-ptr1) {
      /* the whole rest of the text fits with room to spare, adjust for it */
      len-=(last-ptr1);
    }
    if (len<=0) {
      /* saftey check */
      owl_fmtext_append_normal(out, "\n");
      ptr1=ptr2+1;
      continue;
    }

    offset=ptr1-in->textbuff;
    _owl_fmtext_append_fmtext(out, in, offset+acol, offset+acol+len);

    ptr1=ptr2+1;
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
 * bounds don't do anything */
void owl_fmtext_set_char(owl_fmtext *f, int index, int ch)
{
  if ((index < 0) || (index > f->textlen-1)) return;
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
  dst->fmbuff=owl_malloc(mallocsize);
  dst->fgcolorbuff=owl_malloc(mallocsize * sizeof(short));
  dst->bgcolorbuff=owl_malloc(mallocsize * sizeof(short));
  memcpy(dst->textbuff, src->textbuff, src->textlen+1);
  memcpy(dst->fmbuff, src->fmbuff, src->textlen);
  memcpy(dst->fgcolorbuff, src->fgcolorbuff, src->textlen * sizeof(short));
  memcpy(dst->bgcolorbuff, src->bgcolorbuff, src->textlen * sizeof(short));
}

/* highlight all instances of "string".  Return the number of
 * instances found.  This is a case insensitive search.
 */
int owl_fmtext_search_and_highlight(owl_fmtext *f, char *string)
{

  int found, len;
  char *ptr1, *ptr2;

  len=strlen(string);
  found=0;
  ptr1=f->textbuff;
  while (ptr1-f->textbuff <= f->textlen) {
    ptr2=stristr(ptr1, string);
    if (!ptr2) return(found);

    found++;
    _owl_fmtext_add_attr(f, OWL_FMTEXT_ATTR_REVERSE,
                         ptr2 - f->textbuff,
                         ptr2 - f->textbuff + len - 1);

    ptr1=ptr2+len;
  }
  return(found);
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
  if (f->fmbuff) owl_free(f->fmbuff);
  if (f->fgcolorbuff) owl_free(f->fgcolorbuff);
  if (f->bgcolorbuff) owl_free(f->bgcolorbuff);
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
