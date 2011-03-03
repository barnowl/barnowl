#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_t signal_thread;
static sigset_t signal_set;

static void (*signal_cb)(const siginfo_t*, void*);
static void *signal_cbdata;

static void *signal_thread_func(void *data);

/* Initializes the signal thread to listen for 'set' on a dedicated
 * thread. 'callback' is called *on the signal thread* when a signal
 * is received.
 *
 * This function /must/ be called before any other threads are
 * created. (Otherwise the signals will not get blocked correctly.) */
void owl_signal_init(const sigset_t *set, void (*callback)(const siginfo_t*, void*), void *data) {
  int ret;

  signal_set = *set;
  signal_cb = callback;
  signal_cbdata = data;
  /* Block these signals in all threads, so we can get them. */
  if ((ret = pthread_sigmask(SIG_BLOCK, set, NULL)) != 0) {
    errno = ret;
    perror("pthread_sigmask");
  }
  /* Spawn a dedicated thread to sigwait. */
  if ((ret = pthread_create(&signal_thread, NULL,
                            signal_thread_func, NULL)) != 0) {
    errno = ret;
    perror("pthread_create");
    exit(1);
  }
}

static void *signal_thread_func(void *data) {
  while (1) {
    siginfo_t siginfo;
    int ret;

    ret = sigwaitinfo(&signal_set, &siginfo);
    /* TODO: Print an error? */
    if (ret < 0)
      continue;

    signal_cb(&siginfo, signal_cbdata);
    /* Die on SIGTERM. */
    if (siginfo.si_signo == SIGTERM)
      break;
  }
  return NULL;
}

void owl_signal_shutdown(void) {
  pthread_kill(signal_thread, SIGTERM);
  pthread_join(signal_thread, NULL);
}
