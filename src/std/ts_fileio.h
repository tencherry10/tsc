#ifndef TS_FILEIO_H__
#define TS_FILEIO_H__

TSC_EXTERN const char * ts_freadline(char **ret, FILE *stream);
TSC_EXTERN const char * ts_freadlines(char ***ret, int *nlines, FILE *stream);
TSC_EXTERN const char * ts_freadlinesfree(char **ret, int nlines);
TSC_EXTERN const char * ts_freadline0(char **ret, int *sz, FILE *stream);
TSC_EXTERN const char * ts_freadall(char **ret, FILE *stream);

TSC_EXTERN const char * ts_getpass(char **lineptr, FILE *stream);

TSC_EXTERN const char * ts_getcwd(char **ret);
TSC_EXTERN const char * ts_getexecwd(char **ret);
TSC_EXTERN const char * ts_lsdir(char ***ret, size_t *ndir, const char * dir);
TSC_EXTERN const char * ts_dir_exists(int *ret, const char * dir);
TSC_EXTERN const char * ts_file_exists(int *ret, const char * dir);
TSC_EXTERN const char * ts_file_mtime(time_t *ret, const char *path);
TSC_EXTERN const char * ts_set_file_mtime(const char *path, time_t mtime);

TSC_EXTERN const char * ts_get_cmd_result(char **ret, const char * cmd);

TSC_EXTERN const char * ts_chmod(const char *path, mode_t mode);

#endif
