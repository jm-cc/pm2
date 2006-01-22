
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
	__ma_init_info_t *infos;
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
	

#define _ADD_INIT_SECTION(number, text) \
  TBX_INTERNAL const int __ma_init_info_##number \
    TBX_ALIGNED TBX_SECTION(__MA_INIT_SECTION "inf." #number) = number; \
  TBX_INTERNAL const __ma_init_index_t __ma_init_index_##number \
    TBX_SECTION(__MA_INIT_SECTION "ind." #number) \
    = { .infos=(tbx_container_of(&__ma_init_info_##number, \
			     __ma_init_section_index_t, a.section_number) \
    		->b.infos), \
        .prio=number, .debug=text }

#define ADD_INIT_SECTION(number, text) _ADD_INIT_SECTION(number, text)

#if MA_INIT_SELF != 0
#error Adapte init.h and init.c to be in sync
#endif
ADD_INIT_SECTION(MA_INIT_SELF, "Init self");

#if MA_INIT_TLS != 1
#error Adapte init.h and init.c to be in sync
#endif
ADD_INIT_SECTION(MA_INIT_TLS, "Init TLS");

#if MA_INIT_MAIN_LWP != 2
#error Adapte init.h and init.c to be in sync
#endif
ADD_INIT_SECTION(MA_INIT_MAIN_LWP, "Init main lwp");

#if MA_INIT_SCHEDULER != 3
#error Adapte init.h and init.c to be in sync
#endif
ADD_INIT_SECTION(MA_INIT_SCHEDULER, "Init scheduler");

#if MA_INIT_START_LWPS != 4
#error Adapte init.h and init.c to be in sync
#endif
ADD_INIT_SECTION(MA_INIT_START_LWPS, "Init LWPs");

#if MA_INIT_MAX_PARTS != 4
#error Adapte init.h and init.c to be in sync
#endif
ADD_INIT_SECTION(5, "No more init part");

extern const __ma_init_index_t __ma_init_start[];

static int init_done[MA_INIT_MAX_PARTS+1]={0,};

void marcel_init_section(int sec)
{
	const __ma_init_info_t *infos, *last;
	int section;

	/* Quick registration */
	pm2debug_register(&MA_DEBUG_VAR_NAME(MA_FILE_DEBUG));
	//pm2debug_setup(&marcel_mdebug,PM2DEBUG_SHOW,PM2DEBUG_ON);

	mdebug("Init running level %d (%s) start\n",
	       __ma_init_start[sec].prio,
	       __ma_init_start[sec].debug);
	for (section=0; section<=sec; section++) {
		if (init_done[section])
			continue;
		mdebug("Init running level %d (%s)\n",
		       __ma_init_start[section].prio,
		       __ma_init_start[section].debug);
		
		infos=__ma_init_start[section].infos;
		last= (__ma_init_start[section+1].infos)-1;
		
		while (infos < last) {
			mdebug("Init launching for prio %i: %s (%s)\n",
			       (int)(MA_INIT_PRIO_BASE-infos->prio), 
			      *infos->debug, infos->file);
			(*infos->func)();
			infos++;
		}
		init_done[section]=1;
	}
	mdebug("Init running level %d (%s) done\n",
	       __ma_init_start[sec].prio,
	       __ma_init_start[sec].debug);
}

