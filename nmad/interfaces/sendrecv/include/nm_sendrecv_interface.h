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

/** @defgroup sr_interface SendRecv interface
 *
 * This is the nmad SendRecv interface, the main interface to send and receive packets.
 *
 * @{
 */

/* don't include pm2_common.h or tbx.h here. They are not needed and not ISO C compliant */
#include <nm_public.h>
#include <nm_session_interface.h>
#include <nm_core_interface.h>
#include <sys/uio.h>


/** a request status or event */
typedef uint8_t nm_sr_status_t;

/** a posted send has completed */
#define NM_SR_STATUS_SEND_COMPLETED  ((nm_sr_status_t)NM_STATUS_PACK_COMPLETED)
/** a posted recv has completed */
#define NM_SR_STATUS_RECV_COMPLETED  ((nm_sr_status_t)NM_STATUS_UNPACK_COMPLETED)
/** recv operation was canceled */
#define NM_SR_STATUS_RECV_CANCELLED  ((nm_sr_status_t)NM_STATUS_UNPACK_CANCELLED)
/** a send is posted */
#define NM_SR_STATUS_SEND_POSTED     ((nm_sr_status_t)NM_STATUS_PACK_POSTED)
/** a recv is posted */
#define NM_SR_STATUS_RECV_POSTED     ((nm_sr_status_t)NM_STATUS_UNPACK_POSTED)

/** events for nm_sr_monitor() */
typedef enum
  {
    /** an unexpected packet has arrived. Post a recv to get data
     */
    NM_SR_EVENT_RECV_UNEXPECTED = ((nm_sr_status_t)NM_STATUS_UNEXPECTED),
    NM_SR_EVENT_RECV_COMPLETED  = NM_SR_STATUS_RECV_COMPLETED,
    NM_SR_EVENT_SEND_COMPLETED  = NM_SR_STATUS_SEND_COMPLETED,
    NM_SR_EVENT_RECV_CANCELLED  = NM_SR_STATUS_RECV_CANCELLED
  } nm_sr_event_t;


/** a sendrecv request object. Supposedly opaque for applications.*/
typedef struct nm_sr_request_s nm_sr_request_t;

/** information field for sendrecv events */
typedef union
{
  struct
  {
    nm_gate_t p_gate;
    nm_tag_t tag;
    nm_len_t len;
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


/** Initialize the send/receive interface.
 *
 *  @param p_session a nmad session object.
 *  @return The NM status.
 */
extern int nm_sr_init(nm_session_t p_session);

/** Shutdown the send/receive interface.
 *  @param p_session a pointer to a nmad session object
 *  @return The NM status.
 */
extern int nm_sr_exit(nm_session_t p_session);


/* ** Operations on requests (send or recv) **************** */

/** Set a notification function called upon request completion */
extern int nm_sr_request_monitor(nm_session_t p_session, nm_sr_request_t *p_request,
				 nm_sr_event_t mask, nm_sr_event_notifier_t notifier);

/** Ask to enqueue the request in the completion queue upon completion */
extern int nm_sr_request_set_completion_queue(nm_session_t p_session, nm_sr_request_t*p_request);

/** Ask to not enqueue the request in the completion queue upon completion.
 * Remove it from the completion queue if it was already enqueued.
 */
extern int nm_sr_request_unset_completion_queue(nm_session_t p_session, nm_sr_request_t*p_request);

/** Add a user reference to a request */
static inline int nm_sr_request_set_ref(nm_session_t p_session, nm_sr_request_t*p_request, void*ref);

/** Returns the received size of the message with the specified request. */
static inline int nm_sr_get_size(nm_session_t p_session, nm_sr_request_t *request, size_t *size);

/** Retrieve the 'ref' from a sendrecv request. */
static inline int nm_sr_get_ref(nm_session_t p_session,	nm_sr_request_t *p_request, void**ref);

/** Retrieve the tag from a receive request. */
static inline int nm_sr_get_rtag(nm_session_t p_session, nm_sr_request_t *p_request, nm_tag_t*tag);

/** Retrieve the tag from a send request. */
static inline int nm_sr_get_stag(nm_session_t p_session, nm_sr_request_t *p_request, nm_tag_t*tag);


/* ** Building blocks for send ***************************** */

static inline void nm_sr_send_init(nm_session_t p_session, nm_sr_request_t*p_request);
static inline void nm_sr_send_pack_contiguous(nm_session_t p_session, nm_sr_request_t*p_request, 
					      const void*, nm_len_t len);
static inline void nm_sr_send_pack_iov(nm_session_t p_session, nm_sr_request_t*p_request,
				       const struct iovec*iov, int num_entries);
static inline void nm_sr_send_pack_data(nm_session_t p_session, nm_sr_request_t*p_request, 
					struct nm_data_s*p_data);
static inline int  nm_sr_send_isend(nm_session_t p_session, nm_sr_request_t*p_request,
				    nm_gate_t p_gate, nm_tag_t tag);
static inline int  nm_sr_send_issend(nm_session_t p_session, nm_sr_request_t*p_request,
				     nm_gate_t p_gate, nm_tag_t tag);
static inline int  nm_sr_send_rsend(nm_session_t p_session, nm_sr_request_t*p_request,
				    nm_gate_t p_gate, nm_tag_t tag);
			    

/* ** Building blocks for recv ***************************** */

static inline void nm_sr_recv_init(nm_session_t p_session, nm_sr_request_t*p_request);
static inline void nm_sr_recv_unpack_contiguous(nm_session_t p_session, nm_sr_request_t*p_request, 
						void*ptr, nm_len_t len);
static inline void nm_sr_recv_unpack_iov(nm_session_t p_session, nm_sr_request_t*p_request,
					 struct iovec*iov, int num_entry);
static inline void nm_sr_recv_unpack_data(nm_session_t p_session, nm_sr_request_t*p_request, 
					  struct nm_data_s*p_data);
static inline int  nm_sr_recv_irecv(nm_session_t p_session, nm_sr_request_t*p_request,
				    nm_gate_t p_gate, nm_tag_t tag, nm_tag_t mask);


/* ** High level / legacy send ***************************** */

/** Synchronous send. */
static inline int nm_sr_issend(nm_session_t p_session,
			       nm_gate_t p_gate, nm_tag_t tag,
			       const void *data, nm_len_t len,
			       nm_sr_request_t *p_request)
{
  nm_sr_send_init(p_session, p_request);
  nm_sr_send_pack_contiguous(p_session, p_request, data, len);
  const int err = nm_sr_send_issend(p_session, p_request, p_gate, tag);
  return err;
}

/** Post a non blocking send request.
 *  @param p_session a pointer to a nmad session object.
 *  @param p_gate a pointer to the destination gate.
 *  @param tag the message tag.
 *  @param data the data fragment pointer.
 *  @param len the data fragment length.
 *  @param p_request a pointer to a NM request to be filled.
 *  @return The NM status.
 */
static inline int nm_sr_isend(nm_session_t p_session,
			      nm_gate_t p_gate, nm_tag_t tag,
			      const void *data, nm_len_t len,
			      nm_sr_request_t *p_request)
{
  nm_sr_send_init(p_session, p_request);
  nm_sr_send_pack_contiguous(p_session, p_request, data, len);
  const int err = nm_sr_send_isend(p_session, p_request, p_gate, tag);
  return err;
}

static inline int nm_sr_isend_with_ref(nm_session_t p_session,
				       nm_gate_t p_gate, nm_tag_t tag,
				       const void *data, nm_len_t len,
				       nm_sr_request_t *p_request,
				       void*ref)
{
  nm_sr_send_init(p_session, p_request);
  nm_sr_request_set_ref(p_session, p_request, ref);
  nm_sr_send_pack_contiguous(p_session, p_request, data, len);
  if(ref != NULL)
    nm_sr_request_set_completion_queue(p_session, p_request);
  const int err = nm_sr_send_isend(p_session, p_request, p_gate, tag);
  return err;
}

/** Post a ready send request, i.e. assume the receiver is ready
 *  thus removing the need for a rdv.
 *  @param p_session a pointer to a nmad session object.
 *  @param p_gate a pointer to the destination gate.
 *  @param tag the message tag.
 *  @param data the data fragment pointer.
 *  @param len the data fragment length.
 *  @param p_request a pointer to a sendrecv request to be filled.
 *  @return The NM status.
 */
static inline int nm_sr_rsend(nm_session_t p_session,
			      nm_gate_t p_gate, nm_tag_t tag,
			      const void *data, nm_len_t len,
			      nm_sr_request_t *p_request)
{
  nm_sr_send_init(p_session, p_request);
  nm_sr_send_pack_contiguous(p_session, p_request, data, len);
  const int err = nm_sr_send_rsend(p_session, p_request, p_gate, tag);
  return err;
}

/** Test for the completion of a non blocking send request.
 *  @param p_session a pointer to a nmad session object.
 *  @param p_gate a pointer to the destination gate.
 *  @param tag the message tag.
 *  @param iov
 *  @param num_entries
 *  @param p_request a pointer to a sendrecv request to be filled.
 *  @return The NM status.
 */
static inline int nm_sr_isend_iov(nm_session_t p_session,
				  nm_gate_t p_gate, nm_tag_t tag,
				  const struct iovec *iov, int num_entries,
				  nm_sr_request_t *p_request)
{
  nm_sr_send_init(p_session, p_request);
  nm_sr_send_pack_iov(p_session, p_request, iov, num_entries);
  const int err = nm_sr_send_isend(p_session, p_request, p_gate, tag);
  return err;
}

static inline int nm_sr_isend_iov_with_ref(nm_session_t p_session,
					   nm_gate_t p_gate, nm_tag_t tag,
					   const struct iovec *iov, int num_entries,
					   nm_sr_request_t *p_request,
					   void*ref)
{
  nm_sr_send_init(p_session, p_request);
  nm_sr_request_set_ref(p_session, p_request, ref);
  nm_sr_send_pack_iov(p_session, p_request, iov, num_entries);
  if(ref != NULL)
    nm_sr_request_set_completion_queue(p_session, p_request);
  const int err = nm_sr_send_isend(p_session, p_request, p_gate, tag);
  return err;
}

static inline int nm_sr_isend_filter(nm_session_t p_session,
				     nm_gate_t p_gate, nm_tag_t tag,
				     struct nm_data_s*filter,
				     nm_sr_request_t *p_request)
{
  nm_sr_send_init(p_session, p_request);
  nm_sr_send_pack_data(p_session, p_request, filter);
  const int err = nm_sr_send_isend(p_session, p_request, p_gate, tag);
  return err;
}


/** Test for the completion of a non blocking send request.
 *  @param p_session a pointer to a nmad session object.
 *  @param p_request the request to check.
 *  @return The NM status.
 */
extern int nm_sr_stest(nm_session_t p_session,
		       nm_sr_request_t *p_request);

/** Calls the scheduler
 *  @param p_session a pointer to a nmad session object.
 *  @return The NM status.
 */
extern int nm_sr_progress(nm_session_t p_session);

/** Wait for the completion of a non blocking send request.
 *  @param p_session a pointer to a nmad session object.
 *  @param p_request the request to check.
 *  @return The NM status.
 */
extern int nm_sr_swait(nm_session_t p_session,
		       nm_sr_request_t  *p_request);

/** Cancel a emission request.
 *  @param p_session a pointer to a nmad session object.
 *  @param p_request the request to cancel.
 *  @return The NM status.
 */
extern int nm_sr_scancel(nm_session_t p_session,
			 nm_sr_request_t *p_request);


/** Post a non blocking receive request.
 *  @param p_session a pointer to a nmad session object.
 *  @param p_gate a pointer to the destination gate.
 *  @param tag the message tag.
 *  @param data the data fragment pointer.
 *  @param len the data fragment length.
 *  @param p_request a pointer to a sendrecv request to be filled.
 *  @return The NM status.
 */
static inline int nm_sr_irecv(nm_session_t p_session,
			      nm_gate_t p_gate, nm_tag_t tag,
			      void *data, nm_len_t len,
			      nm_sr_request_t *p_request)
{
  nm_sr_recv_init(p_session, p_request);
  nm_sr_recv_unpack_contiguous(p_session, p_request, data, len);
  const int err = nm_sr_recv_irecv(p_session, p_request, p_gate, tag, NM_TAG_MASK_FULL);
  return err;
}

/** Test for the completion of a non blocking receive request.
 *  @param p_session a pointer to a nmad session object.
 *  @param p_gate a pointer to the destination gate.
 *  @param tag the message tag.
 *  @param data the data fragment pointer.
 *  @param len the data fragment length.
 *  @param p_request a pointer to a sendrecv request to be filled.
 *  @param ref
 *  @return The NM status.
 */
static inline int nm_sr_irecv_with_ref(nm_session_t p_session,
				       nm_gate_t p_gate, nm_tag_t tag,
				       void *data, nm_len_t len,
				       nm_sr_request_t *p_request,
				       void *ref)
{
  nm_sr_recv_init(p_session, p_request);
  nm_sr_request_set_ref(p_session, p_request, ref);
  nm_sr_recv_unpack_contiguous(p_session, p_request, data, len);
  if(ref != NULL)
    nm_sr_request_set_completion_queue(p_session, p_request);
  const int err = nm_sr_recv_irecv(p_session, p_request, p_gate, tag, NM_TAG_MASK_FULL);
  return err;
}

static inline int nm_sr_irecv_iov(nm_session_t p_session,
				  nm_gate_t p_gate, nm_tag_t tag,
				  struct iovec *iov, int num_entries,
				  nm_sr_request_t *p_request)
{
  nm_sr_recv_init(p_session, p_request);
  nm_sr_recv_unpack_iov(p_session, p_request, iov, num_entries);
  const int err = nm_sr_recv_irecv(p_session, p_request, p_gate, tag, NM_TAG_MASK_FULL);
  return err;
}

static inline int nm_sr_irecv_iov_with_ref(nm_session_t p_session,
					   nm_gate_t p_gate, nm_tag_t tag,
					   struct iovec *iov, int num_entries,
					   nm_sr_request_t *p_request, void *ref)
{
  nm_sr_recv_init(p_session, p_request);
  nm_sr_request_set_ref(p_session, p_request, ref);
  nm_sr_recv_unpack_iov(p_session, p_request, iov, num_entries);
  if(ref != NULL)
    nm_sr_request_set_completion_queue(p_session, p_request);
  const int err = nm_sr_recv_irecv(p_session, p_request, p_gate, tag, NM_TAG_MASK_FULL);
  return err;
}

static inline int nm_sr_irecv_filter(nm_session_t p_session,
				     nm_gate_t p_gate, nm_tag_t tag,
				     struct nm_data_s*filter,
				     nm_sr_request_t *p_request)
{
  nm_sr_recv_init(p_session, p_request);
  nm_sr_recv_unpack_data(p_session, p_request, filter);
  const int err = nm_sr_recv_irecv(p_session, p_request, p_gate, tag, NM_TAG_MASK_FULL);
  return err;
}


static inline int nm_sr_irecv_filter_with_ref(nm_session_t p_session,
					      nm_gate_t p_gate, nm_tag_t tag,
					      struct nm_data_s*filter,
					      nm_sr_request_t *p_request,
					      void *ref)
{
  nm_sr_recv_init(p_session, p_request);
  nm_sr_request_set_ref(p_session, p_request, ref);
  nm_sr_recv_unpack_data(p_session, p_request, filter);
  if(ref != NULL)
    nm_sr_request_set_completion_queue(p_session, p_request);
  const int err = nm_sr_recv_irecv(p_session, p_request, p_gate, tag, NM_TAG_MASK_FULL);
  return err;
}


/** Test for the completion of a non blocking receive request.
 *  @param p_session a pointer to a nmad session object.
 *  @param p_request the request to check.
 *  @return The NM status.
 */
extern int nm_sr_rtest(nm_session_t p_session, nm_sr_request_t *p_request);


/** Wait for the completion of a non blocking receive request.
 *  @param p_session a pointer to a nmad session object.
 *  @param p_request the request to check.
 *  @return The NM status.
 */
extern int nm_sr_rwait(nm_session_t p_session, nm_sr_request_t *p_request);

/** Cancel a reception request.
 *  @param p_session a pointer to a nmad session object.
 *  @param p_request the request to cancel.
 *  @return The NM status.
 */
extern int nm_sr_rcancel(nm_session_t p_session, nm_sr_request_t *p_request);

/** Retrieve the pkt source of a complete any source receive request.
 *  @param p_session a pointer to a nmad session object.
 *  @param p_request the request to check.
 *  @param p_gate a pointer to the destination gate.
 *  @return The NM status.
 */
extern int nm_sr_recv_source(nm_session_t p_session,
			     nm_sr_request_t *p_request, nm_gate_t *p_gate);

/** Unblockingly check if a packet is available for extraction on the (gate,tag) pair .
 *  @param p_session a pointer to a nmad session object.
 *  @param p_gate a pointer to the expected gate.
 *  @param p_out_gate return value with the originating gate.
 *  @param tag the expected message tag.
 *  @param tag_mask a mask to be applied on tag before matching
 *  @param p_out_tag return value with the tag
 *  @param p_out_len return value with the message length
 *  @return The NM status.
 *  @note p_out_gate, p_out_tag and p_out_len may be NULL
 */
extern int nm_sr_probe(nm_session_t p_session,
		       nm_gate_t p_gate, nm_gate_t *p_out_gate,
		       nm_tag_t tag, nm_tag_t tag_mask, nm_tag_t*p_out_tag,
		       nm_len_t*p_out_len);

/** monitors sendrecv events globally */
extern int nm_sr_monitor(nm_session_t p_session, nm_sr_event_t mask, nm_sr_event_notifier_t notifier);

/** Poll for any completed recv request (any source, any tag).
 * @note Only nm_sr_recv*_with_ref functions (with ref != NULL) generate such an event.
 */
extern int nm_sr_recv_success(nm_session_t p_session, nm_sr_request_t **out_req);

/** Poll for any completed send request.
 * @note Only nm_sr_isend*_with_ref functions (with ref != NULL) generate such an event.
 */
extern int nm_sr_send_success(nm_session_t p_session, nm_sr_request_t **out_req);


/* @} */

#include <nm_sendrecv_private.h> /* private header included for inlining */

/** blocking send */
static inline int nm_sr_send(nm_session_t p_session,
			     nm_gate_t p_gate, nm_tag_t tag,
			     const void *data, nm_len_t len)
{
  nm_sr_request_t request;
  int rc = nm_sr_isend(p_session, p_gate, tag, data, len, &request);
  if(rc == NM_ESUCCESS)
    {
      rc = nm_sr_swait(p_session, &request);
    }
  return rc;
}

/** blocking recv */
static inline int nm_sr_recv(nm_session_t p_session,
			     nm_gate_t p_gate, nm_tag_t tag,
			     void *data, nm_len_t len)
{
  nm_sr_request_t request;
  int rc = nm_sr_irecv(p_session, p_gate, tag, data, len, &request);
  if(rc == NM_ESUCCESS)
    {
      rc = nm_sr_rwait(p_session, &request);
    }
  return rc;
}

#endif /* NM_SENDRECV_INTERFACE_H */

