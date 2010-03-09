#include "owl.h"

int owl_popwin_init(owl_popwin *pw)
{
  pw->active=0;
  pw->lines=0;
  pw->cols=0;
  return(0);
}

int owl_popwin_up(owl_popwin *pw)
{
  int glines, gcols, startcol, startline;
  WINDOW *popwin, *borderwin;

  /* calculate the size of the popwin */
  glines=owl_global_get_lines(&g);
  gcols=owl_global_get_cols(&g);

  pw->lines = owl_util_min(glines,24)*3/4 + owl_util_max(glines-24,0)/2;
  startline = (glines-pw->lines)/2;

  pw->cols = owl_util_min(gcols,90)*15/16 + owl_util_max(gcols-90,0)/2;
  startcol = (gcols-pw->cols)/2;

  borderwin = newwin(pw->lines, pw->cols, startline, startcol);
  pw->borderpanel = new_panel(borderwin);
  popwin = newwin(pw->lines-2, pw->cols-2, startline+1, startcol+1);
  pw->poppanel = new_panel(popwin);
  
  meta(popwin,TRUE);
  nodelay(popwin, 1);
  keypad(popwin, TRUE);

  werase(popwin);
  werase(borderwin);
  if (owl_global_is_fancylines(&g)) {
    box(borderwin, 0, 0);
  } else {
    box(borderwin, '|', '-');
    wmove(borderwin, 0, 0);
    waddch(borderwin, '+');
    wmove(borderwin, pw->lines-1, 0);
    waddch(borderwin, '+');
    wmove(borderwin, pw->lines-1, pw->cols-1);
    waddch(borderwin, '+');
    wmove(borderwin, 0, pw->cols-1);
    waddch(borderwin, '+');
  }
    
  owl_global_set_needrefresh(&g);
  pw->active=1;
  return(0);
}

int owl_popwin_close(owl_popwin *pw)
{
  WINDOW *popwin, *borderwin;

  popwin = panel_window(pw->poppanel);
  borderwin = panel_window(pw->borderpanel);

  del_panel(pw->poppanel);
  del_panel(pw->borderpanel);
  delwin(popwin);
  delwin(borderwin);

  pw->active=0;
  owl_global_set_needrefresh(&g);
  owl_mainwin_redisplay(owl_global_get_mainwin(&g));
  owl_function_full_redisplay();
  return(0);
}

int owl_popwin_is_active(const owl_popwin *pw)
{
  if (pw->active==1) return(1);
  return(0);
}

WINDOW *owl_popwin_get_curswin(const owl_popwin *pw)
{
  return panel_window(pw->poppanel);
}

int owl_popwin_get_lines(const owl_popwin *pw)
{
  return(pw->lines-2);
}

int owl_popwin_get_cols(const owl_popwin *pw)
{
  return(pw->cols-2);
}
