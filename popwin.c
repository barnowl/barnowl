#include "owl.h"

CALLER_OWN owl_popwin *owl_popwin_new(void)
{
  owl_popwin *pw = g_slice_new0(owl_popwin);

  pw->border = owl_window_new(NULL);
  pw->content = owl_window_new(pw->border);
  /* To be thorough, we ensure each signal is disconnected when we close. */
  pw->sig_redraw_id =
    g_signal_connect(pw->border, "redraw", G_CALLBACK(owl_popwin_draw_border), 0);
  pw->sig_resize_id =
    g_signal_connect(pw->border, "resized", G_CALLBACK(owl_popwin_size_content), pw);
  owl_signal_connect_object(owl_window_get_screen(), "resized", G_CALLBACK(owl_popwin_size_border), pw->border, 0);

  /* bootstrap sizing */
  owl_popwin_size_border(owl_window_get_screen(), pw->border);

  owl_window_show(pw->content);

  return pw;
}

int owl_popwin_up(owl_popwin *pw)
{
  if (owl_window_is_shown(pw->border))
    return 1;
  owl_window_show(pw->border);
  return 0;
}

void owl_popwin_size_border(owl_window *parent, void *user_data)
{
  int lines, cols, startline, startcol;
  int glines, gcols;
  owl_window *border = user_data;

  owl_window_get_position(parent, &glines, &gcols, 0, 0);

  lines = MIN(glines, 24) * 3/4 + MAX(glines - 24, 0) / 2;
  startline = (glines-lines)/2;
  cols = MIN(gcols, 90) * 15 / 16 + MAX(gcols - 90, 0) / 2;
  startcol = (gcols-cols)/2;

  owl_window_set_position(border, lines, cols, startline, startcol);
}

void owl_popwin_size_content(owl_window *parent, void *user_data)
{
  int lines, cols;
  owl_popwin *pw = user_data;
  owl_window_get_position(parent, &lines, &cols, 0, 0);
  owl_window_set_position(pw->content, lines-2, cols-2, 1, 1);
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
  if (!owl_window_is_shown(pw->border))
    return 1;
  owl_window_hide(pw->border);
  return 0;
}

void owl_popwin_delete(owl_popwin *pw)
{
  owl_popwin_close(pw);

  /* Remove everything that references us. */
  g_signal_handler_disconnect(pw->border, pw->sig_resize_id);
  g_signal_handler_disconnect(pw->border, pw->sig_redraw_id);
  owl_window_unlink(pw->border);
  g_object_unref(pw->border);
  g_object_unref(pw->content);

  g_slice_free(owl_popwin, pw);
}

int owl_popwin_is_active(const owl_popwin *pw)
{
  return owl_window_is_shown(pw->border);
}

owl_window *owl_popwin_get_content(const owl_popwin *pw)
{
  return pw->content;
}
