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
$Log: mad_registration.h,v $
Revision 1.3  2000/01/10 10:19:42  oaumage
- mad_macros.h: modification de la macro de commande de trace

Revision 1.2  1999/12/15 17:31:25  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Mad_registration.h
 * ==================
 */

#ifndef MAD_REGISTRATION_H
#define MAD_REGISTRATION_H
/*
 * Protocol module identifier
 * --------------------------
 */
typedef enum
{
#ifdef DRV_TCP
  mad_TCP,
#endif /* DRV_TCP */
#ifdef DRV_VIA
  mad_VIA,
#endif /* DRV_VIA */
#ifdef DRV_SISCI
  mad_SISCI,
#endif /* DRV_SISCI */
#ifdef DRV_SBP
  mad_SBP,
#endif /* DRV_SBP */,
#ifdef DRV_MPI
  mad_MPI,
#endif /* DRV_MPI */,
  mad_driver_number /* Must be the last element of the enum declaration */
} mad_driver_id_t, *p_mad_driver_id_t;

#endif /* MAD_REGISTRATION_H */
