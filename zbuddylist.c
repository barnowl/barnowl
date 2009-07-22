#include "owl.h"

void owl_zbuddylist_create(owl_zbuddylist *zb)
{
  owl_list_create(&(zb->zusers));
}

int owl_zbuddylist_adduser(owl_zbuddylist *zb, const char *name)
{
  int i, j;
  char *user;

  user=long_zuser(name);

  j=owl_list_get_size(&(zb->zusers));
  for (i=0; i<j; i++) {
    if (!strcasecmp(user, owl_list_get_element(&(zb->zusers), i))) {
      owl_free(user);
      return(-1);
    }
  }
  owl_list_append_element(&(zb->zusers), user);
  return(0);
}

int owl_zbuddylist_deluser(owl_zbuddylist *zb, const char *name)
{
  int i, j;
  char *user, *ptr;

  user=long_zuser(name);

  j=owl_list_get_size(&(zb->zusers));
  for (i=0; i<j; i++) {
    ptr=owl_list_get_element(&(zb->zusers), i);
    if (!strcasecmp(user, ptr)) {
      owl_list_remove_element(&(zb->zusers), i);
      owl_free(ptr);
      owl_free(user);
      return(0);
    }
  }
  owl_free(user);
  return(-1);
}

int owl_zbuddylist_contains_user(const owl_zbuddylist *zb, const char *name)
{
  int i, j;
  char *user;

  user=long_zuser(name);

  j=owl_list_get_size(&(zb->zusers));
  for (i=0; i<j; i++) {
    if (!strcasecmp(user, owl_list_get_element(&(zb->zusers), i))) {
      owl_free(user);
      return(1);
    }
  }
  owl_free(user);
  return(0);
}
