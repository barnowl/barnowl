#include "owl.h"

static const char fileIdent[] = "$Id$";

void owl_history_init(owl_history *h)
{
  owl_list_create(&(h->hist));
  h->cur=0;			/* current position in history */
  h->touched=0;			/* whether we've gone into history */
  h->partial=0;			/* is the 0th element is partially composed? */
}

char *owl_history_get_prev(owl_history *h)
{

  if (!h) return NULL;
  h->touched=1;

  if (owl_list_get_size(&(h->hist))==0) return(NULL);

  if (h->cur == owl_list_get_size(&(h->hist))-1) {
    return(NULL);
  }

  h->cur++;

  return(owl_list_get_element(&(h->hist), h->cur));
}

char *owl_history_get_next(owl_history *h)
{
  if (!h) return NULL;
  if (owl_list_get_size(&(h->hist))==0) return(NULL);
  if (h->cur==0) {
    return(NULL);
  }

  h->cur--;
  return(owl_list_get_element(&(h->hist), h->cur));
}

void owl_history_store(owl_history *h, char *line)
{
  int size;

  if (!h) return;

  /* if partial is set, remove the first entry first */
  if (h->partial) {
    owl_list_remove_element(&(h->hist), 0);
  }

  /* if we've reached the max history size, pop off the last element */
  size=owl_list_get_size(&(h->hist));
  if (size>OWL_HISTORYSIZE) {
    owl_free(owl_list_get_element(&(h->hist), size-1));
    owl_list_remove_element(&(h->hist), size-1);
  }

  /* add the new line */
  owl_list_prepend_element(&(h->hist), owl_strdup(line));
}

void owl_history_set_partial(owl_history *h)
{
  if (!h) return;
  h->partial=1;
}

void owl_history_reset(owl_history *h)
{
  if (!h) return;
  h->cur=0;
  h->touched=0;
  h->partial=0;
}

int owl_history_is_touched(owl_history *h)
{
  if (!h) return(0);
  if (h->touched) return(1);
  return(0);
}
