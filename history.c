#include "owl.h"

void owl_history_init(owl_history *h)
{
  owl_list_create(&(h->hist));
  h->cur=0;			/* current position in history */
  h->partial = false;		/* is the 0th element is partially composed? */
}

const char *owl_history_get_prev(owl_history *h)
{

  if (!h) return NULL;

  if (owl_list_get_size(&(h->hist))==0) return(NULL);

  if (h->cur == owl_list_get_size(&(h->hist))-1) {
    return(NULL);
  }

  h->cur++;

  return(owl_list_get_element(&(h->hist), h->cur));
}

const char *owl_history_get_next(owl_history *h)
{
  if (!h) return NULL;
  if (owl_list_get_size(&(h->hist))==0) return(NULL);
  if (h->cur==0) {
    return(NULL);
  }

  h->cur--;
  return(owl_list_get_element(&(h->hist), h->cur));
}

void owl_history_store(owl_history *h, const char *line, bool partial)
{
  int size;

  if (!h) return;
  size=owl_list_get_size(&(h->hist));

  owl_history_reset(h);

  /* check if the line is the same as the last */
  if (owl_list_get_size(&(h->hist))>0) {
    if (!strcmp(line, owl_list_get_element(&(h->hist), 0))) return;
  }

  /* if we've reached the max history size, pop off the last element */
  if (size>OWL_HISTORYSIZE) {
    g_free(owl_list_get_element(&(h->hist), size-1));
    owl_list_remove_element(&(h->hist), size-1);
  }

  /* add the new line */
  owl_list_prepend_element(&(h->hist), g_strdup(line));
  h->partial = partial;
}

void owl_history_reset(owl_history *h)
{
  if (!h) return;

  /* if partial is set, remove the first entry first */
  if (h->partial) {
    g_free(owl_list_get_element(&(h->hist), 0));
    owl_list_remove_element(&(h->hist), 0);
    h->partial = false;
  }

  h->cur=0;
}

int owl_history_is_touched(const owl_history *h)
{
  if (!h) return(0);
  return h->cur > 0;
}
