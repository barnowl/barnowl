#include "owl.h"

static void sepbar_redraw(owl_window *w, WINDOW *sepwin, void *user_data);

void owl_sepbar_init(owl_window *w)
{
  g_signal_connect(w, "redraw", G_CALLBACK(sepbar_redraw), NULL);
  /* TODO: handle dirtiness in the sepbar */
  owl_window_dirty(w);
}

static void sepbar_redraw(owl_window *w, WINDOW *sepwin, void *user_data)
{
  const owl_messagelist *ml;
  const owl_view *v;
  int x, y, i;
  const char *foo, *appendtosepbar;
  int cur_numlines, cur_totallines;

  ml=owl_global_get_msglist(&g);
  v=owl_global_get_current_view(&g);

  werase(sepwin);
  wattron(sepwin, A_REVERSE);
  if (owl_global_is_fancylines(&g)) {
    whline(sepwin, ACS_HLINE, owl_global_get_cols(&g));
  } else {
    whline(sepwin, '-', owl_global_get_cols(&g));
  }

  if (owl_global_is_sepbar_disable(&g)) {
    getyx(sepwin, y, x);
    wmove(sepwin, y, owl_global_get_cols(&g)-1);
    return;
  }

  wmove(sepwin, 0, 2);

  if (owl_messagelist_get_size(ml) == 0)
    waddstr(sepwin, " (-/-) ");
  else
    wprintw(sepwin, " (%i/%i/%i) ", owl_global_get_curmsg(&g) + 1,
            owl_view_get_size(v),
            owl_messagelist_get_size(ml));

  foo=owl_view_get_filtname(v);
  if (strcmp(foo, owl_global_get_view_home(&g)))
      wattroff(sepwin, A_REVERSE);
  wprintw(sepwin, " %s ", owl_view_get_filtname(v));
  if (strcmp(foo, owl_global_get_view_home(&g)))
      wattron(sepwin, A_REVERSE);

  i=owl_mainwin_get_last_msg(owl_global_get_mainwin(&g));
  if ((i != -1) &&
      (i < owl_view_get_size(v)-1)) {
    getyx(sepwin, y, x);
    wmove(sepwin, y, x+2);
    wattron(sepwin, A_BOLD);
    waddstr(sepwin, " <more> ");
    wattroff(sepwin, A_BOLD);
  }

  if (owl_global_get_rightshift(&g)>0) {
    getyx(sepwin, y, x);
    wmove(sepwin, y, x+2);
    wprintw(sepwin, " right: %i ", owl_global_get_rightshift(&g));
  }

  if (owl_function_is_away()) {
    getyx(sepwin, y, x);
    wmove(sepwin, y, x+2);
    wattron(sepwin, A_BOLD);
    wattroff(sepwin, A_REVERSE);
    waddstr(sepwin, " AWAY ");
    wattron(sepwin, A_REVERSE);
    wattroff(sepwin, A_BOLD);
  }

  if (owl_mainwin_is_curmsg_truncated(owl_global_get_mainwin(&g))) {
    getyx(sepwin, y, x);
    wmove(sepwin, y, x+2);
    wattron(sepwin, A_BOLD);
    cur_numlines = owl_global_get_curmsg_vert_offset(&g) + 1;
    cur_totallines = owl_message_get_numlines(owl_view_get_element(v, owl_global_get_curmsg(&g)));
    wprintw(sepwin, " <truncated: %d/%d> ", cur_numlines, cur_totallines);
    wattroff(sepwin, A_BOLD);
  }

  if (owl_global_get_curmsg_vert_offset(&g)) {
    getyx(sepwin, y, x);
    wmove(sepwin, y, x+2);
    wattron(sepwin, A_BOLD);
    wattroff(sepwin, A_REVERSE);
    waddstr(sepwin, " SCROLL ");
    wattron(sepwin, A_REVERSE);
    wattroff(sepwin, A_BOLD);
  }

  appendtosepbar = owl_global_get_appendtosepbar(&g);
  if (appendtosepbar && *appendtosepbar) {
    getyx(sepwin, y, x);
    wmove(sepwin, y, x+2);
    waddstr(sepwin, " ");
    waddstr(sepwin, owl_global_get_appendtosepbar(&g));
    waddstr(sepwin, " ");
  }

  getyx(sepwin, y, x);
  wmove(sepwin, y, owl_global_get_cols(&g)-1);
    
  wattroff(sepwin, A_BOLD);
  wattroff(sepwin, A_REVERSE);
}
