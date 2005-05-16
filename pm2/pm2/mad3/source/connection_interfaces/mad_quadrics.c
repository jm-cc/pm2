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
 * Mad_quadrics.c
 * ==============
 */


#include "madeleine.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <elan/elan.h>
#include <elan/capability.h>

/*
 * macros
 * ------
 */
#ifdef MARCEL
#define MAD_QUADRICS_POLLING_MODE \
    (MARCEL_EV_POLL_AT_TIMER_SIG | MARCEL_EV_POLL_AT_YIELD | MARCEL_EV_POLL_AT_IDLE)
#endif /* MARCEL */

/*
 * local structures
 * ----------------
 */
typedef struct s_mad_quadrics_driver_specific
{
        int		 nb_processes;
        int		 nb_lranks;
        int		 nb_nodes;
        int		 node_id;
        int		 node_id_min;
        int		 node_id_max;
        int		*node_ids;
        int		 nb_ctxs;
        int		 ctx_id;
        int		 ctx_id_max;
        int		*ctx_ids;
        char		*capability_str;
        ELAN_BASE	*base;
        u_int		 proc;
        u_int		 nproc;
} mad_quadrics_driver_specific_t, *p_mad_quadrics_driver_specific_t;

typedef struct s_mad_quadrics_adapter_specific
{
        int dummy;
} mad_quadrics_adapter_specific_t, *p_mad_quadrics_adapter_specific_t;

typedef struct s_mad_quadrics_channel_specific
{
        ELAN_TPORT		*port;
        ELAN_QUEUE		*queue;
        char			*first_packet;
        char			*sys_buffer;
        int			 first_packet_length;
        ntbx_process_lrank_t	*lranks;
} mad_quadrics_channel_specific_t, *p_mad_quadrics_channel_specific_t;

typedef struct s_mad_quadrics_connection_specific
{
        int		 remote_node_id;
        int		 remote_ctx_id;
        u_int		 remote_proc;
        tbx_bool_t	 first_incoming_packet_flag;
        tbx_bool_t	 first_outgoing_packet_flag;
        int		 nb_packets;
 } mad_quadrics_connection_specific_t, *p_mad_quadrics_connection_specific_t;

typedef struct s_mad_quadrics_link_specific
{
        int dummy;
} mad_quadrics_link_specific_t, *p_mad_quadrics_link_specific_t;

#ifdef MARCEL
typedef enum e_mad_quadrics_req_type {
        mad_quadrics_req_unknown = 0,
        mad_quadrics_req_Tx,
        mad_quadrics_req_Rx
} mad_quarics_req_type_t, *p_mad_quarics_req_type_t;

typedef struct s_mad_quadrics_ev {
	struct marcel_ev_req	 inst;
        mad_quarics_req_type_t   type;
        ELAN_EVENT		*event;
} mad_quadrics_ev_t, *p_mad_quadrics_ev_t;
#endif /* MARCEL */

#ifdef MARCEL
static struct marcel_ev_server mad_quadrics_ev_server = MARCEL_EV_SERVER_INIT(mad_quadrics_ev_server, "Mad/QUADRICS I/O");
#endif /* MARCEL */


#define MAD_QUADRICS_CONTEXT_ID_OFFSET	2
#define MAD_QUADRICS_MAX_ASYNC_PACKETS	1000
#define FIRST_PACKET_THRESHOLD		32768
/* #define USE_RX_PROBE */

/*
 * Malloc protection hooks
 * -----------------------
 */

/* Prototypes */

#ifdef MARCEL
static
void *
mad_quadrics_malloc_hook(size_t len, const void *caller);

static
void *
mad_quadrics_memalign_hook(size_t alignment, size_t len, const void *caller);

static
void
mad_quadrics_free_hook(void *ptr, const void *caller);

static
void *
mad_quadrics_realloc_hook(void *ptr, size_t len, const void *caller);

static
void
mad_quadrics_malloc_initialize_hook(void);


/* Previous handlers */
static
void *
(*mad_quadrics_old_malloc_hook)(size_t len, const void *caller) = NULL;

static
void *
(*mad_quadrics_old_memalign_hook)(size_t alignment, size_t len, const void *caller) = NULL;

static
void
(*mad_quadrics_old_free_hook)(void *PTR, const void *CALLER) = NULL;

static
void *
(*mad_quadrics_old_realloc_hook)(void *PTR, size_t LEN, const void *CALLER) = NULL;

#endif /* MARCEL */

#if 0
/*
 * Malloc hooks installation is delayed to mad_quadrics_driver_init because
 * __malloc_initialize_hook is already defined by the QUADRICS library.
 *
 * This is expected to be safe because at the time mad_quadrics_driver_init is called,
 * no polling request should already be pending.
 */

/* Entry point */
void (*__malloc_initialize_hook) (void) = mad_quadrics_malloc_initialize_hook;
#endif /* 0 */

#ifdef MARCEL
/* Flag to prevent multiple hooking */
static
int mad_quadrics_malloc_hooked = 0;
#endif /* MARCEL */




/*
 * static functions
 * ----------------
 */
#if 1
static
inline
void
__mad_quadrics_hook_lock(void) {
#ifdef MARCEL
        marcel_ev_lock(&mad_quadrics_ev_server);
        //DISP("<quadrics hook LOCK>");
#endif /* MARCEL */
}

static
inline
void
__mad_quadrics_hook_unlock(void) {
#ifdef MARCEL
        //DISP("<quadrics hook UNLOCK>");
        marcel_ev_unlock(&mad_quadrics_ev_server);
#endif /* MARCEL */
}

#ifdef MARCEL
static
void
mad_quadrics_install_hooks(void) {
        LOG_IN();
        mad_quadrics_old_malloc_hook		= __malloc_hook;
        mad_quadrics_old_memalign_hook	= __memalign_hook;
        mad_quadrics_old_free_hook		= __free_hook;
        mad_quadrics_old_realloc_hook		= __realloc_hook;

        if (__malloc_hook == mad_quadrics_malloc_hook)
                FAILURE("hooks corrupted");

        if (__memalign_hook == mad_quadrics_memalign_hook)
                FAILURE("hooks corrupted");

        if (__realloc_hook == mad_quadrics_realloc_hook)
                FAILURE("hooks corrupted");

        if (__free_hook == mad_quadrics_free_hook)
                FAILURE("hooks corrupted");

        __malloc_hook		= mad_quadrics_malloc_hook;
        __memalign_hook		= mad_quadrics_memalign_hook;
        __free_hook		= mad_quadrics_free_hook;
        __realloc_hook		= mad_quadrics_realloc_hook;
        LOG_OUT();
}

static
void
mad_quadrics_remove_hooks(void) {
        LOG_IN();
        if (__malloc_hook == mad_quadrics_old_malloc_hook)
                FAILURE("hooks corrupted");

        if (__memalign_hook == mad_quadrics_old_memalign_hook)
                FAILURE("hooks corrupted");

        if (__realloc_hook == mad_quadrics_old_realloc_hook)
                FAILURE("hooks corrupted");

        if (__free_hook == mad_quadrics_old_free_hook)
                FAILURE("hooks corrupted");

        __malloc_hook		= mad_quadrics_old_malloc_hook;
        __memalign_hook		= mad_quadrics_old_memalign_hook;
        __free_hook		= mad_quadrics_old_free_hook;
        __realloc_hook		= mad_quadrics_old_realloc_hook;
        LOG_OUT();
}


static
void *
mad_quadrics_malloc_hook(size_t len, const void *caller) {
        void *new_ptr = NULL;

        __mad_quadrics_hook_lock();
        mad_quadrics_remove_hooks();
        LOG_IN();
        new_ptr = malloc(len);
        LOG_OUT();
        mad_quadrics_install_hooks();
        __mad_quadrics_hook_unlock();

        return new_ptr;
}


static
void *
mad_quadrics_memalign_hook(size_t alignment, size_t len, const void *caller) {
        void *new_ptr = NULL;

        __mad_quadrics_hook_lock();
        mad_quadrics_remove_hooks();
        LOG_IN();
        new_ptr = memalign(alignment, len);
        LOG_OUT();
        mad_quadrics_install_hooks();
        __mad_quadrics_hook_unlock();

        return new_ptr;
}


static
void *
mad_quadrics_realloc_hook(void *ptr, size_t len, const void *caller) {
        void *new_ptr = NULL;

        __mad_quadrics_hook_lock();
        mad_quadrics_remove_hooks();
        LOG_IN();
        new_ptr = realloc(ptr, len);
        LOG_OUT();
        mad_quadrics_install_hooks();
        __mad_quadrics_hook_unlock();

        return new_ptr;
}


static
void
mad_quadrics_free_hook(void *ptr, const void *caller) {

        __mad_quadrics_hook_lock();
        mad_quadrics_remove_hooks();
        LOG_IN();
        free(ptr);
        LOG_OUT();
        mad_quadrics_install_hooks();
        __mad_quadrics_hook_unlock();
}


static
void
mad_quadrics_malloc_initialize_hook(void) {
        mad_quadrics_malloc_hooked = 1;
        mad_quadrics_install_hooks();
}
#endif /* MARCEL */

static
inline
void
mad_quadrics_lock(void) {
#ifdef MARCEL
       __mad_quadrics_hook_lock();
        mad_quadrics_remove_hooks();
        //DISP("<quadrics LOCK>");
#endif /* MARCEL */
}

static
inline
void
mad_quadrics_unlock(void) {
#ifdef MARCEL
        //DISP("<quadrics UNLOCK>");
        mad_quadrics_install_hooks();
        __mad_quadrics_hook_unlock();
#endif /* MARCEL */
}
#else
static
inline
void
mad_quadrics_lock(void) {
#ifdef MARCEL
        marcel_ev_lock(&mad_quadrics_ev_server);
        //DISP("<quadrics LOCK>");
#endif /* MARCEL */
}

static
inline
void
mad_quadrics_unlock(void) {
#ifdef MARCEL
        //DISP("<quadrics UNLOCK>");
        marcel_ev_unlock(&mad_quadrics_ev_server);
#endif /* MARCEL */
}
#endif

#ifdef MARCEL
static
int
mad_quadrics_do_poll(marcel_ev_server_t	server,
               marcel_ev_op_t		_op,
               marcel_ev_req_t		req,
               int			nb_ev,
               int			option) {
        p_mad_quadrics_ev_t	p_ev	= NULL;

        LOG_IN();
	p_ev = struct_up(req, mad_quadrics_ev_t, inst);

        switch (p_ev->type) {
                case mad_quadrics_req_Tx: {
                        if (elan_tportTxDone(p_ev->event)) {
                                MARCEL_EV_REQ_SUCCESS(&(p_ev->inst));
                        }

                        break;
                }

                case mad_quadrics_req_Rx: {
                        if (elan_tportRxDone(p_ev->event)) {
                                MARCEL_EV_REQ_SUCCESS(&(p_ev->inst));
                        }

                        break;
                }

                default :
                        FAILURE("unknown request type");

        }

        LOG_OUT();

        return 0;
}
#endif /* MARCEL */

static void
mad_quadrics_blocking_tx_test(ELAN_EVENT *event){

        LOG_IN();
#ifdef MARCEL
        {
                struct marcel_ev_wait	ev_w;
                mad_quadrics_ev_t 	ev	= {
                        .event 	=  event,
                        .type	= mad_quadrics_req_Tx
                };

                marcel_ev_wait(&mad_quadrics_ev_server, &(ev.inst), &ev_w, 0);
        }
#else /* MARCEL */
#if 0
        while (!elan_tportTxDone(event)) ;
#endif
#endif /* MARCEL */
        mad_quadrics_lock();
        elan_tportTxWait(event);
        mad_quadrics_unlock();
        LOG_OUT();
}

static
void *
mad_quadrics_blocking_rx_test(ELAN_EVENT *event,
                              int        *p_sender,
                              size_t     *p_size){
        void *ptr = NULL;

        LOG_IN();
#ifdef MARCEL
        {
                struct marcel_ev_wait	ev_w;
                mad_quadrics_ev_t 	ev	= {
                        .event 	=  event,
                        .type	= mad_quadrics_req_Rx
                };

                marcel_ev_wait(&mad_quadrics_ev_server, &(ev.inst), &ev_w, 0);
        }
#else /* MARCEL */
#if 0
        while (!elan_tportRxDone(event)) ;
#endif
#endif /* MARCEL */

        mad_quadrics_lock();
        ptr = elan_tportRxWait(event, p_sender, NULL, p_size);
        mad_quadrics_unlock();
        LOG_OUT();

        return ptr;
}

/*
 * Registration function
 * ---------------------
 */


char *
mad_quadrics_register(p_mad_driver_interface_t interface) {
        LOG_IN();
        TRACE("Registering QUADRICS driver");
        interface->driver_init                = mad_quadrics_driver_init;
        interface->adapter_init               = mad_quadrics_adapter_init;
        interface->channel_init               = mad_quadrics_channel_init;
        interface->before_open_channel        = mad_quadrics_before_open_channel;
        interface->connection_init            = mad_quadrics_connection_init;
        interface->link_init                  = mad_quadrics_link_init;
        interface->accept                     = mad_quadrics_accept;
        interface->connect                    = mad_quadrics_connect;
        interface->after_open_channel         = NULL;
        interface->before_close_channel       = NULL;
        interface->disconnect                 = mad_quadrics_disconnect;
        interface->after_close_channel        = NULL;
        interface->link_exit                  = NULL;
        interface->connection_exit            = mad_quadrics_connection_exit;
        interface->channel_exit               = mad_quadrics_channel_exit;
        interface->adapter_exit               = mad_quadrics_adapter_exit;
        interface->driver_exit                = mad_quadrics_driver_exit;
        interface->choice                     = NULL;
        interface->get_static_buffer          = NULL;
        interface->return_static_buffer       = NULL;
        interface->new_message                = mad_quadrics_new_message;
        interface->finalize_message           = NULL;
#ifdef MAD_MESSAGE_POLLING
        interface->poll_message               = mad_quadrics_poll_message;
#endif // MAD_MESSAGE_POLLING
        interface->receive_message            = mad_quadrics_receive_message;
        interface->message_received           = NULL;
        interface->send_buffer                = mad_quadrics_send_buffer;
        interface->receive_buffer             = mad_quadrics_receive_buffer;
        interface->send_buffer_group          = mad_quadrics_send_buffer_group;
        interface->receive_sub_buffer_group   = mad_quadrics_receive_sub_buffer_group;
        LOG_OUT();

        return "quadrics";
}


void
mad_quadrics_driver_init(p_mad_driver_t d) {
        p_mad_dir_driver_t			 dd		= NULL;
        p_mad_quadrics_driver_specific_t	 ds		= NULL;
        ntbx_process_lrank_t        		 process_lrank	=   -1;
        ntbx_process_lrank_t        		 i_lrank	=   -1;
        p_ntbx_process_container_t		 dpc		= NULL;
        int					 nb_processes	=    0;
        int					 nb_lranks	=    0;
        int					 nb_nodes	=    0;
        int					 node_id	=   -1;
        int					 node_id_min	=   -1;
        int					 node_id_max	=   -1;
        int					*node_ids	= NULL;
        int					 nb_ctxs	=    0;
        int					 ctx_id		=   -1;
        int					 ctx_id_max	=   -1;
        int					*ctx_ids	= NULL;
        int					*next_node_ctx  = NULL;
        char                                    *capability_str	= NULL;
        ELAN_BASE				*base		= NULL;
        u_int					 proc 		=    0;
        u_int					 nproc		=    0;

        LOG_IN();
        TRACE("Initializing QUADRICS driver");
#ifdef MARCEL
        if (!mad_quadrics_malloc_hooked) {
                mad_quadrics_malloc_initialize_hook();
        }
#endif /* MARCEL */
        d->connection_type  = mad_bidirectional_connection;
        d->buffer_alignment = 32;

        ds  		= TBX_MALLOC(sizeof(mad_quadrics_driver_specific_t));
#ifdef MARCEL
        marcel_ev_server_set_poll_settings(&mad_quadrics_ev_server, MAD_QUADRICS_POLLING_MODE, 1);
        marcel_ev_server_add_callback(&mad_quadrics_ev_server, MARCEL_EV_FUNCTYPE_POLL_POLLONE, mad_quadrics_do_poll);
#endif /* MARCEL */
        d->specific	= ds;


        dd		= d->dir_driver;
        dpc		= dd->pc;
        process_lrank	= d->process_lrank;

        nb_lranks	= ntbx_pc_local_max(dpc)+1;

        if (nb_lranks < 1)
                FAILURE("invalid state");

        node_ids	= TBX_CALLOC(nb_lranks, sizeof(int));
        ctx_ids		= TBX_CALLOC(nb_lranks, sizeof(int));

        /* First pass: get the Quadrics node id of each process */
        TRACE("Quadrics driver - node ids processing: first pass");
        if (!ntbx_pc_first_local_rank(dpc, &i_lrank))
                FAILURE("invalid state");

        do {
                p_mad_dir_driver_process_specific_t	dps 		= NULL;
                ntbx_process_lrank_t        		i_grank		=   -1;
                int					i_node_id	=   -1;

                dps	= ntbx_pc_get_local_specific(dpc, i_lrank);
                i_grank	= ntbx_pc_local_to_global(dpc, i_lrank);

                i_node_id	= (int)tbx_cstr_to_long(dps->parameter);
                if (i_node_id < 0)
                        FAILURE("invalid Quadrics node id");

                nb_processes++;
                TRACE("Quadrics driver - process %d: node id = %d", i_grank, i_node_id);

                node_ids[i_lrank] = i_node_id;

                if (i_lrank == process_lrank) {
                        node_id	= i_node_id;
                }

                if (node_id_min == -1  ||  i_node_id < node_id_min) {
                        node_id_min = i_node_id;
                }

                if (node_id_max == -1  ||  i_node_id > node_id_max) {
                        node_id_max = i_node_id;
                }

        } while (ntbx_pc_next_local_rank(dpc, &i_lrank));

        nb_nodes	= node_id_max - node_id_min + 1;

        /* Second pass: compute the context id of each process */
        TRACE("Quadrics driver - node ids processing: second pass");
        next_node_ctx = TBX_MALLOC(nb_nodes * sizeof(int));
        {
                int i = 0;

                for (i = 0; i < nb_nodes; i++) {
                        next_node_ctx[i] = MAD_QUADRICS_CONTEXT_ID_OFFSET;
                }
        }

        if (!ntbx_pc_first_local_rank(dpc, &i_lrank))
                FAILURE("invalid state");

        do {
                ntbx_process_lrank_t	i_grank		= -1;
                int			i_node_id	= -1;
                int			i_ctx_id	= -1;

                i_grank	= ntbx_pc_local_to_global(dpc, i_lrank);

                i_node_id	= node_ids[i_lrank];
                i_ctx_id	= next_node_ctx[i_node_id - node_id_min]++;

                TRACE("Quadrics driver - process %d: context id = %d", i_grank, i_ctx_id);
                ctx_ids[i_lrank]	= i_ctx_id;

                if (i_lrank == process_lrank) {
                        ctx_id	= i_ctx_id;
                }

                if (ctx_id_max == -1  ||  i_ctx_id > ctx_id_max) {
                        ctx_id_max = i_ctx_id;
                        nb_ctxs++;
                }

        } while (ntbx_pc_next_local_rank(dpc, &i_lrank));

        TRACE_VAL("Quadrics driver - number of processes", nb_processes);
        TRACE_VAL("Quadrics driver - number of nodes", nb_nodes);
        TRACE_VAL("Quadrics driver - number of contexts per nodes", nb_ctxs);
        TRACE_VAL("Quadrics driver - nb_nodes * nb_ctxs", nb_nodes * nb_ctxs);

        if (nb_nodes * nb_ctxs != nb_processes)
                FAILURE("invalid configuration for Quadrics");

        TBX_FREE(next_node_ctx);
        next_node_ctx	= NULL;

        /* Capability string computation */
        {
                char	*str = NULL;
                int 	 alloc_size = 16;
                int      size = 0;

                str = TBX_MALLOC(alloc_size);
                size = snprintf(str, alloc_size, "N%dC%d-%d-%dN%d-%dR1b",
                                node_id,
                                MAD_QUADRICS_CONTEXT_ID_OFFSET,
                                ctx_id,
                                ctx_id_max,
                                node_id_min,
                                node_id_max);

                if (size >= alloc_size) {
                        alloc_size = size+1;
                        str	= TBX_REALLOC(str, alloc_size);
                        size	= snprintf(str, alloc_size, "N%dC%d-%d-%dN%d-%dR1b",
                                           node_id,
                                           MAD_QUADRICS_CONTEXT_ID_OFFSET,
                                           ctx_id,
                                           ctx_id_max,
                                           node_id_min,
                                           node_id_max);
                }

                TRACE_STR("Quadrics driver - capability string", str);

                capability_str = str;
        }

        /* Quadrics initialization */
        mad_quadrics_lock();
        if (elan_generateCapability (capability_str) < 0)
                ERROR("elan_generateCapability");
        mad_quadrics_unlock();

        if (!(base = elan_baseInit(0)))
                ERROR("elan_baseInit");


        proc	= base->state->vp;
        TRACE_VAL("Quadrics driver - proc", proc);

        nproc	= base->state->nvp;
        TRACE_VAL("Quadrics driver - nproc", nproc);

         /* driver specific structure initialization */
        ds->nb_processes	= nb_processes  ;
        ds->nb_lranks		= nb_lranks     ;
        ds->nb_nodes		= nb_nodes      ;
        ds->node_id		= node_id       ;
        ds->node_id_min		= node_id_min   ;
        ds->node_id_max		= node_id_max   ;
        ds->node_ids		= node_ids      ;
        ds->nb_ctxs		= nb_ctxs       ;
        ds->ctx_id		= ctx_id        ;
        ds->ctx_id_max		= ctx_id_max    ;
        ds->ctx_ids		= ctx_ids       ;
        ds->capability_str	= capability_str;
        ds->base		= base;
        ds->proc		= proc;
        ds->nproc		= nproc;

#ifdef MARCEL
	marcel_ev_server_start(&mad_quadrics_ev_server);
#endif /* MARCEL */
        LOG_OUT();
}

void
mad_quadrics_adapter_init(p_mad_adapter_t a) {
        p_mad_quadrics_adapter_specific_t	as	= NULL;

        LOG_IN();
        as		= TBX_MALLOC(sizeof(mad_quadrics_adapter_specific_t));

        if (strcmp(a->dir_adapter->name, "default")) {
                FAILURE("unsupported adapter");
        }

        // unimplemented

        a->specific	= as;
        a->parameter	= tbx_strdup("-");
        LOG_OUT();
}

void
mad_quadrics_channel_init(p_mad_channel_t ch) {
        p_mad_quadrics_driver_specific_t 	 ds 	= NULL;
        p_mad_quadrics_channel_specific_t	 chs	= NULL;
        p_tbx_string_t				 param_str	= NULL;

        LOG_IN();
        ds =	ch->adapter->driver->specific;
        chs	= TBX_MALLOC(sizeof(mad_quadrics_channel_specific_t));

        chs->queue			= NULL;
        chs->port			= NULL;
        chs->first_packet		= TBX_MALLOC(FIRST_PACKET_THRESHOLD);
        chs->sys_buffer			= NULL;
        chs->first_packet_length	= 0;
        chs->lranks			= TBX_CALLOC(ds->nproc, sizeof(ntbx_process_lrank_t));
        ch->specific	= chs;

        param_str	= tbx_string_init_to_uint(ds->proc);
        ch->parameter	= tbx_string_to_cstring(param_str);
        tbx_string_free(param_str);
        param_str	= NULL;
        LOG_OUT();
}

void
mad_quadrics_before_open_channel(p_mad_channel_t ch) {
        p_mad_quadrics_driver_specific_t 	 ds 	= NULL;
        p_mad_quadrics_channel_specific_t	 chs	= NULL;
        ELAN_BASE				*base	= NULL;
        ELAN_TPORT				*p	= NULL;		/* Tagged port handle */
        ELAN_QUEUE				*q	= NULL;		/* Queue handle */

        LOG_IN();
        ds 	= ch->adapter->driver->specific;
        chs	= ch->specific;

        base	= ds->base;

        mad_quadrics_lock();
        if (!(q = elan_gallocQueue(base, base->allGroup)))
                ERROR("elan_gallocQueue");

        if (!(p = elan_tportInit(base->state,
                                 q,
                                 base->tport_nslots,
                                 base->tport_smallmsg, base->tport_bigmsg,
                                 base->tport_stripemsg,
                                 base->waitType, base->retryCount,
                                 &base->shm_key, base->shm_fifodepth,
                                 base->shm_fragsize, 0)))
                ERROR("elan_tportInit");

        mad_quadrics_unlock();
        chs->queue			= q;
        chs->port			= p;
        LOG_OUT();
}

void
mad_quadrics_connection_init(p_mad_connection_t in,
                             p_mad_connection_t out) {

        p_mad_quadrics_connection_specific_t	cs	= NULL;
        p_mad_quadrics_driver_specific_t	ds	= NULL;

        LOG_IN();
        ds		= in->channel->adapter->driver->specific;
        cs		= TBX_MALLOC(sizeof(mad_quadrics_connection_specific_t));
        cs->remote_node_id		= ds->node_ids[in->remote_rank];
        cs->remote_ctx_id		= ds->ctx_ids [in->remote_rank];
        cs->first_incoming_packet_flag	= tbx_false;
        cs->first_outgoing_packet_flag	= tbx_false;
        cs->nb_packets			= 0;

        in->specific	= cs;
        in->nb_link	= 1;

        out->specific	= cs;
        out->nb_link	= 1;
        LOG_OUT();
}

void
mad_quadrics_link_init(p_mad_link_t lnk) {
        LOG_IN();
        // unimplemented
        lnk->link_mode   = mad_link_mode_buffer_group;
        lnk->buffer_mode = mad_buffer_mode_dynamic;
        lnk->group_mode  = mad_group_mode_split;
        LOG_OUT();
}

static
void
mad_quadrics_connect_accept(p_mad_connection_t   cnx,
                            p_mad_adapter_info_t ai) {
        p_mad_quadrics_connection_specific_t	 cs		= NULL;
        p_mad_quadrics_channel_specific_t	 chs		= NULL;
        u_int				 remote_proc	=    0;

        LOG_IN();
        cs	= cnx->specific;
        chs	= cnx->channel->specific;

        remote_proc			=
                tbx_cstr_to_unsigned_long(ai->channel_parameter);
        cs->remote_proc 		= remote_proc;
        chs->lranks[remote_proc]	= cnx->remote_rank;
        LOG_OUT();
}


void
mad_quadrics_accept(p_mad_connection_t   in,
                    p_mad_adapter_info_t ai) {
        LOG_IN();
        mad_quadrics_connect_accept(in, ai);
        LOG_OUT();
}

void
mad_quadrics_connect(p_mad_connection_t   out,
                     p_mad_adapter_info_t ai) {
        LOG_IN();
        mad_quadrics_connect_accept(out, ai);
        LOG_OUT();
}

void
mad_quadrics_disconnect(p_mad_connection_t c) {
        p_mad_quadrics_connection_specific_t cs = NULL;

        LOG_IN();
        cs = c->specific;
        // unimplemented
        LOG_OUT();
}

void
mad_quadrics_connection_exit(p_mad_connection_t in,
                p_mad_connection_t out) {
        p_mad_quadrics_connection_specific_t cs = NULL;

        LOG_IN();
        cs = in->specific;
        // unimplemented
        TBX_FREE(cs);
        in->specific	= NULL;
        out->specific	= NULL;
        LOG_OUT();
}

void
mad_quadrics_channel_exit(p_mad_channel_t ch) {
        p_mad_quadrics_channel_specific_t	chs		= NULL;

        LOG_IN();
        chs = ch->specific;
        // unimplemented
        TBX_FREE(chs);
        ch->specific	= NULL;
        LOG_OUT();
}

void
mad_quadrics_adapter_exit(p_mad_adapter_t a) {
        p_mad_quadrics_adapter_specific_t as = NULL;

        LOG_IN();
        as		= a->specific;
        TBX_FREE(as);
        a->specific	= NULL;
        a->parameter	= NULL;
        LOG_OUT();
}

void
mad_quadrics_driver_exit(p_mad_driver_t d) {
        p_mad_quadrics_driver_specific_t	ds		= NULL;

        LOG_IN();
        TRACE("Finalizing QUADRICS driver");
        ds	= d->specific;
        // unimplemented
        TBX_FREE(ds);
        d->specific	= NULL;
        LOG_OUT();
}

void
mad_quadrics_new_message(p_mad_connection_t out){
        p_mad_quadrics_connection_specific_t	os	= NULL;

        LOG_IN();
        os	= out->specific;
#ifndef USE_RX_PROBE
        os->first_outgoing_packet_flag	= tbx_true;
#endif /* USE_RX_PROBE */
        LOG_OUT();
}

p_mad_connection_t
mad_quadrics_receive_message(p_mad_channel_t ch) {
        p_mad_quadrics_channel_specific_t	 chs		= NULL;
        p_mad_connection_t			 in		= NULL;
        p_mad_quadrics_connection_specific_t	 is		= NULL;
        p_tbx_darray_t				 in_darray	= NULL;
        ELAN_EVENT				*event		= NULL;
        int                                      sender		=    0;
        size_t					 size		=    0;
        ntbx_process_lrank_t			 remote_lrank	=   -1;

        LOG_IN();
        chs		= ch->specific;
        in_darray	= ch->in_connection_darray;

#ifdef USE_RX_PROBE
#if 1
        /* 8us min latency */

        mad_quadrics_lock();
        event = elan_tportRxStart(chs->port	/* port 	*/,
                                  ELAN_TPORT_RXANY|ELAN_TPORT_RXPROBE	/* flags	*/,
                                  0		/* sender mask	*/,
                                  0		/* sender sel	*/,
                                  0		/* tag mask	*/,
                                  0		/* tag sel	*/,
                                  NULL	/* base	*/,
                                  0	/* size	 */);
        mad_quadrics_unlock();
        mad_quadrics_blocking_rx_test(event, &sender, &size);
#else
        /* 11.6 us min latency */

#ifdef MARCEL
#error invalid conditional compilation directives set
#endif
        while (!elan_tportRxPoll(chs->port	/* port 	*/,
                                 0		/* sender mask	*/,
                                 0		/* sender sel	*/,
                                 0		/* tag mask	*/,
                                 0		/* tag sel	*/,
                                 &sender	/* sender	*/,
                                 &tag		/* tag	 	*/,
                                 &size		/* size		*/))
                ;
#endif
#else /* USE_RX_PROBE */
        /* 3.7 us min latency */

        mad_quadrics_lock();
        event = elan_tportRxStart(chs->port	/* port 	*/,
                                  ELAN_TPORT_RXANY /*|ELAN_TPORT_RXBUF*/	/* flags	*/,
                                  0		/* sender mask	*/,
                                  0		/* sender sel	*/,
                                  0		/* tag mask	*/,
                                  0		/* tag sel	*/,
                                  chs->first_packet	/* base	*/,
                                  FIRST_PACKET_THRESHOLD	/* size	 */);
        mad_quadrics_unlock();
        chs->sys_buffer = mad_quadrics_blocking_rx_test(event, &sender, &size);

        chs->first_packet_length = size;
#endif /* USE_RX_PROBE */
        remote_lrank = chs->lranks[sender];

        in	= tbx_darray_get(in_darray, remote_lrank);
        is	= in->specific;

        is-> first_incoming_packet_flag		= tbx_true;
        LOG_OUT();

        return in;
}

void
mad_quadrics_send_buffer(p_mad_link_t     lnk,
                         p_mad_buffer_t   b) {
        p_mad_connection_t			 out		= NULL;
        p_mad_quadrics_connection_specific_t	 os		= NULL;
        p_mad_quadrics_channel_specific_t	 chs		= NULL;
        p_mad_quadrics_driver_specific_t	 ds		= NULL;
        size_t					 length		=    0;
        ELAN_EVENT				*event		= NULL;
        ELAN_FLAGS				 tx_flags	=    0;

        LOG_IN();
        out	= lnk->connection;
        os	= out->specific;
        chs	= out->channel->specific;
        ds	= out->channel->adapter->driver->specific;


#ifndef USE_RX_PROBE
        if (os->first_outgoing_packet_flag) {
                os->first_outgoing_packet_flag = tbx_false;

                length	= tbx_min(b->bytes_written - b->bytes_read,
                                  FIRST_PACKET_THRESHOLD);

                //DISP_VAL("sb: first length", length);
                tx_flags = 0;
                os->nb_packets++;

                if (os->nb_packets > MAD_QUADRICS_MAX_ASYNC_PACKETS) {
                        os->nb_packets	 = 0;
                        tx_flags	|= ELAN_TPORT_TXSYNC;
                }

                mad_quadrics_lock();
                event	= elan_tportTxStart(chs->port, tx_flags, os->remote_proc, ds->proc, 0, b->buffer + b->bytes_read, length);

                mad_quadrics_unlock();
                mad_quadrics_blocking_tx_test(event);
                b->bytes_read += length;

                if (!mad_more_data(b))
                        goto no_more_data;
        }
#endif /* USE_RX_PROBE */

        length = b->bytes_written - b->bytes_read;
        //DISP_VAL("sb: length", length);

        tx_flags = 0;
        os->nb_packets++;

        if (os->nb_packets > MAD_QUADRICS_MAX_ASYNC_PACKETS) {
                os->nb_packets	 = 0;
                tx_flags	|= ELAN_TPORT_TXSYNC;
        }

        mad_quadrics_lock();
        event	= elan_tportTxStart(chs->port, tx_flags, os->remote_proc, ds->proc, 0, b->buffer + b->bytes_read, length);
        mad_quadrics_unlock();
        mad_quadrics_blocking_tx_test(event);

        b->bytes_read += length;

#ifndef USE_RX_PROBE
 no_more_data:
        ;
        //DISP("sb: ok");
#endif /* USE_RX_PROBE */

        LOG_OUT();
}

void
mad_quadrics_receive_buffer(p_mad_link_t    lnk,
                      p_mad_buffer_t *_buffer) {
        p_mad_buffer_t				 b	  	= *_buffer;
        p_mad_connection_t			 in		=  NULL;
        p_mad_quadrics_connection_specific_t	 is		=  NULL;
        p_mad_channel_t				 ch		=  NULL;
        p_mad_quadrics_channel_specific_t	 chs		=  NULL;
        void 					*data_ptr	=  NULL;
        size_t	 				 data_length	=     0;
        ELAN_EVENT				*event		=  NULL;
        int                                      sender		=     0;
        size_t					 size		=     0;

        LOG_IN();
        in	= lnk->connection;
        is	= in->specific;
        ch	= in->channel;
        chs	= ch->specific;

#ifndef USE_RX_PROBE
        if (is->first_incoming_packet_flag) {
                is->first_incoming_packet_flag = tbx_false;

                data_ptr	= b->buffer + b->bytes_written;
                data_length	= tbx_min(b->length - b->bytes_written, FIRST_PACKET_THRESHOLD);

                if (chs->first_packet_length != data_length)
                        FAILURE("invalid first packet length");

                if (chs->sys_buffer == chs->first_packet) {
                        memcpy(data_ptr, chs->first_packet, data_length);
                } else {
                        memcpy(data_ptr, chs->sys_buffer, data_length);
                        mad_quadrics_lock();
                        elan_tportBufFree(chs->port, chs->sys_buffer);
                        mad_quadrics_unlock();
                }

                //DISP_VAL("rb: first length", data_length);

                chs->sys_buffer 	 = NULL;

                b->bytes_written	+= data_length;

                if (mad_buffer_full(b))
                        goto no_more_data;
        }
#endif /* USE_RX_PROBE */

        data_ptr	= b->buffer + b->bytes_written;
        data_length	= b->length - b->bytes_written;

        mad_quadrics_lock();
        event = elan_tportRxStart(chs->port		/* port 	*/,
                                  0			/* flags	*/,
                                  -1			/* sender mask	*/,
                                  is->remote_proc	/* sender sel	*/,
                                  0			/* tag mask	*/,
                                  0			/* tag sel	*/,
                                  data_ptr		/* base		*/,
                                  data_length		/* size		*/);
        mad_quadrics_unlock();
        mad_quadrics_blocking_rx_test(event, &sender, &size);

        if (size != data_length)
                FAILUREF("invalid packet length: expected %zu bytes, got %zu bytes", data_length, size);

        //DISP_VAL("rb: length", data_length);

        b->bytes_written	+= data_length;

#ifndef USE_RX_PROBE
 no_more_data:
        ;
#endif /* USE_RX_PROBE */
        LOG_OUT();
}

void
mad_quadrics_send_buffer_group_1(p_mad_link_t         lnk,
                           p_mad_buffer_group_t buffer_group) {
        LOG_IN();
        if (!tbx_empty_list(&(buffer_group->buffer_list))) {
                tbx_list_reference_t ref;

                tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
                do {
                        p_mad_buffer_t b = NULL;

                        b = tbx_get_list_reference_object(&ref);
                        mad_quadrics_send_buffer(lnk, b);
                } while(tbx_forward_list_reference(&ref));
        }
        LOG_OUT();
}

void
mad_quadrics_receive_sub_buffer_group_1(p_mad_link_t         lnk,
                                  p_mad_buffer_group_t buffer_group) {
        LOG_IN();
        if (!tbx_empty_list(&(buffer_group->buffer_list))) {
                tbx_list_reference_t ref;

                tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
                do {
                        p_mad_buffer_t b = NULL;

                        b = tbx_get_list_reference_object(&ref);
                        mad_quadrics_receive_buffer(lnk, &b);
                } while(tbx_forward_list_reference(&ref));
        }
        LOG_OUT();
}

void
mad_quadrics_send_buffer_group(p_mad_link_t         lnk,
                         p_mad_buffer_group_t buffer_group) {
        LOG_IN();
        mad_quadrics_send_buffer_group_1(lnk, buffer_group);
        LOG_OUT();
}

void
mad_quadrics_receive_sub_buffer_group(p_mad_link_t         lnk,
                                tbx_bool_t           first_sub_group
                                __attribute__ ((unused)),
                                p_mad_buffer_group_t buffer_group) {
        LOG_IN();
        mad_quadrics_receive_sub_buffer_group_1(lnk, buffer_group);
        LOG_OUT();
}
