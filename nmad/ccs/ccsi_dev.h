/*
 *  (C) 2006 by Argonne National Laboratory.
 *   Contact:  Darius Buntinas
 */ 
#ifndef CCSI_DEV_H
#define CCSI_DEV_H

#include "ccsi.h"
#include "ccsi_dev.h"
#include <stdarg.h>

int CCSI_dev_init ();
int CCSI_dev_finalize ();
int CCSI_dev_amrequest (int context_id, CCS_node_t node, void *buf, unsigned count, CCS_datadesc_t datadesc,
			 unsigned handler_id, unsigned num_args, va_list ap);
int CCSI_dev_amreply (struct CCS_token *token, void *buf, unsigned count, CCS_datadesc_t datadesc,
		       unsigned handler_id, unsigned num_args, va_list ap);
void CCSI_dev_poll ();

int CCSI_dev_register_mem (void *addr, unsigned len);
int CCSI_dev_deregister_mem (void *addr, unsigned len);

int CCSI_dev_put (CCS_node_t node, void *src_buf, unsigned src_count, CCS_datadesc_t src_datadesc, 
		   void *dest_buf, unsigned dest_count, CCS_datadesc_t dest_datadesc, CCS_callback_t callback, void *callback_arg);
int CCSI_dev_get (CCS_node_t node, void *src_buf, unsigned src_count, CCS_datadesc_t src_datadesc, 
		   void *dest_buf, unsigned dest_count, CCS_datadesc_t dest_datadesc, CCS_callback_t callback, void *callback_arg);

#endif
