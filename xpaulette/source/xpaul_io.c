
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

/* TODO: utiliser poll au lieu de select (+ voir scalabilité) */
#define XPAUL_FILE_DEBUG xpaul_io
#include "xpaul.h"

#ifdef MARCEL
#include "marcel.h"
#include "marcel_timer.h"
#endif				// MARCEL

#include "xpaul_ev_serv.h"
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#ifndef max
#define max(a, b) \
  (__gnu_extention__ \
   ({ __typeof__ (a) _a=a; \
      __typeof__ (b) _b=b; \
      ((_a) > (_b) ? (_a) : (_b)) \
   }))
#endif

typedef struct xpaul_io_server {
	struct xpaul_server server;
	fd_set polling_rfds, polling_wfds;
	unsigned polling_nb;
	fd_set syscall_rfds, syscall_wfds;
	unsigned syscall_nb;
} *xpaul_io_serverid_t;

typedef enum {
	XPAUL_POLL_READ,
	XPAUL_POLL_WRITE,
	XPAUL_POLL_SELECT
} xpaul_poll_op_t;

#if __GNU__ < 3
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

typedef struct xpaul_tcp_ev {
	struct xpaul_req inst;
	xpaul_poll_op_t op;
	int ret_val;
	union {
		int fd;
		struct {
			fd_set *rfds;
			fd_set *wfds;
			int nfds;
		} MA_GCC_NAME;
	} MA_GCC_NAME;
} *xpaul_tcp_ev_t;

static struct xpaul_io_server xpaul_io_server = {
	.server =
	XPAUL_SERVER_INIT(xpaul_io_server.server, "Unix TCP I/O"),
};

#ifdef MA__LWPS

//#define MAX_REQS 50
typedef struct {
	int n;
	fd_set *readfds;
	fd_set *writefds;
	fd_set *exceptfds;
	struct timeval *timeout;	/* not used (TODO?) */
//	struct list_head lwait[MAX_REQS][2];/* TODO: dynamique? */
} requete;

#endif

static int xpaul_io_group(xpaul_server_t server,
			  xpaul_op_t _op,
			  xpaul_req_t req, int nb_ev, int option)
{
	xpaul_io_serverid_t uid =
	    struct_up(server, struct xpaul_io_server, server);
	xpaul_tcp_ev_t ev;

	xdebug("Grouping IO poll\n");
	uid->polling_nb = 0;
	FD_ZERO(&uid->polling_rfds);
	FD_ZERO(&uid->polling_wfds);

	FOREACH_REQ_REGISTERED(ev, server, inst) {
		if(ev->inst.state & XPAUL_STATE_OCCURED)
			fprintf(stderr, "Query allready occured\n");
		else{
			switch (ev->op) {
			case XPAUL_POLL_READ:{
				FD_SET(ev->FD, &uid->polling_rfds);
				uid->polling_nb = tbx_max(uid->polling_nb, ev->FD + 1);
				break;
			}
			case XPAUL_POLL_WRITE:{
				FD_SET(ev->FD, &uid->polling_wfds);
				uid->polling_nb = tbx_max(uid->polling_nb, ev->FD + 1);
				break;
			}
			case XPAUL_POLL_SELECT:{
				unsigned i;
				if (ev->RFDS != NULL) {
					for (i = 0; i < ev->NFDS; i++)
						if (FD_ISSET(i, ev->RFDS))
							FD_SET(i,
							       &uid->polling_rfds);
				}
				if (ev->WFDS != NULL) {
					for (i = 0; i < ev->NFDS; i++)
						if (FD_ISSET(i, ev->WFDS))
							FD_SET(i,
							       &uid->polling_wfds);
				}
				
				uid->polling_nb = tbx_max(uid->polling_nb, ev->NFDS);
				break;
			}
			default:
				XPAUL_EXCEPTION_RAISE(XPAUL_PROGRAM_ERROR);
			}
		}
	}
	return 0;
}

#ifdef MA__LWPS
static int xpaul_io_syscall_group(xpaul_server_t server,
			  xpaul_op_t _op,
			  xpaul_req_t req, int nb_ev, int option)
{
	xpaul_io_serverid_t uid =
	    struct_up(server, struct xpaul_io_server, server);
	xpaul_tcp_ev_t ev;

	xdebug("Grouping IO poll\n");
	uid->syscall_nb = 0;
	FD_ZERO(&uid->syscall_rfds);
	FD_ZERO(&uid->syscall_wfds);
	
	/* vide la liste des requete groupees pour le polling */
	uid->polling_nb=0;

	FOREACH_REQ_BLOCKING(ev, server, inst) {
		switch (ev->op) {
		case XPAUL_POLL_READ:{
				FD_SET(ev->FD, &uid->syscall_rfds);
				uid->syscall_nb = tbx_max(uid->syscall_nb, ev->FD + 1);
				break;
			}
		case XPAUL_POLL_WRITE:{
				FD_SET(ev->FD, &uid->syscall_wfds);
				uid->syscall_nb = tbx_max(uid->syscall_nb, ev->FD + 1);
				break;
			}
		case XPAUL_POLL_SELECT:{
				unsigned i;
				if (ev->RFDS != NULL) {
					for (i = 0; i < ev->NFDS; i++)
						if (FD_ISSET(i, ev->RFDS))
							FD_SET(i,
							       &uid->syscall_rfds);
				}
				if (ev->WFDS != NULL) {
					for (i = 0; i < ev->NFDS; i++)
						if (FD_ISSET(i, ev->WFDS))
							FD_SET(i,
							       &uid->syscall_wfds);
				}
				uid->syscall_nb = tbx_max(uid->syscall_nb, ev->NFDS);
				break;
			}
		default:
			XPAUL_EXCEPTION_RAISE(XPAUL_PROGRAM_ERROR);
		}

		if(!(ev->inst.state & XPAUL_STATE_EXPORTED)) {
			ev->inst.state|=XPAUL_STATE_EXPORTED;
		}
	}
	return 0;
}
#endif

__tbx_inline__ static void xpaul_io_check_select(xpaul_io_serverid_t uid,
 					 xpaul_tcp_ev_t ev,
					 fd_set * __restrict rfds,
					 fd_set * __restrict wfds)
{
	xdebug("Checking select for IO poll (at least one success)\n");

	switch (ev->op) {
	case XPAUL_POLL_READ:
		if (FD_ISSET(ev->FD, rfds)){
			xpaul_req_success(&ev->inst);
		}
		break;
	case XPAUL_POLL_WRITE:
		if (FD_ISSET(ev->FD, wfds)){
			xpaul_req_success(&ev->inst);
		}
		break;
	case XPAUL_POLL_SELECT:{
			unsigned i;
			unsigned zeroed = 0;
			ev->ret_val = 0;
			if (ev->RFDS != NULL) {
				for (i = 0; i < ev->NFDS; i++)
					if (FD_ISSET(i, ev->RFDS)
					    && FD_ISSET(i, rfds)) {
						if (!zeroed) {
							FD_ZERO(ev->RFDS);
							if (ev->WFDS !=
							    NULL)
								FD_ZERO
								    (ev->
								     WFDS);
							zeroed = 1;
						}
						FD_SET(i, ev->RFDS);
						ev->ret_val++;
					}
			}
			if (ev->WFDS != NULL) {
				for (i = 0; i < ev->NFDS; i++)
					if (FD_ISSET(i, ev->WFDS)
					    && FD_ISSET(i, wfds)) {
						if (!zeroed) {
							FD_ZERO(ev->WFDS);
							if (ev->RFDS !=
							    NULL)
								FD_ZERO
								    (ev->
								     RFDS);
						}
						FD_SET(i, ev->WFDS);
						ev->ret_val++;
					}
			}
			if (ev->ret_val) {
				xpaul_req_success(&ev->inst);
			}
			break;
		}
	default:
		XPAUL_EXCEPTION_RAISE(XPAUL_PROGRAM_ERROR);
	}
}

#ifdef MA__LWPS


/* option : fd à surveiller (passage de commandes) */
static int xpaul_io_block(xpaul_server_t server,
			  xpaul_op_t _op,
			  xpaul_req_t req, int nb_ev, int option)
{				// a preciser
	xpaul_io_serverid_t uid =
	    struct_up(server, struct xpaul_io_server, server);
	xpaul_tcp_ev_t ev;

	int r;
	fd_set rfds, wfds;
	struct timeval tv, *ptv;
	PROF_EVENT1(xpaul_io_block_entry,ev);
	
#ifdef MARCEL
	xdebugl(6, "Syscall function called on LWP %d\n",
		marcel_current_vp());
#endif				// MARCEL

	timerclear(&tv);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	ptv = &tv;

	/* Ajoute le descripteur du tube pour pouvoir etre interrompu par une autre commande */
	FD_SET(option, &uid->syscall_rfds);
	if(option+1>uid->syscall_nb)
		uid->syscall_nb=option+1;

	rfds = uid->syscall_rfds;
	wfds = uid->syscall_wfds;

	do {
		r = select(uid->syscall_nb, &rfds, &wfds, NULL, NULL);
	}while(r==-1 && errno==EINTR);

	if (tbx_unlikely(r == -1)) {
		int found = 0;
		if (errno != EBADF)
			return 0;
		/* A fd is incorrect */
		/* TODO: XXX: lock */
		FOREACH_REQ_BLOCKING(ev, server, inst) {
			xdebug
			    ("Checking select for IO syscall (with badFD)\n");
			switch (ev->op) {
			case XPAUL_POLL_READ:
			case XPAUL_POLL_WRITE:
				break;
			case XPAUL_POLL_SELECT:{
					ev->ret_val =
					    select(ev->NFDS, ev->RFDS,
						   ev->WFDS, NULL, NULL);
					if (ev->ret_val) {
						xpaul_req_success(&ev->
								  inst);
						found = 1;
					}
					break;
				}
			default:
				XPAUL_EXCEPTION_RAISE(XPAUL_PROGRAM_ERROR);
			}
			if (!found) {
				pm2debug
				    ("IO syscall with bad fd not detected.\n"
				     "Please, fix marcel code\n");
			}
		}
	} else if (r == 0) {
		return 0;
	}

	/* TODO: XXX: lock */
	FOREACH_REQ_BLOCKING(ev, server, inst) {
		xpaul_io_check_select(uid, ev, &rfds, &wfds);
	}
	PROF_EVENT(xpaul_blockany_exit);
	return 0;
}


/* option : fd à surveiller (passage de commandes) */
static int xpaul_io_blockone(xpaul_server_t server,
			  xpaul_op_t _op,
			  xpaul_req_t req, int nb_ev, int option)
{				// a preciser

	xpaul_io_serverid_t uid =
	    struct_up(server, struct xpaul_io_server, server);
	xpaul_tcp_ev_t ev = struct_up(req, struct xpaul_tcp_ev, inst);

	fd_set rfds, wfds;
	unsigned nb = 0;
	struct timeval tv;
	int r;

	PROF_EVENT(xpaul_blockone_entry);
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

	if(ev->inst.state & XPAUL_STATE_OCCURED){
		fprintf(stderr, "BLOCKONE: Req %p occured\n",req);
		return 0;
	}
	switch (ev->op) {
	case XPAUL_POLL_READ:{
			FD_SET(ev->FD, &rfds);
			nb = ev->FD + 1;
			break;
		}
	case XPAUL_POLL_WRITE:{
			FD_SET(ev->FD, &wfds);
			nb = ev->FD + 1;
			break;
		}
	case XPAUL_POLL_SELECT:{
			if (ev->RFDS != NULL)
				rfds = *(ev->RFDS);
			if (ev->WFDS != NULL)
				wfds = *(ev->WFDS);
			nb = ev->NFDS;
			break;
		}
	default:
		XPAUL_EXCEPTION_RAISE(XPAUL_PROGRAM_ERROR);
	}

	/* Ajoute le descripteur du tube pour pouvoir etre interrompu par une autre commande */
	FD_SET(option, &rfds);
	if(option + 1 > nb)
		nb=option+1;

	timerclear(&tv);

	errno = 0;
	do {
		r = select(nb, &rfds, &wfds, NULL, NULL);
	}while(r==-1 && errno==EINTR);

	if (r <= 0) {
		ev->ret_val = -errno;
		return 0;
	}

	xpaul_io_check_select(uid, ev, &rfds, &wfds);
	PROF_EVENT(xpaul_blockone_exit);
	return 0;
}
#endif				// MA__LWPS

static int xpaul_io_poll(xpaul_server_t server,
			 xpaul_op_t _op,
			 xpaul_req_t req, int nb_ev, int option)
{
	xpaul_io_serverid_t uid =
	    struct_up(server, struct xpaul_io_server, server);
	xpaul_tcp_ev_t ev;
	int r;
	fd_set rfds, wfds;
	struct timeval tv, *ptv;
	
#ifdef MARCEL
	xdebugl(6, "Polling function called on LWP %d\n",
		marcel_current_vp());
#endif				// MARCEL

	timerclear(&tv);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	ptv = &tv;

	rfds = uid->polling_rfds;
	wfds = uid->polling_wfds;
	r = select(uid->polling_nb, &rfds, &wfds, NULL, ptv);

	if (tbx_unlikely(r == -1)) {
		int found = 0;
		if (errno != EBADF)
			return 0;
		/* A fd is incorrect */
		FOREACH_REQ_POLL(ev, server, inst) {
			xdebug
			    ("Checking select for IO poll (with badFD)\n");
			switch (ev->op) {
			case XPAUL_POLL_READ:
			case XPAUL_POLL_WRITE:
				break;
			case XPAUL_POLL_SELECT:{
					ev->ret_val =
					    select(ev->NFDS, ev->RFDS,
						   ev->WFDS, NULL, NULL);
					if (ev->ret_val) {
						xpaul_req_success(&ev->inst);
						found = 1;
					}
					break;
				}
			default:
				XPAUL_EXCEPTION_RAISE(XPAUL_PROGRAM_ERROR);
			}
			if (!found) {
				pm2debug
				    ("IO poll with bad fd not detected.\n"
				     "Please, fix marcel code\n");
			}
		}
	} else if (r == 0) {
		return 0;
	}

	FOREACH_REQ_POLL(ev, server, inst) {
		xpaul_io_check_select(uid, ev, &rfds, &wfds);
	}
	return 0;
}

static int xpaul_io_fast_poll(xpaul_server_t server,
			      xpaul_op_t _op,
			      xpaul_req_t req, int nb_ev, int option)
{
	xpaul_io_serverid_t uid =
	    struct_up(server, struct xpaul_io_server, server);
	xpaul_tcp_ev_t ev = struct_up(req, struct xpaul_tcp_ev, inst);

	fd_set rfds, wfds;
	unsigned nb = 0;
	struct timeval tv;
	int r;
#ifdef MARCEL
	xdebugl(6, "Fast Polling function called on LWP %d\n",
		marcel_current_vp());
#endif				// MARCEL

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

	if(ev->inst.state & XPAUL_STATE_OCCURED){
		return 0;
	}
	switch (ev->op) {
	case XPAUL_POLL_READ:{
			FD_SET(ev->FD, &rfds);
			nb = ev->FD + 1;
			break;
		}
	case XPAUL_POLL_WRITE:{
			FD_SET(ev->FD, &wfds);
			nb = ev->FD + 1;
			break;
		}
	case XPAUL_POLL_SELECT:{
			if (ev->RFDS != NULL)
				rfds = *(ev->RFDS);
			if (ev->WFDS != NULL)
				wfds = *(ev->WFDS);
			nb = ev->NFDS;
			break;
		}
	default:
		XPAUL_EXCEPTION_RAISE(XPAUL_PROGRAM_ERROR);
	}

	timerclear(&tv);

	errno = 0;

	r = select(nb, &rfds, &wfds, NULL, &tv);
	if (r <= 0) {
		ev->ret_val = -errno;
		return 0;
	}

	xpaul_io_check_select(uid, ev, &rfds, &wfds);
	return 0;
}

int xpaul_read(int fildes, void *buf, size_t nbytes)
{
	int n;
	struct xpaul_tcp_ev ev;
	struct xpaul_wait wait;
	LOG_IN();

	PROF_EVENT(xpaul_read_entry);
	/* Pour eviter de demander à un serveur qui n'est pas lancé */
	if(xpaul_io_server.server.state==XPAUL_SERVER_STATE_LAUNCHED)
	{
		
		do {
			ev.op = XPAUL_POLL_READ;
			ev.FD = fildes;
			xdebug("Reading in fd %i\n", fildes);
			xpaul_wait(&xpaul_io_server.server, &ev.inst, &wait, 0);
			LOG("IO reading fd %i", fildes);
			n = read(fildes, buf, nbytes);
		} while (n == -1 && errno == EINTR);

		PROF_EVENT(xpaul_read_exit);
		LOG_RETURN(n);
	}else
		LOG_RETURN(read(fildes, buf, nbytes));
}

#ifndef __MINGW32__
int xpaul_readv(int fildes, const struct iovec *iov, int iovcnt)
{
	struct xpaul_tcp_ev ev;
	struct xpaul_wait wait;
	LOG_IN();
	ev.op = XPAUL_POLL_READ;
	ev.FD = fildes;
	xdebug("Reading in fd %i\n", fildes);
	xpaul_wait(&xpaul_io_server.server, &ev.inst, &wait, 0);

	LOG("IO readving fd %i", fildes);
	LOG_RETURN(readv(fildes, iov, iovcnt));
}
#endif

int xpaul_write(int fildes, const void *buf, size_t nbytes)
{
	int n;
	struct xpaul_tcp_ev ev;
	struct xpaul_wait wait;
	LOG_IN();

	PROF_EVENT(xpaul_write_entry);
	if(xpaul_io_server.server.state==XPAUL_SERVER_STATE_LAUNCHED)
	{
       
		do {

			ev.op = XPAUL_POLL_WRITE;
			ev.FD = fildes;
			xdebug("Writing in fd %i\n", fildes);
			xpaul_wait(&xpaul_io_server.server, &ev.inst, &wait, 0);

			LOG("IO writing fd %i", fildes);
			n = write(fildes, buf, nbytes);

		} while (n == -1 && errno == EINTR);

		PROF_EVENT(xpaul_write_exit);
		LOG_RETURN(n);
	} else
		LOG_RETURN(write(fildes, buf, nbytes));

}

#ifndef __MINGW32__
int xpaul_writev(int fildes, const struct iovec *iov, int iovcnt)
{
	struct xpaul_tcp_ev ev;
	struct xpaul_wait wait;
	LOG_IN();
	ev.op = XPAUL_POLL_WRITE;
	ev.FD = fildes;
	xdebug("Writing in fd %i\n", fildes);
	xpaul_wait(&xpaul_io_server.server, &ev.inst, &wait, 0);
	LOG("IO writving fd %i", fildes);
	LOG_RETURN(writev(fildes, iov, iovcnt));
}
#endif

int xpaul_select(int nfds, fd_set * __restrict rfds,
		 fd_set * __restrict wfds)
{
	struct xpaul_tcp_ev ev;
	struct xpaul_wait wait;
	LOG_IN();
	if(xpaul_io_server.server.state==XPAUL_SERVER_STATE_LAUNCHED)
	{	
		PROF_EVENT(xpaul_select_entry);
		ev.op = XPAUL_POLL_SELECT;
		ev.RFDS = rfds;
		ev.WFDS = wfds;
		ev.NFDS = nfds;
		xdebug("Selecting within %i fds\n", nfds);
		xpaul_wait(&xpaul_io_server.server, &ev.inst, &wait, 0);
		PROF_EVENT(xpaul_select_exit);
		LOG_RETURN(ev.ret_val >= 0 ? ev.ret_val :
			   (errno = -ev.ret_val, -1));
	}
	return select(nfds, rfds, wfds, NULL, NULL);
}

/* To force the reading/writing of an exact number of bytes */
int xpaul_read_exactly(int fildes, void *buf, size_t nbytes)
{
	size_t to_read = nbytes, n;

	LOG_IN();
	do {
		n = xpaul_read(fildes, buf, to_read);
		if (n < 0)
			return n;
		buf += n;
		to_read -= n;
	} while (to_read);

	LOG_RETURN(nbytes);
}

#ifndef __MINGW32__
int xpaul_readv_exactly(int fildes, const struct iovec *iov, int iovcnt)
{
	return xpaul_readv(fildes, iov, iovcnt);
}
#endif

int xpaul_write_exactly(int fildes, const void *buf, size_t nbytes)
{
	size_t to_write = nbytes, n;

	LOG_IN();
	do {
		n = xpaul_write(fildes, buf, to_write);
		if (n < 0)
			return n;
		buf += n;
		to_write -= n;
	} while (to_write);

	LOG_RETURN(nbytes);
}

#ifndef __MINGW32__
int xpaul_writev_exactly(int fildes, const struct iovec *iov, int iovcnt)
{
	return xpaul_writev(fildes, iov, iovcnt);
}
#endif

/* =============== Asynchronous I/O management =============== */

int xpaul_tselect(int width, fd_set * __restrict readfds,
		  fd_set * __restrict writefds,
		  fd_set * __restrict exceptfds)
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

		timerclear(&timeout);
		res = select(width, &rfds, &wfds, &efds, &timeout);
		if (res <= 0) {
			if (res < 0 && (errno != EINTR))
				break;
#ifdef MARCEL
			marcel_yield();
#endif
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

#if 0
#ifdef MA__LWPS

/* Communication LWP main loop */
any_t xpaul_receiver()
{
	int i, j;
	requete cur_req, bak_req;
	struct sched_param param;
	xpaul_wait_t wait, tmp;

	/* waiter associé a fds[0] */
	struct xpaul_wait foo;
	INIT_LIST_HEAD(&foo.chain_wait);
	marcel_sem_init(&foo.sem, 0);

	list_add(&foo.chain_wait, &reqs.lwait[fds[0]][0]);

	ma_bind_on_processor(0);

	param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	if (!pthread_setschedparam(pthread_self(), SCHED_FIFO, &param)) {	/* Might works sometimes */
		PROF_EVENT(xpaul_io_rcv_fifo);
	}
	j = 0;

	cur_req.readfds = (fd_set *) malloc(sizeof(fd_set));
	cur_req.writefds = (fd_set *) malloc(sizeof(fd_set));
	cur_req.exceptfds = (fd_set *) malloc(sizeof(fd_set));
	FD_ZERO(cur_req.readfds);
	FD_ZERO(cur_req.writefds);
	FD_ZERO(cur_req.exceptfds);

	bak_req.readfds = (fd_set *) malloc(sizeof(fd_set));
	bak_req.writefds = (fd_set *) malloc(sizeof(fd_set));
	bak_req.exceptfds = (fd_set *) malloc(sizeof(fd_set));
	FD_ZERO(bak_req.readfds);
	FD_ZERO(bak_req.writefds);
	FD_ZERO(bak_req.exceptfds);

	FD_SET(fds[0], cur_req.readfds);
	cur_req.n = fds[0] + 1;

	FD_SET(fds[0], bak_req.readfds);
	bak_req.n = fds[0] + 1;


	PROF_EVENT1(reciever_fds, fds[0]);
	for (;;) {
		marcel_sem_P(&req_sem);

		for (i = 0; i < reqs.n; i++) {
			if (FD_ISSET(i, reqs.readfds)) {
				PROF_EVENT1(recv_new_req_read,i);
				FD_SET(i, cur_req.readfds);
				FD_SET(i, bak_req.readfds);								
			}
			if (FD_ISSET(i, reqs.writefds)) {
				PROF_EVENT1(recv_new_req_write,i);
				FD_SET(i, cur_req.writefds);
				FD_SET(i, bak_req.writefds);
			}
		}

		FD_SET(fds[0], cur_req.readfds);
		cur_req.n = (reqs.n > (fds[0] + 1)) ? reqs.n : fds[0] + 1;
		bak_req.n = cur_req.n;

		marcel_sem_V(&req_sem);	

		PROF_EVENT(xpaul_entering_select);
		res = select(cur_req.n,
			     cur_req.readfds,
			     cur_req.writefds, cur_req.exceptfds, NULL);
		PROF_EVENT(xpaul_data_received);

		j++;
		if (res < 0 && (errno != EINTR)) {
			perror("select");
			TBX_FAILURE("system call failed");
		}
		
		if (res > 0) {	/* Don't wake up a thread since we received a new query */
			if (FD_ISSET(fds[0], cur_req.readfds)) {
				PROF_EVENT(xpaul_order_received);
				read(fds[0], &i, sizeof(i));
				res--;
				FD_CLR(fds[0], res_req.readfds);
			}
		}

		if (res > 0) {
			PROF_EVENT1(thread_a_debloquer, res);
			marcel_sem_P(&recved_sem);
			res=0;
/* prépare les résultats et supprime les fd terminés */
			for (i = 0; i < cur_req.n; i++) { 
				if (FD_ISSET(i, cur_req.readfds)){
					res++;
					FD_SET(i, res_req.readfds);
					FD_CLR(i, reqs.readfds);
					FD_CLR(i, bak_req.readfds);
					tmp=list_entry(reqs.lwait[i][0].next, typeof(*wait), chain_wait);
					PROF_EVENT2(waikingup_read, &tmp->sem,i);
					marcel_sem_V(&tmp->sem);
				}
				if (FD_ISSET(i, cur_req.writefds)){
					res++;
					FD_SET(i, res_req.writefds);
					FD_CLR(i, reqs.writefds);
					FD_CLR(i, bak_req.writefds);
					tmp=list_entry(reqs.lwait[i][1].next, typeof(*wait), chain_wait);
					PROF_EVENT2(waikingup_write, &tmp->sem, i);
					marcel_sem_V(&tmp->sem);
				}
			}
			res_req.n = cur_req.n;
			marcel_sem_V(&recved_sem);
			PROF_EVENT1(threads_debloques, res);
		}

		
		/* recupere les fd encore en attente */
		for (i = 0; i < bak_req.n; i++) { 
			if (FD_ISSET(i, bak_req.readfds)
			    && !FD_ISSET(i, cur_req.readfds)) {
				cur_req.n = i + 1;
				PROF_EVENT1(recv_bak_read, i);
				FD_SET(i, cur_req.readfds);
			} else
				FD_CLR(i, cur_req.readfds);
			if (FD_ISSET(i, bak_req.writefds)
			    && !FD_ISSET(i, cur_req.writefds)) {
				PROF_EVENT1(recv_bak_write, i);
				cur_req.n = i + 1;
				FD_SET(i, cur_req.writefds);
			} else
				FD_CLR(i, cur_req.writefds);
		}
	}
}
#endif				// MA__LWPS
#endif

void xpaul_io_init(void)
{
	LOG_IN();
#ifdef MA__LWPS
	xpaul_server_start_lwp(&xpaul_io_server.server, 1);
#endif /* MA__LWPS */
	xpaul_io_server.server.stopable=1;
	xpaul_server_set_poll_settings(&xpaul_io_server.server,
				       XPAUL_POLL_AT_TIMER_SIG
				       | XPAUL_POLL_AT_IDLE, 1, -1);

#ifdef MA__LWPS
	xpaul_server_add_callback(&xpaul_io_server.server,
				  XPAUL_FUNCTYPE_BLOCK_WAITONE,
				  (xpaul_pcallback_t) {
				  .func = &xpaul_io_blockone,.speed =
				  XPAUL_CALLBACK_SLOWEST});

	xpaul_server_add_callback(&xpaul_io_server.server,
				  XPAUL_FUNCTYPE_BLOCK_GROUP,
				  (xpaul_pcallback_t) {
				  .func = &xpaul_io_syscall_group,.speed =
				  XPAUL_CALLBACK_SLOWEST});

	xpaul_server_add_callback(&xpaul_io_server.server,
				  XPAUL_FUNCTYPE_BLOCK_WAITANY,
				  (xpaul_pcallback_t) {
				  .func = &xpaul_io_block,.speed =
				  XPAUL_CALLBACK_SLOWEST});
#endif

#ifndef MARCEL_DO_NOT_GROUP_TCP
	xpaul_server_add_callback(&xpaul_io_server.server,
				  XPAUL_FUNCTYPE_POLL_GROUP,
				  (xpaul_pcallback_t) {
				  .func = &xpaul_io_group,.speed =
				  XPAUL_CALLBACK_SLOWEST});
	xpaul_server_add_callback(&xpaul_io_server.server,
				  XPAUL_FUNCTYPE_POLL_POLLANY,
				  (xpaul_pcallback_t) {
				  .func = &xpaul_io_poll,.speed =
				  XPAUL_CALLBACK_SLOWEST});
#endif
	xpaul_server_add_callback(&xpaul_io_server.server,
				  XPAUL_FUNCTYPE_POLL_POLLONE,
				  (xpaul_pcallback_t) {
				  .func = &xpaul_io_fast_poll,.speed =
				  XPAUL_CALLBACK_SLOWEST});

	xpaul_server_start(&xpaul_io_server.server);
	LOG_OUT();
}

void xpaul_io_stop()
{
	xpaul_server_stop(&xpaul_io_server.server);
}

