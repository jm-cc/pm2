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

#ifndef _NM_SO_PACK_INTERFACE_H_
#define _NM_SO_PACK_INTERFACE_H_

/** \defgroup pack_interface Pack interface
 *
 * This is the NM Pack interface
 *
 * @{
 */

#ifndef NM_SO_ANY_SRC
#define NM_SO_ANY_SRC  ((nm_gate_id_t)-1)
#endif

typedef intptr_t nm_so_pack_interface;

struct nm_so_cnx {
  intptr_t interface;
  nm_gate_id_t gate;
  long tag;
  long first_seq_number;
  long nb_paquets;
};

/** Initialize the interface.
 *
 *  @param p_core a pointer to the NM core object.
 *  @param p_interface a pointer to the NM/SchedOpt interface to be filled by the function.
 *  @return The NM status.
 */
extern int
nm_so_pack_interface_init(struct nm_core *p_core,
			  nm_so_pack_interface *p_interface);


/** Start building and sending a new message.
 *  @param interface a pack interface object.
 *  @param gate_id the gate id to the destination.
 *  @param tag the message tag.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
extern int
nm_so_begin_packing(nm_so_pack_interface interface,
		    nm_gate_id_t gate_id, uint8_t tag,
		    struct nm_so_cnx *cnx);

/** Append a data fragment to the current message.
 *  @param cnx a NM/SO connection pointer.
 *  @param data a pointer to the data fragment.
 *  @param len the length of the data fragment.
 *  @return The NM status.
 */
extern int
nm_so_pack(struct nm_so_cnx *cnx,
	   void *data, uint32_t len);

/** End building and flush the current message.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
extern int
nm_so_end_packing(struct nm_so_cnx *cnx);

/** Start receiving and extracting a new message.
 *  @param interface a pack interface object.
 *  @param gate_id the gate id of the source or -1 for receiving from any source.
 *  @param tag the message tag.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
int
nm_so_begin_unpacking(nm_so_pack_interface interface,
		      nm_gate_id_t gate_id, uint8_t tag,
		      struct nm_so_cnx *cnx);

/** Extract a data fragment from the current message.
 *  @param cnx a NM/SO connection pointer.
 *  @param data a pointer to the data fragment.
 *  @param len the length of the data fragment.
 *  @return The NM status.
 */
int
nm_so_unpack(struct nm_so_cnx *cnx,
	     void *data, uint32_t len);

/** End receiving and flush extraction of the current message.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
int
nm_so_end_unpacking(struct nm_so_cnx *cnx);

/** Wait for ongoing send requests to complete.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
int
nm_so_flush_packs(struct nm_so_cnx *cnx);

/** Wait for ongoing receive requests to complete.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
int
nm_so_flush_unpacks(struct nm_so_cnx *cnx);

/** Check ongoing send requests for completion.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
int
nm_so_test_end_packing(struct nm_so_cnx *cnx);

/** Check ongoing receive requests for completion.
 *  @param cnx a NM/SO connection pointer.
 *  @return The NM status.
 */
int
nm_so_test_end_unpacking(struct nm_so_cnx *cnx);

/* @} */

#endif
