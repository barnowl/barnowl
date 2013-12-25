#include "owl.h"

static GMainLoop *loop = NULL;

void owl_select_init(void)
{
}

void owl_select_run_loop(void)
{
  loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(loop);
}

void owl_select_quit_loop(void)
{
  if (loop) {
    g_main_loop_quit(loop);
    g_main_loop_unref(loop);
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
  g_slice_free(owl_task, t);
}

void owl_select_post_task(void (*cb)(void*), void *cbdata, void (*destroy_cbdata)(void*), GMainContext *context)
{
  GSource *source = g_idle_source_new();
  owl_task *t = g_slice_new0(owl_task);
  t->cb = cb;
  t->cbdata = cbdata;
  t->destroy_cbdata = destroy_cbdata;
  g_source_set_priority(source, G_PRIORITY_DEFAULT);
  g_source_set_callback(source, _run_task, t, _destroy_task);
  g_source_attach(source, context);
  g_source_unref(source);
}
