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
 * Mad_tcp.h
 * =========
 */
#ifndef MAD_TCP_H
#define MAD_TCP_H

/*
 * Structures
 * ----------
 */

/*
 * Functions 
 * ---------
 */

void
mad_tcp_register(p_mad_driver_t driver);

void
mad_tcp_driver_init(p_mad_driver_t);

void
mad_tcp_adapter_init(p_mad_adapter_t);

void
mad_tcp_adapter_configuration_init(p_mad_adapter_t);

/*----*/

void
mad_tcp_channel_init(p_mad_channel_t);

void
mad_tcp_before_open_channel(p_mad_channel_t);

void
mad_tcp_connection_init(p_mad_connection_t,
			p_mad_connection_t);

void
mad_tcp_link_init(p_mad_link_t);

void
mad_tcp_accept(p_mad_channel_t);

void
mad_tcp_connect(p_mad_connection_t);

void
mad_tcp_after_open_channel(p_mad_channel_t);

void
mad_tcp_before_close_channel(p_mad_channel_t);

void
mad_tcp_disconnect(p_mad_connection_t);

void
mad_tcp_after_close_channel(p_mad_channel_t);

p_mad_link_t
mad_tcp_choice(p_mad_connection_t,
	       size_t,
	       mad_send_mode_t,
	       mad_receive_mode_t);

void
mad_tcp_new_message(p_mad_connection_t);

p_mad_connection_t
mad_tcp_receive_message(p_mad_channel_t);

void
mad_tcp_send_buffer(p_mad_link_t,
		    p_mad_buffer_t);
  
void
mad_tcp_receive_buffer(p_mad_link_t,
		       p_mad_buffer_t *);

void
mad_tcp_send_buffer_group(p_mad_link_t,
			  p_mad_buffer_group_t);

void
mad_tcp_receive_sub_buffer_group(p_mad_link_t,
				 mad_bool_t,
				 p_mad_buffer_group_t);


#endif /* MAD_TCP_H */
