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

#include <stdio.h>
#include <stdlib.h>
#include "pm2.h"
#include "dsm_comm.h"
/*
BEGIN_DSM_DATA
int c = 0;
END_DSM_DATA
*/
int req_counter, page_counter, me, other;
marcel_sem_t sem;


void startup_func(int argc, char *argv[], void *arg)
{
  me = pm2_self();
  other = (me == 0) ? 1 : 0;
  req_counter = page_counter = atoi(argv[1]);
  dsm_set_access(0, WRITE_ACCESS);
}


void read_server()
{
 if (req_counter--)
  dsm_send_page_req(other, 0, me, READ_ACCESS, 0);
else
  marcel_sem_V(&sem);
//fprintf(stderr,"rs\n");
}


void receive_page_server()
{
 if (page_counter--)
  dsm_send_page(other, 0, READ_ACCESS, 0);
else
  marcel_sem_V(&sem);
//fprintf(stderr,"rps\n");
}


int pm2_main(int argc, char **argv)
{
  int prot;

#ifdef PROFILE
  profile_activate(FUT_ENABLE, PM2_PROF_MASK | DSM_PROF_MASK);
#endif
  pm2_push_startup_func(startup_func, NULL);
  pm2_set_dsm_page_protection(DSM_UNIFORM_PROTECT, WRITE_ACCESS);
  pm2_set_dsm_page_distribution(DSM_CENTRALIZED, 1);

  prot = dsm_create_protocol(NULL,
			     NULL,
                             read_server,
                             NULL,
                             NULL,
                             receive_page_server,
                             NULL, 
                             NULL, 
                             NULL, 
                             NULL, 
                             NULL 
                             );
  dsm_set_default_protocol(prot);
  marcel_sem_init(&sem, 0);

  pm2_init(&argc, argv);

  if (argc != 2)
    {
      fprintf(stderr, "Usage: perf <iterations> \n");
      exit(1);
    }


  if(pm2_self() == 0) { /* master process */

  pm2_empty();
  read_server();
  marcel_sem_P(&sem);

  pm2_empty();

  receive_page_server();
  marcel_sem_P(&sem);

  pm2_empty();

  pm2_halt();
  }

  pm2_exit();

#ifdef PROFILE
  profile_stop();
#endif

  tfprintf(stderr, "Main is ending\n");
  return 0;
}
