/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

/*
 * Ntbx_udp.h
 * ==========
 */

#ifndef NTBX_UDP_H
#define NTBX_UDP_H

#include <sys/uio.h>


/*
 * constants
 * ---------
 */


/* Three predefined sizes for UDP socket buffers */
#define NTBX_UDP_SO_BUF_LARGE   0x20000
#define NTBX_UDP_SO_BUF_MID     0x4000
#define NTBX_UDP_SO_BUF_SMALL   0x400

/* Max size in bytes of a UDP datagram */
#define NTBX_UDP_MAX_DGRAM_SIZE    (0x10000 - 48)


/*
 * public types
 * ------------
 */

typedef unsigned int       ntbx_udp_port_t,    *p_ntbx_udp_port_t;
typedef struct in_addr     ntbx_udp_in_addr_t, *p_ntbx_udp_in_addr_t;
typedef struct sockaddr_in ntbx_udp_address_t, *p_ntbx_udp_address_t;
typedef int                ntbx_udp_socket_t,  *p_ntbx_udp_socket_t;

typedef int                ntbx_udp_request_id_t;

typedef struct iovec       ntbx_udp_iovec_t,   *p_ntbx_udp_iovec_t;


#endif /* NTBX_UDP_H */








