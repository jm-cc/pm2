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

#ifndef DSM_SYSDEP_IS_DEF
#define DSM_SYSDEP_IS_DEF

#include "dsm_page_size.h" /* generated file */
#include "dsm_const.h"

typedef void ( *dsm_pagefault_handler_t)(int sig, void *addr, dsm_access_t access);
typedef void (*dsm_std_handler_t )(int); 

void dsm_install_pagefault_handler(dsm_pagefault_handler_t handler);

void dsm_uninstall_pagefault_handler();

void dsm_setup_secondary_SIGSEGV_handler(dsm_std_handler_t handler_func);

typedef void (*dsm_safe_ss_t)(char*t);

void dsm_safe_ss(dsm_safe_ss_t f);

#endif

