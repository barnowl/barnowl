/* Dictionary data abstraction.  
 * Maps from strings to pointers.
 * Stores as a sorted list of key/value pairs.
 * O(n) on inserts and deletes.
 * O(n) on searches, although it should switch to a binary search for O(log n)
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";


#define INITSIZE 30
#define GROWAT 2
#define GROWBY 1.5

int owl_dict_create(owl_dict *d) {
  d->size=0;
  d->els=(owl_dict_el *)owl_malloc(INITSIZE*sizeof(owl_dict_el));
  d->avail=INITSIZE;
  if (d->els==NULL) return(-1);
  return(0);
}

int owl_dict_get_size(owl_dict *d) {
  return(d->size);
}

/* Finds the position of an element with key k, or of the element where
 * this element would logically go, and stores the index in pos.
 * TODO: optimize to do a binary search.
 * Returns 1 if found, else 0. */
int _owl_dict_find_pos(owl_dict *d, char *k, int *pos) {
  int i;
  for (i=0; (i<d->size) && strcmp(k,d->els[i].k)>0; i++);
  *pos = i;
  if (i>=d->size || strcmp(k,d->els[i].k)) {
    return(0);
  } else {
    return(1);
  }
}

/* returns the value corresponding to key k */
void *owl_dict_find_element(owl_dict *d, char *k) {
  int found, pos;
  found = _owl_dict_find_pos(d, k, &pos);
  if (!found) {
    return(NULL);
  }
  return(d->els[pos].v);
}

/* creates a list and fills it in with keys.  duplicates the keys, 
 * so they will need to be freed by the caller. */
int owl_dict_get_keys(owl_dict *d, owl_list *l) {
  int i;
  char *dupk;
  if (owl_list_create(l)) return(-1);
  for (i=0; i<d->size; i++) {
    if ((dupk = owl_strdup(d->els[i].k)) == NULL) return(-1);
    owl_list_append_element(l, (void*)dupk);
  }
  return(0);
}

void owl_dict_noop_free(void *x) {
  return;
}

/* Returns 0 on success.  Will copy the key but make 
   a reference to the value.  Will clobber an existing 
   entry with the same key iff free_on_replace!=NULL,
   and will run free_on_replace on the old element.
   Will return -2 if replace=NULL and match was found.
*/
int owl_dict_insert_element(owl_dict *d, char *k, void *v, void (*free_on_replace)(void *old)) {
  int pos, found;
  char *dupk;
  found = _owl_dict_find_pos(d, k, &pos);
  if (found && free_on_replace) {
    free_on_replace(d->els[pos].v);
    d->els[pos].v = v;
    return(0);
  } else if (found && !free_on_replace) {
    return(-2);
  } else {
    if ((d->size+1) > (d->avail/GROWAT)) {
      d->els=owl_realloc(d->els, d->avail*GROWBY*sizeof(void *));
      d->avail=d->avail*GROWBY;
      if (d->els==NULL) return(-1);
    }
    if ((dupk = owl_strdup(k)) == NULL) return(-1);
    if (pos!=d->size) {
      /* shift forward to leave us a slot */
      memmove((void*)(d->els+pos+1), (void*)(d->els+pos), 
	      sizeof(owl_dict_el)*(d->size-pos));
    }
    d->size++;
    d->els[pos].k = dupk;
    d->els[pos].v = v;    
    return(0);
  }
}

/* Doesn't free the value of the element, but does
 * return it so the caller can free it. */
void *owl_dict_remove_element(owl_dict *d, char *k) {
  int i;
  int pos, found;
  void *v;
  found = _owl_dict_find_pos(d, k, &pos);
  if (!found) return(NULL);
  owl_free(d->els[pos].k);
  v = d->els[pos].v;
  for (i=pos; i<d->size-1; i++) {
    d->els[i]=d->els[i+1];
  }
  d->size--;
  return(v);
}

/* elefree should free the value as well */
void owl_dict_free_all(owl_dict *d, void (*elefree)(void *)) {
  int i;

  for (i=0; i<d->size; i++) {
    owl_free(d->els[i].k);
    if (elefree) (elefree)(d->els[i].v);
  }
  if (d->els) owl_free(d->els);
}

void owl_dict_free_simple(owl_dict *d) {
  owl_dict_free_all(d, NULL);
}



/************* REGRESSION TESTS **************/
#ifdef OWL_INCLUDE_REG_TESTS

#include "test.h"

int owl_dict_regtest(void) {
  owl_dict d;
  owl_list l;
  int numfailed=0;
  char *av="aval", *bv="bval", *cv="cval", *dv="dval";

  printf("# BEGIN testing owl_dict\n");
  FAIL_UNLESS("create", 0==owl_dict_create(&d));
  FAIL_UNLESS("insert b", 0==owl_dict_insert_element(&d, "b", (void*)bv, owl_dict_noop_free));
  FAIL_UNLESS("insert d", 0==owl_dict_insert_element(&d, "d", (void*)dv, owl_dict_noop_free));
  FAIL_UNLESS("insert a", 0==owl_dict_insert_element(&d, "a", (void*)av, owl_dict_noop_free));
  FAIL_UNLESS("insert c", 0==owl_dict_insert_element(&d, "c", (void*)cv, owl_dict_noop_free));
  FAIL_UNLESS("reinsert d (no replace)", -2==owl_dict_insert_element(&d, "d", (void*)dv, 0));
  FAIL_UNLESS("find a", (void*)av==owl_dict_find_element(&d, "a"));
  FAIL_UNLESS("find b", (void*)bv==owl_dict_find_element(&d, "b"));
  FAIL_UNLESS("find c", (void*)cv==owl_dict_find_element(&d, "c"));
  FAIL_UNLESS("find d", (void*)dv==owl_dict_find_element(&d, "d"));
  FAIL_UNLESS("find e (non-existent)", NULL==owl_dict_find_element(&d, "e"));
  FAIL_UNLESS("remove d", (void*)dv==owl_dict_remove_element(&d, "d"));
  FAIL_UNLESS("find d (post-removal)", NULL==owl_dict_find_element(&d, "d"));

  FAIL_UNLESS("get_size", 3==owl_dict_get_size(&d));
  FAIL_UNLESS("get_keys", 0==owl_dict_get_keys(&d, &l));
  FAIL_UNLESS("get_keys result size", 3==owl_list_get_size(&l));
  
  /* these assume the returned keys are sorted */
  FAIL_UNLESS("get_keys result val",0==strcmp("a",owl_list_get_element(&l,0)));
  FAIL_UNLESS("get_keys result val",0==strcmp("b",owl_list_get_element(&l,1)));
  FAIL_UNLESS("get_keys result val",0==strcmp("c",owl_list_get_element(&l,2)));

  owl_list_free_all(&l, owl_free);
  owl_dict_free_all(&d, NULL);

  /*  if (numfailed) printf("*** WARNING: failures encountered with owl_dict\n"); */
  printf("# END testing owl_dict (%d failures)\n", numfailed);
  return(numfailed);
}

#endif /* OWL_INCLUDE_REG_TESTS */
