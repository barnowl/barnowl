#include "owl.h"

static const char fileIdent[] = "$Id$";

int owl_popwin_init(owl_popwin *pw)
{
  pw->active=0;
  pw->needsfirstrefresh=0;
  pw->lines=0;
  pw->cols=0;
  return(0);
}

int owl_popwin_up(owl_popwin *pw)
{
  int glines, gcols, startcol, startline;

  /* calculate the size of the popwin */
  glines=owl_global_get_lines(&g);
  gcols=owl_global_get_cols(&g);
  if (glines > 24) {
    pw->lines=glines/2;
    startline=glines/4;
  } else {
    pw->lines=(glines*3)/4;
    startline=glines/8;
  }

  pw->cols=(gcols*15)/16;
  startcol=gcols/32;
  if (pw->cols > 100) {
    pw->cols=100;
    startcol=(gcols-100)/2;
  }

  pw->borderwin=newwin(pw->lines, pw->cols, startline, startcol);
  pw->popwin=newwin(pw->lines-2, pw->cols-2, startline+1, startcol+1);
  pw->needsfirstrefresh=1;
  
  meta(pw->popwin,TRUE);
  nodelay(pw->popwin, 1);
  keypad(pw->popwin, TRUE);

  werase(pw->popwin);
  werase(pw->borderwin);
  if (owl_global_is_fancylines(&g)) {
    box(pw->borderwin, 0, 0);
  } else {
    box(pw->borderwin, '|', '-');
  }
    
  wnoutrefresh(pw->popwin);
  wnoutrefresh(pw->borderwin);
  owl_global_set_needrefresh(&g);
  pw->active=1;
  return(0);
}

int owl_popwin_display_text(owl_popwin *pw, char *text)
{
  waddstr(pw->popwin, text);
  wnoutrefresh(pw->popwin);
  touchwin(pw->borderwin);
  wnoutrefresh(pw->borderwin);
  owl_global_set_needrefresh(&g);
  return(0);
}	      

int owl_popwin_close(owl_popwin *pw)
{
  delwin(pw->popwin);
  delwin(pw->borderwin);
  pw->active=0;
  owl_global_set_needrefresh(&g);
  owl_mainwin_redisplay(owl_global_get_mainwin(&g));
  owl_function_full_redisplay(&g);
  return(0);
}

int owl_popwin_is_active(owl_popwin *pw)
{
  if (pw->active==1) return(1);
  return(0);
}

/* this will refresh the border as well as the text area */
int owl_popwin_refresh(owl_popwin *pw)
{
  touchwin(pw->borderwin);
  touchwin(pw->popwin);

  wnoutrefresh(pw->borderwin);
  wnoutrefresh(pw->popwin);
  owl_global_set_needrefresh(&g);
  return(0);
}

void owl_popwin_set_handler(owl_popwin *pw, void (*func)(int ch))
{
  pw->handler=func;
}

void owl_popwin_unset_handler(owl_popwin *pw)
{
  pw->handler=NULL;
}

WINDOW *owl_popwin_get_curswin(owl_popwin *pw)
{
  return(pw->popwin);
}

int owl_popwin_get_lines(owl_popwin *pw)
{
  return(pw->lines-2);
}

int owl_popwin_get_cols(owl_popwin *pw)
{
  return(pw->cols-2);
}

int owl_popwin_needs_first_refresh(owl_popwin *pw)
{
  if (pw->needsfirstrefresh) return(1);
  return(0);
}

void owl_popwin_no_needs_first_refresh(owl_popwin *pw)
{
  pw->needsfirstrefresh=0;
}
