
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

p_mad_dir_channel_process_specific_t
mad_dir_channel_process_specific_cons(void)
{
  p_mad_dir_channel_process_specific_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_dir_channel_process_specific_t));
  LOG_OUT();
  
  return object;
}

p_mad_dir_channel_t
mad_dir_channel_cons(void)
{
  p_mad_dir_channel_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_dir_channel_t));
  
  object->pc     = ntbx_pc_cons();
  object->public = tbx_true;
  LOG_OUT();
  
  return object;
}

p_mad_dir_vchannel_process_routing_table_t
mad_dir_vchannel_process_routing_table_cons(void)
{
  p_mad_dir_vchannel_process_routing_table_t object = NULL;

  LOG_IN();
  object =
    TBX_CALLOC(1, sizeof(mad_dir_vchannel_process_routing_table_t));
  
  object->destination_rank = -1;
  LOG_OUT();
  
  return object;
}

p_mad_dir_vchannel_process_specific_t
mad_dir_vchannel_process_specific_cons(void)
{
  p_mad_dir_vchannel_process_specific_t object = NULL;
  
  LOG_IN();
  object =
    TBX_CALLOC(1, sizeof(mad_dir_vchannel_process_specific_t));
  
  object->pc = ntbx_pc_cons();
  LOG_OUT();
  
  return object;
}

p_mad_dir_fchannel_t
mad_dir_fchannel_cons(void)
{
  p_mad_dir_fchannel_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_dir_fchannel_t));
  LOG_OUT();
  
  return object;
}

p_mad_dir_vchannel_t
mad_dir_vchannel_cons(void)
{
  p_mad_dir_vchannel_t object = NULL;
  
  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_dir_vchannel_t));

  object->dir_channel_slist  = tbx_slist_nil();
  object->dir_fchannel_slist = tbx_slist_nil();
  object->pc                 = ntbx_pc_cons();
  LOG_OUT();
  
  return object;
}

p_mad_directory_t
mad_directory_cons(void)
{
  p_mad_directory_t object = NULL;
  
  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_directory_t));

  object->process_darray    = tbx_darray_init();
  object->process_slist     = tbx_slist_nil();
  object->node_htable       = tbx_htable_empty_table();
  object->node_slist        = tbx_slist_nil();
  object->driver_htable     = tbx_htable_empty_table();
  object->driver_slist      = tbx_slist_nil();
  object->channel_htable    = tbx_htable_empty_table();
  object->channel_slist     = tbx_slist_nil();
  object->fchannel_htable   = tbx_htable_empty_table();
  object->fchannel_slist    = tbx_slist_nil();
  object->vchannel_htable   = tbx_htable_empty_table();
  object->vchannel_slist    = tbx_slist_nil();
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

  TBX_INIT_SHARED(object);
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

  TBX_INIT_SHARED(object);
  object->id = -1;
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

  TBX_INIT_SHARED(object);
  object->id = -1;

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
 
  TBX_INIT_SHARED(object);

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

  TBX_INIT_SHARED(object);
  object->id = -1;
  LOG_OUT();
  
  return object;
}

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

// mad_madeleine_t
p_mad_madeleine_t
mad_madeleine_cons(void)
{
  p_mad_madeleine_t object = NULL;
  
  LOG_IN();
  object = TBX_CALLOC(1, sizeof(mad_madeleine_t));

  TBX_INIT_SHARED(object);
  object->driver_htable        = tbx_htable_empty_table();
  object->channel_htable       = tbx_htable_empty_table();
  object->public_channel_slist = tbx_slist_nil();
  LOG_OUT();

  return object;
}
