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
$Log: tbx_interface.h,v $
Revision 1.2  2000/01/31 15:59:09  oaumage
- detection de l'absence de GCC
- ajout de aligned_malloc

Revision 1.1  2000/01/13 14:51:29  oaumage
Inclusion de la toolbox dans le repository

______________________________________________________________________________
*/

/*
 * tbx_interface.h
 * --------------------
 */

#ifndef __TBX_INTERFACE_H
#define __TBX_INTERFACE_H

/* 
 * Common
 * ------
 */
void
tbx_init(void);

/*
 * Timing
 * ------
 */
void 
tbx_timing_init(void);

double 
tbx_tick2usec(long long t);

/*
 * Aligned malloc
 * --------------
 */
void *
tbx_aligned_malloc(size_t size,
		   int    align);

void
tbx_aligned_free (void *ptr,
		  int   align);


/*
 * Fast malloc
 * -----------
 */
void
tbx_malloc_init(p_tbx_memory_t  *mem,
		size_t           block_len,
		long             initial_block_number);

void *
tbx_malloc(p_tbx_memory_t mem);

void
tbx_free(p_tbx_memory_t   mem, 
         void            *ptr);

void
tbx_malloc_clean(p_tbx_memory_t memory);



/*
 * List management
 * ---------------
 */
void
tbx_list_manager_init(void);

void
tbx_list_init(p_tbx_list_t list);

void 
tbx_foreach_destroy_list(p_tbx_list_t              list,
			 p_tbx_list_foreach_func_t func);
void 
tbx_list_manager_exit(void);

void
tbx_destroy_list(p_tbx_list_t list);

void 
tbx_append_list(p_tbx_list_t   list,
		void*          object);
void *
tbx_get_list_object(p_tbx_list_t list);

void 
tbx_mark_list(p_tbx_list_t list);

void 
tbx_duplicate_list(p_tbx_list_t source,
		   p_tbx_list_t destination);

void 
tbx_extract_sub_list(p_tbx_list_t source,
		     p_tbx_list_t destination);

tbx_bool_t 
tbx_empty_list(p_tbx_list_t list);

void 
tbx_list_reference_init(p_tbx_list_reference_t ref,
			p_tbx_list_t           list);

void *
tbx_get_list_reference_object(p_tbx_list_reference_t ref);

tbx_bool_t 
tbx_forward_list_reference(p_tbx_list_reference_t ref);

void 
tbx_reset_list_reference(p_tbx_list_reference_t ref);

tbx_bool_t 
tbx_reference_after_end_of_list(p_tbx_list_reference_t ref);

#endif /* __TBX_INTERFACE_H */
