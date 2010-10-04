#include "../config.h"

#include <stddef.h>

#ifndef HAVE_MEMRCHR
void *memrchr(const void *s, int c, size_t n);
#endif  /* HAVE_MEMRCHR */
