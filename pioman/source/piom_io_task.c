
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

#ifdef PIOM_ENABLE_LTASKS
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
typedef struct piom_tcp_task_ev {
	//struct piom_req inst;
	struct piom_ltask task;
	piom_sem_t sem;
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
} *piom_tcp_task_ev_t;


//#ifdef MA__LWPS
///* TODO: useless ? */
////#define MAX_REQS 50
//typedef struct {
//	int n;
//	fd_set *readfds;
//	fd_set *writefds;
//	fd_set *exceptfds;
//	struct timeval *timeout;	/* not used (TODO?) */
////	struct tbx_fast_list_head lwait[MAX_REQS][2];/* TODO: dynamique? */
//} requete;
//
//#endif	/* MA__LWPS */

/* Determine which request of a fdset is successfull */
__tbx_inline__ 
static int
piom_io_task_check_select(piom_tcp_task_ev_t ev,
			  fd_set * __restrict rfds,
			  fd_set * __restrict wfds)
{
	PIOM_LOGF("Checking select for IO poll (at least one success)\n");
	switch (ev->op) {
	case PIOM_POLL_READ:
		if (FD_ISSET(ev->FD, rfds)){
			piom_ltask_completed(&ev->task);
			piom_sem_V(&ev->sem);
			return 7;
		}
		break;
	case PIOM_POLL_WRITE:
		if (FD_ISSET(ev->FD, wfds)){
			piom_ltask_completed (&ev->task);
			piom_sem_V(&ev->sem);
			return 7;
		}
		break;
	case PIOM_POLL_SELECT:{
		/* todo */
		break;
		}
	default:
		PIOM_EXCEPTION_RAISE(PIOM_PROGRAM_ERROR);
	}
	return 0;
}

/* Poll one request */
static int 
piom_io_task_poll(void *arg)
{
	struct piom_tcp_task_ev *ev=(struct piom_tcp_task_ev *)arg;
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

	return piom_io_task_check_select(ev, &rfds, &wfds);
	return 0;
}

/* Function that reads a fd and uses PIOMan */
int 
piom_task_read(int fildes, void *buf, size_t nbytes)
{
	int n;
	struct piom_tcp_task_ev ev;
	marcel_vpset_t task_vpset = MARCEL_VPSET_FULL;

	/* If the server is not running, just perform a classical read */
	if(piom_test_activity())
	{
		do {
			ev.op = PIOM_POLL_READ;
			ev.FD = fildes;
			piom_sem_init(&ev.sem, 0);
			
			piom_ltask_create(&ev.task, 
					  piom_io_task_poll, 
					  &ev,
					  PIOM_LTASK_OPTION_REPEAT, 
					  task_vpset);
			piom_ltask_submit(&ev.task);
			PIOM_LOGF("Reading in fd %i\n", fildes);
			piom_sem_P(&ev.sem);
			piom_ltask_wait(&ev.task);
			LOG("IO reading fd %i", fildes);
			n = read(fildes, buf, nbytes);
		} while (n == -1 && errno == EINTR);
		
		return n;
	}else
		return read(fildes, buf, nbytes);
}

#ifndef __MINGW32__
int 
piom_task_readv(int fildes, const struct iovec *iov, int iovcnt)
{
	struct piom_tcp_task_ev ev;
	marcel_vpset_t task_vpset = MARCEL_VPSET_FULL;
	
        /* If the server is not running, just perform a classical read */
	if(piom_test_activity())
	{
		ev.op = PIOM_POLL_READ;
		ev.FD = fildes;

		piom_sem_init(&ev.sem, 0);
		
		piom_ltask_create(&ev.task, 
				  piom_io_task_poll, 
				  &ev,
				  PIOM_LTASK_OPTION_REPEAT, 
				  task_vpset);
		piom_ltask_submit(&ev.task);
		piom_sem_P(&ev.sem);
		piom_ltask_wait(&ev.task);

		return readv(fildes, iov, iovcnt);
		
	} else
		return readv(fildes, iov, iovcnt);
	
}
#endif	/* __MINGW32__ */

int 
piom_task_write(int fildes, const void *buf, size_t nbytes)
{
	int n;
	struct piom_tcp_task_ev ev;
	marcel_vpset_t task_vpset = MARCEL_VPSET_FULL;
	LOG_IN();

	PROF_EVENT(piom_write_entry);
	if(piom_test_activity())
	{
		do {
			ev.op = PIOM_POLL_WRITE;
			ev.FD = fildes;
			piom_sem_init(&ev.sem, 0);

			piom_ltask_create(&ev.task, 
					  piom_io_task_poll, 
					  &ev, 
					  PIOM_LTASK_OPTION_REPEAT,
					  task_vpset);
			piom_ltask_submit(&ev.task);
			LOG("IO writing fd %i", fildes);
			piom_sem_P(&ev.sem);
			piom_ltask_wait(&ev.task);
			n = write(fildes, buf, nbytes);

		} while (n == -1 && errno == EINTR);

		return n;
	} else
		return write(fildes, buf, nbytes);

}

#ifndef __MINGW32__
int 
piom_task_writev(int fildes, const struct iovec *iov, int iovcnt)
{
	struct piom_tcp_task_ev ev;
	marcel_vpset_t task_vpset = MARCEL_VPSET_FULL;

	if(piom_test_activity())
	{
	
		ev.op = PIOM_POLL_WRITE;
		ev.FD = fildes;
		piom_sem_init(&ev.sem, 0);
		
		piom_ltask_create(&ev.task, 
				  piom_io_task_poll, 
				  &ev, 
				  PIOM_LTASK_OPTION_REPEAT,
				  task_vpset);
		piom_ltask_submit(&ev.task);
		LOG("IO writing fd %i", fildes);
		piom_sem_P(&ev.sem);
		piom_ltask_wait(&ev.task);
		return writev(fildes, iov, iovcnt);
	} else 
		return writev(fildes, iov, iovcnt);
}
#endif

int 
piom_task_select(int nfds, fd_set * __restrict rfds,
		 fd_set * __restrict wfds)
{
	struct piom_tcp_task_ev ev;
	marcel_vpset_t task_vpset = MARCEL_VPSET_FULL;

	if(piom_test_activity())
	{	
		ev.op = PIOM_POLL_SELECT;
		ev.RFDS = rfds;
		ev.WFDS = wfds;
		ev.NFDS = nfds;

		piom_sem_init(&ev.sem, 0);
		
		piom_ltask_create(&ev.task, 
				  piom_io_task_poll, 
				  &ev, 
				  PIOM_LTASK_OPTION_REPEAT,
				  task_vpset);
		piom_ltask_submit(&ev.task);
		
		return ev.ret_val >= 0 ? ev.ret_val :
			   (errno = -ev.ret_val, -1);
	} 
	return select(nfds, rfds, wfds, NULL, NULL);
}

/* To force the reading/writing of an exact number of bytes */
int 
piom_task_read_exactly(int fildes, void *buf, size_t nbytes)
{
	size_t to_read = nbytes, n;
	
	do {
		n = piom_task_read(fildes, buf, to_read);
		if (n < 0)
			return n;
		buf += n;
		to_read -= n;
	} while (to_read);

	return nbytes;
}

#ifndef __MINGW32__
int 
piom_task_readv_exactly(int fildes, const struct iovec *iov, int iovcnt)
{
	return piom_task_readv(fildes, iov, iovcnt);
}
#endif

int 
piom_task_write_exactly(int fildes, const void *buf, size_t nbytes)
{
	size_t to_write = nbytes, n;

	do {
		n = piom_task_write(fildes, buf, to_write);
		if (n < 0)
			return n;
		buf += n;
		to_write -= n;
	} while (to_write);

	return nbytes;
}

#ifndef __MINGW32__
int 
piom_task_writev_exactly(int fildes, const struct iovec *iov, int iovcnt)
{
	return piom_task_writev(fildes, iov, iovcnt);
}
#endif

/* Initialize the server and specifies the callbacks */
void piom_io_task_init(void)
{
	piom_init_ltasks();
}

#endif	/* PIOM_ENABLE_LTASKS */
