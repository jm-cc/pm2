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
$Log: safe_malloc.c,v $
Revision 1.3  2000/02/28 10:30:41  rnamyst
Modified to change #include <> into #include "". Also renamed the files that
should not be automatically included in libdsm.a.

Revision 1.2  2000/01/31 15:58:34  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#include "safe_malloc.h"

#include <stdio.h>
#include <string.h>

#define ALIGNED(x, a)   (((unsigned)(x) + (a) - 1) & ~(unsigned)((a) - 1))

#define ALIGNED16(x)   ALIGNED((x), 16)

static char magic[] =
"MarcelMadeleine MarcelMadeleine MarcelMadeleine MarcelMadeleine "
"MarcelMadeleine MarcelMadeleine MarcelMadeleine MarcelMadeleine "
"MarcelMadeleine MarcelMadeleine MarcelMadeleine MarcelMadeleine "
"MarcelMadeleine MarcelMadeleine MarcelMadeleine MarcelMadeleine "
"MarcelMadeleine MarcelMadeleine MarcelMadeleine MarcelMadeleine "
"MarcelMadeleine MarcelMadeleine MarcelMadeleine MarcelMadeleine "
"MarcelMadeleine MarcelMadeleine MarcelMadeleine MarcelMadeleine "
"MarcelMadeleine MarcelMadeleine MarcelMadeleine MarcelMadeleine"; /* 512 */

typedef struct _head{
  size_t size;
  struct _head *next, *prev;
  unsigned line;
} header;

#define HEAD_SIZE   (ALIGNED16(sizeof(header)) + sizeof(magic))

static header *list, *last = NULL;

static size_t allocated = 0, freed = 0;

static void mem_check()
{
  fprintf(stderr, "*** SafeMalloc Stats ***\n");
  fprintf(stderr, "Allocated: %u, Freed: %u, Lost: %u\n",
	  allocated,
	  freed,
	  allocated - freed);
  if(list != NULL) {
    fprintf(stderr, "SafeMalloc: Warning! All allocated memory has not been restitued :\n");
    safe_malloc_check(SAFE_MALLOC_VERBOSE);
  }
}

void safe_malloc_init()
{
  atexit(mem_check);
}

void *safe_malloc(size_t size, char *file, unsigned line)
{
  header *p = malloc(HEAD_SIZE + size + sizeof(magic) + strlen(file) + 1);

  allocated += size;

  p->next = NULL;
  p->size = size;
  p->line = line;
  memcpy((char *)p + ALIGNED16(sizeof(header)), magic, sizeof(magic));
  memcpy((char *)p + HEAD_SIZE + size, magic, sizeof(magic));
  strcpy((char *)p + HEAD_SIZE + size + sizeof(magic), file);

  if(list == NULL) {
    list = last = p;
    p->prev = NULL;
  } else {
    last->next = p;
    p->prev = last;
    last = p;
  }

  return (char *)p + HEAD_SIZE;
}

void *safe_calloc(size_t nmemb, size_t size, char *file, unsigned line)
{
  void *p = safe_malloc(nmemb * size, file, line);

  memset(p, 0, nmemb * size);

  return p;
}

void *safe_realloc(void *ptr, size_t size, char *file, unsigned line)
{
  return NULL;
}

static void check_chunk(header *p)
{
  if(memcmp((char *)p + ALIGNED16(sizeof(header)), magic, sizeof(magic)))
    fprintf(stderr, "SafeMalloc: Error: The block at %p has a corrupted header:\n",
	    (char *)p + HEAD_SIZE);

  if(memcmp((char *)p + HEAD_SIZE + p->size, magic, sizeof(magic)))
    fprintf(stderr, "SafeMalloc: Error: The block at %p has a corrupted trailer:\n",
	    (char *)p + HEAD_SIZE);
}

void safe_free(void *ptr, char *file, unsigned line)
{
  header *p = (header *)(ptr - HEAD_SIZE);

  check_chunk(p);

  if(p->next == NULL)
    last = p->prev;
  else
    p->next->prev = p->prev;

  if(p->prev == NULL)
    list = p->next;
  else
    p->prev->next = p->next;

  freed += p->size;

  memset((char *)p + ALIGNED16(sizeof(header)), 0, sizeof(magic));
  memset((char *)p + HEAD_SIZE + p->size, 0, sizeof(magic));

  free(p);
}

void safe_malloc_check(int mode)
{
  header *p;

  for(p=list; p; p=p->next) {

    check_chunk(p);

    if(mode == SAFE_MALLOC_VERBOSE) 
      fprintf(stderr, "\t[addr=%p, size=%u, malloc'ed in file %s at line %d]\n",
	      (char *)p + HEAD_SIZE,
	      p->size,
	      (char *)p + HEAD_SIZE + p->size + sizeof(magic),
	      p->line);

  }
}
