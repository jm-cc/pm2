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

typedef struct s_mad_connection           *p_mad_connection_t;
typedef struct s_mad_driver_interface     *p_mad_driver_interface_t;
typedef struct s_mad_link                 *p_mad_link_t;
typedef struct s_mad_driver               *p_mad_driver_t;
typedef struct s_mad_adapter              *p_mad_adapter_t;
typedef struct s_mad_configuration        *p_mad_configuration_t;
typedef struct s_mad_buffer               *p_mad_buffer_t;
typedef struct s_mad_buffer_pair          *p_mad_buffer_pair_t;
typedef struct s_mad_buffer_group         *p_mad_buffer_group_t;
typedef struct s_mad_channel              *p_mad_channel_t;
typedef struct s_mad_list_element         *p_mad_list_element_t;
typedef struct s_mad_list                 *p_mad_list_t;
typedef struct s_mad_list_reference       *p_mad_list_reference_t;
typedef struct s_mad_madeleine            *p_mad_madeleine_t;
typedef struct s_mad_memory               *p_mad_memory_t;
typedef struct s_mad_adapter_description  *p_mad_adapter_description_t;
typedef struct s_mad_adapter_set          *p_mad_adapter_set_t;

#endif /* MAD_POINTERS_H */

