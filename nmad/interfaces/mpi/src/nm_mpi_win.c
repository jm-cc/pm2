/*
 * NewMadeleine
 * Copyright (C) 2016 (see AUTHORS file)
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

#include "nm_mpi_private.h"
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include <Padico/Module.h>
PADICO_MODULE_HOOK(MadMPI);

#define NM_MPI_WIN_ABS(a) ((a)>0?(a):(-(a)))
#define NM_MPI_WIN_ERROR(win_id, errcode) (MPI_Win_call_errhandler(win_id, errcode), errcode)

NM_MPI_HANDLE_TYPE(window, nm_mpi_window_t, _NM_MPI_WIN_OFFSET, 4);
static struct nm_mpi_handle_window_s nm_mpi_windows;

/* ** Memory segment list elements definition ************** */

/** Content for requests queue elements */
PUK_LIST_TYPE(nm_mpi_win_addr,
	      void*begin;
	      MPI_Aint size;
	      );

typedef struct
{
  struct nm_mpi_win_addr_list_s head;
  nm_mpi_spinlock_t lock;
} nm_mpi_win_addrlist_t;

/** Slab allocator for addr list elements */
#define NM_MPI_WIN_INITIAL_ADDR_LELT 8
PUK_ALLOCATOR_TYPE(nm_mpi_win_addr, struct nm_mpi_win_addr_s);
static nm_mpi_win_addr_allocator_t nm_mpi_win_addr_allocator;

/* ** Windows initialization types definition ************** */

typedef struct nm_mpi_win_init_data_s
{
  MPI_Aint size;
  int unit;
  int win_id;
} nm_mpi_win_init_data_t;
static nm_mpi_datatype_t*p_init_datatype;
static MPI_Datatype init_datatype;

/* ** Static function prototypes *************************** */

static inline nm_mpi_window_t*nm_mpi_window_alloc(int comm_size);
static inline void     nm_mpi_window_destroy(nm_mpi_window_t*p_win);
static inline void     nm_mpi_win_exchange_init(nm_mpi_window_t*p_win, int size);
static inline int      nm_mpi_win_exchange_init_shm(nm_mpi_window_t*p_win, int size);
static inline int      nm_mpi_win_flush(nm_mpi_window_t*p_win, int target_rank);
static inline int      nm_mpi_win_flush_all(nm_mpi_window_t*p_win);
static inline int      nm_mpi_win_flush_local(nm_mpi_window_t*p_win, int target_rank);
static inline int      nm_mpi_win_flush_local_all(nm_mpi_window_t*p_win);
static inline int      nm_mpi_win_flush_epoch(nm_mpi_win_epoch_t*p_epoch, nm_mpi_window_t*p_win);
static        void     nm_mpi_win_unexpected_notifier(nm_sr_event_t event,
						      const nm_sr_event_info_t*info, void*ref);
static        void     nm_mpi_win_lock_unexpected(nm_sr_event_t event, const nm_sr_event_info_t*info,
						  void*ref);
static inline int      nm_mpi_win_lock_probe(nm_mpi_win_pass_mngmt_t*p_mng, uint16_t lock_type);
static inline int      nm_mpi_win_lock_is_free(nm_mpi_win_pass_mngmt_t*p_mng);
static        void     nm_mpi_win_free_req_monitor(nm_sr_event_t event, const nm_sr_event_info_t*info,
						   void*ref);
static        void     nm_mpi_win_start_passive_exposure(nm_sr_event_t event,
							 const nm_sr_event_info_t*info, void*ref);
              int      nm_mpi_win_unlock(nm_mpi_window_t*p_win, int from, nm_mpi_win_epoch_t*p_epoch);
static        void     nm_mpi_win_unlock_monitor(nm_sr_event_t event, const nm_sr_event_info_t*info,
						 void*ref);
static inline void     nm_mpi_win_synchronize(nm_mpi_window_t*p_win);
extern        void     nm_mpi_rma_recv(const nm_sr_event_info_t*p_info, nm_mpi_window_t*p_win);
extern        void     nm_mpi_rma_handle_passive(nm_mpi_request_t*p_req);

/* ** Shared memory specific static function prototypes **** */

static        void     nm_mpi_win_start_passive_exposure_shm(nm_sr_event_t event,
								const nm_sr_event_info_t*info,
								void*ref);
static        void     nm_mpi_win_unexpected_notifier_shm(nm_sr_event_t event,
							  const nm_sr_event_info_t*info, void*ref);
static        int      nm_mpi_win_lock_shm(int lock_type, int rank, int assert, nm_mpi_window_t*p_win);
static        int      nm_mpi_win_lock_all_shm(int assert, nm_mpi_window_t*p_win);
static        int      nm_mpi_win_unlock_shm(int rank, nm_mpi_window_t*p_win);
static        int      nm_mpi_win_unlock_all_shm(nm_mpi_window_t*p_win);
extern        void     nm_mpi_rma_handle_passive_shm(nm_mpi_request_t*p_req);
extern        void     nm_mpi_rma_recv_shm(const nm_sr_event_info_t*p_info, nm_mpi_window_t*p_win);

/* ********************************************************* */
/* Aliases */

NM_MPI_ALIAS(MPI_Win_create, mpi_win_create);
NM_MPI_ALIAS(MPI_Win_allocate, mpi_win_allocate);
NM_MPI_ALIAS(MPI_Win_allocate_shared, mpi_win_allocate_shared);
NM_MPI_ALIAS(MPI_Win_shared_query, mpi_win_shared_query);
NM_MPI_ALIAS(MPI_Win_create_dynamic, mpi_win_create_dynamic);
NM_MPI_ALIAS(MPI_Win_attach, mpi_win_attach);
NM_MPI_ALIAS(MPI_Win_detach, mpi_win_detach);
NM_MPI_ALIAS(MPI_Win_free, mpi_win_free);
NM_MPI_ALIAS(MPI_Win_get_group, mpi_win_get_group);
NM_MPI_ALIAS(MPI_Win_set_info, mpi_win_set_info);
NM_MPI_ALIAS(MPI_Win_get_info, mpi_win_get_info);
NM_MPI_ALIAS(MPI_Win_create_keyval, mpi_win_create_keyval);
NM_MPI_ALIAS(MPI_Win_free_keyval, mpi_win_free_keyval);
NM_MPI_ALIAS(MPI_Win_delete_attr, mpi_win_delete_attr);
NM_MPI_ALIAS(MPI_Win_set_attr, mpi_win_set_attr);
NM_MPI_ALIAS(MPI_Win_get_attr, mpi_win_get_attr);
NM_MPI_ALIAS(MPI_Win_set_name, mpi_win_set_name);
NM_MPI_ALIAS(MPI_Win_get_name, mpi_win_get_name);
NM_MPI_ALIAS(MPI_Win_create_errhandler, mpi_win_create_errhandler);
NM_MPI_ALIAS(MPI_Win_set_errhandler, mpi_win_set_errhandler);
NM_MPI_ALIAS(MPI_Win_get_errhandler, mpi_win_get_errhandler);
NM_MPI_ALIAS(MPI_Win_call_errhandler, mpi_win_call_errhandler);
NM_MPI_ALIAS(MPI_Win_fence, mpi_win_fence);
NM_MPI_ALIAS(MPI_Win_start, mpi_win_start);
NM_MPI_ALIAS(MPI_Win_complete, mpi_win_complete);
NM_MPI_ALIAS(MPI_Win_post, mpi_win_post);
NM_MPI_ALIAS(MPI_Win_wait, mpi_win_wait);
NM_MPI_ALIAS(MPI_Win_test, mpi_win_test);
NM_MPI_ALIAS(MPI_Win_lock, mpi_win_lock);
NM_MPI_ALIAS(MPI_Win_lock_all, mpi_win_lock_all);
NM_MPI_ALIAS(MPI_Win_unlock, mpi_win_unlock);
NM_MPI_ALIAS(MPI_Win_unlock_all, mpi_win_unlock_all);
NM_MPI_ALIAS(MPI_Win_flush, mpi_win_flush);
NM_MPI_ALIAS(MPI_Win_flush_all, mpi_win_flush_all);
NM_MPI_ALIAS(MPI_Win_flush_local, mpi_win_flush_local);
NM_MPI_ALIAS(MPI_Win_flush_local_all, mpi_win_flush_local_all);
NM_MPI_ALIAS(MPI_Win_sync, mpi_win_sync);

/* ********************************************************* */

/* ** Memory segments list elements ************************ */

__PUK_SYM_INTERNAL
int nm_mpi_win_addr_is_valid(void*base, MPI_Aint size, nm_mpi_window_t*p_win)
{
  if(!(MPI_WIN_FLAVOR_DYNAMIC == p_win->flavor))
    return 1;
  int is_valid = 0;
  struct nm_mpi_win_addr_s*it;
  nm_mpi_win_addrlist_t*p_list = p_win->p_base;
  nm_mpi_spin_lock(&p_list->lock);
  puk_list_foreach(it, &p_list->head)
    {
      is_valid |= it->begin <= base && it->size >= (size + NM_MPI_WIN_ABS(it->begin - base));
      if(is_valid)
	break;
    }
  nm_mpi_spin_unlock(&p_list->lock);
  return is_valid;
}

/* ** Windows ********************************************** */

static inline nm_mpi_window_t*nm_mpi_window_alloc(int comm_size)
{
  nm_tag_t tag_op, tag_mask_op, tag_sync, tag_mask_sync;
  nm_mpi_window_t*p_win = nm_mpi_handle_window_alloc(&nm_mpi_windows);
  p_win->name           = NULL;
  p_win->p_info         = nm_mpi_info_alloc();
  puk_hashtable_insert(p_win->p_info->content, strdup("no_locks"), strdup("false"));
  puk_hashtable_insert(p_win->p_info->content, strdup("accumulate_ordering"), strdup("rar,raw,war,waw"));
  puk_hashtable_insert(p_win->p_info->content, strdup("accumulate_ops"), strdup("same_op_no_op"));
  puk_hashtable_insert(p_win->p_info->content, strdup("same_size"), strdup("false"));
  puk_hashtable_insert(p_win->p_info->content, strdup("same_disp_unit"), strdup("false"));
  p_win->flavor         = MPI_UNDEFINED;
  p_win->mode           = MPI_MODE_NO_ASSERT;
  p_win->p_base         = NULL;
  p_win->size           = malloc(comm_size * sizeof(MPI_Aint));
  p_win->disp_unit      = malloc(comm_size * sizeof(int));
  p_win->win_ids        = malloc(comm_size * sizeof(int));
  p_win->p_comm         = NULL;
  p_win->p_group        = NULL;
  p_win->mem_model      = MPI_WIN_UNIFIED;
  p_win->p_errhandler   = nm_mpi_errhandler_get(MPI_ERRORS_ARE_FATAL);
  p_win->attrs          = NULL;
  p_win->end_reqs       = calloc(comm_size,  sizeof(nm_mpi_request_t*));
  p_win->msg_count      = calloc(comm_size,  sizeof(uint64_t));
  p_win->access         = malloc(comm_size * sizeof(nm_mpi_win_epoch_t) * 2);
  p_win->exposure       = &p_win->access[comm_size];
  p_win->next_seq       = 0;
  nm_mpi_spin_init(&p_win->lock);
  tag_op                = nm_mpi_rma_win_to_tag(p_win->id) | NM_MPI_TAG_PRIVATE_RMA_BASE;
  tag_mask_op           = NM_MPI_TAG_PRIVATE_RMA_MASK_WIN;
  p_win->monitor        = (struct nm_sr_monitor_s){ .p_notifier = &nm_mpi_win_unexpected_notifier,
						    .event_mask = NM_SR_EVENT_RECV_UNEXPECTED,
						    .p_gate     = NM_ANY_GATE,
						    .tag        = tag_op,
						    .tag_mask   = tag_mask_op,
						    .ref        = p_win };
  tag_sync              = nm_mpi_rma_win_to_tag(p_win->id) | NM_MPI_TAG_PRIVATE_RMA_LOCK;
  tag_mask_sync         = NM_MPI_TAG_PRIVATE_RMA_MASK_SYNC;
  p_win->monitor_sync   = (struct nm_sr_monitor_s){ .p_notifier = &nm_mpi_win_lock_unexpected,
						    .event_mask = NM_SR_EVENT_RECV_UNEXPECTED,
						    .p_gate     = NM_ANY_GATE,
						    .tag        = tag_sync,
						    .tag_mask   = tag_mask_sync,
						    .ref        = p_win };
  for(int i = 0; i < 2 * comm_size; ++i) {
    p_win->access[i].mode      = NM_MPI_WIN_UNUSED;
    p_win->access[i].nmsg      = 0;
    p_win->access[i].completed = 0;
  }
  nm_mpi_request_list_init(&p_win->waiting_queue.q.pending);
  nm_mpi_spin_init(&p_win->waiting_queue.q.lock);
  p_win->waiting_queue.excl_pending = 0;
  p_win->waiting_queue.lock_type    = 0;
  p_win->waiting_queue.naccess      = 0;
  p_win->pending_ops                = malloc(comm_size * sizeof(nm_mpi_win_locklist_t));
  for(int i = 0; i < comm_size; ++i)
    {
      nm_mpi_request_list_init(&p_win->pending_ops[i].pending);
      nm_mpi_spin_init(&p_win->pending_ops[i].lock);
    }
  strcpy(p_win->shared_file_name, "/tmp/padico-d-mpi-shared-XXXXXX");
  return p_win;
}

static inline void nm_mpi_window_destroy(nm_mpi_window_t*p_win)
{
  if(p_win != NULL)
    {
      switch(p_win->flavor)
	{
	case MPI_WIN_FLAVOR_DYNAMIC:
	  {
	    nm_mpi_win_addrlist_t*p_list = p_win->p_base;
	    nm_mpi_spin_lock(&p_list->lock);
	    while(!nm_mpi_win_addr_list_empty(&p_list->head))
	      {
		struct nm_mpi_win_addr_s*it = nm_mpi_win_addr_list_pop_front(&p_list->head);
		nm_mpi_win_addr_free(nm_mpi_win_addr_allocator, it);
	      }
	    nm_mpi_spin_unlock(&p_list->lock);
	    FREE_AND_SET_NULL(p_list);
	  }
	  break;
	case MPI_WIN_FLAVOR_SHARED:
	  {
	    if(0 == p_win->myrank)
	      {
		remove(p_win->shared_file_name);
	      }
	    MPI_Aint total_size = 0;
	    int size = nm_comm_size(p_win->p_comm->p_nm_comm);
	    for(int i=0; i<size; ++i)
	      {
		total_size += p_win->size[i];
	      }
	    munmap(*(void**)p_win->p_base, total_size);
	    FREE_AND_SET_NULL(p_win->p_base);
	  }
	  break;
	case MPI_WIN_FLAVOR_ALLOCATE:
	  {
	    FREE_AND_SET_NULL(p_win->p_base);
	  }
	  break;
	default:
	  {
	    p_win->p_base = NULL;
	  }
	  break;
	}
      FREE_AND_SET_NULL(p_win->size);
      FREE_AND_SET_NULL(p_win->disp_unit);
      FREE_AND_SET_NULL(p_win->win_ids);
      FREE_AND_SET_NULL(p_win->name);
      FREE_AND_SET_NULL(p_win->end_reqs);
      FREE_AND_SET_NULL(p_win->msg_count);
      FREE_AND_SET_NULL(p_win->access);
      FREE_AND_SET_NULL(p_win->pending_ops);
      nm_mpi_info_free(p_win->p_info);
      p_win->p_info       = NULL;
      p_win->flavor       = MPI_UNDEFINED;
      p_win->mode         = MPI_MODE_NO_ASSERT;
      p_win->p_comm       = NULL;
      p_win->p_group      = NULL;
      p_win->mem_model    = MPI_WIN_UNIFIED;
      p_win->p_errhandler = NULL;
      nm_mpi_attrs_destroy(p_win->id, &p_win->attrs);
      nm_mpi_handle_window_free(&nm_mpi_windows, p_win);
    }
}

static inline void nm_mpi_win_exchange_init(nm_mpi_window_t*p_win, int size)
{
  const nm_tag_t tag = NM_MPI_TAG_PRIVATE_WIN_INIT;
  const int   myrank = p_win->myrank;
  nm_mpi_request_t*requests[2 * size];
  nm_mpi_win_init_data_t a_data[size];
  a_data[myrank].size   = p_win->size[myrank];
  a_data[myrank].unit   = p_win->disp_unit[myrank];
  a_data[myrank].win_id = p_win->id;
  for(int i=0; i<size; ++i)
    {
      if(i == myrank) continue;
      requests[2*i]   = nm_mpi_coll_irecv(&a_data[i],      1, p_init_datatype, i, tag, p_win->p_comm);
      requests[2*i+1] = nm_mpi_coll_isend(&a_data[myrank], 1, p_init_datatype, i, tag, p_win->p_comm);
    }
  /* Wait for sends */
  for(int i=0; i<size; ++i)
    {
      if(i == myrank) continue;
      nm_mpi_coll_wait(requests[2*i+1]);
    }
  /* Wait for recieves */
  for(int i = 0; i < size; ++i)
    {
      if(i == myrank) continue;
      nm_mpi_coll_wait(requests[2*i]);
      p_win->size[i]      = a_data[i].size;
      p_win->disp_unit[i] = a_data[i].unit;
      p_win->win_ids[i]   = a_data[i].win_id;
    }
}

static inline int nm_mpi_win_exchange_init_shm(nm_mpi_window_t*p_win, int size)
{
  /* Initial exchange */
  int err = MPI_SUCCESS;
  const nm_tag_t   tag = NM_MPI_TAG_PRIVATE_WIN_INIT;
  const int     myrank = p_win->myrank;
  const uint name_size = 36;
  int fd;
  nm_mpi_datatype_t*p_char = nm_mpi_datatype_get(MPI_CHAR);
  void**p_base = (void**)p_win->p_base;
  if(0 == p_win->myrank)
    {
      /* root */
      nm_mpi_request_t*requests[3 * size + 1];
      nm_mpi_win_init_data_t a_data[size];
      a_data[myrank].size   = p_win->size[myrank];
      a_data[myrank].unit   = p_win->disp_unit[myrank];
      a_data[myrank].win_id = p_win->id;
      for(int i=0; i<size; ++i)
	{
	  if(i == myrank) continue;
	  requests[3 * i]   = nm_mpi_coll_irecv(&a_data[i], 1, p_init_datatype, i, tag, p_win->p_comm);
	}
      /* Wait for recieves */
      MPI_Aint total_size = p_win->size[myrank];
      for(int i=0; i<size; ++i)
	{
	  if(i == myrank) continue;
	  nm_mpi_coll_wait(requests[3 * i]);
	  p_win->size[i]      = a_data[i].size;
	  p_win->disp_unit[i] = a_data[i].unit;
	  p_win->win_ids[i]   = a_data[i].win_id;
	  total_size         += a_data[i].size;
	}
      /* Create shared memory segment */
      fd = mkstemp(p_win->shared_file_name);
      if(-1 == fd)
	{
	  return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RMA_SHARED);
	}
      ftruncate(fd, total_size);
      *p_base = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
      if(MAP_FAILED == *p_base)
	{
	  return (close(fd),NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RMA_SHARED));
	}
      for(int i = 0; i < size; ++i)
	{
	  if(i == myrank) continue;
	  requests[3*i+2] = nm_mpi_coll_isend(p_win->shared_file_name, name_size, p_char, i, tag,
					      p_win->p_comm);
	  requests[3*i+1] = nm_mpi_coll_isend(&a_data[myrank], 1, p_init_datatype, i, tag, p_win->p_comm);
	}
      /* Wait for sends */
      for(int i=0; i<size; ++i)
	{
	  if(i == myrank) continue;
	  nm_mpi_coll_wait(requests[3*i+1]);
	  nm_mpi_coll_wait(requests[3*i+2]);
	}
    }
  else
    {
      nm_mpi_request_t*request = nm_mpi_coll_irecv(p_win->shared_file_name, name_size, p_char, 0, tag, p_win->p_comm);
      nm_mpi_win_exchange_init(p_win, size);
      MPI_Aint total_size = 0;
      for(int i = 0; i < size; ++i)
	{
	  total_size += p_win->size[i];
	}
      nm_mpi_coll_wait(request);
      fd = open(p_win->shared_file_name, O_RDWR);
      if(-1 == fd)
	{
	  return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RMA_SHARED);
	}
      *p_base = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
      if(MAP_FAILED == *p_base)
	{
	  return (close(fd), NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RMA_SHARED));
	}
    }
  close(fd);
  return err;
}

static inline int nm_mpi_win_flush(nm_mpi_window_t*p_win, int target_rank)
{
  int err = nm_mpi_win_flush_epoch(&p_win->exposure[target_rank], p_win);
  assert(nm_mpi_win_completed_epoch(&p_win->exposure[target_rank]));
  return err;
}

static inline int nm_mpi_win_flush_all(nm_mpi_window_t*p_win)
{
  int i, err = MPI_SUCCESS, comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  for(i = 0; i < comm_size; ++i)
    {
      nm_mpi_win_flush(p_win, i);
    }
  return err;
}

static inline int nm_mpi_win_flush_local(nm_mpi_window_t*p_win, int target_rank)
{
  int err = nm_mpi_win_flush_epoch(&p_win->access[target_rank], p_win);
  assert(nm_mpi_win_completed_epoch(&p_win->access[target_rank]));
  return err;
}

static inline int nm_mpi_win_flush_local_all(nm_mpi_window_t*p_win)
{
  int i, err = MPI_SUCCESS, comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  for(i = 0; i < comm_size; ++i)
    {
      nm_mpi_win_flush_local(p_win, i);
    }
  return err;
}

static inline int nm_mpi_win_flush_epoch(nm_mpi_win_epoch_t*p_epoch, nm_mpi_window_t*p_win)
{
  int err = MPI_SUCCESS;
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  while(nm_mpi_win_is_ready(p_epoch) && !nm_mpi_win_completed_epoch(p_epoch))
    {
      nm_sr_progress(p_session);
    }
  return err;
}

__PUK_SYM_INTERNAL
nm_mpi_window_t*nm_mpi_window_get(MPI_Win win)
{
  nm_mpi_window_t*p_win = nm_mpi_handle_window_get(&nm_mpi_windows, win);
  return p_win;
}

static void nm_mpi_win_unexpected_notifier(nm_sr_event_t event, const nm_sr_event_info_t*info, void*ref)
{
  const nm_tag_t    tag = info->recv_unexpected.tag;
  assert(ref);
  nm_mpi_window_t*p_win = (nm_mpi_window_t*)ref;
  if(NULL != p_win)
    {
      switch(tag & NM_MPI_TAG_PRIVATE_RMA_MASK_OP)
	{
	case NM_MPI_TAG_PRIVATE_RMA_PUT:
	case NM_MPI_TAG_PRIVATE_RMA_GET_REQ:
	case NM_MPI_TAG_PRIVATE_RMA_ACC:
	case NM_MPI_TAG_PRIVATE_RMA_GACC_REQ:
	case NM_MPI_TAG_PRIVATE_RMA_FAO_REQ:
	case NM_MPI_TAG_PRIVATE_RMA_CAS_REQ:
	  {
	    nm_mpi_rma_recv(info, p_win);
	  }
	  break;
	default:
	  NM_MPI_FATAL_ERROR("unexpected tag\n");
	}
    }
}

static void nm_mpi_win_free_req_monitor(nm_sr_event_t event, const nm_sr_event_info_t*info, void*ref)
{
  nm_mpi_request_t*p_req = (nm_mpi_request_t*)ref;
  assert(p_req);
  nm_mpi_request_free(p_req);
}

static void nm_mpi_win_lock_unexpected(nm_sr_event_t event, const nm_sr_event_info_t*info, void*ref)
{
  assert(ref);
  nm_mpi_window_t    *p_win = (nm_mpi_window_t*)ref;
  nm_session_t    p_session = info->recv_unexpected.p_session;
  const nm_tag_t        tag = info->recv_unexpected.tag;
  const uint16_t  lock_type = nm_mpi_rma_tag_to_seq(tag);
  const int          source = nm_mpi_communicator_get_dest(p_win->p_comm, info->recv_unexpected.p_gate);
  const nm_tag_t   tag_mask = NM_MPI_TAG_PRIVATE_RMA_MASK_SYNC;
  p_win->exposure[source].mode = NM_MPI_WIN_UNUSED;
  /* Match request */
  nm_mpi_request_t*p_req_lock = nm_mpi_request_alloc();
  nm_sr_recv_init(p_session, &p_req_lock->request_nmad);
  nm_sr_recv_unpack_contiguous(p_session, &p_req_lock->request_nmad, NULL, 0);
  nm_sr_request_set_ref(&p_req_lock->request_nmad, p_req_lock);
  p_req_lock->p_win = p_win;
  nm_sr_request_monitor(p_session, &p_req_lock->request_nmad, NM_SR_EVENT_FINALIZED,
			&nm_mpi_win_start_passive_exposure);
  nm_sr_recv_match_event(p_session, &p_req_lock->request_nmad, info);
  /* Check whether to enqueue the request or start epoch */
  nm_mpi_spin_lock(&p_win->waiting_queue.q.lock);
  if(nm_mpi_win_lock_probe(&p_win->waiting_queue, lock_type))
    {
      /* Start new exposure epoch, LOCK_SHARED or LOCK_EXCLUSIVE */
      __sync_add_and_fetch(&p_win->waiting_queue.naccess, 1);
      __sync_bool_compare_and_swap(&p_win->exposure[source].mode,
				   NM_MPI_WIN_UNUSED,
				   NM_MPI_WIN_PASSIVE_TARGET);
      p_win->waiting_queue.lock_type = lock_type;
      nm_mpi_spin_unlock(&p_win->waiting_queue.q.lock);
      /* Post reception of lock request (triggers the monitor) */
      nm_sr_recv_post(p_session, &p_req_lock->request_nmad);
      /* If self request or shared memory window, send reply */
      if(source == p_win->myrank || MPI_WIN_FLAVOR_SHARED == p_win->flavor)
	{
	  const nm_tag_t tag_lock_r = nm_mpi_rma_create_tag(p_win->win_ids[source], 0,
							    NM_MPI_TAG_PRIVATE_RMA_LOCK_R);
	  nm_mpi_request_t*p_req_ack = nm_mpi_request_alloc();
	  nm_sr_send_init(p_session, &p_req_ack->request_nmad);
	  nm_sr_send_pack_contiguous(p_session, &p_req_ack->request_nmad, NULL, 0);
	  nm_sr_request_set_ref(&p_req_ack->request_nmad, p_req_ack);
	  nm_sr_request_monitor(p_session, &p_req_ack->request_nmad, NM_SR_EVENT_FINALIZED,
				&nm_mpi_win_free_req_monitor);
	  nm_sr_send_isend(p_session, &p_req_ack->request_nmad, info->recv_unexpected.p_gate, tag_lock_r);
	}
      /* Register the unlock request */
      const nm_tag_t tag_unlock = nm_mpi_rma_create_tag(p_win->id, 0, NM_MPI_TAG_PRIVATE_RMA_UNLOCK);
      nm_mpi_request_t*p_req_unlock = nm_mpi_request_alloc();
      nm_sr_recv_init(p_session, &p_req_unlock->request_nmad);
      nm_sr_recv_unpack_contiguous(p_session, &p_req_unlock->request_nmad,
				   &p_win->msg_count[source], sizeof(uint64_t));
      nm_sr_request_set_ref(&p_req_unlock->request_nmad, p_req_unlock);
      p_req_unlock->p_win = p_win;
      nm_sr_request_monitor(p_session, &p_req_unlock->request_nmad, NM_SR_EVENT_FINALIZED,
			    &nm_mpi_win_unlock_monitor);
      nm_sr_recv_irecv(p_session, &p_req_unlock->request_nmad, info->recv_unexpected.p_gate,
		       tag_unlock, tag_mask);
    }
  else
    {
      /* Add the request for lock to the pending list */
      if(NM_MPI_LOCK_EXCLUSIVE == lock_type)
	__sync_add_and_fetch(&p_win->waiting_queue.excl_pending, 1);
      nm_mpi_request_list_push_back(&p_win->waiting_queue.q.pending, p_req_lock);
      nm_mpi_spin_unlock(&p_win->waiting_queue.q.lock);
    }
}

static inline int nm_mpi_win_lock_is_free(nm_mpi_win_pass_mngmt_t*p_mng)
{
  int ret = (NM_MPI_LOCK_NONE == p_mng->lock_type && nm_mpi_request_list_empty(&p_mng->q.pending));
  return ret;
}

static inline int nm_mpi_win_lock_probe(nm_mpi_win_pass_mngmt_t*p_mng, uint16_t lock_type)
{
  int ret =
    nm_mpi_win_lock_is_free(p_mng)
    ||
    (NM_MPI_LOCK_SHARED == lock_type && lock_type == p_mng->lock_type && 0 == p_mng->excl_pending);
  return ret;
}

static void nm_mpi_win_start_passive_exposure(nm_sr_event_t event, const nm_sr_event_info_t*info,
					      void*ref)
{
  nm_gate_t from;
  nm_mpi_request_t*p_req_lock = (nm_mpi_request_t*)ref;
  assert(p_req_lock);
  nm_mpi_window_t*p_win = p_req_lock->p_win;
  assert(p_win);
  if(MPI_WIN_FLAVOR_SHARED != p_win->flavor)
    {
      nm_sr_request_get_gate(&p_req_lock->request_nmad, &from);
      assert(from);
      const int source = nm_mpi_communicator_get_dest(p_win->p_comm, from);
      assert(p_win->exposure[source].mode);
      nm_mpi_win_locklist_t*pendings = &p_win->pending_ops[source];
      nm_mpi_spin_lock(&pendings->lock);
      while(!nm_mpi_request_list_empty(&pendings->pending))
	{
	  nm_mpi_request_t*p_req_lelt = nm_mpi_request_list_pop_front(&pendings->pending);
	  nm_mpi_spin_unlock(&pendings->lock);
	  nm_mpi_rma_handle_passive(p_req_lelt);
	  nm_mpi_spin_lock(&pendings->lock);
	}
      nm_mpi_spin_unlock(&pendings->lock);
    }
  nm_mpi_request_free(p_req_lock);
}

__PUK_SYM_INTERNAL
void nm_mpi_win_enqueue_pending(nm_mpi_request_t*p_req, nm_mpi_window_t*p_win)
{
  nm_mpi_win_locklist_t*pendings = &p_win->pending_ops[p_req->request_source];
  int was_inserted = 0;
  nm_mpi_spin_lock(&pendings->lock);
  if(!((NM_MPI_WIN_PASSIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET_END) & p_req->p_epoch->mode))
    {
      nm_mpi_request_list_push_back(&pendings->pending, p_req);
      was_inserted = 1;
    }
  nm_mpi_spin_unlock(&pendings->lock);
  if(!was_inserted)
    {
      if(MPI_WIN_FLAVOR_SHARED == p_win->flavor)
	nm_mpi_rma_handle_passive_shm(p_req);
      else
	nm_mpi_rma_handle_passive(p_req);
    }
}

static void nm_mpi_win_unlock_monitor(nm_sr_event_t event, const nm_sr_event_info_t*info, void*ref)
{
  assert(ref);
  nm_mpi_request_t*p_req_unlock = (nm_mpi_request_t*)ref;
  assert(p_req_unlock);
  nm_gate_t from;
  nm_sr_request_get_gate(&p_req_unlock->request_nmad, &from);
  assert(from);
  nm_mpi_window_t*p_win = p_req_unlock->p_win;
  nm_tag_t tag;
  nm_sr_request_get_tag(&p_req_unlock->request_nmad, &tag);
  assert(tag);
  const int source = nm_comm_get_dest(p_win->p_comm->p_nm_comm, from);
  p_win->exposure[source].mode = NM_MPI_WIN_PASSIVE_TARGET_END;
  nm_mpi_request_free(p_req_unlock);
  /* Clean the exposure epoch */
  if(nm_mpi_win_completed_epoch(&p_win->exposure[source]))
    {
      nm_mpi_win_unlock(p_win, source, &p_win->exposure[source]);
    }
}

__PUK_SYM_INTERNAL
int nm_mpi_win_unlock(nm_mpi_window_t*p_win, int source, nm_mpi_win_epoch_t*p_epoch)
{
  if(!__sync_bool_compare_and_swap(&p_epoch->mode,
				   NM_MPI_WIN_PASSIVE_TARGET_END,
				   NM_MPI_WIN_UNUSED))
    {
      return MPI_SUCCESS;
    }
  p_epoch->nmsg            = 0;
  p_epoch->completed       = 0;
  p_win->msg_count[source] = 0;
  int  err = MPI_SUCCESS;
  /* Send unlock ACK */
  nm_gate_t from = nm_mpi_communicator_get_gate(p_win->p_comm, source);
  nm_mpi_request_t*p_req_ak = nm_mpi_request_alloc();
  nm_session_t    p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  const nm_tag_t   tag_mask = NM_MPI_TAG_PRIVATE_RMA_MASK_SYNC;
  const nm_tag_t tag_unlock = nm_mpi_rma_create_tag(p_win->id, 0, NM_MPI_TAG_PRIVATE_RMA_UNLOCK);
  const nm_tag_t unlock_ack = nm_mpi_rma_create_tag(p_win->win_ids[source], 0,
						    NM_MPI_TAG_PRIVATE_RMA_UNLOCK_R);
  nm_sr_send_init(p_session, &p_req_ak->request_nmad);
  nm_sr_send_pack_contiguous(p_session, &p_req_ak->request_nmad, NULL, 0);
  nm_sr_request_set_ref(&p_req_ak->request_nmad, p_req_ak);
  nm_sr_request_monitor(p_session, &p_req_ak->request_nmad, NM_SR_EVENT_FINALIZED,
			&nm_mpi_win_free_req_monitor);
  nm_sr_send_isend(p_session, &p_req_ak->request_nmad, from, unlock_ack);
  /* Deal with the possible pending lock requests */
  uint64_t naccess = __sync_sub_and_fetch(&p_win->waiting_queue.naccess, 1);
  nm_mpi_win_synchronize(p_win);
  if(0 < naccess) /* If not the last one to unlock the window */
    return MPI_SUCCESS;
  nm_mpi_spin_lock(&p_win->waiting_queue.q.lock);
  p_win->waiting_queue.lock_type = NM_MPI_LOCK_NONE;
  if(nm_mpi_request_list_empty(&p_win->waiting_queue.q.pending))
    { /* If no lock request pending */
      nm_mpi_spin_unlock(&p_win->waiting_queue.q.lock);
      return MPI_SUCCESS;
    }
  /* Start next exposure */
  nm_tag_t tag;
  nm_mpi_request_t*p_req_unlock;
  uint16_t lock_type;
  nm_mpi_request_t*p_req_lelt = nm_mpi_request_list_pop_front(&p_win->waiting_queue.q.pending);
  nm_mpi_spin_unlock(&p_win->waiting_queue.q.lock);	
  nm_sr_request_get_gate(&p_req_lelt->request_nmad, &from);
  assert(from);
  nm_sr_request_get_tag(&p_req_lelt->request_nmad, &tag);
  assert(tag);
  source = nm_mpi_communicator_get_dest(p_win->p_comm, from);
  lock_type = nm_mpi_rma_tag_to_seq(tag);
  p_epoch = &p_win->exposure[source];
  if(NM_MPI_LOCK_EXCLUSIVE == lock_type)
    __sync_sub_and_fetch(&p_win->waiting_queue.excl_pending, 1);  
  p_win->waiting_queue.lock_type = lock_type;
  /* Dequeue one NM_MPI_LOCK_EXCLUSIVE or all the next NM_MPI_LOCK_SHARED from the pending list */
  do
    {
      /* Start new exposure epoch, LOCK_SHARED or LOCK_EXCLUSIVE */
      p_epoch->mode = NM_MPI_WIN_PASSIVE_TARGET;
      __sync_add_and_fetch(&p_win->waiting_queue.naccess, 1);
      /* Post reception of lock request (triggers the monitor) */
      nm_sr_recv_post(p_session, &p_req_lelt->request_nmad);
      /* If self request or shared memory window, send reply */
      if(source == p_win->myrank || MPI_WIN_FLAVOR_SHARED == p_win->flavor)
	{
	  nm_tag_t tag_lock_r = nm_mpi_rma_create_tag(p_win->win_ids[source], 0,
						      NM_MPI_TAG_PRIVATE_RMA_LOCK_R);
	  nm_mpi_request_t*p_req_ack = nm_mpi_request_alloc();
	  nm_sr_send_init(p_session, &p_req_ack->request_nmad);
	  nm_sr_send_pack_contiguous(p_session, &p_req_ack->request_nmad, NULL, 0);
	  nm_sr_request_set_ref(&p_req_ack->request_nmad, p_req_ack);
	  nm_sr_request_monitor(p_session, &p_req_ack->request_nmad, NM_SR_EVENT_FINALIZED,
				&nm_mpi_win_free_req_monitor);
	  nm_sr_send_isend(p_session, &p_req_ack->request_nmad, from, tag_lock_r);
	}
      /* Register the unlock request */
      p_req_unlock = nm_mpi_request_alloc();
      nm_sr_recv_init(p_session, &p_req_unlock->request_nmad);
      nm_sr_recv_unpack_contiguous(p_session, &p_req_unlock->request_nmad,
				   &p_win->msg_count[source], sizeof(uint64_t));
      nm_sr_request_set_ref(&p_req_unlock->request_nmad, p_req_unlock);
      p_req_unlock->p_win = p_win;
      nm_sr_request_monitor(p_session, &p_req_unlock->request_nmad, NM_SR_EVENT_FINALIZED,
			    &nm_mpi_win_unlock_monitor);
      nm_sr_recv_irecv(p_session, &p_req_unlock->request_nmad, from, tag_unlock, tag_mask);
      /* Free reqlist element and check next one (if LOCK_SHARED) */
      nm_mpi_spin_lock(&p_win->waiting_queue.q.lock);
      p_req_lelt = nm_mpi_request_list_front(&p_win->waiting_queue.q.pending);
      if(NULL == p_req_lelt || NM_MPI_LOCK_EXCLUSIVE == lock_type)
	{
	  nm_mpi_spin_unlock(&p_win->waiting_queue.q.lock);	
	  break;
	}
      nm_mpi_request_list_pop_front(&p_win->waiting_queue.q.pending);
      nm_mpi_spin_unlock(&p_win->waiting_queue.q.lock);	
      nm_sr_request_get_gate(&p_req_lelt->request_nmad, &from);
      assert(from);
      nm_sr_request_get_tag(&p_req_lelt->request_nmad, &tag);
      assert(tag);
      source = nm_mpi_communicator_get_dest(p_win->p_comm, from);
      lock_type = nm_mpi_rma_tag_to_seq(tag);
      p_epoch = &p_win->exposure[source];
    }
  while(p_win->waiting_queue.lock_type == lock_type);
  return err;
}

static inline void nm_mpi_win_synchronize(nm_mpi_window_t*p_win)
{
  __sync_synchronize();    
}

__PUK_SYM_INTERNAL
int nm_mpi_win_send_datatype(nm_mpi_datatype_t*p_datatype, int target_rank, nm_mpi_window_t*p_win)
{
  int err = MPI_SUCCESS;
  assert(p_datatype);
  assert(p_win);
  nm_gate_t gate = nm_mpi_communicator_get_gate(p_win->p_comm, target_rank);
  if(NULL == gate)
    return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RANK);
  err = nm_mpi_datatype_send(gate, p_datatype);
  return err;
}

/* ** Window library functions ***************************** */

__PUK_SYM_INTERNAL
void nm_mpi_win_init(void)
{
  nm_mpi_handle_window_init(&nm_mpi_windows);
  /** Initialize win_init_datatype */
  nm_mpi_win_init_data_t foo;
  const int count = 3;
  int aob[3];          /**< Array of blocklengths */
  MPI_Datatype aot[3]; /**< Array of types */
  MPI_Aint aod[3];     /**< Array of displacement */
  aob[0] = 1; aot[0] = MPI_AINT; aod[0] = 0;
  aob[1] = 1; aot[1] = MPI_INT;  aod[1] = (void*)&foo.unit - (void*)&foo;
  aob[2] = 1; aot[2] = MPI_INT;  aod[2] = (void*)&foo.win_id - (void*)&foo;
  MPI_Type_struct(count, aob, aod, aot, &init_datatype);
  p_init_datatype = nm_mpi_datatype_get(init_datatype);
  p_init_datatype->name = strdup("Window initialization datatype");
  MPI_Type_commit(&init_datatype);
  /** Initialize allocators */
  nm_mpi_win_addr_allocator  = nm_mpi_win_addr_allocator_new(NM_MPI_WIN_INITIAL_ADDR_LELT);
}

__PUK_SYM_INTERNAL
void nm_mpi_win_exit(void)
{
  nm_mpi_win_addr_allocator_delete(nm_mpi_win_addr_allocator);
  p_init_datatype = NULL;
  MPI_Type_free(&init_datatype);
  nm_mpi_handle_window_finalize(&nm_mpi_windows, &nm_mpi_window_destroy);
}

/* ********************************************************* */

int mpi_win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info,
		   MPI_Comm comm, MPI_Win *win)
{
  int err = MPI_SUCCESS;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  nm_session_t       p_session = nm_mpi_communicator_get_session(p_comm);
  if(NULL == p_comm)
    return MPI_ERR_COMM;
  int comm_size = nm_comm_size(p_comm->p_nm_comm);
  int      rank = nm_comm_rank(p_comm->p_nm_comm);
  if(MPI_UNDEFINED == rank)
    return MPI_ERR_COMM;
  struct nm_mpi_info_s*p_info = NULL;
  if(MPI_INFO_NULL != info)
    {
      p_info = nm_mpi_info_get(info);
      if(NULL == p_info)
	return MPI_ERR_INFO;
    }
  nm_mpi_window_t*p_win  = nm_mpi_window_alloc(comm_size);
  nm_mpi_info_update(p_info, p_win->p_info);
  p_win->flavor          = MPI_WIN_FLAVOR_CREATE;
  p_win->myrank          = rank;
  p_win->p_comm          = p_comm;
  p_win->p_base          = base;
  p_win->size[rank]      = size;
  p_win->win_ids[rank]   = p_win->id;
  p_win->disp_unit[rank] = disp_unit;
  nm_mpi_win_exchange_init(p_win, comm_size);
  nm_sr_session_monitor_set(p_session, &p_win->monitor);
  nm_sr_session_monitor_set(p_session, &p_win->monitor_sync);
  *win = p_win->id;
  return err;
}

int mpi_win_allocate(MPI_Aint size, int disp_unit, MPI_Info info,
		     MPI_Comm comm, void *baseptr, MPI_Win *win)
{
  int err = MPI_SUCCESS;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  nm_session_t       p_session = nm_mpi_communicator_get_session(p_comm);
  if(NULL == p_comm)
    return MPI_ERR_COMM;
  int comm_size = nm_comm_size(p_comm->p_nm_comm);
  int      rank = nm_comm_rank(p_comm->p_nm_comm);
  if(MPI_UNDEFINED == rank)
    return MPI_ERR_COMM;
  struct nm_mpi_info_s*p_info = NULL;
  if(MPI_INFO_NULL != info)
    {
      p_info = nm_mpi_info_get(info);
      if(NULL == p_info)
	return MPI_ERR_INFO;
    }
  *(void**)baseptr = malloc(size);
  if(NULL == *(void**)baseptr)
    {
      return MPI_ERR_NO_MEM;
    }
  nm_mpi_window_t*p_win  = nm_mpi_window_alloc(comm_size);
  nm_mpi_info_update(p_info, p_win->p_info);
  p_win->flavor          = MPI_WIN_FLAVOR_ALLOCATE;
  p_win->myrank          = rank;
  p_win->p_comm          = p_comm;
  p_win->p_base          = *(void**)baseptr;
  p_win->size[rank]      = size;
  p_win->win_ids[rank]   = p_win->id;
  p_win->disp_unit[rank] = disp_unit;
  nm_mpi_win_exchange_init(p_win, comm_size);
  nm_sr_session_monitor_set(p_session, &p_win->monitor);
  nm_sr_session_monitor_set(p_session, &p_win->monitor_sync);
  *win = p_win->id;
  return err;
}

int mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPI_Info info,
			    MPI_Comm comm, void *baseptr, MPI_Win *win)
{
  int err = MPI_SUCCESS;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  nm_session_t       p_session = nm_mpi_communicator_get_session(p_comm);
  if(NULL == p_comm)
    return MPI_ERR_COMM;
  int comm_size = nm_comm_size(p_comm->p_nm_comm);
  int      rank = nm_comm_rank(p_comm->p_nm_comm);
  if(MPI_UNDEFINED == rank)
    return MPI_ERR_COMM;
  struct nm_mpi_info_s*p_info = NULL;
  if(MPI_INFO_NULL != info)
    {
      p_info = nm_mpi_info_get(info);
      if(NULL == p_info)
	return MPI_ERR_INFO;
    }
  nm_mpi_window_t*p_win  = nm_mpi_window_alloc(comm_size);
  nm_mpi_info_update(p_info, p_win->p_info);
  p_win->flavor          = MPI_WIN_FLAVOR_SHARED;
  p_win->myrank          = rank;
  p_win->p_comm          = p_comm;
  p_win->size[rank]      = size;
  p_win->win_ids[rank]   = p_win->id;
  p_win->disp_unit[rank] = disp_unit;
  void**p_base           = malloc(comm_size * sizeof(void*));
  p_win->p_base          = p_base;
  if(MPI_SUCCESS != (err = nm_mpi_win_exchange_init_shm(p_win, comm_size)))
    return err;
  for(int i=0; i<comm_size - 1; ++i)
    {
      p_base[i + 1] = p_base[i] + p_win->size[i];
    }
  p_win->monitor.p_notifier = &nm_mpi_win_unexpected_notifier_shm;
  nm_sr_session_monitor_set(p_session, &p_win->monitor);
  nm_sr_session_monitor_set(p_session, &p_win->monitor_sync);
  *win = p_win->id;
  *(void**)baseptr = p_base[rank];
  return err;
}

int mpi_win_shared_query(MPI_Win win, int rank, MPI_Aint *size,
			 int *disp_unit, void *baseptr)
{
  nm_mpi_window_t *p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  if(MPI_WIN_FLAVOR_SHARED != p_win->flavor)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_FLAVOR);
  const int comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  if(MPI_PROC_NULL > rank || rank > comm_size)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_RANK);
  *size = -1;
  if(MPI_PROC_NULL == rank)
    {
      for(int i = 0; i < comm_size; ++i)
	{
	  if(0 < p_win->size[i])
	    {
	      *(void**)baseptr = ((void**)p_win->p_base)[i];
	      *size            = p_win->size[i];
	      *disp_unit       = p_win->disp_unit[i];
	      return MPI_SUCCESS;
	    }
	}
      /* No shared memory allocated */
      *(void**)baseptr = *(void**)p_win->p_base;
      *size            = 0;
      *disp_unit       = 1;
    }
  else
    {
      *(void**)baseptr = ((void**)p_win->p_base)[rank];
      *size            = p_win->size[rank];
      *disp_unit       = p_win->disp_unit[rank];
    }
  return MPI_SUCCESS;
}

int mpi_win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
  int err = MPI_SUCCESS;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  nm_session_t       p_session = nm_mpi_communicator_get_session(p_comm);
  if(NULL == p_comm)
    return MPI_ERR_COMM;
  int comm_size = nm_comm_size(p_comm->p_nm_comm);
  int      rank = nm_comm_rank(p_comm->p_nm_comm);
  if(MPI_UNDEFINED == rank)
    return MPI_ERR_COMM;
  struct nm_mpi_info_s*p_info = NULL;
  if(MPI_INFO_NULL != info)
    {
      p_info = nm_mpi_info_get(info);
      if(NULL == p_info)
	return MPI_ERR_INFO;
    }
  nm_mpi_win_addrlist_t*p_list = malloc(sizeof(nm_mpi_win_addrlist_t));
  nm_mpi_win_addr_list_init(&p_list->head);
  nm_mpi_spin_init(&p_list->lock);
  nm_mpi_window_t*p_win  = nm_mpi_window_alloc(comm_size);
  nm_mpi_info_update(p_info, p_win->p_info);
  p_win->flavor          = MPI_WIN_FLAVOR_DYNAMIC;
  p_win->myrank          = rank;
  p_win->p_comm          = p_comm;
  p_win->p_base          = p_list;
  p_win->size[rank]      = 0;
  p_win->win_ids[rank]   = p_win->id;
  p_win->disp_unit[rank] = 1;
  nm_mpi_win_exchange_init(p_win, comm_size);
  nm_sr_session_monitor_set(p_session, &p_win->monitor);
  nm_sr_session_monitor_set(p_session, &p_win->monitor_sync);
  *win = p_win->id;
  return err;
}

int mpi_win_attach(MPI_Win win, void *base, MPI_Aint size)
{
  int err = MPI_SUCCESS;
  nm_mpi_window_t *p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  struct nm_mpi_win_addr_s*p_addr_lelt = nm_mpi_win_addr_malloc(nm_mpi_win_addr_allocator);
  if(NULL == p_addr_lelt)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_ATTACH);
  if(MPI_WIN_FLAVOR_DYNAMIC != p_win->flavor)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_FLAVOR);
  nm_mpi_win_addrlist_t*p_list = p_win->p_base;
  nm_mpi_spin_lock(&p_list->lock);
#ifdef DEBUG
  /* Check if memory is not already attached */
  struct nm_mpi_win_addr_s*it;
  puk_list_foreach(it, &p_list->head)
    {
      if(it->begin <= base && it->size >= (size + NM_MPI_WIN_ABS(it->begin - base)))
	{
	  err = NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_ATTACH);
	  break;
	}
    }
  if(MPI_SUCCESS != err)
    {
      nm_mpi_spin_unlock(&p_list->lock);
      nm_mpi_win_addr_free(nm_mpi_win_addr_allocator, p_addr_lelt);
      return err;
    }
#endif /* DEBUG */
  p_addr_lelt->begin = base;
  p_addr_lelt->size = size;
  nm_mpi_win_addr_list_push_back(&p_list->head, p_addr_lelt);
  nm_mpi_spin_unlock(&p_list->lock);
  return err;
}

int mpi_win_detach(MPI_Win win, const void *base)
{
  int err = MPI_SUCCESS;
  nm_mpi_window_t *p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  if(MPI_WIN_FLAVOR_DYNAMIC != p_win->flavor)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_FLAVOR);
  nm_mpi_win_addrlist_t*p_list = p_win->p_base;
  nm_mpi_spin_lock(&p_list->lock);
  struct nm_mpi_win_addr_s*it;
  PUK_LIST_FIND(it, &p_list->head, (base == it->begin));
  if(it)
    {
      nm_mpi_win_addr_list_erase(&p_list->head, it);
      nm_mpi_win_addr_free(nm_mpi_win_addr_allocator, it);
    }
  else
    {
      err = NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_RANGE);
    }
  nm_mpi_spin_unlock(&p_list->lock);
  return err;
}

int mpi_win_free(MPI_Win *win)
{
  int i;
  const nm_tag_t     tag = NM_MPI_TAG_PRIVATE_WIN_INIT;
  nm_mpi_window_t *p_win = nm_mpi_window_get(*win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  nm_coll_barrier(p_win->p_comm->p_nm_comm, tag);
  const int    comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  for(i = 0; i < comm_size; ++i)
    {
      do
	{
	  nm_sr_progress(p_session);
	}
      while (!nm_mpi_win_completed_epoch(&p_win->exposure[i])
	     || !nm_mpi_win_completed_epoch(&p_win->access[i]));
    }
  assert(nm_mpi_request_list_empty(&p_win->waiting_queue.q.pending));
  nm_sr_session_monitor_remove(p_session, &p_win->monitor);
  nm_sr_session_monitor_remove(p_session, &p_win->monitor_sync);
  nm_mpi_attrs_destroy(p_win->id, &p_win->attrs);
  nm_mpi_window_destroy(p_win);
  *win = MPI_WIN_NULL;
  return MPI_SUCCESS;
}

int mpi_win_get_group(MPI_Win win, MPI_Group *group)
{
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  nm_mpi_group_t*p_new_group = nm_mpi_group_alloc();
  p_new_group->p_nm_group = nm_group_dup(nm_comm_group(p_win->p_comm->p_nm_comm));
  *group = p_new_group->id;
  return MPI_SUCCESS;
}

int mpi_win_set_info(MPI_Win win, MPI_Info info)
{
  const nm_tag_t    tag = NM_MPI_TAG_PRIVATE_WIN_INIT;
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  struct nm_mpi_info_s*p_info = nm_mpi_info_get(info);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  if(NULL == p_info)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_INFO);
  nm_coll_barrier(p_win->p_comm->p_nm_comm, tag);
  nm_mpi_info_update(p_info, p_win->p_info);
  return MPI_SUCCESS;
}

int mpi_win_get_info(MPI_Win win, MPI_Info *info_used)
{
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_WIN);
  struct nm_mpi_info_s*p_info_dup = nm_mpi_info_dup(p_win->p_info);
  *info_used = p_info_dup->id;
  return MPI_SUCCESS;
}

int mpi_win_create_keyval(MPI_Win_copy_attr_function*copy_fn, MPI_Win_delete_attr_function*delete_fn, int*keyval, void*extra_state)
{
  struct nm_mpi_keyval_s*p_keyval = nm_mpi_keyval_new();
  p_keyval->copy_fn = copy_fn;
  p_keyval->delete_fn = delete_fn;
  p_keyval->extra_state = extra_state;
  *keyval = p_keyval->id;
  return MPI_SUCCESS;
}

int mpi_win_free_keyval(int*keyval)
{
  struct nm_mpi_keyval_s*p_keyval = nm_mpi_keyval_get(*keyval);
  if(p_keyval == NULL)
    return MPI_ERR_KEYVAL;
  nm_mpi_keyval_delete(p_keyval);
  *keyval = MPI_KEYVAL_INVALID;
  return MPI_SUCCESS;
}

int mpi_win_delete_attr(MPI_Win win, int keyval)
{
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(p_win == NULL)
    return MPI_ERR_WIN;
  struct nm_mpi_keyval_s*p_keyval = nm_mpi_keyval_get(keyval);
  if(p_keyval == NULL)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_KEYVAL);
  int err = nm_mpi_attr_delete(p_win->id, p_win->attrs, p_keyval);
  return err;
}

int mpi_win_set_attr(MPI_Win win, int win_keyval, void *attribute_val)
{
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(p_win == NULL)
    return MPI_ERR_WIN;
  struct nm_mpi_keyval_s*p_keyval = nm_mpi_keyval_get(win_keyval);
  if(p_keyval == NULL)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_KEYVAL);
  if(p_win->attrs == NULL)
    {
      p_win->attrs = puk_hashtable_new_ptr();
    }
  int err = nm_mpi_attr_put(p_win->id, p_win->attrs, p_keyval, attribute_val);
  return err;
}

int mpi_win_get_attr(MPI_Win win, int win_keyval, void *attribute_val, int *flag)
{
  int err = MPI_SUCCESS;
  *flag = 0;
  /* Check window validity */
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    {
      return MPI_ERR_WIN;
    }
  /* Check window communicator validity */
  nm_mpi_communicator_t*p_comm = p_win->p_comm;
  if(NULL == p_comm)
    {
      NM_MPI_FATAL_ERROR("Invalid communicator store in the window");
      return NM_MPI_WIN_ERROR(win, MPI_ERR_OTHER);
    }
  /* Check the current process belongs to both the window and the communicator */
  int rank = nm_comm_rank(p_comm->p_nm_comm);
  if(MPI_UNDEFINED == rank)
    {
      NM_MPI_FATAL_ERROR("Invalid communicator store in the window or process does not belong to the window");
      return NM_MPI_WIN_ERROR(win, MPI_ERR_OTHER);
    }
  *flag = 1;
  switch(win_keyval)
    {
    case MPI_WIN_BASE:
      *(void**)attribute_val = p_win->p_base;
      break;
    case MPI_WIN_SIZE:
      *(MPI_Aint**)attribute_val = &p_win->size[rank];
      break;
    case MPI_WIN_DISP_UNIT:
      *(int**)attribute_val = &p_win->disp_unit[rank];
      break;
    case MPI_WIN_CREATE_FLAVOR:
      *(int**)attribute_val = &p_win->flavor;
      break;
    case MPI_WIN_MODEL:
      *(int**)attribute_val = &p_win->mem_model;
      break;
    default:
      {
	struct nm_mpi_keyval_s*p_keyval = nm_mpi_keyval_get(win_keyval);
	*flag = 0;
	if(p_keyval == NULL)
	  return NM_MPI_WIN_ERROR(win, MPI_ERR_KEYVAL);
	nm_mpi_attr_get(p_win->attrs, p_keyval, attribute_val, flag);
      }
      break;
    }  
  return err;
}

int mpi_win_set_name(MPI_Win win, char*win_name)
{
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(p_win == NULL)
    return MPI_ERR_WIN;
  if(p_win->name != NULL)
    {
      free(p_win->name);
    }
  p_win->name = strndup(win_name, MPI_MAX_OBJECT_NAME);
  return MPI_SUCCESS;
}

int mpi_win_get_name(MPI_Win win, char*win_name, int*resultlen)
{
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(p_win == NULL)
    return MPI_ERR_WIN;
  if(p_win->name != NULL)
    {
      strncpy(win_name, p_win->name, MPI_MAX_OBJECT_NAME);
    }
  else
    {
      win_name[0] = '\0';
    }
  *resultlen = strlen(win_name);
  return MPI_SUCCESS;
}

int mpi_win_create_errhandler(MPI_Win_errhandler_fn*function, MPI_Errhandler*errhandler)
{
  return MPI_Errhandler_create(function, errhandler);
}

int mpi_win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler)
{
  nm_mpi_window_t*p_win = nm_mpi_handle_window_get(&nm_mpi_windows, win);
  if(p_win == NULL)
    return MPI_ERR_WIN;
  struct nm_mpi_errhandler_s*p_errhandler = nm_mpi_errhandler_get(errhandler);
  if(p_errhandler == NULL)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_ARG);
  p_win->p_errhandler = p_errhandler;
  return MPI_SUCCESS;
}

int mpi_win_get_errhandler(MPI_Win win, MPI_Errhandler*errhandler)
{
  nm_mpi_window_t*p_win = nm_mpi_handle_window_get(&nm_mpi_windows, win);
  if(p_win == NULL)
    return MPI_ERR_WIN;
  struct nm_mpi_errhandler_s*p_new_errhandler = nm_mpi_errhandler_alloc();
  *p_new_errhandler = *p_win->p_errhandler;
  *errhandler = p_new_errhandler->id;
  return MPI_SUCCESS;
}

int mpi_win_call_errhandler(MPI_Win win, int errorcode)
{
  nm_mpi_window_t*p_win = nm_mpi_handle_window_get(&nm_mpi_windows, win);
  if(p_win == NULL)
    return MPI_ERR_WIN;
  p_win->p_errhandler->function(&win, &errorcode);
  return MPI_SUCCESS;
}

int mpi_win_fence(int assert, MPI_Win win)
{
  int i;
  if(!nm_mpi_win_valid_assert(assert)
     || (assert && !(assert & (MPI_MODE_NOSTORE | MPI_MODE_NOPUT | MPI_MODE_NOPRECEDE | MPI_MODE_NOSUCCEED))))
    return NM_MPI_WIN_ERROR(win, MPI_ERR_ASSERT);
  const nm_tag_t    tag = NM_MPI_TAG_PRIVATE_WIN_BARRIER;
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  const int    comm_size = nm_comm_size(p_win->p_comm->p_nm_comm), myrank = p_win->myrank;
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  if(MPI_MODE_NOPRECEDE & assert || NM_MPI_WIN_UNUSED == p_win->access[myrank].mode)
    {
      nm_coll_barrier(p_win->p_comm->p_nm_comm, tag);
    }
  else
    {
      const nm_tag_t tag_bcast = NM_MPI_TAG_PRIVATE_WIN_FENCE;
      nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(MPI_UINT64_T);
      nm_mpi_request_t*requests[2 * comm_size];
      for(i = 0; i < comm_size; ++i)
	{
	  requests[2*i]   = nm_mpi_coll_irecv(&p_win->msg_count[i], 1, p_datatype, i,
					      tag_bcast, p_win->p_comm);
	  requests[2*i+1] = nm_mpi_coll_isend(&p_win->access[i].nmsg, 1, p_datatype, i,
					      tag_bcast, p_win->p_comm);
	  while(!nm_mpi_win_completed_epoch(&p_win->access[i]))
	    {
	      nm_sr_progress(p_session);
	    }
	  p_win->access[i].mode      = NM_MPI_WIN_UNUSED;
	  p_win->access[i].nmsg      = 0;
	  p_win->access[i].completed = 0;
	}
      for(i = 0; i < comm_size; ++i)
	{
	  /* Wait for sends */
	  nm_mpi_coll_wait(requests[2*i+1]);
	  /* Wait for recieves */
	  nm_mpi_coll_wait(requests[2*i]);
	  while(p_win->msg_count[i] > p_win->exposure[i].completed)
	    {
	      nm_sr_progress(p_session);
	    }
	  p_win->exposure[i].mode      = NM_MPI_WIN_UNUSED;
	  p_win->exposure[i].nmsg      = 0;
	  p_win->exposure[i].completed = 0;
	}
      nm_coll_barrier(p_win->p_comm->p_nm_comm, tag);
      nm_mpi_win_synchronize(p_win);
      if(MPI_WIN_FLAVOR_SHARED == p_win->flavor)
	{
	  for(i = 0; i < comm_size; ++i)
	    {
	      msync(((void**)p_win->p_base)[i], p_win->size[i], MS_SYNC);
	    }
	}
    }
  if(!(MPI_MODE_NOSUCCEED & assert))
    {
      for(i = 0; i < comm_size; ++i)
	{
	  p_win->exposure[i].mode = NM_MPI_WIN_ACTIVE_TARGET;
	  p_win->access[i].mode   = NM_MPI_WIN_ACTIVE_TARGET;
	}
      nm_coll_barrier(p_win->p_comm->p_nm_comm, tag);
    }
  return MPI_SUCCESS;
}

/**
 * Blocking call to start an access epoch
 */
int mpi_win_start(MPI_Group group, int assert, MPI_Win win)
{
  int i, err = MPI_SUCCESS;
  nm_tag_t  tag, tag_mask = NM_MPI_TAG_PRIVATE_RMA_MASK_SYNC;
  nm_gate_t gate;
  if(!nm_mpi_win_valid_assert(assert) || (assert && !(MPI_MODE_NOCHECK & assert)))
    return NM_MPI_WIN_ERROR(win, MPI_ERR_ASSERT);
  nm_mpi_window_t*p_win  = nm_mpi_window_get(win);
  nm_mpi_group_t*p_group = nm_mpi_group_get(group);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  if(NULL == p_group)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_OTHER);
  p_win->p_group          = p_group->p_nm_group;
  nm_session_t  p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  const int     comm_size = nm_group_size(p_win->p_group);
  nm_mpi_request_t p_reqs[comm_size];
  int             targets[comm_size];
  for(i = 0; i < comm_size; ++i)
    {
      gate = nm_group_get_gate(p_win->p_group, i);
      if(tbx_unlikely(gate == NULL))
	{
	  return NM_MPI_WIN_ERROR(win, MPI_ERR_OTHER);
	}
      targets[i] = nm_mpi_communicator_get_dest(p_win->p_comm, gate);
      tag        = nm_mpi_rma_create_tag(p_win->win_ids[targets[i]], 0, NM_MPI_TAG_PRIVATE_RMA_START);
      p_reqs[i].gate                    = nm_group_get_gate(p_win->p_group, i);
      p_reqs[i].sbuf                    = NULL;
      p_reqs[i].count                   = 0;
      p_reqs[i].p_comm                  = p_win->p_comm;
      p_reqs[i].user_tag                = tag;
      p_reqs[i].p_datatype              = nm_mpi_datatype_get(MPI_BYTE);
      p_reqs[i].request_type            = NM_MPI_REQUEST_RECV;
      p_reqs[i].request_source          = targets[i];
      p_reqs[i].communication_mode      = NM_MPI_MODE_IMMEDIATE;
      ++p_reqs[i].p_datatype->refcount;
      nm_sr_recv_init(p_session, &p_reqs[i].request_nmad);
      nm_sr_recv_unpack_contiguous(p_session, &p_reqs[i].request_nmad, NULL, 0);
      err = nm_sr_recv_irecv(p_session, &p_reqs[i].request_nmad, p_reqs[i].gate, tag, tag_mask);
    }
  for(i = 0; i < comm_size; ++i)
    {
      err = nm_sr_rwait(p_session, &p_reqs[i].request_nmad);
      p_reqs[i].request_error = err;
      p_win->access[targets[i]].mode    = NM_MPI_WIN_ACTIVE_TARGET;
      assert(p_win->access[i].nmsg == 0);
    }
  return err;
}

/**
 * Blocking call to finish an access epoch (but should not block as
 * the message is send in eager mode).
 */
int mpi_win_complete(MPI_Win win)
{
  int i, err = MPI_SUCCESS;
  nm_tag_t  tag;
  nm_gate_t gate;
  uint64_t zero = 0;
  nm_mpi_request_t req;
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  const int    comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  nm_mpi_win_flush_local_all(p_win);
  for(i = 0; i < comm_size; ++i)
    {
      if(NM_MPI_WIN_ACTIVE_TARGET != p_win->access[i].mode) continue;
      gate = nm_mpi_communicator_get_gate(p_win->p_comm, i);
      if(tbx_unlikely(gate == NULL))
	{
	  return NM_MPI_WIN_ERROR(win, MPI_ERR_OTHER);
	}
      tag = nm_mpi_rma_create_tag(p_win->win_ids[i], 0, NM_MPI_TAG_PRIVATE_RMA_END);
      req.gate                    = gate;
      req.sbuf                    = (MPI_WIN_FLAVOR_SHARED == p_win->flavor) ? &zero : &p_win->access[i].nmsg;
      req.count                   = 1;
      req.p_comm                  = p_win->p_comm;
      req.user_tag                = tag;
      req.p_datatype              = nm_mpi_datatype_get(MPI_UINT64_T);
      req.request_type            = NM_MPI_REQUEST_SEND;
      req.request_source          = i;
      req.communication_mode      = NM_MPI_MODE_IMMEDIATE;
      ++req.p_datatype->refcount;
      nm_sr_send_init(p_session, &req.request_nmad);
      nm_sr_send_pack_contiguous(p_session, &req.request_nmad, req.sbuf, req.p_datatype->size);
      err = nm_sr_send_isend(p_session, &req.request_nmad, req.gate, tag);
      err = nm_sr_swait(p_session, &req.request_nmad);
      p_win->access[i].mode       = NM_MPI_WIN_UNUSED;
      p_win->access[i].nmsg       = 0;
      p_win->access[i].completed  = 0;
    }
  nm_mpi_win_synchronize(p_win);
  if(MPI_WIN_FLAVOR_SHARED == p_win->flavor)
    {
      for(i = 0; i < comm_size; ++i)
	{
	  msync(((void**)p_win->p_base)[i], p_win->size[i], MS_SYNC);
	}
    }
  return err;
}

/**
 * Blocking call to start an exposure epoch (but should not block as
 * the message is send in eager mode).
 */
int mpi_win_post(MPI_Group group, int assert, MPI_Win win)
{
  int i, err = MPI_SUCCESS;
  if(!nm_mpi_win_valid_assert(assert)
     || (assert && !(assert & (MPI_MODE_NOCHECK | MPI_MODE_NOSTORE | MPI_MODE_NOPUT))))
    return NM_MPI_WIN_ERROR(win, MPI_ERR_ASSERT);
  nm_mpi_window_t*p_win  = nm_mpi_window_get(win);
  nm_mpi_group_t*p_group = nm_mpi_group_get(group);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  if(NULL == p_group)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_OTHER);
  p_win->p_group             = p_group->p_nm_group;
  nm_session_t     p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  const int        comm_size = nm_group_size(p_win->p_group);
  nm_tag_t         tag;
  nm_gate_t        gate;
  int              target;
  nm_mpi_request_t p_reqs[comm_size];
  nm_mpi_request_t *p_req_end;
  for(i = 0; i < comm_size; ++i)
    {
      gate = nm_group_get_gate(p_win->p_group, i);
      if(tbx_unlikely(gate == NULL))
	{
	  return NM_MPI_WIN_ERROR(win, MPI_ERR_OTHER);
	}
      target = nm_mpi_communicator_get_dest(p_win->p_comm, gate);
      tag    = nm_mpi_rma_create_tag(p_win->win_ids[target], 0, NM_MPI_TAG_PRIVATE_RMA_START);
      p_reqs[i].gate                    = gate;
      p_reqs[i].sbuf                    = NULL;
      p_reqs[i].count                   = 0;
      p_reqs[i].p_comm                  = p_win->p_comm;
      p_reqs[i].user_tag                = tag;
      p_reqs[i].p_datatype              = nm_mpi_datatype_get(MPI_BYTE);
      p_reqs[i].request_type            = NM_MPI_REQUEST_SEND;
      p_reqs[i].request_source          = target;
      p_reqs[i].communication_mode      = NM_MPI_MODE_IMMEDIATE;
      ++p_reqs[i].p_datatype->refcount;
      nm_sr_send_init(p_session, &p_reqs[i].request_nmad);
      nm_sr_send_pack_contiguous(p_session, &p_reqs[i].request_nmad, NULL, 0);
      err = nm_sr_send_isend(p_session, &p_reqs[i].request_nmad, gate, tag);
      p_reqs[i].request_error = err;
      /* Closing requests */
      tag       = nm_mpi_rma_create_tag(p_win->id, 0, NM_MPI_TAG_PRIVATE_RMA_END);
      p_req_end = nm_mpi_request_alloc();
      nm_sr_recv_init(p_session, &p_req_end->request_nmad);
      nm_sr_recv_unpack_contiguous(p_session, &p_req_end->request_nmad,
				   &p_win->msg_count[target], sizeof(uint64_t));
      nm_sr_recv_irecv(p_session, &p_req_end->request_nmad, gate, tag, NM_MPI_TAG_PRIVATE_RMA_MASK_SYNC);
      p_win->end_reqs[target]           = p_req_end;
      p_win->exposure[target].mode      = NM_MPI_WIN_ACTIVE_TARGET;
      p_win->exposure[target].nmsg      = 0;
      p_win->exposure[target].completed = 0;
    }
  for(i = 0; i < comm_size; ++i)
    {
      nm_sr_swait(p_session, &p_reqs[i].request_nmad);
    }
  return err;
}
 
/**
 * Blocking call to finish an exposure epoch
 */
int mpi_win_wait(MPI_Win win)
{
  int i, err = MPI_SUCCESS;
  nm_mpi_window_t*p_win  = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  const int    comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  for(i = 0; i < comm_size; ++i)
    {
      if(NULL == p_win->end_reqs[i]) continue;
      err = nm_sr_rwait(p_session, &p_win->end_reqs[i]->request_nmad);
      if(NM_ESUCCESS == err)
	{
	  while(p_win->exposure[i].nmsg < p_win->msg_count[i])
	    {
	      nm_sr_progress(p_session);
	    }
	  nm_mpi_win_flush(p_win, i);
	  p_win->exposure[i].mode      = NM_MPI_WIN_UNUSED;
	  p_win->exposure[i].nmsg      = 0;
	  p_win->exposure[i].completed = 0;
	  nm_mpi_request_free(p_win->end_reqs[i]);
	  p_win->end_reqs[i]  = NULL;
	  p_win->msg_count[i] = 0;
	}
      else
	{
	  return NM_MPI_WIN_ERROR(win, MPI_ERR_OTHER);
	}
    }
  nm_mpi_win_synchronize(p_win);
  return err;
}
     
int mpi_win_test(MPI_Win win, int *flag)
{
  int i, err = MPI_SUCCESS;
  *flag = 0;
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  const int    comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  for(i = 0; i < comm_size; ++i)
    {
      if(NULL == p_win->end_reqs[i]) continue;
      err = nm_sr_rtest(p_session, &p_win->end_reqs[i]->request_nmad);
      if(NM_ESUCCESS == err && p_win->exposure[i].nmsg >= p_win->msg_count[i])
	{
	  nm_mpi_win_flush(p_win, i);
	  p_win->exposure[i].mode      = NM_MPI_WIN_UNUSED;
	  p_win->exposure[i].nmsg      = 0;
	  p_win->exposure[i].completed = 0;
	  nm_mpi_request_free(p_win->end_reqs[i]);
	  p_win->end_reqs[i]  = NULL;
	  p_win->msg_count[i] = 0;
	}
      else
	{
	  nm_sr_progress(p_session);
	  return MPI_SUCCESS;
	}
    }
  /* All pending request are completed */
  *flag = 1;
  nm_mpi_win_synchronize(p_win);
  return MPI_SUCCESS;
}

int mpi_win_lock(int lock_type, int rank, int assert, MPI_Win win)
{
  int err = MPI_SUCCESS;
  if(_NM_MPI_LOCK_OFFSET <= lock_type || 0 >= lock_type)
    return NM_MPI_WIN_ERROR(win, MPI_ERR_LOCKTYPE);
  if(!nm_mpi_win_valid_assert(assert) || (assert && !(assert & MPI_MODE_NOCHECK)))
    return NM_MPI_WIN_ERROR(win, MPI_ERR_ASSERT);
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  if(nm_mpi_win_is_ready(&p_win->access[rank]) && !nm_mpi_win_completed_epoch(&p_win->access[rank])
     || NULL != p_win->end_reqs[rank])
    {
      /* Check if there is really an active  exposure / access epoch currently pending */
      return NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_CONFLICT);
    }
  if(MPI_WIN_FLAVOR_SHARED == p_win->flavor)
    {
      return nm_mpi_win_lock_shm(lock_type, rank, assert, p_win);
    }
  nm_sr_request_t req;
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  nm_gate_t         gate = nm_comm_get_gate(p_win->p_comm->p_nm_comm, rank);
  if(tbx_unlikely(gate == NULL))
    {
      return NM_MPI_WIN_ERROR(win, MPI_ERR_OTHER);
    }
  /* Ending epoch */
  nm_tag_t tag_unlock_ack = nm_mpi_rma_create_tag(p_win->id, 0, NM_MPI_TAG_PRIVATE_RMA_UNLOCK_R);
  p_win->end_reqs[rank] = nm_mpi_request_alloc();
  nm_sr_recv_init(p_session, &p_win->end_reqs[rank]->request_nmad);
  nm_sr_recv_unpack_contiguous(p_session, &p_win->end_reqs[rank]->request_nmad, NULL, 0);
  err = nm_sr_recv_irecv(p_session, &p_win->end_reqs[rank]->request_nmad, gate, tag_unlock_ack,
			 NM_MPI_TAG_PRIVATE_RMA_MASK_SYNC);
  /* Send request */
  nm_tag_t tag = nm_mpi_rma_create_tag(p_win->win_ids[rank], lock_type, NM_MPI_TAG_PRIVATE_RMA_LOCK);
  nm_sr_send_init(p_session, &req);
  nm_sr_send_pack_contiguous(p_session, &req, NULL, 0);
  err = nm_sr_send_isend(p_session, &req, gate, tag);
  nm_sr_swait(p_session, &req);
  /* Recv echo for self lock */
  const nm_tag_t tag_resp = nm_mpi_rma_create_tag(p_win->id, 0, NM_MPI_TAG_PRIVATE_RMA_LOCK_R);
  if(rank == p_win->myrank)
    {
      /* Recv self request echo */
      nm_sr_request_t req_in;
      nm_sr_recv_init(p_session, &req_in);
      nm_sr_recv_unpack_contiguous(p_session, &req_in, NULL, 0);
      nm_sr_recv_irecv(p_session, &req_in, gate, tag_resp, NM_MPI_TAG_PRIVATE_RMA_MASK_SYNC);
      nm_sr_rwait(p_session, &req_in);
    }
  p_win->access[rank].mode = NM_MPI_WIN_PASSIVE_TARGET;
  return err;
}

int mpi_win_lock_all(int assert, MPI_Win win)
{
  int i, err = MPI_SUCCESS;
  nm_gate_t gate;
  if(!nm_mpi_win_valid_assert(assert) || (assert && !(assert & MPI_MODE_NOCHECK)))
    return NM_MPI_WIN_ERROR(win, MPI_ERR_ASSERT);
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  if(MPI_WIN_FLAVOR_SHARED == p_win->flavor)
    {
      return nm_mpi_win_lock_all_shm(assert, p_win);
    }
  nm_tag_t tag;
  nm_session_t    p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  const int       comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  nm_sr_request_t reqs_out[comm_size];
  nm_sr_request_t req_in;
  for(i = 0; i < comm_size; ++i)
    {
      if(nm_mpi_win_is_ready(&p_win->access[i]) && !nm_mpi_win_completed_epoch(&p_win->access[i]))
	{
	  /* Check if there is really an active  exposure / access epoch currently pending */
	  return NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_CONFLICT);
	}
    }
  /* Recv self request echo */
  const nm_tag_t tag_resp = nm_mpi_rma_create_tag(p_win->id, 0, NM_MPI_TAG_PRIVATE_RMA_LOCK_R);
  gate = nm_comm_get_gate(p_win->p_comm->p_nm_comm, p_win->myrank);
  nm_sr_recv_init(p_session, &req_in);
  nm_sr_recv_unpack_contiguous(p_session, &req_in, NULL, 0);
  nm_sr_recv_irecv(p_session, &req_in, gate, tag_resp, NM_MPI_TAG_PRIVATE_RMA_MASK_SYNC);
  /* Send lock requests */
  const nm_tag_t tag_unlock_ack = nm_mpi_rma_create_tag(p_win->id, 0, NM_MPI_TAG_PRIVATE_RMA_UNLOCK_R);
  for(i = 0; i < comm_size; ++i)
    {
      gate = nm_comm_get_gate(p_win->p_comm->p_nm_comm, i);
      if(tbx_unlikely(gate == NULL))
	{
	  return NM_MPI_WIN_ERROR(win, MPI_ERR_OTHER);
	}
      p_win->access[i].mode = NM_MPI_WIN_PASSIVE_TARGET;
      /* Ending epoch */
      p_win->end_reqs[i] = nm_mpi_request_alloc();
      nm_sr_recv_init(p_session, &p_win->end_reqs[i]->request_nmad);
      nm_sr_recv_unpack_contiguous(p_session, &p_win->end_reqs[i]->request_nmad, NULL, 0);
      err = nm_sr_recv_irecv(p_session, &p_win->end_reqs[i]->request_nmad, gate, tag_unlock_ack,
			     NM_MPI_TAG_PRIVATE_RMA_MASK_SYNC);
      /* Send request */
      tag = nm_mpi_rma_create_tag(p_win->win_ids[i], NM_MPI_LOCK_SHARED, NM_MPI_TAG_PRIVATE_RMA_LOCK);
      nm_sr_send_init(p_session, &reqs_out[i]);
      nm_sr_send_pack_contiguous(p_session, &reqs_out[i], NULL, 0);
      err = nm_sr_send_isend(p_session, &reqs_out[i], gate, tag);
    }
  for(i = 0; i < comm_size; ++i)
    {
      nm_sr_swait(p_session, &reqs_out[i]);
    }
  /* Wait for self lock */
  nm_sr_rwait(p_session, &req_in);
  return err;
}

int mpi_win_unlock(int rank, MPI_Win win)
{
  int err = MPI_SUCCESS;
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  if(MPI_WIN_FLAVOR_SHARED == p_win->flavor)
    {
      return nm_mpi_win_unlock_shm(rank, p_win);
    }
  if(!(__sync_bool_compare_and_swap(&p_win->access[rank].mode,
				    NM_MPI_WIN_PASSIVE_TARGET_PENDING,
				    NM_MPI_WIN_PASSIVE_TARGET_END)
       || __sync_bool_compare_and_swap(&p_win->access[rank].mode,
				       NM_MPI_WIN_PASSIVE_TARGET,
				       NM_MPI_WIN_PASSIVE_TARGET_END)))
    {
      return NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_CONFLICT);
    }
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  nm_gate_t         gate = nm_comm_get_gate(p_win->p_comm->p_nm_comm, rank);
  nm_mpi_win_flush_local(p_win, rank);
  if(tbx_unlikely(gate == NULL))
    {
      return NM_MPI_WIN_ERROR(win, MPI_ERR_OTHER);
    }
  /* Send unlock request */
  nm_sr_request_t req_out;
  const nm_tag_t tag = nm_mpi_rma_create_tag(p_win->win_ids[rank], 0, NM_MPI_TAG_PRIVATE_RMA_UNLOCK);
  nm_sr_send_init(p_session, &req_out);
  nm_sr_send_pack_contiguous(p_session, &req_out, &p_win->access[rank].nmsg, sizeof(uint64_t));
  err = nm_sr_send_isend(p_session, &req_out, gate, tag);
  err = nm_sr_swait(p_session, &req_out);
  /* Wait for unlock ack */
  nm_sr_rwait(p_session, &p_win->end_reqs[rank]->request_nmad);
  nm_mpi_request_free(p_win->end_reqs[rank]);
  /* Reset epoch */
  p_win->end_reqs[rank]         = NULL;
  p_win->access[rank].mode      = NM_MPI_WIN_UNUSED;
  p_win->access[rank].nmsg      = 0;
  p_win->access[rank].completed = 0;
  p_win->msg_count[rank]        = 0;
  nm_mpi_win_synchronize(p_win);
  return err;
}

int mpi_win_unlock_all(MPI_Win win)
{
  int i, err = MPI_SUCCESS;
  nm_gate_t gate;
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  if(MPI_WIN_FLAVOR_SHARED == p_win->flavor)
    {
      return nm_mpi_win_unlock_all_shm(p_win);
    }
  nm_tag_t         tag;
  nm_session_t     p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  const int        comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  nm_sr_request_t  reqs_out[comm_size];
  for(i = 0; i < comm_size; ++i)
    {
      if(!(__sync_bool_compare_and_swap(&p_win->access[i].mode,
					NM_MPI_WIN_PASSIVE_TARGET_PENDING,
					NM_MPI_WIN_PASSIVE_TARGET_END)
	   || __sync_bool_compare_and_swap(&p_win->access[i].mode,
					   NM_MPI_WIN_PASSIVE_TARGET,
					   NM_MPI_WIN_PASSIVE_TARGET_END)))
	return NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_CONFLICT);
    }
  nm_mpi_win_flush_local_all(p_win);
  for(i = 0; i < comm_size; ++i)
    {
      gate = nm_comm_get_gate(p_win->p_comm->p_nm_comm, i);
      if(tbx_unlikely(gate == NULL))
	{
	  return NM_MPI_WIN_ERROR(win, MPI_ERR_OTHER);
	}
      tag = nm_mpi_rma_create_tag(p_win->win_ids[i], 0, NM_MPI_TAG_PRIVATE_RMA_UNLOCK);
      nm_sr_send_init(p_session, &reqs_out[i]);
      nm_sr_send_pack_contiguous(p_session, &reqs_out[i], &p_win->access[i].nmsg, sizeof(uint64_t));
      err = nm_sr_send_isend(p_session, &reqs_out[i], gate, tag);
    }
  for(i = 0; i < comm_size; ++i)
    {
      nm_sr_swait(p_session, &reqs_out[i]);
      nm_sr_rwait(p_session, &p_win->end_reqs[i]->request_nmad);
      nm_mpi_request_free(p_win->end_reqs[i]);
      p_win->end_reqs[i]         = NULL;
      p_win->access[i].mode      = NM_MPI_WIN_UNUSED;
      p_win->access[i].nmsg      = 0;
      p_win->access[i].completed = 0;
      p_win->msg_count[i]        = 0;
    }
  nm_mpi_win_synchronize(p_win);
  return err;
}

int mpi_win_flush(int rank, MPI_Win win)
{
  int err = MPI_SUCCESS;
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  if(!((NM_MPI_WIN_PASSIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET_PENDING | NM_MPI_WIN_PASSIVE_TARGET_END)
       & p_win->access[rank].mode))
    return NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_CONFLICT);
  err = nm_mpi_win_flush_local(p_win, rank);
  return err;
}

int mpi_win_flush_all(MPI_Win win)
{
  int err = MPI_SUCCESS;
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  const int comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  for(int i = 0; i < comm_size; ++i)
    {
      if(p_win->access[i].mode && 
	 !((NM_MPI_WIN_PASSIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET_PENDING | NM_MPI_WIN_PASSIVE_TARGET_END)
	   & p_win->access[i].mode))
	return NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_CONFLICT);
    }
  err = nm_mpi_win_flush_local_all(p_win);
  return err;
}

int mpi_win_flush_local(int rank, MPI_Win win)
{
  int err = MPI_SUCCESS;
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  if(!((NM_MPI_WIN_PASSIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET_PENDING | NM_MPI_WIN_PASSIVE_TARGET_END)
       & p_win->access[rank].mode))
    return NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_CONFLICT);
  err = nm_mpi_win_flush_local(p_win, rank);
  return err;
}

int mpi_win_flush_local_all(MPI_Win win)
{
  int err = MPI_SUCCESS;
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  const int comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  for(int i = 0; i < comm_size; ++i)
    {
      if(p_win->access[i].mode && 
	 !((NM_MPI_WIN_PASSIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET_PENDING | NM_MPI_WIN_PASSIVE_TARGET_END)
	   & p_win->access[i].mode))
	return NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_CONFLICT);
    }
  err = nm_mpi_win_flush_local_all(p_win);
  return err;
}

int mpi_win_sync(MPI_Win win)
{
  int err = MPI_SUCCESS;
  nm_mpi_window_t*p_win = nm_mpi_window_get(win);
  if(NULL == p_win)
    return MPI_ERR_WIN;
  const int test_passive = NM_MPI_WIN_PASSIVE_TARGET | NM_MPI_WIN_PASSIVE_TARGET_PENDING | NM_MPI_WIN_PASSIVE_TARGET_END;
  const int comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  for(int i = 0; i < comm_size; ++i)
    {
      if(p_win->access[i].mode && !(test_passive & p_win->access[i].mode))
	return NM_MPI_WIN_ERROR(win, MPI_ERR_RMA_CONFLICT);
    }
  nm_mpi_win_synchronize(p_win);
  return err;
}

/* ** Shared memory synchronization functions definition *** */

static void nm_mpi_win_start_passive_exposure_shm(nm_sr_event_t event, const nm_sr_event_info_t*info,
						  void*ref)
{
  nm_gate_t from;
  nm_mpi_request_t*p_req_lock = (nm_mpi_request_t*)ref;
  assert(p_req_lock);
  nm_mpi_window_t*p_win = p_req_lock->p_win;
  assert(p_win);
  nm_sr_request_get_gate(&p_req_lock->request_nmad, &from);
  assert(from);
  const int source = nm_mpi_communicator_get_dest(p_win->p_comm, from);
  assert(__sync_bool_compare_and_swap(&p_win->access[source].mode,
				      NM_MPI_WIN_PASSIVE_TARGET_PENDING,
				      NM_MPI_WIN_PASSIVE_TARGET)
	 || __sync_bool_compare_and_swap(&p_win->access[source].mode,
					 NM_MPI_WIN_PASSIVE_TARGET_END,
					 NM_MPI_WIN_PASSIVE_TARGET_END));
  nm_mpi_win_locklist_t*pendings = &p_win->pending_ops[source];
  nm_mpi_spin_lock(&pendings->lock);
  while(!nm_mpi_request_list_empty(&pendings->pending))
    {
      nm_mpi_request_t*p_req_lelt = nm_mpi_request_list_pop_front(&pendings->pending);
      nm_mpi_spin_unlock(&pendings->lock);
      nm_mpi_rma_handle_passive_shm(p_req_lelt);
      nm_mpi_spin_lock(&pendings->lock);
    }
  nm_mpi_spin_unlock(&pendings->lock);
  nm_mpi_request_free(p_req_lock);
}

static void nm_mpi_win_unexpected_notifier_shm(nm_sr_event_t event, const nm_sr_event_info_t*info,
					       void*ref)
{
  const nm_tag_t    tag = info->recv_unexpected.tag;
  assert(ref);
  nm_mpi_window_t*p_win = (nm_mpi_window_t*)ref;
  if(NULL != p_win)
    {
      switch(tag & NM_MPI_TAG_PRIVATE_RMA_MASK_OP)
	{
	case NM_MPI_TAG_PRIVATE_RMA_PUT:
	case NM_MPI_TAG_PRIVATE_RMA_GET_REQ:
	case NM_MPI_TAG_PRIVATE_RMA_ACC:
	case NM_MPI_TAG_PRIVATE_RMA_GACC_REQ:
	case NM_MPI_TAG_PRIVATE_RMA_FAO_REQ:
	case NM_MPI_TAG_PRIVATE_RMA_CAS_REQ:
	  {
	    nm_mpi_rma_recv_shm(info, p_win);
	  }
	  break;
	default:
	  NM_MPI_FATAL_ERROR("unexpected tag.\n");
	}
    }
}

static int nm_mpi_win_lock_shm(int lock_type, int rank, int assert, nm_mpi_window_t*p_win)
{
  int err = MPI_SUCCESS;
  nm_sr_request_t req;
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  nm_gate_t         gate = nm_comm_get_gate(p_win->p_comm->p_nm_comm, rank);
  if(tbx_unlikely(gate == NULL))
    {
      return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_OTHER);
    }
  /* Ending epoch */
  nm_tag_t tag_unlock_ack = nm_mpi_rma_create_tag(p_win->id, 0, NM_MPI_TAG_PRIVATE_RMA_UNLOCK_R);
  p_win->end_reqs[rank] = nm_mpi_request_alloc();
  nm_sr_recv_init(p_session, &p_win->end_reqs[rank]->request_nmad);
  nm_sr_recv_unpack_contiguous(p_session, &p_win->end_reqs[rank]->request_nmad, NULL, 0);
  err = nm_sr_recv_irecv(p_session, &p_win->end_reqs[rank]->request_nmad, gate, tag_unlock_ack,
			 NM_MPI_TAG_PRIVATE_RMA_MASK_SYNC);
  /* Send request */
  nm_tag_t tag = nm_mpi_rma_create_tag(p_win->win_ids[rank], lock_type, NM_MPI_TAG_PRIVATE_RMA_LOCK);
  nm_sr_send_init(p_session, &req);
  nm_sr_send_pack_contiguous(p_session, &req, NULL, 0);
  err = nm_sr_send_isend(p_session, &req, gate, tag);
  nm_sr_swait(p_session, &req);
  /* Recv echo */
  const nm_tag_t tag_resp = nm_mpi_rma_create_tag(p_win->id, 0, NM_MPI_TAG_PRIVATE_RMA_LOCK_R);
  if(rank == p_win->myrank)
    {
      /* Recv self request echo */
      nm_sr_request_t req_in;
      nm_sr_recv_init(p_session, &req_in);
      nm_sr_recv_unpack_contiguous(p_session, &req_in, NULL, 0);
      nm_sr_recv_irecv(p_session, &req_in, gate, tag_resp, NM_MPI_TAG_PRIVATE_RMA_MASK_SYNC);
      nm_sr_rwait(p_session, &req_in);
      p_win->access[rank].mode = NM_MPI_WIN_PASSIVE_TARGET;
    }
  else
    {
      p_win->access[rank].mode = NM_MPI_WIN_PASSIVE_TARGET_PENDING;
      nm_mpi_request_t*p_req = nm_mpi_request_alloc();
      p_req->p_win = p_win;
      nm_sr_recv_init(p_session, &p_req->request_nmad);
      nm_sr_recv_unpack_contiguous(p_session, &p_req->request_nmad, NULL, 0);
      nm_sr_request_set_ref(&p_req->request_nmad, p_req);
      nm_sr_request_monitor(p_session, &p_req->request_nmad, NM_SR_EVENT_FINALIZED,
			    &nm_mpi_win_start_passive_exposure_shm);
      nm_sr_recv_irecv(p_session, &p_req->request_nmad, gate, tag_resp, NM_MPI_TAG_PRIVATE_RMA_MASK_SYNC);
    }
  msync(((void**)p_win->p_base)[rank], p_win->size[rank], MS_SYNC);
  return err;
}

static int nm_mpi_win_lock_all_shm(int assert, nm_mpi_window_t*p_win)
{
  int i, err = MPI_SUCCESS;
  nm_gate_t gate;
  nm_tag_t tag;
  nm_session_t    p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  const int       comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  nm_sr_request_t reqs_out[comm_size];
  nm_sr_request_t req_in;
  /* Recv self request echo */
  const nm_tag_t tag_resp = nm_mpi_rma_create_tag(p_win->id, 0, NM_MPI_TAG_PRIVATE_RMA_LOCK_R);
  gate = nm_comm_get_gate(p_win->p_comm->p_nm_comm, p_win->myrank);
  nm_sr_recv_init(p_session, &req_in);
  nm_sr_recv_unpack_contiguous(p_session, &req_in, NULL, 0);
  nm_sr_recv_irecv(p_session, &req_in, gate, tag_resp, NM_MPI_TAG_PRIVATE_RMA_MASK_SYNC);
  for(i = 0; i < comm_size; ++i)
    {
      gate = nm_comm_get_gate(p_win->p_comm->p_nm_comm, i);
      if(tbx_unlikely(gate == NULL))
	{
	  return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_OTHER);
	}
      if(i != p_win->myrank)
	{
	  /* Recv echo */
	  p_win->access[i].mode = NM_MPI_WIN_PASSIVE_TARGET_PENDING;
	  nm_mpi_request_t*p_req = nm_mpi_request_alloc();
	  p_req->p_win = p_win;
	  nm_sr_recv_init(p_session, &p_req->request_nmad);
	  nm_sr_recv_unpack_contiguous(p_session, &p_req->request_nmad, NULL, 0);
	  nm_sr_request_set_ref(&p_req->request_nmad, p_req);
	  nm_sr_request_monitor(p_session, &p_req->request_nmad, NM_SR_EVENT_FINALIZED,
				&nm_mpi_win_start_passive_exposure_shm);
	  nm_sr_recv_irecv(p_session, &p_req->request_nmad, gate, tag_resp,
			   NM_MPI_TAG_PRIVATE_RMA_MASK_SYNC);
	}
      else
	{
	  p_win->access[i].mode = NM_MPI_WIN_PASSIVE_TARGET;
	}
      /* Ending epoch */
      nm_tag_t tag_unlock_ack = nm_mpi_rma_create_tag(p_win->id, 0, NM_MPI_TAG_PRIVATE_RMA_UNLOCK_R);
      p_win->end_reqs[i] = nm_mpi_request_alloc();
      nm_sr_recv_init(p_session, &p_win->end_reqs[i]->request_nmad);
      nm_sr_recv_unpack_contiguous(p_session, &p_win->end_reqs[i]->request_nmad, NULL, 0);
      err = nm_sr_recv_irecv(p_session, &p_win->end_reqs[i]->request_nmad, gate, tag_unlock_ack,
			     NM_MPI_TAG_PRIVATE_RMA_MASK_SYNC);
      /* Send requests */
      tag  = nm_mpi_rma_create_tag(p_win->win_ids[i], NM_MPI_LOCK_SHARED, NM_MPI_TAG_PRIVATE_RMA_LOCK);
      nm_sr_send_init(p_session, &reqs_out[i]);
      nm_sr_send_pack_contiguous(p_session, &reqs_out[i], NULL, 0);
      err = nm_sr_send_isend(p_session, &reqs_out[i], gate, tag);
    }
  for(i = 0; i < comm_size; ++i)
    {
      nm_sr_swait(p_session, &reqs_out[i]);
      msync(((void**)p_win->p_base)[i], p_win->size[i], MS_SYNC);
    }
  nm_sr_rwait(p_session, &req_in);
  return err;
}

static int nm_mpi_win_unlock_shm(int rank, nm_mpi_window_t*p_win)
{
  int err = MPI_SUCCESS;
  if(!(__sync_bool_compare_and_swap(&p_win->access[rank].mode,
				    NM_MPI_WIN_PASSIVE_TARGET_PENDING,
				    NM_MPI_WIN_PASSIVE_TARGET_END)
       || __sync_bool_compare_and_swap(&p_win->access[rank].mode,
				       NM_MPI_WIN_PASSIVE_TARGET,
				       NM_MPI_WIN_PASSIVE_TARGET_END)))
    {
      return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RMA_CONFLICT);
    }
  nm_session_t p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  nm_gate_t         gate = nm_comm_get_gate(p_win->p_comm->p_nm_comm, rank);
  if(tbx_unlikely(gate == NULL))
    {
      return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_OTHER);
    }
  /* Wait for local operation completion */
  nm_mpi_win_flush_local(p_win, rank);
  /* Send unlock */
  nm_sr_request_t req_out;
  const uint64_t nmsg = 0;
  const nm_tag_t  tag = nm_mpi_rma_create_tag(p_win->win_ids[rank], 0, NM_MPI_TAG_PRIVATE_RMA_UNLOCK);
  nm_sr_send_init(p_session, &req_out);
  nm_sr_send_pack_contiguous(p_session, &req_out, &nmsg, sizeof(uint64_t));
  err = nm_sr_send_isend(p_session, &req_out, gate, tag);
  err = nm_sr_swait(p_session, &req_out);
  /* Wait for unlock_ack */
  nm_sr_rwait(p_session, &p_win->end_reqs[rank]->request_nmad);
  nm_mpi_request_free(p_win->end_reqs[rank]);
  /* Reset epoch */
  p_win->end_reqs[rank]         = NULL;
  p_win->access[rank].mode      = NM_MPI_WIN_UNUSED;
  p_win->access[rank].nmsg      = 0;
  p_win->access[rank].completed = 0;
  p_win->msg_count[rank]        = 0;
  nm_mpi_win_synchronize(p_win);
  msync(((void**)p_win->p_base)[rank], p_win->size[rank], MS_SYNC);
  return err;
}

static int nm_mpi_win_unlock_all_shm(nm_mpi_window_t*p_win)
{
  int i, err = MPI_SUCCESS;
  nm_gate_t gate;
  nm_tag_t         tag;
  nm_session_t     p_session = nm_mpi_communicator_get_session(p_win->p_comm);
  const int        comm_size = nm_comm_size(p_win->p_comm->p_nm_comm);
  const uint64_t   nmsg = 0;
  nm_sr_request_t  reqs_out[comm_size];
  for(i = 0; i < comm_size; ++i)
    {
      if(!(__sync_bool_compare_and_swap(&p_win->access[i].mode,
					NM_MPI_WIN_PASSIVE_TARGET_PENDING,
					NM_MPI_WIN_PASSIVE_TARGET_END)
	   || __sync_bool_compare_and_swap(&p_win->access[i].mode,
					   NM_MPI_WIN_PASSIVE_TARGET,
					   NM_MPI_WIN_PASSIVE_TARGET_END)))
	{
	  return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_RMA_CONFLICT);
	}
    }
  /* Wait for local operations completion */
  nm_mpi_win_flush_local_all(p_win);
  /* Send unlock requests */
  for(i = 0; i < comm_size; ++i)
    {
      gate = nm_comm_get_gate(p_win->p_comm->p_nm_comm, i);
      if(tbx_unlikely(gate == NULL))
	{
	  return NM_MPI_WIN_ERROR(p_win->id, MPI_ERR_OTHER);
	}
      tag = nm_mpi_rma_create_tag(p_win->win_ids[i], 0, NM_MPI_TAG_PRIVATE_RMA_UNLOCK);
      nm_sr_send_init(p_session, &reqs_out[i]);
      nm_sr_send_pack_contiguous(p_session, &reqs_out[i], &nmsg, sizeof(uint64_t));
      err = nm_sr_send_isend(p_session, &reqs_out[i], gate, tag);
    }
  /* Wait for unlock ack's */
  for(i = 0; i < comm_size; ++i)
    {
      nm_sr_swait(p_session, &reqs_out[i]);
      nm_sr_rwait(p_session, &p_win->end_reqs[i]->request_nmad);
      nm_mpi_request_free(p_win->end_reqs[i]);
      /* Reset epoch */
      p_win->end_reqs[i]         = NULL;
      p_win->access[i].mode      = NM_MPI_WIN_UNUSED;
      p_win->access[i].nmsg      = 0;
      p_win->access[i].completed = 0;
      p_win->msg_count[i]        = 0;
    }
  nm_mpi_win_synchronize(p_win);
  for(i = 0; i < comm_size; ++i)
    {
      msync(((void**)p_win->p_base)[i], p_win->size[i], MS_SYNC);
    }
  return err;
}
