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
$Log: mad_forward.h,v $
Revision 1.1  2001/01/03 11:05:27  oaumage
- integration des headers du module de forwarding


______________________________________________________________________________
*/

/*
 * Mad_forward.h
 * =============
 */

/* *** Note: Rajouter Lionel dans le copyright *** */
#ifndef MAD_FORWARD_H
#define MAD_FORWARD_H

typedef struct s_mad_msg_hdr_fwd
{
  size_t             length;  /* Pour un groupe, donne le nb de buffers
				 dans le groupe */
  mad_send_mode_t    send_mode;
  mad_receive_mode_t recv_mode;
  mad_link_id_t      in_link_id;
} mad_msg_hdr_fwd_t, *p_mad_msg_hdr_fwd_t;

typedef struct s_mad_group_hdr_fwd
{
  size_t             length; 
  /*  mad_send_mode_t    send_mode;
      mad_receive_mode_t recv_mode;*/
} mad_msg_group_hdr_fwd_t;

typedef struct s_mad_hdr_fwd 
{
  ntbx_host_id_t destination;
} mad_hdr_fwd_t, *p_mad_hdr_fwd_t;

#endif /* MAD_FORWARD_H */

