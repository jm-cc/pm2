
#include "pm2.h"

extern void DSM_LRPC_READ_PAGE_REQ_func(void);
extern void DSM_LRPC_WRITE_PAGE_REQ_func(void);
extern void DSM_LRPC_SEND_PAGE_func(void);
extern void DSM_LRPC_INVALIDATE_REQ_func(void);
extern void DSM_LRPC_INVALIDATE_ACK_func(void);
extern void DSM_LRPC_SEND_DIFFS_func(void);
extern void DSM_LRPC_LOCK_func(void);
extern void DSM_LRPC_UNLOCK_func(void);
extern void DSM_LRPC_MULTIPLE_READ_PAGE_REQ_func(void);
extern void DSM_LRPC_MULTIPLE_WRITE_PAGE_REQ_func(void);
extern void DSM_LRPC_SEND_MULTIPLE_PAGES_READ_func(void);
extern void DSM_LRPC_SEND_MULTIPLE_PAGES_WRITE_func(void);
extern void DSM_LRPC_SEND_MULTIPLE_DIFFS_func(void);
extern void DSM_LRPC_HBRC_DIFFS_func(void);

void dsm_pm2_init_rpc()
{
  pm2_rawrpc_register(&DSM_LRPC_READ_PAGE_REQ, DSM_LRPC_READ_PAGE_REQ_func);
  pm2_rawrpc_register(&DSM_LRPC_WRITE_PAGE_REQ, DSM_LRPC_WRITE_PAGE_REQ_func);
  pm2_rawrpc_register(&DSM_LRPC_SEND_PAGE, DSM_LRPC_SEND_PAGE_func);
  pm2_rawrpc_register(&DSM_LRPC_INVALIDATE_REQ, DSM_LRPC_INVALIDATE_REQ_func);
  pm2_rawrpc_register(&DSM_LRPC_INVALIDATE_ACK, DSM_LRPC_INVALIDATE_ACK_func);
  pm2_rawrpc_register(&DSM_LRPC_SEND_DIFFS, DSM_LRPC_SEND_DIFFS_func);
  pm2_rawrpc_register(&DSM_LRPC_LOCK, DSM_LRPC_LOCK_func);
  pm2_rawrpc_register(&DSM_LRPC_UNLOCK, DSM_LRPC_UNLOCK_func);
  pm2_rawrpc_register(&DSM_LRPC_MULTIPLE_READ_PAGE_REQ, DSM_LRPC_MULTIPLE_READ_PAGE_REQ_func);
  pm2_rawrpc_register(&DSM_LRPC_MULTIPLE_WRITE_PAGE_REQ, DSM_LRPC_MULTIPLE_WRITE_PAGE_REQ_func);
  pm2_rawrpc_register(&DSM_LRPC_SEND_MULTIPLE_PAGES_READ, DSM_LRPC_SEND_MULTIPLE_PAGES_READ_func);
  pm2_rawrpc_register(&DSM_LRPC_SEND_MULTIPLE_PAGES_WRITE, DSM_LRPC_SEND_MULTIPLE_PAGES_WRITE_func);
  pm2_rawrpc_register(&DSM_LRPC_SEND_MULTIPLE_DIFFS, DSM_LRPC_SEND_MULTIPLE_DIFFS_func);
  pm2_rawrpc_register(&DSM_LRPC_HBRC_DIFFS, DSM_LRPC_HBRC_DIFFS_func);
}


