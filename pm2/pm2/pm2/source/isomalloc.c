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

#ifndef DEBUG
#define NDEBUG 1
#endif

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>

#include <marcel.h>
#include <sys/archdep.h>
#include <madeleine.h>

#include <pm2.h>
#include <sys/isomalloc_archdep.h>
#include <sys/isomalloc_rpc.h>
#include <isomalloc.h>
#include <sys/bitmap.h>
#include <magic.h>

#include <isomalloc_timing.h>

#define ALIGN_TO_SLOT(x) (ALIGN(x, SLOT_SIZE))

static unsigned long page_size;
static unsigned local_node_rank = 0, nb_nodes = 1;

#define SLOT_INDEX(addr) \
        ((int)((SLOT_AREA_TOP - (int)addr) / SLOT_SIZE) - 1) 

#define GET_SLOT_ADDR(bit_abs_index) \
        ((void *)(SLOT_AREA_TOP - (bit_abs_index + 1) * SLOT_SIZE))


#undef DEBUG 

/* #define DEBUG_MARCEL */

/********************************  slot map configuration ****************************/

#define MARK_SLOTS_AVAILABLE(start, n, bitmap) set_bits_to_1(start, n, bitmap)

#define MARK_SLOTS_BUSY(start, n, bitmap) reset_bits_to_0(start, n, bitmap)

#define FIRST_SLOT_AVAILABLE(bitmap) first_bit_to_1(bitmap)

#define GET_FIRST_SLOT_AVAILABLE(bitmap) get_first_bit_to_1(bitmap)

#define FIRST_AVAILABLE_CONTIGUOUS_SLOTS(n, bitmap) first_bits_to_1(n, bitmap)

#define GET_FIRST_AVAILABLE_CONTIGUOUS_SLOTS(n, bitmap) get_first_bits_to_1(n, bitmap)


static unsigned int cycle_start = 0;
unsigned int *local_slot_map;

void pm2_set_uniform_slot_distribution(unsigned int nb_contiguous_slots, int nb_cycles)
{
  set_cyclic_sequences(cycle_start + local_node_rank * nb_contiguous_slots, nb_contiguous_slots, nb_contiguous_slots * nb_nodes, nb_cycles, local_slot_map);
 
  /* update current bit pointer */
  cycle_start += nb_contiguous_slots * nb_nodes * nb_cycles;
}


void pm2_set_non_uniform_slot_distribution(int node, unsigned int offset, unsigned int nb_contiguous_slots, unsigned int period, int nb_cycles)
{
if (node == local_node_rank)
  {
    set_cyclic_sequences(cycle_start + offset, nb_contiguous_slots, period, nb_cycles, local_slot_map);
 
  /* update current bit pointer */
    cycle_start += period * nb_cycles;
  }
}


#define begin_slot_distribution() cycle_start = 0

#define end_slot_distribution() pm2_set_uniform_slot_distribution(1, -1)

volatile pm2_isomalloc_config_hook  _pm2_isomalloc_config_func = NULL;

int *_pm2_isomalloc_config_arg = (int *)NULL;

#define BLOCK_CYCLIC 1

#define UNIFORM_ADAPTIVE_BLOCK_CYCLIC 2

#define NON_UNIFORM_ADAPTIVE_BLOCK_CYCLIC 3

#define CUSTOM 0

unsigned int slot_distribution_mode;


void pm2_set_isomalloc_config_func(pm2_isomalloc_config_hook f, int *arg)
{
   _pm2_isomalloc_config_func = f;
   _pm2_isomalloc_config_arg = arg;
}


static void _internal_isomalloc_config_slot_map()
{
 begin_slot_distribution();
 if(_pm2_isomalloc_config_func != NULL)
     (*_pm2_isomalloc_config_func)(_pm2_isomalloc_config_arg);
 end_slot_distribution();
}


/******************************  negociation *********************************/

static marcel_mutex_t isomalloc_global_mutex, isomalloc_slot_mutex, isomalloc_local_mutex, isomalloc_sync_mutex;

static int *tab_sync ;

static marcel_sem_t isomalloc_send_status_sem, isomalloc_receive_status_sem ;

extern marcel_sem_t sem_nego;

static bitmap_t aux_slot_map;

LRPC_REQ(LRPC_ISOMALLOC_SEND_SLOT_STATUS) *global_slot_map;

#define slot_lock() marcel_mutex_lock(&isomalloc_slot_mutex)

#define global_lock() /* fprintf(stderr, "global lock\n"); */marcel_mutex_lock(&isomalloc_global_mutex)

#define slot_unlock() marcel_mutex_unlock(&isomalloc_slot_mutex)

#define global_unlock() /* fprintf(stderr, "global unlock\n"); */marcel_mutex_unlock(&isomalloc_global_mutex)

#define local_lock() /* fprintf(stderr, "local lock\n"); */ marcel_mutex_lock(&isomalloc_local_mutex)

#define local_unlock() /* fprintf(stderr, "local unlock\n"); */marcel_mutex_unlock(&isomalloc_local_mutex)

#define sync_lock() marcel_mutex_lock(&isomalloc_sync_mutex)

#define sync_unlock() marcel_mutex_unlock(&isomalloc_sync_mutex)

static void isomalloc_global_sync();

/* #define DEBUG_NEGOCIATION */


#define forbid_sends() /* fprintf(stderr,"forbid sends\n"); */marcel_sem_P(&sem_nego)

#define allow_sends() /* fprintf(stderr,"allow sends\n"); */marcel_sem_V(&sem_nego)

#define GLOBAL_LOCK_NODE 0

#define wait_for_all_slot_maps() \
 { \
   int j;\
   for (j = 1; j < nb_nodes; j++) \
     marcel_sem_P(&isomalloc_receive_status_sem);\
 }

void  buy_slots_and_redistribute(unsigned int n, unsigned int *rank) 
{ 
  int j, k; 
  unsigned int i, *p1, *p2, *p; 
 /*
   global OR on the slot maps
 */
 p1 = local_slot_map; 
 p2 = (unsigned int *)((local_node_rank == 0) ? global_slot_map[1].slot_map : global_slot_map[0].slot_map); 
 p = (unsigned int *)aux_slot_map; 
 OR_bitmaps_1(p, p1, p2);

 for (k = 1; k < nb_nodes ; k++) 
   if (k != local_node_rank) 
     { 
       p1 = (unsigned int *)global_slot_map[k].slot_map; 
       p = (unsigned int *)aux_slot_map; 
       OR_bitmaps_2(p, p1);
     } 

 /* search for the contiguous slots in the global slot_map */
 i = FIRST_AVAILABLE_CONTIGUOUS_SLOTS(n, aux_slot_map); 
 if (i == -1) 
   RAISE(STORAGE_ERROR); 

 /* mark these slots as busy on all nodes */
 for (j = 0; j < nb_nodes; j++) 
     MARK_SLOTS_BUSY(i, n, global_slot_map[j].slot_map); 


#ifdef DEBUG
 tfprintf(stderr,"slot_negociate: start index =%d, end_index = %d\n",i, i + n -1); 
#endif 
 *rank = i;
}

#define send_slot_maps_to_the_nodes() \
{ \
  int j; \
\
  for (j = 0; j < nb_nodes ; j++) \
    if (j != local_node_rank) \
       QUICK_ASYNC_LRPC(j, LRPC_ISOMALLOC_LOCAL_UNLOCK, &global_slot_map[j]); \
										      }

void launch_negociation_on_all_nodes() 
{ 
 int j; 

 for (j= 0; j < nb_nodes; j++) 
   if (j != local_node_rank) 
     { 
#ifdef DEBUG_NEGOCIATION 
       tfprintf(stderr,"local lock on module %d \n", j); 
#endif 
       ASYNC_LRPC(j, LRPC_ISOMALLOC_LOCAL_LOCK, STD_PRIO, DEFAULT_STACK, &local_node_rank); 
      } 
}

void remote_global_lock() 
{ 
#ifdef DEBUG_NEGOCIATION 
 LRPC_REQ(LRPC_ISOMALLOC_GLOBAL_LOCK) global_lock_req; 

 global_lock_req.master = local_node_rank; 
 LRPC(GLOBAL_LOCK_NODE, LRPC_ISOMALLOC_GLOBAL_LOCK, STD_PRIO, DEFAULT_STACK, &global_lock_req, NULL); 
#else 
     LRPC(GLOBAL_LOCK_NODE, LRPC_ISOMALLOC_GLOBAL_LOCK, STD_PRIO, DEFAULT_STACK, NULL, NULL); 
#endif 
}

#define remote_global_unlock() \
 QUICK_ASYNC_LRPC(GLOBAL_LOCK_NODE, LRPC_ISOMALLOC_GLOBAL_UNLOCK, NULL);

static void *isomalloc_negociate(unsigned int nb_slots)
{
  int i;

 TIMING_EVENT("starting slot_negociate");

#ifdef DEBUG_NEGOCIATION
 tfprintf(stderr,"starting negociation...\n");
#endif

 slot_unlock(); /* modif */

 /*
  global lock
 */

 if (local_node_rank == GLOBAL_LOCK_NODE)
   {
     global_lock();
#ifdef DEBUG_NEGOCIATION
     tfprintf(stderr,"module 0 has taken the global lock\n");
#endif
   }
 else
   remote_global_lock();
 /*
   register as negociation thread
 */ 
 marcel_setspecific(_pm2_isomalloc_nego_key, (any_t) -1);

 /*
   launch negociation threads on all the other nodes
 */
 launch_negociation_on_all_nodes();

 forbid_sends();

 isomalloc_global_sync();

 slot_lock(); /* modif */
 local_lock();

 wait_for_all_slot_maps();

 buy_slots_and_redistribute(nb_slots, &i);

 /*
   Send the updated slot maps to all the other nodes
 */
 send_slot_maps_to_the_nodes();

 local_unlock();

 isomalloc_global_sync(); 

 allow_sends();

 /*
   Unregister as negociation thread
 */ 
 marcel_setspecific(_pm2_isomalloc_nego_key, (any_t) 0);

 /*
  Global unlock
 */
 if (local_node_rank == GLOBAL_LOCK_NODE)
   global_unlock();
 else
   remote_global_unlock();

TIMING_EVENT("ending slot_negociate");
#ifdef DEBUG_NEGOCIATION
 tfprintf(stderr,"negociation ended\n");
#endif

 return GET_SLOT_ADDR(i + nb_slots -1);
}

/*************************************************************************/

#if defined(SOLARIS_SYS) || defined(IRIX_SYS) || defined(FREEBSD_SYS)
static int __zero_fd;
#endif

#define MAX_BUSY_SLOTS     1000

static struct {
  void *addr[MAX_BUSY_SLOTS];
  int *signal[MAX_BUSY_SLOTS];
  unsigned nb;
} busy_slot;

#define MAX_CACHE          1024

typedef struct {
  unsigned last;
  void *addr[MAX_CACHE];
} cache_t;

#define EMPTY_CACHE { 0, }

static cache_t slot_cache = EMPTY_CACHE, migr_cache = EMPTY_CACHE;

static void flush_cache(cache_t *q, char *name)
{
#ifdef DEBUG
  fprintf(stderr, "Flushing %s cache...\n", name);
#endif
  while(q->last) {
    q->last--;
    munmap(q->addr[q->last], SLOT_SIZE);
#ifdef DEBUG
    fprintf(stderr, "munmap(%p), slot index = %d\n",q->addr[q->last] ,SLOT_INDEX(q->addr[q->last]));
#endif
  }
}

#define cache_not_empty(c)    (c.last)

#define cache_not_full(c)     (c.last < MAX_CACHE)

#define add_to_cache(c, ad)   (c.addr[c.last++] = ad)

#define take_from_cache(c)    (c.addr[--c.last])


static int find_in_cache_and_retrieve(cache_t *c, void *addr)
{
  int i = 0;

  for(i=0; i<c->last; i++)
    if(c->addr[i] == addr) {
      c->last--;
      if(i != c->last)
	c->addr[i] = c->addr[c->last];
      return 1;
    }

  return 0;
}

#define init_busy_slots  busy_slot.nb = 0

static int find_busy_slot(void *addr)
{
  int i = 0;

  for(i=0; i<busy_slot.nb; i++)
    if(busy_slot.addr[i] == addr)
      return i;
  return -1;
}

#ifdef ISOMALLOC_USE_MACROS

#define add_busy_slot(address)\
{\
  if(busy_slot.nb == MAX_BUSY_SLOTS)\
    RAISE(CONSTRAINT_ERROR);\
  else {\
    busy_slot.addr[busy_slot.nb] = address;\
    busy_slot.signal[busy_slot.nb++] = NULL;\
  }\
}

#else

static void add_busy_slot(void *addr)
{
#ifdef DEBUG
  fprintf(stderr, "add busy_slot (%d)\n", SLOT_INDEX(addr));
#endif
  if(busy_slot.nb == MAX_BUSY_SLOTS)
    RAISE(CONSTRAINT_ERROR);
  else {
    busy_slot.addr[busy_slot.nb] = addr;
    busy_slot.signal[busy_slot.nb++] = NULL;
  }
}
#endif

#ifdef ISOMALLOC_USE_MACROS

#define remove_busy_slot(slot_number, size)\
{\
  int *sig;\
\
  if((sig = busy_slot.signal[slot_number]) != NULL) {\
    *sig = 1;\
  } else {\
    if(cache_not_full(migr_cache)) {\
      add_to_cache(migr_cache, busy_slot.addr[slot_number]);\
    } else {\
      munmap(busy_slot.addr[slot_number], size);\
    }\
  }\
\
  busy_slot.nb--;\
  if(slot_number != busy_slot.nb) {\
    busy_slot.addr[slot_number] = busy_slot.addr[busy_slot.nb];\
    busy_slot.signal[slot_number] = busy_slot.signal[busy_slot.nb];\
  }\
}

#else

static void remove_busy_slot(int i, size_t size)
{
  int *sig;

#ifdef DEBUG
  fprintf(stderr, "remove busy_slot (%d)\n", i);
#endif
  if((sig = busy_slot.signal[i]) != NULL) {
    /*
      A thread is (already!) waiting for the slot (a ping-pong effect).
      In this case, the slot is not given back to the system.
    */
    *sig = 1;
#ifdef DEBUG
    fprintf(stderr, "Slot %p got without mmap!\n", busy_slot.addr[i]);
#endif
  } else {
    /*
      Soon after a migration, a slot is set free and no one asks for it:
      the slot is then either added to the cache or given back to the system
      (unmapped), if the cache is full.
    */
    if(cache_not_full(migr_cache)) {
      add_to_cache(migr_cache, busy_slot.addr[i]);
    } else {
#ifdef DEBUG
      fprintf(stderr, "remove_busy_slot: munmap(%p), slot index = %d\n",busy_slot.addr[i] ,SLOT_INDEX(busy_slot.addr[i]));
#endif
      munmap(busy_slot.addr[i], size);
    }
  }

  busy_slot.nb--;
  if(i != busy_slot.nb) {
    busy_slot.addr[i] = busy_slot.addr[busy_slot.nb];
    busy_slot.signal[i] = busy_slot.signal[busy_slot.nb];
  }
#ifdef DEBUG
      fprintf(stderr, "end of remove busy_slot: first slot available = %d\n", FIRST_SLOT_AVAILABLE(local_slot_map));
#endif
}
#endif


#ifdef ISOMALLOC_USE_MACROS

#define get_busy_slot(addr, size, sig)\
{\
  int i = find_busy_slot(addr);\
\
  if(i == -1) {\
    if(find_in_cache_and_retrieve(&migr_cache, addr)) {\
    } else {\
      mmap(addr,\
	   size,\
	   PROT_READ | PROT_WRITE,\
	   MMAP_MASK,\
	   FILE_TO_MAP, 0);\
    }\
    *sig = 1;\
  } else {\
    *sig = 0;\
    busy_slot.signal[i] = sig;\
  }\
}

#else

static void get_busy_slot(void *addr, size_t size, int *signal)
{
  int i = find_busy_slot(addr);

  if(i == -1) {
    /*
      The slot is not marked "busy". Either it is in the cache, or it has been
      given back to the system (unmapped).
    */
    if(find_in_cache_and_retrieve(&migr_cache, addr)) {
      /* nothing to do ! */
    } else {
      mmap(addr,
	   size,
	   PROT_READ | PROT_WRITE,
	   MMAP_MASK,
	   FILE_TO_MAP, 0);
#ifdef DEBUG
      fprintf(stderr, "mmap contraint => %p, slot index = %d\n", addr, SLOT_INDEX(addr));
#endif
    }
    *signal = 1;
  } else {
    /*
      The slot is still busy. It will become available as soon as the thread
      which (temporarilly!) possesses the slot executes "isomalloc_free".
    */
    *signal = 0;
    busy_slot.signal[i] = signal;
  }
}

#endif


static void *isomalloc_malloc(size_t size, size_t *granted_size, void *addr)
{
 void *ptr, *add;

  size = (size_t)(ALIGN_TO_SLOT(size));

  if (granted_size != NULL)
    *granted_size = size;

  if(addr == NULL)  /* no address has been specified */ 
    {
     if(size == SLOT_SIZE)  /* Constraint-free single-slot allocation */
       {
	if(cache_not_empty(slot_cache)) 
          {
	    add = take_from_cache(slot_cache);
#ifdef DEBUG_MARCEL
	    fprintf(stderr, "isomalloc_malloc: slot allocated in cache: %p (index = %d)\n", add, SLOT_INDEX(add));
#endif
	  }
	else
	  {
	    add = GET_SLOT_ADDR(GET_FIRST_SLOT_AVAILABLE(local_slot_map));
#ifdef DEBUG_MARCEL
	     fprintf(stderr, "isomalloc_malloc: before mmap: first slot available = %d\n", SLOT_INDEX(add));
#endif
	   }
       }
     else /* Constraint-free multi-slot allocation */
       {
	 int i = GET_FIRST_AVAILABLE_CONTIGUOUS_SLOTS(size/SLOT_SIZE, local_slot_map);

	 if (i == -1) /* The necessary number of slots are not locally available. */
	   {
	     add = isomalloc_negociate(size/SLOT_SIZE);
	   }
	 else
	   add = GET_SLOT_ADDR(i + size/SLOT_SIZE - 1);

       }

/* TIMING_EVENT("before mmap"); */
     ptr = mmap(add,
		size,
		PROT_READ | PROT_WRITE,
		MMAP_MASK,
		FILE_TO_MAP, 0);
/* TIMING_EVENT("after mmap"); */
      if(ptr == (void *)-1) 
	RAISE(STORAGE_ERROR);

#ifdef DEBUG
      fprintf(stderr, "mmap non-contraint => %p, starting slot index = %d, slots = %ld\n", ptr, SLOT_INDEX(ptr), size/SLOT_SIZE);
#endif
#ifdef DEBUG
      fprintf(stderr, "end of isomalloc_malloc: after mmap :first slot available = %d\n", FIRST_SLOT_AVAILABLE(local_slot_map));
#endif
      return ptr;
    }
  else 
    {
    /*
      Here, a particular slot address is specified. This is typically the case
      of an incoming migration.
    */
    int sig;

#ifdef DEBUG
    fprintf(stderr, "isomalloc_malloc with specified address\n");
#endif
    get_busy_slot(addr, size, &sig);
    slot_unlock();

    while(sig == 0) {
#ifdef DEBUG
      fprintf(stderr, "slot is busy\n");
#endif
      marcel_trueyield();
    }
#ifdef DEBUG
    fprintf(stderr, "slot is available !\n");
    fprintf(stderr, "end of isomalloc_malloc: first slot available = %d\n", FIRST_SLOT_AVAILABLE(local_slot_map));
#endif
    slot_lock();
    return addr;
  }
}


static void isomalloc_free(void *addr, size_t size)
{
int i, n_slots, index ;

  if((i = find_busy_slot(addr)) != -1) {
    /*
      If the slot is marked "busy" (i.e. migration in progress),
      then the slot does not get unmapped, 
      so that the next incoming migration could proceed faster.
    */
    remove_busy_slot(i, size);
  } else {
    /*
      If the slot is not marked "busy", the slot is given back to the system
      (a thread is finishing its execution).
    */
    if((size == SLOT_SIZE) && cache_not_full(slot_cache)) {
      add_to_cache(slot_cache, addr);
#ifdef DEBUG_MARCEL
      fprintf(stderr, "isomalloc_free: added to cache: %p (index = %d)\n",addr, SLOT_INDEX(addr));
#endif
    } else {
      index = SLOT_INDEX(addr);
#ifdef DEBUG_MARCEL
      fprintf(stderr, "isomalloc_free: munmap(%p), slot index = %d\n", addr, index);
#endif
      munmap(addr, size);
      n_slots = size/SLOT_SIZE;
      MARK_SLOTS_AVAILABLE(index - n_slots + 1, n_slots, local_slot_map);
#ifdef DEBUG
      fprintf(stderr, "slots marked available: %d-%d\n", index - n_slots + 1, index);
#endif
#ifdef DEBUG
      fprintf(stderr, "end of isomalloc_free: first slot available = %d\n", FIRST_SLOT_AVAILABLE(local_slot_map));
#endif
    }
  }
}


void slot_init(unsigned myrank, unsigned confsize)
{
  int i;

#if defined(SOLARIS_SYS) || defined(IRIX_SYS) || defined(FREEBSD_SYS)
  __zero_fd = open("/dev/zero", O_RDWR);
#ifndef FREEBSD_SYS
  page_size = sysconf(_SC_PAGESIZE);
#endif
#elif defined(UNICOS_SYS)
  page_size = 1024;
#else
  page_size = getpagesize();
#endif

  init_busy_slots;

  local_node_rank = myrank;
  nb_nodes = confsize;

  marcel_mutex_init(&isomalloc_global_mutex, NULL);
  marcel_mutex_init(&isomalloc_local_mutex, NULL);
  marcel_mutex_init(&isomalloc_slot_mutex, NULL);
  marcel_mutex_init(&isomalloc_sync_mutex, NULL);

  tab_sync = (unsigned int *)tmalloc(nb_nodes * sizeof(unsigned int));

  for (i = 0 ; i < nb_nodes ; i++)
    tab_sync[i] = 0;

  marcel_sem_init(&isomalloc_send_status_sem, 0);
  marcel_sem_init(&isomalloc_receive_status_sem, 0);

  global_slot_map = (LRPC_REQ(LRPC_ISOMALLOC_SEND_SLOT_STATUS) *) tmalloc(nb_nodes * sizeof(LRPC_REQ(LRPC_ISOMALLOC_SEND_SLOT_STATUS)));
  local_slot_map = (unsigned int *)global_slot_map[local_node_rank].slot_map;
  global_slot_map[local_node_rank].sender = local_node_rank;
  memset(local_slot_map,0, BITMAP_SIZE*sizeof(unsigned int));
  _internal_isomalloc_config_slot_map();

#ifdef DEBUG
  display_bitmap(0, 100, local_slot_map); 
#endif
}


void slot_slot_busy(void *addr)
{
  /*
    This function is called before a migration, to indicate that the slot
    may be asked for again before its getting liberated, due to a ping-pong
    effect.
  */
  lock_task();

  add_busy_slot(slot_get_header_address(addr));

  unlock_task();
}


void slot_list_busy(slot_descr_t *descr)
{
  slot_header_t *slot_ptr;

  if (descr == NULL)
    return;

  lock_task();

  slot_ptr = slot_get_first(descr);
  while (slot_ptr != NULL)
    {
      add_busy_slot(slot_ptr);
      slot_ptr = slot_get_next(slot_ptr);
    }

  unlock_task();
}


void slot_exit()
{
  flush_cache(&slot_cache, "slot");
  flush_cache(&migr_cache, "migration");
  tfree(global_slot_map);
  tfree(tab_sync);
#ifdef DEBUG
  fprintf(stderr, "Isomalloc exited\n");
#endif
}

void slot_print_header(slot_header_t *ptr)
{
  if (ptr == NULL)return;
  fprintf(stderr,"\nslot header:\n");
  assert(ptr->magic_number == SLOT_MAGIC_NUM);
  fprintf(stderr, "  start address = %p  available size = %d  magic number = %x\n  previous slot address = %p  next slot address = %p\n", ptr, ptr->size, ptr->magic_number, ptr->prev, ptr->next);
}

void slot_print_list(slot_descr_t *descr)
{
  slot_header_t *ptr;

  if (descr == NULL) return;
  ptr = descr->slots;
  fprintf(stderr,"\nHere is the slot list:\n");
  while (ptr != NULL)
    { 
     slot_print_header(ptr);
     ptr = ptr->next;
    }
  fprintf(stderr,"last slot: %p\n", descr->last_slot);
}

void slot_init_list(slot_descr_t *descr)
{
  descr->slots = NULL;
  descr->last_slot = NULL;
#ifdef DEBUG
  descr->magic_number = SLOT_LIST_MAGIC_NUM;
  fprintf(stderr,"slot_list initialized: list address = %p\n", descr);
#endif
}


static void *slot_alloc(slot_descr_t *descr, size_t req_size, size_t *granted_size, void *addr)
{
  size_t overall_size;
  slot_header_t *header_ptr;

#ifdef DEBUG
  fprintf(stderr,"slot_alloc is starting, req_size = %d\n", req_size);
#endif
  /*
    allocate a slot 
  */

  TIMING_EVENT("slot_alloc is starting: isomalloc..."); 

  header_ptr = (slot_header_t *)isomalloc_malloc((req_size + SLOT_HEADER_SIZE), &overall_size, (void *)(addr ? (char *)addr - SLOT_HEADER_SIZE : NULL));
  /* 
     fill in the header 
  */
  header_ptr->size = overall_size - SLOT_HEADER_SIZE;
  if (granted_size != NULL) *granted_size = header_ptr->size;
  header_ptr->magic_number = SLOT_MAGIC_NUM;
  /* 
     chain the slot if requested 
  */
  if(descr != NULL) {
#ifdef DEBUG
    assert(descr->magic_number == SLOT_LIST_MAGIC_NUM);
#endif
    header_ptr->next = descr->slots; 
    if(descr->slots != NULL)
      descr->slots->prev = header_ptr;
    else{
#ifdef DEBUG
      fprintf(stderr,"first slot chained\n");
#endif
      descr->last_slot = header_ptr;
    }

    /*
      update the list head 
    */
    descr->slots = header_ptr;

#ifdef DEBUG
/*  
    fprintf(stderr,"Here is the new slot:\n");
    slot_print_header(descr->slots);
*/
#endif
  }
  else
    header_ptr->next = NULL;
  header_ptr->prev = NULL;
  /*
    return a pointer to the available zone 
  */

  TIMING_EVENT("slot_alloc is ending");

#ifdef DEBUG
  fprintf(stderr,"slot_alloc is ending\n");
#endif

  return (void *)((char *)header_ptr + SLOT_HEADER_SIZE);
}

void *slot_general_alloc(slot_descr_t *descr, size_t req_size, size_t *granted_size, void *addr)
{
  void *ptr;

#ifdef DEBUG
  fprintf(stderr,"slot_general_alloc is starting, req_size = %d, task = %p\n", req_size, marcel_self());
#endif
 slot_lock();
 ptr =  slot_alloc(descr, req_size, granted_size, addr);
 slot_unlock();
 return ptr;
}

void slot_free(slot_descr_t *descr, void * addr)
{
  slot_header_t * header_ptr; 

#ifdef DEBUG
  fprintf(stderr,"slot_free is starting\n");
#endif
  slot_lock(); /* a supprimer et a mettre dans slot_general_alloc */
  /* 
     get the address of the slot header 
  */
  header_ptr = (slot_header_t *)((char *)addr - SLOT_HEADER_SIZE);
#ifdef DEBUG
  assert(header_ptr->magic_number == SLOT_MAGIC_NUM);
/*fprintf(stderr,"\nslot to set free:\n");
  slot_print_header(header_ptr);*/
#endif

  /* 
     suppress the slot from the list if necessary 
  */
  if(descr != NULL){

    if (header_ptr->prev != NULL)
      header_ptr->prev->next = header_ptr->next;
    else 
      /* 
	 the slot to suppress is the head of the list 
      */
      descr->slots = header_ptr->next;
    if (header_ptr->next != NULL)
      header_ptr->next->prev = header_ptr->prev;
    else
      { 
      /*
	the slot to suppress is the tail of the list 
      */
      if (header_ptr->prev != NULL)
	/* 
	   the slot to suppress is not the only one in the list 
	*/
	header_ptr->prev->next = NULL;

      /*
	update the address of the last slot in the list
      */
      descr->last_slot = header_ptr->prev;
      }
 }
  /* 
     set the slot free 
  */
  isomalloc_free(header_ptr, slot_get_overall_size(header_ptr));

#ifdef DEBUG
  fprintf(stderr,"slot_free is ending\n");
  /*  
      slot_print_list(descr);
  */
#endif
 slot_unlock();

}

void slot_flush_list(slot_descr_t *descr)
{
  slot_header_t *slot_ptr, *current_slot;

#ifdef DEBUG
  fprintf(stderr, "slot_flush_list is starting...\n");
#endif

 if (descr == NULL) return; 
 /* 
    nothing to do: the slot list is empty 
  */
#ifdef DEBUG
 assert(descr->magic_number == SLOT_LIST_MAGIC_NUM);
#endif
 slot_lock();

 slot_ptr = descr->slots;

  while (slot_ptr != NULL) 
    /* 
       for every slot in the list 
    */
    {
#ifdef DEBUG
      assert(slot_ptr->magic_number == SLOT_MAGIC_NUM);
#endif
      current_slot = slot_ptr;
      slot_ptr = slot_ptr->next;
      isomalloc_free(current_slot, slot_get_overall_size(current_slot));
    }
  descr->slots = NULL;
#ifdef DEBUG
  fprintf(stderr, "slot_flush_list is ending...\n");
#endif
  slot_unlock();
}

#ifndef ISOMALLOC_USE_MACROS

slot_header_t *slot_get_first(slot_descr_t *descr)
{
  if (descr != NULL)
    {
#ifdef DEBUG
      assert(descr->magic_number == SLOT_LIST_MAGIC_NUM);
#endif
      return descr->slots;
    }
  else
    return (slot_header_t *)NULL;
}


slot_header_t *slot_get_next(slot_header_t *slot_ptr)
{
  if (slot_ptr != NULL)
    {
#ifdef DEBUG
      assert(slot_ptr->magic_number == SLOT_MAGIC_NUM);
#endif
      return slot_ptr->next;
    }
  else
    return (slot_header_t *)NULL;
}


slot_header_t *slot_get_prev(slot_header_t *slot_ptr)
{
  if (slot_ptr != NULL)
    {
#ifdef DEBUG
      assert(slot_ptr->magic_number == SLOT_MAGIC_NUM);
#endif
      return slot_ptr->prev;
    }
  else
    return (slot_header_t *)NULL;
}


void * slot_get_usable_address(slot_header_t *slot_ptr)
{
  if (slot_ptr != NULL)
    {
      return (void *)((char *)slot_ptr + SLOT_HEADER_SIZE);
    }
  else
    return (void *)NULL;
}


slot_header_t *slot_get_header_address(void *usable_address)
{
  if (usable_address != NULL)
    {
      slot_header_t * slot_ptr = (slot_header_t *)((char *)usable_address - SLOT_HEADER_SIZE);
      return slot_ptr;
    }
  else
    return (slot_header_t *)NULL;
}


size_t slot_get_usable_size(slot_header_t *slot_ptr)
{
  if (slot_ptr != NULL)
    {
#ifdef DEBUG
      assert(slot_ptr->magic_number == SLOT_MAGIC_NUM);
#endif
      return slot_ptr->size;
    }
  else
    return -1;
}


size_t slot_get_overall_size(slot_header_t *slot_ptr)
{
  if (slot_ptr != NULL)
    {
#ifdef DEBUG
      assert(slot_ptr->magic_number == SLOT_MAGIC_NUM);
#endif
      return slot_ptr->size + SLOT_HEADER_SIZE;
    }
  else
    return -1;
}


size_t slot_get_header_size()
{
  return SLOT_HEADER_SIZE;
}

#endif

void slot_pack_all(slot_descr_t *descr)
{
  slot_header_t *slot_ptr = slot_get_first(descr);

   /* 
      pack the address of the first slot 
   */
   mad_pack_byte(MAD_IN_HEADER, (char *)&slot_ptr, sizeof(slot_header_t *));
   /* 
      pack the slot list 
   */
   while (slot_ptr != NULL)
     {
       /* 
	  pack the header 
       */
       mad_pack_byte(MAD_IN_HEADER, (char*)slot_ptr, sizeof(slot_header_t));
       /*
	 pack the remaining slot data 
       */
       mad_pack_byte(MAD_BY_COPY, (char*)slot_ptr + SLOT_HEADER_SIZE, slot_get_usable_size(slot_ptr));

       slot_ptr = slot_get_next(slot_ptr);
     }
}


void slot_unpack_all()
{
  slot_header_t *slot_ptr;
  slot_header_t slot_header;
  void *usable_add;

   /* 
      unpack the address of the first slot 
   */
   mad_unpack_byte(MAD_IN_HEADER, (char *)&slot_ptr, sizeof(slot_header_t *));
#ifdef DEBUG
   tfprintf(stderr,"the address of the first slot is %p\n", slot_ptr);
#endif

   /* 
      unpack the slots 
   */
   while(slot_ptr != NULL)
     {
       /* 
	  unpack the slot header 
       */
       mad_unpack_byte(MAD_IN_HEADER, (char *)&slot_header, sizeof(slot_header_t));
       /* 
	  allocate memory for the slot 
       */
       usable_add = slot_alloc(NULL, slot_get_usable_size(&slot_header), NULL, slot_get_usable_address(slot_ptr));
       /* 
	  copy the header 
       */
       *slot_ptr = slot_header;
       /* 
	  unpack the remaining slot data
       */
       mad_unpack_byte(MAD_BY_COPY, (char*)slot_ptr + SLOT_HEADER_SIZE, slot_get_usable_size(slot_ptr));

       slot_ptr = slot_ptr->next;
     }
}

/*********************** multislot allocation services ******************************/


BEGIN_SERVICE(LRPC_ISOMALLOC_GLOBAL_LOCK)
#ifdef DEBUG_NEGOCIATION
     fprintf(stderr,"GLOBAL_LOCK on module %d is starting (master = %d)...\n",local_node_rank, req.master);
#endif
     global_lock();

#ifdef DEBUG_NEGOCIATION
     fprintf(stderr,"GLOBAL_LOCK on module %d is ending.\n",local_node_rank);
#endif
END_SERVICE(LRPC_ISOMALLOC_GLOBAL_LOCK)


BEGIN_SERVICE(LRPC_ISOMALLOC_GLOBAL_UNLOCK)
#ifdef DEBUG_NEGOCIATION
     fprintf(stderr,"GLOBAL_UNLOCK on module %d is starting...\n",local_node_rank);
#endif
     global_unlock();
#ifdef DEBUG_NEGOCIATION
     fprintf(stderr,"GLOBAL_UNLOCK on module %d is ending.\n",local_node_rank);
#endif
END_SERVICE(LRPC_ISOMALLOC_GLOBAL_UNLOCK)


BEGIN_SERVICE(LRPC_ISOMALLOC_LOCAL_LOCK)
#ifdef DEBUG_NEGOCIATION
     fprintf(stderr,"LOCAL_LOCK on module %d is starting (master = %d)...\n",local_node_rank, req.master);
#endif
 /*
   register as negociation thread
 */
     marcel_setspecific(_pm2_isomalloc_nego_key, (any_t) -1);

     forbid_sends();

     isomalloc_global_sync(); 

     local_lock();

     QUICK_ASYNC_LRPC(req.master, LRPC_ISOMALLOC_SEND_SLOT_STATUS, &global_slot_map[local_node_rank]);

#ifdef DEBUG_NEGOCIATION
     fprintf(stderr,"LOCAL_LOCK: waiting for the updated slot map... \n");
#endif

     marcel_sem_P(&isomalloc_send_status_sem);

#ifdef DEBUG_NEGOCIATION
     fprintf(stderr,"LOCAL_LOCK: got the slot status back \n");
#endif

     local_unlock(); 

     isomalloc_global_sync();

     allow_sends();

#ifdef DEBUG_NEGOCIATION
     fprintf(stderr,"LOCAL_LOCK on module %d is ending.\n",local_node_rank);
#endif
END_SERVICE(LRPC_ISOMALLOC_LOCAL_LOCK)


BEGIN_SERVICE(LRPC_ISOMALLOC_LOCAL_UNLOCK)
#ifdef DEBUG_NEGOCIATION
     fprintf(stderr,"LOCAL_UNLOCK on module %d is starting...\n",local_node_rank);
#endif
     memcpy(global_slot_map[local_node_rank].slot_map, &req, sizeof(req));
     marcel_sem_V(&isomalloc_send_status_sem);
#ifdef DEBUG_NEGOCIATION
     fprintf(stderr,"LOCAL_UNLOCK on module %d is ending.\n",local_node_rank);
#endif
END_SERVICE(LRPC_ISOMALLOC_LOCAL_UNLOCK)

BEGIN_SERVICE(LRPC_ISOMALLOC_SYNC)
#ifdef DEBUG_NEGOCIATION
int i;
     fprintf(stderr,"SYNC on module %d...\n",local_node_rank);
#endif
     sync_lock(); /*Useless: normally there are no concurrent writes...*/
     tab_sync[req.sender]++;
#ifdef DEBUG_NEGOCIATION
     fprintf(stderr,"t[%d] = %d\n", req.sender, tab_sync[req.sender]);
     for (i=0 ; i < nb_nodes ; i++)
       fprintf(stderr,"--->t[%d] = %d\n", i, tab_sync[i]);
#endif
     sync_unlock();
END_SERVICE(LRPC_ISOMALLOC_SYNC)

BEGIN_SERVICE(LRPC_ISOMALLOC_SEND_SLOT_STATUS)
#ifdef DEBUG_NEGOCIATION
     fprintf(stderr,"SLOT INDEX ARRIVED AT one %d from node %d\n",local_node_rank, req.sender);
#endif
     memcpy(&global_slot_map[req.sender], &req, sizeof(req));
     marcel_sem_V(&isomalloc_receive_status_sem);
END_SERVICE(LRPC_ISOMALLOC_SEND_SLOT_STATUS)


int received_from_all_other_nodes ()
{
int i = 0;

while(( i < nb_nodes) && (tab_sync[i] >= 1)) i++;
#ifdef DEBUG_NEGOCIATION
if (i>=nb_nodes)
  fprintf(stderr,"received:ok\n");
#endif
return (i == nb_nodes);

}

static void isomalloc_global_sync()
{
 int i;
 LRPC_REQ(LRPC_ISOMALLOC_SYNC) me;
 me.sender = local_node_rank;

 tab_sync[local_node_rank]++;
 /* 
    send messages to all nodes
 */
 for (i=0 ; i < nb_nodes ; i++)
   if (i!=local_node_rank)
    QUICK_ASYNC_LRPC(i, LRPC_ISOMALLOC_SYNC, &me);

 /* 
    wait for messages from all nodes
 */
#ifdef DEBUG_NEGOCIATION
 fprintf(stderr,"sync ??\n");
#endif
 while (!received_from_all_other_nodes())
   marcel_trueyield();
#ifdef DEBUG_NEGOCIATION
 for (i=0 ; i < nb_nodes ; i++)
   fprintf(stderr,"sync ok: t[%d] = %d\n", i, tab_sync[i]);
#endif
 sync_lock();
 for (i=0 ; i < nb_nodes ; i++)
   tab_sync[i]--;
#ifdef DEBUG_NEGOCIATION
 for (i=0 ; i < nb_nodes ; i++)
   fprintf(stderr,"sync ended: t[%d] = %d\n", i, tab_sync[i]);
#endif
 sync_unlock();
}
