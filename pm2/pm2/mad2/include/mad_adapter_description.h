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
$Log: mad_adapter_description.h,v $
Revision 1.2  1999/12/15 17:31:18  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Mad_adapter_description.h
 * ==================
 */

#ifndef MAD_ADAPTER_DESCRIPTION_H
#define MAD_ADAPTER_DESCRIPTION_H

typedef struct s_mad_adapter_description
{
  mad_driver_id_t   driver_id;
  char             *adapter_selector;
} mad_adapter_description_t;

typedef struct s_mad_adapter_set
{
  int                           size;
  p_mad_adapter_description_t   description;
} mad_adapter_set_t;

#endif /* MAD_ADAPTER_DESCRIPTION_H */
