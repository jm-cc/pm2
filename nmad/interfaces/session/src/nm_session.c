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

#include <nm_public.h>
#include <nm_private.h>
#include <nm_core_interface.h>
#include <nm_session_interface.h>
#include <nm_session_private.h>
#include <nm_sendrecv_interface.h>
#include <Padico/Puk.h>
#include <tbx.h>

#include <string.h>
#include <assert.h>

#include <Padico/Module.h>
PADICO_MODULE_HOOK(NewMad_Core);


/** A driver descriptor
 */
struct nm_session_driver_s
{
  struct nm_drv*p_drv;
  int index;
  char*name;
};

static struct
{
  struct nm_core*p_core;    /**< the current nmad object. */
  int n_drivers;            /**< number of drivers */
  struct nm_session_driver_s*drivers; /**< array of drivers */
  const char*local_url;     /**< the local nmad driver url */
  padico_string_t url_string; /**< local url as a string */
  puk_hashtable_t gates;    /**< list of connected gates */
  puk_hashtable_t sessions; /**< table of active sessions, hashed by session hashcode */
  int ref_count;            /**< ref. counter for core = number of active sessions */
} nm_session =
  {
    .p_core    = NULL,
    .n_drivers = 0,
    .drivers   = NULL,
    .local_url = NULL,
    .gates     = NULL,
    .sessions  = NULL,
    .ref_count = 0
  };


/* ********************************************************* */

int nm_session_code_eq(const void*key1, const void*key2)
{
  const uint32_t*p_code1 = key1;
  const uint32_t*p_code2 = key2;
  return (*p_code1 == *p_code2);
}

uint32_t nm_session_code_hash(const void*key)
{
  const uint32_t*p_hash_code = key;
  return *p_hash_code;
}

/** Initialize the strategy */
static void nm_session_init_strategy(int*argc, char**argv)
{
  const char*strategy_name = getenv("NMAD_STRATEGY");
  if(!strategy_name)
    {
#if defined(CONFIG_STRAT_SPLIT_BALANCE)
      strategy_name = "split_balance";
#elif defined(CONFIG_STRAT_SPLIT_ALL)
      strategy_name = "split_all";
#elif defined(CONFIG_STRAT_DEFAULT)
      strategy_name = "default";
#elif defined(CONFIG_STRAT_AGGREG)
      strategy_name = "aggreg";
#elif defined(CONFIG_STRAT_AGGREG_AUTOEXTENDED)
      strategy_name = "aggreg_autoextended";
#else /*  defined(CONFIG_STRAT_CUSTOM) */
      strategy_name = "aggreg";
#endif
    }
  puk_component_t strategy = nm_core_component_load("Strategy", strategy_name);
  int err = nm_core_set_strategy(nm_session.p_core, strategy);
  if(err != NM_ESUCCESS)
    {
      fprintf(stderr, "# session: error %d while loading strategy %s.\n", err, strategy_name);
      abort();
    }
}

/** add the given driver to the session */
static void nm_session_add_driver(puk_component_t component, int index)
{
  assert(component != NULL);
  const char*driver_url = NULL;
  struct nm_drv*p_drv = NULL;
  struct nm_driver_query_param param = { .key = NM_DRIVER_QUERY_BY_NOTHING };
  if(index >= 0)
    {
      param.key = NM_DRIVER_QUERY_BY_INDEX;
      param.value.index = index;
    }
  else
    {
      index = 0;
    }
  int err = nm_core_driver_load_init(nm_session.p_core, component, &param, &p_drv, &driver_url);
  if(err != NM_ESUCCESS)
    {
      fprintf(stderr, "# session: error %d while loading driver %s\n", err, p_drv->driver->name);
      abort();
    }
  const struct nm_drv_iface_s*drv_iface = puk_component_get_driver_NewMad_Driver(component, NULL);
  assert(drv_iface != NULL);
  const char*driver_realname = drv_iface->name;
  nm_session.n_drivers++;
  nm_session.drivers = realloc(nm_session.drivers, nm_session.n_drivers * sizeof(struct nm_session_driver_s));
  nm_session.drivers[nm_session.n_drivers - 1].p_drv = p_drv;
  nm_session.drivers[nm_session.n_drivers - 1].index = index;
  nm_session.drivers[nm_session.n_drivers - 1].name  = strdup(driver_realname);
  if(nm_session.url_string == NULL)
    {
      nm_session.url_string = padico_string_new();
    }
  else
    {
      padico_string_catf(nm_session.url_string, "+");
    }
  padico_string_catf(nm_session.url_string, "%s:%d#%s", driver_realname, index, driver_url);
}

/** Initialize the drivers */
static void nm_session_init_drivers(int*argc, char**argv)
{
  const char*assembly_name = getenv("NMAD_ASSEMBLY");
  const char*driver_env = getenv("NMAD_DRIVER");
  if(driver_env && (strlen(driver_env) == 0))
    driver_env = NULL;
  if((!driver_env) && (!assembly_name))
    {
      driver_env = "tcp";
    }

  nm_session.n_drivers = 0;
  nm_session.drivers = NULL;
  nm_session.url_string = NULL;

  if(driver_env)
    {
      /* parse driver_string */
      char*driver_string = strdup(driver_env); /* strtok writes into the string */
      char*token = strtok(driver_string, "+");
      while(token)
	{
	  char*driver_name = strdup(token); /* take a copy so as our writes don't confuse strtok */
	  char*index_string = strchr(driver_name, ':');
	  int index = -1;
	  if(index_string)
	    {
	      *index_string = '\0';
	      index_string++;
	      index = atoi(index_string);
	    }
	  if((strcmp(driver_name, "ib") == 0) || (strcmp(driver_name, "ibv") == 0))
	    {
	      free(driver_name);
	      driver_name = strdup("ibverbs");
	    }
	  else if((strcmp(driver_name, "myri") == 0) || (strcmp(driver_name, "myrinet") == 0))
	    {
	      free(driver_name);
	      driver_name = strdup("mx");
	    }
	  puk_component_t driver_assembly = nm_core_component_load("Driver", driver_name);
	  nm_session_add_driver(driver_assembly, index);
	  free(driver_name);
	  token = strtok(NULL, "+");
	}
      free(driver_string);
    }
  else
    {
      /* assembly name given (e.g. NewMadico) */
      NM_DISPF("# session: loading assembly %s\n", assembly_name);
      puk_component_t driver_assembly = puk_adapter_resolve(assembly_name);
      nm_session_add_driver(driver_assembly, -1);
    }
  /* load default driver */
  puk_component_t driver_self = puk_adapter_resolve("NewMad_Driver_self");
  nm_session_add_driver(driver_self, -1);
  /* get local url */
  nm_session.local_url = strdup(padico_string_get(nm_session.url_string));
  padico_string_delete(nm_session.url_string);
  nm_session.url_string = NULL;
}


/* ********************************************************* */
/* ** Session interface implementation */

int nm_session_create(nm_session_t*pp_session, const char*label)
{
  if(!nm_session.sessions)
    {
      nm_session.gates = puk_hashtable_new_string();
      nm_session.sessions = puk_hashtable_new(&nm_session_code_hash, &nm_session_code_eq);
    }
  assert(label != NULL);
  assert(pp_session != NULL);
  const int len = strlen(label);
  const uint32_t hash_code = puk_hash_oneatatime((void*)label, len);
  struct nm_session_s*p_session = puk_hashtable_lookup(nm_session.sessions, &hash_code);
  if(p_session != NULL)
    {
      fprintf(stderr, "# session: collision detected while creating session %s- a session with hashcode %d already exist (%s)\n",
	      label, hash_code, p_session->label);
      abort();
    }
#ifndef NM_TAGS_AS_INDIRECT_HASH
  if(puk_hashtable_size(nm_session.sessions) > 0)
    {
      fprintf(stderr, "# session: current flavor does not support multiple sessions. Please activate option 'tag_huge'.\n");
      abort();
    }
#endif /* NM_TAG_STRUCT */
  p_session = TBX_MALLOC(sizeof(struct nm_session_s));
  p_session->p_core = NULL;
  p_session->label = strdup(label);
  p_session->hash_code = hash_code;
  puk_hashtable_insert(nm_session.sessions, &p_session->hash_code, p_session);
  nm_session.ref_count++;
  *pp_session = p_session;
  return NM_ESUCCESS;
}

int nm_session_init(nm_session_t p_session, int*argc, char**argv, const char**p_local_url)
{
  assert(p_session != NULL);
  if(nm_session.p_core == NULL)
    {
      /* Initializes the global nmad core */
      int err = nm_core_init(argc, argv, &nm_session.p_core);
      if(err != NM_ESUCCESS)
	{
	  fprintf(stderr, "# session: error %d while initializing nmad core.\n", err);
	  abort();
	}
      
      /* load strategy */
      nm_session_init_strategy(argc, argv);
      
      /* load driver */
      nm_session_init_drivers(argc, argv);
    }
  p_session->p_core = nm_session.p_core;
  nm_sr_init(p_session);
  *p_local_url = nm_session.local_url;

  return NM_ESUCCESS;
}

int nm_session_connect(nm_session_t p_session, nm_gate_t*pp_gate, const char*url)
{
  assert(p_session != NULL);
  assert(nm_session.gates != NULL);
  nm_gate_t p_gate = puk_hashtable_lookup(nm_session.gates, url);
  char*remote_url = NULL;
  while(p_gate == NULL)
    {
      assert(nm_session.p_core != NULL);
      /* create gate */
      int err = nm_core_gate_init(nm_session.p_core, &p_gate);
      assert(err == NM_ESUCCESS);
      if(strcmp(url, nm_session.local_url) == 0)
	{
	  /* ** loopback connect- use driver 'self' (hardwired) */
	  int i;
	  for(i = 0; i < nm_session.n_drivers; i++)
	    {
	      if(strcmp(nm_session.drivers[i].name, "self") == 0)
		break;
	    }
	  assert(i < nm_session.n_drivers);
	  struct nm_drv*p_drv = nm_session.drivers[i].p_drv;
	  err = nm_core_gate_connect(nm_session.p_core, p_gate, p_drv, "-");
	  assert(err == NM_ESUCCESS);
	  remote_url = strdup(url);
	}
      else
	{
	  /* ** remote connection- use provided url */
	  /* parse remote url, store in hashtable (driver_name -> driver_url) */
	  puk_hashtable_t url_table = puk_hashtable_new_string();
	  char*parse_string = strdup(url);
	  char*token = strtok(parse_string, "+");
	  while(token != NULL)
	    {
	      char*driver_string = strdup(token);
	      char*driver_name = driver_string;
	      char*driver_url = strchr(driver_name, '#');
	      *driver_url = '\0';
	      driver_url++;
	      if(puk_hashtable_lookup(url_table, driver_name))
		{
		  fprintf(stderr, "# session: duplicate driver %s in url '%s'.\n", driver_name, url);
		  abort();
		}
	      puk_hashtable_insert(url_table, driver_name, driver_url);
	      token = strtok(NULL, "+");
	    }
	  free(parse_string);
	  /* connect gate */
	  const int is_server = (strcmp(url, nm_session.local_url) > 0);
	  /* connect all drivers */
	  int i;
	  int connected = 0;
	  for(i = 0; i < nm_session.n_drivers; i++)
	    {
	      const struct nm_session_driver_s*p_session_drv = &nm_session.drivers[i];
	      char driver_name[256];
	      snprintf(driver_name, 256, "%s:%d", p_session_drv->p_drv->driver->name, p_session_drv->index);
	      const char*driver_url = puk_hashtable_lookup(url_table, driver_name);
	      if(strcmp(p_session_drv->p_drv->driver->name, "self") == 0)
		continue;
	      if(driver_url == NULL)
		{
		  fprintf(stderr, "# session: peer node does not advertise any url for driver %s in url '%s'- skipping.\n",
			  driver_name, url);
		  continue;
		}
	      if(is_server)
		{
		  err = nm_core_gate_accept(nm_session.p_core, p_gate, p_session_drv->p_drv, driver_url);
		  if(err != NM_ESUCCESS)
		    {
		      fprintf(stderr, "# session: error %d while connecting driver %s\n", err, driver_name);
		      abort();
		    }
		}
	      else
		{
		  err = nm_core_gate_connect(nm_session.p_core, p_gate, p_session_drv->p_drv, driver_url);
		  if(err != NM_ESUCCESS)
		    {
		      fprintf(stderr, "# session: error %d while connecting driver %s\n", err, driver_name);
		      abort();
		    }
		}
	      connected++;
	    }
	  if(connected == 0)
	    {
	      fprintf(stderr, "# session: no common driver found for local and peer node. Abort.\n");
	      abort();
	    }
	  /* destroy the url hashtable */
	  puk_hashtable_enumerator_t e = puk_hashtable_enumerator_new(url_table);
	  char*url_entry = (char*)puk_hashtable_enumerator_next_key(e);
	  while(url_entry != NULL)
	    {
	      puk_hashtable_remove(url_table, url_entry);
	      free(url_entry);
	      url_entry = (char*)puk_hashtable_enumerator_next_key(e);
	    };
	  puk_hashtable_enumerator_delete(e);
	  puk_hashtable_delete(url_table);
	  url_table = NULL;
	  /* get peer identity */
	  nm_sr_request_t sreq1, sreq2, rreq1, rreq2;
	  nm_tag_t tag = 42;
	  int local_url_size = strlen(nm_session.local_url);
	  err = nm_sr_isend(p_session, p_gate, tag, &local_url_size, sizeof(local_url_size), &sreq1);
	  if(err != NM_ESUCCESS)
	    abort();
	  nm_sr_isend(p_session, p_gate, tag, nm_session.local_url, local_url_size, &sreq2);
	  int remote_url_size = -1;
	  nm_sr_irecv(p_session, p_gate, tag, &remote_url_size, sizeof(remote_url_size), &rreq1);
	  nm_sr_rwait(p_session, &rreq1);
	  remote_url = TBX_MALLOC(remote_url_size + 1);
	  nm_sr_irecv(p_session, p_gate, tag, remote_url, remote_url_size, &rreq2);
	  nm_sr_rwait(p_session, &rreq2);
	  nm_sr_swait(p_session, &sreq1);
	  nm_sr_swait(p_session, &sreq2);
	  remote_url[remote_url_size] = '\0';
	}
      /* insert new gate into hashtable */
      puk_hashtable_insert(nm_session.gates, remote_url, p_gate);
      /* lookup again, in case the new gate is not the expected one */
      p_gate = puk_hashtable_lookup(nm_session.gates, url);
    }
  *pp_gate = p_gate;
  return NM_ESUCCESS;
}

int nm_session_destroy(nm_session_t p_session)
{
  puk_hashtable_remove(nm_session.sessions, p_session);
  TBX_FREE((void*)p_session->label);
  TBX_FREE(p_session);
  nm_session.ref_count--;
  if(nm_session.ref_count == 0)
    {
      nm_core_exit(nm_session.p_core);
      nm_session.p_core = NULL;
      if(nm_session.drivers != NULL)
	{
	  int i;
	  for(i = 0; i < nm_session.n_drivers; i++)
	    {
	      free(nm_session.drivers[i].name);
	    }
	  free(nm_session.drivers);
	  nm_session.drivers = NULL;
	}
      nm_session.n_drivers = 0;
      free((void*)nm_session.local_url);
      nm_session.local_url = NULL;
      while(puk_hashtable_size(nm_session.gates) != 0)
	{
	  char*url = NULL;
	  puk_hashtable_getentry(nm_session.gates, &url, NULL);
	  puk_hashtable_remove(nm_session.gates, url);
	  TBX_FREE(url);
	}
      puk_hashtable_delete(nm_session.gates);
      puk_hashtable_delete(nm_session.sessions);
    }
  return NM_ESUCCESS;
}


