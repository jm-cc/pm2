
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

#define MA_FILE_DEBUG io
#include "marcel.h"

#include <unistd.h>
#include <errno.h>

#ifndef max
#define max(a, b) \
  (__gnu_extention__ \
   ({ __typeof__ (a) _a=a; \
      __typeof__ (b) _b=b; \
      ((_a) > (_b) ? (_a) : (_b)) \
   })) 
#endif

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

typedef struct tcp_ev {
	struct marcel_ev inst;
	poll_op_t op;
	int ret_val;
	union {
		int fd;
		struct {
			fd_set *rfds;
			fd_set *wfds;
			int nfds;	
		};
	};
} *tcp_ev_t;

struct unix_io_server unix_io_server = {
    .server=MARCEL_EV_SERVER_INIT(unix_io_server.server, "Unix TCP I/O"),
};

static int unix_io_group(marcel_ev_serverid_t id, 
			  marcel_ev_op_t _op,
			  marcel_ev_inst_t _ev, 
			  int nb_ev, int option)
{
	unix_io_serverid_t uid=struct_up(id, struct unix_io_server, server);
	tcp_ev_t ev, tmp;

	mdebug("Grouping IO poll\n");
	uid->nb = 0;
	FD_ZERO(&uid->rfds);
	FD_ZERO(&uid->wfds);
	
	FOREACH_EV_POLL(ev, tmp, id, inst) {
		switch(ev->op) {
		case POLL_READ : {
			FD_SET(ev->fd, &uid->rfds);
			uid->nb = max(uid->nb, ev->fd+1);
			break;
		}
		case POLL_WRITE : {
			FD_SET(ev->fd, &uid->wfds);
			uid->nb = max(uid->nb, ev->fd+1);
			break;
		}
		case POLL_SELECT : {
			unsigned i;
			if(ev->rfds != NULL) {
				for(i=0; i<ev->nfds; i++)
					if(FD_ISSET(i, ev->rfds))
						FD_SET(i, &uid->rfds);
			}
			if(ev->wfds != NULL) {
				for(i=0; i<ev->nfds; i++)
					if(FD_ISSET(i, ev->wfds))
						FD_SET(i, &uid->wfds);
			}
			uid->nb = max(uid->nb, ev->nfds);
			break;
		}
		default :
			RAISE(PROGRAM_ERROR);
		}
	}
	return 0;
}

inline static void unix_io_check_select(unix_io_serverid_t uid, tcp_ev_t ev,
					fd_set *rfds, fd_set *wfds)
{
	mdebug("Checking select for IO poll (at least one success)\n");
	switch(ev->op) {
	case POLL_READ :
		if(FD_ISSET(ev->fd, rfds))
			MARCEL_EV_POLL_SUCCESS(&uid->server, &ev->inst);
		break;
	case POLL_WRITE :
		if(FD_ISSET(ev->fd, wfds))
			MARCEL_EV_POLL_SUCCESS(&uid->server, &ev->inst);
		break;
	case POLL_SELECT : {
		unsigned i;
		unsigned zeroed=0;
		ev->ret_val=0;
		if(ev->rfds != NULL) {
			for(i=0; i<ev->nfds; i++)
				if(FD_ISSET(i, ev->rfds) && FD_ISSET(i, rfds)) {
					if (!zeroed) {
						FD_ZERO(ev->rfds);
						if(ev->wfds != NULL)
							FD_ZERO(ev->wfds);
						zeroed=1;
					}
					FD_SET(i, ev->rfds);
					ev->ret_val++;
				}
		}
		if(ev->wfds != NULL) {
			for(i=0; i<ev->nfds; i++)
				if(FD_ISSET(i, ev->wfds) && FD_ISSET(i, wfds)) {
					if (!zeroed) {
						FD_ZERO(ev->wfds);
						if(ev->rfds != NULL)
							FD_ZERO(ev->rfds);
					}
					FD_SET(i, ev->wfds);
					ev->ret_val++;
				}
		}
		if (ev->ret_val) {
			MARCEL_EV_POLL_SUCCESS(&uid->server, &ev->inst);
		}
		break;
	}
	default :
		RAISE(PROGRAM_ERROR);
	}
}

static int unix_io_poll(marcel_ev_serverid_t id, 
			 marcel_ev_op_t _op,
			 marcel_ev_inst_t _ev, 
			 int nb_ev, int option)
{
	unix_io_serverid_t uid=struct_up(id, struct unix_io_server, server);
	tcp_ev_t ev, tmp;
	int r;
	fd_set rfds, wfds;
	struct timeval tv, *ptv;
  
#ifndef MA__ACTIVATION
	// Trop de messages avec les activations
	mdebug("Polling function called on LWP %d\n",
	       marcel_current_vp());
#endif

	timerclear(&tv);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	ptv = &tv;

	rfds = uid->rfds;
	wfds = uid->wfds;
	r = select(uid->nb, &rfds, &wfds, NULL, ptv);

	if (tbx_unlikely(r==EBADF)) {
		int found=0;
		/* L'un des fd est invalide */
		FOREACH_EV_POLL(ev, tmp, id, inst) {
			mdebug("Checking select for IO poll (with badFD)\n");
			switch(ev->op) {
			case POLL_READ :
			case POLL_WRITE :
				break;
			case POLL_SELECT : {
				ev->ret_val=select(ev->nfds, ev->rfds,
						   ev->wfds, NULL, NULL);
				if (ev->ret_val) {
					MARCEL_EV_POLL_SUCCESS(&uid->server, &ev->inst);
					found=1;
				}
				break;
			}
			default :
				RAISE(PROGRAM_ERROR);
			}
			if (!found) {
				pm2debug("IO poll with bad fd not detected.\n"
						"Please, fix marcel code\n");
			}
		}
	}
		
	if (r <= 0)
		return 0;

	FOREACH_EV_POLL(ev, tmp, id, inst) {
		unix_io_check_select(uid, ev, &rfds, &wfds);
	}
	return 0;
}

static int unix_io_fast_poll(marcel_ev_serverid_t id, 
			      marcel_ev_op_t _op,
			      marcel_ev_inst_t _ev, 
			      int nb_ev, int option)
{
	unix_io_serverid_t uid=struct_up(id, struct unix_io_server, server);
	tcp_ev_t ev=struct_up(_ev, struct tcp_ev, inst);

	fd_set rfds, wfds;
	unsigned nb = 0;
	struct timeval tv;
	int r;

#ifndef MA__ACTIVATION
	// Trop de messages avec les activations
	mdebug("Fast Polling function called on LWP %d\n",
	       marcel_current_vp());
#endif
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

	switch(ev->op) {
	case POLL_READ : {
		FD_SET(ev->fd, &rfds);
		nb = ev->fd + 1;
		break;
	}
	case POLL_WRITE : {
		FD_SET(ev->fd, &wfds);
		nb = ev->fd + 1;
		break;
	}
	case POLL_SELECT : {
		if(ev->rfds != NULL)
			rfds = *(ev->rfds);
		if(ev->wfds != NULL)
			wfds = *(ev->wfds);
		nb = ev->nfds;
		break;
	}
	default :
		RAISE(PROGRAM_ERROR);
	}

	timerclear(&tv);
	
	r = select(nb, &rfds, &wfds, NULL, &tv);

	if(r <= 0) {
		ev->ret_val=r;
		return 0;
	}

	unix_io_check_select(uid, ev, &rfds, &wfds);
	return 0;
}

int marcel_read(int fildes, void *buf, size_t nbytes)
{
	int n;
#ifndef MA__ACTIVATION
	struct tcp_ev ev;

	ev.op = POLL_READ;
	ev.fd = fildes;
	mdebug("Reading in fd %i\n", fildes);
	marcel_ev_wait(&unix_io_server.server, &ev.inst);
#endif

	LOG("IO reading fd %i", fildes);
	do {
		n = read(fildes, buf, nbytes);
	} while( n == -1 && errno == EINTR);
	
	return n;
}

int marcel_readv(int fildes, const struct iovec *iov, int iovcnt)
{
#ifndef MA__ACTIVATION
	struct tcp_ev ev;

	ev.op = POLL_READ;
	ev.fd = fildes;
	mdebug("Reading in fd %i\n", fildes);
	marcel_ev_wait(&unix_io_server.server, &ev.inst);
#endif

	LOG("IO readving fd %i", fildes);
	return readv(fildes, iov, iovcnt);
}

int marcel_write(int fildes, const void *buf, size_t nbytes)
{
	int n;
#ifndef MA__ACTIVATION
	struct tcp_ev ev;

	ev.op = POLL_WRITE;
	ev.fd = fildes;
	mdebug("Writing in fd %i\n", fildes);
	marcel_ev_wait(&unix_io_server.server, &ev.inst);
#endif

	LOG("IO writing fd %i", fildes);
	do {
		n = write(fildes, buf, nbytes);
	} while( n == -1 && errno == EINTR);
	
	return n;
}

int marcel_writev(int fildes, const struct iovec *iov, int iovcnt)
{
#ifndef MA__ACTIVATION
	struct tcp_ev ev;

	ev.op = POLL_WRITE;
	ev.fd = fildes;
	mdebug("Writing in fd %i\n", fildes);
	marcel_ev_wait(&unix_io_server.server, &ev.inst);
#endif

	LOG("IO writving fd %i", fildes);
	return writev(fildes, iov, iovcnt);
}

int marcel_select(int nfds, fd_set *rfds, fd_set *wfds)
{
#ifdef MA__ACTIVATION
	return select(nfds, rfds, wfds, NULL, NULL);
#else
	struct tcp_ev ev;

	ev.op = POLL_SELECT;
	ev.rfds = rfds;
	ev.wfds = wfds;
	ev.nfds = nfds;
	mdebug("Selecting within %i fds\n", nfds);
	marcel_ev_wait(&unix_io_server.server, &ev.inst);
	return ev.ret_val;
#endif
}

/* To force the reading/writing of an exact number of bytes */

int marcel_read_exactly(int fildes, void *buf, size_t nbytes)
{
	size_t to_read = nbytes, n;

	do {
		n = marcel_read(fildes, buf, to_read);
		if(n < 0)
			return n;
		buf += n;
		to_read -= n;
	} while(to_read);
	
	return nbytes;
}

int marcel_readv_exactly(int fildes, const struct iovec *iov, int iovcnt)
{
	return marcel_readv(fildes, iov, iovcnt);
}

int marcel_write_exactly(int fildes, const void *buf, size_t nbytes)
{
	size_t to_write = nbytes, n;

	do {
		n = marcel_write(fildes, buf, to_write);
		if(n < 0)
			return n;
		buf += n;
		to_write -= n;
	} while(to_write);
	
	return nbytes;
}

int marcel_writev_exactly(int fildes, const struct iovec *iov, int iovcnt)
{
	return marcel_writev(fildes, iov, iovcnt);
}

/* =============== Gestion des E/S non bloquantes =============== */

int tselect(int width, fd_set *readfds, fd_set *writefds, fd_set *exceptfds)
{
#ifdef MA__ACTIVATION
	return select(width, readfds, writefds, exceptfds, NULL);
#else
	int res = 0;
	struct timeval timeout;
	fd_set rfds, wfds, efds;
	
	do {
		FD_ZERO(&rfds); FD_ZERO(&wfds); FD_ZERO(&efds);
		if(readfds) rfds = *readfds;
		if(writefds) wfds = *writefds;
		if(exceptfds) efds = *exceptfds;
		
		timerclear(&timeout);
		res = select(width, &rfds, &wfds, &efds, &timeout);
		if(res <= 0) {
			if (res < 0 && (errno != EINTR))
				break;
			marcel_yield();
		}
	} while(res <= 0);
	
	if(readfds) *readfds = rfds;
	if(writefds) *writefds = wfds;
	if(exceptfds) *exceptfds = efds;
   
	return res;
#endif /* MA__ACTIVATION */
}

void __init marcel_io_init(void)
{
	marcel_ev_server_set_poll_settings(&unix_io_server.server, 
					   MARCEL_POLL_AT_TIMER_SIG 
					   | MARCEL_POLL_AT_YIELD
					   | MARCEL_POLL_AT_IDLE,
					   1);
#ifndef MARCEL_DO_NOT_GROUP_TCP
	marcel_ev_server_add_callback(&unix_io_server.server,
				      MARCEL_EV_FUNCTYPE_POLL_GROUP,
				      &unix_io_group);
	marcel_ev_server_add_callback(&unix_io_server.server,
				      MARCEL_EV_FUNCTYPE_POLL_ALL,
				      &unix_io_poll);
#endif
	marcel_ev_server_add_callback(&unix_io_server.server,
				      MARCEL_EV_FUNCTYPE_POLL_ONE,
				      &unix_io_fast_poll);
	marcel_ev_server_start(&unix_io_server.server);
}

__ma_initfunc(marcel_io_init, MA_INIT_IO,
		   "standard I/O polling");

