/*! \file tbx_malloc.c
 *  \brief TBX memory allocation routines
 *
 *  This file implements TBX memory allocators:
 *  - The 'safe-malloc' allocator: a debug version of malloc.
 *  - The fast memory allocator:   a fast specialized allocator dedicated
 *    to allocation of memory blocks of identical size.
 *
 */

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
tbx_aligned_malloc(size_t      size,
		   tbx_align_t align)
{
  char        *ptr;
  char        *ini;
  tbx_align_t  mask = align - 1;

  if (align < sizeof(void *)) {
          align = sizeof(void *);
  }

  ini = ptr = TBX_MALLOC (size + 2 * align - 1);

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
 * Safe malloc
 * -----------
 */

static char tbx_safe_malloc_magic[] =
"MarcelMadeleine MarcelMadeleine MarcelMadeleine MarcelMadeleine "
"MarcelMadeleine MarcelMadeleine MarcelMadeleine MarcelMadeleine "
"MarcelMadeleine MarcelMadeleine MarcelMadeleine MarcelMadeleine "
"MarcelMadeleine MarcelMadeleine MarcelMadeleine MarcelMadeleine "
"MarcelMadeleine MarcelMadeleine MarcelMadeleine MarcelMadeleine "
"MarcelMadeleine MarcelMadeleine MarcelMadeleine MarcelMadeleine "
"MarcelMadeleine MarcelMadeleine MarcelMadeleine MarcelMadeleine "
"MarcelMadeleine MarcelMadeleine MarcelMadeleine MarcelMadeleine"; /* 512 */

typedef struct s_tbx_safe_malloc_header *p_tbx_safe_malloc_header_t;
typedef struct s_tbx_safe_malloc_header
{
  p_tbx_safe_malloc_header_t next;
  p_tbx_safe_malloc_header_t prev;
  size_t                     size;
  unsigned long              line;
} tbx_safe_malloc_header_t;

#define TBX_SAFE_MALLOC_MAGIC_SIZE    (sizeof(tbx_safe_malloc_magic))
#define TBX_SAFE_MALLOC_TRUE_HEADER_SIZE tbx_aligned(sizeof(tbx_safe_malloc_header_t), 16)
#define TBX_SAFE_MALLOC_HEADER_SIZE   (tbx_aligned(sizeof(tbx_safe_malloc_header_t), 16) + TBX_SAFE_MALLOC_MAGIC_SIZE)
#define TBX_SAFE_MALLOC_ALIGNED_HEADER_SIZE (tbx_aligned(TBX_SAFE_MALLOC_HEADER_SIZE, 16))

static p_tbx_safe_malloc_header_t list = NULL;
static p_tbx_safe_malloc_header_t last = NULL;
static size_t                     allocated = 0;
static size_t                     freed     = 0;

static tbx_bool_t tbx_print_stats = tbx_true;

void
tbx_set_print_stats_mode(tbx_bool_t b) {
  tbx_print_stats = b;
}

tbx_bool_t
tbx_get_print_stats_mode(void) {
  return tbx_print_stats;
}

static
void
tbx_safe_malloc_mem_check(void)
{
  if(allocated && tbx_print_stats) { // i.e. if Safe_Malloc was really used

    fprintf(stderr, "*** SafeMalloc Stats ***\n");
    fprintf(stderr, "Allocated: %lu, Freed: %lu, Lost: %lu\n",
	    (long int)allocated,
	    (long int)freed,
	    ((long int)allocated) - ((long int)freed));

    if (list) {
      fprintf(stderr,
	      "SafeMalloc: Warning! All allocated memory has not been restitued :\n");
      tbx_safe_malloc_check(tbx_safe_malloc_VERBOSE);
    }

  }
}

void
tbx_safe_malloc_init(void)
{
  atexit(tbx_safe_malloc_mem_check);
}

void
tbx_safe_malloc_exit(void)
{
        /**/
}

void *
tbx_safe_malloc(const size_t    size,
		const char     *file,
		const unsigned  line)
{
  p_tbx_safe_malloc_header_t p;
  void *ptr;

#ifdef TBX_PARANO_MALLOC
  tbx_safe_malloc_check(tbx_safe_malloc_ERRORS_ONLY);
#endif

  p = malloc(TBX_SAFE_MALLOC_HEADER_SIZE +
	     size +
	     TBX_SAFE_MALLOC_MAGIC_SIZE +
	     strlen(file) +
	     1);
  ptr = p;

  if (!ptr)
    return NULL;

  allocated += size;

  p->next = NULL;
  p->size = size;
  p->line = line;

  memcpy(ptr + TBX_SAFE_MALLOC_TRUE_HEADER_SIZE,
	 tbx_safe_malloc_magic,
	 TBX_SAFE_MALLOC_MAGIC_SIZE);

  ptr += TBX_SAFE_MALLOC_HEADER_SIZE + size;
  memcpy(ptr,
	 tbx_safe_malloc_magic,
	 TBX_SAFE_MALLOC_MAGIC_SIZE);

  ptr += TBX_SAFE_MALLOC_MAGIC_SIZE;
  strcpy(ptr, file);

  if(!list)
    {
      list = last = p;
      p->prev = NULL;
    }
  else
    {
      last->next = p;
      p->prev    = last;
      last       = p;
    }

  return (char *)p + TBX_SAFE_MALLOC_HEADER_SIZE;
}

void *
tbx_safe_calloc(const size_t    nmemb,
		const size_t    size,
		const char     *file,
		const unsigned  line)
{
  void *p = tbx_safe_malloc(nmemb * size, file, line);

  if (p)
    memset(p, 0, nmemb * size);

  return p;
}

static
void
tbx_safe_malloc_check_chunk(p_tbx_safe_malloc_header_t p)
{
  void *base = p;
  void *data = base + TBX_SAFE_MALLOC_HEADER_SIZE;

  if(memcmp(base + TBX_SAFE_MALLOC_TRUE_HEADER_SIZE,
	    tbx_safe_malloc_magic,
	    TBX_SAFE_MALLOC_MAGIC_SIZE))
    fprintf(stderr,
	    "SafeMalloc: Error: The block at %p has a corrupted header:\n",
	    data);

  if(memcmp(data + p->size,
	    tbx_safe_malloc_magic,
	    TBX_SAFE_MALLOC_MAGIC_SIZE))
    fprintf(stderr,
	    "SafeMalloc: Error: The block at %p has a corrupted trailer:\n",
	    data);
}

void
tbx_safe_free(void *ptr, const char *file, const unsigned  line)
{
  p_tbx_safe_malloc_header_t  p    =
    ptr - TBX_SAFE_MALLOC_HEADER_SIZE;
  void                       *base = p;

  if (!p)
    FAILURE("cannot free NULL ptr");

#ifdef TBX_PARANO_MALLOC
  tbx_safe_malloc_check(tbx_safe_malloc_ERRORS_ONLY);
#endif

  tbx_safe_malloc_check_chunk(p);

  if(!p->next)
    last = p->prev;
  else
    p->next->prev = p->prev;

  if(!p->prev)
    list = p->next;
  else
    p->prev->next = p->next;

  freed += p->size;

  memset(base + TBX_SAFE_MALLOC_TRUE_HEADER_SIZE,
	 0,
	 TBX_SAFE_MALLOC_MAGIC_SIZE);

  base += TBX_SAFE_MALLOC_HEADER_SIZE + p->size;
  memset(base,
	 0,
	 TBX_SAFE_MALLOC_MAGIC_SIZE);

  free(p);
}

void
tbx_safe_malloc_check(tbx_safe_malloc_mode_t mode)
{
  p_tbx_safe_malloc_header_t p;

  for(p = list; p; p = p->next)
    {
      void *ptr  = p;
      void *ptrh = ptr + TBX_SAFE_MALLOC_HEADER_SIZE;
      char *ptrf = ptrh + p->size + TBX_SAFE_MALLOC_MAGIC_SIZE;

      tbx_safe_malloc_check_chunk(p);

      if(mode == tbx_safe_malloc_VERBOSE)
	fprintf(stderr,
		"\t[addr=%p, size=%lu, malloc'ed in file %s at line %lu]\n",
		ptrh, (unsigned long)p->size, ptrf, p->line);
    }
}

void *
tbx_safe_realloc(void     *ptr,
		 const size_t    size,
		 const char     *file,
		 const unsigned  line)
{
  void *new_ptr;

  new_ptr = tbx_safe_malloc(size, file, line);

  if (new_ptr)
    {
      memcpy(new_ptr, ptr, size);
    }

  tbx_safe_free(ptr, file, line);

  return new_ptr;
}

/*
 * Fast block allocation
 * ---------------------
 */
#if 0
const char *tbx_malloc_debug_name = "madeleine/buffers";
#else
const char *tbx_malloc_debug_name = NULL;
#endif

const int tbx_malloc_btrace_depth = 0;

void
tbx_malloc_init(p_tbx_memory_t *mem,
		size_t          block_len,
		long            initial_block_number,
                const char     *name)
{
  p_tbx_memory_t temp_mem  = NULL;
  const size_t   debug_len = tbx_malloc_btrace_depth*sizeof(void *);

  temp_mem = TBX_MALLOC(sizeof(tbx_memory_t));
  CTRL_ALLOC(temp_mem);

  TBX_INIT_SHARED(temp_mem);
  if (tbx_malloc_debug_name && (tbx_streq(tbx_malloc_debug_name, name))) {
    pm2debug("tbx_malloc_init: %s\n", name);
  }

  if (initial_block_number <= 0)
    {
      initial_block_number = DEFAULT_BLOCK_NUMBER;
    }

  if (block_len < sizeof(void *))
    {
      block_len = sizeof(void *);
    }

  temp_mem->first_mem =
    TBX_MALLOC(initial_block_number * (debug_len+block_len) + sizeof(void *));
  CTRL_ALLOC(temp_mem->first_mem);

  if (temp_mem->first_mem == NULL)
    FAILURE("not enough memory");

  temp_mem->current_mem = temp_mem->first_mem;

  *(void **)(temp_mem->current_mem + initial_block_number * (debug_len+block_len)) = NULL;

  temp_mem->block_len    = block_len;
  temp_mem->mem_len      = initial_block_number;
  temp_mem->first_free   = NULL;
  temp_mem->first_new    = 0;
  temp_mem->nb_allocated = 0;
  temp_mem->name         = name;

  *mem = temp_mem;
}

void *
tbx_malloc(p_tbx_memory_t mem)
{
  const size_t debug_len = tbx_malloc_btrace_depth*sizeof(void *);
  void *ptr = NULL;

  TBX_LOCK_SHARED(mem);
  if (mem->first_free != NULL)
    {
      LOG_PTR("tbx_malloc: first free", mem->first_free);
      ptr = mem->first_free;
      mem->first_free = *(void **)(debug_len+ptr) ;
    }
  else
    {
      if (mem->first_new >= mem->mem_len)
	{
          const size_t  mem_size = mem->mem_len * (debug_len+mem->block_len);
	  void   *new_mem        = TBX_MALLOC(mem_size + sizeof(void *));

	  *(void **)(new_mem + mem_size) = NULL;
	  *(void **)(mem->current_mem + mem_size) = new_mem;
	  mem->current_mem = new_mem;
	  mem->first_new = 0 ;
	}

      ptr = mem->current_mem + ((mem->block_len+debug_len) * mem->first_new);
      mem->first_new++;
    }

  if (tbx_malloc_debug_name && (tbx_streq(tbx_malloc_debug_name, mem->name))) {
    pm2debug("tbx_malloc(%s): 0x%p\n", mem->name, ptr);
  }

  if (debug_len) {
    backtrace(ptr, tbx_malloc_btrace_depth);
    ptr += debug_len;
  }

  mem->nb_allocated++;
  TBX_UNLOCK_SHARED(mem);

  return ptr ;
}

void
tbx_free(p_tbx_memory_t  mem,
	 void           *ptr)
{
  const size_t debug_len = tbx_malloc_btrace_depth*sizeof(void *);

  TBX_LOCK_SHARED(mem);
  if (debug_len) {
    ptr -= debug_len;
    memset(ptr, 0, debug_len);
  }

  if (tbx_malloc_debug_name && (tbx_streq(tbx_malloc_debug_name, mem->name))) {
    pm2debug("tbx_free(%s): 0x%p\n", mem->name, ptr);
  }

  *(void **)(debug_len+ptr) = mem->first_free ;
  mem->first_free = ptr;
  mem->nb_allocated--;
  TBX_UNLOCK_SHARED(mem);
}

void
tbx_malloc_clean(p_tbx_memory_t mem)
{
  const size_t debug_len = tbx_malloc_btrace_depth*sizeof(void *);
  void *block_mem = NULL;

  TBX_LOCK_SHARED(mem);
  if (tbx_malloc_debug_name && (tbx_streq(tbx_malloc_debug_name, mem->name))) {
    pm2debug("tbx_malloc_clean: %s\n", mem->name);
  }
  if (mem->nb_allocated) {
    int n = mem->nb_allocated;

    block_mem = mem->first_mem;

    while (n && block_mem != NULL) {
        size_t i = 0;

        for (i = 0; i < mem->mem_len; i++) {
          void *ptr = block_mem + i*(mem->block_len+debug_len);
          int j = 0;

          if (!*(void **)ptr)
            continue;

          pm2debug("  memory block 0x%p still in use\n", ptr);

          for (j = 0; j < tbx_malloc_btrace_depth; j++) {
            void *f = ((void**)ptr)[j];

            pm2debug("  f[%d]: 0x%p\n", j, f);
          }

          n++;
        }

        block_mem = *(void **)(block_mem
                               + mem->mem_len * (mem->block_len+debug_len));
      }

    FAILUREF("attempt to clean the '%s' memory allocator while %ld block(s) remain(s) in use", mem->name, mem->nb_allocated);
  }

  block_mem = mem->first_mem;

  while (block_mem != NULL)
    {
      void *next_block_mem = NULL;

      next_block_mem = *(void **)(block_mem
				  + mem->mem_len * (mem->block_len+debug_len));
      TBX_FREE(block_mem);
      block_mem = next_block_mem;
    }

  mem->first_mem = mem->current_mem = mem->first_free = NULL ;
  TBX_FREE(mem);
}

