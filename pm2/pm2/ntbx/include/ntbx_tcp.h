
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
#define NTBX_TCP_MAX_SLEEP_RETRY        12
#define NTBX_TCP_SLEEP_DELAY             5
#define NTBX_TCP_MAX_TIMEOUT_RETRY      10

#define NTBX_TCP_SELECT_TIMEOUT         60

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
