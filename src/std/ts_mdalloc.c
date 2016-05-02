#include "libts.h"

// these functions are kind of interesting.
// they use up a non-trivial amount of re-direction pointer memory at the beginning in order to 
// "emulate" static multi-dimensional pointer accesses

// these are the sort of things that the compiler provides for you for free for 
// static multi-dimension matrices. Although, the static matrices must be regular. 
// whereas these functions can make optimal irregular matrices

// once you are willing to accept the additional memory usage (which isn't too bad) 
// a couple nice side effects jump out:
//  1. you don't need a nested loop to clean out this matrix. A single free is enough.
//  2. it turns out, if your matrices are big, you can't use static matrices anyway inside functions b/c it chews up stack space.
//      and you STILL can not use globals b/c of potential ownership issues. So you will have to pay the penalty of multiple malloc/free
//  3. Because you are mallocing one big memory, you don't get fragmentation and you get nice cacheing effects
//  4. and to make it even better, b/c you are repeated access the indirection header. 
//      They will mostly be in the cache as you traverse down the matrix 
//      (meaning access mat3d[z][0] will also fetch mat3d[z][1], mat3d[z][2], etc...
//  5. it is easy to shallow copy these matrices (just a memcpy)

// this matrix's performance will heavily depend on traversal direction. The cache-ing effect will only help
// if you traverse in x then y then z. If you traverse in y or in z first, 
// you may have to constantly move data in and out of the cache.
// although this is a problem generally for any array-based matrices structure

// the biggest problem with these matrices data structure is that they are fixed in stone.
// you have to known a priori what the size shall be. 
// in practice, this isn't a problem though. 
// If I need to do a lot of low level data traversals that will justify the use of this datastructure
// then I better know what the memory usage ought to be, and if I don't, 
// an array based datastructure is probably a bad idea anyway.

const char* ts_alloc2d(void ***ret, size_t y, size_t x, size_t sz) {
  size_t header = y*sizeof(void*);
  size_t body   = y*x*sz;
  tsunlikely_if( (*ret = (void **) malloc(header + body)) == NULL )
    return "OOM";
  
  (*ret)[0] = (void *) ((char *) (*ret) + header);
  for(size_t j = 1 ; j < y ; j++) {
    (*ret)[j] = (void *) ((char *) (*ret)[j-1] + x * sz);
  }    
  return NULL;
}

const char* ts_alloc2d_irregular(void ***ret, size_t y, size_t * x_per_y, size_t sz) {
  size_t header   = y*sizeof(void*);
  size_t tot_elem = 0;
  for(size_t j = 0 ; j < y ; j++) {
    tot_elem += x_per_y[j];
  }
  
  tsunlikely_if( (*ret = (void **) malloc(header + tot_elem * sz)) == NULL )
    return "OOM";
  
  (*ret)[0] = (void *) ((char *) (*ret) + header);
  for(size_t j = 1 ; j < y ; j++) {
    (*ret)[j] = (void *) ((char *) (*ret)[j-1] + x_per_y[j-1] * sz);
  }

  return NULL;
}

const char* ts_alloc3d(void ****ret, size_t z, size_t y, size_t x, size_t sz) {
  size_t header  = z*(y+1)*sizeof(void*);
  size_t body    = z*y*x*sz;
  tsunlikely_if( (*ret = (void ***) malloc(header + body)) == NULL) 
    return "OOM";
  
  for(size_t k = 0 ; k < z ; k++) {
    (*ret)[k] = (void **) ((char *) (*ret) + z*sizeof(void*) + k * y * sizeof(void*));
    for(size_t j = 0 ; j < y ; j++) {
      (*ret)[k][j] = (void *) ((char *) (*ret) + header + k*y*x*sz + j*x*sz);
    }
  }
  return NULL;
}

const char* ts_alloc3d_irregular(void ****ret, size_t z, size_t y, size_t ** x_per_z_y, size_t sz) {
  size_t header   = z*(y+1)*sizeof(void*);
  size_t tot_elem = 0;
  for(size_t k = 0 ; k < z ; k++) {
    for(size_t j = 0 ; j < y ; j++) {
      tot_elem += x_per_z_y[k][j];
    }  
  }
  
  tsunlikely_if( (*ret = (void ***) malloc(header + tot_elem * sz)) == NULL )
    return "OOM";
  
  for(size_t k = 0 ; k < z ; k++) {
    (*ret)[k]     = (void **) ((char *) (*ret) + z*sizeof(void*) + k * y * sizeof(void*));
    (*ret)[k][0]  = (void *)  ((char *) (*ret) + header);
    for(size_t j = 1 ; j < y ; j++) {
      (*ret)[k][j] = (void *) ((char *) (*ret)[k][j-1] + x_per_z_y[k][j-1]*sz);
    }
  }
  return NULL;
}

