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

#if defined(MAD2) || defined(PM2)
static unsigned int pm2self       = 0;
static unsigned int pm2_conf_size = 0;
#endif

void common_attr_init(common_attr_t *attr)
{
#if defined (MAD3) || defined (MAD4)
  attr->madeleine = NULL;
#endif // MAD 3 || MAD4

#ifdef MAD2
  attr->madeleine = NULL;
  attr->rank = 0;
  attr->adapter_set = NULL;
#ifdef APPLICATION_SPAWN
  attr->url = NULL;
#endif // APPLICATION_SPAWN
#endif // MAD2
}

void common_pre_init(int *argc, char *argv[],
		     common_attr_t *attr)
{
  if (!attr) {
    common_attr_init(&default_static_attr);
    attr = &default_static_attr;
  }

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

#if defined (MAD3) || defined (MAD4)
  /*
   * Mad3 memory managers
   * --------------------
   *
   * Provides:
   * - memory management for MadIII internal data structures
   *
   * Requires:
   * - TBX services
   */
  mad_memory_manager_init(*argc, argv);
#ifdef MARCEL
  mad_forward_memory_manager_init(*argc, argv);
  mad_mux_memory_manager_init(*argc, argv);
#endif // MARCEL
#endif /* MAD3 || MAD4 */

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
    void *adapter_set        =
      (attr && attr->adapter_set) ? attr->adapter_set : NULL;

    attr->madeleine          =
      mad_object_init(*argc, argv, configuration_file, adapter_set);
  }
#endif /* MAD2 */

#if defined (MAD3) || defined (MAD4)
  /*
   * Mad3 core initialization
   * ------------------------
   *
   * Provides:
   * - Mad3 core objects initialization
   * - the `madeleine' object
   *
   * Requires:
   * - NTBX services
   * - TBX services
   * - Mad3 memory manager services
   */

    attr->madeleine = mad_object_init(*argc, argv);
#endif /* MAD3 */

#if defined(PM2)
  /*
   * PM2 mad2/3 interface layer initialization
   * ------------------------------------------------
   *
   * Provides:
   * - Stores the value of the main "madeleine" object
   * - Initializes some internal mutexes
   *
   * Requires:
   * - Mad2/3 core initialization
   * - Marcel Data Initialization
   */
  pm2_mad_init(attr->madeleine);
#endif /* PM2 */

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

#if defined (MAD3) || defined (MAD4)
  /*
   * Mad3 structure initializations
   * ------------------------------
   *
   * Provides:
   * - Mad2 initialization from command line arguments
   * - directory
   * - drivers initialization
   *
   * Requires:
   * - the `madeleine' object
   */
  mad_cmd_line_init(attr->madeleine, *argc, argv);
  mad_leonie_link_init(attr->madeleine, *argc, argv);
  mad_leonie_command_init(attr->madeleine, *argc, argv);
  mad_directory_init(attr->madeleine, *argc, argv);
#ifdef MAD3
  mad_drivers_init(attr->madeleine, argc, &argv);
#else /* MAD4 */
  mad_dir_driver_init(attr->madeleine, argc, &argv);
#endif /* MAD4 */

#ifdef PM2
  pm2self       = attr->madeleine->session->process_rank;
  pm2_conf_size = tbx_slist_get_length(attr->madeleine->dir->process_slist);
// Warning:
// The number of processes and the global rank maximum may be different !!!
// number of processes = tbx_slist_get_length(madeleine->dir->process_slist));
// global rank max     = tbx_darray_length(madeleine->dir->process_darray));
#endif /* PM2 */
#endif /* MAD3 || MAD4 */

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

#ifdef PM2
  pm2self = attr->madeleine->configuration->local_host_id;
#endif

  if (attr)
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

#ifdef PM2
  pm2_conf_size = attr->madeleine->configuration->size;
#endif

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
  if (attr && !attr->rank)
    {
      // url: shall we store it or display it?
      if (attr->url)
	{
	  strcpy(attr->url, mad_generate_url(attr->madeleine));
	}
      else
	{
	  DISP("Run slave processes this way:");
	  DISP("   pm2load %s --mad-slave --mad-url '%s' "
	       "--mad-rank <r> <arg0>...", getenv("PM2_PROG_NAME"),
	       mad_generate_url(attr->madeleine));
	}
    }
#endif /* MAD2 && APPLICATION_SPAWN */
}

void common_post_init(int *argc, char *argv[],
		      common_attr_t *attr)
{

  if (!attr)
    attr = &default_static_attr;

#if defined(MAD2) && defined(APPLICATION_SPAWN)
  if (attr->rank)
    mad_parse_url(attr->madeleine);
#endif /* MAD2 && APPLICATION_SPAWN */

#if defined(MAD2) || defined(MAD3) || defined(MAD4)
  /*
   * Mad2/3 command line clean-up
   * --------------------------
   *
   * Provides:
   * - command line clean-up
   *
   * Requires:
   * - Mad2 initialization from command line arguments
   */
  mad_purge_command_line(attr->madeleine, argc, argv);
#endif /* MAD2/3 */

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

#ifdef PM2
  pm2_init_set_rank(argc, argv, pm2self, pm2_conf_size);
#endif /* PM2 */

#if defined(PROFILE) && (defined(PM2) || defined(MAD2))
  profile_set_tracefile("/tmp/prof_file_%d", pm2self);
#endif /* PROFILE */
#if defined(PROFILE) && (defined(MAD3))
  profile_set_tracefile("/tmp/prof_file_%d",
		  mad_get_madeleine()->session->process_rank);
#endif /* PROFILE */

#ifdef MARCEL
  marcel_start_sched(argc, argv);
#endif /* MARCEL */

#if defined (MAD3)
  mad_channels_init(attr->madeleine);
#elif defined(MAD4)
  mad_dir_channel_init(attr->madeleine);
#endif /* MAD3 */

#if defined (MAD3) && defined (MARCEL)
  if (attr->madeleine->settings->leonie_dynamic_mode) {
    mad_command_thread_init(attr->madeleine);
  }
#endif /* MAD3 && MARCEL */

#ifdef PM2
  pm2_init_thread_related(argc, argv);
#endif /* PM2 */

#ifdef DSM
  dsm_pm2_init_before_startup_funcs(pm2self, pm2_conf_size);
#endif /* DSM */

#ifdef PM2
  pm2_init_exec_startup_funcs(argc, argv);
#endif

#ifdef DSM
   dsm_pm2_init_after_startup_funcs(pm2self, pm2_conf_size);
#endif /* DSM */

#ifdef PM2
  pm2_net_init_channels(argc, argv);
#endif /* PM2 */

#ifdef PM2
  pm2_net_servers_start(argc, argv);
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

void
common_exit(common_attr_t *attr)
{
  LOG_IN();
  if (!attr)
    {
      attr = &default_static_attr;
    }

#ifdef PM2
  pm2_net_request_end();
  pm2_net_wait_end();
#endif // PM2

#if defined (MAD3) || defined (MAD4)
  //
  // Leonie termination synchronisation
  // ----------------------------------
  //
  // Provides:
  // - 'leo_command_end' command transmission to Leonie
  // - synchronized virtual channels shutdown steering by Leonie
  // - regular channels clean-up
  //
  // Requires:
  // - Marcel full activity
  //

  mad_leonie_sync(attr->madeleine);
  mad_dir_channels_exit(attr->madeleine);
#endif // MAD3 || MAD4


#ifdef PM2
  pm2_thread_exit();
  block_exit();

#ifdef DSM
  dsm_pm2_exit();
#endif // DSM
#endif // PM2

#if defined  (MAD3) || defined (MAD4)
  mad_dir_driver_exit(attr->madeleine);
  mad_directory_exit(attr->madeleine);
  mad_leonie_link_exit(attr->madeleine);
#ifdef MAD3
  mad_leonie_command_exit(attr->madeleine);
#endif // MAD3
  mad_object_exit(attr->madeleine);
  attr->madeleine = NULL;
#ifdef MARCEL
  mad_mux_memory_manager_exit();
  mad_forward_memory_manager_exit();
#endif // MARCEL

  mad_memory_manager_exit();
#endif // MAD3 || MAD4

#ifdef MAD2
  mad_exit(attr->madeleine);
  attr->madeleine = NULL;
#endif // MAD2

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
  marcel_finish();
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

  LOG_OUT();
}
