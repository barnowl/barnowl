#include "owl.h"
#include <stdlib.h>

#define INITSIZE 10
#define GROWBY 3 / 2

void owl_list_create(owl_list *l)
{
  l->size=0;
  l->list=g_new(void *, INITSIZE);
  l->avail=INITSIZE;
}

int owl_list_get_size(const owl_list *l)
{
  return(l->size);
}


static void owl_list_grow(owl_list *l, int n)
{
  void *ptr;

  if ((l->size+n) > l->avail) {
    int avail = MAX(l->avail * GROWBY, l->size + n);
    ptr = g_renew(void *, l->list, avail);
    if (ptr==NULL) abort();
    l->list=ptr;
    l->avail = avail;
  }

}

void *owl_list_get_element(const owl_list *l, int n)
{
  if (n>l->size-1) return(NULL);
  return(l->list[n]);
}

int owl_list_insert_element(owl_list *l, int at, void *element)
{
  int i;
  if(at < 0 || at > l->size) return -1;
  owl_list_grow(l, 1);

  for (i=l->size; i>at; i--) {
    l->list[i]=l->list[i-1];
  }

  l->list[at] = element;
  l->size++;
  return(0);
}

int owl_list_append_element(owl_list *l, void *element)
{
  return owl_list_insert_element(l, l->size, element);
}

int owl_list_prepend_element(owl_list *l, void *element)
{
  return owl_list_insert_element(l, 0, element);
}

int owl_list_remove_element(owl_list *l, int n)
{
  int i;

  if (n>l->size-1) return(-1);
  for (i=n; i<l->size-1; i++) {
    l->list[i]=l->list[i+1];
  }
  l->size--;
  return(0);
}

void owl_list_cleanup(owl_list *l, void (*elefree)(void *))
{
  int i;

  if (elefree) {
    for (i = 0; i < l->size; i++) {
      (elefree)(l->list[i]);
    }
  }
  g_free(l->list);
}
