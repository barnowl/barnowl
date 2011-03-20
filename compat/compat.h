#ifndef INC_BARNOWL_COMPAT_COMPAT_H
#define INC_BARNOWL_COMPAT_COMPAT_H

#include "../config.h"

#include <stddef.h>

#if !HAVE_DECL_MEMRCHR
void *memrchr(const void *s, int c, size_t n);
#endif  /* !HAVE_DECL_MEMRCHR */

#endif /* INC_BARNOWL_COMPAT_COMPAT_H */
