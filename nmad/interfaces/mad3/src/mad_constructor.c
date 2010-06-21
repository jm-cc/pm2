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
 * Mad_constructor.c
 * =================
 */

#include <stdlib.h>
#include <stdio.h>

#include "madeleine.h"

p_mad_dir_node_t
mad_dir_node_cons(void)
{
  p_mad_dir_node_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_dir_node_t));

  object->pc = ntbx_pc_cons();
  LOG_OUT();

  return object;
}

p_mad_dir_adapter_t
mad_dir_adapter_cons(void)
{
  p_mad_dir_adapter_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_dir_adapter_t));
  LOG_OUT();

  return object;
}

p_mad_dir_driver_process_specific_t
mad_dir_driver_process_specific_cons(void)
{
  p_mad_dir_driver_process_specific_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_dir_driver_process_specific_t));

  object->adapter_htable = tbx_htable_empty_table();
  object->adapter_slist  = tbx_slist_nil();
  LOG_OUT();

  return object;
}

p_mad_dir_driver_t
mad_dir_driver_cons(void)
{
  p_mad_dir_driver_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_dir_driver_t));

  object->pc = ntbx_pc_cons();
  LOG_OUT();

  return object;
}

p_mad_dir_connection_t
mad_dir_connection_cons(void)
{
  p_mad_dir_connection_t object = NULL;

  LOG_IN();
  object     = TBX_CALLOC(1, sizeof(mad_dir_connection_t));
  object->pc = ntbx_pc_cons();
  LOG_OUT();

  return object;
}

p_mad_dir_channel_t
mad_dir_channel_cons(void)
{
  p_mad_dir_channel_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_dir_channel_t));

  object->pc          = ntbx_pc_cons();
  object->not_private = tbx_true;
  object->dir_channel_slist  = tbx_slist_nil();
  object->dir_fchannel_slist = tbx_slist_nil();
  object->sub_channel_name_slist = tbx_slist_nil();
  LOG_OUT();

  return object;
}

p_mad_dir_connection_data_t
mad_dir_connection_data_cons(void)
{
  p_mad_dir_connection_data_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_dir_connection_data_t));
  LOG_OUT();

  return object;
}

p_mad_directory_t
mad_directory_cons(void)
{
  p_mad_directory_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_directory_t));

  object->process_darray     = tbx_darray_init();
  object->process_slist      = tbx_slist_nil();
  object->node_htable        = tbx_htable_empty_table();
  object->node_slist         = tbx_slist_nil();
  object->driver_htable      = tbx_htable_empty_table();
  object->driver_slist       = tbx_slist_nil();
  object->channel_htable     = tbx_htable_empty_table();
  object->channel_slist      = tbx_slist_nil();
  object->fchannel_slist     = tbx_slist_nil();
  object->vchannel_slist     = tbx_slist_nil();
  object->xchannel_slist     = tbx_slist_nil();
  LOG_OUT();

  return object;
}



// mad_driver_interface_t
p_mad_driver_interface_t
mad_driver_interface_cons(void)
{
  p_mad_driver_interface_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_driver_interface_t));
  LOG_OUT();

  return object;
}

// mad_driver_t
p_mad_driver_t
mad_driver_cons(void)
{
  p_mad_driver_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_driver_t));

  object->id               =   -1;
  object->buffer_alignment =    0;
  object->connection_type  = mad_undefined_connection_type;
  LOG_OUT();

  return object;
}

// mad_adapter_t
p_mad_adapter_t
mad_adapter_cons(void)
{
  p_mad_adapter_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_adapter_t));

  object->id        =  -1;
  object->parameter = "-";
  LOG_OUT();

  return object;
}

// mad_channel_t
p_mad_channel_t
mad_channel_cons(void)
{
  p_mad_channel_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_channel_t));

  object->id              =  -1;
  object->parameter       = "-";
  object->sub_channel_darray = tbx_darray_init();
  tbx_darray_expand_and_set(object->sub_channel_darray, 0, object);

#ifdef MARCEL
  marcel_mutex_init(&(object->reception_lock_mutex), NULL);
#endif // MARCEL
  LOG_OUT();

  return object;
}

// mad_connection_t
p_mad_connection_t
mad_connection_cons(void)
{
  p_mad_connection_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_connection_t));

  object->parameter = "-";

  object->user_buffer_list_reference = TBX_MALLOC(sizeof(tbx_list_reference_t));
  object->user_buffer_list           = TBX_MALLOC(sizeof(tbx_list_t));
  object->buffer_list                = TBX_MALLOC(sizeof(tbx_list_t));
  object->buffer_group_list          = TBX_MALLOC(sizeof(tbx_list_t));
  object->pair_list                  = TBX_MALLOC(sizeof(tbx_list_t));


#ifdef MARCEL
  marcel_mutex_init(&(object->lock_mutex), NULL);
#endif // MARCEL
  LOG_OUT();

  return object;
}

// mad_link_t
p_mad_link_t
mad_link_cons(void)
{
  p_mad_link_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_link_t));

  object->id = -1;

  object->link_mode        = mad_link_mode_buffer;
  object->buffer_mode      = mad_buffer_mode_dynamic;
  object->group_mode       = mad_group_mode_split;
  object->buffer_list      = TBX_MALLOC(sizeof(tbx_list_t));
  object->user_buffer_list = TBX_MALLOC(sizeof(tbx_list_t));
  LOG_OUT();

  return object;
}

// mad_pmi_t
#ifdef MAD3_PMI
p_mad_pmi_t
mad_pmi_cons(void)
{
  p_mad_pmi_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_pmi_t));
  LOG_OUT();

  return object;
}
#endif /* MAD3_PMI */

// mad_session_t
p_mad_session_t
mad_session_cons(void)
{
  p_mad_session_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_session_t));

  object->process_rank = -1;
  LOG_OUT();

  return object;
}

// mad_settings_t
p_mad_settings_t
mad_settings_cons(void)
{
  p_mad_settings_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_settings_t));
  LOG_OUT();

  return object;
}

// mad_dynamic_t
p_mad_dynamic_t
mad_dynamic_cons(void)
{
  p_mad_dynamic_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_dynamic_t));
  LOG_OUT();

  return object;
}


// mad_madeleine_t
p_mad_madeleine_t
mad_madeleine_cons(void)
{
  p_mad_madeleine_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_madeleine_t));

  object->device_htable        = tbx_htable_empty_table();
  object->network_htable       = tbx_htable_empty_table();
  object->channel_htable       = tbx_htable_empty_table();
  object->public_channel_slist = tbx_slist_nil();
  LOG_OUT();

  return object;
}
