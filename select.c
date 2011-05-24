#include "owl.h"

static GMainLoop *loop = NULL;
static GMainContext *main_context;
static int dispatch_active = 0;

static GSource *owl_io_dispatch_source;

/* Returns the valid owl_io_dispatch for a given file descriptor. */
static owl_io_dispatch *owl_select_find_valid_io_dispatch_by_fd(const int fd)
{
  int i, len;
  const owl_list *dl;
  owl_io_dispatch *d;
  dl = owl_global_get_io_dispatch_list(&g);
  len = owl_list_get_size(dl);
  for(i = 0; i < len; i++) {
    d = owl_list_get_element(dl, i);
    if (d->fd == fd && d->valid) return d;
  }
  return NULL;
}

static int owl_select_find_io_dispatch(const owl_io_dispatch *in)
{
  int i, len;
  const owl_list *dl;

  if (in != NULL) {
    dl = owl_global_get_io_dispatch_list(&g);
    len = owl_list_get_size(dl);
    for(i = 0; i < len; i++) {
      const owl_io_dispatch *d = owl_list_get_element(dl, i);
      if (d == in) return i;
    }
  }
  return -1;
}

static void owl_select_invalidate_io_dispatch(owl_io_dispatch *d)
{
  if (d == NULL || !d->valid)
    return;
  d->valid = false;
  g_source_remove_poll(owl_io_dispatch_source, &d->pollfd);
}

void owl_select_remove_io_dispatch(const owl_io_dispatch *in)
{
  int elt;
  if (in != NULL) {
    elt = owl_select_find_io_dispatch(in);
    if (elt != -1) {
      owl_list *dl = owl_global_get_io_dispatch_list(&g);
      owl_io_dispatch *d = owl_list_get_element(dl, elt);
      if (dispatch_active)
        d->needs_gc = 1;
      else {
	owl_select_invalidate_io_dispatch(d);
        owl_list_remove_element(dl, elt);
        if (d->destroy)
          d->destroy(d);
        g_free(d);
      }
    }
  }
}

static void owl_select_io_dispatch_gc(void)
{
  int i;
  owl_list *dl;

  dl = owl_global_get_io_dispatch_list(&g);
  /*
   * Count down so we aren't set off by removing items from the list
   * during the iteration.
   */
  for(i = owl_list_get_size(dl) - 1; i >= 0; i--) {
    owl_io_dispatch *d = owl_list_get_element(dl, i);
    if(d->needs_gc) {
      owl_select_remove_io_dispatch(d);
    }
  }
}

/* Each FD may have at most one valid dispatcher.
 * If a new dispatch is added for an FD, the old one is removed.
 * mode determines what types of events are watched for, and may be any combination of:
 * OWL_IO_READ, OWL_IO_WRITE, OWL_IO_EXCEPT
 */
const owl_io_dispatch *owl_select_add_io_dispatch(int fd, int mode, void (*cb)(const owl_io_dispatch *, void *), void (*destroy)(const owl_io_dispatch *), void *data)
{
  owl_io_dispatch *d = g_new(owl_io_dispatch, 1);
  owl_list *dl = owl_global_get_io_dispatch_list(&g);
  owl_io_dispatch *other;

  d->fd = fd;
  d->valid = true;
  d->needs_gc = 0;
  d->mode = mode;
  d->callback = cb;
  d->destroy = destroy;
  d->data = data;

  /* TODO: Allow changing fd and mode in the middle? Probably don't care... */
  d->pollfd.fd = fd;
  d->pollfd.events = 0;
  if (d->mode & OWL_IO_READ)
    d->pollfd.events |= G_IO_IN | G_IO_HUP | G_IO_ERR;
  if (d->mode & OWL_IO_WRITE)
    d->pollfd.events |= G_IO_OUT | G_IO_ERR;
  if (d->mode & OWL_IO_EXCEPT)
    d->pollfd.events |= G_IO_PRI | G_IO_ERR;
  g_source_add_poll(owl_io_dispatch_source, &d->pollfd);


  other = owl_select_find_valid_io_dispatch_by_fd(fd);
  if (other)
    owl_select_invalidate_io_dispatch(other);
  owl_list_append_element(dl, d);

  return d;
}

static gboolean owl_io_dispatch_prepare(GSource *source, int *timeout) {
  *timeout = -1;
  return FALSE;
}

static gboolean owl_io_dispatch_check(GSource *source) {
  int i, len;
  const owl_list *dl;

  dl = owl_global_get_io_dispatch_list(&g);
  len = owl_list_get_size(dl);
  for(i = 0; i < len; i++) {
    owl_io_dispatch *d = owl_list_get_element(dl, i);
    if (!d->valid) continue;
    if (d->pollfd.revents & G_IO_NVAL) {
      owl_function_debugmsg("Pruning defunct dispatch on fd %d.", d->fd);
      owl_select_invalidate_io_dispatch(d);
    }
    if (d->pollfd.revents & d->pollfd.events)
      return TRUE;
  }
  return FALSE;
}

static gboolean owl_io_dispatch_dispatch(GSource *source, GSourceFunc callback, gpointer user_data) {
  int i, len;
  const owl_list *dl;

  dispatch_active = 1;
  dl = owl_global_get_io_dispatch_list(&g);
  len = owl_list_get_size(dl);
  for (i = 0; i < len; i++) {
    owl_io_dispatch *d = owl_list_get_element(dl, i);
    if (!d->valid) continue;
    if ((d->pollfd.revents & d->pollfd.events) && d->callback != NULL) {
      d->callback(d, d->data);
    }
  }
  dispatch_active = 0;
  owl_select_io_dispatch_gc();

  return TRUE;
}

static GSourceFuncs owl_io_dispatch_funcs = {
  owl_io_dispatch_prepare,
  owl_io_dispatch_check,
  owl_io_dispatch_dispatch,
  NULL
};

void owl_select_init(void)
{
  owl_io_dispatch_source = g_source_new(&owl_io_dispatch_funcs, sizeof(GSource));
  g_source_attach(owl_io_dispatch_source, NULL);
}

void owl_select_run_loop(void)
{
  main_context = g_main_context_default();
  loop = g_main_loop_new(main_context, FALSE);
  g_main_loop_run(loop);
}

void owl_select_quit_loop(void)
{
  if (loop) {
    g_main_loop_quit(loop);
    loop = NULL;
  }
}

typedef struct _owl_task { /*noproto*/
  void (*cb)(void *);
  void *cbdata;
  void (*destroy_cbdata)(void *);
} owl_task;

static gboolean _run_task(gpointer data)
{
  owl_task *t = data;
  if (t->cb)
    t->cb(t->cbdata);
  return FALSE;
}

static void _destroy_task(void *data)
{
  owl_task *t = data;
  if (t->destroy_cbdata)
    t->destroy_cbdata(t->cbdata);
  g_free(t);
}

void owl_select_post_task(void (*cb)(void*), void *cbdata, void (*destroy_cbdata)(void*), GMainContext *context)
{
  GSource *source = g_idle_source_new();
  owl_task *t = g_new0(owl_task, 1);
  t->cb = cb;
  t->cbdata = cbdata;
  t->destroy_cbdata = destroy_cbdata;
  g_source_set_priority(source, G_PRIORITY_DEFAULT);
  g_source_set_callback(source, _run_task, t, _destroy_task);
  g_source_attach(source, context);
  g_source_unref(source);
}
