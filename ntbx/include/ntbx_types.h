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
 * ntbx_types.h
 * ============
 */

#ifndef NTBX_TYPES_H
#define NTBX_TYPES_H


/*
 * Simple types
 * ------------
 */
typedef int ntbx_node_id_t,       *p_ntbx_node_id_t;
typedef int ntbx_process_lrank_t, *p_ntbx_process_lrank_t;
typedef int ntbx_process_grank_t, *p_ntbx_process_grank_t;

/* Begin: MadII compatibility */
typedef int ntbx_host_id_t, *p_ntbx_host_id_t;
/* End: MadII compatibility */


/*
 * Pointers
 * --------
 */
typedef struct s_ntbx_client            *p_ntbx_client_t;
typedef struct s_ntbx_server            *p_ntbx_server_t;
typedef struct s_ntbx_process_info      *p_ntbx_process_info_t;
typedef struct s_ntbx_process_container *p_ntbx_process_container_t;
typedef struct s_ntbx_process           *p_ntbx_process_t;
typedef struct s_ntbx_topology_element  *p_ntbx_topology_element_t;
typedef struct s_ntbx_topology_table    *p_ntbx_topology_table_t;


/*
 * Pack buffers
 * ------------
 */
#define NTBX_PACK_BUFFER_LEN     32
#define NTBX_PACK_INT_LEN        11
#define NTBX_PACK_UINT_LEN       10
#define NTBX_PACK_LONG_LEN       21
#define NTBX_PACK_ULONG_LEN      20
#define NTBX_PACK_MANTISSA_LEN   14
#define NTBX_PACK_BUFFER_TAG_LEN  4

typedef struct s_ntbx_pack_buffer
{
  char buffer[NTBX_PACK_BUFFER_LEN];
} ntbx_pack_buffer_t, *p_ntbx_pack_buffer_t;


/*
 * Connection data
 * ---------------
 * (Attention aux types tableaux !!!)
 */
#define NTBX_CONNECTION_DATA_LEN 11
typedef struct s_ntbx_connection_data
{
  char data[NTBX_CONNECTION_DATA_LEN];
} ntbx_connection_data_t, *p_ntbx_connection_data_t;


/*
 * Status
 * ------
 */
typedef enum
{
  ntbx_success =  0,
  ntbx_failure = -1,
} ntbx_status_t, *p_ntbx_status_t;


/*
 * Communication objects
 * ---------------------
 */

/*...Client/Server structures...........*/
typedef struct s_ntbx_client
{
  unsigned long        local_host_ip; /*  network form ! */
  char                *local_host;
  p_tbx_slist_t        local_alias;
  char                *remote_host;
  p_tbx_slist_t        remote_alias;
  tbx_bool_t           read_ok;
  tbx_bool_t           write_ok;
  size_t	       read_rq;
  tbx_bool_t           read_rq_flag;
  size_t               write_rq;
  tbx_bool_t           write_rq_flag;
  void                *specific;
} ntbx_client_t;

typedef struct s_ntbx_server
{
  unsigned long           local_host_ip; /*  network form ! */
  char                   *local_host;
  p_tbx_slist_t           local_alias;
  ntbx_connection_data_t  connection_data;
  void                   *specific;
} ntbx_server_t;

/*
 * Configuration objects
 * ---------------------
 */

/*  A process container element */
typedef struct s_ntbx_process_info
{
  ntbx_process_lrank_t local_rank;
  p_ntbx_process_t     process;
  void                *specific;
} ntbx_process_info_t;

/*  A process container */
typedef struct s_ntbx_process_container
{
  ntbx_process_lrank_t   local_array_size;
  p_ntbx_process_info_t *local_index;
  ntbx_process_grank_t   global_array_size;
  p_ntbx_process_info_t *global_index;
  int                    count;
} ntbx_process_container_t;

/*  A routing table */
/*...Topology Constants ...................*/
typedef enum
{
  ntbx_topology_empty   = 0,
  ntbx_topology_regular,
  ntbx_topology_full,
  ntbx_topology_star,
  ntbx_topology_ring,
  ntbx_topology_grid,
  ntbx_topology_torus,
  ntbx_topology_hypercube,
  ntbx_topology_function,
} ntbx_topology_t, *p_ntbx_topology_t;

typedef struct s_ntbx_topology_element
{
  void *specific;
} ntbx_topology_element_t;

typedef struct s_ntbx_topology_table
{
  p_ntbx_topology_element_t *table;
  ntbx_process_lrank_t       size;
} ntbx_topology_table_t;

/*  A processus */
typedef struct s_ntbx_process
{
  /*  Global internal id */
  ntbx_process_grank_t  global_rank;

  /*  Objects referencing that process */
  p_tbx_htable_t        ref;

  /*  Pid */
  pid_t                 pid;

  /*  Module-specific data */
  void                 *specific;

} ntbx_process_t;

#endif /* NTBX_TYPES_H */
