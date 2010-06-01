#ifndef __BARNOWL_WINDOW_H__
#define __BARNOWL_WINDOW_H__

#include <glib.h>

typedef struct _owl_window owl_window;

owl_window *owl_window_get_screen(void);

owl_window *owl_window_new(owl_window *parent);
void owl_window_delete(owl_window *w);

void owl_window_set_redraw_cb(owl_window *w, void (*cb)(owl_window*, WINDOW*, void*), void *cbdata, void (*cbdata_destroy)(void*));
void owl_window_set_size_cb(owl_window *w, void (*cb)(owl_window*, void*), void *cbdata, void (*cbdata_destroy)(void*));
void owl_window_set_destroy_cb(owl_window *w, void (*cb)(owl_window*, void*), void *cbdata, void (*cbdata_destroy)(void*));

void owl_window_children_foreach(owl_window *parent, GFunc func, gpointer user_data);
void owl_window_children_foreach_onearg(owl_window *parent, void (*func)(owl_window*));
owl_window *owl_window_get_parent(owl_window *w);

void owl_window_show(owl_window *w, int recurse);
void owl_window_hide(owl_window *w);
int owl_window_is_shown(owl_window *w);
int owl_window_is_realized(owl_window *w);
int owl_window_is_toplevel(owl_window *w);

void owl_window_dirty(owl_window *w);
void owl_window_dirty_children(owl_window *w);
void owl_window_redraw_scheduled(void);

void owl_window_get_position(owl_window *w, int *nlines, int *ncols, int *begin_y, int *begin_x);
void owl_window_set_position(owl_window *w, int nlines, int ncols, int begin_y, int begin_x);
void owl_window_move(owl_window *w, int begin_y, int begin_x);
void owl_window_resize(owl_window *w, int nlines, int ncols);
void owl_window_recompute_position(owl_window *w);

void owl_window_top(owl_window *w);
owl_window *owl_window_above(owl_window *w);
owl_window *owl_window_below(owl_window *w);

void owl_window_erase_cb(owl_window *w, WINDOW *win, void *user_data);

#endif /* __BARNOWL_WINDOW_H__ */
