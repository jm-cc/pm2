/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

/*
 * tbx_malloc_inline.h
 * -------------------
 */

#ifndef TBX_MALLOC_INLINE_H
#define TBX_MALLOC_INLINE_H

/** \addtogroup malloc_interface
 *
 * @{
 */
#define DEFAULT_BLOCK_NUMBER 1024

#include "tbx_compiler.h"
#include "tbx_macros.h"

/*
 * Aligned block allocation
 * ------------------------
 */
TBX_FMALLOC
static
__inline__
void *
tbx_aligned_malloc(size_t      size,
		   tbx_align_t align)
{
  char        *ptr;
  char        *ini;
  tbx_align_t  mask = align - 1;

  if (align < sizeof(void *)) {
          align = sizeof(void *);
  }

  ini = ptr = (char*)TBX_MALLOC (size + 2 * align - 1);

  if (ptr != NULL && ((tbx_align_t) ptr & mask) != 0)
    {
      ptr = (char *) (((tbx_align_t) ptr + mask) & ~mask);
    }

  if (ptr != NULL)
    {
      *(char **) ptr = ini;
      ptr += align;
    }

  return (void *)ptr;
}

static
__inline__
void
tbx_aligned_free (void *ptr,
		  tbx_align_t   align)
{
  if (align < sizeof(void *)) {
          align = sizeof(void *);
  }

  TBX_FREE (*(char **) ((char *) ptr - align));
}

/*
 * Fast block allocation
 * ---------------------
 */
#if 0
#  define TBX_MALLOC_DEBUG_NAME ("iovecs")
#else
#  undef  TBX_MALLOC_DEBUG_NAME
#endif

#ifdef HAVE_BACKTRACE
#  define TBX_MALLOC_BTRACE_DEPTH  0
#  define TBX_MALLOC_DEBUG_LEN  (TBX_MALLOC_BTRACE_DEPTH*sizeof(void *))
#else
#  define TBX_MALLOC_BTRACE_DEPTH  0
#  define TBX_MALLOC_DEBUG_LEN     0
#endif /* HAVE_BACKTRACE */

static
__inline__
void
tbx_malloc_init(p_tbx_memory_t *mem,
		size_t          block_len,
		long            initial_block_number,
                const char     *name)
{
  p_tbx_memory_t temp_mem  = NULL;

  temp_mem = (p_tbx_memory_t)TBX_MALLOC(sizeof(tbx_memory_t));
  CTRL_ALLOC(temp_mem);

  TBX_INIT_SHARED(temp_mem);
#ifdef TBX_MALLOC_DEBUG_NAME
  if (tbx_streq(TBX_MALLOC_DEBUG_NAME, name)) {
    pm2debug("tbx_malloc_init: %s\n", name);
  }
#endif /* TBX_MALLOC_DEBUG_NAME */

  if (initial_block_number <= 0)
    {
      initial_block_number = DEFAULT_BLOCK_NUMBER;
    }

  if (block_len < sizeof(void *))
    {
      block_len = sizeof(void *);
    }

  temp_mem->first_mem =
    TBX_MALLOC(initial_block_number * (TBX_MALLOC_DEBUG_LEN+block_len) + sizeof(void *));
  CTRL_ALLOC(temp_mem->first_mem);
  memset(temp_mem->first_mem, 0, initial_block_number * (TBX_MALLOC_DEBUG_LEN+block_len) + sizeof(void *));

  if (temp_mem->first_mem == NULL)
    TBX_FAILURE("not enough memory");

  temp_mem->current_mem = temp_mem->first_mem;

  *(void **)((char*)temp_mem->current_mem + initial_block_number * (TBX_MALLOC_DEBUG_LEN+block_len)) = NULL;

  temp_mem->block_len    = block_len;
  temp_mem->mem_len      = initial_block_number;
  temp_mem->first_free   = NULL;
  temp_mem->first_new    = 0;
  temp_mem->nb_allocated = 0;
  temp_mem->name         = name;

  *mem = temp_mem;
}

TBX_FMALLOC
static
__inline__
void *
tbx_malloc(p_tbx_memory_t mem)
{
  void *ptr = NULL;

  TBX_LOCK_SHARED(mem);
  if (mem->first_free != NULL)
    {
      LOG_PTR("tbx_malloc: first free", mem->first_free);
      ptr = mem->first_free;
      mem->first_free = *(void **)(TBX_MALLOC_DEBUG_LEN+(char*)ptr) ;
    }
  else
    {
      if (mem->first_new >= mem->mem_len)
	{
          const size_t  mem_size = mem->mem_len * (TBX_MALLOC_DEBUG_LEN+mem->block_len);
	  void   *new_mem        = TBX_MALLOC(mem_size + sizeof(void *));
          memset(new_mem, 0, mem_size + sizeof(void *));

	  *(void **)((char*)new_mem + mem_size) = NULL;
	  *(void **)((char*)mem->current_mem + mem_size) = new_mem;
	  mem->current_mem = new_mem;
	  mem->first_new = 0 ;
	}

      ptr = (char*)mem->current_mem + ((mem->block_len+TBX_MALLOC_DEBUG_LEN) * mem->first_new);
      mem->first_new++;
    }
#ifdef TBX_MALLOC_DEBUG_NAME
  if (tbx_streq(TBX_MALLOC_DEBUG_NAME, mem->name)) {
    pm2debug("tbx_malloc(%s): 0x%p\n", mem->name, ptr);
  }
#endif /* TBX_MALLOC_DEBUG_NAME */

#if TBX_MALLOC_BTRACE_DEPTH
  backtrace(ptr, TBX_MALLOC_BTRACE_DEPTH);
  ptr += TBX_MALLOC_DEBUG_LEN;
#endif /* TBX_MALLOC_BTRACE_DEPTH */

  mem->nb_allocated++;
  TBX_UNLOCK_SHARED(mem);

  return ptr ;
}

static
__inline__
void
tbx_free(p_tbx_memory_t  mem,
	 void           *ptr)
{
  TBX_LOCK_SHARED(mem);
#if TBX_MALLOC_BTRACE_DEPTH
  ptr -= TBX_MALLOC_DEBUG_LEN;
  memset(ptr, 0, TBX_MALLOC_DEBUG_LEN);
#endif /* TBX_MALLOC_BTRACE_DEPTH */

#ifdef TBX_MALLOC_DEBUG_NAME
  if (tbx_streq(TBX_MALLOC_DEBUG_NAME, mem->name)) {
    pm2debug("tbx_free(%s): 0x%p\n", mem->name, ptr);
  }
#endif /* TBX_MALLOC_DEBUG_NAME */
  *(void **)(TBX_MALLOC_DEBUG_LEN+(char*)ptr) = mem->first_free ;
  mem->first_free = ptr;
  mem->nb_allocated--;
  TBX_UNLOCK_SHARED(mem);
}

static
__inline__
void
tbx_malloc_clean(p_tbx_memory_t mem)
{
  void *block_mem = NULL;

  TBX_LOCK_SHARED(mem);
#ifdef TBX_MALLOC_DEBUG_NAME
  if (tbx_streq(TBX_MALLOC_DEBUG_NAME, mem->name)) {
    pm2debug("tbx_malloc_clean: %s\n", mem->name);
  }
#endif /* TBX_MALLOC_DEBUG_NAME */
  if (mem->nb_allocated) {
    int n = mem->nb_allocated;

    block_mem = mem->first_mem;

    while (n && block_mem != NULL) {
        long i = 0;

        for (i = 0; i < mem->mem_len; i++) {
          void **ptr = (void **)((char*)block_mem + i*(mem->block_len+TBX_MALLOC_DEBUG_LEN));

          if (!*ptr)
            continue;

          pm2debug("tbx_malloc_clean: %s - memory block 0x%p still in use\n", mem->name, ptr);

#if TBX_MALLOC_BTRACE_DEPTH
          {
            int j = 0;
            char ** str_array;
            str_array = backtrace_symbols(ptr,  TBX_MALLOC_BTRACE_DEPTH);
            for (j = 0; j < TBX_MALLOC_BTRACE_DEPTH; j++) {
              pm2debug("  f[%d]: %s\n", j, str_array[j]);
            }
          }
#endif /* TBX_MALLOC_BTRACE_DEPTH */

          n++;
        }

        block_mem = *(void **)((char*)block_mem
                               + mem->mem_len * (mem->block_len+TBX_MALLOC_DEBUG_LEN));
      }

    WARN("attempt to clean the '%s' memory allocator while %ld block(s) remain(s) in use", mem->name, mem->nb_allocated);
  }

  block_mem = mem->first_mem;

  while (block_mem != NULL)
    {
      void *next_block_mem = NULL;

      next_block_mem = *(void **)((char*)block_mem
				  + mem->mem_len * (mem->block_len+TBX_MALLOC_DEBUG_LEN));
      TBX_FREE(block_mem);
      block_mem = next_block_mem;
    }

  mem->first_mem = mem->current_mem = mem->first_free = NULL ;
  TBX_FREE(mem);
}

/* @} */

#endif /* TBX_MALLOC_INLINE_H */

