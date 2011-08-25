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

static const struct tbx_checksum_s
{
	const char*name;
	uint32_t (*func)(const void*, size_t);
} checksums[] =
{
	{ .name = "xor32",         .func = &tbx_checksum_xor32 },
	{ .name = "plain32",       .func = &tbx_checksum_plain32 },
	{ .name = "block64",       .func = &tbx_checksum_block64 },
	{ .name = "Adler32",       .func = &tbx_checksum_adler32 },
	{ .name = "Fletcher64",    .func = &tbx_checksum_fletcher64 },
	{ .name = "Jenkins one-at-a-time", .func = &tbx_checksum_jenkins },
	{ .name = "FNV1a",         .func = &tbx_checksum_fnv1a },
	{ .name = "Knuth",         .func = &tbx_checksum_knuth },
	{ .name = "MurmurHash2a",  .func = &tbx_checksum_murmurhash2a },
	{ .name = "MurmurHash64a", .func = &tbx_checksum_murmurhash64a },
	{ .name = "Paul Hsieh SuperFast", .func = &tbx_checksum_hsieh },
#ifdef __SSE4_2__
	{ .name = "SSE4.2 CRC32",  .func = &tbx_checksum_crc32 },
#endif
	{ .name = NULL, .func = NULL }
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
	for (i = 0; checksums[i].name; i++) {
		printf("# %s\n", checksums[i].name);
		checksum = 0;
		best = DBL_MAX;
		for (j = 0; j < ITER_LARGE; j++) {
			TBX_GET_TICK(t1);
			checksum = (*checksums[i].func)(buffer, MAX_SIZE);
			TBX_GET_TICK(t2);
			time_usec = TBX_TIMING_DELAY(t1, t2);
			if(time_usec < best)
				best = time_usec;
		}
		printf("  checksum = %08x; time = %6.2f usec.; %6.2f MB/s; %6.3f nsec/word\n",
		       (unsigned)checksum, best, MAX_SIZE/best, (1000.0*best*8)/MAX_SIZE);
	}

	printf("\n# ## in cache (64 kB block)\n");
	for (i = 0; checksums[i].name; i++) {
		const size_t small_size = 64 * 1024;
		printf("# %s\n", checksums[i].name);
		checksum = (*checksums[i].func)(buffer, small_size);
		best = DBL_MAX;
		for (j = 0; j < ITER_SMALL; j++) {
			TBX_GET_TICK(t1);
			checksum = (*checksums[i].func)(buffer, small_size);
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
