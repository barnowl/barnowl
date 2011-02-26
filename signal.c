#include <glib.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static GThread *signal_thread;
static sigset_t signal_set;

static void (*signal_cb)(int, void*);
static void *signal_cbdata;

static gpointer signal_thread_func(gpointer data);
static gboolean signal_thunk(gpointer data);

void owl_signal_init(const sigset_t *set, void (*callback)(int, void*), void *data) {
  GError *error = NULL;

  signal_set = *set;
  signal_cb = callback;
  signal_cbdata = data;
  /* Block these signals in all threads, so we can get them. */
  pthread_sigmask(SIG_BLOCK, set, NULL);
  /* Spawn a dedicated thread to sigwait. */
  signal_thread = g_thread_create(signal_thread_func, g_main_context_default(),
				  FALSE, &error);
  if (signal_thread == NULL) {
    fprintf(stderr, "Failed to create signal thread: %s\n", error->message);
    exit(1);
  }
}

static gpointer signal_thread_func(gpointer data) {
  GMainContext *context = data;

  while (1) {
    GSource *source;
    int signal;
    int ret;

    ret = sigwait(&signal_set, &signal);
    /* TODO: Print an error? man page claims it never errors. */
    if (ret != 0)
      continue;

    /* Send a message to the other main. */
    source = g_idle_source_new();
    g_source_set_priority(source, G_PRIORITY_DEFAULT);
    g_source_set_callback(source, signal_thunk, GINT_TO_POINTER(signal), NULL);
    g_source_attach(source, context);
    g_source_unref(source);
  }
  return NULL;
}

static gboolean signal_thunk(gpointer data) {
  signal_cb(GPOINTER_TO_INT(data), signal_cbdata);
  return FALSE;
}
