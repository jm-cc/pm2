/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: init.c,v $
Revision 1.10  2000/12/11 08:31:13  oaumage
- support Leonie

Revision 1.9  2000/11/20 10:26:39  oaumage
- initialisation, nouvelle version

Revision 1.8  2000/11/16 16:16:37  rnamyst
mad1 is up-to-date again + various bug fixes (Malefiles)

Revision 1.7  2000/11/16 14:21:48  oaumage
- correction external spawn

Revision 1.6  2000/11/16 13:24:02  oaumage
- mise a jour initialisation

Revision 1.5  2000/11/16 11:11:48  rnamyst
Bug fixed in mad_purge_command_line + small changes in pm2-config (handling of 'common').

Revision 1.4  2000/11/13 20:41:33  rnamyst
common_init now performs calls to all libraries

Revision 1.3  2000/11/10 14:17:52  oaumage
- nouvelle procedure d'initialisation

Revision 1.2  2000/11/06 15:02:18  rnamyst
pm2_init() has now a modular structure (in fact, common_init).

Revision 1.1  2000/11/05 20:50:20  rnamyst
The 'common' module now (almost) behaves like a classical one : there is a libcommon.a file, etc.

______________________________________________________________________________
*/

#ifdef PM2
#include "pm2.h"
#endif /* PM2 */

#ifdef MARCEL
#include "marcel.h"
#endif /* MARCEL */

#ifdef MAD1
#include "madeleine.h"
#endif /* MAD1 */

#ifdef MAD2
#include "madeleine.h"
#endif /* MAD2 */

#ifdef TBX
#include "tbx.h"
#endif /* TBX */

#ifdef NTBX
#include "ntbx.h"
#endif /* NTBX */

void common_init(int *argc, char *argv[])
{
#ifdef MAD2
  /* Objet Madeleine (II) */
  p_mad_madeleine_t madeleine = NULL;
#endif /* MAD2 */

#ifdef PM2
  unsigned pm2_self, pm2_conf_size;
#endif /* PM2 */

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
    void *adapter_set        = NULL;
    
    madeleine = mad_object_init(*argc, argv, configuration_file, adapter_set);
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
  pm2_mad_init(madeleine);
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
   * - pm2_self
   * - pm2_conf_size
   *
   * Requires:
   * - the `madeleine' object
   */
  mad_spawn_driver_init(madeleine, argc, argv);
#endif /* MAD2 && EXTERNAL_SPAWN */

#ifdef MAD2
  /*
   * Mad2 command line parsing
   * -------------------------
   *
   * Provides:
   * - Mad2 initialization from command line arguments
   * - node rank
   * - pm2_self
   *
   * Requires:
   * - the `madeleine' object
   */
  mad_cmd_line_init(madeleine, *argc, argv);

#ifdef PM2
  pm2_self = madeleine->configuration->local_host_id;
#endif /* PM2 */

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
  mad_output_redirection_init(madeleine, *argc, argv);
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
  mad_configuration_init(madeleine, *argc, argv);
  
#ifdef PM2
  pm2_conf_size = madeleine->configuration->size;
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
  mad_network_components_init(madeleine, *argc, argv);
#endif /* MAD2 */

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
  mad_purge_command_line(madeleine, argc, argv);
#endif /* MAD2 */

#ifdef MAD2

#ifndef EXTERNAL_SPAWN
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
  mad_slave_spawn(madeleine, *argc, argv);
#endif /* EXTERNAL_SPAWN */
  
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
  mad_connect(madeleine, *argc, argv);
#endif /* MAD2 */

#ifdef MAD1
  mad_init(argc, argv, 0, NULL, &pm2_conf_size, &pm2_self);
#endif /* MAD1 */

#ifdef PM2
  pm2_init_open_channels(argc, argv, pm2_self, pm2_conf_size);
#endif /* PM2 */

#ifdef MARCEL
  marcel_start_sched(argc, argv);
#endif /* MARCEL */

#ifdef PM2
  pm2_init_thread_related(argc, argv);
#endif /* PM2 */

#ifdef DSM
  dsm_pm2_init(pm2_self, pm2_conf_size);
#endif /* DSM */

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
