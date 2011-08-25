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


#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>
#include "marcel.h"


typedef enum {
	POLL_READ,
	POLL_WRITE
} io_op_t;

typedef struct {
	struct marcel_ev_req inst;
	io_op_t op;
	int fd;
} io_ev_t;


/********************************** IO server implementation ******************************/

static struct marcel_ev_server unix_io_server;


static int unix_io_pollgroup(marcel_ev_server_t server,
			     marcel_ev_op_t _op TBX_UNUSED,
			     marcel_ev_req_t req TBX_UNUSED,
			     int nb_ev TBX_UNUSED, int option TBX_UNUSED)
{
	io_ev_t *ev;
	fd_set rfds, wfds;
	int nb;
	struct timeval tv;
	int r;
 
	MARCEL_LOG("Polling function called on LWP %d\n", ma_vpnum(MA_LWP_SELF));

	nb = 0;
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

	FOREACH_REQ_POLL(ev, server, inst) {
		if ((O_NONBLOCK & fcntl(ev->fd, F_GETFL)))
			MARCEL_EV_REQ_SUCCESS(&ev->inst);
		else {
			if (ev->op == POLL_READ)
				FD_SET(ev->fd, &rfds);
			else
				FD_SET(ev->fd, &wfds);

			if (nb <= ev->fd)
				nb = ev->fd + 1;
		}
	}
	if (tbx_unlikely(0 == nb))
		return 0;
 
	tv.tv_sec = tv.tv_usec = 0;
	marcel_extlib_protect();
	r = MA_REALSYM(select)(nb, &rfds, &wfds, NULL, &tv);
	marcel_extlib_unprotect();

	if (r > 0) {
		MARCEL_LOG("Checking select for IO poll (at least one success)\n");
		FOREACH_REQ_POLL(ev, server, inst) {
			if (tbx_likely(FD_ISSET(ev->fd, &rfds) || FD_ISSET(ev->fd, &wfds)))
				MARCEL_EV_REQ_SUCCESS(&ev->inst);
		}
	}

	return 0;
}

static int unix_io_pollone(marcel_ev_server_t server TBX_UNUSED,
			   marcel_ev_op_t _op TBX_UNUSED,
			   marcel_ev_req_t req,
			   int nb_ev TBX_UNUSED, int option TBX_UNUSED)
{
	io_ev_t *ev;
	fd_set *rfds, *wfds, fds;
	unsigned nb;
	struct timeval tv;
	int r;
 
	MARCEL_LOG("Fast Polling function called on LWP %d\n", ma_vpnum(MA_LWP_SELF));
 
	ev = tbx_container_of(req, io_ev_t, inst);

	if ((O_NONBLOCK & fcntl(ev->fd, F_GETFL))) {
		MARCEL_EV_REQ_SUCCESS(&ev->inst);
		return 0;
	}
	
	FD_ZERO(&fds);
	FD_SET(ev->fd, &fds);
	nb = ev->fd + 1; 
	switch (ev->op) {
	        case POLL_READ:
			rfds = &fds;
			wfds = NULL;
			break;
 
	        case POLL_WRITE:
			rfds = NULL;
			wfds = &fds;
			break;
	
	        default:
			MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
	}
 
	errno = 0;
	tv.tv_sec = tv.tv_usec = 0;
	marcel_extlib_protect();
	r = MA_REALSYM(select)(nb, rfds, wfds, NULL, &tv);
	marcel_extlib_unprotect();

	if (r > 0) {
		MARCEL_LOG("Checking select for IO poll (at least one success)\n");
		if (tbx_likely(FD_ISSET(ev->fd, &fds)))
			MARCEL_EV_REQ_SUCCESS(&ev->inst);
	}

	return 0;
}

void ma_io_init(void)
{
	MARCEL_LOG_IN();
	marcel_ev_server_init(&unix_io_server, "Unix I/O");
	marcel_ev_server_set_poll_settings(&unix_io_server, MARCEL_EV_POLL_AT_TIMER_SIG | MARCEL_EV_POLL_AT_IDLE, 1);
	marcel_ev_server_add_callback(&unix_io_server, MARCEL_EV_FUNCTYPE_POLL_POLLONE, &unix_io_pollone);
	marcel_ev_server_add_callback(&unix_io_server, MARCEL_EV_FUNCTYPE_POLL_GROUP, &unix_io_pollgroup);
	marcel_ev_server_add_callback(&unix_io_server, MARCEL_EV_FUNCTYPE_POLL_POLLANY, &unix_io_pollgroup);
	marcel_ev_server_start(&unix_io_server);
	MARCEL_LOG_OUT();
}


#ifdef MA_DO_NOT_LAUNCH_SIGNAL_TIMER
#  define ma_block_call(retval, evfd, evop, invoke)			\
	do {								\
		struct marcel_ev_wait wait;				\
		io_ev_t ev;						\
		ev.fd = evfd;						\
		ev.op = evop;						\
		marcel_ev_wait(&unix_io_server, &ev.inst, &wait, 0);	\
		retval = invoke;					\
	} while (0);
#else
#  ifdef SA_RESTART
#    define ma_block_call(retval, evfd, evop, invoke)	\
	marcel_extlib_protect();			\
	retval = invoke;				\
	marcel_extlib_unprotect();
#  else
#    define ma_block_call(retval, evfd, evop, invoke)			\
	do {								\
		marcel_extlib_protect();				\
		retval = invoke;					\
		marcel_extlib_unprotect();				\
	} while (retval == -1 && errno == EINTR);
#  endif /** SA_RESTART **/
#endif


/********************************* File IO functions *************************************/
DEF_MARCEL(ssize_t, read, (int fildes, void *buf, size_t nbytes), (fildes, buf, nbytes),
{
	int cancel;
	ssize_t b_read;

	MARCEL_LOG_IN();

	cancel = __pmarcel_enable_asynccancel();
	ma_block_call(b_read, fildes, POLL_READ, syscall(SYS_read, fildes, buf, nbytes));
	__pmarcel_disable_asynccancel(cancel);

	MARCEL_LOG_RETURN(b_read);
})
DEF_STRONG_ALIAS(MARCEL_NAME(read), LPT_NAME(read))
versioned_symbol(libpthread, LPT_NAME(read), LIBC_NAME(read), GLIBC_2_0);
DEF___LIBC(ssize_t, read, (int fildes, void *buf, size_t nbytes), (fildes, buf, nbytes))

DEF_MARCEL(ssize_t, pread, (int fd, void *buf, size_t count, off_t pos), (fd, buf, count, pos),
{
	int cancel;
	ssize_t b_read;
	
	MARCEL_LOG_IN();

	cancel = __pmarcel_enable_asynccancel();
	ma_block_call(b_read, fd, POLL_READ, MA_REALSYM(pread)(fd, buf, count, pos));
	__pmarcel_disable_asynccancel(cancel);

	MARCEL_LOG_RETURN(b_read);
})
DEF_STRONG_ALIAS(MARCEL_NAME(pread), LPT_NAME(pread))
DEF_STRONG_ALIAS(MARCEL_NAME(pread), LPT_NAME(pread64))
versioned_symbol(libpthread, LPT_NAME(pread), LIBC_NAME(pread), GLIBC_2_0);
versioned_symbol(libpthread, LPT_NAME(pread64), LIBC_NAME(pread64), GLIBC_2_2);
DEF___LIBC(ssize_t, pread, (int fd, void *buf, size_t count, off_t pos), (fd, buf, count, pos))
DEF___LIBC(ssize_t, pread64, (int fd, void *buf, size_t count, off64_t pos), (fd, buf, count, pos))

DEF_MARCEL(ssize_t, readv, (int fildes, const struct iovec *iov, int iovcnt), (fildes, iov, iovcnt),
{
	int cancel;
	ssize_t b_read;

	MARCEL_LOG_IN();

	cancel = __pmarcel_enable_asynccancel();
	ma_block_call(b_read, fildes, POLL_READ, MA_REALSYM(readv)(fildes, iov, iovcnt));
	__pmarcel_disable_asynccancel(cancel);

	MARCEL_LOG_RETURN(b_read);
})
DEF_STRONG_ALIAS(MARCEL_NAME(readv), LPT_NAME(readv))
versioned_symbol(libpthread, LPT_NAME(readv), LIBC_NAME(readv), GLIBC_2_0);
DEF___LIBC(ssize_t, readv, (int fildes, const struct iovec *iov, int iovcnt), (fildes, iov, iovcnt))

DEF_MARCEL(ssize_t, write, (int fildes, const void *buf, size_t nbytes), (fildes, buf, nbytes),
{
	int cancel;
	ssize_t b_written;

	MARCEL_LOG_IN();

	cancel = __pmarcel_enable_asynccancel();
	ma_block_call(b_written, fildes, POLL_WRITE, syscall(SYS_write, fildes, buf, nbytes))
	__pmarcel_disable_asynccancel(cancel);

	MARCEL_LOG_RETURN(b_written);
})
DEF_STRONG_ALIAS(MARCEL_NAME(write), LPT_NAME(write))
versioned_symbol(libpthread, LPT_NAME(write), LIBC_NAME(write), GLIBC_2_0);
DEF___LIBC(ssize_t, write, (int fildes, const void *buf, size_t nbytes), (fildes, buf, nbytes))

DEF_MARCEL(ssize_t, pwrite, (int fd, const void *buf, size_t count, off_t pos), (fd, buf, count, pos),
{
	int cancel;
	ssize_t b_written;
	
	MARCEL_LOG_IN();

	cancel = __pmarcel_enable_asynccancel();
	ma_block_call(b_written, fd, POLL_WRITE, MA_REALSYM(pwrite)(fd, buf, count, pos));
	__pmarcel_disable_asynccancel(cancel);

	MARCEL_LOG_RETURN(b_written);
})
DEF_STRONG_ALIAS(MARCEL_NAME(pwrite), LPT_NAME(pwrite))
DEF_STRONG_ALIAS(MARCEL_NAME(pwrite), LPT_NAME(pwrite64))
versioned_symbol(libpthread, LPT_NAME(pwrite), LIBC_NAME(pwrite), GLIBC_2_0);
versioned_symbol(libpthread, LPT_NAME(pwrite64), LIBC_NAME(pwrite64), GLIBC_2_2);
DEF___LIBC(ssize_t, pwrite, (int fd, void *buf, size_t count, off_t pos), (fd, buf, count, pos))
DEF___LIBC(ssize_t, pwrite64, (int fd, void *buf, size_t count, off64_t pos), (fd, buf, count, pos))

DEF_MARCEL(ssize_t, writev, (int fildes, const struct iovec *iov, int iovcnt), (fildes, iov, iovcnt),
{
	int cancel;
	ssize_t b_written;

	MARCEL_LOG_IN();

	cancel = __pmarcel_enable_asynccancel();
	ma_block_call(b_written, fildes, POLL_WRITE, MA_REALSYM(writev)(fildes, iov, iovcnt));
	__pmarcel_disable_asynccancel(cancel);
	
	MARCEL_LOG_RETURN(b_written);
})
DEF_STRONG_ALIAS(MARCEL_NAME(writev), LPT_NAME(writev))
versioned_symbol(libpthread, LPT_NAME(writev), LIBC_NAME(writev), GLIBC_2_0);
DEF___LIBC(ssize_t, writev, (int fildes, const struct iovec *iov, int iovcnt), (fildes, iov, iovcnt))


/********************************* Network IO functions *************************************/
DEF_MARCEL(int, connect, (int sockfd, const struct sockaddr * serv_addr, socklen_t addrlen),
	   (sockfd, serv_addr, addrlen),
{
	int cancel;
	int ret;

	MARCEL_LOG_IN();

	cancel = __pmarcel_enable_asynccancel();
	ma_block_call(ret, sockfd, POLL_WRITE, MA_REALSYM(connect)(sockfd, serv_addr, addrlen));
	__pmarcel_disable_asynccancel(cancel);
	
	MARCEL_LOG_RETURN(ret);
})
DEF_STRONG_ALIAS(MARCEL_NAME(connect), LPT_NAME(connect))
versioned_symbol(libpthread, LPT_NAME(connect), LIBC_NAME(connect), GLIBC_2_0);
DEF___LIBC(int, connect, (int sockfd, const struct sockaddr * serv_addr, socklen_t addrlen), (sockfd, serv_addr, addrlen))

DEF_MARCEL(int, accept, (int sockfd, struct sockaddr * serv_addr, socklen_t *addrlen),
	   (sockfd, serv_addr, addrlen),
{
	int cancel;
	int ret;

	MARCEL_LOG_IN();

	cancel = __pmarcel_enable_asynccancel();
	ma_block_call(ret, sockfd, POLL_READ, MA_REALSYM(accept)(sockfd, serv_addr, addrlen));
	__pmarcel_disable_asynccancel(cancel);
	
	MARCEL_LOG_RETURN(ret);
})
DEF_STRONG_ALIAS(MARCEL_NAME(accept), LPT_NAME(accept))
versioned_symbol(libpthread, LPT_NAME(accept), LIBC_NAME(accept), GLIBC_2_0);
DEF___LIBC(int, accept, (int sockfd, struct sockaddr * serv_addr, socklen_t *addrlen), (sockfd, serv_addr, addrlen))

DEF_MARCEL(ssize_t, sendto, (int s, const void *buf, size_t len, int flags, const struct sockaddr * to, socklen_t tolen),
	   (s, buf, len, flags, to, tolen),
{
	int cancel;
	ssize_t ret;

	MARCEL_LOG_IN();

	cancel = __pmarcel_enable_asynccancel();
	ma_block_call(ret, s, POLL_WRITE, MA_REALSYM(sendto)(s, buf, len, flags, to, tolen));
	__pmarcel_disable_asynccancel(cancel);
	
	MARCEL_LOG_RETURN(ret);
})
DEF_STRONG_ALIAS(MARCEL_NAME(sendto), LPT_NAME(sendto))
versioned_symbol(libpthread, LPT_NAME(sendto), LIBC_NAME(sendto), GLIBC_2_0);
DEF___LIBC(ssize_t, sendto, (int s, const void *buf, size_t len, int flags, const struct sockaddr * to, socklen_t tolen),
	   (s, buf, len, flags, to, tolen))

DEF_MARCEL(ssize_t, send, (int s, const void *buf, size_t len, int flags), (s, buf, len, flags),
{
	return marcel_sendto(s, buf, len, flags, NULL, 0);
})
DEF_STRONG_ALIAS(MARCEL_NAME(send), LPT_NAME(send))
versioned_symbol(libpthread, LPT_NAME(send), LIBC_NAME(send), GLIBC_2_0);
DEF___LIBC(ssize_t, send, (int s, const void *buf, size_t len, int flags), (s, buf, len, flags))

DEF_MARCEL(ssize_t, sendmsg, (int socket, const struct msghdr *message, int flags), (socket, message, flags),
{
	int cancel;
	ssize_t ret;

	MARCEL_LOG_IN();

	cancel = __pmarcel_enable_asynccancel();
	ma_block_call(ret, socket, POLL_WRITE, MA_REALSYM(sendmsg)(socket, message, flags));
	__pmarcel_disable_asynccancel(cancel);
	
	MARCEL_LOG_RETURN(ret);
})
DEF_STRONG_ALIAS(MARCEL_NAME(sendmsg), LPT_NAME(sendmsg))
versioned_symbol(libpthread, LPT_NAME(sendmsg), LIBC_NAME(sendmsg), GLIBC_2_0);
DEF___LIBC(ssize_t, sendmsg, (int socket, const struct msghdr *message, int flags), (socket, message, flags))

DEF_MARCEL(ssize_t, recvfrom, (int s, void *buf, size_t len, int flags, struct sockaddr * addr, socklen_t * addrlen),
	   (s, buf, len, flags, addr, addrlen),
{
	int cancel;
	ssize_t ret;

	MARCEL_LOG_IN();

	cancel = __pmarcel_enable_asynccancel();
	ma_block_call(ret, s, POLL_READ, MA_REALSYM(recvfrom)(s, buf, len, flags, addr, addrlen));
	__pmarcel_disable_asynccancel(cancel);
	
	MARCEL_LOG_RETURN(ret);
})
DEF_STRONG_ALIAS(MARCEL_NAME(recvfrom), LPT_NAME(recvfrom))
versioned_symbol(libpthread, LPT_NAME(recvfrom), LIBC_NAME(recvfrom), GLIBC_2_0);
DEF___LIBC(ssize_t, recvfrom, (int s, void *buf, size_t len, int flags, struct sockaddr * addr, socklen_t * addrlen),
	   (s, buf, len, flags, addr, addrlen))

DEF_MARCEL(ssize_t, recv, (int s, void *buf, size_t len, int flags), (s, buf, len, flags),
{
	return marcel_recvfrom(s, buf, len, flags, NULL, NULL);
})
DEF_STRONG_ALIAS(MARCEL_NAME(recv), LPT_NAME(recv))
versioned_symbol(libpthread, LPT_NAME(recv), LIBC_NAME(recv), GLIBC_2_0);
DEF___LIBC(ssize_t, recv, (int s, void *buf, size_t len, int flags), (s, buf, len, flags))

DEF_MARCEL(ssize_t, recvmsg, (int socket, struct msghdr *message, int flags), (socket, message, flags),
{
	int cancel;
	ssize_t ret;

	MARCEL_LOG_IN();

	cancel = __pmarcel_enable_asynccancel();
	ma_block_call(ret, socket, POLL_READ, MA_REALSYM(recvmsg)(socket, message, flags));
	__pmarcel_disable_asynccancel(cancel);
	
	MARCEL_LOG_RETURN(ret);
})
DEF_STRONG_ALIAS(MARCEL_NAME(recvmsg), LPT_NAME(recvmsg))
versioned_symbol(libpthread, LPT_NAME(recvmsg), LIBC_NAME(recvmsg), GLIBC_2_0);
DEF___LIBC(ssize_t, recvmsg, (int socket, struct msghdr *message, int flags), (socket, message, flags))
