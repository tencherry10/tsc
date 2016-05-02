#ifndef TS_STRING_H__
#define TS_STRING_H__

TSC_EXTERN int          ts_startswith(const char *s, const char *start);
TSC_EXTERN int          ts_endswith(const char *s, const char *end);
TSC_EXTERN char *       ts_trimleft_inplace(char *s);
TSC_EXTERN char *       ts_trimright_inplace(char *s);
TSC_EXTERN char *       ts_trim_inplace(char *s);
TSC_EXTERN char *       ts_upper_inplace(char *str);
TSC_EXTERN char *       ts_lower_inplace(char *str);
TSC_EXTERN const char * ts_trim(char **ret, const char *s);
TSC_EXTERN const char * ts_strdup(char **ret, const char *s);
TSC_EXTERN const char * ts_strndup(char **ret, const char *s, size_t n);
TSC_EXTERN const char * ts_flatten(char **ret, char ** str_array, size_t n, 
  const char * sep, size_t sep_sz);
TSC_EXTERN const char * ts_trunc(char **ret, const char *s, size_t n);
TSC_EXTERN const char * ts_upper(char **ret, char *s);
TSC_EXTERN const char * ts_lower(char **ret, char *s);

#endif
