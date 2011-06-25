#include "owl.h"

void owl_history_init(owl_history *h)
{
  g_queue_init(&h->hist);
  h->cur = h->hist.tail;	/* current position in history */
  h->partial = false;		/* is the 0th element is partially composed? */
}

const char *owl_history_get_prev(owl_history *h)
{
  if (!h) return NULL;

  if (h->cur == NULL || g_list_previous(h->cur) == NULL) return NULL;

  h->cur = g_list_previous(h->cur);
  return h->cur->data;
}

const char *owl_history_get_next(owl_history *h)
{
  if (!h) return NULL;

  if (h->cur == NULL || g_list_next(h->cur) == NULL) return NULL;

  h->cur = g_list_next(h->cur);
  return h->cur->data;
}

void owl_history_store(owl_history *h, const char *line, bool partial)
{
  if (!h) return;

  owl_history_reset(h);

  /* check if the line is the same as the last */
  if (!partial && !g_queue_is_empty(&h->hist) &&
      strcmp(line, g_queue_peek_tail(&h->hist)) == 0)
    return;

  /* if we've reached the max history size, pop off the last element */
  if (g_queue_get_length(&h->hist) > OWL_HISTORYSIZE)
    g_free(g_queue_pop_head(&h->hist));

  /* add the new line */
  g_queue_push_tail(&h->hist, g_strdup(line));
  h->partial = partial;
  h->cur = h->hist.tail;
}

void owl_history_reset(owl_history *h)
{
  if (!h) return;

  /* if partial is set, remove the first entry first */
  if (h->partial) {
    g_free(g_queue_pop_tail(&h->hist));
    h->partial = false;
  }

  h->cur = h->hist.tail;
}

int owl_history_is_touched(const owl_history *h)
{
  if (!h) return(0);
  return h->cur != NULL && g_list_next(h->cur) != NULL;
}
