
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
typedef int ntbx_host_id_t,    *p_ntbx_host_id_t;


/*
 * Pointers
 * --------
 */
typedef struct s_ntbx_host   *p_ntbx_host_t;
typedef struct s_ntbx_client *p_ntbx_client_t;
typedef struct s_ntbx_server *p_ntbx_server_t;


/*
 * Pack buffers
 * ------------
 */
#define NTBX_PACK_BUFFER_LEN     17
#define NTBX_PACK_BUFFER_TAG_LEN  4
typedef struct
{
  char buffer[NTBX_PACK_BUFFER_LEN];
} ntbx_pack_buffer_t, *p_ntbx_pack_buffer_t;


/*
 * Connection data
 * ---------------
 * (Attention aux types tableaux !!!)
 */
#define NTBX_CONNECTION_DATA_LEN 11
typedef struct
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

/*...State Constant.....................*/
typedef enum
{
  ntbx_client_state_uninitialized = 0,
  ntbx_client_state_initialized,
  ntbx_client_state_connected,
  ntbx_client_state_data_ready,
  ntbx_client_state_write_ready,
  ntbx_client_state_peer_closed,
  ntbx_client_state_shutdown,
  ntbx_client_state_closed,
} ntbx_client_state_t, *p_ntbx_client_state_t;

typedef enum
{
  ntbx_server_state_uninitialized = 0,
  ntbx_server_state_initialized,
  ntbx_server_state_accepting,
  ntbx_server_state_connection_ready,
  ntbx_server_state_shutdown,
  ntbx_server_state_closed,
} ntbx_server_state_t, *p_ntbx_server_state_t;


/*...Client/Server structures...........*/
typedef struct s_ntbx_client
{
  ntbx_client_state_t  state;
  char                *local_host;
  char                *remote_host;
  void                *specific;
} ntbx_client_t;

typedef struct s_ntbx_server
{
  ntbx_server_state_t     state;
  char                   *local_host;
  ntbx_connection_data_t  connection_data;
  void                   *specific;
} ntbx_server_t;

#endif /* NTBX_TYPES_H */
