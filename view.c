#include "owl.h"

void owl_view_create(owl_view *v, owl_filter *f) {
  v->filter=f;
  owl_messagelist_create(&(v->ml));
  owl_view_recalculate(v);
}

void owl_view_consider_message(owl_view *v, owl_message *m) {
  /* if the message matches the filter then add to view */
  if (owl_filter_message_match(v->filter, m)) {
    owl_messagelist_append_element(&(v->ml), m);
  }
}

void owl_view_recalculate(owl_view *v) {
  /* add all the global messages that match the filter */
  int i, j;
  owl_messagelist *gml;
  owl_messagelist *ml;
  owl_message *m;

  gml=owl_global_get_msglist(&g);
  ml=&(v->ml);

  /* nuke the old list */
  owl_list_free_simple((owl_list *) ml);
  owl_messagelist_create(&(v->ml));

  /* find all the messages we want */
  j=owl_messagelist_get_size(gml);
  for (i=0; i<j; i++) {
    m=owl_messagelist_get_element(gml, i);
    if (owl_filter_message_match(v->filter, m)) {
      owl_messagelist_append_element(ml, m);
    }
  }
}

owl_message *owl_view_get_element(owl_view *v, int index) {
  return(owl_messagelist_get_element(&(v->ml), index));
}

void owl_view_delete_element(owl_view *v, int index) {
  owl_messagelist_delete_element(&(v->ml), index);
}

void owl_view_undelete_element(owl_view *v, int index) {
  owl_messagelist_undelete_element(&(v->ml), index);
}

int owl_view_get_size(owl_view *v) {
  return(owl_messagelist_get_size(&(v->ml)));
}

char *owl_view_get_filtname(owl_view *v) {
  return(owl_filter_get_name(v->filter));
}

void owl_view_free(owl_view *v) {
  owl_list_free_simple((owl_list *) &(v->ml));
}
