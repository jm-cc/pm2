
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

static common_attr_t default_static_attr;

#if defined(PM2) || defined(MAD2)
static unsigned pm2self, pm2_conf_size;
#endif


void common_attr_init(common_attr_t *attr)
{
#ifdef MAD2
  attr->madeleine = NULL;
  attr->rank = 0;
  attr->adapter_set = NULL;
#ifdef APPLICATION_SPAWN
  attr->url = NULL;
#endif
#endif
}

void common_pre_init(int *argc, char *argv[],
		     common_attr_t *attr)
{
  if(!attr) {
    common_attr_init(&default_static_attr);
    attr = &default_static_attr;
  }

#ifdef WIN_SYS
  marcel_for_win_init(argc, argv);
#endif /* WIN_SYS */

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
   * Provides:
   * - runtime activable logs
   *
   * Requires:
   * - ??? <to be completed>
   */
  pm2debug_init_ext(argc, argv, PM2DEBUG_DO_OPT); 
#endif /* PM2DEBUG */

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
  marcel_init_data(argc, argv);
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
  tbx_init(*argc, argv);
#endif /* TBX */

#ifdef MAD1
  mad_buffers_init();
#endif /* MAD1 */

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
  ntbx_init(*argc, argv);
#endif /* NTBX */

#ifdef MAD2
  /*
   * Mad2 memory manager 
   * -------------------
   *
   * Provides:
   * - memory management for MadII internal buffer structure
   *
   * Requires:
   * - TBX services
   */
  mad_memory_manager_init(*argc, argv);
#endif /* MAD2 */

#ifdef MAD2
  /*
   * Mad2 core initialization 
   * ------------------------
   *
   * Provides:
   * - Mad2 core objects initialization
   * - the `madeleine' object
   *
   * Requires:
   * - NTBX services
   * - Mad2 memory manager services
   *
   * Problemes:
   * - argument "configuration file"
   * - argument "adapter_set"
   */
  {
    void *configuration_file = NULL;
    void *adapter_set = (attr && attr->adapter_set) ? attr->adapter_set : NULL;

    attr->madeleine = mad_object_init(*argc, argv, configuration_file, adapter_set);
  }
#endif /* MAD2 */

#if defined(PM2) && defined(MAD2)
  /*
   * PM2 mad2/mad1 compatibility layer initialization
   * ------------------------------------------------
   *
   * Provides:
   * - Stores the value of the main "madeleine" object
   * - Initializes some internal mutexes
   *
   * Requires:
   * - Mad2 core initialization
   * - Marcel Data Initialization
   */
  pm2_mad_init(attr->madeleine);
#endif /* PM2 && MAD2 */

#if defined(MAD2) && defined(EXTERNAL_SPAWN)
  /*
   * Mad2 spawn driver initialization
   * --------------------------------
   *
   * Provides:
   * - Mad2 initialization from driver info
   *
   * Should provide:
   * - node rank
   * - pm2self
   * - pm2_conf_size
   *
   * Requires:
   * - the `madeleine' object
   */
  mad_spawn_driver_init(attr->madeleine, argc, argv);
#endif /* MAD2 && EXTERNAL_SPAWN */

#ifdef MAD2
  /*
   * Mad2 command line parsing
   * -------------------------
   *
   * Provides:
   * - Mad2 initialization from command line arguments
   * - node rank
   * - pm2self
   *
   * Requires:
   * - the `madeleine' object
   */
  mad_cmd_line_init(attr->madeleine, *argc, argv);

  pm2self = attr->madeleine->configuration->local_host_id;

  if(attr)
    attr->rank = attr->madeleine->configuration->local_host_id;

#endif /* MAD2 */ 

#ifdef MAD2
  /*
   * Mad2 output redirection
   * -------------------------
   *
   * Provides:
   * - Output redirection to log files 
   *
   * Requires:
   * - the `madeleine' object
   * - the node rank
   * - high priority
   */
  mad_output_redirection_init(attr->madeleine, *argc, argv);
#endif /* MAD2 */

#ifdef MAD2
  /*
   * Mad2 session configuration initialization
   * -----------------------------------------
   *
   * Provides:
   * - session configuration information
   * - pm2_conf_size
   *
   * Requires:
   * - the `madeleine' object
   */
  mad_configuration_init(attr->madeleine, *argc, argv);
  
  pm2_conf_size = attr->madeleine->configuration->size;

#endif /* MAD2 */

#ifdef MAD2
  /*
   * Mad2 network components initialization
   * --------------------------------------
   *
   * Provides:
   * - MadII network components ready to be connected
   * - connection data
   *
   * Requires:
   * - the `madeleine' object
   * - session configuration information
   */
  mad_network_components_init(attr->madeleine, *argc, argv);
#endif /* MAD2 */

#if defined(MAD2) && defined(APPLICATION_SPAWN)
  if(attr && attr->rank == 0) {
    // url: shall we store it or display it?
    if(attr->url) {
      strcpy(attr->url, mad_generate_url(attr->madeleine));
    } else {
      DISP("Run slave processes this way:");
      DISP("   pm2load %s --mad-slave --mad-url '%s' --mad-rank <r> <arg0>...",
	   getenv("PM2_PROG_NAME"), mad_generate_url(attr->madeleine));
    }
  }
#endif
}

void common_post_init(int *argc, char *argv[],
		      common_attr_t *attr)
{

  if(!attr)
    attr = &default_static_attr;

#if defined(MAD2) && defined(APPLICATION_SPAWN)
  if(attr->rank != 0)
    mad_parse_url(attr->madeleine);
#endif

#ifdef MAD2
  /*
   * Mad2 command line clean-up
   * --------------------------
   *
   * Provides:
   * - command line clean-up
   *
   * Requires:
   * - Mad2 initialization from command line arguments
   */
  mad_purge_command_line(attr->madeleine, argc, argv);
#endif /* MAD2 */

#ifdef MAD2

#if !defined(EXTERNAL_SPAWN) && !defined(LEONIE_SPAWN) && !defined(APPLICATION_SPAWN)
  /*
   * Mad2 slave nodes spawn
   * ----------------------
   *
   * Provides:
   * - slave nodes
   *
   * Requires:
   * - command line clean-up
   * - MadII network components ready to be connected
   * - connection data
   */
  mad_slave_spawn(attr->madeleine, *argc, argv);
#endif /* EXTERNAL_SPAWN && LEONIE_SPAWN && APPLICATION_SPAWN */

#endif /* MAD 2 */

#ifdef MAD2
  /*
   * Mad2 network connection
   * -----------------------
   *
   * Provides:
   * - network connection
   *
   * Requires:
   * - slave nodes
   * - MadII network components ready to be connected
   * - connection data
   */
  mad_connect(attr->madeleine, *argc, argv);
#endif /* MAD2 */

#ifdef MAD1
  mad_init(argc, argv, 0, NULL, &pm2_conf_size, &pm2self);
#endif /* MAD1 */

#ifdef PM2
  pm2_init_set_rank(argc, argv, pm2self, pm2_conf_size);
#endif /* PM2 */

#if defined(PROFILE) && (defined(PM2) || defined(MAD2))
  profile_set_tracefile("/tmp/prof_file_%d", pm2self);
#endif /* PROFILE */

#ifdef MARCEL
  marcel_start_sched(argc, argv);
#endif /* MARCEL */

#ifdef PM2
  pm2_init_thread_related(argc, argv);
#endif /* PM2 */

#ifdef DSM
  dsm_pm2_init(pm2self, pm2_conf_size);
#endif /* DSM */

#ifdef PM2
  pm2_init_exec_startup_funcs(argc, argv);
#endif

#ifdef PM2
  pm2_init_open_channels(argc, argv);
#endif /* PM2 */

#ifdef PM2
  pm2_init_listen_network(argc, argv);
#endif /* PM2 */

#ifdef PM2
  pm2_init_purge_cmdline(argc, argv);
#endif /* PM2 */

#ifdef MARCEL
  marcel_purge_cmdline(argc, argv);
#endif /* PM2 */

#ifdef NTBX
  ntbx_purge_cmd_line(argc, argv);
#endif /* NTBX */

#ifdef TBX
  tbx_purge_cmd_line(argc, argv);
#endif /* TBX */

#ifdef PM2DEBUG  
/*
   * Logging services - args clean-up
   * --------------------------------
   *
   * Provides:
   * - pm2debug command line arguments clean-up
   *
   * Requires:
   * - ??? <to be completed>
   */
  pm2debug_init_ext(argc, argv, PM2DEBUG_CLEAROPT); 
#endif /* PM2DEBUG */
}
