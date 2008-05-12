#include <string.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";

#define BOTTOM_OFFSET 1

/* initialize the viewwin e.  'win' is an already initialzed curses
 * window that will be used by viewwin
 */
void owl_viewwin_init_text(owl_viewwin *v, WINDOW *win, int winlines, int wincols, char *text)
{
  owl_fmtext_init_null(&(v->fmtext));
  if (text) {
    owl_fmtext_append_normal(&(v->fmtext), text);
    if (text[strlen(text)-1]!='\n' && text[0]!='\0') {
      owl_fmtext_append_normal(&(v->fmtext), "\n");
    }
    v->textlines=owl_fmtext_num_lines(&(v->fmtext));
  }
  v->topline=0;
  v->rightshift=0;
  v->winlines=winlines;
  v->wincols=wincols;
  v->curswin=win;
  v->onclose_hook = NULL;
}

void owl_viewwin_append_text(owl_viewwin *v, char *text) {
    owl_fmtext_append_normal(&(v->fmtext), text);
    v->textlines=owl_fmtext_num_lines(&(v->fmtext));  
}

/* initialize the viewwin e.  'win' is an already initialzed curses
 * window that will be used by viewwin
 */
void owl_viewwin_init_fmtext(owl_viewwin *v, WINDOW *win, int winlines, int wincols, owl_fmtext *fmtext)
{
  owl_fmtext_copy(&(v->fmtext), fmtext);
  v->textlines=owl_fmtext_num_lines(&(v->fmtext));
  v->topline=0;
  v->rightshift=0;
  v->winlines=winlines;
  v->wincols=wincols;
  v->curswin=win;
}

void owl_viewwin_set_curswin(owl_viewwin *v, WINDOW *w, int winlines, int wincols)
{
  v->curswin=w;
  v->winlines=winlines;
  v->wincols=wincols;
}

void owl_viewwin_set_onclose_hook(owl_viewwin *v, void (*onclose_hook) (owl_viewwin *vwin, void *data), void *onclose_hook_data) {
  v->onclose_hook = onclose_hook;
  v->onclose_hook_data = onclose_hook_data;
}

/* regenerate text on the curses window. */
/* if update == 1 then do a doupdate() */
void owl_viewwin_redisplay(owl_viewwin *v, int update)
{
  owl_fmtext fm1, fm2;
  
  werase(v->curswin);
  wmove(v->curswin, 0, 0);

  owl_fmtext_init_null(&fm1);
  owl_fmtext_init_null(&fm2);
  
  owl_fmtext_truncate_lines(&(v->fmtext), v->topline, v->winlines-BOTTOM_OFFSET, &fm1);
  owl_fmtext_truncate_cols(&fm1, v->rightshift, v->wincols-1+v->rightshift, &fm2);

  owl_fmtext_curs_waddstr_without_search(&fm2, v->curswin);

  /* print the message at the bottom */
  wmove(v->curswin, v->winlines-1, 0);
  wattrset(v->curswin, A_REVERSE);
  if (v->textlines - v->topline > v->winlines-BOTTOM_OFFSET) {
    waddstr(v->curswin, "--More-- (Space to see more, 'q' to quit)");
  } else {
    waddstr(v->curswin, "--End-- (Press 'q' to quit)");
  }
  wattroff(v->curswin, A_REVERSE);
  wnoutrefresh(v->curswin);

  if (update==1) {
    doupdate();
  }

  owl_fmtext_free(&fm1);
  owl_fmtext_free(&fm2);
}

void owl_viewwin_pagedown(owl_viewwin *v)
{
  v->topline+=v->winlines - BOTTOM_OFFSET;
  if ( (v->topline+v->winlines-BOTTOM_OFFSET) > v->textlines) {
    v->topline = v->textlines - v->winlines + BOTTOM_OFFSET;
  }
}

void owl_viewwin_linedown(owl_viewwin *v)
{
  v->topline++;
  if ( (v->topline+v->winlines-BOTTOM_OFFSET) > v->textlines) {
    v->topline = v->textlines - v->winlines + BOTTOM_OFFSET;
  }
}

void owl_viewwin_pageup(owl_viewwin *v)
{
  v->topline-=v->winlines;
  if (v->topline<0) v->topline=0;
}

void owl_viewwin_lineup(owl_viewwin *v)
{
  v->topline--;
  if (v->topline<0) v->topline=0;
}

void owl_viewwin_right(owl_viewwin *v, int n)
{
  v->rightshift+=n;
}

void owl_viewwin_left(owl_viewwin *v, int n)
{
  v->rightshift-=n;
  if (v->rightshift<0) v->rightshift=0;
}

void owl_viewwin_top(owl_viewwin *v)
{
  v->topline=0;
  v->rightshift=0;
}

void owl_viewwin_bottom(owl_viewwin *v)
{
  v->topline = v->textlines - v->winlines + BOTTOM_OFFSET;
}

void owl_viewwin_free(owl_viewwin *v)
{
  if (v->onclose_hook) {
    v->onclose_hook(v, v->onclose_hook_data);
    v->onclose_hook = NULL;
    v->onclose_hook_data = NULL;
  }
  owl_fmtext_free(&(v->fmtext));
}
