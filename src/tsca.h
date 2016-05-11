#ifdef NEVER
  case $0 in
  *.h)  ;;
  bash) echo 'Use "bash *.h" and not "bash < *.h"' >&2; exit 1;;
  sh)   echo 'Use "sh *.h" and not "sh < *.h"' >&2; exit 1;;
  *)    echo 'Filename must end in ".h"' >&2; exit 1;;
  esac
  
  build=0; run=0; clean=0;
  case $1 in
  "")     build=1 ; run=1;;
  build)  build=1;;
  run)    run=1;;
  clean)  clean=1;;
  *)      echo 'Unrecognized command' >&2; exit 1;;
  esac
  
  if [ "$build" -ne "0" ] ; then
    echo Compiling $0:
    echo ${CC=cc} -std=c11 -DTSC_DEFINE -DTSC_MAIN -x c ${0} -o ${0%.*}
    ${CC=cc} -std=c11 -DTSC_DEFINE -DTSC_MAIN -x c ${0} -o ${0%.*}
    chmod +x ${0%.*}
  fi
  
  if [ "$run" -ne "0" ]; then
    echo 
    echo output:
    ./${0%.*}
    rm -f ./${0%.*}
  fi
  
  if [ "$clean" -ne "0" ]; then
    rm -f ${0%.*}
  fi
  
  exit 0
#endif

/// tsca.h 
/// Public Domain -- no warranty is offered or implied (see https://wiki.creativecommons.org/wiki/CC0)
///
/// However, portions of this code is motivated directly / indirectly from other code base with permissive license
/// They are listed in LICENSE.txt and for compliance it is best to include that in your LICENSE.txt if you choose to use tsc.h
///
/// This file is just like tsc.h except that fatal errors (like OOM) are automatically aborted. 
/// There are no conflicts with tsc.h (in fact tsca.h includes tsc.h)

#ifndef TSC__INCLUDE_TSCA_H
#define TSC__INCLUDE_TSCA_H
//}
////////////////////////////////////////
// Global headers                     //
//{/////////////////////////////////////

#include "tsc.h"

//}/////////////////////////////////////
// tsca_vec define                    //
//{/////////////////////////////////////


#define tsca_roundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))
#define tsca_vec_type(name)         tsc_vec_##name##_t
#define tsca_vec_define(name, type) typedef struct { size_t n, m; type *a; } tsca_vec_##name##_t;
#define tsca_vec_compact(name, v)   tsc_vec_resize(name, v, (v).n)
#define tsca_vec_init(name, v)      ((v).n = (v).m = 0, (v).a = 0)
#define tsca_vec_destroy(name, v)   free((v).a)
#define tsca_vec_A(name, v, i)      ((v).a[(i)])
#define tsca_vec_pop(name, v)       ((v).a[--(v).n])
#define tsca_vec_size(name, v)      ((v).n)
#define tsca_vec_max(name, v)       ((v).m)
#define tsca_vec_ptr(name, v)       ((v).a)
#define tsca_vec_clear(name, v)     ((v).n = 0)
#define tsca_vec_first(name, v)     ((v).a[0])
#define tsca_vec_last(name, v)      ((v).a[(v).n-1])

#define tsca_vec_resize(name, v, s) do {                                                      \
  tsc_unlikely_if(reallocf((v).a, sizeof(tsca_vec_##name##_t)*(s))) == NULL ) abort();        \
  (v).m = (s); } while(0)        
#define tsca_vec_copy(name, d, s) do {                                                        \
  tsc_unlikely_if((d).a == (s).a) break;                                                      \
  if ((d).m < (s).n) tsca_vec_resize(name, (d), (s).n);                                       \
  (d).n = (s).n;                                                                              \
  memcpy((d).a, (s).a, sizeof(tsca_vec_##name##_t) * (s).n); } while(0)
#define tsca_vec_push(name, v, x) do {                                                        \
  if((v).n == (v).m) {                                                                        \
    (v).m = (v).m ? (v).m << 1 : 4;                                                           \
    tsca_vec_resize(name, v, (v).m);                                                          \
  }                                                                                           \
  (v).a[(v).n++]= (x); } while(0)      
#define tsca_vec_extend(name, d, s) do {                                                      \
  size_t n = (d).n + (s).n;                                                                   \
  if((d).m < n) tsca_vec_resize(name, (d), n);                                                \
  memcpy((d).a + (d).n, (s).a, sizeof(tsca_vec_##name##_t) * (s).n);                          \
  (d).n = n; } while(0)
#define tsca_vec_reverse(name, v) do {                                                        \
  tsca_vec_##name##_t tmp;                                                                    \
  for( size_t i = ((v).n-1) >> 1 ; (i + 1) > 0 ; --i) {                                       \
    tmp = (v).a[i]; (v).a[i] = (v).a[(v).n - i - 1]; (v).a[(v).n - i - 1] = tmp;              \
  } } while(0)
#define tsc_vec_splice(v, start, count) do {                                                  \
    tsc_unlikely_if(count == 0) break;                                                        \
    tsc_unlikely_if(start > (v).n) {                                                          \
      (v).n = 0;                                                                              \
      break;                                                                                  \
    }                                                                                         \
    tsc_unlikely_if(start + count > (v).n) {                                                  \
      (v).n = start;                                                                          \
      break;                                                                                  \
    }                                                                                         \
    memmove((v).a + start, (v).a + start + count,                                             \
      sizeof((v).a[0]) * ((v).n - start - count));                                            \
    (v).n -= count;                                                                           \
  } while(0)        
#define tsc_vec_foreach(v, var, iter)                                                         \
  tsc_likely_if( (v).n > 0 )                                                                  \
    for ( (iter) = 0; ((iter) < (v).n) && (((var) = (v).a[(iter)]), 1); ++(iter))       
#define tsc_vec_foreach_rev(v, var, iter)                                                     \
  tsc_likely_if( (v).n > 0 )                                                                  \
    for ( (iter) = v.n - 1 ; ((iter + 1) > 0) && (((var) = (v).a[(iter)]), 1); --(iter))    
#define tsc_vec_foreach_ptr(v, var, iter)                                                     \
  tsc_likely_if( (v).n > 0 )                                                                  \
    for ( (iter) = 0; ((iter) < (v).n) && (((var) = &((v).a[(iter)])), 1); ++(iter))
#define tsc_vec_foreach_ptr_rev(v, var, iter)                                                 \
  tsc_likely_if( (v).n > 0 )                                                                  \
    for ( (iter) = v.n - 1 ; ((iter + 1) > 0) && (((var) = &((v).a[(iter)]) ), 1); --(iter))
#define tsc_vec_map(v, fn) do {                                                               \
  tsc_likely_if( (v).n > 0 ) {                                                                \
    for ( size_t i = 0; i < (v).n ; ++i) {                                                    \
      (v).a[i] = fn((v).a[i]);                                                                \
    }                                                                                         \
  }                                                                                           \
  } while(0)
#define tsca_vec_subvec(name, d, s, start, count) do {                                        \
  tsc_unlikely_if(d == s) tsc_vec_splice(*dst, start, count);                             \
  
  }
#define tsca_vec_insert(name, v, idx, x) tsc_vec_##name##_insert(&(v), idx, x)
#define tsca_vec_pushfront(name, v, x) tsc_vec_##name##_insert(&(v), 0, x)
#define tsca_vec_all(name,v) tsc_vec_##name##_all(&(v))
#define tsca_vec_any(name,v) tsc_vec_##name##_any(&(v))


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
