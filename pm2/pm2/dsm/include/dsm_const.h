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

#ifndef DSM_CONST_IS_DEF
#define DSM_CONST_IS_DEF

//#define DEBUG1
//#define DEBUG2
//#define DEBUG3

typedef enum{NO_ACCESS, READ_ACCESS, WRITE_ACCESS, UNKNOWN_ACCESS} dsm_access_t;

typedef int dsm_node_t;

#define NO_NODE -1

typedef void (*dsm_rf_action_t)(unsigned long index); // read fault handler

typedef void (*dsm_wf_action_t)(unsigned long index); // write fault handler

typedef void (*dsm_rs_action_t)(unsigned long index, dsm_node_t req_node, int tag); // read server

typedef void (*dsm_ws_action_t)(unsigned long index, dsm_node_t req_node, int tag); // write server

typedef void (*dsm_is_action_t)(unsigned long index, dsm_node_t req_node, dsm_node_t new_owner); // invalidate server

typedef void (*dsm_rp_action_t)(void *addr, dsm_access_t access, dsm_node_t reply_node, int tag); // receive page server

typedef void (*dsm_erp_action_t)(void *addr, dsm_access_t access, dsm_node_t reply_node, unsigned long page_size, int tag); // expert receive page server

typedef void (*dsm_acq_action_t)(); // acquire func

typedef void (*dsm_rel_action_t)(); // release func

typedef void (*dsm_pi_action_t)(int prot_id); // acquire func

typedef void (*dsm_pa_action_t)(int index); // release func

#endif


