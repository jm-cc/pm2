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
 * Ntbx_tcp.c
 * ----------
 */


#include "tbx.h"
#include "ntbx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
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
#ifdef PIOMAN
#include "pioman.h"
#endif
/*
 * Macro and constant definition
 * ========================================================================
 */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif /* MAXHOSTNAMELEN */

#if defined(SOLARIS_SYS) || defined(IRIX_SYS) || defined(DARWIN_SYS)
#define socklen_t int
#endif // SOLARIS_SYS || IRIX_SYS || DARWIN_SYS

/*
 * Data Structures
 * ========================================================================
 */
typedef struct s_ntbx_tcp_client_specific
{
        ntbx_tcp_socket_t descriptor;
} ntbx_tcp_client_specific_t, *p_ntbx_tcp_client_specific_t;


typedef struct s_ntbx_tcp_server_specific
{
        ntbx_tcp_socket_t descriptor;
        ntbx_tcp_port_t   port;
} ntbx_tcp_server_specific_t, *p_ntbx_tcp_server_specific_t;

/*
 * Functions
 * ========================================================================
 */

/*
 * General Purpose functions
 * -------------------------
 */

char *
ntbx_tcp_h_errno_to_str(void) {
        char *msg = NULL;

        switch (h_errno) {
                case HOST_NOT_FOUND:
                        msg = "HOST_NOT_FOUND";
                break;

                case TRY_AGAIN:
                        msg = "TRY_AGAIN";
                break;

                case NO_RECOVERY:
                        msg = "NO_RECOVERY";
                break;

                case NO_ADDRESS:
                        msg = "NO_ADDRESS";
                break;
        }

        return msg;
}

/* ...Read/Write services ..............*/

ssize_t
ntbx_tcp_read_call(int		 s,
                   void		*p,
                   size_t	 l) {
      ssize_t result = 0;

#if(defined(PIOMAN) && !defined(PIOM_DISABLE_LTASKS))
      result = piom_task_read(s, p, l);
#elif defined(MARCEL)
      result = marcel_read(s, p, l);
#else 
      result = read(s, p, l);
#endif /* PIOMAN */

      return result;
}

ssize_t
ntbx_tcp_write_call(int		 	 s,
                    const void		*p,
                    const size_t	 l) {
      ssize_t result = 0;

#if(defined(PIOMAN) && !defined(PIOM_DISABLE_LTASKS))
      result = piom_task_write(s, p, l);
#elif defined(MARCEL)
      result = marcel_write(s, p, l);
#else 
      result = write(s, p, l);
#endif /* PIOMAN */

      return result;
}


/* read a block */
void
ntbx_tcp_read(int     socket_fd,
	      void   *ptr,
	      size_t  length)
{
        size_t bytes_read = 0;
        PM2_LOG_IN();
        while (bytes_read < length) {
                int status;

                status = ntbx_tcp_read_call(socket_fd, ptr + bytes_read, length - bytes_read);

                if (status == -1) {
                        if (errno == EINTR) {
                                continue;
                        } else {
                                perror("read");
                                TBX_FAILURE("system call failed");
                        }
                } else if (status == 0) {
                        TBX_FAILURE("connection closed");
                } else {
                        bytes_read += status;
                }
        }
        PM2_LOG_OUT();
}


/* write a block */
void
ntbx_tcp_write(int           socket_fd,
	       const void   *ptr,
	       const size_t  length)
{
        size_t bytes_written = 0;
        PM2_LOG_IN();
        while (bytes_written < length) {
                int status;

                status = ntbx_tcp_write_call(socket_fd, ptr + bytes_written, length - bytes_written);

                if (status == -1) {
                        if (errno == EINTR) {
                                continue;
                        } else {
                                perror("write");
                                TBX_FAILURE("system call failed");
                        }
                } else if (status == 0) {
                        TBX_FAILURE("connection closed");
                } else {
                        bytes_written += status;
                }
        }
        PM2_LOG_OUT();
}

/*
 * Setup functions
 * ---------------
 */
void
ntbx_tcp_address_fill(p_ntbx_tcp_address_t   address,
		      ntbx_tcp_port_t        port,
		      char                  *host_name) {
        struct hostent *host_entry;

        PM2_LOG_IN();
        if (!(host_entry = gethostbyname(host_name))) {
                char *msg = NULL;

                msg = ntbx_tcp_h_errno_to_str();
                TBX_FAILUREF("ntbx_tcp_address_fill: gethostbyname: %s", msg);
        }

        address->sin_family = AF_INET;
        address->sin_port   = htons(port);
        memcpy(&address->sin_addr.s_addr,
               host_entry->h_addr,
               (size_t)host_entry->h_length);

        memset(address->sin_zero, 0, 8);
        PM2_LOG_OUT();
}

void
ntbx_tcp_address_fill_ip(p_ntbx_tcp_address_t   address,
			 ntbx_tcp_port_t        port,
			 unsigned long         *ip) {
        struct hostent *host_entry;

        PM2_LOG_IN();
        if (!(host_entry = gethostbyaddr((char *)ip,
                                         sizeof(unsigned long), AF_INET))) {
                char *msg = NULL;

                msg = ntbx_tcp_h_errno_to_str();
                TBX_FAILUREF("ntbx_tcp_address_fill_ip: gethostbyaddr: %s", msg);
        }

        address->sin_family = AF_INET;
        address->sin_port   = htons(port);
        memcpy(&address->sin_addr.s_addr,
               host_entry->h_addr,
               (size_t)host_entry->h_length);
        memset(address->sin_zero, 0, 8);
        PM2_LOG_OUT();
}

void
ntbx_tcp_socket_setup(ntbx_tcp_socket_t desc) {
        int           val    = 1;
        socklen_t     len    = sizeof(int);
        struct linger ling   = { 1, 50 };

        PM2_LOG_IN();
        SYSCALL(setsockopt(desc, IPPROTO_TCP, TCP_NODELAY, (char *)&val, len));
        SYSCALL(setsockopt(desc, SOL_SOCKET, SO_LINGER, (char *)&ling,
                           sizeof(struct linger)));
        PM2_LOG_OUT();
}

void
ntbx_tcp_socket_setup_delay(ntbx_tcp_socket_t desc) {
        int           val    = 0;
        socklen_t     len    = sizeof(int);

        PM2_LOG_IN();
        SYSCALL(setsockopt(desc, IPPROTO_TCP, TCP_NODELAY, (char *)&val, len));
        PM2_LOG_OUT();
}

void
ntbx_tcp_socket_setup_nodelay(ntbx_tcp_socket_t desc) {
        int           val    = 1;
        socklen_t     len    = sizeof(int);

        PM2_LOG_IN();
        SYSCALL(setsockopt(desc, IPPROTO_TCP, TCP_NODELAY, (char *)&val, len));
        PM2_LOG_OUT();
}


void
ntbx_tcp_client_setup_delay(p_ntbx_client_t  client) {
        p_ntbx_tcp_client_specific_t client_specific = client->specific;
        ntbx_tcp_socket_t desc	= client_specific->descriptor;
        int           val    = 0;
        socklen_t     len    = sizeof(int);

        PM2_LOG_IN();
        SYSCALL(setsockopt(desc, IPPROTO_TCP, TCP_NODELAY, (char *)&val, len));
        PM2_LOG_OUT();
}

void
ntbx_tcp_client_setup_nodelay(p_ntbx_client_t  client) {
        p_ntbx_tcp_client_specific_t client_specific = client->specific;
        ntbx_tcp_socket_t desc	= client_specific->descriptor;
        int           val    = 1;
        socklen_t     len    = sizeof(int);

        PM2_LOG_IN();
        SYSCALL(setsockopt(desc, IPPROTO_TCP, TCP_NODELAY, (char *)&val, len));
        PM2_LOG_OUT();
}

ntbx_tcp_socket_t
ntbx_tcp_socket_create(p_ntbx_tcp_address_t address,
		       ntbx_tcp_port_t      port) {
        socklen_t          len  = sizeof(ntbx_tcp_address_t);
        ntbx_tcp_address_t temp;
        int                desc;

        PM2_LOG_IN();
        SYSCALL(desc = socket(AF_INET, SOCK_STREAM, 0));

        temp.sin_family      = AF_INET;
        temp.sin_addr.s_addr = htonl(INADDR_ANY);
        temp.sin_port        = htons(port);

        SYSCALL(bind(desc, (struct sockaddr *)&temp, len));

        if (address) {
                SYSCALL(getsockname(desc, (struct sockaddr *)address, &len));
        }

        PM2_LOG_OUT();
        return desc;
}

/*
 * Communication functions
 * -----------------------
 */


/*...Initialisation.....................*/

/* Setup a server socket.  */
void
ntbx_tcp_server_init(p_ntbx_server_t server)
{
        PM2_LOG_IN();
        ntbx_tcp_server_init_ext(server, 0);
        PM2_LOG_OUT();
}


/* Setup a server socket. If the port number is 0, it is automatically assigned, otherwise the given value is used */
void
ntbx_tcp_server_init_ext(p_ntbx_server_t server,
                         unsigned short int port)
{
        p_ntbx_tcp_server_specific_t tcp_specific = NULL;
        struct hostent              *local_host_entry = NULL;
        ntbx_tcp_address_t           address;

        PM2_LOG_IN();
        server->local_host = TBX_MALLOC(MAXHOSTNAMELEN + 1);
        CTRL_ALLOC(server->local_host);
        gethostname(server->local_host, MAXHOSTNAMELEN);

        /*
          {
          int l = strlen(server->local_host);

          if (l < MAXHOSTNAMELEN-1){
          server->local_host[l++] = '.';
          getdomainname(server->local_host+l, MAXHOSTNAMELEN-l);
          }
          }
          DISP("hostname = [%s]", server->local_host);
        */

        local_host_entry = gethostbyname(server->local_host);
        if (!local_host_entry) {
                char *msg = NULL;

                msg = ntbx_tcp_h_errno_to_str();
                TBX_FAILUREF("ntbx_tcp_server_init: gethostbyname(%s): %s", server->local_host, msg);
        }

        server->local_host_ip =
                (unsigned long) *(unsigned long *)(local_host_entry->h_addr);

        {
                char **ptr = local_host_entry->h_aliases;

                while (*ptr)
                        {
                                char *alias = NULL;

                                alias = tbx_strdup(*ptr);
                                tbx_slist_append(server->local_alias, alias);

                                ptr++;
                        }
        }

        tbx_slist_append(server->local_alias, server->local_host);
        server->local_host = tbx_strdup(local_host_entry->h_name);

        tcp_specific = TBX_MALLOC(sizeof(ntbx_tcp_server_specific_t));
        CTRL_ALLOC(tcp_specific);
        server->specific = tcp_specific;

        tcp_specific->descriptor = ntbx_tcp_socket_create(&address, port);
        SYSCALL(listen(tcp_specific->descriptor, tbx_min(5, SOMAXCONN)));

        ntbx_tcp_socket_setup(tcp_specific->descriptor);

        tcp_specific->port = (ntbx_tcp_port_t)ntohs(address.sin_port);
        sprintf(server->connection_data.data, "%d", tcp_specific->port);
        PM2_LOG_OUT();
}


/* Setup a client socket */
void
ntbx_tcp_client_init(p_ntbx_client_t client)
{
        p_ntbx_tcp_client_specific_t tcp_specific  = NULL;
        struct hostent              *local_host_entry = NULL;
        ntbx_tcp_address_t           address;

        PM2_LOG_IN();
        client->local_host = TBX_MALLOC(MAXHOSTNAMELEN + 1);
        memset(client->local_host, 0, MAXHOSTNAMELEN + 1);
        gethostname(client->local_host, MAXHOSTNAMELEN);

        local_host_entry = gethostbyname(client->local_host);
        if (!local_host_entry) {
                char *msg = NULL;

                msg = ntbx_tcp_h_errno_to_str();
                TBX_FAILUREF("ntbx_tcp_client_init: gethostbyname: %s", msg);
        }

#ifdef LEO_IP
        client->local_host_ip =
                (unsigned long) *(unsigned long *)(local_host_entry->h_addr);
#endif // LEO_IP

        {
                char **ptr = local_host_entry->h_aliases;

                while (*ptr)
                        {
                                char *alias = NULL;

                                alias = tbx_strdup(*ptr);
                                tbx_slist_append(client->local_alias, alias);

                                ptr++;
                        }
        }

        tbx_slist_append(client->local_alias, client->local_host);
        client->local_host = tbx_strdup(local_host_entry->h_name);

        client->remote_host = NULL;

        tcp_specific = TBX_MALLOC(sizeof(ntbx_tcp_client_specific_t));
        CTRL_ALLOC(tcp_specific);
        client->specific = tcp_specific;

        tcp_specific->descriptor = ntbx_tcp_socket_create(&address, 0);

        ntbx_tcp_socket_setup(tcp_specific->descriptor);
        PM2_LOG_OUT();
}


/*...Reset..............................*/

/* Reset a client object */
void
ntbx_tcp_client_reset(p_ntbx_client_t client)
{
        p_ntbx_tcp_client_specific_t tcp_specific = NULL;

        PM2_LOG_IN();
        tcp_specific = client->specific;

        while (!tbx_slist_is_nil(client->local_alias)) {
                char *str = NULL;

                str = tbx_slist_extract(client->local_alias);
                TBX_FREE(str);
        }

        while (!tbx_slist_is_nil(client->remote_alias)) {
                char *str = NULL;

                str = tbx_slist_extract(client->remote_alias);
                TBX_FREE(str);
        }

        SYSCALL(close(tcp_specific->descriptor));

        TBX_FREE(tcp_specific);
        client->specific = NULL;

        TBX_FREE(client->remote_host);
        client->remote_host = NULL;

        client->read_rq		= 0;
        client->read_rq_flag	= tbx_false;
        client->write_rq	= 0;
        client->write_rq_flag	= tbx_false;
        PM2_LOG_OUT();
}


/* Reset a server object */
void
ntbx_tcp_server_reset(p_ntbx_server_t server)
{
        p_ntbx_tcp_server_specific_t tcp_specific = NULL;

        PM2_LOG_IN();
        tcp_specific = server->specific;

        while (!tbx_slist_is_nil(server->local_alias)) {
                char *str = NULL;

                str = tbx_slist_extract(server->local_alias);
                TBX_FREE(str);
        }

        SYSCALL(close(tcp_specific->descriptor));
        TBX_FREE(tcp_specific);
        server->specific = NULL;
        PM2_LOG_OUT();
}


/*...Connection.........................*/

/* Connect a client socket to a server */
static
ntbx_status_t
ntbx_tcp_client_connect_body(p_ntbx_client_t           client,
			     char                     *server_host_name,
			     unsigned long            *server_ip,
			     p_ntbx_connection_data_t  server_connection_data)
{
        p_ntbx_tcp_client_specific_t client_specific = client->specific;
        struct hostent              *remote_host_entry  = NULL;
        ntbx_tcp_port_t              server_port     =
                atoi((char *)server_connection_data);
        ntbx_tcp_address_t           server_address;

        PM2_LOG_IN();
        if (server_ip)
                ntbx_tcp_address_fill_ip(&server_address, server_port, server_ip);
        else if (server_host_name)
                ntbx_tcp_address_fill(&server_address, server_port, server_host_name);
        else
                TBX_FAILURE("TCP client connect failed");

        while (connect(client_specific->descriptor,
                       (struct sockaddr *)&server_address,
                       sizeof(ntbx_tcp_address_t)) == -1) {
                if (errno == EINTR) {
                        continue;
                } else {
                        perror("connect");
                        TBX_FAILURE("ntbx_tcp_client_connect failed");
                }
        }

        remote_host_entry   = (server_ip)
                ? gethostbyaddr((char *)server_ip, sizeof(unsigned long), AF_INET)
                : gethostbyname(server_host_name);
        if (!remote_host_entry) {
                char *msg = NULL;

                msg = ntbx_tcp_h_errno_to_str();
                TBX_FAILUREF("ntbx_tcp_client_connect_body: %s: %s", ((server_ip)?"gethostbyaddr":"gethostbyname"), msg);
        }
        client->remote_host = tbx_strdup(remote_host_entry->h_name);

        {
                char **ptr = remote_host_entry->h_aliases;

                while (*ptr) {
                        char *alias = NULL;

                        alias = tbx_strdup(*ptr);
                        tbx_slist_append(client->remote_alias, alias);

                        ptr++;
                }
        }

        PM2_LOG_OUT();
        return ntbx_success;
}

/* Connect a client socket to a server from its ip (network format) */
ntbx_status_t
ntbx_tcp_client_connect_ip(p_ntbx_client_t           client,
			   unsigned long             server_ip,
			   p_ntbx_connection_data_t  server_connection_data)
{
        return ntbx_tcp_client_connect_body(client, NULL, &server_ip,
                                            server_connection_data);
}

/* Connect a client socket to a server from its name */
ntbx_status_t
ntbx_tcp_client_connect(p_ntbx_client_t           client,
			char                     *server_host_name,
			p_ntbx_connection_data_t  server_connection_data)
{
        return ntbx_tcp_client_connect_body(client, server_host_name, NULL,
                                            server_connection_data);
}

/* Accept an incoming client connection request */
ntbx_status_t
ntbx_tcp_server_accept(p_ntbx_server_t server, p_ntbx_client_t client)
{
        p_ntbx_tcp_server_specific_t server_specific    = server->specific;
        p_ntbx_tcp_client_specific_t client_specific    = NULL;
        unsigned int                 remote_address_len = sizeof(ntbx_tcp_address_t);
        struct hostent              *remote_host_entry  = NULL;
        ntbx_tcp_address_t           remote_address;
        ntbx_tcp_socket_t            descriptor;

        PM2_LOG_IN();
        while ((descriptor = accept(server_specific->descriptor,
                                    (struct sockaddr *)&remote_address,
                                    &remote_address_len)) == -1) {
                if (errno == EINTR) {
                        continue;
                } else {
                        perror("accept");
                        TBX_FAILURE("ntbx_tcp_server_accept");
                }
        }

        client_specific = TBX_MALLOC(sizeof(ntbx_tcp_client_specific_t));
        CTRL_ALLOC(client_specific);

        client_specific->descriptor = descriptor;
        client->specific            = client_specific;

        remote_host_entry =
                gethostbyaddr((const char *)&remote_address.sin_addr.s_addr,
                              sizeof(remote_address.sin_addr.s_addr),
                              remote_address.sin_family);

        if (!remote_host_entry) {
                char *msg = NULL;

                msg = ntbx_tcp_h_errno_to_str();
                TBX_FAILUREF("ntbx_tcp_server_accept: gethostbyaddr: %s", msg);
        }

        client->remote_host = tbx_strdup(remote_host_entry->h_name);

        {
                char **ptr = remote_host_entry->h_aliases;

                while (*ptr) {
                        char *alias = NULL;

                        alias = tbx_strdup(*ptr);
                        tbx_slist_append(client->remote_alias, alias);

                        ptr++;
                }
        }
        PM2_LOG_OUT();
        return ntbx_success;
}


/*...Disconnection......................*/

/* Disconnect a client socket */
void
ntbx_tcp_client_disconnect(p_ntbx_client_t client)
{
        p_ntbx_tcp_client_specific_t tcp_specific = client->specific;

        PM2_LOG_IN();
        SYSCALL(close(tcp_specific->descriptor));

        while (!tbx_slist_is_nil(client->local_alias)) {
                char *str = NULL;

                str = tbx_slist_extract(client->local_alias);
                TBX_FREE(str);
        }

        while (!tbx_slist_is_nil(client->remote_alias)) {
                char *str = NULL;

                str = tbx_slist_extract(client->remote_alias);
                TBX_FREE(str);
        }

        TBX_FREE(tcp_specific);
        client->specific = NULL;
        TBX_FREE(client->remote_host);
        client->remote_host = NULL;
        PM2_LOG_OUT();
}


/* Disconnect a server */
void
ntbx_tcp_server_disconnect(p_ntbx_server_t server)
{
        p_ntbx_tcp_server_specific_t tcp_specific = server->specific;

        PM2_LOG_IN();
        SYSCALL(close(tcp_specific->descriptor));

        while (!tbx_slist_is_nil(server->local_alias)) {
                char *str = NULL;

                str = tbx_slist_extract(server->local_alias);
                TBX_FREE(str);
        }

        TBX_FREE(tcp_specific);
        server->specific = NULL;
        PM2_LOG_OUT();
}


/*...Polling............................*/

/* data ready to read */
int
ntbx_tcp_read_poll(int              nb_clients,
		   p_ntbx_client_t *client_array)
{
        fd_set           read_fds;
        int              max_fds = 0;
        int              i;

        PM2_LOG_IN();

        FD_ZERO(&read_fds);
        for (i = 0; i < nb_clients; i++) {
                p_ntbx_tcp_client_specific_t tcp_specific = client_array[i]->specific;

                client_array[i]->read_ok = tbx_false;
                FD_SET(tcp_specific->descriptor, &read_fds);
                max_fds = tbx_max(max_fds, tcp_specific->descriptor);
        }

        while(tbx_true) {
                fd_set local_read_fds = read_fds;
                int    status         = 0;

#if(defined(PIOMAN) && !defined(PIOM_DISABLE_LTASKS))
		if(piom_test_activity())
		{
			status=piom_task_select(max_fds + 1, &local_read_fds, NULL);
		}
		else
#elif defined(MARCEL)
                if (marcel_test_activity())
                  {
                    status = marcel_select(max_fds + 1, &local_read_fds, NULL);
                  }
                else
#endif // MARCEL
                  {
                    status = select(max_fds + 1, &local_read_fds, NULL, NULL, NULL);
                  }

                if (status == -1) {
                        if (errno == EINTR) {
                                continue;
                        } else {
                                perror("select");
                                TBX_FAILURE("ntbx_tcp_read_poll failed");
                        }
                } else if (status == 0) {
                        return ntbx_failure;
                } else {
                        for (i = 0; i < nb_clients; i++) {
                                p_ntbx_client_t              client       = client_array[i];
                                p_ntbx_tcp_client_specific_t tcp_specific = client->specific;

                                if (FD_ISSET(tcp_specific->descriptor, &local_read_fds)) {
                                        client->read_ok = tbx_true;
                                }
                        }

                        return status;
                }
        }

        TBX_FAILURE("invalid path");
}


/* data ready to write */
int
ntbx_tcp_write_poll(int              nb_clients,
		    p_ntbx_client_t *client_array)
{
        fd_set           write_fds;
        int              max_fds = 0;
        int              i;

        PM2_LOG_IN();
        FD_ZERO(&write_fds);
        for (i = 0; i < nb_clients; i++) {
                p_ntbx_tcp_client_specific_t tcp_specific = client_array[i]->specific;

                client_array[i]->write_ok = tbx_false;
                FD_SET(tcp_specific->descriptor, &write_fds);
                max_fds = tbx_max(max_fds, tcp_specific->descriptor);
        }

        while(tbx_true) {
                fd_set local_write_fds = write_fds;
                int    status          = 0;

#if(defined(PIOMAN) && !defined(PIOM_DISABLE_LTASKS))
		if (piom_test_activity())
                  {
			  status = piom_task_select(max_fds + 1, NULL, &local_write_fds);
                  }
                else

#elif defined(MARCEL)
                if (marcel_test_activity())
                  {
			  status = marcel_select(max_fds + 1, NULL, &local_write_fds);
                  }
                else
#endif /* PIOMAN */
		{
			status = select(max_fds + 1, NULL, &local_write_fds, NULL, NULL);
		}

                if (status == -1) {
                        if (errno == EINTR) {
                                continue;
                        } else {
                                perror("select");
                                TBX_FAILURE("ntbx_tcp_write_poll failed");
                        }
                } else if (status == 0) {
                        continue;
                } else {
                        for (i = 0; i < nb_clients; i++) {
                                p_ntbx_client_t              client       =
                                        client_array[i];
                                p_ntbx_tcp_client_specific_t tcp_specific =
                                        client->specific;

                                if (FD_ISSET(tcp_specific->descriptor, &local_write_fds)) {
                                        client->write_ok = tbx_true;
                                }
                        }

                        PM2_LOG_OUT();
                        return status;
                }
        }

        TBX_FAILURE("invalid path");
}


/*...Read/Write...(block)...............*/

/* read a block */
int
ntbx_tcp_read_block(p_ntbx_client_t  client,
		    void            *ptr,
		    size_t           length) {
        p_ntbx_tcp_client_specific_t client_specific = client->specific;
        size_t                       bytes_read      = 0;

        PM2_LOG_IN();
        while (bytes_read < length) {
                int status;

#if(defined(PIOMAN) && !defined(PIOM_DISABLE_LTASKS))
		if (piom_test_activity())
		{
                    status = piom_task_read(client_specific->descriptor,
					    ptr + bytes_read,
					    length - bytes_read);
                  }
                else
#elif defined(MARCEL)
                if (marcel_test_activity())
                  {
                    status = marcel_read(client_specific->descriptor,
                                         ptr + bytes_read,
                                         length - bytes_read);
                  }
                else
#endif /* PIOMAN */
                  {
                    status = read(client_specific->descriptor,
                                  ptr + bytes_read,
                                  length - bytes_read);
                  }


                if (status == -1) {
                        if (errno == EINTR) {
                                continue;
                        } else {
                                perror("read");
                                TBX_FAILURE("ntbx_tcp_read_block");
                        }
                } else if (status == 0) {
                        return ntbx_failure;
                } else {
                        bytes_read += status;
                }
        }

        if (!client->read_rq_flag) {
                client->read_rq++;
        }
        PM2_LOG_OUT();

        return ntbx_success;
}


/* write a block */
int
ntbx_tcp_write_block(p_ntbx_client_t  client,
		     const void      *ptr,
		     const size_t     length) {
        p_ntbx_tcp_client_specific_t client_specific = client->specific;
        size_t                       bytes_written   = 0;

        PM2_LOG_IN();
        while (bytes_written < length) {
                int status;

#if(defined(PIOMAN) && !defined(PIOM_DISABLE_LTASKS))
                if (piom_test_activity())
                  {
		    status = piom_task_write(client_specific->descriptor,
					     ptr + bytes_written,
					     length - bytes_written);
                  }
                else

#elif defined(MARCEL)
                if (marcel_test_activity())
                  {
                    status = marcel_write(client_specific->descriptor,
                                          ptr + bytes_written,
                                          length - bytes_written);
                  }
                else
#endif /* PIOMAN */
                  {
                    status = write(client_specific->descriptor,
                                   ptr + bytes_written,
                                   length - bytes_written);
                  }

                if (status == -1) {
                        if (errno == EINTR) {
                                continue;
                        } else {
                                perror("write");
                                TBX_FAILURE("ntbx_tcp_write_block");
                        }
                } else if (status == 0) {
                        return ntbx_failure;
                } else {
                        bytes_written += status;
                }
        }

        if (!client->write_rq_flag) {
                client->write_rq++;
        }
        PM2_LOG_OUT();

        return ntbx_success;
}

/*...Read/Write...(pack.buffer).........*/

/* read a pack buffer */
int
ntbx_tcp_read_pack_buffer(p_ntbx_client_t      client,
			  p_ntbx_pack_buffer_t pack_buffer)
{
        int	   status		= ntbx_failure;
        tbx_bool_t rq_flag_toggled	= tbx_false;

        PM2_LOG_IN();
        if (!client->read_rq_flag) {
                client->read_rq_flag = tbx_true;
                rq_flag_toggled      = tbx_true;
        }

        status = ntbx_tcp_read_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
        if (rq_flag_toggled) {
                client->read_rq++;
                client->read_rq_flag = tbx_false;
        }
        PM2_LOG_OUT();

        return status;
}


/* write a pack buffer */
int
ntbx_tcp_write_pack_buffer(p_ntbx_client_t      client,
			   p_ntbx_pack_buffer_t pack_buffer)
{
        int status = ntbx_failure;
        tbx_bool_t rq_flag_toggled	= tbx_false;

        PM2_LOG_IN();
        if (!client->write_rq_flag) {
                client->write_rq_flag = tbx_true;
                rq_flag_toggled      = tbx_true;
        }

        status = ntbx_tcp_write_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
        if (rq_flag_toggled) {
                client->write_rq++;
                client->write_rq_flag = tbx_false;
        }
        PM2_LOG_OUT();
        return status;
}


/*...Read/Write...(string).........*/

/* read a string buffer */
int
ntbx_tcp_read_string(p_ntbx_client_t   client,
                     char            **string)
{
        int                status          = ntbx_failure;
        int                len             =           -1;
        tbx_bool_t         rq_flag_toggled = tbx_false;
        ntbx_pack_buffer_t pack_buffer;


        PM2_LOG_IN();
        if (!client->read_rq_flag) {
                client->read_rq_flag = tbx_true;
                rq_flag_toggled      = tbx_true;
        }

        memset(&pack_buffer, 0, sizeof(ntbx_pack_buffer_t));
        *string = NULL;

        status = ntbx_tcp_read_pack_buffer(client, &pack_buffer);
        if (status == ntbx_failure) {
                return ntbx_failure;
        }

        len = ntbx_unpack_int(&pack_buffer);
        if (len < 0)
                TBX_FAILURE("synchronization error");

        *string = TBX_MALLOC((size_t)len);
        CTRL_ALLOC(*string);

        status = ntbx_tcp_read_block(client, *string, len);

        if (rq_flag_toggled) {
                client->read_rq++;
                client->read_rq_flag = tbx_false;
        }
        PM2_LOG_OUT();
        return status;
}


/* write a string buffer */
int
ntbx_tcp_write_string(p_ntbx_client_t  client,
		       const char      *string)
{
        int                status          = ntbx_failure;
        int                len             =           -1;
        tbx_bool_t         rq_flag_toggled = tbx_false;
        ntbx_pack_buffer_t pack_buffer;

        PM2_LOG_IN();
        if (!client->write_rq_flag) {
                client->write_rq_flag = tbx_true;
                rq_flag_toggled      = tbx_true;
        }

        memset(&pack_buffer, 0, sizeof(ntbx_pack_buffer_t));
        len = strlen(string) + 1;
        ntbx_pack_int(len, &pack_buffer);
        status = ntbx_tcp_write_pack_buffer(client, &pack_buffer);
        if (status == ntbx_failure) {
                return ntbx_failure;
        }

        status = ntbx_tcp_write_block(client, string, len);
        if (rq_flag_toggled) {
                client->write_rq++;
                client->write_rq_flag = tbx_false;
        }
        PM2_LOG_OUT();
        return status;
}

