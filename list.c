#include "owl.h"
#include <stdlib.h>

#define INITSIZE 30
#define GROWAT 2
#define GROWBY 1.5

int owl_list_create(owl_list *l) {
  l->size=0;
  l->list=(void **)owl_malloc(INITSIZE*sizeof(void *));
  l->avail=INITSIZE;
  if (l->list==NULL) return(-1);
  return(0);
}

int owl_list_get_size(owl_list *l) {
  return(l->size);
}

void *owl_list_get_element(owl_list *l, int n) {
  if (n>l->size-1) return(NULL);
  return(l->list[n]);
}

int owl_list_append_element(owl_list *l, void *element) {
  if ((l->size+1) > (l->avail/GROWAT)) {
    l->list=owl_realloc(l->list, l->avail*GROWBY*sizeof(void *));
    l->avail=l->avail*GROWBY;
    if (l->list==NULL) return(-1);
  }

  l->list[l->size]=element;
  l->size++;
  return(0);
}

int owl_list_prepend_element(owl_list *l, void *element) {
  int i;
 
  if ((l->size+1) > (l->avail/GROWAT)) {
    l->list=owl_realloc(l->list, l->avail*GROWBY*sizeof(void *));
    l->avail=l->avail*GROWBY;
    if (l->list==NULL) return(-1);
  }

  for (i=l->size; i>0; i--) {
    l->list[i]=l->list[i-1];
  }
  l->list[0]=element;
  l->size++;
  return(0);
}

int owl_list_remove_element(owl_list *l, int n) {
  int i;

  if (n>l->size-1) return(-1);
  for (i=n; i<l->size-1; i++) {
    l->list[i]=l->list[i+1];
  }
  l->size--;
  return(0);
}

/* todo: might leak memory */
int owl_list_replace_element(owl_list *l, int n, void *element) {
  if (n>l->size-1) return(-1);

  l->list[n]=element;
  return(0);
}

void owl_list_free_all(owl_list *l, void (*elefree)(void *)) {
  int i;

  for (i=0; i<l->size; i++) {
    (elefree)(l->list[i]);
  }
  owl_free(l->list);
}

void owl_list_free_simple(owl_list *l) {
  if (l->list) owl_free(l->list);
}
