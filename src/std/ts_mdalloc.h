#ifndef TS_MDALLOC_H__
#define TS_MDALLOC_H__

TSC_EXTERN const char * ts_alloc2d(void ***ret, size_t y, size_t x, size_t sz);
TSC_EXTERN const char * ts_alloc2d_irregular(void ***ret, size_t y, size_t * x_per_y, size_t sz);
TSC_EXTERN const char * ts_alloc3d(void ****ret, size_t z, size_t y, size_t x, size_t sz);
TSC_EXTERN const char * ts_alloc3d_irregular(void ****ret, size_t z, size_t y, size_t ** x_per_z_y, size_t sz);

#endif
