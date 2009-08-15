#include "owl.h"

void owl_errqueue_init(owl_errqueue *eq)
{
  owl_list_create(&(eq->errlist));
}

void owl_errqueue_append_err(owl_errqueue *eq, const char *msg)
{
  owl_list_append_element(&(eq->errlist), owl_strdup(msg));
}

/* fmtext should already be initialized */
void owl_errqueue_to_fmtext(const owl_errqueue *eq, owl_fmtext *fm)
{
  int i, j;

  j=owl_list_get_size(&(eq->errlist));
  for (i=0; i<j; i++) {
    owl_fmtext_append_normal(fm, owl_list_get_element(&(eq->errlist), i));
    owl_fmtext_append_normal(fm, "\n");
  }
}
