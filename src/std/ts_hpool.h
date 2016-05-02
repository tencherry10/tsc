#ifndef TS_HPOOL_H__
#define TS_HPOOL_H__

#ifdef USE_TS_HPOOL


// opague data structures
typedef struct ts_hpool_t  ts_hpool_t;

TSC_EXTERN const char * ts_hpool_init(ts_hpool_t *heap, void *mem, size_t mem_sz);
TSC_EXTERN void         ts_hpool_deinit(ts_hpool_t *heap);
TSC_EXTERN void         ts_hpool_free(ts_hpool_t *heap, void *ptr);
TSC_EXTERN void         ts_hpool_freeall(ts_hpool_t *heap);
TSC_EXTERN void *       ts_hpool_malloc(ts_hpool_t *heap, size_t size);
TSC_EXTERN void *       ts_hpool_calloc(ts_hpool_t *heap, size_t n, size_t size);
TSC_EXTERN void *       ts_hpool_realloc(ts_hpool_t *heap, void *ptr, size_t size);
TSC_EXTERN void         ts_hpool_attach(ts_hpool_t *heap, void *ptr, void *parent);
TSC_EXTERN void *       ts_hpool_info(ts_hpool_t *heap, void *ptr);

#endif
#endif
