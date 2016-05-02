#ifndef TS_POOL_H__
#define TS_POOL_H__

#ifdef USE_TS_POOL

typedef struct ts_pool_t   ts_pool_t;

TSC_EXTERN const char * ts_pool_init(ts_pool_t *heap, void *mem, size_t mem_sz);
TSC_EXTERN void         ts_pool_deinit(ts_pool_t *heap);
TSC_EXTERN void         ts_pool_free(ts_pool_t *heap, void *ptr);
TSC_EXTERN void         ts_pool_freeall(ts_pool_t *heap);
TSC_EXTERN void *       ts_pool_malloc(ts_pool_t *heap, size_t size);
TSC_EXTERN void *       ts_pool_calloc(ts_pool_t *heap, size_t n, size_t size);
TSC_EXTERN void *       ts_pool_realloc(ts_pool_t *heap, void *ptr, size_t size);
TSC_EXTERN void *       ts_pool_info(ts_pool_t *heap, void *ptr);

#endif
#endif
