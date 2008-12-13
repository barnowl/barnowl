#include "owl.h"

#if (GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION < 14)
/* Our own implementation of g_unichar_is_mark for glib versions that
   are too old to have one. */
int g_unichar_ismark(gunichar ucs)
{
  GUnicodeType t = g_unichar_type(ucs);
  switch (t) {
  case G_UNICODE_NON_SPACING_MARK:
  case G_UNICODE_COMBINING_MARK:
  case G_UNICODE_ENCLOSING_MARK:
       return 1;
  default:
       return 0;
  }
}
#endif
