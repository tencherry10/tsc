#ifndef TSC__INCLUDE_TSC_CURL_H
#define TSC__INCLUDE_TSC_CURL_H

#include <openssl/ssl.h>
#include <curl/curl.h>


#define E_CURL(code) do {                   \
    if( (ecurl = (code)) != CURLE_OK ) {    \
      return curl_easy_strerror(ecurl);     \
    } } while(0)

#define auto_curl       __attribute__((cleanup(auto_cleanup_curl)))       CURL *
#define auto_curl_slist __attribute__((cleanup(auto_cleanup_curl_slist))) struct curl_slist *

static inline void auto_cleanup_curl(CURL **p) { 
  if(p && *p) {
    curl_easy_cleanup(*p);
    *p = NULL;
  }
}

static inline void auto_cleanup_curl_slist(struct curl_slist **p) {
  if(p && *p) {
    curl_slist_free_all(*p);
    *p = NULL;
  }  
}

TSC_EXTERN const char * 
tsc_ftp_getinfo(long *file_time, double *file_sz, const char *url, int en_ssl, const char * cert);

TSC_EXTERN const char * 
tsc_ftp_get(const char *url, const char *fname, int en_ssl, const char * cert);

#endif

#ifdef TSC_CURL_DEFINE


static CURLcode sslctx_certpem(CURL * curl, void *sslctx, void *userp) {
  X509_STORE  *store  = NULL;
  X509        *cert   = NULL;
  const char  *pem    = (const char*) userp; 
  BIO         *bio;
  
  (void) curl;
  
  /* get a BIO */ 
  if( (bio=BIO_new_mem_buf((void *) pem, -1)) == NULL)
    return CURLE_OUT_OF_MEMORY;
  
  /* use it to read the PEM formatted certificate from memory into an X509
   * structure that SSL can use */ 
  PEM_read_bio_X509(bio, &cert, 0, NULL);
  if(cert == NULL) {
    BIO_free(bio);
    return CURLE_SSL_CERTPROBLEM;
  }
  /* get a pointer to the X509 certificate store (which may be empty!) */ 
  store=SSL_CTX_get_cert_store((SSL_CTX *)sslctx);
 
  /* add our certificate to this store */ 
  if(X509_STORE_add_cert(store, cert)==0) {
    X509_free(cert);
    BIO_free(bio);   
    return CURLE_SSL_CERTPROBLEM;
  }
 
  /* decrease reference counts */ 
  X509_free(cert);
  BIO_free(bio);
  /* all set to go */ 
  return CURLE_OK;
}

static size_t header_get_size_cb(void *ptr, size_t size, size_t nmemb, void *data) {
  (void)ptr;
  (void)data;
  /* we are not interested in the headers itself,
     so we only return the size we would have saved ... */ 
  return (size_t)(size * nmemb);
}

static size_t ign_cb(void *content, size_t size, size_t nmemb, void *userp) {
  (void)content;
  (void)userp;
  return size * nmemb;
}

const char * 
tsc_ftp_getinfo(long *file_time, double *file_sz, const char *url, int en_ssl, const char * cert) {
  CURLcode  ecurl = 0;
  auto_curl req   = NULL;
  
  if( (req = curl_easy_init()) == NULL ) 
    return "curl init failed";   
  
  E_CURL(curl_easy_setopt(req,  CURLOPT_URL, url));
  E_CURL(curl_easy_setopt(req,  CURLOPT_NOBODY, 1L));         // No download 
  E_CURL(curl_easy_setopt(req,  CURLOPT_FILETIME, 1L));       // Ask for filetime
  E_CURL(curl_easy_setopt(req,  CURLOPT_HEADERFUNCTION, header_get_size_cb));
  E_CURL(curl_easy_setopt(req,  CURLOPT_HEADER, 0L));         // No header
  
  if(en_ssl) {
    E_CURL(curl_easy_setopt(req,  CURLOPT_USE_SSL, CURLUSESSL_ALL));
    if(cert) {
      E_CURL(curl_easy_setopt(req, CURLOPT_WRITEFUNCTION,     ign_cb));
      E_CURL(curl_easy_setopt(req, CURLOPT_SSLCERTTYPE,       "PEM"));
      E_CURL(curl_easy_setopt(req, CURLOPT_SSL_CTX_FUNCTION,  sslctx_certpem));
      E_CURL(curl_easy_setopt(req, CURLOPT_SSL_CTX_DATA,      (void *) cert));      
    }
  }
  
  E_CURL(curl_easy_perform(req));
  
  E_CURL(curl_easy_getinfo(req, CURLINFO_FILETIME, file_time));
  E_CURL(curl_easy_getinfo(req, CURLINFO_CONTENT_LENGTH_DOWNLOAD, file_sz));
  
  return NULL;
}

static size_t ftp_write_cb(void *buffer, size_t size, size_t nmemb, void *userp) {
  FILE *fp = (FILE *) userp;
  return fwrite(buffer, size, nmemb, fp);
}

const char * tsc_ftp_get(const char *url, const char *fname, int en_ssl, const char * cert) {
  CURLcode  ecurl = 0;
  auto_curl req   = NULL;
  auto_file fp    = NULL;
  
  if( (req = curl_easy_init()) == NULL ) 
    return "curl init failed"; 
  
  if( (fp = fopen(fname, "wb")) == NULL)
    return "fopen failed";
  
  E_CURL(curl_easy_setopt(req, CURLOPT_URL, url));
  E_CURL(curl_easy_setopt(req, CURLOPT_WRITEFUNCTION, ftp_write_cb));
  E_CURL(curl_easy_setopt(req, CURLOPT_WRITEDATA, fp));
  if(en_ssl) {
    E_CURL(curl_easy_setopt(req, CURLOPT_USE_SSL, CURLUSESSL_ALL));
    if(cert) {
      E_CURL(curl_easy_setopt(req, CURLOPT_SSLCERTTYPE, "PEM"));
      E_CURL(curl_easy_setopt(req, CURLOPT_SSL_CTX_FUNCTION, sslctx_certpem));
      E_CURL(curl_easy_setopt(req, CURLOPT_SSL_CTX_DATA, (void *) cert));
    }
  }
  E_CURL(curl_easy_perform(req));
  
  return NULL;
}

#endif
