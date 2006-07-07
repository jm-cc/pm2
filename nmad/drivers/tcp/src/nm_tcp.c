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
#include <limits.h>
#include <sys/uio.h>
#include <sys/poll.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <assert.h>

#include <tbx.h>

#include "nm_tcp_private.h"

struct nm_tcp_drv {
        /* server socket	*/
        int	server_fd;
};

struct nm_tcp_trk {

        /* number of poll array entries to process until next poll	*/
        int nb_incoming;

        /* next poll array entry to process				*/
        uint8_t next_entry;

        /* array for polling multiple descriptors			*/
        struct pollfd	*poll_array;

        /* poll array index to gate id reverse mapping			*/
        uint8_t		*gate_map;
};

struct nm_tcp_gate {
        int	fd[255];
};

struct nm_tcp_pkt_wrap {
        struct nm_iovec_iter vi;
};

static
char *
nm_tcp_h_errno_to_str(void) {
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

static
int
nm_tcp_socket_create(struct sockaddr_in	*address,
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

static
void
nm_tcp_address_fill(struct sockaddr_in	*address,
                    unsigned short	 port,
                    char                  *host_name) {
        struct hostent *host_entry;

        if (!(host_entry = gethostbyname(host_name))) {
                char *msg = NULL;

                msg	= nm_tcp_h_errno_to_str();
                NM_DISPF("gethostbyname error: %s", msg);
        }

        address->sin_family	= AF_INET;
        address->sin_port	= htons(port);
        memcpy(&address->sin_addr.s_addr,
               host_entry->h_addr,
               (size_t)host_entry->h_length);

        memset(address->sin_zero, 0, 8);
}

static
int
nm_tcp_init		(struct nm_drv *p_drv) {
        struct nm_tcp_drv	*p_tcp_drv	= NULL;
        uint16_t		 port;
        struct sockaddr_in       address;
        p_tbx_string_t		 url_string	= NULL;
        int			 err;

        /* private data							*/
	p_tcp_drv	= TBX_MALLOC(sizeof (struct nm_tcp_drv));
        p_drv->priv	= p_tcp_drv;

        /* server socket						*/
        p_tcp_drv->server_fd	= nm_tcp_socket_create(&address, 0);
        SYSCALL(listen(p_tcp_drv->server_fd, tbx_min(5, SOMAXCONN)));

        /* driver url encoding						*/
        port		= ntohs(address.sin_port);
        url_string	= tbx_string_init_to_int(port);
        p_drv->url	= tbx_string_to_cstring(url_string);
        tbx_string_free(url_string);

        /* driver capabilities encoding					*/
        p_drv->cap.has_trk_rq_stream			= 1;
        p_drv->cap.has_selective_receive		= 1;
        p_drv->cap.has_concurrent_selective_receive	= 1;

        err = NM_ESUCCESS;

        return err;
}

static
int
nm_tcp_exit		(struct nm_drv *p_drv) {
	struct nm_tcp_drv	*p_tcp_drv	= NULL;
        int			 err;

	p_tcp_drv	= p_drv->priv;
        TBX_FREE(p_drv->url);
        SYSCALL(close(p_tcp_drv->server_fd));
        TBX_FREE(p_tcp_drv);

        err = NM_ESUCCESS;

        return err;
}

static
int
nm_tcp_open_trk		(struct nm_trk_rq	*p_trk_rq) {
        struct nm_trk		*p_trk		= NULL;
        struct nm_tcp_trk	*p_tcp_trk	= NULL;
        int			 err;

        p_trk	= p_trk_rq->p_trk;

        /* private data							*/
	p_tcp_trk	= TBX_MALLOC(sizeof (struct nm_tcp_trk));
        memset(p_tcp_trk, 0, sizeof (struct nm_tcp_trk));
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

static
int
nm_tcp_close_trk	(struct nm_trk	*p_trk) {
        struct nm_tcp_trk	*p_tcp_trk	= NULL;
        int			 err;

        p_tcp_trk	= p_trk->priv;
        TBX_FREE(p_tcp_trk->poll_array);
        TBX_FREE(p_tcp_trk->gate_map);
        TBX_FREE(p_tcp_trk);

        err = NM_ESUCCESS;

        return err;
}

static
int
nm_tcp_connect_accept	(struct nm_cnx_rq	*p_crq,
                         int			 fd) {
        struct nm_tcp_gate	*p_tcp_gate	= NULL;
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
        struct nm_tcp_trk	*p_tcp_trk	= NULL;
        int			 n;
        int			 err;

        p_gate		= p_crq->p_gate;
        p_drv		= p_crq->p_drv;
        p_trk		= p_crq->p_trk;
        p_tcp_trk	= p_trk->priv;

        NM_TRACE_VAL("tcp connect/accept trk id", p_trk->id);
        NM_TRACE_VAL("tcp connect/accept new socket on fd", fd);

        /* private data				*/
        p_tcp_gate	= p_gate->p_gate_drv_array[p_drv->id]->info;
        if (!p_tcp_gate) {
                p_tcp_gate	= TBX_MALLOC(sizeof (struct nm_tcp_gate));
                memset(p_tcp_gate, 0, sizeof(struct nm_tcp_gate));

                /* update gate private data		*/
                p_gate->p_gate_drv_array[p_drv->id]->info	= p_tcp_gate;
        }

        p_tcp_gate->fd[p_trk->id]	= fd;

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

        err = NM_ESUCCESS;

        return err;
}

static
int
nm_tcp_connect		(struct nm_cnx_rq *p_crq) {
        uint16_t		 port;
        struct sockaddr_in	 address;
        int			 fd;
        int			 err;

        /* TCP connect 				*/
        port	= strtol(p_crq->remote_drv_url, (char **)NULL, 10);
        fd	= nm_tcp_socket_create(NULL, 0);

        nm_tcp_address_fill(&address, port, p_crq->remote_host_url);

        SYSCALL(connect(fd, (struct sockaddr *)&address,
                        sizeof(struct sockaddr_in)));

        err = nm_tcp_connect_accept(p_crq, fd);

        return err;
}

static
int
nm_tcp_accept		(struct nm_cnx_rq *p_crq) {
        struct nm_tcp_drv	*p_tcp_drv	= NULL;
        struct nm_drv		*p_drv		= NULL;
        int			 fd;
        int			 err;

        p_drv		= p_crq->p_drv;
        p_tcp_drv	= p_drv->priv;

        /* TCP accept 				*/
        SYSCALL(fd = accept(p_tcp_drv->server_fd, NULL, NULL));

        err = nm_tcp_connect_accept(p_crq, fd);

        return err;
}

static
int
nm_tcp_disconnect	(struct nm_cnx_rq *p_crq) {
        int	err;

        err = NM_ESUCCESS;

        return err;
}


static
int
nm_tcp_outgoing_poll	(struct nm_pkt_wrap *p_pw) {
	struct pollfd		 pollfd;
        struct nm_tcp_gate	*p_tcp_gate	= NULL;
        int			 ret;
        int			 err;

        if (!p_pw->gate_priv) {
                p_pw->gate_priv	= p_pw->p_gate->p_gate_drv_array[p_pw->p_drv->id]->info;
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
                        err = -NM_ECLOSED;
                } else if (pollfd.revents & POLLNVAL) {
                        err = -NM_EINVAL;
                } else {
                        err = -NM_EBROKEN;
                }

                goto out;
        }

        err	= NM_ESUCCESS;

 out:
        return err;
}

static
int
nm_tcp_incoming_poll	(struct nm_pkt_wrap *p_pw) {
	struct pollfd		 pollfd;
        struct nm_drv		*p_drv		= NULL;
        struct nm_gate		*p_gate		= NULL;
        struct nm_tcp_gate	*p_tcp_gate	= NULL;
        struct nm_tcp_trk	*p_tcp_trk	= NULL;
        struct nm_core		*p_core		= NULL;
        struct pollfd		*p_gate_pollfd	= NULL;

        int			 ret;
        int			 err;

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

                err = -NM_EINVAL;
                goto out;

        active_gate_found:
                p_tcp_trk->next_entry	= i;
                p_pw->p_gate		=
                        p_core->gate_array + p_tcp_trk->gate_map[i];
        }

        /* gate already selected or selective recv 		*/

        p_gate		= p_pw->p_gate;

        if (!p_pw->gate_priv) {
                p_pw->gate_priv		=
                        p_pw->p_gate->p_gate_drv_array[p_drv->id]->info;
        }

        p_tcp_gate	= p_pw->gate_priv;

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
                        if (p_gate_pollfd->revents & POLLHUP) {
                                NM_TRACEF("tcp incoming single poll: pollhup");
                               err = -NM_ECLOSED;
                        } else if (p_gate_pollfd->revents & POLLNVAL) {
                                NM_TRACEF("tcp incoming single poll: pollnval");
                                err = -NM_EINVAL;
                        } else {
                                err = -NM_EBROKEN;
                        }

                        p_gate_pollfd->revents = 0;
                        goto out;
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
                        err = -NM_ECLOSED;
                } else if (pollfd.revents & POLLNVAL) {
                        NM_TRACEF("tcp incoming single poll: pollnval");
                        err = -NM_EINVAL;
                } else {
                        err = -NM_EBROKEN;
                }

                goto out;
        }

 poll_not_needed:
        err	= NM_ESUCCESS;

 out:
        return err;
}

/* used for post and poll */
static
int
nm_tcp_send_iov	(struct nm_pkt_wrap *p_pw) {
        struct nm_tcp_gate	*p_tcp_gate	= NULL;
        struct nm_tcp_pkt_wrap	*p_tcp_pw	= NULL;
        struct nm_iovec_iter	*p_vi		= NULL;
        struct iovec		*p_cur		= NULL;
        int			 ret;
        int			 err;

        err	= nm_tcp_outgoing_poll(p_pw);
        if (err < 0)
                goto out;

	p_tcp_gate	= p_pw->gate_priv;
        p_tcp_pw	= p_pw->drv_priv;
        if (!p_tcp_pw) {
                p_tcp_pw	= TBX_MALLOC(sizeof(struct nm_tcp_pkt_wrap));
                /* current entry num is first entry num
                 */
                p_tcp_pw->vi.v_cur	= p_pw->v_first;

                /* save a copy of current entry
                 */
                p_tcp_pw->vi.cur_copy	= p_pw->v[p_pw->v_first];

                /* current vector size is full vector size
                 */
                p_tcp_pw->vi.v_cur_size	= p_pw->v_nb;
        }

        /* get a pointer to the iterator
         */
        p_vi	= &(p_tcp_pw->vi);

        /* get a pointer to the current entry
         */
        p_cur	= p_pw->v + p_tcp_pw->vi.v_cur;

        NM_TRACE_VAL("tcp outgoing trk id", p_pw->p_trk->id);
        NM_TRACE_VAL("tcp outgoing fd", p_tcp_gate->fd[p_pw->p_trk->id]);
        NM_TRACE_VAL("tcp outgoing cur size", p_vi->v_cur_size);
        NM_TRACE_PTR("tcp outgoing cur base", p_cur->iov_base);
        NM_TRACE_VAL("tcp outgoing cur len", p_cur->iov_len);

        if (!p_vi->v_cur_size) {
                err	= NM_ESUCCESS;

                goto out;
        }

 writev_again:
        ret	= writev(p_tcp_gate->fd[p_pw->p_trk->id], p_cur, p_vi->v_cur_size);
        NM_TRACE_VAL("tcp outgoing writev ret", ret);

        if (ret < 0) {
                if (errno == EINTR)
                        goto writev_again;

                perror("writev");
                err = -NM_ESCFAILD;
                goto out;
        }

        if (ret	== 0) {
                NM_TRACEF("connection closed");
                err = -NM_ECLOSED;
                goto out;
        }

        do {
                uint64_t len	= tbx_min(p_cur->iov_len, ret);
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

                        goto out;
                }

                p_cur++;
                p_vi->cur_copy = *p_cur;
                ret -= len;
        } while (ret);

        err	= -NM_EAGAIN;

 out:
        return err;
}

/* used for post and poll */
static
int
nm_tcp_recv_iov	(struct nm_pkt_wrap *p_pw) {
        struct nm_tcp_gate	*p_tcp_gate	= NULL;
        struct nm_tcp_pkt_wrap	*p_tcp_pw	= NULL;
        struct nm_iovec_iter	*p_vi		= NULL;
        struct iovec		*p_cur		= NULL;
        int			 ret;
        int			 err;

        err	= nm_tcp_incoming_poll(p_pw);
        if (err < 0)
                goto out;

        p_tcp_gate	= p_pw->gate_priv;

        p_tcp_pw	= p_pw->drv_priv;
        if (!p_tcp_pw) {
                p_tcp_pw	= TBX_MALLOC(sizeof(struct nm_tcp_pkt_wrap));
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
        }

        /* get a pointer to the iterator
         */
        p_vi	= &(p_tcp_pw->vi);

        /* get a pointer to the current entry
         */
        p_cur	= p_pw->v + p_tcp_pw->vi.v_cur;

        NM_TRACE_VAL("tcp incoming trk id", p_pw->p_trk->id);
        NM_TRACE_VAL("tcp incoming fd", p_tcp_gate->fd[p_pw->p_trk->id]);
        NM_TRACE_VAL("tcp incoming cur size", p_vi->v_cur_size);
        NM_TRACE_PTR("tcp incoming cur base", p_cur->iov_base);
        NM_TRACE_VAL("tcp incoming cur len", p_cur->iov_len);

        if (!p_vi->v_cur_size) {
                err	= NM_ESUCCESS;

                goto out;
        }

 readv_again:
        ret	= readv(p_tcp_gate->fd[p_pw->p_trk->id], p_cur, p_vi->v_cur_size);

        if (ret < 0) {
                if (errno == EINTR)
                        goto readv_again;

                perror("readv");

                err = -NM_ESCFAILD;
                goto out;
        }

        if (ret	== 0) {
                NM_TRACEF("connection closed");
                err = -NM_ECLOSED;
                goto out;
        }

        do {
                uint64_t len	= tbx_min(p_cur->iov_len, ret);
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

                        goto out;
                }

                p_cur++;
                p_vi->cur_copy = *p_cur;
                ret -= len;
        } while (ret);

        err	= -NM_EAGAIN;

 out:
        return err;
}

int
nm_tcp_load(struct nm_drv_ops *p_ops) {
        p_ops->init		= nm_tcp_init;
        p_ops->exit             = nm_tcp_exit;
        p_ops->open_trk		= nm_tcp_open_trk;
        p_ops->close_trk	= nm_tcp_close_trk;
        p_ops->connect		= nm_tcp_connect;
        p_ops->accept		= nm_tcp_accept;
        p_ops->disconnect       = nm_tcp_disconnect;
        p_ops->post_send_iov	= nm_tcp_send_iov;
        p_ops->post_recv_iov    = nm_tcp_recv_iov;
        p_ops->poll_send_iov	= nm_tcp_send_iov;
        p_ops->poll_recv_iov	= nm_tcp_recv_iov;

        return NM_ESUCCESS;
}

