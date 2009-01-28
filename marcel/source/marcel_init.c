
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


#define MA_FILE_DEBUG init
#include "marcel.h"
#include "tbx_compiler.h"
#ifdef LINUX_SYS
#include <sys/utsname.h>
#endif
#include <sys/resource.h>
#include <stdlib.h>
#include <string.h>

#ifdef MA__LIBPTHREAD
# include <Padico/Puk-ABI.h>
#endif


tbx_flag_t marcel_activity = tbx_flag_clear;



int marcel_test_activity(void)
{
	tbx_bool_t result;

	LOG_IN();
	result = tbx_flag_test(&marcel_activity);
	LOG_RETURN(result);
}

#ifdef STACKALIGN
void marcel_initialize(int* argc, char**argv)
{
	if (!marcel_test_activity()) {
		LOG_IN();
		marcel_init(argc,argv);
		LOG_OUT();
	}
}
#endif

#ifdef MA__NUMA

static void show_synthetic_topology_help(void)
{
	fprintf(stderr,
					"<topology-description> is a space-separated list of positive integers,\n"
					"each of which denotes the number of children attached to a each node of the\n"
					"corresponding topology level.\n\n"
					"Example: \"2 4 2\" denotes a 2-node machine with 4 dual-core CPUs.\n");
}

/* Read from DESCRIPTION a series of integers describing a symmetrical
	 topology and update `ma_synthetic_topology_description' accordingly.  On
	 success, return zero.  */
static int parse_synthetic_topology_description(const char *description) {
	const char *pos, *next_pos;
	unsigned long item, count;

	for (pos = description, count = 0;
			 *pos;
			 pos = next_pos) {
		item = strtoul(pos, (char **)&next_pos, 10);
		if(next_pos == pos)
			return 1;

		MA_BUG_ON(count + 1 >= MA_SYNTHETIC_TOPOLOGY_MAX_DEPTH);
		MA_BUG_ON(item > UINT_MAX);

		ma_synthetic_topology_description[count++] = (unsigned)item;
	}

	if(count > 0) {
		ma_synthetic_topology_description[count + 1] = 0;
		ma_use_synthetic_topology = 1;
	}

	return (count > 0) ? 0 : 1;
}

#endif /* MA__NUMA */


static void marcel_parse_cmdline_early(int *argc, char **argv,
    tbx_bool_t just_strip)
{
	int i, j;

	if (!argc)
		return;

	i = j = 1;

	while (i < *argc) {
		/* Handled in marcel_parse_cmdline_lastly, don't spit an error and don't drop them either */
		if (!strcmp(argv[i], "--marcel-top")) {
			if (just_strip) {
				argv[j++] = argv[i++];
				argv[j++] = argv[i++];
			} else
				i += 2;
		} else if (!strcmp(argv[i], "--marcel-xtop")) {
			if (just_strip)
				argv[j++] = argv[i++];
			else
				i++;
		} else
#ifdef MA__LWPS
		if (!strcmp(argv[i], "--marcel-nvp")) {
			i += 2;
			if (i > *argc) {
				fprintf(stderr,
				    "Fatal error: --marcel-nvp option must be followed "
				    "by <nb_of_virtual_processors>.\n");
				exit(1);
			}
			if (just_strip)
				continue;

			ma__nb_vp = atoi(argv[i - 1]);
			if (ma__nb_vp < 1 || ma__nb_vp > MA_NR_LWPS) {
				fprintf(stderr,
				    "Error: nb of VP should be between 1 and %ld\n",
				    (long) MA_NR_LWPS);
				exit(1);
			}
		} else if (!strcmp(argv[i], "--marcel-cpustride")) {
			int stride;

			i += 2;
			if (i > *argc) {
				fprintf(stderr,
				    "Fatal error: --marcel-cpustride option must be followed "
				    "by <nb_of_processors_to_seek>.\n");
				exit(1);
			}
			if (just_strip)
				continue;

			stride = atoi(argv[i - 1]);
			if (stride < 0) {
				fprintf(stderr,
				    "Error: CPU stride should be positive\n");
				exit(1);
			}
			marcel_cpu_stride = stride;
		} else if (!strcmp(argv[i], "--marcel-firstcpu")) {
			int first_cpu;

			i += 2;
			if (i > *argc) {
				fprintf(stderr,
				    "Fatal error: --marcel-firstcpu option must be followed "
				    "by <num_of_first_processor>.\n");
				exit(1);
			}
			if (just_strip)
				continue;

			first_cpu = atoi(argv[i - 1]);
			if (first_cpu < 0) {
				fprintf(stderr,
				    "Error: CPU number should be positive\n");
				exit(1);
			}
			marcel_first_cpu = first_cpu;
		} else
#ifdef MA__NUMA
		if (!strcmp(argv[i], "--marcel-maxarity")) {
			int topo_max_arity;

			i += 2;
			if (i > *argc) {
				fprintf(stderr,
				    "Fatal error: --marcel-maxarity option must be followed "
				    "by <maximum_topology_arity>.\n");
				exit(1);
			}
			if (just_strip)
				continue;

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
#ifdef MA__NUMA
		if (!strcmp(argv[i], "--marcel-synthetic-topology")) {
			int err;

			i += 2;
			if (i > *argc) {
				fprintf(stderr,
								"Fatal error: --marcel-synthetic-topology option must be followed "
								"by <topology-description>.\n");
				show_synthetic_topology_help();
				exit(1);
			}
			if (just_strip)
				continue;

			err = parse_synthetic_topology_description(argv[i - 1]);
			if (err) {
				fprintf (stderr,
								 "Fatal error: invalid argument for --marcel-synthetic-topology.\n");
				show_synthetic_topology_help();
				exit(1);
			}
		} else
#endif
#ifdef MA__BUBBLES
		if (!strcmp (argv[i], "--marcel-bubble-scheduler")) {
			const marcel_bubble_sched_t *scheduler;

			i += 2;
			if (i > *argc) {
				fprintf(stderr,
								"Fatal error: --marcel-bubble-scheduler option must be followed "
								"by the name of a bubble scheduler.\n");
				exit(1);
			}
			if (just_strip)
				continue;

			scheduler = marcel_lookup_bubble_scheduler(argv[i - 1]);
			if (scheduler == NULL) {
				fprintf (stderr,
								 "Fatal error: unknown bubble scheduler `%s'.\n", argv[i]);
				exit(1);
			}
			marcel_bubble_change_sched((marcel_bubble_sched_t *)scheduler);
		} else
#endif
#ifdef MA__NUMA
		if (!strcmp(argv[i], "--marcel-topology-fsys-root")) {
			i += 2;
			if (i > *argc) {
				fprintf(stderr,
								"Fatal error: --marcel-topology-fsys-root option must be followed "
								"by the path of a directory.\n");
				exit(1);
			}
			if (just_strip)
				continue;

			if (ma_topology_set_fsys_root(argv[i - 1])) {
				fprintf(stderr, "failed to set root directory to `%s': %m\n", argv[i - 1]);
				exit(1);
			}

			/* Notify Marcel that we are using a "virtual" topology.  */
			ma_use_synthetic_topology = 1;
		} else
#endif
		if (!strncmp(argv[i], "--marcel", 8)) {
			fprintf(stderr, "--marcel flags are:\n"
			    "--marcel-top file		Dump a top-like output to file\n"
			    "--marcel-top |cmd		Same as above, but to command cmd via a pipe\n"
			    "--marcel-xtop		Same as above, in a freshly created xterm\n"
#ifdef MA__LWPS
			    "--marcel-nvp n		Force number of VPs to n\n"
			    "--marcel-firstcpu cpu	Allocate VPs from the given cpu"
#ifdef MA__NUMA
			    " (topologic number, not OS number)"
#endif
			    "\n"
			    "--marcel-cpustride stride	Allocate VPs every stride cpus"
#ifdef MA__NUMA
			    " (topologic numbers, not OS numbers)"
#endif
			    "\n"
#ifdef MA__NUMA
			    "--marcel-maxarity arity	Insert fake levels until topology arity is at most arity\n"
#endif
#endif
#ifdef MA__BUBBLES
					"--marcel-bubble-scheduler sched   Use the given bubble scheduler\n"
#endif
#ifdef MA__NUMA
					"--marcel-synthetic-topology topo  Create a synthetic or \"fake\" topology\n"
					"                                  according to the given description\n"
					"--marcel-topology-fsys-root path  Use the given path as the root directory\n"
					"                                  when accessing, e.g., `/sys' on Linux\n"
#endif
			    );
			exit(1);
		} else {
			if (just_strip)
				argv[j++] = argv[i++];
			else
				i++;
		}
	}

	if (just_strip) {
		*argc = j;
		argv[j] = NULL;
	} else  {
#ifdef MA__LWPS
		mdebug
		    ("\t\t\t<Suggested nb of Virtual Processors : %d, stride %d, first %d>\n",
		     ma__nb_vp, marcel_cpu_stride, marcel_first_cpu);
#endif
	}
}

/* Options that need Marcel to be alread up and running */
static void marcel_parse_cmdline_lastly(int *argc, char **argv,
    tbx_bool_t just_strip)
{
	int i, j;

	if (!argc)
		return;

	i = j = 1;

	while (i < *argc) {
		if (!strcmp(argv[i], "--marcel-top")) {
			i += 2;
			if (i > *argc) {
				fprintf(stderr,
				    "Fatal error: --marcel-top option must be followed "
				    "by <out_file>.\n");
				exit(1);
			}
			if (just_strip)
				continue;

			if (marcel_init_top(argv[i - 1])) {
				fprintf(stderr,
				    "Error: invalid top out_file %s\n",
				    argv[i - 1]);
				exit(1);
			}
		} else if (!strcmp(argv[i], "--marcel-xtop")) {
			i++;
			if (just_strip)
				continue;

			if (marcel_init_top
			    ("|xterm -S//0 -geometry 120x26")) {
				fprintf(stderr,
				    "Error: can't launch xterm\n");
				exit(1);
			}
		} else {
			if (just_strip)
				argv[j++] = argv[i++];
			else
				i++;
		}
	}
	if (just_strip) {
		*argc = j;
		argv[j] = NULL;
	}
}

static void marcel_strip_cmdline(int *argc, char *argv[])
{
	marcel_parse_cmdline_early(argc, argv, tbx_true);

	marcel_debug_init(argc, argv, PM2DEBUG_CLEAROPT);

	marcel_parse_cmdline_lastly(argc, argv, tbx_true);
}

#ifdef MA__LIBPTHREAD

#include <dlfcn.h>

/* Issue a warning if Marcel and PukABI symbols don't prevail over
   libpthread/libc symbols, i.e., if they were not preloaded.  */
static void
assert_preloaded (void) {
	void *self;
	tbx_bool_t determined = tbx_false;

	self = dlopen(NULL, RTLD_LAZY);

	if (self != NULL) {
		void *marcel, *pukabi;

		marcel = dlopen("libpthread.so", RTLD_LAZY);
		if(marcel != NULL) {
			void *global_pcreate, *marcel_pcreate;

			global_pcreate = dlsym(self, "pthread_create");
			marcel_pcreate = dlsym(marcel, "pthread_create");

			if (marcel_pcreate != NULL && global_pcreate != NULL) {
				determined = tbx_true;
				if (marcel_pcreate != global_pcreate) {
					/* This is unlikely.  It would mean that somehow two different
						 `libpthread.so' were loaded.  */
					fprintf(stderr, "[Marcel] it appears that Marcel's libpthread "
						"was not preloaded\n");
					fprintf(stderr, "[Marcel] make sure to preload it "
						"using the `LD_PRELOAD' environment variable\n");
					abort();
				}
			}

			dlclose (marcel);
		}

		pukabi = dlopen("libPukABI.so", RTLD_LAZY);
		if (pukabi != NULL) {
			void *global_malloc, *pukabi_malloc;

			global_malloc = dlsym(self, "malloc");
			pukabi_malloc = dlsym(pukabi, "malloc");

			if (pukabi_malloc != NULL && global_malloc != NULL) {
				determined = tbx_true;
				if (pukabi_malloc != global_malloc) {
					fprintf(stderr, "[Marcel] it appears that PukABI "
						"was not preloaded\n");
					fprintf(stderr, "[Marcel] make sure to preload it "
						"using the `LD_PRELOAD' environment variable\n");
				}
			}

			dlclose (pukabi);
		}

		dlclose (self);
	}

	if (!determined) {
		fprintf(stderr, "[Marcel] unable to determine whether "
			"libpthread and/or PukABI were correctly preloaded");
		fprintf(stderr, "[Marcel] are both libraries in the loader "
			"search path?\n");
	}
}

#endif

// Does not start any internal threads.
// When completed, fork calls are still allowed.
void marcel_init_data(int *argc, char *argv[])
{
	static volatile tbx_bool_t already_called = tbx_false;

	// Only execute this function once
	if (already_called)
		return;
	already_called = tbx_true;

	// Parse command line
	marcel_parse_cmdline_early(argc, argv, tbx_false);

	// Windows/Cygwin specific stuff
	marcel_win_sys_init();

	marcel_init_section(MA_INIT_SCHEDULER);

	// Initialize debug facilities
	marcel_debug_init(argc, argv, PM2DEBUG_DO_OPT);

	marcel_parse_cmdline_lastly(argc, argv, tbx_false);

#ifdef MA__LIBPTHREAD
	puk_abi_init();

	/* Tell PukABI how to protect libc calls.  */
	puk_abi_set_spinlock_handlers (marcel_extlib_protect, marcel_extlib_unprotect, NULL);

	assert_preloaded ();
#endif
}

// When completed, some threads may be started
// Fork calls are now prohibited in non libpthread versions
void marcel_start_sched(void)
{
	// Start scheduler (i.e. run LWP/activations, start timer)
	marcel_activity = tbx_flag_set;
	marcel_init_section(MA_INIT_START_LWPS);
}

void marcel_purge_cmdline(int *argc, char *argv[])
{
	static volatile tbx_bool_t already_called = tbx_false;

	marcel_init_section(MA_INIT_MAX_PARTS);

	// Only execute this function once
	if (already_called)
		return;
	already_called = tbx_true;

	// Remove marcel-specific arguments from command line
	marcel_strip_cmdline(argc, argv);
}

void marcel_finish_prepare(void)
{
	marcel_gensched_shutdown();
	marcel_activity = tbx_flag_clear;
}

void marcel_finish(void)
{
	marcel_slot_exit();
	ma_topo_exit();
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
	if (!marcel_header_hash_matches_binary (header_hash))
	{
		fprintf(stderr, "error: Marcel binary incompatibility detected\n");
		fprintf(stderr, "client header hash:  %s\n", header_hash);
		fprintf(stderr, "library header hash: %s\n", valid_header_hash);
		exit(1);
	}
}

#ifndef STANDARD_MAIN

#ifdef WIN_SYS
void win_stack_allocate(unsigned n)
{
	int tab[n];

	tab[0] = 0;
}
#endif				// WIN_SYS

volatile int __marcel_main_ret;
marcel_ctx_t __ma_initial_main_ctx;

#ifdef MARCEL_MAIN_AS_FUNC
int go_marcel_main(int (*main_func)(int, char*[]), int argc, char *argv[])
#else /* MARCEL_MAIN_AS_FUNC */
extern int marcel_main(int argc, char *argv[]);

int main(int argc, char *argv[])
#endif /* MARCEL_MAIN_AS_FUNC */
{
	static int __ma_argc;
	static char **__ma_argv;
	static int (*__ma_main)(int, char*[]);
	unsigned long new_sp;

#ifdef LINUX_SYS
	struct utsname utsname;

	uname(&utsname);
	if (strncmp(utsname.release, LINUX_VERSION, strlen(LINUX_VERSION))) {
		fprintf(stderr,"Marcel was compiled for Linux "LINUX_VERSION", but you are running Linux %s, can't continue\n",utsname.version);
		exit(1);
	}
#endif

	marcel_debug_init(&argc, argv, PM2DEBUG_DO_OPT|PM2DEBUG_CLEAROPT);

	if(!marcel_ctx_setjmp(__ma_initial_main_ctx)) {
		struct rlimit rlim;

#ifdef MA__SELF_VAR
		ma_self =
#endif
		__main_thread = (marcel_t)((((unsigned long)get_sp() - 128) &
					    ~(THREAD_SLOT_SIZE-1)) -
					   MAL(sizeof(marcel_task_t)));

		mdebug("\t\t\t<main_thread is %p>\n", __main_thread);

                __ma_argc = argc; __ma_argv = argv;
#ifdef MARCEL_MAIN_AS_FUNC
		__ma_main = main_func;
#else
		__ma_main = &marcel_main;
#endif
		new_sp = (unsigned long)__main_thread - TOP_STACK_FREE_AREA;

		getrlimit(RLIMIT_STACK, &rlim);
		if (get_sp() - new_sp > rlim.rlim_cur) {
			fprintf(stderr,"The current stack resource limit is too small (%ld) for the chosen marcel stack size (%ld).  Please increase it (ulimit -s, and maximum is %ld) or decrease THREAD_SLOT_SIZE in marcel/include/sys/isomalloc_archdep.h\n", (long)rlim.rlim_cur, (long)THREAD_SLOT_SIZE, (long) rlim.rlim_max);
			abort();
		}
		if (THREAD_SLOT_SIZE <= sizeof(struct marcel_task)) {
			fprintf(stderr,"THREAD_SLOT_SIZE is too small (%ld) to hold "
							"`marcel_t' (%zd) and the thread's stack.  Please "
							"increase it in marcel/include/sys/isomalloc_archdep.h\n",
							THREAD_SLOT_SIZE, sizeof(struct marcel_task));
			abort();
		}

		/* On se contente de descendre la pile. Tout va bien, même sur Itanium */
#ifdef WIN_SYS
		win_stack_allocate(get_sp() - new_sp);
#endif
		set_sp(new_sp);

                __marcel_main_ret = (*__ma_main)(__ma_argc, __ma_argv);

		marcel_ctx_longjmp(__ma_initial_main_ctx, 1);
	}

	return __marcel_main_ret;
}


#endif // STANDARD_MAIN

typedef struct {
	int prio;
	const char* debug;
} TBX_ALIGNED __ma_init_index_t;

typedef struct {
	struct {
		int section_number;
	} a TBX_ALIGNED;
	struct {
		__ma_init_info_t infos[2];
	} b TBX_ALIGNED;
} TBX_ALIGNED __ma_init_section_index_t;

int ma_init_done[MA_INIT_MAX_PARTS+1]={0,};

// Section MA_INIT_SELF
#ifdef MA__LIBPTHREAD
extern void ma_check_lpt_sizes(void);
#endif
#ifdef MA__LWPS
extern const __ma_init_info_t ma_init_info_initialize_topology;
extern const __ma_init_info_t ma_init_info_marcel_topology_notifier_register;
extern const __ma_init_info_t ma_init_info_marcel_topology_call_UP_PREPARE;
#endif // MA__LWPS
extern const __ma_init_info_t ma_init_info_main_thread_init;

// Section MA_INIT_MAIN_LWP
extern const __ma_init_info_t ma_init_info_marcel_debug_init_auto;
extern const __ma_init_info_t ma_init_info_marcel_slot_init;
extern const __ma_init_info_t ma_init_info_linux_sched_init;
#ifdef MA__BUBBLES
extern const __ma_init_info_t ma_init_info_bubble_sched_init;
#endif // MA__BUBBLES
extern const __ma_init_info_t ma_init_info_softirq_init;
extern const __ma_init_info_t ma_init_info_marcel_io_init;
#ifndef __MINGW32__
extern const __ma_init_info_t ma_init_info_sig_init;
extern const __ma_init_info_t ma_init_info_marcel_sig_timer_notifier_register;
#endif
extern const __ma_init_info_t ma_init_info_timer_start;
extern const __ma_init_info_t ma_init_info_marcel_lwp_finished;
extern const __ma_init_info_t ma_init_info_marcel_lwp_notifier_register;
#if defined(LINUX_SYS) || defined(GNU_SYS)
extern const __ma_init_info_t ma_init_info_marcel_random_lwp_notifier_register;
#endif // LINUX_SYS || GNU_SYS
extern const __ma_init_info_t ma_init_info_marcel_generic_sched_notifier_register;
#ifdef MARCEL_POSTEXIT_ENABLED
extern const __ma_init_info_t ma_init_info_marcel_postexit_notifier_register;
#endif /* MARCEL_POSTEXIT_ENABLED */
extern const __ma_init_info_t ma_init_info_marcel_linux_sched_notifier_register;
extern const __ma_init_info_t ma_init_info_marcel_ksoftirqd_notifier_register;
extern const __ma_init_info_t ma_init_info_marcel_timers_notifier_register;
extern const __ma_init_info_t ma_init_info_marcel_lwp_call_ONLINE;
extern const __ma_init_info_t ma_init_info_marcel_lwp_call_UP_PREPARE;
#if defined(LINUX_SYS) || defined(GNU_SYS)
extern const __ma_init_info_t ma_init_info_marcel_random_lwp_call_UP_PREPARE;
#endif // LINUX_SYS || GNU_SYS
extern const __ma_init_info_t ma_init_info_marcel_generic_sched_call_UP_PREPARE;
#ifdef MARCEL_POSTEXIT_ENABLED
extern const __ma_init_info_t ma_init_info_marcel_postexit_call_UP_PREPARE;
#endif /* MARCEL_POSTEXIT_ENABLED */
extern const __ma_init_info_t ma_init_info_marcel_linux_sched_call_UP_PREPARE;
extern const __ma_init_info_t ma_init_info_marcel_timers_call_UP_PREPARE;
#ifdef PROFILE
extern const __ma_init_info_t ma_init_info_marcel_int_catcher_call_ONLINE;
#endif // PROFILE

// Section MA_INIT_SCHEDULER
extern const __ma_init_info_t ma_init_info_marcel_generic_sched_call_ONLINE;
#ifdef MARCEL_POSTEXIT_ENABLED
extern const __ma_init_info_t ma_init_info_marcel_postexit_call_ONLINE;
#endif /* MARCEL_POSTEXIT_ENABLED */
extern const __ma_init_info_t ma_init_info_marcel_sig_timer_call_ONLINE;
extern const __ma_init_info_t ma_init_info_marcel_ksoftirqd_call_UP_PREPARE;
extern const __ma_init_info_t ma_init_info_marcel_ksoftirqd_call_ONLINE;

// Section MA_INIT_START_LWPS
#ifdef MA__LWPS
extern const __ma_init_info_t ma_init_info_marcel_gensched_start_lwps;
#endif

__ma_init_index_t ma_init_start[MA_INIT_MAX_PARTS+1] = {{MA_INIT_SELF, "Init self"},
                                                        {MA_INIT_TLS,  "Init TLS"},
                                                        {MA_INIT_MAIN_LWP, "Init Main LWP"},
                                                        {MA_INIT_SCHEDULER, "Init scheduler"},
                                                        {MA_INIT_START_LWPS, "Init LWPS"}};

static void call_init_function(const __ma_init_info_t *infos) {
	mdebug("Init launching for prio %i: %s (%s)\n",
               (int)(MA_INIT_PRIO_BASE-infos->prio),
               *infos->debug, infos->file);
        infos->func();
}

#ifdef MARCEL_LIBNUMA
extern void numa_init(void);
#pragma weak numa_init
#endif

void marcel_init_section(int sec)
{
	int section;

	/* Quick registration */
	pm2debug_register(&MA_DEBUG_VAR_NAME(MA_FILE_DEBUG));
	//pm2debug_setup(&marcel_mdebug,PM2DEBUG_SHOW,PM2DEBUG_ON);

	mdebug("Asked for level %d (%s)\n",
	    ma_init_start[sec].prio, ma_init_start[sec].debug);
	for (section = 0; section <= sec; section++) {
		if (ma_init_done[section])
			continue;
		mdebug("Init running level %d (%s)\n",
		    ma_init_start[section].prio, ma_init_start[section].debug);

		if (section == MA_INIT_SELF) {
#ifdef MARCEL_LIBNUMA
			if (numa_init)
				numa_init();
#endif
#ifdef PROFILE
			profile_init();
#endif
			ma_linux_sched_init0();
#ifdef MA__LIBPTHREAD
			ma_check_lpt_sizes();
#endif
			call_init_function(&ma_init_info_main_thread_init);
#ifdef MA__LWPS
			call_init_function(&ma_init_info_initialize_topology);
			call_init_function(&ma_init_info_marcel_topology_notifier_register);
			call_init_function(&ma_init_info_marcel_topology_call_UP_PREPARE);
#endif				// MA__LWPS
		} else if (section == MA_INIT_MAIN_LWP) {
			ma_allocator_init();
#ifdef MARCEL_DEVIATION_ENABLED
			ma_deviate_init();
#endif /* MARCEL_DEVIATION_ENABLED */
#ifdef MARCEL_SIGNALS_ENABLED
			ma_signals_init();
#endif
#ifdef PROFILE
			call_init_function(&ma_init_info_marcel_int_catcher_call_ONLINE);
#endif				// PROFILE
			call_init_function(&ma_init_info_marcel_debug_init_auto);
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
			call_init_function(&ma_init_info_marcel_random_lwp_call_UP_PREPARE);
#endif				// LINUX_SYS || GNU_SYS
			call_init_function(&ma_init_info_marcel_lwp_call_ONLINE);
			call_init_function(&ma_init_info_marcel_generic_sched_call_UP_PREPARE);
#ifdef MARCEL_POSTEXIT_ENABLED
			call_init_function(&ma_init_info_marcel_postexit_call_UP_PREPARE);
#endif /* MARCEL_POSTEXIT_ENABLED */
			call_init_function(&ma_init_info_timer_start);
#ifndef __MINGW32__
			call_init_function(&ma_init_info_sig_init);
#endif
			call_init_function(&ma_init_info_marcel_linux_sched_call_UP_PREPARE);
			call_init_function(&ma_init_info_softirq_init);
			call_init_function(&ma_init_info_marcel_timers_call_UP_PREPARE);
			call_init_function(&ma_init_info_marcel_io_init);
#if defined(LINUX_SYS) || defined(GNU_SYS)
			call_init_function(&ma_init_info_marcel_random_lwp_notifier_register);
#endif				// LINUX_SYS || GNU_SYS
			call_init_function(&ma_init_info_marcel_lwp_notifier_register);
			call_init_function(&ma_init_info_marcel_generic_sched_notifier_register);
#ifdef MARCEL_POSTEXIT_ENABLED
			call_init_function(&ma_init_info_marcel_postexit_notifier_register);
#endif /* MARCEL_POSTEXIT_ENABLED */
#ifndef __MINGW32__
			call_init_function(&ma_init_info_marcel_sig_timer_notifier_register);
#endif
			call_init_function(&ma_init_info_marcel_linux_sched_notifier_register);
			call_init_function(&ma_init_info_marcel_ksoftirqd_notifier_register);
			call_init_function(&ma_init_info_marcel_timers_notifier_register);
			call_init_function(&ma_init_info_marcel_lwp_finished);
		} else if (section == MA_INIT_SCHEDULER) {
			call_init_function(&ma_init_info_marcel_generic_sched_call_ONLINE);
#ifdef MARCEL_POSTEXIT_ENABLED
			call_init_function(&ma_init_info_marcel_postexit_call_ONLINE);
#endif /* MARCEL_POSTEXIT_ENABLED */
			call_init_function(&ma_init_info_marcel_sig_timer_call_ONLINE);
			call_init_function(&ma_init_info_marcel_ksoftirqd_call_UP_PREPARE);
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
	mdebug("Init running level %d (%s) done\n",
	    ma_init_start[sec].prio, ma_init_start[sec].debug);
}

#ifdef MA__LIBPTHREAD
#  ifndef STACKALIGN
#    ifdef STANDARD_MAIN
static
void
marcel_constructor(void) __attribute__((constructor));

static
void
marcel_constructor(void) {
	if (!marcel_test_activity()) {
		LOG_IN();
		marcel_init(NULL, NULL);
		LOG_OUT();
	}
}
#    else
#      error STANDARD_MAIN option required
#    endif
#  endif
#endif
