#include "owl.h"

void owl_errqueue_init(owl_errqueue *eq)
{
  eq->errlist = g_ptr_array_new();
}

void owl_errqueue_append_err(owl_errqueue *eq, const char *msg)
{
  g_ptr_array_add(eq->errlist, g_strdup(msg));
}

/* fmtext should already be initialized */
void owl_errqueue_to_fmtext(const owl_errqueue *eq, owl_fmtext *fm)
{
  int i;
  for (i = 0; i < eq->errlist->len; i++) {
    owl_fmtext_append_normal(fm, eq->errlist->pdata[i]);
    owl_fmtext_append_normal(fm, "\n");
  }
}
