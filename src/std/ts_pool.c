#include "libts.h"

// ideas for improvements
// 1. pool size are limited to 2^16*blocksize, maybe we should make a larger pool?
// 2. need to add stats to hpool / pool

#ifdef USE_TS_POOL

#define TS_POOL_ATTPACKPRE
#define TS_POOL_ATTPACKSUF __attribute__((__packed__))

#ifndef TS_POOL_CRITICAL_ENTRY
  #define TS_POOL_CRITICAL_ENTRY()
#endif
#ifndef TS_POOL_CRITICAL_EXIT
  #define TS_POOL_CRITICAL_EXIT()
#endif

TS_POOL_ATTPACKPRE typedef struct ts_pool_ptr {
  uint16_t next;
  uint16_t prev;
} TS_POOL_ATTPACKSUF ts_pool_ptr;

#if INTPTR_MAX == INT32_MAX

TS_POOL_ATTPACKPRE typedef struct ts_pool_block {
  union {
    ts_pool_ptr  used;
  } header;
  union {
    ts_pool_ptr  free;
    uint8_t       data[4];
  } body;
} TS_POOL_ATTPACKSUF ts_pool_block;


#else

TS_POOL_ATTPACKPRE typedef struct ts_pool_block {
  union {
    ts_pool_ptr  used;
  } header;
  uint8_t padding[4];
  union {
    ts_pool_ptr  free;
    uint8_t       data[8];
  } body;
} TS_POOL_ATTPACKSUF ts_pool_block;

#endif

struct ts_pool_t {
  ts_pool_block  *heap;
  uint16_t        numblocks;
  uint16_t        allocd;
};


// Pool Allocator based on Ralph Hempel's umm_malloc
// changes made:
// 1. move under tsc naming space for consistency
// 2. change types to stdint types (uint*_t)
// 3. pass in pools instead of depending on one global pool
// 4. removal of debug log
// 5. fix some pedantic warnings about pointer arithmetic over void*
// 6. use platform independent printf formatting for pointers PRIXPTR

// couple notes about this allocator
// 1. it is rather inefficient for small allocations. amount of memory used
//    for 32bit machine: numbytes < 4 ? 16 : (ceil((numbytes - 4)/16)+1)*16
//    for 64bit machine: numbytes < 8 ? 24 : (ceil((numbytes - 8)/24)+1)*24
//
//    this is only a bit worse than default malloc performance of:
//    for 32bit machine:  ceil((numbytes+4)/8)*8 < 16 ? 16 : ceil((numbytes+4)/8)*8
//    for 64bit machine:  ceil((numbytes+8)/16)*16 ? 32 : ceil((numbytes+8)/16)*16
// 2. if you are worried about memory fragmentation use TS_POOL_BEST_FIT
//    just be aware the TS_POOL_BEST_FIT could potentially be very expensive b/c in the worse case 
//    it has to traverse ALL free list before deciding on best fit

#define TS_POOL_FREELIST_MASK (0x8000)
#define TS_POOL_BLOCKNO_MASK  (0x7FFF)
#define TS_POOL_BLOCK(b)  (heap->heap[b])
#define TS_POOL_NBLOCK(b) (TS_POOL_BLOCK(b).header.used.next)
#define TS_POOL_PBLOCK(b) (TS_POOL_BLOCK(b).header.used.prev)
#define TS_POOL_NFREE(b)  (TS_POOL_BLOCK(b).body.free.next)
#define TS_POOL_PFREE(b)  (TS_POOL_BLOCK(b).body.free.prev)
#define TS_POOL_DATA(b)   (TS_POOL_BLOCK(b).body.data)


typedef struct ts_pool_info_t {
  uint16_t totalEntries,  usedEntries,  freeEntries; 
  uint16_t totalBlocks,   usedBlocks,   freeBlocks; 
} ts_pool_info_t;

void *ts_pool_info(ts_pool_t *heap, void *ptr) {
  ts_pool_info_t heapInfo;
  unsigned short int blockNo = 0;

  // Protect the critical section...
  //
  TS_POOL_CRITICAL_ENTRY();
  
  // Clear out all of the entries in the heapInfo structure before doing
  // any calculations..
  //
  memset( &heapInfo, 0, sizeof( heapInfo ) );

  printf("\n\nDumping the ts_pool_heap...\n" );

  printf("|0x%" PRIXPTR "|B %5i|NB %5i|PB %5i|Z %5i|NF %5i|PF %5i|\n",
          (uintptr_t)(&TS_POOL_BLOCK(blockNo)),
          blockNo,
          TS_POOL_NBLOCK(blockNo) & TS_POOL_BLOCKNO_MASK,
          TS_POOL_PBLOCK(blockNo),
          (TS_POOL_NBLOCK(blockNo) & TS_POOL_BLOCKNO_MASK )-blockNo,
          TS_POOL_NFREE(blockNo),
          TS_POOL_PFREE(blockNo) );

  // Now loop through the block lists, and keep track of the number and size
  // of used and free blocks. The terminating condition is an nb pointer with
  // a value of zero...
  
  blockNo = TS_POOL_NBLOCK(blockNo) & TS_POOL_BLOCKNO_MASK;

  while( TS_POOL_NBLOCK(blockNo) & TS_POOL_BLOCKNO_MASK ) {
    ++heapInfo.totalEntries;
    heapInfo.totalBlocks += (TS_POOL_NBLOCK(blockNo) & TS_POOL_BLOCKNO_MASK )-blockNo;

    // Is this a free block?

    if( TS_POOL_NBLOCK(blockNo) & TS_POOL_FREELIST_MASK ) {
      ++heapInfo.freeEntries;
      heapInfo.freeBlocks += (TS_POOL_NBLOCK(blockNo) & TS_POOL_BLOCKNO_MASK )-blockNo;

      printf("|0x%" PRIXPTR "|B %5i|NB %5i|PB %5i|Z %5i|NF %5i|PF %5i|\n",
              (uintptr_t)(&TS_POOL_BLOCK(blockNo)),
              blockNo,
              TS_POOL_NBLOCK(blockNo) & TS_POOL_BLOCKNO_MASK,
              TS_POOL_PBLOCK(blockNo),
              (TS_POOL_NBLOCK(blockNo) & TS_POOL_BLOCKNO_MASK )-blockNo,
              TS_POOL_NFREE(blockNo),
              TS_POOL_PFREE(blockNo) );
     
      // Does this block address match the ptr we may be trying to free?

      if( ptr == &TS_POOL_BLOCK(blockNo) ) {
       
        // Release the critical section...
        //
        TS_POOL_CRITICAL_EXIT();
 
        return( ptr );
      }
    } else {
      ++heapInfo.usedEntries;
      heapInfo.usedBlocks += (TS_POOL_NBLOCK(blockNo) & TS_POOL_BLOCKNO_MASK )-blockNo;

      printf("|0x%" PRIXPTR "|B %5i|NB %5i|PB %5i|Z %5i|\n",
              (uintptr_t)(&TS_POOL_BLOCK(blockNo)),
              blockNo,
              TS_POOL_NBLOCK(blockNo) & TS_POOL_BLOCKNO_MASK,
              TS_POOL_PBLOCK(blockNo),
              (TS_POOL_NBLOCK(blockNo) & TS_POOL_BLOCKNO_MASK )-blockNo);
    }

    blockNo = TS_POOL_NBLOCK(blockNo) & TS_POOL_BLOCKNO_MASK;
  }

  // Update the accounting totals with information from the last block, the
  // rest must be free!

  heapInfo.freeBlocks  += heap->numblocks - blockNo;
  heapInfo.totalBlocks += heap->numblocks - blockNo;

  printf("|0x%" PRIXPTR "|B %5i|NB %5i|PB %5i|Z %5i|NF %5i|PF %5i|\n",
          (uintptr_t)(&TS_POOL_BLOCK(blockNo)),
          blockNo,
          TS_POOL_NBLOCK(blockNo) & TS_POOL_BLOCKNO_MASK,
          TS_POOL_PBLOCK(blockNo),
          heap->numblocks - blockNo,
          TS_POOL_NFREE(blockNo),
          TS_POOL_PFREE(blockNo) );

  printf("Total Entries %5i    Used Entries %5i    Free Entries %5i\n",
          heapInfo.totalEntries,
          heapInfo.usedEntries,
          heapInfo.freeEntries );

  printf("Total Blocks  %5i    Used Blocks  %5i    Free Blocks  %5i\n",
          heapInfo.totalBlocks,
          heapInfo.usedBlocks,
          heapInfo.freeBlocks  );

  // Release the critical section...
  //
  TS_POOL_CRITICAL_EXIT();
 
  return( NULL );
}

static uint16_t ts_pool_blocks( size_t size ) {

  // The calculation of the block size is not too difficult, but there are
  // a few little things that we need to be mindful of.
  //
  // When a block removed from the free list, the space used by the free
  // pointers is available for data. That's what the first calculation
  // of size is doing.

  if( size <= (sizeof(((ts_pool_block *)0)->body)) )
    return( 1 );

  // If it's for more than that, then we need to figure out the number of
  // additional whole blocks the size of an ts_pool_block are required.

  size -= ( 1 + (sizeof(((ts_pool_block *)0)->body)) );

  return( 2 + size/(sizeof(ts_pool_block)) );
}

static void ts_pool_make_new_block(ts_pool_t *heap, uint16_t c, uint16_t blocks, uint16_t freemask) {
  TS_POOL_NBLOCK(c+blocks) = TS_POOL_NBLOCK(c) & TS_POOL_BLOCKNO_MASK;
  TS_POOL_PBLOCK(c+blocks) = c;
  TS_POOL_PBLOCK(TS_POOL_NBLOCK(c) & TS_POOL_BLOCKNO_MASK) = (c+blocks);
  TS_POOL_NBLOCK(c) = (c+blocks) | freemask;
}

static void ts_pool_disconnect_from_free_list(ts_pool_t *heap, uint16_t c) {
  // Disconnect this block from the FREE list
  TS_POOL_NFREE(TS_POOL_PFREE(c)) = TS_POOL_NFREE(c);
  TS_POOL_PFREE(TS_POOL_NFREE(c)) = TS_POOL_PFREE(c);
  // And clear the free block indicator
  TS_POOL_NBLOCK(c) &= (~TS_POOL_FREELIST_MASK);
}

static void ts_pool_assimilate_up(ts_pool_t *heap, uint16_t c) {
  if( TS_POOL_NBLOCK(TS_POOL_NBLOCK(c)) & TS_POOL_FREELIST_MASK ) {
    // The next block is a free block, so assimilate up and remove it from the free list
    // Disconnect the next block from the FREE list

    ts_pool_disconnect_from_free_list(heap, TS_POOL_NBLOCK(c) );

    // Assimilate the next block with this one

    TS_POOL_PBLOCK(TS_POOL_NBLOCK(TS_POOL_NBLOCK(c)) & TS_POOL_BLOCKNO_MASK) = c;
    TS_POOL_NBLOCK(c) = TS_POOL_NBLOCK(TS_POOL_NBLOCK(c)) & TS_POOL_BLOCKNO_MASK;
  } 
}

static uint16_t ts_pool_assimilate_down(ts_pool_t *heap,  uint16_t c, uint16_t freemask) {
  TS_POOL_NBLOCK(TS_POOL_PBLOCK(c)) = TS_POOL_NBLOCK(c) | freemask;
  TS_POOL_PBLOCK(TS_POOL_NBLOCK(c)) = TS_POOL_PBLOCK(c);
  return ( TS_POOL_PBLOCK(c) );
}

const char * ts_pool_init(ts_pool_t *heap, void *mem, size_t mem_sz) {
  size_t numblocks = mem_sz / sizeof(ts_pool_block);
  if(mem == NULL) {
    heap->heap  = (ts_pool_block *) malloc(numblocks*sizeof(ts_pool_block));
    if(heap->heap == NULL) {
      heap->allocd    = 0;
      heap->numblocks = 0;
      return "OOM";
    }    
    heap->allocd= 1;
  } else {
    heap->heap = (ts_pool_block *) mem;
    heap->allocd= 0;
  }
  memset(heap->heap, 0, numblocks * sizeof(ts_pool_block));
  heap->numblocks = numblocks;
  return NULL;
}

void ts_pool_deinit(ts_pool_t *heap) {
  if(heap->allocd)
    free(heap->heap);
  memset(heap, 0, sizeof(ts_pool_t));
}

void ts_pool_free(ts_pool_t *heap, void *ptr) {
  uint16_t c;

  // If we're being asked to free a NULL pointer, well that's just silly!

  if( NULL == ptr )
    return;
  
  // FIXME: At some point it might be a good idea to add a check to make sure
  //        that the pointer we're being asked to free up is actually within
  //        the ts_pool_heap!
  //
  // NOTE:  See the new ts_pool_info() function that you can use to see if a ptr is
  //        on the free list!

  // Protect the critical section...
  //
  TS_POOL_CRITICAL_ENTRY();
  
  // Figure out which block we're in. Note the use of truncated division...

  c = ((char *)ptr-(char *)(&(heap->heap[0])))/sizeof(ts_pool_block);

  // Now let's assimilate this block with the next one if possible.
  
  ts_pool_assimilate_up(heap, c );

  // Then assimilate with the previous block if possible

  if( TS_POOL_NBLOCK(TS_POOL_PBLOCK(c)) & TS_POOL_FREELIST_MASK ) {
    c = ts_pool_assimilate_down(heap, c, TS_POOL_FREELIST_MASK);
  } else {
    // The previous block is not a free block, so add this one to the head of the free list
    TS_POOL_PFREE(TS_POOL_NFREE(0)) = c;
    TS_POOL_NFREE(c)   = TS_POOL_NFREE(0);
    TS_POOL_PFREE(c)   = 0;
    TS_POOL_NFREE(0)   = c;
    TS_POOL_NBLOCK(c) |= TS_POOL_FREELIST_MASK;
  }

  // Release the critical section...
  //
  TS_POOL_CRITICAL_EXIT();
}

void ts_pool_freeall(ts_pool_t *heap) {
  memset(heap->heap, 0, heap->numblocks * sizeof(ts_pool_block));
}

void * ts_pool_malloc(ts_pool_t *heap, size_t size) {
  uint16_t          blocks;
  volatile uint16_t blockSize;  // why is this volatile (for thread safety?)
  uint16_t          bestSize;
  uint16_t          bestBlock;
  uint16_t          cf;

  // the very first thing we do is figure out if we're being asked to allocate
  // a size of 0 - and if we are we'll simply return a null pointer. if not
  // then reduce the size by 1 byte so that the subsequent calculations on
  // the number of blocks to allocate are easier...

  if( 0 == size )
    return NULL;

  TS_POOL_CRITICAL_ENTRY();

  blocks = ts_pool_blocks( size );

  // Now we can scan through the free list until we find a space that's big
  // enough to hold the number of blocks we need.
  //
  // This part may be customized to be a best-fit, worst-fit, or first-fit
  // algorithm

  cf = TS_POOL_NFREE(0);

  bestBlock = TS_POOL_NFREE(0);
  bestSize  = 0x7FFF;

  while( TS_POOL_NFREE(cf) ) {
    blockSize = (TS_POOL_NBLOCK(cf) & TS_POOL_BLOCKNO_MASK) - cf;

#if defined TS_POOL_FIRST_FIT
    // This is the first block that fits!
    if( (blockSize >= blocks) )
        break;
#elif defined TS_POOL_BEST_FIT
    if( (blockSize >= blocks) && (blockSize < bestSize) ) {
      bestBlock = cf;
      bestSize  = blockSize;
      if(blockSize == blocks)   // perfect size found, not need to keep searching
        break;
    }
#endif

    cf = TS_POOL_NFREE(cf);
  }

  if( 0x7FFF != bestSize ) {
    cf        = bestBlock;
    blockSize = bestSize;
  }

  if( TS_POOL_NBLOCK(cf) & TS_POOL_BLOCKNO_MASK ) {
    // This is an existing block in the memory heap, we just need to split off
    // what we need, unlink it from the free list and mark it as in use, and
    // link the rest of the block back into the freelist as if it was a new
    // block on the free list...

    if( blockSize == blocks ) {
      // It's an exact fit and we don't neet to split off a block.
      // Disconnect this block from the FREE list

      ts_pool_disconnect_from_free_list( heap, cf );

    } else {
     // It's not an exact fit and we need to split off a block.

     ts_pool_make_new_block(heap, cf, blockSize-blocks, TS_POOL_FREELIST_MASK );

     cf += blockSize-blocks;
     }
  } else {
    // We're at the end of the heap - allocate a new block, but check to see if
    // there's enough memory left for the requested block! Actually, we may need
    // one more than that if we're initializing the ts_pool_heap for the first
    // time, which happens in the next conditional...

    if( heap->numblocks <= cf+blocks+1 ) {
      TS_POOL_CRITICAL_EXIT();
      return NULL;
    }

    // Now check to see if we need to initialize the free list...this assumes
    // that the BSS is set to 0 on startup. We should rarely get to the end of
    // the free list so this is the "cheapest" place to put the initialization!

    if( 0 == cf ) {
      TS_POOL_NBLOCK(0) = 1;
      TS_POOL_NFREE(0)  = 1;
      cf            = 1;
    }

    TS_POOL_NFREE(TS_POOL_PFREE(cf)) = cf+blocks;

    memcpy( &TS_POOL_BLOCK(cf+blocks), &TS_POOL_BLOCK(cf), sizeof(ts_pool_block) );

    TS_POOL_NBLOCK(cf)           = cf+blocks;
    TS_POOL_PBLOCK(cf+blocks)    = cf;
  }

  // Release the critical section...
  //
  TS_POOL_CRITICAL_EXIT();

  return( (void *)&TS_POOL_DATA(cf) );
}

void * ts_pool_calloc(ts_pool_t *heap, size_t n, size_t len) {
  size_t sz = len * n;
  void *p = ts_pool_malloc(heap, sz);
  return p ? memset(p, 0, sz) : NULL;
}

void * ts_pool_realloc(ts_pool_t *heap, void *ptr, size_t size ) {
  uint16_t  blocks;
  uint16_t  blockSize;
  uint16_t  c;
  size_t    curSize;

  // This code looks after the case of a NULL value for ptr. The ANSI C
  // standard says that if ptr is NULL and size is non-zero, then we've
  // got to work the same a malloc(). If size is also 0, then our version
  // of malloc() returns a NULL pointer, which is OK as far as the ANSI C
  // standard is concerned.

  if( NULL == ptr )
    return ts_pool_malloc(heap, size);

  // Now we're sure that we have a non_NULL ptr, but we're not sure what
  // we should do with it. If the size is 0, then the ANSI C standard says that
  // we should operate the same as free.

  if( 0 == size ) {
    ts_pool_free(heap, ptr);
    return NULL;
  }

  // Protect the critical section...
  //
  TS_POOL_CRITICAL_ENTRY();

  // Otherwise we need to actually do a reallocation. A naiive approach
  // would be to malloc() a new block of the correct size, copy the old data
  // to the new block, and then free the old block.
  //
  // While this will work, we end up doing a lot of possibly unnecessary
  // copying. So first, let's figure out how many blocks we'll need.

  blocks = ts_pool_blocks( size );

  // Figure out which block we're in. Note the use of truncated division...

  c = ((char *)ptr-(char *)(&(heap->heap[0])))/sizeof(ts_pool_block);

  // Figure out how big this block is...

  blockSize = (TS_POOL_NBLOCK(c) - c);

  // Figure out how many bytes are in this block
    
  curSize   = (blockSize*sizeof(ts_pool_block))-(sizeof(((ts_pool_block *)0)->header));

  // Ok, now that we're here, we know the block number of the original chunk
  // of memory, and we know how much new memory we want, and we know the original
  // block size...

  if( blockSize == blocks ) {
    // This space intentionally left blank - return the original pointer!

    // Release the critical section...
    //
    TS_POOL_CRITICAL_EXIT();

    return ptr;
  }

  // Now we have a block size that could be bigger or smaller. Either
  // way, try to assimilate up to the next block before doing anything...
  //
  // If it's still too small, we have to free it anyways and it will save the
  // assimilation step later in free :-)

  ts_pool_assimilate_up(heap, c );

  // Now check if it might help to assimilate down, but don't actually
  // do the downward assimilation unless the resulting block will hold the
  // new request! If this block of code runs, then the new block will
  // either fit the request exactly, or be larger than the request.

  if( (TS_POOL_NBLOCK(TS_POOL_PBLOCK(c)) & TS_POOL_FREELIST_MASK) &&
      (blocks <= (TS_POOL_NBLOCK(c)-TS_POOL_PBLOCK(c)))    ) {
  
    // Check if the resulting block would be big enough...

    // Disconnect the previous block from the FREE list

    ts_pool_disconnect_from_free_list(heap, TS_POOL_PBLOCK(c) );

    // Connect the previous block to the next block ... and then
    // realign the current block pointer

    c = ts_pool_assimilate_down(heap, c, 0);

    // Move the bytes down to the new block we just created, but be sure to move
    // only the original bytes.

    memmove( (void *)&TS_POOL_DATA(c), ptr, curSize );
 
    // And don't forget to adjust the pointer to the new block location!
        
    ptr = (void *)&TS_POOL_DATA(c);
  }

  // Now calculate the block size again...and we'll have three cases

  blockSize = (TS_POOL_NBLOCK(c) - c);

  if( blockSize == blocks ) {
    // This space intentionally left blank - return the original pointer!

  } else if (blockSize > blocks ) {
    // New block is smaller than the old block, so just make a new block
    // at the end of this one and put it up on the free list...

    ts_pool_make_new_block(heap, c, blocks, 0 );
    
    ts_pool_free(heap, (void *)&TS_POOL_DATA(c+blocks) );
  } else {
    // New block is bigger than the old block...
    
    void *oldptr = ptr;

    // Now ts_pool_malloc() a new/ one, copy the old data to the new block, and
    // free up the old block, but only if the malloc was sucessful!

    if( (ptr = ts_pool_malloc(heap, size)) ) {
      memcpy( ptr, oldptr, curSize );
      ts_pool_free(heap, oldptr );
    }
    
  }

  // Release the critical section...
  //
  TS_POOL_CRITICAL_EXIT();

  return( ptr );
}

#endif

