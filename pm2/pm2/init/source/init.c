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
$Log: init.c,v $
Revision 1.2  2000/11/06 15:02:18  rnamyst
pm2_init() has now a modular structure (in fact, common_init).

Revision 1.1  2000/11/05 20:50:20  rnamyst
The 'common' module now (almost) behaves like a classical one : there is a libcommon.a file, etc.

______________________________________________________________________________
*/

#include "pm2.h"

void common_init(int *argc, char *argv[])
{
#ifdef PM2
  unsigned pm2_self, pm2_conf_size;
#endif

#ifdef PROFILE
  profile_init();
#endif

#ifdef PM2
  pm2_init_data(argc, argv);
#endif

#ifdef MARCEL
  //marcel_init(argc, argv);
#endif

#if defined(MAD1) || defined(MAD2)
  mad_init_thread_related(argc, argv);
#endif

#if defined(MAD1) || defined(MAD2)
  mad_init(argc, argv);

#ifdef PM2
  pm2_self = mad_who_am_i();
  pm2_conf_size = mad_config_size();
#endif
#endif

#ifdef PM2
  pm2_init_open_channels(argc, argv, pm2_self, pm2_conf_size);
#endif

#ifdef PM2
  pm2_init_thread_related(argc, argv);
#endif

#ifdef DSM
  dsm_pm2_init(pm2_self, pm2_conf_size);
#endif

#ifdef PM2
  pm2_init_listen_network(argc, argv);
#endif

#ifdef PM2
  pm2_init_purge_cmdline(argc, argv);
#endif
}
