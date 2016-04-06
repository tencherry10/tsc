
#ifndef TSCP__INCLUDE_TSC_H
#define TSCP__INCLUDE_TSC_H
//}

TSC_EXTERN const char * tscp_strdup(tsc_pool_t *p, char **ret, const char *s);
TSC_EXTERN const char * tscp_strndup(tsc_pool_t *p, char **ret, const char *s, size_t n);

#define TSCP_DEFINE
#ifdef  TSCP_DEFINE
//}

const char * tscp_strdup(tsc_pool_t *p, char **ret, const char *s) {
  tsc_unlikely_if(s == NULL) { *ret = NULL; return NULL; }
  size_t len = strlen(s) + 1;
  tsc_unlikely_if( (*ret = (char *) tsc_pool_malloc(p, len)) == NULL )
    return "OOM";
  memcpy(*ret, s, len);
  return NULL;
}

//{
#endif
//{
#endif
