
#ifndef NETINTERF_EST_DEF
#define NETINTERF_EST_DEF

#include <sys/uio.h>
#include <mad_types.h>

typedef void (*network_init_func_t)(int *argc, char **argv, int nb_proc, int *tids, int *nb, int *whoami);

typedef void (*network_send_func_t)(int dest_node, struct iovec *vector, size_t count);

typedef void (*network_receive_func_t)(char **head);

typedef void (*network_receive_data_func_t)(struct iovec *vector, size_t count);

typedef void (*network_exit_func_t)(void);

typedef struct {
  network_init_func_t         network_init;
  network_send_func_t         network_send;
  network_receive_func_t      network_receive;
  network_receive_data_func_t network_receive_data;
  network_exit_func_t         network_exit;
} netinterf_t;

#endif



