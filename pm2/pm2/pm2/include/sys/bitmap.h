
#ifndef BITMAP_IS_DEF
#define BITMAP_IS_DEF

/*#define DEBUG*/

#include <sys/types.h>

#define BITMAP_SIZE 1024*64*16

#define BITMAP_SIZE_IN_WORDS (BITMAP_SIZE >> 5)

//typedef unsigned int bitmap_t[BITMAP_SIZE_IN_WORDS]; /* bit array: 1 bit per slot */

int first_bit_to_1(unsigned int *crt_bitmap);
  /* 
     Returns the rank of the first bit set to 1 in the bitmap,
     This function has no side effects; the next call to
    "first_bit_to_1_available()" will return the same address.  
  */

int get_first_bit_to_1(unsigned int *crt_bitmap);
  /* 
     Returns the rank of the first bit set to 1 in the bitmap and 
     resets it to 0, such that the next call to
     "first_bit_to_1()" will return another rank. Thus, this
     function has side effects.  
  */

int first_bits_to_1(unsigned int n, unsigned int *crt_bitmap);
  /* 
     Returns the rank of the first bit in the first series of n contiguous bits
     set to 1 in the bitmap (starting from the lsb).
     This function has no side effects; the next call to
    "first_bits_to_1()" will return the same rank.  
  */

int first_bits_to_1_aligned(unsigned int n, unsigned int *crt_bitmap, unsigned int align);
  /* 
     Returns the rank of the first bit in the first series of n
     contiguous bits set to 1 in the bitmap (starting from the lsb), where
     the first bit is aligned with respect to 'align'.  This last parameter
     needs to be a power of 2.  This function has no side effects; the next
     call to "first_bits_set_to_1()" will return the same rank.  
  */

int get_first_bits_to_1(unsigned int n, unsigned int *crt_bitmap);
  /* 
     Returns the rank of the first bit in the first series of n contiguous bits
     set to 1 in the bitmap (starting from the lsb) and resets them to 0.
     Thus, this function has side effects; the next call to
    "get_first_bits_to_1()" will return another value.  
  */

int get_first_bits_to_1_aligned(unsigned int n, unsigned int *crt_bitmap, unsigned int align);
  /* Returns the rank of the first bit in the first series of n
     contiguous bits set to 1 in the bitmap (starting from the lsb),
     where the first bit is aligned with respect to 'align' and resets
     them to 0.  Thus, this function has side effects; the next call
     to "get_first_bits_set_to_1()" will return another value.  */


void set_bits_to_1(unsigned int start, unsigned int n,  unsigned int *crt_bitmap);
  /* 
     Sets to 1 'n' contiguous bits in the given bitmap, starting at
     position 'start'.  
  */

void reset_bits_to_0(unsigned int start, unsigned int n, unsigned int *crt_bitmap );
  /* 
     Sets to 0 'n' contiguous bits in the given bitmap, starting at
     position 'start'.  
  */

void display_bitmap(unsigned min, unsigned max, unsigned int *crt_bitmap );
  /*
    Displays the value of the bits ranging from min to max in the given bitmap.
  */
int first_series_of_1_from_offset(unsigned int *crt_bitmap, int bitmap_size, int offset, int *length);
  /*
    Returns the starting position and the length of the first series of 1's
    in the bitmap starting at position offset.
  */

void OR_bitmaps_1(unsigned int *dest, unsigned int *src1, unsigned int *src2);

void OR_bitmaps_2(unsigned int *dest, unsigned int *src);

void set_cyclic_sequences(unsigned int start, unsigned int bits_to_1, unsigned int period, int nb_cycles, unsigned int *crt_bitmap);

/* For the following functions/macros, size in bits */

int bitmap_is_empty(unsigned int *crt_bitmap, int size);

#define clear_bitmap(bitmap, size) memset(bitmap, 0, size/8)

void bitmap_set_size(int size);

int bitmap_get_size();

#endif


