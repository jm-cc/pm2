
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

#ifdef USE_FKT

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sendfile.h>
#include <sys/mount.h>
#include <sys/vfs.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "fkt.h"
#include <fkt/get_cpu_info.h>
#include <fkt/sysmap.h>
#include <fkt/block.h>
#include <fkt/names.h>
#include <fkt/pids.h>
#include "fkt-tools.h"

#include "pm2_profile.h"
#include "fkt_pm2.h"

#ifndef WHITE_SPACE
#define WHITE_SPACE	" \t\n\f\v\r"
#endif

#define FKT_DEV "/dev/fkt"
#define MAXCPUS	16

#define UNAMESTRLEN 256

static clock_t	start_jiffies, stop_jiffies;
static time_t	start_time, stop_time;
static int fd,fkt=-1;
static pid_t pid,kpid;
static size_t size;
static size_t page_size;

struct code_list_item	{
  unsigned int code;
  char *name;
  struct code_list_item	*link;
};

static void record_time( int ncpus, double mhz[] )
{
  int i;

  BEGIN_BLOCK(fd);
  if( write(fd, (void *)&ncpus, sizeof(ncpus)) < 0 )
    perror("write ncpus");
  for( i = 0;  i < ncpus;  i++ ) {
    if( write(fd, (void *)&mhz[i], sizeof(mhz[i])) < 0 )
      perror("write mhz");
  }
  if( write(fd, (void *)&pid, sizeof(pid)) < 0 )
    perror("write pid");
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
  if( write(fd, (void *)&page_size, sizeof(page_size)) < 0 )
    perror("write page_size");
  END_BLOCK(fd);
}

void fkt_keychange(int how, unsigned int keymask) {
  int res;
  if (fkt<0) {
    fprintf(stderr,"keychange: fkt file not opened\n");
    return;
  }
  switch (how) {
  case FKT_DISABLE: keymask&=~FKT_KEYMASK28; break;
  case FKT_ENABLE: case FKT_SETMASK: keymask|=FKT_KEYMASK28; break;
  }
  if ((res=ioctl(fkt, how, keymask))<0)
    fprintf(stderr,"fkt_keychange(%x,%x):%d:%s\n",how,keymask,res,strerror(errno));
}

int fkt_record(char *file, unsigned int initmask, int powpages, int dma)
{
  double mhz[MAXCPUS];
  int ncpus;
  struct statfs statfs;
  struct stat   st;
  off_t off, dummyoff=0;
  struct utsname unameinfo;
  char unamestr[UNAMESTRLEN];
  unsigned int len;
  ssize_t ret;
  static const unsigned zero = 0;

  if (fkt<0) {
    if ((fkt=open(FKT_DEV, O_RDONLY|O_NONBLOCK))<0) {
      perror("open(\"" FKT_DEV "\")");
      return -1;
    }
  }

  ncpus = fkt_get_cpu_info(MAXCPUS, mhz);
  page_size = getpagesize();
  kpid = (unsigned long) getpid();

  if ((fd = open(file, O_RDWR|O_CREAT|O_TRUNC|O_EXCL, 0666)) < 0) {
    if (errno == EEXIST) {
      fprintf(stderr, "%s already exists\nSkipping recording\n", file);
      close(fkt);
      fkt=-1;
      return -1;
    }
    perror(file);
    exit(EXIT_FAILURE);
  }
  fprintf(stderr, "%s opened\n", file);

  if (!size) {
    if( fstat(fd, &st) ) {
      perror("getting attributes of trace file");
      exit(EXIT_FAILURE);
    }
    if( S_ISBLK(st.st_mode) ) {
      int sectors;
      printf("block device\n");
      if( ioctl(fd,BLKGETSIZE,&sectors) ) {
        perror("getting device size\n");
        exit(EXIT_FAILURE);
      }
      if( sectors >= 0xffffffffUL >> 9 )
        size=0xffffffffUL;
      else
        size = sectors << 9;
    } else {
      if( fstatfs(fd, &statfs) ) {
        perror("getting file system size\n");
        exit(EXIT_FAILURE);
      }
      if( statfs.f_bavail >= 0xffffffffUL / statfs.f_bsize )
        size = 0xffffffffUL;
      else
        size = statfs.f_bsize * statfs.f_bavail;
    }
    printf("%uM\n",size>>20);
  }

  if (ioctl(fkt, FKT_SETINITMASK, initmask|FKT_KEYMASK28) < 0) {
    perror("setting initial mask");
    exit(EXIT_FAILURE);
  }

  if (powpages)
  if (ioctl(fkt, FKT_SETINITPOWPAGES, powpages) < 0) {
    perror("setting initial number of pages");
    exit(EXIT_FAILURE);
  }

  if (dma)
  if (ioctl(fkt, FKT_SETTRYDMA, dma) < 0)
    perror("asking for trying to get dma");

  dumptime(&start_time, &start_jiffies);

  BEGIN_BLOCK(fd);

  record_time( ncpus, mhz );
  BEGIN_BLOCK(fd);
  fkt_record_irqs(fd);
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

  /* merge of System.map and /proc/ksyms */
  if( !stat("/proc/kallsyms",&st) )
    {/* on >=2.5 kernels, kallsyms gives everything we may want */
    //fkt_get_sysmap("/proc/kallsyms",SYSMAP_MODLIST,0);
    fkt_get_sysmap("/proc/kallsyms",SYSMAP_PROC,0);
    }
  else
    {/* but on 2.4 kernels, only ksyms is available */
    fkt_get_sysmap("/proc/ksyms",SYSMAP_MODLIST,0);
    fkt_get_sysmap("/proc/ksyms",SYSMAP_PROC,0);
    }

  BEGIN_BLOCK(fd);
  fkt_record_sysmap(fd);
  END_BLOCK(fd);

  BEGIN_BLOCK(fd);
  fkt_record_pids(fd);
  END_BLOCK(fd);

  if( write(fd,&zero,sizeof(zero)) < (ssize_t) sizeof(zero) )
    {
    perror("ending the main block");
    exit(EXIT_FAILURE);
    }

  /* seek to a page boundary */
  if( (off = lseek(fd, 0, SEEK_CUR)) < 0 )
    {
    perror("getting seek");
    exit(EXIT_FAILURE);
    }
  off = (off+page_size-1)&(~(page_size-1));

  if( (dummyoff = lseek(fd, off, SEEK_SET)) != off )
    {
    perror("seeking to a page boundary");
    exit(EXIT_FAILURE);
    }
  /* and close the main block */
  END_BLOCK(fd);

  /* ok, can now fork */
  if (!(pid=fork())) {
  /* for blocking in sendfile() */
    pid=getpid();
    if ((ret=sendfile(fd, fkt, &dummyoff, size))<0) {
      perror("sendfile");
      fprintf(stderr,"hoping the trace wasn't lost\n");
    }
    dumptime(&stop_time, &stop_jiffies);

    if ( lseek(fd, 0, SEEK_SET) < 0 ) {
      perror("seeking back to the beginning");
      exit(EXIT_FAILURE);
    }
    /* rewrite time. beware that record_time's record size shouldn't change in
     * the meanwhile ! */
    ENTER_BLOCK(fd);
    record_time(ncpus, mhz);
    LEAVE_BLOCK(fd);

    close(fd);
    exit(EXIT_SUCCESS);
  }

  /* wait for sendfile to be ready */
  if (ioctl(fkt, FKT_WAITREADY)<0)
    perror("FKT_WAITREADY");

  close(fd);
  return 0;
}

void fkt_stop(void) {
  kill(pid,SIGTERM);
  waitpid(pid,NULL,0);
}

int fkt_new_lwp(unsigned int thread_num, unsigned int lwp_logical_num) {
	unsigned long entries [] = { FKT_USER_FORK_CODE, thread_num, lwp_logical_num };
	if (__pm2_profile_active) {
		return ioctl(fkt, FKT_USER_PROBE2, entries);
	}
	return 0;
}


#endif // USE_FKT
