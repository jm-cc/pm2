
#include "marcel.h"

#include <unistd.h>

#ifndef max
#define max(a, b)  ((a) > (b) ? (a) : (b))
#endif

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
  
#ifndef MA__ACTIVATION
  // Trop de messages avec les activations
  mdebug("Polling function called on LWP %d (%2d A, %2d S, %2d B)\n",
	 marcel_current_vp(), active, sleeping, blocked);

  if(active) {
    mdebug("E/S non bloquante\n");
#endif

    timerclear(&tv);
    ptv = &tv;
#ifndef MA__ACTIVATION
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

static void *unix_io_fast_poll(marcel_pollid_t id, any_t arg, boolean first_call)
{
  unix_io_arg_t *myarg = (unix_io_arg_t *)arg;
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

  switch(myarg->op) {
    case POLL_READ : {
      FD_SET(myarg->fd, &rfds);
      nb = myarg->fd + 1;
      break;
    }
    case POLL_WRITE : {
      FD_SET(myarg->fd, &wfds);
      nb = myarg->fd + 1;
      break;
    }
    case POLL_SELECT : {
      if(myarg->rfds != NULL)
	rfds = *(myarg->rfds);
      if(myarg->wfds != NULL)
	wfds = *(myarg->wfds);
      nb = myarg->nfds;
      break;
    }
    default :
      RAISE(PROGRAM_ERROR);
  }

  timerclear(&tv);

  r = select(nb, &rfds, &wfds, NULL, &tv);

  if(r > 0) {
    switch(myarg->op) {
    case POLL_READ :
      if(FD_ISSET(myarg->fd, &rfds))
	return MARCEL_POLL_SUCCESS(id);
      break;
    case POLL_WRITE :
      if(FD_ISSET(myarg->fd, &wfds))
	return MARCEL_POLL_SUCCESS(id);
      break;
    case POLL_SELECT : {
      unsigned i;
      if(myarg->rfds != NULL) {
	for(i=0; i<nb; i++)
	  if(FD_ISSET(i, &rfds)) {
	    FD_ZERO(myarg->rfds);
	    if(myarg->wfds != NULL)
	      FD_ZERO(myarg->wfds);
	    FD_SET(i, myarg->rfds);
	    return MARCEL_POLL_SUCCESS(id);
	  }
      }
      if(myarg->wfds != NULL) {
	for(i=0; i<nb; i++)
	  if(FD_ISSET(i, &wfds)) {
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

  return MARCEL_POLL_FAILED;
}

void marcel_io_init()
{
  unix_io_pollid = marcel_pollid_create(unix_io_group,
					unix_io_poll,
					unix_io_fast_poll,
					MARCEL_POLL_AT_TIMER_SIG 
					| MARCEL_POLL_AT_YIELD );
}

int marcel_read(int fildes, void *buf, size_t nbytes)
{
#ifndef MA__ACTIVATION
  unix_io_arg_t myarg;

  myarg.op = POLL_READ;
  myarg.fd = fildes;
  marcel_poll(unix_io_pollid, (any_t)&myarg);
#endif

  LOG("IO reading fd %i", fildes);
  return read(fildes, buf, nbytes);
}

int marcel_readv(int fildes, const struct iovec *iov, int iovcnt)
{
#ifndef MA__ACTIVATION
  unix_io_arg_t myarg;

  myarg.op = POLL_READ;
  myarg.fd = fildes;
  marcel_poll(unix_io_pollid, (any_t)&myarg);
#endif

  LOG("IO readving fd %i", fildes);
  return readv(fildes, iov, iovcnt);
}

int marcel_write(int fildes, void *buf, size_t nbytes)
{
#ifndef MA__ACTIVATION
  unix_io_arg_t myarg;

  myarg.op = POLL_WRITE;
  myarg.fd = fildes;
  marcel_poll(unix_io_pollid, (any_t)&myarg);
#endif

  LOG("IO writing fd %i", fildes);
  return write(fildes, buf, nbytes);
}

int marcel_writev(int fildes, const struct iovec *iov, int iovcnt)
{
#ifndef MA__ACTIVATION
  unix_io_arg_t myarg;

  myarg.op = POLL_WRITE;
  myarg.fd = fildes;
  marcel_poll(unix_io_pollid, (any_t)&myarg);
#endif

  LOG("IO writving fd %i", fildes);
  return writev(fildes, iov, iovcnt);
}

int marcel_select(int nfds, fd_set *rfds, fd_set *wfds)
{
#ifndef MA__ACTIVATION
  unix_io_arg_t myarg;

  myarg.op = POLL_SELECT;
  myarg.rfds = rfds;
  myarg.wfds = wfds;
  myarg.nfds = nfds;

  marcel_poll(unix_io_pollid, (any_t)&myarg);
#else
  LOG("IO selecting");
  select(nfds, rfds, wfds, NULL, NULL);
#endif

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


