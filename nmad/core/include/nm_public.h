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

#ifndef NM_PUBLIC_H
#define NM_PUBLIC_H

/* don't include pm2_common.h or tbx.h here. They are not needed and not ISO C compliant */
#include <stdlib.h>
#include <Padico/Puk.h>
#include <nm_errno.h>
#include <tbx_fast_list.h>

/* ** Core ************************************************* */

typedef struct nm_core*nm_core_t;

/* ** Gates ************************************************ */

typedef struct nm_gate*nm_gate_t;

#define NM_GATE_NONE ((nm_gate_t)NULL)

#define NM_ANY_GATE NM_GATE_NONE


/* ** Tags ************************************************* */

#if defined(CONFIG_TAG_AS_FLAT_ARRAY)
/* 8 bits, contained in flat array  */
#  define NM_TAGS_AS_FLAT_ARRAY
typedef uint8_t nm_tag_t;
#  define NM_ANY_TAG ((nm_tag_t)-1)
#  define NM_SO_MAX_TAGS (nm_tag_t)(255)
#elif defined(CONFIG_TAG_AS_HASHTABLE)
/* 32 bits, contained in hashtable */
#  define NM_TAGS_AS_HASHTABLE
typedef uint32_t nm_tag_t;
#  define NM_ANY_TAG ((nm_tag_t)-1)
#  define NM_SO_MAX_TAGS (nm_tag_t)(UINT32_MAX - 1ULL)
#elif defined(CONFIG_TAG_HUGE)
/* 64 bits, contained in hashtable */
#  define NM_TAGS_AS_INDIRECT_HASH
typedef uint64_t nm_tag_t; 
#  define NM_ANY_TAG ((nm_tag_t)-1)
#  define NM_SO_MAX_TAGS (nm_tag_t)(UINT64_MAX - 1ULL);
#else
#  error NewMad: no container type defined for tag-indexed tables
#endif

/* ** Drivers ********************************************** */

typedef struct nm_drv*nm_drv_t;


/* ** Polling ********************************************** */

#if(!defined(PIOMAN) || defined(PIOM_POLLING_DISABLED))
/* use nmad progression */
#define NMAD_POLL 1
#else
/* use pioman as a progression engine */
#define PIOMAN_POLL 1
#endif

/* ** Packs/unpacks **************************************** */

/* @note pack/unpack related types are supposedly private.
 * However, they are allocated by the sendrecv interface in
 * its sr_request_t type, thus the types must be visible by
 * the application.
 */

/** status of a pack/unpack request */
typedef uint16_t nm_so_status_t;

/** pack/unpack flags
 * @note for now, flags are included in status */
typedef nm_so_status_t nm_so_flag_t;

/** Sequence number */
typedef uint16_t nm_seq_t;

/** An unpack request */
struct nm_unpack_s
{
  nm_so_status_t status;
  void *data;
  int expected_len;
  int cumulated_len;
  nm_gate_t p_gate;
  nm_tag_t tag;
  nm_seq_t seq;
  struct tbx_fast_list_head _link;
};

/** A pack request */
struct nm_pack_s
{
  nm_so_status_t status;
  void*data; /**< actually, char*, struct iovec*, or DLOOP_Segment* depending on pack type (see status) */
  int len;   /**< cumulated data length */
  int done;
  nm_gate_t p_gate;
  nm_tag_t tag;
  nm_seq_t seq;
  struct tbx_fast_list_head _link;
};


/* ** Core functions *************************************** */

puk_component_t nm_core_component_load(const char*entity, 
				       const char*name);

int nm_core_init(int	   *argc,
		 char	   *argv[],
		 nm_core_t *pp_core);

int nm_core_exit(nm_core_t p_core);

int nm_core_driver_load(nm_core_t         p_core,
			puk_component_t driver,
			nm_drv_t*pp_drv);

int nm_core_driver_init(nm_core_t p_core,
			nm_drv_t p_drv,
			char	**p_url);

struct nm_driver_query_param
{
  enum {
    NM_DRIVER_QUERY_BY_NOTHING=0,
    NM_DRIVER_QUERY_BY_INDEX,
    /* NM_DRIVER_QUERY_BY_NUMA_NODE, */
    /* NM_DRIVER_QUERY_BY_SPEED, */
  } key;
  union {
    uint32_t index;
  } value;
};

int nm_core_driver_query(nm_core_t p_core,
			 nm_drv_t p_drv,
			 struct nm_driver_query_param *params,
			 int nparam);

int nm_core_driver_load_init_some_with_params(nm_core_t p_core,
					      int count,
					      puk_component_t*driver_array,
					      struct nm_driver_query_param **params_array,
					      int *nparam_array,
					      nm_drv_t *p_array,
					      char **p_url_array);

/** Simple helpers to prevent basic applications from having to
 * do load and init when they don't tweak anything in between.
 */
static inline int
nm_core_driver_load_init_some(nm_core_t p_core,
			      int count,
			      puk_component_t*driver_array,
			      nm_drv_t *p_drv_array,
			      char **p_url_array)
{
  struct nm_driver_query_param**params_array = malloc(count * sizeof(struct nm_driver_query_param*));
  int*nparam_array = malloc(count * sizeof(int));
  int i;
  for(i = 0; i < count; i++)
    {
      params_array[i] = NULL;
      nparam_array[i] = 0;
    }
  int err = nm_core_driver_load_init_some_with_params(p_core, count, driver_array,
						      params_array, nparam_array,
						      p_drv_array, p_url_array);
  free(params_array);
  free(nparam_array);
  return err;
}

static inline int
nm_core_driver_load_init_with_params(nm_core_t p_core,
				     puk_component_t driver,
				     struct nm_driver_query_param *params,
				     int nparam,
				     nm_drv_t *pp_drv,
				     char **p_url)
{
  struct nm_driver_query_param * params_array[1] = { params };
  int nparam_array[1] = { nparam };
  return nm_core_driver_load_init_some_with_params(p_core, 1, &driver,
						   params_array, nparam_array,
						   pp_drv, p_url);
}

static inline int
nm_core_driver_load_init(nm_core_t		 p_core,
			 puk_component_t          driver,
			 nm_drv_t		 *pp_drv,
			 char			**p_url)
{
  return nm_core_driver_load_init_with_params(p_core, driver, NULL, 0, pp_drv, p_url);
}


int nm_core_gate_init(nm_core_t p_core,
		      nm_gate_t *pp_gate);

int nm_core_gate_accept(nm_core_t  p_core,
			nm_gate_t  p_gate,
			nm_drv_t   p_drv,
			const char *drv_trk_url);

int nm_core_gate_connect(nm_core_t p_core,
			 nm_gate_t gate,
			 nm_drv_t  p_drv,
			 const char *drv_trk_url);

/** Get the user-registered per-gate data */
void*nm_gate_ref_get(struct nm_gate*p_gate);

/** Set the user-registered per-gate data */
void nm_gate_ref_set(struct nm_gate*p_gate, void*ref);


int nm_schedule(nm_core_t p_core);

#ifdef PIOMAN_POLL

int nm_core_disable_progression(struct nm_core*p_core);

int nm_core_enable_progression(struct nm_core*p_core);

#else /* PIOMAN_POLL */
static inline int nm_core_disable_progression(struct nm_core*p_core)
{
  return 0;
}
static inline int nm_core_enable_progression(struct nm_core*p_core)
{
  return 0;
}
#endif /* PIOMAN_POLL */


#endif /* NM_PUBLIC_H */


