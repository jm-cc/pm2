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
