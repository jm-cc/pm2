/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: bitmap.c,v $
Revision 1.5  2000/02/28 11:16:55  rnamyst
Changed #include <> into #include "".

Revision 1.4  2000/01/31 15:58:16  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "sys/archdep.h"
#include "sys/bitmap.h"
#include "marcel.h"

/* #define DEBUG */

#define SET_BIT_IN_WORD(word, bit_rel_index) \
        word |= 1 << bit_rel_index

#define SET_BIT(bit_abs_index, crt_bitmap) \
        SET_BIT_IN_WORD(crt_bitmap[(bit_abs_index) >> 5], (bit_abs_index) % 32)

#define RESET_BIT_IN_WORD(word, bit_rel_index) \
        word &= ~(1 << bit_rel_index)

#define RESET_BIT(bit_abs_index, crt_bitmap) \
        RESET_BIT_IN_WORD(crt_bitmap[(bit_abs_index) >> 5], (bit_abs_index) % 32)

#define GET_BIT_IN_WORD(word, bit_rel_index)\
        ((word & (int)(1 << (bit_rel_index))) >> (bit_rel_index))

#define GET_BIT(bit_abs_index, crt_bitmap) \
        GET_BIT_IN_WORD(crt_bitmap[(bit_abs_index) >> 5], (bit_abs_index) % 32)

#define WORD_SIZE 32

#define LOG_WORD_SIZE 5

#define ALL_BITS_SET_TO_1 0xffffffff

#define ALL_BITS_SET_TO_0 0


#define SHIFT_TO_LEFT(c, n) c <<= (n)

#define SHIFTED_TO_LEFT(c, n) ((c) << (n))

#define SHIFT_TO_RIGHT(c, n) (c) >>= (n)

#define SHIFTED_TO_RIGHT(c, n) ((c) >> (n))

#define SEQUENCE_OF_1(n) (((n)==32)?0xffffffff:((1 << (n)) - 1))

static int first_bit_set(unsigned k)
{
  /*
    Returns the rank of the least significant bit set to 1
    in the unsigned int 'k' (e.g. first_bit_set(1) returns 0
    and first_bit_set(4) returns 2). Returns -1 if all bits 
    are set to 0 (i.e. if k = 0).
  */
  int i, index = 0;
  unsigned int mask, width = WORD_SIZE/2;

  if (k == 0) 
    return -1;

  mask = SEQUENCE_OF_1(width);
  for (i = 0; i < LOG_WORD_SIZE; i++)
    {
      if ((k & mask) == 0)
	index += width;
      SHIFT_TO_RIGHT(width, 1);
      mask =  SHIFTED_TO_LEFT(SEQUENCE_OF_1(width), index);
    }

  return index;
}


static int first_bit_set_from_offset(unsigned int w, int offset)
{
  int i;

  for(i = offset; i < WORD_SIZE; i++)   
      if (GET_BIT_IN_WORD(w,i) == 1)
	return i;
  return -1;
}


int first_bit_to_1(unsigned int *crt_bitmap)
{
  /* 
     Returns the rank of the first bit set to 1 in the bitmap,
     This function has no side effects; the next call to
    "first_bit_to_1()" will return the same address.  
  */
  unsigned int i = 0, max = BITMAP_SIZE;

  /*
    Look for the first non-zero word.
  */
  while ((crt_bitmap[i] == 0) && (i < max))
     i++;

  /*
    Look for the first bit set to 1 in that word, if found.
  */
  if (crt_bitmap[i] != 0)
    { 
      return first_bit_set(crt_bitmap[i]) + (i * 32);
    }

  /*
    Return -1 if all words are zero.
  */
  return -1;
}

int first_series_of_1_from_offset(unsigned int *crt_bitmap, int bitmap_size, int offset, int *length)
{
  int start, s, w_rank = offset/WORD_SIZE;
  unsigned bitmap_size_in_words = bitmap_size/WORD_SIZE;

  if ((start = first_bit_set_from_offset(crt_bitmap[w_rank], offset%WORD_SIZE)) != -1)
      start += w_rank * WORD_SIZE;
  else
    {
      w_rank++;
      while(w_rank < bitmap_size_in_words)
	if(crt_bitmap[w_rank] == 0)
	  w_rank++;
	else
	  break;
      
      if (w_rank >= bitmap_size_in_words)
	return -1;
      else
	start = first_bit_set(crt_bitmap[w_rank]) + w_rank * WORD_SIZE;
    }

  /* count 1's: */
  s = start + 1;
  while (s < bitmap_size)
    if (GET_BIT(s, crt_bitmap))
      s++;
    else
      break;

  *length = s - start;
  return start;
}
      
 
int get_first_bit_to_1(bitmap_t crt_bitmap)
{
  /* 
     Returns the rank of the first slot that can be mmapped and marks
     the slot as "busy", such that the next call to
     "first_bit_to_1()" will return another address. Thus, this
     function has side effects.  
  */
 int r = first_bit_to_1(crt_bitmap);

 RESET_BIT(r, crt_bitmap);
 return r;
}


void set_bits_to_1(unsigned int start, unsigned int n,  bitmap_t crt_bitmap)
{
  /* 
     Sets to 1 'n' contiguous bits in the given bitmap, starting at
     position 'start'.  
  */
 unsigned int end = start + n, 
              first_word = start >> 5,
              last_word = (end - 1) >> 5;
 if (n <= 0)
   return;
 

 if (first_word == last_word)
     /* 
	There is a single word to modify in the bitmap.
     */
     crt_bitmap[first_word] |= SHIFTED_TO_LEFT(SEQUENCE_OF_1(n), start % 32);
 else
   {
     int i;

     /* set bits in the first word */
     crt_bitmap[first_word] |= ~(SEQUENCE_OF_1(start % 32));
     
     /* set bits in the intermediate words */
     for (i = first_word + 1; i < last_word ; i++)
       crt_bitmap[i] = ALL_BITS_SET_TO_1;
     
     /* set bits in the last word */
     crt_bitmap[last_word] |= (end % 32)?SEQUENCE_OF_1(end % 32):ALL_BITS_SET_TO_1;
   }
}


void reset_bits_to_0(unsigned int start, unsigned int n, bitmap_t crt_bitmap )
{
  /* 
     Resets to 0 'n' contiguous bits in the given bitmap, starting at
     position 'start'.  
  */
 unsigned int end = start + n, 
              first_word = start >> 5,
              last_word = (end - 1 ) >> 5;
 
 if (first_word == last_word)
   crt_bitmap[first_word] &= ~(((1 << n) - 1) << (start % 32));
 else
   {
     int i;

     /* reset bits in the first word */
     crt_bitmap[first_word] &= SEQUENCE_OF_1(start % 32);
     
     /* reset bits in the intermediate words */
     for (i = first_word + 1; i < last_word ; i++)
       crt_bitmap[i] = ALL_BITS_SET_TO_0;
     
     /* reset bits in the last word */
     crt_bitmap[last_word] &= ~(SEQUENCE_OF_1(end % 32));
   }
}


void display_bitmap(unsigned min, unsigned max, unsigned int *crt_bitmap)
{
  /*
    Displays the value of the bits ranging from min to max in the given bitmap.
  */
  unsigned int i, j, n, min_word = min >> 5, max_word = (max >> 5) + 1;

  for (i = min_word ; i < max_word ; i++)
    {
      n = i << 5;
      for (j = 0; j < 32 ; j++)
	fprintf(stderr, " index %d: %d\t", n + j, crt_bitmap[i] & (1 << j)?1:0);
    } 
    
}


static int count_msb_set_to_1(unsigned int word, unsigned int *crt_bitmap)
{
  /*
    Returns the number of contiguous most significant bits set to 1 in 'word'
    (e.g. returns 4 for the binary word 0001101010001001001001111101111).
  */
   unsigned int width, mask;
   int c = 0;

   width = WORD_SIZE/2; 
   mask = ~(SEQUENCE_OF_1(width)); 
   while (width > 0) 
     { 
	 if ((crt_bitmap[word] & mask) == mask) 
	   c += width; 
	 SHIFT_TO_RIGHT(width, 1);
	 mask = SHIFTED_TO_LEFT(SEQUENCE_OF_1(width), WORD_SIZE - width - c); 
     } 
   return c;
}


int first_bits_to_1(unsigned int n, unsigned int *crt_bitmap)
{
  /* 
     Returns the rank of the first bit in the first series of n contiguous bits
     set to 1 in the bitmap (starting from the lsb).
     This function has no side effects; the next call to
    "first_bits_set_to_1()" will return the same rank.  
  */
 unsigned int mask, remainder_mask;
 int remainder, k = 0;
 int i;

if (n < WORD_SIZE)
  {
    while(k < BITMAP_SIZE)
      {
	mask = SEQUENCE_OF_1(n);
	i = 0;
	while( (i < WORD_SIZE) && ((crt_bitmap[k] & mask) != mask))
	  {
	    SHIFT_TO_LEFT(mask, 1); 
	    i++;
	  }
	if (i <= WORD_SIZE - (int)n)
	  /*
	    n contiguous bits set 1 have been found.
	  */
	  return (k * WORD_SIZE) + i;
	else
	  if (i < WORD_SIZE)
	    {
	      /*
		Less than n contiguous bits set 1 have been found.
		Check if the remaining necessary bits are set to 1 
		in the next word.
	      */
	      remainder_mask = SEQUENCE_OF_1(n - WORD_SIZE + i);
	      if ((crt_bitmap[k + 1] & remainder_mask) == remainder_mask)
		return ((k * WORD_SIZE) + i);
	    }
	k++;
      }
  }
 else
   {
     int nb;
     unsigned int k1;
 
     while (k < BITMAP_SIZE)
       {
	 if (n >= WORD_SIZE * 2 - 1) /* At least a full word set to 0xffffffff is required */
	   /*
	     Look for the next word set to 0xffffffff
	   */
	   while (crt_bitmap[k] != ALL_BITS_SET_TO_1)
	     k++;
	 else
	   /*
	     Look for the next word whose msb is set to 1
	   */
	   {
	     mask = SHIFTED_TO_LEFT(1, WORD_SIZE - 1);
	     while ((crt_bitmap[k] & mask) != mask)
	       k++;
	     k++; /* !! For uniformity reasons !! */
	   }
	 /* 
	    In both cases above we are now interested in the
	    msb set to 1 in the word k - 1.
	 */
	 nb = (k > 0)?count_msb_set_to_1(k - 1, crt_bitmap):0;

	 /*
	   Count the remaining bits that are necessary and the index where to start checking.
	 */
	 if(n >= WORD_SIZE * 2 - 1)
	   {
	     remainder = (int)n - WORD_SIZE - nb;
	     k1 = k + 1;
	   }
	 else
	   {
	     remainder = (int)n - nb;
	     k1 = k;
	   }
	 /* 
	    As long as the remainder is longer than a word, check word by word, if
	    all bits are set to 1.
	 */
	 while (remainder >= WORD_SIZE) 
	   if (crt_bitmap[k1] != ALL_BITS_SET_TO_1)
	     break;
	   else
	     {
	       remainder -= WORD_SIZE;
	       k1++;
	     }
	 /*
	   Here the remainder (if it exists) is shorted than a word.
	   Check if all remaining bits are set to 1.
	 */
      	 if ((remainder < WORD_SIZE) && (remainder != 0))
	   {
	     remainder_mask = SEQUENCE_OF_1(remainder);
	      if ((crt_bitmap[k1] & remainder_mask) == remainder_mask)
		return (k * WORD_SIZE - nb);
	    }
	     /*
	       Not enough contiguous bits set to 1. Keep searching...
	       Note that when looking for a word whose bits are all set to 1, 
	       the search may go on with the word k1 + 1, since the word k1 is
	       guaranteed to have a zero, otherwise the test above would not 
	       have failed. Nevertheless, then looking for a word whose msb is set
	       to 1, the search must go on with the word k1.
	     */
	 
	 k+=(n >= WORD_SIZE * 2 - 1)?k1 + 1: k1;
       }
   }
 return -1;
}


int get_first_bits_to_1(unsigned int n, unsigned int *crt_bitmap)
{
  /* 
     Returns the rank of the first bit in the first series of n contiguous bits
     set to 1 in the bitmap (starting from the lsb) and resets them to 0.
     Thus, this function has side effects; the next call to
    "get_first_bits_set_to_1()" will return another value.  
  */
 int i = first_bits_to_1(n, crt_bitmap);
 if (i != -1)
   {
     reset_bits_to_0(i, n, crt_bitmap);
     return i;
   }
 else
   return -1;
}

void OR_bitmaps_1(bitmap_t dest, bitmap_t src1, bitmap_t src2)
{
  unsigned int *p, *p1, *p2;
  int j;

  p1 = (unsigned int *)src1; 
  p2 = (unsigned int *)src2; 
  p = (unsigned int *)dest; 

  for (j = 0 ; j < BITMAP_SIZE; j++, p++, p1++, p2++) 
   *p = *p1 | *p2; 
}


void OR_bitmaps_2(bitmap_t dest, bitmap_t src)
{
  unsigned int *p, *p1;
  int j;

  p1 = (unsigned int *)src; 
  p = (unsigned int *)dest; 

  for (j = 0 ; j < BITMAP_SIZE; j++, p++, p1++) 
    *p |= *p1; 
}


void set_cyclic_sequences(unsigned int start, unsigned int bits_to_1, unsigned int period, int nb_cycles, bitmap_t crt_bitmap)
{
  int j;
 
  if (bits_to_1 > period)
    RAISE(PROGRAM_ERROR); 

  /* Do not go beyond MAX_BITS */ 
  if (start + bits_to_1 > MAX_BITS)
    return;

  if (nb_cycles == -1) 
    /* 
       Fill in the whole bitmap.
    */
    nb_cycles = (MAX_BITS - start)/period;

  for (j = 0 ; j < nb_cycles; j++, start += period)
    set_bits_to_1(start, bits_to_1, crt_bitmap);
#ifdef DEBUG
  fprintf(stderr, "last local slot: %d\n", start + bits_to_1 - 1);
#endif

}


int bitmap_is_empty(unsigned int *crt_bitmap, int size)
{
  unsigned int i = 0, bitmap_size_in_words =  size/WORD_SIZE;
  
  while(i < bitmap_size_in_words)
    if(crt_bitmap[i] == 0)
      i++;
    else
      break;
  
  return (i < bitmap_size_in_words)?0:1;
}


