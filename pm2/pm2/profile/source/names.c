/*	record_irqs.c	*/
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

#define FUT

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fkt.h"
#include "names.h"
#include "sysmap.h"

/*
 * IRQs
 */

struct code_list_item	{
	unsigned int			code;
	char					*name;
	struct code_list_item	*link;
};

static struct code_list_item	*irq_list;
unsigned int nirqs;

#define LINE_SIZE	128

#ifndef WHITE_SPACE
#define WHITE_SPACE	" \t\n\f\v\r"
#endif

void record_irqs(int fd)
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

void load_irqs( int fd )
	{
	struct code_list_item	*ptr;
	unsigned int		i, k, n;
	unsigned int			ncpus;


	if( read(fd, (void *)&nirqs, sizeof(nirqs)) <= 0 )
		{
		perror("read nirqs");
		exit(EXIT_FAILURE);
		}
	printf("%14s = %u\n", "nirqs", nirqs);
	if( read(fd, (void *)&ncpus, sizeof(ncpus)) <= 0 )
		{
		perror("read ncpus");
		exit(EXIT_FAILURE);
		}
	printf("%14s = %u\n", "ncpus", ncpus);

	for( n = 0;  n < nirqs;  n++ )
		{/* build circular list with 1 item for each interrupt */
		ptr = (struct code_list_item *)malloc(sizeof(struct code_list_item));
		read(fd, (void *)&ptr->code, sizeof(ptr->code));
		read(fd, (void *)&i, sizeof(i));
		k = (i + 3) & ~3;
		ptr->name = malloc(k);
		read(fd, (void *)ptr->name, k);
		ptr->name[i-1] = '\0';
		if( irq_list == NULL )
			ptr->link = ptr;
		else
			{
			ptr->link = irq_list->link;
			irq_list->link = ptr;
			}
		irq_list = ptr;
		printf("irq %2u: %s\n", ptr->code, ptr->name);
		}
	printf("\n");
	if( (ptr = irq_list) != NULL )
		{/* make circular list into singly-linked, NULL-terminated list */
		irq_list = irq_list->link;
		ptr->link = NULL;
		}
	}

char *find_irq( unsigned int code )
	{
	struct code_list_item	*ptr;
	static char buf[32];

	for( ptr = irq_list;  ptr != NULL;  ptr = ptr->link )
		{
		if( ptr->code == code )
			return ptr->name;
		}
	sprintf(buf,"unknown irq %d",code);
	return buf;
	}

/*
 * Traps, sysIRQs and syscalls
 */

char	*traps[NTRAPS+1] =	{
	"divide_error",					/*  0 */
	"debug",						/*  1 */
	"nmi",							/*  2 */
	"int3",							/*  3 */
	"overflow",						/*  4 */
	"bounds",						/*  5 */
	"invalid_op",					/*  6 */
	"device_not_available",			/*  7 */
	"double_fault",					/*  8 */
	"coprocessor_seg_overrun",		/*  9 */
	"invalid_TSS",					/* 10 */
	"segment_not_present",			/* 11 */
	"stack_segment",				/* 12 */
	"general_protection",			/* 13 */
	"page_fault",					/* 14 */
	"spurious_interrupt_bug",		/* 15 */
	"coprocessor_error",			/* 16 */
	"alignment_check",				/* 17 */
	"machine_check"					/* 18 */
	"simd_coprocessor_error",		/* 19 */
	"unknown trap",					/* 20 */
};

char	*sys_irqs[NSYS_IRQS] = {
	"local timer",			/* 0xef */
	"thermal APIC",			/* 0xf0 */
	"reserved f1",			/* 0xf1 */
	"reserved f2",			/* 0xf2 */
	"reserved f3",			/* 0xf3 */
	"reserved f4",			/* 0xf4 */
	"reserved f5",			/* 0xf5 */
	"reserved f6",			/* 0xf6 */
	"reserved f7",			/* 0xf7 */
	"reserved f8",			/* 0xf8 */
	"reserved f9",			/* 0xf9 */
	"reserved fa",			/* 0xfa */
	"call function",		/* 0xfb */
	"reschedule",			/* 0xfc */
	"invalidate TLB",		/* 0xfd */
	"error APIC",			/* 0xfe */
	"spurious APIC",		/* 0xff */
};

char	*sys_calls[NSYS_CALLS+1] = {
	"sys_setup",					/* 0 */
	"sys_exit",
	"sys_fork",
	"sys_read",
	"sys_write",
	"sys_open",						/* 5 */
	"sys_close",
	"sys_waitpid",
	"sys_creat",
	"sys_link",
	"sys_unlink",					/* 10 */
	"sys_execve",
	"sys_chdir",
	"sys_time",
	"sys_mknod",
	"sys_chmod",					/* 15 */
	"sys_chown",
	"sys_break",
	"sys_stat",
	"sys_lseek",
	"sys_getpid",					/* 20 */
	"sys_mount",
	"sys_umount",
	"sys_setuid",
	"sys_getuid",
	"sys_stime",					/* 25 */
	"sys_ptrace",
	"sys_alarm",
	"sys_fstat",
	"sys_pause",
	"sys_utime",					/* 30 */
	"sys_stty",
	"sys_gtty",
	"sys_access",
	"sys_nice",
	"sys_ftime",					/* 35 */
	"sys_sync",
	"sys_kill",
	"sys_rename",
	"sys_mkdir",
	"sys_rmdir",					/* 40 */
	"sys_dup",
	"sys_pipe",
	"sys_times",
	"sys_prof",
	"sys_brk",						/* 45 */
	"sys_setgid",
	"sys_getgid",
	"sys_signal",
	"sys_geteuid",
	"sys_getegid",					/* 50 */
	"sys_acct",
	"sys_phys",
	"sys_lock",
	"sys_ioctl",
	"sys_fcntl",					/* 55 */
	"sys_mpx",
	"sys_setpgid",
	"sys_ulimit",
	"sys_olduname",
	"sys_umask",					/* 60 */
	"sys_chroot",
	"sys_ustat",
	"sys_dup2",
	"sys_getppid",
	"sys_getpgrp",					/* 65 */
	"sys_setsid",
	"sys_sigaction",
	"sys_sgetmask",
	"sys_ssetmask",
	"sys_setreuid",					/* 70 */
	"sys_setregid",
	"sys_sigsuspend",
	"sys_sigpending",
	"sys_sethostname",
	"sys_setrlimit",				/* 75 */
	"sys_getrlimit",
	"sys_getrusage",
	"sys_gettimeofday",
	"sys_settimeofday",
	"sys_getgroups",				/* 80 */
	"sys_setgroups",
	"old_select",
	"sys_symlink",
	"sys_lstat",
	"sys_readlink",					/* 85 */
	"sys_uselib",
	"sys_swapon",
	"sys_reboot",
	"old_readdir",
	"old_mmap",						/* 90 */
	"sys_munmap",
	"sys_truncate",
	"sys_ftruncate",
	"sys_fchmod",
	"sys_fchown",					/* 95 */
	"sys_getpriority",
	"sys_setpriority",
	"sys_profil",
	"sys_statfs",
	"sys_fstatfs",					/* 100 */
	"sys_ioperm",
	"sys_socketcall",
	"sys_syslog",
	"sys_setitimer",
	"sys_getitimer",				/* 105 */
	"sys_newstat",
	"sys_newlstat",
	"sys_newfstat",
	"sys_uname",
	"sys_iopl",						/* 110 */
	"sys_vhangup",
	"sys_idle",
	"sys_vm86",
	"sys_wait4",
	"sys_swapoff",					/* 115 */
	"sys_sysinfo",
	"sys_ipc",
	"sys_fsync",
	"sys_sigreturn",
	"sys_clone",					/* 120 */
	"sys_setdomainname",
	"sys_newuname",
	"sys_modify_ldt",
	"sys_adjtimex",
	"sys_mprotect",					/* 125 */
	"sys_sigprocmask",
	"sys_create_module",
	"sys_init_module",
	"sys_delete_module",
	"sys_get_kernel_syms",			/* 130 */
	"sys_quotactl",
	"sys_getpgid",
	"sys_fchdir",
	"sys_bdflush",
	"sys_sysfs",					/* 135 */
	"sys_personality",
	"afs_syscall",
	"sys_setfsuid",
	"sys_setfsgid",
	"sys_llseek",					/* 140 */
	"sys_getdents",
	"sys_select",
	"sys_flock",
	"sys_msync",
	"sys_readv",					/* 145 */
	"sys_writev",
	"sys_getsid",
	"sys_fdatasync",
	"sys_sysctl",
	"sys_mlock",					/* 150 */
	"sys_munlock",
	"sys_mlockall",
	"sys_munlockall",
	"sys_sched_setparam",
	"sys_sched_getparam",			/* 155 */
	"sys_sched_setscheduler",
	"sys_sched_getscheduler",
	"sys_sched_yield",
	"sys_sched_get_priority_max",
	"sys_sched_get_priority_min",	/* 160 */
	"sys_sched_rr_get_interval",
	"sys_nanosleep",
	"sys_mremap",
	"sys_setresuid",
	"sys_getresuid",				/* 165 */
	"sys_vm86",
	"sys_query_module",
	"sys_poll",
	"sys_nfsservctl",
	"sys_setresgid",				/* 170 */
	"sys_getresgid",
	"sys_prctl",
	"sys_rt_sigreturn",
	"sys_rt_sigaction",
	"sys_rt_sigprocmask",			/* 175 */
	"sys_rt_sigpending",
	"sys_rt_sigtimedwait",
	"sys_rt_sigqueueinfo",
	"sys_rt_sigsuspend",
	"sys_pread",					/* 180 */
	"sys_pwrite",
	"sys_chown",
	"sys_getcwd",
	"sys_capget",
	"sys_capset",					/* 185 */
	"sys_sigaltstack",
	"sys_sendfile",
	"sys_ni_syscall",				/* streams1 */
	"sys_ni_syscall",				/* streams2 */
	"sys_vfork",					/* 190 */
	"sys_getrlimit",
	"sys_mmap2",
	"sys_truncate64",
	"sys_ftruncate64",
	"sys_stat64",					/* 195 */
	"sys_lstat64",
	"sys_fstat64",
	"sys_lchown",
	"sys_getuid",
	"sys_getgid",					/* 200 */
	"sys_geteuid",
	"sys_getegid",
	"sys_setreuid",
	"sys_setregid",
	"sys_getgroups",				/* 205 */
	"sys_setgroups",
	"sys_fchown",
	"sys_setresuid",
	"sys_getresuid",
	"sys_setresgid",				/* 210 */
	"sys_getresgid",
	"sys_chown",
	"sys_setuid",
	"sys_setgid",
	"sys_setfsuid",					/* 215 */
	"sys_setfsgid",
	"sys_pivot_root",
	"sys_mincore",
	"sys_madvise",
	"sys_getdents64",				/* 220 */
	"sys_fcntl64",
	"sys_ni_syscall",				/* reserved for TUX */
	"sys_ni_syscall",				/* reserved for Security */
	"sys_gettid",
	"sys_readahead",				/* 225 */
	"sys_lsetxattr",
	"sys_fsetxattr",
	"sys_getxattr",
	"sys_lgetxattr",				/* 230 */
	"sys_fgetxattr",
	"sys_listxattr",
	"sys_llistxattr",
	"sys_flistxattr",
	"sys_removexattr",				/* 235 */
	"sys_lremovexattr",
	"sys_fremovexattr",
	"sys_tkill",
	"sys_sendfile64",
	"sys_ni_syscall",				/* 240 reserved for futex */
	"sys_sched_setaffinity",
	"sys_sched_getaffinity",
	"sys_set_thread_area",
	"sys_get_thread_area",
	"sys_io_setup",					/* 245 */
	"sys_io_destroy",
	"sys_io_getevents",
	"sys_io_submit",
	"sys_io_cancel",
	"sys_alloc_hugepages",			/* 250 */
	"sys_free_hugepages",
	"sys_exit_group",
	"sys_lookup_dcookie",
	"sys_epoll_create",
	"sys_epoll_ctl",				/* 255 */
	"sys_epoll_wait",
	"sys_remap_file_pages",
	"sys_set_tid_address",
	"sys_timer_create",
	"sys_timer_settime",			/* 260 */
	"sys_timer_gettime",
	"sys_timer_getoverrun",
	"sys_timer_delete",
	"sys_clock_settime",
	"sys_clock_gettime",			/* 265 */
	"sys_clock_getres",
	"sys_clock_nanosleep",
	"sys_statfs64",
	"sys_fstatfs64",
	"sys_tgkill",					/* 270 */
	"sys_utimes",
	"sys_fkt_probe0",				/* 272 */	   /* rdr */
	"sys_fkt_probe1",				/* 273 */	   /* rdr */
	"sys_fkt_probe2",				/* 274 */	   /* rdr */
};

int find_syscall( int code, char *category, char **name, int *should_be_hex )
	{
	int		z;

	*should_be_hex = 0;
	if( code <= FKT_SYS_CALL_MASK )
		if ( code == FKT_LCALL7 )
			{
			z = 0x7;
			*should_be_hex = 1;
			*category = 'S';
			*name = "lcall7";
			}
		else if ( code == FKT_LCALL27 )
			{
			z = 0x27;
			*should_be_hex = 1;
			*category = 'S';
			*name = "lcall27";
			}
		else
			{/* this is a true system call */
			z = code;
			*should_be_hex = 1;
			*category = 'S';
			*name = sys_calls[z];
			}
	else if( code < FKT_TRAP_LIMIT_CODE )
		{/* this is a trap */
		z = code - FKT_TRAP_BASE;
		if( z > NTRAPS )
			z = NTRAPS;
		*category = 'T';
		*name = traps[z];
		}
	else if ( code < FKT_IRQ_SYS )
		{/* device IRQ */
		z = code - FKT_IRQ_TIMER;
		*category = 'I';
		*name = find_irq(z);
		}
	else
		{/* system IRQ */
		z = code - FKT_IRQ_SYS;
		*should_be_hex = 1;
		*category = 'I';
		*name = sys_irqs[z];
		z += 0xef;
		}
	return z;
	}

/*
 * kernel functions
 */

struct code_name	{
	unsigned int	code;
	char			*name;
};

struct code_name	fkt_code_table[] = {
	{FKT_SETUP_CODE,				"fkt_setup"},
	{FKT_KEYCHANGE_CODE,			"fkt_keychange"},
	{FKT_RESET_CODE,				"fkt_reset"},
	{FKT_CALIBRATE0_CODE,			"fkt_calibrate0"},
	{FKT_CALIBRATE1_CODE,			"fkt_calibrate1"},
	{FKT_CALIBRATE2_CODE,			"fkt_calibrate2"},
	{FKT_RECORD_WRITING_CODE,		"fkt_record_writing"},
	{FKT_RECORD_WRITTEN_CODE,		"fkt_record_written"},
	{FKT_RECORD_EXTRA_WRITTEN_CODE,	"fkt_record_extra_written"},
	{FKT_RECORD_EXTRA_PAGE_CODE,	"fkt_record_extra_page"},
	{FKT_RECORD_FORCE_SYNC_CODE,	"fkt_record_force_sync"},
	{FKT_RECORD_KILLED_CODE,		"fkt_record_killed"},
	{FKT_END_IO_SYNC_CODE,			"end_buffer_io_sync"},
	{FKT_END_IO_ASYNC_CODE,			"end_buffer_io_async"},
	{FKT_DO_FORK_CODE,				"do_fork"},
	{FKT_DO_EXECVE_CODE,			"do_execve"},
	{FKT_END_OF_PID_CODE,			"end_of_pid"},
	{FKT_RET_FROM_SYS_CALL_CODE,	"ret_from_sys_call"},

	{FKT_FKT_FORCE_SYNC_ENTRY_CODE,	"fkt_force_sync_entry"},
	{FKT_FKT_FORCE_SYNC_EXIT_CODE,	"fkt_force_sync_exit"},
	{FKT_FKT_EXTRA_ALLOC_ENTRY_CODE,"fkt_extra_alloc_entry"},
	{FKT_FKT_EXTRA_ALLOC_EXIT_CODE,	"fkt_extra_alloc_exit"},

	{FKT_LL_RW_BLOCK_ENTRY_CODE,	"ll_rw_block_entry"},
	{FKT_LL_RW_BLOCK_EXIT_CODE,		"ll_rw_block_exit"},
//	{FKT_EXT2_WRITE_ENTRY_CODE,		"ext2_file_write_entry"},
//	{FKT_EXT2_WRITE_EXIT_CODE,		"ext2_file_write_exit"},
	{FKT_EXT2_GETBLK_ENTRY_CODE,	"ext2_getblk_entry"},
	{FKT_EXT2_GETBLK_EXIT_CODE,		"ext2_getblk_exit"},
//	{FKT_INODE_GETBLK_ENTRY_CODE,	"inode_getblk_entry"},
//	{FKT_INODE_GETBLK_EXIT_CODE,	"inode_getblk_exit"},
//	{FKT_BLOCK_GETBLK_ENTRY_CODE,	"block_getblk_entry"},
//	{FKT_BLOCK_GETBLK_EXIT_CODE,	"block_getblk_exit"},
	{FKT_SWITCH_TO_CODE,			"switch_to"},
	{FKT_WAIT4_ENTRY_CODE,			"wait4_entry"},
	{FKT_WAIT4_EXIT_CODE,			"wait4_exit"},
	{FKT_GENERIC_MAKE_REQUEST_ENTRY_CODE,			"generic_make_request_entry"},
	{FKT_GENERIC_MAKE_REQUEST_EXIT_CODE,			"generic_make_request_exit"},
	{FKT___MAKE_REQUEST_ENTRY_CODE,			"__make_request_entry"},
	{FKT___MAKE_REQUEST_EXIT_CODE,			"__make_request_exit"},
	{FKT_FSYNC_BUFFERS_LIST_ENTRY_CODE,		"fsync_buffers_list_entry"},
	{FKT_FSYNC_BUFFERS_LIST_EXIT_CODE,		"fsync_buffers_list_exit"},

	/* sadhna start */

	/* socket interface routines */
	{FKT_SOCKET_ENTRY_CODE,			"socket_entry"},
	{FKT_SOCKET_EXIT_CODE,			"socket_exit"},
	{FKT_BIND_ENTRY_CODE,			"bind_entry"},
	{FKT_BIND_EXIT_CODE,			"bind_exit"},
	{FKT_LISTEN_ENTRY_CODE,			"listen_entry"},
	{FKT_LISTEN_EXIT_CODE,			"listen_exit"},
	{FKT_ACCEPT_ENTRY_CODE,			"accept_entry"},
	{FKT_ACCEPT_EXIT_CODE,			"accept_exit"},
	{FKT_CONNECT_ENTRY_CODE,		"connect_entry"},
	{FKT_CONNECT_EXIT_CODE,			"connect_exit"},
	{FKT_SEND_ENTRY_CODE,			"send_entry"},
	{FKT_SEND_EXIT_CODE,			"send_exit"},
	{FKT_SENDTO_ENTRY_CODE,			"sendto_entry"},
	{FKT_SENDTO_EXIT_CODE,			"sendto_exit"},
	{FKT_RECVFROM_ENTRY_CODE,		"recvfrom_entry"},
	{FKT_RECVFROM_EXIT_CODE,		"recvfrom_exit"},
	{FKT_SHUTDOWN_ENTRY_CODE,		"shutdown_entry"},
	{FKT_SHUTDOWN_EXIT_CODE,		"shutdown_exit"},

	/* lamp routines */
	{FKT_LAMP_CREATE_ENTRY_CODE,	"lamp_create_entry"},
	{FKT_LAMP_CREATE_EXIT_CODE,		"lamp_create_exit"},
	{FKT_LAMP_BIND_ENTRY_CODE,		"lamp_bind_entry"},
	{FKT_LAMP_BIND_EXIT_CODE,		"lamp_bind_exit"},
	{FKT_LAMP_LISTEN_ENTRY_CODE,	"lamp_listen_entry"},
	{FKT_LAMP_LISTEN_EXIT_CODE,		"lamp_listen_exit"},
	{FKT_LAMP_ACCEPT_ENTRY_CODE,	"lamp_accept_entry"},
	{FKT_LAMP_ACCEPT_EXIT_CODE,		"lamp_accept_exit"},
	{FKT_LAMP_CONNECT_ENTRY_CODE,	"lamp_connect_entry"},
	{FKT_LAMP_CONNECT_EXIT_CODE,	"lamp_connect_exit"},
	{FKT_LAMP_SENDMSG_ENTRY_CODE,	"lamp_sendmsg_entry"},
	{FKT_LAMP_SENDMSG_EXIT_CODE,	"lamp_sendmsg_exit"},
	{FKT_LAMP_RECVMSG_ENTRY_CODE,	"lamp_recvmsg_entry"},
	{FKT_LAMP_RECVMSG_EXIT_CODE,	"lamp_recvmsg_exit"},
	{FKT_LAMP_DATA_RCV_ENTRY_CODE,	"lamp_data_rcv_entry"},
	{FKT_LAMP_DATA_RCV_EXIT_CODE,	"lamp_data_rcv_exit"},
	{FKT_LAMP_SIGNAL_RCV_ENTRY_CODE,"lamp_signal_rcv_entry"},
	{FKT_LAMP_SIGNAL_RCV_EXIT_CODE,	"lamp_signal_rcv_exit"},
	{FKT_LAMP_SHUTDOWN_ENTRY_CODE,	"lamp_shutdown_entry"},
	{FKT_LAMP_SHUTDOWN_EXIT_CODE,	"lamp_shutdown_exit"},
	{FKT_LAMP_RECVMSG_SCHEDI_CODE,	"lamp_recvmsg_sched_entry"},
	{FKT_LAMP_RECVMSG_SCHEDO_CODE,	"lamp_recvmsg_sched_exit"},

	/*	inet routines */
	{FKT_INET_CREATE_ENTRY_CODE,	"inet_create_entry"},
	{FKT_INET_CREATE_EXIT_CODE,		"inet_create_exit"},
	{FKT_INET_BIND_ENTRY_CODE,		"inet_bind_entry"},
	{FKT_INET_BIND_EXIT_CODE,		"inet_bind_exit"},
	{FKT_INET_LISTEN_ENTRY_CODE,	"inet_listen_entry"},
	{FKT_INET_LISTEN_EXIT_CODE,		"inet_listen_exit"},
	{FKT_INET_ACCEPT_ENTRY_CODE,	"inet_accept_entry"},
	{FKT_INET_ACCEPT_EXIT_CODE,		"inet_accept_exit"},
	{FKT_INET_STREAM_CONNECT_ENTRY_CODE,"inet_stream_connect_entry"},
	{FKT_INET_STREAM_CONNECT_EXIT_CODE,	"inet_stream_connect_exit"},
	{FKT_INET_SENDMSG_ENTRY_CODE,	"inet_sendmsg_entry"},
	{FKT_INET_SENDMSG_EXIT_CODE,	"inet_sendmsg_exit"},
	{FKT_INET_RECVMSG_ENTRY_CODE,	"inet_recvmsg_entry"},
	{FKT_INET_RECVMSG_EXIT_CODE,	"inet_recvmsg_exit"},
	{FKT_INET_SHUTDOWN_ENTRY_CODE,	"inet_shutdown_entry"},
	{FKT_INET_SHUTDOWN_EXIT_CODE,	"inet_shutdown_exit"},

	/*	net routines */
	{FKT_NET_RX_ACTION_ENTRY_CODE,	"net_rx_action_entry"},
	{FKT_NET_RX_ACTION_EXIT_CODE,	"net_rx_action_exit"},
//	{FKT_NET_BH_QUEUE_ENTRY_CODE,	"net_bh_queue_entry"},
//	{FKT_NET_BH_QUEUE_EXIT_CODE,	"net_bh_queue_exit"},
	{FKT_NETIF_RX_ENTRY_CODE,		"netif_rx_entry"},
	{FKT_NETIF_RX_EXIT_CODE,		"netif_rx_exit"},
	{FKT_NET_RX_DEQUEUE_ENTRY_CODE,	"net_rx_dequeue_entry"},
	{FKT_NET_RX_DEQUEUE_EXIT_CODE, 	"net_rx_dequeue_exit"},

	/* sadhna end */

	/* miru start */

	/*	tcp routines */
	{FKT_TCP_SENDMSG_ENTRY_CODE,	"tcp_sendmsg_entry"},
	{FKT_TCP_SENDMSG_EXIT_CODE,		"tcp_sendmsg_exit"},
	{FKT_TCP_SEND_SKB_ENTRY_CODE,	"tcp_send_skb_entry"},
	{FKT_TCP_SEND_SKB_EXIT_CODE,	"tcp_send_skb_exit"},
	{FKT_TCP_TRANSMIT_SKB_ENTRY_CODE,"tcp_transmit_skb_entry"},
	{FKT_TCP_TRANSMIT_SKB_EXIT_CODE,"tcp_transmit_skb_exit"},
	{FKT_TCP_WRITE_XMIT_ENTRY_CODE,	"tcp_write_xmit_entry"},
	{FKT_TCP_WRITE_XMIT_EXIT_CODE,	"tcp_write_xmit_exit"},
	{FKT_TCP_V4_RCV_ENTRY_CODE,		"tcp_v4_rcv_entry"},
	{FKT_TCP_V4_RCV_EXIT_CODE,		"tcp_v4_rcv_exit"},
//	{FKT_TCP_RECVMSG_SCHEDI_CODE,	"tcp_data_wait_entry"},
//	{FKT_TCP_RECVMSG_SCHEDO_CODE,	"tcp_data_wait_exit"},
	{FKT_TCP_RCV_ESTABLISHED_ENTRY_CODE,"tcp_rcv_established_entry"},
	{FKT_TCP_RCV_ESTABLISHED_EXIT_CODE,"tcp_rcv_established_exit"},
	{FKT_TCP_DATA_ENTRY_CODE,		"tcp_data_entry"},
	{FKT_TCP_DATA_EXIT_CODE,		"tcp_data_exit"},
	{FKT_TCP_ACK_ENTRY_CODE,		"tcp_ack_entry"},
	{FKT_TCP_ACK_EXIT_CODE,			"tcp_ack_exit"},

	{FKT_TCP_RECVMSG_ENTRY_CODE,	"tcp_recvmsg_entry"},
	{FKT_TCP_RECVMSG_EXIT_CODE,		"tcp_recvmsg_exit"},
//	{FKT_TCP_V4_RCV_CSUM_ENTRY_CODE,"tcp_v4_checksum_init_entry"},
//	{FKT_TCP_V4_RCV_CSUM_EXIT_CODE,	"tcp_v4_checksum_init_exit"},
	{FKT_TCP_RECVMSG_OK_ENTRY_CODE,	"tcp_recvmsg_ok_entry"},
	{FKT_TCP_RECVMSG_OK_EXIT_CODE,	"tcp_recvmsg_ok_exit"},
	{FKT_TCP_SEND_ACK_ENTRY_CODE,	"tcp_send_ack_entry"},
	{FKT_TCP_SEND_ACK_EXIT_CODE,	"tcp_send_ack_exit"},
//	{FKT_TCP_V4_RCV_HW_ENTRY_CODE,	"tcp_v4_rcv_hw_entry"},
//	{FKT_TCP_V4_RCV_HW_EXIT_CODE,	"tcp_v4_rcv_hw_exit"},
//	{FKT_TCP_V4_RCV_NONE_ENTRY_CODE,"tcp_v4_rcv_none_entry"},
//	{FKT_TCP_V4_RCV_NONE_EXIT_CODE,	"tcp_v4_rcv_none_exit"},
	{FKT_WAIT_FOR_TCP_MEMORY_ENTRY_CODE,"wait_for_tcp_memory_entry"},
	{FKT_WAIT_FOR_TCP_MEMORY_EXIT_CODE,"wait_for_tcp_memory_exit"},
//	{FKT_TCP_RECVMSG_CSUM_ENTRY_CODE,"tcp_recvmsg_csum_entry"},
//	{FKT_TCP_RECVMSG_CSUM_EXIT_CODE,"tcp_recvmsg_csum_exit"},
//	{FKT_TCP_RECVMSG_CSUM2_ENTRY_CODE,"tcp_recvmsg_csum2_entry"},
//	{FKT_TCP_RECVMSG_CSUM2_EXIT_CODE,"tcp_recvmsg_csum2_exit"},
	{FKT_TCP_SYNC_MSS_ENTRY_CODE,	"tcp_sync_mss_entry"},
	{FKT_TCP_SYNC_MSS_EXIT_CODE,	"tcp_sync_mss_exit"},
	{FKT_TCP_ALLOC_PAGE_ENTRY_CODE,	"tcp_alloc_page_entry"},
	{FKT_TCP_ALLOC_PAGE_EXIT_CODE,	"tcp_alloc_page_exit"},

	/* udp routines */
	{FKT_UDP_SENDMSG_ENTRY_CODE,	"udp_sendmsg_entry"},
	{FKT_UDP_SENDMSG_EXIT_CODE,		"udp_sendmsg_exit"},
	{FKT_UDP_QUEUE_RCV_SKB_ENTRY_CODE,"udp_queue_rcv_skb_entry"},
	{FKT_UDP_QUEUE_RCV_SKB_EXIT_CODE,"udp_queue_rcv_skb_exit"},
	{FKT_UDP_GETFRAG_ENTRY_CODE,	"udp_getfrag_entry"},
	{FKT_UDP_GETFRAG_EXIT_CODE,		"udp_getfrag_exit"},
	{FKT_UDP_RECVMSG_ENTRY_CODE,	"udp_recvmsg_entry"},
	{FKT_UDP_RECVMSG_EXIT_CODE,		"udp_recvmsg_exit"},
	{FKT_UDP_RCV_ENTRY_CODE,		"udp_rcv_entry"},
	{FKT_UDP_RCV_EXIT_CODE,			"udp_rcv_exit"},

	/*	ip routines */
	{FKT_IP_RCV_ENTRY_CODE,			"ip_rcv_entry"},
	{FKT_IP_RCV_EXIT_CODE,			"ip_rcv_exit"},
	{FKT_IP_QUEUE_XMIT_ENTRY_CODE,	"ip_queue_xmit_entry"},
	{FKT_IP_QUEUE_XMIT_EXIT_CODE,	"ip_queue_xmit_exit"},
	{FKT_IP_BUILD_XMIT_ENTRY_CODE,	"ip_build_xmit_entry"},
	{FKT_IP_BUILD_XMIT_EXIT_CODE,	"ip_build_xmit_exit"},
	{FKT_IP_LOCAL_DELIVER_ENTRY_CODE,"ip_local_deliver_entry"},
	{FKT_IP_LOCAL_DELIVER_EXIT_CODE,"ip_local_deliver_exit"},
	{FKT_IP_DEFRAG_ENTRY_CODE,		"ip_defrag_entry"},
	{FKT_IP_DEFRAG_EXIT_CODE,		"ip_defrag_exit"},
	{FKT_IP_FRAG_REASM_ENTRY_CODE,	"ip_frag_reasm_entry"},
	{FKT_IP_FRAG_REASM_EXIT_CODE,	"ip_frag_reasm_exit"},

	/*	ethernet cards */
	{FKT_TULIP_START_XMIT_ENTRY_CODE,"tulip_start_xmit_entry"},
	{FKT_TULIP_START_XMIT_EXIT_CODE,"tulip_start_xmit_exit"},
	{FKT_TULIP_INTERRUPT_ENTRY_CODE,"tulip_interrupt_entry"},
	{FKT_TULIP_INTERRUPT_EXIT_CODE,	"tulip_interrupt_exit"},
	{FKT_TULIP_RX_ENTRY_CODE,		"tulip_rx_entry"},
	{FKT_TULIP_RX_EXIT_CODE,		"tulip_rx_exit"},
	{FKT_TULIP_TX_ENTRY_CODE,		"tulip_tx_entry"},
	{FKT_TULIP_TX_EXIT_CODE,		"tulip_tx_exit"},

	{FKT_VORTEX_INTERRUPT_ENTRY_CODE,"vortex_interrupt_entry"},
	{FKT_VORTEX_INTERRUPT_EXIT_CODE,"vortex_interrupt_exit"},
	{FKT_BOOMERANG_INTERRUPT_ENTRY_CODE,"boomerang_interrupt_entry"},
	{FKT_BOOMERANG_INTERRUPT_EXIT_CODE,"boomerang_interrupt_exit"},
	{FKT_BOOMERANG_START_XMIT_ENTRY_CODE,"boomerang_start_xmit_entry"},
	{FKT_BOOMERANG_START_XMIT_EXIT_CODE,"boomerang_start_xmit_exit"},
	{FKT_BOOMERANG_RX_ENTRY_CODE,	"boomerang_rx_entry"},
	{FKT_BOOMERANG_RX_EXIT_CODE,	"boomerang_rx_exit"},
	{FKT_BOOMERANG_TX_ENTRY_CODE,	"boomerang_tx_entry"},
	{FKT_BOOMERANG_TX_EXIT_CODE,	"boomerang_tx_exit"},

	{FKT_ALLOC_SKB_ENTRY_CODE,		"alloc_skb_entry"},
	{FKT_ALLOC_SKB_EXIT_CODE,		"alloc_skb_exit"},
	
	{FKT_DO_SOFTIRQ_ENTRY_CODE,		"do_softirq_entry"},
	{FKT_DO_SOFTIRQ_EXIT_CODE,		"do_softirq_exit"},

	{FKT_ACE_INTERRUPT_ENTRY_CODE,  "ace_interrupt_entry"},
	{FKT_ACE_INTERRUPT_EXIT_CODE,   "ace_interrupt_exit"},
	{FKT_ACE_RX_INT_ENTRY_CODE,		"ace_rx_int_entry"},
	{FKT_ACE_RX_INT_EXIT_CODE,		"ace_rx_int_exit"},
	{FKT_ACE_TX_INT_ENTRY_CODE,		"ace_tx_int_entry"},
	{FKT_ACE_TX_INT_EXIT_CODE,		"ace_tx_int_exit"},
	{FKT_ACE_START_XMIT_ENTRY_CODE, "ace_start_xmit_entry"},
	{FKT_ACE_START_XMIT_EXIT_CODE,  "ace_start_xmit_exit"},

	/* miru end */


	/*	the sys_xxx functions */
	/* achadda start */
	{FKT_READ_ENTRY_CODE,			"sys_read_entry"},
	{FKT_READ_EXIT_CODE,			"sys_read_exit"},

	/* vs start */
	{FKT_WRITE_ENTRY_CODE,			"sys_write_entry"},
	{FKT_WRITE_EXIT_CODE,			"sys_write_exit"},

	/* rdr */
	{FKT_LSEEK_ENTRY_CODE,			"sys_lseek_entry"},
	{FKT_LSEEK_EXIT_CODE,			"sys_lseek_exit"},

	/* vs start */
	{FKT_FSYNC_ENTRY_CODE,			"sys_fsync_entry"},
	{FKT_FSYNC_EXIT_CODE,			"sys_fsync_exit"},
	{FKT_NANOSLEEP_ENTRY_CODE,		"sys_nanosleep_entry"},
	{FKT_NANOSLEEP_EXIT_CODE,		"sys_nanosleep_exit"},


	/* rdr start */
//	{FKT_DO_SD_REQUEST_ENTRY_CODE,	"do_sd_request_entry"},
//	{FKT_DO_SD_REQUEST_EXIT_CODE,	"do_sd_request_exit"},
//	{FKT_REQUEUE_SD_REQUEST_ENTRY_CODE, "requeue_sd_request_entry"},
//	{FKT_REQUEUE_SD_REQUEST_EXIT_CODE, "requeue_sd_request_exit"},
	{FKT_SD_OPEN_ENTRY_CODE,		"sd_open_entry"},
	{FKT_SD_OPEN_EXIT_CODE,			"sd_open_exit"},
	{FKT_SD_RELEASE_ENTRY_CODE,		"sd_release_entry"},
	{FKT_SD_RELEASE_EXIT_CODE,		"sd_release_exit"},
	{FKT_SD_IOCTL_ENTRY_CODE,		"sd_ioctl_entry"},
	{FKT_SD_IOCTL_EXIT_CODE,		"sd_ioctl_exit"},
	{FKT_SCSI_DO_CMD_ENTRY_CODE,	"scsi_do_cmd_entry"},
	{FKT_SCSI_DO_CMD_EXIT_CODE,		"scsi_do_cmd_exit"},
	{FKT_SCSI_IOCTL_ENTRY_CODE,		"scsi_ioctl_entry"},
	{FKT_SCSI_IOCTL_EXIT_CODE,		"scsi_ioctl_exit"},
	{FKT_SCSI_DONE_ENTRY_CODE,		"scsi_done_entry"},
	{FKT_SCSI_DONE_EXIT_CODE,		"scsi_done_exit"},
	{FKT_SCSI_DISPATCH_CMD_ENTRY_CODE,"scsi_dispatch_cmd_entry"},
	{FKT_SCSI_DISPATCH_CMD_EXIT_CODE,"scsi_dispatch_cmd_exit"},
	{FKT_SCSI_RETRY_COMMAND_ENTRY_CODE,"scsi_retry_command_entry"},
	{FKT_SCSI_RETRY_COMMAND_EXIT_CODE,"scsi_retry_command_exit"},
	{FKT_SG_OPEN_ENTRY_CODE,		"sg_open_entry"},
	{FKT_SG_OPEN_EXIT_CODE,			"sg_open_exit"},
	{FKT_SG_RELEASE_ENTRY_CODE,		"sg_release_entry"},
	{FKT_SG_RELEASE_EXIT_CODE,		"sg_release_exit"},
	{FKT_SG_IOCTL_ENTRY_CODE,		"sg_ioctl_entry"},
	{FKT_SG_IOCTL_EXIT_CODE,		"sg_ioctl_exit"},
	{FKT_SG_READ_ENTRY_CODE,		"sg_read_entry"},
	{FKT_SG_READ_EXIT_CODE,			"sg_read_exit"},
	{FKT_SG_NEW_READ_ENTRY_CODE,	"sg_new_read_entry"},
	{FKT_SG_NEW_READ_EXIT_CODE,		"sg_new_read_exit"},
	{FKT_SG_WRITE_ENTRY_CODE,		"sg_write_entry"},
	{FKT_SG_WRITE_EXIT_CODE,		"sg_write_exit"},
	{FKT_SG_NEW_WRITE_ENTRY_CODE,	"sg_new_write_entry"},
	{FKT_SG_NEW_WRITE_EXIT_CODE,	"sg_new_write_exit"},
	{FKT_SG_GET_RQ_MARK_ENTRY_CODE,	"sg_get_rq_mark_entry"},
	{FKT_SG_GET_RQ_MARK_EXIT_CODE,	"sg_get_rq_mark_exit"},
	{FKT_SG_CMD_DONE_BH_ENTRY_CODE,	"sg_cmd_done_bh_entry"},
	{FKT_SG_CMD_DONE_BH_EXIT_CODE,	"sg_cmd_done_bh_exit"},
	{FKT_SCSI_SEP_IOCTL_ENTRY_CODE,	"scsi_sep_ioctl_entry"},
	{FKT_SCSI_SEP_IOCTL_EXIT_CODE,	"scsi_sep_ioctl_exit"},
	{FKT_SCSI_SEP_QUEUECOMMAND_ENTRY_CODE,"scsi_sep_queuecommand_entry"},
	{FKT_SCSI_SEP_QUEUECOMMAND_EXIT_CODE,"scsi_sep_queuecommand_exit"},
	/* rdr end */

	/* vs start */
//	{FKT_SEAGATE_INTERNAL_CMD_ENTRY_CODE,"seagate_internal_cmd_entry"},
//	{FKT_SEAGATE_INTERNAL_CMD_EXIT_CODE,"seagate_internal_cmd_exit"},
//	{FKT_QLOGIC_QCMND_ENTRY_CODE,	"qlogic_qcmnd_entry"},
//	{FKT_QLOGIC_QCMND_EXIT_CODE,	"qlogic_qcmnd_exit"},
//	{FKT_ISP_RET_STATUS_ENTRY_CODE,	"qlogic_ret_status_entry"},
//	{FKT_ISP_RET_STATUS_EXIT_CODE,	"qlogic_ret_status_exit"},
//	{AM53C974_QCMD_ENTRY_CODE,	"am53c974_qcmd_entry"},
//	{AM53C974_QCMD_EXIT_CODE,	"am53c974_qcmd_exit"},

	/*****  vs: old codes, need to be reassigned if used again *****
	{0x581, "ext2_sync_file_entry"},
	{0x681, "ext2_sync_file_exit"},
	{0x582, "sync_tind_entry"},
	{0x682, "sync_tind_exit"},
	{0x583, "sync_dind_entry"},
	{0x683, "sync_dind_exit"},
	{0x584, "sync_indirect_entry"},
	{0x684, "sync_indirect_exit"},
	{0x585, "sync_direct_entry"},
	{0x685, "sync_direct_exit"},
	{0x586, "sync_iblock_entry"},
	{0x686, "sync_iblock_exit"},
	{0x587, "sync_block_entry"},
	{0x687, "sync_block_exit"},
	*****/

//	{REQUE_DEVM_DEV_BLK_CODE,	"reque_prior_devm_dev_blk"},
//	{REQUE_BLK_CODE,		"reque_after_blk"},
	/* vs end */

	{0, NULL}
};

#ifdef FUT
struct code_name fut_code_table [] = 
	{
#if !defined(PREPROC) && !defined(DEPEND)
#include "fut_print.h"
#endif
	{0, NULL }
	};
#endif

char *find_name( unsigned int code, int keep_entry, int maxlen, struct code_name *code_table )
	{
	struct code_name	*ptr;
	static char			local_buf[128];
	char				*name=NULL;
	int					len, elen;

	if( code >= GCC_TRACED_FUNCTION_MINI )
#ifndef FUT
		name = lookup_symbol(code | GCC_TRACED_FUNCTION_EXIT);
#else
		name = lookup_symbol(code & (~GCC_TRACED_FUNCTION_EXIT));
#endif
	else
		for( ptr = code_table;  ptr->code != 0;  ptr++ )
			if( ptr->code == code )
				{
				name = ptr->name;
				break;
				}
	if( !name )
		{
		name = local_buf;
		sprintf(name,"unknown code %x",code);
		}
	if( !keep_entry )
		{/* caller wants _entry stripped off end of name */
		len = strlen(name);
		elen = strlen("_entry");
		if( len > elen  && strcmp(&name[len-elen], "_entry") == 0 )
			{
			strcpy(local_buf, name);
			name = local_buf;
			name[len-elen] = '\0';
			}
		}
	if( maxlen > 0 && strlen(name) > maxlen )
		{
		if( name != local_buf )
			{
			strcpy(local_buf, name);
			name = local_buf;
			}
		name[maxlen] = '\0';
		}
	return name;
	}

/* vi: ts=4
 */
