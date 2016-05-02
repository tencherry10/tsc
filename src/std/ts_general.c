#include "libts.h"

void * reallocf(void *ptr, size_t size) {
  void *nptr = realloc(ptr, size);
  // OOM condition
  if (!nptr && ptr && size != 0)
		free(ptr);
  return nptr;
}
