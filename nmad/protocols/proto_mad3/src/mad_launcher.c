/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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
 * mad_launcher.c
 * ==============
 */


#include <nm_public.h>
#include <nm_launcher.h>
#include <nm_mad3_public.h>
#include <nm_mad3_private.h>

struct mad_launcher_status_s
{
  p_mad_madeleine_t madeleine;
  p_mad_session_t session;
  struct nm_core*p_core;
  struct nm_so_interface*p_so_sr_if;
  nm_so_pack_interface p_so_pack_if;
  int rank;
  int size;
};

/* ********************************************************* */

static void*mad_launcher_instanciate(puk_instance_t, puk_context_t);
static void mad_launcher_destroy(void*_status);

const static struct puk_adapter_driver_s mad_launcher_adapter =
  {
    .instanciate = &mad_launcher_instanciate,
    .destroy     = &mad_launcher_destroy
  };

/* ********************************************************* */

static void mad_launcher_init(void*_status, int*argc, char**argv, const char*group_name, int(*sched_load)(struct nm_sched_ops*));
static int                    mad_launcher_get_size(void*_status);
static int                    mad_launcher_get_rank(void*_status);
static struct nm_so_interface*mad_launcher_get_so_sr_if(void*_status);
static nm_so_pack_interface   mad_launcher_get_so_pack_if(void*_status);
static void                   mad_launcher_get_gate_ids(void*_status, nm_gate_id_t*ids);

const static struct newmad_launcher_driver_s mad_launcher_driver =
  {
    .init           = &mad_launcher_init,
    .get_size       = &mad_launcher_get_size,
    .get_rank       = &mad_launcher_get_rank,
    .get_so_sr_if   = &mad_launcher_get_so_sr_if,
    .get_so_pack_if = &mad_launcher_get_so_pack_if,
    .get_gate_ids   = &mad_launcher_get_gate_ids
  };


/* ********************************************************* */

static void*mad_launcher_instanciate(puk_instance_t instance, puk_context_t context)
{
  struct mad_launcher_status_s*status = padico_malloc(sizeof(struct mad_launcher_status_s));
  status->madeleine = NULL;
  status->session = NULL;
  return status;
}

static void mad_launcher_destroy(void*_status)
{
  struct mad_launcher_status_s*status = _status;
  if(status->madeleine)
    {
      mad_exit(status->madeleine);
      status->madeleine = NULL;
    }
  padico_free(_status);
}

/* ********************************************************* */
static void mad_launcher_init(void*_status, int*argc, char**argv,
			      const char*group_name, int(*sched_load)(struct nm_sched_ops*))
{
  struct mad_launcher_status_s*status = _status;
 
  /*
   * Initialization of various libraries.
   * Reference to the Madeleine object.
   */
  status->madeleine = mad_init(argc, argv);

  /*
   * Reference to the session information object
   */
  status->session = status->madeleine->session;

  /*
   * Globally unique process rank.
   */
  status->rank = status->session->process_rank;

  /*
   * How to obtain the configuration size.
   */
  status->size = tbx_slist_get_length(status->madeleine->dir->process_slist);

  /*
   * Reference to the NewMadeleine core object
   */
  status->p_core = mad_nmad_get_core();
  status->p_so_sr_if = mad_nmad_get_sr_interface();
  status->p_so_pack_if = (nm_so_pack_interface)status->p_so_sr_if;
  
}

static int mad_launcher_get_size(void*_status)
{
  struct mad_launcher_status_s*status = _status;
  return status->size;
}

static int mad_launcher_get_rank(void*_status)
{
  struct mad_launcher_status_s*status = _status;
  return status->rank;
}

static struct nm_so_interface*mad_launcher_get_so_sr_if(void*_status)
{
  struct mad_launcher_status_s*status = _status;
  return status->p_so_sr_if;
}

static nm_so_pack_interface mad_launcher_get_so_pack_if(void*_status)
{
  struct mad_launcher_status_s*status = _status;
  return status->p_so_pack_if;
}

static void mad_launcher_get_gate_ids(void*_status, nm_gate_id_t*ids)
{
  struct mad_launcher_status_s*status = _status;
  int                              dest;
  int                              source;
  p_mad_channel_t                  channel    = NULL;
  p_mad_connection_t               connection = NULL;
  p_mad_nmad_connection_specific_t cs	      = NULL;


  /** Get a reference to the channel structure */
  channel = tbx_htable_get(status->madeleine->channel_htable, "pm2");

  /** If that fails, it means that our process does not belong to the channel */
  if (!channel) {
    fprintf(stderr, "I don't belong to this channel");
    LOG_OUT();
  }

  for(dest = 0 ; dest < status->size ; dest++) {
    ids[dest] = NM_ANY_GATE;
    if (dest == status->rank) continue;

    connection = tbx_darray_get(channel->out_connection_darray, dest);
    if (!connection) {
      NM_DISPF("Cannot find a connection between %d and %d", status->rank, dest);
    }
    else {
      cs = connection->specific;
      ids[dest] = cs->gate_id;
      ids[cs->gate_id] = dest;
      NM_TRACEF("Connection out (%p) with dest %d is %d\n", connection, dest, cs->gate_id);
    }
  }

  for(source = 0 ; source < status->size ; source++) {
    ids[source] = NM_ANY_GATE;
    if (source == status->rank) continue;

    connection =  tbx_darray_get(channel->in_connection_darray, source);
    if (!connection) {
      NM_DISPF("Cannot find a in connection between %d and %d", status->rank, source);
    }
    else {
      NM_TRACEF("Connection in: %p\n", connection);
      cs = connection->specific;
      ids[source] = cs->gate_id;
      ids[cs->gate_id] = source;
    }
  }

}

/* ********************************************************* */

extern void nm_mad3_launcher_init(void)
{
  puk_adapter_declare("NewMad_Launcher",
                      puk_adapter_provides("PadicoAdapter",   &mad_launcher_adapter),
                      puk_adapter_provides("NewMad_Launcher", &mad_launcher_driver ));
}

