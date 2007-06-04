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


#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <tbx.h>

#include <nm_so_private.h>

#include "nm_so_pack_interface.h"
#include "nm_so_sendrecv_interface.h"
#include "nm_so_sendrecv_interface_private.h"
#include "nm_so_tracks.h"

/** Connection storage for the pack interface.
 */
struct __nm_so_cnx {

        /** Interface used on the connexion.
         */
  struct nm_so_interface *p_interface;

        /** Source or destination gate id.
         */
  long gate_id;

        /** Message tag.
         */
  unsigned long tag;

        /** Sequence number of the first fragment.
         */
  unsigned long first_seq_number;

        /** Number of fragments.
         */
  unsigned long nb_paquets;
};

/** Initialize the interface.
 *
 *  @param p_core a pointer to the NM core object.
 *  @param p_interface a pointer to the NM/SchedOpt interface to be filled by the function.
 *  @return The NM status.
 */
int
nm_so_pack_interface_init(struct nm_core *p_core,
			  nm_so_pack_interface *p_interface)
{
  assert(sizeof(struct nm_so_cnx) == sizeof(struct __nm_so_cnx));

  return nm_so_sr_init(p_core,
                       (struct nm_so_interface **)p_interface);
}

/** Start buildind and sending a new message.
 *  @param interface a pack interface object.
 *  @param gate_id the gate id to the destination.
 *  @param tag the message tag.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
int
nm_so_begin_packing(nm_so_pack_interface interface,
		    uint16_t gate_id, uint8_t tag,
		    struct nm_so_cnx *cnx)
{
  struct __nm_so_cnx *_cnx = (struct __nm_so_cnx *)cnx;

  _cnx->p_interface = (struct nm_so_interface *)interface;
  _cnx->gate_id = gate_id;
  _cnx->tag = tag;
  _cnx->first_seq_number = nm_so_sr_get_current_send_seq(_cnx->p_interface,
							 gate_id, tag);
  _cnx->nb_paquets = 0;

  return NM_ESUCCESS;
}

/** Append a data fragment to the current message.
 *  @param cnx a NM/SO connection pointer.
 *  @param data a pointer to the data fragment.
 *  @param len the length of the data fragment.
 *  @return The NM status.
 */
int
nm_so_pack(struct nm_so_cnx *cnx,
	   void *data, uint32_t len)
{
  struct __nm_so_cnx *_cnx = (struct __nm_so_cnx *)cnx;

  _cnx->nb_paquets++;

  return nm_so_sr_isend(_cnx->p_interface, _cnx->gate_id, _cnx->tag,
			data, len, NULL);
}

/** End building and flush the current message.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
int
nm_so_end_packing(struct nm_so_cnx *cnx)
{
  return nm_so_flush_packs(cnx);
}

/** Start receiving and extracting a new message.
 *  @param interface a pack interface object.
 *  @param gate_id the gate id of the source or -1 for receiving from any source.
 *  @param tag the message tag.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
int
nm_so_begin_unpacking(nm_so_pack_interface interface,
		      long gate_id, uint8_t tag,
		      struct nm_so_cnx *cnx)
{
  struct __nm_so_cnx *_cnx = (struct __nm_so_cnx *)cnx;

  _cnx->p_interface = (struct nm_so_interface *)interface;
  _cnx->gate_id = gate_id;
  _cnx->tag = tag;
  _cnx->nb_paquets = 0;

  if(gate_id != NM_SO_ANY_SRC)
    _cnx->first_seq_number = nm_so_sr_get_current_recv_seq(_cnx->p_interface,
							   gate_id, tag);

  return NM_ESUCCESS;
}

/** Extract a data fragment from the current message.
 *  @param cnx a NM/SO connection pointer.
 *  @param data a pointer to the data fragment.
 *  @param len the length of the data fragment.
 *  @return The NM status.
 */
int
nm_so_unpack(struct nm_so_cnx *cnx,
	     void *data, uint32_t len)
{
  struct __nm_so_cnx *_cnx = (struct __nm_so_cnx *)cnx;

  if(_cnx->gate_id != NM_SO_ANY_SRC) {
    _cnx->nb_paquets++;

    return nm_so_sr_irecv(_cnx->p_interface, _cnx->gate_id, _cnx->tag,
			  data, len, NULL);

  } else {
  /* Here, we know that 1) begin_unpacking has been called with
     ANY_SRC as the gate_id parameter and 2) this is the first
     unpack. So we have to perform a synchronous recv in order to wait
     for the real gate_id to be known before next unpacks... */

    nm_so_request request;
    int err;

    err = nm_so_sr_irecv(_cnx->p_interface, NM_SO_ANY_SRC, _cnx->tag,
			 data, len, &request);
    if(err != NM_ESUCCESS)
      return err;

    err = nm_so_sr_rwait(_cnx->p_interface, request);
    if(err != NM_ESUCCESS)
      return err;

    err = nm_so_sr_recv_source(_cnx->p_interface, request, &_cnx->gate_id);

    _cnx->first_seq_number = nm_so_sr_get_current_recv_seq(_cnx->p_interface,
							   _cnx->gate_id,
							   _cnx->tag);
    return err;
  }
}

/** End receiving and flush extraction of the current message.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
int
nm_so_end_unpacking(struct nm_so_cnx *cnx)
{
  return nm_so_flush_unpacks(cnx);
}

/** Wait for ongoing send requests to complete.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
int
nm_so_flush_packs(struct nm_so_cnx *cnx)
{
  struct __nm_so_cnx *_cnx = (struct __nm_so_cnx *)cnx;

  return nm_so_sr_swait_range(_cnx->p_interface, _cnx->gate_id, _cnx->tag,
			      _cnx->first_seq_number, _cnx->nb_paquets);
}

/** Wait for ongoing receive requests to complete.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
int
nm_so_flush_unpacks(struct nm_so_cnx *cnx)
{
  struct __nm_so_cnx *_cnx = (struct __nm_so_cnx *)cnx;

	return nm_so_sr_rwait_range(_cnx->p_interface, _cnx->gate_id, _cnx->tag,
				    _cnx->first_seq_number, _cnx->nb_paquets);
}

/** Check ongoing send requests for completion.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
int
nm_so_test_end_packing(struct nm_so_cnx *cnx) {
  struct __nm_so_cnx *_cnx = (struct __nm_so_cnx *)cnx;

  return nm_so_sr_stest_range(_cnx->p_interface, _cnx->gate_id, _cnx->tag,
			      _cnx->first_seq_number, _cnx->nb_paquets);
}

/** Check ongoing receive requests for completion.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
int
nm_so_test_end_unpacking(struct nm_so_cnx *cnx) {
  struct __nm_so_cnx *_cnx = (struct __nm_so_cnx *)cnx;

  return nm_so_sr_stest_range(_cnx->p_interface, _cnx->gate_id, _cnx->tag,
			      _cnx->first_seq_number, _cnx->nb_paquets);
}
