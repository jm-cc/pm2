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


/** events for nm_sr_monitor() */
typedef enum
  {
    /** an unexpected packet has arrived. Post a recv to get data */
    NM_SR_EVENT_RECV_UNEXPECTED = NM_STATUS_UNEXPECTED,
    /** a posted recv has completed */
    NM_SR_EVENT_RECV_COMPLETED  = NM_STATUS_UNPACK_COMPLETED,
    /** a posted send has completed */
    NM_SR_EVENT_SEND_COMPLETED  = NM_STATUS_PACK_COMPLETED,
    /** recv operation was canceled */
    NM_SR_EVENT_RECV_CANCELLED  = NM_STATUS_UNPACK_CANCELLED,
    /** request finalized, may be freed by user */
    NM_SR_EVENT_FINALIZED       = NM_STATUS_FINALIZED
  } nm_sr_event_t;


/** a sendrecv request object. Supposedly opaque for applications. */
typedef struct nm_sr_request_s nm_sr_request_t;

/** information field for sendrecv events */
typedef union
{
  struct
  {
    nm_gate_t p_gate;
    nm_tag_t tag;
    nm_len_t len;
    nm_session_t p_session;
  } recv_unexpected; /**< field for global event NM_SR_EVENT_RECV_UNEXPECTED */
  struct
  {
    nm_sr_request_t*p_request;
  } req; /**< field for req events NM_SR_EVENT_SEND_COMPLETED, RECV_COMPLETED, CANCELLED, FINALIZED */
} nm_sr_event_info_t;

/** notification function for sendrecv events. 
 * Received ref comes from monitor->ref in case of session monitor, and request->ref in case of request monitor */
typedef void (*nm_sr_event_notifier_t)(nm_sr_event_t event, const nm_sr_event_info_t*event_info, void*ref);

struct nm_sr_monitor_s
{
  nm_sr_event_notifier_t p_notifier;
  nm_sr_event_t event_mask;
  nm_gate_t p_gate;        /**< listen for events on given gate; NM_ANY_GATE for any */
  nm_tag_t tag;            /**< tag value for event filter */
  nm_tag_t tag_mask;       /**< tag mask for event filter- fire event when event.tag & monitor.tag_mask == monitor.tag; set tag = 0 && tag_mask = NM_TAG_MASK_NONE to catch any tag; set tag_mask = NM_TAG_MASK_FULL to catch a given tag */
  void*ref;                /**< reference for user */
};

/** open a new session ready for sendrecv */
static inline nm_session_t nm_sr_session_open(const char*label);

/** close a sendrecv session */
static inline void nm_sr_session_close(nm_session_t p_session);

/** Initialize the sendrecv interface for the given session */
extern int nm_sr_init(nm_session_t p_session);

/** Shutdown the sendrecv interface for the given session. */
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
static inline int nm_sr_request_set_ref(nm_sr_request_t*p_request, void*ref);

/** Retrieve the 'ref' from a sendrecv request. */
static inline int nm_sr_request_get_ref(nm_sr_request_t*p_request, void**ref);

/** Retrieve the tag from a sendrecv request. */
static inline int nm_sr_request_get_tag(nm_sr_request_t*p_request, nm_tag_t*tag);

/** Returns the received size of the message with the specified request. */
static inline int nm_sr_request_get_size(nm_sr_request_t *request, nm_len_t*size);

/** legacy symbol name */
#define nm_sr_get_stag(SESSION, REQ, TAG)  nm_sr_request_get_tag(REQ, TAG)
/** legacy symbol name */
#define nm_sr_get_rtag(SESSION, REQ, TAG)  nm_sr_request_get_tag(REQ, TAG)
/** legacy symbol name */
#define nm_sr_get_ref(SESSION, REQ, REF)   nm_sr_request_get_ref(REQ, REF)
/** legacy symbol name */
#define nm_sr_get_size(SESSION, REQ, SIZE) nm_sr_request_get_size(REQ, SIZE)

/* ** Building blocks for send ***************************** */

static inline void nm_sr_send_init(nm_session_t p_session, nm_sr_request_t*p_request);
static inline void nm_sr_send_pack_contiguous(nm_session_t p_session, nm_sr_request_t*p_request, 
					      const void*, nm_len_t len);
static inline void nm_sr_send_pack_iov(nm_session_t p_session, nm_sr_request_t*p_request,
				       const struct iovec*iov, int num_entries);
static inline void nm_sr_send_pack_data(nm_session_t p_session, nm_sr_request_t*p_request, 
					const struct nm_data_s*p_data);
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
					  const struct nm_data_s*p_data);
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
  nm_sr_request_set_ref(p_request, ref);
  nm_sr_send_pack_contiguous(p_session, p_request, data, len);
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
  nm_sr_request_set_ref(p_request, ref);
  nm_sr_send_pack_iov(p_session, p_request, iov, num_entries);
  const int err = nm_sr_send_isend(p_session, p_request, p_gate, tag);
  return err;
}

static inline int nm_sr_isend_data(nm_session_t p_session,
				   nm_gate_t p_gate, nm_tag_t tag,
				   struct nm_data_s*filter,
				   nm_sr_request_t *p_request, void*ref)
{
  nm_sr_send_init(p_session, p_request);
  nm_sr_request_set_ref(p_request, ref);
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
  nm_sr_request_set_ref(p_request, ref);
  nm_sr_recv_unpack_contiguous(p_session, p_request, data, len);
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
  nm_sr_request_set_ref(p_request, ref);
  nm_sr_recv_unpack_iov(p_session, p_request, iov, num_entries);
  const int err = nm_sr_recv_irecv(p_session, p_request, p_gate, tag, NM_TAG_MASK_FULL);
  return err;
}

static inline int nm_sr_irecv_data(nm_session_t p_session,
				   nm_gate_t p_gate, nm_tag_t tag,
				   struct nm_data_s*filter,
				   nm_sr_request_t *p_request, void*ref)
{
  nm_sr_recv_init(p_session, p_request);
  nm_sr_request_set_ref(p_request, ref);
  nm_sr_recv_unpack_data(p_session, p_request, filter);
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

/** set a monitor for events on the session */
int nm_sr_session_monitor_set(nm_session_t p_session, const struct nm_sr_monitor_s*p_monitor);

/** remove the event monitor from the session */
int nm_sr_session_monitor_remove(nm_session_t p_session, const struct nm_sr_monitor_s*p_monitor);

/** Poll for any completed recv request (any source, any tag).
 * @note call nm_sr_request_set_completion_queue() to generate such an event.
 */
extern int nm_sr_recv_success(nm_session_t p_session, nm_sr_request_t **out_req);

/** Poll for any completed send request.
 * @note call nm_sr_request_set_completion_queue() to generate such an event.
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

