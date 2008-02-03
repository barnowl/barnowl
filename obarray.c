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
int owl_obarray_lookup(owl_obarray *oa, char * key, char ** val) /*noproto*/
{
  int first, last, mid;
  char * str;
  int cmp;

  mid = 0;
  first = 0;
  last = owl_list_get_size(&(oa->strings)) - 1;
  while(first <= last) {
    mid = first + (last - first)/2;
    str = (char*)owl_list_get_element(&(oa->strings), mid);
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
char * owl_obarray_find(owl_obarray *oa, char * string)
{
  char *v;
  owl_obarray_lookup(oa, string, &v);
  return v;
}

/* Inserts the string into the obarray if it doesn't exist */
char * owl_obarray_insert(owl_obarray *oa, char * string)
{
  char *v;
  int i;
  i = owl_obarray_lookup(oa, string, &v);
  if(!v) {
    v = owl_strdup(string);
    owl_list_insert_element(&(oa->strings), i, v);
  }
  return v;
}

void owl_obarray_init(owl_obarray *oa)
{
  owl_list_create(&(oa->strings));
}

/**************************************************************************/
/************************* REGRESSION TESTS *******************************/
/**************************************************************************/

#ifdef OWL_INCLUDE_REG_TESTS

#include "test.h"

int owl_obarray_regtest(void) {
  int numfailed = 0;
  char *p,*p2;

  owl_obarray oa;
  owl_obarray_init(&oa);

  printf("# BEGIN testing owl_obarray\n");

  p = owl_obarray_insert(&oa, "test");
  FAIL_UNLESS("returned string is equal", p && !strcmp(p, "test"));
  p2 = owl_obarray_insert(&oa, "test");
  FAIL_UNLESS("returned string is equal", p2 && !strcmp(p2, "test"));
  FAIL_UNLESS("returned the same string", p2 && p == p2);

  p = owl_obarray_insert(&oa, "test2");
  FAIL_UNLESS("returned string is equal", p && !strcmp(p, "test2"));
  p2 = owl_obarray_find(&oa, "test2");
  FAIL_UNLESS("returned the same string", p2 && !strcmp(p2, "test2"));

  p = owl_obarray_find(&oa, "nothere");
  FAIL_UNLESS("Didn't find a string that isn't there", p == NULL);

  printf("# END testing owl_obarray (%d failures)\n", numfailed);

  return numfailed;
}

#endif /* OWL_INCLUDE_REG_TESTS */
