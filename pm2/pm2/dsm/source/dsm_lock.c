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
*/

#include "marcel.h"
#include "dsm_lock.h"

#define DSM_LOCKS 15

static struct {
  dsm_lock_struct_t locks[DSM_LOCKS];
  int nb;
} _dsm_lock_table;


void dsm_lock_init(dsm_lock_t *lock, dsm_lock_attr_t *attr)
{
  int i;

  LOG_IN();

  *lock = &_dsm_lock_table.locks[_dsm_lock_table.nb++];

  if(!attr)
    (*lock)->dsm_mutex.owner = 0;
  else
    (*lock)->dsm_mutex.owner = attr->dsm_mutex_attr.owner;

  marcel_mutex_init(&((*lock)->dsm_mutex.mutex), NULL);

  if (attr)
    {
      (*lock)->nb_prot = attr->nb_prot;
      
      for(i = 0; i < attr->nb_prot; i++)
	((*lock)->prot)[i] = (attr->prot)[i];
    }
  else
     (*lock)->nb_prot =0;
  
  LOG_OUT();
}




