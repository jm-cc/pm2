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

/* Options: DSM_TABLE_TRACE */


#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <malloc.h>
#include <assert.h>
#include "marcel.h"
#include "fifo_credits.h"
#include "dsm_page_manager.h"
#include "dsm_sysdep.h"
#include "dsm_pm2.h"
#include "dsm_const.h"
#include "dsm_bitmap.h"
#include "isoaddr.h"
#include "tbx.h"

#define DSM_PAGEALIGN(X) ((((int)(X))+(DSM_PAGE_SIZE-1)) & ~(DSM_PAGE_SIZE-1))
#define USER_DATA_SIZE 8

typedef struct _dsm_page_table_entry
{
  dsm_node_t       prob_owner;
  dsm_node_t       next_owner;
  fifo_t           pending_req;
  dsm_node_t       *copyset;
  int              copyset_size;
  dsm_access_t     access;
  dsm_access_t     pending_access;
  marcel_mutex_t   mutex;
  marcel_cond_t    cond;
  marcel_sem_t     sem;
  unsigned long    size;
  void             *addr;
  int              protocol;
  dsm_bitmap_t     bitmap;
  int user_data1[USER_DATA_SIZE];
  void *user_data2;
  void *twin;
} dsm_page_table_entry_t;

typedef dsm_page_table_entry_t **dsm_page_table_t;

static dsm_page_table_t dsm_page_table;

static int nb_static_dsm_pages;
static char *static_dsm_base_addr;
extern char dsm_data_begin, dsm_data_end;

static int nb_pseudo_static_dsm_pages;
static char *pseudo_static_dsm_base_addr, *pseudo_static_dsm_end_addr;
static int pseudo_static_dsm_area_size = 0;

static dsm_node_t dsm_local_node_rank = 0, dsm_nb_nodes = 1;
static int _dsm_page_distrib_mode;
static int _dsm_page_distrib_arg;
static int _dsm_page_protect_mode = DSM_OWNER_WRITE_ACCESS_OTHER_NO_ACCESS;
static int _dsm_page_protect_arg = 0;

static dsm_user_data1_init_func_t _dsm_user_data1_init_func;
static dsm_user_data2_init_func_t _dsm_user_data2_init_func;

static int _default_dsm_protocol = LI_HUDAK;

static int __inline__ _dsm_get_prot(dsm_access_t access) __attribute__ ((unused));
static int __inline__ _dsm_get_prot(dsm_access_t access)
{
  switch(access){
  case NO_ACCESS: return PROT_NONE; break;
  case READ_ACCESS: return PROT_READ; break;
  default: return PROT_READ|PROT_WRITE; /* WRITE_ACCESS */
  }
}

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

void pm2_set_dsm_page_protection(int mode, ...)
{
  va_list l;
  
  _dsm_page_protect_mode = mode;
  va_start(l, mode);
  switch(mode) {
  case DSM_OWNER_WRITE_ACCESS_OTHER_NO_ACCESS: break;
  case DSM_OWNER_READ_ACCESS_OTHER_NO_ACCESS: break;
  case DSM_CUSTOM_PROTECT: _dsm_page_protect_arg = (int)va_arg(l, int *); break;
  case DSM_UNIFORM_PROTECT:_dsm_page_protect_arg = va_arg(l, int); break;
  }
  va_end(l);
}


dsm_node_t dsm_self()
{
  return dsm_local_node_rank;
}

#define dsm_true_static_addr(addr) (&dsm_data_begin <= (char *)(addr) && (char *)(addr) < &dsm_data_end)

#define dsm_pseudo_static_addr(addr) (pseudo_static_dsm_base_addr <= (char *)(addr) && (char *)(addr) < pseudo_static_dsm_end_addr)

/*boolean dsm_addr(void *addr)
{
  return (dsm_true_static_addr(addr) || dsm_pseudo_static_addr(addr) || (isoaddr_addr(addr) && isoaddr_page_get_status(isoaddr_page_index(addr)) == ISO_SHARED));
}
*/

boolean dsm_static_addr(void *addr)
{
  return (dsm_true_static_addr(addr) || dsm_pseudo_static_addr(addr));
}


void *dsm_page_base(void *addr)
{
  if (dsm_static_addr(addr))
    return (void *)(DSM_PAGEALIGN((char*)addr - DSM_PAGE_SIZE + 1));
  else
    return (void *)isoaddr_page_addr(isoaddr_page_get_master(isoaddr_page_index(addr)));
} 


unsigned int dsm_page_offset(void *addr)
{
  if (dsm_static_addr(addr))
    return ((unsigned int)addr & (unsigned int)(DSM_PAGE_SIZE - 1));
  else
    return (unsigned int)((char*)addr - (char*)dsm_page_base(addr));
}


unsigned long dsm_page_index(void *addr)
{
    unsigned long index;

    if(dsm_true_static_addr(addr))
      index = ((char *)addr - static_dsm_base_addr)/DSM_PAGE_SIZE;
    else if (dsm_pseudo_static_addr(addr))
      index = ((char *)addr - pseudo_static_dsm_base_addr)/DSM_PAGE_SIZE + nb_static_dsm_pages;
    else
      index = isoaddr_page_get_master(isoaddr_page_index(addr)) + nb_static_dsm_pages + nb_pseudo_static_dsm_pages;
#ifdef DSM_TABLE_TRACE
    fprintf(stderr,"dsm_page_index: addr = %p => index = %ld, %s\n", addr, index, dsm_true_static_addr(addr)?"static":(dsm_pseudo_static_addr(addr)?"pseudo-static":"isoaddr"));
#endif
    return index;
}


unsigned long dsm_isoaddr_page_index(int index)
{
  return isoaddr_page_get_master(index) + nb_static_dsm_pages + nb_pseudo_static_dsm_pages;
}


unsigned long dsm_get_nb_static_pages()
{
  return nb_static_dsm_pages;
}


unsigned long dsm_get_nb_pseudo_static_pages()
{
  return nb_pseudo_static_dsm_pages;
}


void dsm_set_pseudo_static_area_size(unsigned size)
{
  pseudo_static_dsm_area_size = DSM_PAGEALIGN(size);

 /* global vars for the pseudo static area: */ 
  if (pseudo_static_dsm_area_size != 0) {
    nb_pseudo_static_dsm_pages = pseudo_static_dsm_area_size/DSM_PAGE_SIZE; // pseudo_static_dsm_area_size is a multiple of DSM_PAGE_SIZE
    pseudo_static_dsm_base_addr = (char *) DSM_PAGEALIGN(malloc(pseudo_static_dsm_area_size + DSM_PAGE_SIZE - 1));
    pseudo_static_dsm_end_addr = pseudo_static_dsm_base_addr + pseudo_static_dsm_area_size;
#ifdef DSM_TABLE_TRACE
    fprintf(stderr,"static area starts at : %p\tends at %p, size = %d pages\n", pseudo_static_dsm_base_addr, pseudo_static_dsm_end_addr, nb_pseudo_static_dsm_pages);
  /* Here I should check if the pseudo static area corresponds to the same range on all nodes. */
#endif
  }
}


void *dsm_get_pseudo_static_dsm_start_addr()
{
  return (void *)pseudo_static_dsm_base_addr;
}



static __inline__ void _dsm_global_vars_init(int my_rank, int confsize)
{
  char *page;

  dsm_local_node_rank = (dsm_node_t)my_rank;
  dsm_nb_nodes = confsize;

  /* global vars for the static area: */ 
  static_dsm_base_addr = (char *) DSM_PAGEALIGN(&dsm_data_begin);

  for (page = static_dsm_base_addr; page < (char *) &dsm_data_end; page += DSM_PAGE_SIZE)
    nb_static_dsm_pages++;
}

#ifdef min
/* Suppression d'une collision avec la macro min de la toolbox */
#undef min
#endif /* min */

#define min(a,b) ((a) < (b))?(a):(b)

static __inline__ void _dsm_page_ownership_init()
{
  int i;
  dsm_access_t access, owner_access;

  switch(_dsm_page_protect_mode){
  case DSM_OWNER_WRITE_ACCESS_OTHER_NO_ACCESS: owner_access = WRITE_ACCESS; break;
  case DSM_OWNER_READ_ACCESS_OTHER_NO_ACCESS: owner_access = READ_ACCESS; break;
  default: owner_access = NO_ACCESS;break;
  }

  switch(_dsm_page_distrib_mode){
  case DSM_CENTRALIZED :
    {
      if (dsm_local_node_rank == (dsm_node_t)_dsm_page_distrib_arg)
	access = owner_access;
      else
	access = NO_ACCESS;
      for (i = 0; i < nb_static_dsm_pages; i++)
	{
	  dsm_page_table[i]->prob_owner = (dsm_node_t)_dsm_page_distrib_arg;
	  dsm_page_table[i]->access = access;
	}
   if (_dsm_page_protect_mode == DSM_OWNER_WRITE_ACCESS_OTHER_NO_ACCESS || _dsm_page_protect_mode == DSM_OWNER_READ_ACCESS_OTHER_NO_ACCESS)
     mprotect(static_dsm_base_addr, nb_static_dsm_pages * DSM_PAGE_SIZE, _dsm_get_prot(access)); 
   break;    
    }
  
  case DSM_CYCLIC :
    {
      /* Here, _dsm_page_distrib_arg stores the chunk size: the number of
	 contiguous pages to assign to the same node */
      int bound, j, curOwner = 0;
      
      i = 0;
      while(i < nb_static_dsm_pages)
	{
	  access = (curOwner == dsm_local_node_rank)?owner_access:NO_ACCESS;
	  bound = min(_dsm_page_distrib_arg, nb_static_dsm_pages - i);
	  for (j = 0; j < bound; j++)
	    {
	      dsm_page_table[i]->prob_owner = curOwner;
	      dsm_page_table[i]->access = access;      
	      i++;
	    }
	  if (_dsm_page_protect_mode == DSM_OWNER_WRITE_ACCESS_OTHER_NO_ACCESS || _dsm_page_protect_mode == DSM_OWNER_READ_ACCESS_OTHER_NO_ACCESS)
	    mprotect(dsm_page_table[i - bound]->addr, bound * DSM_PAGE_SIZE, _dsm_get_prot(access));
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
	  dsm_page_table[i]->prob_owner = (dsm_node_t)curOwner;
	  if (curOwner == dsm_local_node_rank)
	    dsm_page_table[i]->access = owner_access;
	  else
	    dsm_page_table[i]->access = NO_ACCESS;
	  
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
      fprintf(stderr, "low = %d, high = %d, split = %d, chunk = %d\n", low, high, split, chunk);
      if (_dsm_page_protect_mode == DSM_OWNER_WRITE_ACCESS_OTHER_NO_ACCESS || _dsm_page_protect_mode == DSM_OWNER_READ_ACCESS_OTHER_NO_ACCESS)     
	mprotect(static_dsm_base_addr + (low * DSM_PAGE_SIZE),
		 (high - low) * DSM_PAGE_SIZE, _dsm_get_prot(dsm_page_table[i]->access));
      break;
    }
  case DSM_CUSTOM :
    {
      int *array = (int *)_dsm_page_distrib_arg;
      for (i = 0; i < nb_static_dsm_pages; i++)
	{
	  dsm_page_table[i]->prob_owner = array[i];
	  if (_dsm_page_protect_mode == DSM_OWNER_WRITE_ACCESS_OTHER_NO_ACCESS  || _dsm_page_protect_mode == DSM_OWNER_READ_ACCESS_OTHER_NO_ACCESS)
	    dsm_set_access(i, (dsm_local_node_rank == array[i])?owner_access:NO_ACCESS);
	  else
	    dsm_set_access_without_protect(i, (dsm_local_node_rank == array[i])?owner_access:NO_ACCESS);
	}
      break;
    }
  }

   if (_dsm_page_protect_mode != DSM_OWNER_WRITE_ACCESS_OTHER_NO_ACCESS && _dsm_page_protect_mode != DSM_OWNER_READ_ACCESS_OTHER_NO_ACCESS) 
     switch(_dsm_page_protect_mode){
     case DSM_UNIFORM_PROTECT:
       dsm_set_uniform_access((dsm_access_t)_dsm_page_protect_arg);break;
     case DSM_CUSTOM_PROTECT:
       RAISE(NOT_IMPLEMENTED);break;
     }
}


static __inline__ void _dsm_pseudo_static_page_ownership_init()
{
  int i;
  dsm_access_t access, owner_access;

  switch(_dsm_page_protect_mode){
  case DSM_OWNER_WRITE_ACCESS_OTHER_NO_ACCESS: owner_access = WRITE_ACCESS; break;
  case DSM_OWNER_READ_ACCESS_OTHER_NO_ACCESS: owner_access = READ_ACCESS; break;
  default: owner_access = NO_ACCESS;break;
  }

  switch(_dsm_page_distrib_mode){
  case DSM_CENTRALIZED :
    {
      if (dsm_local_node_rank == (dsm_node_t)_dsm_page_distrib_arg)
	access = owner_access;
      else
	access = NO_ACCESS;

      for (i = nb_static_dsm_pages; i < nb_static_dsm_pages + nb_pseudo_static_dsm_pages; i++)
	{
	  dsm_page_table[i]->prob_owner = (dsm_node_t)_dsm_page_distrib_arg;
	  dsm_page_table[i]->access = access;
	}
   if (_dsm_page_protect_mode == DSM_OWNER_WRITE_ACCESS_OTHER_NO_ACCESS || _dsm_page_protect_mode == DSM_OWNER_READ_ACCESS_OTHER_NO_ACCESS)
     mprotect(pseudo_static_dsm_base_addr, pseudo_static_dsm_area_size, _dsm_get_prot(access));
   break;    
    }
  
  case DSM_CYCLIC :
    {
      /* Here, _dsm_page_distrib_arg stores the chunk size: the number of
	 contiguous pages to assign to the same node */
      int bound, j, k, curOwner = 0;
      
      i = nb_static_dsm_pages;
      k = nb_static_dsm_pages + nb_pseudo_static_dsm_pages;
      while(i < k)
	{
	  access = (curOwner == dsm_local_node_rank)?WRITE_ACCESS:NO_ACCESS;
	  bound = min(_dsm_page_distrib_arg, k - i);
	  for (j = 0; j < bound; j++)
	    {
	      dsm_page_table[i]->prob_owner = curOwner;
	      dsm_page_table[i]->access = access;    
	      i++;
	    }
	  if (_dsm_page_protect_mode == DSM_OWNER_WRITE_ACCESS_OTHER_NO_ACCESS  || _dsm_page_protect_mode == DSM_OWNER_READ_ACCESS_OTHER_NO_ACCESS)
	    mprotect(dsm_page_table[i - bound]->addr, bound * DSM_PAGE_SIZE, _dsm_get_prot(access));
	  curOwner = (curOwner + 1) % dsm_nb_nodes;
	}
      break;
    }
  case DSM_BLOCK :
    {
      int chunk = nb_pseudo_static_dsm_pages / dsm_nb_nodes;
      int split = nb_pseudo_static_dsm_pages % dsm_nb_nodes;
      int curOwner = 0;
      int curCount = 0;
      int low;
      int high;

      if (split == 0)
	{
	  split = dsm_nb_nodes;
	  chunk -= 1;
	}
      for (i = nb_static_dsm_pages; i < nb_static_dsm_pages + nb_pseudo_static_dsm_pages; i++)
	{
	  dsm_page_table[i]->prob_owner = (dsm_node_t)curOwner;
	  if (curOwner == dsm_local_node_rank)
	    dsm_page_table[i]->access = owner_access;
	  else
	    dsm_page_table[i]->access = NO_ACCESS;
	  
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
      dsm_pseudo_static_set_no_access();
      
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
      if (_dsm_page_protect_mode == DSM_OWNER_WRITE_ACCESS_OTHER_NO_ACCESS  || _dsm_page_protect_mode == DSM_OWNER_READ_ACCESS_OTHER_NO_ACCESS)     
	mprotect(pseudo_static_dsm_base_addr + (low * DSM_PAGE_SIZE),
		 (high - low) * DSM_PAGE_SIZE,
		 owner_access);
      break;
    }
  case DSM_CUSTOM :
    {
      int k, *array = (int *)_dsm_page_distrib_arg;
      k = nb_static_dsm_pages;
      for (i = 0; i < nb_pseudo_static_dsm_pages; i++)
	{
	  dsm_page_table[i + nb_static_dsm_pages]->prob_owner = array[i];
	  if (_dsm_page_protect_mode == DSM_OWNER_WRITE_ACCESS_OTHER_NO_ACCESS || _dsm_page_protect_mode == DSM_OWNER_READ_ACCESS_OTHER_NO_ACCESS)
	    dsm_set_access(i + nb_static_dsm_pages, (dsm_local_node_rank == array[i])?owner_access:NO_ACCESS);
	  else
	    dsm_set_access_without_protect(i + nb_static_dsm_pages, (dsm_local_node_rank == array[i])?owner_access:NO_ACCESS);
	}
      break;
    }
  }
   if (_dsm_page_protect_mode != DSM_OWNER_WRITE_ACCESS_OTHER_NO_ACCESS && _dsm_page_protect_mode != DSM_OWNER_READ_ACCESS_OTHER_NO_ACCESS) 
     switch(_dsm_page_protect_mode){
     case DSM_UNIFORM_PROTECT:
       dsm_pseudo_static_set_uniform_access((dsm_access_t)_dsm_page_protect_arg);break;
     case DSM_CUSTOM_PROTECT:
       RAISE(NOT_IMPLEMENTED);break;
     }
}



void dsm_set_user_data1_init_func(dsm_user_data1_init_func_t func)
{
  _dsm_user_data1_init_func = func;
}


void dsm_set_user_data2_init_func(dsm_user_data2_init_func_t func)
{
  _dsm_user_data2_init_func = func;
}


void dsm_page_table_init(int my_rank, int confsize)
{
  int i;

  _dsm_global_vars_init(my_rank, confsize);

//  dsm_page_table = (dsm_page_table_t)tmalloc((nb_static_dsm_pages + nb_pseudo_static_dsm_pages) * sizeof(dsm_page_table_entry_t));

    dsm_page_table = (dsm_page_table_t)calloc(1, (nb_static_dsm_pages + nb_pseudo_static_dsm_pages + ISOADDR_PAGES) * sizeof(dsm_page_table_entry_t *));

  if (dsm_page_table == NULL)
    RAISE(STORAGE_ERROR);

  /* Common initializations for all nodes */

  for (i = 0; i < nb_static_dsm_pages + nb_pseudo_static_dsm_pages; i++)
    {
      dsm_page_table[i] = (dsm_page_table_entry_t *)tmalloc(sizeof(dsm_page_table_entry_t));
      dsm_page_table[i]->next_owner = (dsm_node_t)-1;
      fifo_init(&dsm_page_table[i]->pending_req, 2 * dsm_nb_nodes - 1);
      if ((dsm_page_table[i]->copyset = (dsm_node_t *)tmalloc(dsm_nb_nodes * sizeof(dsm_node_t))) == NULL)
	RAISE(STORAGE_ERROR);
      dsm_page_table[i]->copyset_size = 0;
      dsm_page_table[i]->pending_access = NO_ACCESS;
      marcel_mutex_init(&dsm_page_table[i]->mutex, NULL);
      marcel_cond_init(&dsm_page_table[i]->cond, NULL);
      marcel_sem_init(&dsm_page_table[i]->sem, 0);
      dsm_page_table[i]->size = DSM_PAGE_SIZE;
      if (i < nb_static_dsm_pages )
	dsm_page_table[i]->addr = static_dsm_base_addr + DSM_PAGE_SIZE * i;
      else
	dsm_page_table[i]->addr = pseudo_static_dsm_base_addr + DSM_PAGE_SIZE * (i - nb_static_dsm_pages);
      if (_dsm_user_data1_init_func)
	(*_dsm_user_data1_init_func)(dsm_page_table[i]->user_data1);
      if (_dsm_user_data2_init_func)
	(*_dsm_user_data2_init_func)(&dsm_page_table[i]->user_data2);
      else
	dsm_page_table[i]->user_data2 = NULL;
      dsm_page_table[i]->protocol = _default_dsm_protocol;

      /* pjh: to avoid repeated allocation */
      dsm_page_table[i]->bitmap = NULL;
    }

  _dsm_page_ownership_init();
  if (pseudo_static_dsm_area_size != 0)
    _dsm_pseudo_static_page_ownership_init();

  /* Perform protocol-specific initializations */
  for (i = 0; i < dsm_registered_protocols(); i++)
    {
    if (dsm_get_prot_init_func(i) != NULL)
      (*dsm_get_prot_init_func(i))(i);
    }
}


void dsm_display_page_ownership()
{
  int i;

  fprintf(stderr,"static pages:\n");
  for (i = 0; i < nb_static_dsm_pages; i++)
    tfprintf(stderr,"page %d: owner = %d, access = %d, addr = %p\n", i, dsm_page_table[i]->prob_owner, dsm_page_table[i]->access, dsm_page_table[i]->addr);

  if (pseudo_static_dsm_area_size != 0){
  fprintf(stderr,"pseudo-static pages:\n");
  for (i = nb_static_dsm_pages; i < nb_static_dsm_pages + nb_pseudo_static_dsm_pages; i++)
    tfprintf(stderr,"page %d: owner = %d, access = %d, addr = %p\n", i, dsm_page_table[i]->prob_owner, dsm_page_table[i]->access, dsm_page_table[i]->addr);
  }
    
}


void dsm_set_no_access()
{  
  if (mprotect(static_dsm_base_addr, nb_static_dsm_pages * DSM_PAGE_SIZE, PROT_NONE) != 0) 
    {
      perror("dsm_set_no_access could not set protection rights");
      exit(1);
    }
}


void dsm_pseudo_static_set_no_access()
{  
  if (mprotect(pseudo_static_dsm_base_addr, pseudo_static_dsm_area_size, PROT_NONE) != 0) 
    {
      perror("dsm_pseudo_static_set_no_access could not set protection rights");
      exit(1);
    }
}


void dsm_set_write_access()
{  
  mprotect(static_dsm_base_addr, nb_static_dsm_pages * DSM_PAGE_SIZE, PROT_READ|PROT_WRITE);
}


void dsm_pseudo_static_set_write_access()
{  
  mprotect(pseudo_static_dsm_base_addr, pseudo_static_dsm_area_size, PROT_READ|PROT_WRITE);
}


void dsm_set_uniform_access(dsm_access_t access)
{
  int prot;

  switch(access){
  case NO_ACCESS: prot = PROT_NONE; break;
  case READ_ACCESS: prot = PROT_READ; break;
  default: prot = PROT_READ|PROT_WRITE; /* WRITE_ACCESS */
  }
  mprotect(static_dsm_base_addr, nb_static_dsm_pages * DSM_PAGE_SIZE, prot);
}


void dsm_pseudo_static_set_uniform_access(dsm_access_t access)
{
  int prot;

  switch(access){
  case NO_ACCESS: prot = PROT_NONE; break;
  case READ_ACCESS: prot = PROT_READ; break;
  default: prot = PROT_READ|PROT_WRITE; /* WRITE_ACCESS */
  }
  mprotect(pseudo_static_dsm_base_addr, pseudo_static_dsm_area_size, prot);
}


void dsm_protect_page(void *addr, dsm_access_t access)
{
  int prot;

  switch(access){
  case NO_ACCESS: prot = PROT_NONE; break;
  case READ_ACCESS: prot = PROT_READ; break;
  default: prot = PROT_READ|PROT_WRITE; /* WRITE_ACCESS */
  }

  if (mprotect((char *)dsm_page_base(addr), dsm_page_table[dsm_page_index(addr)]->size, prot) != 0)
    {
      perror("dsm_set_no_access could not set protection rights");
      exit(1);
    }
}


void dsm_map_page(void *addr, dsm_access_t access)
{
  int prot;

  switch(access){
  case NO_ACCESS: prot = PROT_NONE; break;
  case READ_ACCESS: prot = PROT_READ; break;
  default: prot = PROT_READ|PROT_WRITE; /* WRITE_ACCESS */
  }

  if (mmap((char *)dsm_page_base(addr), dsm_page_table[dsm_page_index(addr)]->size, prot, MMAP_MASK, FILE_TO_MAP,0) == (void *)-1)
    {
      perror("dsm_map_page: could not map");
      exit(1);
    }
}


void dsm_set_prob_owner(unsigned long index, dsm_node_t owner)
{
  dsm_page_table[index]->prob_owner = owner;
#ifdef DSM_TABLE_TRACE
  tfprintf(stderr,"  Owner set: %d\n", owner);
#endif
}


dsm_node_t dsm_get_prob_owner(unsigned long index)
{
  return dsm_page_table[index]->prob_owner;
}


void dsm_set_next_owner(unsigned long index, dsm_node_t next)
{
  dsm_page_table[index]->next_owner = next;
}


void dsm_clear_next_owner(unsigned long index)
{
  dsm_page_table[index]->next_owner = NO_NODE;
}


dsm_node_t dsm_get_next_owner(unsigned long index)
{
  return dsm_page_table[index]->next_owner;
}


void dsm_store_pending_request(unsigned long index, dsm_node_t node)
     // Used to store remote read requests on a node 
     // where there is a read access pending
{
    fifo_set_next(&dsm_page_table[index]->pending_req, node);
}


dsm_node_t dsm_get_next_pending_request(unsigned long index)
{
  return fifo_get_next(&dsm_page_table[index]->pending_req);
}


boolean dsm_next_owner_is_set(unsigned long index)
{
  return (dsm_page_table[index]->next_owner != NO_NODE);
}


void dsm_set_access(unsigned long index, dsm_access_t access)
{
  dsm_protect_page(dsm_page_table[index]->addr, access);
  dsm_page_table[index]->access = access;
#ifdef DSM_TABLE_TRACE
  tfprintf(stderr,"  Access set: %d\n", access);
#endif
}


static void dsm_enable_access(unsigned long index, dsm_access_t access, boolean map)
{
  if (map)
    dsm_map_page(dsm_page_table[index]->addr, access);
  dsm_page_table[index]->access = access;
#ifdef DSM_TABLE_TRACE
  tfprintf(stderr,"  Access enabled: %d\n", access);
#endif
}


void dsm_set_access_without_protect(unsigned long index, dsm_access_t access)
{
  dsm_page_table[index]->access = access;
}


dsm_access_t dsm_get_access(unsigned long index)
{
  return dsm_page_table[index]->access;
}


void dsm_set_pending_access(unsigned long index, dsm_access_t access)
{
  dsm_page_table[index]->pending_access = access;
}


dsm_access_t dsm_get_pending_access(unsigned long index)
{
  return dsm_page_table[index]->pending_access;
}


void dsm_set_page_size(unsigned long index, unsigned long size)
{
  dsm_page_table[index]->size = size;
}


unsigned long dsm_get_page_size(unsigned long index)
{
  return dsm_page_table[index]->size;
}


void dsm_set_page_addr(unsigned long index, void *addr)
{
  dsm_page_table[index]->addr = addr;
}


void *dsm_get_page_addr(unsigned long index)
{
  return dsm_page_table[index]->addr;
}


void dsm_wait_for_page(unsigned long index)
{
  marcel_cond_wait(&dsm_page_table[index]->cond, &dsm_page_table[index]->mutex);
}


void dsm_signal_page_ready(unsigned long index)
{
  //  dsm_lock_page(index);
  marcel_cond_broadcast(&dsm_page_table[index]->cond);
  //  dsm_unlock_page(index);
}


void dsm_lock_page(unsigned long index)
{
#ifdef DSM_TABLE_TRACE
  fprintf(stderr,"  Before lock page:(I am %p)\n", marcel_self());
#endif
  marcel_mutex_lock(&dsm_page_table[index]->mutex);
#ifdef DSM_TABLE_TRACE
  fprintf(stderr,"  After lock page:(I am %p)\n", marcel_self());
#endif
}


void dsm_unlock_page(unsigned long index)
{
  marcel_mutex_unlock(&dsm_page_table[index]->mutex);
#ifdef DSM_TABLE_TRACE
  fprintf(stderr,"  Unlocked page (I am %p)\n", marcel_self());
#endif
}


void dsm_wait_ack(unsigned long index, int value)
{
  int i;
#ifdef DSM_TABLE_TRACE
  fprintf(stderr,"  Waiting for %d ack (I am %p)\n", value, marcel_self());
#endif
  for (i = 0; i < value; i++)
    marcel_sem_P(&dsm_page_table[index]->sem);
}


void dsm_signal_ack(unsigned long index, int value)
{
  int i;
  
  for (i = 0; i < value; i++)
    marcel_sem_V(&dsm_page_table[index]->sem);
}


int dsm_get_copyset_size(unsigned long index)
{
  return dsm_page_table[index]->copyset_size;
}


void dsm_add_to_copyset(unsigned long index, dsm_node_t node)
{
  int *copyset_size = &dsm_page_table[index]->copyset_size;

  dsm_page_table[index]->copyset[(*copyset_size)++] = node;
}


dsm_node_t dsm_get_next_from_copyset(unsigned long index)
{
  int *copyset_size = &dsm_page_table[index]->copyset_size;

  if (*copyset_size >0)
    {
      (*copyset_size)--;
      return dsm_page_table[index]->copyset[*copyset_size];
    }
  else
    return NO_NODE;
}


void dsm_remove_from_copyset(unsigned long index, dsm_node_t node)
/* 
   Do nothing if node not found. 
*/
{
  int i, copyset_size = dsm_page_table[index]->copyset_size;

  for (i = 0; i < copyset_size; i++)
    if (dsm_page_table[index]->copyset[i] == node)
      {
	if (i != copyset_size - 1) 
	  /* copy last element to the current location */
	  dsm_page_table[index]->copyset[i] = dsm_page_table[index]->copyset[copyset_size - 1];
	
	dsm_page_table[index]->copyset_size--;
	break;
      }
}


int dsm_get_user_data1(unsigned long index, int rank)
{
  if (rank >= USER_DATA_SIZE)
    RAISE(CONSTRAINT_ERROR);
  
  return dsm_page_table[index]->user_data1[rank];
}


void dsm_set_user_data1(unsigned long index, int rank, int value)
{
  if (rank >= USER_DATA_SIZE)
    RAISE(CONSTRAINT_ERROR);
  
  dsm_page_table[index]->user_data1[rank] = value;
}


void dsm_increment_user_data1(unsigned long index, int rank)
{
  if (rank >= USER_DATA_SIZE)
    RAISE(CONSTRAINT_ERROR);
  
  dsm_page_table[index]->user_data1[rank]++;
}


void dsm_decrement_user_data1(unsigned long index, int rank)
{
  if (rank >= USER_DATA_SIZE)
    RAISE(CONSTRAINT_ERROR);
  
  dsm_page_table[index]->user_data1[rank]--;
}


void dsm_alloc_user_data2(unsigned long index, int size)
{
  dsm_page_table[index]->user_data2 = malloc(size);
}


void dsm_free_user_data2(unsigned long index)
{
  free(dsm_page_table[index]->user_data2);
}

void *dsm_get_user_data2(unsigned long index)
{
  return dsm_page_table[index]->user_data2;
}


void dsm_set_user_data2(unsigned long index, void *addr)
{
  dsm_page_table[index]->user_data2 = addr;
}
/********************************************************************/
/* Experimental functions added by Vincent Bernardi to manage twins */
/********************* To handle with care **************************/
/********************************************************************/

void dsm_alloc_twin(unsigned long index)
{
#ifdef DEBUG
  assert(dsm_page_table[index]->twin == NULL);
#endif //DEBUG

  dsm_page_table[index]->twin = TBX_MALLOC(DSM_PAGE_SIZE);
  CTRL_ALLOC(dsm_page_table[index]->twin);
  memcpy(dsm_page_table[index]->twin, dsm_page_table[index]->addr, DSM_PAGE_SIZE);
}


void dsm_free_twin(unsigned long index)
{
  free(dsm_page_table[index]->twin);
  dsm_page_table[index]->twin = NULL;
}

void *dsm_get_twin(unsigned long index)
{
  return dsm_page_table[index]->twin;
}

void dsm_set_twin(unsigned long index, void *addr)
{
  dsm_page_table[index]->twin = addr;
}
/********************************************************************/

boolean dsm_empty_page_entry(unsigned long index)
{
  return !dsm_page_table[index];
}


void dsm_validate_page_entry(unsigned long index)
{
  if (!dsm_page_table[index])
    dsm_page_table[index] = (dsm_page_table_entry_t *)tmalloc(sizeof(dsm_page_table_entry_t));
}


void dsm_alloc_page_entry(unsigned long index)
{
  dsm_page_table[index] = (dsm_page_table_entry_t *)tmalloc(sizeof(dsm_page_table_entry_t));
}


void dsm_enable_page_entry(unsigned long index, dsm_node_t owner, int protocol, void *addr, size_t size, boolean map)
{
#if 1//def DSM_TABLE_TRACE
  fprintf(stderr,"Enabling table entry for page %ld, owner = %d , prot = %d, addr = %p, size = %d (I am %p)\n", index, owner, protocol, addr, size, marcel_self());
#endif
  dsm_page_table[index] = (dsm_page_table_entry_t *)tmalloc(sizeof(dsm_page_table_entry_t));
  dsm_page_table[index]->next_owner = (dsm_node_t)-1;
  fifo_init(&dsm_page_table[index]->pending_req, 2 * dsm_nb_nodes - 1);
  if ((dsm_page_table[index]->copyset = (dsm_node_t *)tmalloc(dsm_nb_nodes * sizeof(dsm_node_t))) == NULL)
    RAISE(STORAGE_ERROR);
  dsm_page_table[index]->copyset_size = 0;
  dsm_page_table[index]->pending_access = NO_ACCESS;
  marcel_mutex_init(&dsm_page_table[index]->mutex, NULL);
  marcel_cond_init(&dsm_page_table[index]->cond, NULL);
  marcel_sem_init(&dsm_page_table[index]->sem, 0);
  dsm_page_table[index]->size = size;//DSM_PAGE_SIZE;
  dsm_page_table[index]->addr = addr;//isoaddr_page_addr(index - nb_static_dsm_pages - nb_pseudo_static_dsm_pages);
  dsm_page_table[index]->prob_owner = owner;
  if (_dsm_user_data1_init_func)
    (*_dsm_user_data1_init_func)(dsm_page_table[index]->user_data1);
  if (_dsm_user_data2_init_func)
    (*_dsm_user_data2_init_func)(&dsm_page_table[index]->user_data2);
  else
    dsm_page_table[index]->user_data2 = NULL;

  if (protocol != DEFAULT_DSM_PROTOCOL)
    dsm_page_table[index]->protocol = protocol;
  else
    dsm_page_table[index]->protocol = _default_dsm_protocol;

  switch (_dsm_page_protect_mode) {
  case DSM_OWNER_WRITE_ACCESS_OTHER_NO_ACCESS: dsm_enable_access(index, (owner == dsm_local_node_rank)?WRITE_ACCESS:NO_ACCESS, map);break;
  case DSM_UNIFORM_PROTECT: dsm_enable_access(index, (dsm_access_t)_dsm_page_protect_arg, map);break;
  case DSM_CUSTOM_PROTECT: RAISE(NOT_IMPLEMENTED);break;
  }
  /* Perform protocol-specific initializations. */
   if (dsm_get_page_add_func(protocol) != NULL)
    (*dsm_get_page_add_func(protocol))(index); 
}


void dsm_set_default_protocol(int protocol)
{
  _default_dsm_protocol = protocol;
}


void dsm_set_page_protocol(unsigned long index, int protocol)
{
  dsm_page_table[index]->protocol = protocol;
}


int dsm_get_page_protocol(unsigned long index)
{
  return dsm_page_table[index]->protocol;
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
	dsm_set_access(i, NO_ACCESS);
	dsm_free_page_bitmap(i);
      }
}

void dsm_invalidate_not_owned_pages_without_protect()
{
  unsigned long i;

  for (i = 0; i < nb_static_dsm_pages; i++)
    if ((dsm_get_prob_owner(i) != dsm_self())  && (dsm_get_access(i) != NO_ACCESS))
      {
	dsm_set_access_without_protect(i, NO_ACCESS);
	dsm_free_page_bitmap(i);
      }
}


void dsm_pseudo_static_invalidate_not_owned_pages()
{
  unsigned long i;

  for (i = nb_static_dsm_pages; i < nb_static_dsm_pages + nb_pseudo_static_dsm_pages; i++)
    if ((dsm_get_prob_owner(i) != dsm_self())  && (dsm_get_access(i) != NO_ACCESS))
      {
	dsm_lock_page(i);
	dsm_set_access(i, NO_ACCESS);
	dsm_free_page_bitmap(i);
	dsm_unlock_page(i);
      }
}

void dsm_pseudo_static_invalidate_not_owned_pages_without_protect()
{
  unsigned long i;

  for (i = nb_static_dsm_pages; i < nb_static_dsm_pages + nb_pseudo_static_dsm_pages; i++)
    if ((dsm_get_prob_owner(i) != dsm_self())  && (dsm_get_access(i) != NO_ACCESS))
      {
	dsm_lock_page(i);
	dsm_set_access_without_protect(i, NO_ACCESS);
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
  dsm_page_table[index]->bitmap = dsm_bitmap_alloc(dsm_page_table[index]->size);
}


void dsm_free_page_bitmap(unsigned long index)
{
  dsm_bitmap_free(dsm_page_table[index]->bitmap);

  /* pjh: to avoid repeated allocation */
  dsm_page_table[index]->bitmap = NULL;
}


void dsm_mark_dirty(void *addr, int length)
{
  unsigned long index = dsm_page_index(addr);

#ifdef DSM_TABLE_TRACE
  tfprintf(stderr,"dsm_mark_dirty: addr = %p, index = %ld, length=%d\n",
    addr, index, length);
#endif
  if (dsm_get_prob_owner(index) != dsm_self())
  {
#ifdef DSM_TABLE_TRACE
  tfprintf(stderr,"dsm_mark_dirty: page offset = %d, length=%d\n", dsm_page_offset(addr), length);
#endif
    /* pjh: make sure that no one is sending diffs while I am in here! */
    dsm_lock_page(index);
    dsm_bitmap_mark_dirty(dsm_page_offset(addr), length,
      dsm_page_table[index]->bitmap);
    dsm_unlock_page(index);
  }
}



void *dsm_get_next_modified_data(unsigned long index, int *size)
{
  static int _offset = 0;
  int start;
 
  if ((start = first_series_of_1_from_offset(dsm_page_table[index]->bitmap, dsm_page_table[index]->size, _offset, size)) == -1)
    {
      _offset = 0;
#ifdef DSM_TABLE_TRACE
      tfprintf(stderr,"dsm_get_next_modified_data(%d) returns NULL\n", (int) index);
#endif
      return NULL;
    }
  else
    {
      _offset = start + *size;
#ifdef DSM_TABLE_TRACE
      tfprintf(stderr,"dsm_get_next_modified_data(%d) returns %p\n", (int) index,
	       (void *)((char*)dsm_get_page_addr(index) + start));
#endif
      return (void *)((char*)dsm_get_page_addr(index) + start);
    }
}


boolean dsm_page_bitmap_is_empty(unsigned long index) 
{
#ifdef DSM_TABLE_TRACE
tfprintf(stderr, "dsm_bitmap_is_empty(%ld)\n", index);
#endif
  return dsm_bitmap_is_empty(dsm_page_table[index]->bitmap, dsm_page_table[index]->size);
}


void dsm_page_bitmap_clear(unsigned long index) 
{
#ifdef DSM_TABLE_TRACE
tfprintf(stderr, "dsm_page_bitmap_clear(%ld) called\n", index);
#endif
  dsm_bitmap_clear(dsm_page_table[index]->bitmap, dsm_page_table[index]->size);
}

/* pjh: to avoid repeated allocation */
boolean dsm_page_bitmap_is_allocated(unsigned long index)
{
  return (dsm_page_table[index]->bitmap != NULL);
}

