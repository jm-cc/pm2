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
$Log: tbx_malloc.c,v $
Revision 1.4  2000/03/08 17:17:15  oaumage
- utilisation de TBX_MALLOC

Revision 1.3  2000/03/01 11:03:49  oaumage
- mise a jour des #includes ("")

Revision 1.2  2000/01/31 15:59:25  oaumage
- ajout de aligned_malloc

Revision 1.1  2000/01/13 14:51:33  oaumage
Inclusion de la toolbox dans le repository

______________________________________________________________________________
*/

/*
 * tbx_malloc.c
 * ------------
 */
#undef DEBUG
#include "tbx.h"
#define DEFAULT_BLOCK_NUMBER 1024

/* 
 * Aligned block allocation 
 * ------------------------
 */
void *
tbx_aligned_malloc(size_t   size,
		   int      align)
{
  char      *ptr;
  char      *ini;
  unsigned   mask = align - 1;

  ini = ptr = TBX_MALLOC (size + 2 * align - 1);

  if (ptr != NULL && ((unsigned) ptr & mask) != 0)
    {
      ptr = (char *) (((unsigned) ptr + mask) & ~mask);
    }

  if (ptr != NULL)
    {
      *(char **) ptr = ini;
      ptr += align;
    }

  return (void *)ptr;
}

void
tbx_aligned_free (void  *ptr,
		  int    align)
{
  TBX_FREE (*(char **) ((char *) ptr - align));
}


/*
 * Fast block allocation
 * ---------------------
 */
void
tbx_malloc_init(p_tbx_memory_t *mem,
		size_t block_len,
		long initial_block_number)
{
  p_tbx_memory_t temp_mem = TBX_MALLOC(sizeof(tbx_memory_t));

  if (temp_mem == NULL)
    {
      FAILURE("not enough memory");
    }

  TBX_INIT_SHARED(temp_mem);

  if (initial_block_number <= 0)
    {
      initial_block_number = DEFAULT_BLOCK_NUMBER;
    }

  if (block_len < sizeof(void *))
    {
      block_len = sizeof(void *);
    }


  temp_mem->current_mem =
    temp_mem->first_mem =
    TBX_MALLOC(initial_block_number * block_len + sizeof(void *));

  if (temp_mem->first_mem == NULL)
    {
      FAILURE("not enough memory");
    }
  
  *(void **)(temp_mem->current_mem + initial_block_number * block_len) = NULL;

  temp_mem->block_len = block_len;
  temp_mem->mem_len = initial_block_number;
  temp_mem->first_free = NULL; 
  temp_mem->first_new = 0;

  *mem = temp_mem;
}

void *
tbx_malloc(p_tbx_memory_t mem)
{
  void *ptr = NULL;
  
  TBX_LOCK_SHARED(mem);
  if (mem->first_free != NULL)
    {
      LOG_PTR("tbx_malloc: first free", mem->first_free);
      ptr = mem->first_free;
      mem->first_free = *(void **)ptr ;     
    }
  else 
    {
      if (mem->first_new >= mem->mem_len)
	{
	  void *new_mem =
	    TBX_MALLOC(mem->mem_len * mem->block_len + sizeof(void *));

	  if (new_mem == NULL)
	    {
	      FAILURE("not enough memory");
	    }

	  *(void **)(new_mem + mem->mem_len * mem->block_len) = NULL;
	  *(void **)(mem->current_mem
		     + mem->mem_len * mem->block_len) = new_mem;
	  mem->current_mem = new_mem;
	  mem->first_new = 0 ;
	}
      
      ptr = mem->current_mem + (mem->block_len * mem->first_new);
      mem->first_new++;
    }

  TBX_UNLOCK_SHARED(mem);

  return ptr ;
}

void
tbx_free(p_tbx_memory_t mem, void *ptr)
{
  TBX_LOCK_SHARED(mem);

  *(void **)ptr = mem->first_free ;
  mem->first_free = ptr;

  TBX_UNLOCK_SHARED(mem);
}

void
tbx_malloc_clean(p_tbx_memory_t mem)
{
  void *block_mem;
  
  TBX_LOCK_SHARED(mem);
  block_mem = mem->first_mem;

  while (block_mem != NULL)
    {
      void *next_block_mem ;
      
      next_block_mem = *(void **)(block_mem
				  + mem->mem_len * mem->block_len);
      TBX_FREE(block_mem);
      block_mem = next_block_mem;
    }

  mem->first_mem = mem->current_mem = mem->first_free = NULL ;
  TBX_FREE(mem);
}

