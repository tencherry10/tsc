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

#endif
