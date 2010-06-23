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

/* Code de démarrage de marcel */


#define  MARCEL_INIT_C
#include "marcel.h"
#include "tbx_macros.h"
#include "tbx_compiler.h"
#include "tbx_program_argument.h"
#include "tbx_hooks.h"
#ifdef LINUX_SYS
#include <sys/utsname.h>
#endif
#include <sys/resource.h>
#include <stdlib.h>
#include <string.h>
#ifdef CONFIG_PUK_PUKABI
#include <errno.h>
#include <Padico/Puk-ABI.h>
#ifdef MA__LIBPTHREAD
#include <dlfcn.h>
#endif
#endif /* CONFIG_PUK_PUKABI */
#ifdef MA__BUBBLES
#include "marcel_bubble_sched_lookup.h"
#endif


TBX_VISIBILITY_PUSH_INTERNAL

tbx_flag_t ma_activity  = tbx_flag_clear;
/* children after fork(2) must not call marcel_end() see pthread_ptfork: cleanup_child_after_fork */
tbx_flag_t ma_is_parent = tbx_flag_set;

TBX_VISIBILITY_PUSH_DEFAULT


int marcel_test_activity(void)
{
	return (int)tbx_flag_test(&ma_activity);
}

void marcel_initialize(int *argc, char *argv[])
{
	if (! argc)
		exit(1);

	if (! marcel_test_activity()) {
		MARCEL_LOG_IN();
		marcel_init_data(*argc, argv); 
		marcel_start_sched(); 
		marcel_purge_cmdline();
		tbx_pa_copy_args(argc, argv);
		MARCEL_LOG_OUT();
	}
}


static void marcel_parse_cmdline_early(int argc, char *argv[])
{
	int i;

	i = 1;
	while (i < argc) {
#ifdef MA__LWPS
		if (!strcmp(argv[i], "--marcel-nvp")) {
			i += 2;
			if (i > argc) {
				fprintf(stderr,
					"Fatal error: --marcel-nvp option must be followed "
					"by <nb_of_virtual_processors>.\n");
				exit(1);
			}
			marcel_nb_vp = atoi(argv[i - 1]);
			if (marcel_nb_vp < 1 || marcel_nb_vp > MA_NR_VPS) {
				fprintf(stderr,
					"Error: nb of VP should be between 1 and %ld\n",
					(long) MA_NR_VPS);
				exit(1);
			}

		} else if (!strcmp(argv[i], "--marcel-cpustride")) {
			int stride;

			i += 2;
			if (i > argc) {
				fprintf(stderr,
					"Fatal error: --marcel-cpustride option must be followed "
					"by <nb_of_processors_to_seek>.\n");
				exit(1);
			}
			stride = atoi(argv[i - 1]);
			if (stride < 0) {
				fprintf(stderr, "Error: CPU stride should be positive\n");
				exit(1);
			}
			marcel_cpu_stride = stride;


		} else if (!strcmp(argv[i], "--marcel-firstcpu")) {
			int first_cpu;

			i += 2;
			if (i > argc) {
				fprintf(stderr,
					"Fatal error: --marcel-firstcpu option must be followed "
					"by <num_of_first_processor>.\n");
				exit(1);
			}
			first_cpu = atoi(argv[i - 1]);
			if (first_cpu < 0) {
				fprintf(stderr, "Error: CPU number should be positive\n");
				exit(1);
			}
			marcel_first_cpu = first_cpu;
		} else
#ifdef MA__NUMA
		if (!strcmp(argv[i], "--marcel-maxarity")) {
			int topo_max_arity;
			i += 2;
			if (i > argc) {
				fprintf(stderr,
					"Fatal error: --marcel-maxarity option must be followed "
					"by <maximum_topology_arity>.\n");
				exit(1);
			}
			topo_max_arity = atoi(argv[i - 1]);
			if (topo_max_arity < 0) {
				fprintf(stderr,
					"Error: Maximum topology arity should be positive\n");
				exit(1);
			}
			marcel_topo_max_arity = topo_max_arity;
		} else
#endif
#endif
#ifdef MA__BUBBLES
		if (!strcmp(argv[i], "--marcel-list-schedulers")) {
			int j = 0;

			fprintf(stderr, "Bubble schedulers:");
			while (ma_bubble_schedulers[j].name != NULL) {
				fprintf(stderr, " %s", ma_bubble_schedulers[j].name);
				j ++;
			}
			fprintf(stderr, "\n");
			exit(1);
		} else
			
		if (!strcmp(argv[i], "--marcel-bubble-scheduler")) {
			const marcel_bubble_sched_t *scheduler;

			i += 2;
			if (i > argc) {
				fprintf(stderr,
					"Fatal error: --marcel-bubble-scheduler option must be followed "
					"by the name of a bubble scheduler.\n");
				exit(1);
			}

			scheduler = marcel_lookup_bubble_scheduler(argv[i - 1]);
			if (scheduler == NULL) {
				fprintf(stderr,
					"Fatal error: unknown bubble scheduler `%s'.\n",
					argv[i - 1]);
				exit(1);
			}
			marcel_bubble_set_sched((marcel_bubble_sched_t *) scheduler);
		} else
#endif
		if (!strncmp(argv[i], "--marcel", 8)) {
			MARCEL_SHOW_OPTION("marcel-top file", "Dump a top-like output to file");
			MARCEL_SHOW_OPTION("marcel-top |cmd", "Same as above, but to command cmd via a pipe");
			MARCEL_SHOW_OPTION("marcel-xtop", "Same as above, in a freshly created xterm");
#ifdef MA__BUBBLES
			MARCEL_SHOW_OPTION("marcel-top-bubbles", "In marcel-xtop, show the bubbles by default");
			MARCEL_SHOW_OPTION("marcel-bubble-scheduler sched", "Use the given bubble scheduler");
			MARCEL_SHOW_OPTION("marcel-list-schedulers", "List all marcel schedulers");
#endif
#ifdef MA__LWPS
			MARCEL_SHOW_OPTION("marcel-nvp n", "Force number of VPs to n");
			MARCEL_SHOW_OPTION("marcel-maxarity arity", "Insert fake levels until topology arity is at most arity");
			MARCEL_SHOW_OPTION("marcel-firstcpu cpu", "Allocate VPs from the given cpu (topologic number, not OS number)");
			MARCEL_SHOW_OPTION("marcel-cpustride stride", "Allocate VPs every stride cpus (topologic number, not OS number)");
#endif
			exit(1);
		} else {
			i++;
		}
	}

#ifdef MA__LWPS
	MARCEL_LOG
	    ("\t\t\t<Suggested nb of Virtual Processors : %d, stride %d, first %d>\n",
	     marcel_nb_vp, marcel_cpu_stride, marcel_first_cpu);
#endif
}

/* Options that need Marcel to be alread up and running */
static void marcel_parse_cmdline_lastly(int argc, char *argv[])
{
	int i;

	i = 1;
	while (i < argc) {
		if (!strcmp(argv[i], "--marcel-top")) {
			i += 2;
			if (i > argc) {
				fprintf(stderr,
					"Fatal error: --marcel-top option must be followed "
					"by <out_file>.\n");
				exit(1);
			}
			if (marcel_init_top(argv[i - 1])) {
				fprintf(stderr,
					"Error: invalid top out_file %s\n", argv[i - 1]);
				exit(1);
			}

		} else if (!strcmp(argv[i], "--marcel-xtop")) {
			i++;
			if (marcel_init_top("|xterm -S//0 -geometry 130x26")) {
				fprintf(stderr, "Error: can't launch xterm\n");
				exit(1);
			}
#ifdef MA__BUBBLES
		} else if (!strcmp(argv[i], "--marcel-top-bubbles")) {
			ma_top_bubbles = 1;
#endif

		} else {
			i++;
		}
	}
}

#ifdef CONFIG_PUK_PUKABI
#ifdef MA__LIBPTHREAD
#include <dlfcn.h>

/* Issue a warning if Marcel and PukABI symbols don't prevail over
   libpthread/libc symbols, i.e., if they were not preloaded.  */
static void assert_preloaded(void)
{
	void *self;
	tbx_bool_t determined = tbx_false;

	self = dlopen(NULL, RTLD_LAZY);

	if (self != NULL) {
		void *marcel, *pukabi;

		marcel = dlopen("libpthread.so", RTLD_LAZY);
		if (marcel != NULL) {
			void *global_pcreate, *marcel_pcreate;

			global_pcreate = dlsym(self, "pthread_create");
			marcel_pcreate = dlsym(marcel, "pthread_create");

			if (marcel_pcreate != NULL && global_pcreate != NULL) {
				determined = tbx_true;
				if (marcel_pcreate != global_pcreate) {
					/* This is unlikely.  It would mean that somehow two different
					   `libpthread.so' were loaded.  */
					fprintf(stderr,
						"[Marcel] it appears that Marcel's libpthread "
						"was not preloaded\n");
					fprintf(stderr,
						"[Marcel] make sure to preload it "
						"using the `LD_PRELOAD' environment variable\n");
					abort();
				}
			}

			dlclose(marcel);
		}

		pukabi = dlopen("libPukABI.so", RTLD_LAZY);
		if (pukabi != NULL) {
			void *global_malloc, *pukabi_malloc, *real_malloc;

			global_malloc = dlsym(self, "malloc");
			real_malloc = dlsym(self, "__libc_malloc");
			pukabi_malloc = dlsym(pukabi, "malloc");

			if (pukabi_malloc != NULL && global_malloc != NULL) {
				determined = tbx_true;
				if (pukabi_malloc != global_malloc) {
					if (real_malloc != NULL
					    && global_malloc != real_malloc) {
						/* GNU Bash, for instance, provides its own `malloc'.  */
						fprintf(stderr,
							"[Marcel] this program (%s) appears to provide its own malloc(3)"
							", which may not be thread-safe\n",
							program_invocation_name ? :
							"unknown");
					} else {
						fprintf(stderr,
							"[Marcel] it appears that PukABI "
							"was not preloaded\n");
						fprintf(stderr,
							"[Marcel] make sure to preload it "
							"using the `LD_PRELOAD' environment variable\n");
					}
				}
			}

			dlclose(pukabi);
		}

		dlclose(self);
	}

	if (!determined) {
		fprintf(stderr, "[Marcel] unable to determine whether "
			"libpthread and/or PukABI were correctly preloaded");
		fprintf(stderr, "[Marcel] are both libraries in the loader "
			"search path?\n");
	}
}

#endif
#endif


// Does not start any internal threads.
// When completed, fork calls are still allowed.
void marcel_init_data(int argc, char *argv[])
{
	struct rlimit rlim;
	static volatile tbx_bool_t already_called = tbx_false;

	// Only execute this function once
	if (already_called)
		return;
	already_called = tbx_true;

	// defines process stack limits
	getrlimit(RLIMIT_STACK, &rlim);
	if (rlim.rlim_cur == RLIM_INFINITY)
		TBX_FAILURE("Please configure your system with a correct stack size limit: ulimit -s xxxx\n");
	ma_main_stacklimit = get_sp() - rlim.rlim_cur;

#ifdef TBX
#ifdef MA__LIBPTHREAD
	/* use ma_syscall functions instead of libc functions
	   avoid cancel point                                 */
	tbx_open = ma_open;
	tbx_close = ma_close;
	tbx_read = ma_read;
	tbx_write = ma_write;
	tbx_nanosleep = ma_nanosleep;
#endif

	// get full argv
	tbx_init(&argc, &argv);
#endif				/* TBX */

#ifdef PM2_TOPOLOGY
	// Launch hwloc
	tbx_topology_init(argc, argv);
#endif

	// Parse command line
	marcel_parse_cmdline_early(argc, argv);
	marcel_init_section(MA_INIT_SCHEDULER);

	// Initialize debug facilities
	marcel_debug_init();
	marcel_parse_cmdline_lastly(argc, argv);

#ifdef CONFIG_PUK_PUKABI
	/* Tell PukABI how to protect libc calls.  */
	puk_abi_set_spinlock_handlers((void (*)(void)) marcel_extlib_protect,
				      (void (*)(void)) marcel_extlib_unprotect, NULL);
	puk_abi_set_errno_handler(&marcel___errno_location);
	puk_abi_seterrno(0);

	/* Note: We don't need to explicitly handle the vararg functions of the
	 * `execv*' family because PukABI implements them in terms of the
	 * non-vararg functions above.  */
	puk_abi_set_virtual(execve, ma_execve);
	puk_abi_set_virtual(execv, ma_execv);
	puk_abi_set_virtual(execvp, ma_execvp);
	puk_abi_set_virtual(system, ma_system);
#ifdef HAVE__POSIX_SPAWN
	puk_abi_set_virtual(posix_spawn, ma_posix_spawn);
	puk_abi_set_virtual(posix_spawnp, ma_posix_spawnp);
#endif


	/* re-route *sleep functions */
	puk_abi_set_virtual(sleep, marcel_sleep);
	puk_abi_set_virtual(usleep, marcel_usleep);
	puk_abi_set_virtual(nanosleep, marcel_nanosleep);

#ifdef MA__LIBPTHREAD
	assert_preloaded();
#endif
#endif				/* CONFIG_PUK_PUKABI */

	ma_init_scheduling_hook();
}

// When completed, some threads may be started
// Fork calls are now prohibited in non libpthread versions
void marcel_start_sched(void)
{
	// Start scheduler (i.e. run LWP/activations, start timer)
	ma_activity = tbx_flag_set;
	marcel_init_section(MA_INIT_START_LWPS);
}

// Remove marcel-specific arguments from command line
void marcel_purge_cmdline()
{
	static volatile tbx_bool_t already_called = tbx_false;

	// Only execute this function once
	if (! already_called) {
		already_called = tbx_true;
		tbx_pa_free_module_args("marcel");

		marcel_init_section(MA_INIT_MAX_PARTS);
	}
}

void marcel_end(void)
{
	if (! ma_activity || ! ma_is_parent)
		return;

	if (MARCEL_SELF != __main_thread)
		MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);

	ma_wait_all_tasks_end();
	ma_gensched_shutdown();

#ifdef TBX
	tbx_exit();
#endif
	marcel_slot_exit();
	ma_topo_exit();
#ifdef PM2_TOPOLOGY
	tbx_topology_destroy();
#endif
	marcel_debug_exit();

	/** unset hooks before marcel exit **/
#if defined(TBX) && defined(MA__LIBPTHREAD)
	/** restore hook ==> use libc function **/
	tbx_open = open;
	tbx_close = close;
	tbx_read = read;
	tbx_write = write;
	tbx_nanosleep = nanosleep;
#endif

#ifdef CONFIG_PUK_PUKABI
	puk_abi_set_virtual(execve, NULL);
	puk_abi_set_virtual(execv, NULL);
	puk_abi_set_virtual(execvp, NULL);
	puk_abi_set_virtual(posix_spawn, NULL);
	puk_abi_set_virtual(posix_spawnp, NULL);
	puk_abi_set_virtual(system, NULL);

	puk_abi_set_virtual(sleep, NULL);
	puk_abi_set_virtual(usleep, NULL);
	puk_abi_set_virtual(nanosleep, NULL);

	puk_abi_unset_errno_handler();
	puk_abi_set_spinlock_handlers(NULL, NULL, NULL);
#endif

	ma_activity = tbx_flag_clear;
}

static const char valid_header_hash[] = MARCEL_HEADER_HASH;

tbx_bool_t marcel_header_hash_matches_binary(const char *header_hash)
{
	if (!strcmp(valid_header_hash, header_hash))
		return tbx_true;

	return tbx_false;
}

void marcel_ensure_abi_compatibility(const char *header_hash)
{
	if (!marcel_header_hash_matches_binary(header_hash)) {
		fprintf(stderr, "error: Marcel binary incompatibility detected\n");
		fprintf(stderr, "client header hash:  %s\n", header_hash);
		fprintf(stderr, "library header hash: %s\n", valid_header_hash);
		exit(1);
	}
}

#ifndef STANDARD_MAIN

volatile int __marcel_main_ret;
marcel_ctx_t __ma_initial_main_ctx;

#ifdef MARCEL_MAIN_AS_FUNC
int go_marcel_main(int (*main_func) (int, char *[]), int argc, char *argv[])
#else				/* MARCEL_MAIN_AS_FUNC */
extern int marcel_main(int argc, char *argv[]);

int main(int argc, char *argv[])
#endif				/* MARCEL_MAIN_AS_FUNC */
{
	static int __ma_argc;
	static char **__ma_argv;
	static int (*__ma_main) (int, char *[]);
	unsigned long new_sp;

#ifdef LINUX_SYS
	struct utsname utsname;

	uname(&utsname);
	if (strncmp(utsname.release, LINUX_VERSION, strlen(LINUX_VERSION))) {
		fprintf(stderr,
			"Marcel was compiled for Linux " LINUX_VERSION
			", but you are running Linux %s, can't continue\n",
			utsname.version);
		exit(1);
	}
#endif

	marcel_debug_init();

	if (!marcel_ctx_setjmp(__ma_initial_main_ctx)) {
		struct rlimit rlim;

#ifdef MA__SELF_VAR
		__ma_self =
#endif
		    __main_thread = (marcel_t) ((((unsigned long) get_sp() - 128) &
						 ~(THREAD_SLOT_SIZE - 1)) -
						MAL(sizeof(marcel_task_t)));

		MARCEL_LOG("\t\t\t<main_thread is %p>\n", __main_thread);

		__ma_argc = argc;
		__ma_argv = argv;
#ifdef MARCEL_MAIN_AS_FUNC
		__ma_main = main_func;
#else
		__ma_main = &marcel_main;
#endif
		new_sp = (unsigned long) __main_thread - TOP_STACK_FREE_AREA;

		getrlimit(RLIMIT_STACK, &rlim);
		if (get_sp() - new_sp > rlim.rlim_cur) {
			fprintf(stderr,
				"The current stack resource limit is too small (%ld) for the chosen marcel stack size (%ld).  Please increase it (ulimit -s, and maximum is %ld) or decrease THREAD_SLOT_SIZE in marcel/include/sys/isomalloc_archdep.h\n",
				(long) rlim.rlim_cur, (long) THREAD_SLOT_SIZE,
				(long) rlim.rlim_max);
			abort();
		}
		if (THREAD_SLOT_SIZE <= sizeof(marcel_task_t)) {
			fprintf(stderr, "THREAD_SLOT_SIZE is too small (%ld) to hold "
				"`marcel_t' (%zd) and the thread's stack.  Please "
				"increase it in marcel/include/sys/isomalloc_archdep.h\n",
				THREAD_SLOT_SIZE, sizeof(marcel_task_t));
			abort();
		}

		/* On se contente de descendre la pile. Tout va bien, même sur Itanium */
		set_sp(new_sp);

		__marcel_main_ret = (*__ma_main) (__ma_argc, __ma_argv);

		marcel_ctx_longjmp(__ma_initial_main_ctx, 1);
	}

	return __marcel_main_ret;
}


#endif				// STANDARD_MAIN

typedef struct {
	int prio;
	const char *debug;
} TBX_ALIGNED __ma_init_index_t;

typedef struct {
	struct {
		int section_number;
	} a TBX_ALIGNED;
	struct {
		__ma_init_info_t infos[2];
	} b TBX_ALIGNED;
} TBX_ALIGNED __ma_init_section_index_t;

int ma_init_done[MA_INIT_MAX_PARTS + 1] = { 0, };

// Section MA_INIT_SELF
#ifdef MA__LWPS
extern const __ma_init_info_t ma_init_info_initialize_topology;
#endif				// MA__LWPS
extern const __ma_init_info_t ma_init_info_main_thread_init;

// Section MA_INIT_MAIN_LWP
extern const __ma_init_info_t ma_init_info_marcel_slot_init;
extern const __ma_init_info_t ma_init_info_linux_sched_init;
#ifdef MA__BUBBLES
extern const __ma_init_info_t ma_init_info_bubble_sched_init;
#endif				// MA__BUBBLES
extern const __ma_init_info_t ma_init_info_softirq_init;
extern const __ma_init_info_t ma_init_info_sig_init;
extern const __ma_init_info_t ma_init_info_marcel_sig_timer_notifier_register;
extern const __ma_init_info_t ma_init_info_fork_handling_init;
extern const __ma_init_info_t ma_init_info_timer_start;
extern const __ma_init_info_t ma_init_info_marcel_lwp_finished;
extern const __ma_init_info_t ma_init_info_marcel_lwp_notifier_register;
#if defined(LINUX_SYS) || defined(GNU_SYS)
extern const __ma_init_info_t ma_init_info_marcel_random_lwp_notifier_register;
#endif				// LINUX_SYS || GNU_SYS
extern const __ma_init_info_t ma_init_info_marcel_generic_sched_notifier_register;
#ifdef MARCEL_POSTEXIT_ENABLED
extern const __ma_init_info_t ma_init_info_marcel_postexit_notifier_register;
#endif				/* MARCEL_POSTEXIT_ENABLED */
extern const __ma_init_info_t ma_init_info_marcel_linux_sched_notifier_register;
extern const __ma_init_info_t ma_init_info_marcel_ksoftirqd_notifier_register;
extern const __ma_init_info_t ma_init_info_marcel_timers_notifier_register;
extern const __ma_init_info_t ma_init_info_marcel_lwp_call_ONLINE;
extern const __ma_init_info_t ma_init_info_marcel_lwp_call_UP_PREPARE;
#if defined(LINUX_SYS) || defined(GNU_SYS)
extern const __ma_init_info_t ma_init_info_marcel_random_lwp_call_UP_PREPARE;
#endif				// LINUX_SYS || GNU_SYS
extern const __ma_init_info_t ma_init_info_marcel_generic_sched_call_UP_PREPARE;
#ifdef MARCEL_POSTEXIT_ENABLED
extern const __ma_init_info_t ma_init_info_marcel_postexit_call_UP_PREPARE;
#endif				/* MARCEL_POSTEXIT_ENABLED */
extern const __ma_init_info_t ma_init_info_marcel_linux_sched_call_UP_PREPARE;
extern const __ma_init_info_t ma_init_info_marcel_timers_call_UP_PREPARE;
#ifdef PROFILE
extern const __ma_init_info_t ma_init_info_marcel_int_catcher_call_ONLINE;
#endif				// PROFILE

// Section MA_INIT_SCHEDULER
extern const __ma_init_info_t ma_init_info_marcel_generic_sched_call_ONLINE;
#ifdef MARCEL_POSTEXIT_ENABLED
extern const __ma_init_info_t ma_init_info_marcel_postexit_call_ONLINE;
#endif				/* MARCEL_POSTEXIT_ENABLED */
extern const __ma_init_info_t ma_init_info_marcel_sig_timer_call_ONLINE;
extern const __ma_init_info_t ma_init_info_marcel_ksoftirqd_call_UP_PREPARE;
extern const __ma_init_info_t ma_init_info_marcel_ksoftirqd_call_ONLINE;

// Section MA_INIT_START_LWPS
#ifdef MA__LWPS
extern const __ma_init_info_t ma_init_info_marcel_gensched_start_lwps;
#endif

TBX_VISIBILITY_PUSH_INTERNAL __ma_init_index_t ma_init_start[MA_INIT_MAX_PARTS + 1] = { 
	{
		MA_INIT_SELF, "Init self"
	},
	{
		MA_INIT_TLS, "Init TLS"
	}, 
	{
		MA_INIT_MAIN_LWP, "Init Main LWP"
	}, 
	{
		MA_INIT_SCHEDULER, "Init scheduler"
	}, 
	{
		MA_INIT_START_LWPS,
		"Init LWPS"
	}
};

TBX_VISIBILITY_POP static void call_init_function(const __ma_init_info_t * infos)
{
	MARCEL_LOG("Init launching for prio %i: %s (%s)\n", 
		   (int) (MA_INIT_PRIO_BASE - infos->prio), *infos->debug, infos->file);
	infos->func();
}

void marcel_init_section(int sec)
{
	int section;

	MARCEL_LOG("Asked for level %d (%s)\n",
		   ma_init_start[sec].prio, ma_init_start[sec].debug);
	for (section = 0; section <= sec; section++) {
		if (ma_init_done[section])
			continue;
		MARCEL_LOG("Init running level %d (%s)\n",
			   ma_init_start[section].prio, ma_init_start[section].debug);

		if (section == MA_INIT_SELF) {
#ifdef PROFILE
			profile_init();
#endif
			ma_linux_sched_init0();
			call_init_function(&ma_init_info_main_thread_init);
#ifdef MA__LWPS
			call_init_function(&ma_init_info_initialize_topology);
#else
			marcel_nb_vp = 1;
#endif				// MA__LWPS
		} else if (section == MA_INIT_MAIN_LWP) {
			ma_allocator_init();
#ifdef MARCEL_DEVIATION_ENABLED
			ma_deviate_init();
#endif				/* MARCEL_DEVIATION_ENABLED */
#ifdef MARCEL_SIGNALS_ENABLED
			ma_signals_init();
#endif
#ifdef PROFILE
			call_init_function(&ma_init_info_marcel_int_catcher_call_ONLINE);
#endif				// PROFILE
			call_init_function(&ma_init_info_marcel_slot_init);
#ifdef MA__BUBBLES
			call_init_function(&ma_init_info_bubble_sched_init);
#endif				// MA__BUBBLES
			call_init_function(&ma_init_info_linux_sched_init);
#ifdef MA__BUBBLES
			ma_bubble_sched_init2();
#endif				// MA__BUBBLES
			call_init_function(&ma_init_info_marcel_lwp_call_UP_PREPARE);
#if defined(LINUX_SYS) || defined(GNU_SYS)
			call_init_function
			    (&ma_init_info_marcel_random_lwp_call_UP_PREPARE);
#endif				// LINUX_SYS || GNU_SYS
			call_init_function(&ma_init_info_marcel_lwp_call_ONLINE);
			call_init_function
			    (&ma_init_info_marcel_generic_sched_call_UP_PREPARE);
#ifdef MARCEL_POSTEXIT_ENABLED
			call_init_function(&ma_init_info_marcel_postexit_call_UP_PREPARE);
#endif				/* MARCEL_POSTEXIT_ENABLED */
			call_init_function(&ma_init_info_timer_start);
			call_init_function(&ma_init_info_sig_init);
			call_init_function(&ma_init_info_fork_handling_init);
			call_init_function
			    (&ma_init_info_marcel_linux_sched_call_UP_PREPARE);
			call_init_function(&ma_init_info_softirq_init);
			call_init_function(&ma_init_info_marcel_timers_call_UP_PREPARE);
#if defined(LINUX_SYS) || defined(GNU_SYS)
			call_init_function
			    (&ma_init_info_marcel_random_lwp_notifier_register);
#endif				// LINUX_SYS || GNU_SYS
			call_init_function(&ma_init_info_marcel_lwp_notifier_register);
			call_init_function
			    (&ma_init_info_marcel_generic_sched_notifier_register);
#ifdef MARCEL_POSTEXIT_ENABLED
			call_init_function
			    (&ma_init_info_marcel_postexit_notifier_register);
#endif				/* MARCEL_POSTEXIT_ENABLED */
			call_init_function
			    (&ma_init_info_marcel_sig_timer_notifier_register);
			call_init_function
			    (&ma_init_info_marcel_linux_sched_notifier_register);
			call_init_function
			    (&ma_init_info_marcel_ksoftirqd_notifier_register);
			call_init_function(&ma_init_info_marcel_timers_notifier_register);
			call_init_function(&ma_init_info_marcel_lwp_finished);
		} else if (section == MA_INIT_SCHEDULER) {
			call_init_function
			    (&ma_init_info_marcel_generic_sched_call_ONLINE);
#ifdef MARCEL_POSTEXIT_ENABLED
			call_init_function(&ma_init_info_marcel_postexit_call_ONLINE);
#endif				/* MARCEL_POSTEXIT_ENABLED */
			call_init_function(&ma_init_info_marcel_sig_timer_call_ONLINE);
			call_init_function
			    (&ma_init_info_marcel_ksoftirqd_call_UP_PREPARE);
			call_init_function(&ma_init_info_marcel_ksoftirqd_call_ONLINE);
		} else if (section == MA_INIT_START_LWPS) {
#ifdef MA__LWPS
			call_init_function(&ma_init_info_marcel_gensched_start_lwps);
#endif				// MA__LWPS
#ifdef MA__BUBBLES
			__ma_bubble_sched_start();
#endif				//MA__BUBBLES
		}

		ma_init_done[section] = 1;
	}
	MARCEL_LOG("Init running level %d (%s) done\n",
		   ma_init_start[sec].prio, ma_init_start[sec].debug);
}

#ifdef MA__LIBPTHREAD
static
void marcel_constructor(void) __attribute__ ((constructor));

static
void marcel_constructor(void)
{
	int argc = 0;

	if (!marcel_test_activity()) {
		MARCEL_LOG_IN();
#  ifdef __PIC__
		__pthread_initialize_minimal();
#  endif
		marcel_init(&argc, NULL);
		MARCEL_LOG_OUT();
	}
}
#endif
