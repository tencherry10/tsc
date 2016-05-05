#include "libts.h"

#define MUL_NO_OVERFLOW ((size_t)1 << (sizeof(size_t) * 4))

void * reallocf(void *ptr, size_t size) {
  void *nptr = realloc(ptr, size);
  // OOM condition
  if (!nptr && ptr && size != 0)
		free(ptr);
  return nptr;
}

void * reallocarray(void *ptr, size_t nmemb, size_t size) {
	if ((nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
	    nmemb > 0 && SIZE_MAX / nmemb < size) {
		errno = ENOMEM;
		return NULL;
	}
	return realloc(ptr, size * nmemb);
}
