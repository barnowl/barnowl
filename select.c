#include "owl.h"

static const char fileIdent[] = "$Id: select.c 894 2008-01-17 07:13:44Z asedeno $";

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

/* Adds a new owl_dispatch to the list, replacing existing ones if needed. */
void owl_select_add_dispatch(owl_dispatch *d)
{
  int elt;
  owl_list *dl;

  elt = owl_select_find_dispatch(d->fd);
  dl = owl_global_get_dispatchlist(&g);
  
  if (elt != -1) {  /* If we have a dispatch for this FD */
    owl_dispatch *d_old;
    d_old = (owl_dispatch*)owl_list_get_element(dl, elt);
    /* Ignore if we're adding the same dispatch again.  Otherwise
       replace the old dispatch. */
    if (d_old != d) {
      owl_list_replace_element(dl, elt, d);
      owl_free(d_old);
    }
  }
  else {
    owl_list_append_element(dl, d);
  }
}

/* Removes an owl_dispatch to the list, based on it's file descriptor. */
void owl_select_remove_dispatch(int fd)
{
  int elt;
  owl_list *dl;

  elt = owl_select_find_dispatch(fd);
  dl = owl_global_get_dispatchlist(&g);
  
  if (elt != -1) {
    owl_dispatch *d;
    d = (owl_dispatch*)owl_list_get_element(dl, elt);
    owl_list_remove_element(dl, elt);
    if (d->pfunc) {
      owl_perlconfig_dispatch_free(d);
    }
    owl_free(d);
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
    if (d->pfunc == NULL) {
      /* don't mess with non-perl dispatch functions from here. */
      return 1;
    }
  }

  d = malloc(sizeof(owl_dispatch));
  d->fd = fd;
  d->cfunc = NULL;
  d->pfunc = cb;
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
    if (d->pfunc != NULL) {
      owl_select_remove_dispatch(fd);
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

void owl_select_dispatch(fd_set *fds, int max_fd)
{
  int i, len;
  owl_dispatch *d;
  owl_list *dl;

  dl = owl_global_get_dispatchlist(&g);
  len = owl_select_dispatch_count();
  for(i = 0; i < len; i++) {
    d = (owl_dispatch*)owl_list_get_element(dl, i);
    /* While d shouldn't normally be null, the list may be altered by
     * functions we dispatch to. */
    if (d != NULL && FD_ISSET(d->fd, fds)) {
      if (d->cfunc != NULL) {
        (d->cfunc)();
      }
      else if (d->pfunc != NULL) {
        owl_perlconfig_do_dispatch(d);
      }
    }
  }
}

void owl_select()
{
  int max_fd;
  fd_set r;
  fd_set e;
  struct timeval timeout;

  timeout.tv_sec = 1;
  timeout.tv_usec = 0;

  max_fd = owl_select_dispatch_prepare_fd_sets(&r, &e);

  if ( select(max_fd+1, &r, NULL, &e, &timeout) ) {
    /* NOTE: the same dispatch function is called for both exceptional
       and read ready FDs. */
    owl_select_dispatch(&r, max_fd);
  }
}
