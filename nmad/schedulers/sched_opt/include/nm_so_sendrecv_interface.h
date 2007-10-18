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

#ifndef _NM_SO_SENDRECV_INTERFACE_H_
#define _NM_SO_SENDRECV_INTERFACE_H_

/** \defgroup sr_interface SendRecv interface
 *
 * This is the NM SendRecv interface
 *
 * @{
 */

#include "tbx.h"
#include "nm_so_sendrecv_interface_private.h"

#ifndef NM_SO_ANY_SRC
#define NM_SO_ANY_SRC  ((nm_gate_id_t)-1)
#endif

#ifdef NMAD_QOS
/* Parameters for the QoS strategy 
 */
#define NM_SO_NB_POLICIES 6

#define NM_SO_POLICY_UNDEFINED NM_SO_NB_POLICIES
#define NM_SO_POLICY_FIFO 0
#define NM_SO_POLICY_LATENCY 1
#define NM_SO_POLICY_RATE 2
#define NM_SO_POLICY_PRIORITY 3
#define NM_SO_POLICY_PRIORITY_LATENCY 4
#define NM_SO_POLICY_PRIORITY_RATE 5

#define NM_SO_NB_PRIORITIES 3
#endif /* NMAD_QOS */

struct nm_so_interface;
struct iovec;

struct nm_so_request_s;
typedef struct nm_so_request_s nm_so_request;

/** Initialize the send/receive interface.
 *
 *  @param p_core a pointer to the NM core object.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface to be filled by the function.
 *  @return The NM status.
 */
extern int
nm_so_sr_init(struct nm_core *p_core,
	      struct nm_so_interface **p_so_interface);

/** Shutdown the send/receive interface.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @return The NM status.
 */
extern int
nm_so_sr_exit(struct nm_so_interface *p_so_interface);

/** Post a non blocking send request.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param gate_id the destination gate id.
 *  @param tag the message tag.
 *  @param data the data fragment pointer.
 *  @param len the data fragment length.
 *  @param p_request a pointer to a NM/SO request to be filled.
 *  @return The NM status.
 */
extern int
nm_so_sr_isend(struct nm_so_interface *p_so_interface,
	       nm_gate_id_t gate_id, uint8_t tag,
	       void *data, uint32_t len,
	       nm_so_request *p_request);

extern int
nm_so_sr_isend_extended(struct nm_so_interface *p_so_interface,
                        nm_gate_id_t gate_id, uint8_t tag,
                        void *data, uint32_t len,
                        tbx_bool_t is_completed,
                        nm_so_request *p_request);

/** Post a ready send request. When seading a large packet requiring a
 *  RDV, waits for completion of RDV, i.e returns when the matching
 *  receive has been posted.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param gate_id the destination gate id.
 *  @param tag the message tag.
 *  @param data the data fragment pointer.
 *  @param len the data fragment length.
 *  @param p_request a pointer to a NM/SO request to be filled.
 *  @return The NM status.
 */
extern int
nm_so_sr_rsend(struct nm_so_interface *p_so_interface,
	       nm_gate_id_t gate_id, uint8_t tag,
	       void *data, uint32_t len,
	       nm_so_request *p_request);

/** Test for the completion of a non blocking send request.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param request the request to check.
 *  @return The NM status.
 */
extern int
nm_so_sr_isend_iov(struct nm_so_interface *p_so_interface,
                   nm_gate_id_t gate_id, uint8_t tag,
                   struct iovec *iov, int nb_entries,
                   nm_so_request *p_request);

extern int
nm_so_sr_stest(struct nm_so_interface *p_so_interface,
	       nm_so_request request);

/** Calls the scheduler
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @return The NM status.
 */
extern int
nm_so_sr_progress(struct nm_so_interface *p_so_interface);

/** Test for the completion of a SET of non blocking receive requests.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param request the request to check.
 *  @return The NM status.
 */
extern int
nm_so_sr_req_test(struct nm_so_interface *p_so_interface,
                  nm_so_request request);

/** Test for the completion of a continuous series of non blocking send requests.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param gate_id the destination gate id.
 *  @param tag the message tag.
 *  @param seq_inf the sequence number of the first request to check
 *  @param nb the number of requests to check.
 *  @return The NM status.
 */
extern int
nm_so_sr_stest_range(struct nm_so_interface *p_so_interface,
		     nm_gate_id_t gate_id, uint8_t tag,
		     unsigned long seq_inf, unsigned long nb);

/** Wait for the completion of a non blocking send request.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param request the request to check.
 *  @return The NM status.
 */
extern int
nm_so_sr_swait(struct nm_so_interface *p_so_interface,
	       nm_so_request request);

/** Wait for the completion of a continuous series of non blocking send requests.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param gate_id the destination gate id.
 *  @param tag the message tag.
 *  @param seq_inf the sequence number of the first request to check
 *  @param nb the number of requests to check.
 *  @return The NM status.
 */
extern int
nm_so_sr_swait_range(struct nm_so_interface *p_so_interface,
		     nm_gate_id_t gate_id, uint8_t tag,
		     unsigned long seq_inf, unsigned long nb);

/** Cancel a emission request.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param request the request to cancel.
 *  @return The NM status.
 */
extern int
nm_so_sr_scancel(struct nm_so_interface *p_so_interface,
                 nm_so_request request);

/** Post a non blocking receive request.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param gate_id the source gate id or NM_SO_ANY_SRC for receiving from any source.
 *  @param tag the message tag.
 *  @param data the data fragment pointer.
 *  @param len the data fragment length.
 *  @param p_request a pointer to a NM/SO request to be filled.
 *  @return The NM status.
 */
extern int
nm_so_sr_irecv(struct nm_so_interface *p_so_interface,
	       nm_gate_id_t gate_id, uint8_t tag,
	       void *data, uint32_t len,
	       nm_so_request *p_request);

/** Test for the completion of a non blocking receive request.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param request the request to check.
 *  @return The NM status.
 */
extern int
nm_so_sr_irecv_with_ref(struct nm_so_interface *p_so_interface,
                        nm_gate_id_t gate_id, uint8_t tag,
                        void *data, uint32_t len,
                        nm_so_request *p_request,
                        void *ref);

extern int
nm_so_sr_irecv_iov(struct nm_so_interface *p_so_interface,
                   nm_gate_id_t gate_id, uint8_t tag,
                   struct iovec *iov, int nb_entries,
                   nm_so_request *p_request);

extern int
nm_so_sr_irecv_iov_with_ref(struct nm_so_interface *p_so_interface,
                            nm_gate_id_t gate_id, uint8_t tag,
                            struct iovec *iov, int nb_entries,
                            nm_so_request *p_request, void *ref);

extern int
nm_so_sr_rtest(struct nm_so_interface *p_so_interface,
	       nm_so_request request);

/** Test for the completion of a continuous series of non blocking receive requests.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param gate_id the destination gate id.
 *  @param tag the message tag.
 *  @param seq_inf the sequence number of the first request to check
 *  @param nb the number of requests to check.
 *  @return The NM status.
 */
extern int
nm_so_sr_rtest_range(struct nm_so_interface *p_so_interface,
		     nm_gate_id_t gate_id, uint8_t tag,
		     unsigned long seq_inf, unsigned long nb);

/** Wait for the completion of a non blocking receive request.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param request the request to check.
 *  @return The NM status.
 */
extern int
nm_so_sr_rwait(struct nm_so_interface *p_so_interface,
	       nm_so_request request);

/** Wait for the completion of a continuous series of non blocking receive requests.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param gate_id the source gate id.
 *  @param tag the message tag.
 *  @param seq_inf the sequence number of the first request to check
 *  @param nb the number of requests to check.
 *  @return The NM status.
 */
extern int
nm_so_sr_rwait_range(struct nm_so_interface *p_so_interface,
		     nm_gate_id_t gate_id, uint8_t tag,
		     unsigned long seq_inf, unsigned long nb);

/** Cancel a reception request.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param request the request to cancel.
 *  @return The NM status.
 */
extern int
nm_so_sr_rcancel(struct nm_so_interface *p_so_interface,
                 nm_so_request request);

/** Retrieve the pkt source of a complete any source receive request.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param request the request to check.
 *  @param gate_id a pointer to where the source gate id should be put.
 *  @return The NM status.
 */
extern int
nm_so_sr_recv_source(struct nm_so_interface *p_so_interface,
                     nm_so_request request, nm_gate_id_t *gate_id);

/** Unblockingly check if a packet is available for extraction on the (gate,tag) pair .
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param gate_id the source gate id.
 *  @param out_gate_id a pointer to the
 *  @param tag the message tag.
 *  @return The NM status.
 */
extern int
nm_so_sr_probe(struct nm_so_interface *p_so_interface,
               nm_gate_id_t gate_id, nm_gate_id_t *out_gate_id, uint8_t tag);

extern int
nm_so_sr_recv_success(struct nm_so_interface *p_so_interface, void **ref);

/** Get the current send sequence number for the (gate,tag).
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param gate_id the destination gate id.
 *  @param tag the message tag.
 *  @return The send sequence number.
 */
extern unsigned long
nm_so_sr_get_current_send_seq(struct nm_so_interface *p_so_interface,
			      nm_gate_id_t gate_id, uint8_t tag);

/** Get the current receive sequence number for the (gate,tag).
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param gate_id the source gate id.
 *  @param tag the message tag.
 *  @return The receive sequence number.
 */
extern unsigned long
nm_so_sr_get_current_recv_seq(struct nm_so_interface *p_so_interface,
			      nm_gate_id_t gate_id, uint8_t tag);

/** Returns the received size of the message with the specified
 *  request.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param request the request to check.
 *  @param tag the tag to check
 *  @param size returns the size of the message
 *  @return The NM status.
 */
extern int
nm_so_sr_get_size(struct nm_so_interface *p_so_interface,
                  nm_so_request *request, uint8_t tag,
                  int *size);

#ifdef NMAD_QOS

/** Set the specified priority value to the specified tag
 *  @param tag the tag of a flow
 *  @param priority the priority value to map to the tag
 */
extern void
nm_so_set_priority(uint8_t tag, uint8_t priority);

/** Get the priority value for the specified tag
 *  @param tag the tag of a flow
 *  @return The priority value mapped to the tag
 */
extern uint8_t
nm_so_get_priority(uint8_t tag);

/** Set the specified policy to the specified tag
 *  @param tag the tag of a flow
 *  @param policy the policy value to map to the tag
 */
extern void
nm_so_set_policy(uint8_t tag, uint8_t policy);

/** Get the policy for the specified tag
 *  @param tag the tag of a flow
 *  @return The policy mapped to the tag
 */
extern uint8_t
nm_so_get_policy(uint8_t tag);

#endif /* NMAD_QOS */

/* @} */

#endif
