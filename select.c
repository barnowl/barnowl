#include "owl.h"

static GMainLoop *loop = NULL;
static GMainContext *context;
static int dispatch_active = 0;

static GSource *owl_timer_source;
static GSource *owl_io_dispatch_source;

static int _owl_select_timer_cmp(const owl_timer *t1, const owl_timer *t2);
static void owl_select_io_dispatch_gc(void);

static gboolean owl_timer_prepare(GSource *source, int *timeout) {
  GList **timers = owl_global_get_timerlist(&g);
  GTimeVal now;

  /* TODO: In the far /far/ future, g_source_get_time is what the cool
   * kids use to get system monotonic time. */
  g_source_get_current_time(source, &now);

  /* FIXME: bother with millisecond accuracy now that we can? */
  if (*timers) {
    owl_timer *t = (*timers)->data;
    *timeout = t->time - now.tv_sec;
    if (*timeout <= 0) {
      *timeout = 0;
      return TRUE;
    }
    if (*timeout > 60 * 1000)
      *timeout = 60 * 1000;
  } else {
    *timeout = 60 * 1000;
  }
  return FALSE;
}

static gboolean owl_timer_check(GSource *source) {
  GList **timers = owl_global_get_timerlist(&g);
  GTimeVal now;

  /* TODO: In the far /far/ future, g_source_get_time is what the cool
   * kids use to get system monotonic time. */
  g_source_get_current_time(source, &now);

  /* FIXME: bother with millisecond accuracy now that we can? */
  if (*timers) {
    owl_timer *t = (*timers)->data;
    return t->time >= now.tv_sec;
  }
  return FALSE;
}


static gboolean owl_timer_dispatch(GSource *source, GSourceFunc callback, gpointer user_data) {
  GList **timers = owl_global_get_timerlist(&g);
  GTimeVal now;

  /* TODO: In the far /far/ future, g_source_get_time is what the cool
   * kids use to get system monotonic time. */
  g_source_get_current_time(source, &now);

  /* FIXME: bother with millisecond accuracy now that we can? */
  while(*timers) {
    owl_timer *t = (*timers)->data;
    int remove = 0;

    if(t->time > now.tv_sec)
      break;

    /* Reschedule if appropriate */
    if(t->interval > 0) {
      t->time = now.tv_sec + t->interval;
      *timers = g_list_remove(*timers, t);
      *timers = g_list_insert_sorted(*timers, t,
                                     (GCompareFunc)_owl_select_timer_cmp);
    } else {
      remove = 1;
    }

    /* Do the callback */
    t->callback(t, t->data);
    if(remove) {
      owl_select_remove_timer(t);
    }
  }
  return TRUE;
}

static GSourceFuncs owl_timer_funcs = {
  owl_timer_prepare,
  owl_timer_check,
  owl_timer_dispatch,
  NULL
};

static int _owl_select_timer_cmp(const owl_timer *t1, const owl_timer *t2) {
  return t1->time - t2->time;
}

owl_timer *owl_select_add_timer(const char* name, int after, int interval, void (*cb)(owl_timer *, void *), void (*destroy)(owl_timer*), void *data)
{
  owl_timer *t = g_new(owl_timer, 1);
  GList **timers = owl_global_get_timerlist(&g);

  t->time = time(NULL) + after;
  t->interval = interval;
  t->callback = cb;
  t->destroy = destroy;
  t->data = data;
  t->name = name ? g_strdup(name) : NULL;

  *timers = g_list_insert_sorted(*timers, t,
                                 (GCompareFunc)_owl_select_timer_cmp);
  return t;
}

void owl_select_remove_timer(owl_timer *t)
{
  GList **timers = owl_global_get_timerlist(&g);
  if (t && g_list_find(*timers, t)) {
    *timers = g_list_remove(*timers, t);
    if(t->destroy) {
      t->destroy(t);
    }
    g_free(t->name);
    g_free(t);
  }
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
    const owl_io_dispatch *d = owl_list_get_element(dl, i);
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

static const owl_io_dispatch *owl_select_find_io_dispatch_by_fd(const int fd)
{
  int i, len;
  const owl_list *dl;
  owl_io_dispatch *d;
  dl = owl_global_get_io_dispatch_list(&g);
  len = owl_list_get_size(dl);
  for(i = 0; i < len; i++) {
    d = owl_list_get_element(dl, i);
    if (d->fd == fd) return d;
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
        owl_list_remove_element(dl, elt);
        if (d->destroy)
          d->destroy(d);
	g_source_remove_poll(owl_io_dispatch_source, &d->pollfd);
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

/* Each FD may have at most one dispatcher.
 * If a new dispatch is added for an FD, the old one is removed.
 * mode determines what types of events are watched for, and may be any combination of:
 * OWL_IO_READ, OWL_IO_WRITE, OWL_IO_EXCEPT
 */
const owl_io_dispatch *owl_select_add_io_dispatch(int fd, int mode, void (*cb)(const owl_io_dispatch *, void *), void (*destroy)(const owl_io_dispatch *), void *data)
{
  owl_io_dispatch *d = g_new(owl_io_dispatch, 1);
  owl_list *dl = owl_global_get_io_dispatch_list(&g);

  d->fd = fd;
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


  owl_select_remove_io_dispatch(owl_select_find_io_dispatch_by_fd(fd));
  owl_list_append_element(dl, d);

  return d;
}

int owl_select_add_perl_io_dispatch(int fd, int mode, SV *cb)
{
  const owl_io_dispatch *d = owl_select_find_io_dispatch_by_fd(fd);
  if (d != NULL && d->callback != owl_perlconfig_io_dispatch) {
    /* Don't mess with non-perl dispatch functions from here. */
    return 1;
  }
  owl_select_add_io_dispatch(fd, mode, owl_perlconfig_io_dispatch, owl_perlconfig_io_dispatch_destroy, cb);
  return 0;
}

int owl_select_remove_perl_io_dispatch(int fd)
{
  const owl_io_dispatch *d = owl_select_find_io_dispatch_by_fd(fd);
  if (d != NULL && d->callback == owl_perlconfig_io_dispatch) {
    /* Only remove perl io dispatchers from here. */
    owl_select_remove_io_dispatch(d);
    return 0;
  }
  return 1;
}

void owl_select_init(void)
{
  owl_timer_source = g_source_new(&owl_timer_funcs, sizeof(GSource));
  g_source_attach(owl_timer_source, NULL);

  owl_io_dispatch_source = g_source_new(&owl_io_dispatch_funcs, sizeof(GSource));
  g_source_attach(owl_io_dispatch_source, NULL);
}

void owl_select_run_loop(void)
{
  context = g_main_context_default();
  loop = g_main_loop_new(context, FALSE);
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

void owl_select_post_task(void (*cb)(void*), void *cbdata, void (*destroy_cbdata)(void*))
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
