/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 2.0

             Gabriel Antoniu, Luc Bouge, Christian Perez,
                Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 8512 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/


#include <stdio.h>
#include <malloc.h>
#include "dsm_page_manager.h"
#include "dsm_protocol_policy.h" 

#if 1
dsm_protocol_t 
dsmlib_hyp_java_prot = {NULL, 
			NULL, 
			NULL, 
			dsmlib_ws_hyp_send_page_for_write_access, 
			NULL,
			NULL, 
			dsmlib_erp_hyp_receive_page
};
#endif
#if 0
dsm_protocol_t 
dsmlib_hyp_java_prot = {NULL, 
			NULL, 
			NULL, 
			dsmlib_ws_hyp_send_page_for_write_access, 
			NULL,
			dsmlib_rp_hyp_validate_page
};
#endif

dsm_protocol_t 
dsmlib_ddm_li_hudak_prot = {dsmlib_rf_ask_for_read_copy,
			    dsmlib_wf_ask_for_write_access,
			    dsmlib_rs_send_read_copy,
			    dsmlib_ws_send_page_for_write_access, 
			    dsmlib_is_invalidate,
			    dsmlib_rp_validate_page,
			    NULL
};

dsm_protocol_t 
dsmlib_migrate_thread_prot = {dsmlib_migrate_thread,
			      dsmlib_migrate_thread,
			      NULL,
			      NULL, 
			      NULL,
			      NULL,
			      NULL
};

/* Global data structures for the protocol policy module */

static struct _protocol
{
  dsm_protocol_t *protocol_table; /* the protocol table: 
				     associates protocols to pages */
  unsigned long size;             /* size of the protocol table */
} dsm_protocol_info;


/* Public functions */

void dsm_init_protocol_table(dsm_protocol_t *protocol)
{
  int i;

  dsm_protocol_info.size = dsm_get_nb_static_pages();
  dsm_protocol_info.protocol_table = (dsm_protocol_t *)malloc(dsm_protocol_info.size * sizeof(dsm_protocol_t));

  for (i = 0; i < dsm_protocol_info.size; i++)
    dsm_protocol_info.protocol_table[i] = *protocol;
}


void dsm_set_page_protocol(unsigned long index, dsm_protocol_t *protocol)
{
/*    dsm_protocol_info.protocol_table[index].read_fault = (*protocol).read_fault; */
/*    dsm_protocol_info.protocol_table[index].write_fault = (*protocol).write_fault; */
  dsm_protocol_info.protocol_table[index] = *protocol;
}


dsm_protocol_t *dsm_get_page_protocol(unsigned long index)
{
  return &dsm_protocol_info.protocol_table[index];
}


void dsm_set_read_fault_action(unsigned long index, dsm_rf_action_t action)
{
  dsm_protocol_info.protocol_table[index].read_fault = action;
}


dsm_rf_action_t dsm_get_read_fault_action(unsigned long index)
{
  return dsm_protocol_info.protocol_table[index].read_fault;
}


void dsm_set_write_fault_action(unsigned long index, dsm_wf_action_t action)
{
  dsm_protocol_info.protocol_table[index].write_fault = action;
}


dsm_wf_action_t dsm_get_write_fault_action(unsigned long index)
{
  return dsm_protocol_info.protocol_table[index].write_fault;
}


void dsm_set_read_server(unsigned long index, dsm_rs_action_t action)
{
  dsm_protocol_info.protocol_table[index].read_server = action;
}


dsm_rs_action_t dsm_get_read_server(unsigned long index)
{
  return dsm_protocol_info.protocol_table[index].read_server;
}

void dsm_set_write_server(unsigned long index, dsm_ws_action_t action)
{
  dsm_protocol_info.protocol_table[index].write_server = action;
}


dsm_ws_action_t dsm_get_write_server(unsigned long index)
{
  return dsm_protocol_info.protocol_table[index].write_server;
}


void dsm_set_invalidate_server(unsigned long index, dsm_is_action_t action)
{
  dsm_protocol_info.protocol_table[index].invalidate_server = action;
}


dsm_is_action_t dsm_get_invalidate_server(unsigned long index)
{
  return dsm_protocol_info.protocol_table[index].invalidate_server;
}


void dsm_set_receive_page_server(unsigned long index, dsm_rp_action_t action)
{
  dsm_protocol_info.protocol_table[index].receive_page_server = action;
}


dsm_rp_action_t dsm_get_receive_page_server(unsigned long index)
{
  return dsm_protocol_info.protocol_table[index].receive_page_server;
}


void dsm_set_expert_receive_page_server(unsigned long index, dsm_erp_action_t action)
{
  dsm_protocol_info.protocol_table[index].expert_receive_page_server = action;
}


dsm_erp_action_t dsm_get_expert_receive_page_server(unsigned long index)
{
  return dsm_protocol_info.protocol_table[index].expert_receive_page_server;
}



