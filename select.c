#include "owl.h"

static int dispatch_active = 0;
static int psa_active = 0;

int _owl_select_timer_cmp(const owl_timer *t1, const owl_timer *t2) {
  return t1->time - t2->time;
}

int _owl_select_timer_eq(const owl_timer *t1, const owl_timer *t2) {
  return t1 == t2;
}

owl_timer *owl_select_add_timer(int after, int interval, void (*cb)(owl_timer *, void *), void (*destroy)(owl_timer*), void *data)
{
  owl_timer *t = owl_malloc(sizeof(owl_timer));
  GList **timers = owl_global_get_timerlist(&g);

  t->time = time(NULL) + after;
  t->interval = interval;
  t->callback = cb;
  t->destroy = destroy;
  t->data = data;

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
    owl_free(t);
  }
}

void owl_select_process_timers(struct timespec *timeout)
{
  time_t now = time(NULL);
  GList **timers = owl_global_get_timerlist(&g);

  while(*timers) {
    owl_timer *t = (*timers)->data;
    int remove = 0;

    if(t->time > now)
      break;

    /* Reschedule if appropriate */
    if(t->interval > 0) {
      t->time = now + t->interval;
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

  if(*timers) {
    owl_timer *t = (*timers)->data;
    timeout->tv_sec = t->time - now;
    if (timeout->tv_sec > 60)
      timeout->tv_sec = 60;
  } else {
    timeout->tv_sec = 60;
  }

  timeout->tv_nsec = 0;
}

/* Returns the index of the dispatch for the file descriptor. */
int owl_select_find_dispatch(int fd)
{
  int i, len;
  const owl_list *dl;
  const owl_dispatch *d;
  
  dl = owl_global_get_dispatchlist(&g);
  len = owl_list_get_size(dl);
  for(i = 0; i < len; i++) {
    d = owl_list_get_element(dl, i);
    if (d->fd == fd) return i;
  }
  return -1;
}

void owl_select_remove_dispatch_at(int elt) /* noproto */
{
  owl_list *dl;
  owl_dispatch *d;

  dl = owl_global_get_dispatchlist(&g);
  d = owl_list_get_element(dl, elt);
  owl_list_remove_element(dl, elt);
  if (d->destroy) {
    d->destroy(d);
  }
}

/* Adds a new owl_dispatch to the list, replacing existing ones if needed. */
void owl_select_add_dispatch(owl_dispatch *d)
{
  int elt;
  owl_list *dl;

  d->needs_gc = 0;

  elt = owl_select_find_dispatch(d->fd);
  dl = owl_global_get_dispatchlist(&g);
  
  if (elt != -1) {  /* If we have a dispatch for this FD */
    owl_dispatch *d_old;
    d_old = owl_list_get_element(dl, elt);
    /* Ignore if we're adding the same dispatch again.  Otherwise
       replace the old dispatch. */
    if (d_old != d) {
      owl_select_remove_dispatch_at(elt);
    }
  }
  owl_list_append_element(dl, d);
}

/* Removes an owl_dispatch to the list, based on it's file descriptor. */
void owl_select_remove_dispatch(int fd)
{
  int elt;
  owl_list *dl;
  owl_dispatch *d;

  elt = owl_select_find_dispatch(fd);
  if(elt == -1) {
    return;
  } else if(dispatch_active) {
    /* Defer the removal until dispatch is done walking the list */
    dl = owl_global_get_dispatchlist(&g);
    d = owl_list_get_element(dl, elt);
    d->needs_gc = 1;
  } else {
    owl_select_remove_dispatch_at(elt);
  }
}

int owl_select_dispatch_count(void)
{
  return owl_list_get_size(owl_global_get_dispatchlist(&g));
}

int owl_select_add_perl_dispatch(int fd, SV *cb)
{
  int elt;
  owl_dispatch *d;
  elt = owl_select_find_dispatch(fd);
  if (elt != -1) {
    d = owl_list_get_element(owl_global_get_dispatchlist(&g), elt);
    if (d->cfunc != owl_perlconfig_dispatch) {
      /* don't mess with non-perl dispatch functions from here. */
      return 1;
    }
  }

  d = owl_malloc(sizeof(owl_dispatch));
  d->fd = fd;
  d->cfunc = owl_perlconfig_dispatch;
  d->destroy = owl_perlconfig_dispatch_free;
  d->data = cb;
  owl_select_add_dispatch(d);
  return 0;
}

int owl_select_remove_perl_dispatch(int fd)
{
  int elt;
  owl_dispatch *d;
  
  elt = owl_select_find_dispatch(fd);
  if (elt != -1) {
    d = owl_list_get_element(owl_global_get_dispatchlist(&g), elt);
    if (d->cfunc == owl_perlconfig_dispatch) {
      owl_select_remove_dispatch_at(elt);
      return 0;
    }
  }
  return 1;
}

int owl_select_dispatch_prepare_fd_sets(fd_set *r, fd_set *e)
{
  int i, len, max_fd;
  owl_dispatch *d;
  const owl_list *dl;

  dl = owl_global_get_dispatchlist(&g);
  max_fd = 0;
  len = owl_select_dispatch_count();
  for(i = 0; i < len; i++) {
    d = owl_list_get_element(dl, i);
    FD_SET(d->fd, r);
    FD_SET(d->fd, e);
    if (max_fd < d->fd) max_fd = d->fd;
  }
  return max_fd + 1;
}

void owl_select_gc(void)
{
  int i;
  owl_list *dl;

  dl = owl_global_get_dispatchlist(&g);
  /*
   * Count down so we aren't set off by removing items from the list
   * during the iteration.
   */
  for(i = owl_list_get_size(dl) - 1; i >= 0; i--) {
    const owl_dispatch *d = owl_list_get_element(dl, i);
    if(d->needs_gc) {
      owl_select_remove_dispatch_at(i);
    }
  }
}

void owl_select_dispatch(fd_set *fds, int max_fd)
{
  int i, len;
  owl_dispatch *d;
  const owl_list *dl;

  dl = owl_global_get_dispatchlist(&g);
  len = owl_select_dispatch_count();

  dispatch_active = 1;

  for(i = 0; i < len; i++) {
    d = owl_list_get_element(dl, i);
    /* While d shouldn't normally be null, the list may be altered by
     * functions we dispatch to. */
    if (d != NULL && !d->needs_gc && FD_ISSET(d->fd, fds)) {
      if (d->cfunc != NULL) {
        d->cfunc(d);
      }
    }
  }

  dispatch_active = 0;
  owl_select_gc();
}

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
        owl_free(d);
      }
    }
  }
}

void owl_select_io_dispatch_gc(void)
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
  owl_io_dispatch *d = owl_malloc(sizeof(owl_io_dispatch));
  owl_list *dl = owl_global_get_io_dispatch_list(&g);

  d->fd = fd;
  d->needs_gc = 0;
  d->mode = mode;
  d->callback = cb;
  d->destroy = destroy;
  d->data = data;

  owl_select_remove_io_dispatch(owl_select_find_io_dispatch_by_fd(fd));
  owl_list_append_element(dl, d);

  return d;
}

int owl_select_prepare_io_dispatch_fd_sets(fd_set *rfds, fd_set *wfds, fd_set *efds) {
  int i, len, max_fd;
  owl_io_dispatch *d;
  owl_list *dl = owl_global_get_io_dispatch_list(&g);

  max_fd = 0;
  len = owl_list_get_size(dl);
  for (i = 0; i < len; i++) {
    d = owl_list_get_element(dl, i);
    if (d->mode & (OWL_IO_READ | OWL_IO_WRITE | OWL_IO_EXCEPT)) {
      if (max_fd < d->fd) max_fd = d->fd;
      if (d->mode & OWL_IO_READ) FD_SET(d->fd, rfds);
      if (d->mode & OWL_IO_WRITE) FD_SET(d->fd, wfds);
      if (d->mode & OWL_IO_EXCEPT) FD_SET(d->fd, efds);
    }
  }
  return max_fd + 1;
}

void owl_select_io_dispatch(const fd_set *rfds, const fd_set *wfds, const fd_set *efds, const int max_fd)
{
  int i, len;
  owl_io_dispatch *d;
  owl_list *dl = owl_global_get_io_dispatch_list(&g);

  dispatch_active = 1;
  len = owl_list_get_size(dl);
  for (i = 0; i < len; i++) {
    d = owl_list_get_element(dl, i);
    if (d->fd < max_fd && d->callback != NULL &&
        ((d->mode & OWL_IO_READ && FD_ISSET(d->fd, rfds)) ||
         (d->mode & OWL_IO_WRITE && FD_ISSET(d->fd, wfds)) ||
         (d->mode & OWL_IO_EXCEPT && FD_ISSET(d->fd, efds)))) {
      d->callback(d, d->data);
    }
  }
  dispatch_active = 0;
  owl_select_io_dispatch_gc();
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

int owl_select_aim_hack(fd_set *rfds, fd_set *wfds)
{
  aim_conn_t *cur;
  aim_session_t *sess;
  int max_fd;

  max_fd = 0;
  sess = owl_global_get_aimsess(&g);
  for (cur = sess->connlist, max_fd = 0; cur; cur = cur->next) {
    if (cur->fd != -1) {
      FD_SET(cur->fd, rfds);
      if (cur->status & AIM_CONN_STATUS_INPROGRESS) {
        /* Yes, we're checking writable sockets here. Without it, AIM
           login is really slow. */
        FD_SET(cur->fd, wfds);
      }
      
      if (cur->fd > max_fd)
        max_fd = cur->fd;
    }
  }
  return max_fd;
}

void owl_process_input_char(owl_input j)
{
  int ret;
  owl_popwin *pw;
  owl_editwin *tw;

  owl_global_set_lastinputtime(&g, time(NULL));
  pw=owl_global_get_popwin(&g);
  tw=owl_global_get_typwin(&g);

  owl_global_set_lastinputtime(&g, time(NULL));
  /* find and activate the current keymap.
   * TODO: this should really get fixed by activating
   * keymaps as we switch between windows... 
   */
  if (pw && owl_popwin_is_active(pw) && owl_global_get_viewwin(&g)) {
    owl_context_set_popless(owl_global_get_context(&g), 
                            owl_global_get_viewwin(&g));
    owl_function_activate_keymap("popless");
  } else if (owl_global_is_typwin_active(&g) 
             && owl_editwin_get_style(tw)==OWL_EDITWIN_STYLE_ONELINE) {
    /*
      owl_context_set_editline(owl_global_get_context(&g), tw);
      owl_function_activate_keymap("editline");
    */
  } else if (owl_global_is_typwin_active(&g) 
             && owl_editwin_get_style(tw)==OWL_EDITWIN_STYLE_MULTILINE) {
    owl_context_set_editmulti(owl_global_get_context(&g), tw);
    owl_function_activate_keymap("editmulti");
  } else {
    owl_context_set_recv(owl_global_get_context(&g));
    owl_function_activate_keymap("recv");
  }
  /* now actually handle the keypress */
  ret = owl_keyhandler_process(owl_global_get_keyhandler(&g), j);
  if (ret!=0 && ret!=1) {
    owl_function_makemsg("Unable to handle keypress");
  }
}

void owl_select_mask_signals(sigset_t *oldmask) {
  sigset_t set;

  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTSTP);
  sigprocmask(SIG_BLOCK, &set, oldmask);
}

void owl_select_handle_intr(sigset_t *restore)
{
  owl_input in;

  owl_global_unset_interrupted(&g);

  sigprocmask(SIG_SETMASK, restore, NULL);

  in.ch = in.uch = owl_global_get_startup_tio(&g)->c_cc[VINTR];
  owl_process_input_char(in);
}

void owl_select_check_tstp(void) {
  if(owl_global_is_sigstp(&g)) {
    owl_function_makemsg("Use :suspend to suspend.");
    owl_global_unset_got_sigstp(&g);
  }
}

owl_ps_action *owl_select_add_pre_select_action(int (*cb)(owl_ps_action *, void *), void (*destroy)(owl_ps_action *), void *data)
{
  owl_ps_action *a = owl_malloc(sizeof(owl_ps_action));
  owl_list *psa_list = owl_global_get_psa_list(&g);
  a->needs_gc = 0;
  a->callback = cb;
  a->destroy = destroy;
  a->data = data;
  owl_list_append_element(psa_list, a);
  return a;
}

void owl_select_psa_gc(void)
{
  int i;
  owl_list *psa_list;
  owl_ps_action *a;

  psa_list = owl_global_get_psa_list(&g);
  for (i = owl_list_get_size(psa_list) - 1; i >= 0; i--) {
    a = owl_list_get_element(psa_list, i);
    if (a->needs_gc) {
      owl_list_remove_element(psa_list, i);
      if (a->destroy) {
        a->destroy(a);
      }
      owl_free(a);
    }
  }
}

void owl_select_remove_pre_select_action(owl_ps_action *a)
{
  a->needs_gc = 1;
  if (!psa_active)
    owl_select_psa_gc();
}

int owl_select_do_pre_select_actions(void)
{
  int i, len, ret;
  owl_list *psa_list;

  psa_active = 1;
  ret = 0;
  psa_list = owl_global_get_psa_list(&g);
  len = owl_list_get_size(psa_list);
  for (i = 0; i < len; i++) {
    owl_ps_action *a = owl_list_get_element(psa_list, i);
    if (a->callback != NULL && a->callback(a, a->data)) {
      ret = 1;
    }
  }
  psa_active = 0;
  owl_select_psa_gc();
  return ret;
}

void owl_select(void)
{
  int i, max_fd, max_fd2, aim_done, ret;
  fd_set r;
  fd_set w;
  fd_set e;
  fd_set aim_rfds, aim_wfds;
  struct timespec timeout;
  sigset_t mask;

  owl_select_process_timers(&timeout);

  owl_select_mask_signals(&mask);

  owl_select_check_tstp();
  if(owl_global_is_interrupted(&g)) {
    owl_select_handle_intr(&mask);
    return;
  }
  FD_ZERO(&r);
  FD_ZERO(&w);
  FD_ZERO(&e);

  max_fd = owl_select_dispatch_prepare_fd_sets(&r, &e);
  max_fd2 = owl_select_prepare_io_dispatch_fd_sets(&r, &w, &e);
  if (max_fd < max_fd2) max_fd = max_fd2;

  /* AIM HACK: 
   *
   *  The problem - I'm not sure where to hook into the owl/faim
   *  interface to keep track of when the AIM socket(s) open and
   *  close. In particular, the bosconn thing throws me off. So,
   *  rather than register particular dispatchers for AIM, I look up
   *  the relevant FDs and add them to select's watch lists, then
   *  check for them individually before moving on to the other
   *  dispatchers. --asedeno
   */
  aim_done = 1;
  FD_ZERO(&aim_rfds);
  FD_ZERO(&aim_wfds);
  if (owl_global_is_doaimevents(&g)) {
    aim_done = 0;
    max_fd2 = owl_select_aim_hack(&aim_rfds, &aim_wfds);
    if (max_fd < max_fd2) max_fd = max_fd2;
    for(i = 0; i <= max_fd2; i++) {
      if (FD_ISSET(i, &aim_rfds)) {
        FD_SET(i, &r);
        FD_SET(i, &e);
      }
      if (FD_ISSET(i, &aim_wfds)) {
        FD_SET(i, &w);
        FD_SET(i, &e);
      }
    }
  }
  /* END AIM HACK */

  if (owl_select_do_pre_select_actions()) {
    timeout.tv_sec = 0;
    timeout.tv_nsec = 0;
  }

  ret = pselect(max_fd+1, &r, &w, &e, &timeout, &mask);

  if(ret < 0 && errno == EINTR) {
    owl_select_check_tstp();
    if(owl_global_is_interrupted(&g)) {
      owl_select_handle_intr(NULL);
    }
    sigprocmask(SIG_SETMASK, &mask, NULL);
    return;
  }

  sigprocmask(SIG_SETMASK, &mask, NULL);

  if(ret > 0) {
    /* Merge fd_sets and clear AIM FDs. */
    for(i = 0; i <= max_fd; i++) {
      /* Merge all interesting FDs into one set, since we have a
         single dispatch per FD. */
      if (FD_ISSET(i, &r) || FD_ISSET(i, &w) || FD_ISSET(i, &e)) {
        /* AIM HACK: no separate dispatch, just process here if
           needed, and only once per run through. */
        if (!aim_done && (FD_ISSET(i, &aim_rfds) || FD_ISSET(i, &aim_wfds))) {
          owl_process_aim();
          aim_done = 1;
        }
        else {
          FD_SET(i, &r);
        }
      }
    }
    /* NOTE: the same dispatch function is called for both exceptional
       and read ready FDs. */
    owl_select_dispatch(&r, max_fd);
    owl_select_io_dispatch(&r, &w, &e, max_fd);
  }
}
