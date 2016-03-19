// tsc.h 
// Public Domain -- no warranty is offered or implied (see http://www.wtfpl.net/)

// However, portions of this code is motivated directly / indirectly from other code base with permissive license
// They are listed in LICENSE.txt and for compliance it is best to include that in your LICENSE.txt if you choose to use tsc.h

// ideas for improvements
// 1. pool size are limited to 2^16*blocksize, maybe we should make a larger pool?
// 2. tsc_freadlines is unnecessarily complicated. Why not just memmap and search for '\n'
// 3. need to add stats to hpool / pool
// 4. consider using memory map for tsc_freadall 

#ifndef TSC__INCLUDE_TSC_H
#define TSC__INCLUDE_TSC_H
//}

#define _POSIX_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>

#ifdef __cplusplus
  #define TSC_EXTERN extern "C"
#else
  #define TSC_EXTERN extern
#endif

#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
  #define tsc_likely(expr) __builtin_expect(!!(expr), 1)
  #define tsc_likely_if(expr) if(__builtin_expect(!!(expr), 1))
  #define tsc_unlikely(expr) __builtin_expect(!!(expr), 0)
  #define tsc_unlikely_if(expr) if(__builtin_expect(!!(expr), 0))
#else
  #define tsc_likely(expr) (expr)
  #define tsc_likely_if(expr) if((expr))
  #define tsc_unlikely(expr) (expr)
  #define tsc_unlikely_if(expr) if((expr))
#endif

/// tsc_vec class is done with some nasty gnarly macros. This is intentional.
/// In C, where generic container is not possible, the only way to get a cleaner implementation
/// is to use void * pointers and then cast to and from it. The problem with that approach is that
/// casts prevents optimization and loop unrolling (type information is lost).

//{ tsc_vec_define
// CAUTION, if OOM happens in resize / copy / push, you are responsible for cleaning up dst
// Not possible to do it here b/c we don't have info on the resources pointed by a[i]

#define tsc_roundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))
#define tsc_vec_resize(name, v, s) tsc_vec_##name##_resize(&(v),(s))
#define tsc_vec_copy(name, d, s) tsc_vec_##name##_copy(&(d),&(s))
#define tsc_vec_push(name, v, x) tsc_vec_##name##_push(&(v), (x))
#define tsc_vec_compact(name, v) tsc_vec_##name##_resize(&(v), (v).n)
#define tsc_vec_extend(name, d, s) tsc_vec_##name##_extend(&(d),&(s))
#define tsc_vec_reverse(name, v) tsc_vec_##name##_reverse(&(v))
#define tsc_vec_subvec(name, d, s, start, count) tsc_vec_##name##_subvec(&(d), &(s), start, count)
#define tsc_vec_insert(name, v, idx, x) tsc_vec_##name##_insert(&(v), idx, x)
#define tsc_vec_pushfront(name, v, x) tsc_vec_##name##_insert(&(v), 0, x)
#define tsc_vec_all(name,v) tsc_vec_##name##_all(&(v))
#define tsc_vec_any(name,v) tsc_vec_##name##_any(&(v))

#define tsc_vec_init(v) ((v).n = (v).m = 0, (v).a = 0)
#define tsc_vec_destroy(v) free((v).a)
#define tsc_vec_A(v, i) ((v).a[(i)])
#define tsc_vec_pop(v) ((v).a[--(v).n])
#define tsc_vec_size(v) ((v).n)
#define tsc_vec_max(v) ((v).m)
#define tsc_vec_ptr(v) ((v).a)
#define tsc_vec_clear(v) ((v).n = 0)
#define tsc_vec_first(v) ((v).a[0])
#define tsc_vec_last(v) ((v).a[(v).n-1])
#define tsc_vec_type(name) tsc_vec_##name##_t

#define tsc_vec_define(name, type)                                                      \
  typedef struct { size_t n, m; type *a; } tsc_vec_##name##_t;                          \
  static inline const char* tsc_vec_##name##_resize(tsc_vec_##name##_t *v, size_t s) {  \
    type *tmp;                                                                          \
    tsc_unlikely_if((tmp = (type*)realloc(v->a, sizeof(type) * s)) == NULL )                        \
      return "OOM";                                                                     \
    v->m = s; v->a = tmp; return NULL; }                                                \
  static inline const char*                                                             \
  tsc_vec_##name##_copy(tsc_vec_##name##_t *dst, tsc_vec_##name##_t *src) {             \
    const char *estr;                                                                   \
    tsc_unlikely_if(dst == src) return "COPYING FROM SRC TO SRC";                                   \
    if (dst->m < src->n)                                                                \
      tsc_unlikely_if( (estr = tsc_vec_##name##_resize(dst, src->n)) != NULL )                       \
        return estr;                                                                    \
    dst->n = src->n;                                                                    \
    memcpy(dst->a, src->a, sizeof(type) * src->n); return NULL; }                       \
  static inline const char* tsc_vec_##name##_push(tsc_vec_##name##_t *v, type x) {      \
    const char *estr;                                                                   \
    if(v->n == v->m) {                                                                  \
      size_t m = v->m ? v->m << 1 : 4;                                                  \
      tsc_unlikely_if( (estr = tsc_vec_##name##_resize(v, m)) != NULL )                 \
        return estr;                                                                    \
    }                                                                                   \
    v->a[v->n++]= x; return NULL; }                                                     \
  static inline const char*                                                             \
  tsc_vec_##name##_extend(tsc_vec_##name##_t *dst, tsc_vec_##name##_t *src) {           \
    const char *estr;                                                                   \
    size_t n = dst->n + src->n;                                                         \
    if(dst->m < n)                                                                      \
      tsc_unlikely_if( (estr = tsc_vec_##name##_resize(dst, n)) != NULL )               \
        return estr;                                                                    \
    memcpy(dst->a + dst->n, src->a, sizeof(type) * src->n);                             \
    dst->n = n; return NULL; }                                                          \
  static inline const char * tsc_vec_##name##_reverse(tsc_vec_##name##_t *v) {          \
    type tmp;                                                                           \
    for( size_t i = (v->n-1) >> 1 ; (i + 1) > 0 ; --i) {                                \
      tmp = v->a[i]; v->a[i] = v->a[v->n - i - 1]; v->a[v->n - i - 1] = tmp;            \
    }                                                                                   \
    return NULL; }                                                                      \
  static inline const char* tsc_vec_##name##_subvec(tsc_vec_##name##_t *dst,            \
    tsc_vec_##name##_t *src, size_t start, size_t count)                                \
  {                                                                                     \
    tsc_unlikely_if(dst == src) {                                                       \
      tsc_vec_splice(*dst, start, count);                                               \
      return NULL;                                                                      \
    }                                                                                   \
    const char *estr;                                                                   \
    size_t n;                                                                           \
    if(start > src->n) {                                                                \
      dst->n = 0;                                                                       \
      return NULL;                                                                      \
    }                                                                                   \
    if(start + count > src->n) {                                                        \
      n = src->n - start;                                                               \
    } else {                                                                            \
      n = count;                                                                        \
    }                                                                                   \
    tsc_unlikely_if( (estr = tsc_vec_##name##_resize(dst, n)) != NULL )                 \
      return estr;                                                                      \
    memcpy(dst->a, src->a + start, sizeof(type) * n);                                   \
    dst->n = n; return NULL; }                                                          \
  static inline const char*                                                             \
  tsc_vec_##name##_insert(tsc_vec_##name##_t *v, size_t idx, type x) {                  \
    tsc_unlikely_if(idx > v->n) return "INDEX OUT OF BOUND";                            \
    if(idx == v->n) return tsc_vec_##name##_push(v, x);                                 \
    if(v->n == v->m) {                                                                  \
      const char *estr;                                                                 \
      size_t m = v->m ? v->m << 1 : 4;                                                  \
      tsc_unlikely_if( (estr = tsc_vec_##name##_resize(v, m)) != NULL )                 \
        return estr;                                                                    \
    }                                                                                   \
    for(size_t i = v->n ; i > idx ; --i ) {                                             \
      v->a[i] = v->a[i-1];                                                              \
    }                                                                                   \
    v->n++; v->a[idx] = x; return NULL; }                                               \
  static inline int tsc_vec_##name##_all(tsc_vec_##name##_t *v) {                       \
    int ret = 1;                                                                        \
    for(size_t i = 0 ; i < v->n ; ++i)                                                  \
      ret = ret && !!(v->a[i]);                                                         \
    return ret; }                                                                       \
  static inline int tsc_vec_##name##_any(tsc_vec_##name##_t *v) {                       \
    int ret = 0;                                                                        \
    for(size_t i = 0 ; i < v->n ; ++i)                                                  \
      ret = ret || !!(v->a[i]);                                                         \
    return ret; }
#define tsc_vec_splice(v, start, count) do {                                            \
    tsc_unlikely_if(count == 0) break;                                                  \
    tsc_unlikely_if(start > (v).n) {                                                    \
      (v).n = 0;                                                                        \
      break;                                                                            \
    }                                                                                   \
    tsc_unlikely_if(start + count > (v).n) {                                            \
      (v).n = start;                                                                    \
      break;                                                                            \
    }                                                                                   \
    memmove((v).a + start, (v).a + start + count,                                       \
      sizeof((v).a[0]) * ((v).n - start - count));                                      \
    (v).n -= count;                                                                     \
  } while(0)
#define tsc_vec_foreach(v, var, iter)                                                   \
  tsc_likely_if( (v).n > 0 )                                                            \
    for ( (iter) = 0; ((iter) < (v).n) && (((var) = (v).a[(iter)]), 1); ++(iter))
#define tsc_vec_foreach_rev(v, var, iter)                                               \
  tsc_likely_if( (v).n > 0 )                                                            \
    for ( (iter) = v.n - 1 ; ((iter + 1) > 0) && (((var) = (v).a[(iter)]), 1); --(iter))
#define tsc_vec_foreach_ptr(v, var, iter)                                               \
  tsc_likely_if( (v).n > 0 )                                                            \
    for ( (iter) = 0; ((iter) < (v).n) && (((var) = &((v).a[(iter)])), 1); ++(iter))
#define tsc_vec_foreach_ptr_rev(v, var, iter)                                           \
  tsc_likely_if( (v).n > 0 )                                                            \
    for ( (iter) = v.n - 1 ; ((iter + 1) > 0) && (((var) = &((v).a[(iter)]) ), 1); --(iter))
#define tsc_vec_map(v, fn) do {                                                         \
    tsc_likely_if( (v).n > 0 ) {                                                        \
      for ( size_t i = 0; i < (v).n ; ++i) {                                              \
        (v).a[i] = fn((v).a[i]);                                                          \
      }                                                                                 \
    }                                                                                   \
  } while(0)
//}

/// helper that never return error

TSC_EXTERN int    tsc_strstartswith(const char *s, const char *start);
TSC_EXTERN int    tsc_strendswith(const char *s, const char *end);
TSC_EXTERN char * tsc_strtrimleft_inplace(char *s);
TSC_EXTERN char * tsc_strtrimright_inplace(char *s);
TSC_EXTERN char * tsc_strtrim_inplace(char *s);
TSC_EXTERN char * tsc_strupper_inplace(char *str);
TSC_EXTERN char * tsc_strlower_inplace(char *str);

/// general helper

TSC_EXTERN const char * tsc_strtrim(char **ret, const char *s);
TSC_EXTERN const char * tsc_strdup(char **ret, const char *s);
TSC_EXTERN const char * tsc_strndup(char **ret, const char *s, size_t n);
TSC_EXTERN const char * tsc_strflatten(char **ret, char ** str_array, size_t n, const char * sep, size_t sep_sz);
TSC_EXTERN inline const char * tsc_strtrunc(char **ret, const char *s, size_t n);
TSC_EXTERN inline const char * tsc_strupper(char **ret, char *s);
TSC_EXTERN inline const char * tsc_strlower(char **ret, char *s);

/// files / IO
TSC_EXTERN const char * tsc_freadline(char **ret, FILE *stream);
TSC_EXTERN const char * tsc_freadlines(char ***ret, int *nlines, FILE *stream);
TSC_EXTERN const char * tsc_freadlinesfree(char **ret, int nlines);
TSC_EXTERN const char * tsc_freadline0(char **ret, int *sz, FILE *stream);

TSC_EXTERN const char * tsc_getpass(char **lineptr, FILE *stream);

TSC_EXTERN const char * tsc_freadall(char **ret, FILE *stream);

/// memory
TSC_EXTERN const char * tsc_alloc2d(void ***ret, size_t y, size_t x, size_t sz);
TSC_EXTERN const char * tsc_alloc2d_irregular(void ***ret, size_t y, size_t * x_per_y, size_t sz);
TSC_EXTERN const char * tsc_alloc3d(void ****ret, size_t z, size_t y, size_t x, size_t sz);
TSC_EXTERN const char * tsc_alloc3d_irregular(void ****ret, size_t z, size_t y, size_t ** x_per_z_y, size_t sz);

/// encoding / decoding
TSC_EXTERN const char * tsc_base64_enc(char **ret, char *data, size_t sz);
TSC_EXTERN const char * tsc_base64_dec(char **ret, size_t* retsz, char *data, size_t datsz);

#define TSC_POOL_ATTPACKPRE
#define TSC_POOL_ATTPACKSUF __attribute__((__packed__))

#ifndef TSC_POOL_CRITICAL_ENTRY
  #define TSC_POOL_CRITICAL_ENTRY()
#endif
#ifndef TSC_POOL_CRITICAL_EXIT
  #define TSC_POOL_CRITICAL_EXIT()
#endif

#ifndef TSC_HPOOL_CRITICAL_ENTRY
  #define TSC_HPOOL_CRITICAL_ENTRY()
#endif
#ifndef TSC_HPOOL_CRITICAL_EXIT
  #define TSC_HPOOL_CRITICAL_EXIT()
#endif

#ifndef TSC_POOL_FIRST_FIT
#  ifndef TSC_POOL_BEST_FIT
#    define TSC_POOL_BEST_FIT
#  endif
#endif

#ifndef TSC_HPOOL_FIRST_FIT
#  ifndef TSC_HPOOL_BEST_FIT
#    define TSC_HPOOL_BEST_FIT
#  endif
#endif



TSC_POOL_ATTPACKPRE typedef struct tsc_pool_ptr {
  uint16_t next;
  uint16_t prev;
} TSC_POOL_ATTPACKSUF tsc_pool_ptr;

#if INTPTR_MAX == INT32_MAX

TSC_POOL_ATTPACKPRE typedef struct tsc_hpool_block {
  union {
    tsc_pool_ptr  used;
  } header;
  struct {
    uint16_t parent,  child;
    uint16_t next,    prev;
  } hier;
  union {
    tsc_pool_ptr  free;
    uint8_t       data[4];
  } body;
} TSC_POOL_ATTPACKSUF tsc_hpool_block;

TSC_POOL_ATTPACKPRE typedef struct tsc_pool_block {
  union {
    tsc_pool_ptr  used;
  } header;
  union {
    tsc_pool_ptr  free;
    uint8_t       data[4];
  } body;
} TSC_POOL_ATTPACKSUF tsc_pool_block;


#else

// on 64-bit architecture, I want to ensure that pointers are aligned on 8byte boundary
// so sizeof(tsc_pool_block) = 24 as opposed to 16 which is easier to divided into
// fortunately, divide by a constant 24 is optimized into a multiple and right shift.

TSC_POOL_ATTPACKPRE typedef struct tsc_hpool_block {
  union {
    tsc_pool_ptr  used;
  } header;
  struct {
    uint16_t parent,  child;
    uint16_t next,    prev;
  } hier;
  uint8_t padding[4];
  union {
    tsc_pool_ptr  free;
    uint8_t       data[8];
  } body;
} TSC_POOL_ATTPACKSUF tsc_hpool_block;

TSC_POOL_ATTPACKPRE typedef struct tsc_pool_block {
  union {
    tsc_pool_ptr  used;
  } header;
  uint8_t padding[4];
  union {
    tsc_pool_ptr  free;
    uint8_t       data[8];
  } body;
} TSC_POOL_ATTPACKSUF tsc_pool_block;

#endif

typedef struct tsc_pool_t {
  tsc_pool_block  *heap;
  uint16_t        numblocks;
  uint16_t        allocd;
} tsc_pool_t;

typedef struct tsc_hpool_t {
  tsc_hpool_block   *heap;
  uint16_t          numblocks;
  uint16_t          allocd;
} tsc_hpool_t;

TSC_EXTERN const char * tsc_hpool_init(tsc_hpool_t *heap, tsc_hpool_block *mem, uint16_t numblocks);
TSC_EXTERN void         tsc_hpool_deinit(tsc_hpool_t *heap);
TSC_EXTERN void         tsc_hpool_free(tsc_hpool_t *heap, void *ptr);
TSC_EXTERN void         tsc_hpool_freeall(tsc_hpool_t *heap);
TSC_EXTERN void *       tsc_hpool_malloc(tsc_hpool_t *heap, size_t size);
TSC_EXTERN void *       tsc_hpool_calloc(tsc_hpool_t *heap, size_t n, size_t size);
TSC_EXTERN void *       tsc_hpool_realloc(tsc_hpool_t *heap, void *ptr, size_t size);
TSC_EXTERN void         tsc_hpool_attach(tsc_hpool_t *heap, void *ptr, void *parent);
TSC_EXTERN void *       tsc_hpool_info(tsc_hpool_t *heap, void *ptr);
      
TSC_EXTERN const char * tsc_pool_init(tsc_pool_t *heap, tsc_pool_block *mem, uint16_t numblocks);
TSC_EXTERN void         tsc_pool_deinit(tsc_pool_t *heap);
TSC_EXTERN void         tsc_pool_free(tsc_pool_t *heap, void *ptr);
TSC_EXTERN void         tsc_pool_freeall(tsc_pool_t *heap);
TSC_EXTERN void *       tsc_pool_malloc(tsc_pool_t *heap, size_t size);
TSC_EXTERN void *       tsc_pool_calloc(tsc_pool_t *heap, size_t n, size_t size);
TSC_EXTERN void *       tsc_pool_realloc(tsc_pool_t *heap, void *ptr, size_t size);
TSC_EXTERN void *       tsc_pool_info(tsc_pool_t *heap, void *ptr);

#define TSC_DEFINE
#ifdef TSC_DEFINE
//}

int tsc_strstartswith(const char *s, const char *start) {
  for(;; s++, start++)
    if(!*start)
      return 1;
    else if(*s != *start) 
      return 0;
}

int tsc_strendswith(const char *s, const char *end) {
  int end_len;
  int str_len;
  
  tsc_unlikely_if(NULL == s || NULL == end) return 0;

  end_len = strlen(end);
  str_len = strlen(s);

  return str_len < end_len ? 1 : !strcmp(s + str_len - end_len, end);
}

char* tsc_strtrimleft_inplace(char *str) {
  int len = strlen(str);
  tsc_unlikely_if(len == 0) return str;
  char *cur = str;
  while (*cur && isspace(*cur)) {
    ++cur;
    --len;
  }
  if (str != cur) memmove(str, cur, len + 1);
  return str;
}

char* tsc_strtrimright_inplace(char *str) {
  int len = strlen(str);
  tsc_unlikely_if(len == 0) return str;
  char *cur = str + len - 1;
  while (cur != str && isspace(*cur)) --cur;
  cur[isspace(*cur) ? 0 : 1] = '\0';
  return str;
}

char* tsc_strtrim_inplace(char *s) {
  tsc_strtrimright_inplace(s);
  tsc_strtrimleft_inplace(s);
  return s;
}

char* tsc_strupper_inplace(char *str) {
  for (int i = 0, len = strlen(str); i < len; i++) {
    if (islower(str[i])) {
      str[i] &= ~0x20;
    }
  }
  return str;
}

char* tsc_strlower_inplace(char *str) {
  for (int i = 0, len = strlen(str); i < len; i++) {
    if (isupper(str[i])) {
      str[i] |= 0x20;
    }
  }
  return str;
}

const char * tsc_strflatten(char **ret, char ** str_array, size_t n, const char * sep, size_t sep_sz) {
  size_t lens[n];
  size_t size   = 0;
  size_t pos    = 0;
  
  for(size_t i = 0 ; i < n ; i++) {
    lens[i] = strlen(str_array[i]);
    size   += lens[i];
  }
  
  tsc_unlikely_if( (*ret = (char *) malloc(size + 1 + (n-1)*sep_sz)) == NULL )
    return "OOM";
    
  (*ret)[size + (n-1)*sep_sz] = '\0';
  
  for(size_t i = 0 ; i < (n - 1) ; i++) {
    memcpy(*ret + pos, str_array[i], lens[i]);
    for(size_t j = 0 ; j < sep_sz ; j++)
      (*ret)[pos + lens[i]] = sep[j];
    pos += lens[i] + sep_sz;
  }
  
  memcpy(*ret + pos, str_array[n-1], lens[n - 1]);
  
  return NULL;
}


inline const char * tsc_strtrunc(char **ret, const char *s, size_t n) {
  return tsc_strndup(ret, s, n);
}


const char * tsc_strtrim(char **ret, const char *s) {
  const char *end;
  *ret = NULL;
  
  while (isspace(*s))
    s++;

  tsc_unlikely_if(*s == 0) // only spaces
    return NULL;

  // rtrim
  end = s + strlen(s) - 1;
  while (end > s && isspace(*end))
    end--;
  
  return tsc_strndup(ret, s, end - s + 1);
}

const char * tsc_strdup(char **ret, const char *s) {
  tsc_unlikely_if(s == NULL) { *ret = NULL; return NULL; }
  size_t len = strlen(s) + 1;
  tsc_unlikely_if( (*ret = (char *) malloc(len)) == NULL )
    return "OOM";
  memcpy(*ret, s, len);
  return NULL;
}

const char * tsc_strndup(char **ret, const char *s, size_t n) {
  tsc_unlikely_if(s == NULL) { *ret = NULL; return NULL; }
  tsc_unlikely_if( (*ret = (char *) malloc(n+1)) == NULL )
    return "OOM";
  strncpy(*ret, s, n);  // use strncpy to ensure *ret is properly padded with zero
  (*ret)[n] = '\0';
  return NULL;
}

inline const char * tsc_strupper(char **ret, char *s) {
  const char  *estr = NULL;
  if( (estr = tsc_strdup(ret, s)) != NULL )
    return estr;
  tsc_strupper_inplace(*ret);
  return NULL;
}

inline const char * tsc_strlower(char **ret, char *s) {
  const char  *estr = NULL;
  tsc_unlikely_if( (estr = tsc_strdup(ret, s)) != NULL )
    return estr;
  tsc_strlower_inplace(*ret);
  return NULL;
}

// this function models after python's readline function
// it will take care of all the corner cases of fgets
// but it assumes there are no \0 in the file/stream
//
// returns NULL on success, otherwise it return a error string

const char * tsc_freadline(char **ret, FILE *stream) {
  char        *tmp  = NULL;
  int         avail = 32;
  int         n     = 0;
  int         nlast;
  
  tsc_unlikely_if( (*ret = (char *) calloc(avail,1)) == NULL )
    return "OOM";
  
  while(1) {
    if( fgets(*ret + n, avail - n, stream) == NULL ) {
      tsc_unlikely_if(ferror(stream) != 0) {
        free(*ret); *ret = NULL; return "FGETS FAILED";
      } 
      return "EOF";
    }
    
    nlast = strlen(*ret)-1;
    
    if( (*ret)[nlast] == '\n') { // if fgets ended b/c of newline, properly end function
      return NULL;
    }
    
    if( feof(stream) != 0 ) {
      if(nlast == (avail - 2)) {  // bad luck, eof at boundary of buffer (need to grow it by 1)
        tsc_unlikely_if( (tmp = (char *) realloc(*ret, ++avail)) == NULL ) {
          free(*ret); *ret = NULL; return "OOM";
        }
      }
      return "EOF";
    }
    
    if(nlast == (avail - 2)) {  // not eof and no new line, buffer size must be not big enough
      n     = avail - 1;        // avail - 1 b/c we want to reuse the old \0 spot
      avail = avail << 1;
      tsc_unlikely_if( (tmp = (char *) realloc(*ret, avail)) == NULL ) {
        free(*ret); *ret = NULL; return "OOM";
      }
      *ret = tmp;
    }
  }
}

const char * tsc_freadlines(char ***ret, int *nlines, FILE *stream) {
  const char  *estr   = NULL;
  char        *line   = NULL;
  int         avail   = 8;
  char        **tmp   = NULL;
  
  *nlines = 0;
  
  tsc_unlikely_if( (*ret = (char **) calloc(avail,sizeof(char *))) == NULL )
    return "OOM";
  
  while(1) {
    if( (estr = tsc_freadline(&line, stream)) != NULL) {
      if(!strcmp("EOF", estr)) {
        (*ret)[(*nlines)++]=line;
        return NULL;
      }
      return estr;
    } else {
      (*ret)[(*nlines)++]=line;
      line = NULL;
      if(avail == *nlines) {
        tsc_unlikely_if( (tmp = (char **) realloc(*ret, avail << 1)) == NULL ) {
          for(int i = 0 ; i < avail ; i++) 
            free((*ret)[i]);
          free((*ret)); *ret = NULL; return "OOM";
        }
        *ret = tmp;
        avail = avail << 1;
      }
    }
  }
}

const char * tsc_freadlinesfree(char **ret, int nlines) {
  for(int i = 0 ; i < nlines ; i++) 
    free(ret[i]);
  free(ret);
  return NULL;
}

const char * tsc_getpass(char **lineptr, FILE *stream) {
  struct termios o, n;
  
  if(tcgetattr(fileno(stream), &o) != 0)
    return "FAILED to get old stream settings";
  
  n = o;
  n.c_lflag &= ~ECHO;   
  
  if(tcsetattr(fileno(stream), TCSAFLUSH, &n) != 0)
    return "FAILED to set new stream settings";
  
  const char *estr = tsc_freadline(lineptr, stream);
  
  if(tcsetattr(fileno(stream), TCSAFLUSH, &o) != 0)
    return "FAILED to reset stream settings";
  
  return estr;
}

const char * tsc_freadall(char **ret, FILE *stream) {
  char    *tmp  = NULL;  
  size_t  avail = 256;
  size_t  n     = 0;
  size_t  nread;

  tsc_unlikely_if( (*ret = (char *) malloc(avail+1)) == NULL )
    return "OOM";
  (*ret)[avail] = '\0';
  
  while(1) {
    nread = fread(*ret + n, 1, avail - n, stream);
    
    if(nread != (avail - n)) {
      tsc_unlikely_if(ferror(stream) != 0) {
        free(*ret); *ret = NULL; return "FGETS FAILED";
      } 
      return "EOF";
    }
    
    n = avail;
    avail = avail << 1;
    tsc_unlikely_if( (tmp = (char *) realloc(*ret, avail+1)) == NULL ) {
      free(*ret); *ret = NULL; return "OOM";
    }
    *ret = tmp;
    (*ret)[avail] = '\0';
  }
}

// same as tsc_freadline, but will handle \0
// which is why sz parameter is needed. When \0 exist *sz != strlen(ret)

const char * tsc_freadline0(char **ret, int *sz, FILE *stream) {
  char        *tmp  = NULL;
  int         avail = 32;
  int         n     = 0;
  int         nlast;
  
  tsc_unlikely_if( (*ret = (char *) calloc(avail, 1)) == NULL )
    return "OOM";
    
  while(1) {
    if( fgets(*ret + n, avail - n, stream) == NULL ) {
      tsc_unlikely_if(ferror(stream) != 0) {
        if(sz) *sz = 0;
        free(*ret); *ret = NULL; return "FGETS FAILED";
      } 
      if(sz) *sz = n;
      return "EOF";
    }
    
    for(nlast = avail - 1 ; nlast >= 0 ; nlast--) {
      if((*ret)[nlast] != '\0') {
        break;
      }
    }
    
    if( (*ret)[nlast] == '\n') { // if fgets ended b/c of newline, properly end function
      if(sz) *sz = nlast+1;
      return NULL;
    }
    
    if( feof(stream) != 0 ) {
      if(nlast == (avail - 2)) {  // bad luck, eof at boundary of buffer (need to grow it by 1)
        tsc_unlikely_if( (tmp = (char *) realloc(*ret, ++avail)) == NULL ) {
          if(sz) *sz = 0; 
          free(*ret); *ret = NULL; return "OOM";
        }
      }
      if(sz) *sz = nlast+1;
      return "EOF";
    }
    
    if(nlast == (avail - 2)) {  // not eof and no new line, buffer size must be not big enough
      n     = avail - 1;        // avail - 1 b/c we want to reuse the old \0 spot
      avail = avail << 1;
      tsc_unlikely_if( (tmp = (char *) realloc(*ret, avail)) == NULL ) {
        if(sz) *sz = 0;
        free(*ret); *ret = NULL; return "OOM";
      }
      *ret = tmp;
      memset(*ret + n, 0, avail - n);
    }
  }
}

// Pool Allocator based on Ralph Hempel's umm_malloc
// changes made:
// 1. move under tsc naming space for consistency
// 2. change types to stdint types (uint*_t)
// 3. pass in pools instead of depending on one global pool
// 4. removal of debug log
// 5. fix some pedantic warnings about pointer arithmetic over void*
// 6. use platform independent printf formatting for pointers PRIXPTR

// couple notes about this allocator
// 1. it is rather inefficient for small allocations. amount of memory used
//    for 32bit machine: numbytes < 4 ? 16 : (ceil((numbytes - 4)/16)+1)*16
//    for 64bit machine: numbytes < 8 ? 24 : (ceil((numbytes - 8)/24)+1)*24
//
//    this is only a bit worse than default malloc performance of:
//    for 32bit machine:  ceil((numbytes+4)/8)*8 < 16 ? 16 : ceil((numbytes+4)/8)*8
//    for 64bit machine:  ceil((numbytes+8)/16)*16 ? 32 : ceil((numbytes+8)/16)*16
// 2. if you are worried about memory fragmentation use TSC_POOL_BEST_FIT
//    just be aware the TSC_POOL_BEST_FIT could potentially be very expensive b/c in the worse case 
//    it has to traverse ALL free list before deciding on best fit

#define TSC_POOL_FREELIST_MASK (0x8000)
#define TSC_POOL_BLOCKNO_MASK  (0x7FFF)
#define TSC_POOL_BLOCK(b)  (heap->heap[b])
#define TSC_POOL_NBLOCK(b) (TSC_POOL_BLOCK(b).header.used.next)
#define TSC_POOL_PBLOCK(b) (TSC_POOL_BLOCK(b).header.used.prev)
#define TSC_POOL_NFREE(b)  (TSC_POOL_BLOCK(b).body.free.next)
#define TSC_POOL_PFREE(b)  (TSC_POOL_BLOCK(b).body.free.prev)
#define TSC_POOL_DATA(b)   (TSC_POOL_BLOCK(b).body.data)

#define TSC_HPOOL_FREELIST_MASK (0x8000)
#define TSC_HPOOL_BLOCKNO_MASK  (0x7FFF)
#define TSC_HPOOL_BLOCK(b)  (heap->heap[b])
#define TSC_HPOOL_NBLOCK(b) (TSC_HPOOL_BLOCK(b).header.used.next)
#define TSC_HPOOL_PBLOCK(b) (TSC_HPOOL_BLOCK(b).header.used.prev)
#define TSC_HPOOL_NFREE(b)  (TSC_HPOOL_BLOCK(b).body.free.next)
#define TSC_HPOOL_PFREE(b)  (TSC_HPOOL_BLOCK(b).body.free.prev)
#define TSC_HPOOL_DATA(b)   (TSC_HPOOL_BLOCK(b).body.data)
#define TSC_HPOOL_PARENT(b) (TSC_HPOOL_BLOCK(b).hier.parent)
#define TSC_HPOOL_CHILD(b)  (TSC_HPOOL_BLOCK(b).hier.child)
#define TSC_HPOOL_NSIBL(b)  (TSC_HPOOL_BLOCK(b).hier.next)
#define TSC_HPOOL_PSIBL(b)  (TSC_HPOOL_BLOCK(b).hier.prev)

typedef struct tsc_pool_info_t {
  uint16_t totalEntries,  usedEntries,  freeEntries; 
  uint16_t totalBlocks,   usedBlocks,   freeBlocks; 
} tsc_pool_info_t;

void *tsc_pool_info(tsc_pool_t *heap, void *ptr) {
  tsc_pool_info_t heapInfo;
  unsigned short int blockNo = 0;

  // Protect the critical section...
  //
  TSC_POOL_CRITICAL_ENTRY();
  
  // Clear out all of the entries in the heapInfo structure before doing
  // any calculations..
  //
  memset( &heapInfo, 0, sizeof( heapInfo ) );

  printf("\n\nDumping the tsc_pool_heap...\n" );

  printf("|0x%" PRIXPTR "|B %5i|NB %5i|PB %5i|Z %5i|NF %5i|PF %5i|\n",
          (uintptr_t)(&TSC_POOL_BLOCK(blockNo)),
          blockNo,
          TSC_POOL_NBLOCK(blockNo) & TSC_POOL_BLOCKNO_MASK,
          TSC_POOL_PBLOCK(blockNo),
          (TSC_POOL_NBLOCK(blockNo) & TSC_POOL_BLOCKNO_MASK )-blockNo,
          TSC_POOL_NFREE(blockNo),
          TSC_POOL_PFREE(blockNo) );

  // Now loop through the block lists, and keep track of the number and size
  // of used and free blocks. The terminating condition is an nb pointer with
  // a value of zero...
  
  blockNo = TSC_POOL_NBLOCK(blockNo) & TSC_POOL_BLOCKNO_MASK;

  while( TSC_POOL_NBLOCK(blockNo) & TSC_POOL_BLOCKNO_MASK ) {
    ++heapInfo.totalEntries;
    heapInfo.totalBlocks += (TSC_POOL_NBLOCK(blockNo) & TSC_POOL_BLOCKNO_MASK )-blockNo;

    // Is this a free block?

    if( TSC_POOL_NBLOCK(blockNo) & TSC_POOL_FREELIST_MASK ) {
      ++heapInfo.freeEntries;
      heapInfo.freeBlocks += (TSC_POOL_NBLOCK(blockNo) & TSC_POOL_BLOCKNO_MASK )-blockNo;

      printf("|0x%" PRIXPTR "|B %5i|NB %5i|PB %5i|Z %5i|NF %5i|PF %5i|\n",
              (uintptr_t)(&TSC_POOL_BLOCK(blockNo)),
              blockNo,
              TSC_POOL_NBLOCK(blockNo) & TSC_POOL_BLOCKNO_MASK,
              TSC_POOL_PBLOCK(blockNo),
              (TSC_POOL_NBLOCK(blockNo) & TSC_POOL_BLOCKNO_MASK )-blockNo,
              TSC_POOL_NFREE(blockNo),
              TSC_POOL_PFREE(blockNo) );
     
      // Does this block address match the ptr we may be trying to free?

      if( ptr == &TSC_POOL_BLOCK(blockNo) ) {
       
        // Release the critical section...
        //
        TSC_POOL_CRITICAL_EXIT();
 
        return( ptr );
      }
    } else {
      ++heapInfo.usedEntries;
      heapInfo.usedBlocks += (TSC_POOL_NBLOCK(blockNo) & TSC_POOL_BLOCKNO_MASK )-blockNo;

      printf("|0x%" PRIXPTR "|B %5i|NB %5i|PB %5i|Z %5i|\n",
              (uintptr_t)(&TSC_POOL_BLOCK(blockNo)),
              blockNo,
              TSC_POOL_NBLOCK(blockNo) & TSC_POOL_BLOCKNO_MASK,
              TSC_POOL_PBLOCK(blockNo),
              (TSC_POOL_NBLOCK(blockNo) & TSC_POOL_BLOCKNO_MASK )-blockNo);
    }

    blockNo = TSC_POOL_NBLOCK(blockNo) & TSC_POOL_BLOCKNO_MASK;
  }

  // Update the accounting totals with information from the last block, the
  // rest must be free!

  heapInfo.freeBlocks  += heap->numblocks - blockNo;
  heapInfo.totalBlocks += heap->numblocks - blockNo;

  printf("|0x%" PRIXPTR "|B %5i|NB %5i|PB %5i|Z %5i|NF %5i|PF %5i|\n",
          (uintptr_t)(&TSC_POOL_BLOCK(blockNo)),
          blockNo,
          TSC_POOL_NBLOCK(blockNo) & TSC_POOL_BLOCKNO_MASK,
          TSC_POOL_PBLOCK(blockNo),
          heap->numblocks - blockNo,
          TSC_POOL_NFREE(blockNo),
          TSC_POOL_PFREE(blockNo) );

  printf("Total Entries %5i    Used Entries %5i    Free Entries %5i\n",
          heapInfo.totalEntries,
          heapInfo.usedEntries,
          heapInfo.freeEntries );

  printf("Total Blocks  %5i    Used Blocks  %5i    Free Blocks  %5i\n",
          heapInfo.totalBlocks,
          heapInfo.usedBlocks,
          heapInfo.freeBlocks  );

  // Release the critical section...
  //
  TSC_POOL_CRITICAL_EXIT();
 
  return( NULL );
}


void *tsc_hpool_info(tsc_hpool_t *heap, void *ptr) {
  tsc_pool_info_t heapInfo;
  unsigned short int blockNo = 0;

  // Protect the critical section...
  //
  TSC_HPOOL_CRITICAL_ENTRY();
  
  // Clear out all of the entries in the heapInfo structure before doing
  // any calculations..
  //
  memset( &heapInfo, 0, sizeof( heapInfo ) );

  printf("\n\nDumping the tsc_hpool_heap...\n" );

  printf("|0x%" PRIXPTR "|B %5i|NB %5i|PB %5i|Z %5i|                                   |NF %5i|PF %5i|\n",
          (uintptr_t)(&TSC_HPOOL_BLOCK(blockNo)),
          blockNo,
          TSC_HPOOL_NBLOCK(blockNo) & TSC_HPOOL_BLOCKNO_MASK,
          TSC_HPOOL_PBLOCK(blockNo),
          (TSC_HPOOL_NBLOCK(blockNo) & TSC_HPOOL_BLOCKNO_MASK )-blockNo,
          TSC_HPOOL_NFREE(blockNo),
          TSC_HPOOL_PFREE(blockNo) );

  // Now loop through the block lists, and keep track of the number and size
  // of used and free blocks. The terminating condition is an nb pointer with
  // a value of zero...
  
  blockNo = TSC_HPOOL_NBLOCK(blockNo) & TSC_HPOOL_BLOCKNO_MASK;

  while( TSC_HPOOL_NBLOCK(blockNo) & TSC_HPOOL_BLOCKNO_MASK ) {
    ++heapInfo.totalEntries;
    heapInfo.totalBlocks += (TSC_HPOOL_NBLOCK(blockNo) & TSC_HPOOL_BLOCKNO_MASK )-blockNo;

    // Is this a free block?

    if( TSC_HPOOL_NBLOCK(blockNo) & TSC_HPOOL_FREELIST_MASK ) {
      ++heapInfo.freeEntries;
      heapInfo.freeBlocks += (TSC_HPOOL_NBLOCK(blockNo) & TSC_HPOOL_BLOCKNO_MASK )-blockNo;

      printf("|0x%" PRIXPTR "|B %5i|NB %5i|PB %5i|Z %5i|                                   |NF %5i|PF %5i|\n",
              (uintptr_t)(&TSC_HPOOL_BLOCK(blockNo)),
              blockNo,
              TSC_HPOOL_NBLOCK(blockNo) & TSC_HPOOL_BLOCKNO_MASK,
              TSC_HPOOL_PBLOCK(blockNo),
              (TSC_HPOOL_NBLOCK(blockNo) & TSC_HPOOL_BLOCKNO_MASK )-blockNo,
              TSC_HPOOL_NFREE(blockNo),
              TSC_HPOOL_PFREE(blockNo) );
     
      // Does this block address match the ptr we may be trying to free?

      if( ptr == &TSC_HPOOL_BLOCK(blockNo) ) {
       
        // Release the critical section...
        //
        TSC_HPOOL_CRITICAL_EXIT();
 
        return( ptr );
      }
    } else {
      ++heapInfo.usedEntries;
      heapInfo.usedBlocks += (TSC_HPOOL_NBLOCK(blockNo) & TSC_HPOOL_BLOCKNO_MASK )-blockNo;

      printf("|0x%" PRIXPTR "|B %5i|NB %5i|PB %5i|Z %5i|PA %5i|CH %5i|NS %5i|PS %5i|\n",
              (uintptr_t)(&TSC_HPOOL_BLOCK(blockNo)),
              blockNo,
              TSC_HPOOL_NBLOCK(blockNo) & TSC_HPOOL_BLOCKNO_MASK,
              TSC_HPOOL_PBLOCK(blockNo),
              (TSC_HPOOL_NBLOCK(blockNo) & TSC_HPOOL_BLOCKNO_MASK )-blockNo,
              TSC_HPOOL_PARENT(blockNo),
              TSC_HPOOL_CHILD(blockNo),
              TSC_HPOOL_NSIBL(blockNo),
              TSC_HPOOL_PSIBL(blockNo));
    }

    blockNo = TSC_HPOOL_NBLOCK(blockNo) & TSC_HPOOL_BLOCKNO_MASK;
  }

  // Update the accounting totals with information from the last block, the
  // rest must be free!

  heapInfo.freeBlocks  += heap->numblocks - blockNo;
  heapInfo.totalBlocks += heap->numblocks - blockNo;

  printf("|0x%" PRIXPTR "|B %5i|NB %5i|PB %5i|Z %5i|                                   |NF %5i|PF %5i|\n",
          (uintptr_t)(&TSC_HPOOL_BLOCK(blockNo)),
          blockNo,
          TSC_HPOOL_NBLOCK(blockNo) & TSC_HPOOL_BLOCKNO_MASK,
          TSC_HPOOL_PBLOCK(blockNo),
          heap->numblocks - blockNo,
          TSC_HPOOL_NFREE(blockNo),
          TSC_HPOOL_PFREE(blockNo) );

  printf("Total Entries %5i    Used Entries %5i    Free Entries %5i\n",
          heapInfo.totalEntries,
          heapInfo.usedEntries,
          heapInfo.freeEntries );

  printf("Total Blocks  %5i    Used Blocks  %5i    Free Blocks  %5i\n",
          heapInfo.totalBlocks,
          heapInfo.usedBlocks,
          heapInfo.freeBlocks  );

  // Release the critical section...
  //
  TSC_HPOOL_CRITICAL_EXIT();
 
  return( NULL );
}

static uint16_t tsc_pool_blocks( size_t size ) {

  // The calculation of the block size is not too difficult, but there are
  // a few little things that we need to be mindful of.
  //
  // When a block removed from the free list, the space used by the free
  // pointers is available for data. That's what the first calculation
  // of size is doing.

  if( size <= (sizeof(((tsc_pool_block *)0)->body)) )
    return( 1 );

  // If it's for more than that, then we need to figure out the number of
  // additional whole blocks the size of an tsc_pool_block are required.

  size -= ( 1 + (sizeof(((tsc_pool_block *)0)->body)) );

  return( 2 + size/(sizeof(tsc_pool_block)) );
}


static uint16_t tsc_hpool_blocks( size_t size ) {

  // The calculation of the block size is not too difficult, but there are
  // a few little things that we need to be mindful of.
  //
  // When a block removed from the free list, the space used by the free
  // pointers is available for data. That's what the first calculation
  // of size is doing.

  if( size <= (sizeof(((tsc_hpool_block *)0)->body)) )
    return( 1 );

  // If it's for more than that, then we need to figure out the number of
  // additional whole blocks the size of an tsc_hpool_block are required.

  size -= ( 1 + (sizeof(((tsc_hpool_block *)0)->body)) );

  return( 2 + size/(sizeof(tsc_hpool_block)) );
}



static void tsc_pool_make_new_block(tsc_pool_t *heap, uint16_t c, uint16_t blocks, uint16_t freemask) {
  TSC_POOL_NBLOCK(c+blocks) = TSC_POOL_NBLOCK(c) & TSC_POOL_BLOCKNO_MASK;
  TSC_POOL_PBLOCK(c+blocks) = c;
  TSC_POOL_PBLOCK(TSC_POOL_NBLOCK(c) & TSC_POOL_BLOCKNO_MASK) = (c+blocks);
  TSC_POOL_NBLOCK(c) = (c+blocks) | freemask;
}

static void tsc_hpool_make_new_block(tsc_hpool_t *heap, uint16_t c, uint16_t blocks, uint16_t freemask) {
  TSC_HPOOL_NBLOCK(c+blocks) = TSC_HPOOL_NBLOCK(c) & TSC_HPOOL_BLOCKNO_MASK;
  TSC_HPOOL_PBLOCK(c+blocks) = c;
  TSC_HPOOL_PARENT(c+blocks) = 0;
  TSC_HPOOL_CHILD(c+blocks)  = 0;
  TSC_HPOOL_NSIBL(c+blocks)  = 0;
  TSC_HPOOL_PSIBL(c+blocks)  = 0;
  TSC_HPOOL_PBLOCK(TSC_HPOOL_NBLOCK(c) & TSC_HPOOL_BLOCKNO_MASK) = (c+blocks);
  TSC_HPOOL_NBLOCK(c) = (c+blocks) | freemask;
}

static void tsc_pool_disconnect_from_free_list(tsc_pool_t *heap, uint16_t c) {
  // Disconnect this block from the FREE list
  TSC_POOL_NFREE(TSC_POOL_PFREE(c)) = TSC_POOL_NFREE(c);
  TSC_POOL_PFREE(TSC_POOL_NFREE(c)) = TSC_POOL_PFREE(c);
  // And clear the free block indicator
  TSC_POOL_NBLOCK(c) &= (~TSC_POOL_FREELIST_MASK);
}

static void tsc_hpool_disconnect_from_free_list(tsc_hpool_t *heap, uint16_t c) {
  // Disconnect this block from the FREE list
  TSC_HPOOL_NFREE(TSC_HPOOL_PFREE(c)) = TSC_HPOOL_NFREE(c);
  TSC_HPOOL_PFREE(TSC_HPOOL_NFREE(c)) = TSC_HPOOL_PFREE(c);
  // And clear the free block indicator
  TSC_HPOOL_NBLOCK(c) &= (~TSC_HPOOL_FREELIST_MASK);
}

static void tsc_pool_assimilate_up(tsc_pool_t *heap, uint16_t c) {
  if( TSC_POOL_NBLOCK(TSC_POOL_NBLOCK(c)) & TSC_POOL_FREELIST_MASK ) {
    // The next block is a free block, so assimilate up and remove it from the free list
    // Disconnect the next block from the FREE list

    tsc_pool_disconnect_from_free_list(heap, TSC_POOL_NBLOCK(c) );

    // Assimilate the next block with this one

    TSC_POOL_PBLOCK(TSC_POOL_NBLOCK(TSC_POOL_NBLOCK(c)) & TSC_POOL_BLOCKNO_MASK) = c;
    TSC_POOL_NBLOCK(c) = TSC_POOL_NBLOCK(TSC_POOL_NBLOCK(c)) & TSC_POOL_BLOCKNO_MASK;
  } 
}

static void tsc_hpool_assimilate_up(tsc_hpool_t *heap, uint16_t c) {
  if( TSC_HPOOL_NBLOCK(TSC_HPOOL_NBLOCK(c)) & TSC_HPOOL_FREELIST_MASK ) {
    // The next block is a free block, so assimilate up and remove it from the free list
    // Disconnect the next block from the FREE list

    tsc_hpool_disconnect_from_free_list(heap, TSC_HPOOL_NBLOCK(c) );

    // Assimilate the next block with this one

    TSC_HPOOL_PBLOCK(TSC_HPOOL_NBLOCK(TSC_HPOOL_NBLOCK(c)) & TSC_HPOOL_BLOCKNO_MASK) = c;
    TSC_HPOOL_NBLOCK(c) = TSC_HPOOL_NBLOCK(TSC_HPOOL_NBLOCK(c)) & TSC_HPOOL_BLOCKNO_MASK;
  } 
}

static uint16_t tsc_pool_assimilate_down(tsc_pool_t *heap,  uint16_t c, uint16_t freemask) {
  TSC_POOL_NBLOCK(TSC_POOL_PBLOCK(c)) = TSC_POOL_NBLOCK(c) | freemask;
  TSC_POOL_PBLOCK(TSC_POOL_NBLOCK(c)) = TSC_POOL_PBLOCK(c);
  return ( TSC_POOL_PBLOCK(c) );
}

static uint16_t tsc_hpool_assimilate_down(tsc_hpool_t *heap,  uint16_t c, uint16_t freemask) {
  TSC_HPOOL_NBLOCK(TSC_HPOOL_PBLOCK(c)) = TSC_HPOOL_NBLOCK(c) | freemask;
  TSC_HPOOL_PBLOCK(TSC_HPOOL_NBLOCK(c)) = TSC_HPOOL_PBLOCK(c);
  return ( TSC_HPOOL_PBLOCK(c) );
}

static void tsc_hpool_freechild(tsc_hpool_t *heap, uint16_t c) {
  if(c) {
    // unlink parent
    TSC_HPOOL_CHILD(TSC_HPOOL_PARENT(c)) = 0;
    uint16_t i, inext;
    for(i = c, inext = TSC_HPOOL_NSIBL(i); 
      i != 0 ; 
      i = inext, inext = TSC_HPOOL_NSIBL(i)) 
    {
      tsc_hpool_free(heap, (void *)&TSC_HPOOL_DATA(i));
    }        
  }
}

static void tsc_hpool_relink_hier(tsc_hpool_t *heap, uint16_t c, uint16_t newc) {
  if(TSC_HPOOL_PSIBL(c)) {
    TSC_HPOOL_PSIBL(newc) = TSC_HPOOL_PSIBL(c);
    TSC_HPOOL_NSIBL(TSC_HPOOL_PSIBL(c)) = newc;
    TSC_HPOOL_PSIBL(c) = 0;
  }
  if(TSC_HPOOL_NSIBL(c)) {
    TSC_HPOOL_NSIBL(newc) = TSC_HPOOL_NSIBL(c);
    TSC_HPOOL_PSIBL(TSC_HPOOL_NSIBL(c)) = newc;
    TSC_HPOOL_NSIBL(c) = 0;
  }
  if(TSC_HPOOL_PARENT(c)) {
    TSC_HPOOL_PARENT(newc) = TSC_HPOOL_PARENT(c);
    if(TSC_HPOOL_CHILD(TSC_HPOOL_PARENT(c)) == c) {
      TSC_HPOOL_CHILD(TSC_HPOOL_PARENT(c)) = newc;
    }
    TSC_HPOOL_PARENT(c) = 0;
  }
  
  
  if(TSC_HPOOL_CHILD(c)) {
    TSC_HPOOL_CHILD(newc) = TSC_HPOOL_CHILD(c);
    for(uint16_t i = TSC_HPOOL_CHILD(c); i != 0 ; i = TSC_HPOOL_NSIBL(i)) {
      TSC_HPOOL_PARENT(i) = newc;
    }
    TSC_HPOOL_CHILD(c) = 0;
  }
}

const char * tsc_pool_init(tsc_pool_t *heap, tsc_pool_block *mem, uint16_t numblocks) {
  if(mem == NULL) {
    heap->heap  = (tsc_pool_block *) malloc(numblocks*sizeof(tsc_pool_block));
    if(heap->heap == NULL) {
      heap->allocd    = 0;
      heap->numblocks = 0;
      return "OOM";
    }    
    heap->allocd= 1;
  } else {
    heap->heap = mem;
    heap->allocd= 0;
  }
  memset(heap->heap, 0, numblocks * sizeof(tsc_pool_block));
  heap->numblocks = numblocks;
  return NULL;
}

const char * tsc_hpool_init(tsc_hpool_t *heap, tsc_hpool_block *mem, uint16_t numblocks) {
  if(mem == NULL) {
    heap->heap  = (tsc_hpool_block *) malloc(numblocks*sizeof(tsc_hpool_block));
    if(heap->heap == NULL) {
      heap->allocd    = 0;
      heap->numblocks = 0;
      return "OOM";
    }
    heap->allocd= 1;
  } else {
    heap->heap  = mem;
    heap->allocd= 0;
  }
  memset(heap->heap, 0, numblocks * sizeof(tsc_hpool_block));
  heap->numblocks = numblocks;
  return NULL;
}

void tsc_pool_deinit(tsc_pool_t *heap) {
  if(heap->allocd)
    free(heap->heap);
  memset(heap, 0, sizeof(tsc_pool_t));
}

void tsc_hpool_deinit(tsc_hpool_t *heap) {
  if(heap->allocd)
    free(heap->heap);
  memset(heap, 0, sizeof(tsc_hpool_t));
}

void tsc_pool_free(tsc_pool_t *heap, void *ptr) {
  uint16_t c;

  // If we're being asked to free a NULL pointer, well that's just silly!

  if( NULL == ptr )
    return;
  
  // FIXME: At some point it might be a good idea to add a check to make sure
  //        that the pointer we're being asked to free up is actually within
  //        the tsc_pool_heap!
  //
  // NOTE:  See the new tsc_pool_info() function that you can use to see if a ptr is
  //        on the free list!

  // Protect the critical section...
  //
  TSC_POOL_CRITICAL_ENTRY();
  
  // Figure out which block we're in. Note the use of truncated division...

  c = ((char *)ptr-(char *)(&(heap->heap[0])))/sizeof(tsc_pool_block);

  // Now let's assimilate this block with the next one if possible.
  
  tsc_pool_assimilate_up(heap, c );

  // Then assimilate with the previous block if possible

  if( TSC_POOL_NBLOCK(TSC_POOL_PBLOCK(c)) & TSC_POOL_FREELIST_MASK ) {
    c = tsc_pool_assimilate_down(heap, c, TSC_POOL_FREELIST_MASK);
  } else {
    // The previous block is not a free block, so add this one to the head of the free list
    TSC_POOL_PFREE(TSC_POOL_NFREE(0)) = c;
    TSC_POOL_NFREE(c)   = TSC_POOL_NFREE(0);
    TSC_POOL_PFREE(c)   = 0;
    TSC_POOL_NFREE(0)   = c;
    TSC_POOL_NBLOCK(c) |= TSC_POOL_FREELIST_MASK;
  }

  // Release the critical section...
  //
  TSC_POOL_CRITICAL_EXIT();
}


void tsc_hpool_free(tsc_hpool_t *heap, void *ptr) {
  uint16_t c;

  // If we're being asked to free a NULL pointer, well that's just silly!

  if( NULL == ptr )
    return;
  
  // FIXME: At some point it might be a good idea to add a check to make sure
  //        that the pointer we're being asked to free up is actually within
  //        the tsc_hpool_heap!
  //
  // NOTE:  See the new tsc_hpool_info() function that you can use to see if a ptr is
  //        on the free list!

  // Figure out which block we're in. Note the use of truncated division...

  c = ((char *)ptr-(char *)(&(heap->heap[0])))/sizeof(tsc_hpool_block);
  
  // relink sibling
  if(TSC_HPOOL_CHILD(TSC_HPOOL_PARENT(c)) == c) TSC_HPOOL_CHILD(TSC_HPOOL_PARENT(c)) = TSC_HPOOL_NSIBL(c);
  if(TSC_HPOOL_PSIBL(c)) TSC_HPOOL_NSIBL(TSC_HPOOL_PSIBL(c)) = TSC_HPOOL_NSIBL(c);
  if(TSC_HPOOL_NSIBL(c)) TSC_HPOOL_PSIBL(TSC_HPOOL_NSIBL(c)) = TSC_HPOOL_PSIBL(c);


  // free all children recursively
  tsc_hpool_freechild(heap, TSC_HPOOL_CHILD(c));
  
  // Protect the critical section...
  //
  TSC_HPOOL_CRITICAL_ENTRY();

  // Now let's assimilate this block with the next one if possible.
  
  tsc_hpool_assimilate_up(heap, c );

  // Then assimilate with the previous block if possible

  if( TSC_HPOOL_NBLOCK(TSC_HPOOL_PBLOCK(c)) & TSC_HPOOL_FREELIST_MASK ) {
    c = tsc_hpool_assimilate_down(heap, c, TSC_HPOOL_FREELIST_MASK);
  } else {
    // The previous block is not a free block, so add this one to the head of the free list
    TSC_HPOOL_PFREE(TSC_HPOOL_NFREE(0)) = c;
    TSC_HPOOL_NFREE(c)   = TSC_HPOOL_NFREE(0);
    TSC_HPOOL_PFREE(c)   = 0;
    TSC_HPOOL_NFREE(0)   = c;
    TSC_HPOOL_NBLOCK(c) |= TSC_HPOOL_FREELIST_MASK;
    TSC_HPOOL_NSIBL(c)   = 0;
    TSC_HPOOL_PSIBL(c)   = 0;    
  }

  // Release the critical section...
  //
  TSC_HPOOL_CRITICAL_EXIT();
}

void tsc_pool_freeall(tsc_pool_t *heap) {
  memset(heap->heap, 0, heap->numblocks * sizeof(tsc_pool_block));
}

void tsc_hpool_freeall(tsc_hpool_t *heap) {
  memset(heap->heap, 0, heap->numblocks * sizeof(tsc_hpool_block));
}

void * tsc_pool_malloc(tsc_pool_t *heap, size_t size) {
  uint16_t          blocks;
  volatile uint16_t blockSize;  // why is this volatile (for thread safety?)
  uint16_t          bestSize;
  uint16_t          bestBlock;
  uint16_t          cf;

  // the very first thing we do is figure out if we're being asked to allocate
  // a size of 0 - and if we are we'll simply return a null pointer. if not
  // then reduce the size by 1 byte so that the subsequent calculations on
  // the number of blocks to allocate are easier...

  if( 0 == size )
    return NULL;

  TSC_POOL_CRITICAL_ENTRY();

  blocks = tsc_pool_blocks( size );

  // Now we can scan through the free list until we find a space that's big
  // enough to hold the number of blocks we need.
  //
  // This part may be customized to be a best-fit, worst-fit, or first-fit
  // algorithm

  cf = TSC_POOL_NFREE(0);

  bestBlock = TSC_POOL_NFREE(0);
  bestSize  = 0x7FFF;

  while( TSC_POOL_NFREE(cf) ) {
    blockSize = (TSC_POOL_NBLOCK(cf) & TSC_POOL_BLOCKNO_MASK) - cf;

#if defined TSC_POOL_FIRST_FIT
    // This is the first block that fits!
    if( (blockSize >= blocks) )
        break;
#elif defined TSC_POOL_BEST_FIT
    if( (blockSize >= blocks) && (blockSize < bestSize) ) {
      bestBlock = cf;
      bestSize  = blockSize;
      if(blockSize == blocks)   // perfect size found, not need to keep searching
        break;
    }
#endif

    cf = TSC_POOL_NFREE(cf);
  }

  if( 0x7FFF != bestSize ) {
    cf        = bestBlock;
    blockSize = bestSize;
  }

  if( TSC_POOL_NBLOCK(cf) & TSC_POOL_BLOCKNO_MASK ) {
    // This is an existing block in the memory heap, we just need to split off
    // what we need, unlink it from the free list and mark it as in use, and
    // link the rest of the block back into the freelist as if it was a new
    // block on the free list...

    if( blockSize == blocks ) {
      // It's an exact fit and we don't neet to split off a block.
      // Disconnect this block from the FREE list

      tsc_pool_disconnect_from_free_list( heap, cf );

    } else {
     // It's not an exact fit and we need to split off a block.

     tsc_pool_make_new_block(heap, cf, blockSize-blocks, TSC_POOL_FREELIST_MASK );

     cf += blockSize-blocks;
     }
  } else {
    // We're at the end of the heap - allocate a new block, but check to see if
    // there's enough memory left for the requested block! Actually, we may need
    // one more than that if we're initializing the tsc_pool_heap for the first
    // time, which happens in the next conditional...

    if( heap->numblocks <= cf+blocks+1 ) {
      TSC_POOL_CRITICAL_EXIT();
      return NULL;
    }

    // Now check to see if we need to initialize the free list...this assumes
    // that the BSS is set to 0 on startup. We should rarely get to the end of
    // the free list so this is the "cheapest" place to put the initialization!

    if( 0 == cf ) {
      TSC_POOL_NBLOCK(0) = 1;
      TSC_POOL_NFREE(0)  = 1;
      cf            = 1;
    }

    TSC_POOL_NFREE(TSC_POOL_PFREE(cf)) = cf+blocks;

    memcpy( &TSC_POOL_BLOCK(cf+blocks), &TSC_POOL_BLOCK(cf), sizeof(tsc_pool_block) );

    TSC_POOL_NBLOCK(cf)           = cf+blocks;
    TSC_POOL_PBLOCK(cf+blocks)    = cf;
  }

  // Release the critical section...
  //
  TSC_POOL_CRITICAL_EXIT();

  return( (void *)&TSC_POOL_DATA(cf) );
}

void * tsc_hpool_malloc(tsc_hpool_t *heap, size_t size) {
  uint16_t          blocks;
  volatile uint16_t blockSize;  // why is this volatile (for thread safety?)
  uint16_t          bestSize;
  uint16_t          bestBlock;
  uint16_t          cf;

  // the very first thing we do is figure out if we're being asked to allocate
  // a size of 0 - and if we are we'll simply return a null pointer. if not
  // then reduce the size by 1 byte so that the subsequent calculations on
  // the number of blocks to allocate are easier...

  if( 0 == size )
    return NULL;

  TSC_HPOOL_CRITICAL_ENTRY();

  blocks = tsc_hpool_blocks( size );

  // Now we can scan through the free list until we find a space that's big
  // enough to hold the number of blocks we need.
  //
  // This part may be customized to be a best-fit, worst-fit, or first-fit
  // algorithm

  cf = TSC_HPOOL_NFREE(0);

  bestBlock = TSC_HPOOL_NFREE(0);
  bestSize  = 0x7FFF;

  while( TSC_HPOOL_NFREE(cf) ) {
    blockSize = (TSC_HPOOL_NBLOCK(cf) & TSC_HPOOL_BLOCKNO_MASK) - cf;

#if defined TSC_HPOOL_FIRST_FIT
    // This is the first block that fits!
    if( (blockSize >= blocks) )
        break;
#elif defined TSC_HPOOL_BEST_FIT
    if( (blockSize >= blocks) && (blockSize < bestSize) ) {
      bestBlock = cf;
      bestSize  = blockSize;
      if(blockSize == blocks)   // perfect size found, not need to keep searching
        break;
    }
#endif

    cf = TSC_HPOOL_NFREE(cf);
  }

  if( 0x7FFF != bestSize ) {
    cf        = bestBlock;
    blockSize = bestSize;
  }

  if( TSC_HPOOL_NBLOCK(cf) & TSC_HPOOL_BLOCKNO_MASK ) {
    // This is an existing block in the memory heap, we just need to split off
    // what we need, unlink it from the free list and mark it as in use, and
    // link the rest of the block back into the freelist as if it was a new
    // block on the free list...

    if( blockSize == blocks ) {
      // It's an exact fit and we don't neet to split off a block.
      // Disconnect this block from the FREE list

      tsc_hpool_disconnect_from_free_list( heap, cf );

    } else {
     // It's not an exact fit and we need to split off a block.

     tsc_hpool_make_new_block(heap, cf, blockSize-blocks, TSC_HPOOL_FREELIST_MASK );

     cf += blockSize-blocks;
     }
  } else {
    // We're at the end of the heap - allocate a new block, but check to see if
    // there's enough memory left for the requested block! Actually, we may need
    // one more than that if we're initializing the tsc_hpool_heap for the first
    // time, which happens in the next conditional...

    if( heap->numblocks <= cf+blocks+1 ) {
      TSC_HPOOL_CRITICAL_EXIT();
      return NULL;
    }

    // Now check to see if we need to initialize the free list...this assumes
    // that the BSS is set to 0 on startup. We should rarely get to the end of
    // the free list so this is the "cheapest" place to put the initialization!

    if( 0 == cf ) {
      TSC_HPOOL_NBLOCK(0) = 1;
      TSC_HPOOL_NFREE(0)  = 1;
      cf            = 1;
    }

    TSC_HPOOL_NFREE(TSC_HPOOL_PFREE(cf)) = cf+blocks;

    memcpy( &TSC_HPOOL_BLOCK(cf+blocks), &TSC_HPOOL_BLOCK(cf), sizeof(tsc_hpool_block) );

    TSC_HPOOL_NBLOCK(cf)           = cf+blocks;
    TSC_HPOOL_PBLOCK(cf+blocks)    = cf;
  }

  // Release the critical section...
  //
  TSC_HPOOL_CRITICAL_EXIT();

  return( (void *)&TSC_HPOOL_DATA(cf) );
}

void * tsc_pool_calloc(tsc_pool_t *heap, size_t n, size_t len) {
  size_t sz = len * n;
  void *p = tsc_pool_malloc(heap, sz);
  return p ? memset(p, 0, sz) : NULL;
}

void * tsc_hpool_calloc(tsc_hpool_t *heap, size_t n, size_t len) {
  size_t sz = len * n;
  void *p = tsc_hpool_malloc(heap, sz);
  return p ? memset(p, 0, sz) : NULL;
}

void * tsc_pool_realloc(tsc_pool_t *heap, void *ptr, size_t size ) {
  uint16_t  blocks;
  uint16_t  blockSize;
  uint16_t  c;
  size_t    curSize;

  // This code looks after the case of a NULL value for ptr. The ANSI C
  // standard says that if ptr is NULL and size is non-zero, then we've
  // got to work the same a malloc(). If size is also 0, then our version
  // of malloc() returns a NULL pointer, which is OK as far as the ANSI C
  // standard is concerned.

  if( NULL == ptr )
    return tsc_pool_malloc(heap, size);

  // Now we're sure that we have a non_NULL ptr, but we're not sure what
  // we should do with it. If the size is 0, then the ANSI C standard says that
  // we should operate the same as free.

  if( 0 == size ) {
    tsc_pool_free(heap, ptr);
    return NULL;
  }

  // Protect the critical section...
  //
  TSC_POOL_CRITICAL_ENTRY();

  // Otherwise we need to actually do a reallocation. A naiive approach
  // would be to malloc() a new block of the correct size, copy the old data
  // to the new block, and then free the old block.
  //
  // While this will work, we end up doing a lot of possibly unnecessary
  // copying. So first, let's figure out how many blocks we'll need.

  blocks = tsc_pool_blocks( size );

  // Figure out which block we're in. Note the use of truncated division...

  c = ((char *)ptr-(char *)(&(heap->heap[0])))/sizeof(tsc_pool_block);

  // Figure out how big this block is...

  blockSize = (TSC_POOL_NBLOCK(c) - c);

  // Figure out how many bytes are in this block
    
  curSize   = (blockSize*sizeof(tsc_pool_block))-(sizeof(((tsc_pool_block *)0)->header));

  // Ok, now that we're here, we know the block number of the original chunk
  // of memory, and we know how much new memory we want, and we know the original
  // block size...

  if( blockSize == blocks ) {
    // This space intentionally left blank - return the original pointer!

    // Release the critical section...
    //
    TSC_POOL_CRITICAL_EXIT();

    return ptr;
  }

  // Now we have a block size that could be bigger or smaller. Either
  // way, try to assimilate up to the next block before doing anything...
  //
  // If it's still too small, we have to free it anyways and it will save the
  // assimilation step later in free :-)

  tsc_pool_assimilate_up(heap, c );

  // Now check if it might help to assimilate down, but don't actually
  // do the downward assimilation unless the resulting block will hold the
  // new request! If this block of code runs, then the new block will
  // either fit the request exactly, or be larger than the request.

  if( (TSC_POOL_NBLOCK(TSC_POOL_PBLOCK(c)) & TSC_POOL_FREELIST_MASK) &&
      (blocks <= (TSC_POOL_NBLOCK(c)-TSC_POOL_PBLOCK(c)))    ) {
  
    // Check if the resulting block would be big enough...

    // Disconnect the previous block from the FREE list

    tsc_pool_disconnect_from_free_list(heap, TSC_POOL_PBLOCK(c) );

    // Connect the previous block to the next block ... and then
    // realign the current block pointer

    c = tsc_pool_assimilate_down(heap, c, 0);

    // Move the bytes down to the new block we just created, but be sure to move
    // only the original bytes.

    memmove( (void *)&TSC_POOL_DATA(c), ptr, curSize );
 
    // And don't forget to adjust the pointer to the new block location!
        
    ptr = (void *)&TSC_POOL_DATA(c);
  }

  // Now calculate the block size again...and we'll have three cases

  blockSize = (TSC_POOL_NBLOCK(c) - c);

  if( blockSize == blocks ) {
    // This space intentionally left blank - return the original pointer!

  } else if (blockSize > blocks ) {
    // New block is smaller than the old block, so just make a new block
    // at the end of this one and put it up on the free list...

    tsc_pool_make_new_block(heap, c, blocks, 0 );
    
    tsc_pool_free(heap, (void *)&TSC_POOL_DATA(c+blocks) );
  } else {
    // New block is bigger than the old block...
    
    void *oldptr = ptr;

    // Now tsc_pool_malloc() a new/ one, copy the old data to the new block, and
    // free up the old block, but only if the malloc was sucessful!

    if( (ptr = tsc_pool_malloc(heap, size)) ) {
      memcpy( ptr, oldptr, curSize );
      tsc_pool_free(heap, oldptr );
    }
    
  }

  // Release the critical section...
  //
  TSC_POOL_CRITICAL_EXIT();

  return( ptr );
}

void * tsc_hpool_realloc(tsc_hpool_t *heap, void *ptr, size_t size ) {
  uint16_t  blocks;
  uint16_t  blockSize;
  uint16_t  c, newc;
  size_t    curSize;

  // This code looks after the case of a NULL value for ptr. The ANSI C
  // standard says that if ptr is NULL and size is non-zero, then we've
  // got to work the same a malloc(). If size is also 0, then our version
  // of malloc() returns a NULL pointer, which is OK as far as the ANSI C
  // standard is concerned.

  if( NULL == ptr )
    return tsc_hpool_malloc(heap, size);

  // Now we're sure that we have a non_NULL ptr, but we're not sure what
  // we should do with it. If the size is 0, then the ANSI C standard says that
  // we should operate the same as free.

  if( 0 == size ) {
    tsc_hpool_free(heap, ptr);
    return NULL;
  }

  // Protect the critical section...
  //
  TSC_HPOOL_CRITICAL_ENTRY();

  // Otherwise we need to actually do a reallocation. A naiive approach
  // would be to malloc() a new block of the correct size, copy the old data
  // to the new block, and then free the old block.
  //
  // While this will work, we end up doing a lot of possibly unnecessary
  // copying. So first, let's figure out how many blocks we'll need.

  blocks = tsc_hpool_blocks( size );

  // Figure out which block we're in. Note the use of truncated division...

  c = ((char *)ptr-(char *)(&(heap->heap[0])))/sizeof(tsc_hpool_block);

  // Figure out how big this block is...

  blockSize = (TSC_HPOOL_NBLOCK(c) - c);

  // Figure out how many bytes are in this block
    
  curSize   = (blockSize*sizeof(tsc_hpool_block))-(sizeof(((tsc_hpool_block *)0)->header));

  // Ok, now that we're here, we know the block number of the original chunk
  // of memory, and we know how much new memory we want, and we know the original
  // block size...

  if( blockSize == blocks ) {
    // This space intentionally left blank - return the original pointer!

    // Release the critical section...
    //
    TSC_HPOOL_CRITICAL_EXIT();

    return ptr;
  }

  // Now we have a block size that could be bigger or smaller. Either
  // way, try to assimilate up to the next block before doing anything...
  //
  // If it's still too small, we have to free it anyways and it will save the
  // assimilation step later in free :-)

  tsc_hpool_assimilate_up(heap, c );

  // Now check if it might help to assimilate down, but don't actually
  // do the downward assimilation unless the resulting block will hold the
  // new request! If this block of code runs, then the new block will
  // either fit the request exactly, or be larger than the request.

  if( (TSC_HPOOL_NBLOCK(TSC_HPOOL_PBLOCK(c)) & TSC_HPOOL_FREELIST_MASK) &&
      (blocks <= (TSC_HPOOL_NBLOCK(c)-TSC_HPOOL_PBLOCK(c)))    ) {
  
    // Check if the resulting block would be big enough...

    // Disconnect the previous block from the FREE list

    tsc_hpool_disconnect_from_free_list(heap, TSC_HPOOL_PBLOCK(c) );

    // Connect the previous block to the next block ... and then
    // realign the current block pointer

    newc = tsc_hpool_assimilate_down(heap, c, 0);

    tsc_hpool_relink_hier(heap, c, newc);
        
    // Move the bytes down to the new block we just created, but be sure to move
    // only the original bytes.

    memmove( (void *)&TSC_HPOOL_DATA(newc), ptr, curSize );
 
    // And don't forget to adjust the pointer to the new block location!
        
    ptr = (void *)&TSC_HPOOL_DATA(newc);
    c = newc;
  }

  // Now calculate the block size again...and we'll have three cases

  blockSize = (TSC_HPOOL_NBLOCK(c) - c);

  if( blockSize == blocks ) {
    // This space intentionally left blank - return the original pointer!

  } else if (blockSize > blocks ) {
    // New block is smaller than the old block, so just make a new block
    // at the end of this one and put it up on the free list...

    tsc_hpool_make_new_block(heap, c, blocks, 0 );
    
    tsc_hpool_free(heap, (void *)&TSC_HPOOL_DATA(c+blocks) );
  } else {
    // New block is bigger than the old block...
    
    void *oldptr = ptr;

    // Now tsc_hpool_malloc() a new/ one, copy the old data to the new block, and
    // free up the old block, but only if the malloc was sucessful!

    if( (ptr = tsc_hpool_malloc(heap, size)) ) {
      newc = ((char *)ptr-(char *)(&(heap->heap[0])))/sizeof(tsc_hpool_block);
      
      tsc_hpool_relink_hier(heap, c, newc);
      
      memcpy( ptr, oldptr, curSize );
      tsc_hpool_free(heap, oldptr );
    }
    
  }

  // Release the critical section...
  //
  TSC_HPOOL_CRITICAL_EXIT();

  return( ptr );
}

void tsc_hpool_attach(tsc_hpool_t *heap, void *ptr, void *parent) {
  uint16_t c, cparent;
  
  if(NULL == ptr)
    return;
  
  c = ((char *)ptr-(char *)(&(heap->heap[0])))/sizeof(tsc_hpool_block);
  cparent = ((char *)parent-(char *)(&(heap->heap[0])))/sizeof(tsc_hpool_block);
  
  // relink sibling
  if(TSC_HPOOL_PSIBL(c)) TSC_HPOOL_NSIBL(TSC_HPOOL_PSIBL(c)) = TSC_HPOOL_NSIBL(c);
  if(TSC_HPOOL_NSIBL(c)) TSC_HPOOL_PSIBL(TSC_HPOOL_NSIBL(c)) = TSC_HPOOL_PSIBL(c);
  
  // detach parent
  if(TSC_HPOOL_CHILD(TSC_HPOOL_PARENT(c)) == c) {
    TSC_HPOOL_CHILD(TSC_HPOOL_PARENT(c)) = TSC_HPOOL_NSIBL(c);
    
  }
  
  // attach to new parent
  TSC_HPOOL_PARENT(c) = cparent;
  TSC_HPOOL_PSIBL(c) = 0;
  TSC_HPOOL_NSIBL(c) = TSC_HPOOL_CHILD(cparent);
  TSC_HPOOL_PSIBL(TSC_HPOOL_CHILD(cparent)) = c;
  TSC_HPOOL_CHILD(cparent) = c;
}

// these functions are kind of interesting.
// they use up a non-trivial amount of re-direction pointer memory at the beginning in order to 
// "emulate" static multi-dimensional pointer accesses

// these are the sort of things that the compiler provides for you for free for 
// static multi-dimension matrices. Although, the static matrices must be regular. 
// whereas these functions can make optimal irregular matrices

// once you are willing to accept the additional memory usage (which isn't too bad) 
// a couple nice side effects jump out:
//  1. you don't need a nested loop to clean out this matrix. A single free is enough.
//  2. it turns out, if your matrices are big, you can't use static matrices anyway inside functions b/c it chews up stack space.
//      and you STILL can not use globals b/c of potential ownership issues. So you will have to pay the penalty of multiple malloc/free
//  3. Because you are mallocing one big memory, you don't get fragmentation and you get nice cacheing effects
//  4. and to make it even better, b/c you are repeated access the indirection header. 
//      They will mostly be in the cache as you traverse down the matrix 
//      (meaning access mat3d[z][0] will also fetch mat3d[z][1], mat3d[z][2], etc...
//  5. it is easy to shallow copy these matrices (just a memcpy)

// this matrix's performance will heavily depend on traversal direction. The cache-ing effect will only help
// if you traverse in x then y then z. If you traverse in y or in z first, 
// you may have to constantly move data in and out of the cache.
// although this is a problem generally for any array-based matrices structure

// the biggest problem with these matrices data structure is that they are fixed in stone.
// you have to known a priori what the size shall be. 
// in practice, this isn't a problem though. 
// If I need to do a lot of low level data traversals that will justify the use of this datastructure
// then I better know what the memory usage ought to be, and if I don't, 
// an array based datastructure is probably a bad idea anyway.

const char* tsc_alloc2d(void ***ret, size_t y, size_t x, size_t sz) {
  size_t header = y*sizeof(void*);
  size_t body   = y*x*sz;
  if( (*ret = (void **) malloc(header + body)) == NULL )
    return "OOM";
  
  (*ret)[0] = (void *) ((char *) (*ret) + header);
  for(size_t j = 1 ; j < y ; j++) {
    (*ret)[j] = (void *) ((char *) (*ret)[j-1] + x * sz);
  }    
  return NULL;
}

const char* tsc_alloc2d_irregular(void ***ret, size_t y, size_t * x_per_y, size_t sz) {
  size_t header   = y*sizeof(void*);
  size_t tot_elem = 0;
  for(size_t j = 0 ; j < y ; j++) {
    tot_elem += x_per_y[j];
  }
  
  if( (*ret = (void **) malloc(header + tot_elem * sz)) == NULL )
    return "OOM";
  
  (*ret)[0] = (void *) ((char *) (*ret) + header);
  for(size_t j = 1 ; j < y ; j++) {
    (*ret)[j] = (void *) ((char *) (*ret)[j-1] + x_per_y[j-1] * sz);
  }

  return NULL;
}

const char* tsc_alloc3d(void ****ret, size_t z, size_t y, size_t x, size_t sz) {
  size_t header  = z*(y+1)*sizeof(void*);
  size_t body    = z*y*x*sz;
  if( (*ret = (void ***) malloc(header + body)) == NULL) 
    return "OOM";
  
  for(size_t k = 0 ; k < z ; k++) {
    (*ret)[k] = (void **) ((char *) (*ret) + z*sizeof(void*) + k * y * sizeof(void*));
    for(size_t j = 0 ; j < y ; j++) {
      (*ret)[k][j] = (void *) ((char *) (*ret) + header + k*y*x*sz + j*x*sz);
    }
  }
  return NULL;
}

const char* tsc_alloc3d_irregular(void ****ret, size_t z, size_t y, size_t ** x_per_z_y, size_t sz) {
  size_t header   = z*(y+1)*sizeof(void*);
  size_t tot_elem = 0;
  for(size_t k = 0 ; k < z ; k++) {
    for(size_t j = 0 ; j < y ; j++) {
      tot_elem += x_per_z_y[k][j];
    }  
  }
  
  if( (*ret = (void ***) malloc(header + tot_elem * sz)) == NULL )
    return "OOM";
  
  for(size_t k = 0 ; k < z ; k++) {
    (*ret)[k]     = (void **) ((char *) (*ret) + z*sizeof(void*) + k * y * sizeof(void*));
    (*ret)[k][0]  = (void *)  ((char *) (*ret) + header);
    for(size_t j = 1 ; j < y ; j++) {
      (*ret)[k][j] = (void *) ((char *) (*ret)[k][j-1] + x_per_z_y[k][j-1]*sz);
    }
  }
  return NULL;
}

const char* tsc_base64_enc(char **ret, char *data, size_t sz) {
  static char base64_enc_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                    'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                    '4', '5', '6', '7', '8', '9', '+', '/'};
  static int  mod_table[] = {0, 2, 1};
  size_t i, j;
  size_t enc_sz = 4 * ((sz + 2) / 3);

  tsc_unlikely_if( (*ret = (char *) malloc(enc_sz+1)) == NULL )
    return "OOM";
  
  (*ret)[enc_sz] = '\0';
  
  for (i = 0, j = 0; i < sz;) {
    char octet_a = i < sz ? data[i++] : 0;
    char octet_b = i < sz ? data[i++] : 0;
    char octet_c = i < sz ? data[i++] : 0;

    uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

    (*ret)[j++] = base64_enc_table[(triple >> 3 * 6) & 0x3F];
    (*ret)[j++] = base64_enc_table[(triple >> 2 * 6) & 0x3F];
    (*ret)[j++] = base64_enc_table[(triple >> 1 * 6) & 0x3F];
    (*ret)[j++] = base64_enc_table[(triple >> 0 * 6) & 0x3F];
  }
  
  for (int i = 0; i < mod_table[sz % 3]; i++)
    (*ret)[enc_sz - 1 - i] = '=';
  
  return NULL;
}

const char* tsc_base64_dec(char **ret, size_t* retsz, char *data, size_t datsz) {
  static char base64_dec_table[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,    
  };
  size_t i, j;
  tsc_unlikely_if(datsz % 4 != 0)
    return "tsc_base64_dec invalid data size";
  
  *retsz = datsz / 4 * 3;
  if(data[datsz-1] == '=') --(*retsz);
  if(data[datsz-2] == '=') --(*retsz);
  
  tsc_unlikely_if( (*ret = (char *) malloc(*retsz+1)) == NULL )
    return "OOM";
  
  (*ret)[*retsz] = '\0';
  
  for (i = 0, j = 0; i < datsz;) {
    char sextet_a = data[i] == '=' ? 0 : base64_dec_table[(unsigned char)data[i]]; ++i;
    char sextet_b = data[i] == '=' ? 0 : base64_dec_table[(unsigned char)data[i]]; ++i;
    char sextet_c = data[i] == '=' ? 0 : base64_dec_table[(unsigned char)data[i]]; ++i;
    char sextet_d = data[i] == '=' ? 0 : base64_dec_table[(unsigned char)data[i]]; ++i;
    
    if((sextet_a | sextet_b | sextet_c | sextet_d) & 0x80) {
      free(*ret);
      *ret = NULL;
      return "tsc_base64_dec invalid data";
    }
    
    uint32_t triple = (sextet_a << 3 * 6) | (sextet_b << 2 * 6) | 
                      (sextet_c << 1 * 6) | (sextet_d << 0 * 6) ;

    if (j < *retsz) (*ret)[j++] = (triple >> 2 * 8) & 0xFF;
    if (j < *retsz) (*ret)[j++] = (triple >> 1 * 8) & 0xFF;
    if (j < *retsz) (*ret)[j++] = (triple >> 0 * 8) & 0xFF;
  }
  
  return NULL;
}

//{
#endif
//{
#endif
