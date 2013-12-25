#include "owl.h"

CALLER_OWN owl_messagelist *owl_messagelist_new(void)
{
  owl_messagelist *ml = g_slice_new(owl_messagelist);
  ml->list = g_ptr_array_new();
  return ml;
}

void owl_messagelist_delete(owl_messagelist *ml, bool free_messages)
{
  if (free_messages)
    g_ptr_array_foreach(ml->list, (GFunc)owl_message_delete, NULL);
  g_ptr_array_free(ml->list, true);
  g_slice_free(owl_messagelist, ml);
}

int owl_messagelist_get_size(const owl_messagelist *ml)
{
  return ml->list->len;
}

void *owl_messagelist_get_element(const owl_messagelist *ml, int n)
{
  /* we assume things like owl_view_get_element(v, owl_global_get_curmsg(&g))
   * work even when there are no messages in the message list.  So don't
   * segfault if someone asks for the zeroth element of an empty list.
   */
  if (n >= ml->list->len) return NULL;
  return ml->list->pdata[n];
}

int owl_messagelist_get_index_by_id(const owl_messagelist *ml, int target_id)
{
  /* return the message index with id == 'id'.  If it doesn't exist return -1. */
  int first, last, mid, msg_id;
  owl_message *m;

  first = 0;
  last = ml->list->len - 1;
  while (first <= last) {
    mid = (first + last) / 2;
    m = ml->list->pdata[mid];
    msg_id = owl_message_get_id(m);
    if (msg_id == target_id) {
      return mid;
    } else if (msg_id < target_id) {
      first = mid + 1;
    } else {
      last = mid - 1;
    }
  }
  return -1;
}

owl_message *owl_messagelist_get_by_id(const owl_messagelist *ml, int target_id)
{
  /* return the message with id == 'id'.  If it doesn't exist return NULL. */
  int n = owl_messagelist_get_index_by_id(ml, target_id);
  if (n < 0) return NULL;
  return ml->list->pdata[n];
}

void owl_messagelist_append_element(owl_messagelist *ml, void *element)
{
  g_ptr_array_add(ml->list, element);
}

/* do we really still want this? */
int owl_messagelist_delete_element(owl_messagelist *ml, int n)
{
  /* mark a message as deleted */
  owl_message_mark_delete(ml->list->pdata[n]);
  return(0);
}

int owl_messagelist_undelete_element(owl_messagelist *ml, int n)
{
  /* mark a message as deleted */
  owl_message_unmark_delete(ml->list->pdata[n]);
  return(0);
}

void owl_messagelist_delete_and_expunge_element(owl_messagelist *ml, int n)
{
  owl_message_delete(g_ptr_array_remove_index(ml->list, n));
}

int owl_messagelist_expunge(owl_messagelist *ml)
{
  /* expunge deleted messages */
  int i;
  GPtrArray *newlist;
  owl_message *m;

  newlist = g_ptr_array_new();
  /*create a new list without messages marked as deleted */
  for (i = 0; i < ml->list->len; i++) {
    m = ml->list->pdata[i];
    if (owl_message_is_delete(m)) {
      owl_message_delete(m);
    } else {
      g_ptr_array_add(newlist, m);
    }
  }

  /* free the old list */
  g_ptr_array_free(ml->list, true);

  /* copy the new list to the old list */
  ml->list = newlist;

  return(0);
}

void owl_messagelist_invalidate_formats(const owl_messagelist *ml)
{
  int i;
  owl_message *m;

  for (i = 0; i < ml->list->len; i++) {
    m = ml->list->pdata[i];
    owl_message_invalidate_format(m);
  }
}
