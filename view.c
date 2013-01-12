#include "owl.h"

void owl_view_create(owl_view *v, const char *name, owl_filter *f, const owl_style *s)
{
  v->name=g_strdup(name);
  v->filter=f;
  v->style=s;
  v->ml = owl_messagelist_new();
  owl_view_recalculate(v);
}

const char *owl_view_get_name(const owl_view *v)
{
  return(v->name);
}

/* if the message matches the filter then add to view */
void owl_view_consider_message(owl_view *v, owl_message *m)
{
  if (owl_filter_message_match(v->filter, m)) {
    owl_messagelist_append_element(v->ml, m);
  }
}

/* remove all messages, add all the global messages that match the
 * filter.
 */
void owl_view_recalculate(owl_view *v)
{
  int i, j;
  const owl_messagelist *gml;
  owl_message *m;

  gml=owl_global_get_msglist(&g);

  /* nuke the old list, don't free the messages */
  owl_messagelist_delete(v->ml, false);
  v->ml = owl_messagelist_new();

  /* find all the messages we want */
  j=owl_messagelist_get_size(gml);
  for (i=0; i<j; i++) {
    m=owl_messagelist_get_element(gml, i);
    if (owl_filter_message_match(v->filter, m)) {
      owl_messagelist_append_element(v->ml, m);
    }
  }
}

void owl_view_new_filter(owl_view *v, owl_filter *f)
{
  v->filter=f;
  owl_view_recalculate(v);
}

void owl_view_set_style(owl_view *v, const owl_style *s)
{
  v->style=s;
}

const owl_style *owl_view_get_style(const owl_view *v)
{
  return(v->style);
}

const char *owl_view_get_style_name(const owl_view *v) {
  return(owl_style_get_name(v->style));
}

owl_message *owl_view_get_element(const owl_view *v, int index)
{
  return owl_messagelist_get_element(v->ml, index);
}

void owl_view_delete_element(owl_view *v, int index)
{
  owl_messagelist_delete_element(v->ml, index);
}

void owl_view_undelete_element(owl_view *v, int index)
{
  owl_messagelist_undelete_element(v->ml, index);
}

int owl_view_get_size(const owl_view *v)
{
  return owl_messagelist_get_size(v->ml);
}

/* Returns the position in the view with a message closest
 * to the passed msgid. */
int owl_view_get_nearest_to_msgid(const owl_view *v, int targetid)
{
  int first, last, mid = 0, max, bestdist, curid = 0;

  first = 0;
  last = max = owl_view_get_size(v) - 1;
  while (first <= last) {
    mid = (first + last) / 2;
    curid = owl_message_get_id(owl_view_get_element(v, mid));
    if (curid == targetid) {
      return(mid);
    } else if (curid < targetid) {
      first = mid + 1;
    } else {
      last = mid - 1;
    }
  }
  bestdist = abs(targetid-curid);
  if (curid < targetid && mid+1 < max) {
    curid = owl_message_get_id(owl_view_get_element(v, mid+1));
    mid = (bestdist < abs(targetid-curid)) ? mid : mid+1;
  }
  else if (curid > targetid && mid-1 >= 0) {
    curid = owl_message_get_id(owl_view_get_element(v, mid-1));
    mid = (bestdist < abs(targetid-curid)) ? mid : mid-1;
  }
  return mid;
}

int owl_view_get_nearest_to_saved(const owl_view *v)
{
  int cachedid;

  cachedid = v->cachedmsgid;
  if (cachedid<0) return(0);
  return (owl_view_get_nearest_to_msgid(v, cachedid));
}

void owl_view_save_curmsgid(owl_view *v, int curid)
{
  v->cachedmsgid = curid;
}

/* fmtext should already be initialized */
void owl_view_to_fmtext(const owl_view *v, owl_fmtext *fm)
{
  owl_fmtext_append_normal(fm, "Name: ");
  owl_fmtext_append_normal(fm, v->name);
  owl_fmtext_append_normal(fm, "\n");

  owl_fmtext_append_normal(fm, "Filter: ");
  owl_fmtext_append_normal(fm, owl_filter_get_name(v->filter));
  owl_fmtext_append_normal(fm, "\n");

  owl_fmtext_append_normal(fm, "Style: ");
  owl_fmtext_append_normal(fm, owl_style_get_name(v->style));
  owl_fmtext_append_normal(fm, "\n");
}

const char *owl_view_get_filtname(const owl_view *v)
{
  return(owl_filter_get_name(v->filter));
}

void owl_view_cleanup(owl_view *v)
{
  owl_messagelist_delete(v->ml, false);
  g_free(v->name);
}
