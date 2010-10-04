#include "compat.h"

void *memrchr(const void *s, int c, size_t n) {
  int i;
  const unsigned char *ss = s;
  for (i = n-1; i >= 0; i--) {
    if (ss[i] == (unsigned char)c)
      return ss + i;
  }
  return NULL;
}
