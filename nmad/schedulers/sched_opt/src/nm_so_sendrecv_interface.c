/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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

#ifdef PIOMAN
#include "nm_piom.h"
#endif

#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <pm2_common.h>

#include <nm_public.h>
#include "nm_so_private.h"
#include "nm_so_sendrecv_interface.h"
#include "nm_so_sendrecv_interface_private.h"
#include "nm_so_debug.h"
#include "nm_so_tracks.h"
#include "nm_log.h"

#define NM_SO_STATUS_SEND_COMPLETED  ((uint8_t)1)
#define NM_SO_STATUS_RECV_COMPLETED  ((uint8_t)2)

#ifdef PIOMAN
#define nm_so_cond_test(STATUS, BITMASK)       piom_cond_test((STATUS),   (BITMASK))
#define nm_so_cond_mask(STATUS, BITMASK)       piom_cond_mask((STATUS),   (BITMASK))
#define nm_so_cond_signal(STATUS, BITMASK)     piom_cond_signal((STATUS), (BITMASK))
#define nm_so_cond_wait(STATUS, BITMASK, CORE) piom_cond_wait((STATUS),   (BITMASK))
/** Post PIOMan poll requests (except when offloading PIO)
 */
static inline void nm_so_post_all(struct nm_core*p_core)
{
#if !defined(PIO_OFFLOAD)
  nm_piom_post_all(p_core);
#endif
}
/** Post PIOMan poll requests even when offloading PIO.
 * This avoids scheduling a tasklet on another lwp
 */
static inline void nm_so_post_all_force(struct nm_core*p_core)
{
  nm_piom_post_all(p_core);
}
#else /* PIOMAN */
static inline int  nm_so_cond_test(nm_so_status_t*status, uint8_t bitmask)
{
  return ((*status) & bitmask);
}
static inline void nm_so_cond_mask(nm_so_status_t*status, uint8_t bitmask)
{
  *status &= bitmask;
}
static inline void nm_so_cond_signal(nm_so_status_t*status, uint8_t bitmask)
{
  *status |= bitmask;
}
static inline void nm_so_cond_wait(nm_so_status_t*status, uint8_t bitmask, struct nm_core*p_core)
{
  if(status != NULL)
    {
      while(!((*status) & bitmask)) {
	nm_schedule(p_core);
      }
    }
}
static inline void nm_so_post_all(struct nm_core*p_core)
{ /* do nothing */ }
static inline void nm_so_post_all_force(struct nm_core*p_core)
{ /* do nothing */ }
#endif /* PIOMAN */

/** Transfer types for isend/irecv code factorization
 */
enum transfer_type {
  datatype_transfer,
  iov_transfer,
  contiguous_transfer,
  extended_transfer /* only for the sending side */
};

/** Status for basic receive requests.
 */
struct status {
  nm_so_status_t status;
  void *ref;
};

/** Status for any source receive requests.
 */
struct any_src_status {
  nm_so_status_t status;
  nm_gate_id_t gate_id;
  int is_first_request;
  void *ref;
};

/** Gate storage of the SR interface.
 */
struct nm_so_sr_gate {
  /* WARNING: better replace the following array by two separate
     bitmaps, to save space and avoid false sharing between send and
     recv operations. status[tag_id][seq] */
  struct status status[NM_SO_MAX_TAGS][NM_SO_PENDING_PACKS_WINDOW];
};

/** Structure to keep a reference on a user object.
 */
struct ref_link{
  void *ref;
  struct list_head link;
};
static p_tbx_memory_t nm_sr_ref_mem;
static struct list_head completed_rreq;

#define nm_l2ref(l) \
        ((struct ref_link *)((char *)(l) -\
         (unsigned long)(&((struct ref_link *)0)->link)))

/** Obscure macro for obtaining a pointer to the whole any_src_status
    structure from a pointer to its status field.
 */
#define outer_any_src_struct(p_status) \
	((struct any_src_status *)((uint8_t *)(p_status) - \
         (unsigned long)(&((struct any_src_status *)0)->status)))


/** Array of any_src_status structs. One entry per tag.
 */
static struct any_src_status any_src[NM_SO_MAX_TAGS];

#ifdef NMAD_QOS
/** Array of priority value for each tag
 */
static uint8_t priorities[NM_SO_MAX_TAGS];

/** Array of policy linked with corresponding tag
 */
static uint8_t policies[NM_SO_MAX_TAGS];
#endif /* NMAD_QOS */

static int nm_so_sr_init_gate(struct nm_gate *p_gate);
static int nm_so_sr_exit_gate(struct nm_gate *p_gate);
static int nm_so_sr_pack_success(struct nm_gate *p_gate,
                                 uint8_t tag, uint8_t seq);
static int nm_so_sr_unpack_success(struct nm_gate *p_gate,
                                   uint8_t tag, uint8_t seq,
                                   tbx_bool_t any_src);

/** Send/recv API.
 */
struct nm_so_interface_ops sr_ops = {
  .init_gate = nm_so_sr_init_gate,
  .exit_gate = nm_so_sr_exit_gate,
  .pack_success = nm_so_sr_pack_success,
  .unpack_success = nm_so_sr_unpack_success,
};

/* User interface */

int
nm_so_sr_init(struct nm_core *p_core,
	      struct nm_so_interface **p_so_interface)
{
  struct nm_so_interface *p_so_int = NULL;
  int i;

  NM_SO_SR_LOG_IN();
#ifndef CONFIG_SCHED_OPT
  NM_SO_SR_LOG_OUT();
  return -NM_EINVAL;
#endif

  p_so_int = TBX_MALLOC(sizeof(struct nm_so_interface));
  if(p_so_interface == NULL) {
    NM_SO_SR_LOG_OUT();
    return -NM_ENOMEM;
  }

  /* Initialize shorcuts pointers */
  p_so_int->p_core = p_core;
  p_so_int->p_so_sched = p_core->p_sched->sch_private;

  /* Fill-in scheduler callbacks */
  p_so_int->p_so_sched->current_interface = &sr_ops;

  *p_so_interface = p_so_int;

  tbx_malloc_init(&nm_sr_ref_mem, sizeof(struct ref_link), NM_SO_PENDING_PACKS_WINDOW,
                  "nmad/.../sched_opt/src/nm_so_send_recv_interface.c");

  for(i=0 ; i<NM_SO_MAX_TAGS ; i++) {
    any_src[i].is_first_request = 1;
#ifdef NMAD_QOS
    priorities[i] = NM_SO_NB_PRIORITIES - 1;
    policies[i] = NM_SO_POLICY_PRIORITY;
#endif /* NMAD_QOS */
  }

#ifdef NMAD_QOS
  priorities[0] = 0;
  priorities[1] = 0;
  priorities[2] = 0;
  priorities[3] = 0;
  priorities[4] = 0;
#endif /* NMAD_QOS */

  NM_SO_SR_LOG_OUT();
  return NM_ESUCCESS;
}

int
nm_so_sr_exit(struct nm_so_interface *p_so_interface)
{
  NM_SO_SR_LOG_IN();

  tbx_malloc_clean(nm_sr_ref_mem);

  TBX_FREE(p_so_interface);
  p_so_interface = NULL;

  NM_SO_SR_LOG_OUT();
  return NM_ESUCCESS;
}

int
nm_so_sr_request_init(nm_so_request *p_request) {
  if(p_request) {
    p_request->status = NULL;
    p_request->seq = -1;
    p_request->gate_id = -1;
  }
  return NM_ESUCCESS;
}

/* Send operations */
static int
__isend(struct nm_so_interface *p_so_interface,
        nm_gate_id_t gate_id, uint8_t tag,
        enum transfer_type sending_type,
        void *data, uint32_t len,
        tbx_bool_t is_completed,
        nm_so_request *p_request){
  struct nm_core *p_core = p_so_interface->p_core;
  struct nm_gate *p_gate = p_core->gate_array + gate_id;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sr_gate *p_sr_gate = p_so_gate->interface_private;
  uint8_t seq;
  int ret;

  NM_SO_SR_LOG_IN();
  nmad_lock();

  assert((sending_type != iov_transfer) || (len < NM_SO_PREALLOC_IOV_LEN));

  seq = p_so_gate->send_seq_number[tag]++;

  nm_so_cond_mask(&p_sr_gate->status[tag][seq].status, ~NM_SO_STATUS_SEND_COMPLETED);
  if(p_request) {
    p_request->status = &p_sr_gate->status[tag][seq].status;
    p_request->seq = seq;
    p_request->gate_id = gate_id;
  }

  switch(sending_type) {
  case datatype_transfer:
    NM_SO_SR_TRACE("Sending data described by a datatype\n");

    struct DLOOP_Segment *segp = data;
    ret = p_so_gate->strategy_receptacle.driver->pack_datatype(p_so_gate->strategy_receptacle._status,
							       p_gate, tag, seq, segp);
    break;

  case iov_transfer:
    NM_SO_SR_TRACE("Sending data described by an iovec\n");
    struct iovec *iov = data;
    ret = p_so_gate->strategy_receptacle.driver->packv(p_so_gate->strategy_receptacle._status,
						       p_gate, tag, seq, iov, len);
    break;

  case extended_transfer:
    NM_SO_SR_TRACE("Extendend sending\n");
    ret = p_so_gate->strategy_receptacle.driver->pack_extended(p_so_gate->strategy_receptacle._status,
							       p_gate, tag, seq, data, len, is_completed);
    break;

  case contiguous_transfer:
    NM_SO_SR_TRACE("Sending contiguous data\n");
    ret = p_so_gate->strategy_receptacle.driver->pack(p_so_gate->strategy_receptacle._status,
						      p_gate, tag, seq, data, len);
    break;

  default:
    TBX_FAILURE("Incorrect sending type.");
    break;
  }

  nm_so_post_all(p_core);
  nmad_unlock();
  NM_SO_SR_LOG_OUT();
  return ret;
}

int
nm_so_sr_isend(struct nm_so_interface *p_so_interface,
	       nm_gate_id_t gate_id, uint8_t tag,
	       void *data, uint32_t len,
	       nm_so_request *p_request){
  const enum transfer_type tt = contiguous_transfer;
  return __isend(p_so_interface, gate_id, tag, tt, data, len, tbx_false, p_request);
}

int
nm_so_sr_isend_extended(struct nm_so_interface *p_so_interface,
                        nm_gate_id_t gate_id, uint8_t tag,
                        void *data, uint32_t len,
                        tbx_bool_t is_completed,
                        nm_so_request *p_request)
{
  int ret;

  const struct nm_core *p_core = p_so_interface->p_core;
  const struct nm_gate *p_gate = p_core->gate_array + gate_id;
  const struct nm_so_gate *p_so_gate = p_gate->sch_private;
  if (!p_so_gate->strategy_receptacle.driver->pack_extended) {
    const enum transfer_type tt = contiguous_transfer;
    NM_DISPF("The current strategy does not provide a extended pack");
    ret = __isend(p_so_interface, gate_id, tag, tt, data, len, tbx_false, p_request);
  } else {
    const enum transfer_type tt = extended_transfer;
    ret = __isend(p_so_interface, gate_id, tag, tt, data, len, is_completed, p_request);
  }

  NM_SO_SR_LOG_OUT();
  return ret;
}

int
nm_so_sr_rsend(struct nm_so_interface *p_so_interface,
	       nm_gate_id_t gate_id, uint8_t tag,
	       void *data, uint32_t len,
	       nm_so_request *p_request){
  struct nm_core *p_core = p_so_interface->p_core;
  const enum transfer_type tt = contiguous_transfer;
  int ret = -NM_EAGAIN;

  NM_SO_SR_LOG_IN();

  ret = __isend(p_so_interface, gate_id, tag, tt, data, len, tbx_false, p_request);

  if (ret != NM_ESUCCESS)
    goto exit;

  if(len > NM_SO_MAX_SMALL) {

#ifdef PIOMAN
    /* TODO : use piom_cond_signal when an ACK arrives in order to have a *real* rsend  */
    nmad_lock();
    if(! nm_so_cond_test(p_request->status, NM_SO_STATUS_SEND_COMPLETED)) {
      nm_so_post_all(p_core);
    }
    nmad_unlock();
    nm_so_cond_wait(p_request->status, NM_SO_STATUS_SEND_COMPLETED, p_core);

#else
    struct nm_gate *p_gate = p_core->gate_array + gate_id;
    struct nm_so_gate *p_so_gate = p_gate->sch_private;
    nm_so_status_t *status = &(p_so_gate->status[tag][p_request->seq]);

    NM_SO_SR_TRACE("Waiting for status %p\n", status);
    nm_so_cond_wait(status, NM_SO_STATUS_ACK_HERE, p_core);

#endif
    NM_SO_SR_TRACE("Waiting for status %p completed\n", p_request->status);
  }

 exit:
  NM_SO_SR_LOG_OUT();
  return ret;
}

int
nm_so_sr_isend_iov(struct nm_so_interface *p_so_interface,
                   nm_gate_id_t gate_id, uint8_t tag,
                   struct iovec *iov, int nb_entries,
                   nm_so_request *p_request)

{
  const enum transfer_type tt = iov_transfer;
  return __isend(p_so_interface, gate_id, tag, tt, iov, nb_entries, tbx_false, p_request);
}

int
nm_so_sr_isend_datatype(struct nm_so_interface *p_so_interface,
                        nm_gate_id_t gate_id, uint8_t tag,
                        struct DLOOP_Segment *segp,
                        nm_so_request *p_request){
  const enum transfer_type tt = datatype_transfer;
  return __isend(p_so_interface, gate_id, tag, tt, segp, 0, tbx_false, p_request);
}

/** Test for the completion of a non blocking send request.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param request the request to check.
 *  @return The NM status.
 */
int
nm_so_sr_stest(struct nm_so_interface *p_so_interface,
	       nm_so_request request)
{
  int rc = NM_ESUCCESS;
  NM_SO_SR_LOG_IN();

  if(nm_so_cond_test(request.status, NM_SO_STATUS_SEND_COMPLETED))
    goto exit;

#ifdef PIOMAN
  /* Useless ? */
  __piom_check_polling(PIOM_POLL_AT_IDLE);
#else
  nm_schedule(p_so_interface->p_core);
#endif

  rc = (nm_so_cond_test(request.status, NM_SO_STATUS_SEND_COMPLETED)) ?
    NM_ESUCCESS : -NM_EAGAIN;

 exit:
  NM_SO_SR_LOG_OUT();
  return rc;
}

int
nm_so_sr_stest_range(struct nm_so_interface *p_so_interface,
		     nm_gate_id_t gate_id, uint8_t tag,
		     unsigned long seq_inf, unsigned long nb) {
  struct nm_core *p_core = p_so_interface->p_core;
  struct nm_gate *p_gate = p_core->gate_array + gate_id;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sr_gate *p_sr_gate = p_so_gate->interface_private;
  uint8_t seq = seq_inf;
  int ret = -NM_EAGAIN;
  nm_so_request request;

  NM_SO_SR_LOG_IN();

  while(nb--) {
    request.status = &p_sr_gate->status[tag][seq].status;
#warning "attention ! ca c'est une structure normalement !"
    request.seq = seq;
    ret = nm_so_sr_stest(p_so_interface, request);

    if (ret == -NM_EAGAIN) {
      NM_SO_SR_LOG_OUT();
      return ret;
    }
    seq++;
  }

  NM_SO_SR_LOG_OUT();
  return ret;
}

int nm_so_sr_flush(struct nm_so_interface *p_so_interface) {
  struct nm_core *p_core = p_so_interface->p_core;
  int ret = NM_EAGAIN;
  int i;

  for(i=0 ; i<p_core->nb_gates ; i++) {
    struct nm_gate *p_gate = p_core->gate_array + i;
    struct nm_so_gate *p_so_gate = p_gate->sch_private;
    struct nm_so_strategy_driver *strategy = p_so_gate->strategy_receptacle.driver;

    if (tbx_likely(strategy->flush != NULL)) {
      NM_SO_SR_TRACE("Flushing gate %d\n", i);
      ret = strategy->flush(p_so_gate->strategy_receptacle._status, p_gate);
    }
  }
  NM_SO_SR_LOG_OUT();
  return ret;
}

int
nm_so_sr_swait(struct nm_so_interface *p_so_interface,
	       nm_so_request request)
{
  NM_SO_SR_LOG_IN();

  if (p_so_interface->p_so_sched->strategy_driver->flush != NULL) {
    nm_so_sr_flush(p_so_interface);
  }

  if(! nm_so_cond_test(request.status, NM_SO_STATUS_SEND_COMPLETED)) {
    nmad_lock();
    nm_so_post_all_force(p_so_interface->p_core);
    nmad_unlock();
    nm_so_cond_wait(request.status, NM_SO_STATUS_SEND_COMPLETED, p_so_interface->p_core);
  }

  NM_SO_SR_LOG_OUT();
  return NM_ESUCCESS;
}

int
nm_so_sr_swait_range(struct nm_so_interface *p_so_interface,
		     nm_gate_id_t gate_id, uint8_t tag,
		     unsigned long seq_inf, unsigned long nb)
{
  struct nm_core *p_core = p_so_interface->p_core;
  struct nm_gate *p_gate = p_core->gate_array + gate_id;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sr_gate *p_sr_gate = p_so_gate->interface_private;
  uint8_t seq = seq_inf;
  nm_so_request request;

  NM_SO_SR_LOG_IN();

  while(nb--) {
    request.status = &p_sr_gate->status[tag][seq].status;
    request.seq = seq;
    request.gate_id = gate_id;
    nm_so_sr_swait(p_so_interface, request);

    seq++;
  }

  NM_SO_SR_LOG_OUT();
  return NM_ESUCCESS;
}

int
nm_so_sr_scancel(struct nm_so_interface *p_so_interface,
                 nm_so_request request)
{
  return NM_ENOTIMPL;
}

/* Receive operations */
static int
irecv_with_ref(struct nm_so_interface *p_so_interface,
               nm_gate_id_t gate_id, uint8_t tag,
               enum transfer_type reception_type,
               void *data_description, uint32_t len,
               nm_so_request *p_request,
               void *ref){
  struct nm_core *p_core = p_so_interface->p_core;
  int ret;

  NM_SO_SR_LOG_IN();
  if(gate_id == NM_SO_ANY_SRC) {

    if (any_src[tag].is_first_request == 0 && !(nm_so_cond_test(&any_src[tag].status, NM_SO_STATUS_RECV_COMPLETED))) {
      nm_so_request request;
      NM_SO_SR_TRACE("Irecv not completed for ANY_SRC tag=%d\n", tag);
      request.status = &any_src[tag].status;
      request.gate_id = -1;
      nmad_lock();
      nm_so_post_all(p_core);
      nmad_unlock();
      nm_so_sr_rwait(p_so_interface, request);
    }
    nmad_lock();
    any_src[tag].is_first_request = 0;
    any_src[tag].ref = ref;
    nm_so_cond_mask(&any_src[tag].status, ~NM_SO_STATUS_RECV_COMPLETED);
    if(p_request) {
      p_request->status = &any_src[tag].status;
      p_request->gate_id = -1;
    }

    NM_SO_SR_TRACE_LEVEL(3, "IRECV ANY_SRC: tag = %d, request = %p\n", tag, &any_src[tag].status);

    if(reception_type == datatype_transfer){
      struct DLOOP_Segment *segp = data_description;
      ret = __nm_so_unpack_datatype_any_src(p_core, tag, segp);

    } else if (reception_type == iov_transfer) {
      struct iovec *iov = data_description;
      ret = __nm_so_unpackv_any_src(p_core, tag, iov, len);

    } else if (reception_type == contiguous_transfer){
      ret = __nm_so_unpack_any_src(p_core, tag, data_description, len);
    } else {
      TBX_FAILURE("Reception type failed");
    }

    NM_SO_SR_TRACE_LEVEL(3, "IRECV ANY_SRC completed : tag = %d, request = %p\n", tag, &any_src[tag].status);

    nm_so_post_all(p_core);
    nmad_unlock();
    NM_SO_SR_LOG_OUT();
    return ret;

  } else {
    nmad_lock();
    struct nm_gate *p_gate = p_core->gate_array + gate_id;
    struct nm_so_gate *p_so_gate = p_gate->sch_private;
    struct nm_so_sr_gate *p_sr_gate = p_so_gate->interface_private;
    struct nm_so_strategy_driver *strategy = p_so_gate->strategy_receptacle.driver;
    uint8_t seq;

    if (tbx_likely(strategy->flush != NULL)) {
      nm_so_sr_flush(p_so_interface);
    }

    seq = p_so_gate->recv_seq_number[tag]++;

    nm_so_cond_mask(&p_sr_gate->status[tag][seq].status, ~NM_SO_STATUS_RECV_COMPLETED);
    if(p_request) {
      p_request->status = &p_sr_gate->status[tag][seq].status;
      p_request->seq = seq;
      p_request->gate_id = gate_id;
    }

    p_sr_gate->status[tag][seq].ref = ref;
    NM_SO_SR_TRACE_LEVEL(3, "IRECV: tag = %d, seq = %d, gate_id = %d request %p\n", tag, seq, gate_id,
                         &p_sr_gate->status[tag][seq].status);

    if (reception_type == datatype_transfer) {
      struct DLOOP_Segment *segp = data_description;
      ret = __nm_so_unpack_datatype(p_gate, tag, seq, segp);

    } else if (reception_type == iov_transfer) {
      struct iovec *iov = data_description;
      ret = __nm_so_unpackv(p_gate, tag, seq, iov, len);

    } else if (reception_type == contiguous_transfer) {
      ret = __nm_so_unpack(p_gate, tag, seq, data_description, len);

    } else {
      TBX_FAILURE("Reception type failed");
    }

    nm_so_post_all(p_core);
    nmad_unlock();
    NM_SO_SR_LOG_OUT();
    return ret;
  }
}


int
nm_so_sr_irecv_with_ref(struct nm_so_interface *p_so_interface,
                        nm_gate_id_t gate_id, uint8_t tag,
                        void *data, uint32_t len,
                        nm_so_request *p_request,
                        void *ref){
  const enum transfer_type tt = contiguous_transfer;
  return irecv_with_ref(p_so_interface, gate_id, tag, tt, data, len, p_request, ref);
}

int
nm_so_sr_irecv(struct nm_so_interface *p_so_interface,
               nm_gate_id_t gate_id, uint8_t tag,
               void *data, uint32_t len,
               nm_so_request *p_request){
  const enum transfer_type tt = contiguous_transfer;
  return irecv_with_ref(p_so_interface, gate_id, tag, tt, data, len, p_request, NULL);
}

int
nm_so_sr_irecv_iov_with_ref(struct nm_so_interface *p_so_interface,
                            nm_gate_id_t gate_id, uint8_t tag,
                            struct iovec *iov, int nb_entries,
                            nm_so_request *p_request, void *ref){
  const enum transfer_type tt = iov_transfer;
  return irecv_with_ref(p_so_interface, gate_id, tag, tt, iov, nb_entries, p_request, ref);
}


int nm_so_sr_irecv_iov(struct nm_so_interface *p_so_interface,
                       nm_gate_id_t gate_id, uint8_t tag,
                       struct iovec *iov, int nb_entries,
                       nm_so_request *p_request){
  const enum transfer_type tt = iov_transfer;
  return irecv_with_ref(p_so_interface, gate_id, tag, tt, iov, nb_entries, p_request, NULL);
}

int
nm_so_sr_irecv_datatype_with_ref(struct nm_so_interface *p_so_interface,
                                 nm_gate_id_t gate_id, uint8_t tag,
                                 struct DLOOP_Segment *segp,
                                 nm_so_request *p_request,
                                 void *ref) {
  const enum transfer_type tt = datatype_transfer;
  return irecv_with_ref(p_so_interface, gate_id, tag, tt, segp, 0, p_request, ref);
}

int nm_so_sr_irecv_datatype(struct nm_so_interface *p_so_interface,
                            nm_gate_id_t gate_id, uint8_t tag,
                            struct DLOOP_Segment *segp,
                            nm_so_request *p_request){
  const enum transfer_type tt = datatype_transfer;
  return irecv_with_ref(p_so_interface, gate_id, tag, tt, segp, 0, p_request, NULL);
}

/** Test for the completion of a non blocking receive request.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param request the request to check.
 *  @return The NM status.
 */
int
nm_so_sr_rtest(struct nm_so_interface *p_so_interface,
	       nm_so_request request)
{
  int rc = NM_ESUCCESS;
  NM_SO_SR_LOG_IN();

  NM_SO_SR_TRACE("request %p completion = %d\n", request.status,
  		 nm_so_cond_test(request.status, NM_SO_STATUS_RECV_COMPLETED));

  if(nm_so_cond_test(request.status, NM_SO_STATUS_RECV_COMPLETED)) {
    rc = NM_ESUCCESS;
    goto exit;
  }

#ifdef PIOMAN
  /* Useless ? */
  __piom_check_polling(PIOM_POLL_AT_IDLE);
#else
  nm_schedule(p_so_interface->p_core);
#endif

  rc = (nm_so_cond_test(request.status, NM_SO_STATUS_RECV_COMPLETED)) ?
    NM_ESUCCESS : -NM_EAGAIN;

 exit:
  NM_SO_SR_LOG_OUT();
  return rc;
}

int
nm_so_sr_rtest_range(struct nm_so_interface *p_so_interface,
		     nm_gate_id_t gate_id, uint8_t tag,
		     unsigned long seq_inf, unsigned long nb) {
  struct nm_core *p_core = p_so_interface->p_core;
  struct nm_gate *p_gate = p_core->gate_array + gate_id;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sr_gate *p_sr_gate = p_so_gate->interface_private;
  uint8_t seq = seq_inf;
  int ret = -NM_EAGAIN;
  nm_so_request request;

  NM_SO_SR_LOG_IN();

  while(nb--) {
    request.status = &p_sr_gate->status[tag][seq].status;
    request.seq = seq;
    ret = nm_so_sr_rtest(p_so_interface, request);

    if (ret == -NM_EAGAIN) {
      NM_SO_SR_LOG_OUT();
      return ret;
    }
    seq++;
  }

  NM_SO_SR_LOG_OUT();
  return ret;
}

int
nm_so_sr_rwait(struct nm_so_interface *p_so_interface,
	       nm_so_request request)
{
  NM_SO_SR_LOG_IN();

  if (p_so_interface->p_so_sched->strategy_driver->flush != NULL) {
    nm_so_sr_flush(p_so_interface);
  }

  NM_SO_SR_TRACE("request %p completion = %d\n", request.status,
		 nm_so_cond_test(request.status, NM_SO_STATUS_RECV_COMPLETED));
  if(!nm_so_cond_test(request.status, NM_SO_STATUS_RECV_COMPLETED)) {
    nmad_lock();
    nm_so_post_all_force(p_so_interface->p_core);
    nmad_unlock();
    nm_so_cond_wait(request.status, NM_SO_STATUS_RECV_COMPLETED, p_so_interface->p_core);
  }

  NM_SO_SR_TRACE("request %p completed\n", request.status);
  NM_SO_SR_LOG_OUT();
  return NM_ESUCCESS;
}

int
nm_so_sr_get_size(struct nm_so_interface *p_so_interface,
                  nm_so_request *request, uint8_t tag,
                  int *size) {
  struct nm_core *p_core = p_so_interface->p_core;;
  struct nm_gate *p_gate = NULL;
  struct nm_so_gate *p_so_gate = NULL;
  struct nm_so_sched *p_so_sched =  NULL;
  int is_any_source = 0;

  if (request->gate_id == -1) {
    struct any_src_status *p_status = outer_any_src_struct(request->status);
    request->gate_id = p_status->gate_id;
    is_any_source = 1;
  }

  p_gate = p_core->gate_array + request->gate_id;
  p_so_gate = p_gate->sch_private;
  p_so_sched =  p_so_gate->p_so_sched;

  if (is_any_source) {
    *size = p_so_sched->any_src[tag].expected_len;
  }
  else {
    *size = p_so_gate->recv[tag][request->seq].unpack_here.expected_len;
  }

  NM_SO_SR_TRACE("SIZE: tag = %d, seq = %d, gate = %ld --> %d\n", tag, request->seq, request->gate_id, *size);

  return NM_ESUCCESS;
}

int
nm_so_sr_recv_source(struct nm_so_interface *p_so_interface TBX_UNUSED,
                     nm_so_request request, nm_gate_id_t *gate_id)
{
#ifdef PIOMAN
  struct any_src_status *p_status =
    struct_up(request.status, struct any_src_status, status);
#else
  struct any_src_status *p_status = outer_any_src_struct(request.status);
#endif

  NM_SO_SR_LOG_IN();

  if(gate_id)
    *gate_id = p_status->gate_id;

  NM_SO_SR_LOG_OUT();
  return NM_ESUCCESS;
}

int
nm_so_sr_probe(struct nm_so_interface *p_so_interface,
               nm_gate_id_t gate_id, nm_gate_id_t *out_gate_id, uint8_t tag)
{
  struct nm_core *p_core = p_so_interface->p_core;
  int i;

  NM_SO_SR_LOG_IN();

  if (gate_id == NM_SO_ANY_SRC) {
    for(i = 0; i < p_core->nb_gates; i++) {
      struct nm_gate *p_gate = p_core->gate_array + i;
      struct nm_so_gate *p_so_gate = p_gate->sch_private;
      uint8_t seq = p_so_gate->recv_seq_number[tag];
      volatile uint8_t *status = &(p_so_gate->status[tag][seq]);
      if ((*status & NM_SO_STATUS_PACKET_HERE) || (*status & NM_SO_STATUS_RDV_HERE)) {
	*out_gate_id = i ;
        NM_SO_SR_LOG_OUT();
        return NM_ESUCCESS;
      }
    }
  }
  else {
    struct nm_gate *p_gate = p_core->gate_array + gate_id;
    struct nm_so_gate *p_so_gate = p_gate->sch_private;
    uint8_t seq = p_so_gate->recv_seq_number[tag];
    volatile uint8_t *status = &(p_so_gate->status[tag][seq]);
    if ((*status & NM_SO_STATUS_PACKET_HERE) || (*status & NM_SO_STATUS_RDV_HERE)) {
      *out_gate_id = gate_id;
      NM_SO_SR_LOG_OUT();
      return NM_ESUCCESS;
    }
  }

  nmad_lock();
  // Nothing on none of the gates
  for(i = 0; i < p_core->nb_gates; i++) {
    nm_so_refill_regular_recv(&p_core->gate_array[i]);
  }
  nmad_unlock();
#ifndef PIOMAN
  nm_schedule(p_core);
#endif
  *out_gate_id = NM_SO_ANY_SRC;
  NM_SO_SR_LOG_OUT();
  return -NM_EAGAIN;
}

int
nm_so_sr_rwait_range(struct nm_so_interface *p_so_interface,
		     nm_gate_id_t gate_id, uint8_t tag,
		     unsigned long seq_inf, unsigned long nb)
{
  struct nm_core *p_core = p_so_interface->p_core;
  struct nm_gate *p_gate = p_core->gate_array + gate_id;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sr_gate *p_sr_gate = p_so_gate->interface_private;
  uint8_t seq = seq_inf;
  nm_so_request request;

  NM_SO_SR_LOG_IN();

  while(nb--) {
    request.status = &p_sr_gate->status[tag][seq].status;
    request.seq = seq;
    request.gate_id = gate_id;

    NM_SO_SR_TRACE_LEVEL(3, "asking for request %p (tag %d seq %d gate_id %d) completion\n", &p_sr_gate->status[tag][seq].status, tag, seq, gate_id);

    nm_so_sr_rwait(p_so_interface, request);
    seq++;
  }

  NM_SO_SR_LOG_OUT();
  return NM_ESUCCESS;
}

int
nm_so_sr_recv_success(struct nm_so_interface *p_so_interface, void **ref){
  struct nm_core *p_core = p_so_interface->p_core;

  nm_schedule(p_core);

  if(!list_empty(&completed_rreq)){
    struct ref_link *ref_link= nm_l2ref(completed_rreq.next);
    list_del(completed_rreq.next);

    *ref = ref_link->ref;

    tbx_free(nm_sr_ref_mem, ref_link);

  } else {
    *ref = NULL;
  }

  return NM_ESUCCESS;
}

int
nm_so_sr_rcancel(struct nm_so_interface *p_so_interface,
                 nm_so_request request)
{
  return NM_ENOTIMPL;
}

unsigned long
nm_so_sr_get_current_send_seq(struct nm_so_interface *p_so_interface,
			      nm_gate_id_t gate_id, uint8_t tag)
{
  struct nm_core *p_core = p_so_interface->p_core;
  struct nm_gate *p_gate = p_core->gate_array + gate_id;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  unsigned long ret;

  NM_SO_SR_LOG_IN();

  ret = p_so_gate->send_seq_number[tag];
  NM_SO_SR_LOG_OUT();
  return ret;
}

unsigned long
nm_so_sr_get_current_recv_seq(struct nm_so_interface *p_so_interface,
			      nm_gate_id_t gate_id, uint8_t tag)
{
  struct nm_core *p_core = p_so_interface->p_core;
  struct nm_gate *p_gate = p_core->gate_array + gate_id;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  unsigned long ret;

  NM_SO_SR_LOG_IN();

  ret = p_so_gate->recv_seq_number[tag];
  NM_SO_SR_LOG_OUT();
  return ret;
}

/* Callback functions */
/** Initialize the gate storage for the SR interface.
 *  @param p_gate the pointer to the gate object.
 *  @return The NM status.
 */
static
int nm_so_sr_init_gate(struct nm_gate *p_gate)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sr_gate *p_sr_gate;

  NM_SO_SR_LOG_IN();

  p_sr_gate = TBX_MALLOC(sizeof(struct nm_so_sr_gate));
  if(p_sr_gate == NULL) {
    NM_SO_SR_LOG_OUT();
    return -NM_ENOMEM;
  }

  INIT_LIST_HEAD(&completed_rreq);

  p_so_gate->interface_private = p_sr_gate;

#ifdef PIOMAN
  int i,j;
  for(i=0;i<NM_SO_MAX_TAGS;i++)
   for(j=0;j<NM_SO_PENDING_PACKS_WINDOW;j++)
     piom_cond_init(&p_sr_gate->status[i][j].status,0);
  for(i=0;i<NM_SO_MAX_TAGS;i++)
     piom_cond_init(&any_src[i].status,0);
#endif

  NM_SO_SR_LOG_OUT();
  return NM_ESUCCESS;
}

/** Clean-up the gate storage for the SR interface.
 *  @param p_gate the pointer to the gate object.
 *  @return The NM status.
 */
static
int nm_so_sr_exit_gate(struct nm_gate *p_gate)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sr_gate *p_sr_gate = p_so_gate->interface_private;

  NM_SO_SR_LOG_IN();

  TBX_FREE(p_sr_gate);
  p_so_gate->interface_private = NULL;
  NM_SO_SR_LOG_OUT();
  return NM_ESUCCESS;
}

/** Check the status for a send request (gate,tag,seq).
 *  @param p_gate the pointer to the gate object.
 *  @param tag the message tag.
 *  @param seq the fragment sequence number.
 *  @return The NM status.
 */
static
int nm_so_sr_pack_success(struct nm_gate *p_gate,
                          uint8_t tag, uint8_t seq)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sr_gate *p_sr_gate = p_so_gate->interface_private;

  NM_SO_SR_LOG_IN();
  NM_SO_SR_TRACE("data sent for request = %p - tag %d , seq %d\n", &p_sr_gate->status[tag][seq].status, tag, seq);

  nm_so_cond_signal(&p_sr_gate->status[tag][seq].status, NM_SO_STATUS_SEND_COMPLETED);

  NM_SO_SR_LOG_OUT();
  return NM_ESUCCESS;
}

/** Check the status for a receive request (gate/is_any_src,tag,seq).
 *  @param p_gate the pointer to the gate object or @c NULL if @p is_any_src is @c true.
 *  @param tag the message tag.
 *  @param seq the fragment sequence number (ignored if @p is_any_src is @c true).
 *  @param is_any_src whether to check for a specific gate or for any gate.
 *  @return The NM status.
 */
static
int nm_so_sr_unpack_success(struct nm_gate *p_gate,
                            uint8_t tag, uint8_t seq,
                            tbx_bool_t is_any_src)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sr_gate *p_sr_gate = p_so_gate->interface_private;
  void *ref = NULL;

  NM_SO_SR_LOG_IN();

  NM_SO_SR_TRACE("data received for request = %p (any_src request %p) - tag %d, seq %d\n", &p_sr_gate->status[tag][seq].status, is_any_src ? &any_src[tag].status : NULL, tag, seq);

  nm_so_cond_signal(&p_sr_gate->status[tag][seq].status, NM_SO_STATUS_RECV_COMPLETED);

  if(is_any_src) {
    any_src[tag].gate_id = p_gate->id;
    nm_so_cond_signal(&any_src[tag].status, NM_SO_STATUS_RECV_COMPLETED);

    ref = any_src[tag].ref;
  } else {
    ref = p_sr_gate->status[tag][seq].ref;
  }

  if(ref){
    struct ref_link *ref_link = tbx_malloc(nm_sr_ref_mem);

    ref_link->ref = ref;

    list_add_tail(&ref_link->link, &completed_rreq);
  }

  NM_SO_SR_LOG_OUT();
  return NM_ESUCCESS;
}

#ifdef NMAD_QOS
void
nm_so_set_priority(uint8_t tag, uint8_t priority)
{
  if(tag < NM_SO_MAX_TAGS && priority < NM_SO_NB_PRIORITIES)
    priorities[tag] = priority;
}

uint8_t
nm_so_get_priority(uint8_t tag)
{
  if (tag < NM_SO_MAX_TAGS)
    return priorities[tag];

  /* Undefined priority */
  return NM_SO_NB_PRIORITIES - 1;
}

void
nm_so_set_policy(uint8_t tag, uint8_t policy)
{
  if(tag < NM_SO_MAX_TAGS && policy < NM_SO_NB_POLICIES)
    policies[tag] = policy;
}

uint8_t
nm_so_get_policy(uint8_t tag)
{
  if (tag < NM_SO_MAX_TAGS)
    return policies[tag];

  /* Undefined policy */
  return NM_SO_POLICY_FIFO;
}
#endif /* NMAD_QOS */

int
nm_so_sr_progress(struct nm_so_interface *p_so_interface)
{
  /* We assume that PIOMan makes the communications progress */
#ifndef PIOMAN
  NM_SO_SR_LOG_IN();
  struct nm_core *p_core = p_so_interface->p_core;
  nm_schedule(p_core);
  NM_SO_SR_LOG_OUT();
#endif

  return NM_ESUCCESS;
}

int
nm_so_sr_req_test(struct nm_so_interface *p_so_interface TBX_UNUSED,
                  nm_so_request request)
{
  NM_SO_SR_LOG_IN();
  int rc = nm_so_cond_test(request.status, NM_SO_STATUS_RECV_COMPLETED)?
    NM_ESUCCESS : -NM_EAGAIN;
  NM_SO_SR_LOG_OUT();
  return rc;
}
