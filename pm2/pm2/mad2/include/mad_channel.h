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
$Log: mad_channel.h,v $
Revision 1.5  2001/01/03 11:05:25  oaumage
- integration des headers du module de forwarding

Revision 1.4  2000/03/08 17:18:48  oaumage
- support de compilation avec Marcel sans PM2
- pre-support de packages de Threads != Marcel

Revision 1.3  2000/01/13 14:44:31  oaumage
- adaptation pour la prise en compte de la toolbox
- suppression des fichiers redondant

Revision 1.2  1999/12/15 17:31:19  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Mad_channel.h
 * =============
 */

#ifndef MAD_CHANNEL_H
#define MAD_CHANNEL_H

typedef struct s_mad_channel
{
  TBX_SHARED;
  mad_channel_id_t          id ;
  p_mad_adapter_t           adapter;
#ifdef MAD_FORWARDING
  tbx_bool_t                is_forward;
  struct s_mad_user_channel *user_channel;
#endif //MAD_FORWARDING
  volatile tbx_bool_t       reception_lock;
  p_mad_connection_t        input_connection; 
  p_mad_connection_t        output_connection;
  p_mad_driver_specific_t   specific;
} mad_channel_t;

#ifdef MAD_FORWARDING
typedef struct s_mad_user_channel
{
  TBX_SHARED;
  p_mad_channel_t  *channels;
  p_mad_channel_t  *forward_channels;
  mad_adapter_id_t *adapter_ids;
  ntbx_host_id_t   *gateways;

  int               nb_active_channels;
  marcel_sem_t      sem_message_ready;
  marcel_sem_t      sem_lock_msgs;
  marcel_sem_t      sem_msg_handled;
  p_mad_connection_t msg_connection;

  char* name;
} mad_user_channel_t, *p_mad_user_channel_t;
#endif //MAD_FORWARDING

#endif /* MAD_CHANNEL_H */
