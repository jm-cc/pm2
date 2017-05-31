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
#include <Padico/Puk.h>

#include <string.h>
#include <assert.h>

#include <Padico/Module.h>
PADICO_MODULE_HOOK(NewMad_Core);

static struct
{
  struct nm_core*p_core;      /**< the current nmad object. */
  const char*local_url;       /**< the local nmad driver url */
  padico_string_t url_string; /**< local url as a string */
  puk_hashtable_t sessions;   /**< active sessions, hashed by session hashcode */
  int ref_count;              /**< ref. counter for core = number of active sessions */
  nm_session_selector_t selector;
  void*selector_arg;
  nm_drv_t p_drv_self;        /**< driver to connect to self */
} nm_session =
  {
    .p_core    = NULL,
    .local_url = NULL,
    .url_string = NULL,
    .sessions  = NULL,
    .ref_count = 0,
    .selector  = NULL,
    .p_drv_self = NULL
  };


/* ********************************************************* */

int nm_session_code_eq(const void*key1, const void*key2)
{
  const nm_session_hash_t*p_code1 = key1;
  const nm_session_hash_t*p_code2 = key2;
  return (*p_code1 == *p_code2);
}

uint32_t nm_session_code_hash(const void*key)
{
  const nm_session_hash_t*p_hash_code = key;
  return *p_hash_code;
}

/** Initialize the strategy */
static void nm_session_init_strategy(void)
{
  const char*strategy_name = getenv("NMAD_STRATEGY");
  if(!strategy_name)
    {
      strategy_name = "aggreg";
    }
  puk_component_t strategy = nm_core_component_load("Strategy", strategy_name);
  int err = nm_core_set_strategy(nm_session.p_core, strategy);
  if(err != NM_ESUCCESS)
    {
      NM_FATAL("# session: error %d while loading strategy '%s'.\n", err, strategy_name);
    }
}

/** add the given driver to the session */
#warning TODO- include index as a NewMad_Driver component attribute
nm_drv_t nm_session_add_driver(puk_component_t component, int index)
{
  assert(component != NULL);
  const char*driver_url = NULL;
  nm_drv_t p_drv = NULL;
  struct nm_driver_query_param param = { .key = NM_DRIVER_QUERY_BY_NOTHING };
  if(index >= 0)
    {
      param.key = NM_DRIVER_QUERY_BY_INDEX;
      param.value.index = index;
    }
  int err = nm_core_driver_load_init(nm_session.p_core, component, &param, &p_drv, &driver_url);
  if(err != NM_ESUCCESS)
    {
      NM_FATAL("# session: error %d while loading driver '%s'.\n", err, component->name);
    }
  if(nm_session.url_string == NULL)
    {
      nm_session.url_string = padico_string_new();
    }
  else
    {
      padico_string_catf(nm_session.url_string, "+");
    }
  padico_string_catf(nm_session.url_string, "%s#%d=%s", component->name, p_drv->index, driver_url);
  return p_drv;
}

/** Initialize default drivers */
static void nm_session_init_drivers(void)
{
  const char*driver_env = getenv("NMAD_DRIVER");
  if(driver_env && (strlen(driver_env) == 0))
    driver_env = NULL;
  if(!driver_env)
    {
      driver_env = "tcp";
    }

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

  /* load default driver */
  puk_component_t driver_self = nm_core_component_load("Driver", "self");
  nm_session.p_drv_self = nm_session_add_driver(driver_self, -1);
}


/* ********************************************************* */
/* ** Session interface implementation */

int nm_session_open(nm_session_t*pp_session, const char*label)
{
#ifndef NM_TAGS_AS_INDIRECT_HASH
  NM_FATAL("# session: nm_session_open()- current flavor does not support multiple sessions. Please activate configure option '--enable-taghuge'.\n");
  abort();
#endif /* NM_TAG_AS_INDIRECT_HASH */
  if(nm_session.sessions == NULL || nm_session.p_core == NULL)
    {
      NM_FATAL("# session: module not yet initialized. Cannot open new session.\n");
    }
  const int len = strlen(label);
  const uint32_t hash_code = puk_hash_oneatatime((void*)label, len);
  struct nm_session_s*p_session = puk_hashtable_lookup(nm_session.sessions, &hash_code);
  if(p_session != NULL)
    {
      if(strcmp(label, p_session->label) != 0)
	{
	  NM_FATAL("# session: hash collision detected (%s / %s).\n", label, p_session->label);
	}
      *pp_session = NULL;
      return -NM_EALREADY;
    }
  p_session = malloc(sizeof(struct nm_session_s));
  p_session->label = strdup(label);
  p_session->hash_code = hash_code;
  p_session->p_core = nm_session.p_core;
  p_session->p_sr_session = NULL;
  p_session->p_sr_destructor = NULL;
  puk_hashtable_insert(nm_session.sessions, &p_session->hash_code, p_session);
  nm_session.ref_count++;
  *pp_session = p_session;
  return NM_ESUCCESS;
}

int nm_session_create(nm_session_t*pp_session, const char*label)
{
  if(!nm_session.sessions)
    {
      nm_session.sessions = puk_hashtable_new(&nm_session_code_hash, &nm_session_code_eq);
    }
  assert(label != NULL);
  assert(pp_session != NULL);
  const int len = strlen(label);
  const uint32_t hash_code = puk_hash_oneatatime((void*)label, len);
  struct nm_session_s*p_session = puk_hashtable_lookup(nm_session.sessions, &hash_code);
  if(p_session != NULL)
    {
      NM_FATAL("# session: collision detected while creating session %s- a session with hashcode %d already exist (%s)\n",
	      label, hash_code, p_session->label);
    }
#ifndef NM_TAGS_AS_INDIRECT_HASH
  if(puk_hashtable_size(nm_session.sessions) > 0)
    {
      NM_FATAL("# session: current flavor does not support multiple sessions. Please activate configure option '--enable-taghuge'.\n");
    }
#endif /* NM_TAG_AS_INDIRECT_HASH */
  p_session = malloc(sizeof(struct nm_session_s));
  p_session->p_core = NULL;
  p_session->label = strdup(label);
  p_session->hash_code = hash_code;
  p_session->p_sr_session = NULL;
  p_session->p_sr_destructor = NULL;
  puk_hashtable_insert(nm_session.sessions, &p_session->hash_code, p_session);
  if(nm_session.p_core == NULL)
    {
      /* Initializes the global nmad core */
      int fake_argc = 1;
      char*fake_argv[2] = { "nm_session", NULL };
      int err = nm_core_init(&fake_argc, fake_argv, &nm_session.p_core);
      if(err != NM_ESUCCESS)
	{
	  NM_FATAL("# session: error %d while initializing nmad core.\n", err);
	}
    }
  p_session->p_core = nm_session.p_core;
  nm_session.ref_count++;
  *pp_session = p_session;
  return NM_ESUCCESS;
}

int nm_session_init(nm_session_t p_session, int*argc, char**argv, const char**p_local_url)
{
  assert(p_session != NULL);
      
  /* load strategy */
  nm_session_init_strategy();

  if(nm_session.p_core->nb_drivers == 0)
    {
      /* load driver */
      nm_session_init_drivers();
    }
  if(nm_session.local_url == NULL)
    nm_session.local_url = padico_strdup(padico_string_get(nm_session.url_string));
  *p_local_url = nm_session.local_url;
  return NM_ESUCCESS;
}

void nm_session_set_selector(nm_session_selector_t selector, void*_arg)
{
  nm_session.selector = selector;
  nm_session.selector_arg = _arg;
}

/** default selector to choose drivers */
static nm_drv_vect_t nm_session_default_selector(const char*peer_url, void*_arg)
{
  nm_drv_vect_t v = nm_drv_vect_new();
  if(strcmp(peer_url, nm_session.local_url) == 0)
    {
      /* ** loopback connect- use driver 'self' (hardwired) */
      nm_drv_t p_drv = nm_session.p_drv_self;
      nm_drv_vect_push_back(v, p_drv);
    }
  else
    {
      /* ** remote connection- find common drivers */
      nm_drv_t p_drv;
      NM_FOR_EACH_DRIVER(p_drv, nm_session.p_core)
	{
	  char driver_name[256];
	  snprintf(driver_name, 256, "%s#%d", p_drv->assembly->name, p_drv->index);
	  if(p_drv == nm_session.p_drv_self)
	    {
	      continue;
	    }
	  else if(strstr(peer_url, driver_name) != NULL)
	    {
	      nm_drv_vect_push_back(v, p_drv);
	    }
	  else
	    {
	      NM_WARN("# session: peer node does not advertise any url for driver %s in url '%s'- skipping.\n",
		      driver_name, peer_url);
	      continue;
	    }
	}
    }
  return v;
}

int nm_session_connect(nm_session_t p_session, nm_gate_t*pp_gate, const char*url)
{
  assert(p_session != NULL);
  if(nm_session.selector == NULL)
    nm_session.selector = &nm_session_default_selector;
  nm_drv_vect_t v = (*nm_session.selector)(url, nm_session.selector_arg);
  if(nm_drv_vect_empty(v))
    {
      NM_FATAL("# session: no common driver found for local (%s) and peer (%s) node. Abort.\n",
	       nm_session.local_url, url);
   }
  assert(nm_session.p_core != NULL);
  /* create gate */
  nm_gate_t p_gate = NULL;
  int err = nm_core_gate_init(nm_session.p_core, &p_gate);
  assert(err == NM_ESUCCESS);
  
  /* parse remote url, store in hashtable (driver_name -> driver_url) */
  puk_hashtable_t url_table = puk_hashtable_new_string();
  char*parse_string = strdup(url);
  char*token = strtok(parse_string, "+");
  while(token != NULL)
    {
      char*driver_string = strdup(token);
      char*driver_name = driver_string;
      char*driver_url = strchr(driver_name, '=');
      *driver_url = '\0';
      driver_url++;
      if(puk_hashtable_lookup(url_table, driver_name))
	{
	  NM_FATAL("# session: duplicate driver '%s' in url '%s'.\n", driver_name, url);
	}
      puk_hashtable_insert(url_table, driver_name, driver_url);
      token = strtok(NULL, "+");
    }
  free(parse_string);
  /* connect all drivers */
  nm_drv_vect_itor_t i;
  puk_vect_foreach(i, nm_drv, v)
    {
      nm_drv_t p_drv = *i;
      char driver_name[256];
      snprintf(driver_name, 256, "%s#%d", p_drv->assembly->name, p_drv->index);
      const char*driver_url = puk_hashtable_lookup(url_table, driver_name);
      assert(driver_url != NULL);
      err = nm_core_gate_connect(nm_session.p_core, p_gate, p_drv, driver_url);
      if(err != NM_ESUCCESS)
	{
	  NM_FATAL("# session: error %d while connecting driver '%s'.\n", err, driver_name);
	}
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

  nm_drv_vect_delete(v);
  *pp_gate = p_gate;
  return NM_ESUCCESS;
}

int nm_session_destroy(nm_session_t p_session)
{
  uint32_t hashcode = p_session->hash_code;
  if(puk_hashtable_lookup(nm_session.sessions, &hashcode) == NULL)
    {
      NM_FATAL("session: cannot find session hash=%d; p_session=%p in session base- cannot destroy.\n",
	       hashcode, p_session);
    }
  if(p_session->p_sr_destructor)
    {
      (*p_session->p_sr_destructor)(p_session);
    }
  puk_hashtable_remove(nm_session.sessions, &p_session->hash_code);
  free((void*)p_session->label);
  free(p_session);
  nm_session.ref_count--;
  if(nm_session.ref_count == 0)
    {
      nm_core_exit(nm_session.p_core);
      nm_session.p_core = NULL;
      free((void*)nm_session.local_url);
      nm_session.local_url = NULL;
      puk_hashtable_delete(nm_session.sessions);
    }
  return NM_ESUCCESS;
}

nm_session_t nm_session_lookup(nm_session_hash_t hashcode)
{
  struct nm_session_s*p_session = puk_hashtable_lookup(nm_session.sessions, &hashcode);
  return p_session;
}


