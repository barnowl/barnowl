#define OWL_PERL
#include "owl.h"
#include <dlfcn.h>
#include <gperl.h>
#include <gperl_marshal.h>

gboolean (*gvalue_from_sv) (GValue * value, SV * sv) = NULL;
SV * (*sv_from_gvalue) (const GValue * value) = NULL;
GClosure * (*perl_closure_new) (SV * callback, SV * data, gboolean swap) = NULL;

/* Caller does not own return value */
const char *owl_closure_init(void)
{
  void *handle = NULL;
  const char *res = "Unable to dlopen self - huh?";

  handle = dlopen(NULL, RTLD_LAZY | RTLD_NOLOAD);
  if(handle) {
    gvalue_from_sv = dlsym(handle, "gperl_value_from_sv");
    sv_from_gvalue = dlsym(handle, "gperl_sv_from_value");
    perl_closure_new = dlsym(handle, "gperl_closure_new");
    /* ... */
    if (dlclose(handle) != 0
        || gvalue_from_sv == NULL
        || sv_from_gvalue == NULL
        || perl_closure_new == NULL) {
            res = dlerror();
    } else {
      res = NULL;
    }
  }
  return res;
}

