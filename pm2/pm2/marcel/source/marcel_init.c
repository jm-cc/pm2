
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

void
marcel_set_activity(void)
{
  LOG_IN();
  if (tbx_flag_test(&marcel_activity))
    FAILURE("marcel_activity flag already set");
  
  tbx_flag_set(&marcel_activity);
  LOG_OUT();
}

void
marcel_clear_activity(void)
{
  LOG_IN();
  if (!tbx_flag_test(&marcel_activity))
    FAILURE("marcel_activity flag already cleared");

  tbx_flag_clear(&marcel_activity);
  LOG_OUT();
}

/*
 * End: added by O.A.
 * ------------------
 */

static unsigned __nb_lwp = 0;

static void marcel_parse_cmdline(int *argc, char **argv, boolean do_not_strip)
{
  int i, j;

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

  if(do_not_strip)
    mdebug("\t\t\t<Suggested nb of Virtual Processors : %d>\n", __nb_lwp);
}

void marcel_strip_cmdline(int *argc, char *argv[])
{
  marcel_parse_cmdline(argc, argv, FALSE);

  marcel_debug_init(argc, argv, PM2DEBUG_CLEAROPT);
}

// Cannot start some internal threads or activations.
// When completed, fork calls are still allowed.
void marcel_init_data(int *argc, char *argv[])
{
  static volatile boolean already_called = FALSE;

  marcel_init_section(MA_INIT_SCHEDULER);
  // Only execute this function once
  if(already_called)
    return;
  already_called = TRUE;

  // Windows/Cygwin specific stuff
  marcel_win_sys_init(argc, argv);

  // Initialize debug facilities
  marcel_debug_init(argc, argv, PM2DEBUG_DO_OPT);

  // Parse command line
  marcel_parse_cmdline(argc, argv, TRUE);

  // Initialize the stack memory allocator
  // This is unuseful if PM2 is used, but it is not harmful anyway ;-)
  //marcel_slot_init();

  // Initialize scheduler
  //marcel_sched_init();

  // Initialize mechanism for handling Unix I/O through the scheduler
  //marcel_io_init();
}

// When completed, some threads/activations may be started
// Fork calls are now prohibited in non libpthread versions
void marcel_start_sched(int *argc, char *argv[])
{
  static volatile boolean already_called = FALSE;

  // Only execute this function once
  if(already_called)
    return;
  already_called = TRUE;

  // Start scheduler (i.e. run LWP/activations, start timer)
  marcel_sched_start(__nb_lwp);
  marcel_set_activity();
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

void marcel_end(void)
{
  marcel_sched_shutdown();
  marcel_slot_exit();
  mdebug("threads created in cache : %ld\n", marcel_cachedthreads());
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

marcel_t __main_thread;
static volatile int __main_ret;
static marcel_ctx_t __initial_main_ctx;

#ifdef MARCEL_MAIN_AS_FUNC
int go_marcel_main(int argc, char *argv[])
#warning go_marcel_main defined
#else
int main(int argc, char *argv[])
#endif // MARCEL_MAIN_AS_FUNC
{
	static int __argc;
	static char **__argv;

	/* Quick registration */
	pm2debug_register(&ma_debug_init);

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

#ifdef WIN_SYS
		win_stack_allocate(THREAD_SLOT_SIZE);
#endif

		/* On se contente de descendre la pile. Tout va bien, m�me sur Itanium */
		set_sp((unsigned long)__main_thread - TOP_STACK_FREE_AREA);
		
#ifdef ENABLE_STACK_JUMPING
		*((marcel_t *)((char *)__main_thread + MAL(sizeof(marcel_task_t)) - 
			       sizeof(void *))) =  __main_thread;
#endif
		
		__main_ret = marcel_main(__argc, __argv);
		
		marcel_ctx_longjmp(__initial_main_ctx, 1);
	}

	return __main_ret;
}


#endif // STANDARD_MAIN

typedef struct {
	__ma_init_info_t *infos;
	int prio;
	char* debug;
} __ma_init_index_t;

#define _ADD_INIT_SECTION(number, text) \
  int __ma_init_info_##number \
    __attribute__((section(__MA_INIT_SECTION "info." #number)))=number; \
  const __ma_init_index_t __ma_init_index_##number \
    __attribute__((section(__MA_INIT_SECTION "index." #number))) \
    = { .infos=(__ma_init_info_t *)((&__ma_init_info_##number)+1), \
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

extern __ma_init_index_t __ma_init_start[];

static int init_done[MA_INIT_MAX_PARTS+1]={0,};

void marcel_init_section(int sec)
{
	__ma_init_info_t *infos, *last;
	int section;
	debug("Init running level %d (%s) start\n",
	      __ma_init_start[sec].prio,
	      __ma_init_start[sec].debug);
	for (section=0; section<=sec; section++) {
		if (init_done[section])
			continue;
		debug("Init running level %d (%s)\n",
		      __ma_init_start[section].prio,
		      __ma_init_start[section].debug);
		
		infos=__ma_init_start[section].infos;
		last= (__ma_init_start[section+1].infos)-1;
		
		while (infos < last) {
			debug("Init launching for prio %i: %s (%s)\n",
			      (int)(MA_INIT_PRIO_BASE-infos->prio), 
			      infos->debug, infos->file);
			(*infos->func)();
			infos++;
		}
		init_done[section]=1;
	}
	debug("Init running level %d (%s) done\n",
	      __ma_init_start[sec].prio,
	      __ma_init_start[sec].debug);
}

extern int marcel_lwp_force_link;
void marcel_force_link(void)
{
	/* Ne sert qu'� forcer l'inclusion de fichiers lors de la
	 * cr�ation du binaire (�dition statique)
	 */
	marcel_lwp_force_link++;
}
