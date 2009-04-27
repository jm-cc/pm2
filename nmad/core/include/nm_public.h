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

/* don't include pm2_common.h r tbx.h here. They are not needed and not ISO C compliant */
#include <stdlib.h>
#include <Padico/Puk.h>
#include <nm_errno.h>

/* ** Core ************************************************* */

typedef struct nm_core*nm_core_t;

/* ** Gates ************************************************ */

typedef struct nm_gate*nm_gate_t;

#define NM_GATE_NONE ((nm_gate_t)NULL)

typedef nm_gate_t nm_gate_id_t;

#define NUMBER_OF_GATES          255

#define NM_ANY_GATE NM_GATE_NONE


/* ** Tags ************************************************* */

#if defined(CONFIG_TAG_AS_FLAT_ARRAY)
/* 8 bits, contained in flat array  */
#  define NM_TAGS_AS_FLAT_ARRAY
typedef uint8_t nm_tag_t;
#  define NM_SO_MAX_TAGS 128 /**< Maximum number of tags */
#elif defined(CONFIG_TAG_AS_HASHTABLE)
/* 32 bits, contained in hashtable */
#  define NM_TAGS_AS_HASHTABLE
typedef uint32_t nm_tag_t;
#  define NM_SO_MAX_TAGS (nm_tag_t)(UINT32_MAX - 128ULL) /**< Maximum number of tags  */
#elif defined(CONFIG_TAG_HUGE)
/* 64 bits, contained in hashtable */
#  define NM_TAGS_AS_HASHTABLE
#  define NM_TAGS_HUGE
typedef uint64_t nm_tag_t;
#  define NM_SO_MAX_TAGS (nm_tag_t)(UINT64_MAX - 128ULL);
#else
#  error NewMad: no container type defined for tag-indexed tables
#endif

#define NM_TAGS_PREALLOC 128

/* ** Drivers ********************************************** */

typedef uint16_t nm_drv_id_t;

/* Just for clarity of code (when using constant parameters) */
#define NM_DRV(n)  ((nm_drv_id_t)(n))

/** Maximum number of drivers.
 * @note There should *not* be a hard-coded value.
 */
#define NM_DRV_MAX 8

/** The default network to use when several networks are
 *   available, but the strategy does not support multi-rail
 */
#define NM_DRV_DEFAULT NM_DRV(0)

/* legacy */
#define NM_SO_DEFAULT_NET NM_DRV_DEFAULT


/* ** Core functions *************************************** */

puk_component_t nm_core_component_load(const char*entity, 
				       const char*name);

int nm_core_init(int	   *argc,
		 char	   *argv[],
		 nm_core_t *pp_core);

int nm_core_exit(nm_core_t p_core);

int nm_core_driver_load(nm_core_t         p_core,
			puk_component_t driver,
			nm_drv_id_t *p_id);

int nm_core_driver_init(nm_core_t p_core,
			nm_drv_id_t id,
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
			 nm_drv_id_t id,
			 struct nm_driver_query_param *params,
			 int nparam);

int nm_core_driver_load_init_some_with_params(nm_core_t p_core,
					      int count,
					      puk_component_t*driver_array,
					      struct nm_driver_query_param **params_array,
					      int *nparam_array,
					      nm_drv_id_t *p_id_array,
					      char **p_url_array);

/** Simple helpers to prevent basic applications from having to
 * do load and init when they don't tweak anything in between.
 */
static inline int
nm_core_driver_load_init_some(nm_core_t p_core,
			      int count,
			      puk_component_t*driver_array,
			      nm_drv_id_t *p_id_array,
			      char **p_url_array)
{
  struct nm_driver_query_param * params_array[NM_DRV_MAX] = { NULL };
  int nparam_array[NM_DRV_MAX] = { 0 };
  return nm_core_driver_load_init_some_with_params(p_core, count, driver_array,
						   params_array, nparam_array,
						   p_id_array, p_url_array);
}

static inline int
nm_core_driver_load_init_with_params(nm_core_t p_core,
				     puk_component_t driver,
				     struct nm_driver_query_param *params,
				     int nparam,
				     nm_drv_id_t *p_id,
				     char **p_url)
{
  struct nm_driver_query_param * params_array[1] = { params };
  int nparam_array[1] = { nparam };
  return nm_core_driver_load_init_some_with_params(p_core, 1, &driver,
						   params_array, nparam_array,
						   p_id, p_url);
}

static inline int
nm_core_driver_load_init(nm_core_t		 p_core,
			 puk_component_t          driver,
			 nm_drv_id_t		 *p_id,
			 char			**p_url)
{
  return nm_core_driver_load_init_with_params(p_core, driver, NULL, 0, p_id, p_url);
}

int nm_core_driver_exit	(nm_core_t p_core);


int nm_core_gate_init(nm_core_t p_core,
		      nm_gate_t *pp_gate);

int nm_core_gate_accept(nm_core_t  p_core,
			nm_gate_t  p_gate,
			nm_drv_id_t  drv_id,
			const char *drv_trk_url);

int nm_core_gate_connect(nm_core_t p_core,
			 nm_gate_t gate,
			 nm_drv_id_t drv_id,
			 const char *drv_trk_url);

/** Get the user-registered per-gate data */
void*nm_gate_ref_get(struct nm_gate*p_gate);

/** Set the user-registered per-gate data */
void nm_gate_ref_set(struct nm_gate*p_gate, void*ref);


int nm_schedule(nm_core_t p_core);

#ifdef PIOMAN
void nmad_lock(void);

void nmad_unlock(void);

int nm_piom_post_all(nm_core_t p_core);

#else /* PIOMAN */

static inline void 
nmad_lock(void)
{ }

static inline void 
nmad_unlock(void)
{ }


void nm_so_debug_init(int* argc, char** argv, int debug_flags);

#endif /* PIOMAN */

#endif /* NM_PUBLIC_H */


