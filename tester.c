#include "owl.h"
#include <unistd.h>
#include <stdlib.h>

static const char fileIdent[] = "$Id$";

owl_global g;

#define OWL_DICT_NTESTS  20
#define OWL_UTIL_NTESTS  9
#define OWL_OBARRAY_NTESTS  5
#define OWL_VARIABLE_NTESTS  52
#define OWL_FILTER_NTESTS  23

int main(int argc, char **argv, char **env)
{
  owl_errqueue_init(owl_global_get_errqueue(&g));
  owl_obarray_init(&(g.obarray));

  int numfailures=0;
  printf("1..%d\n", OWL_UTIL_NTESTS+OWL_DICT_NTESTS+OWL_VARIABLE_NTESTS
         +OWL_FILTER_NTESTS+OWL_OBARRAY_NTESTS);
  numfailures += owl_util_regtest();
  numfailures += owl_dict_regtest();
  numfailures += owl_variable_regtest();
  numfailures += owl_filter_regtest();
  numfailures += owl_obarray_regtest();
  if (numfailures) {
      fprintf(stderr, "# *** WARNING: %d failures total\n", numfailures);
  }
  return(numfailures);
}
