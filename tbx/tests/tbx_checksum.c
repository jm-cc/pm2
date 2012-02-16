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
    "xor", 
    "plain", 
    "block64",
    "adler",
    "fletcher",
    "jenkins",
    "fnv1a",
    "knuth",
    "murmurhash2a",
    "murmurhash64a",
    "hsieh"
#ifdef __SSE4_2__
    ,"crc"
#endif /* __SSE4_2__ */
  };

int main(int argc, char *argv[])
{
	tbx_tick_t t1, t2;
	char *buffer;
	double best, time_usec;
	uint32_t checksum;
	int i, j;

	if (! (buffer = (char *)malloc(MAX_SIZE)))
		return EXIT_FAILURE;
	for(i = 0; i < MAX_SIZE; i++)
		buffer[i] = (char)rand();

	tbx_init(&argc, &argv);

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
	
	tbx_exit();
	free(buffer);

	return 0;
}
