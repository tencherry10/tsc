#ifndef TS_VEC_H__
#define TS_VEC_H__

/// ts_vec class is done with some nasty gnarly macros. This is intentional.
/// In C, where generic container is not possible, the only way to get a cleaner implementation
/// is to use void * pointers and then cast to and from it. The problem with that approach is that
/// casts prevents optimization and loop unrolling (type information is lost).

// CAUTION, if OOM happens in resize / copy / push, you are responsible for cleaning up dst
// Not possible to do it here b/c we don't have info on the resources pointed by a[i]

#define ts_roundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))
#define ts_vec_resize(name, v, s) ts_vec_##name##_resize(&(v),(s))
#define ts_vec_copy(name, d, s) ts_vec_##name##_copy(&(d),&(s))
#define ts_vec_push(name, v, x) ts_vec_##name##_push(&(v), (x))
#define ts_vec_compact(name, v) ts_vec_##name##_resize(&(v), (v).n)
#define ts_vec_extend(name, d, s) ts_vec_##name##_extend(&(d),&(s))
#define ts_vec_reverse(name, v) ts_vec_##name##_reverse(&(v))
#define ts_vec_subvec(name, d, s, start, count) ts_vec_##name##_subvec(&(d), &(s), start, count)
#define ts_vec_insert(name, v, idx, x) ts_vec_##name##_insert(&(v), idx, x)
#define ts_vec_pushfront(name, v, x) ts_vec_##name##_insert(&(v), 0, x)
#define ts_vec_all(name,v) ts_vec_##name##_all(&(v))
#define ts_vec_any(name,v) ts_vec_##name##_any(&(v))

#define ts_vec_init(v) ((v).n = (v).m = 0, (v).a = 0)
#define ts_vec_destroy(v) free((v).a)
#define ts_vec_A(v, i) ((v).a[(i)])
#define ts_vec_pop(v) ((v).a[--(v).n])
#define ts_vec_size(v) ((v).n)
#define ts_vec_max(v) ((v).m)
#define ts_vec_ptr(v) ((v).a)
#define ts_vec_clear(v) ((v).n = 0)
#define ts_vec_first(v) ((v).a[0])
#define ts_vec_last(v) ((v).a[(v).n-1])
#define ts_vec_type(name) ts_vec_##name##_t

#define ts_vec_define(name, type)                                                      \
  typedef struct { size_t n, m; type *a; } ts_vec_##name##_t;                          \
  static inline const char* ts_vec_##name##_resize(ts_vec_##name##_t *v, size_t s) {  \
    type *tmp;                                                                          \
    tsunlikely_if((tmp = (type*)realloc(v->a, sizeof(type) * s)) == NULL )                        \
      return "OOM";                                                                     \
    v->m = s; v->a = tmp; return NULL; }                                                \
  static inline const char*                                                             \
  ts_vec_##name##_copy(ts_vec_##name##_t *dst, ts_vec_##name##_t *src) {             \
    const char *estr;                                                                   \
    tsunlikely_if(dst == src) return "COPYING FROM SRC TO SRC";                                   \
    if (dst->m < src->n)                                                                \
      tsunlikely_if( (estr = ts_vec_##name##_resize(dst, src->n)) != NULL )                       \
        return estr;                                                                    \
    dst->n = src->n;                                                                    \
    memcpy(dst->a, src->a, sizeof(type) * src->n); return NULL; }                       \
  static inline const char* ts_vec_##name##_push(ts_vec_##name##_t *v, type x) {      \
    const char *estr;                                                                   \
    if(v->n == v->m) {                                                                  \
      size_t m = v->m ? v->m << 1 : 4;                                                  \
      tsunlikely_if( (estr = ts_vec_##name##_resize(v, m)) != NULL )                 \
        return estr;                                                                    \
    }                                                                                   \
    v->a[v->n++]= x; return NULL; }                                                     \
  static inline const char*                                                             \
  ts_vec_##name##_extend(ts_vec_##name##_t *dst, ts_vec_##name##_t *src) {           \
    const char *estr;                                                                   \
    size_t n = dst->n + src->n;                                                         \
    if(dst->m < n)                                                                      \
      tsunlikely_if( (estr = ts_vec_##name##_resize(dst, n)) != NULL )               \
        return estr;                                                                    \
    memcpy(dst->a + dst->n, src->a, sizeof(type) * src->n);                             \
    dst->n = n; return NULL; }                                                          \
  static inline const char * ts_vec_##name##_reverse(ts_vec_##name##_t *v) {          \
    type tmp;                                                                           \
    for( size_t i = (v->n-1) >> 1 ; (i + 1) > 0 ; --i) {                                \
      tmp = v->a[i]; v->a[i] = v->a[v->n - i - 1]; v->a[v->n - i - 1] = tmp;            \
    }                                                                                   \
    return NULL; }                                                                      \
  static inline const char* ts_vec_##name##_subvec(ts_vec_##name##_t *dst,            \
    ts_vec_##name##_t *src, size_t start, size_t count)                                \
  {                                                                                     \
    tsunlikely_if(dst == src) {                                                       \
      ts_vec_splice(*dst, start, count);                                               \
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
    tsunlikely_if( (estr = ts_vec_##name##_resize(dst, n)) != NULL )                 \
      return estr;                                                                      \
    memcpy(dst->a, src->a + start, sizeof(type) * n);                                   \
    dst->n = n; return NULL; }                                                          \
  static inline const char*                                                             \
  ts_vec_##name##_insert(ts_vec_##name##_t *v, size_t idx, type x) {                  \
    tsunlikely_if(idx > v->n) return "INDEX OUT OF BOUND";                            \
    if(idx == v->n) return ts_vec_##name##_push(v, x);                                 \
    if(v->n == v->m) {                                                                  \
      const char *estr;                                                                 \
      size_t m = v->m ? v->m << 1 : 4;                                                  \
      tsunlikely_if( (estr = ts_vec_##name##_resize(v, m)) != NULL )                 \
        return estr;                                                                    \
    }                                                                                   \
    for(size_t i = v->n ; i > idx ; --i ) {                                             \
      v->a[i] = v->a[i-1];                                                              \
    }                                                                                   \
    v->n++; v->a[idx] = x; return NULL; }                                               \
  static inline int ts_vec_##name##_all(ts_vec_##name##_t *v) {                       \
    int ret = 1;                                                                        \
    for(size_t i = 0 ; i < v->n ; ++i)                                                  \
      ret = ret && !!(v->a[i]);                                                         \
    return ret; }                                                                       \
  static inline int ts_vec_##name##_any(ts_vec_##name##_t *v) {                       \
    int ret = 0;                                                                        \
    for(size_t i = 0 ; i < v->n ; ++i)                                                  \
      ret = ret || !!(v->a[i]);                                                         \
    return ret; }
#define ts_vec_splice(v, start, count) do {                                            \
    tsunlikely_if(count == 0) break;                                                  \
    tsunlikely_if(start > (v).n) {                                                    \
      (v).n = 0;                                                                        \
      break;                                                                            \
    }                                                                                   \
    tsunlikely_if(start + count > (v).n) {                                            \
      (v).n = start;                                                                    \
      break;                                                                            \
    }                                                                                   \
    memmove((v).a + start, (v).a + start + count,                                       \
      sizeof((v).a[0]) * ((v).n - start - count));                                      \
    (v).n -= count;                                                                     \
  } while(0)
#define ts_vec_foreach(v, var, iter)                                                   \
  tslikely_if( (v).n > 0 )                                                            \
    for ( (iter) = 0; ((iter) < (v).n) && (((var) = (v).a[(iter)]), 1); ++(iter))
#define ts_vec_foreach_rev(v, var, iter)                                               \
  tslikely_if( (v).n > 0 )                                                            \
    for ( (iter) = v.n - 1 ; ((iter + 1) > 0) && (((var) = (v).a[(iter)]), 1); --(iter))
#define ts_vec_foreach_ptr(v, var, iter)                                               \
  tslikely_if( (v).n > 0 )                                                            \
    for ( (iter) = 0; ((iter) < (v).n) && (((var) = &((v).a[(iter)])), 1); ++(iter))
#define ts_vec_foreach_ptr_rev(v, var, iter)                                           \
  tslikely_if( (v).n > 0 )                                                            \
    for ( (iter) = v.n - 1 ; ((iter + 1) > 0) && (((var) = &((v).a[(iter)]) ), 1); --(iter))
#define ts_vec_map(v, fn) do {                                                          \
    tslikely_if( (v).n > 0 ) {                                                         \
      for ( size_t i = 0; i < (v).n ; ++i) {                                            \
        (v).a[i] = fn((v).a[i]);                                                        \
      }                                                                                 \
    }                                                                                   \
  } while(0)

#define ts_make_vec(name, type)                                                         \
  typedef struct { size_t n, m; type *a; } name##_t;                                    \
  static inline void name##_init(name##_t *v) { v->n = 0; v->m = 0; v->a = 0; }         \
  static inline void name##_destroy(name##_t *v) { free(v->a); }                        \
  static inline void name##_clear(name##_t *v) { v->n = 0; }                            \
  static inline type name##_elem(name##_t *v, size_t i) { return v->a[i]; }             \
  static inline type name##_at(name##_t *v, size_t i) { return v->a[i]; }               \
  static inline type name##_pop(name##_t *v) { return v->a[--(v->n)]; }                 \
  static inline type name##_first(name##_t *v) { return v->a[0]; }                      \
  static inline type name##_last(name##_t *v) { return v->a[v->n-1]; }                  \
  static inline size_t name##_size(name##_t *v) { return v->n; }                        \
  static inline size_t name##_max(name##_t *v) { return v->m; }                         \
  static inline type* name##_ptr(name##_t *v) { return v->a; }                          \
  static inline const char* name##_resize(name##_t *v, size_t s) {                      \
    type *tmp;                                                                          \
    tsunlikely_if((tmp = (type*)realloc(v->a, sizeof(type) * s)) == NULL )              \
      return "OOM";                                                                     \
    v->m = s; v->a = tmp; return NULL; }                                                \
  static inline const char* name##_copy(name##_t *dst, name##_t *src) {                 \
    const char *estr;                                                                   \
    tsunlikely_if(dst == src) return NULL;                                              \
    if (dst->m < src->n)                                                                \
      tsunlikely_if( (estr = name##_resize(dst, src->n)) != NULL )                      \
        return estr;                                                                    \
    dst->n = src->n;                                                                    \
    memcpy(dst->a, src->a, sizeof(type) * src->n); return NULL; }                       \
  static inline const char* name##_push(name##_t *v, type x) {                          \
    const char *estr;                                                                   \
    if(v->n == v->m) {                                                                  \
      size_t m = v->m ? v->m << 1 : 4;                                                  \
      tsunlikely_if( (estr = name##_resize(v, m)) != NULL )                             \
        return estr;                                                                    \
    }                                                                                   \
    v->a[v->n++]= x; return NULL; }                                                     \
  static inline const char* name##_compact(name##_t *v) {                               \
    return name##_resize(v, v->n); }                                                    \
  static inline const char* name##_extend(name##_t *dst, name##_t *src) {               \
    const char *estr;                                                                   \
    size_t n = dst->n + src->n;                                                         \
    if(dst->m < n)                                                                      \
      tsunlikely_if( (estr = name##_resize(dst, n)) != NULL )                           \
        return estr;                                                                    \
    memcpy(dst->a + dst->n, src->a, sizeof(type) * src->n);                             \
    dst->n = n; return NULL; }                                                          \
  static inline const char* name##_reverse(name##_t *v) {                               \
    type tmp;                                                                           \
    for( size_t i = (v->n-1) >> 1 ; (i + 1) > 0 ; --i) {                                \
      tmp = v->a[i]; v->a[i] = v->a[v->n - i - 1]; v->a[v->n - i - 1] = tmp;            \
    }                                                                                   \
    return NULL; }                                                                      \
  static inline const char* name##_splice(name##_t *v, size_t start, size_t count) {    \
    tsunlikely_if(count == 0) return NULL;                                              \
    tsunlikely_if(start > v->n) { v->n = 0; return NULL; }                              \
    tsunlikely_if(start + count > v->n) { v->n = start; return NULL; }                  \
    memmove(v->a + start, v->a + start + count, sizeof(v->a[0])*(v->n - start - count));\
    v->n -= count; return NULL; }                                                       \
  static inline const char* name##_subvec(name##_t *dst,                                \
    name##_t *src, size_t start, size_t count) {                                        \
    const char *estr; size_t n;                                                         \
    tsunlikely_if(dst == src) { name##_splice(dst, start, count); return NULL; }        \
    if(start > src->n) { dst->n = 0; return NULL; }                                     \
    if(start + count > src->n)  n = src->n - start;                                     \
    else                        n = count;                                              \
    tsunlikely_if( (estr = name##_resize(dst, n)) != NULL )                             \
      return estr;                                                                      \
    memcpy(dst->a, src->a + start, sizeof(type) * n);                                   \
    dst->n = n; return NULL; }                                                          \
  static inline const char* name##_insert(name##_t *v, size_t idx, type x) {            \
    tsunlikely_if(idx > v->n) return "INDEX OUT OF BOUND";                              \
    if(idx == v->n) return name##_push(v, x);                                           \
    if(v->n == v->m) {                                                                  \
      const char *estr;                                                                 \
      size_t m = v->m ? v->m << 1 : 4;                                                  \
      tsunlikely_if( (estr = name##_resize(v, m)) != NULL )                             \
        return estr;                                                                    \
    }                                                                                   \
    for(size_t i = v->n ; i > idx ; --i ) {                                             \
      v->a[i] = v->a[i-1];                                                              \
    }                                                                                   \
    v->n++; v->a[idx] = x; return NULL; }                                               \
  static inline const char* name##_pushfront(name##_t *v, type x) {                     \
    return name##_insert(v, 0, x); }                                                    \
  static inline int name##_all(name##_t *v) {                                           \
    int ret = 1;                                                                        \
    for(size_t i = 0 ; i < v->n ; ++i)                                                  \
      ret = ret && !!(v->a[i]);                                                         \
    return ret; }                                                                       \
  static inline int name##_any(name##_t *v) {                                           \
    int ret = 0;                                                                        \
    for(size_t i = 0 ; i < v->n ; ++i)                                                  \
      ret = ret || !!(v->a[i]);                                                         \
    return ret; }
    
#endif
