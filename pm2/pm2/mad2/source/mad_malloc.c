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

/*
 * mad_malloc.c
 * ------------
 */
#undef DEBUG
#include <madeleine.h>
#define DEFAULT_BLOCK_NUMBER 1024

void
mad_malloc_init(p_mad_memory_t *mem,
		size_t block_len,
		long initial_block_number)
{
  p_mad_memory_t temp_mem = malloc(sizeof(mad_memory_t));

  if (temp_mem == NULL)
    {
      FAILURE("not enough memory");
    }

  PM2_INIT_SHARED(temp_mem);

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
    malloc(initial_block_number * block_len + sizeof(void *));

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
mad_malloc(p_mad_memory_t mem)
{
  void *ptr = NULL;
  
  PM2_LOCK_SHARED(mem);
  if (mem->first_free != NULL)
    {
      LOG_PTR("mad_malloc: first free", mem->first_free);
      ptr = mem->first_free;
      mem->first_free = *(void **)ptr ;     
    }
  else 
    {
      if (mem->first_new >= mem->mem_len)
	{
	  void *new_mem =
	    malloc(mem->mem_len * mem->block_len + sizeof(void *));

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

  PM2_UNLOCK_SHARED(mem);

  return ptr ;
}

void
mad_free(p_mad_memory_t mem, void *ptr)
{
  PM2_LOCK_SHARED(mem);

  *(void **)ptr = mem->first_free ;
  mem->first_free = ptr;

  PM2_UNLOCK_SHARED(mem);
}

void
mad_malloc_clean(p_mad_memory_t mem)
{
  void *block_mem;
  
  PM2_LOCK_SHARED(mem);
  block_mem = mem->first_mem;

  while (block_mem != NULL)
    {
      void *next_block_mem ;
      
      next_block_mem = *(void **)(block_mem
				  + mem->mem_len * mem->block_len);
      free(block_mem);
      block_mem = next_block_mem;
    }

  mem->first_mem = mem->current_mem = mem->first_free = NULL ;
  free(mem);
}
