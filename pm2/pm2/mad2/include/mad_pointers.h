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
$Log: mad_pointers.h,v $
Revision 1.6  2001/01/16 09:55:48  oaumage
- integration du mecanisme de forwarding

Revision 1.5  2001/01/03 11:05:28  oaumage
- integration des headers du module de forwarding

Revision 1.4  2000/11/10 14:17:55  oaumage
- nouvelle procedure d'initialisation

Revision 1.3  2000/03/07 10:38:56  oaumage
- pre-support multi-cluster

Revision 1.2  1999/12/15 17:31:25  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Mad_pointers.h
 * ==============
 */

#ifndef MAD_POINTERS_H
#define MAD_POINTERS_H

/* ... Point-to-point connection ..................................... */
typedef struct s_mad_connection           *p_mad_connection_t;

/* ... Protocol generic interface .................................... */
typedef struct s_mad_driver_interface     *p_mad_driver_interface_t;

/* ... Transfer method ............................................... */
typedef struct s_mad_link                 *p_mad_link_t;

/* ... Protocol module ............................................... */
typedef struct s_mad_driver               *p_mad_driver_t;

/* ... Network interface card ........................................ */
typedef struct s_mad_adapter              *p_mad_adapter_t;

/* ... Cluster members ............................................... */
typedef struct s_mad_configuration        *p_mad_configuration_t;

/* ... Virtual buffers ............................................... */
typedef struct s_mad_buffer               *p_mad_buffer_t;

/* ... Virtual buffer pair ........................................... */
typedef struct s_mad_buffer_pair          *p_mad_buffer_pair_t;

/* ... Group of virtual buffers ...................................... */
typedef struct s_mad_buffer_group         *p_mad_buffer_group_t;

/* ... Communication channel ......................................... */
typedef struct s_mad_channel              *p_mad_channel_t;

/* ... Main madeleine object ......................................... */
typedef struct s_mad_madeleine            *p_mad_madeleine_t;

/* ... Madeleine module settings ..................................... */
typedef struct s_mad_settings             *p_mad_settings_t;

/* ... Adapter identification ........................................ */
typedef struct s_mad_adapter_description  *p_mad_adapter_description_t;

/* ... Set of adapter identifications ................................ */
typedef struct s_mad_adapter_set          *p_mad_adapter_set_t;

/* ... Channel supporting hosts ...................................... */
typedef struct s_mad_cluster              *p_mad_cluster_t;

/* ... Virtual channel characteristics ............................... */
typedef struct s_mad_channel_description  *p_mad_channel_description_t;

#ifdef MAD_FORWARDING
/* ... User channel characteristics .................................. */
typedef struct s_mad_user_channel         *p_mad_user_channel_t;
#endif /* MAD_FORWARDING */

#endif /* MAD_POINTERS_H */

