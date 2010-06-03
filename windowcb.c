#include "owl.h"

void owl_window_erase_cb(owl_window *w, WINDOW *win, void *user_data)
{
  werase(win);
  owl_window_dirty_children(w);
}
