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

/* Initializes the signal thread to listen for 'set' on a dedicated
 * thread. 'callback' is called *on the signal thread* when a signal
 * is received.
 *
 * This function /must/ be called before any other threads are
 * created. (Otherwise the signals will not get blocked correctly.) */
void owl_signal_init(const sigset_t *set, void (*callback)(int, void*), void *data) {
  GError *error = NULL;

  signal_set = *set;
  signal_cb = callback;
  signal_cbdata = data;
  /* Block these signals in all threads, so we can get them. */
  pthread_sigmask(SIG_BLOCK, set, NULL);
  /* Spawn a dedicated thread to sigwait. */
  signal_thread = g_thread_create(signal_thread_func, NULL, FALSE, &error);
  if (signal_thread == NULL) {
    fprintf(stderr, "Failed to create signal thread: %s\n", error->message);
    exit(1);
  }
}

static gpointer signal_thread_func(gpointer data) {
  while (1) {
     int signal;
    int ret;

    ret = sigwait(&signal_set, &signal);
    /* TODO: Print an error? man page claims it never errors. */
    if (ret != 0)
      continue;

    signal_cb(signal, signal_cbdata);
  }
  return NULL;
}
