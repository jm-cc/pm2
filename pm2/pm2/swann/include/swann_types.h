
/*
 * swann_types.h
 * -------------
 */

#ifndef __SWANN_TYPES_H
#define __SWANN_TYPES_H

typedef enum
{
  swann_success =  0,
  swann_failure = -1,
} swann_status_t, *p_swann_status_t;

typedef int swann_net_client_id_t, *p_swann_net_client_id_t;
typedef int swann_net_server_id_t, *p_swann_net_server_id_t;

#endif /* __SWANN_TYPES_H */
