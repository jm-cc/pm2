/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 2.0

             Gabriel Antoniu, Luc Bouge, Christian Perez,
                Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 8512 CNRS-INRIA
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

#ifndef DSM_PM2_IS_DEF
#define DSM_PM2_IS_DEF

#include "dsm_const.h" /* access_t */
#include "dsm_sysdep.h"

/* the following 2 need to be included by the user
 * program to allow protocol initialization 
 * using library functions
 */
#include "dsm_protocol_lib.h" 
#include "dsm_protocol_policy.h"

/* to enable the user to configure the distribution of dsm pages */
#include "dsm_page_manager.h"

/* to enable the user to use dsm mutex */
#include "dsm_mutex.h"

#define BEGIN_DSM_DATA \
  asm (".data"); \
  asm (DSM_ASM_PAGEALIGN); \
  char dsm_data_begin = {}; 

#define END_DSM_DATA \
  char dsm_data_end = {}; \
  asm (DSM_ASM_PAGEALIGN);

#define DSM_NEWPAGE \
  asm (DSM_ASM_PAGEALIGN);

void dsm_pm2_init(int my_rank, int confsize);

void dsm_pm2_exit();

void pm2_set_dsm_protocol(dsm_protocol_t *protocol);

#endif




