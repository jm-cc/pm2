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
$Log: ntbx_interface.h,v $
Revision 1.1  2000/02/08 15:25:21  oaumage
- ajout du sous-repertoire `net' a la toolbox


______________________________________________________________________________
*/

/*
 * NTbx_interface.h
 * ====================
 */

#ifndef NTBX_INTERFACE_H
#define NTBX_INTERFACE_H

void
ntbx_init(void);

#ifdef NTBX_TCP
void
ntbx_tcp_address_fill(p_ntbx_tcp_address_t  address, 
		      ntbx_tcp_port_t       port,
		      char                 *host_name);

void
ntbx_tcp_socket_setup(ntbx_tcp_socket_t desc);

ntbx_tcp_socket_t
ntbx_tcp_socket_create(p_ntbx_tcp_address_t address,
		       ntbx_tcp_port_t      port);
#endif /* NTBX_TCP */

#endif /* NTBX_INTERFACE_H */
