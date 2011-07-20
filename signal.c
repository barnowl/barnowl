#include <errno.h>
#include <glib.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_t signal_thread;
static sigset_t signal_set;

static void (*signal_cb)(const siginfo_t*, void*);
static void *signal_cbdata;

static void *signal_thread_func(void *data);

static void dummy_handler(int signum)
{
  /* Do nothing. This should never get called. It'd be nice to report the error
   * or something, but you can't have nice things in a signal handler. */
}

#define OR_DIE(s, syscall)       \
  G_STMT_START {		 \
    if ((syscall) == -1) {	 \
      perror((s));		 \
      exit(1);			 \
    }				 \
  } G_STMT_END

/* Initializes the signal thread to listen for 'signals' on a dedicated
 * thread. 'callback' is called *on the signal thread* when a signal
 * is received.
 *
 * This function /must/ be called before any other threads are
 * created. (Otherwise the signals will not get blocked correctly.) */
void owl_signal_init(const int *signals, int num_signals, void (*callback)(const siginfo_t*, void*), void *data) {
  struct sigaction sig_dummy = { .sa_handler = dummy_handler };
  int ret;
  int i;

  signal_cb = callback;
  signal_cbdata = data;

  /* Stuff the signals into our sigset_t. Also assign all of them to a dummy
   * handler. Otherwise, if their default is SIG_IGN, they will get dropped if
   * delivered while processing. On Solaris, they will not get delivered at
   * all. */
  OR_DIE("sigemptyset", sigemptyset(&signal_set));
  for (i = 0; i < num_signals; i++) {
    OR_DIE("sigaddset", sigaddset(&signal_set, signals[i]));
    OR_DIE("sigaction", sigaction(signals[i], &sig_dummy, NULL));
  }

  /* Block these signals in all threads, so we can get them. */
  if ((ret = pthread_sigmask(SIG_BLOCK, &signal_set, NULL)) != 0) {
    errno = ret;
    perror("pthread_sigmask");
    exit(1);
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
