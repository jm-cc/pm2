/*	get_cpu_mhz.c	*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "get_cpu_mhz.h"

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
