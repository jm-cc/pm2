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
$Log: slot_distrib.c,v $
Revision 1.4  2000/07/20 09:04:17  oaumage
- Corrections diverses

Revision 1.3  2000/02/28 11:17:22  rnamyst
Changed #include <> into #include "".

Revision 1.2  2000/01/31 15:58:35  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/


#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "sys/archdep.h"
#include "pm2.h"
#include "isomalloc.h"
#include "sys/slot_distrib.h"

void UNIFORM_BLOCK_CYCLIC_DISTRIB_FUNC(int *arg)
{
  /* Fonction non declaree (mise en commentaire dans isomalloc.h) */
  pm2_set_uniform_slot_distribution((unsigned int)arg, -1);
}


void NON_UNIFORM_BLOCK_CYCLIC_DISTRIB_FUNC(int *arg)
{
int i, n = arg[0];
int offset = 0;
int period = 0;

  for (i = 1; i <= n; i++) 
    period += arg[i];

  /* Idem ici: pm2_set_non_uniform_slot_distribution n'est plus declaree dans 
     isomalloc.h */
  for (i = 0; i < n; offset += arg[i+1], i++)
    pm2_set_non_uniform_slot_distribution(i, offset, arg[i+1], period, -1);
}


void UNIFORM_ADAPTIVE_BLOCK_CYCLIC_DISTRIB_FUNC(int *arg)
{
  pm2_set_uniform_slot_distribution((unsigned int)arg, -1);
}

void NON_UNIFORM_ADAPTIVE_BLOCK_CYCLIC_DISTRIB_FUNC(int *arg)
{
int i, n = arg[0];
int offset = 0;
int period = 0;

  for (i = 1; i <= n; i++) 
    period += arg[i];

  for (i = 0; i < n; offset += arg[i+1], i++)
    pm2_set_non_uniform_slot_distribution(i, offset, arg[i+1], period, -1);
}

