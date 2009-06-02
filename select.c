#include "owl.h"

static const char fileIdent[] = "$Id: select.c 894 2008-01-17 07:13:44Z asedeno $";

static int dispatch_active = 0;

int _owl_select_timer_cmp(owl_timer *t1, owl_timer *t2) {
  return t1->time - t2->time;
}

int _owl_select_timer_eq(owl_timer *t1, owl_timer *t2) {
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
  owl_list *dl;
  owl_dispatch *d;
  
  dl = owl_global_get_dispatchlist(&g);
  len = owl_list_get_size(dl);
  for(i = 0; i < len; i++) {
    d = (owl_dispatch*)owl_list_get_element(dl, i);
    if (d->fd == fd) return i;
  }
  return -1;
}

void owl_select_remove_dispatch_at(int elt) /* noproto */
{
  owl_list *dl;
  owl_dispatch *d;

  dl = owl_global_get_dispatchlist(&g);
  d = (owl_dispatch*)owl_list_get_element(dl, elt);
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
    d_old = (owl_dispatch*)owl_list_get_element(dl, elt);
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
    d = (owl_dispatch*)owl_list_get_element(dl, elt);
    d->needs_gc = 1;
  } else {
    owl_select_remove_dispatch_at(elt);
  }
}

int owl_select_dispatch_count()
{
  return owl_list_get_size(owl_global_get_dispatchlist(&g));
}

int owl_select_add_perl_dispatch(int fd, SV *cb)
{
  int elt;
  owl_dispatch *d;
  elt = owl_select_find_dispatch(fd);
  if (elt != -1) {
    d = (owl_dispatch*)owl_list_get_element(owl_global_get_dispatchlist(&g), elt);
    if (d->cfunc != owl_perlconfig_dispatch) {
      /* don't mess with non-perl dispatch functions from here. */
      return 1;
    }
  }

  d = malloc(sizeof(owl_dispatch));
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
    d = (owl_dispatch*)owl_list_get_element(owl_global_get_dispatchlist(&g), elt);
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
  owl_list *dl;

  dl = owl_global_get_dispatchlist(&g);
  FD_ZERO(r);
  FD_ZERO(e);
  max_fd = 0;
  len = owl_select_dispatch_count(g);
  for(i = 0; i < len; i++) {
    d = (owl_dispatch*)owl_list_get_element(dl, i);
    FD_SET(d->fd, r);
    FD_SET(d->fd, e);
    if (max_fd < d->fd) max_fd = d->fd;
  }
  return max_fd + 1;
}

void owl_select_gc()
{
  int i;
  owl_list *dl;

  dl = owl_global_get_dispatchlist(&g);
  /*
   * Count down so we aren't set off by removing items from the list
   * during the iteration.
   */
  for(i = owl_list_get_size(dl) - 1; i >= 0; i--) {
    owl_dispatch *d = owl_list_get_element(dl, i);
    if(d->needs_gc) {
      owl_select_remove_dispatch_at(i);
    }
  }
}

void owl_select_dispatch(fd_set *fds, int max_fd)
{
  int i, len;
  owl_dispatch *d;
  owl_list *dl;

  dl = owl_global_get_dispatchlist(&g);
  len = owl_select_dispatch_count();

  dispatch_active = 1;

  for(i = 0; i < len; i++) {
    d = (owl_dispatch*)owl_list_get_element(dl, i);
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

int owl_select_aim_hack(fd_set *rfds, fd_set *wfds)
{
  aim_conn_t *cur;
  aim_session_t *sess;
  int max_fd;

  FD_ZERO(rfds);
  FD_ZERO(wfds);
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

void owl_select_handle_intr()
{
  owl_input in;

  owl_global_unset_interrupted(&g);
  owl_function_unmask_sigint(NULL);

  in.ch = in.uch = owl_global_get_startup_tio(&g)->c_cc[VINTR];
  owl_process_input_char(in);
}

void owl_select()
{
  int i, max_fd, aim_max_fd, aim_done, ret;
  fd_set r;
  fd_set e;
  fd_set aim_rfds, aim_wfds;
  struct timespec timeout;
  sigset_t mask;

  owl_select_process_timers(&timeout);

  owl_function_mask_sigint(&mask);
  if(owl_global_is_interrupted(&g)) {
    owl_select_handle_intr();
    return;
  }

  max_fd = owl_select_dispatch_prepare_fd_sets(&r, &e);

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
    aim_max_fd = owl_select_aim_hack(&aim_rfds, &aim_wfds);
    if (max_fd < aim_max_fd) max_fd = aim_max_fd;
    for(i = 0; i <= aim_max_fd; i++) {
      if (FD_ISSET(i, &aim_rfds)) {
        FD_SET(i, &r);
        FD_SET(i, &e);
      }
    }
  }
  /* END AIM HACK */


  ret = pselect(max_fd+1, &r, &aim_wfds, &e, &timeout, &mask);

  if(ret < 0 && errno == EINTR) {
    if(owl_global_is_interrupted(&g)) {
      owl_select_handle_intr();
    }
    return;
  }

  owl_function_unmask_sigint(NULL);

  if(ret > 0) {
    /* Merge fd_sets and clear AIM FDs. */
    for(i = 0; i <= max_fd; i++) {
      /* Merge all interesting FDs into one set, since we have a
         single dispatch per FD. */
      if (FD_ISSET(i, &r) || FD_ISSET(i, &aim_wfds) || FD_ISSET(i, &e)) {
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
  }
}
