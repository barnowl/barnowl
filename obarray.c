#include <stdlib.h>
#include <string.h>
#include "owl.h"

/*
 * At the moment, the obarray is represented as a sorted list of
 * strings. I judge this a reasonable tradeoff between simplicity and
 * reasonable efficient lookup (insertion should be comparatively
 * rare)
 */

/* Helper method: Lookup a key in the obarray. If the key exists,
 * return its index, and the interned value in *val. Otherwise, return
 * the index it should be inserted at.
 */
static int owl_obarray_lookup(const owl_obarray *oa, const char *key, const char **val)
{
  int first, last, mid;
  const char * str;
  int cmp;

  mid = 0;
  first = 0;
  last = owl_list_get_size(&(oa->strings)) - 1;
  while(first <= last) {
    mid = first + (last - first)/2;
    str = owl_list_get_element(&(oa->strings), mid);
    cmp = strcmp(key, str);
    if(cmp == 0) {
      *val = str;
      return mid;
    } else if(cmp < 0) {
      first = mid + 1;
    } else {
      last = mid - 1;
    }
  }
  *val = NULL;
  return mid;
}

/* Returns NULL if the string doesn't exist in the obarray */
const char * owl_obarray_find(const owl_obarray *oa, const char * string)
{
  const char *v;
  owl_obarray_lookup(oa, string, &v);
  return v;
}

/* Inserts the string into the obarray if it doesn't exist */
const char * owl_obarray_insert(owl_obarray *oa, const char * string)
{
  const char *v;
  int i;
  i = owl_obarray_lookup(oa, string, &v);
  if(!v) {
    char *v2 = owl_strdup(string);
    owl_list_insert_element(&(oa->strings), i, v2);
    return v2;
  }
  return v;
}

void owl_obarray_init(owl_obarray *oa)
{
  owl_list_create(&(oa->strings));
}
