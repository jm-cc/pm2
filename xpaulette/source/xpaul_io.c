
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

#define MA_FILE_DEBUG xpaul_io
#include "xpaul.h"

#ifdef MARCEL
#include "marcel.h"
#include "marcel_timer.h"
#endif // MARCEL

#include "xpaul_ev_serv.h"
#include <sys/time.h>
#include <sys/types.h>
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

typedef struct xpaul_io_server {
	struct xpaul_server server;
	fd_set rfds, wfds;
	unsigned nb;
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
    .server=XPAUL_SERVER_INIT(xpaul_io_server.server, "Unix TCP I/O"),
};

#ifdef MA__LWPS

typedef struct{
  int n;
  fd_set *readfds; 
  fd_set *writefds;
  fd_set *exceptfds; 
  struct timeval *timeout; // n'est pas pris en compte (TODO?)
}requete;

static int fds[2];
static requete reqs; // liste chainée?
static requete res_req;
static volatile int res;
#ifdef MARCEL
static marcel_sem_t recved_sem;
static marcel_sem_t req_sem; // pour acceder à reqs
#endif // MARCEL
static int vp_nb;

#endif

static int xpaul_io_group(xpaul_server_t server, 
			  xpaul_op_t _op,
			  xpaul_req_t req, 
			  int nb_ev, int option)
{
        PROF_EVENT(xpaul_io_group_entry);
	xpaul_io_serverid_t uid=struct_up(server, struct xpaul_io_server, server);
	xpaul_tcp_ev_t ev;

	xdebug("Grouping IO poll\n");
	uid->nb = 0;
	FD_ZERO(&uid->rfds);
	FD_ZERO(&uid->wfds);
	
	FOREACH_REQ_POLL(ev, server, inst) {
		switch(ev->op) {
		case XPAUL_POLL_READ : {
			FD_SET(ev->FD, &uid->rfds);
			uid->nb = tbx_max(uid->nb, ev->FD+1);
			break;
		}
		case XPAUL_POLL_WRITE : {
			FD_SET(ev->FD, &uid->wfds);
			uid->nb = tbx_max(uid->nb, ev->FD+1);
			break;
		}
		case XPAUL_POLL_SELECT : {
			unsigned i;
			if(ev->RFDS != NULL) {
				for(i=0; i<ev->NFDS; i++)
					if(FD_ISSET(i, ev->RFDS))
						FD_SET(i, &uid->rfds);
			}
			if(ev->WFDS != NULL) {
				for(i=0; i<ev->NFDS; i++)
					if(FD_ISSET(i, ev->WFDS))
						FD_SET(i, &uid->wfds);
			}
			uid->nb = tbx_max(uid->nb, ev->NFDS);
			break;
		}
		default :
			XPAUL_EXCEPTION_RAISE(XPAUL_PROGRAM_ERROR);
		}
	}
	PROF_EVENT(xpaul_io_group_exit);
	return 0;
}

inline static void xpaul_io_check_select(xpaul_io_serverid_t uid, xpaul_tcp_ev_t ev,
					fd_set * __restrict rfds,
					fd_set * __restrict wfds)
{
	xdebug("Checking select for IO poll (at least one success)\n");
	switch(ev->op) {
	case XPAUL_POLL_READ :
		if(FD_ISSET(ev->FD, rfds))
			XPAUL_REQ_SUCCESS(&ev->inst);
		break;
	case XPAUL_POLL_WRITE :
		if(FD_ISSET(ev->FD, wfds))
			XPAUL_REQ_SUCCESS(&ev->inst);
		break;
	case XPAUL_POLL_SELECT : {
		unsigned i;
		unsigned zeroed=0;
		ev->ret_val=0;
		if(ev->RFDS != NULL) {
			for(i=0; i<ev->NFDS; i++)
				if(FD_ISSET(i, ev->RFDS) && FD_ISSET(i, rfds)) {
					if (!zeroed) {
						FD_ZERO(ev->RFDS);
						if(ev->WFDS != NULL)
							FD_ZERO(ev->WFDS);
						zeroed=1;
					}
					FD_SET(i, ev->RFDS);
					ev->ret_val++;
				}
		}
		if(ev->WFDS != NULL) {
			for(i=0; i<ev->NFDS; i++)
				if(FD_ISSET(i, ev->WFDS) && FD_ISSET(i, wfds)) {
					if (!zeroed) {
						FD_ZERO(ev->WFDS);
						if(ev->RFDS != NULL)
							FD_ZERO(ev->RFDS);
					}
					FD_SET(i, ev->WFDS);
					ev->ret_val++;
				}
		}
		if (ev->ret_val) {
		  XPAUL_REQ_SUCCESS(&ev->inst);
		}
		break;
	}
	default :
		XPAUL_EXCEPTION_RAISE(XPAUL_PROGRAM_ERROR);
	}
}

#ifdef MA__LWPS
/* lance un select sur un autre LWP pour une meilleur reactivite */
int xpaul_ask_for_select(int n, fd_set *readfds, fd_set *writefds,
		     fd_set *exceptfds, struct timeval *timeout)
{
  PROF_IN();
  int i,ok=0;
  int res=0;

  struct marcel_sched_param param,backup;
  param.__sched_priority=MA_MAX_RT_PRIO;
  marcel_sched_getparam(marcel_self(), &backup);
  marcel_sched_setparam(marcel_self(), &param);


  marcel_sem_P(&req_sem);
  for(i=0;i<n;i++)
    {
      if(readfds)
	if(FD_ISSET(i,readfds))
	  FD_SET(i,reqs.readfds);
      if(writefds)
	if(FD_ISSET(i,writefds))
	  FD_SET(i,reqs.writefds);
      if(exceptfds)
	if(FD_ISSET(i,exceptfds))
	  FD_SET(i,reqs.exceptfds);
    }
  reqs.n=(reqs.n>n)?reqs.n:n;
  marcel_sem_V(&req_sem);

  // TODO : remplacer par un signal (quand cela sera possible)
  i=write(fds[1], &i,sizeof(i)); // on envoi n'importe quoi pour reveiller
  sched_yield();
    do {
      marcel_sem_P(&recved_sem);

      for(i=0;i<n;i++)
	{ 
	  // Verifie si c'est bien notre requete qui est finie.
	  // TODO : reveiller directement le bon thread
	  if(readfds)
	    if(FD_ISSET(i,readfds) &&FD_ISSET(i,res_req.readfds))
	      {
		ok=1;
		break;
	      }
	  if(writefds)
	    if(FD_ISSET(i,writefds) &&FD_ISSET(i,res_req.writefds))
	      {
		ok=1;
		break;
	      }
	  if(exceptfds)
	    if(FD_ISSET(i,exceptfds) &&FD_ISSET(i,res_req.exceptfds))
	      {
		ok=1;
		break;
	      }
	}
      for(i=0;i<n;i++)
	{ 
	  if(readfds)
	    if(FD_ISSET(i,res_req.readfds))
	      {
		FD_SET(i,readfds);
		FD_CLR(i,res_req.readfds);
		res++;
	      }
	  if(writefds)
	    if(FD_ISSET(i,res_req.writefds))
	      {
		FD_SET(i,writefds);
		FD_CLR(i,res_req.writefds);
		res++;
	      }
	  if(exceptfds)
	    if(FD_ISSET(i,res_req.exceptfds))
	      {
		FD_SET(i,exceptfds);
		FD_CLR(i,res_req.exceptfds);
		res++;
	      }
	}
      for(i=0;i<res_req.n;i++) 
	if(FD_ISSET(i,res_req.readfds) || FD_ISSET(i,res_req.writefds) || FD_ISSET(i,res_req.exceptfds))
	  {
	    marcel_sem_V(&recved_sem); // d'autres threads doivent etre réveillés
	  }

    }while(!ok);

    marcel_sched_setparam(marcel_self(), &backup);
    PROF_OUT();

    return res;
}

static int xpaul_io_block(xpaul_server_t server, 
			  xpaul_op_t _op,
			  xpaul_req_t req, 
			  int nb_ev, int option)
{ // a preciser
        PROF_EVENT(xpaul_io_block_entry);
	xpaul_io_serverid_t uid=struct_up(server, struct xpaul_io_server, server);
	xpaul_tcp_ev_t ev=struct_up(req, struct xpaul_tcp_ev, inst);

	fd_set rfds, wfds;
	unsigned nb = 0;
	struct timeval tv;
	int r;

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

	switch(ev->op) 
	  {
	  case XPAUL_POLL_READ : {
	    FD_SET(ev->FD, &rfds);
	    nb = ev->FD + 1;
	    break;
	  }
	  case XPAUL_POLL_WRITE : {
	    FD_SET(ev->FD, &wfds);
	    nb = ev->FD + 1;
	    break;
	  }
	  case XPAUL_POLL_SELECT : {
	    if(ev->RFDS != NULL)
	      rfds = *(ev->RFDS);
	    if(ev->WFDS != NULL)
	      wfds = *(ev->WFDS);
	    nb = ev->NFDS;
	    break;
	  }
	  default :
	    XPAUL_EXCEPTION_RAISE(XPAUL_PROGRAM_ERROR);
	  }

	timerclear(&tv);
	
	errno = 0;
	r = xpaul_ask_for_select(nb, &rfds, &wfds, NULL, &tv);

	if(r <= 0) {
		ev->ret_val=-errno;
		PROF_EVENT(xpaul_io_block_exit2);
		return 0;
	}

	xpaul_io_check_select(uid, ev, &rfds, &wfds);
	PROF_EVENT(xpaul_io_block_exit);
	return 0;
}
#endif // MA__LWPS

static int xpaul_io_poll(xpaul_server_t server, 
			xpaul_op_t _op,
			xpaul_req_t req, 
			int nb_ev, int option)
{
        PROF_EVENT(xpaul_io_poll_entry);
	xpaul_io_serverid_t uid=struct_up(server, struct xpaul_io_server, server);
	xpaul_tcp_ev_t ev;
	int r;
	fd_set rfds, wfds;
	struct timeval tv, *ptv;
  
#ifdef MARCEL
	// Trop de messages avec les activations
	xdebugl(6, "Polling function called on LWP %d\n",
		marcel_current_vp());
#endif // MARCEL

	timerclear(&tv);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	ptv = &tv;

	rfds = uid->rfds;
	wfds = uid->wfds;
	r = select(uid->nb, &rfds, &wfds, NULL, ptv);

	if (tbx_unlikely(r==-1)) {
		int found=0;
		if (errno != EBADF)
			return 0;
		/* L'un des fd est invalide */
		FOREACH_REQ_POLL(ev, server, inst) {
			xdebug("Checking select for IO poll (with badFD)\n");
			switch(ev->op) {
			case XPAUL_POLL_READ :
			case XPAUL_POLL_WRITE :
				break;
			case XPAUL_POLL_SELECT : {
				ev->ret_val=select(ev->NFDS, ev->RFDS,
						   ev->WFDS, NULL, NULL);
				//						   ev->WFDS, NULL, ptv);
				if (ev->ret_val) {
					XPAUL_REQ_SUCCESS(&ev->inst);
					found=1;
				}
				break;
			}
			default :
				XPAUL_EXCEPTION_RAISE(XPAUL_PROGRAM_ERROR);
			}
			if (!found) {
				pm2debug("IO poll with bad fd not detected.\n"
						"Please, fix marcel code\n");
			}
		}
	} else if (r == 0){
	  return 0;
	}

	FOREACH_REQ_POLL(ev, server, inst) {
		xpaul_io_check_select(uid, ev, &rfds, &wfds);
	}
	return 0;
}

static int xpaul_io_fast_poll(xpaul_server_t server, 
			      xpaul_op_t _op,
			      xpaul_req_t req, 
			      int nb_ev, int option)
{
	xpaul_io_serverid_t uid=struct_up(server, struct xpaul_io_server, server);
	xpaul_tcp_ev_t ev=struct_up(req, struct xpaul_tcp_ev, inst);

	fd_set rfds, wfds;
	unsigned nb = 0;
	struct timeval tv;
	int r;
#ifdef MARCEL
	// Trop de messages avec les activations
	xdebugl(6, "Fast Polling function called on LWP %d\n",
	       marcel_current_vp());
#endif // MARCEL

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

	switch(ev->op) {
	case XPAUL_POLL_READ : {
		FD_SET(ev->FD, &rfds);
		nb = ev->FD + 1;
		break;
	}
	case XPAUL_POLL_WRITE : {
		FD_SET(ev->FD, &wfds);
		nb = ev->FD + 1;
		break;
	}
	case XPAUL_POLL_SELECT : {
		if(ev->RFDS != NULL)
			rfds = *(ev->RFDS);
		if(ev->WFDS != NULL)
			wfds = *(ev->WFDS);
		nb = ev->NFDS;
		break;
	}
	default :
		XPAUL_EXCEPTION_RAISE(XPAUL_PROGRAM_ERROR);
	}

	timerclear(&tv);
	
	errno = 0;

	r = select(nb, &rfds, &wfds, NULL, &tv);

	if(r <= 0) {
	  
	  ev->ret_val=-errno;
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
	do {
		ev.op = XPAUL_POLL_READ;
		ev.FD = fildes;
		xdebug("Reading in fd %i\n", fildes);
		xpaul_wait(&xpaul_io_server.server, &ev.inst, &wait, 0);
		LOG("IO reading fd %i", fildes);
		n = read(fildes, buf, nbytes);
	} while( n == -1 && errno == EINTR);

	LOG_RETURN(n);
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
	do {
		ev.op = XPAUL_POLL_WRITE;
		ev.FD = fildes;
		xdebug("Writing in fd %i\n", fildes);
		xpaul_wait(&xpaul_io_server.server, &ev.inst, &wait, 0);

		LOG("IO writing fd %i", fildes);
		n = write(fildes, buf, nbytes);
	} while( n == -1 && errno == EINTR);
	
	LOG_RETURN(n);
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

int xpaul_select(int nfds, fd_set * __restrict rfds, fd_set * __restrict wfds)
{
	struct xpaul_tcp_ev ev;
	struct xpaul_wait wait;
	
	LOG_IN();
	ev.op = XPAUL_POLL_SELECT;
	ev.RFDS = rfds;
	ev.WFDS = wfds;
	ev.NFDS = nfds;
	xdebug("Selecting within %i fds\n", nfds);
	xpaul_wait(&xpaul_io_server.server, &ev.inst, &wait, 0);
	LOG_RETURN(ev.ret_val >= 0 ? ev.ret_val :
			(errno = -ev.ret_val, -1));
}

/* To force the reading/writing of an exact number of bytes */

int xpaul_read_exactly(int fildes, void *buf, size_t nbytes)
{
	size_t to_read = nbytes, n;

	LOG_IN();
	do {
		n = xpaul_read(fildes, buf, to_read);
		if(n < 0)
			return n;
		buf += n;
		to_read -= n;
	} while(to_read);
	
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
		if(n < 0)
			return n;
		buf += n;
		to_write -= n;
	} while(to_write);
	
	LOG_RETURN(nbytes);
}

#ifndef __MINGW32__
int xpaul_writev_exactly(int fildes, const struct iovec *iov, int iovcnt)
{
	return xpaul_writev(fildes, iov, iovcnt);
}
#endif

/* =============== Gestion des E/S non bloquantes =============== */

int xpaul_tselect(int width, fd_set * __restrict readfds,
		  fd_set * __restrict writefds, fd_set * __restrict exceptfds)
{
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
#ifdef MARCEL
			marcel_yield();
#endif
		}
	} while(res <= 0);
	
	if(readfds) *readfds = rfds;
	if(writefds) *writefds = wfds;
	if(exceptfds) *exceptfds = efds;
   
	return res;
}


#ifdef MA__LWPS

/* Charge le LWP de communication des com */
any_t xpaul_receiver()
{
  int i,j;
  requete cur_req, bak_req;
  struct sched_param param;

  ma_bind_on_processor(0);

  // on ignore SIGALRM pour garder une priorité elevee
  marcel_sig_disable_interrupts();

  param.sched_priority=sched_get_priority_max(SCHED_FIFO);
  if(! pthread_setschedparam(pthread_self(),SCHED_FIFO,&param))
    { // on ne sait jamais, mais ca se tente...
      PROF_EVENT(xpaul_io_rcv_fifo);
    }
  j=0;

  cur_req.readfds=(fd_set*) malloc(sizeof(fd_set));
  cur_req.writefds=(fd_set*) malloc(sizeof(fd_set));
  cur_req.exceptfds=(fd_set*) malloc(sizeof(fd_set));
  FD_ZERO(cur_req.readfds);
  FD_ZERO(cur_req.writefds);
  FD_ZERO(cur_req.exceptfds);

  bak_req.readfds=(fd_set*) malloc(sizeof(fd_set));
  bak_req.writefds=(fd_set*) malloc(sizeof(fd_set));
  bak_req.exceptfds=(fd_set*) malloc(sizeof(fd_set));
  FD_ZERO(bak_req.readfds);
  FD_ZERO(bak_req.writefds);
  FD_ZERO(bak_req.exceptfds);

  FD_SET(fds[0],cur_req.readfds);
  cur_req.n=fds[0]+1;
  FD_SET(fds[0],bak_req.readfds);
  bak_req.n=fds[0]+1;

  for(;;) {
    marcel_sem_P(&req_sem);

    for(i=0;i<reqs.n;i++){
      if(FD_ISSET(i, reqs.readfds)){
	FD_SET(i, cur_req.readfds);
	FD_SET(i, bak_req.readfds);
	FD_CLR(i,reqs.readfds);
      }
      if(FD_ISSET(i, reqs.writefds)){
	FD_SET(i, cur_req.writefds);
	FD_SET(i, bak_req.writefds);
	FD_CLR(i,reqs.writefds);
      }  
      if(FD_ISSET(i, reqs.exceptfds)){
	FD_SET(i, cur_req.exceptfds);
	FD_SET(i, bak_req.exceptfds);
	FD_CLR(i,reqs.exceptfds);
      }
    }

    FD_SET(fds[0],cur_req.readfds);
    cur_req.n= (reqs.n > (fds[0]+1))?reqs.n : fds[0]+1;
    bak_req.n=cur_req.n;

    marcel_sem_V(&req_sem);
    PROF_EVENT(xpaul_entering_select);
    res=select( cur_req.n,
		cur_req.readfds,
		cur_req.writefds,
		cur_req.exceptfds,
		NULL);
    PROF_EVENT(xpaul_data_received);

    j++;
    if(res<0 && (errno != EINTR)){ 
      perror("select");
      TBX_FAILURE("system call failed");	  
    }
    
    if(res>0) {
      for(i=0;i<cur_req.n;i++)
	{
	  if(FD_ISSET(i,cur_req.readfds)){
	    FD_SET(i,res_req.readfds);
	  }
	  if(FD_ISSET(i,cur_req.writefds))
	    FD_SET(i,res_req.writefds);
	  if(FD_ISSET(i,cur_req.exceptfds))
	    FD_SET(i,res_req.exceptfds);
	}
      res_req.n=cur_req.n;
    }
   
    if(res>0){ // pour eviter de reveiller un thread qui n'a rien recu
      if(FD_ISSET(fds[0], cur_req.readfds)){
	PROF_EVENT(xpaul_order_received);
	read(fds[0], &i,sizeof(i));
	res--;
	FD_CLR(fds[0], res_req.readfds);
      }
    }
   
    if(res>0) {
      PROF_EVENT(xpaul_waking_up_a_thread);
      marcel_sem_V(&recved_sem); // TODO : choisir le thread qu'on reveille (ne reveiller que ceux qu'il faut)
     }
    for(i=0;i<bak_req.n;i++)
      {
	if(FD_ISSET(i,bak_req.readfds) && !FD_ISSET(i,cur_req.readfds)){
	  cur_req.n=i+1;
	  FD_SET(i,cur_req.readfds);
	}
	else
	  FD_CLR(i,cur_req.readfds);
	if(FD_ISSET(i,bak_req.writefds) && !FD_ISSET(i,cur_req.writefds)){
	  cur_req.n=i+1;
	  FD_SET(i,cur_req.writefds);
	}
	else
	  FD_CLR(i,cur_req.writefds);
	if(FD_ISSET(i,bak_req.exceptfds) && !FD_ISSET(i,cur_req.exceptfds)){
	  cur_req.n=i+1;
	  FD_SET(i,cur_req.exceptfds);
	}
	else
	  FD_CLR(i,cur_req.exceptfds);
      }
  }
}
#endif // MA__LWPS


void xpaul_io_init(void)
{
	LOG_IN();
	xpaul_server_set_poll_settings(&xpaul_io_server.server, 
				       XPAUL_POLL_AT_TIMER_SIG 
				       | XPAUL_POLL_AT_IDLE,
				       1,-1);
	
#ifdef MA__LWPS 
	// TODO: aggregation de requêtes
	xpaul_server_add_callback(&xpaul_io_server.server,
				  XPAUL_FUNCTYPE_BLOCK_WAITONE,
				  (xpaul_pcallback_t)  { .func=&xpaul_io_block,
							    .speed=XPAUL_CALLBACK_SLOWEST } );
	
	xpaul_server_add_callback(&xpaul_io_server.server,
				  XPAUL_FUNCTYPE_BLOCK_WAITONE_TIMEOUT,
				  (xpaul_pcallback_t)  { .func=&xpaul_io_block,
							    .speed=XPAUL_CALLBACK_SLOWEST });
	
	xpaul_server_add_callback(&xpaul_io_server.server,
				  XPAUL_FUNCTYPE_BLOCK_GROUP,
				  (xpaul_pcallback_t)  { .func=&xpaul_io_block,
							    .speed=XPAUL_CALLBACK_SLOWEST });
	
	xpaul_server_add_callback(&xpaul_io_server.server,
				  XPAUL_FUNCTYPE_BLOCK_WAITANY,
				  (xpaul_pcallback_t)  { .func=&xpaul_io_block,
							    .speed=XPAUL_CALLBACK_SLOWEST });
	
	xpaul_server_add_callback(&xpaul_io_server.server,
				  XPAUL_FUNCTYPE_BLOCK_WAITANY_TIMEOUT,
				  (xpaul_pcallback_t)  { .func=&xpaul_io_block,
							    .speed=XPAUL_CALLBACK_SLOWEST });
	
	xpaul_server_add_callback(&xpaul_io_server.server,
				  XPAUL_FUNCTYPE_UNBLOCK_WAITONE,
				  (xpaul_pcallback_t)  { .func=&xpaul_io_block,
							    .speed=XPAUL_CALLBACK_SLOWEST });
	
	xpaul_server_add_callback(&xpaul_io_server.server,
				  XPAUL_FUNCTYPE_UNBLOCK_WAITANY,
				  (xpaul_pcallback_t)  { .func=&xpaul_io_block,
							    .speed=XPAUL_CALLBACK_SLOWEST });
	
	
#endif
	
#ifndef MARCEL_DO_NOT_GROUP_TCP
 	xpaul_server_add_callback(&xpaul_io_server.server, 
				  XPAUL_FUNCTYPE_POLL_GROUP,
				  (xpaul_pcallback_t)  { .func=&xpaul_io_group,
							    .speed=XPAUL_CALLBACK_SLOWEST });
	xpaul_server_add_callback(&xpaul_io_server.server,
				  XPAUL_FUNCTYPE_POLL_POLLANY,
				  (xpaul_pcallback_t)  { .func=&xpaul_io_poll,
							    .speed=XPAUL_CALLBACK_SLOWEST });
#endif
	xpaul_server_add_callback(&xpaul_io_server.server,
				  XPAUL_FUNCTYPE_POLL_POLLONE,
				  (xpaul_pcallback_t)  { .func=&xpaul_io_fast_poll,
							    .speed=XPAUL_CALLBACK_SLOWEST });
	
	xpaul_server_start(&xpaul_io_server.server);
	LOG_OUT();
}


#ifdef MA__LWPS

/* creation du lwp de comm */
void xpaul_init_receiver(void)
{
  vp_nb=marcel_add_lwp();
  marcel_attr_t attr;

  marcel_t recv_pid;

  if (pipe(fds) == -1) { perror("pipe"); exit(EXIT_FAILURE); }

  marcel_sem_init(&recved_sem,0);
  marcel_sem_init(&req_sem,1);

  res_req.readfds=(fd_set*) malloc(sizeof(fd_set));
  res_req.writefds=(fd_set*) malloc(sizeof(fd_set));
  res_req.exceptfds=(fd_set*) malloc(sizeof(fd_set));
  FD_ZERO(res_req.readfds);
  FD_ZERO(res_req.writefds);
  FD_ZERO(res_req.exceptfds);

  reqs.readfds=(fd_set*) malloc(sizeof(fd_set));
  reqs.writefds=(fd_set*) malloc(sizeof(fd_set));
  reqs.exceptfds=(fd_set*) malloc(sizeof(fd_set));
  FD_ZERO(reqs.readfds);
  FD_ZERO(reqs.writefds);
  FD_ZERO(reqs.exceptfds);

  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, tbx_true);
  marcel_attr_setvpmask(&attr, MARCEL_VPMASK_ALL_BUT_VP(vp_nb));
  marcel_attr_setname(&attr, "receiver");

  marcel_create(&recv_pid, &attr, xpaul_receiver, NULL);
  nb_comm_threads=1;
}

#endif
