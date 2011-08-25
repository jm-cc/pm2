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
#include <sys/times.h>
#include "fxt/fxt.h"
#include "fxt/fut.h"
#include "fxt/fxt-tools.h"
#include "pm2_fxt-tools.h"

#include <tbx.h>

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
unsigned long * volatile fut_next_slot = NULL;

/*	points to byte beyond end of buffer (multiple of 4) */
unsigned long * volatile fut_last_slot = NULL;

fxt_t fut;
static struct fxt_infos *fut_infos;

/*	points to region of allocated buffer space */
static char	*bufptr = NULL;

/*	number of contiguous allocated bytes pointed to by bufptr */
static unsigned int	nallocated = 0;

void dumptime( time_t *the_time, clock_t *the_jiffies)
{
  struct tms cur_time;

  if( (*the_time = time(NULL)) == -1 )
    perror("time");

  if( (*the_jiffies = times(&cur_time)) == (clock_t) -1 )
    perror("times");
}

/*	called once to set up tracing.
	includes mallocing the buffer to hold the trace.
	returns number of bytes allocated if all ok, else a negative error code.
*/
int fut_setup( unsigned int nlongs, unsigned int keymask, unsigned int threadid )
	/* TODO: move to libfxt */
	{
	unsigned int	nbytes;
	unsigned long	*iptr;


	/* paranoia, so nobody waits for us while we are in here */
	fut_active = 0;

	fut = fxt_setinfos(FXT_SPACE_USER);
	fut_infos = fxt_infos(fut);

	/*	remember pid of process that called setup */
	fut_infos->record_pid = getpid();

	if( bufptr != NULL )
		{/* previous allocation region was not released, do it now */
		TBX_FREE(bufptr);
		/* nothing allocated now */
		bufptr = NULL;
		nallocated = 0;
		}

	/*	allocate buffer */
	nbytes = nlongs * sizeof(long);		/* force multiple of 4/8 bytes */
	if( (bufptr = (char *)TBX_MALLOC(nbytes)) == NULL )
		return -ENOMEM;
	nallocated = nbytes;
	nlongs = nbytes / sizeof(long);
		

	fut_last_slot = (unsigned long *)(bufptr + nbytes);

	/*	touch all pages so they get loaded */
	for( iptr = (unsigned long *)bufptr;  iptr < fut_last_slot;  )
		{
		*iptr = 0;
		iptr += 0x100;
		}

	dumptime(&fut_infos->start_time, &fut_infos->start_jiffies);

	fut_next_slot = (unsigned long *)bufptr;
	fut_active = keymask & FULL_ACTIVE_MASK;

	FUT_PROBE1(FUT_GCC_INSTRUMENT_KEYMASK,
		FUT_GCC_INSTRUMENT_ENTRY_CODE,
		fut_setup);
	FUT_PROBE3(-1, FUT_SETUP_CODE, fut_active, threadid, nlongs);
	FUT_PROBE0(-1, FUT_CALIBRATE0_CODE);
	FUT_PROBE0(-1, FUT_CALIBRATE0_CODE);
	FUT_PROBE0(-1, FUT_CALIBRATE0_CODE);
	FUT_PROBE1(-1, FUT_CALIBRATE1_CODE, 1);
	FUT_PROBE1(-1, FUT_CALIBRATE1_CODE, 1);
	FUT_PROBE1(-1, FUT_CALIBRATE1_CODE, 1);
	FUT_PROBE2(-1, FUT_CALIBRATE2_CODE, 1, 2);
	FUT_PROBE2(-1, FUT_CALIBRATE2_CODE, 1, 2);
	FUT_PROBE2(-1, FUT_CALIBRATE2_CODE, 1, 2);

	return nlongs;
	}


/*	called repeatedly to restart tracing.
	returns previous value of fut_active if all ok, else a negative error code.
*/
int fut_keychange( int how, unsigned int keymask, unsigned int threadid )
	{
	unsigned int	old_active = fut_active;
#if 0
	FUT_PROBE1(FUT_GCC_INSTRUMENT_KEYMASK,
		FUT_GCC_INSTRUMENT_ENTRY_CODE,
		&fut_keychange);
#endif

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
#if 0
	printf("Recording keychange %p\n", &fut_keychange);
	FUT_PROBE1(FUT_GCC_INSTRUMENT_KEYMASK,
                   FUT_GCC_INSTRUMENT_EXIT_CODE,
                   &fut_keychange);
#endif
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
		TBX_FREE(bufptr);
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
	unsigned int	nlongs;


	/* paranoia, so nobody waits for us while we are in here */
	fut_active = 0;

	if( bufptr == NULL  ||  nallocated == 0 )
		{/* buffer was never allocated, return error */
		return -ENOMEM;
		}

	/* reset the buffer to completely empty */
	fut_last_slot = (unsigned long *)(bufptr + nallocated);
	fut_next_slot = (unsigned long *)bufptr;
	fut_active = keymask & FULL_ACTIVE_MASK;
	nlongs = nallocated / sizeof(long);

	FUT_PROBE3(-1, FUT_RESET_CODE, fut_active, threadid, nlongs);
	FUT_PROBE0(-1, FUT_CALIBRATE0_CODE);
	FUT_PROBE0(-1, FUT_CALIBRATE0_CODE);
	FUT_PROBE0(-1, FUT_CALIBRATE0_CODE);
	FUT_PROBE1(-1, FUT_CALIBRATE1_CODE, 1);
	FUT_PROBE1(-1, FUT_CALIBRATE1_CODE, 1);
	FUT_PROBE1(-1, FUT_CALIBRATE1_CODE, 1);
	FUT_PROBE2(-1, FUT_CALIBRATE2_CODE, 1, 2);
	FUT_PROBE2(-1, FUT_CALIBRATE2_CODE, 1, 2);
	FUT_PROBE2(-1, FUT_CALIBRATE2_CODE, 1, 2);

	return nlongs;
	}


/*	called repeatedly to copy current buffer into user space.
	returns nlongs >= 0 if all ok, else a negative error code.
*/
int fut_getbuffer( int *nlongs, unsigned long **buffer )
	{
	int				local_nlongs = 0;


	/* paranoia, so nobody waits for us while we are in here */
	fut_active = 0;

	if( bufptr == NULL )
		{
		local_nlongs = -EPERM;
		goto out;
		}

	/*	get number of ints worth of trace data currently in the buffer */
	local_nlongs = ((char *)fut_next_slot - bufptr) / sizeof(long);

	if( nlongs != NULL )
		*nlongs = local_nlongs;

	if( buffer != NULL )
		*buffer = (unsigned long *)bufptr;

out:
	return local_nlongs;
	}

int fut_endup( char *filename )
	{
	int n, nlongs=0, fd;
	long size;
	unsigned long *copy=NULL;

	/* stop all futher tracing */
	fut_active = 0;

	dumptime(&fut_infos->stop_time,&fut_infos->stop_jiffies);

	if( (n = fut_getbuffer(&nlongs, &copy)) < 0 )
		return n;

	size = nlongs * sizeof(long);

	if( filename == NULL )
		filename = DEFAULT_TRACE_FILE;

	if( (fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0 )
		return fd;

	fut_infos->page_size = size;
	fxt_fdwrite(fut,fd);

	fxt_fdevents_start(fut, fd, FXT_TRACE_USER_RAW);
	//if( write(fd, (void *)&size, sizeof(size)) < sizeof(size) )
	//  perror("write buffer's size");
	
	if( write(fd, (void *)copy, size) < 0 )
		perror("write buffer");
	fxt_fdevents_stop(fut, fd);

	if( close(fd) < 0 )
		perror(filename);

	//fut_done(); // Removed by Raymond

	return nlongs;
	}
