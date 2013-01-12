#include "owl.h"

static void owl_msgwin_redraw(owl_window *w, WINDOW *curswin, void *msgwin_);

/* Maintains a small status window of text */

void owl_msgwin_init(owl_msgwin *msgwin, owl_window *window)
{
  msgwin->msg = NULL;
  msgwin->window = g_object_ref(window);
  msgwin->redraw_id = g_signal_connect(window, "redraw", G_CALLBACK(owl_msgwin_redraw), msgwin);
  owl_window_dirty(window);
}

static void owl_msgwin_redraw(owl_window *w, WINDOW *curswin, void *msgwin_)
{
  owl_msgwin *msgwin = msgwin_;

  werase(curswin);
  if (msgwin->msg)
    waddstr(curswin, msgwin->msg);
}

void owl_msgwin_set_text(owl_msgwin *msgwin, const char *msg)
{
  owl_msgwin_set_text_nocopy(msgwin, g_strdup(msg));
}

void owl_msgwin_set_text_nocopy(owl_msgwin *msgwin, char *msg)
{
  g_free(msgwin->msg);
  msgwin->msg = msg;
  owl_window_dirty(msgwin->window);
}

void owl_msgwin_cleanup(owl_msgwin *msgwin)
{
  g_free(msgwin->msg);
  msgwin->msg = NULL;
  if (msgwin->window) {
    g_signal_handler_disconnect(msgwin->window, msgwin->redraw_id);
    g_object_unref(msgwin->window);
    msgwin->window = NULL;
    msgwin->redraw_id = 0;
  }
}
