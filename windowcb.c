#include "owl.h"

void owl_window_erase_cb(owl_window *w, WINDOW *win, void *user_data)
{
  werase(win);
  owl_window_dirty_children(w);
}

void owl_window_fill_parent_cb(owl_window *parent, void *user_data)
{
  int lines, cols;
  owl_window *window = user_data;

  /* Make this panel full-screen */
  owl_window_get_position(parent, &lines, &cols, NULL, NULL);
  owl_window_set_position(window, lines, cols, 0, 0);
}
