/*	get_cpu_mhz.c	*/
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

#include "get_cpu_mhz.h"

#ifndef X86_ARCH
#error "PROFILE FACILITIES ARE CURRENTLY ONLY AVAILABLE ON X86 ARCHITECTURES"
#endif


#ifdef LINUX_SYS

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUF_SIZE	4096
#define TAG			"cpu MHz"

double get_cpu_mhz( void )
	{
	FILE	*fptr;
	char	*ptr, buffer[BUF_SIZE];
	double	cpu_mhz;

	if( (fptr=fopen("/proc/cpuinfo", "r")) == NULL )
		{
		fprintf(stderr, "unable to open /proc/cpuinfo\n");
		exit(EXIT_FAILURE);
		}

	for( ; ; )
		{
		if( fgets(buffer, BUF_SIZE, fptr) == NULL )
			{
			if( feof(fptr) )
				fprintf(stderr, "end of file on /proc/cpuinfo\n");
			else
				fprintf(stderr, "unable to read /proc/cpuinfo\n");
			exit(EXIT_FAILURE);
			}
		/***** fputs(buffer, stdout); *****/
		if( strncmp(buffer, TAG, strlen(TAG)) == 0 )
			{/* found the line starting with the tag */
			/***** fprintf(stderr, "found line starting with %s\n", TAG); *****/
			if( (ptr = strchr(buffer, ':')) == NULL )
				{
				fprintf(stderr, "cannot find ':' in %s\n", buffer);
				exit(EXIT_FAILURE);
				}
			if( sscanf(ptr+1, "%lf", &cpu_mhz) != 1 )
				{
				fprintf(stderr, "cannot find number after ':' in %s\n", buffer);
				exit(EXIT_FAILURE);
				}
			/***** fprintf(stderr, "found cpu mhz %.2f\n", cpu_mhz); *****/
			fclose(fptr);
			return cpu_mhz;
			}
		}
	}

#endif // LINUX_SYS


#ifdef SOLARIS_SYS

#include <unistd.h>
#include <sys/types.h>
#include <sys/processor.h>


double get_cpu_mhz( void )
{
  processor_info_t info;

  processor_info(0, &info);

  return (double)info.pi_clock;

}

#endif // SOLARIS_SYS

