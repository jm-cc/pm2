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
$Log: mad_forwarder.h,v $
Revision 1.1  2001/01/03 11:05:29  oaumage
- integration des headers du module de forwarding


______________________________________________________________________________
*/

/*
 * Mad_forwarder.h
 * ===============
 */
#ifndef MAD_FORWARDER_H
#define MAD_FORWARDER_H

/*
 * Structures
 * ----------
 */

/*
 * Functions 
 * ---------
 */

void
mad_fwd_link_init(p_mad_link_t);

void
mad_fwd_new_message(p_mad_connection_t);

void
mad_fwd_send_buffer(p_mad_link_t,
		    p_mad_buffer_t);

void
mad_fwd_send_buffer_group(p_mad_link_t,
			  p_mad_buffer_group_t);

void
mad_fwd_receive_buffer(p_mad_link_t,
		       p_mad_buffer_t *);

void
mad_fwd_receive_sub_buffer_group(p_mad_link_t,
				 tbx_bool_t,
				 p_mad_buffer_group_t);

#endif /* MAD_FORWARDER_H */
