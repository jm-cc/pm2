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
$Log: mad_adapter.h,v $
Revision 1.3  2000/03/08 17:18:46  oaumage
- support de compilation avec Marcel sans PM2
- pre-support de packages de Threads != Marcel

Revision 1.2  1999/12/15 17:31:18  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Mad_adapter.h
 * ==============
 */

#ifndef MAD_ADAPTER_H
#define MAD_ADAPTER_H

typedef struct s_mad_adapter
{
  /* Common use fields */
  TBX_SHARED;
  p_mad_driver_t            driver;
  mad_adapter_id_t          id; 
  char                     *name;
  char                     *master_parameter;
  char                     *parameter;
  p_mad_driver_specific_t   specific; 
} mad_adapter_t;

#endif /* MAD_ADAPTER_H */
