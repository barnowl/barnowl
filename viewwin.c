#include <string.h>
#include "owl.h"

#define BOTTOM_OFFSET 1

static void owl_viewwin_redraw(owl_window *w, WINDOW *curswin, void *user_data);
static void owl_viewwin_set_window(owl_viewwin *v, owl_window *w);

/* Create a viewwin.  'win' is an already initialized owl_window that
 * will be used by the viewwin
 */
owl_viewwin *owl_viewwin_new_text(owl_window *win, const char *text)
{
  owl_viewwin *v = owl_malloc(sizeof(owl_viewwin));
  memset(v, 0, sizeof(*v));
  owl_fmtext_init_null(&(v->fmtext));
  if (text) {
    owl_fmtext_append_normal(&(v->fmtext), text);
    if (text[0] != '\0' && text[strlen(text) - 1] != '\n') {
      owl_fmtext_append_normal(&(v->fmtext), "\n");
    }
    v->textlines=owl_fmtext_num_lines(&(v->fmtext));
  }
  v->topline=0;
  v->rightshift=0;
  v->onclose_hook = NULL;

  owl_viewwin_set_window(v, win);
  return v;
}

void owl_viewwin_append_text(owl_viewwin *v, const char *text) {
    owl_fmtext_append_normal(&(v->fmtext), text);
    v->textlines=owl_fmtext_num_lines(&(v->fmtext));
    owl_viewwin_dirty(v);
}

/* Schedule a redraw of 'v'. Exported for hooking into the search
   string; when we have some way of listening for changes, this can be
   removed. */
void owl_viewwin_dirty(owl_viewwin *v)
{
  if (v->window)
    owl_window_dirty(v->window);
}

/* Create a viewwin.  'win' is an already initialized owl_window that
 * will be used by the viewwin
 */
owl_viewwin *owl_viewwin_new_fmtext(owl_window *win, const owl_fmtext *fmtext)
{
  char *text;
  owl_viewwin *v = owl_malloc(sizeof(owl_viewwin));
  memset(v, 0, sizeof(*v));

  owl_fmtext_copy(&(v->fmtext), fmtext);
  text = owl_fmtext_print_plain(fmtext);
  if (text[0] != '\0' && text[strlen(text) - 1] != '\n') {
      owl_fmtext_append_normal(&(v->fmtext), "\n");
  }
  owl_free(text);
  v->textlines=owl_fmtext_num_lines(&(v->fmtext));
  v->topline=0;
  v->rightshift=0;

  owl_viewwin_set_window(v, win);
  return v;
}

static void owl_viewwin_set_window(owl_viewwin *v, owl_window *w)
{
  v->window = w;
  if (w) {
    g_object_ref(v->window);
    v->sig_redraw_id = g_signal_connect(w, "redraw", G_CALLBACK(owl_viewwin_redraw), v);
  }
}

void owl_viewwin_set_onclose_hook(owl_viewwin *v, void (*onclose_hook) (owl_viewwin *vwin, void *data), void *onclose_hook_data) {
  v->onclose_hook = onclose_hook;
  v->onclose_hook_data = onclose_hook_data;
}

/* regenerate text on the curses window. */
static void owl_viewwin_redraw(owl_window *w, WINDOW *curswin, void *user_data)
{
  owl_fmtext fm1, fm2;
  owl_viewwin *v = user_data;
  int winlines, wincols;

  owl_window_get_position(w, &winlines, &wincols, 0, 0);
  
  werase(curswin);
  wmove(curswin, 0, 0);

  owl_fmtext_init_null(&fm1);
  owl_fmtext_init_null(&fm2);
  
  owl_fmtext_truncate_lines(&(v->fmtext), v->topline, winlines-BOTTOM_OFFSET, &fm1);
  owl_fmtext_truncate_cols(&fm1, v->rightshift, wincols-1+v->rightshift, &fm2);

  owl_fmtext_curs_waddstr(&fm2, curswin);

  /* print the message at the bottom */
  wmove(curswin, winlines-1, 0);
  wattrset(curswin, A_REVERSE);
  if (v->textlines - v->topline > winlines-BOTTOM_OFFSET) {
    waddstr(curswin, "--More-- (Space to see more, 'q' to quit)");
  } else {
    waddstr(curswin, "--End-- (Press 'q' to quit)");
  }
  wattroff(curswin, A_REVERSE);

  owl_fmtext_cleanup(&fm1);
  owl_fmtext_cleanup(&fm2);
}

void owl_viewwin_pagedown(owl_viewwin *v)
{
  int winlines;
  owl_window_get_position(v->window, &winlines, 0, 0, 0);
  v->topline+=winlines - BOTTOM_OFFSET;
  if ( (v->topline+winlines-BOTTOM_OFFSET) > v->textlines) {
    v->topline = v->textlines - winlines + BOTTOM_OFFSET;
  }
  owl_viewwin_dirty(v);
}

void owl_viewwin_linedown(owl_viewwin *v)
{
  int winlines;
  owl_window_get_position(v->window, &winlines, 0, 0, 0);
  v->topline++;
  if ( (v->topline+winlines-BOTTOM_OFFSET) > v->textlines) {
    v->topline = v->textlines - winlines + BOTTOM_OFFSET;
  }
  owl_viewwin_dirty(v);
}

void owl_viewwin_pageup(owl_viewwin *v)
{
  int winlines;
  owl_window_get_position(v->window, &winlines, 0, 0, 0);
  v->topline-=winlines;
  if (v->topline<0) v->topline=0;
  owl_viewwin_dirty(v);

}

void owl_viewwin_lineup(owl_viewwin *v)
{
  v->topline--;
  if (v->topline<0) v->topline=0;
  owl_viewwin_dirty(v);
}

void owl_viewwin_right(owl_viewwin *v, int n)
{
  v->rightshift+=n;
  owl_viewwin_dirty(v);
}

void owl_viewwin_left(owl_viewwin *v, int n)
{
  v->rightshift-=n;
  if (v->rightshift<0) v->rightshift=0;
  owl_viewwin_dirty(v);
}

void owl_viewwin_top(owl_viewwin *v)
{
  v->topline=0;
  v->rightshift=0;
  owl_viewwin_dirty(v);
}

void owl_viewwin_bottom(owl_viewwin *v)
{
  int winlines;
  owl_window_get_position(v->window, &winlines, 0, 0, 0);
  v->topline = v->textlines - winlines + BOTTOM_OFFSET;
  owl_viewwin_dirty(v);
}

void owl_viewwin_delete(owl_viewwin *v)
{
  if (v->onclose_hook) {
    v->onclose_hook(v, v->onclose_hook_data);
    v->onclose_hook = NULL;
    v->onclose_hook_data = NULL;
  }
  if (v->window) {
    g_signal_handler_disconnect(v->window, v->sig_redraw_id);
    g_object_unref(v->window);
    v->window = NULL;
  }
  owl_fmtext_cleanup(&(v->fmtext));
  owl_free(v);
}
