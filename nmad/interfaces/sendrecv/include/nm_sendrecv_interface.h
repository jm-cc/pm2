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

#ifndef NM_SENDRECV_INTERFACE_H
#define NM_SENDRECV_INTERFACE_H

/** \defgroup sr_interface SendRecv interface
 *
 * This is the NM SendRecv interface
 *
 * @{
 */

#include <pm2_list.h>
#include <nm_public.h>
#ifdef PIOMAN
#include "pioman.h"
#endif
/*#include <ccs_public.h>*/
struct CCSI_Segment;

#include <sys/uio.h>


typedef uint8_t nm_sr_status_t;

/** a posted send has completed */
#define NM_SR_STATUS_SEND_COMPLETED  ((nm_sr_status_t)0x01)
/** a posted recv has completed */
#define NM_SR_STATUS_RECV_COMPLETED  ((nm_sr_status_t)0x02)
/** recv operation was canceled */
#define NM_SR_STATUS_RECV_CANCELLED  ((nm_sr_status_t)0x04)
/** a send is posted */
#define NM_SR_STATUS_SEND_POSTED     ((nm_sr_status_t)0x08)
/** a recv is posted */
#define NM_SR_STATUS_RECV_POSTED     ((nm_sr_status_t)0x10)

/** events for nm_sr_monitor() */
typedef enum
  {
    /** an unexpected packet has arrived. Post a recv to get data
     */
    NM_SR_EVENT_RECV_UNEXPECTED = ((nm_sr_status_t)0x80),
    NM_SR_EVENT_RECV_COMPLETED  = NM_SR_STATUS_RECV_COMPLETED,
    NM_SR_EVENT_SEND_COMPLETED  = NM_SR_STATUS_SEND_COMPLETED,
    NM_SR_EVENT_RECV_CANCELLED  = NM_SR_STATUS_RECV_CANCELLED
  } nm_sr_event_t;


#ifdef PIOMAN
/** a status with synchronization- PIOMan version */
typedef piom_cond_t nm_sr_cond_t;
#else /* PIOMAN */
/** a status with synchronization- PIOMan-less version */
typedef volatile nm_sr_status_t nm_sr_cond_t;
#endif /* PIOMAN */

/** a sendrecv request object. Supposedly opaque for applications.*/
typedef struct nm_sr_request_s nm_sr_request_t;

/** information field for sendrecv events */
typedef union
{
  struct
  {
    nm_gate_t p_gate;
    nm_tag_t tag;
    uint32_t len;
  } recv_unexpected;
  struct
  {
    nm_sr_request_t*p_request;
    nm_gate_t p_gate;
  } recv_completed;
  struct
  {
    nm_sr_request_t*p_request;
  } send_completed;
} nm_sr_event_info_t;

/** notification function for sendrecv events */
typedef void (*nm_sr_event_notifier_t)(nm_sr_event_t event, const nm_sr_event_info_t*event_info);

typedef struct
{
  nm_sr_event_t mask;
  nm_sr_event_notifier_t notifier;
} nm_sr_event_monitor_t;

#define NM_SR_EVENT_MONITOR_NULL ((nm_sr_event_monitor_t){ .mask = 0, .notifier = NULL })

PUK_VECT_TYPE(nm_sr_event_monitor, nm_sr_event_monitor_t)

/** internal defintion of the sendrecv request */
struct nm_sr_request_s
{
  nm_sr_cond_t status;
  uint8_t seq;
  nm_gate_t p_gate;
  nm_tag_t tag;
  nm_sr_event_monitor_t monitor;
  void *ref;
  struct list_head _link;
};


/** Transfer types for isend/irecv */
enum nm_sr_transfer_type
  {
    nm_sr_datatype_transfer,
    nm_sr_iov_transfer,
    nm_sr_contiguous_transfer
  };
typedef enum nm_sr_transfer_type nm_sr_transfer_type_t;


/** Initialize the send/receive interface.
 *
 *  @param p_core a pointer to the NM core object.
 *  @return The NM status.
 */
extern int nm_sr_init(nm_core_t p_core);

/** Shutdown the send/receive interface.
 *  @param p_core a pointer to the NM core object
 *  @return The NM status.
 */
extern int nm_sr_exit(nm_core_t p_core);

#ifdef PIOMAN
/** Attach a piom_sem_t to a request. This piom_sem_t is woken 
 *  up when the request is completed.
 *  @param p_request a pointer to a NM/SO request to be filled.
 *  @param p_sem a pointer to the piom_sem_t to attach.
 *  @return The NM status.
 */
extern int nm_sr_attach(nm_sr_request_t *p_request, piom_sh_sem_t *p_sem);
#endif

extern void nm_sr_debug_init(int* argc, char** argv, int debug_flags);

/** Post a non blocking send request- most generic version.
 *  @param p_core a pointer to the NM core object.
 *  @param p_gate a pointer to the destination gate.
 *  @param tag the message tag.
 *  @param sending_type the type of data layout
 *  @param data the data fragment pointer, interpretation depends on sending_type.
 *  @param len the data fragment length.
 *  @param p_request a pointer to a NM request to be filled.
 *  @return The NM status.
 */
extern int nm_sr_isend_generic(struct nm_core *p_core,
			       nm_gate_t p_gate, nm_tag_t tag,
			       nm_sr_transfer_type_t sending_type,
			       const void *data, uint32_t len,
			       nm_sr_request_t *p_request,
			       void*ref);

/** Post a non blocking send request.
 *  @param p_core a pointer to the NM core object.
 *  @param p_gate a pointer to the destination gate.
 *  @param tag the message tag.
 *  @param data the data fragment pointer.
 *  @param len the data fragment length.
 *  @param p_request a pointer to a NM request to be filled.
 *  @return The NM status.
 */
static inline int nm_sr_isend(nm_core_t p_core,
			      nm_gate_t p_gate, nm_tag_t tag,
			      const void *data, uint32_t len,
			      nm_sr_request_t *p_request)
{
  return nm_sr_isend_generic(p_core, p_gate, tag, nm_sr_contiguous_transfer, data, len, p_request, NULL);
}

static inline int nm_sr_isend_with_ref(nm_core_t p_core,
				       nm_gate_t p_gate, nm_tag_t tag,
				       const void *data, uint32_t len,
				       nm_sr_request_t *p_request,
				       void*ref)
{
  return nm_sr_isend_generic(p_core, p_gate, tag, nm_sr_contiguous_transfer, data, len, p_request, ref);
}

/** Post a ready send request, i.e. assume the receiver is ready
 *  thus removing the need for a rdv.
 *  @param p_core a pointer to the NM core object
 *  @param p_gate a pointer to the destination gate.
 *  @param tag the message tag.
 *  @param data the data fragment pointer.
 *  @param len the data fragment length.
 *  @param p_request a pointer to a NM/SO request to be filled.
 *  @return The NM status.
 */
extern int nm_sr_rsend(nm_core_t p_core,
		       nm_gate_t p_gate, nm_tag_t tag,
		       const void *data, uint32_t len,
		       nm_sr_request_t *p_request);

/** Test for the completion of a non blocking send request.
 *  @param p_core a pointer to the NM core object
 *  @param p_gate a pointer to the destination gate.
 *  @param tag the message tag.
 *  @param iov
 *  @param nb_entries
 *  @param p_request a pointer to a NM/SO request to be filled.
 *  @return The NM status.
 */
static inline int nm_sr_isend_iov(nm_core_t p_core,
				  nm_gate_t p_gate, nm_tag_t tag,
				  const struct iovec *iov, int nb_entries,
				  nm_sr_request_t *p_request)
{
  return nm_sr_isend_generic(p_core, p_gate, tag, nm_sr_iov_transfer, iov, nb_entries, p_request, NULL);

}

static inline int nm_sr_isend_iov_with_ref(nm_core_t p_core,
					   nm_gate_t p_gate, nm_tag_t tag,
					   const struct iovec *iov, int nb_entries,
					   nm_sr_request_t *p_request,
					   void*ref)
{
  return nm_sr_isend_generic(p_core, p_gate, tag, nm_sr_iov_transfer, iov, nb_entries, p_request, ref);

}

/** Test for the completion of a non blocking send request.
 *  @param p_core a pointer to the NM core object
 *  @param p_gate a pointer to the destination gate.
 *  @param tag the message tag.
 *  @param segp
 *  @param p_request the request to check.
 *  @return The NM status.
 */
static inline int nm_sr_isend_datatype(nm_core_t p_core,
				       nm_gate_t p_gate, nm_tag_t tag,
				       const struct CCSI_Segment *segp,
				       nm_sr_request_t *p_request)
{
  return nm_sr_isend_generic(p_core, p_gate, tag, nm_sr_datatype_transfer, segp, 0, p_request, NULL);

}

extern int nm_sr_stest(nm_core_t p_core,
		       nm_sr_request_t *p_request);

/** Calls the scheduler
 *  @param p_core a pointer to the NM core object
 *  @return The NM status.
 */
extern int nm_sr_progress(nm_core_t p_core);

/** Wait for the completion of a non blocking send request.
 *  @param p_core a pointer to the NM core object
 *  @param request the request to check.
 *  @return The NM status.
 */
extern int nm_sr_swait(nm_core_t p_core,
		       nm_sr_request_t  *p_request);

/** Cancel a emission request.
 *  @param p_core a pointer to the NM core object
 *  @param request the request to cancel.
 *  @return The NM status.
 */
extern int nm_sr_scancel(nm_core_t p_core,
			 nm_sr_request_t *p_request);


extern int nm_sr_irecv_generic(nm_core_t p_core,
			       nm_gate_t p_gate, nm_tag_t tag,
			       nm_sr_transfer_type_t reception_type,
			       void *data_description, uint32_t len,
			       nm_sr_request_t *p_request,
			       void *ref);

/** Post a non blocking receive request.
 *  @param p_core a pointer to the NM core object
 *  @param p_gate a pointer to the destination gate.
 *  @param tag the message tag.
 *  @param data the data fragment pointer.
 *  @param len the data fragment length.
 *  @param p_request a pointer to a NM/SO request to be filled.
 *  @return The NM status.
 */
static inline int nm_sr_irecv(nm_core_t p_core,
			      nm_gate_t p_gate, nm_tag_t tag,
			      void *data, uint32_t len,
			      nm_sr_request_t *p_request)
{
  return nm_sr_irecv_generic(p_core, p_gate, tag, nm_sr_contiguous_transfer, data, len, p_request, NULL);
}

/** Test for the completion of a non blocking receive request.
 *  @param p_core a pointer to the NM core object
 *  @param p_gate a pointer to the destination gate.
 *  @param tag the message tag.
 *  @param data the data fragment pointer.
 *  @param len the data fragment length.
 *  @param p_request a pointer to a NM/SO request to be filled.
 *  @param ref
 *  @return The NM status.
 */
static inline int nm_sr_irecv_with_ref(nm_core_t p_core,
				       nm_gate_t p_gate, nm_tag_t tag,
				       void *data, uint32_t len,
				       nm_sr_request_t *p_request,
				       void *ref)
{
  return nm_sr_irecv_generic(p_core, p_gate, tag, nm_sr_contiguous_transfer, data, len, p_request, ref);
}

static inline int nm_sr_irecv_iov(nm_core_t p_core,
				  nm_gate_t p_gate, nm_tag_t tag,
				  struct iovec *iov, int nb_entries,
				  nm_sr_request_t *p_request)
{
  return nm_sr_irecv_generic(p_core, p_gate, tag, nm_sr_iov_transfer, iov, nb_entries, p_request, NULL);
}

static inline int nm_sr_irecv_iov_with_ref(nm_core_t p_core,
					   nm_gate_t p_gate, nm_tag_t tag,
					   struct iovec *iov, int nb_entries,
					   nm_sr_request_t *p_request, void *ref)
{
  return nm_sr_irecv_generic(p_core, p_gate, tag, nm_sr_iov_transfer, iov, nb_entries, p_request, ref);
}

static inline int nm_sr_irecv_datatype(nm_core_t p_core,
				nm_gate_t p_gate, nm_tag_t tag,
				struct CCSI_Segment *segp,
				nm_sr_request_t *p_request)
{
  return nm_sr_irecv_generic(p_core, p_gate, tag, nm_sr_datatype_transfer, segp, 0, p_request, NULL);
}


static inline int nm_sr_irecv_datatype_with_ref(struct nm_core *p_core,
						nm_gate_t p_gate, nm_tag_t tag,
						struct CCSI_Segment *segp,
						nm_sr_request_t *p_request,
						void *ref)
{
  return nm_sr_irecv_generic(p_core, p_gate, tag, nm_sr_datatype_transfer, segp, 0, p_request, ref);
}


/** Test for the completion of a non blocking receive request.
 *  @param p_core a pointer to the NM core object
 *  @param request the request to check.
 *  @return The NM status.
 */
extern int nm_sr_rtest(nm_core_t p_core,
		       nm_sr_request_t *p_request);


/** Wait for the completion of a non blocking receive request.
 *  @param p_core a pointer to the NM core object
 *  @param request the request to check.
 *  @return The NM status.
 */
extern int nm_sr_rwait(nm_core_t p_core,
		       nm_sr_request_t *p_request);

/** Cancel a reception request.
 *  @param p_core a pointer to the NM core object
 *  @param request the request to cancel.
 *  @return The NM status.
 */
extern int nm_sr_rcancel(nm_core_t p_core,
			 nm_sr_request_t *p_request);

/** Retrieve the pkt source of a complete any source receive request.
 *  @param p_core a pointer to the NM core object
 *  @param request the request to check.
 *  @param p_gate a pointer to the destination gate.
 *  @return The NM status.
 */
extern int nm_sr_recv_source(nm_core_t p_core,
			     nm_sr_request_t *p_request, nm_gate_t *p_gate);

/** Unblockingly check if a packet is available for extraction on the (gate,tag) pair .
 *  @param p_core a pointer to the NM core object
 *  @param p_gate a pointer to the destination gate.
 *  @param p_out_gate a pointer to the destination gate.
 *  @param tag the message tag.
 *  @return The NM status.
 */
extern int nm_sr_probe(nm_core_t p_core,
		       nm_gate_t p_gate, nm_gate_t *p_out_gate, nm_tag_t tag);

/** monitors sendrecv events globally */
extern int nm_sr_monitor(nm_core_t p_core, nm_sr_event_t mask, nm_sr_event_notifier_t notifier);

/** monitors sendrecv events for a given request */
extern int nm_sr_request_monitor(nm_core_t p_core, nm_sr_request_t *p_request,
				 nm_sr_event_t mask, nm_sr_event_notifier_t notifier);


/** Poll for any completed recv request (any source, any tag).
 * @note Only nm_sr_recv*_with_ref functions (with ref != NULL) generate such an event.
 */
extern int nm_sr_recv_success(nm_core_t p_core, nm_sr_request_t **out_req);

/** Poll for any completed send request.
 * @note Only nm_sr_isend*_with_ref functions (with ref != NULL) generate such an event.
 */
extern int nm_sr_send_success(nm_core_t p_core, nm_sr_request_t **out_req);

/** Returns the received size of the message with the specified
 *  request.
 *  @param p_core a pointer to the NM core object
 *  @param request the request to check.
 *  @param size returns the size of the message
 *  @return The NM status.
 */
extern int nm_sr_get_size(nm_core_t p_core,
			  nm_sr_request_t *request,
			  size_t *size);

/** Retrieve the 'ref' from a completed receive request.
 */
static inline int nm_sr_get_ref(nm_core_t p_core,
				nm_sr_request_t *p_request,
				void**ref)
{
  *ref = p_request->ref;
  return NM_ESUCCESS;
}

/** Retrieve the tag from a sendreceive request.
 */
static inline int nm_sr_get_tag(nm_core_t p_core,
				nm_sr_request_t *p_request,
				nm_tag_t*tag)
{
  *tag = p_request->tag;
  return NM_ESUCCESS;
}


/* @} */

#endif /* NM_SENDRECV_INTERFACE_H */

