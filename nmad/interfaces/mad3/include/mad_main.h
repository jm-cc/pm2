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

/*
 * Mad_main.h
 * ===========
 */

#ifndef MAD_MAIN_H
#define MAD_MAIN_H

#include "tbx_compiler.h"

typedef enum e_mad_vrp_parameter_opcode
  {
    mad_op_uninitialized = 0,
    mad_op_optional_block,
  } mad_parameter_opcode_t, *p_mad_parameter_opcode_t;

typedef enum e_mad_status_opcode
  {
    mad_os_uninitialized = 0,
    mad_os_lost_block,
  } mad_status_opcode_t, *p_mad_status_opcode_t;

/*
 * Structures
 * ----------
 */

typedef struct s_mad_session
{
  p_ntbx_client_t      leonie_link;
  ntbx_process_grank_t process_rank;
  ntbx_process_grank_t session_id;
#ifdef MARCEL
  marcel_t             command_thread;
#endif /* MARCEL */
} mad_session_t;

typedef struct s_mad_settings
{
  char         *leonie_server_host_name;
  char         *leonie_server_port;
  tbx_bool_t    leonie_dynamic_mode;
} mad_settings_t;

typedef struct s_mad_dynamic
{
  volatile tbx_bool_t mergeable;
  volatile tbx_bool_t updated;
  volatile tbx_bool_t merge_done;
  volatile tbx_bool_t split_done;
} mad_dynamic_t;


typedef struct s_mad_madeleine
{
  p_mad_dynamic_t     dynamic;
  p_mad_settings_t    settings;
  p_mad_session_t     session;
  p_mad_directory_t   dir;
  p_mad_directory_t   old_dir;
  p_mad_directory_t   new_dir;
  p_tbx_htable_t      device_htable;
  p_tbx_htable_t      network_htable;
  p_tbx_htable_t      channel_htable;
  p_tbx_slist_t       public_channel_slist;
  p_tbx_darray_t      cnx_darray;
  int                 master_channel_id;
#ifdef MAD3_PMI
  p_mad_pmi_t         pmi;
#endif /* MAD3_PMI */
} mad_madeleine_t;

/*
 * Functions
 * ---------
 */
p_mad_madeleine_t
mad_object_init(int   TBX_UNUSED   argc,
		char  TBX_UNUSED **argv);

void
mad_cmd_line_init(p_mad_madeleine_t   madeleine,
		  int                 argc,
		  char              **argv);

void
mad_leonie_link_init(p_mad_madeleine_t              madeleine,
		     int               TBX_UNUSED   argc,
		     char              TBX_UNUSED **argv);

void
mad_directory_init(p_mad_madeleine_t               madeleine,
		   int                TBX_UNUSED   argc,
		   char               TBX_UNUSED **argv);

void
mad_drivers_init(p_mad_madeleine_t                madeleine,
                 int                TBX_UNUSED   *p_argc,
                 char               TBX_UNUSED ***p_argv);

void
mad_channels_init(p_mad_madeleine_t               madeleine);

void
mad_dir_driver_init(p_mad_madeleine_t    madeleine,
		   int                  *p_argc,
		   char               ***p_argv);

void
mad_dir_channel_init(p_mad_madeleine_t   madeleine);

void
mad_purge_command_line(p_mad_madeleine_t TBX_UNUSED madeleine,
		       int                         *_argc,
		       char                       **_argv);

p_mad_madeleine_t
mad_get_madeleine(void);

/*
 * Private part : these functions should not be called directly
 * --------------...............................................
 */

p_mad_madeleine_t
mad_init(int   *argc,
	 char **argv);

#ifdef MAD3_PMI
void
mad_pmi_init(p_mad_madeleine_t madeleine);
#endif /* MAD3_PMI */

// NTBX data transmission wrappers
TBX_INTERNAL void
mad_ntbx_send_int(p_ntbx_client_t client,
		  const int       data);

TBX_INTERNAL int
mad_ntbx_receive_int(p_ntbx_client_t client);

TBX_INTERNAL void
mad_ntbx_send_unsigned_int(p_ntbx_client_t    client,
			   const unsigned int data);

TBX_INTERNAL unsigned int
mad_ntbx_receive_unsigned_int(p_ntbx_client_t client);

TBX_INTERNAL void
mad_ntbx_send_string(p_ntbx_client_t  client,
		     const char      *string);

TBX_INTERNAL char *
mad_ntbx_receive_string(p_ntbx_client_t client);

TBX_INTERNAL void
mad_leonie_send_int_nolock(const int data);

TBX_INTERNAL int
mad_leonie_receive_int_nolock(void);

TBX_INTERNAL void
mad_leonie_send_unsigned_int_nolock(const unsigned int data);

TBX_INTERNAL unsigned int
mad_leonie_receive_unsigned_int_nolock(void);

TBX_INTERNAL void
mad_leonie_send_int(const int data);

TBX_INTERNAL int
mad_leonie_receive_int(void);

TBX_INTERNAL void
mad_leonie_send_unsigned_int(const unsigned int data);

TBX_INTERNAL unsigned int
mad_leonie_receive_unsigned_int(void);

TBX_INTERNAL void
mad_leonie_send_string(const char *string);

TBX_INTERNAL char *
mad_leonie_receive_string(void);

TBX_INTERNAL void
mad_leonie_send_string_nolock(const char *string);

TBX_INTERNAL char *
mad_leonie_receive_string_nolock(void);

void
mad_leonie_command_init(p_mad_madeleine_t   madeleine,
			int                 argc,
			char              **argv);

void
mad_leonie_command_exit(p_mad_madeleine_t   madeleine);

void
mad_leonie_print(char *fmt, ...)  TBX_FORMAT (printf, 1, 2);

TBX_INTERNAL void
mad_leonie_print_without_nl(char *fmt, ...)  TBX_FORMAT (printf, 1, 2);

TBX_INTERNAL void
mad_leonie_barrier(void);

void
mad_leonie_beat(void);

void
mad_command_thread_init(p_mad_madeleine_t madeleine);

void
mad_command_thread_exit(p_mad_madeleine_t madeleine);


#define LDISP(str, ...)      mad_leonie_print(str , ## __VA_ARGS__)
#define LDISP_IN()           mad_leonie_print(__FUNCTION__": -->")
#define LDISP_OUT()          mad_leonie_print(__FUNCTION__": <--")
#define LDISP_VAL(str, val)  mad_leonie_print(str " = %d" , (int)(val))
#define LDISP_CHAR(val)      mad_leonie_print("%c" , (char)(val))
#define LDISP_PTR(str, ptr)  mad_leonie_print(str " = %p" , (void *)(ptr))
#define LDISP_STR(str, str2) mad_leonie_print(str ": %s" , (char *)(str2))

// Directory transmission
TBX_INTERNAL void
mad_dir_directory_get(p_mad_madeleine_t madeleine);

TBX_INTERNAL void
mad_new_directory_from_leony(p_mad_madeleine_t madeleine);

TBX_INTERNAL void
mad_directory_update(p_mad_madeleine_t madeleine);

TBX_INTERNAL void
mad_directory_rollback(p_mad_madeleine_t madeleine);

TBX_INTERNAL int
mad_directory_is_updated(p_mad_madeleine_t madeleine);

#endif /* MAD_MAIN_H */
