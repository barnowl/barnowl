#include "owl.h"
#include <stdlib.h>
#include <string.h>

static const char fileIdent[] = "$Id$";

void owl_fmtext_init_null(owl_fmtext *f) {
  f->textlen=0;
  f->textbuff=owl_strdup("");
  f->fmbuff=owl_malloc(5);
  f->colorbuff=owl_malloc(5);
  f->fmbuff[0]=OWL_FMTEXT_ATTR_NONE;
  f->colorbuff[0]=OWL_COLOR_DEFAULT;
}


void _owl_fmtext_set_attr(owl_fmtext *f, int attr, int first, int last) {
  int i;
  for (i=first; i<=last; i++) {
    f->fmbuff[i]=(unsigned char) attr;
  }
}

void _owl_fmtext_add_attr(owl_fmtext *f, int attr, int first, int last) {
  int i;
  for (i=first; i<=last; i++) {
    f->fmbuff[i]|=(unsigned char) attr;
  }
}

void _owl_fmtext_set_color(owl_fmtext *f, int color, int first, int last) {
  int i;
  for (i=first; i<=last; i++) {
    f->colorbuff[i]=(unsigned char) color;
  }
}


void owl_fmtext_append_attr(owl_fmtext *f, char *text, int attr, int color) {
  int newlen;

  newlen=strlen(f->textbuff)+strlen(text);
  f->textbuff=owl_realloc(f->textbuff, newlen+2);
  f->fmbuff=owl_realloc(f->fmbuff, newlen+2);
  f->colorbuff=owl_realloc(f->colorbuff, newlen+2);

  strcat(f->textbuff, text);
  _owl_fmtext_set_attr(f, attr, f->textlen, newlen);
  _owl_fmtext_set_color(f, color, f->textlen, newlen);
  f->textlen=newlen;
}


void owl_fmtext_append_normal(owl_fmtext *f, char *text) {
  owl_fmtext_append_attr(f, text, OWL_FMTEXT_ATTR_NONE, OWL_COLOR_DEFAULT);
}

void owl_fmtext_append_normal_color(owl_fmtext *f, char *text, int color) {
  owl_fmtext_append_attr(f, text, OWL_FMTEXT_ATTR_NONE, color);
}


void owl_fmtext_append_bold(owl_fmtext *f, char *text) {
  owl_fmtext_append_attr(f, text, OWL_FMTEXT_ATTR_BOLD, OWL_COLOR_DEFAULT);
}


void owl_fmtext_append_reverse(owl_fmtext *f, char *text) {
  owl_fmtext_append_attr(f, text, OWL_FMTEXT_ATTR_REVERSE, OWL_COLOR_DEFAULT);
}


void owl_fmtext_append_reversebold(owl_fmtext *f, char *text) {
  owl_fmtext_append_attr(f, text, OWL_FMTEXT_ATTR_REVERSE | OWL_FMTEXT_ATTR_BOLD, OWL_COLOR_DEFAULT);
}


void owl_fmtext_addattr(owl_fmtext *f, int attr) {
  /* add the attribute to all text */
  int i, j;

  j=f->textlen;
  for (i=0; i<j; i++) {
    f->fmbuff[i] |= attr;
  }
}

void owl_fmtext_colorize(owl_fmtext *f, int color) {
  /* everywhere the color is OWL_COLOR_DEFAULT, change it to be 'color' */
  int i, j;

  j=f->textlen;
  for(i=0; i<j; i++) {
    if (f->colorbuff[i]==OWL_COLOR_DEFAULT) f->colorbuff[i] = color;
  }
}


void owl_fmtext_append_ztext(owl_fmtext *f, char *text) {
  int stacksize, curattrs, curcolor;
  char *ptr, *txtptr, *buff, *tmpptr;
  int attrstack[32], chrstack[32];

  curattrs=OWL_FMTEXT_ATTR_NONE;
  curcolor=OWL_COLOR_DEFAULT;
  stacksize=0;
  txtptr=text;
  while (1) {
    ptr=strpbrk(txtptr, "@{[<()>]}");
    if (!ptr) {
      /* add all the rest of the text and exit */
      owl_fmtext_append_attr(f, txtptr, curattrs, curcolor);
      return;
    } else if (ptr[0]=='@') {
      /* add the text up to this point then deal with the stack */
      buff=owl_malloc(ptr-txtptr+20);
      strncpy(buff, txtptr, ptr-txtptr);
      buff[ptr-txtptr]='\0';
      owl_fmtext_append_attr(f, buff, curattrs, curcolor);
      owl_free(buff);

      /* update pointer to point at the @ */
      txtptr=ptr;

      /* now the stack */

      /* if we've hit our max stack depth, print the @ and move on */
      if (stacksize==32) {
	owl_fmtext_append_attr(f, "@", curattrs, curcolor);
	txtptr++;
	continue;
      }

      /* if it's an @@, print an @ and continue */
      if (txtptr[1]=='@') {
	owl_fmtext_append_attr(f, "@", curattrs, curcolor);
	txtptr+=2;
	continue;
      }
	
      /* if there's no opener, print the @ and continue */
      tmpptr=strpbrk(txtptr, "(<[{ ");
      if (!tmpptr || tmpptr[0]==' ') {
	owl_fmtext_append_attr(f, "@", curattrs, curcolor);
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
	stacksize++;
	curattrs|=OWL_FMTEXT_ATTR_BOLD;
	txtptr+=6;
	owl_free(buff);
	continue;
      } else if (!strcasecmp(buff, "@b")) {
	attrstack[stacksize]=OWL_FMTEXT_ATTR_BOLD;
	chrstack[stacksize]=tmpptr[0];
	stacksize++;
	curattrs|=OWL_FMTEXT_ATTR_BOLD;
	txtptr+=3;
	owl_free(buff);
	continue;
      } else if (!strcasecmp(buff, "@i")) {
	attrstack[stacksize]=OWL_FMTEXT_ATTR_UNDERLINE;
	chrstack[stacksize]=tmpptr[0];
	stacksize++;
	curattrs|=OWL_FMTEXT_ATTR_UNDERLINE;
	txtptr+=3;
	owl_free(buff);
	continue;
      } else if (!strcasecmp(buff, "@italic")) {
	attrstack[stacksize]=OWL_FMTEXT_ATTR_UNDERLINE;
	chrstack[stacksize]=tmpptr[0];
	stacksize++;
	curattrs|=OWL_FMTEXT_ATTR_UNDERLINE;
	txtptr+=8;
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
	  owl_free(buff);
	  txtptr=tmpptr+1;
	  continue;

	} else {

	}

      } else {
	/* if we didn't understand it, we'll print it.  This is different from zwgc
	   but zwgc seems to be smarter about some screw cases than I am */
	owl_fmtext_append_attr(f, "@", curattrs, curcolor);
	txtptr++;
	continue;
      }

    } else if (ptr[0]=='}' || ptr[0]==']' || ptr[0]==')' || ptr[0]=='>') {
      /* add the text up to this point first */
      buff=owl_malloc(ptr-txtptr+20);
      strncpy(buff, txtptr, ptr-txtptr);
      buff[ptr-txtptr]='\0';
      owl_fmtext_append_attr(f, buff, curattrs, curcolor);
      owl_free(buff);

      /* now deal with the closer */
      txtptr=ptr;

      /* first, if the stack is empty we must bail (just print and go) */
      if (stacksize==0) {
	buff=owl_malloc(5);
	buff[0]=ptr[0];
	buff[1]='\0';
	owl_fmtext_append_attr(f, buff, curattrs, curcolor);
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
	owl_fmtext_append_attr(f, buff, curattrs, curcolor);
	owl_free(buff);
	txtptr++;
	continue;
      }
    } else {
      /* we've found an unattached opener, print everything and move on */
      buff=owl_malloc(ptr-txtptr+20);
      strncpy(buff, txtptr, ptr-txtptr+1);
      buff[ptr-txtptr+1]='\0';
      owl_fmtext_append_attr(f, buff, curattrs, curcolor);
      owl_free(buff);
      txtptr=ptr+1;
      continue;
    }
  }

}

void owl_fmtext_append_fmtext(owl_fmtext *f, owl_fmtext *in, int start, int stop) {
  int newlen, i;

  newlen=strlen(f->textbuff)+(stop-start+1);
  f->textbuff=owl_realloc(f->textbuff, newlen+1);
  f->fmbuff=owl_realloc(f->fmbuff, newlen+1);
  f->colorbuff=owl_realloc(f->colorbuff, newlen+1);

  strncat(f->textbuff, in->textbuff+start, stop-start+1);
  f->textbuff[newlen]='\0';
  for (i=start; i<=stop; i++) {
    f->fmbuff[f->textlen+(i-start)]=in->fmbuff[i];
    f->colorbuff[f->textlen+(i-start)]=in->colorbuff[i];
  }
  f->textlen=newlen;
}

void owl_fmtext_append_spaces(owl_fmtext *f, int nspaces) {
  int i;
  for (i=0; i<nspaces; i++) {
    owl_fmtext_append_normal(f, " ");
  }
}

/* requires that the list values are strings or NULL.
 * joins the elements together with join_with. 
 * If format_fn is specified, passes it the list element value
 * and it will return a string which this needs to free. */
void owl_fmtext_append_list(owl_fmtext *f, owl_list *l, char *join_with, char *(format_fn)(void*)) {
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


/* caller is responsible for freeing */
char *owl_fmtext_print_plain(owl_fmtext *f) {
  return owl_strdup(f->textbuff);
}


void owl_fmtext_curs_waddstr(owl_fmtext *f, WINDOW *w) {
  char *tmpbuff;
  int position, trans1, trans2, len, lastsame;

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
    trans2=owl_util_find_trans(f->colorbuff+position, len-position);

    if (trans1<trans2) {
      lastsame=position+trans1;
    } else {
      lastsame=position+trans2;
    }

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
      if (f->colorbuff[position]!=OWL_COLOR_DEFAULT) {
	wattron(w, COLOR_PAIR(f->colorbuff[position]));
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


int owl_fmtext_truncate_lines(owl_fmtext *in, int aline, int lines, owl_fmtext *out) {
  /* start with line aline (where the first line is 0) and print
   *  'lines' lines
   */
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
    ptr2=strchr(ptr1, '\n');
    offset=ptr1-in->textbuff;
    if (!ptr2) {
      owl_fmtext_append_fmtext(out, in, offset, in->textlen-1);
      return(-1);
    }
    owl_fmtext_append_fmtext(out, in, offset, (ptr2-ptr1)+offset);
    ptr1=ptr2+1;
  }
  return(0);
}


void owl_fmtext_truncate_cols(owl_fmtext *in, int acol, int bcol, owl_fmtext *out) {
  char *ptr1, *ptr2, *last;
  int len, offset;
  
  /* the first column is column 0 */

  /* the message is expected to end in a new line for now */

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
      len=ptr2-(ptr1+acol);
    }
    if (len>last-ptr1) {
      len-=(last-ptr1);
    }
    if (len<=0) {
      owl_fmtext_append_normal(out, "\n");
      ptr1=ptr2+1;
      continue;
    }

    offset=ptr1-in->textbuff;
    owl_fmtext_append_fmtext(out, in, offset+acol, offset+acol+len);

    ptr1=ptr2+1;
  }
}


int owl_fmtext_num_lines(owl_fmtext *f) {
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


char *owl_fmtext_get_text(owl_fmtext *f) {
  return(f->textbuff);
}

void owl_fmtext_set_char(owl_fmtext *f, int index, int ch) {
  /* set the charater at 'index' to be 'char'.  If index is out of
   * bounds don't do anything */
  if ((index < 0) || (index > f->textlen-1)) return;
  f->textbuff[index]=ch;
}

void owl_fmtext_free(owl_fmtext *f) {
  if (f->textbuff) owl_free(f->textbuff);
  if (f->fmbuff) owl_free(f->fmbuff);
  if (f->colorbuff) owl_free(f->colorbuff);
}


void owl_fmtext_copy(owl_fmtext *dst, owl_fmtext *src) {
  dst->textlen=src->textlen;
  dst->textbuff=owl_malloc(src->textlen+5);
  dst->fmbuff=owl_malloc(src->textlen+5);
  dst->colorbuff=owl_malloc(src->textlen+5);
  memcpy(dst->textbuff, src->textbuff, src->textlen);
  memcpy(dst->fmbuff, src->fmbuff, src->textlen);
  memcpy(dst->colorbuff, src->colorbuff, src->textlen);
}


int owl_fmtext_search_and_highlight(owl_fmtext *f, char *string) {
  /* highlight all instance of "string".  Return the number of
   * instances found.  This is case insensitive. */

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

int owl_fmtext_search(owl_fmtext *f, char *string) {
  /* return 1 if the string is found, 0 if not.  This is case
   *  insensitive */

  if (stristr(f->textbuff, string)) return(1);
  return(0);
}
