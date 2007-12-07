/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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

#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/uio.h>
#include <sys/poll.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/utsname.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <tbx.h>

#include "nm_public.h"
#include "nm_pkt_wrap.h"
#include "nm_drv.h"
#include "nm_trk.h"
#include "nm_gate.h"
#include "nm_core.h"
#include "nm_trk_rq.h"
#include "nm_cnx_rq.h"
#include "nm_errno.h"
#include "nm_log.h"

/** TCP/datagram specific driver data.
 */
struct nm_tcpdg_drv {
        /** server socket	*/
        int	server_fd;
};

/** TCP/datagram specific track data.
 */
struct nm_tcpdg_trk {

        /** Number of pending poll array entries to process before
            calling poll again. */
        int nb_incoming;

        /** Next pending poll array entry to process.
         */
        uint8_t next_entry;

        /** Array for polling multiple descriptors.
         */
        struct pollfd	*poll_array;

        /** Poll array index to gate id reverse mapping. Allows to
         *  find the source of a packet when performing gateless
         *  requests.
         */
        uint8_t		*gate_map;
};

/** TCP/datagram specific gate data.
 */
struct nm_tcpdg_gate {
        /** Array of sockets, one socket per track.
         */
        int	fd[255];

        /** Reference counter.
         */
        int	ref_count;
};

/** TCP/datagram specific pkt wrapper data.
 */
struct nm_tcpdg_pkt_wrap {
        /** Reception state-machine state.
           0: reading the message length
           1: reading the message body
         */
        uint8_t			state;

        /** Buffer ptr.
           Current location in sending the header.
         */
        uint8_t			*ptr;

        /** Remaining length.
           Current remaining length in sending either the header
           or the body (according to the state).
         */
        uint64_t		 rem_length;

        /** Actual packet length to receive.
            May be different from the length that is expected.
         */
        uint64_t		 pkt_length;

        /** Message header storage.
         */
        struct {

                uint64_t	length;
        } h;

        /* Message body iovec iterator.
         */
        struct nm_iovec_iter	vi;
};

/** Provide status messages for herrno errors.
    @return Status message, or NULL if error is unknown.
 */
static
char *
nm_tcpdg_h_errno_to_str(void) {
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

/** Encapsulate the creation of bound server socket.
 *  @param address the address to bind the socket to.
 *  @param port the TCP port to bind the socket to.
 *  @return The bound socket.
 */
static
int
nm_tcpdg_socket_create(struct sockaddr_in	*address,
                     unsigned short 	 port) {
        socklen_t          len  = sizeof(struct sockaddr_in);
        struct sockaddr_in temp;
        int                desc;

        SYSCALL(desc = socket(AF_INET, SOCK_STREAM, 0));

        temp.sin_family      = AF_INET;
        temp.sin_addr.s_addr = htonl(INADDR_ANY);
        temp.sin_port        = htons(port);

        SYSCALL(bind(desc, (struct sockaddr *)&temp, len));

        if (address) {
                SYSCALL(getsockname(desc, (struct sockaddr *)address, &len));
        }

        return desc;
}

/** Encapsulate the filling of an address struct.
 *  @note The function uses gethostbyname for resolving the hostname.
 *  @param address the address struct to fill.
 *  @param port the TCP port.
 *  @param host_name the machine hostname.
 */
static
void
nm_tcpdg_address_fill(struct sockaddr_in	*address,
                    unsigned short	 port,
                    char                  *host_name) {
        struct hostent *host_entry;

        if (!(host_entry = gethostbyname(host_name))) {
                char *msg = NULL;

                msg	= nm_tcpdg_h_errno_to_str();
                NM_DISPF("gethostbyname error: %s", msg);
        }

        address->sin_family	= AF_INET;
        address->sin_port	= htons(port);
        memcpy(&address->sin_addr.s_addr,
               host_entry->h_addr,
               (size_t)host_entry->h_length);

        memset(address->sin_zero, 0, 8);
}

/** Query the TCP driver with datagram emulation.
 *  @param p_drv the driver.
 *  @return The NM status code.
 */
static
int
nm_tcpdg_query		(struct nm_drv *p_drv,
			 struct nm_driver_query_param *params TBX_UNUSED,
			 int nparam TBX_UNUSED) {
	struct nm_tcpdg_drv	*p_tcp_drv	= NULL;
	int			 err;

	/* private data							*/
	p_tcp_drv	= TBX_MALLOC(sizeof (struct nm_tcpdg_drv));
	if (!p_tcp_drv) {
		err = -NM_ENOMEM;
		goto out;
	}

	memset(p_tcp_drv, 0, sizeof (struct nm_tcpdg_drv));
	p_drv->priv = p_tcp_drv;

	/* driver capabilities encoding					*/
	p_drv->cap.has_trk_rq_stream			= 1;
	p_drv->cap.has_selective_receive		= 1;
	p_drv->cap.has_concurrent_selective_receive	= 1;
#ifdef PM2_NUIOA
	p_drv->cap.numa_node = PM2_NUIOA_ANY_NODE;
#endif

	err = NM_ESUCCESS;

 out:
	return err;
}

/** Initialize the TCP driver with datagram emulation.
 *  @param p_drv the driver.
 *  @return The NM status code.
 */
static
int
nm_tcpdg_init		(struct nm_drv *p_drv) {
	struct nm_tcpdg_drv	*p_tcp_drv	= p_drv->priv;
        uint16_t		 port;
        struct sockaddr_in       address;
        p_tbx_string_t		 url_string	= NULL;
	struct utsname utsname;
        int			 err;

        /* server socket						*/
        p_tcp_drv->server_fd	= nm_tcpdg_socket_create(&address, 0);
        SYSCALL(listen(p_tcp_drv->server_fd, tbx_min(5, SOMAXCONN)));

	/* retrieve the system hostname and use it as a default hostname,
	 * leonie or the user will replace it if needed
	 */
	uname(&utsname);
#ifndef CONFIG_PROTO_MAD3
	WARN("Using system uts nodename \"%s\" for local url, might need to be superseded by a network-valid name",
	     utsname.nodename);
#endif

        /* driver url encoding						*/
        port		= ntohs(address.sin_port);
	url_string	= tbx_string_init_to_cstring(utsname.nodename);
	tbx_string_append_char(url_string, ':');
	tbx_string_append_uint(url_string, port);
        p_drv->url	= tbx_string_to_cstring(url_string);
        tbx_string_free(url_string);
	p_drv->name = tbx_strdup("tcp");

        err = NM_ESUCCESS;

        return err;
}

/** Cleanup the TCP driver with datagram emulation.
 *  @param p_drv the driver.
 *  @return The NM status code.
 */
static
int
nm_tcpdg_exit		(struct nm_drv *p_drv) {
	struct nm_tcpdg_drv	*p_tcp_drv	= NULL;
        int			 err;

	p_tcp_drv	= p_drv->priv;
        TBX_FREE(p_drv->url);
        SYSCALL(close(p_tcp_drv->server_fd));
        TBX_FREE(p_tcp_drv);

        err = NM_ESUCCESS;

        return err;
}

/** Open a new track.
 *  @param p_trk_rq the request details for opening the track.
 *  @return The NM status code.
 */
static
int
nm_tcpdg_open_trk		(struct nm_trk_rq	*p_trk_rq) {
        struct nm_trk		*p_trk		= NULL;
        struct nm_tcpdg_trk	*p_tcp_trk	= NULL;
        int			 err;

        p_trk	= p_trk_rq->p_trk;

        /* private data							*/
	p_tcp_trk	= TBX_MALLOC(sizeof (struct nm_tcpdg_trk));
        memset(p_tcp_trk, 0, sizeof (struct nm_tcpdg_trk));
        p_trk->priv	= p_tcp_trk;

        /* track capabilities encoding					*/
        p_trk->cap.rq_type	= nm_trk_rq_stream;
        p_trk->cap.iov_type	= nm_trk_iov_both_assym;
        p_trk->cap.max_pending_send_request	= 1;
        p_trk->cap.max_pending_recv_request	= 1;
        p_trk->cap.max_single_request_length	= SSIZE_MAX;
        p_trk->cap.max_iovec_request_length	= 0;
#ifdef IOV_MAX
        p_trk->cap.max_iovec_size		= IOV_MAX;
#else /* IOV_MAX */
        p_trk->cap.max_iovec_size		= sysconf(_SC_IOV_MAX);
#endif /* IOV_MAX */
        err = NM_ESUCCESS;

        return err;
}

/** Close a track.
 *  @param p_trk the track.
 *  @return The NM status code.
 */
static
int
nm_tcpdg_close_trk	(struct nm_trk	*p_trk) {
        struct nm_tcpdg_trk	*p_tcp_trk	= NULL;
        int			 err;

        p_tcp_trk	= p_trk->priv;
        TBX_FREE(p_tcp_trk->poll_array);
        TBX_FREE(p_tcp_trk->gate_map);
        TBX_FREE(p_tcp_trk);

        err = NM_ESUCCESS;

        return err;
}

/** Implement the code common to connect and accept operations.
 *  @param p_crq the connection request.
 *  @param fd the socket of the connection.
 *  @return The NM status code.
 */
static
int
nm_tcpdg_connect_accept	(struct nm_cnx_rq	*p_crq,
                         int			 fd) {
        struct nm_tcpdg_gate	*p_tcp_gate	= NULL;
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
        struct nm_tcpdg_trk	*p_tcp_trk	= NULL;
        int			 val    	=    1;
        socklen_t		 len    	= sizeof(int);
        int			 n;
        int			 err;

        p_gate		= p_crq->p_gate;
        p_drv		= p_crq->p_drv;
        p_trk		= p_crq->p_trk;
        p_tcp_trk	= p_trk->priv;

        SYSCALL(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&val, len));
	SYSCALL(fcntl(fd, F_SETFL, O_NONBLOCK));

        NM_TRACE_VAL("tcp connect/accept trk id", p_trk->id);
        NM_TRACE_VAL("tcp connect/accept gate id", p_gate->id);
        NM_TRACE_VAL("tcp connect/accept drv id", p_drv->id);
        NM_TRACE_VAL("tcp connect/accept new socket on fd", fd);

        /* private data				*/
        p_tcp_gate	= p_gate->p_gate_drv_array[p_drv->id]->info;
        if (!p_tcp_gate) {
                p_tcp_gate	= TBX_MALLOC(sizeof (struct nm_tcpdg_gate));
                memset(p_tcp_gate, 0, sizeof(struct nm_tcpdg_gate));

                /* update gate private data		*/
                p_gate->p_gate_drv_array[p_drv->id]->info	= p_tcp_gate;
                NM_TRACEF("drv_id %d, p_gate %p", p_drv->id, p_gate->p_gate_drv_array[p_drv->id]->info );
                p_tcp_gate->ref_count	= 1;
        } else {
                p_tcp_gate->ref_count++;
        }

        p_tcp_gate->fd[p_trk->id]	= fd;
        NM_TRACE_PTR("tcp connect/accept p_tcp_gate", p_tcp_gate);

        /* update trk private data		*/
        n = p_drv->nb_gates;

        if (n) {
                p_tcp_trk->poll_array	=
                        TBX_REALLOC(p_tcp_trk->poll_array,
                                    (n+1) * sizeof (struct pollfd));

                p_tcp_trk->gate_map	=
                        TBX_REALLOC(p_tcp_trk->gate_map, n+1);
        } else {
                p_tcp_trk->poll_array	= TBX_MALLOC(sizeof (struct pollfd));
                p_tcp_trk->gate_map	= TBX_MALLOC(1);
        }

        p_tcp_trk->poll_array[n].fd		= p_tcp_gate->fd[p_trk->id];
        p_tcp_trk->poll_array[n].events	= POLLIN;
        p_tcp_trk->poll_array[n].revents	= 0;
        p_tcp_trk->gate_map[n]		= p_gate->id;

        NMAD_EVENT_NEW_TRK(p_gate->id, p_drv->id, p_trk->id);
        err = NM_ESUCCESS;

        return err;
}

/** Open an outgoing connection.
 *  @param p_crq the connection request.
 *  @return The NM status code.
 */
static
int
nm_tcpdg_connect		(struct nm_cnx_rq *p_crq) {
        uint16_t		 port;
        struct sockaddr_in	 address;
        int			 fd;
        int			 err;
        char 			*saveptr = NULL;
	char *remote_hostname, *remote_port;
	/* save the url since strtok might change it */
	char *remote_drv_url = tbx_strdup(p_crq->remote_drv_url);

        /* TCP connect 				*/
	remote_hostname = strtok_r(remote_drv_url, ":", &saveptr);
	if (!*saveptr) {
		/* reached the end of the string => was no ':' */
		WARN("Missing colon in url \"%s\", prefix \"<hostname>:\" required",
		     remote_drv_url);
		err = -NM_EINVAL;
		goto out;
	}

	remote_port = strtok_r(NULL, "#", &saveptr);
        port	= strtol(remote_port, (char **)NULL, 10);
        fd	= nm_tcpdg_socket_create(NULL, 0);

        nm_tcpdg_address_fill(&address, port, remote_hostname);

        SYSCALL(connect(fd, (struct sockaddr *)&address,
                        sizeof(struct sockaddr_in)));

        err = nm_tcpdg_connect_accept(p_crq, fd);

 out:
	TBX_FREE(remote_drv_url);
        return err;
}

/** Open an incoming connection.
 *  @param p_crq the connection request.
 *  @return The NM status code.
 */
static
int
nm_tcpdg_accept		(struct nm_cnx_rq *p_crq) {
        struct nm_tcpdg_drv	*p_tcp_drv	= NULL;
        struct nm_drv		*p_drv		= NULL;
        int			 fd;
        int			 err;

        p_drv		= p_crq->p_drv;
        p_tcp_drv	= p_drv->priv;

        /* TCP accept 				*/
        SYSCALL(fd = accept(p_tcp_drv->server_fd, NULL, NULL));

        err = nm_tcpdg_connect_accept(p_crq, fd);

        return err;
}

/** Closes a connection.
 *  @warning The function does nothing for now.
 *  @param p_crq the connection request.
 *  @return The NM status code.
 */
static
int
nm_tcpdg_disconnect	(struct nm_cnx_rq *p_crq) {
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_tcpdg_gate	*p_tcp_gate	= NULL;
        int	err;

        p_gate		= p_crq->p_gate;
        p_drv		= p_crq->p_drv;
        p_tcp_gate	= p_gate->p_gate_drv_array[p_drv->id]->info;
        p_tcp_gate->ref_count--;
        if (!p_tcp_gate->ref_count) {
                TBX_FREE(p_tcp_gate);
                p_gate->p_gate_drv_array[p_drv->id]->info	= 0;
        }

        err = NM_ESUCCESS;

        return err;
}

/** Check if a pkt wrapper send request may progress.
 *  @param p_pw the pkt wrapper.
 *  @return The NM status code.
 */
static
int
nm_tcpdg_outgoing_poll	(struct nm_pkt_wrap *p_pw) {
	struct pollfd		 pollfd;
        struct nm_tcpdg_gate	*p_tcp_gate	= NULL;
        int			 ret;
        int			 err;

        NM_LOG_IN();
        if (!p_pw->gate_priv) {
                p_pw->gate_priv	= p_pw->p_gate->p_gate_drv_array[p_pw->p_drv->id]->info;
                NM_TRACEF("nm_tcpdg_outgoing_poll: filling gate_priv - p_pw = %p, p_gate = %p, p_gate->id = %d, gate_priv = %p",
                          p_pw, p_pw->p_gate, p_pw->p_gate->id, p_pw->gate_priv);
        }

	p_tcp_gate	= p_pw->gate_priv;

        pollfd.fd	= p_tcp_gate->fd[p_pw->p_trk->id];
        pollfd.events	= POLLOUT;
        pollfd.revents	= 0;

 poll_again:
        ret	= poll(&pollfd, 1, 0);
        if (ret < 0) {

                /* redo interrupted poll			*/
                if (errno == EINTR)
                        goto poll_again;

                perror("poll");

                /* poll syscall failed				*/
                WARN("-NM_ESCFAILD");
                err = -NM_ESCFAILD;
                goto out;
        }

        /* fd not ready						*/
        if (ret	== 0) {
                err = -NM_EAGAIN;
                goto out;
        }

        /* fd ready, check condition				*/
        if (pollfd.revents != POLLOUT) {
                if (pollfd.revents & POLLHUP) {
                        WARN("-NM_ECLOSED");
                        err = -NM_ECLOSED;
                } else if (pollfd.revents & POLLNVAL) {
                        WARN("-NM_EINVAL");
                        err = -NM_EINVAL;
                } else {
                        WARN("-NM_EBROKEN");
                        err = -NM_EBROKEN;
                }

                goto out;
        }

        err	= NM_ESUCCESS;

 out:
        NM_LOG_OUT();

        return err;
}

/** Check if a pkt wrapper receive request may progress.
 *  @param p_pw the pkt wrapper.
 *  @return The NM status code.
 */
static
int
nm_tcpdg_incoming_poll	(struct nm_pkt_wrap *p_pw) {
	struct pollfd		 pollfd;
        struct nm_drv		*p_drv		= NULL;
        struct nm_gate		*p_gate		= NULL;
        struct nm_tcpdg_gate	*p_tcp_gate	= NULL;
        struct nm_tcpdg_trk	*p_tcp_trk	= NULL;
        struct nm_core		*p_core		= NULL;
        struct pollfd		*p_gate_pollfd	= NULL;

        int			 ret;
        int			 err;

        NM_LOG_IN();
        p_tcp_trk	= p_pw->p_trk->priv;
        p_drv		= p_pw->p_drv;
        p_core		= p_drv->p_core;

        if (!p_pw->p_gate) {
                int i;
                /* gate not selected, permissive recv 			*/
                /* unimplemented */

                /* 2 cases
                   - pending multi-poll
                   - no pending multi-poll
                */

                if (!p_tcp_trk->nb_incoming) {
                        /* no pending multipoll				*/
                        p_tcp_trk->next_entry	= 0;

                        NM_TRACE_VAL("tcp incoming multi poll: nb gates", p_drv->nb_gates);
                        NM_TRACE_VAL("tcp incoming multi poll: gate 0 fd", p_tcp_trk->poll_array[0].fd);

                multi_poll_again:
                        ret	= poll(p_tcp_trk->poll_array, p_drv->nb_gates, 0);
                        if (ret < 0) {
                                if (errno == EINTR)
                                        goto multi_poll_again;

                                perror("poll");

                                WARN("-NM_ESCFAILD");
                                err = -NM_ESCFAILD;
                                goto out;
                        }

                        if (ret	== 0) {
                                NM_TRACEF("tcp incoming multi poll: no hit");
                                err = -NM_EAGAIN;
                                goto out;
                        }

                        NM_TRACE_VAL("tcp incoming multi poll ret", ret);

                        p_tcp_trk->nb_incoming	= ret;
                }


                /* pending multipoll				*/

                i = p_tcp_trk->next_entry;
                while (i < p_drv->nb_gates) {
                        if (p_tcp_trk->poll_array[i].revents) {
                                goto active_gate_found;
                        }

                        i++;
                }

                WARN("-NM_EINVAL");
                err = -NM_EINVAL;
                goto out;

        active_gate_found:
                p_tcp_trk->next_entry	= i;
                p_pw->p_gate		=
                        p_core->gate_array + p_tcp_trk->gate_map[i];
                NM_TRACE_VAL("gateless request: active gate id ", p_pw->p_gate->id);
        }

        /* gate already selected or selective recv 		*/

        p_gate		= p_pw->p_gate;

        if (!p_pw->gate_priv) {
                p_pw->gate_priv		=
                        p_pw->p_gate->p_gate_drv_array[p_drv->id]->info;
        }

        p_tcp_gate	= p_pw->gate_priv;
	
	if (p_tcp_trk->nb_incoming) {
		/* check former multipoll result 			*/
		p_gate_pollfd	= p_tcp_trk->poll_array + p_gate->id;
		
		if (p_gate_pollfd->revents) {
			p_tcp_trk->nb_incoming--;
			
			if (p_tcp_trk->next_entry == p_gate->id) {
				p_tcp_trk->next_entry++;
			}
			
			if (p_gate_pollfd->revents == POLLIN) {
				p_gate_pollfd->revents = 0;
				goto poll_not_needed;
			} else {
				 NM_TRACE_VAL("tcp incoming single poll: event on fd", p_gate_pollfd->fd);
				if (p_gate_pollfd->revents & POLLHUP) {
					NM_TRACEF("tcp incoming single poll: pollhup");
					err = -NM_ECLOSED;
				} else if (p_gate_pollfd->revents & POLLNVAL) {
					NM_TRACEF("tcp incoming single poll: pollnval");
                                        WARN("-NM_EINVAL");
					err = -NM_EINVAL;
				} else {
					NM_TRACEF("tcp incoming single poll: connection broken");
					err = -NM_EBROKEN;
				}
				
				p_gate_pollfd->revents = 0;
				goto out;
			}
		}
	}
	
        /* poll needed			 			*/
        pollfd.fd	= p_tcp_gate->fd[p_pw->p_trk->id];
        pollfd.events	= POLLIN;
        pollfd.revents	= 0;

        NM_TRACE_VAL("tcp incoming single poll: pollfd", pollfd.fd);

 poll_single_again:
        ret	= poll(&pollfd, 1, 0);
        if (ret < 0) {

                /* redo interrupted poll			*/
                if (errno == EINTR)
                        goto poll_single_again;

                perror("poll");

                /* poll syscall failed				*/
                err = -NM_ESCFAILD;
                goto out;
        }

        /* fd not ready						*/
        if (ret	== 0) {
                NM_TRACEF("tcp incoming single poll: no hit");
                err = -NM_EAGAIN;
                goto out;
        }

        NM_TRACE_VAL("tcp incoming single poll ret", ret);

        /* fd ready, check condition				*/
        if (pollfd.revents != POLLIN) {
                if (pollfd.revents & POLLHUP) {
                        NM_TRACEF("tcp incoming single poll: pollhup");
                        WARN("-NM_ECLOSED");
                        err = -NM_ECLOSED;
                } else if (pollfd.revents & POLLNVAL) {
                        NM_TRACEF("tcp incoming single poll: pollnval");
                        WARN("-NM_EINVAL");
                        err = -NM_EINVAL;
                } else {
                        WARN("-NM_EBROKEN");
                        err = -NM_EBROKEN;
                }

                goto out;
        }

 poll_not_needed:
        err	= NM_ESUCCESS;

 out:
        NM_LOG_OUT();

        return err;
}

/** Post a new send request or try to make a pending request progress.
 */
static
int
nm_tcpdg_send_iov	(struct nm_pkt_wrap *p_pw) {
        struct nm_tcpdg_gate		*p_tcp_gate	= NULL;
        struct nm_tcpdg_pkt_wrap	*p_tcp_pw	= NULL;
        int				 fd		=   -1;
        int				 ret;
        int				 err;

        NM_LOG_IN();
        err	= nm_tcpdg_outgoing_poll(p_pw);
        if (err < 0) {
                if (err == -NM_EAGAIN) {
                        goto out;
                } else {
                        goto out_complete;
                }
        }

        NM_TRACEF("nm_tcpdg_send_iov: pw details --- gate_priv - p_pw = %p, p_gate = %p, p_gate->id = %d, gate_priv = %p",
                  p_pw, p_pw->p_gate, p_pw->p_gate->id, p_pw->gate_priv);
	p_tcp_gate	= p_pw->gate_priv;
        NM_TRACE_PTR("p_tcp_gate", p_tcp_gate);
        p_tcp_pw	= p_pw->drv_priv;
        if (!p_tcp_pw) {
                NM_TRACEF("setup vector iterator");

                p_tcp_pw	= TBX_MALLOC(sizeof(struct nm_tcpdg_pkt_wrap));
                p_pw->drv_priv	= p_tcp_pw;

                /* current entry num is first entry num
                 */
                p_tcp_pw->vi.v_cur	= p_pw->v_first;

                /* save a copy of current entry
                 */
                p_tcp_pw->vi.cur_copy	= p_pw->v[p_pw->v_first];

                /* current vector size is full vector size
                 */
                p_tcp_pw->vi.v_cur_size	= p_pw->v_nb;

                /* message header
                 */
                p_tcp_pw->h.length	= p_pw->length;

                /* initial state
                 */
                p_tcp_pw->state		= 0;

                /* header ptr
                 */
                p_tcp_pw->ptr		= (uint8_t *)&(p_tcp_pw->h);

                /* remaining length
                 */
                p_tcp_pw->rem_length	= sizeof(p_tcp_pw->h);

                NMAD_EVENT_SND_START(p_pw->p_gate->id, p_pw->p_drv->id, p_pw->p_trk->id, p_pw->length);
        }

        fd	= p_tcp_gate->fd[p_pw->p_trk->id];

        if (p_tcp_pw->state) {
                struct nm_iovec_iter	*p_vi;
                struct iovec		*p_cur;

        write_body:
                NM_TRACEF("tcp outgoing: sending body");
                /* get a pointer to the iterator
                 */
                p_vi	= &(p_tcp_pw->vi);

                /* get a pointer to the current entry
                 */
                p_cur	= p_pw->v + p_tcp_pw->vi.v_cur;

                NM_TRACE_VAL("tcp outgoing trk id", p_pw->p_trk->id);
                NM_TRACE_VAL("tcp outgoing gate id", p_pw->p_gate->id);
                NM_TRACE_VAL("tcp outgoing fd", p_tcp_gate->fd[p_pw->p_trk->id]);
                NM_TRACE_VAL("tcp outgoing cur size", p_vi->v_cur_size);
                NM_TRACE_PTR("tcp outgoing cur base", p_cur->iov_base);
                NM_TRACE_VAL("tcp outgoing cur len", p_cur->iov_len);

                if (!p_vi->v_cur_size) {
                        err	= NM_ESUCCESS;

                        goto out_complete;
                }

        writev_again:
                ret	= writev(fd, p_cur, p_vi->v_cur_size);
                NM_TRACE_VAL("tcp outgoing writev status", ret);

                if (ret < 0) {
                        if (errno == EINTR)
                                goto writev_again;

                        perror("writev");
                        err = -NM_ESCFAILD;
                        goto out_complete;
                }

                if (ret	== 0) {
                        NM_TRACEF("connection closed");
                        err = -NM_ECLOSED;
                        goto out_complete;
                }

                do {
                        uint64_t len	= tbx_min(p_cur->iov_len, (uint64_t)ret);
                        p_cur->iov_base	+= len;
                        p_cur->iov_len	-= len;

                        /* still something to send for this vector entry?
                         */
                        if (p_cur->iov_len)
                                break;

                        /* restore vector entry
                         */
                        *p_cur = p_vi->cur_copy;

                        /* increment cursors
                         */
                        p_vi->v_cur++;
                        p_vi->v_cur_size--;

                        if (!p_vi->v_cur_size) {
                                err	= NM_ESUCCESS;

                                goto out_complete;
                        }

                        p_cur++;
                        p_vi->cur_copy = *p_cur;
                        ret -= len;
                } while (ret);
        } else {
                NM_TRACEF("tcp outgoing: sending header");
                NM_TRACE_VAL("tcp header outgoing gate id", p_pw->p_gate->id);
        write_again:
                ret	= write(fd, p_tcp_pw->ptr, p_tcp_pw->rem_length);
                NM_TRACE_VAL("tcp outgoing write status", ret);

                if (ret < 0) {
                        if (errno == EINTR)
                                goto write_again;

                        perror("write");
                        err = -NM_ESCFAILD;
                        goto out_complete;
                }

                if (ret	== 0) {
                        NM_TRACEF("connection closed");
                        err = -NM_ECLOSED;
                        goto out_complete;
                }

                p_tcp_pw->ptr		+= ret;
                p_tcp_pw->rem_length	-= ret;

                if (!p_tcp_pw->rem_length) {
                        p_tcp_pw->state	= 1;
                        p_tcp_pw->rem_length	= p_tcp_pw->h.length;

                        err	= nm_tcpdg_outgoing_poll(p_pw);
                        if (err < 0) {
                                if (err == -NM_EAGAIN) {
                                        goto out;
                                } else {
                                        goto out_complete;
                                }
                        }

                        goto write_body;
                }
        }

        err	= -NM_EAGAIN;

 out:
        NM_LOG_OUT();

        return err;

 out_complete:
        if (p_pw->drv_priv) {
                TBX_FREE(p_pw->drv_priv);
                p_pw->drv_priv	= NULL;
        }

        if (p_pw->gate_priv) {
                p_pw->gate_priv = NULL;
        }

        goto out;
}

/** Post a new receive request or try to make a pending request progress.
 */
static
int
nm_tcpdg_recv_iov	(struct nm_pkt_wrap *p_pw) {
        struct nm_tcpdg_gate		*p_tcp_gate	= NULL;
        struct nm_tcpdg_pkt_wrap	*p_tcp_pw	= NULL;
        int				 fd;
        int			 	 ret;
        int			 	 err;

        NM_LOG_IN();
        err	= nm_tcpdg_incoming_poll(p_pw);
        if (err < 0) {
                if (err == -NM_EAGAIN) {
                        goto out;
                } else {
                        goto out_complete;
                }
        }

        p_tcp_gate	= p_pw->gate_priv;
        NM_TRACE_PTR("p_tcp_gate", p_tcp_gate);
        NM_TRACE_PTR("p_gate", p_pw->p_gate);
        NM_TRACEF("drv_id %d p_tcp_gate(2) %p", p_pw->p_drv->id, p_pw->p_gate->p_gate_drv_array[p_pw->p_drv->id]->info);

        p_tcp_pw	= p_pw->drv_priv;
        if (!p_tcp_pw) {
                NM_TRACEF("setup vector iterator");
                p_tcp_pw	= TBX_MALLOC(sizeof(struct nm_tcpdg_pkt_wrap));
                p_pw->drv_priv	= p_tcp_pw;

                /* current entry num is first entry num
                 */
                p_tcp_pw->vi.v_cur	= p_pw->v_first;

                /* save a copy of current entry
                 */
                p_tcp_pw->vi.cur_copy	= p_pw->v[p_pw->v_first];

                /* current vector size is full vector size
                 */
                p_tcp_pw->vi.v_cur_size	= p_pw->v_nb;

                /* message header
                 */
                p_tcp_pw->h.length	= 0;

                /* initial state
                 */
                p_tcp_pw->state		= 0;

                /* header ptr
                 */
                p_tcp_pw->ptr		= (uint8_t *)&(p_tcp_pw->h);

                /* remaining length
                 */
                p_tcp_pw->rem_length	= sizeof(p_tcp_pw->h);
        }

        fd	= p_tcp_gate->fd[p_pw->p_trk->id];
        NM_TRACE_VAL("tcp incoming - state value", p_tcp_pw->state);

        if (p_tcp_pw->state) {
                struct nm_iovec_iter	*p_vi;
                struct iovec		*p_cur;

        read_body:
                NM_TRACEF("tcp incoming: receiving body");

                /* get a pointer to the iterator
                 */
                p_vi	= &(p_tcp_pw->vi);

                /* get a pointer to the current entry
                 */
                p_cur	= p_pw->v + p_tcp_pw->vi.v_cur;

                NM_TRACE_VAL("tcp incoming trk id", p_pw->p_trk->id);
                NM_TRACE_VAL("tcp incoming gate id", p_pw->p_gate->id);
                NM_TRACE_VAL("tcp incoming fd", p_tcp_gate->fd[p_pw->p_trk->id]);
                NM_TRACE_VAL("tcp incoming cur size", p_vi->v_cur_size);
                NM_TRACE_PTR("tcp incoming cur base", p_cur->iov_base);
                NM_TRACE_VAL("tcp incoming cur len", p_cur->iov_len);

                if (!p_vi->v_cur_size) {
                        err	= NM_ESUCCESS;

                        goto out_complete;
                }

        read_body_again:
                /* We cannot use readv because if the incoming message is shorter
                   than expected, readv could end-up reading part of the next message
                   as well.
                 */

                ret	= read(fd, p_cur->iov_base,
                               tbx_min(p_cur->iov_len, p_tcp_pw->rem_length));
                NM_TRACE_VAL("tcp incoming read status", ret);

                if (ret < 0) {
                        if (errno == EINTR)
                                goto read_body_again;

                        perror("read");

                        err = -NM_ESCFAILD;
                        goto out_complete;
                }

                if (ret	== 0) {
                        NM_TRACEF("connection closed");
                        err = -NM_ECLOSED;
                        goto out_complete;
                }

                p_tcp_pw->rem_length	-= ret;
                NM_TRACE_VAL("tcp incoming rem length", p_tcp_pw->rem_length);

                if (!p_tcp_pw->rem_length) {
                        NMAD_EVENT_RCV_END(p_pw->p_gate->id, p_pw->p_drv->id, p_pw->p_trk->id, p_tcp_pw->pkt_length);
                        err	= NM_ESUCCESS;

                        *p_cur = p_vi->cur_copy;
                        goto out_complete;
                }

                p_cur->iov_base		+= ret;
                p_cur->iov_len		-= ret;
                NM_TRACE_VAL("tcp incoming iov_len", p_cur->iov_len);

                /* still something to receive for this vector entry?
                 */
                if (p_cur->iov_len) {
                        NM_TRACEF("tcp incoming iov: next time...");
                        err	= -NM_EAGAIN;
                        goto out;
                }

                NM_TRACEF("tcp incoming iov: next vector entry");

                /* restore vector entry
                 */
                *p_cur = p_vi->cur_copy;

                /* increment cursors
                 */
                p_vi->v_cur++;
                p_vi->v_cur_size--;

                if (!p_vi->v_cur_size) {
                        NM_TRACEF("tcp incoming iov: no more vector entry");

                        if (p_tcp_pw->rem_length) {
                                NM_TRACEF("tcp incoming iov: truncating message");
                                /* message truncated
                                 */
                                WARN_VAL("message truncated: remaining bytes", p_tcp_pw->rem_length);
                                WARN("-NM_EINVAL");
                                err	= -NM_EINVAL;
                        } else {
                                NMAD_EVENT_RCV_END(p_pw->p_gate->id, p_pw->p_drv->id, p_pw->p_trk->id, p_pw->length);
                                fprintf(stderr, "tcp incoming iov: receive complete\n");
                                NM_TRACEF("tcp incoming iov: receive complete");
                                err	= NM_ESUCCESS;
                        }

                        goto out_complete;
                }

                p_cur++;
                p_vi->cur_copy = *p_cur;

                NM_TRACEF("tcp incoming iov: polling for next vector entry");
                err	= nm_tcpdg_incoming_poll(p_pw);
                if (err < 0) {
                        if (err == -NM_EAGAIN) {
                                goto out;
                        } else {
                                goto out_complete;
                        }
                }

                NM_TRACEF("tcp incoming iov: looping on next vector entry");
                goto read_body_again;
        } else {
                NM_TRACEF("tcp incoming: receiving header");
                NM_TRACE_VAL("tcp header incoming gate id", p_pw->p_gate->id);
        read_header_again:
                ret	= read(fd, p_tcp_pw->ptr, p_tcp_pw->rem_length);
                NM_TRACE_VAL("tcp header incoming read status", ret);

                if (ret < 0) {
                        if (errno == EINTR)
                                goto read_header_again;

                        perror("read");
                        err = -NM_ESCFAILD;
                        goto out_complete;
                }

                if (ret	== 0) {
                        NM_TRACEF("connection closed");
                        err = -NM_ECLOSED;
                        goto out_complete;
                }

                p_tcp_pw->ptr		+= ret;
                p_tcp_pw->rem_length	-= ret;
                NM_TRACE_VAL("tcp header incoming - header length remaining", p_tcp_pw->rem_length);

                if (!p_tcp_pw->rem_length) {
                        NM_TRACEF("tcp header incoming - header receive complete, switching to state 1");
                        p_tcp_pw->state	= 1;
                        p_tcp_pw->rem_length	= p_tcp_pw->h.length;
                        p_tcp_pw->pkt_length	= p_tcp_pw->h.length;

                        NM_TRACE_VAL("tcp header incoming pkt length", p_tcp_pw->rem_length);
                        err	= nm_tcpdg_incoming_poll(p_pw);
                        if (err < 0) {
                                if (err == -NM_EAGAIN) {
                                        goto out;
                                } else {
                                        goto out_complete;
                                }
                        }

                        NM_TRACEF("tcp header incoming - going to read body");
                        goto read_body;
                }
        }

        err	= -NM_EAGAIN;

 out:
        NM_LOG_OUT();

        return err;

 out_complete:
        if (p_pw->drv_priv) {
                TBX_FREE(p_pw->drv_priv);
                p_pw->drv_priv	= NULL;
        }

        if (p_pw->gate_priv) {
                p_pw->gate_priv = NULL;
        }

        goto out;
}

/** Load the TCP driver with datagram emulation.
 *  @param p_ops the driver operation structure to fill.
 *  @return the NM status code.
 */
int
nm_tcpdg_load(struct nm_drv_ops *p_ops) {
        p_ops->query		= nm_tcpdg_query;
        p_ops->init		= nm_tcpdg_init;
        p_ops->exit             = nm_tcpdg_exit;
        p_ops->open_trk		= nm_tcpdg_open_trk;
        p_ops->close_trk	= nm_tcpdg_close_trk;
        p_ops->connect		= nm_tcpdg_connect;
        p_ops->accept		= nm_tcpdg_accept;
        p_ops->disconnect       = nm_tcpdg_disconnect;
        p_ops->post_send_iov	= nm_tcpdg_send_iov;
        p_ops->post_recv_iov    = nm_tcpdg_recv_iov;
        p_ops->poll_send_iov	= nm_tcpdg_send_iov;
        p_ops->poll_recv_iov	= nm_tcpdg_recv_iov;

        return NM_ESUCCESS;
}
