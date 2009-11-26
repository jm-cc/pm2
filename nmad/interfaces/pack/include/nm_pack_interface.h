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

#ifndef NM_PACK_INTERFACE_H
#define NM_PACK_INTERFACE_H

/** \defgroup pack_interface Pack interface
 *
 * This is the NM Pack interface
 *
 * @{
 */

#include <nm_public.h>
#include <nm_sendrecv_interface.h>
#include <nm_session_interface.h>

/** Connection storage for the pack interface.
 */
struct nm_pack_cnx
{
  /** Interface used on the connexion. */
  nm_session_t p_session;

  /** Source or destination gate id. */
  nm_gate_t gate;

  /** Message tag. */
  nm_tag_t tag;

  /** sendrecv requests */
  nm_sr_request_t*reqs;

  /** Number of packets. */
  unsigned long nb_packets;
};
typedef struct nm_pack_cnx nm_pack_cnx_t;


/** Start building and sending a new message.
 *  @param p_session a pointer to a nmad session object.
 *  @param gate the gate to the destination.
 *  @param tag the message tag.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
extern int nm_begin_packing(nm_session_t p_session,
			    nm_gate_t gate, nm_tag_t tag,
			    nm_pack_cnx_t *cnx);

/** Append a data fragment to the current message.
 *  @param cnx a NM/SO connection pointer.
 *  @param data a pointer to the data fragment.
 *  @param len the length of the data fragment.
 *  @return The NM status.
 */
extern int nm_pack(nm_pack_cnx_t *cnx,
		   const void *data, uint32_t len);

/** End building and flush the current message.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
extern int nm_end_packing(nm_pack_cnx_t *cnx);

/** Cancel the current message.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
extern int nm_cancel_packing(nm_pack_cnx_t *cnx);

/** Start receiving and extracting a new message.
 *  @param p_session a pointer to a nmad session object.
 *  @param gate the gate of the source or -1 for receiving from any source.
 *  @param tag the message tag.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
extern int nm_begin_unpacking(nm_session_t p_session,
			      nm_gate_t gate, nm_tag_t tag,
			      nm_pack_cnx_t *cnx);

/** Extract a data fragment from the current message.
 *  @param cnx a NM/SO connection pointer.
 *  @param data a pointer to the data fragment.
 *  @param len the length of the data fragment.
 *  @return The NM status.
 */
extern int nm_unpack(nm_pack_cnx_t *cnx,
		     void *data, uint32_t len);

/** End receiving and flush extraction of the current message.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
extern int nm_end_unpacking(nm_pack_cnx_t *cnx);

/** Cancel the current message.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
extern int nm_cancel_unpacking(nm_pack_cnx_t *cnx);

/** Wait for ongoing send requests to complete.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
extern int nm_flush_packs(nm_pack_cnx_t *cnx);

/** Wait for ongoing receive requests to complete.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
extern int nm_flush_unpacks(nm_pack_cnx_t *cnx);

/** Check ongoing send requests for completion.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
extern int nm_test_end_packing(nm_pack_cnx_t *cnx);

/** Check ongoing receive requests for completion.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
extern int nm_test_end_unpacking(nm_pack_cnx_t *cnx);

/* @} */

#endif
