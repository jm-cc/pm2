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
$Log: mad_forward_interface.h,v $
Revision 1.1  2001/01/03 11:05:27  oaumage
- integration des headers du module de forwarding


______________________________________________________________________________
*/

/*
 * Mad_forward_interface.h
 * =======================
 */

/* *** Note: Rajouter Lionel dans le copyright *** */
#ifndef MAD_FORWARD_INTERFACE_H
#define MAD_FORWARD_INTERFACE_H

void 
mad_forward(p_mad_channel_t in_channel);

p_mad_channel_t 
mad_find_channel(ntbx_host_id_t        remote_host_id,
		 p_mad_user_channel_t  user_channel,
		 ntbx_host_id_t       *gateway);

void
mad_polling(p_mad_channel_t in_channel);

/* *** Note : a supprimer des que possible *** */
extern p_mad_driver_interface_t fwd_interface;

#endif /* MAD_FORWARD_INTERFACE_H */

