

#ifndef DSM_HBRC_MW_UPDATE_IS_DEF
#define DSM_HBRC_MW_UPDATE_IS_DEF

#include <assert.h>
#include "marcel.h" // tfprintf 
#include "pm2.h"
#include "dsm_const.h" 
#include "dsm_page_manager.h"
#include "dsm_comm.h"
#include "dsm_protocol_lib.h"
#include "dsm_protocol_policy.h"
#include "dsmlib_belist.h"
#include "dsm_page_size.h"
#include "dsm_mutex.h"


void dsmlib_hbrc_mw_update_init(int protocol_number);

void dsmlib_hbrc_mw_update_rfh(unsigned long index);

void dsmlib_hbrc_mw_update_wfh(unsigned long index);

void dsmlib_hbrc_mw_update_rs(unsigned long index, dsm_node_t req_node, int arg);

void dsmlib_hbrc_mw_update_ws(unsigned long index, dsm_node_t req_node, int arg);

void dsmlib_hbrc_mw_update_is(unsigned long index, dsm_node_t req_node, dsm_node_t new_owner);

void dsmlib_hbrc_mw_update_rps(void *addr, dsm_access_t access, dsm_node_t reply_node, int arg);

void dsmlib_hbrc_acquire();

void dsmlib_hbrc_release();

void dsmlib_hbrc_add_page(unsigned long index);

void dsmlib_hbrc_mw_update_prot_init(int prot);

void dsmlib_hrbc_send_diffs(unsigned long index, dsm_node_t dest_node);

void DSM_HRBC_DIFFS_threaded_func(void);

void DSM_LRPC_HBRC_DIFFS_func(void);

void dsmlib_hrbc_invalidate_copyset(unsigned long index, dsm_node_t req_node, dsm_node_t new_owner);
#endif










