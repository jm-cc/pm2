
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

/* TODO: use poll instead of select */

#include "pioman.h"

#ifdef MARCEL
#include "marcel.h"
#include "marcel_timer.h"
#endif	/* MARCEL */

#include "piom.h"
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

/* implementation-specific server structure
 * contains a server + usefull variables (fd_sets, etc.)
 */
typedef struct piom_io_server {
	struct piom_server server;
	fd_set polling_rfds, polling_wfds;
	unsigned polling_nb;
	fd_set syscall_rfds, syscall_wfds;
	unsigned syscall_nb;
} *piom_io_serverid_t;

typedef enum {
	PIOM_POLL_READ,
	PIOM_POLL_WRITE,
	PIOM_POLL_SELECT
} piom_poll_op_t;

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

/* implementation-specific request */
typedef struct piom_tcp_ev {
	struct piom_req inst;
	piom_poll_op_t op;
	int ret_val;
	/* file descriptor(s) to poll */
	union {
		int fd;
		struct {
			fd_set *rfds;
			fd_set *wfds;
			int nfds;
		} MA_GCC_NAME;
	} MA_GCC_NAME;
} *piom_tcp_ev_t;

/* initialize the server */
static struct piom_io_server piom_io_server;

#ifdef MA__LWPS
/* TODO: useless ? */
//#define MAX_REQS 50
typedef struct {
	int n;
	fd_set *readfds;
	fd_set *writefds;
	fd_set *exceptfds;
	struct timeval *timeout;	/* not used (TODO?) */
//	struct tbx_fast_list_head lwait[MAX_REQS][2];/* TODO: dynamique? */
} requete;

#endif	/* MA__LWPS */

/* Add a request to a group of requests
 * Used to poll all the requests with *one* select/poll
 */
static int 
piom_io_group(piom_server_t server,
	      piom_op_t _op,
	      piom_req_t req, int nb_ev, int option)
{
	piom_io_serverid_t uid =
	    struct_up(server, struct piom_io_server, server);
	piom_tcp_ev_t ev;

	PIOM_LOGF("Grouping IO poll\n");
	uid->polling_nb = 0;
	FD_ZERO(&uid->polling_rfds);
	FD_ZERO(&uid->polling_wfds);

	FOREACH_REQ_REGISTERED(ev, server, inst) {
		PIOM_WARN_ON(ev->inst.state & PIOM_STATE_OCCURED);
		switch (ev->op) {
		case PIOM_POLL_READ:{
			FD_SET(ev->FD, &uid->polling_rfds);
			uid->polling_nb = tbx_max(uid->polling_nb, ev->FD + 1);
			break;
		}
		case PIOM_POLL_WRITE:{
			FD_SET(ev->FD, &uid->polling_wfds);
			uid->polling_nb = tbx_max(uid->polling_nb, ev->FD + 1);
			break;
		}
		case PIOM_POLL_SELECT:{
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
			PIOM_EXCEPTION_RAISE(PIOM_PROGRAM_ERROR);
		}
	}
	return 0;
}

/* TODO: use PIOM_BLOCKING_CALLS instead */
#ifdef MA__LWPS

/* Add a request to a group of requests */
static int 
piom_io_syscall_group(piom_server_t server,
		      piom_op_t _op,
		      piom_req_t req, int nb_ev, int option)
{
	piom_io_serverid_t uid =
	    struct_up(server, struct piom_io_server, server);
	piom_tcp_ev_t ev;

	PIOM_LOGF("Grouping IO syscalls\n");
	uid->syscall_nb = 0;
	FD_ZERO(&uid->syscall_rfds);
	FD_ZERO(&uid->syscall_wfds);
	
	/* empty the list of grouped requests for polling */
	uid->polling_nb=0;

	FOREACH_REQ_BLOCKING(ev, server, inst) {
		switch (ev->op) {
		case PIOM_POLL_READ:{
				FD_SET(ev->FD, &uid->syscall_rfds);
				uid->syscall_nb = tbx_max(uid->syscall_nb, ev->FD + 1);
				break;
			}
		case PIOM_POLL_WRITE:{
				FD_SET(ev->FD, &uid->syscall_wfds);
				uid->syscall_nb = tbx_max(uid->syscall_nb, ev->FD + 1);
				break;
			}
		case PIOM_POLL_SELECT:{
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
			PIOM_EXCEPTION_RAISE(PIOM_PROGRAM_ERROR);
		}

		if(!(ev->inst.state & PIOM_STATE_EXPORTED)) {
			ev->inst.state|=PIOM_STATE_EXPORTED;
		}
	}
	return 0;
}
#endif

/* Determine which request of a fdset is successfull */
__tbx_inline__ 
static void 
piom_io_check_select(piom_io_serverid_t uid,
		     piom_tcp_ev_t ev,
		     fd_set * __restrict rfds,
		     fd_set * __restrict wfds)
{
	PIOM_LOGF("Checking select for IO poll (at least one success)\n");

	switch (ev->op) {
	case PIOM_POLL_READ:
		if (FD_ISSET(ev->FD, rfds)){
			piom_req_success(&ev->inst);
		}
		break;
	case PIOM_POLL_WRITE:
		if (FD_ISSET(ev->FD, wfds)){
			piom_req_success(&ev->inst);
		}
		break;
	case PIOM_POLL_SELECT:{
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
				piom_req_success(&ev->inst);
			}
			break;
		}
	default:
		PIOM_EXCEPTION_RAISE(PIOM_PROGRAM_ERROR);
	}
}

#ifdef MA__LWPS


/* Block until a request succeeds
 * option contains a fd to watch (in order to add a fd to the fdset)
 */
static int 
piom_io_block(piom_server_t server,
	      piom_op_t _op,
	      piom_req_t req, int nb_ev, int option)
{
	piom_io_serverid_t uid =
	    struct_up(server, struct piom_io_server, server);
	piom_tcp_ev_t ev;

	int r;
	fd_set rfds, wfds;
	struct timeval tv, *ptv;
	PROF_EVENT1(piom_io_block_entry,ev);
	
	PIOM_LOGF("Syscall function called on LWP %d\n",
		marcel_current_vp());

	timerclear(&tv);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	ptv = &tv;

	/* Add the pipe's fd in order to be interruptible */
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
			PIOM_LOGF
			    ("Checking select for IO syscall (with badFD)\n");
			switch (ev->op) {
			case PIOM_POLL_READ:
			case PIOM_POLL_WRITE:
				break;
			case PIOM_POLL_SELECT:{
					ev->ret_val =
					    select(ev->NFDS, ev->RFDS,
						   ev->WFDS, NULL, NULL);
					if (ev->ret_val) {
						piom_req_success(&ev->
								  inst);
						found = 1;
					}
					break;
				}
			default:
				PIOM_EXCEPTION_RAISE(PIOM_PROGRAM_ERROR);
			}
			if (!found) {
				PIOM_DISPF
				    ("IO syscall with bad fd not detected.\n"
				     "Please, fix marcel code\n");
			}
		}
	} else if (r == 0) {
		return 0;
	}

	/* TODO: XXX: lock */
	FOREACH_REQ_BLOCKING(ev, server, inst) {
		piom_io_check_select(uid, ev, &rfds, &wfds);
	}
	PROF_EVENT(piom_blockany_exit);
	return 0;
}


/* Block until req succeeds
 * option contains a fd to watch (in order to add a fd to the fdset)
 */
static int 
piom_io_blockone(piom_server_t server,
		 piom_op_t _op,
		 piom_req_t req, int nb_ev, int option)
{

	piom_io_serverid_t uid =
	    struct_up(server, struct piom_io_server, server);
	piom_tcp_ev_t ev = struct_up(req, struct piom_tcp_ev, inst);

	fd_set rfds, wfds;
	unsigned nb = 0;
	struct timeval tv;
	int r;

	PROF_EVENT(piom_blockone_entry);
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

	PIOM_BUG_ON(ev->inst.state & PIOM_STATE_OCCURED);

	switch (ev->op) {
	case PIOM_POLL_READ:{
			FD_SET(ev->FD, &rfds);
			nb = ev->FD + 1;
			break;
		}
	case PIOM_POLL_WRITE:{
			FD_SET(ev->FD, &wfds);
			nb = ev->FD + 1;
			break;
		}
	case PIOM_POLL_SELECT:{
			if (ev->RFDS != NULL)
				rfds = *(ev->RFDS);
			if (ev->WFDS != NULL)
				wfds = *(ev->WFDS);
			nb = ev->NFDS;
			break;
		}
	default:
		PIOM_EXCEPTION_RAISE(PIOM_PROGRAM_ERROR);
	}

	/* Add the pipe's fd in order to be interruptible */
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

	piom_io_check_select(uid, ev, &rfds, &wfds);
	PROF_EVENT(piom_blockone_exit);
	return 0;
}
#endif	/* MA__LWPS */

/* Poll a group of requests */
static int 
piom_io_poll(piom_server_t server,
	     piom_op_t _op,
	     piom_req_t req, int nb_ev, int option)
{
	piom_io_serverid_t uid =
	    struct_up(server, struct piom_io_server, server);
	piom_tcp_ev_t ev;
	int r;
	fd_set rfds, wfds;
	struct timeval tv, *ptv;
	
#ifdef MARCEL
	PIOM_LOGF("Polling function called on LWP %d\n",
		marcel_current_vp());
#endif	/* MARCEL */

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
			PIOM_LOGF
			    ("Checking select for IO poll (with badFD)\n");
			switch (ev->op) {
			case PIOM_POLL_READ:
			case PIOM_POLL_WRITE:
				break;
			case PIOM_POLL_SELECT:{
					ev->ret_val =
					    select(ev->NFDS, ev->RFDS,
						   ev->WFDS, NULL, NULL);
					if (ev->ret_val) {
						piom_req_success(&ev->inst);
						found = 1;
					}
					break;
				}
			default:
				PIOM_EXCEPTION_RAISE(PIOM_PROGRAM_ERROR);
			}
			if (!found) {
			    PIOM_DISPF
				    ("IO poll with bad fd not detected.\n"
				     "Please, fix marcel code\n");
			}
		}
	} else if (r == 0) {
		return 0;
	}

	FOREACH_REQ_POLL(ev, server, inst) {
		piom_io_check_select(uid, ev, &rfds, &wfds);
	}
	return 0;
}

/* Poll one request */
static int 
piom_io_fast_poll(piom_server_t server,
		  piom_op_t _op,
		  piom_req_t req, int nb_ev, int option)
{
	piom_io_serverid_t uid =
	    struct_up(server, struct piom_io_server, server);
	piom_tcp_ev_t ev = struct_up(req, struct piom_tcp_ev, inst);

	fd_set rfds, wfds;
	unsigned nb = 0;
	struct timeval tv;
	int r;
#ifdef MARCEL
	PIOM_LOGF("Fast Polling function called on LWP %d\n",
		marcel_current_vp());
#endif	/* MARCEL */

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

	if(ev->inst.state & PIOM_STATE_OCCURED){
		return 0;
	}
	switch (ev->op) {
	case PIOM_POLL_READ:{
			FD_SET(ev->FD, &rfds);
			nb = ev->FD + 1;
			break;
		}
	case PIOM_POLL_WRITE:{
			FD_SET(ev->FD, &wfds);
			nb = ev->FD + 1;
			break;
		}
	case PIOM_POLL_SELECT:{
			if (ev->RFDS != NULL)
				rfds = *(ev->RFDS);
			if (ev->WFDS != NULL)
				wfds = *(ev->WFDS);
			nb = ev->NFDS;
			break;
		}
	default:
		PIOM_EXCEPTION_RAISE(PIOM_PROGRAM_ERROR);
	}

	timerclear(&tv);

	errno = 0;

	r = select(nb, &rfds, &wfds, NULL, &tv);
	if (r <= 0) {
		ev->ret_val = -errno;
		return 0;
	}

	piom_io_check_select(uid, ev, &rfds, &wfds);
	return 0;
}

#ifdef PIOM_DISABLE_LTASKS

/* Function that reads a fd and uses PIOMan */
int 
piom_read(int fildes, void *buf, size_t nbytes)
{
	int n;
	struct piom_tcp_ev ev;
	struct piom_wait wait;
	PIOM_LOG_IN();

	PROF_EVENT(piom_read_entry);
	/* If the server is not running, just perform a classical read */
	if(piom_io_server.server.state==PIOM_SERVER_STATE_LAUNCHED)
	{
		
		do {
			ev.op = PIOM_POLL_READ;
			ev.FD = fildes;
			PIOM_LOGF("Reading in fd %i\n", fildes);
			piom_wait(&piom_io_server.server, &ev.inst, &wait, 0);
			n = read(fildes, buf, nbytes);
		} while (n == -1 && errno == EINTR);

		PROF_EVENT(piom_read_exit);
		PIOM_LOG_RETURN(n);
	}else
		PIOM_LOG_RETURN(read(fildes, buf, nbytes));
}

#ifndef __MINGW32__
int 
piom_readv(int fildes, const struct iovec *iov, int iovcnt)
{
	struct piom_tcp_ev ev;
	struct piom_wait wait;
	PIOM_LOG_IN();
	ev.op = PIOM_POLL_READ;
	ev.FD = fildes;
	PIOM_LOGF("Reading in fd %i\n", fildes);
	piom_wait(&piom_io_server.server, &ev.inst, &wait, 0);
	PIOM_LOG_RETURN(readv(fildes, iov, iovcnt));
}
#endif

int 
piom_write(int fildes, const void *buf, size_t nbytes)
{
	int n;
	struct piom_tcp_ev ev;
	struct piom_wait wait;
	PIOM_LOG_IN();

	PROF_EVENT(piom_write_entry);
	if(piom_io_server.server.state==PIOM_SERVER_STATE_LAUNCHED)
	{
       
		do {

			ev.op = PIOM_POLL_WRITE;
			ev.FD = fildes;
			PIOM_LOGF("Writing in fd %i\n", fildes);
			piom_wait(&piom_io_server.server, &ev.inst, &wait, 0);

			n = write(fildes, buf, nbytes);

		} while (n == -1 && errno == EINTR);

		PROF_EVENT(piom_write_exit);
		PIOM_LOG_RETURN(n);
	} else
		PIOM_LOG_RETURN(write(fildes, buf, nbytes));
}

#ifndef __MINGW32__
int 
piom_writev(int fildes, const struct iovec *iov, int iovcnt)
{
	struct piom_tcp_ev ev;
	struct piom_wait wait;
	PIOM_LOG_IN();
	ev.op = PIOM_POLL_WRITE;
	ev.FD = fildes;
	PIOM_LOGF("Writing in fd %i\n", fildes);
	piom_wait(&piom_io_server.server, &ev.inst, &wait, 0);
	PIOM_LOG_RETURN(writev(fildes, iov, iovcnt));
}
#endif

int 
piom_select(int nfds, fd_set * __restrict rfds,
		 fd_set * __restrict wfds)
{
	struct piom_tcp_ev ev;
	struct piom_wait wait;
	PIOM_LOG_IN();
	if(piom_io_server.server.state==PIOM_SERVER_STATE_LAUNCHED)
	{	
		PROF_EVENT(piom_select_entry);
		ev.op = PIOM_POLL_SELECT;
		ev.RFDS = rfds;
		ev.WFDS = wfds;
		ev.NFDS = nfds;
		PIOM_LOGF("Selecting within %i fds\n", nfds);
		piom_wait(&piom_io_server.server, &ev.inst, &wait, 0);
		PROF_EVENT(piom_select_exit);
		PIOM_LOG_RETURN(ev.ret_val >= 0 ? ev.ret_val :
			   (errno = -ev.ret_val, -1));
	}
	return select(nfds, rfds, wfds, NULL, NULL);
}

/* To force the reading/writing of an exact number of bytes */
int 
piom_read_exactly(int fildes, void *buf, size_t nbytes)
{
	size_t to_read = nbytes, n;

	PIOM_LOG_IN();
	do {
		n = piom_read(fildes, buf, to_read);
		if (n < 0)
			return n;
		buf += n;
		to_read -= n;
	} while (to_read);

	PIOM_LOG_RETURN(nbytes);
}

#ifndef __MINGW32__
int 
piom_readv_exactly(int fildes, const struct iovec *iov, int iovcnt)
{
	return piom_readv(fildes, iov, iovcnt);
}
#endif

int 
piom_write_exactly(int fildes, const void *buf, size_t nbytes)
{
	size_t to_write = nbytes, n;

	PIOM_LOG_IN();
	do {
		n = piom_write(fildes, buf, to_write);
		if (n < 0)
			return n;
		buf += n;
		to_write -= n;
	} while (to_write);

	PIOM_LOG_RETURN(nbytes);
}

#ifndef __MINGW32__
int 
piom_writev_exactly(int fildes, const struct iovec *iov, int iovcnt)
{
	return piom_writev(fildes, iov, iovcnt);
}
#endif
#endif /* PIOM_DISABLE_LTASKS */
/* =============== Asynchronous I/O management =============== */

int piom_tselect(int width, fd_set * __restrict readfds,
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

/* Initialize the server and specifies the callbacks */
void piom_io_init(void)
{
	PIOM_LOG_IN();
	/* unregister Marcel IO so that *we* manage IO requests */
	marcel_unregister_scheduling_hook(marcel_check_polling, MARCEL_SCHEDULING_POINT_TIMER);
	marcel_unregister_scheduling_hook(marcel_check_polling, MARCEL_SCHEDULING_POINT_YIELD);
	marcel_unregister_scheduling_hook(marcel_check_polling, MARCEL_SCHEDULING_POINT_LIB_ENTRY);
	marcel_unregister_scheduling_hook(marcel_check_polling, MARCEL_SCHEDULING_POINT_IDLE);
	marcel_unregister_scheduling_hook(marcel_check_polling, MARCEL_SCHEDULING_POINT_CTX_SWITCH);

	piom_server_init(&piom_io_server.server, "Unix TCP I/O");
#ifdef MA__LWPS
//	piom_server_start_lwp(&piom_io_server.server, 1);
#endif /* MA__LWPS */
	piom_io_server.server.stopable=1;
	/* The server can be called on CPU idleness and timer interrupt */
	piom_server_set_poll_settings(&piom_io_server.server,
				       PIOM_POLL_AT_TIMER_SIG
				       | PIOM_POLL_AT_IDLE, 1, -1);

#ifdef MA__LWPS
	piom_server_add_callback(&piom_io_server.server,
				  PIOM_FUNCTYPE_BLOCK_WAITONE,
				  (piom_pcallback_t) {
				  .func = &piom_io_blockone,.speed =
				  PIOM_CALLBACK_SLOWEST});

	piom_server_add_callback(&piom_io_server.server,
				  PIOM_FUNCTYPE_BLOCK_GROUP,
				  (piom_pcallback_t) {
				  .func = &piom_io_syscall_group,.speed =
				  PIOM_CALLBACK_SLOWEST});

	piom_server_add_callback(&piom_io_server.server,
				  PIOM_FUNCTYPE_BLOCK_WAITANY,
				  (piom_pcallback_t) {
				  .func = &piom_io_block,.speed =
				  PIOM_CALLBACK_SLOWEST});
#endif	/* MA__LWPS */

#ifndef MARCEL_DO_NOT_GROUP_TCP
	piom_server_add_callback(&piom_io_server.server,
				  PIOM_FUNCTYPE_POLL_GROUP,
				  (piom_pcallback_t) {
				  .func = &piom_io_group,.speed =
				  PIOM_CALLBACK_SLOWEST});
	piom_server_add_callback(&piom_io_server.server,
				  PIOM_FUNCTYPE_POLL_POLLANY,
				  (piom_pcallback_t) {
				  .func = &piom_io_poll,.speed =
				  PIOM_CALLBACK_SLOWEST});
#endif	/* MARCEL_DO_NOT_GROUP_TCP */
	piom_server_add_callback(&piom_io_server.server,
				  PIOM_FUNCTYPE_POLL_POLLONE,
				  (piom_pcallback_t) {
				  .func = &piom_io_fast_poll,.speed =
				  PIOM_CALLBACK_SLOWEST});

	piom_server_start(&piom_io_server.server);
	PIOM_LOG_OUT();
}

void 
piom_io_stop()
{
	piom_server_stop(&piom_io_server.server);
}

