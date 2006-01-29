
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

/*
 * Begin: added by O.A.
 * --------------------
 */
static tbx_flag_t marcel_activity = tbx_flag_clear;

int
marcel_test_activity(void)
{
  tbx_bool_t result = tbx_false;

  LOG_IN();
  result = tbx_flag_test(&marcel_activity);
  LOG_OUT();

  return result;
}

/*
 * End: added by O.A.
 * ------------------
 */

static void marcel_parse_cmdline_early(int *argc, char **argv, boolean do_not_strip)
{
  int i, j;
#ifdef MA__LWPS
  unsigned __nb_lwp = 0;
#endif

  if (!argc)
    return;

  i = j = 1;

  while(i < *argc) {
#ifdef MA__LWPS
    if(!strcmp(argv[i], "--marcel-nvp")) {
      if(i == *argc-1) {
	fprintf(stderr,
		"Fatal error: --marcel-nvp option must be followed "
		"by <nb_of_virtual_processors>.\n");
	exit(1);
      }
      if(do_not_strip) {
	__nb_lwp = atoi(argv[i+1]);
	if(__nb_lwp < 1 || __nb_lwp > MA_NR_LWPS) {
	  fprintf(stderr,
		  "Error: nb of VP should be between 1 and %d\n",
		  MA_NR_LWPS);
	  exit(1);
	}
	argv[j++] = argv[i++];
	argv[j++] = argv[i++];
      } else
	i += 2;
      continue;
    } else
#endif
      argv[j++] = argv[i++];
  }
  *argc = j;
  argv[j] = NULL;

  if(do_not_strip) {
#ifdef MA__LWPS
    marcel_lwp_fix_nb_vps(__nb_lwp);
    mdebug("\t\t\t<Suggested nb of Virtual Processors : %d>\n", __nb_lwp);
#endif
  }
}

static void marcel_parse_cmdline_lastly(int *argc, char **argv, boolean do_not_strip)
{
  int i, j;

  if (!argc)
    return;

  i = j = 1;

  while(i < *argc) {
    if(!strcmp(argv[i], "--marcel-top")) {
      if (i == *argc-1) {
	fprintf(stderr,
		"Fatal error: --marcel-top option must be followed "
		"by <out_file>.\n");
	exit(1);
      }
      if(do_not_strip) {
	if (marcel_init_top(argv[i+1])) {
	  fprintf(stderr,
		  "Error: invalid top out_file %s\n",argv[i+1]);
	  exit(1);
	}
	argv[j++] = argv[i++];
	argv[j++] = argv[i++];
      } else
	i += 2;
      continue;
    } else
    if(!strcmp(argv[i], "--marcel-xtop")) {
      if (do_not_strip) {
        if (marcel_init_top("|xterm -S//0 -geometry 120x26")) {
	  fprintf(stderr, "Error: can't launch xterm\n");
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
  marcel_parse_cmdline_early(argc, argv, FALSE);

  marcel_debug_init(argc, argv, PM2DEBUG_CLEAROPT);

  marcel_parse_cmdline_lastly(argc, argv, FALSE);
}

// Cannot start some internal threads or activations.
// When completed, fork calls are still allowed.
void marcel_init_data(int *argc, char *argv[])
{
  static volatile boolean already_called = FALSE;

  // Only execute this function once
  if(already_called)
    return;
  already_called = TRUE;

  // Parse command line
  marcel_parse_cmdline_early(argc, argv, TRUE);

  // Windows/Cygwin specific stuff
  marcel_win_sys_init(argc, argv);

  marcel_init_section(MA_INIT_SCHEDULER);

  // Initialize debug facilities
  marcel_debug_init(argc, argv, PM2DEBUG_DO_OPT);

  marcel_parse_cmdline_lastly(argc, argv, TRUE);
}

// When completed, some threads/activations may be started
// Fork calls are now prohibited in non libpthread versions
void marcel_start_sched(int *argc, char *argv[])
{
  // Start scheduler (i.e. run LWP/activations, start timer)
  marcel_init_section(MA_INIT_START_LWPS);
  marcel_activity = tbx_flag_set;
}

void marcel_purge_cmdline(int *argc, char *argv[])
{
  static volatile boolean already_called = FALSE;

  marcel_init_section(MA_INIT_MAX_PARTS);

  // Only execute this function once
  if(already_called)
    return;
  already_called = TRUE;

  // Remove marcel-specific arguments from command line
  marcel_strip_cmdline(argc, argv);
}

void marcel_finish(void)
{
  marcel_gensched_shutdown();
  marcel_slot_exit();
  ma_topo_exit();
  marcel_exit_top();
}

#ifndef STANDARD_MAIN

extern int marcel_main(int argc, char *argv[]);

#ifdef WIN_SYS
void win_stack_allocate(unsigned n)
{
  int tab[n];

  tab[0] = 0;
}
#endif // WIN_SYS

static volatile int __main_ret;
static marcel_ctx_t __initial_main_ctx;

#ifdef MARCEL_MAIN_AS_FUNC
int go_marcel_main(int argc, char *argv[])
#ifdef PM2_DEV
#warning go_marcel_main defined
#endif
#else
int main(int argc, char *argv[])
#endif // MARCEL_MAIN_AS_FUNC
{
	static int __argc;
	static char **__argv;
	unsigned long new_sp;

#ifdef MAD2
	marcel_debug_init(&argc, argv, PM2DEBUG_DO_OPT);
#else
	marcel_debug_init(&argc, argv, PM2DEBUG_DO_OPT|PM2DEBUG_CLEAROPT);
#endif
	if(!marcel_ctx_setjmp(__initial_main_ctx)) {

		__main_thread = (marcel_t)((((unsigned long)get_sp() - 128) &
					    ~(THREAD_SLOT_SIZE-1)) -
					   MAL(sizeof(marcel_task_t)));

		mdebug("\t\t\t<main_thread is %p>\n", __main_thread);

                __argc = argc; __argv = argv;

		new_sp = (unsigned long)__main_thread - TOP_STACK_FREE_AREA;

		/* On se contente de descendre la pile. Tout va bien, même sur Itanium */
#ifdef WIN_SYS
		win_stack_allocate(get_sp() - new_sp);
#endif
		set_sp(new_sp);

#ifdef ENABLE_STACK_JUMPING
		*((marcel_t *)((char *)__main_thread + MAL(sizeof(marcel_task_t)) - 
			       sizeof(void *))) =  __main_thread;
#endif

                __main_ret = marcel_main(__argc, __argv);

#ifdef MA__ACTIVATION
		marcel_upcalls_disallow();
#endif
		marcel_ctx_longjmp(__initial_main_ctx, 1);
	}

	return __main_ret;
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

static int init_done[MA_INIT_MAX_PARTS+1]={0,};

// Section MA_INIT_SELF
#ifdef MA__LWPS
extern const __ma_init_info_t ma_init_info_topo_discover;
#ifdef MA__NUMA
extern const __ma_init_info_t ma_init_info_marcel_topology_notifier_register;
extern const __ma_init_info_t ma_init_info_marcel_topology_call_UP_PREPARE;
#endif // MA__NUMA
#endif // MA__LWPS
extern const __ma_init_info_t ma_init_info_main_thread_init;

// Section MA_INIT_MAIN_LWP
extern const __ma_init_info_t ma_init_info_marcel_debug_init_auto;
extern const __ma_init_info_t ma_init_info_marcel_slot_init;
extern const __ma_init_info_t ma_init_info_sched_init;
#ifdef MA__BUBBLES
extern const __ma_init_info_t ma_init_info_bubble_sched_init;
#endif // MA__BUBBLES
extern const __ma_init_info_t ma_init_info_softirq_init;
extern const __ma_init_info_t ma_init_info_marcel_io_init;
#ifdef MA__TIMER
extern const __ma_init_info_t ma_init_info_sig_init;
#endif // MA__TIMER
extern const __ma_init_info_t ma_init_info_timer_start;
extern const __ma_init_info_t ma_init_info_marcel_lwp_finished;
extern const __ma_init_info_t ma_init_info_marcel_lwp_notifier_register;
#if defined(LINUX_SYS) || defined(GNU_SYS)
extern const __ma_init_info_t ma_init_info_marcel_random_lwp_notifier_register;
#endif // LINUX_SYS || GNU_SYS
extern const __ma_init_info_t ma_init_info_marcel_generic_sched_notifier_register;
extern const __ma_init_info_t ma_init_info_marcel_postexit_notifier_register;
extern const __ma_init_info_t ma_init_info_marcel_fault_catcher_notifier_register;
extern const __ma_init_info_t ma_init_info_marcel_linux_sched_notifier_register;
extern const __ma_init_info_t ma_init_info_marcel_ksoftirqd_notifier_register;
extern const __ma_init_info_t ma_init_info_marcel_timers_notifier_register;
extern const __ma_init_info_t ma_init_info_marcel_sig_timer_notifier_register;
extern const __ma_init_info_t ma_init_info_marcel_lwp_call_ONLINE;
extern const __ma_init_info_t ma_init_info_marcel_lwp_call_UP_PREPARE;
#if defined(LINUX_SYS) || defined(GNU_SYS)
extern const __ma_init_info_t ma_init_info_marcel_random_lwp_call_UP_PREPARE;
#endif // LINUX_SYS || GNU_SYS
extern const __ma_init_info_t ma_init_info_marcel_generic_sched_call_UP_PREPARE;
extern const __ma_init_info_t ma_init_info_marcel_postexit_call_UP_PREPARE;
extern const __ma_init_info_t ma_init_info_marcel_linux_sched_call_UP_PREPARE;
extern const __ma_init_info_t ma_init_info_marcel_timers_call_UP_PREPARE;
#ifdef PROFILE
extern const __ma_init_info_t ma_init_info_marcel_int_catcher_call_ONLINE;
#endif // PROFILE

// Section MA_INIT_SCHEDULER
#ifdef MA__ACTIVATION
extern const __ma_init_info_t ma_init_info_marcel_upcall_call_UP_PREPARE;
#endif // MA__ACTIVATION
extern const __ma_init_info_t ma_init_info_marcel_generic_sched_call_ONLINE;
extern const __ma_init_info_t ma_init_info_marcel_postexit_call_ONLINE;
extern const __ma_init_info_t ma_init_info_marcel_sig_timer_call_ONLINE;
extern const __ma_init_info_t ma_init_info_marcel_ksoftirqd_call_UP_PREPARE;
extern const __ma_init_info_t ma_init_info_marcel_ksoftirqd_call_ONLINE;

// Section MA_INIT_START_LWPS
#ifdef MA__LWPS
extern const __ma_init_info_t ma_init_info_marcel_gensched_start_lwps;
#endif
#ifdef MA__ACTIVATION
extern const __ma_init_info_t ma_init_info_init_upcalls;
#endif // MA__ACTIVATION

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

void marcel_init_section(int sec) {
	int section;

	/* Quick registration */
	pm2debug_register(&MA_DEBUG_VAR_NAME(MA_FILE_DEBUG));
	//pm2debug_setup(&marcel_mdebug,PM2DEBUG_SHOW,PM2DEBUG_ON);

	mdebug("Asked for level %d (%s)\n",
	       ma_init_start[sec].prio,
	       ma_init_start[sec].debug);
	for (section=0; section<=sec; section++) {
		if (init_done[section])
			continue;
		mdebug("Init running level %d (%s)\n",
		       ma_init_start[section].prio,
		       ma_init_start[section].debug);

                if (section == MA_INIT_SELF) {
#ifdef MA__LWPS
                  call_init_function(&ma_init_info_topo_discover);
#ifdef MA__NUMA
		  call_init_function(&ma_init_info_marcel_topology_notifier_register);
		  call_init_function(&ma_init_info_marcel_topology_call_UP_PREPARE);
#endif // MA__NUMA
#endif // MA__LWPS
                  call_init_function(&ma_init_info_main_thread_init);
                }
                else if (section == MA_INIT_MAIN_LWP) {
#ifdef PROFILE
                  call_init_function(&ma_init_info_marcel_int_catcher_call_ONLINE);
#endif // PROFILE
                  call_init_function(&ma_init_info_marcel_debug_init_auto);
                  call_init_function(&ma_init_info_marcel_slot_init);
                  call_init_function(&ma_init_info_sched_init);
                  call_init_function(&ma_init_info_marcel_lwp_call_UP_PREPARE);
#if defined(LINUX_SYS) || defined(GNU_SYS)
                  call_init_function(&ma_init_info_marcel_random_lwp_call_UP_PREPARE);
#endif // LINUX_SYS || GNU_SYS
                  call_init_function(&ma_init_info_marcel_lwp_call_ONLINE);
                  call_init_function(&ma_init_info_marcel_generic_sched_call_UP_PREPARE);
                  call_init_function(&ma_init_info_marcel_postexit_call_UP_PREPARE);
                  call_init_function(&ma_init_info_timer_start);
#ifdef MA__TIMER
                  call_init_function(&ma_init_info_sig_init);
#endif // MA__TIMER
                  call_init_function(&ma_init_info_marcel_linux_sched_call_UP_PREPARE);
#ifdef MA__BUBBLES
                  call_init_function(&ma_init_info_bubble_sched_init);
#endif // MA__BUBBLES
                  call_init_function(&ma_init_info_softirq_init);
                  call_init_function(&ma_init_info_marcel_timers_call_UP_PREPARE);
                  call_init_function(&ma_init_info_marcel_io_init);
#if defined(LINUX_SYS) || defined(GNU_SYS)
                  call_init_function(&ma_init_info_marcel_random_lwp_notifier_register);
#endif // LINUX_SYS || GNU_SYS
                  call_init_function(&ma_init_info_marcel_lwp_notifier_register);
                  call_init_function(&ma_init_info_marcel_generic_sched_notifier_register);
                  call_init_function(&ma_init_info_marcel_postexit_notifier_register);
                  call_init_function(&ma_init_info_marcel_fault_catcher_notifier_register);
                  call_init_function(&ma_init_info_marcel_sig_timer_notifier_register);
                  call_init_function(&ma_init_info_marcel_linux_sched_notifier_register);
                  call_init_function(&ma_init_info_marcel_ksoftirqd_notifier_register);
                  call_init_function(&ma_init_info_marcel_timers_notifier_register);
                  call_init_function(&ma_init_info_marcel_lwp_finished);
                }
                else if (section == MA_INIT_SCHEDULER) {
#ifdef MA__ACTIVATION
                  call_init_function(&ma_init_info_marcel_upcall_call_UP_PREPARE)
#endif // MA__ACTIVATION
                    call_init_function(&ma_init_info_marcel_generic_sched_call_ONLINE);
                    call_init_function(&ma_init_info_marcel_postexit_call_ONLINE);
                    call_init_function(&ma_init_info_marcel_sig_timer_call_ONLINE);
                    call_init_function(&ma_init_info_marcel_ksoftirqd_call_UP_PREPARE);
                    call_init_function(&ma_init_info_marcel_ksoftirqd_call_ONLINE);
                }
                else if (section == MA_INIT_START_LWPS) {
#ifdef MA__LWPS
                  call_init_function(&ma_init_info_marcel_gensched_start_lwps);
#endif // MA__LWPS
#ifdef MA__ACTIVATION
                  call_init_function(&ma_init_info_init_upcalls);
#endif //MA__ACTIVATION
                }

		init_done[section]=1;
	}
	mdebug("Init running level %d (%s) done\n",
	       ma_init_start[sec].prio,
	       ma_init_start[sec].debug);
}
