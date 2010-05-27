#include "owl.h"

#include <assert.h>

struct _owl_window { /*noproto*/
  /* hierarchy information */
  owl_window *parent;
  owl_window *child;
  owl_window *next, *prev;
  /* flags */
  int dirty : 1;
  int dirty_subtree : 1;
  int mapped : 1;
  int is_screen : 1;
  /* window information */
  WINDOW *win;
  PANEL *pan;
  int nlines, ncols;
  int begin_y, begin_x;
  /* hooks */
  void (*redraw_cb)(owl_window *, WINDOW *, void *);
  void *redraw_cbdata;
  void (*redraw_cbdata_destroy)(void *);

  void (*resize_cb)(owl_window *, void *);
  void *resize_cbdata;
  void (*resize_cbdata_destroy)(void *);

  void (*destroy_cb)(owl_window *, void *);
  void *destroy_cbdata;
  void (*destroy_cbdata_destroy)(void *);
};

static owl_window *_owl_window_new(owl_window *parent, int nlines, int ncols, int begin_y, int begin_x);

static void _owl_window_unlink(owl_window *w);
static void _owl_window_link(owl_window *w, owl_window *parent);

static void _owl_window_create_curses(owl_window *w);
static void _owl_window_destroy_curses(owl_window *w);

static void _owl_window_map_internal(owl_window *w);
static void _owl_window_unmap_internal(owl_window *w);

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
    /* The screen is magical. It's 'mapped', but the only mean of it going
     * invisible is if we're tore down curses (i.e. screen resize) */
    screen = _owl_window_new(NULL, g.lines, g.cols, 0, 0);
    screen->is_screen = 1;
    owl_window_map(screen, 0);
  }
  return screen;
}

/** Creation and Destruction **/

owl_window *owl_window_new(owl_window *parent, int nlines, int ncols, int begin_y, int begin_x)
{
  if (!parent)
    parent = owl_window_get_screen();
  return _owl_window_new(parent, nlines, ncols, begin_y, begin_x);
}

static owl_window *_owl_window_new(owl_window *parent, int nlines, int ncols, int begin_y, int begin_x)
{
  owl_window *w;

  w = owl_malloc(sizeof(owl_window));
  memset(w, 0, sizeof(*w));

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

void owl_window_delete(owl_window *w)
{
  /* destroy all children */
  owl_window_children_foreach_onearg(w, owl_window_delete);

  /* notify everyone of your imminent demise */
  if (w->destroy_cb)
    w->destroy_cb(w, w->destroy_cbdata);

  /* clear all cbs */
  owl_window_set_redraw_cb(w, 0, 0, 0);
  owl_window_set_resize_cb(w, 0, 0, 0);
  owl_window_set_destroy_cb(w, 0, 0, 0);

  /* destroy curses structures */
  owl_window_unmap(w);
  if (w->pan) {
    del_panel(w->pan);
  }

  /* remove from hierarchy */
  _owl_window_unlink(w);
  owl_free(w);
}

/** Callbacks **/

void owl_window_set_redraw_cb(owl_window *w, void (*cb)(owl_window*, WINDOW*, void*), void *cbdata, void (*cbdata_destroy)(void*))
{
  if (w->redraw_cbdata_destroy) {
    w->redraw_cbdata_destroy(w->redraw_cbdata);
    w->redraw_cbdata = 0;
    w->redraw_cbdata_destroy = 0;
  }

  w->redraw_cb = cb;
  w->redraw_cbdata = cbdata;
  w->redraw_cbdata_destroy = cbdata_destroy;

  /* mark the window as dirty, to take new cb in account */
  owl_window_dirty(w);
}

void owl_window_set_resize_cb(owl_window *w, void (*cb)(owl_window*, void*), void *cbdata, void (*cbdata_destroy)(void*))
{
  if (w->resize_cbdata_destroy) {
    w->resize_cbdata_destroy(w->resize_cbdata);
    w->resize_cbdata = 0;
    w->resize_cbdata_destroy = 0;
  }

  w->resize_cb = cb;
  w->resize_cbdata = cbdata;
  w->resize_cbdata_destroy = cbdata_destroy;
}

void owl_window_set_destroy_cb(owl_window *w, void (*cb)(owl_window*, void*), void *cbdata, void (*cbdata_destroy)(void*))
{
  if (w->destroy_cbdata_destroy) {
    w->destroy_cbdata_destroy(w->destroy_cbdata);
    w->destroy_cbdata = 0;
    w->destroy_cbdata_destroy = 0;
  }

  w->destroy_cb = cb;
  w->destroy_cbdata = cbdata;
  w->destroy_cbdata_destroy = cbdata_destroy;
}

/** Hierarchy **/

static void _owl_window_unlink(owl_window *w)
{
  /* unlink parent/child information */
  if (w->parent) {
    if (w->prev)
      w->prev->next = w->next;
    if (w->next)
      w->next->prev = w->prev;
    if (w->parent->child == w)
      w->parent->child = w->next;
  }
  w->parent = NULL;
}

static void _owl_window_link(owl_window *w, owl_window *parent)
{
  if (w->parent == parent)
    return;
  if (w->parent)
    _owl_window_unlink(w);

  w->parent = parent;
  if (parent) {
    w->next = parent->child;
    parent->child = w;
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

void owl_window_children_foreach_onearg(owl_window *parent, void (*func)(owl_window*))
{
  owl_window *w;
  for (w = parent->child;
       w != NULL;
       w = w->next) {
    func(w);
  }
}

owl_window *owl_window_get_parent(owl_window *w)
{
  return w->parent;
}

/** Internal window management **/

static void _owl_window_create_curses(owl_window *w)
{
  if (w->is_screen) {
    resizeterm(w->nlines, w->ncols);
    w->win = stdscr;
  } else if (w->pan) {
    w->win = newwin(w->nlines, w->ncols, w->begin_y, w->begin_x);
    replace_panel(w->pan, w->win);
  } else {
    w->win = derwin(w->parent->win, w->nlines, w->ncols, w->begin_y, w->begin_x);
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

static void _map_recurse_curry(owl_window *w)
{
  owl_window_map(w, 1);
}

void owl_window_map(owl_window *w, int recurse)
{
  w->mapped = 1;
  _owl_window_map_internal(w);
  if (w->pan)
    show_panel(w->pan);
  if (recurse)
    owl_window_children_foreach_onearg(w, _map_recurse_curry);
}

void owl_window_unmap(owl_window *w)
{
  /* you can't unmap the screen */
  if (w->is_screen)
    return;
  w->mapped = 0;
  if (w->pan)
    hide_panel(w->pan);
  _owl_window_unmap_internal(w);
}

int owl_window_is_mapped(owl_window *w)
{
  return w->mapped;
}

int owl_window_is_visible(owl_window *w)
{
  return w->win != NULL;
}

int owl_window_is_toplevel(owl_window *w)
{
  return w->pan != NULL;
}

static void _owl_window_map_internal(owl_window *w)
{
  /* check if we can create a window */
  if ((w->parent && w->parent->win == NULL)
      || !w->mapped
      || w->win != NULL)
    return;
  _owl_window_create_curses(w);
  /* schedule a repaint */
  owl_window_dirty(w);
  /* map the children, unless we failed */
  if (w->win)
    owl_window_children_foreach_onearg(w, _owl_window_map_internal);
}

static void _owl_window_unmap_internal(owl_window *w)
{
  if (w->win == NULL)
    return;
  /* unmap all the children */
  owl_window_children_foreach_onearg(w, _owl_window_unmap_internal);
  _owl_window_destroy_curses(w);
  /* subwins leave a mess in the parent; dirty it */
  if (w->parent)
    owl_window_dirty(w->parent);
}

/** Painting and book-keeping **/

void owl_window_dirty(owl_window *w)
{
  /* don't put the screen on this list; pointless */
  if (w->is_screen)
    return;
  if (!w->dirty) {
    w->dirty = 1;
    while (w && !w->dirty_subtree) {
      w->dirty_subtree = 1;
      w = w->parent;
    }
    owl_global_set_needrefresh(&g);
  }
}

void owl_window_dirty_children(owl_window *w)
{
  owl_window_children_foreach_onearg(w, owl_window_dirty);
}

static void _owl_window_redraw(owl_window *w)
{
  if (!w->dirty) return;
  if (w->win && w->redraw_cb) {
    w->redraw_cb(w, w->win, w->redraw_cbdata);
    wsyncup(w->win);
  }
  w->dirty = 0;
}

static void _owl_window_redraw_subtree(owl_window *w)
{
  if (!w->dirty_subtree)
    return;
  _owl_window_redraw(w);
  owl_window_children_foreach_onearg(w, _owl_window_redraw_subtree);
}

/*
Redraw all the windows with scheduled redraws.
NOTE: This function shouldn't be called outside the event loop
*/
void owl_window_redraw_scheduled(void)
{
  _owl_window_redraw_subtree(owl_window_get_screen());
}

void owl_window_erase_cb(owl_window *w, WINDOW *win, void *user_data)
{
  werase(win);
  owl_window_dirty_children(w);
}

/** Window position **/

void owl_window_get_position(owl_window *w, int *nlines, int *ncols, int *begin_y, int *begin_x)
{
  if (nlines)  *nlines  = w->nlines;
  if (ncols)   *ncols   = w->ncols;
  if (begin_y) *begin_y = w->begin_y;
  if (begin_x) *begin_x = w->begin_x;
}

void owl_window_set_position(owl_window *w, int nlines, int ncols, int begin_y, int begin_x)
{
  /* can't move the screen */
  if (w->is_screen) {
    begin_y = begin_x = 0;
  }

  if (w->nlines == nlines && w->ncols == ncols) {
    /* moving is easier */
    owl_window_move(w, begin_y, begin_x);
    return;
  }

  w->begin_y = begin_y;
  w->begin_x = begin_x;
  w->nlines = nlines;
  w->ncols = ncols;
  /* window is mapped, we must try to have a window at the end */
  if (w->mapped) {
    /* resizing in ncurses is hard: give up do a unmap/map */
    _owl_window_unmap_internal(w);
  }
  /* call the resize hooks BEFORE remapping, so that everything can resize */
  if (w->resize_cb)
    w->resize_cb(w, w->resize_cbdata);
  if (w->mapped) {
    _owl_window_map_internal(w);
  }
}

void owl_window_resize(owl_window *w, int nlines, int ncols)
{
  owl_window_set_position(w, nlines, ncols, w->begin_y, w->begin_x);
}

void owl_window_move(owl_window *w, int begin_y, int begin_x)
{
  /* can't move the screen */
  if (w->is_screen) return;

  w->begin_y = begin_y;
  w->begin_x = begin_x;
  if (w->mapped) {
    /* Window is mapped, we must try to have a window at the end */
    if (w->win) {
      /* We actually do have a window; let's move it */
      if (w->pan) {
        if (move_panel(w->pan, begin_y, begin_x) == OK)
          return;
      } else {
        if (mvderwin(w->win, begin_y, begin_x) == OK) {
          /* now both we and the parent are dirty */
          owl_window_dirty(w->parent);
          owl_window_dirty(w);
          return;
        }
      }
    }
    /* We don't have a window or failed to move it. Fine. Brute force. */
    _owl_window_unmap_internal(w);
    _owl_window_map_internal(w);
  }
}

/** Stacking order **/

void owl_window_top(owl_window *w) {
  if (!w->pan) {
    owl_function_debugmsg("Warning: owl_window_top only makes sense on top-level window");
    return;
  }
  top_panel(w->pan);
}

owl_window *owl_window_above(owl_window *w) {
  PANEL *pan;

  if (!w->pan) {
    owl_function_debugmsg("Warning: owl_window_above only makes sense on top-level window");
    return NULL;
  }
  pan = panel_above(w->pan);
  /* cast because panel_userptr pointlessly returns a const void * */
  return pan ? (void*) panel_userptr(pan) : NULL;
}

owl_window *owl_window_below(owl_window *w) {
  PANEL *pan;

  if (!w->pan) {
    owl_function_debugmsg("Warning: owl_window_above only makes sense on top-level window");
    return NULL;
  }
  pan = panel_below(w->pan);
  /* cast because panel_userptr pointlessly returns a const void * */
  return pan ? (void*) panel_userptr(pan) : NULL;
}
