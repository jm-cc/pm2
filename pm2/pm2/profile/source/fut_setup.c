/*	fut_setup.c */
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


/*	fut = Fast User Tracing */

#define CONFIG_FUT

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include "fut.h"
#include <fkt/get_cpu_info.h>
#include <fkt/block.h>
#include <fkt/sysmap.h>
#include <fkt/pids.h>
#include <fkt/names.h>
#include "fkt-tools.h"

#ifdef MARCEL
#  include "sys/marcel_flags.h"
#endif

#define MAXCPUS 16

#define UNAMESTRLEN 256


/*	active masks should never become negative numbers in order to prevent
	problems returning them as function results (they look like error codes!) */
#define FULL_ACTIVE_MASK	0x7fffffff

/*	3 external ints */

/*	set to non-zero when probing is active */
volatile unsigned int fut_active = 0;

/*	points to next unused byte in buffer (multiple of 4) */
volatile unsigned int *fut_next_slot = NULL;

/*	points to byte beyond end of buffer (multiple of 4) */
volatile unsigned int *fut_last_slot = NULL;


/*	points to region of allocated buffer space */
static char	*bufptr = NULL;

/*	number of contiguous allocated bytes pointed to by bufptr */
static unsigned int	nallocated = 0;

/*	holds pid of process that called fut_setup() */
static unsigned int	fut_pid = 0;

/*	holds times of setup(), endup() */
static time_t	fut_start_time = 0, fut_stop_time = 0;
static clock_t	fut_start_jiffies = 0, fut_stop_jiffies = 0;

/*	holds cpu mhz as retrieved from /proc/cpuinfo */
static double	mhz[MAXCPUS];

/*	holds cpu number */
static int	ncpus;

void dumptime( time_t *the_time, clock_t *the_jiffies)
{
  struct tms cur_time;

  if( (*the_time = time(NULL)) == -1 )
    perror("time");

  if( (*the_jiffies = times(&cur_time)) < 0 )
    perror("times");
}

/*	called once to set up tracing.
	includes mallocing the buffer to hold the trace.
	returns number of bytes allocated if all ok, else a negative error code.
*/
int fut_setup( unsigned int nints, unsigned int keymask, unsigned int threadid )
	{
	unsigned int	nbytes, *iptr;


	/* paranoia, so nobody waits for us while we are in here */
	fut_active = 0;

	/*	remember pid of process that called setup */
	fut_pid = getpid();

	/*	find out speed of this cpu in mhz */
	ncpus = fkt_get_cpu_info(MAXCPUS, mhz);

	if( bufptr != NULL )
		{/* previous allocation region was not released, do it now */
		free(bufptr);
		/* nothing allocated now */
		bufptr = NULL;
		nallocated = 0;
		}

	/*	allocate buffer */
	nbytes = nints << 2;			/* force multiple of 4 bytes */
	if( (bufptr = (char *)malloc(nbytes)) == NULL )
		return -ENOMEM;
	nallocated = nbytes;
	nints = nbytes >> 2;
		

	fut_last_slot = (unsigned int *)(bufptr + nbytes);

	/*	touch all pages so they get loaded */
	for( iptr = (unsigned int *)bufptr;  iptr < fut_last_slot;  )
		{
		*iptr = 0;
		iptr += 0x100;
		}

	dumptime(&fut_start_time, &fut_start_jiffies);

	fut_next_slot = (unsigned int *)bufptr;
	fut_active = keymask & FULL_ACTIVE_MASK;

	FUT_PROBE3(-1, FUT_SETUP_CODE, fut_active, threadid, nints);
	FUT_PROBE0(-1, FUT_CALIBRATE0_CODE);
	FUT_PROBE0(-1, FUT_CALIBRATE0_CODE);
	FUT_PROBE0(-1, FUT_CALIBRATE0_CODE);
	FUT_PROBE1(-1, FUT_CALIBRATE1_CODE, 1);
	FUT_PROBE1(-1, FUT_CALIBRATE1_CODE, 1);
	FUT_PROBE1(-1, FUT_CALIBRATE1_CODE, 1);
	FUT_PROBE2(-1, FUT_CALIBRATE2_CODE, 1, 2);
	FUT_PROBE2(-1, FUT_CALIBRATE2_CODE, 1, 2);
	FUT_PROBE2(-1, FUT_CALIBRATE2_CODE, 1, 2);

	return nints;
	}


/*	called repeatedly to restart tracing.
	returns previous value of fut_active if all ok, else a negative error code.
*/
int fut_keychange( int how, unsigned int keymask, unsigned int threadid )
	{
	unsigned int	old_active = fut_active;


	if( fut_next_slot >= fut_last_slot  ||  bufptr == NULL )
		return -EPERM;

	switch( how )
		{
	case FUT_ENABLE:
		fut_active |= keymask & FULL_ACTIVE_MASK;
		break;
	case FUT_DISABLE:
		fut_active &= (~keymask) & FULL_ACTIVE_MASK;
		break;
	case FUT_SETMASK:
		fut_active = keymask & FULL_ACTIVE_MASK;
		break;
	default:
		return -EINVAL;
		}

	FUT_PROBE2(-1, FUT_KEYCHANGE_CODE, fut_active, threadid);
	return old_active;
	}


/*	called once when completely done with buffer.
	returns previous value of active if all ok, else a negative error code.
*/
int fut_done( void )
	{
	unsigned int	old_active = fut_active;


	fut_active = 0;
	if( bufptr != NULL )
		{/* previous allocation region was not released, do it now */
		free(bufptr);
		bufptr = NULL;			/* nothing allocated now */
		nallocated = 0;
		}

	return old_active;
	}


/*	called to reset tracing to refill the entire buffer again.
	returns number of bytes in buffer if all ok, else a negative error code.
*/
int fut_reset( unsigned int keymask, unsigned int threadid )
	{
	unsigned int	nints;


	/* paranoia, so nobody waits for us while we are in here */
	fut_active = 0;

	if( bufptr == NULL  ||  nallocated == 0 )
		{/* buffer was never allocated, return error */
		return -ENOMEM;
		}

	/* reset the buffer to completely empty */
	fut_last_slot = (unsigned int *)(bufptr + nallocated);
	fut_next_slot = (unsigned int *)bufptr;
	fut_active = keymask & FULL_ACTIVE_MASK;
	nints = nallocated >> 2;

	FUT_PROBE3(-1, FUT_RESET_CODE, fut_active, threadid, nints);
	FUT_PROBE0(-1, FUT_CALIBRATE0_CODE);
	FUT_PROBE0(-1, FUT_CALIBRATE0_CODE);
	FUT_PROBE0(-1, FUT_CALIBRATE0_CODE);
	FUT_PROBE1(-1, FUT_CALIBRATE1_CODE, 1);
	FUT_PROBE1(-1, FUT_CALIBRATE1_CODE, 1);
	FUT_PROBE1(-1, FUT_CALIBRATE1_CODE, 1);
	FUT_PROBE2(-1, FUT_CALIBRATE2_CODE, 1, 2);
	FUT_PROBE2(-1, FUT_CALIBRATE2_CODE, 1, 2);
	FUT_PROBE2(-1, FUT_CALIBRATE2_CODE, 1, 2);

	return nints;
	}


/*	called repeatedly to copy current buffer into user space.
	returns nints >= 0 if all ok, else a negative error code.
*/
int fut_getbuffer( int *nints, unsigned int **buffer )
	{
	int				local_nints = 0;


	/* paranoia, so nobody waits for us while we are in here */
	fut_active = 0;

	if( bufptr == NULL )
		{
		local_nints = -EPERM;
		goto out;
		}

	/*	get number of ints worth of trace data currently in the buffer */
	local_nints = ((char *)fut_next_slot - bufptr) >> 2;

	if( nints != NULL )
		*nints = local_nints;

	if( buffer != NULL )
		*buffer = (unsigned int *)bufptr;

out:
	return local_nints;
	}

static void record_time( int fd, int ncpus, double mhz[], size_t page_size )
{
  int i;
  int kpid;
  int pid;
  int format;

#ifdef MA__FUT_RECORD_TID
  format=FUT_FORMAT_SMP_THREAD;
#else
  format=FUT_FORMAT_MONO;
#endif

  BEGIN_BLOCK(fd);
  if( write(fd, (void *)&ncpus, sizeof(ncpus)) < 0 )
    perror("write ncpus");
  for( i = 0;  i < ncpus;  i++ ) {
    if( write(fd, (void *)&mhz[i], sizeof(mhz[i])) < 0 )
      perror("write mhz");
  }
  pid = getpid();
  if( write(fd, (void *)&pid, sizeof(pid)) < 0 )
    perror("write pid");
  kpid = 0;
  if( write(fd, (void *)&kpid, sizeof(kpid)) < 0 )
    perror("write kpid");
  if( write(fd, (void *)&fut_start_time, sizeof(fut_start_time)) < 0 )
    perror("write start_time");
  if( write(fd, (void *)&fut_stop_time, sizeof(fut_stop_time)) < 0 )
    perror("write stop_time");
  if( write(fd, (void *)&fut_start_jiffies, sizeof(fut_start_jiffies)) < 0 )
    perror("write start_jiffies");
  if( write(fd, (void *)&fut_stop_jiffies, sizeof(fut_stop_jiffies)) < 0 )
    perror("write stop_jiffies");
  if( write(fd, (void *)&page_size, sizeof(page_size)) < 0 )
    perror("write page_size");
  if( write(fd, (void *)&format, sizeof(format)) < 0 )
    perror("write format");
  END_BLOCK(fd);
}

int fut_endup( char *filename )
	{
	int n, nints, size, fd;
	static const unsigned int zero = 0;
	unsigned int *copy;
	unsigned int len;
	struct utsname unameinfo;
	char unamestr[UNAMESTRLEN];

	/* stop all futher tracing */
	fut_active = 0;

	dumptime(&fut_stop_time,&fut_stop_jiffies);

	if( (n = fut_getbuffer(&nints, &copy)) < 0 )
		return n;

	size = nints << 2;

	if( filename == NULL )
		filename = DEFAULT_TRACE_FILE;

	if( (fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0 )
		return fd;

	BEGIN_BLOCK(fd);
	record_time( fd, ncpus, mhz, size+sizeof(size) );
	BEGIN_BLOCK(fd);
	// no irq
	write(fd,(void *)&zero,sizeof(zero));
	// no cpu for irqs
	write(fd,(void *)&zero,sizeof(zero));
	END_BLOCK(fd);

  /* uname -a */
  if( uname(&unameinfo) )
    {
    perror("getting uname information");
    exit(EXIT_FAILURE);
    }

  snprintf(unamestr,UNAMESTRLEN,"%s %s %s %s %s",
        unameinfo.sysname,
        unameinfo.nodename,
        unameinfo.release,
        unameinfo.version,
        unameinfo.machine);
  unamestr[UNAMESTRLEN-1]='\0';

  len = strlen(unamestr);
  BEGIN_BLOCK(fd);
  write(fd,&len,sizeof(len));
  write(fd,unamestr,len);
  END_BLOCK(fd);

  BEGIN_BLOCK(fd);
  fkt_record_sysmap(fd);
  END_BLOCK(fd);

  BEGIN_BLOCK(fd);
  fkt_record_pids(fd);
  END_BLOCK(fd);

  END_BLOCK(fd);

  size+=sizeof(size);
  if( write(fd, (void *)&size, sizeof(size)) < sizeof(size) )
    perror("write buffer's size");
  size-=sizeof(size);

	if( write(fd, (void *)copy, size) < 0 )
		perror("write buffer");

	if( close(fd) < 0 )
		perror(filename);

	//fut_done(); // Removed by Raymond

	return nints;
	}
