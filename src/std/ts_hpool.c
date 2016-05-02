#ifdef USE_TS_HPOOL

#define TS_HPOOL_ATTPACKPRE
#define TS_HPOOL_ATTPACKSUF __attribute__((__packed__))

#ifndef TS_HPOOL_CRITICAL_ENTRY
  #define TS_HPOOL_CRITICAL_ENTRY()
#endif
#ifndef TS_HPOOL_CRITICAL_EXIT
  #define TS_HPOOL_CRITICAL_EXIT()
#endif

#ifndef TS_HPOOL_FIRST_FIT
#  ifndef TS_HPOOL_BEST_FIT
#    define TS_HPOOL_BEST_FIT
#  endif
#endif

TS_HPOOL_ATTPACKPRE typedef struct ts_hpool_ptr {
  uint16_t next;
  uint16_t prev;
} TS_HPOOL_ATTPACKSUF ts_hpool_ptr;

#if INTPTR_MAX == INT32_MAX

TS_HPOOL_ATTPACKPRE typedef struct ts_hpool_block {
  union {
    ts_hpool_ptr  used;
  } header;
  struct {
    uint16_t parent,  child;
    uint16_t next,    prev;
  } hier;
  union {
    ts_hpool_ptr  free;
    uint8_t       data[4];
  } body;
} TS_HPOOL_ATTPACKSUF ts_hpool_block;


#else

// on 64-bit architecture, I want to ensure that pointers are aligned on 8byte boundary
// so sizeof(ts_hpool_block) = 24 as opposed to 16 which is easier to divided into
// fortunately, divide by a constant 24 is optimized into a multiple and right shift.

TS_HPOOL_ATTPACKPRE typedef struct ts_hpool_block {
  union {
    ts_hpool_ptr  used;
  } header;
  struct {
    uint16_t parent,  child;
    uint16_t next,    prev;
  } hier;
  uint8_t padding[4];
  union {
    ts_hpool_ptr  free;
    uint8_t       data[8];
  } body;
} TS_HPOOL_ATTPACKSUF ts_hpool_block;


#endif

struct ts_hpool_t {
  ts_hpool_block   *heap;
  uint16_t          numblocks;
  uint16_t          allocd;
};


#define TS_HPOOL_FREELIST_MASK (0x8000)
#define TS_HPOOL_BLOCKNO_MASK  (0x7FFF)
#define TS_HPOOL_BLOCK(b)  (heap->heap[b])
#define TS_HPOOL_NBLOCK(b) (TS_HPOOL_BLOCK(b).header.used.next)
#define TS_HPOOL_PBLOCK(b) (TS_HPOOL_BLOCK(b).header.used.prev)
#define TS_HPOOL_NFREE(b)  (TS_HPOOL_BLOCK(b).body.free.next)
#define TS_HPOOL_PFREE(b)  (TS_HPOOL_BLOCK(b).body.free.prev)
#define TS_HPOOL_DATA(b)   (TS_HPOOL_BLOCK(b).body.data)
#define TS_HPOOL_PARENT(b) (TS_HPOOL_BLOCK(b).hier.parent)
#define TS_HPOOL_CHILD(b)  (TS_HPOOL_BLOCK(b).hier.child)
#define TS_HPOOL_NSIBL(b)  (TS_HPOOL_BLOCK(b).hier.next)
#define TS_HPOOL_PSIBL(b)  (TS_HPOOL_BLOCK(b).hier.prev)

typedef struct ts_hpool_info_t {
  uint16_t totalEntries,  usedEntries,  freeEntries; 
  uint16_t totalBlocks,   usedBlocks,   freeBlocks; 
} ts_hpool_info_t;

void *ts_hpool_info(ts_hpool_t *heap, void *ptr) {
  ts_hpool_info_t heapInfo;
  unsigned short int blockNo = 0;

  // Protect the critical section...
  //
  TS_HPOOL_CRITICAL_ENTRY();
  
  // Clear out all of the entries in the heapInfo structure before doing
  // any calculations..
  //
  memset( &heapInfo, 0, sizeof( heapInfo ) );

  printf("\n\nDumping the ts_hpool_heap...\n" );

  printf("|0x%" PRIXPTR "|B %5i|NB %5i|PB %5i|Z %5i|                                   |NF %5i|PF %5i|\n",
          (uintptr_t)(&TS_HPOOL_BLOCK(blockNo)),
          blockNo,
          TS_HPOOL_NBLOCK(blockNo) & TS_HPOOL_BLOCKNO_MASK,
          TS_HPOOL_PBLOCK(blockNo),
          (TS_HPOOL_NBLOCK(blockNo) & TS_HPOOL_BLOCKNO_MASK )-blockNo,
          TS_HPOOL_NFREE(blockNo),
          TS_HPOOL_PFREE(blockNo) );

  // Now loop through the block lists, and keep track of the number and size
  // of used and free blocks. The terminating condition is an nb pointer with
  // a value of zero...
  
  blockNo = TS_HPOOL_NBLOCK(blockNo) & TS_HPOOL_BLOCKNO_MASK;

  while( TS_HPOOL_NBLOCK(blockNo) & TS_HPOOL_BLOCKNO_MASK ) {
    ++heapInfo.totalEntries;
    heapInfo.totalBlocks += (TS_HPOOL_NBLOCK(blockNo) & TS_HPOOL_BLOCKNO_MASK )-blockNo;

    // Is this a free block?

    if( TS_HPOOL_NBLOCK(blockNo) & TS_HPOOL_FREELIST_MASK ) {
      ++heapInfo.freeEntries;
      heapInfo.freeBlocks += (TS_HPOOL_NBLOCK(blockNo) & TS_HPOOL_BLOCKNO_MASK )-blockNo;

      printf("|0x%" PRIXPTR "|B %5i|NB %5i|PB %5i|Z %5i|                                   |NF %5i|PF %5i|\n",
              (uintptr_t)(&TS_HPOOL_BLOCK(blockNo)),
              blockNo,
              TS_HPOOL_NBLOCK(blockNo) & TS_HPOOL_BLOCKNO_MASK,
              TS_HPOOL_PBLOCK(blockNo),
              (TS_HPOOL_NBLOCK(blockNo) & TS_HPOOL_BLOCKNO_MASK )-blockNo,
              TS_HPOOL_NFREE(blockNo),
              TS_HPOOL_PFREE(blockNo) );
     
      // Does this block address match the ptr we may be trying to free?

      if( ptr == &TS_HPOOL_BLOCK(blockNo) ) {
       
        // Release the critical section...
        //
        TS_HPOOL_CRITICAL_EXIT();
 
        return( ptr );
      }
    } else {
      ++heapInfo.usedEntries;
      heapInfo.usedBlocks += (TS_HPOOL_NBLOCK(blockNo) & TS_HPOOL_BLOCKNO_MASK )-blockNo;

      printf("|0x%" PRIXPTR "|B %5i|NB %5i|PB %5i|Z %5i|PA %5i|CH %5i|NS %5i|PS %5i|\n",
              (uintptr_t)(&TS_HPOOL_BLOCK(blockNo)),
              blockNo,
              TS_HPOOL_NBLOCK(blockNo) & TS_HPOOL_BLOCKNO_MASK,
              TS_HPOOL_PBLOCK(blockNo),
              (TS_HPOOL_NBLOCK(blockNo) & TS_HPOOL_BLOCKNO_MASK )-blockNo,
              TS_HPOOL_PARENT(blockNo),
              TS_HPOOL_CHILD(blockNo),
              TS_HPOOL_NSIBL(blockNo),
              TS_HPOOL_PSIBL(blockNo));
    }

    blockNo = TS_HPOOL_NBLOCK(blockNo) & TS_HPOOL_BLOCKNO_MASK;
  }

  // Update the accounting totals with information from the last block, the
  // rest must be free!

  heapInfo.freeBlocks  += heap->numblocks - blockNo;
  heapInfo.totalBlocks += heap->numblocks - blockNo;

  printf("|0x%" PRIXPTR "|B %5i|NB %5i|PB %5i|Z %5i|                                   |NF %5i|PF %5i|\n",
          (uintptr_t)(&TS_HPOOL_BLOCK(blockNo)),
          blockNo,
          TS_HPOOL_NBLOCK(blockNo) & TS_HPOOL_BLOCKNO_MASK,
          TS_HPOOL_PBLOCK(blockNo),
          heap->numblocks - blockNo,
          TS_HPOOL_NFREE(blockNo),
          TS_HPOOL_PFREE(blockNo) );

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
  TS_HPOOL_CRITICAL_EXIT();
 
  return( NULL );
}

static uint16_t ts_hpool_blocks( size_t size ) {

  // The calculation of the block size is not too difficult, but there are
  // a few little things that we need to be mindful of.
  //
  // When a block removed from the free list, the space used by the free
  // pointers is available for data. That's what the first calculation
  // of size is doing.

  if( size <= (sizeof(((ts_hpool_block *)0)->body)) )
    return( 1 );

  // If it's for more than that, then we need to figure out the number of
  // additional whole blocks the size of an ts_hpool_block are required.

  size -= ( 1 + (sizeof(((ts_hpool_block *)0)->body)) );

  return( 2 + size/(sizeof(ts_hpool_block)) );
}



static void ts_hpool_make_new_block(ts_hpool_t *heap, uint16_t c, uint16_t blocks, uint16_t freemask) {
  TS_HPOOL_NBLOCK(c+blocks) = TS_HPOOL_NBLOCK(c) & TS_HPOOL_BLOCKNO_MASK;
  TS_HPOOL_PBLOCK(c+blocks) = c;
  TS_HPOOL_PARENT(c+blocks) = 0;
  TS_HPOOL_CHILD(c+blocks)  = 0;
  TS_HPOOL_NSIBL(c+blocks)  = 0;
  TS_HPOOL_PSIBL(c+blocks)  = 0;
  TS_HPOOL_PBLOCK(TS_HPOOL_NBLOCK(c) & TS_HPOOL_BLOCKNO_MASK) = (c+blocks);
  TS_HPOOL_NBLOCK(c) = (c+blocks) | freemask;
}

static void ts_hpool_disconnect_from_free_list(ts_hpool_t *heap, uint16_t c) {
  // Disconnect this block from the FREE list
  TS_HPOOL_NFREE(TS_HPOOL_PFREE(c)) = TS_HPOOL_NFREE(c);
  TS_HPOOL_PFREE(TS_HPOOL_NFREE(c)) = TS_HPOOL_PFREE(c);
  // And clear the free block indicator
  TS_HPOOL_NBLOCK(c) &= (~TS_HPOOL_FREELIST_MASK);
}

static void ts_hpool_assimilate_up(ts_hpool_t *heap, uint16_t c) {
  if( TS_HPOOL_NBLOCK(TS_HPOOL_NBLOCK(c)) & TS_HPOOL_FREELIST_MASK ) {
    // The next block is a free block, so assimilate up and remove it from the free list
    // Disconnect the next block from the FREE list

    ts_hpool_disconnect_from_free_list(heap, TS_HPOOL_NBLOCK(c) );

    // Assimilate the next block with this one

    TS_HPOOL_PBLOCK(TS_HPOOL_NBLOCK(TS_HPOOL_NBLOCK(c)) & TS_HPOOL_BLOCKNO_MASK) = c;
    TS_HPOOL_NBLOCK(c) = TS_HPOOL_NBLOCK(TS_HPOOL_NBLOCK(c)) & TS_HPOOL_BLOCKNO_MASK;
  } 
}

static uint16_t ts_hpool_assimilate_down(ts_hpool_t *heap,  uint16_t c, uint16_t freemask) {
  TS_HPOOL_NBLOCK(TS_HPOOL_PBLOCK(c)) = TS_HPOOL_NBLOCK(c) | freemask;
  TS_HPOOL_PBLOCK(TS_HPOOL_NBLOCK(c)) = TS_HPOOL_PBLOCK(c);
  return ( TS_HPOOL_PBLOCK(c) );
}

static void ts_hpool_freechild(ts_hpool_t *heap, uint16_t c) {
  if(c) {
    // unlink parent
    TS_HPOOL_CHILD(TS_HPOOL_PARENT(c)) = 0;
    uint16_t i, inext;
    for(i = c, inext = TS_HPOOL_NSIBL(i); 
      i != 0 ; 
      i = inext, inext = TS_HPOOL_NSIBL(i)) 
    {
      ts_hpool_free(heap, (void *)&TS_HPOOL_DATA(i));
    }        
  }
}

static void ts_hpool_relink_hier(ts_hpool_t *heap, uint16_t c, uint16_t newc) {
  if(TS_HPOOL_PSIBL(c)) {
    TS_HPOOL_PSIBL(newc) = TS_HPOOL_PSIBL(c);
    TS_HPOOL_NSIBL(TS_HPOOL_PSIBL(c)) = newc;
    TS_HPOOL_PSIBL(c) = 0;
  }
  if(TS_HPOOL_NSIBL(c)) {
    TS_HPOOL_NSIBL(newc) = TS_HPOOL_NSIBL(c);
    TS_HPOOL_PSIBL(TS_HPOOL_NSIBL(c)) = newc;
    TS_HPOOL_NSIBL(c) = 0;
  }
  if(TS_HPOOL_PARENT(c)) {
    TS_HPOOL_PARENT(newc) = TS_HPOOL_PARENT(c);
    if(TS_HPOOL_CHILD(TS_HPOOL_PARENT(c)) == c) {
      TS_HPOOL_CHILD(TS_HPOOL_PARENT(c)) = newc;
    }
    TS_HPOOL_PARENT(c) = 0;
  }
  
  
  if(TS_HPOOL_CHILD(c)) {
    TS_HPOOL_CHILD(newc) = TS_HPOOL_CHILD(c);
    for(uint16_t i = TS_HPOOL_CHILD(c); i != 0 ; i = TS_HPOOL_NSIBL(i)) {
      TS_HPOOL_PARENT(i) = newc;
    }
    TS_HPOOL_CHILD(c) = 0;
  }
}

const char * ts_hpool_init(ts_hpool_t *heap, void *mem, size_t mem_sz) {
  size_t numblocks = mem_sz / sizeof(ts_hpool_block);
  if(mem == NULL) {
    heap->heap  = (ts_hpool_block *) malloc(numblocks*sizeof(ts_hpool_block));
    if(heap->heap == NULL) {
      heap->allocd    = 0;
      heap->numblocks = 0;
      return "OOM";
    }
    heap->allocd= 1;
  } else {
    heap->heap  = (ts_hpool_block *) mem;
    heap->allocd= 0;
  }
  memset(heap->heap, 0, numblocks * sizeof(ts_hpool_block));
  heap->numblocks = numblocks;
  return NULL;
}

void ts_hpool_deinit(ts_hpool_t *heap) {
  if(heap->allocd)
    free(heap->heap);
  memset(heap, 0, sizeof(ts_hpool_t));
}

void ts_hpool_free(ts_hpool_t *heap, void *ptr) {
  uint16_t c;

  // If we're being asked to free a NULL pointer, well that's just silly!

  if( NULL == ptr )
    return;
  
  // FIXME: At some point it might be a good idea to add a check to make sure
  //        that the pointer we're being asked to free up is actually within
  //        the ts_hpool_heap!
  //
  // NOTE:  See the new ts_hpool_info() function that you can use to see if a ptr is
  //        on the free list!

  // Figure out which block we're in. Note the use of truncated division...

  c = ((char *)ptr-(char *)(&(heap->heap[0])))/sizeof(ts_hpool_block);
  
  // relink sibling
  if(TS_HPOOL_CHILD(TS_HPOOL_PARENT(c)) == c) TS_HPOOL_CHILD(TS_HPOOL_PARENT(c)) = TS_HPOOL_NSIBL(c);
  if(TS_HPOOL_PSIBL(c)) TS_HPOOL_NSIBL(TS_HPOOL_PSIBL(c)) = TS_HPOOL_NSIBL(c);
  if(TS_HPOOL_NSIBL(c)) TS_HPOOL_PSIBL(TS_HPOOL_NSIBL(c)) = TS_HPOOL_PSIBL(c);


  // free all children recursively
  ts_hpool_freechild(heap, TS_HPOOL_CHILD(c));
  
  // Protect the critical section...
  //
  TS_HPOOL_CRITICAL_ENTRY();

  // Now let's assimilate this block with the next one if possible.
  
  ts_hpool_assimilate_up(heap, c );

  // Then assimilate with the previous block if possible

  if( TS_HPOOL_NBLOCK(TS_HPOOL_PBLOCK(c)) & TS_HPOOL_FREELIST_MASK ) {
    c = ts_hpool_assimilate_down(heap, c, TS_HPOOL_FREELIST_MASK);
  } else {
    // The previous block is not a free block, so add this one to the head of the free list
    TS_HPOOL_PFREE(TS_HPOOL_NFREE(0)) = c;
    TS_HPOOL_NFREE(c)   = TS_HPOOL_NFREE(0);
    TS_HPOOL_PFREE(c)   = 0;
    TS_HPOOL_NFREE(0)   = c;
    TS_HPOOL_NBLOCK(c) |= TS_HPOOL_FREELIST_MASK;
    TS_HPOOL_NSIBL(c)   = 0;
    TS_HPOOL_PSIBL(c)   = 0;    
  }

  // Release the critical section...
  //
  TS_HPOOL_CRITICAL_EXIT();
}

void ts_hpool_freeall(ts_hpool_t *heap) {
  memset(heap->heap, 0, heap->numblocks * sizeof(ts_hpool_block));
}

void * ts_hpool_malloc(ts_hpool_t *heap, size_t size) {
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

  TS_HPOOL_CRITICAL_ENTRY();

  blocks = ts_hpool_blocks( size );

  // Now we can scan through the free list until we find a space that's big
  // enough to hold the number of blocks we need.
  //
  // This part may be customized to be a best-fit, worst-fit, or first-fit
  // algorithm

  cf = TS_HPOOL_NFREE(0);

  bestBlock = TS_HPOOL_NFREE(0);
  bestSize  = 0x7FFF;

  while( TS_HPOOL_NFREE(cf) ) {
    blockSize = (TS_HPOOL_NBLOCK(cf) & TS_HPOOL_BLOCKNO_MASK) - cf;

#if defined TS_HPOOL_FIRST_FIT
    // This is the first block that fits!
    if( (blockSize >= blocks) )
        break;
#elif defined TS_HPOOL_BEST_FIT
    if( (blockSize >= blocks) && (blockSize < bestSize) ) {
      bestBlock = cf;
      bestSize  = blockSize;
      if(blockSize == blocks)   // perfect size found, not need to keep searching
        break;
    }
#endif

    cf = TS_HPOOL_NFREE(cf);
  }

  if( 0x7FFF != bestSize ) {
    cf        = bestBlock;
    blockSize = bestSize;
  }

  if( TS_HPOOL_NBLOCK(cf) & TS_HPOOL_BLOCKNO_MASK ) {
    // This is an existing block in the memory heap, we just need to split off
    // what we need, unlink it from the free list and mark it as in use, and
    // link the rest of the block back into the freelist as if it was a new
    // block on the free list...

    if( blockSize == blocks ) {
      // It's an exact fit and we don't neet to split off a block.
      // Disconnect this block from the FREE list

      ts_hpool_disconnect_from_free_list( heap, cf );

    } else {
     // It's not an exact fit and we need to split off a block.

     ts_hpool_make_new_block(heap, cf, blockSize-blocks, TS_HPOOL_FREELIST_MASK );

     cf += blockSize-blocks;
     }
  } else {
    // We're at the end of the heap - allocate a new block, but check to see if
    // there's enough memory left for the requested block! Actually, we may need
    // one more than that if we're initializing the ts_hpool_heap for the first
    // time, which happens in the next conditional...

    if( heap->numblocks <= cf+blocks+1 ) {
      TS_HPOOL_CRITICAL_EXIT();
      return NULL;
    }

    // Now check to see if we need to initialize the free list...this assumes
    // that the BSS is set to 0 on startup. We should rarely get to the end of
    // the free list so this is the "cheapest" place to put the initialization!

    if( 0 == cf ) {
      TS_HPOOL_NBLOCK(0) = 1;
      TS_HPOOL_NFREE(0)  = 1;
      cf            = 1;
    }

    TS_HPOOL_NFREE(TS_HPOOL_PFREE(cf)) = cf+blocks;

    memcpy( &TS_HPOOL_BLOCK(cf+blocks), &TS_HPOOL_BLOCK(cf), sizeof(ts_hpool_block) );

    TS_HPOOL_NBLOCK(cf)           = cf+blocks;
    TS_HPOOL_PBLOCK(cf+blocks)    = cf;
  }

  // Release the critical section...
  //
  TS_HPOOL_CRITICAL_EXIT();

  return( (void *)&TS_HPOOL_DATA(cf) );
}

void * ts_hpool_calloc(ts_hpool_t *heap, size_t n, size_t len) {
  size_t sz = len * n;
  void *p = ts_hpool_malloc(heap, sz);
  return p ? memset(p, 0, sz) : NULL;
}

void * ts_hpool_realloc(ts_hpool_t *heap, void *ptr, size_t size ) {
  uint16_t  blocks;
  uint16_t  blockSize;
  uint16_t  c, newc;
  size_t    curSize;

  // This code looks after the case of a NULL value for ptr. The ANSI C
  // standard says that if ptr is NULL and size is non-zero, then we've
  // got to work the same a malloc(). If size is also 0, then our version
  // of malloc() returns a NULL pointer, which is OK as far as the ANSI C
  // standard is concerned.

  if( NULL == ptr )
    return ts_hpool_malloc(heap, size);

  // Now we're sure that we have a non_NULL ptr, but we're not sure what
  // we should do with it. If the size is 0, then the ANSI C standard says that
  // we should operate the same as free.

  if( 0 == size ) {
    ts_hpool_free(heap, ptr);
    return NULL;
  }

  // Protect the critical section...
  //
  TS_HPOOL_CRITICAL_ENTRY();

  // Otherwise we need to actually do a reallocation. A naiive approach
  // would be to malloc() a new block of the correct size, copy the old data
  // to the new block, and then free the old block.
  //
  // While this will work, we end up doing a lot of possibly unnecessary
  // copying. So first, let's figure out how many blocks we'll need.

  blocks = ts_hpool_blocks( size );

  // Figure out which block we're in. Note the use of truncated division...

  c = ((char *)ptr-(char *)(&(heap->heap[0])))/sizeof(ts_hpool_block);

  // Figure out how big this block is...

  blockSize = (TS_HPOOL_NBLOCK(c) - c);

  // Figure out how many bytes are in this block
    
  curSize   = (blockSize*sizeof(ts_hpool_block))-(sizeof(((ts_hpool_block *)0)->header));

  // Ok, now that we're here, we know the block number of the original chunk
  // of memory, and we know how much new memory we want, and we know the original
  // block size...

  if( blockSize == blocks ) {
    // This space intentionally left blank - return the original pointer!

    // Release the critical section...
    //
    TS_HPOOL_CRITICAL_EXIT();

    return ptr;
  }

  // Now we have a block size that could be bigger or smaller. Either
  // way, try to assimilate up to the next block before doing anything...
  //
  // If it's still too small, we have to free it anyways and it will save the
  // assimilation step later in free :-)

  ts_hpool_assimilate_up(heap, c );

  // Now check if it might help to assimilate down, but don't actually
  // do the downward assimilation unless the resulting block will hold the
  // new request! If this block of code runs, then the new block will
  // either fit the request exactly, or be larger than the request.

  if( (TS_HPOOL_NBLOCK(TS_HPOOL_PBLOCK(c)) & TS_HPOOL_FREELIST_MASK) &&
      (blocks <= (TS_HPOOL_NBLOCK(c)-TS_HPOOL_PBLOCK(c)))    ) {
  
    // Check if the resulting block would be big enough...

    // Disconnect the previous block from the FREE list

    ts_hpool_disconnect_from_free_list(heap, TS_HPOOL_PBLOCK(c) );

    // Connect the previous block to the next block ... and then
    // realign the current block pointer

    newc = ts_hpool_assimilate_down(heap, c, 0);

    ts_hpool_relink_hier(heap, c, newc);
        
    // Move the bytes down to the new block we just created, but be sure to move
    // only the original bytes.

    memmove( (void *)&TS_HPOOL_DATA(newc), ptr, curSize );
 
    // And don't forget to adjust the pointer to the new block location!
        
    ptr = (void *)&TS_HPOOL_DATA(newc);
    c = newc;
  }

  // Now calculate the block size again...and we'll have three cases

  blockSize = (TS_HPOOL_NBLOCK(c) - c);

  if( blockSize == blocks ) {
    // This space intentionally left blank - return the original pointer!

  } else if (blockSize > blocks ) {
    // New block is smaller than the old block, so just make a new block
    // at the end of this one and put it up on the free list...

    ts_hpool_make_new_block(heap, c, blocks, 0 );
    
    ts_hpool_free(heap, (void *)&TS_HPOOL_DATA(c+blocks) );
  } else {
    // New block is bigger than the old block...
    
    void *oldptr = ptr;

    // Now ts_hpool_malloc() a new/ one, copy the old data to the new block, and
    // free up the old block, but only if the malloc was sucessful!

    if( (ptr = ts_hpool_malloc(heap, size)) ) {
      newc = ((char *)ptr-(char *)(&(heap->heap[0])))/sizeof(ts_hpool_block);
      
      ts_hpool_relink_hier(heap, c, newc);
      
      memcpy( ptr, oldptr, curSize );
      ts_hpool_free(heap, oldptr );
    }
    
  }

  // Release the critical section...
  //
  TS_HPOOL_CRITICAL_EXIT();

  return( ptr );
}

void ts_hpool_attach(ts_hpool_t *heap, void *ptr, void *parent) {
  uint16_t c, cparent;
  
  if(NULL == ptr)
    return;
  
  c = ((char *)ptr-(char *)(&(heap->heap[0])))/sizeof(ts_hpool_block);
  cparent = ((char *)parent-(char *)(&(heap->heap[0])))/sizeof(ts_hpool_block);
  
  // relink sibling
  if(TS_HPOOL_PSIBL(c)) TS_HPOOL_NSIBL(TS_HPOOL_PSIBL(c)) = TS_HPOOL_NSIBL(c);
  if(TS_HPOOL_NSIBL(c)) TS_HPOOL_PSIBL(TS_HPOOL_NSIBL(c)) = TS_HPOOL_PSIBL(c);
  
  // detach parent
  if(TS_HPOOL_CHILD(TS_HPOOL_PARENT(c)) == c) {
    TS_HPOOL_CHILD(TS_HPOOL_PARENT(c)) = TS_HPOOL_NSIBL(c);
    
  }
  
  // attach to new parent
  TS_HPOOL_PARENT(c) = cparent;
  TS_HPOOL_PSIBL(c) = 0;
  TS_HPOOL_NSIBL(c) = TS_HPOOL_CHILD(cparent);
  TS_HPOOL_PSIBL(TS_HPOOL_CHILD(cparent)) = c;
  TS_HPOOL_CHILD(cparent) = c;
}



#endif
