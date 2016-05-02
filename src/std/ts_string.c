#include "libts.h"

char* ts_trimleft_inplace(char *str) {
  int len = strlen(str);
  tsunlikely_if(len == 0) return str;
  char *cur = str;
  while (*cur && isspace(*cur)) {
    ++cur;
    --len;
  }
  if (str != cur) memmove(str, cur, len + 1);
  return str;
}

char* ts_trimright_inplace(char *str) {
  int len = strlen(str);
  tsunlikely_if(len == 0) return str;
  char *cur = str + len - 1;
  while (cur != str && isspace(*cur)) --cur;
  cur[isspace(*cur) ? 0 : 1] = '\0';
  return str;
}

char* ts_trim_inplace(char *s) {
  ts_trimright_inplace(s);
  ts_trimleft_inplace(s);
  return s;
}

char* ts_upper_inplace(char *str) {
  for (int i = 0, len = strlen(str); i < len; i++) {
    if (islower(str[i])) {
      str[i] &= ~0x20;
    }
  }
  return str;
}

char* ts_lower_inplace(char *str) {
  for (int i = 0, len = strlen(str); i < len; i++) {
    if (isupper(str[i])) {
      str[i] |= 0x20;
    }
  }
  return str;
}

const char * ts_flatten(char **ret, char ** str_array, size_t n, const char * sep, size_t sep_sz) {
  size_t lens[n];
  size_t size   = 0;
  size_t pos    = 0;
  
  for(size_t i = 0 ; i < n ; i++) {
    lens[i] = strlen(str_array[i]);
    size   += lens[i];
  }
  
  tsunlikely_if( (*ret = (char *) malloc(size + 1 + (n-1)*sep_sz)) == NULL )
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

inline const char * ts_trunc(char **ret, const char *s, size_t n) {
  return ts_ndup(ret, s, n);
}

const char * ts_trim(char **ret, const char *s) {
  const char *end;
  *ret = NULL;
  
  while (isspace(*s))
    s++;

  tsunlikely_if(*s == 0) // only spaces
    return NULL;

  // rtrim
  end = s + strlen(s) - 1;
  while (end > s && isspace(*end))
    end--;
  
  return ts_ndup(ret, s, end - s + 1);
}

const char * ts_dup(char **ret, const char *s) {
  tsunlikely_if(s == NULL) { *ret = NULL; return NULL; }
  size_t len = strlen(s) + 1;
  tsunlikely_if( (*ret = (char *) malloc(len)) == NULL )
    return "OOM";
  memcpy(*ret, s, len);
  return NULL;
}

const char * ts_ndup(char **ret, const char *s, size_t n) {
  tsunlikely_if(s == NULL) { *ret = NULL; return NULL; }
  tsunlikely_if( (*ret = (char *) malloc(n+1)) == NULL )
    return "OOM";
  strncpy(*ret, s, n);  // use strncpy to ensure *ret is properly padded with zero
  (*ret)[n] = '\0';
  return NULL;
}

inline const char * ts_upper(char **ret, char *s) {
  const char  *estr = NULL;
  if( (estr = ts_dup(ret, s)) != NULL )
    return estr;
  ts_upper_inplace(*ret);
  return NULL;
}

inline const char * ts_lower(char **ret, char *s) {
  const char  *estr = NULL;
  tsunlikely_if( (estr = ts_dup(ret, s)) != NULL )
    return estr;
  ts_lower_inplace(*ret);
  return NULL;
}



