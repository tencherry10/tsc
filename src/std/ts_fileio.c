#include "libts.h"


const char * ts_strtol(long int* ret, const char * s, int base) {
  const char *estr = NULL;
  char *endptr;
  
  errno = 0;
  *ret = strtol(s, &endptr, base);
  if(errno) return strerror(errno);
  if(endptr != (s + strlen(s)))
    return "PARSE FAILED";
  return estr;
}

// this function models after python's readline function
// it will take care of all the corner cases of fgets
// but it assumes there are no \0 in the file/stream
//
// returns NULL on success, otherwise it return a error string

const char * ts_freadline(char **ret, FILE *stream) {
  char        *tmp  = NULL;
  int         avail = 32;
  int         n     = 0;
  int         nlast;
  
  tsunlikely_if( (*ret = (char *) calloc(avail,1)) == NULL )
    return "OOM";
  
  while(1) {
    if( fgets(*ret + n, avail - n, stream) == NULL ) {
      tsunlikely_if(ferror(stream) != 0) {
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
        tsunlikely_if( (*ret = (char *) reallocf(*ret, ++avail)) == NULL )
          return "OOM";
      }
      return "EOF";
    }
    
    if(nlast == (avail - 2)) {  // not eof and no new line, buffer size must be not big enough
      n     = avail - 1;        // avail - 1 b/c we want to reuse the old \0 spot
      avail = avail << 1;
      tsunlikely_if( (*ret = (char *) reallocf(*ret, avail)) == NULL )
        return "OOM";
    }
  }
}

const char * ts_freadlines(char ***ret, int *nlines, FILE *stream) {
  const char  *estr   = NULL;
  char        *line   = NULL;
  int         avail   = 8;
  char        **tmp   = NULL;
  
  *nlines = 0;
  
  tsunlikely_if( (*ret = (char **) calloc(avail,sizeof(char *))) == NULL )
    return "OOM";
  
  while(1) {
    if( (estr = ts_freadline(&line, stream)) != NULL) {
      if(!strcmp("EOF", estr)) {
        (*ret)[(*nlines)++]=line;
        return NULL;
      }
      return estr;
    } else {
      (*ret)[(*nlines)++]=line;
      line = NULL;
      if(avail == *nlines) {
        tsunlikely_if( (tmp = (char **) realloc(*ret, avail << 1)) == NULL ) {
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

const char * ts_freadlinesfree(char **ret, int nlines) {
  for(int i = 0 ; i < nlines ; i++) 
    free(ret[i]);
  free(ret);
  return NULL;
}

const char * ts_getpass(char **lineptr, FILE *stream) {
  struct termios o, n;
  
  if(tcgetattr(fileno(stream), &o) != 0)
    return "FAILED to get old stream settings";
  
  n = o;
  n.c_lflag &= ~ECHO;   
  
  if(tcsetattr(fileno(stream), TCSAFLUSH, &n) != 0)
    return "FAILED to set new stream settings";
  
  const char *estr = ts_freadline(lineptr, stream);
  
  if(tcsetattr(fileno(stream), TCSAFLUSH, &o) != 0)
    return "FAILED to reset stream settings";
  
  return estr;
}

const char * ts_freadall(char **ret, FILE *stream) {
  char    *tmp  = NULL;  
  size_t  avail = 256;
  size_t  n     = 0;
  size_t  nread;

  tsunlikely_if( (*ret = (char *) malloc(avail+1)) == NULL )
    return "OOM";
  (*ret)[avail] = '\0';
  
  while(1) {
    nread = fread(*ret + n, 1, avail - n, stream);
    
    if(nread != (avail - n)) {
      tsunlikely_if(ferror(stream) != 0) {
        free(*ret); *ret = NULL; return "FGETS FAILED";
      } 
      for(size_t i = n + nread; i <= avail ; i++)
        (*ret)[i] = '\0';
      return NULL;
    }
    
    n = avail;
    avail = avail << 1;
    tsunlikely_if( (*ret = (char *) reallocf(*ret, avail+1)) == NULL )
      return "OOM";
    (*ret)[avail] = '\0';
  }
}

const char * ts_getcwd(char **ret) {
  char    *tmp;
  size_t  avail = 32;
  
  tsunlikely_if( (*ret = (char *) malloc(avail+1)) == NULL )
    return "OOM";
  (*ret)[avail] = '\0';  
  
  while(1) {
    if(getcwd(*ret, avail) != NULL) {
      return NULL;
    }

    avail = avail << 1;
    tsunlikely_if( (*ret = (char *) reallocf(*ret, avail+1)) == NULL )
      return "OOM";
    (*ret)[avail] = '\0';    
  }
}

const char * ts_getexecwd(char **ret) {
  char    *tmp;
  size_t  avail = 32;
  ssize_t retsz;
  
  tsunlikely_if( (*ret = (char *) malloc(avail+1)) == NULL )
    return "OOM";
  (*ret)[avail] = '\0';  
  
  while(1) {
    errno = 0;
    retsz = readlink("/proc/self/exe", *ret, avail);
    if(retsz == -1) {
      return strerror(errno);
    }
    if(retsz < (ssize_t) avail)
      break;

    avail = avail << 1;
    tsunlikely_if( (*ret = (char *) reallocf(*ret, avail+1)) == NULL )
      return "OOM";
    (*ret)[avail] = '\0';    
  }
  
  char * dir = dirname(*ret);
  tsunlikely_if( ts_strdup(&tmp, dir) != NULL ) {
    free(*ret); *ret = NULL; return "OOM";
  }
  free(*ret); 
  *ret = tmp; 
  
  return NULL;
}

ts_make_vec(sz_vec, size_t)
#define auto_sz_vec __attribute__((cleanup(auto_cleanup_sz_vec))) sz_vec_t
static inline void auto_cleanup_sz_vec(sz_vec_t *p) { 
  sz_vec_destroy(p);
}

const char * ts_lsdir(char ***ret, size_t *ndir, const char * curdir) {
  auto_dir      dir = NULL;
  const char    *estr = NULL;
  struct dirent *ent;
  auto_sz_vec   dirlen;
  *ret = NULL;
  *ndir = 0;
  
  sz_vec_init(&dirlen);
  
  E_ERRNO_NULL(dir = opendir(curdir));
  
  while( (ent = readdir(dir)) ) {
    sz_vec_push(&dirlen, strlen(ent->d_name)+1);
  }
  
  E_TSC(ts_alloc2d_irregular((void ***) ret, sz_vec_size(&dirlen), sz_vec_ptr(&dirlen), 1));
  
  errno = 0;
  if( closedir(dir) != 0) {
    dir = NULL;
    return strerror(errno);
  }
  dir = NULL;
  
  if( (dir = opendir(curdir) ) == NULL) {
    free(*ret);
    *ret = NULL;    
    return strerror(errno);
  }
  
  size_t i = 0;
  while( (ent = readdir(dir)) ) {
    strncpy((*ret)[i++], ent->d_name, strlen(ent->d_name)+1);
  }  

  errno = 0;
  if( closedir(dir) != 0) {
    free(*ret);
    *ret  = NULL;    
    dir   = NULL;
    return strerror(errno);
  }
  dir = NULL;
  
  *ndir = sz_vec_size(&dirlen);
  
  return NULL;
}

const char * ts_dir_exists(int *ret, const char * path) {
  struct stat s = {0};
  *ret = 0;
  errno = 0;
  if(stat(path, &s) != 0) {
    if(errno == ENOENT)
      return NULL;
    return strerror(errno);
  }
  
  if(S_ISDIR(s.st_mode))
    *ret = 1;
  
  return NULL;
}

const char * ts_file_exists(int *ret, const char * path) {
  struct stat s = {0};
  *ret = 0;
  errno = 0;
  if(stat(path, &s) != 0) {
    if(errno == ENOENT)
      return NULL;
    return strerror(errno);
  }
  
  if(S_ISREG(s.st_mode))
    *ret = 1;
  
  return NULL;
}

const char * ts_file_mtime(time_t *ret, const char *path) {
  struct stat statbuf;
  *ret = 0;
  if(stat(path, &statbuf) == -1)
    return "ts_file_mtime: stat failed";
  *ret = statbuf.st_mtime;
  return NULL;
}

const char * ts_set_file_mtime(const char *path, time_t mtime) {
  struct utimbuf times;
  times.actime  = mtime;
  times.modtime = mtime;
  E_ERRNO_NEG1(utime(path, &times));
  return NULL;
}

const char * ts_chmod(const char *path, mode_t mode) {
  E_ERRNO_NEG1(chmod(path, mode));
  return NULL;
}

const char * ts_get_cmd_result(char **ret, const char * cmd) {
  const char  *estr;
  auto_pfile  fp = NULL;
  *ret = NULL;
  
  if( (fp = popen(cmd, "r")) == NULL )
    return "popen failed";
  
  E_TSC(ts_freadall(ret, fp));
  
  return NULL;
}


// same as ts_freadline, but will handle \0
// which is why sz parameter is needed. When \0 exist *sz != strlen(ret)

const char * ts_freadline0(char **ret, int *sz, FILE *stream) {
  char        *tmp  = NULL;
  int         avail = 32;
  int         n     = 0;
  int         nlast;
  
  tsunlikely_if( (*ret = (char *) calloc(avail, 1)) == NULL )
    return "OOM";
    
  while(1) {
    if( fgets(*ret + n, avail - n, stream) == NULL ) {
      tsunlikely_if(ferror(stream) != 0) {
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
        tsunlikely_if( (*ret = (char *) reallocf(*ret, ++avail)) == NULL ) {
          if(sz) *sz = 0; 
          return "OOM";
        }
      }
      if(sz) *sz = nlast+1;
      return "EOF";
    }
    
    if(nlast == (avail - 2)) {  // not eof and no new line, buffer size must be not big enough
      n     = avail - 1;        // avail - 1 b/c we want to reuse the old \0 spot
      avail = avail << 1;
      tsunlikely_if( (*ret = (char *) reallocf(*ret, avail)) == NULL ) {
        if(sz) *sz = 0;
        return "OOM";
      }
      memset(*ret + n, 0, avail - n);
    }
  }
}
