
#ifndef DSM_CONST_IS_DEF
#define DSM_CONST_IS_DEF

//#define DEBUG1
//#define DEBUG2
//#define DEBUG3

typedef enum{NO_ACCESS, READ_ACCESS, WRITE_ACCESS, UNKNOWN_ACCESS} dsm_access_t;

typedef int dsm_node_t;

#define NO_NODE -1

typedef void (*dsm_rf_action_t)(unsigned long index); // read fault handler

typedef void (*dsm_wf_action_t)(unsigned long index); // write fault handler

typedef void (*dsm_rs_action_t)(unsigned long index, dsm_node_t req_node, int tag); // read server

typedef void (*dsm_ws_action_t)(unsigned long index, dsm_node_t req_node, int tag); // write server

typedef void (*dsm_is_action_t)(unsigned long index, dsm_node_t req_node, dsm_node_t new_owner); // invalidate server

typedef void (*dsm_rp_action_t)(void *addr, dsm_access_t access, dsm_node_t reply_node, int tag); // receive page server

typedef void (*dsm_erp_action_t)(void *addr, dsm_access_t access, dsm_node_t reply_node, unsigned long page_size, int tag); // expert receive page server

typedef void (*dsm_acq_action_t)(); // acquire func

typedef void (*dsm_rel_action_t)(); // release func

typedef void (*dsm_pi_action_t)(int prot_id); // protocol init func

typedef void (*dsm_pa_action_t)(unsigned long index); // page add func

#endif


