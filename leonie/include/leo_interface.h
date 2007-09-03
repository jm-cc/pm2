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
 * leo_interface.h
 * ---------------
 */

#ifndef LEO_INTERFACE_H
#define LEO_INTERFACE_H

// Objects initialisation
//------------------------/////////////////////////////////////////////////

p_leo_settings_t
leo_settings_init(void);

p_leo_process_specific_t
leo_process_specific_init(void);

p_ntbx_server_t
leo_net_server_init(void);

p_tbx_htable_t
leo_htable_init(void);

p_leo_directory_t
leo_directory_init(void);

p_leo_dir_adapter_t
leo_dir_adapter_init(void);

p_leo_dir_driver_process_specific_t
leo_dir_driver_process_specific_init(void);

p_leo_dir_node_t
leo_dir_node_init(void);

p_leo_dir_driver_t
leo_dir_driver_init(void);

p_leo_dir_connection_t
leo_dir_connection_init(void);

p_leo_dir_channel_t
leo_dir_channel_init(void);

p_leo_dir_fchannel_t
leo_dir_fchannel_init(void);

p_leo_dir_connection_data_t
leo_dir_connection_data_init(void);

p_leo_dir_vxchannel_t
leo_dir_vxchannel_init(void);

p_leo_networks_t
leo_networks_init(void);

p_leo_spawn_groups_t
leo_spawn_groups_init(void);

p_leo_spawn_group_t
leo_spawn_group_init(void);

p_leo_loader_t
leo_loader_init(void);

p_leonie_t
leonie_init(void);


// Include files processing
//--------------------------///////////////////////////////////////////////
void
process_network_include_file(p_leo_networks_t networks,
			     p_tbx_htable_t   include_htable);

void
include_network_files(p_leo_networks_t networks,
		      p_tbx_slist_t    include_files_slist);


// Loaders
//---------////////////////////////////////////////////////////////////////
p_tbx_htable_t
leo_loaders_register(void);


// Communications
//----------------/////////////////////////////////////////////////////////
void
leo_send_int(p_ntbx_client_t client,
	     const int       data);

int
leo_receive_int(p_ntbx_client_t client);

void
leo_send_unsigned_int(p_ntbx_client_t    client,
		      const unsigned int data);

unsigned int
leo_receive_unsigned_int(p_ntbx_client_t client);

void
leo_send_string(p_ntbx_client_t  client,
		const char      *string);

char *
leo_receive_string(p_ntbx_client_t client);

// File Processing
//-----------------////////////////////////////////////////////////////////
void
process_application(p_leonie_t leonie);

// Processes Spawning
//--------------------/////////////////////////////////////////////////////
void
spawn_processes(p_leonie_t leonie);

void
send_directory(p_leonie_t leonie);

// Session Control
//-----------------////////////////////////////////////////////////////////
void
init_drivers(p_leonie_t leonie);

void
init_channels(p_leonie_t leonie);

void
init_fchannels(p_leonie_t leonie);

void
init_vchannels(p_leonie_t leonie);

void
init_xchannels(p_leonie_t leonie);

void
exit_session(p_leonie_t leonie);

// Clean-up
//----------///////////////////////////////////////////////////////////////
void
dir_xchannel_disconnect(p_leonie_t leonie);

void
dir_vchannel_disconnect(p_leonie_t leonie);

void
directory_exit(p_leonie_t leonie);


// Usage
//-------//////////////////////////////////////////////////////////////////
void
leo_usage(void) TBX_NORETURN;

void
leo_help(void) TBX_NORETURN;

void
leo_terminate(const char *msg, p_leo_settings_t settings) TBX_NORETURN;

void
leo_error(const char *command, p_leo_settings_t settings) TBX_NORETURN;

void
leonie_processes_cleanup(void);

// Helpers
//---------////////////////////////////////////////////////////////////////
#ifdef NEED_LEO_HELPERS

// prototypes

static
p_ntbx_client_t
process_to_client(p_ntbx_process_t p) TBX_UNUSED;

static
p_ntbx_client_t
get_client(p_ntbx_process_container_t pc,
           ntbx_process_lrank_t       l) TBX_UNUSED;

static
void
do_pc_global(p_ntbx_process_container_t _pc,
             void (*_f)(ntbx_process_grank_t _g)) TBX_UNUSED;

static
void
do_pc2_global(p_ntbx_process_container_t _pc,
             void (*_f)(ntbx_process_grank_t _g1,
                        ntbx_process_grank_t _g2)) TBX_UNUSED;

static
void
do_pc_global_s(p_ntbx_process_container_t _pc,
              void (*_f)(ntbx_process_grank_t  _g,
                         void                 *_s)) TBX_UNUSED;

static
void
do_pc_global_local_s(p_ntbx_process_container_t _pc,
                     void (*_f)(ntbx_process_grank_t  _g,
                                ntbx_process_lrank_t  _l,
                                void                 *_s)) TBX_UNUSED;

static
void
do_pc_send_global(p_ntbx_process_container_t _pc,
                 void (*_f)(ntbx_process_grank_t _g,
                            p_ntbx_client_t      _client)) TBX_UNUSED;

static
void
do_pc_cond_global(p_ntbx_process_container_t _pc,
                  tbx_bool_t (*_f)(ntbx_process_grank_t _g)) TBX_UNUSED;

static
void
do_pc_cond_send_global(p_ntbx_process_container_t _pc,
                      tbx_bool_t (*_f)(ntbx_process_grank_t _l,
                                       p_ntbx_client_t _client)) TBX_UNUSED;

// pc/local
static
void
do_pc_local(p_ntbx_process_container_t _pc,
            void (*_f)(ntbx_process_lrank_t _l)) TBX_UNUSED;

static
void
do_pc2_local(p_ntbx_process_container_t _pc,
             void (*_f)(ntbx_process_lrank_t _l1,
                        ntbx_process_lrank_t _l2)) TBX_UNUSED;

static
void
do_pc_local_s(p_ntbx_process_container_t _pc,
              void (*_f)(ntbx_process_lrank_t  _l,
                         void                 *_s)) TBX_UNUSED;

static
void
do_pc_local_p(p_ntbx_process_container_t _pc,
              void (*_f)(ntbx_process_lrank_t _l,
                         p_ntbx_process_t     _p)) TBX_UNUSED;

static
void
do_pc_send_local(p_ntbx_process_container_t _pc,
                 void (*_f)(ntbx_process_lrank_t _l,
                            p_ntbx_client_t      _client)) TBX_UNUSED;

static
void
do_pc_send_local_s(p_ntbx_process_container_t _pc,
                   void (*_f)(ntbx_process_lrank_t  _l,
                              p_ntbx_client_t       _client,
                              void                 *_s)) TBX_UNUSED;

static
void
do_pc_cond_local(p_ntbx_process_container_t _pc,
                 tbx_bool_t (*_f)(ntbx_process_lrank_t _l)) TBX_UNUSED;

static
void
do_pc_cond_send_local(p_ntbx_process_container_t _pc,
                      tbx_bool_t (*_f)(ntbx_process_lrank_t _l,
                                       p_ntbx_client_t _client)) TBX_UNUSED;

static
void
do_pc_s(p_ntbx_process_container_t _pc,
        void (*_f)(void *_s)) TBX_UNUSED;

static
void
do_pc_send(p_ntbx_process_container_t _pc,
           void (*_f)(p_ntbx_client_t _client)) TBX_UNUSED;

static
void
do_pc_send_string(p_ntbx_process_container_t _pc,
                  char*                      _str) TBX_UNUSED;

static
void
do_pc_send_s(p_ntbx_process_container_t _pc,
                 void (*_f)(p_ntbx_client_t  _client,
                            void            *_s)) TBX_UNUSED;

static
void
do_slist(p_tbx_slist_t _l,
         void (*_f)(void *_p)) TBX_UNUSED;

static
void
do_extract_slist(p_tbx_slist_t _l,
                 void (*_f)(void *_p)) TBX_UNUSED;

static
void
do_slist_nref(p_tbx_slist_t _l,
              p_tbx_slist_nref_t _nref,
              void (*_f)(void *_p)) TBX_UNUSED;

static
void
with_all_processes(p_leo_directory_t dir,
                   void (*_f)(p_ntbx_client_t _client)) TBX_UNUSED;


// definitions

static
p_ntbx_client_t
process_to_client(p_ntbx_process_t p)
{
  p_leo_process_specific_t ps = NULL;
  p_ntbx_client_t          cl = NULL;

  ps = p->specific;
  cl = ps?ps->client:NULL;

  return cl;
}

static
p_ntbx_client_t
get_client(p_ntbx_process_container_t pc,
           ntbx_process_lrank_t       l)
{
  p_ntbx_process_t p  = NULL;

  p = ntbx_pc_get_local_process(pc, l);
  return process_to_client(p);
}

// pc/global
static
void
do_pc_global(p_ntbx_process_container_t _pc,
            void (*_f)(ntbx_process_grank_t _g))
{
  ntbx_process_grank_t _g = -1;

  if (!ntbx_pc_first_global_rank(_pc, &_g))
    return;

  do
    {
      _f(_g);
    }
  while (ntbx_pc_next_global_rank(_pc, &_g));
}

static
void
do_pc2_global(p_ntbx_process_container_t _pc,
             void (*_f)(ntbx_process_grank_t _g1,
                        ntbx_process_grank_t _g2))
{
  ntbx_process_grank_t _g1 = -1;

  if (!ntbx_pc_first_global_rank(_pc, &_g1))
    return;

  do
    {
      ntbx_process_grank_t _g2 = -1;

      ntbx_pc_first_global_rank(_pc, &_g2);
      do
        {
          _f(_g1, _g2);
        }
      while (ntbx_pc_next_global_rank(_pc, &_g2));
    }
  while (ntbx_pc_next_global_rank(_pc, &_g1));
}

static
void
do_pc_global_s(p_ntbx_process_container_t _pc,
              void (*_f)(ntbx_process_grank_t  _g,
                         void                 *_s))
{
  ntbx_process_grank_t _g = -1;

  if (!ntbx_pc_first_global_rank(_pc, &_g))
    return;

  do
    {
      void *_s = ntbx_pc_get_global_specific(_pc, _g);
      _f(_g, _s);
    }
  while (ntbx_pc_next_global_rank(_pc, &_g));
}

static
void
do_pc_global_local_s(p_ntbx_process_container_t _pc,
                     void (*_f)(ntbx_process_grank_t  _g,
                                ntbx_process_lrank_t  _l,
                                void                 *_s))
{
  ntbx_process_grank_t _g = -1;
  ntbx_process_grank_t _l = -1;

  if (!ntbx_pc_first_global_rank(_pc, &_g))
    return;

  do
    {
      void *_s = ntbx_pc_get_global_specific(_pc, _g);
      _l = ntbx_pc_global_to_local(_pc, _g);
      _f(_g, _l, _s);
    }
  while (ntbx_pc_next_global_rank(_pc, &_g));
}

static
void
do_pc_send_global(p_ntbx_process_container_t _pc,
                 void (*_f)(ntbx_process_grank_t _g,
                            p_ntbx_client_t      _client))
{
  void _h(ntbx_process_grank_t _g)
    {
      p_ntbx_client_t _client = get_client(_pc, _g);
      _f(_g, _client);
    }

  LOG_IN();
  do_pc_global(_pc, _h);
  LOG_OUT();
}

static
void
do_pc_cond_global(p_ntbx_process_container_t _pc,
                 tbx_bool_t (*_f)(ntbx_process_grank_t _g))
{
  ntbx_process_grank_t _g = -1;

  LOG_IN();
  if (!ntbx_pc_first_global_rank(_pc, &_g))
    {
      LOG_OUT();
      return;
    }

  while (_f(_g) && ntbx_pc_next_global_rank(_pc, &_g))
    ;

  LOG_OUT();
}

static
void
do_pc_cond_send_global(p_ntbx_process_container_t _pc,
                      tbx_bool_t (*_f)(ntbx_process_grank_t _l,
                                       p_ntbx_client_t _client))
{
  tbx_bool_t _h(ntbx_process_grank_t _l)
    {
      p_ntbx_client_t _client = get_client(_pc, _l);
      return _f(_l, _client);
    }

  LOG_IN();
  do_pc_cond_global(_pc, _h);
  LOG_OUT();
}

// pc/local
static
void
do_pc_local(p_ntbx_process_container_t _pc,
            void (*_f)(ntbx_process_lrank_t _l))
{
  ntbx_process_lrank_t _l = -1;

  if (!ntbx_pc_first_local_rank(_pc, &_l))
    return;

  do
    {
      _f(_l);
    }
  while (ntbx_pc_next_local_rank(_pc, &_l));
}

static
void
do_pc2_local(p_ntbx_process_container_t _pc,
             void (*_f)(ntbx_process_lrank_t _l1,
                        ntbx_process_lrank_t _l2))
{
  ntbx_process_lrank_t _l1 = -1;

  if (!ntbx_pc_first_local_rank(_pc, &_l1))
    return;

  do
    {
      ntbx_process_lrank_t _l2 = -1;

      ntbx_pc_first_local_rank(_pc, &_l2);
      do
        {
          _f(_l1, _l2);
        }
      while (ntbx_pc_next_local_rank(_pc, &_l2));
    }
  while (ntbx_pc_next_local_rank(_pc, &_l1));
}

static
void
do_pc_local_s(p_ntbx_process_container_t _pc,
              void (*_f)(ntbx_process_lrank_t  _l,
                         void                 *_s))
{
  ntbx_process_lrank_t _l = -1;

  if (!ntbx_pc_first_local_rank(_pc, &_l))
    return;

  do
    {
      void *_s = ntbx_pc_get_local_specific(_pc, _l);
      _f(_l, _s);
    }
  while (ntbx_pc_next_local_rank(_pc, &_l));
}

static
void
do_pc_local_p(p_ntbx_process_container_t _pc,
              void (*_f)(ntbx_process_lrank_t _l,
                         p_ntbx_process_t     _p))
{
  ntbx_process_lrank_t _l = -1;

  if (!ntbx_pc_first_local_rank(_pc, &_l))
    return;

  do
    {
      p_ntbx_process_t _p = ntbx_pc_get_local_process(_pc, _l);
      _f(_l, _p);
    }
  while (ntbx_pc_next_local_rank(_pc, &_l));
}

static
void
do_pc_send_local(p_ntbx_process_container_t _pc,
                 void (*_f)(ntbx_process_lrank_t _l,
                            p_ntbx_client_t      _client))
{
  void _h(ntbx_process_lrank_t _l)
    {
      p_ntbx_client_t _client = get_client(_pc, _l);
      _f(_l, _client);
    }

  LOG_IN();
  do_pc_local(_pc, _h);
  LOG_OUT();
}

static
void
do_pc_send_local_s(p_ntbx_process_container_t _pc,
                   void (*_f)(ntbx_process_lrank_t  _l,
                              p_ntbx_client_t       _client,
                              void                 *_s))
{
  void _h(ntbx_process_lrank_t _l, void *_s)
    {
      p_ntbx_client_t _client = get_client(_pc, _l);

      _f(_l, _client, _s);
    }

  LOG_IN();
  do_pc_local_s(_pc, _h);
  LOG_OUT();
}

static
void
do_pc_cond_local(p_ntbx_process_container_t _pc,
                 tbx_bool_t (*_f)(ntbx_process_lrank_t _l))
{
  ntbx_process_lrank_t _l = -1;

  LOG_IN();
  if (!ntbx_pc_first_local_rank(_pc, &_l))
    {
      LOG_OUT();
      return;
    }

  while (_f(_l) && ntbx_pc_next_local_rank(_pc, &_l))
    ;

  LOG_OUT();
}

static
void
do_pc_cond_send_local(p_ntbx_process_container_t _pc,
                      tbx_bool_t (*_f)(ntbx_process_lrank_t _l,
                                       p_ntbx_client_t _client))
{
  tbx_bool_t _h(ntbx_process_lrank_t _l)
    {
      p_ntbx_client_t _client = get_client(_pc, _l);
      return _f(_l, _client);
    }

  LOG_IN();
  do_pc_cond_local(_pc, _h);
  LOG_OUT();
}

// pc
static
void
do_pc_s(p_ntbx_process_container_t _pc,
        void (*_f)(void *_s))
{
  ntbx_process_lrank_t _l = -1;

  if (!ntbx_pc_first_local_rank(_pc, &_l))
    return;

  do
    {
      void *_s = ntbx_pc_get_local_specific(_pc, _l);
      _f(_s);
    }
  while (ntbx_pc_next_local_rank(_pc, &_l));
}

static
void
do_pc_send(p_ntbx_process_container_t _pc,
           void (*_f)(p_ntbx_client_t _client))
{
  void _h(ntbx_process_lrank_t _l)
    {
      p_ntbx_client_t _client = get_client(_pc, _l);
      _f(_client);
    }

  LOG_IN();
  do_pc_local(_pc, _h);
  LOG_OUT();
}

static
void
do_pc_send_string(p_ntbx_process_container_t _pc,
                  char*                      _str)
{
  void _h(p_ntbx_client_t cl) {
    leo_send_string(cl, _str);
  }

  LOG_IN();
  do_pc_send(_pc, _h);
  LOG_OUT();
}

static
void
do_pc_send_s(p_ntbx_process_container_t _pc,
                 void (*_f)(p_ntbx_client_t  _client,
                            void            *_s))
{
  void _h(ntbx_process_lrank_t _l, void *_s)
    {
      p_ntbx_client_t _client = get_client(_pc, _l);
      _f(_client, _s);
    }

  LOG_IN();
  do_pc_local_s(_pc, _h);
  LOG_OUT();
}

// slist
static
void
do_slist(p_tbx_slist_t _l,
         void (*_f)(void *_p))
{
  if (tbx_slist_is_nil(_l))
    return;

  tbx_slist_ref_to_head(_l);
  do
    {
      void *_p = tbx_slist_ref_get(_l);
      _f(_p);
    }
  while (tbx_slist_ref_forward(_l));
}

static
void
do_extract_slist(p_tbx_slist_t _l,
                void (*_f)(void *_p))
{
  while(!tbx_slist_is_nil(_l))
    {
      void *_p = tbx_slist_extract(_l);
      _f(_p);
    }
}

static
void
do_slist_nref(p_tbx_slist_t _l,
              p_tbx_slist_nref_t _nref,
              void (*_f)(void *_p))
{
  if (tbx_slist_is_nil(_l))
    return;

  tbx_slist_nref_to_head(_nref);
  do
    {
      void *_p = tbx_slist_nref_get(_nref);
      _f(_p);
    }
  while (tbx_slist_nref_forward(_nref));
}

static
void
with_all_processes(p_leo_directory_t dir,
                   void (*_f)(p_ntbx_client_t _client))
{
  void _h(void *_p)
    {
      _f(process_to_client(_p));
    }

  do_slist(dir->process_slist, _h);
}
#endif /* NEED_LEO_HELPER */
#endif /* LEO_INTERFACE_H */
