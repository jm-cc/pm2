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

______________________________________________________________________________
$Log: mad_communication_interface.h,v $
Revision 1.4  2000/03/02 15:45:26  oaumage
- support du polling Nexus

Revision 1.3  2000/02/08 17:47:19  oaumage
- prise en compte des types de la net toolbox

Revision 1.2  1999/12/15 17:31:20  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Mad_communication_interface.h
 * =============================
 */

#ifndef MAD_COMMUNICATION_INTERFACE_H
#define MAD_COMMUNICATION_INTERFACE_H

/*
 * Functions 
 * ---------
 */

#ifdef MAD_NEXUS
p_mad_connection_t
mad_message_ready(p_mad_channel_t channel);
#endif /* MAD_NEXUS */

p_mad_connection_t
mad_begin_packing(p_mad_channel_t   channel,
		  ntbx_host_id_t    remote_host_id);

p_mad_connection_t
mad_begin_unpacking(p_mad_channel_t channel);

void
mad_end_packing(p_mad_connection_t connection);

void
mad_end_unpacking(p_mad_connection_t connection);

void
mad_pack(p_mad_connection_t   connection,
	 void                *buffer,
	 size_t               buffer_length,
	 mad_send_mode_t      send_mode,
	 mad_receive_mode_t   receive_mode
	 );

void
mad_unpack(p_mad_connection_t   connection,
	 void                  *buffer,
	 size_t                 buffer_length,
	 mad_send_mode_t        send_mode,
	 mad_receive_mode_t     receive_mode
	 );

#endif /* MAD_COMMUNICATION_INTERFACE_H */
