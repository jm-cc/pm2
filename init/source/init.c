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

#include "pm2_common.h"
#include "tbx_program_argument.h"
#include "tbx_debug.h"
#include <alloca.h>

#ifdef NMAD
#  include "nm_launcher.h"
#endif

static common_attr_t default_static_attr;

#if defined(PM2)
static unsigned int pm2self       = 0;
static unsigned int pm2_conf_size = 0;
#endif

void common_attr_init(common_attr_t *attr TBX_UNUSED)
{
}


void common_pre_init(int *argc, char *argv[], common_attr_t *attr)
{
	if (!attr) {
		common_attr_init(&default_static_attr);
		attr = &default_static_attr;
	}

#ifdef TBX
	tbx_pa_parse(*argc, argv, PM2_ENVVARNAME);
#endif

#ifdef PROFILE
	/*
	 * Profiling services
	 * ------------------
	 *
	 * Provides:
	 * - Fast User Traces internal initialization
	 *
	 * Requires:
	 * - nothing
	 */
	profile_init();
#endif /* PROFILE */

#ifdef PM2DEBUG
	/*
	 * Logging services
	 * ------------------
	 *
	 */
	PM2_LOG_ADD() ;
#endif

#ifdef PM2
	/*
	 * PM2 Data Initialization
	 * -----------------------
	 *
	 * Provides:
	 * - Initialization of internal static data structures
	 * - Calls pm2_rpc_init if not already called
	 *
	 * Requires:
	 * - Nothing
	 *
	 * Enforces:
	 * - Further calls to pm2_register are not allowed
	 */
	pm2_init_data(argc, argv);
#endif /* PM2 */

#if defined(PM2_TOPOLOGY)
	tbx_topology_init(*argc, argv) ;
#endif /* PM2_TOPOLOGY */

#ifdef MARCEL
	/*
	 * Marcel Data Initialization
	 * --------------------------
	 *
	 * Provides:
	 * - Initialization of internal static data structures
	 * - Initialization of internal mutexes and locks
	 * - Initialization of Scheduler (but no activity is started)
	 *
	 * Requires:
	 * - Nothing
	 */
	marcel_init_data(*argc, argv);
#endif /* MARCEL */

#ifdef TBX
	/*
	 * Toolbox services
	 * ----------------
	 *
	 * Provides:
	 * - fast malloc
	 * - safe malloc
	 * - timing
	 * - list objects
	 * - slist objects
	 * - htable objects
	 *
	 * Requires:
	 * - Marcel Data Initialization
	 */
	tbx_init(argc, &argv) ;
#endif /* TBX */

#ifdef NTBX
	/*
	 * Net-Toolbox services
	 * --------------------
	 *
	 * Provides:
	 * - pack/unpack routines for hetereogeneous architecture communication
	 * - safe non-blocking client-server framework over TCP
	 *
	 * Requires:
	 * - TBX services
	 */
	ntbx_init(argc, argv);
#endif /* NTBX */

#ifdef PIOMAN
	/*
	 * PIOMan IO initialization
	 * -----------------------------------------
	 *
	 * Provides:
	 * - TCP piom_server creation and initialization
	 *
	 * Requires:
	 * - nothing ? (to be tested)
	 */
#ifndef PIOM_DISABLE_LTASKS
	piom_init_ltasks();
#endif
	piom_io_init();
#endif /* PIOMAN */
}

void common_post_init(int *argc, char *argv[],
		      common_attr_t *attr)
{
#ifdef TBX
	tbx_pa_get_args(argc, &argv);
#endif
  
	if (!attr)
		attr = &default_static_attr;

#ifdef PM2
	pm2_init_set_rank(argc, argv, pm2self, pm2_conf_size);
#endif /* PM2 */

#if defined(PROFILE) && defined(PM2)
	profile_set_tracefile("/tmp/prof_file_%d", pm2self);
#endif /* PROFILE */

#if defined(PROFILE) && defined(NMAD)
#  if defined(CONFIG_PROTO_MAD3)
	profile_set_tracefile("/tmp/prof_file_%d",
			      mad_get_madeleine()->session->process_rank);
#  else
	{
#    define PM2_INIT_MAX_HOSTNAME_LEN 17
		char         hn[PM2_INIT_MAX_HOSTNAME_LEN];
		unsigned int pid;

		gethostname(hn, PM2_INIT_MAX_HOSTNAME_LEN);
		hn[PM2_INIT_MAX_HOSTNAME_LEN-1] = '\0';

		pid = (unsigned int) getpid();
		profile_set_tracefile("/tmp/prof_file_%s_%u",
				      hn, pid);
	}
#    undef PM2_INIT_MAX_HOSTNAME_LEN
#  endif
#endif /* PROFILE */

#ifdef MARCEL
	marcel_start_sched();
#endif /* MARCEL */

#ifdef PM2
	pm2_init_thread_related(argc, argv);
#endif /* PM2 */

#ifdef PM2
	pm2_init_exec_startup_funcs(argc, argv);
#endif

#ifdef PM2
	pm2_net_init_channels(argc, argv);
#endif /* PM2 */

#ifdef PM2
	pm2_net_servers_start(argc, argv);
#endif /* PM2 */

#ifdef PM2
	//pm2_init_purge_cmdline(argc, argv);
#endif /* PM2 */

#ifdef MARCEL
	marcel_purge_cmdline();
#endif /* PM2 */

#ifdef NTBX
	//ntbx_purge_cmd_line(argc, argv);
#endif /* NTBX */

#ifdef TBX
	tbx_purge_cmd_line();
#endif /* TBX */
}

void
common_exit(common_attr_t *attr)
{
	PM2_LOG_IN();

/* If several modules are activated (such as NMad + Marcel),
 * common_exit may be called several times. To avoid double-free problems
 * let's return if common_exit as already been called.
 */
	static tbx_bool_t exiting = tbx_false;
	if(exiting)
		return;
	exiting=tbx_true;

	if (!attr)
		attr = &default_static_attr;

#ifdef PM2
	pm2_net_request_end();
	pm2_net_wait_end();
#endif // PM2

#ifdef NMAD
	nm_launcher_exit();
#endif

#ifdef PM2
	pm2_thread_exit();
	block_exit();
#endif // PM2

#ifdef NTBX
	//
	// NTBX clean-up
	// -------------
	//
	// Provides:
	// - ntbx structures clean-up
	//
	// Requires:
	// - Marcel synchronization services
	// - TBX full activity
	//

	ntbx_exit();
#endif // NTBX

#ifdef TBX
	//
	// TBX clean-up
	// -------------
	//
	// Provides:
	// - tbx structures clean-up
	//
	// Requires:
	// - Marcel synchronization services
	//

	tbx_exit();
#endif // TBX

#ifdef MARCEL
	// Marcel shutdown
	// --------------------------------
	marcel_end();
#endif // MARCEL

#ifdef PROFILE
	/*
	 * Profiling services
	 * ------------------
	 *
	 * Provides:
	 * - Fast User Traces internal initialization
	 *
	 * Requires:
	 * - nothing
	 */
	profile_exit();
#endif /* PROFILE */

#ifdef PM2_TOPOLOGY
	tbx_topology_destroy() ;
#endif /* PM2_TOPOLOGY */

	PM2_LOG_OUT();

#ifdef PM2DEBUG
	PM2_LOG_DEL() ;
#endif
}
