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
#define MARCEL_INTERNAL_INCLUDE
#include "tbx.h"

/*
 * Safe malloc
 * -----------
 */
#ifdef TBX_SAFE_MALLOC
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

#ifdef MARCEL
static ma_spinlock_t	mutex = MA_SPIN_LOCK_UNLOCKED;
#define lock() ma_spin_lock(&mutex)
#define unlock() ma_spin_unlock(&mutex)
#else
#define lock() ((void)0)
#define unlock() ((void)0)
#endif

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
  lock();
  if(allocated && tbx_print_stats) { // i.e. if Safe_Malloc was really used

    fprintf(stderr, "#### SafeMalloc Stats ***\n");
    fprintf(stderr, "#### Allocated: %lu, Freed: %lu, Lost: %lu\n",
	    (long int)allocated,
	    (long int)freed,
	    ((long int)allocated) - ((long int)freed));

    if (list) {
      fprintf(stderr,
	      "SafeMalloc: Warning! All allocated memory has not been restitued :\n");
      unlock();
      tbx_safe_malloc_check(tbx_safe_malloc_VERBOSE);
      return;
    }

  }
  unlock();
}

void
tbx_safe_malloc_init(void)
{
#ifdef MARCEL
  marcel_atexit((marcel_atexit_func_t) tbx_safe_malloc_mem_check, NULL);
#else
  atexit(tbx_safe_malloc_mem_check);
#endif
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

  p = malloc(TBX_SAFE_MALLOC_HEADER_SIZE +
	     size +
	     TBX_SAFE_MALLOC_MAGIC_SIZE +
	     strlen(file) +
	     1);

  ptr = p;

  if (!ptr)
    return NULL;

#ifdef TBX_PARANO_MALLOC
  tbx_safe_malloc_check(tbx_safe_malloc_ERRORS_ONLY);
#endif

  lock();
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
  unlock();

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
	    "SafeMalloc: Error: The block at %p has a corrupted header. Size %lu malloc'ed in file %s at line %lu\n",
	    data, (unsigned long)p->size, (char*) data + p->size + TBX_SAFE_MALLOC_MAGIC_SIZE, p->line);

  if(memcmp(data + p->size,
	    tbx_safe_malloc_magic,
	    TBX_SAFE_MALLOC_MAGIC_SIZE))
    fprintf(stderr,
	    "SafeMalloc: Error: The block at %p has a corrupted trailer. Size %lu malloc'ed in file %s at line %lu\n",
	    data, (unsigned long)p->size, (char *) data + p->size + TBX_SAFE_MALLOC_MAGIC_SIZE, p->line);
}

void
tbx_safe_free(void *ptr, const char *file TBX_UNUSED, const unsigned  line TBX_UNUSED)
{
  p_tbx_safe_malloc_header_t  p    =
    ptr - TBX_SAFE_MALLOC_HEADER_SIZE;
  void                       *base = p;

  if (!p)
    TBX_FAILURE("cannot free NULL ptr");

  tbx_safe_malloc_check_chunk(p);

#ifdef TBX_PARANO_MALLOC
  tbx_safe_malloc_check(tbx_safe_malloc_ERRORS_ONLY);
#endif

  lock();
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
  unlock();

  free(p);
}

void
tbx_safe_malloc_check(tbx_safe_malloc_mode_t mode)
{
  p_tbx_safe_malloc_header_t p;

  lock();
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
  unlock();
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
#endif /* TBX_SAFE_MALLOC */

