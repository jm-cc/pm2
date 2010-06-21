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

#include "marcel.h"
#include <unistd.h>
#include <errno.h>

typedef struct unix_io_server {
	struct marcel_ev_server server;
	fd_set rfds, wfds;
	unsigned nb;
} *unix_io_serverid_t;

typedef enum {
	POLL_READ,
	POLL_WRITE,
	POLL_SELECT
} poll_op_t;

#undef NFDS
#if __GNUC__ < 3
#  define MA_GCC_NAME __ma_gcc
#  define FD MA_GCC_NAME.fd
#  define RFDS MA_GCC_NAME.MA_GCC_NAME.rfds
#  define WFDS MA_GCC_NAME.MA_GCC_NAME.wfds
#  define NFDS MA_GCC_NAME.MA_GCC_NAME.nfds
#else
#  define MA_GCC_NAME
#  define FD fd
#  define RFDS rfds
#  define WFDS wfds
#  define NFDS nfds
#endif
typedef struct tcp_ev {
	struct marcel_ev_req inst;
	poll_op_t op;
	int ret_val;
	union {
		int fd;
		struct {
			fd_set *rfds;
			fd_set *wfds;
			int nfds;
		} MA_GCC_NAME;
	} MA_GCC_NAME;
} *tcp_ev_t;

static struct unix_io_server unix_io_server;


static int unix_io_group(marcel_ev_server_t server,
			 marcel_ev_op_t _op TBX_UNUSED,
			 marcel_ev_req_t req TBX_UNUSED,
			 int nb_ev TBX_UNUSED, int option TBX_UNUSED)
{
	unix_io_serverid_t uid = tbx_container_of(server, struct unix_io_server, server);
	tcp_ev_t ev;

	MARCEL_LOG("Grouping IO poll\n");
	uid->nb = 0;
	FD_ZERO(&uid->rfds);
	FD_ZERO(&uid->wfds);

	FOREACH_REQ_POLL(ev, server, inst) {
		switch (ev->op) {
		case POLL_READ:{
				FD_SET(ev->FD, &uid->rfds);
				uid->nb = tbx_max((int) uid->nb, ev->FD + 1);
				break;
			}
		case POLL_WRITE:{
				FD_SET(ev->FD, &uid->wfds);
				uid->nb = tbx_max((int) uid->nb, ev->FD + 1);
				break;
			}
		case POLL_SELECT:{
				int i;
				if (ev->RFDS != NULL) {
					for (i = 0; i < ev->NFDS; i++)
						if (FD_ISSET(i, ev->RFDS))
							FD_SET(i, &uid->rfds);
				}
				if (ev->WFDS != NULL) {
					for (i = 0; i < ev->NFDS; i++)
						if (FD_ISSET(i, ev->WFDS))
							FD_SET(i, &uid->wfds);
				}
				uid->nb = tbx_max((int) uid->nb, ev->NFDS);
				break;
			}
		default:
			MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
		}
	}
	return 0;
}

inline static void unix_io_check_select(unix_io_serverid_t uid TBX_UNUSED, tcp_ev_t ev,
					fd_set * __restrict rfds,
					fd_set * __restrict wfds)
{
	MARCEL_LOG("Checking select for IO poll (at least one success)\n");
	switch (ev->op) {
	case POLL_READ:
		if (FD_ISSET(ev->FD, rfds))
			MARCEL_EV_REQ_SUCCESS(&ev->inst);
		break;
	case POLL_WRITE:
		if (FD_ISSET(ev->FD, wfds))
			MARCEL_EV_REQ_SUCCESS(&ev->inst);
		break;
	case POLL_SELECT:{
			int i;
			unsigned zeroed = 0;
			ev->ret_val = 0;
			if (ev->RFDS != NULL) {
				for (i = 0; i < ev->NFDS; i++)
					if (FD_ISSET(i, ev->RFDS) && FD_ISSET(i, rfds)) {
						if (!zeroed) {
							FD_ZERO(ev->RFDS);
							if (ev->WFDS != NULL)
								FD_ZERO(ev->WFDS);
							zeroed = 1;
						}
						FD_SET(i, ev->RFDS);
						ev->ret_val++;
					}
			}
			if (ev->WFDS != NULL) {
				for (i = 0; i < ev->NFDS; i++)
					if (FD_ISSET(i, ev->WFDS) && FD_ISSET(i, wfds)) {
						if (!zeroed) {
							FD_ZERO(ev->WFDS);
							if (ev->RFDS != NULL)
								FD_ZERO(ev->RFDS);
						}
						FD_SET(i, ev->WFDS);
						ev->ret_val++;
					}
			}
			if (ev->ret_val) {
				MARCEL_EV_REQ_SUCCESS(&ev->inst);
			}
			break;
		}
	default:
		MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
	}
}

#define ma_timerclear(tv) ((tv)->tv_sec = (tv)->tv_usec = 0)
static int unix_io_poll(marcel_ev_server_t server,
			marcel_ev_op_t _op TBX_UNUSED,
			marcel_ev_req_t req TBX_UNUSED,
			int nb_ev TBX_UNUSED, int option TBX_UNUSED)
{
	unix_io_serverid_t uid = tbx_container_of(server, struct unix_io_server, server);
	tcp_ev_t ev;
	int r;
	fd_set rfds, wfds;
	struct timeval tv, *ptv;

	MARCEL_LOG("Polling function called on LWP %d\n", ma_vpnum(MA_LWP_SELF));

	ma_timerclear(&tv);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	ptv = &tv;

	rfds = uid->rfds;
	wfds = uid->wfds;
	r = select(uid->nb, &rfds, &wfds, NULL, ptv);

	if (tbx_unlikely(r == -1)) {
		int found = 0;
		if (errno != EBADF)
			return 0;
		/* L'un des fd est invalide */
		FOREACH_REQ_POLL(ev, server, inst) {
			MARCEL_LOG("Checking select for IO poll (with badFD)\n");
			switch (ev->op) {
			case POLL_READ:
			case POLL_WRITE:
				break;
			case POLL_SELECT:{
					ev->ret_val = select(ev->NFDS, ev->RFDS,
							     ev->WFDS, NULL, NULL);
					if (ev->ret_val) {
						MARCEL_EV_REQ_SUCCESS(&ev->inst);
						found = 1;
					}
					break;
				}
			default:
				MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
			}
			if (!found) {
				PM2_LOG("IO poll with bad fd not detected.\n"
					"Please, fix marcel code\n");
			}
		}
	} else if (r == 0)
		return 0;

	FOREACH_REQ_POLL(ev, server, inst) {
		unix_io_check_select(uid, ev, &rfds, &wfds);
	}
	return 0;
}

static int unix_io_fast_poll(marcel_ev_server_t server,
			     marcel_ev_op_t _op TBX_UNUSED,
			     marcel_ev_req_t req,
			     int nb_ev TBX_UNUSED, int option TBX_UNUSED)
{
	unix_io_serverid_t uid = tbx_container_of(server, struct unix_io_server, server);
	tcp_ev_t ev = tbx_container_of(req, struct tcp_ev, inst);

	fd_set rfds, wfds;
	unsigned nb = 0;
	struct timeval tv;
	int r;

	MARCEL_LOG("Fast Polling function called on LWP %d\n", ma_vpnum(MA_LWP_SELF));
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

	switch (ev->op) {
	case POLL_READ:{
			FD_SET(ev->FD, &rfds);
			nb = ev->FD + 1;
			break;
		}
	case POLL_WRITE:{
			FD_SET(ev->FD, &wfds);
			nb = ev->FD + 1;
			break;
		}
	case POLL_SELECT:{
			if (ev->RFDS != NULL)
				rfds = *(ev->RFDS);
			if (ev->WFDS != NULL)
				wfds = *(ev->WFDS);
			nb = ev->NFDS;
			break;
		}
	default:
		MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
	}

	ma_timerclear(&tv);

	errno = 0;
	r = select(nb, &rfds, &wfds, NULL, &tv);

	if (r <= 0) {
		ev->ret_val = -errno;
		return 0;
	}

	unix_io_check_select(uid, ev, &rfds, &wfds);
	return 0;
}

int marcel_read(int fildes, void *buf, size_t nbytes)
{
	int n;
	struct tcp_ev ev;
	struct marcel_ev_wait wait;
	MARCEL_LOG_IN();
	do {
		ev.op = POLL_READ;
		ev.FD = fildes;
		MARCEL_LOG("Reading in fd %i\n", fildes);
		marcel_ev_wait(&unix_io_server.server, &ev.inst, &wait, 0);
		n = read(fildes, buf, nbytes);
	} while (n == -1 && errno == EINTR);

	MARCEL_LOG_RETURN(n);
}

int marcel_readv(int fildes, const struct iovec *iov, int iovcnt)
{
	struct tcp_ev ev;
	struct marcel_ev_wait wait;
	MARCEL_LOG_IN();
	ev.op = POLL_READ;
	ev.FD = fildes;
	MARCEL_LOG("Reading in fd %i\n", fildes);
	marcel_ev_wait(&unix_io_server.server, &ev.inst, &wait, 0);

	MARCEL_LOG_RETURN(readv(fildes, iov, iovcnt));
}

int marcel_write(int fildes, const void *buf, size_t nbytes)
{
	int n;
	struct tcp_ev ev;
	struct marcel_ev_wait wait;
	MARCEL_LOG_IN();
	do {
		ev.op = POLL_WRITE;
		ev.FD = fildes;
		MARCEL_LOG("Writing in fd %i\n", fildes);
		marcel_ev_wait(&unix_io_server.server, &ev.inst, &wait, 0);
		n = write(fildes, buf, nbytes);
	} while (n == -1 && errno == EINTR);

	MARCEL_LOG_RETURN(n);
}

int marcel_writev(int fildes, const struct iovec *iov, int iovcnt)
{
	struct tcp_ev ev;
	struct marcel_ev_wait wait;
	MARCEL_LOG_IN();
	ev.op = POLL_WRITE;
	ev.FD = fildes;
	MARCEL_LOG("Writing in fd %i\n", fildes);
	marcel_ev_wait(&unix_io_server.server, &ev.inst, &wait, 0);
	MARCEL_LOG_RETURN(writev(fildes, iov, iovcnt));
}

int marcel_select(int nfds, fd_set * __restrict rfds, fd_set * __restrict wfds)
{
	struct tcp_ev ev;
	struct marcel_ev_wait wait;

	MARCEL_LOG_IN();
	ev.op = POLL_SELECT;
	ev.RFDS = rfds;
	ev.WFDS = wfds;
	ev.NFDS = nfds;
	MARCEL_LOG("Selecting within %i fds\n", nfds);
	marcel_ev_wait(&unix_io_server.server, &ev.inst, &wait, 0);
	MARCEL_LOG_RETURN(ev.ret_val >= 0 ? ev.ret_val : (errno = -ev.ret_val, -1));
}

/* To force the reading/writing of an exact number of bytes */

int marcel_read_exactly(int fildes, void *buf, size_t nbytes)
{
	size_t to_read = nbytes;
	ssize_t n;

	MARCEL_LOG_IN();
	do {
		n = marcel_read(fildes, buf, to_read);
		if (n < 0)
			return n;
		buf = (char *)buf + n;
		to_read -= n;
	} while (to_read);

	MARCEL_LOG_RETURN(nbytes);
}

int marcel_readv_exactly(int fildes, const struct iovec *iov, int iovcnt)
{
	return marcel_readv(fildes, iov, iovcnt);
}

int marcel_write_exactly(int fildes, const void *buf, size_t nbytes)
{
	size_t to_write = nbytes;
	ssize_t n;

	MARCEL_LOG_IN();
	do {
		n = marcel_write(fildes, buf, to_write);
		if (n < 0)
			return n;
		buf = (char *)buf + n;
		to_write -= n;
	} while (to_write);

	MARCEL_LOG_RETURN(nbytes);
}

int marcel_writev_exactly(int fildes, const struct iovec *iov, int iovcnt)
{
	return marcel_writev(fildes, iov, iovcnt);
}

/* =============== Gestion des E/S non bloquantes =============== */

int tselect(int width, fd_set * __restrict readfds,
	    fd_set * __restrict writefds, fd_set * __restrict exceptfds)
{
	int res = 0;
	struct timeval timeout;
	fd_set rfds, wfds, efds;

	do {
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_ZERO(&efds);
		if (readfds)
			rfds = *readfds;
		if (writefds)
			wfds = *writefds;
		if (exceptfds)
			efds = *exceptfds;

		ma_timerclear(&timeout);
		res = select(width, &rfds, &wfds, &efds, &timeout);
		if (res <= 0) {
			if (res < 0 && (errno != EINTR))
				break;
			marcel_yield();
		}
	} while (res <= 0);

	if (readfds)
		*readfds = rfds;
	if (writefds)
		*writefds = wfds;
	if (exceptfds)
		*exceptfds = efds;

	return res;
}

void ma_io_init(void)
{
	MARCEL_LOG_IN();
	marcel_ev_server_init(&unix_io_server.server, "Unix TCP I/O");
	marcel_ev_server_set_poll_settings(&unix_io_server.server,
					   MA_EV_POLL_AT_TIMER_SIG | MA_EV_POLL_AT_IDLE,
					   1);
#ifndef MARCEL_DO_NOT_GROUP_TCP
	marcel_ev_server_add_callback(&unix_io_server.server,
				      MARCEL_EV_FUNCTYPE_POLL_GROUP, &unix_io_group);
	marcel_ev_server_add_callback(&unix_io_server.server,
				      MARCEL_EV_FUNCTYPE_POLL_POLLANY, &unix_io_poll);
#endif
	marcel_ev_server_add_callback(&unix_io_server.server,
				      MARCEL_EV_FUNCTYPE_POLL_POLLONE,
				      &unix_io_fast_poll);
	marcel_ev_server_start(&unix_io_server.server);
	MARCEL_LOG_OUT();
}
