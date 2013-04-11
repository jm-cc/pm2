/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2010 "the PM2 team" (see AUTHORS file)
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

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <tbx.h>

#define ITER_SMALL 10000
#define ITER_LARGE 10
#define MAX_SIZE   (128*1024*1024)

static const char*checksums[] =
  { 
#ifdef __SSE2__
    "xor", 
#endif /* __SSE2__ */
    "plain", 
    "block64",
    "adler",
    "fletcher",
    "jenkins",
    "fnv1a",
    "knuth",
    "murmurhash2a",
    "murmurhash64a",
    "hsieh",
    "softcrc"
#ifdef __SSE4_2__
    ,"crc"
#endif /* __SSE4_2__ */
  };

int main(int argc, char *argv[])
{
	tbx_tick_t t1, t2;
	char *buffer, *buf2;
	double best, time_usec;
	uint32_t checksum;
	int i, j;

	if (! (buffer = (char *)malloc(MAX_SIZE)))
		return EXIT_FAILURE;
	buf2 = malloc(MAX_SIZE);
	for(i = 0; i < MAX_SIZE; i++)
		buffer[i] = (char)rand();

	tbx_init(&argc, &argv);

	/* ** asymptotic behavior */
	printf("\n# ## in memory (128 MB block)\n");
	for (i = 0; i < sizeof(checksums)/sizeof(char*); i++)
	  {
	    printf("# %s\n", checksums[i]);
	    tbx_checksum_t c = tbx_checksum_get(checksums[i]);
	    checksum = 0;
	    best = DBL_MAX;
	    for (j = 0; j < ITER_LARGE; j++)
	      {
		TBX_GET_TICK(t1);
		checksum = (*c->func)(buffer, MAX_SIZE);
		TBX_GET_TICK(t2);
		time_usec = TBX_TIMING_DELAY(t1, t2);
		if(time_usec < best)
		  best = time_usec;
		}
	    printf("  checksum = %08x; time = %6.2f usec.; %6.2f MB/s; %6.3f nsec/word\n",
		   (unsigned)checksum, best, MAX_SIZE/best, (1000.0*best*8)/MAX_SIZE);
	}
	/* ** in cache */
	printf("\n# ## in cache (64 kB block)\n");
	for (i = 0; i < sizeof(checksums)/sizeof(char*); i++)
	  {
	    const size_t small_size = 64 * 1024;
	    printf("# %s\n", checksums[i]);
	    tbx_checksum_t c = tbx_checksum_get(checksums[i]);
	    best = DBL_MAX;
	    for (j = 0; j < ITER_SMALL; j++) 
	      {
		TBX_GET_TICK(t1);
		checksum = (*c->func)(buffer, small_size);
		TBX_GET_TICK(t2);
		time_usec = TBX_TIMING_DELAY(t1, t2);
		if(time_usec < best)
		  best = time_usec;
	      }
	    printf("  checksum = %08x; time = %6.2f usec.; %6.2f MB/s; %6.3f nsec/word\n",
		   (unsigned)checksum, best, small_size/best, (1000.0*best*8)/small_size);
	}
	/* ** memcpy, checksum */
	printf("\n# ## memcpy, checksum- 128MB block\n");
	for (i = 0; i < sizeof(checksums)/sizeof(char*); i++)
	  {
	    tbx_checksum_t c = tbx_checksum_get(checksums[i]);
	    printf("# ## %s\n", checksums[i]);
	    printf("#  size (byte) | time (usec.) | BW (MB/s) | nsec/word | (checksum value)\n");
	    ssize_t copy_size;
	    for(copy_size = 16*1024; copy_size <= MAX_SIZE; copy_size *= 2)
	      {
		best = DBL_MAX;
		for (j = 0; j < ITER_LARGE; j++) 
		  {
		    TBX_GET_TICK(t1);
		    memcpy(buf2, buffer, copy_size);
		    checksum = (*c->func)(buffer, copy_size);
		    TBX_GET_TICK(t2);
		    time_usec = TBX_TIMING_DELAY(t1, t2);
		    if(time_usec < best)
		      best = time_usec;
		  }
		printf("%8d \t %6.2f \t %6.2f \t %6.3f \t %08x\n",
		       (int)copy_size, best, copy_size/best, (1000.0*best*8)/MAX_SIZE, (unsigned)checksum);
	      }
	  }
	/* ** checksum and copy */
	printf("\n# ## checksum_and_copy- 128MB block\n");
	for (i = 0; i < sizeof(checksums)/sizeof(char*); i++)
	  {
	    tbx_checksum_t c = tbx_checksum_get(checksums[i]);
	    if(c->checksum_and_copy != NULL)
	      {
		printf("# ## %s\n", checksums[i]);
		printf("#  size (byte) | time (usec.) | BW (MB/s) | nsec/word | (checksum value)\n");
		ssize_t copy_size;
		for(copy_size = 16*1024; copy_size <= MAX_SIZE; copy_size *= 2)
		  {
		    best = DBL_MAX;
		    for (j = 0; j < ITER_LARGE; j++) 
		      {
			TBX_GET_TICK(t1);
			checksum = (*c->checksum_and_copy)(buf2, buffer, copy_size);
			TBX_GET_TICK(t2);
			time_usec = TBX_TIMING_DELAY(t1, t2);
			if(time_usec < best)
			  best = time_usec;
		      }
		    printf("%8d \t %6.2f \t %6.2f \t %6.3f \t %08x\n",
			   (int)copy_size, best, copy_size/best, (1000.0*best*8)/MAX_SIZE, (unsigned)checksum);
		  }
	      }
	  }
	
	tbx_exit();
	free(buffer);

	return 0;
}
