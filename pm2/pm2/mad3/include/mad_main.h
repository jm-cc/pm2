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

/*
 * Structures
 * ----------
 */

typedef struct s_mad_session
{
  p_ntbx_client_t      leonie_link;
  ntbx_process_grank_t process_rank;
} mad_session_t;

typedef struct s_mad_settings
{
  char *leonie_server_host_name;
  char *leonie_server_port;
} mad_settings_t;


typedef struct s_mad_madeleine
{
  TBX_SHARED;
  p_mad_settings_t  settings;
  p_mad_session_t   session;
  p_mad_directory_t dir;
  p_tbx_htable_t    driver_htable;
  p_tbx_htable_t    channel_htable;
  p_tbx_slist_t     public_channel_slist;
} mad_madeleine_t;

/*
 * Functions 
 * ---------
 */
p_mad_madeleine_t
mad_object_init(int    argc TBX_UNUSED,
		char **argv TBX_UNUSED);

void
mad_cmd_line_init(p_mad_madeleine_t   madeleine,
		  int                 argc,
		  char              **argv);

void
mad_leonie_link_init(p_mad_madeleine_t   madeleine,
		     int                 argc TBX_UNUSED,
		     char              **argv TBX_UNUSED);

void
mad_directory_init(p_mad_madeleine_t   madeleine,
		   int                 argc TBX_UNUSED,
		   char              **argv TBX_UNUSED);

void
mad_dir_driver_init(p_mad_madeleine_t   madeleine);

void
mad_dir_channel_init(p_mad_madeleine_t   madeleine);

void
mad_dir_vchannel_exit(p_mad_madeleine_t madeleine);

void
mad_purge_command_line(p_mad_madeleine_t   madeleine TBX_UNUSED,
		       int                *_argc,
		       char              **_argv);

p_mad_madeleine_t
mad_get_madeleine(void);

/*
 * Private part : these functions should not be called directly
 * --------------...............................................
 */

p_mad_madeleine_t
mad_init(int   *argc,
	 char **argv);

// NTBX data transmission wrappers
void
mad_ntbx_send_int(p_ntbx_client_t client,
		  const int       data);

int
mad_ntbx_receive_int(p_ntbx_client_t client);

void
mad_ntbx_send_unsigned_int(p_ntbx_client_t    client,
			   const unsigned int data);

unsigned int
mad_ntbx_receive_unsigned_int(p_ntbx_client_t client);

void
mad_ntbx_send_string(p_ntbx_client_t  client,
		     const char      *string);

char *
mad_ntbx_receive_string(p_ntbx_client_t client);

// Directory transmission
void
mad_dir_directory_get(p_mad_madeleine_t madeleine);

#endif /* MAD_MAIN_H */
