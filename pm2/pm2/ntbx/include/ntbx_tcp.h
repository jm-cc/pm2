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
$Log: ntbx_tcp.h,v $
Revision 1.3  2000/04/27 09:01:30  oaumage
- fusion de la branche pm2_mad2_multicluster

Revision 1.2.2.1  2000/04/15 15:20:12  oaumage
- correction de l'entete du fichier

Revision 1.2  2000/02/17 09:14:27  oaumage
- ajout du support de TCP a la net toolbox
- ajout de fonctions d'empaquetage de donnees numeriques

Revision 1.1  2000/02/08 15:25:23  oaumage
- ajout du sous-repertoire `net' a la toolbox


______________________________________________________________________________
*/

/*
 * Ntbx_tcp.h
 * ==========
 */

#ifndef NTBX_TCP_H
#define NTBX_TCP_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>


/*
 * constants
 * ---------
 */

#define NTBX_TCP_MAX_RETRY_BEFORE_SLEEP 10
#define NTBX_TCP_MAX_SLEEP_RETRY         5
#define NTBX_TCP_SLEEP_DELAY             1
#define NTBX_TCP_MAX_TIMEOUT_RETRY       5

#define NTBX_TCP_SELECT_TIMEOUT         30

/*
 * public types
 * ------------
 */

typedef int                ntbx_tcp_port_t,    *p_ntbx_tcp_port_t;
typedef struct sockaddr_in ntbx_tcp_address_t, *p_ntbx_tcp_address_t;
typedef int                ntbx_tcp_socket_t,  *p_ntbx_tcp_socket_t;

/*
 * public structures
 * -----------------
 */
typedef struct
{
  int count;
  int sleep_count;
  int timeout_count;
} ntbx_tcp_retry_t, *p_ntbx_tcp_retry_t;

#endif /* NTBX_TCP_H */
