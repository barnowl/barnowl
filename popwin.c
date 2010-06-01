#include "owl.h"

int owl_popwin_init(owl_popwin *pw)
{
  pw->active=0;
  return(0);
}

int owl_popwin_up(owl_popwin *pw)
{
  pw->border = owl_window_new(NULL);
  pw->content = owl_window_new(pw->border);
  owl_window_set_redraw_cb(pw->border, owl_popwin_draw_border, pw, 0);
  owl_window_set_size_cb(pw->border, owl_popwin_size_border, 0, 0);
  owl_window_set_size_cb(pw->content, owl_popwin_size_content, 0, 0);
  owl_window_show(pw->border, 1);

  pw->active=1;
  return(0);
}

void owl_popwin_size_border(owl_window *border, void *user_data)
{
  int lines, cols, startline, startcol;
  int glines, gcols;
  owl_window *parent = owl_window_get_parent(border);

  owl_window_get_position(parent, &glines, &gcols, 0, 0);

  lines = owl_util_min(glines,24)*3/4 + owl_util_max(glines-24,0)/2;
  startline = (glines-lines)/2;
  cols = owl_util_min(gcols,90)*15/16 + owl_util_max(gcols-90,0)/2;
  startcol = (gcols-cols)/2;

  owl_window_set_position(border, lines, cols, startline, startcol);
}

void owl_popwin_size_content(owl_window *content, void *user_data)
{
  int lines, cols;
  owl_window *parent = owl_window_get_parent(content);
  owl_window_get_position(parent, &lines, &cols, 0, 0);
  owl_window_set_position(content, lines-2, cols-2, 1, 1);
}

void owl_popwin_draw_border(owl_window *w, WINDOW *borderwin, void *user_data)
{
  int lines, cols;
  owl_window_get_position(w, &lines, &cols, 0, 0);
  if (owl_global_is_fancylines(&g)) {
    box(borderwin, 0, 0);
  } else {
    box(borderwin, '|', '-');
    wmove(borderwin, 0, 0);
    waddch(borderwin, '+');
    wmove(borderwin, lines-1, 0);
    waddch(borderwin, '+');
    wmove(borderwin, lines-1, cols-1);
    waddch(borderwin, '+');
    wmove(borderwin, 0, cols-1);
    waddch(borderwin, '+');
  }
}

int owl_popwin_close(owl_popwin *pw)
{
  owl_window_delete(pw->border);
  pw->border = 0;
  pw->content = 0;
  pw->active=0;
  return(0);
}

int owl_popwin_is_active(const owl_popwin *pw)
{
  return pw->active;
}

owl_window *owl_popwin_get_content(const owl_popwin *pw)
{
  return pw->content;
}
