/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 2.0

             Gabriel Antoniu, Luc Bouge, Christian Perez,
                Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 8512 CNRS-INRIA
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


#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdarg.h>
#include "errno.h"
#include "marcel.h"
#include "fifo_credits.h"
#include "dsm_page_manager.h"
#include "dsm_sysdep.h"
#include "dsm_pm2.h"
#include "dsm_const.h"
#include "dsm_bitmap.h"

#define DSM_PAGEALIGN(X) ((((int)(X))+(DSM_PAGE_SIZE-1)) & ~(DSM_PAGE_SIZE-1))

typedef struct _dsm_page_table_entry
{
  int              state;
  dsm_node_t       prob_owner;
  dsm_node_t       next_owner;
  fifo_t           pending_req;
  dsm_node_t       *copyset;
  int              copyset_size;
  dsm_access_t     access;
  dsm_access_t     pending_access;
/*    dsm_protocol_t   protocol; */
  marcel_mutex_t   mutex;
  marcel_cond_t    cond;
  marcel_sem_t     sem;
  unsigned long    size;
  void             *addr;
  dsm_bitmap_t     bitmap;
} dsm_page_table_entry_t;

typedef dsm_page_table_entry_t *dsm_page_table_t;

static dsm_page_table_t dsm_page_table;

static int nb_static_dsm_pages;
static char *static_dsm_base_addr;
extern char dsm_data_begin, dsm_data_end;
static dsm_node_t dsm_local_node_rank = 0, dsm_nb_nodes = 1;
static int _dsm_page_distrib_mode;
static int _dsm_page_distrib_arg;


void pm2_set_dsm_page_distribution(int mode, ...)
{
  va_list l;
  
  _dsm_page_distrib_mode = mode;
  va_start(l, mode);
  switch(mode) {
  case DSM_CENTRALIZED: 
  case DSM_CYCLIC: _dsm_page_distrib_arg = va_arg(l, int); break;
  case DSM_CUSTOM: _dsm_page_distrib_arg = (int)va_arg(l, int *); break;
  case DSM_BLOCK: break;
  }
  va_end(l);
}


dsm_node_t dsm_self()
{
  return dsm_local_node_rank;
}


boolean dsm_addr(void *addr)
{
  return (&dsm_data_begin <= (char *)addr && (char *)addr < &dsm_data_end);
}

void *dsm_page_base(void *addr)
{
  return (void *)(DSM_PAGEALIGN((char*)addr - DSM_PAGE_SIZE + 1));
} 

unsigned int dsm_page_offset(void *addr)
{
  return ((unsigned int)addr & (unsigned int)(DSM_PAGE_SIZE - 1));
}

unsigned long dsm_page_index(void *addr)
{
  return (unsigned long)(((char *)addr - static_dsm_base_addr)/DSM_PAGE_SIZE);
}

unsigned long dsm_get_nb_static_pages()
{
  return nb_static_dsm_pages;
}


static void _dsm_global_vars_init(int my_rank, int confsize)
{
  char *page;

  dsm_local_node_rank = (dsm_node_t)my_rank;
  dsm_nb_nodes = confsize;

  static_dsm_base_addr = (char *) DSM_PAGEALIGN(&dsm_data_begin);

  for (page = static_dsm_base_addr; page < (char *) &dsm_data_end; page += DSM_PAGE_SIZE)
    nb_static_dsm_pages++;
}

#define min(a,b) ((a) < (b))?(a):(b)

void dsm_page_ownership_init()
{
  int i;

  switch(_dsm_page_distrib_mode){
  case DSM_CENTRALIZED :
    {
      dsm_access_t access = (dsm_local_node_rank == (dsm_node_t)_dsm_page_distrib_arg)?WRITE_ACCESS:NO_ACCESS;
      for (i = 0; i < nb_static_dsm_pages; i++)
	{
	  dsm_page_table[i].prob_owner = (dsm_node_t)_dsm_page_distrib_arg;
	  dsm_page_table[i].access = access;
	}
      
      mprotect(static_dsm_base_addr, nb_static_dsm_pages * DSM_PAGE_SIZE, access == WRITE_ACCESS? PROT_READ|PROT_WRITE:PROT_NONE);
      break;    
    }
  
  case DSM_CYCLIC :
    {
      /* Here, _dsm_page_distrib_arg stores the chunk size: the number of
	 contiguous pages to assign to the same node */
      int bound, j, curOwner = 0;
      dsm_access_t access;
      
      i = 0;
      while(i < nb_static_dsm_pages)
	{
	  access = (curOwner == dsm_local_node_rank)?WRITE_ACCESS:NO_ACCESS;
	  bound = min(_dsm_page_distrib_arg, nb_static_dsm_pages - i);
	  for (j = 0; j < bound; j++)
	    {
	      dsm_page_table[i].prob_owner = curOwner;
	      dsm_page_table[i].access = access;      
	      i++;
	    }
	  mprotect(dsm_page_table[i - bound].addr, bound * DSM_PAGE_SIZE, access == WRITE_ACCESS? PROT_READ|PROT_WRITE:PROT_NONE);
	  curOwner = (curOwner + 1) % dsm_nb_nodes;
	}
      break;
    }
  case DSM_BLOCK :
    {
      int chunk = nb_static_dsm_pages / dsm_nb_nodes;
      int split = nb_static_dsm_pages % dsm_nb_nodes;
      int curOwner = 0;
      int curCount = 0;
      int low;
      int high;

      if (split == 0)
	{
	  split = dsm_nb_nodes;
	  chunk -= 1;
	}
      for (i = 0; i < nb_static_dsm_pages; i++)
	{
	  dsm_page_table[i].prob_owner = (dsm_node_t)curOwner;
	  if (curOwner == dsm_local_node_rank)
	    dsm_page_table[i].access = WRITE_ACCESS;
	  else
	    dsm_page_table[i].access = NO_ACCESS;
	  
	  curCount++;
	  if (curOwner < split)
	    {
	      if (curCount == chunk+1)
		{
		  curOwner++;
		  curCount = 0;
		}
	    }
	  else
	    {
	      if (curCount == chunk)
		{
		  curOwner++;
		  curCount = 0;
		}
	    }
	}
      dsm_set_no_access();
      
      /* what are the bounds on my chunk? */
      if (dsm_local_node_rank < split)
	{
	  low = (dsm_local_node_rank * (chunk + 1));
	  high = low + (chunk + 1);
	}
      else
	{
	  low = (split * (chunk + 1)) + ((dsm_local_node_rank - split) * chunk);
	  high = low + chunk;
	}
      
      mprotect(static_dsm_base_addr + (low * DSM_PAGE_SIZE),
	       (high - low + 1) * DSM_PAGE_SIZE,
	       PROT_READ|PROT_WRITE);
      break;
    }
  case DSM_CUSTOM :
    {
      int *array = (int *)_dsm_page_distrib_arg;
      for (i = 0; i < nb_static_dsm_pages; i++)
	{
	  dsm_page_table[i].prob_owner = array[i];
	  dsm_set_access(i, (dsm_local_node_rank == array[i])?WRITE_ACCESS:NO_ACCESS);
	}
      break;
    }
  }
  
}


void dsm_page_table_init(int my_rank, int confsize)
{
  int i;

  /* pjh: contiguous allocation of pages
   *
   *      The idea is to give all nodes a roughly equal number of
   *      contiguous pages. Every node has at least "chunk" number
   *      of pages. Nodes with node numbers less than "split" have
   *      one additional page assigned. (That is, the leftover pages
   *      are assigned one to a node starting with node 0, node 1, etc.)
   */
  int curOwner;
  int curCount;

  _dsm_global_vars_init(my_rank, confsize);

  dsm_page_table = (dsm_page_table_t)tmalloc(nb_static_dsm_pages * sizeof(dsm_page_table_entry_t));
  if (dsm_page_table == NULL)
    RAISE(STORAGE_ERROR);

  /* Common initializations for all nodes */

  curOwner = 0; /* pjh */
  curCount = 0;

  for (i = 0; i < nb_static_dsm_pages; i++)
    {
      dsm_page_table[i].next_owner = (dsm_node_t)-1;
      fifo_init(&dsm_page_table[i].pending_req, 2 * dsm_nb_nodes - 1);
      if ((dsm_page_table[i].copyset = (dsm_node_t *)tmalloc(dsm_nb_nodes * sizeof(dsm_node_t))) == NULL)
	RAISE(STORAGE_ERROR);
      dsm_page_table[i].copyset_size = 0;
      dsm_page_table[i].pending_access = NO_ACCESS;
      marcel_mutex_init(&dsm_page_table[i].mutex, NULL);
      marcel_cond_init(&dsm_page_table[i].cond, NULL);
      marcel_sem_init(&dsm_page_table[i].sem, 0);
      dsm_page_table[i].size = DSM_PAGE_SIZE;
      dsm_page_table[i].addr = static_dsm_base_addr + DSM_PAGE_SIZE * i;
    }
  dsm_page_ownership_init();
}

void dsm_display_page_ownership()
{
  int i;

  for (i = 0; i < nb_static_dsm_pages; i++)
    tfprintf(stderr,"page %d: owner = %d, access = %d\n", i, dsm_page_table[i].prob_owner, dsm_page_table[i].access);
}

void dsm_set_no_access()
{  
  if (mprotect(static_dsm_base_addr, nb_static_dsm_pages * DSM_PAGE_SIZE, PROT_NONE) != 0) 
    {
      perror("dsm_set_no_access could not set protection rights");
      exit(1);
    }
}


void dsm_set_write_access()
{  
  mprotect(static_dsm_base_addr, nb_static_dsm_pages * DSM_PAGE_SIZE, PROT_READ|PROT_WRITE);
}


void dsm_protect_page(void *addr, dsm_access_t access)
{
  int prot;

  switch(access){
  case NO_ACCESS: prot = PROT_NONE; break;
  case READ_ACCESS: prot = PROT_READ; break;
  default: prot = PROT_READ|PROT_WRITE; /* WRITE_ACCESS */
  }

  mprotect((char *)dsm_page_base(addr), DSM_PAGE_SIZE, prot);
}


void dsm_set_prob_owner(unsigned long index, dsm_node_t owner)
{
  dsm_page_table[index].prob_owner = owner;
#ifdef DEBUG3
  tfprintf(stderr,"owner set: %d\n", owner);
#endif
}


dsm_node_t dsm_get_prob_owner(unsigned long index)
{
  return dsm_page_table[index].prob_owner;
}


void dsm_set_next_owner(unsigned long index, dsm_node_t next)
{
  dsm_page_table[index].next_owner = next;
}


void dsm_clear_next_owner(unsigned long index)
{
  dsm_page_table[index].next_owner = NO_NODE;
}


dsm_node_t dsm_get_next_owner(unsigned long index)
{
  return dsm_page_table[index].next_owner;
}


void dsm_store_pending_request(unsigned long index, dsm_node_t node)
     // Used to store remote read requests on a node 
     // where there is a read access pending
{
    fifo_set_next(&dsm_page_table[index].pending_req, node);
}


dsm_node_t dsm_get_next_pending_request(unsigned long index)
{
  return fifo_get_next(&dsm_page_table[index].pending_req);
}


boolean dsm_next_owner_is_set(unsigned long index)
{
  return (dsm_page_table[index].next_owner != NO_NODE);
}


void dsm_set_access(unsigned long index, dsm_access_t access)
{
  dsm_protect_page(dsm_page_table[index].addr, access);
  dsm_page_table[index].access = access;
#ifdef DEBUG3
  tfprintf(stderr,"access set: %d\n", access);
#endif
}


dsm_access_t dsm_get_access(unsigned long index)
{
  return dsm_page_table[index].access;
}


void dsm_set_pending_access(unsigned long index, dsm_access_t access)
{

   dsm_page_table[index].pending_access = access;
}


dsm_access_t dsm_get_pending_access(unsigned long index)
{
  return dsm_page_table[index].pending_access;
}


void dsm_set_page_size(unsigned long index, unsigned long size)
{
  dsm_page_table[index].size = size;
}


unsigned long dsm_get_page_size(unsigned long index)
{
  return dsm_page_table[index].size;
}


void dsm_set_page_addr(unsigned long index, void *addr)
{
  dsm_page_table[index].addr = addr;
}


void *dsm_get_page_addr(unsigned long index)
{
  return dsm_page_table[index].addr;
}


void dsm_wait_for_page(unsigned long index)
{
  marcel_cond_wait(&dsm_page_table[index].cond, &dsm_page_table[index].mutex);
}


void dsm_signal_page_ready(unsigned long index)
{
  //  dsm_lock_page(index);
  marcel_cond_broadcast(&dsm_page_table[index].cond);
  //  dsm_unlock_page(index);
}


void dsm_lock_page(unsigned long index)
{
#ifdef DEBUG1
  fprintf(stderr,"before lock page:(I am %p)\n", marcel_self());
#endif
  marcel_mutex_lock(&dsm_page_table[index].mutex);
#ifdef DEBUG1
  fprintf(stderr,"after lock page:(I am %p)\n", marcel_self());
#endif
}


void dsm_unlock_page(unsigned long index)
{
  marcel_mutex_unlock(&dsm_page_table[index].mutex);
#ifdef DEBUG1
  fprintf(stderr,"unlocked page (I am %p)\n", marcel_self());
#endif
}


void dsm_wait_ack(unsigned long index, int value)
{
  int i;
#ifdef DEBUG1
  fprintf(stderr,"Waiting for %d ack (I am %p)\n", value, marcel_self());
#endif
  for (i = 0; i < value; i++)
    marcel_sem_P(&dsm_page_table[index].sem);
}


void dsm_signal_ack(unsigned long index, int value)
{
  int i;
  
  for (i = 0; i < value; i++)
    marcel_sem_V(&dsm_page_table[index].sem);
}


int dsm_get_copyset_size(unsigned long index)
{
  return dsm_page_table[index].copyset_size;
}


void dsm_add_to_copyset(unsigned long index, dsm_node_t node)
{
  int *copyset_size = &dsm_page_table[index].copyset_size;

  dsm_page_table[index].copyset[(*copyset_size)++] = node;
}


dsm_node_t dsm_get_next_from_copyset(unsigned long index)
{
  int *copyset_size = &dsm_page_table[index].copyset_size;

  if (*copyset_size >0)
    {
      (*copyset_size)--;
      return dsm_page_table[index].copyset[*copyset_size];
    }
  else
    return NO_NODE;
}


void dsm_remove_from_copyset(unsigned long index, dsm_node_t node)
/* 
   Do nothing if node not found. 
*/
{
  int i, copyset_size = dsm_page_table[index].copyset_size;

  for (i = 0; i < copyset_size; i++)
    if (dsm_page_table[index].copyset[i] == node)
      {
	if (i != copyset_size - 1) 
	  /* copy last element to the current location */
	  dsm_page_table[index].copyset[i] = dsm_page_table[index].copyset[copyset_size - 1];
	
	dsm_page_table[index].copyset_size--;
	break;
      }
}


void dsm_set_page_state(unsigned long index, dsm_state_t state)
{
  dsm_page_table[index].state = state;
}


dsm_state_t dsm_get_page_state(unsigned long index)
{
  return dsm_page_table[index].state;
}

/*********************** Hyperion stuff: ****************************/


/* Need mutual exclusion when modifying the the access field.
 * Optimization for Hyperion: no need for mutual exclusion when reading
 * the owner, since it never changes. Thus, only non local pages are 
 * locked/unlocked. Moreover, if Hyperion guarantees that the function 
 * is called within a critical section, the lock operations can be removed.
 */
void dsm_invalidate_not_owned_pages()
{
  unsigned long i;

  for (i = 0; i < nb_static_dsm_pages; i++)
    if ((dsm_get_prob_owner(i) != dsm_self())  && (dsm_get_access(i) != NO_ACCESS))
      {
	dsm_lock_page(i);
	dsm_set_access(i, NO_ACCESS);
	dsm_free_page_bitmap(i);
	dsm_unlock_page(i);
      }
}

/* dsm_alloc_page_bitmap should be called after the initialization 
 * of the page table, so the field 'size' is supposed to have been
 * previously initialized.
 */
void dsm_alloc_page_bitmap(unsigned long index)
{
  dsm_page_table[index].bitmap = dsm_bitmap_alloc(dsm_page_table[index].size);
}


void dsm_free_page_bitmap(unsigned long index)
{
  dsm_bitmap_free(dsm_page_table[index].bitmap);
}

//#define DEBUG_HYP
void dsm_mark_dirty(void *addr, int length)
{
#ifdef DEBUG_HYP
  fprintf(stderr,"dsm_mark_dirty: addr = %p, length=%d\n", addr, length);
#endif
  if (dsm_get_prob_owner(dsm_page_index(addr)) != dsm_self())
    dsm_bitmap_mark_dirty(dsm_page_offset(addr), length, dsm_page_table[dsm_page_index(addr)].bitmap);
}


void *dsm_get_next_modified_data(unsigned long index, int *size)
{
  static int _offset = 0;
  int start;
  
  if ((start = first_series_of_1_from_offset(dsm_page_table[index].bitmap, dsm_page_table[index].size/8, _offset, size)) == -1)
    {
      _offset = 0;
      return NULL;
    }
  else
    {
      _offset = start + *size;
      return (void *)((char*)dsm_get_page_addr(index) + start);
    }
}


boolean dsm_page_bitmap_is_empty(unsigned long index) 
{
  return dsm_bitmap_is_empty(dsm_page_table[index].bitmap, dsm_page_table[index].size/8);
}


void dsm_page_bitmap_clear(unsigned long index) 
{
  dsm_bitmap_clear(dsm_page_table[index].bitmap, dsm_page_table[index].size);
}
