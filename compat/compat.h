#include "../config.h"

#include <stddef.h>

#if !HAVE_DECL_MEMRCHR
void *memrchr(const void *s, int c, size_t n);
#endif  /* !HAVE_DECL_MEMRCHR */
