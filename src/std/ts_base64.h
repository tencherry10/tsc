#ifndef TS_BASE64_H__
#define TS_BASE64_H__

TSC_EXTERN const char * ts_base64_enc(char **ret, char *data, size_t sz);
TSC_EXTERN const char * ts_base64_dec(char **ret, size_t* retsz, char *data, size_t datsz);

#endif
