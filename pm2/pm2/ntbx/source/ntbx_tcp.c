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
$Log: ntbx_tcp.c,v $
Revision 1.1  2000/02/08 15:25:30  oaumage
- ajout du sous-repertoire `net' a la toolbox


______________________________________________________________________________
*/

/*
 * tbx_tcp.c
 * ---------
 */
#include <tbx.h>
#include <ntbx.h>

void
ntbx_tcp_address_fill(p_ntbx_tcp_address_t   address, 
		      ntbx_tcp_port_t        port,
		      char                  *host_name)
{
  struct hostent *host_entry;

  LOG_IN();
  if (!(host_entry = gethostbyname(host_name)))
    FAILURE("ERROR: Cannot find host internet address");
      
  address->sin_family = AF_INET;
  address->sin_port   = htons(port);
  memcpy(&address->sin_addr.s_addr, host_entry->h_addr, host_entry->h_length);
  LOG_OUT();
}

void
ntbx_tcp_socket_setup(ntbx_tcp_socket_t desc)
{
  int           val    = 1;
  int           packet = 0x8000;
  struct linger ling   = { 1, 50 };
  int           len    = sizeof(int);
  
  LOG_IN();      
  SYSCALL(setsockopt(desc, IPPROTO_TCP, TCP_NODELAY, (char *)&val, len));
  SYSCALL(setsockopt(desc, SOL_SOCKET, SO_LINGER, (char *)&ling,
		     sizeof(struct linger)));
  SYSCALL(setsockopt(desc, SOL_SOCKET, SO_SNDBUF, (char *)&packet, len));
  SYSCALL(setsockopt(desc, SOL_SOCKET, SO_RCVBUF, (char *)&packet, len));
  LOG_OUT();
}

ntbx_tcp_socket_t 
ntbx_tcp_socket_create(p_ntbx_tcp_address_t address,
		       ntbx_tcp_port_t      port)
{
  socklen_t          len  = sizeof(ntbx_tcp_address_t);
  ntbx_tcp_address_t temp;
  int                desc;

  LOG_IN();
  SYSCALL(desc = socket(AF_INET, SOCK_STREAM, 0));
  
  temp.sin_family      = AF_INET;
  temp.sin_addr.s_addr = htonl(INADDR_ANY);
  temp.sin_port        = htons(port);

  SYSCALL(bind(desc, (struct sockaddr *)&temp, len));

  if (address)
    SYSCALL(getsockname(desc, (struct sockaddr *)address, &len));

  LOG_OUT();
  return desc;
}

