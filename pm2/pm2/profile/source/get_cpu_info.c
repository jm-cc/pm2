/*	get_cpu_info.c	*/
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

#include "get_cpu_info.h"

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
#define PROC_FILE	"/proc/cpuinfo"

int get_cpu_info( int maxn, double mhz[] )
	{
	FILE	*fptr;
	char	*ptr, buffer[BUF_SIZE];
	double	cpu_mhz;
	int		n;

	n = 0;
	if( (fptr=fopen(PROC_FILE, "r")) == NULL )
		{
		fprintf(stderr, "unable to open %s\n", PROC_FILE);
		exit(EXIT_FAILURE);
		}

	for( ; ; )
		{/* once around loop for each line in the file */
		if( fgets(buffer, BUF_SIZE, fptr) == NULL )
			{
			if( feof(fptr) )
				{/* end of the file, did we find any cpus? */
				fclose(fptr);
				if( n <= 0 )
					/* no, end of file was premature */
					fprintf(stderr, "end of file on %s\n", PROC_FILE);
				else
					/* yes, normal return with that number */
					return n;
				}
			else
				fprintf(stderr, "unable to read %s\n", PROC_FILE);
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
			if( n >= maxn )
				fprintf(stderr,"too many cpus found, max of %d allowed\n",maxn);
			else
				{
				mhz[n] = cpu_mhz;
				n++;
				}
			}
		}
	}

#endif // LINUX_SYS


#ifdef SOLARIS_SYS

#include <unistd.h>
#include <sys/types.h>
#include <sys/processor.h>


int get_cpu_info( int maxn, double mhz[] )
{
  processor_info_t info;
  processorid_t n,cpuid_max;

  cpuid_max = sysconf(_SC_CPUID_MAX);

  if (cpuid_max>=maxn) {
    fprintf(stderr,"too many cpus found, max of %d allowed\n",maxn);
    cpuid_max=maxn-1;
  }

  for (n=0;n<=cpuid_max;n++) {
    if (processor_info(n, &info)<0) {
      perror("processor_info");
      fprintf(stderr,"processor %d\n", n);
      exit(EXIT_FAILURE);
    }
    mhz[n] = (double)info.pi_clock;
  }
  return n;
}

#endif // SOLARIS_SYS

