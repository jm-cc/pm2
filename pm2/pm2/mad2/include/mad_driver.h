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
$Log: mad_driver.h,v $
Revision 1.3  2000/01/13 14:44:31  oaumage
- adaptation pour la prise en compte de la toolbox
- suppression des fichiers redondant

Revision 1.2  1999/12/15 17:31:21  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Mad_driver.h
 * ============
 */

#ifndef MAD_DRIVER_H
#define MAD_DRIVER_H

typedef enum
{
  mad_unidirectional_connection,
  mad_bidirectional_connection

} mad_connection_type_t, *p_mad_connection_type_t;

typedef struct s_mad_driver
{
  /* Common use fields */
  PM2_SHARED;
  p_mad_madeleine_t           madeleine;
  mad_driver_id_t             id;
  mad_driver_interface_t      interface;
  
  /* Settings */
  mad_buffer_alignment_t      buffer_alignment;
  mad_connection_type_t       connection_type;

  /* Internal use fields */
  tbx_list_t                  adapter_list;

  /* Driver specific */
  p_mad_driver_specific_t     specific;
} mad_driver_t;

#endif /* MAD_DRIVER_H */
