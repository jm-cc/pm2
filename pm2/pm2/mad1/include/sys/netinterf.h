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

#ifndef NETINTERF_EST_DEF
#define NETINTERF_EST_DEF

#include <sys/uio.h>
#include <mad_types.h>

typedef void (*network_init_func_t)(int *argc, char **argv, int nb_proc, int *tids, int *nb, int *whoami);

typedef void (*network_send_func_t)(int dest_node, struct iovec *vector, size_t count);

typedef void (*network_receive_func_t)(char **head);

typedef void (*network_receive_data_func_t)(struct iovec *vector, size_t count);

typedef void (*network_exit_func_t)(void);

typedef struct {
  network_init_func_t         network_init;
  network_send_func_t         network_send;
  network_receive_func_t      network_receive;
  network_receive_data_func_t network_receive_data;
  network_exit_func_t         network_exit;
} netinterf_t;

#endif



