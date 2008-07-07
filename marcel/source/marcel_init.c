
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

/* Code de d�marrage de marcel */


#define MA_FILE_DEBUG init
#include "marcel.h"
#include "tbx_compiler.h"
#ifdef LINUX_SYS
#include <sys/utsname.h>
#endif
#include <sys/resource.h>
#include <stdlib.h>
#include <string.h>

#if (defined MARCEL_MALLOC_PREEMPTION_DEBUG) && (defined __GNU_LIBRARY__)
# include <malloc.h>
#endif

tbx_flag_t marcel_activity = tbx_flag_clear;

#if (defined MARCEL_MALLOC_PREEMPTION_DEBUG) && (defined __GNU_LIBRARY__)

/* Malloc hooks.  */

static void * (* previous_malloc_hook) (size_t, const void *);
static void * (* previous_realloc_hook) (void *, size_t, const void *);
static void (* previous_free_hook) (void *, const void *);
static void * (* previous_memalign_hook) (size_t, size_t, const void *);

static void *ma_malloc_hook(size_t size, const void *caller);
static void *ma_realloc_hook(void *ptr, size_t size, const void *caller);
static void  ma_free_hook(void *ptr, const void *caller);
static void *ma_memalign_hook(size_t alignment, size_t size, const void *caller);


/* Install the `malloc' hooks that were current before Marcel was
	 initialized.  */
#define INSTALL_INITIAL_MALLOC_HOOKS()					\
  do {																					\
		__malloc_hook = previous_malloc_hook;				\
		__realloc_hook = previous_realloc_hook;			\
		__free_hook = previous_free_hook;						\
		__memalign_hook = previous_memalign_hook;		\
	} while (0)

/* Install Marcel's own `malloc' hooks.  */
#define INSTALL_MARCEL_MALLOC_HOOKS()						\
  do {																					\
		__malloc_hook = ma_malloc_hook;							\
		__realloc_hook = ma_realloc_hook;						\
		__free_hook = ma_free_hook;									\
		__memalign_hook = ma_memalign_hook;					\
	} while (0)

/* Make sure the current LWP is not executing one of libc's `malloc'-related
	 functions.  The goal is to make sure LWPs are not preempted while
	 executing such functions.  */
#define ASSERT_LWP_NOT_IN_LIBC_MALLOC()												\
  do {																												\
		if (__ma_get_lwp_var(in_libc_malloc))											\
			abort();																								\
		/* MA_BUG_ON(!marcel_thread_is_preemption_disabled()); */	\
	} while (0)


static void *ma_malloc_hook(size_t size, const void *caller)
{
	void *result;

	ASSERT_LWP_NOT_IN_LIBC_MALLOC ();

	INSTALL_INITIAL_MALLOC_HOOKS ();
	__ma_get_lwp_var(in_libc_malloc)++;

	if (__malloc_hook != NULL) {
		MA_BUG_ON(__malloc_hook == ma_malloc_hook);
		result = __malloc_hook(size, caller);
	}
	else
		result = malloc(size);

	__ma_get_lwp_var(in_libc_malloc)--;
	INSTALL_MARCEL_MALLOC_HOOKS ();

	return result;
}

static void *ma_realloc_hook(void *ptr, size_t size, const void *caller)
{
	void *result;

	ASSERT_LWP_NOT_IN_LIBC_MALLOC ();

	INSTALL_INITIAL_MALLOC_HOOKS ();
	__ma_get_lwp_var(in_libc_malloc)++;

	if (__realloc_hook != NULL) {
		MA_BUG_ON(__realloc_hook == ma_realloc_hook);
		result = __realloc_hook(ptr, size, caller);
	}
	else
		result = realloc(ptr, size);

	__ma_get_lwp_var(in_libc_malloc)--;
	INSTALL_MARCEL_MALLOC_HOOKS ();

	return result;
}

static void ma_free_hook(void *ptr, const void *caller)
{
	ASSERT_LWP_NOT_IN_LIBC_MALLOC ();

	INSTALL_INITIAL_MALLOC_HOOKS ();
	__ma_get_lwp_var(in_libc_malloc)++;

	if (__free_hook != NULL) {
		MA_BUG_ON(__free_hook == ma_free_hook);
		__free_hook(ptr, caller);
	}
	else
		free(ptr);

	__ma_get_lwp_var(in_libc_malloc)--;
	INSTALL_MARCEL_MALLOC_HOOKS ();
}

static void *ma_memalign_hook(size_t alignment, size_t size, const void *caller)
{
	void *result;

	ASSERT_LWP_NOT_IN_LIBC_MALLOC ();

	INSTALL_INITIAL_MALLOC_HOOKS ();
	__ma_get_lwp_var(in_libc_malloc)++;

	if (__memalign_hook != NULL) {
		MA_BUG_ON(__memalign_hook == ma_memalign_hook);
		result = __memalign_hook(alignment, size, caller);
	}
	else
		result = memalign(alignment, size);

	__ma_get_lwp_var(in_libc_malloc)--;
	INSTALL_MARCEL_MALLOC_HOOKS ();

	return result;
}

#endif

/* Initialize Marcel's `malloc' debugging tools.  */
static void marcel_malloc_debug_init(void)
{
#ifdef MARCEL_MALLOC_PREEMPTION_DEBUG
# ifndef __GNU_LIBRARY__
#  ifdef __GNUC__
#   warning "The `MARCEL_MALLOC_PREEMPTION_DEBUG' option has no effect on non-GNU systems"
#  endif
# else
	/* Use glibc's malloc hooks to detect "unprotected" uses of `malloc ()',
		 `free ()' and friends.  "Unprotected" means that preemption is not
		 disabled while one of these functions is called; `marcel_malloc ()' and
		 friends do make sure that preemption is disabled before calling the libc
		 functions.  */
	previous_malloc_hook = __malloc_hook;
	previous_realloc_hook = __realloc_hook;
	previous_free_hook = __free_hook;
	previous_memalign_hook = __memalign_hook;

	INSTALL_MARCEL_MALLOC_HOOKS ();
# endif
#endif
}

/* Reinstall the initial `malloc' hooks.  */
static void marcel_malloc_debug_finish(void)
{
#if (defined MARCEL_MALLOC_PREEMPTION_DEBUG) && (defined __GNU_LIBRARY__)
	INSTALL_INITIAL_MALLOC_HOOKS ();
#endif
}



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

static void marcel_parse_cmdline_early(int *argc, char **argv,
    tbx_bool_t do_not_strip)
{
	int i, j;
#ifdef MA__LWPS
	unsigned __nb_lwp = 0;
#endif

	if (!argc)
		return;

	i = j = 1;

	while (i < *argc) {
		if (!strcmp(argv[i], "--marcel-top")) {
			argv[j++] = argv[i++];
			argv[j++] = argv[i++];
			continue;
		} else if (!strcmp(argv[i], "--marcel-xtop")) {
			argv[j++] = argv[i++];
			continue;
		} else
#ifdef MA__LWPS
		if (!strcmp(argv[i], "--marcel-nvp")) {
			if (i == *argc - 1) {
				fprintf(stderr,
				    "Fatal error: --marcel-nvp option must be followed "
				    "by <nb_of_virtual_processors>.\n");
				exit(1);
			}
			if (do_not_strip) {
				__nb_lwp = atoi(argv[i + 1]);
				if (__nb_lwp < 1 || __nb_lwp > MA_NR_LWPS) {
					fprintf(stderr,
					    "Error: nb of VP should be between 1 and %ld\n",
					    (long) MA_NR_LWPS);
					exit(1);
				}
				argv[j++] = argv[i++];
				argv[j++] = argv[i++];
			} else
				i += 2;
			continue;
		} else if (!strcmp(argv[i], "--marcel-cpustride")) {
			if (i == *argc - 1) {
				fprintf(stderr,
				    "Fatal error: --marcel-cpustride option must be followed "
				    "by <nb_of_processors_to_seek>.\n");
				exit(1);
			}
			if (do_not_strip) {
				marcel_cpu_stride = atoi(argv[i + 1]);
				if (marcel_cpu_stride < 0) {
					fprintf(stderr,
					    "Error: CPU stride should be positive\n");
					exit(1);
				}
				argv[j++] = argv[i++];
				argv[j++] = argv[i++];
			} else
				i += 2;
			continue;
		} else if (!strcmp(argv[i], "--marcel-firstcpu")) {
			if (i == *argc - 1) {
				fprintf(stderr,
				    "Fatal error: --marcel-firstcpu option must be followed "
				    "by <num_of_first_processor>.\n");
				exit(1);
			}
			if (do_not_strip) {
				marcel_first_cpu = atoi(argv[i + 1]);
				if (marcel_first_cpu < 0) {
					fprintf(stderr,
					    "Error: CPU number should be positive\n");
					exit(1);
				}
				argv[j++] = argv[i++];
				argv[j++] = argv[i++];
			} else
				i += 2;
			continue;
		} else
#ifdef MA__NUMA
		if (!strcmp(argv[i], "--marcel-maxarity")) {
			if (i == *argc - 1) {
				fprintf(stderr,
				    "Fatal error: --marcel-maxarity option must be followed "
				    "by <maximum_topology_arity>.\n");
				exit(1);
			}
			if (do_not_strip) {
				marcel_topo_max_arity = atoi(argv[i + 1]);
				if (marcel_topo_max_arity < 0) {
					fprintf(stderr,
					    "Error: Maximum topology arity should be positive\n");
					exit(1);
				}
				argv[j++] = argv[i++];
				argv[j++] = argv[i++];
			} else
				i += 2;
			continue;
		} else
#endif
#endif
		if (!strncmp(argv[i], "--marcel", 8)) {
			fprintf(stderr, "--marcel flags are:\n"
			    "--marcel-top file		Dump a top-like output to file\n"
			    "--marcel-top |cmd		Same as above, but to command cmd via a pipe\n"
			    "--marcel-xtop		Same as above, in a freshly created xterm\n"
#ifdef MA__LWPS
			    "--marcel-nvp n		Force number of VPs to n\n"
			    "--marcel-firstcpu cpu	Allocate VPs from the given cpu\n"
			    "--marcel-cpustride stride	Allocate VPs every stride cpus\n"
#ifdef MA__NUMA
			    "--marcel-maxarity arity	Insert fake levels until topology arity is at most arity\n"
#endif
#endif
			    );
			exit(1);
		} else
			argv[j++] = argv[i++];
	}
	*argc = j;
	argv[j] = NULL;

	if (do_not_strip) {
#ifdef MA__LWPS
		marcel_lwp_fix_nb_vps(__nb_lwp);
		mdebug
		    ("\t\t\t<Suggested nb of Virtual Processors : %d, stride %d, first %d>\n",
		     __nb_lwp, marcel_cpu_stride, marcel_first_cpu);
#endif
	}
}

static void marcel_parse_cmdline_lastly(int *argc, char **argv,
    tbx_bool_t do_not_strip)
{
	int i, j;

	if (!argc)
		return;

	i = j = 1;

	while (i < *argc) {
		if (!strcmp(argv[i], "--marcel-top")) {
			if (i == *argc - 1) {
				fprintf(stderr,
				    "Fatal error: --marcel-top option must be followed "
				    "by <out_file>.\n");
				exit(1);
			}
			if (do_not_strip) {
				if (marcel_init_top(argv[i + 1])) {
					fprintf(stderr,
					    "Error: invalid top out_file %s\n",
					    argv[i + 1]);
					exit(1);
				}
				argv[j++] = argv[i++];
				argv[j++] = argv[i++];
			} else
				i += 2;
			continue;
		} else if (!strcmp(argv[i], "--marcel-xtop")) {
			if (do_not_strip) {
				if (marcel_init_top
				    ("|xterm -S//0 -geometry 120x26")) {
					fprintf(stderr,
					    "Error: can't launch xterm\n");
					exit(1);
				}
				argv[j++] = argv[i++];
			} else
				i++;
		} else
			argv[j++] = argv[i++];
	}
	*argc = j;
	argv[j] = NULL;
}

void marcel_strip_cmdline(int *argc, char *argv[])
{
	marcel_parse_cmdline_early(argc, argv, tbx_false);

	marcel_debug_init(argc, argv, PM2DEBUG_CLEAROPT);

	marcel_parse_cmdline_lastly(argc, argv, tbx_false);
}

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
	marcel_parse_cmdline_early(argc, argv, tbx_true);

	// Windows/Cygwin specific stuff
	marcel_win_sys_init(argc, argv);

	marcel_init_section(MA_INIT_SCHEDULER);

	// Initialize debug facilities
	marcel_debug_init(argc, argv, PM2DEBUG_DO_OPT);

	marcel_malloc_debug_init();

	marcel_parse_cmdline_lastly(argc, argv, tbx_true);
}

// When completed, some threads may be started
// Fork calls are now prohibited in non libpthread versions
void marcel_start_sched(int *argc, char *argv[])
{
	// Start scheduler (i.e. run LWP/activations, start timer)
	marcel_init_section(MA_INIT_START_LWPS);
	marcel_activity = tbx_flag_set;
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
	marcel_malloc_debug_finish();
}

void marcel_finish(void)
{
	marcel_slot_exit();
	ma_topo_exit();
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
			fprintf(stderr,"The current stack resource limit is too small (%ld) for the chosen marcel stack size (%ld).  Please increase it (ulimit -s, and maximum is %ld) or decrease THREAD_SLOT_SIZE in marcel/include/sys/isomalloc_archdep.h", (long)rlim.rlim_cur, (long)THREAD_SLOT_SIZE, (long) rlim.rlim_max);
			abort();
		}

		/* On se contente de descendre la pile. Tout va bien, m�me sur Itanium */
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
	char* debug;
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
extern const __ma_init_info_t ma_init_info_topo_discover;
extern const __ma_init_info_t ma_init_info_marcel_topology_notifier_register;
extern const __ma_init_info_t ma_init_info_marcel_topology_call_UP_PREPARE;
#endif // MA__LWPS
extern const __ma_init_info_t ma_init_info_main_thread_init;

// Section MA_INIT_MAIN_LWP
extern const __ma_init_info_t ma_init_info_marcel_debug_init_auto;
extern const __ma_init_info_t ma_init_info_marcel_slot_init;
extern const __ma_init_info_t ma_init_info_linux_sched_init;
extern void ma_linux_sched_init0(void);
#ifdef MA__BUBBLES
extern const __ma_init_info_t ma_init_info_bubble_sched_init;
extern void ma_bubble_sched_init2(void);
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
#ifdef MA__BUBBLES
extern void __ma_bubble_sched_start(void);
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
#ifdef PROFILE
			profile_init();
#endif
			ma_linux_sched_init0();
#ifdef MA__LIBPTHREAD
			ma_check_lpt_sizes();
#endif
			call_init_function(&ma_init_info_main_thread_init);
#ifdef MA__LWPS
			call_init_function(&ma_init_info_topo_discover);
			call_init_function(&ma_init_info_marcel_topology_notifier_register);
			call_init_function(&ma_init_info_marcel_topology_call_UP_PREPARE);
#endif				// MA__LWPS
		} else if (section == MA_INIT_MAIN_LWP) {
			ma_allocator_init();
			ma_deviate_init();
			ma_signals_init();
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
