
#ifndef DSM_PROT_LIB_IS_DEF
#define DSM_PROT_LIB_IS_DEF

#include "dsm_const.h" 

void dsmlib_rf_ask_for_read_copy(unsigned long index);

void dsmlib_migrate_thread(unsigned long index);

void dsmlib_wf_ask_for_write_access(unsigned long index);

void dsmlib_rs_send_read_copy(unsigned long index, dsm_node_t req_node, int tag);

void dsmlib_ws_send_page_for_write_access(unsigned long index, dsm_node_t req_node, int tag);

void dsmlib_is_invalidate(unsigned long index, dsm_node_t req_node, dsm_node_t new_owner);

void dsmlib_rp_validate_page(void *addr, dsm_access_t access, dsm_node_t reply_node, int tag);

#endif




