#ifndef TS_MACRO_H__
#define TS_MACRO_H__

#ifdef __cplusplus
  #define TSC_EXTERN extern "C"
#else
  #define TSC_EXTERN extern
#endif

#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
  #define tslikely(expr)      __builtin_expect(!!(expr), 1)
  #define tslikely_if(expr)   if(__builtin_expect(!!(expr), 1))
  #define tsunlikely(expr)    __builtin_expect(!!(expr), 0)
  #define tsunlikely_if(expr) if(__builtin_expect(!!(expr), 0))
  #define tsinline            __attribute__((always_inline))
#else
  #define tslikely(expr)      (expr)
  #define tslikely_if(expr)   if((expr))
  #define tsunlikely(expr)    (expr)
  #define tsunlikely_if(expr) if((expr))
  #define tsinline            
#endif

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#define E_TSC(expr)  do         { \
  if( (estr = (expr)) != NULL ) { \
    return estr;                  \
  } } while(0)

#define E_ABORT(expr)  do       {           \
  if( (estr = (expr)) != NULL ) {           \
    printf("abort with error: %s\n", estr); \
    abort();                                \
  } } while(0)
  

#define E_MAIN(expr) do {               \
    if( (estr = (expr)) != NULL ) {     \
      printf("failed with %s\n", estr); \
      return 1;                         \
    }                                   \
  } while(0)

#define E_ERRNO_NEG1(expr) do { \
  errno = 0;                    \
  if( (expr) == -1 ) {          \
    return strerror(errno);     \
  } } while(0)

#define E_ERRNO_NULL(expr) do { \
  errno = 0;                    \
  if( (expr) == NULL ) {        \
    return strerror(errno);     \
  } } while(0)

#endif
