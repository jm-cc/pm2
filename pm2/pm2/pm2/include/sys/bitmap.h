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
$Log: bitmap.h,v $
Revision 1.5  2000/07/14 16:17:08  gantoniu
Merged with branch dsm3

Revision 1.4.10.1  2000/06/13 16:44:06  gantoniu
New dsm branch.

Revision 1.4.8.3  2000/06/09 17:55:51  gantoniu
Added support for alignment of dynamic allocated data. Thread stacks are now
guaranteed to be aligned with respect to THREAD_SLOT_SIZE whatever the
isoaddress page distribution may be.

Revision 1.4.8.2  2000/06/07 18:06:10  gantoniu
Worked on aligned stack alignment.

Revision 1.4.8.1  2000/06/07 09:19:37  gantoniu
Merging new dsm with current PM2 : first try.

Revision 1.4.4.2  2000/04/30 18:54:04  gantoniu
Extended negociation procedure to allow multi-slot data allocation.
Negociation ok (old RPCs).

Revision 1.4.4.1  2000/04/10 18:50:43  gantoniu
Ajout couche isoaddr + integration d'une table generale des pages
(isomalloc + dsm), avec les fonctions de gestion associees.

Tests ok avec les examples isomalloc.

Revision 1.4  2000/01/31 15:50:14  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

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


