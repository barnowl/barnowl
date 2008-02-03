#include "owl.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

static const char fileIdent[] = "$Id$";

#define INCR 5000

/* initialize the editwin e.
 * 'win' is an already initialzed curses window that will be used by editwin
 */
void owl_editwin_init(owl_editwin *e, WINDOW *win, int winlines, int wincols, int style, owl_history *hist)
{
  e->buff=owl_malloc(INCR);  
  e->buff[0]='\0';
  e->bufflen=0;
  e->hist=hist;
  e->allocated=INCR;
  e->buffx=0;
  e->buffy=0;
  e->topline=0;
  e->winlines=winlines;
  e->wincols=wincols;
  e->fillcol=owl_editwin_limit_maxcols(wincols-7, owl_global_get_edit_maxfillcols(&g));
  e->wrapcol=owl_editwin_limit_maxcols(wincols-7, owl_global_get_edit_maxwrapcols(&g));
  e->curswin=win;
  e->style=style;
  if ((style!=OWL_EDITWIN_STYLE_MULTILINE) &&
      (style!=OWL_EDITWIN_STYLE_ONELINE)) {
    e->style=OWL_EDITWIN_STYLE_MULTILINE;
  }
  e->lock=0;
  e->dotsend=0;
  e->echochar='\0';

  /* We get initialized multiple times, but we need to hold on to
     the callbacks, so we can't NULL them here. */
  /*
    e->command = NULL;
    e->callback = NULL;
    e->cbdata = NULL;
  */
  if (win) werase(win);
}

void owl_editwin_set_curswin(owl_editwin *e, WINDOW *w, int winlines, int wincols)
{
  e->curswin=w;
  e->winlines=winlines;
  e->wincols=wincols;
  e->fillcol=owl_editwin_limit_maxcols(wincols-7, owl_global_get_edit_maxfillcols(&g));
  e->wrapcol=owl_editwin_limit_maxcols(wincols-7, owl_global_get_edit_maxwrapcols(&g));
}

/* echo the character 'ch' for each normal character keystroke,
 * excepting locktext.  This is useful for entering passwords etc.  If
 * ch=='\0' characters are echo'd normally
 */
void owl_editwin_set_echochar(owl_editwin *e, int ch)
{
  e->echochar=ch;
}

WINDOW *owl_editwin_get_curswin(owl_editwin *e)
{
  return(e->curswin);
}

void owl_editwin_set_history(owl_editwin *e, owl_history *h)
{
  e->hist=h;
}

owl_history *owl_editwin_get_history(owl_editwin *e)
{
  return(e->hist);
}

void owl_editwin_set_dotsend(owl_editwin *e)
{
  e->dotsend=1;
}

void owl_editwin_set_command(owl_editwin *e, char *command) {
  if(e->command) owl_free(e->command);
  e->command = owl_strdup(command);
}

char *owl_editwin_get_command(owl_editwin *e) {
  if(e->command) return e->command;
  return "";
}

void owl_editwin_set_callback(owl_editwin *e, void (*cb)(owl_editwin*)) {
  e->callback = cb;
}

void (*owl_editwin_get_callback(owl_editwin *e))(owl_editwin*) {
  return e->callback;
}

void owl_editwin_set_cbdata(owl_editwin *e, void *data) {
  e->cbdata = data;
}

void* owl_editwin_get_cbdata(owl_editwin *e) {
  return e->cbdata;
}

void owl_editwin_do_callback(owl_editwin *e) {
  void (*cb)(owl_editwin*);
  cb=owl_editwin_get_callback(e);
  if(!cb) {
    owl_function_error("Internal error: No editwin callback!");
  } else {
    /* owl_function_error("text: |%s|", owl_editwin_get_text(e)); */
    cb(e);
  }
}

int owl_editwin_limit_maxcols(int v, int maxv)
{
  if (maxv > 5 && v > maxv) {
    return(maxv);
  } else {
    return(v);
  }
}

/* set text to be 'locked in' at the beginning of the buffer, any
 * previous text (including locked text) will be overwritten
 */
void owl_editwin_set_locktext(owl_editwin *e, char *text)
{
  
  int x, y;

  x=e->buffx;
  y=e->buffy;
  e->buffx=0;
  e->buffy=0;
  owl_editwin_overwrite_string(e, text);
  e->lock=strlen(text);
  /* if (text[e->lock-1]=='\n') e->lock--; */
  e->buffx=x;
  e->buffy=y;
  owl_editwin_adjust_for_locktext(e);
  owl_editwin_redisplay(e, 0);
}

int owl_editwin_get_style(owl_editwin *e)
{
  return(e->style);
}

void owl_editwin_new_style(owl_editwin *e, int newstyle, owl_history *h)
{
  char *ptr;

  owl_editwin_set_history(e, h);
  if (e->style==newstyle) return;

  if (newstyle==OWL_EDITWIN_STYLE_MULTILINE) {
    e->style=newstyle;
  } else if (newstyle==OWL_EDITWIN_STYLE_ONELINE) {
    e->style=newstyle;

    /* nuke everything after the first line */
    if (e->bufflen > 0) {
      ptr=strchr(e->buff, '\n')-1;
      if (ptr) {
	e->bufflen=ptr - e->buff;
	e->buff[e->bufflen]='\0';
	e->buffx=0;
	e->buffy=0;
      }
    }
  }
}

/* completly reinitialize the buffer */
void owl_editwin_fullclear(owl_editwin *e)
{
  owl_free(e->buff);
  owl_editwin_init(e, e->curswin, e->winlines, e->wincols, e->style, e->hist);
}

/* clear all text except for locktext and put the cursor at the
 * beginning
 */
void owl_editwin_clear(owl_editwin *e)
{

  int lock;
  int dotsend=e->dotsend;
  char *locktext=NULL;
  char echochar=e->echochar;

  lock=0;
  if (e->lock > 0) {
    lock=1;

    locktext=owl_malloc(e->lock+20);
    strncpy(locktext, e->buff, e->lock);
    locktext[e->lock]='\0';
  }

  owl_free(e->buff);
  owl_editwin_init(e, e->curswin, e->winlines, e->wincols, e->style, e->hist);

  if (lock > 0) {
    owl_editwin_set_locktext(e, locktext);
  }
  if (dotsend) {
    owl_editwin_set_dotsend(e);
  }
  if (echochar) {
    owl_editwin_set_echochar(e, echochar);
  }

  if (locktext) owl_free(locktext);
  owl_editwin_adjust_for_locktext(e);
}

/* malloc more space for the buffer */
void _owl_editwin_addspace(owl_editwin *e)
{
  e->buff=owl_realloc(e->buff, e->allocated+INCR);
  if (!e->buff) {
    /* error */
    return;
  }
  e->allocated+=INCR;
}

void owl_editwin_recenter(owl_editwin *e)
{
  e->topline=e->buffy-(e->winlines/2);
  if (e->topline<0) e->topline=0;
  if (e->topline>owl_editwin_get_numlines(e)) e->topline=owl_editwin_get_numlines(e);
}

/* regenerate the text on the curses window */
/* if update == 1 then do a doupdate(), otherwise do not */
void owl_editwin_redisplay(owl_editwin *e, int update)
{
  
  char *ptr1, *ptr2, *ptr3, *buff;
  int i;

  werase(e->curswin);
  wmove(e->curswin, 0, 0);

  /* start at topline */
  ptr1=e->buff;
  for (i=0; i<e->topline; i++) {
    ptr2=strchr(ptr1, '\n');
    if (!ptr2) {
      /* we're already on the last line */
      break;
    }
    ptr1=ptr2+1;
  }
  /* ptr1 now stores the starting point */

  /* find the ending point and store it in ptr3 */
  ptr2=ptr1;
  ptr3=ptr1;
  for (i=0; i<e->winlines; i++) {
    ptr3=strchr(ptr2, '\n');
    if (!ptr3) {
      /* we've hit the last line */
      /* print everything to the end */
      ptr3=e->buff+e->bufflen-1;
      ptr3--;
      break;
    }
    ptr2=ptr3+1;
  }
  ptr3+=2;

  buff=owl_malloc(ptr3-ptr1+50);
  strncpy(buff, ptr1, ptr3-ptr1);
  buff[ptr3-ptr1]='\0';
  if (e->echochar=='\0') {
    waddstr(e->curswin, buff);
  } else {
    /* translate to echochar, *except* for the locktext */
    int len;
    int dolocklen=e->lock-(ptr1-e->buff);

    for (i=0; i<dolocklen; i++) {
      waddch(e->curswin, buff[i]);
    }
    len=strlen(buff);
    for (i=0; i<len-dolocklen; i++) {
      waddch(e->curswin, e->echochar);
    }
  }
  wmove(e->curswin, e->buffy-e->topline, e->buffx);
  wnoutrefresh(e->curswin);
  if (update==1) {
    doupdate();
  }
  owl_free(buff);
}


/* linewrap the word just before the cursor.
 * returns 0 on success
 * returns -1 if we could not wrap.
 */
int _owl_editwin_linewrap_word(owl_editwin *e)
{
  int i, z;

  z=_owl_editwin_get_index_from_xy(e);
  /* move back and line wrap the previous word */
  for (i=z-1; ; i--) {
    /* move back until you find a space or hit the beginning of the line */
    if (e->buff[i]==' ') {
      /* replace the space with a newline */
      e->buff[i]='\n';
      e->buffy++;
      e->buffx=z-i-1;
      /* were we on the last line */
      return(0);
    } else if (e->buff[i]=='\n' || i<=e->lock) {
      /* we hit the begginning of the line or the buffer, we cannot
       * wrap.
       */
      return(-1);
    }
  }
}

/* insert a character at the current point (shift later
 * characters over)
 */
void owl_editwin_insert_char(owl_editwin *e, char c)
{
  
  int z, i, ret;

  /* \r is \n */
  if (c=='\r') {
    c='\n';
  }

  if (c=='\n' && e->style==OWL_EDITWIN_STYLE_ONELINE) {
    /* perhaps later this will change some state that allows the string
       to be read */
    return;
  }

  /* make sure there is enough memory for the new text */
  if ((e->bufflen+1) > (e->allocated-5)) {
    _owl_editwin_addspace(e);
  }

  /* get the insertion point */
  z=_owl_editwin_get_index_from_xy(e);

  /* If we're going to insert at the last column do word wrapping, unless it's a \n */
  if ((e->buffx+1==e->wrapcol) && (c!='\n')) {
    ret=_owl_editwin_linewrap_word(e);
    if (ret==-1) {
      /* we couldn't wrap, insert a hard newline instead */
      owl_editwin_insert_char(e, '\n');
    }
  }

  z=_owl_editwin_get_index_from_xy(e);
  /* shift all the other characters right */
  for (i=e->bufflen; i>z; i--) {
    e->buff[i]=e->buff[i-1];
  }

  /* insert the new one */
  e->buff[z]=c;

  /* housekeeping */
  e->bufflen++;
  e->buff[e->bufflen]='\0';

  /* advance the cursor */
  if (c=='\n') {
    e->buffx=0;
    e->buffy++;
  } else {
    e->buffx++;
  }
}

/* overwrite the character at the current point with 'c' */
void owl_editwin_overwrite_char(owl_editwin *e, char c)
{
  int z;
  
  /* \r is \n */
  if (c=='\r') {
    c='\n';
  }

  if (c=='\n' && e->style==OWL_EDITWIN_STYLE_ONELINE) {
    /* perhaps later this will change some state that allows the string
       to be read */
    return;
  }

  z=_owl_editwin_get_index_from_xy(e);

  /* only if we are at the end of the buffer do we create new space */
  if (z==e->bufflen) {
    if ((e->bufflen+1) > (e->allocated-5)) {
      _owl_editwin_addspace(e);
    }
  }
  
  e->buff[z]=c;

  /* housekeeping if we are at the end of the buffer */
  if (z==e->bufflen) {
    e->bufflen++;
    e->buff[e->bufflen]='\0';
  }

  /* advance the cursor */
  if (c=='\n') {
    e->buffx=0;
    e->buffy++;
  } else {
    e->buffx++;
  }

}

/* delete the character at the current point, following chars
 * shift left.
 */ 
void owl_editwin_delete_char(owl_editwin *e)
{
  int z, i;

  if (e->bufflen==0) return;
  
  /* get the deletion point */
  z=_owl_editwin_get_index_from_xy(e);

  if (z==e->bufflen) return;

  for (i=z; i<e->bufflen; i++) {
    e->buff[i]=e->buff[i+1];
  }
  e->bufflen--;
  e->buff[e->bufflen]='\0';
}

/* Swap the character at point with the character at point-1 and
 * advance the pointer.  If point is at beginning of buffer do
 * nothing.  If point is after the last character swap point-1 with
 * point-2.  (Behaves as observed in tcsh and emacs).  
 */
void owl_editwin_transpose_chars(owl_editwin *e)
{
  int z;
  char tmp;

  if (e->bufflen==0) return;
  
  /* get the cursor point */
  z=_owl_editwin_get_index_from_xy(e);

  if (z==e->bufflen) {
    /* point is after last character */
    z--;
  }  

  if (z-1 < e->lock) {
    /* point is at beginning of buffer, do nothing */
    return;
  }

  tmp=e->buff[z];
  e->buff[z]=e->buff[z-1];
  e->buff[z-1]=tmp;
  owl_editwin_key_right(e);
}

/* insert 'string' at the current point, later text is shifted
 * right
 */
void owl_editwin_insert_string(owl_editwin *e, char *string)
{
  int i, j;

  j=strlen(string);
  for (i=0; i<j; i++) {
    owl_editwin_insert_char(e, string[i]);
  }
}

/* write 'string' at the current point, overwriting text that is
 * already there
 */

void owl_editwin_overwrite_string(owl_editwin *e, char *string)
{
  int i, j;

  j=strlen(string);
  for (i=0; i<j; i++) {
    owl_editwin_overwrite_char(e, string[i]);
  }
}

/* get the index into e->buff for the current cursor
 * position.
 */
int _owl_editwin_get_index_from_xy(owl_editwin *e)
{
  int i;
  char *ptr1, *ptr2;

  if (e->bufflen==0) return(0);
  
  /* first go to the yth line */
  ptr1=e->buff;
  for (i=0; i<e->buffy; i++) {
    ptr2=strchr(ptr1, '\n');
    if (!ptr2) {
      /* we're already on the last line */
      break;
    }
    ptr1=ptr2+1;
  }

  /* now go to the xth character */
  ptr2=strchr(ptr1, '\n');
  if (!ptr2) {
    ptr2=e->buff+e->bufflen;
  }

  if ((ptr2-ptr1) < e->buffx) {
    ptr1=ptr2-1;
  } else {
    ptr1+=e->buffx;
  }

  /* printf("DEBUG: index is %i\r\n", ptr1-e->buff); */
  return(ptr1-e->buff);
}

void _owl_editwin_set_xy_by_index(owl_editwin *e, int index)
{
  int z, i;

  z=_owl_editwin_get_index_from_xy(e);
  if (index>z) {
    for (i=0; i<index-z; i++) {
      owl_editwin_key_right(e);
    }
  } else if (index<z) {
    for (i=0; i<z-index; i++) {
      owl_editwin_key_left(e);
    }
  }
}

void owl_editwin_adjust_for_locktext(owl_editwin *e)
{
  /* if we happen to have the cursor over locked text
   * move it to be out of the locktext region */
  if (_owl_editwin_get_index_from_xy(e)<e->lock) {
    _owl_editwin_set_xy_by_index(e, e->lock);
  }
}

void owl_editwin_backspace(owl_editwin *e)
{
  /* delete the char before the current one
   * and shift later chars left
   */
  if (_owl_editwin_get_index_from_xy(e) > e->lock) {
    owl_editwin_key_left(e);
    owl_editwin_delete_char(e);
  }
  owl_editwin_adjust_for_locktext(e);
}

void owl_editwin_key_up(owl_editwin *e)
{
  if (e->buffy > 0) e->buffy--;
  if (e->buffx >= owl_editwin_get_numchars_on_line(e, e->buffy)) {
    e->buffx=owl_editwin_get_numchars_on_line(e, e->buffy);
  }

  /* do we need to scroll? */
  if (e->buffy-e->topline < 0) {
    e->topline-=e->winlines/2;
  }

  owl_editwin_adjust_for_locktext(e);
}

void owl_editwin_key_down(owl_editwin *e)
{
  /* move down if we can */
  if (e->buffy+1 < owl_editwin_get_numlines(e)) e->buffy++;

  /* if we're past the last character move back */
  if (e->buffx >= owl_editwin_get_numchars_on_line(e, e->buffy)) {
    e->buffx=owl_editwin_get_numchars_on_line(e, e->buffy);
  }

  /* do we need to scroll? */
  if (e->buffy-e->topline > e->winlines) {
    e->topline+=e->winlines/2;
  }

  /* adjust for locktext */
  owl_editwin_adjust_for_locktext(e);
}

void owl_editwin_key_left(owl_editwin *e)
{
  /* move left if we can, and maybe up a line */
  if (e->buffx>0) {
    e->buffx--;
  } else if (e->buffy>0) {
    e->buffy--;
    e->buffx=owl_editwin_get_numchars_on_line(e, e->buffy);
  }

  /* do we need to scroll up? */
  if (e->buffy-e->topline < 0) {
    e->topline-=e->winlines/2;
  }

  /* make sure to avoid locktext */
  owl_editwin_adjust_for_locktext(e);
}

void owl_editwin_key_right(owl_editwin *e)
{
  int i;

  /* move right if we can, and skip down a line if needed */
  i=owl_editwin_get_numchars_on_line(e, e->buffy);
  if (e->buffx < i) {
    e->buffx++;
    /*  } else if (e->buffy+1 < owl_editwin_get_numlines(e)) { */
  } else if (_owl_editwin_get_index_from_xy(e) < e->bufflen) {
    if (e->style==OWL_EDITWIN_STYLE_MULTILINE) {
      e->buffx=0;
      e->buffy++;
    }
  }

  /* do we need to scroll down? */
  if (e->buffy-e->topline >= e->winlines) {
    e->topline+=e->winlines/2;
  }
}

void owl_editwin_move_to_nextword(owl_editwin *e)
{
  int i, x;

  /* if we're starting on a space, find the first non-space */
  i=_owl_editwin_get_index_from_xy(e);
  if (e->buff[i]==' ') {
    for (x=i; x<e->bufflen; x++) {
      if (e->buff[x]!=' ' && e->buff[x]!='\n') {
	_owl_editwin_set_xy_by_index(e, x);
	break;
      }
    }
  }

  /* find the next space, newline or end of line and go there, if
     already at the end of the line, continue on to the next */
  i=owl_editwin_get_numchars_on_line(e, e->buffy);
  if (e->buffx < i) {
    /* move right till end of line */
    while (e->buffx < i) {
      e->buffx++;
      if (e->buff[_owl_editwin_get_index_from_xy(e)]==' ') return;
      if (e->buffx == i) return;
    }
  } else if (e->buffx == i) {
    /* try to move down */
    if (e->style==OWL_EDITWIN_STYLE_MULTILINE) {
      if (e->buffy+1 <  owl_editwin_get_numlines(e)) {
	e->buffx=0;
	e->buffy++;
	owl_editwin_move_to_nextword(e);
      }
    }
  }
}

/* go backwards to the last non-space character
 */
void owl_editwin_move_to_previousword(owl_editwin *e)
{
  int i, x;

  /* are we already at the beginning of the word? */
  i=_owl_editwin_get_index_from_xy(e);
  if ( (e->buff[i]!=' ' && e->buff[i]!='\n' && e->buff[i]!='\0') &&
       (e->buff[i-1]==' ' || e->buff[i-1]=='\n') ) {
    owl_editwin_key_left(e);
  }
    
  /* are we starting on a space character? */
  i=_owl_editwin_get_index_from_xy(e);
  if (e->buff[i]==' ' || e->buff[i]=='\n' || e->buff[i]=='\0') {
    /* find the first non-space */
    for (x=i; x>=e->lock; x--) {
      if (e->buff[x]!=' ' && e->buff[x]!='\n' && e->buff[x]!='\0') {
	_owl_editwin_set_xy_by_index(e, x);
	break;
      }
    }
  }

  /* find the last non-space */
  i=_owl_editwin_get_index_from_xy(e);
  for (x=i; x>=e->lock; x--) {
    if (e->buff[x-1]==' ' || e->buff[x-1]=='\n') {
      _owl_editwin_set_xy_by_index(e, x);
      break;
    }
  }
  _owl_editwin_set_xy_by_index(e, x);
}


void owl_editwin_delete_nextword(owl_editwin *e)
{
  int z;

  if (e->bufflen==0) return;

  /* if we start out on a space character then gobble all the spaces
     up first */
  while (1) {
    z=_owl_editwin_get_index_from_xy(e);
    if (e->buff[z]==' ' || e->buff[z]=='\n') {
      owl_editwin_delete_char(e);
    } else {
      break;
    }
  }

  /* then nuke the next word */
  while (1) {
    z=_owl_editwin_get_index_from_xy(e);
    /* z == e->bufflen check added to prevent a hang I (nelhage) have
       seen repeatedly while using owl. I'm not sure precisely what
       conditions lead to it. */
    if (z == e->bufflen
        || e->buff[z+1]==' ' || e->buff[z+1]=='\n' || e->buff[z+1]=='\0') break;
    owl_editwin_delete_char(e);
  }
  owl_editwin_delete_char(e);
}

void owl_editwin_delete_previousword(owl_editwin *e)
{
  /* go backwards to the last non-space character, then delete chars */
  int i, startpos, endpos;

  startpos = _owl_editwin_get_index_from_xy(e);
  owl_editwin_move_to_previousword(e);
  endpos = _owl_editwin_get_index_from_xy(e);
  for (i=0; i<startpos-endpos; i++) {
    owl_editwin_delete_char(e);
  }
}

void owl_editwin_delete_to_endofline(owl_editwin *e)
{
  int i;

  if (owl_editwin_get_numchars_on_line(e, e->buffy)>e->buffx) {
    /* normal line */
    i=_owl_editwin_get_index_from_xy(e);
    while(i < e->bufflen) {
      if (e->buff[i]!='\n') {
	owl_editwin_delete_char(e);
      } else if ((e->buff[i]=='\n') && (i==e->bufflen-1)) {
	owl_editwin_delete_char(e);
      } else {
	return;
      }
    }
  } else if (e->buffy+1 < owl_editwin_get_numlines(e)) {
    /* line with cursor at the end but not on very last line */
    owl_editwin_key_right(e);
    owl_editwin_backspace(e);
  }
}

void owl_editwin_move_to_line_end(owl_editwin *e)
{
  e->buffx=owl_editwin_get_numchars_on_line(e, e->buffy);
}

void owl_editwin_move_to_line_start(owl_editwin *e)
{
  e->buffx=0;
  owl_editwin_adjust_for_locktext(e);
}

void owl_editwin_move_to_end(owl_editwin *e)
{
  /* go to last char */
  e->buffy=owl_editwin_get_numlines(e)-1;
  e->buffx=owl_editwin_get_numchars_on_line(e, e->buffy);
  owl_editwin_key_right(e);

  /* do we need to scroll? */
  /*
  if (e->buffy-e->topline > e->winlines) {
    e->topline+=e->winlines/2;
  }
  */
  owl_editwin_recenter(e);
}

void owl_editwin_move_to_top(owl_editwin *e)
{
  _owl_editwin_set_xy_by_index(e, 0);

  /* do we need to scroll? */
  e->topline=0;

  owl_editwin_adjust_for_locktext(e);
}

void owl_editwin_fill_paragraph(owl_editwin *e)
{
  int i, save;

  /* save our starting point */
  save=_owl_editwin_get_index_from_xy(e);

  /* scan back to the beginning of this paragraph */
  for (i=save; i>=e->lock; i--) {
    if ( (i<=e->lock) ||
	 ((e->buff[i]=='\n') && (e->buff[i-1]=='\n'))) {
      _owl_editwin_set_xy_by_index(e, i+1);
      break;
    }
  }

  /* main loop */
  while (1) {
    i=_owl_editwin_get_index_from_xy(e);

    /* bail if we hit the end of the buffer */
    if (i>=e->bufflen) break;

    /* bail if we hit the end of the paragraph */
    if (e->buff[i]=='\n' && e->buff[i+1]=='\n') break;

    /* if we've travelled too far, linewrap */
    if ((e->buffx) >= e->fillcol) {
      _owl_editwin_linewrap_word(e);
    }

    /* did we hit the end of a line too soon? */
    i=_owl_editwin_get_index_from_xy(e);
    if (e->buff[i]=='\n' && e->buffx<e->fillcol-1) {
      /* ********* we need to make sure we don't pull in a word that's too long ***********/
      e->buff[i]=' ';
    }
    
    /* fix spacing */
    i=_owl_editwin_get_index_from_xy(e);
    if (e->buff[i]==' ' && e->buff[i+1]==' ') {
      if (e->buff[i-1]=='.' || e->buff[i-1]=='!' || e->buff[i-1]=='?') {
	owl_editwin_key_right(e);
      } else {
	owl_editwin_delete_char(e);
	/* if we did this ahead of the save point, adjust it */
	if (i<save) save--;
      }
    } else {
      owl_editwin_key_right(e);
    }

  }

  /* put cursor back at starting point */
  _owl_editwin_set_xy_by_index(e, save);

  /* do we need to scroll? */
  if (e->buffy-e->topline < 0) {
    e->topline-=e->winlines/2;
  }
}

/* returns true if only whitespace remains */
int owl_editwin_is_at_end(owl_editwin *e)
{
  int cur=_owl_editwin_get_index_from_xy(e);
  return (only_whitespace(e->buff+cur));
}

int owl_editwin_check_dotsend(owl_editwin *e)
{
  int i;

  if (!e->dotsend) return(0);
  for (i=e->bufflen-1; i>0; i--) {
    if (e->buff[i] == '.' 
	&& (e->buff[i-1] == '\n' || e->buff[i-1] == '\r')
	&& (e->buff[i+1] == '\n' || e->buff[i+1] == '\r')) {
      e->bufflen = i;
      e->buff[i] = '\0';
      return(1);
    }
    if (!isspace((int) e->buff[i])) {
      return(0);
    }
  }
  return(0);
}

void owl_editwin_post_process_char(owl_editwin *e, int j)
{
  /* check if we need to scroll down */
  if (e->buffy-e->topline >= e->winlines) {
    e->topline+=e->winlines/2;
  }
  if ((j==13 || j==10) && owl_editwin_check_dotsend(e)) {
    owl_command_editmulti_done(e);
    return;
  }
  owl_editwin_redisplay(e, 0);  
}

void owl_editwin_process_char(owl_editwin *e, int j)
{
  if (j == ERR) return;
  if (j>127 || ((j<32) && (j!=10) && (j!=13))) {
    return;
  } else {
    owl_editwin_insert_char(e, j);
  }
}

char *owl_editwin_get_text(owl_editwin *e)
{
  return(e->buff+e->lock);
}

int owl_editwin_get_numchars_on_line(owl_editwin *e, int line)
{
  int i;
  char *ptr1, *ptr2;

  if (e->bufflen==0) return(0);
  
  /* first go to the yth line */
  ptr1=e->buff;
  for (i=0; i<line; i++) {
    ptr2=strchr(ptr1, '\n');
    if (!ptr2) {
      /* we're already on the last line */
      return(0);
    }
    ptr1=ptr2+1;
  }

  /* now go to the xth character */
  ptr2=strchr(ptr1, '\n');
  if (!ptr2) {
    return(e->buff + e->bufflen - ptr1);
  }
  return(ptr2-ptr1); /* don't count the newline for now */
}

int owl_editwin_get_numlines(owl_editwin *e)
{
  return(owl_text_num_lines(e->buff));
}

