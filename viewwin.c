#include <string.h>
#include "owl.h"

#define BOTTOM_OFFSET 1

static void owl_viewwin_redraw_content(owl_window *w, WINDOW *curswin, void *user_data);
static void owl_viewwin_redraw_status(owl_window *w, WINDOW *curswin, void *user_data);
static void owl_viewwin_layout(owl_viewwin *v);
static void owl_viewwin_set_window(owl_viewwin *v, owl_window *w);

/* Create a viewwin.  'win' is an already initialized owl_window that
 * will be used by the viewwin
 */
owl_viewwin *owl_viewwin_new_text(owl_window *win, const char *text)
{
  owl_viewwin *v = g_new0(owl_viewwin, 1);
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

/* Create a viewwin.  'win' is an already initialized owl_window that
 * will be used by the viewwin
 */
owl_viewwin *owl_viewwin_new_fmtext(owl_window *win, const owl_fmtext *fmtext)
{
  char *text;
  owl_viewwin *v = g_new0(owl_viewwin, 1);

  owl_fmtext_copy(&(v->fmtext), fmtext);
  text = owl_fmtext_print_plain(fmtext);
  if (text[0] != '\0' && text[strlen(text) - 1] != '\n') {
      owl_fmtext_append_normal(&(v->fmtext), "\n");
  }
  g_free(text);
  v->textlines=owl_fmtext_num_lines(&(v->fmtext));
  v->topline=0;
  v->rightshift=0;

  owl_viewwin_set_window(v, win);
  return v;
}

static void owl_viewwin_set_window(owl_viewwin *v, owl_window *w)
{
  v->window = g_object_ref(w);
  v->content = owl_window_new(v->window);
  v->status = owl_window_new(v->window);
  v->cmdwin = NULL;

  v->sig_content_redraw_id =
    g_signal_connect(v->content, "redraw", G_CALLBACK(owl_viewwin_redraw_content), v);
  v->sig_status_redraw_id =
    g_signal_connect(v->status, "redraw", G_CALLBACK(owl_viewwin_redraw_status), v);
  v->sig_resize_id =
    g_signal_connect_swapped(v->window, "resized", G_CALLBACK(owl_viewwin_layout), v);
  owl_viewwin_layout(v);

  owl_window_show(v->content);
  owl_window_show(v->status);
}

void owl_viewwin_set_onclose_hook(owl_viewwin *v, void (*onclose_hook) (owl_viewwin *vwin, void *data), void *onclose_hook_data) {
  v->onclose_hook = onclose_hook;
  v->onclose_hook_data = onclose_hook_data;
}

static void owl_viewwin_layout(owl_viewwin *v)
{
  int lines, cols;
  owl_window_get_position(v->window, &lines, &cols, NULL, NULL);
  owl_window_set_position(v->content, lines - BOTTOM_OFFSET, cols, 0, 0);
  /* Only one of these will be visible at a time: */
  owl_window_set_position(v->status, BOTTOM_OFFSET, cols, lines - BOTTOM_OFFSET, 0);
  if (v->cmdwin)
    owl_window_set_position(v->cmdwin, BOTTOM_OFFSET, cols, lines - BOTTOM_OFFSET, 0);
}

/* regenerate text on the curses window. */
static void owl_viewwin_redraw_content(owl_window *w, WINDOW *curswin, void *user_data)
{
  owl_fmtext fm1, fm2;
  owl_viewwin *v = user_data;
  int winlines, wincols;

  owl_window_get_position(w, &winlines, &wincols, 0, 0);

  werase(curswin);
  wmove(curswin, 0, 0);

  owl_fmtext_init_null(&fm1);
  owl_fmtext_init_null(&fm2);

  owl_fmtext_truncate_lines(&(v->fmtext), v->topline, winlines, &fm1);
  owl_fmtext_truncate_cols(&fm1, v->rightshift, wincols-1+v->rightshift, &fm2);

  owl_fmtext_curs_waddstr(&fm2, curswin, OWL_FMTEXT_ATTR_NONE, OWL_COLOR_DEFAULT, OWL_COLOR_DEFAULT);

  owl_fmtext_cleanup(&fm1);
  owl_fmtext_cleanup(&fm2);
}

static void owl_viewwin_redraw_status(owl_window *w, WINDOW *curswin, void *user_data)
{
  owl_viewwin *v = user_data;
  int winlines, wincols;

  owl_window_get_position(v->content, &winlines, &wincols, 0, 0);

  werase(curswin);
  wmove(curswin, 0, 0);
  wattrset(curswin, A_REVERSE);
  if (v->textlines - v->topline > winlines) {
    waddstr(curswin, "--More-- (Space to see more, 'q' to quit)");
  } else {
    waddstr(curswin, "--End-- (Press 'q' to quit)");
  }
  wattroff(curswin, A_REVERSE);
}

char *owl_viewwin_command_search(owl_viewwin *v, int argc, const char *const *argv, const char *buff)
{
  int direction, consider_current;
  const char *buffstart;

  direction=OWL_DIRECTION_DOWNWARDS;
  buffstart=skiptokens(buff, 1);
  if (argc>1 && !strcmp(argv[1], "-r")) {
    direction=OWL_DIRECTION_UPWARDS;
    buffstart=skiptokens(buff, 2);
  }

  if (argc==1 || (argc==2 && !strcmp(argv[1], "-r"))) {
    consider_current = false;
  } else {
    owl_function_set_search(buffstart);
    consider_current = true;
  }

  if (!owl_viewwin_search(v, owl_global_get_search_re(&g), consider_current, direction))
    owl_function_error("No more matches");
  return NULL;
}

typedef struct _owl_viewwin_search_data { /*noproto*/
  owl_viewwin *v;
  int direction;
} owl_viewwin_search_data;

static void owl_viewwin_callback_search(owl_editwin *e)
{
  int consider_current = false;
  const char *line = owl_editwin_get_text(e);
  owl_viewwin_search_data *data = owl_editwin_get_cbdata(e);

  /* Given an empty string, just continue the current search. */
  if (line && *line) {
    owl_function_set_search(line);
    consider_current = true;
  }
  if (!owl_viewwin_search(data->v, owl_global_get_search_re(&g),
                          consider_current, data->direction))
    owl_function_error("No matches");
}

char *owl_viewwin_command_start_search(owl_viewwin *v, int argc, const char *const *argv, const char *buff)
{
  int direction;
  const char *buffstart;
  owl_editwin *tw;
  owl_context *ctx;
  owl_viewwin_search_data *data;

  direction=OWL_DIRECTION_DOWNWARDS;
  buffstart=skiptokens(buff, 1);
  if (argc>1 && !strcmp(argv[1], "-r")) {
    direction=OWL_DIRECTION_UPWARDS;
    buffstart=skiptokens(buff, 2);
  }

  /* TODO: Add a search history? */
  tw = owl_viewwin_set_typwin_active(v, NULL);
  owl_editwin_set_locktext(tw, (direction == OWL_DIRECTION_DOWNWARDS) ? "/" : "?");
  owl_editwin_insert_string(tw, buffstart);

  data = g_new(owl_viewwin_search_data, 1);
  data->v = v;
  data->direction = direction;

  ctx = owl_editcontext_new(OWL_CTX_EDITLINE, tw, "editline",
                            owl_viewwin_deactivate_editcontext, v);
  ctx->cbdata = v;
  owl_global_push_context_obj(&g, ctx);
  owl_editwin_set_callback(tw, owl_viewwin_callback_search);
  owl_editwin_set_cbdata(tw, data, g_free);
  /* We aren't saving tw, so release the reference we were given. */
  owl_editwin_unref(tw);
  return NULL;
}

char *owl_viewwin_start_command(owl_viewwin *v, int argc, const char *const *argv, const char *buff)
{
  owl_editwin *tw;
  owl_context *ctx;

  buff = skiptokens(buff, 1);

  tw = owl_viewwin_set_typwin_active(v, owl_global_get_cmd_history(&g));
  owl_editwin_set_locktext(tw, ":");

  owl_editwin_insert_string(tw, buff);

  ctx = owl_editcontext_new(OWL_CTX_EDITLINE, tw, "editline",
                            owl_viewwin_deactivate_editcontext, v);
  owl_global_push_context_obj(&g, ctx);
  owl_editwin_set_callback(tw, owl_callback_command);
  /* We aren't saving tw, so release the reference we were given. */
  owl_editwin_unref(tw);

  return NULL;
}

void owl_viewwin_deactivate_editcontext(owl_context *ctx) {
  owl_viewwin *v = ctx->cbdata;
  owl_viewwin_set_typwin_inactive(v);
}

owl_editwin *owl_viewwin_set_typwin_active(owl_viewwin *v, owl_history *hist) {
  int lines, cols;
  owl_editwin *cmdline;
  if (v->cmdwin)
    return NULL;
  /* Create the command line. */
  v->cmdwin = owl_window_new(v->window);
  owl_viewwin_layout(v);
  owl_window_get_position(v->cmdwin, &lines, &cols, NULL, NULL);
  cmdline = owl_editwin_new(v->cmdwin, lines, cols, OWL_EDITWIN_STYLE_ONELINE, hist);
  /* Swap out the bottom window. */
  owl_window_hide(v->status);
  owl_window_show(v->cmdwin);
  return cmdline;
}

void owl_viewwin_set_typwin_inactive(owl_viewwin *v) {
  if (v->cmdwin) {
    /* Swap out the bottom window. */
    owl_window_hide(v->cmdwin);
    owl_window_show(v->status);
    /* Destroy the window itself. */
    owl_window_unlink(v->cmdwin);
    g_object_unref(v->cmdwin);
    v->cmdwin = NULL;
  }
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
  owl_window_dirty(v->content);
  owl_window_dirty(v->status);
}

void owl_viewwin_down(owl_viewwin *v, int amount) {
  int winlines;
  owl_window_get_position(v->content, &winlines, 0, 0, 0);
  /* Don't scroll past the bottom. */
  amount = MIN(amount, v->textlines - (v->topline + winlines));
  /* But if we're already past the bottom, don't back up either. */
  if (amount > 0) {
    v->topline += amount;
    owl_viewwin_dirty(v);
  }
}

void owl_viewwin_up(owl_viewwin *v, int amount)
{
  v->topline -= amount;
  if (v->topline<0) v->topline=0;
  owl_viewwin_dirty(v);
}

void owl_viewwin_pagedown(owl_viewwin *v)
{
  int winlines;
  owl_window_get_position(v->content, &winlines, 0, 0, 0);
  owl_viewwin_down(v, winlines);
}

void owl_viewwin_linedown(owl_viewwin *v)
{
  owl_viewwin_down(v, 1);
}

void owl_viewwin_pageup(owl_viewwin *v)
{
  int winlines;
  owl_window_get_position(v->content, &winlines, 0, 0, 0);
  owl_viewwin_up(v, winlines);
}

void owl_viewwin_lineup(owl_viewwin *v)
{
  owl_viewwin_up(v, 1);
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
  owl_window_get_position(v->content, &winlines, 0, 0, 0);
  v->topline = v->textlines - winlines;
  owl_viewwin_dirty(v);
}

/* This is a bit of a hack, because regexec doesn't have an 'n'
 * version. */
static int _re_memcompare(const owl_regex *re, const char *string, int start, int end)
{
  int ans;
  char *tmp = g_strndup(string + start, end - start);
  ans = owl_regex_compare(re, tmp, NULL, NULL);
  g_free(tmp);
  return !ans;
}

/* Scroll in 'direction' to the next line containing 're' in 'v',
 * starting from the current line. Returns 0 if no occurrence is
 * found.
 *
 * If consider_current is true then stay on the current line
 * if it matches.
 */
int owl_viewwin_search(owl_viewwin *v, const owl_regex *re, int consider_current, int direction)
{
  int start, end, offset;
  int lineend, linestart;
  const char *buf, *linestartp;
  owl_fmtext_line_extents(&v->fmtext, v->topline, &start, &end);
  if (direction == OWL_DIRECTION_DOWNWARDS) {
    offset = owl_fmtext_search(&v->fmtext, re,
			       consider_current ? start : end);
    if (offset < 0)
      return 0;
    v->topline = owl_fmtext_line_number(&v->fmtext, offset);
    owl_viewwin_dirty(v);
    return 1;
  } else {
    /* TODO: This is a hack. Really, we should have an owl_fmlines or
     * something containing an array of owl_fmtext split into
     * lines. Also, it cannot handle multi-line regex, if we ever care about
     * them. */
    buf = owl_fmtext_get_text(&v->fmtext);
    lineend = consider_current ? end : start;
    while (lineend > 0) {
      linestartp = memrchr(buf, '\n', lineend - 1);
      linestart = linestartp ? linestartp - buf + 1 : 0;
      if (_re_memcompare(re, buf, linestart, lineend)) {
        v->topline = owl_fmtext_line_number(&v->fmtext, linestart);
        owl_viewwin_dirty(v);
        return 1;
      }
      lineend = linestart;
    }
    return 0;
  }
}

void owl_viewwin_delete(owl_viewwin *v)
{
  if (v->onclose_hook) {
    v->onclose_hook(v, v->onclose_hook_data);
    v->onclose_hook = NULL;
    v->onclose_hook_data = NULL;
  }
  owl_viewwin_set_typwin_inactive(v);
  /* TODO: This is far too tedious. owl_viewwin should own v->window
   * and just unlink it in one go. Signals should also go away for
   * free. */
  g_signal_handler_disconnect(v->window, v->sig_resize_id);
  g_signal_handler_disconnect(v->content, v->sig_content_redraw_id);
  g_signal_handler_disconnect(v->status, v->sig_status_redraw_id);
  owl_window_unlink(v->content);
  owl_window_unlink(v->status);
  g_object_unref(v->window);
  g_object_unref(v->content);
  g_object_unref(v->status);
  owl_fmtext_cleanup(&(v->fmtext));
  g_free(v);
}
