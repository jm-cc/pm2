/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

/*
 * Mad_via.h
 * ---------
 */

#ifndef __MAD_VIA_H
#define __MAD_VIA_H

#include <vipl.h> /* for the M-VIA implementation only */

/* Protocol module registering */
p_mad_protocol_settings_t
mad_via_register();

/* Protocol module initialization */
void
mad_via_main_init(p_mad_madeleine_t madeleine);

/* Adapter module initialization */
void
mad_via_master_init_local(p_mad_adapter_t adapter);

void
mad_via_master_init_remote(p_mad_adapter_t adapter);

void
mad_via_slave_init_local(p_mad_adapter_t adapter);

void
mad_via_slave_init_remote(p_mad_adapter_t adapter);
/* Channel opening */
void
mad_via_begin_open_channel(p_mad_channel_t channel);

p_mad_connection_t
mad_via_open_input_connection(p_mad_channel_t   channel,
			      p_mad_host_id_t   remote_host_id);

p_mad_connection_t
mad_via_open_output_connection(p_mad_channel_t   channel,
			       mad_host_id_t     remote_host_id);

void
mad_via_init_communication(p_mad_communication_t communication);

void
mad_via_end_open_channel(p_mad_channel_t channel);

/* Channel closing */
void
mad_via_begin_close_channel(p_mad_channel_t channel);

void
mad_via_close_input_connection(p_mad_connection_t connnection);

void
mad_via_close_output_connection(p_mad_connection_t connection);

void
mad_via_end_close_channel(p_mad_channel_t channel);

/* Link selection */
mad_link_id_t
mad_via_choice(size_t               buffer_length,
	       mad_send_mode_t      send_mode,
	       mad_receive_mode_t   receive_mode);

/* Communication initialization */
void
mad_via_init_send_communication(p_mad_connection_t connection);

mad_host_id_t
mad_via_accept_receive_communication(p_mad_channel_t channel);

/* Static buffers (credits) */
p_mad_buffer_t
mad_via_get_static_buffer(p_mad_link_t link);

void
mad_via_return_static_buffer(p_mad_link_t     link,
			     p_mad_buffer_t   buffer);

/* Data transfer interface */
void
mad_via_send_buffer(p_mad_link_t     link,
		    p_mad_buffer_t   buffer);
  
void
mad_via_receive_buffer(p_mad_link_t     link,
		       p_mad_buffer_t  *buffer);

void
mad_via_send_buffer_group(p_mad_link_t           link,
			  p_mad_buffer_group_t   buffer_group);

void
mad_via_receive_sub_buffer_group(p_mad_link_t           link,
				 mad_bool_t             first_sub_buffer_group,
				 p_mad_buffer_group_t   buffer_group);
#endif /* __MAD_VIA_H */
