

#ifndef DSM_BITMAP_IS_DEF
#define DSM_BITMAP_IS_DEF

#include "sys/bitmap.h"
#include "marcel.h" // tmalloc

typedef unsigned int* dsm_bitmap_t;

/* For the following function, size is in bits */

#define dsm_bitmap_alloc(size) \
  ((dsm_bitmap_t) tcalloc(((size) % 8) ? ((size) / 8 + 1) : ((size) / 8), 1))
  
#define dsm_bitmap_free(b) tfree(b)

#define dsm_bitmap_mark_dirty(offset, length, bitmap) set_bits_to_1(offset, length, bitmap)

#define dsm_bitmap_is_empty(b, size) bitmap_is_empty(b, size)

#define dsm_bitmap_clear(b,size) clear_bitmap(b, size)

#endif
