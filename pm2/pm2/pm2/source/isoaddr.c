
/* Options: ISOADDR_ALOC_TRACE, ISOADDR_NEGOCIATION_TRACE, ISOADDR_INFO_TRACE, ASSERT */

//#define ISOADDR_ALOC_TRACE
//#define ISOADDR_NEGOCIATION_TRACE
//#define ISOADDR_INFO_TRACE
//#define ASSERT

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>
#include <malloc.h>
#include <stdarg.h>

#include "marcel.h"
#include "sys/archdep.h"
#include "madeleine.h"
#include "pm2.h"
#include "sys/isomalloc_archdep.h"
#include "sys/bitmap.h"
#include "magic.h"
#include "dsm_page_size.h"
#include "isoaddr.h"
#include "slot_alloc.h"


#define _SLOT_SIZE       DSM_PAGE_SIZE /* 4 Ko */
#define ALIGN_TO_SLOT(x) (ALIGN(x, _SLOT_SIZE))

static unsigned _local_node_rank = 0;
static node_t  _nb_nodes = 1;


BEGIN_LRPC_LIST
  LRPC_ISOMALLOC_GLOBAL_LOCK,
  LRPC_ISOMALLOC_GLOBAL_UNLOCK,
  LRPC_ISOMALLOC_LOCAL_LOCK,
  LRPC_ISOMALLOC_LOCAL_UNLOCK,
  LRPC_ISOMALLOC_SYNC,
  LRPC_ISOMALLOC_SEND_SLOT_STATUS,
  ISOADDR_LRPC_INFO_REQ,
  ISOADDR_LRPC_INFO
END_LRPC_LIST

#define SLOT_INDEX(addr) \
        ((int)(((int)ISOADDR_AREA_TOP  - ((int)addr & ~(_SLOT_SIZE - 1))) / _SLOT_SIZE) - 1) // big bug out 5/05/00 !

#define GET_SLOT_ADDR(bit_abs_index) \
        ((void *)((int)ISOADDR_AREA_TOP  - (bit_abs_index + 1) * _SLOT_SIZE))

int isoaddr_page_index(void *addr)
{
  return SLOT_INDEX(addr);
}


int isoaddr_addr(void *addr)
{
#if 0
  fprintf(stderr,"isoaddr(%p)? -> %d (top = %p, bottom = %p, dyn = %ld)\n", addr,((int)addr < ISOADDR_AREA_TOP && (int)addr > (ISOADDR_AREA_TOP - DYN_DSM_AREA_SIZE)), ISOADDR_AREA_TOP,  (ISOADDR_AREA_TOP - DYN_DSM_AREA_SIZE), DYN_DSM_AREA_SIZE); 
#endif
  return (int)(((char *)addr < (char *)ISOADDR_AREA_TOP) && ((char *)addr > ((char *)ISOADDR_AREA_TOP - (int)DYN_DSM_AREA_SIZE)));
//return (unsigned long)addr > (unsigned long)((long int)ISOADDR_AREA_TOP - (long int)DYN_DSM_AREA_SIZE);
}


void *isoaddr_page_addr(int index)
{
  return GET_SLOT_ADDR(index);
}

/*********************************************************************/
/* Page info structures and associated info */


typedef struct _page_info_entry
{
  node_t           owner;
  int              status;
  int              pending;
  int              master;
  marcel_mutex_t   mutex;
  marcel_cond_t    cond;
} page_info_entry_t;


typedef page_info_entry_t *page_info_table_t;

static page_info_table_t _info_table;
static int _page_distrib_mode = CYCLIC;
static int _page_distrib_arg = 16;

#ifdef min
#undef min
#endif /* min */

#define min(a,b) ((a) < (b))?(a):(b)

static void _page_ownership_init()
{
  int i;

  LOG_IN();

  switch(_page_distrib_mode){
  case CENTRALIZED :
    {
      for (i = 0; i < ISOADDR_PAGES; i++)
	  _info_table[i].owner = (node_t)_page_distrib_arg;
      break;    
    }
  
  case CYCLIC :
    {
      /* Here, _page_distrib_arg stores the chunk size: the number of
	 contiguous pages to assign to the same node */
      int bound, j, curOwner = 0;
      
      i = 0;
      while(i < ISOADDR_PAGES)
	{
	  bound = min(_page_distrib_arg, ISOADDR_PAGES - i);
	  for (j = 0; j < bound; j++)
	    {
	      _info_table[i].owner = curOwner;
	      i++;
	    }
	  curOwner = (curOwner + 1) % _nb_nodes;
	}
      break;
    }
  case BLOCK :
    {
      int chunk = ISOADDR_PAGES / _nb_nodes;
      int split = ISOADDR_PAGES % _nb_nodes;
      int curOwner = 0;
      int curCount = 0;

      if (split == 0)
	{
	  split = _nb_nodes;
	  chunk -= 1;
	}
      for (i = 0; i < ISOADDR_PAGES; i++)
	{
	  _info_table[i].owner = (node_t)curOwner;
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
      break;
    }
  case CUSTOM :
    {
      int *array = (int *)_page_distrib_arg;
      for (i = 0; i < ISOADDR_PAGES; i++)
	  _info_table[i].owner = array[i];
      break;
    }
  }

 LOG_OUT();
}


static void _isoaddr_page_info_init()
{
  int i;

  LOG_IN();

#ifdef ISOADDR_INFO_TRACE
  fprintf(stderr,"Alloc page table(%d)\n",sizeof(page_info_entry_t) * ISOADDR_PAGES);
#endif
  _info_table = (page_info_table_t)tmalloc(sizeof(page_info_entry_t) * ISOADDR_PAGES);
  if (!_info_table)
    RAISE(CONSTRAINT_ERROR);
  
  for(i = 0; i < ISOADDR_PAGES ; i++)
    {
      _info_table[i].status = ISO_UNUSED;
      _info_table[i].pending = 0;
      _info_table[i].master = i;
      marcel_mutex_init(&_info_table[i].mutex, NULL);
      marcel_cond_init(&_info_table[i].cond, NULL);
    }
  _page_ownership_init();

  LOG_OUT();
}


void isoaddr_page_set_owner(int index, node_t owner)
{
#ifdef ISOADDR_INFO_TRACE
  fprintf(stderr,"  Set owner (%d) -> %d\n", index, owner);
#endif
  _info_table[index].owner = owner;
}


node_t isoaddr_page_get_owner(int index)
{
  return _info_table[index].owner;
}


void isoaddr_page_set_master(int index, int master)
{
#ifdef ISOADDR_INFO_TRACE
    fprintf(stderr,"  Set master (%d) -> %d\n", index, master);
#endif
  _info_table[index].master = master;
}


int isoaddr_page_get_master(int index)
{
  return _info_table[index].master;
}


void isoaddr_page_set_status(int index, int status)
{
#ifdef ISOADDR_INFO_TRACE
  fprintf(stderr,"  Set status (%d) -> %d\n", index, status);
#endif
  _info_table[index].status = status;
}


static __inline__ void _isoaddr_page_set_pending_req(int index)
{
  _info_table[index].pending = 1;
}


static __inline__ void _isoaddr_page_remove_pending_req(int index)
{
  _info_table[index].pending = 0;
}



int _isoaddr_page_get_pending_req(int index)
{
  return _info_table[index].pending;
}


static __inline__ void _isoaddr_page_info_wait(int index)
{
  marcel_cond_wait(&_info_table[index].cond, &_info_table[index].mutex);
}


static __inline__ void _isoaddr_page_info_ready(int index)
{
  marcel_cond_broadcast(&_info_table[index].cond);
}


void isoaddr_page_info_lock(int index)
{
  marcel_mutex_lock(&_info_table[index].mutex);
}


void isoaddr_page_info_unlock(int index)
{
  marcel_mutex_unlock(&_info_table[index].mutex);
}


void isoaddr_page_set_distribution(int mode, ...)
{
  va_list l;
  
  _page_distrib_mode = mode;
  va_start(l, mode);
  switch(mode) {
  case CENTRALIZED: 
  case CYCLIC: _page_distrib_arg = va_arg(l, int); break;
  case CUSTOM: _page_distrib_arg = (int)va_arg(l, int *); break;
  case BLOCK: break;
  }
  va_end(l);
}


void isoaddr_page_display_info()
{
  int i;

  for (i = 0 ; i < ISOADDR_PAGES ; i++)
    tfprintf(stderr,"Page %d: owner = %d, status = %d, pending = %d\n", i, _info_table[i].owner, _info_table[i].status, _info_table[i].pending);
}

#ifdef DSM
/******* a few comm functionalities for page info ********/


static void _isoaddr_send_info_req(node_t dest_node, int index, node_t req_node)
{

  LOG_IN();

#ifdef ASSERT
  assert(dest_node != _local_node_rank);
#endif
#ifdef ISOADDR_INFO_TRACE
  tfprintf(stderr, "isoaddr_send_info_req: begin packing %ld!\n", index);
#endif
  pm2_rawrpc_begin((int)dest_node, ISOADDR_LRPC_INFO_REQ, NULL);
  pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&index, sizeof(int));
  pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&req_node, sizeof(node_t));
  pm2_rawrpc_end();
#ifdef ISOADDR_INFO_TRACE
  tfprintf(stderr, "isoaddr_send_info_req: sent (%ld)!\n", index);
#endif

  LOG_OUT();
}


static void _isoaddr_send_info(node_t dest_node, int index)
{
  int prot = 0, master = isoaddr_page_get_master(index), nb_pages = 1, k, shared;
  node_t dsm_owner;

  LOG_IN();

  //  if (_info_table[index].status == ISO_PRIVATE)
  if (_info_table[master].status == ISO_DEFAULT)
    slot_set_shared(GET_SLOT_ADDR(index));
#ifdef ISOADDR_INFO_TRACE
  fprintf(stderr, "isoaddr_send_info: index = %d, status = %d!\n", index, _info_table[master].status);
#endif

  if ((shared = (_info_table[master].status == ISO_SHARED)))
    {
      int i = dsm_isoaddr_page_index(index);
      prot = dsm_get_page_protocol(i);
      dsm_owner = dsm_get_prob_owner(i);
    }

  pm2_rawrpc_begin((int)dest_node, ISOADDR_LRPC_INFO, NULL);
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&index, sizeof(int));
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&master, sizeof(int));

  k = master;
  while (isoaddr_page_get_master(--k) == master) 
    nb_pages++;

  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&nb_pages, sizeof(int));
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&_info_table[index].owner, sizeof(node_t));
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&_info_table[master].status, sizeof(int)); 
  if (shared)
    {
      pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)&prot, sizeof(int));
      pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)&dsm_owner, sizeof(int));
    }
  pm2_rawrpc_end();

#ifdef ISOADDR_INFO_TRACE
  if (shared)
    fprintf(stderr, "Page info (%d) sent -> %d: status = %d, prot = %d, master = %d, nb_pages = %d, dsm_owner = %d\n", index, dest_node, _info_table[master].status, prot, master, nb_pages, dsm_owner);
  else
    fprintf(stderr, "Page info (%d) sent -> %d: status = %d, master = %d, nb_pages = %d\n", index, dest_node, _info_table[master].status, master, nb_pages); 
#endif

  LOG_OUT();
}


static void _ISOADDR_INFO_REQ_threaded_func(void)
{
  node_t req_node, owner;
  int index;

  LOG_IN();

  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&index, sizeof(int));
  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&req_node, sizeof(node_t));
  pm2_rawrpc_waitdata(); 
#ifdef ISOADDR_INFO_TRACE
  fprintf(stderr, "Page info req(%d) received : status = %d, owner = %d\n", index, _info_table[_info_table[index].master].status, _info_table[_info_table[index].master].owner);
#endif
  isoaddr_page_info_lock(index); // Useful ?

  owner = isoaddr_page_get_owner(_info_table[index].master);
  if (owner == _local_node_rank)
    _isoaddr_send_info(req_node, index);
  else
    _isoaddr_send_info_req(owner, index, req_node);

  isoaddr_page_info_unlock(index); // Useful ?

  LOG_OUT();
}


void ISOADDR_INFO_REQ_func(void)
{
  pm2_thread_create((pm2_func_t) _ISOADDR_INFO_REQ_threaded_func, NULL);
}


static void _ISOADDR_INFO_threaded_func(void)
{
  int index, master, prot, nb_pages, owner, status, i;
  dsm_node_t dsm_owner;

  LOG_IN();

  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&index, sizeof(int));

  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&master, sizeof(int));
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&nb_pages, sizeof(int));
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&owner, sizeof(node_t));
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&status, sizeof(int)); 

  isoaddr_page_info_lock(master);
#ifdef ISOADDR_INFO_TRACE
  isoaddr_page_set_owner(master, owner);
  isoaddr_page_set_status(master, status);
  isoaddr_page_set_master(master, master);
#else
  _info_table[master].owner = owner;
  _info_table[master].status = status;
  _info_table[master].master = master;
#endif
  isoaddr_page_info_unlock(master);

  for (i = master - 1; i > master - nb_pages; i--)
    {
      isoaddr_page_info_lock(i);
#ifdef ISOADDR_INFO_TRACE
      isoaddr_page_set_owner(i, owner);
      isoaddr_page_set_master(i, master);
#else
      _info_table[i].owner = owner;
      _info_table[i].master = master;
#endif
      isoaddr_page_info_unlock(i);
    }

  if (status == ISO_SHARED)
    {
      pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)&prot, sizeof(int)); 
      pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)&dsm_owner, sizeof(int));
    }
  pm2_rawrpc_waitdata(); 

#ifdef ISOADDR_INFO_TRACE
  if (status == ISO_SHARED)
    fprintf(stderr, "Page info (%d) received : master status = %d, owner = %d, prot = %d, master = %d, nb_pages = %d\n", index, _info_table[master].status, _info_table[index].owner, prot, master, nb_pages);
  else
    fprintf(stderr, "Page info (%d) received : master status = %d, owner = %d, master = %d, nb_pages = %d\n", index, _info_table[master].status, _info_table[index].owner, master, nb_pages);
#endif

  isoaddr_page_info_lock(index);
  
  if (status == ISO_SHARED)
    dsm_enable_page_entry(dsm_isoaddr_page_index(index), dsm_owner, prot, isoaddr_page_addr(master), nb_pages*DSM_PAGE_SIZE, TRUE);

  _isoaddr_page_remove_pending_req(index);

#ifdef ISOADDR_INFO_TRACE
  fprintf(stderr, "Page info (%d) recorded, threads can wake\n", index);
#endif

  _isoaddr_page_info_ready(index);

  isoaddr_page_info_unlock(index);

  LOG_OUT();
}


void ISOADDR_INFO_func(void)
{
  pm2_thread_create((pm2_func_t) _ISOADDR_INFO_threaded_func, NULL);
}
#endif //DSM 


int isoaddr_page_get_status(int index)
{
#ifdef DSM 
  int master = _info_table[index].master;
#endif

  LOG_IN();

#ifdef DSM

#ifdef ISOADDR_INFO_TRACE
  fprintf(stderr, "Entering get_status (%d), master = %d, status = %d, owner = %d\n", index, master, _info_table[master].status, _info_table[master].owner );
#endif

  if (_info_table[master].owner != _local_node_rank &&  _info_table[master].status != ISO_SHARED)
     {
       isoaddr_page_info_lock(index);
       
       if (!_isoaddr_page_get_pending_req(index))
	 {
	   _isoaddr_page_set_pending_req(index);
	   _isoaddr_send_info_req(_info_table[index].owner, index, _local_node_rank);
#ifdef ISOADDR_INFO_TRACE
	   fprintf(stderr, "Sent info request (%d) \n", index);
#endif
	 }
       _isoaddr_page_info_wait(index);
#ifdef ISOADDR_INFO_TRACE
       fprintf(stderr, "Got status ! (%d) master = %d, status = %d\n", index, _info_table[index].master, _info_table[_info_table[index].master].status );
#endif       
       isoaddr_page_info_unlock(index);
     }
#ifdef ISOADDR_INFO_TRACE
       fprintf(stderr, "get_status (%d) returns %d\n", index, _info_table[_info_table[index].master].status);
#endif
#endif// DSM

  LOG_OUT();

  return _info_table[_info_table[index].master].status;
}

/********************************  slot map configuration ****************************/

#define MARK_SLOTS_AVAILABLE(start, n, bitmap) set_bits_to_1(start, n, bitmap); 

#define MARK_SLOTS_BUSY(start, n, bitmap) reset_bits_to_0(start, n, bitmap); 

#define FIRST_SLOT_AVAILABLE(bitmap) first_bit_to_1(bitmap)

#define GET_FIRST_SLOT_AVAILABLE(bitmap) get_first_bit_to_1(bitmap)

#define FIRST_AVAILABLE_CONTIGUOUS_SLOTS(n, bitmap) first_bits_to_1(n, bitmap)

#define FIRST_AVAILABLE_ALIGNED_CONTIGUOUS_SLOTS(n, bitmap, align) first_bits_to_1_aligned(n, bitmap, align)

#define GET_FIRST_AVAILABLE_CONTIGUOUS_SLOTS(n, bitmap) get_first_bits_to_1(n, bitmap)

#define GET_FIRST_AVAILABLE_ALIGNED_CONTIGUOUS_SLOTS(n, bitmap, align) get_first_bits_to_1_aligned(n, bitmap, align)

static unsigned int cycle_start = 0;
unsigned int *local_slot_map;

static __inline__ void _pm2_set_uniform_slot_distribution(unsigned int nb_contiguous_slots, int nb_cycles)
{
  set_cyclic_sequences(cycle_start + _local_node_rank * nb_contiguous_slots, nb_contiguous_slots, nb_contiguous_slots * _nb_nodes, nb_cycles, local_slot_map);
 
  /* update current bit pointer */
  cycle_start += nb_contiguous_slots * _nb_nodes * nb_cycles;
}


static void _pm2_set_non_uniform_slot_distribution(int node, unsigned int offset, unsigned int nb_contiguous_slots, unsigned int period, int nb_cycles) __attribute__ ((unused));
static void _pm2_set_non_uniform_slot_distribution(int node, unsigned int offset, unsigned int nb_contiguous_slots, unsigned int period, int nb_cycles)
{
if (node == _local_node_rank)
  {
    set_cyclic_sequences(cycle_start + offset, nb_contiguous_slots, period, nb_cycles, local_slot_map);
 
  /* update current bit pointer */
    cycle_start += period * nb_cycles;
  }
}


#define begin_slot_distribution() cycle_start = 0

#define end_slot_distribution() _pm2_set_uniform_slot_distribution(16, -1)


static void _internal_isoaddr_config_slot_map()
{
  bitmap_set_size(ISOADDR_PAGES);

  begin_slot_distribution();
  switch (_page_distrib_mode) {
  case CENTRALIZED: 
    {
      if (_local_node_rank == _page_distrib_arg)
	MARK_SLOTS_AVAILABLE(0, ISOADDR_PAGES, local_slot_map);
      break;
    }
  case CYCLIC: _pm2_set_uniform_slot_distribution(_page_distrib_arg, -1); break;
  case CUSTOM: RAISE(NOT_IMPLEMENTED); break;
  case BLOCK: _pm2_set_uniform_slot_distribution(ISOADDR_PAGES/_nb_nodes, 1); break;
  }
//  end_slot_distribution();
}


/******************************  negociation *********************************/

static marcel_mutex_t isomalloc_global_mutex, isomalloc_slot_mutex, isomalloc_local_mutex, isomalloc_sync_mutex;

static int *tab_sync ;

static marcel_sem_t isomalloc_send_status_sem, isomalloc_receive_status_sem ;

extern marcel_sem_t sem_nego;

static unsigned int *aux_slot_map;

static unsigned int **global_slot_map;

//static pm2_completion_t c;

#define global_lock() /* fprintf(stderr, "GLOBAL_LOCK\n"); */ marcel_mutex_lock(&isomalloc_global_mutex)

#define global_unlock() /* fprintf(stderr, "GLOBAL_UNLOCK\n");*/  marcel_mutex_unlock(&isomalloc_global_mutex)

#define local_lock()  /* fprintf(stderr, "LOCAL_LOCK\n");*/  marcel_mutex_lock(&isomalloc_local_mutex)

#define local_unlock() /* fprintf(stderr, "LOCAL_UNLOCK\n"); */marcel_mutex_unlock(&isomalloc_local_mutex)

#define sync_lock() marcel_mutex_lock(&isomalloc_sync_mutex)

#define sync_unlock() marcel_mutex_unlock(&isomalloc_sync_mutex)

#define slot_lock()  /* fprintf(stderr, "SLOT_LOCK\n");  */ marcel_mutex_lock(&isomalloc_slot_mutex)

#define slot_unlock() /* fprintf(stderr, "SLOT_UNLOCK\n"); */ marcel_mutex_unlock(&isomalloc_slot_mutex)

static void _isomalloc_global_sync();



#define _forbid_sends() fprintf(stderr,"forbid sends\n"); marcel_sem_P(&sem_nego)

#define _allow_sends() fprintf(stderr,"allow sends\n"); marcel_sem_V(&sem_nego)

#define GLOBAL_LOCK_NODE 0

#define _wait_for_all_slot_maps() \
 { \
   int j;\
   for (j = 1; j < _nb_nodes; j++) \
     marcel_sem_P(&isomalloc_receive_status_sem);\
 }

static __inline__ void  _buy_slots_and_redistribute(unsigned int n, unsigned int *rank, unsigned int align) 
{ 
  int j, k; 
  unsigned int i, *p1, *p2, *p; 
 /*
   global OR on the slot maps
 */
 p1 = local_slot_map; 
 p2 = (unsigned int *)((_local_node_rank == 0) ? global_slot_map[1] : global_slot_map[0]); 
 p = (unsigned int *)aux_slot_map; 
 OR_bitmaps_1(p, p1, p2);

 for (k = 1; k < _nb_nodes ; k++) 
   if (k != _local_node_rank) 
     { 
       p1 = (unsigned int *)global_slot_map[k]; 
       p = (unsigned int *)aux_slot_map; 
       OR_bitmaps_2(p, p1);
     } 

 /* search for the contiguous slots in the global slot_map */
 i = FIRST_AVAILABLE_ALIGNED_CONTIGUOUS_SLOTS(n, aux_slot_map, align); 
 if (i == -1) 
   RAISE(STORAGE_ERROR); 

 /* mark these slots as busy on all nodes */
 for (j = 0; j < _nb_nodes; j++) 
     MARK_SLOTS_BUSY(i, n, global_slot_map[j]); 


#ifdef ISOADDR_NEGOCIATION_TRACE
 tfprintf(stderr,"slot_negociate: start index =%d, end_index = %d\n",i, i + n -1); 
#endif 
 *rank = i;
}


void _send_slot_maps_to_the_nodes(int i, int n, isoaddr_attr_t *attr)
{ 
  int j; 
  for (j = 0; j < _nb_nodes ; j++) 
    { 
      if (j != _local_node_rank)
       {
         pm2_rawrpc_begin(j, LRPC_ISOMALLOC_LOCAL_UNLOCK, NULL);
         pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&_local_node_rank, sizeof(int));
         pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&i, sizeof(int));
         pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&n, sizeof(int));
         pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&attr->status, sizeof(int));
         pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&attr->atomic, sizeof(int));
         if (attr->status == ISO_SHARED)
	   pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&attr->protocol, sizeof(int));
	 pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)global_slot_map[j], bitmap_get_size()/8);
#ifdef ISOADDR_NEGOCIATION_TRACE 
	 fprintf(stderr, "LOCAL UNLOCK packed :sender = %d, start = %d, nb = %d, status = %d, prot = %d\n", _local_node_rank, i, n, attr->status, attr->protocol);
#endif
	 pm2_rawrpc_end();
	}
    }
}


static __inline__ void _launch_negociation_on_all_nodes() 
{ 
 int j; 

 for (j= 0; j < _nb_nodes; j++) 
   if (j != _local_node_rank) 
     { 
#ifdef ISOADDR_NEGOCIATION_TRACE 
       tfprintf(stderr,"Launching local lock on module %d \n", j); 
#endif 
       pm2_rawrpc_begin(j, LRPC_ISOMALLOC_LOCAL_LOCK, NULL);
       pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&_local_node_rank, sizeof(unsigned int));
       pm2_rawrpc_end();
      } 
}

static void _remote_global_lock() 
{ 
  pm2_completion_t c;

  pm2_completion_init(&c, NULL, NULL);

#ifdef ISOADDR_NEGOCIATION_TRACE 
  pm2_rawrpc_begin(GLOBAL_LOCK_NODE, LRPC_ISOMALLOC_GLOBAL_LOCK, NULL);
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&_local_node_rank, sizeof(unsigned int));
  pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER,&c);
  pm2_rawrpc_end();
  pm2_completion_wait(&c);
#else 
  pm2_rawrpc_begin(GLOBAL_LOCK_NODE, LRPC_ISOMALLOC_GLOBAL_LOCK, NULL);
  pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER,&c);
  pm2_rawrpc_end();
  pm2_completion_wait(&c);
#endif 
}


#define remote_global_unlock() \
   pm2_rawrpc_begin(GLOBAL_LOCK_NODE, LRPC_ISOMALLOC_GLOBAL_UNLOCK, NULL);\
   pm2_rawrpc_end()


static void *_isomalloc_negociate(unsigned int nb_slots, isoaddr_attr_t *attr)
{
  int i;

#ifdef ISOADDR_NEGOCIATION_TRACE
 tfprintf(stderr,"Starting negociation...(I am %p)\n", marcel_self());
#endif

 // local_unlock(); // au lieu de slot_unlock(); /* modif */

 /*
  global lock
 */

 if (_local_node_rank == GLOBAL_LOCK_NODE)
   {
     global_lock();
#ifdef ISOADDR_NEGOCIATION_TRACE
     tfprintf(stderr,"Module %d has taken the GLOBAL LOCK\n", _local_node_rank);
#endif
   }
 else
   _remote_global_lock();
 /*
   register as negociation thread
 */ 
 marcel_setspecific(_pm2_isomalloc_nego_key, (any_t) -1);
#ifdef ISOADDR_NEGOCIATION_TRACE
     tfprintf(stderr,"!!!!!! Got global lock! I am now the negociation thread: %p\n",marcel_self() );
#endif

 /*
   launch negociation threads on all the other nodes
 */
 _launch_negociation_on_all_nodes();

 _forbid_sends();

 _isomalloc_global_sync();

 // slot_lock() 6/04/00
 local_lock();

 _wait_for_all_slot_maps();

 _buy_slots_and_redistribute(nb_slots, &i, attr->align);

 /*
   Send the updated slot maps to all the other nodes
 */
 _send_slot_maps_to_the_nodes(i, nb_slots, attr);

 local_unlock();

 _isomalloc_global_sync(); 

 // _allow_sends();

 /*
   Unregister as negociation thread
 */ 
 marcel_setspecific(_pm2_isomalloc_nego_key, (any_t) 0);
#ifdef ISOADDR_NEGOCIATION_TRACE
     tfprintf(stderr,"!!!!!! I am no longer the negociation thread: %p\n",marcel_self() );
#endif
     _allow_sends();
 /*
  Global unlock
 */
 if (_local_node_rank == GLOBAL_LOCK_NODE)
   {
     global_unlock();
   }
 else
   {
     remote_global_unlock();
   }

#ifdef ISOADDR_NEGOCIATION_TRACE
 tfprintf(stderr,"Global unlock: Negociation ended\n");
#endif

 return GET_SLOT_ADDR(i + nb_slots -1);
}

/*************************************************************************/

#if defined(SOLARIS_SYS) || defined(IRIX_SYS) || defined(FREEBSD_SYS)
int __zero_fd;
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

static cache_t slot_cache = EMPTY_CACHE, migr_cache = EMPTY_CACHE, stack_cache =  EMPTY_CACHE;

static __inline__ void flush_cache(cache_t *q, size_t size, char *name)
{
#ifdef ISOADDR_ALOC_TRACE
  fprintf(stderr, "Flushing %s cache...\n", name);
#endif
  while(q->last) {
    q->last--;
    munmap(q->addr[q->last], size);
#ifdef ISOADDR_ALOC_TRACE
    fprintf(stderr, "munmap(%p), slot index = %d\n",q->addr[q->last] ,SLOT_INDEX(q->addr[q->last]));
#endif
  }
}

#define cache_not_empty(c)    (c.last)

#define cache_not_full(c)     (c.last < MAX_CACHE)

#define add_to_cache(c, ad)   (c.addr[c.last++] = ad)

#define take_from_cache(c)    (c.addr[--c.last])


static __inline__ int _find_in_cache_and_retrieve(cache_t *c, void *addr)
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

static __inline__ int _find_busy_slot(void *addr)
{
  int i = 0;

  for(i=0; i<busy_slot.nb; i++)
    if(busy_slot.addr[i] == addr)
      return i;
  return -1;
}


void isoaddr_add_busy_slot(void *addr)
{
#ifdef ISOADDR_ALOC_TRACE
  fprintf(stderr, "add busy_slot (%d)\n", SLOT_INDEX(addr));
#endif
  if(busy_slot.nb == MAX_BUSY_SLOTS)
    RAISE(CONSTRAINT_ERROR);
  else {
    busy_slot.addr[busy_slot.nb] = addr;
    busy_slot.signal[busy_slot.nb++] = NULL;
  }
}


#ifdef ISOMALLOC_USE_MACROS

static __inline__ void _remove_busy_slot(slot_number, size)
{
  int *sig;

  if((sig = busy_slot.signal[slot_number]) != NULL) {
    *sig = 1;
  } else {
    if(cache_not_full(migr_cache)) {
      add_to_cache(migr_cache, busy_slot.addr[slot_number]);
    } else {
      munmap(busy_slot.addr[slot_number], size);
    }
  }

  busy_slot.nb--;
  if(slot_number != busy_slot.nb) {
    busy_slot.addr[slot_number] = busy_slot.addr[busy_slot.nb];
    busy_slot.signal[slot_number] = busy_slot.signal[busy_slot.nb];
  }
}

#else

static __inline__ void _remove_busy_slot(int i, size_t size)
{
  int *sig;

#ifdef ISOADDR_ALOC_TRACE
  fprintf(stderr, "remove busy_slot (%d)\n", i);
#endif
  if((sig = busy_slot.signal[i]) != NULL) {
    /*
      A thread is (already!) waiting for the slot (a ping-pong effect).
      In this case, the slot is not given back to the system.
    */
    *sig = 1;
#ifdef ISOADDR_ALOC_TRACE
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
#ifdef ISOADDR_ALOC_TRACE
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
#ifdef ISOADDR_ALOC_TRACE
      fprintf(stderr, "end of remove busy_slot: first slot available = %d\n", FIRST_SLOT_AVAILABLE(local_slot_map));
#endif
}
#endif



static __inline__ void _get_busy_slot(void *addr, size_t size, int *signal)
{
  int i = _find_busy_slot(addr);

  if(i == -1) {
    /*
      The slot is not marked "busy". Either it is in the cache, or it has been
      given back to the system (unmapped).
    */
    if(_find_in_cache_and_retrieve(&migr_cache, addr)) {
      /* nothing to do ! */
    } else {
      mmap(addr,
	   size,
	   PROT_READ | PROT_WRITE,
	   MMAP_MASK,
	   FILE_TO_MAP, 0);
#ifdef ISOADDR_ALOC_TRACE
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


void *isoaddr_malloc(size_t size, size_t *granted_size, void *addr, isoaddr_attr_t *attr)
{
 void *ptr;
 int i, master;

  LOG_IN();

  local_lock();//6/04/00

  size = (size_t)((size == THREAD_STACK_SIZE)?THREAD_SLOT_SIZE:(ALIGN_TO_SLOT(size)));
#ifdef ISOADDR_ALOC_TRACE
   fprintf(stderr, "isoaddr_malloc(%d)\n", size);
#endif

  if (granted_size != NULL)
    *granted_size = size;

  if(addr == NULL)  /* no address has been specified */ 
    {
     if(size == _SLOT_SIZE)  /* Constraint-free single-slot allocation */
       {
	if(cache_not_empty(slot_cache)) 
          {
	    ptr = take_from_cache(slot_cache);
#ifdef ISOADDR_ALOC_TRACE
	    fprintf(stderr, "isoaddr_malloc: slot allocated in cache: %p (index = %d)\n", ptr, SLOT_INDEX(ptr));
#endif
	  }
	else
	  {
	    ptr = GET_SLOT_ADDR(GET_FIRST_SLOT_AVAILABLE(local_slot_map));
#ifdef ISOADDR_ALOC_TRACE
	     fprintf(stderr, "isoaddr_malloc: before mmap: first slot available = %d\n", SLOT_INDEX(ptr));
#endif
	     ptr = mmap(ptr,
			size,
			PROT_READ | PROT_WRITE,
			MMAP_MASK,
			FILE_TO_MAP, 0);
	     if(ptr == (void *)-1) 
	       RAISE(STORAGE_ERROR);

	   }
	//	isoaddr_page_set_status(SLOT_INDEX(ptr), status);
       }
     else /* Constraint-free multi-slot allocation */
       {
	 if((size == THREAD_SLOT_SIZE) && (cache_not_empty(stack_cache))) 
	   {
	     ptr = take_from_cache(stack_cache);
#ifdef ISOADDR_ALOC_TRACE
	     fprintf(stderr, "isoaddr_malloc: stack allocated in cache: %p (slot index = %d)\n", ptr, SLOT_INDEX(ptr));
#endif
	   }
	 else
	   {
	     i = GET_FIRST_AVAILABLE_ALIGNED_CONTIGUOUS_SLOTS(size/_SLOT_SIZE, local_slot_map, (size == THREAD_SLOT_SIZE)?THREAD_SLOT_SIZE/_SLOT_SIZE:ALIGN_TO_SLOT(attr->align)/_SLOT_SIZE);
	     
	     if (i == -1) /* The necessary number of slots are not locally available. */
	       {
#ifdef ISOADDR_ALOC_TRACE
		 fprintf(stderr, "isoaddr_malloc: need to negociate %d slots\n", size/_SLOT_SIZE);
#endif
		 if (size == THREAD_SLOT_SIZE)
		   attr->align = THREAD_SLOT_SIZE/_SLOT_SIZE;
		 
		 local_unlock();
		 ptr = _isomalloc_negociate(size/_SLOT_SIZE, attr);
		 local_lock();
	       }
	     else
	       {
#ifdef ISOADDR_ALOC_TRACE
		 fprintf(stderr, "isoaddr_malloc: got %d slots locally\n", size/_SLOT_SIZE);
#endif
		 ptr = GET_SLOT_ADDR(i + size/_SLOT_SIZE - 1);
	       }
	     ptr = mmap(ptr,
			size,
			PROT_READ | PROT_WRITE,
			MMAP_MASK,
			FILE_TO_MAP, 0);
	     if(ptr == (void *)-1) 
	       RAISE(STORAGE_ERROR);

	   }
       }
     
#ifdef ISOADDR_ALOC_TRACE
      fprintf(stderr, "mmap non-contraint => %p, starting slot index = %d, slots = %d\n", ptr, SLOT_INDEX(ptr), size/_SLOT_SIZE);
      fprintf(stderr, "end of isoaddr_malloc: after mmap :first slot available = %d\n", FIRST_SLOT_AVAILABLE(local_slot_map));
#endif
      master = SLOT_INDEX(ptr);
      isoaddr_page_info_lock(master);
#ifdef ISOADDR_INFO_TRACE
      isoaddr_page_set_master(master, master);
      isoaddr_page_set_status(master, attr->status);
      isoaddr_page_set_owner(master, _local_node_rank);
#else
      _info_table[master].master = master;
      _info_table[master].status = attr->status;
      _info_table[master].owner = _local_node_rank;
#endif
      isoaddr_page_info_unlock(master);

      if (attr->atomic || attr->status != ISO_SHARED)
	{
	  for (i = 1; i < size/_SLOT_SIZE; i++)
	    {
	      isoaddr_page_info_lock(master - i);
#ifdef ISOADDR_INFO_TRACE
	      isoaddr_page_set_master(master - i, master);
#else
	      _info_table[master - i].master = master;
#endif
	      
	      isoaddr_page_info_unlock(master - i);
	    }
	}
      else
	{
	  for (i = 1; i < size/_SLOT_SIZE; i++)
	    {
	      isoaddr_page_info_lock(master - i);
#ifdef ISOADDR_INFO_TRACE     
	      isoaddr_page_set_status(master - i, attr->status);
	      isoaddr_page_set_master(master - i, master - i);
	      isoaddr_page_set_owner(master - i, _local_node_rank);
#else
	      _info_table[master - i].status = attr->status;
	      _info_table[master - i].master = master - i;
	      _info_table[master - i].owner = _local_node_rank;
#endif
	      isoaddr_page_info_unlock(master - i);
	    }
	}
	  
      local_unlock();

      LOG_OUT();

      return ptr;
    }
  else 
    {
    /*
      Here, a particular slot address is specified. This is typically the case
      of an incoming migration.
    */
    int sig;

#ifdef ISOADDR_ALOC_TRACE
    fprintf(stderr, "isoaddr_malloc with specified address\n");
#endif
    _get_busy_slot(addr, size, &sig);
    local_unlock();//au lieu de slot_unlock 6/04/00

    while(sig == 0) {
#ifdef ISOADDR_ALOC_TRACE
      fprintf(stderr, "slot is busy\n");
#endif
      marcel_trueyield();
    }
#ifdef ISOADDR_ALOC_TRACE
    fprintf(stderr, "slot is available !\n");
#endif
    master = SLOT_INDEX(addr);
    isoaddr_page_info_lock(master);
#ifdef ISOADDR_INFO_TRACE
    isoaddr_page_set_master(master, master);
    isoaddr_page_set_status(master, attr->status);
#else
    _info_table[master].master = master;
    _info_table[master].status = attr->status;
#endif
    isoaddr_page_info_unlock(master);

    if (attr->atomic|| attr->status != ISO_SHARED)
      {
	for (i = 1; i < size/_SLOT_SIZE; i++)
	  {
	    isoaddr_page_info_lock(master - i);
#ifdef ISOADDR_INFO_TRACE    
	    isoaddr_page_set_master(master - i, master);
#else
	    _info_table[master - i].master = master;
#endif
	    
	    isoaddr_page_info_unlock(master - i);
	  }
      }
    else
      {
	for (i = 1; i < size/_SLOT_SIZE; i++)
	  {
	    isoaddr_page_info_lock(master - i);
#ifdef ISOADDR_INFO_TRACE   
	    isoaddr_page_set_status(master - i, attr->status);
	    isoaddr_page_set_master(master - i, master - i);
#else
	    _info_table[master - i].status = attr->status;
	    _info_table[master - i].master = master - i;
#endif
	    
	    isoaddr_page_info_unlock(master - i);
	  }
      }
#ifdef ISOADDR_ALOC_TRACE
    fprintf(stderr, "end of isoaddr_malloc: first slot available = %d\n", FIRST_SLOT_AVAILABLE(local_slot_map));
#endif

    LOG_OUT();

    return addr;
    }
}


void isoaddr_free(void *addr, size_t size)
{
  int i, n_slots, index ;
 
  LOG_IN();

  local_lock();//6/04/00

  if((i = _find_busy_slot(addr)) != -1) {
    /*
      If the slot is marked "busy" (i.e. migration in progress),
      then the slot does not get unmapped, 
      so that the next incoming migration could proceed faster.
    */
    _remove_busy_slot(i, size);
  } else {
    /*
      If the slot is not marked "busy", the slot is given back to the system
      (a thread is finishing its execution).
    */
    if((size == _SLOT_SIZE) && cache_not_full(slot_cache)) {
      add_to_cache(slot_cache, addr);
#ifdef ISOADDR_ALOC_TRACE
      fprintf(stderr, "isoaddr_free: added to cache: %p (index = %d)\n",addr, SLOT_INDEX(addr));
#endif
    } 
    else 
      {
	if ((size == THREAD_SLOT_SIZE) && cache_not_full(stack_cache)){
	  
	  add_to_cache(stack_cache, addr);
	  
#ifdef ISOADDR_ALOC_TRACE
	  fprintf(stderr, "isoaddr_free: added to stack cache: %p (index = %d)\n",addr, SLOT_INDEX(addr));
#endif
	} else {

      index = SLOT_INDEX(addr);
#ifdef ISOADDR_ALOC_TRACE
      fprintf(stderr, "isoaddr_free: munmap(%p), slot index = %d\n", addr, index);
#endif
      munmap(addr, size);
      n_slots = size/_SLOT_SIZE;
      MARK_SLOTS_AVAILABLE(index - n_slots + 1, n_slots, local_slot_map);
#ifdef ISOADDR_ALOC_TRACE
      fprintf(stderr, "slots marked available: %d-%d\n", index - n_slots + 1, index);
#endif
#ifdef ISOADDR_ALOC_TRACE
      fprintf(stderr, "end of isoaddr_free: first slot available = %d\n", FIRST_SLOT_AVAILABLE(local_slot_map));
#endif
	}
      }
  }

  /*  for (i = 0; i < size/SLOT_SIZE; i++)
      isoaddr_page_set_status(SLOT_INDEX(addr) - i, ISO_AVAILABLE);*/
  local_unlock();//6/04/00

  LOG_OUT();
}


void isoaddr_init(unsigned myrank, unsigned confsize)
{
  int i;

  LOG_IN();

#if defined(SOLARIS_SYS) || defined(IRIX_SYS) || defined(FREEBSD_SYS)
  __zero_fd = open("/dev/zero", O_RDWR);
#endif

  init_busy_slots;

  _local_node_rank = myrank;
  _nb_nodes = confsize;

  marcel_mutex_init(&isomalloc_global_mutex, NULL);
  marcel_mutex_init(&isomalloc_local_mutex, NULL);
  marcel_mutex_init(&isomalloc_slot_mutex, NULL);
  marcel_mutex_init(&isomalloc_sync_mutex, NULL);

  tab_sync = (unsigned int *)tmalloc(_nb_nodes * sizeof(unsigned int));

  for (i = 0 ; i < _nb_nodes ; i++)
    tab_sync[i] = 0;

  marcel_sem_init(&isomalloc_send_status_sem, 0);
  marcel_sem_init(&isomalloc_receive_status_sem, 0);

  global_slot_map = (unsigned int **) tmalloc(_nb_nodes * sizeof(unsigned int *));
  for (i = 0 ; i < _nb_nodes ; i++)
    global_slot_map[i] = (unsigned int *)tmalloc(ISOADDR_PAGES/8);
  local_slot_map = global_slot_map[_local_node_rank];

  aux_slot_map= (unsigned int *)tmalloc(ISOADDR_PAGES/8);
  memset(local_slot_map,0, ISOADDR_PAGES/8);
  _internal_isoaddr_config_slot_map();
  _isoaddr_page_info_init();

#ifdef ISOADDR_INFO_TRACE
  display_bitmap(0, 100, local_slot_map); 
  //  isoaddr_page_display_info();
#endif
 
  LOG_OUT();
}


void isoaddr_exit()
{
  int i;

  LOG_IN();

  flush_cache(&slot_cache, _SLOT_SIZE, "slot");
  flush_cache(&slot_cache, THREAD_SLOT_SIZE, "stack");
  flush_cache(&migr_cache, _SLOT_SIZE, "migration");
  for (i = 0 ; i < _nb_nodes ; i++)
    tfree(global_slot_map[i]);
  tfree(global_slot_map);
  tfree(aux_slot_map);
  tfree(tab_sync);
  tfree(_info_table);
#ifdef ISOADDR_INFO_TRACE
  fprintf(stderr, "Isoaddr exited\n");
#endif

  LOG_OUT();
}


/*********************** multislot allocation services ******************************/


void LRPC_ISOMALLOC_GLOBAL_LOCK_threaded_func(void)
{
  pm2_completion_t c;

#ifdef ISOADDR_NEGOCIATION_TRACE
  int master;
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&master, sizeof(unsigned int));
#endif
  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER,&c);
  pm2_rawrpc_waitdata();

#ifdef ISOADDR_NEGOCIATION_TRACE
  fprintf(stderr,"GLOBAL_LOCK on module %d is starting(c.master = %d, c.ptr = %p, I am %p)...\n",_local_node_rank, c.proc, c.c_ptr, marcel_self());
#endif
  global_lock();
#ifdef ISOADDR_NEGOCIATION_TRACE
  fprintf(stderr,"GLOBAL_LOCK on module %d :signal completion (&c = %p)\n",_local_node_rank, &c);
  fprintf(stderr,"GLOBAL_LOCK on module %d :signal completion (c.master = %d, c.ptr = %p, I am %p)...\n",_local_node_rank, c.proc, c.c_ptr, marcel_self());
#endif
  pm2_completion_signal(&c);
#ifdef ISOADDR_NEGOCIATION_TRACE
  fprintf(stderr,"GLOBAL_LOCK on module %d is ending (master = %d, I am %p)...\n",_local_node_rank, master, marcel_self());
#endif
}


void LRPC_ISOMALLOC_GLOBAL_LOCK_func(void)
{
  pm2_thread_create((pm2_func_t) LRPC_ISOMALLOC_GLOBAL_LOCK_threaded_func, NULL);
}


void LRPC_ISOMALLOC_GLOBAL_UNLOCK_func(void)
{
#ifdef ISOADDR_NEGOCIATION_TRACE
  fprintf(stderr,"GLOBAL_UNLOCK on module %d is starting...(I am %p)...\n",_local_node_rank, marcel_self());
#endif
  pm2_rawrpc_waitdata();
  global_unlock();
#ifdef ISOADDR_NEGOCIATION_TRACE
  fprintf(stderr,"GLOBAL_UNLOCK on module %d is ending.\n",_local_node_rank);
#endif
}


void LRPC_ISOMALLOC_LOCAL_LOCK_threaded_func(void)
{
  int master;

  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&master, sizeof(unsigned int));
  pm2_rawrpc_waitdata();
#ifdef ISOADDR_NEGOCIATION_TRACE
     fprintf(stderr,"---->>>LOCAL_LOCK on module %d is starting (master = %d, I am %p)...\n",_local_node_rank, master, marcel_self());
#endif
 /*
   register as negociation thread
 */
     marcel_setspecific(_pm2_isomalloc_nego_key, (any_t) -1);

     _forbid_sends();

     _isomalloc_global_sync(); 

     local_lock();
#ifdef ISOADDR_NEGOCIATION_TRACE
     fprintf(stderr,"LOCAL_LOCK: sending slot map... \n");
#endif
     pm2_rawrpc_begin(master, LRPC_ISOMALLOC_SEND_SLOT_STATUS, NULL);
     pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&_local_node_rank, sizeof(unsigned int));
     pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)(global_slot_map[_local_node_rank]), bitmap_get_size()/8);
     pm2_rawrpc_end();

#ifdef ISOADDR_NEGOCIATION_TRACE
     fprintf(stderr,"LOCAL_LOCK: waiting for the updated slot map... \n");
#endif

     marcel_sem_P(&isomalloc_send_status_sem);

#ifdef ISOADDR_NEGOCIATION_TRACE
     fprintf(stderr,"LOCAL_LOCK: got the slot status back \n");
#endif

     local_unlock(); 

     _isomalloc_global_sync();

     _allow_sends();

#ifdef ISOADDR_NEGOCIATION_TRACE
     fprintf(stderr,"----<<<<LOCAL_LOCK on module %d is ending.\n",_local_node_rank);
#endif
}


void LRPC_ISOMALLOC_LOCAL_LOCK_func(void)
{
  pm2_thread_create((pm2_func_t) LRPC_ISOMALLOC_LOCAL_LOCK_threaded_func, NULL);
}


void LRPC_ISOMALLOC_LOCAL_UNLOCK_func(void)
{
  int sender, i, start_index, nb_slots, status, prot, atomic;
  
#ifdef ISOADDR_NEGOCIATION_TRACE
  fprintf(stderr,"LOCAL_UNLOCK on module %d is starting...\n",_local_node_rank);
#endif
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&sender, sizeof(int));
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&start_index, sizeof(int));
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&nb_slots, sizeof(int));
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&status, sizeof(int));
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&atomic, sizeof(int));
  if(status == ISO_SHARED)
    pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&prot, sizeof(int)); 
#ifdef ISOADDR_NEGOCIATION_TRACE
  fprintf(stderr, "LOCAL UNLOCK unpacked :sender = %d, start = %d, nb = %d, status = %d, prot = %d, atomic = %d\n", sender, start_index, nb_slots, status, prot, atomic);
#endif
  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)global_slot_map[_local_node_rank], bitmap_get_size()/8);
  pm2_rawrpc_waitdata();
  
#ifdef ISOADDR_NEGOCIATION_TRACE
  fprintf(stderr, "LOCAL UNLOCK bitmap unpacked \n");
#endif
  marcel_sem_V(&isomalloc_send_status_sem);

 if (atomic)  
   {
	 isoaddr_page_info_lock(start_index);
#ifdef ISOADDR_INFO_TRACE   
	 isoaddr_page_set_owner(start_index + nb_slots - 1, sender);
	 isoaddr_page_set_status(start_index + nb_slots - 1, status);
	 isoaddr_page_set_master(start_index + nb_slots - 1, start_index + nb_slots - 1);
#else
	 _info_table[start_index].owner = sender; 
	 _info_table[start_index].master = start_index + nb_slots - 1;
	 _info_table[start_index].status = status; 
#endif

	 isoaddr_page_info_unlock(start_index);

     for (i = 0 ; i < nb_slots - 1; i++)
       {
	 isoaddr_page_info_lock(start_index + i);
#ifdef ISOADDR_INFO_TRACE 
	 isoaddr_page_set_owner(start_index + i, sender);
	 isoaddr_page_set_master(start_index + i, start_index + nb_slots - 1);
#else
	 _info_table[start_index + i].owner = sender; 
	 _info_table[start_index + i].master = start_index + nb_slots - 1;
#endif
	 isoaddr_page_info_unlock(start_index + i);
       }
#ifdef DSM
     if(status == ISO_SHARED)
       dsm_enable_page_entry(dsm_isoaddr_page_index(start_index), sender, prot, isoaddr_page_addr(start_index + nb_slots - 1), DSM_PAGE_SIZE * nb_slots, TRUE);
#endif
   }
 else
   {
    for (i = 0 ; i < nb_slots; i++)
       {
	 isoaddr_page_info_lock(start_index + i);
#ifdef ISOADDR_INFO_TRACE 
	 isoaddr_page_set_owner(start_index + i, sender);
	 isoaddr_page_set_status(start_index + i, status);
	 isoaddr_page_set_master(start_index + i, start_index + i);
#else
	 _info_table[start_index + i].owner = sender; 
	 _info_table[start_index + i].master = start_index + i;
	 _info_table[start_index + i].status = status; 
#endif
	 isoaddr_page_info_unlock(start_index + i);
#ifdef DSM
	 if(status == ISO_SHARED)
	   dsm_enable_page_entry(dsm_isoaddr_page_index(start_index + i), sender, prot, isoaddr_page_addr(start_index + i), DSM_PAGE_SIZE, TRUE);
#endif
       }
   }
    

#ifdef ISOADDR_NEGOCIATION_TRACE
  fprintf(stderr,"LOCAL_UNLOCK on module %d is ending.\n",_local_node_rank);
#endif
}


void LRPC_ISOMALLOC_SEND_SLOT_STATUS_func(void)
{
     int sender;

     pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&sender, sizeof(int));
     pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)(global_slot_map[sender]), bitmap_get_size()/8);
     pm2_rawrpc_waitdata();

#ifdef ISOADDR_NEGOCIATION_TRACE
     fprintf(stderr,"SLOT INDEX ARRIVED AT one %d from node %d\n",_local_node_rank, sender);
#endif
     marcel_sem_V(&isomalloc_receive_status_sem);
}


void LRPC_ISOMALLOC_SYNC_func(void)
{
  int sender;
#ifdef ISOADDR_NEGOCIATION_TRACE
  int i;
#endif

  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&sender, sizeof(unsigned int));
  pm2_rawrpc_waitdata();
#ifdef ISOADDR_NEGOCIATION_TRACE
  fprintf(stderr,"SYNC func on module %d...\n",_local_node_rank);
#endif
  sync_lock(); /*Useless: normally there are no concurrent writes...*/
  tab_sync[sender]++;
#ifdef ISOADDR_NEGOCIATION_TRACE
  fprintf(stderr,"SYNC func: t[%d] = %d\n", sender, tab_sync[sender]);
  for (i=0 ; i < _nb_nodes ; i++)
    fprintf(stderr,"SYNC func: --->t[%d] = %d\n", i, tab_sync[i]);
#endif
  sync_unlock();
}


int received_from_all_other_nodes ()
{
int i = 0;

while(( i < _nb_nodes) && (tab_sync[i] >= 1)) i++;
#ifdef ISOADDR_NEGOCIATION_TRACE
if (i>=_nb_nodes)
  fprintf(stderr,"Received all acks for sync\n");
#endif
return (i == _nb_nodes);

}


static void _isomalloc_global_sync()
{
 int i;

 tab_sync[_local_node_rank]++;
 /* 
    send messages to all nodes
 */
 for (i=0 ; i < _nb_nodes ; i++)
   if (i!=_local_node_rank)
     {
       pm2_rawrpc_begin(i, LRPC_ISOMALLOC_SYNC, NULL);
       pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&_local_node_rank, sizeof(unsigned int));
       pm2_rawrpc_end();
     }
 /* 
    wait for messages from all nodes
 */
#ifdef ISOADDR_NEGOCIATION_TRACE
 fprintf(stderr,"Starting sync...\n");
#endif
 while (!received_from_all_other_nodes())
   marcel_trueyield();
#ifdef ISOADDR_NEGOCIATION_TRACE
 for (i=0 ; i < _nb_nodes ; i++)
   fprintf(stderr,"Sync ok: t[%d] = %d\n", i, tab_sync[i]);
#endif
 sync_lock();
 for (i=0 ; i < _nb_nodes ; i++)
   tab_sync[i]--;
#ifdef ISOADDR_NEGOCIATION_TRACE
 for (i=0 ; i < _nb_nodes ; i++)
   fprintf(stderr,"Sync ended: t[%d] = %d\n", i, tab_sync[i]);
#endif
 sync_unlock();
}


void isoaddr_init_rpc()
{
#ifdef DSM
  pm2_rawrpc_register(&ISOADDR_LRPC_INFO_REQ, ISOADDR_INFO_REQ_func);
  pm2_rawrpc_register(&ISOADDR_LRPC_INFO, ISOADDR_INFO_func);
#endif
  pm2_rawrpc_register(&LRPC_ISOMALLOC_GLOBAL_LOCK, LRPC_ISOMALLOC_GLOBAL_LOCK_func);
  pm2_rawrpc_register(&LRPC_ISOMALLOC_GLOBAL_UNLOCK, LRPC_ISOMALLOC_GLOBAL_UNLOCK_func);
  pm2_rawrpc_register(&LRPC_ISOMALLOC_LOCAL_LOCK, LRPC_ISOMALLOC_LOCAL_LOCK_func);
  pm2_rawrpc_register(&LRPC_ISOMALLOC_LOCAL_UNLOCK, LRPC_ISOMALLOC_LOCAL_UNLOCK_func);
  pm2_rawrpc_register(&LRPC_ISOMALLOC_SYNC, LRPC_ISOMALLOC_SYNC_func);
  pm2_rawrpc_register(&LRPC_ISOMALLOC_SEND_SLOT_STATUS, LRPC_ISOMALLOC_SEND_SLOT_STATUS_func);
}
