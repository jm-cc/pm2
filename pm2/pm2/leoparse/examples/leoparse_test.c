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
$Log: leoparse_test.c,v $
Revision 1.2  2000/11/16 13:24:03  oaumage
- mise a jour initialisation


______________________________________________________________________________
*/

/*
 * leoparse_test.c
 * ===============
 */ 

#include <stdlib.h>
#include <stdio.h>
#include "leoparse.h"

void
usage(void)
{
  fprintf(stderr, "usage: pm2_load leoparse_test <conf_file>\n");
  exit(1);
}

int
main(int    argc,
     char **argv)
{
  p_tbx_htable_t result = NULL;

  tbx_init(argc, argv);
  leoparse_init(argc, argv);

  tbx_purge_cmd_line(&argc, argv);
  leoparse_purge_cmd_line(&argc, argv);

  argc--; argv++;
  if (!argc)
    usage();

  result = leoparse_parse_local_file(*argv);
  printf("Parsing succeeded\n");
  
  return 0;
}

