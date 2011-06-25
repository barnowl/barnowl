/* Dictionary data abstraction.  
 * Maps from strings to pointers.
 * Stores as a sorted list of key/value pairs.
 * O(n) on inserts and deletes.
 * O(log n) on searches
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "owl.h"


#define INITSIZE 30
#define GROWBY 3 / 2

void owl_dict_create(owl_dict *d) {
  d->size=0;
  d->els=g_new(owl_dict_el, INITSIZE);
  d->avail=INITSIZE;
}

int owl_dict_get_size(const owl_dict *d) {
  return(d->size);
}

/* Finds the position of an element with key k, or of the element where
 * this element would logically go, and stores the index in pos.
 * Returns 1 if found, else 0. */
int _owl_dict_find_pos(const owl_dict *d, const char *k, int *pos) {
  int lo = 0, hi = d->size;
  while (lo < hi) {
    int mid = (lo + hi)/2; /* lo goes up and we can't hit hi, so no +1 */
    int cmp = strcmp(k, d->els[mid].k);
    if (cmp < 0) {
      hi = mid;
    } else if (cmp > 0) {
      lo = mid+1;
    } else {
      *pos = mid;
      return 1;
    }
  }
  *pos = lo;
  return 0;
}

/* returns the value corresponding to key k */
void *owl_dict_find_element(const owl_dict *d, const char *k) {
  int found, pos;
  found = _owl_dict_find_pos(d, k, &pos);
  if (!found) {
    return(NULL);
  }
  return(d->els[pos].v);
}

/* Returns a GPtrArray of dictionary keys. Duplicates the keys, so
 * they will need to be freed by the caller with g_free. */
CALLER_OWN GPtrArray *owl_dict_get_keys(const owl_dict *d) {
  GPtrArray *keys = g_ptr_array_sized_new(d->size);
  int i;
  for (i = 0; i < d->size; i++) {
    g_ptr_array_add(keys, g_strdup(d->els[i].k));
  }
  return keys;
}

void owl_dict_noop_delete(void *x)
{
  return;
}

/* Returns 0 on success.  Will copy the key but make 
   a reference to the value.  Will clobber an existing 
   entry with the same key iff delete_on_replace!=NULL,
   and will run delete_on_replace on the old element.
   Will return -2 if replace=NULL and match was found.
*/
int owl_dict_insert_element(owl_dict *d, const char *k, void *v, void (*delete_on_replace)(void *old))
{
  int pos, found;
  found = _owl_dict_find_pos(d, k, &pos);
  if (found && delete_on_replace) {
    delete_on_replace(d->els[pos].v);
    d->els[pos].v = v;
    return(0);
  } else if (found && !delete_on_replace) {
    return(-2);
  } else {
    if (d->size + 1 > d->avail) {
      int avail = MAX(d->avail * GROWBY, d->size + 1);
      d->els = g_renew(owl_dict_el, d->els, avail);
      d->avail = avail;
      if (d->els==NULL) return(-1);
    }
    if (pos!=d->size) {
      /* shift forward to leave us a slot */
      memmove(d->els+pos+1, d->els+pos, 
	      sizeof(owl_dict_el)*(d->size-pos));
    }
    d->size++;
    d->els[pos].k = g_strdup(k);
    d->els[pos].v = v;    
    return(0);
  }
}

/* Doesn't free the value of the element, but does
 * return it so the caller can free it. */
CALLER_OWN void *owl_dict_remove_element(owl_dict *d, const char *k)
{
  int i;
  int pos, found;
  void *v;
  found = _owl_dict_find_pos(d, k, &pos);
  if (!found) return(NULL);
  g_free(d->els[pos].k);
  v = d->els[pos].v;
  for (i=pos; i<d->size-1; i++) {
    d->els[i]=d->els[i+1];
  }
  d->size--;
  return(v);
}

/* elefree should free the value as well */
void owl_dict_cleanup(owl_dict *d, void (*elefree)(void *))
{
  int i;

  for (i=0; i<d->size; i++) {
    g_free(d->els[i].k);
    if (elefree) (elefree)(d->els[i].v);
  }
  if (d->els) g_free(d->els);
}

