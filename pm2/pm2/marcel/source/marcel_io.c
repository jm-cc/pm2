/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: marcel_io.c,v $
Revision 1.6  2000/04/17 08:31:51  rnamyst
Changed DEBUG into MA__DEBUG.

Revision 1.5  2000/04/11 09:07:21  rnamyst
Merged the "reorganisation" development branch.

Revision 1.4.2.3  2000/04/11 08:17:31  rnamyst
Comments are back !

Revision 1.4.2.2  2000/04/08 15:09:14  vdanjean
few bugs fixed

Revision 1.4.2.1  2000/03/15 15:55:07  vdanjean
réorganisation de marcel : commit pour CVS

Revision 1.4  2000/02/28 10:25:03  rnamyst
Changed #include <> into #include "".

Revision 1.3  2000/02/01 14:31:21  rnamyst
Minor modifications.

Revision 1.2  2000/01/31 15:57:15  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

/* #define MA__DEBUG */

#include "marcel.h"

#include <unistd.h>

#define max(a, b)  ((a) > (b) ? (a) : (b))

static marcel_pollid_t unix_io_pollid;

typedef enum {
  POLL_READ,
  POLL_WRITE,
  POLL_SELECT
} poll_op_t;

static struct {
  fd_set rfds, wfds;
  unsigned nb;
} unix_io_args;

typedef struct { /* Should be a union */
  poll_op_t op;
  int fd;
  fd_set *rfds;
  fd_set *wfds;
  int nfds;
} unix_io_arg_t;

static void unix_io_group(marcel_pollid_t id)
{
  unix_io_arg_t *myarg;

  unix_io_args.nb = 0;
  FD_ZERO(&unix_io_args.rfds);
  FD_ZERO(&unix_io_args.wfds);

  FOREACH_POLL(id, myarg) {
    switch(myarg->op) {
    case POLL_READ : {
      FD_SET(myarg->fd, &unix_io_args.rfds);
      unix_io_args.nb = max(unix_io_args.nb, myarg->fd+1);
      break;
    }
    case POLL_WRITE : {
      FD_SET(myarg->fd, &unix_io_args.wfds);
      unix_io_args.nb = max(unix_io_args.nb, myarg->fd+1);
      break;
    }
    case POLL_SELECT : {
      unsigned i;
      if(myarg->rfds != NULL) {
	for(i=0; i<myarg->nfds; i++)
	  if(FD_ISSET(i, myarg->rfds))
	    FD_SET(i, &unix_io_args.rfds);
      }
      if(myarg->wfds != NULL) {
	for(i=0; i<myarg->nfds; i++)
	  if(FD_ISSET(i, myarg->wfds))
	    FD_SET(i, &unix_io_args.wfds);
      }
      unix_io_args.nb = max(unix_io_args.nb, myarg->nfds);
      break;
    }
    default :
      RAISE(PROGRAM_ERROR);
    }
  }
}

static void *unix_io_poll(marcel_pollid_t id,
			  unsigned active, unsigned sleeping, unsigned blocked)
{
  unix_io_arg_t *myarg;
  int r;
  fd_set rfds, wfds;
  struct timeval tv, *ptv;
  
#ifndef MA__ACT
  // Trop de messages avec les activations
  mdebug("Polling function called on LWP %d (%2d A, %2d S, %2d B)\n",
	 marcel_current_vp(), active, sleeping, blocked);

  if(active) {
    mdebug("E/S non bloquante\n");
#endif

    timerclear(&tv);
    ptv = &tv;
#ifndef MA__ACT
  } else if(sleeping) {
    mdebug("E/S a duree limitee\n");

    tv.tv_sec = 0;
    tv.tv_usec = 5000;
    ptv = &tv;
  } else {
    mdebug("E/S bloquante\n");

    tv.tv_sec = 0;
    tv.tv_usec = 5000;
    ptv = &tv;
    /* ptv = NULL; */
  }
#endif

  rfds = unix_io_args.rfds;
  wfds = unix_io_args.wfds;
  r = select(unix_io_args.nb, &rfds, &wfds, NULL, ptv);

  if(r > 0) {
    FOREACH_POLL(id, myarg) {
      switch(myarg->op) {
      case POLL_READ :
	if(FD_ISSET(myarg->fd, &rfds))
	  return MARCEL_POLL_SUCCESS(id);
	else
	  break;
      case POLL_WRITE :
	if(FD_ISSET(myarg->fd, &wfds))
	  return MARCEL_POLL_SUCCESS(id);
	else
	  break;
      case POLL_SELECT : {
	unsigned i;
	if(myarg->rfds != NULL) {
	  for(i=0; i<myarg->nfds; i++)
	    if(FD_ISSET(i, myarg->rfds) && FD_ISSET(i, &rfds)) {
	      FD_ZERO(myarg->rfds);
	      if(myarg->wfds != NULL)
		FD_ZERO(myarg->wfds);
	      FD_SET(i, myarg->rfds);
	      return MARCEL_POLL_SUCCESS(id);
	    }
	}
	if(myarg->wfds != NULL) {
	  for(i=0; i<myarg->nfds; i++)
	    if(FD_ISSET(i, myarg->wfds) && FD_ISSET(i, &wfds)) {
	      FD_ZERO(myarg->wfds);
	      if(myarg->rfds != NULL)
		FD_ZERO(myarg->rfds);
	      FD_SET(i, myarg->wfds);
	      return MARCEL_POLL_SUCCESS(id);
	    }
	}
	break;
      }
      default :
	RAISE(PROGRAM_ERROR);
      }
    }
  }

  return MARCEL_POLL_FAILED;
}

void marcel_io_init()
{
  const unsigned divisor = 1;

  unix_io_pollid = marcel_pollid_create(unix_io_group,
					unix_io_poll,
					divisor);
}

int marcel_read(int fildes, void *buf, size_t nbytes)
{
  unix_io_arg_t myarg;

  myarg.op = POLL_READ;
  myarg.fd = fildes;
  marcel_poll(unix_io_pollid, (any_t)&myarg);

  return read(fildes, buf, nbytes);
}

int marcel_readv(int fildes, const struct iovec *iov, int iovcnt)
{
  unix_io_arg_t myarg;

  myarg.op = POLL_READ;
  myarg.fd = fildes;
  marcel_poll(unix_io_pollid, (any_t)&myarg);

  return readv(fildes, iov, iovcnt);
}

int marcel_write(int fildes, void *buf, size_t nbytes)
{
  unix_io_arg_t myarg;

  myarg.op = POLL_WRITE;
  myarg.fd = fildes;
  marcel_poll(unix_io_pollid, (any_t)&myarg);

  return write(fildes, buf, nbytes);
}

int marcel_writev(int fildes, const struct iovec *iov, int iovcnt)
{
  unix_io_arg_t myarg;

  myarg.op = POLL_WRITE;
  myarg.fd = fildes;
  marcel_poll(unix_io_pollid, (any_t)&myarg);

  return writev(fildes, iov, iovcnt);
}

int marcel_select(int nfds, fd_set *rfds, fd_set *wfds)
{
  unix_io_arg_t myarg;

  myarg.op = POLL_SELECT;
  myarg.rfds = rfds;
  myarg.wfds = wfds;
  myarg.nfds = nfds;

  marcel_poll(unix_io_pollid, (any_t)&myarg);

  return 1;
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

int marcel_write_exactly(int fildes, void *buf, size_t nbytes)
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


