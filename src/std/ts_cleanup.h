#ifndef TS_CLEANUP_H_
#define TS_CLEANUP_H_

#define auto_cstr   __attribute__((cleanup(auto_cleanup_cstr)))   char *
#define auto_file   __attribute__((cleanup(auto_cleanup_file)))   FILE *
#define auto_pfile  __attribute__((cleanup(auto_cleanup_pfile)))  FILE *
#define auto_dir    __attribute__((cleanup(auto_cleanup_dir)))    DIR *

static inline void auto_cleanup_file(FILE **fp) {
  if(fp && *fp) {
    fclose(*fp);
    *fp = NULL;
  }
}

static inline void auto_cleanup_pfile(FILE **fp) {
  if(fp && *fp) {
    pclose(*fp);
    *fp = NULL;
  }
}

static inline void auto_cleanup_cstr(char **s) { 
  if(s && *s) {
    free(*s);
    *s = NULL;
  }
}

static inline void auto_cleanup_dir(DIR **dp) {
  if(dp && *dp) {
    closedir(*dp);
    *dp = NULL;
  }  
}

#endif
