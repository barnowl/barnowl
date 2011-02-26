#include "owl.h"

#include <assert.h>

struct _owl_window { /*noproto*/
  GObject object;
  /* hierarchy information */
  owl_window *parent;
  owl_window *child;
  owl_window *next, *prev;
  /* flags */
  int dirty : 1;
  int dirty_subtree : 1;
  int shown : 1;
  int is_screen : 1;
  /* window information */
  WINDOW *win;
  PANEL *pan;
  int nlines, ncols;
  int begin_y, begin_x;
};

enum {
  REDRAW,
  RESIZED,
  LAST_SIGNAL
};

static guint window_signals[LAST_SIGNAL] = { 0 };

static void owl_window_dispose(GObject *gobject);
static void owl_window_finalize(GObject *gobject);

static owl_window *_owl_window_new(owl_window *parent, int nlines, int ncols, int begin_y, int begin_x);

static void _owl_window_link(owl_window *w, owl_window *parent);

static void _owl_window_create_curses(owl_window *w);
static void _owl_window_destroy_curses(owl_window *w);

static void _owl_window_realize(owl_window *w);
static void _owl_window_unrealize(owl_window *w);

static owl_window *cursor_owner;
static owl_window *default_cursor;

/* clang gets upset about the glib argument chopping hack because it manages to
 * inline owl_window_children_foreach. user_data should be a pointer to a
 * FuncOneArg. */
typedef void (*FuncOneArg)(void *);
static void first_arg_only(gpointer data, gpointer user_data)
{
  FuncOneArg *func = user_data;
  (*func)(data);
}

G_DEFINE_TYPE (OwlWindow, owl_window, G_TYPE_OBJECT)

static void owl_window_class_init (OwlWindowClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  /* Set up the vtabl */
  gobject_class->dispose = owl_window_dispose;
  gobject_class->finalize = owl_window_finalize;

  klass->redraw = NULL;
  klass->resized = NULL;

  /* Create the signals, remember IDs */
  window_signals[REDRAW] =
    g_signal_new("redraw",
                 G_TYPE_FROM_CLASS(gobject_class),
                 G_SIGNAL_RUN_CLEANUP,
                 G_STRUCT_OFFSET(OwlWindowClass, redraw),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__POINTER,
                 G_TYPE_NONE,
                 1,
                 G_TYPE_POINTER, NULL);

  /* TODO: maybe type should be VOID__INT_INT_INT_INT; will need to generate a
   * marshaller */
  window_signals[RESIZED] =
    g_signal_new("resized",
                 G_TYPE_FROM_CLASS(gobject_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(OwlWindowClass, resized),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE,
                 0,
                 NULL);
}

static void owl_window_dispose (GObject *object)
{
  owl_window *w = OWL_WINDOW (object);

  /* Unmap the window */
  owl_window_hide (w);

  /* Unlink and unref all children */
  while (w->child) {
    owl_window *child = w->child;
    owl_window_unlink (child);
  }

  /* Remove from hierarchy */
  owl_window_unlink (w);

  G_OBJECT_CLASS (owl_window_parent_class)->dispose (object);
}

static void owl_window_finalize (GObject *object)
{
  owl_window *w = OWL_WINDOW(object);

  if (w->pan) {
    del_panel(w->pan);
    w->pan = NULL;
  }

  G_OBJECT_CLASS (owl_window_parent_class)->finalize (object);
}

static void owl_window_init (owl_window *w)
{
}

/** singletons **/

static WINDOW *_dummy_window(void)
{
  static WINDOW *dummy = NULL;
  if (!dummy) {
    dummy = newwin(1, 1, 0, 0);
  }
  return dummy;
}

owl_window *owl_window_get_screen(void)
{
  static owl_window *screen = NULL;
  if (!screen) {
    /* The screen is magical. It's 'shown', but the only mean of it going
     * invisible is if we're tore down curses (i.e. screen resize) */
    screen = _owl_window_new(NULL, g.lines, g.cols, 0, 0);
    screen->is_screen = 1;
    owl_window_show(screen);
  }
  return screen;
}

/** Creation and Destruction **/

owl_window *owl_window_new(owl_window *parent)
{
  if (!parent)
    parent = owl_window_get_screen();
  return _owl_window_new(parent, 0, 0, 0, 0);
}

static owl_window *_owl_window_new(owl_window *parent, int nlines, int ncols, int begin_y, int begin_x)
{
  owl_window *w;

  w = g_object_new (OWL_TYPE_WINDOW, NULL);
  if (w == NULL) g_error("Failed to create owl_window instance");

  w->nlines = nlines;
  w->ncols = ncols;
  w->begin_y = begin_y;
  w->begin_x = begin_x;

  _owl_window_link(w, parent);
  if (parent && parent->is_screen) {
    w->pan = new_panel(_dummy_window());
    set_panel_userptr(w->pan, w);
  }

  return w;
}

/** Hierarchy **/

void owl_window_unlink(owl_window *w)
{
  /* make sure the window is unmapped first */
  _owl_window_unrealize(w);
  /* unlink parent/child information */
  if (w->parent) {
    if (w->prev)
      w->prev->next = w->next;
    if (w->next)
      w->next->prev = w->prev;
    if (w->parent->child == w)
      w->parent->child = w->next;
    w->parent = NULL;
    g_object_unref (w);
  }
}

static void _owl_window_link(owl_window *w, owl_window *parent)
{
  if (w->parent == parent)
    return;

  owl_window_unlink(w);
  if (parent) {
    w->parent = parent;
    w->next = parent->child;
    if (w->next)
      w->next->prev = w;
    parent->child = w;
    g_object_ref (w);
  }
}

/* mimic g_list_foreach for consistency */
void owl_window_children_foreach(owl_window *parent, GFunc func, gpointer user_data)
{
  owl_window *w;
  for (w = parent->child;
       w != NULL;
       w = w->next) {
    func(w, user_data);
  }
}

owl_window *owl_window_parent(owl_window *w)
{
  return w->parent;
}

owl_window *owl_window_first_child(owl_window *w)
{
  return w->child;
}

owl_window *owl_window_next_sibling(owl_window *w)
{
  return w->next;
}

owl_window *owl_window_previous_sibling(owl_window *w)
{
  return w->prev;
}

/** Internal window management **/

static void _owl_window_create_curses(owl_window *w)
{
  if (w->is_screen) {
    resizeterm(w->nlines, w->ncols);
    w->win = stdscr;
  } else {
    /* Explicitly disallow realizing an unlinked non-root */
    if (w->parent == NULL || w->parent->win == NULL)
      return;
    if (w->pan) {
      w->win = newwin(w->nlines, w->ncols, w->begin_y, w->begin_x);
      replace_panel(w->pan, w->win);
    } else {
      w->win = derwin(w->parent->win, w->nlines, w->ncols, w->begin_y, w->begin_x);
    }
  }
}

static void _owl_window_destroy_curses(owl_window *w)
{
  if (w->is_screen) {
    /* don't deallocate the dummy */
    w->win = NULL;
  } else {
    if (w->pan) {
      /* panels assume their windows always exist, so we put in a fake one */
      replace_panel(w->pan, _dummy_window());
    }
    if (w->win) {
      /* and destroy our own window */
      delwin(w->win);
      w->win = NULL;
    }
  }
}

void owl_window_show(owl_window *w)
{
  w->shown = 1;
  _owl_window_realize(w);
  if (w->pan)
    show_panel(w->pan);
}

void owl_window_show_all(owl_window *w)
{
  FuncOneArg ptr = (FuncOneArg)owl_window_show;
  owl_window_show(w);
  owl_window_children_foreach(w, first_arg_only, &ptr);
}

void owl_window_hide(owl_window *w)
{
  /* you can't unmap the screen */
  if (w->is_screen)
    return;
  w->shown = 0;
  if (w->pan)
    hide_panel(w->pan);
  _owl_window_unrealize(w);
}

bool owl_window_is_shown(owl_window *w)
{
  return w->shown;
}

bool owl_window_is_realized(owl_window *w)
{
  return w->win != NULL;
}

bool owl_window_is_toplevel(owl_window *w)
{
  return w->pan != NULL;
}

bool owl_window_is_subwin(owl_window *w)
{
  return w->pan == NULL && !w->is_screen;
}

static bool _owl_window_should_realize(owl_window *w)
{
  return owl_window_is_shown(w) &&
    (!w->parent || owl_window_is_realized(w->parent));
}

static void _owl_window_realize_later(owl_window *w)
{
  if (owl_window_is_realized(w) || !_owl_window_should_realize(w))
    return;
  owl_window_dirty(w);
}

static void _owl_window_realize(owl_window *w)
{
  FuncOneArg ptr = (FuncOneArg)_owl_window_realize_later;
  /* check if we can create a window */
  if (owl_window_is_realized(w) || !_owl_window_should_realize(w))
    return;
  if (w->nlines <= 0 || w->ncols <= 0)
    return;
  _owl_window_create_curses(w);
  if (w->win == NULL)
    return;
  /* schedule a repaint */
  owl_window_dirty(w);
  /* map the children */
  owl_window_children_foreach(w, first_arg_only, &ptr);
}

static void _owl_window_unrealize(owl_window *w)
{
  FuncOneArg ptr = (FuncOneArg)_owl_window_unrealize;
  if (w->win == NULL)
    return;
  /* unmap all the children */
  owl_window_children_foreach(w, first_arg_only, &ptr);
  _owl_window_destroy_curses(w);
  w->dirty = w->dirty_subtree = 0;
  /* subwins leave a mess in the parent; dirty it */
  if (w->parent)
    owl_window_dirty(w->parent);
}

/** Painting and book-keeping **/

void owl_window_set_cursor(owl_window *w)
{
  if (cursor_owner)
    g_object_remove_weak_pointer(G_OBJECT(cursor_owner), (gpointer*) &cursor_owner);
  cursor_owner = w;
  if (w)
    g_object_add_weak_pointer(G_OBJECT(w), (gpointer*) &cursor_owner);
  owl_window_dirty(owl_window_get_screen());
}

void owl_window_set_default_cursor(owl_window *w)
{
  if (default_cursor)
    g_object_remove_weak_pointer(G_OBJECT(default_cursor), (gpointer*) &default_cursor);
  default_cursor = w;
  if (w)
    g_object_add_weak_pointer(G_OBJECT(w), (gpointer*) &default_cursor);
  owl_window_dirty(owl_window_get_screen());
}

static owl_window *_get_cursor(void)
{
  if (cursor_owner && owl_window_is_realized(cursor_owner))
    return cursor_owner;
  if (default_cursor && owl_window_is_realized(default_cursor))
    return default_cursor;
  return owl_window_get_screen();
}

void owl_window_dirty(owl_window *w)
{
  if (!_owl_window_should_realize(w))
    return;
  if (!w->dirty) {
    w->dirty = 1;
    while (w && !w->dirty_subtree) {
      w->dirty_subtree = 1;
      w = w->parent;
    }
  }
}

void owl_window_dirty_children(owl_window *w)
{
  FuncOneArg ptr = (FuncOneArg)owl_window_dirty;
  owl_window_children_foreach(w, first_arg_only, &ptr);
}

static void _owl_window_redraw(owl_window *w)
{
  if (!w->dirty) return;
  _owl_window_realize(w);
  if (w->win && !w->is_screen) {
    if (owl_window_is_subwin(w)) {
      /* If a subwin, we might have gotten random touched lines from wsyncup or
       * past drawing. That information is useless, so we discard it all */
      untouchwin(w->win);
    }
    g_signal_emit(w, window_signals[REDRAW], 0, w->win);
    wsyncup(w->win);
  }
  w->dirty = 0;
}

static void _owl_window_redraw_subtree(owl_window *w)
{
  FuncOneArg ptr = (FuncOneArg)_owl_window_redraw_subtree;
  if (!w->dirty_subtree)
    return;
  _owl_window_redraw(w);
  owl_window_children_foreach(w, first_arg_only, &ptr);
  w->dirty_subtree = 0;
}

/*
Redraw all the windows with scheduled redraws.
NOTE: This function shouldn't be called outside the event loop
*/
void owl_window_redraw_scheduled(void)
{
  owl_window *cursor;
  owl_window *screen = owl_window_get_screen();

  if (!screen->dirty_subtree)
    return;
  _owl_window_redraw_subtree(screen);
  update_panels();
  cursor = _get_cursor();
  if (cursor && cursor->win) {
    /* untouch it to avoid drawing; window should be clean now, but we must
     * clean up in case there was junk left over on a subwin (cleaning up after
     * subwin drawing isn't sufficient because a wsyncup messes up subwin
     * ancestors */
    untouchwin(cursor->win);
    wnoutrefresh(cursor->win);
  }
  doupdate();
}

/** Window position **/

void owl_window_get_position(owl_window *w, int *nlines, int *ncols, int *begin_y, int *begin_x)
{
  if (nlines)  *nlines  = w->nlines;
  if (ncols)   *ncols   = w->ncols;
  if (begin_y) *begin_y = w->begin_y;
  if (begin_x) *begin_x = w->begin_x;
}

void owl_window_move(owl_window *w, int begin_y, int begin_x)
{
  /* It is possible to move a window efficiently with move_panel and mvderwin,
   * but begy and begx are then wrong. Currently, this only effects the
   * wnoutrefresh to move cursor. TODO: fix that and reinstate that
   * optimization if it's worth the trouble */
  owl_window_set_position(w, w->nlines, w->ncols, begin_y, begin_x);
}

void owl_window_set_position(owl_window *w, int nlines, int ncols, int begin_y, int begin_x)
{
  int resized;

  if (w->nlines == nlines && w->ncols == ncols
      && w->begin_y == begin_y && w->begin_x == begin_x) {
    return;
  }
  resized = w->nlines != nlines || w->ncols != ncols;

  _owl_window_unrealize(w);
  w->begin_y = begin_y;
  w->begin_x = begin_x;
  w->nlines = nlines;
  w->ncols = ncols;
  if (resized)
    g_signal_emit(w, window_signals[RESIZED], 0);
  if (w->shown) {
    /* ncurses is screwy: give up and recreate windows at the right place */
    _owl_window_realize_later(w);
  }
}

void owl_window_resize(owl_window *w, int nlines, int ncols)
{
  owl_window_set_position(w, nlines, ncols, w->begin_y, w->begin_x);
}

/** Redrawing main loop hooks **/

static bool _owl_window_should_redraw(void) {
  return g.resizepending || owl_window_get_screen()->dirty_subtree;
}

static gboolean _owl_window_redraw_prepare(GSource *source, int *timeout) {
  *timeout = -1;
  return _owl_window_should_redraw();
}

static gboolean _owl_window_redraw_check(GSource *source) {
  return _owl_window_should_redraw();
}

static gboolean _owl_window_redraw_dispatch(GSource *source, GSourceFunc callback, gpointer user_data) {
  owl_colorpair_mgr *cpmgr;

  /* if a resize has been scheduled, deal with it */
  owl_global_check_resize(&g);
  /* update the terminal if we need to */
  owl_window_redraw_scheduled();
  /* On colorpair shortage, reset and redraw /everything/. NOTE: if
   * the current screen uses too many colorpairs, this draws
   * everything twice. But this is unlikely; COLOR_PAIRS is 64 with
   * 8+1 colors, and 256^2 with 256+1 colors. (+1 for default.) */
  cpmgr = owl_global_get_colorpair_mgr(&g);
  if (cpmgr->overflow) {
    owl_function_debugmsg("colorpairs: color shortage; reset pairs and redraw. COLOR_PAIRS = %d", COLOR_PAIRS);
    owl_fmtext_reset_colorpairs(cpmgr);
    owl_function_full_redisplay();
    owl_window_redraw_scheduled();
  }
  return TRUE;
}

static GSourceFuncs redraw_funcs = {
  _owl_window_redraw_prepare,
  _owl_window_redraw_check,
  _owl_window_redraw_dispatch,
  NULL
};

GSource *owl_window_redraw_source_new(void) {
  GSource *source;
  source = g_source_new(&redraw_funcs, sizeof(GSource));
  /* TODO: priority?? */
  return source;
}
