
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

#ifndef X86_ARCH
#error "PROFILE FACILITIES ARE CURRENTLY ONLY AVAILABLE ON X86 ARCHITECTURES"
#endif

#ifdef USE_FKT

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <sys/times.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <linux/fkt.h>
#include "pm2_profile.h"
#include "fkt_head.h"
#include "fkt_pm2.h"

#define MAXCPUS	16

#ifndef WHITE_SPACE
#define WHITE_SPACE	" \t\n\f\v\r"
#endif

struct code_list_item	{
  unsigned int code;
  char *name;
  struct code_list_item	*link;
};

static clock_t	start_jiffies = 0, stop_jiffies = 0;
static time_t	start_time = 0, stop_time = 0;

static void dumptime(time_t *the_time, clock_t *the_jiffies)
{
  struct tms cur_time;

  if( (*the_time = time(NULL)) == -1 )
    perror("time");

  if( (*the_jiffies = times(&cur_time)) < 0 )
    perror("times");
}

void fkt_start_time(void)
{
  dumptime(&start_time, &start_jiffies);
}

void fkt_stop_time(void)
{
  dumptime(&stop_time, &stop_jiffies);
}

static int get_cpu_info(int maxn, double mhz[])
{
  FILE	*fptr;
  char	*ptr, buffer[4096];
  double	cpu_mhz;
  int		n;

  n = 0;
  if( (fptr=fopen("/proc/cpuinfo", "r")) == NULL ) {
    fprintf(stderr, "unable to open %s\n", "/proc/cpuinfo");
    exit(EXIT_FAILURE);
  }

  for( ; ; )
    {/* once around loop for each line in the file */
      if( fgets(buffer, 4096, fptr) == NULL )
	{
	  if( feof(fptr) )
	    {/* end of the file, did we find any cpus? */
	      fclose(fptr);
	      if( n <= 0 )
		/* no, end of file was premature */
		fprintf(stderr, "end of file on %s\n", "/proc/cpuinfo");
	      else
		/* yes, normal return with that number */
		return n;
	    }
	  else
	    fprintf(stderr, "unable to read %s\n", "/proc/cpuinfo");
	  exit(EXIT_FAILURE);
	}
      /***** fputs(buffer, stdout); *****/
      if( strncmp(buffer, "cpu MHz", strlen("cpu MHz")) == 0 )
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

static void record_irqs(int fd)
{
  FILE					*table;
  struct code_list_item	*ptr, *irq_list = NULL;
  char					*lptr, *xptr, line[128];
  unsigned int			n, i, nirqs, ncpus;


  nirqs = 0;
  ncpus = 0;
  if( (table = fopen("/proc/interrupts", "r")) == NULL )
    {
      fprintf(stderr, "unable to open /proc/interrupts\n");
      write(fd, (void *)&nirqs, sizeof(nirqs));
      write(fd, (void *)&ncpus, sizeof(ncpus));
      return;
    }

  /*	first line of file is headers of form CPUn */
  if( fgets(line, 128, table) == NULL )
    {
      fprintf(stderr, "unable to read /proc/interrupts\n");
      write(fd, (void *)&nirqs, sizeof(nirqs));
      write(fd, (void *)&ncpus, sizeof(ncpus));
      fclose(table);
      return;
    }
	
  for( lptr = line;  strtok(lptr, WHITE_SPACE) != NULL;  ncpus++ )
    lptr = NULL;
  //printf("       : ncpus %5u\n", ncpus);

  /*	remaining lines of file are the IRQs */
  nirqs = 0;
  while( fgets(line, 128, table) != NULL )
    {/* build circular list with 1 item for each interrupt */
      n = strtoul(line, &lptr, 10);	/* scan off leading number */
      if( *lptr != ':' )				/* which must end with : */
	continue;					/* or it is not an IRQ number */
      ptr = (struct code_list_item *)malloc(sizeof(struct code_list_item));
      ptr->code = n;
      
      /*	scan over the interrupt counts, one per cpu, and controller type */
      xptr = lptr;
      for( lptr++, i=0;  i<=ncpus && (xptr = strtok(lptr, WHITE_SPACE))!=NULL;
	   i++ )
	lptr = NULL;
      
      /*	get to the leading character of the interrupt name */
      xptr += strlen(xptr) + 1;
      xptr += strspn(xptr, WHITE_SPACE);
      
      i = strlen(xptr);
      ptr->name = malloc(i);
      strncpy(ptr->name, xptr, i);
      ptr->name[i-1] = '\0';
      if( irq_list == NULL )
	ptr->link = ptr;
      else
	{
	  ptr->link = irq_list->link;
	  irq_list->link = ptr;
	}
      irq_list = ptr;
      //printf(" irq %2u: %s\n", ptr->code, ptr->name);
      nirqs++;
    }
  //printf("       : nirqs %4u\n", nirqs);
  //printf("       : ncpus %4u\n", ncpus);
  fclose(table);
  write(fd, (void *)&nirqs, sizeof(nirqs));
  write(fd, (void *)&ncpus, sizeof(ncpus));
  if( (ptr = irq_list) != NULL )
    {/* make circular list into singly-linked, NULL-terminated list */
      irq_list = irq_list->link;
      ptr->link = NULL;
    }
  for( ptr = irq_list;  ptr != NULL;  ptr = ptr->link )
    {
      write(fd, (void *)&ptr->code, sizeof(ptr->code));
      i = strlen(ptr->name)+1;
      write(fd, (void *)&i, sizeof(i));
      i = (i + 3) & ~3;
      write(fd, ptr->name, i);
    }
}

void fkt_record(char *file)
{
  int i, nints;
  unsigned long pid, kpid;
  int max_nints = 0;
  double mhz[MAXCPUS];
  int ncpus;
  int fd;
  unsigned int	*buffer = NULL;

  fkt_keychange(FKT_DISABLE, FKT_KEYMASKALL);
  fkt_stop_time();

  ncpus = get_cpu_info(MAXCPUS, mhz);

  errno = 0;
  while( (nints = fkt_getcopy(0, NULL)) < 0  &&  errno == EAGAIN )
    errno = 0;

  if( (buffer = malloc(nints*4)) == NULL ) {
    fprintf(stderr, "unable to malloc %d ints\n", nints);
    exit(EXIT_FAILURE);
  }

  errno = 0;
  while( (nints = fkt_getcopy(nints, buffer)) < 0  &&  errno == EAGAIN )
    errno = 0;

  fd = open(file, O_WRONLY|O_CREAT|O_TRUNC, 0666);
  if(fd == -1)
    perror("open");

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
  if( write(fd, (void *)&start_time, sizeof(start_time)) < 0 )
    perror("write start_time");
  if( write(fd, (void *)&stop_time, sizeof(stop_time)) < 0 )
    perror("write stop_time");
  if( write(fd, (void *)&start_jiffies, sizeof(start_jiffies)) < 0 )
    perror("write start_jiffies");
  if( write(fd, (void *)&stop_jiffies, sizeof(stop_jiffies)) < 0 )
    perror("write stop_jiffies");
  record_irqs(fd);
  if( write(fd, (void *)&max_nints, sizeof(max_nints)) < 0 )
    perror("write max_nints");
  if( write(fd, (void *)&nints, sizeof(nints)) < 0 )
    perror("write nints");
  if( write(fd, (void *)buffer, nints*4) < 0 )
    perror("write buffer");

  close(fd);

  fkt_endup();
}

#endif // USE_FKT
