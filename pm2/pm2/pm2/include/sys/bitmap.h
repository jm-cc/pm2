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
*/

#ifndef BITMAP_IS_DEF
#define BITMAP_IS_DEF

/*#define DEBUG*/

#include <sys/types.h>

#define MAX_BITS 2000*32

#define BITMAP_SIZE (MAX_BITS >> 5)

typedef unsigned int bitmap_t[BITMAP_SIZE]; /* bit array: 1 bit per slot */

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

int get_first_bits_to_1(unsigned int n, unsigned int *crt_bitmap);
  /* 
     Returns the rank of the first bit in the first series of n contiguous bits
     set to 1 in the bitmap (starting from the lsb) and resets them to 0.
     Thus, this function has side effects; the next call to
    "get_first_bits_to_1()" will return another value.  
  */

void set_bits_to_1(unsigned int start, unsigned int n,  bitmap_t crt_bitmap);
  /* 
     Sets to 1 'n' contiguous bits in the given bitmap, starting at
     position 'start'.  
  */

void reset_bits_to_0(unsigned int start, unsigned int n, bitmap_t crt_bitmap );
  /* 
     Sets to 0 'n' contiguous bits in the given bitmap, starting at
     position 'start'.  
  */

void display_bitmap(unsigned min, unsigned max, unsigned int *crt_bitmap );
  /*
    Displays the value of the bits ranging from min to max in the given bitmap.
  */

void OR_bitmaps_1(bitmap_t dest, bitmap_t src1, bitmap_t src2);

void OR_bitmaps_2(bitmap_t dest, bitmap_t src);

void set_cyclic_sequences(unsigned int start, unsigned int bits_to_1, unsigned int period, int nb_cycles, bitmap_t crt_bitmap);

#endif
