#include <sys/poll.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "owl.h"

static const char fileIdent[] = "$Id $";

#define OWL_MUX_MAX_FD (FD_SETSIZE-1)

static int owl_mux_nexthandle = 5000; /* next handle to hand out */
static int owl_mux_needgc = 0;	/* number of muxevents needing to be gc'd */

/* returns a handle id or 0 on failure */
int owl_muxevents_add(owl_muxevents *muxevents, int fd, int eventmask, void (*handler_fn)(int handle, int fd, int eventmask, void *data), void *data) {
  owl_mux *mux;
  
  mux = owl_malloc(sizeof(owl_mux));
  if (!mux) return 0;
  if (fd > OWL_MUX_MAX_FD) {
    owl_function_error("owl_muxevents_add: fd %d too large", fd);
    return 0;
  }
  mux->handle = owl_mux_nexthandle++;
  mux->fd = fd;
  mux->active = 1;
  mux->eventmask = eventmask;
  mux->data = data;
  mux->handler_fn = handler_fn;
  if (0!=owl_list_append_element(muxevents, mux)) {
    owl_free(mux);
    return(0);
  }
  return mux->handle;
}

/* deactivates a muxevent entry with the given handle */
void owl_muxevents_remove(owl_muxevents *muxevents, int handle) {
  int max, i;
  owl_mux *m;

  max = owl_list_get_size(muxevents);
  for (i=0; i<max; i++) {
    m = (owl_mux*)owl_list_get_element(muxevents, i);
    if (m->handle == handle) {
      m->active=0;
      owl_mux_needgc++;
    }
  }
}

/* cleans up a muxevents list at a safe time (ie, when it is
   not being traversed). */
void owl_muxevents_gc(owl_muxevents *muxevents) {
  int max, i, done=0;
  owl_mux *m;

  if (!owl_mux_needgc) return;
  while (!done) {
    max = owl_list_get_size(muxevents);
    for (i=0; i<max; i++) {
      m = (owl_mux*)owl_list_get_element(muxevents, i);
      if (!m->active) {
	owl_list_remove_element(muxevents, i);
	owl_free(m);
	owl_mux_needgc--;
	break;
      }
    }  
    if (i==max) done=1;
  }
}

/* dispatches out events */
void owl_muxevents_dispatch(owl_muxevents *muxevents, int timeout_usec) {
  int nevents, i, rv, emask;
  owl_mux *m;
  fd_set rfds, wfds, efds;
  int maxfd=0;
  struct timeval tv;

  FD_ZERO(&rfds);
  FD_ZERO(&wfds);
  FD_ZERO(&efds);
  nevents = owl_list_get_size(muxevents);
  for (i=0; i<nevents; i++) {
      m = (owl_mux*)owl_list_get_element(muxevents, i);
      if (m->eventmask & OWL_MUX_READ)   FD_SET(m->fd, &rfds);
      if (m->eventmask & OWL_MUX_WRITE)  FD_SET(m->fd, &wfds);
      if (m->eventmask & OWL_MUX_EXCEPT) FD_SET(m->fd, &efds);
      if (m->fd > maxfd) maxfd = m->fd;
  }
  tv.tv_sec = 0;
  tv.tv_usec = timeout_usec;
  rv = select(maxfd+1, &rfds, &wfds, &efds, &tv);
  if (rv == 0) return;
  
  for (i=0; i<nevents; i++) {
      m = (owl_mux*)owl_list_get_element(muxevents, i);
      if (!m->active) continue;
      emask = 0;
      if (m->eventmask & OWL_MUX_READ && FD_ISSET(m->fd, &rfds)) {
	emask |= OWL_MUX_READ;
      }
      if (m->eventmask & OWL_MUX_WRITE && FD_ISSET(m->fd, &wfds)) {
	emask |= OWL_MUX_WRITE;
      }
      if (m->eventmask & OWL_MUX_EXCEPT && FD_ISSET(m->fd, &efds)) {
	emask |= OWL_MUX_EXCEPT;
      }
      owl_function_debugmsg("muxevents: dispatching %s%s%s to %d\n",
			    emask&OWL_MUX_READ?"[read]":"",
			    emask&OWL_MUX_WRITE?"[write]":"",
			    emask&OWL_MUX_EXCEPT?"[except]":"",
			    m->fd);
      if (emask) {
	m->handler_fn(m->handle, m->fd, emask, m->data);
      }
  }

  owl_function_debugmsg("muxevents: finishing dispatch\n");
  /* we need to do this here so that the size of muxevents
  * doesn't change while we're traversing it... */
  owl_muxevents_gc(muxevents);
}


