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
 * Mad_gm.c
 * ========
 */


#include "madeleine.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gm.h>

/*
DEBUG_DECLARE(gm)
#undef DEBUG_NAME
#define DEBUG_NAME gm
*/

/*
 *  Macros
 */
#if 0
#define __RDTSC__ ({jlong time;__asm__ __volatile__ ("rdtsc" : "=A" (time));time;})
#else
#define __RDTSC__ 0LL
#endif

/* Debugging macros */
#if 0
#define __trace__(s, p...) fprintf(stderr, "[%Ld]:%s:%d: "s"\n", __RDTSC__, __FUNCTION__, __LINE__ , ## p)
#else
#define __trace__(s, p...)
#endif

#if 0
#define __disp__(s, p...) fprintf(stderr, "[%Ld]:%s:%d: "s"\n", __RDTSC__, __FUNCTION__, __LINE__ , ## p)
#define __in__()          fprintf(stderr, "[%Ld]:%s:%d: -->\n", __RDTSC__, __FUNCTION__, __LINE__)
#define __out__()         fprintf(stderr, "[%Ld]:%s:%d: <--\n", __RDTSC__, __FUNCTION__, __LINE__)
#define __err__()         fprintf(stderr, "[%Ld]:%s:%d: <!!\n", __RDTSC__, __FUNCTION__, __LINE__)
#else
#define __disp__(s, p...)
#define __in__() 
#define __out__()
#define __err__()
#endif

/* Error message macros */
#if 1
#define __error__(s, p...) fprintf(stderr, "[%Ld]:%s:%d: *error* "s"\n", __RDTSC__, __FUNCTION__, __LINE__ , ## p)
#else
#define __error__(s, p...)
#endif

#define __gmerror__(x) (mad_gm_error((x), __LINE__))

#define MAD_GM_MAX_BLOCK_LEN	 	(2*1024*1024)
#define MAD_GM_CACHE_GRANULARITY	 0x1000
#define MAD_GM_CACHE_SIZE		 0x1000

typedef struct s_mad_gm_cache *p_mad_gm_cache_t;
typedef struct s_mad_gm_cache {
        void             *ptr;
        int               len;
        int               ref_count;
        p_mad_gm_cache_t  next;
} mad_gm_cache_t;

typedef struct s_mad_gm_port {
        int dummy;

	struct gm_port   *p_gm_port;
        int               number;
        int               local_port_id;
        unsigned int      local_node_id;
        p_mad_gm_cache_t  cache_head;
        p_mad_adapter_t   adapter;
        p_tbx_darray_t    out_darray;
        p_tbx_darray_t    in_darray;
} mad_gm_port_t, *p_mad_gm_port_t;

typedef struct s_mad_gm_driver_specific {
        int dummy;
} mad_gm_driver_specific_t, *p_mad_gm_driver_specific_t;

typedef struct s_mad_gm_adapter_specific {
        int            dummy;
        int            device_id;
        p_tbx_darray_t port_darray;
} mad_gm_adapter_specific_t, *p_mad_gm_adapter_specific_t;

typedef struct s_mad_gm_channel_specific {
        int dummy;
        p_mad_gm_port_t in_port;
} mad_gm_channel_specific_t, *p_mad_gm_channel_specific_t;

typedef struct s_mad_gm_connection_specific {
        int dummy;

        volatile int    lock;
        gm_status_t     request_status;
        p_mad_buffer_t  buffer;
        int             remote_port_id;
        unsigned int    remote_node_id;
        p_mad_gm_port_t port;

} mad_gm_connection_specific_t, *p_mad_gm_connection_specific_t;

typedef struct s_mad_gm_link_specific {
        int dummy;
} mad_gm_link_specific_t, *p_mad_gm_link_specific_t;

/*
 *  Global variables
 * __________________
 */

const int mad_gm_pub_port_array[] = {
        2,
        4,
        5,
        6,
        7,
        -1
};

/*
 *  Functions
 * ___________
 */

/* GM error message display fonction. */
static
void
mad_gm_error(gm_status_t gm_status, int line)
{
	char *msg = NULL;

	switch (gm_status) {
	case GM_SUCCESS:
		break;

        case GM_FAILURE:
                msg = "GM failure";
                break;
                
        case GM_INPUT_BUFFER_TOO_SMALL:
                msg = "GM input buffer too small";
                break;
                        
        case GM_OUTPUT_BUFFER_TOO_SMALL:
                msg = "GM output buffer too small";
                break;
                
        case GM_TRY_AGAIN:
                msg = "GM try again";
                break;

        case GM_BUSY:
                msg = "GM busy";
                break;
                
        case GM_MEMORY_FAULT:
                msg = "GM memory fault";
                break;

        case GM_INTERRUPTED:
                msg = "GM interrupted";
                break;
                
        case GM_INVALID_PARAMETER:
                msg = "GM invalid parameter";
                break;
                
        case GM_OUT_OF_MEMORY:
                msg = "GM out of memory";
                break;
                
        case GM_INVALID_COMMAND:
                msg = "GM invalid command";
                break;

        case GM_PERMISSION_DENIED:
                msg = "GM permission denied";
                break;

        case GM_INTERNAL_ERROR:
                msg = "GM internal error";
                break;

        case GM_UNATTACHED:
                msg = "GM unattached";
                break;

        case GM_UNSUPPORTED_DEVICE:
                msg = "GM unsupported device";
                break;

        case GM_SEND_TIMED_OUT:
		msg = "GM send timed out";
                break;

        case GM_SEND_REJECTED:
		msg = "GM send rejected";
                break;

        case GM_SEND_TARGET_PORT_CLOSED:
		msg = "GM send target port closed";
                break;

        case GM_SEND_TARGET_NODE_UNREACHABLE:
		msg = "GM send target node unreachable";
                break;

        case GM_SEND_DROPPED:
		msg = "GM send dropped";
                break;

        case GM_SEND_PORT_CLOSED:
		msg = "GM send port closed";
                break;

        case GM_NODE_ID_NOT_YET_SET:
                msg = "GM id not yet set";
                break;

        case GM_STILL_SHUTTING_DOWN:
                msg = "GM still shutting down";
                break;

        case GM_CLONE_BUSY:
                msg = "GM clone busy";
                break;

        case GM_NO_SUCH_DEVICE:
                msg = "GM no such device";
                break;

        case GM_ABORTED:
                msg = "GM aborted";
                break;

#if GM_API_VERSION >= GM_API_VERSION_1_5
        case GM_INCOMPATIBLE_LIB_AND_DRIVER:
                msg = "GM incompatible lib and driver";
                break;

        case GM_UNTRANSLATED_SYSTEM_ERROR:
                msg = "GM untranslated system error";
                break;

        case GM_ACCESS_DENIED:
                msg = "GM access denied";
                break;
#endif

	default:
		msg = "unknown GM error";
		break;
	}

	if (msg) {
		DISP("%d:%s\n", line, msg);
                gm_perror ("gm_message", gm_status);
	}
}


/* --- */

static
void
mad_gm_round2page(void **p_ptr,
                  int   *p_len) {
        void *ptr = NULL;
        int   len =    0;

        LOG_IN();
        ptr = *p_ptr;
        len = *p_len;

        {
                unsigned long mask    = (MAD_GM_CACHE_GRANULARITY - 1);
                unsigned long lng     = (long)ptr;
                unsigned long tmp_lng = (lng & ~mask);
                unsigned long offset  = lng - tmp_lng;

                ptr -= offset;
                len += offset;

                if (len & mask) {
                        len = (len & ~mask) + MAD_GM_CACHE_GRANULARITY;
                }
        }
        
        *p_ptr = ptr;
        *p_len = len;
        LOG_OUT();
}

static
int
mad_gm_register_block(p_mad_gm_port_t    port,
                      void             * ptr,
                      int                len,
                      p_mad_gm_cache_t *_p_cache) {
	struct gm_port    *p_gm_port = NULL;
        gm_status_t        gms       = GM_SUCCESS;
        p_mad_gm_cache_t   cache     = NULL;
        p_mad_gm_cache_t  *p_cache   = NULL;

        LOG_IN();
	p_gm_port =  port->p_gm_port;
        p_cache   = &port->cache_head;

        mad_gm_round2page(&ptr, &len);        

        while ((cache = *p_cache)) {
                if (ptr      >=  cache->ptr
                    &&
                    ptr+len  <=  cache->ptr+cache->len) {
                        cache->ref_count++;

                        if (p_cache != &port->cache_head) {
                                *p_cache = cache->next;
                                cache->next = port->cache_head;
                                port->cache_head = cache;
                        }

                        goto success;
                }

                p_cache = &(cache->next);
        }

        cache = TBX_MALLOC(sizeof(mad_gm_cache_t));

        gms = gm_register_memory(p_gm_port, ptr, len);
        if (gms) {
                __error__("memory registration failed");
                __gmerror__(gms);
                TBX_FREE(cache);
                cache = NULL;
                goto error;
        }

        cache->ptr       = ptr;
        cache->len       = len;
        cache->ref_count = 1;
        cache->next      = port->cache_head;

        port->cache_head = cache;
                
 success:
        *_p_cache = cache;
        LOG_OUT();
        return 0;

 error:
        FAILURE("mad_gm_register_block");
}

static
int
mad_gm_deregister_block(p_mad_gm_port_t  port, 
                        p_mad_gm_cache_t cache) {
	struct gm_port *p_gm_port = NULL;

        LOG_IN();
	p_gm_port = port->p_gm_port;

        if (!--cache->ref_count) {
                p_mad_gm_cache_t *p_cache = NULL;
                int               i       =    0;

                p_cache = &port->cache_head;

                while (*p_cache != cache) {
                        p_cache = &((*p_cache)->next);
                        i++;
                }
                
                if (i >= MAD_GM_CACHE_SIZE) {
                        gm_status_t gms =  GM_SUCCESS;

                        gms = gm_deregister_memory(p_gm_port,
                                                   cache->ptr,
                                                   cache->len);
                        if (gms) {
                                __error__("memory deregistration failed");
                                __gmerror__(gms);
                                goto error;
                        }

                        cache->ptr = NULL;
                        cache->len =    0;

                        *p_cache = cache->next;
                        cache->next = NULL;

                        TBX_FREE(cache);
                }
        }

        LOG_OUT();
        return 0;
        
 error:
        FAILURE("mad_gm_deregister_block failed");
}

static
p_mad_gm_port_t
mad_gm_port_open(p_mad_adapter_t a,
                 int             n) {
        p_mad_gm_adapter_specific_t  as        = NULL;
        p_mad_gm_port_t              port      = NULL;
	struct gm_port              *p_gm_port = NULL;
	gm_status_t                  gms       = GM_SUCCESS;
	int                          port_id   =  0;
	unsigned int                 node_id   =  0;

        LOG_IN();
        port_id = mad_gm_pub_port_array[n];

	gms = gm_open(&p_gm_port, as->device_id, port_id,
			  "mad_gm", GM_API_VERSION_1_1);
	if (gms != GM_SUCCESS) {
                __error__("gm_open failed");
		__gmerror__(gms);
                goto error;
	}
	
	port = TBX_MALLOC(sizeof(mad_gm_port_t));

	gms = gm_get_node_id(p_gm_port, &node_id);
	if (gms != GM_SUCCESS) {
                __error__("gm_get_node_id failed");
		__gmerror__(gms);
                goto error;
	}
        
        port->p_gm_port = p_gm_port;
        port->number    = n;
        port->local_port_id = port_id;
        port->local_node_id = node_id;
        port->cache_head    = NULL;
        port->adapter       = a;
        port->out_darray    = tbx_darray_init();
        port->in_darray     = tbx_darray_init();

        LOG_OUT();
        return port;
        
 error:
        FAILURE("mad_gm_port_open failed");
}



void
mad_gm_register(p_mad_driver_t driver)
{
        p_mad_driver_interface_t interface = NULL;

#ifdef DEBUG
        DEBUG_INIT(gm);
#endif // DEBUG

        LOG_IN();
        TRACE("Registering GM driver");
        interface = driver->interface;

        driver->connection_type  = mad_unidirectional_connection;
        driver->buffer_alignment = 1;
        driver->name             = tbx_strdup("gm");

        interface->driver_init                = mad_gm_driver_init;
        interface->adapter_init               = mad_gm_adapter_init;
        interface->channel_init               = mad_gm_channel_init;
        interface->before_open_channel        = NULL;
        interface->connection_init            = mad_gm_connection_init;
        interface->link_init                  = mad_gm_link_init;
        interface->accept                     = mad_gm_accept;
        interface->connect                    = mad_gm_connect;
        interface->after_open_channel         = NULL;
        interface->before_close_channel       = NULL;
        interface->disconnect                 = mad_gm_disconnect;
        interface->after_close_channel        = NULL;
        interface->link_exit                  = mad_gm_link_exit;
        interface->connection_exit            = mad_gm_connection_exit;
        interface->channel_exit               = mad_gm_channel_exit;
        interface->adapter_exit               = mad_gm_adapter_exit;
        interface->driver_exit                = mad_gm_driver_exit;
        interface->choice                     = NULL;
        interface->get_static_buffer          = NULL;
        interface->return_static_buffer       = NULL;
        interface->new_message                = mad_gm_new_message;
        interface->finalize_message           = NULL;
#ifdef MAD_MESSAGE_POLLING
        interface->poll_message               = mad_gm_poll_message;
#endif // MAD_MESSAGE_POLLING
        interface->receive_message            = mad_gm_receive_message;
        interface->message_received           = mad_gm_message_received;
        interface->send_buffer                = mad_gm_send_buffer;
        interface->receive_buffer             = mad_gm_receive_buffer;
        interface->send_buffer_group          = mad_gm_send_buffer_group;
        interface->receive_sub_buffer_group   = mad_gm_receive_sub_buffer_group;
        LOG_OUT();
}


void
mad_gm_driver_init(p_mad_driver_t d) {
        p_mad_gm_driver_specific_t ds  = NULL;
        gm_status_t                gms = GM_SUCCESS;

        LOG_IN();
        TRACE("Initializing GM driver");
        ds          = TBX_MALLOC(sizeof(mad_gm_driver_specific_t));
        d->specific = ds;
        gms = gm_init();
        if (gms) {
                __error__("gm initialization failed");
                __gmerror__(gms);
                goto error;
        }

        LOG_OUT();
        return;

 error:
        FAILURE("mad_gm_driver_init failed");
}

void
mad_gm_adapter_init(p_mad_adapter_t a) {
        p_mad_gm_adapter_specific_t as        = NULL;
        p_tbx_string_t              param_str = NULL;
        int                         device_id =    0;

        LOG_IN();
        if (tbx_streq(a->dir_adapter->name, "default")) {
                device_id = 0;
        } else {
                device_id = strtol(a->dir_adapter->name, NULL, 0);
        }

        as            = TBX_MALLOC(sizeof(mad_gm_adapter_specific_t));
        as->device_id = device_id;
        a->specific   = as;

        a->mtu = MAD_FORWARD_MAX_MTU;

        param_str    = tbx_string_init_to_int(device_id);
        a->parameter = tbx_string_to_cstring(param_str);
        tbx_string_free(param_str);
        param_str    = NULL;

        {
                struct gm_port *p_gm_port = NULL;
                gm_status_t     gms       = GM_SUCCESS;

                gms = gm_open(&p_gm_port, device_id, mad_gm_pub_port_array[0],
                              "mad_gm", GM_API_VERSION_1_1);
                if (gms != GM_SUCCESS) {
                        __error__("gm_open failed");
                        __gmerror__(gms);
                        goto error;
                }

                gm_close(p_gm_port);
                p_gm_port = NULL;
        }

        as->port_darray = tbx_darray_init();
        LOG_OUT();

 error:
        FAILURE("mad_gm_adapter_init failed");
}

void
mad_gm_channel_init(p_mad_channel_t ch) {
        p_mad_gm_channel_specific_t chs = NULL;

        LOG_IN();
        chs          = TBX_MALLOC(sizeof(mad_gm_channel_specific_t));
        ch->specific = chs;
        LOG_OUT();
}

void
mad_gm_connection_init(p_mad_connection_t in,
		       p_mad_connection_t out) {
        p_mad_channel_t             ch  = NULL;
        p_mad_adapter_t             a   = NULL;
        p_mad_gm_adapter_specific_t as  = NULL;
        const int nb_link = 1;

        LOG_IN();
        ch = (in?:out)->channel;
        a  =  ch->adapter;
        as =  a->specific;

        if (in) {
                p_mad_gm_connection_specific_t is       = NULL;
                ntbx_process_grank_t           gr       =    0;
                int                            port_num =    0;
                p_mad_gm_port_t                port     = NULL;

                is           = TBX_CALLOC(1, sizeof(mad_gm_connection_specific_t));
                in->specific = is;
                in->nb_link  = nb_link;

                gr = ntbx_pc_local_to_global(ch->pc, in->remote_rank);

                while (1) {
                        if (mad_gm_pub_port_array[port_num] < 0) {
                                __error__("no GM port left");
                                goto error;
                        }

                        port = tbx_darray_expand_and_get(as->port_darray, port_num);
                        if (!port) {
                                port = mad_gm_port_open(a, port_num);

                                break;
                        }
                        
                        if (!tbx_darray_expand_and_get(port->in_darray, gr)) {
                                break;
                        }

                        port_num++;
                }
        }

        if (out) {
                p_mad_gm_connection_specific_t os = NULL;

                os            = TBX_CALLOC(1, sizeof(mad_gm_connection_specific_t));
                out->specific = os;
                out->nb_link  = nb_link;
        }
        LOG_OUT();

 error:
        FAILURE("mad_gm_connection_init failed");
}

void
mad_gm_link_init(p_mad_link_t l) {
        p_mad_gm_link_specific_t ls = NULL;

        LOG_IN();
        ls 		 = TBX_CALLOC(1, sizeof(mad_gm_link_specific_t));
        l->specific    = ls;
        l->link_mode   = mad_link_mode_buffer;
        l->buffer_mode = mad_buffer_mode_dynamic;
        l->group_mode  = mad_group_mode_split;
        LOG_OUT();
}

void
mad_gm_accept(p_mad_connection_t   in,
	      p_mad_adapter_info_t ai) {
        p_mad_gm_connection_specific_t is  = NULL;
        p_mad_channel_t                ch  = NULL;
        p_mad_gm_channel_specific_t    chs = NULL;
        p_mad_adapter_t                a   = NULL;
        p_mad_gm_adapter_specific_t    as  = NULL;

        LOG_IN();
        is  = in->specific;
        ch  = in->channel;
        chs = ch->specific;
        a   = ch->adapter;
        as  = a->specific;

        {
                char *ptr1 = NULL;
                char *ptr2 = NULL;

                is->remote_port_id  = strtol(ai->channel_parameter, &ptr1, 0);
                if ((!is->remote_port_id) && (ai->channel_parameter == ptr1))
                        FAILURE("could not extract the remote port id");

                is->remote_node_id  = strtoul(ptr1, &ptr2, 0);
                if ((!is->remote_node_id) && (ptr1 == ptr2))
                        FAILURE("could not extract the remote node id");
        }
        LOG_OUT();
}

void
mad_gm_connect(p_mad_connection_t   out,
	       p_mad_adapter_info_t ai) {
        p_mad_gm_connection_specific_t os  = NULL;
        p_mad_channel_t                ch  = NULL;
        p_mad_gm_channel_specific_t    chs = NULL;
        p_mad_adapter_t                a   = NULL;
        p_mad_gm_adapter_specific_t    as  = NULL;

        LOG_IN();
        os  = out->specific;
        ch  = out->channel;
        chs = ch->specific;
        a   = ch->adapter;
        as  = a->specific;

        {
                char *ptr1 = NULL;
                char *ptr2 = NULL;

                os->remote_port_id  = strtol(ai->channel_parameter, &ptr1, 0);
                if ((!os->remote_port_id) && (ai->channel_parameter == ptr1))
                        FAILURE("could not extract the remote port id");

                os->remote_node_id  = strtoul(ptr1, &ptr2, 0);
                if ((!os->remote_node_id) && (ptr1 == ptr2))
                        FAILURE("could not extract the remote node id");
        }
        LOG_OUT();
}

void
mad_gm_disconnect(p_mad_connection_t cnx) {
        LOG_IN();
        FAILURE("unimplemented");
        LOG_OUT();
}

void
mad_gm_new_message(p_mad_connection_t out) {
        p_mad_gm_connection_specific_t os = NULL;

        LOG_IN();
        os = out->specific;
        FAILURE("unimplemented");
        LOG_OUT();
}

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_gm_poll_message(p_mad_channel_t channel) {
        LOG_IN();
        FAILURE("unimplemented");
        LOG_OUT();
}
#endif // MAD_MESSAGE_POLLING

p_mad_connection_t
mad_gm_receive_message(p_mad_channel_t ch) {
        p_mad_gm_channel_specific_t  chs       = NULL;
        p_tbx_darray_t               in_darray = NULL;

        LOG_IN();
        chs = ch->specific;
        in_darray = ch->in_connection_darray;
        FAILURE("unimplemented");
        LOG_OUT();
}

void
mad_gm_message_received(p_mad_connection_t in) {
        p_mad_channel_t             ch  = NULL;
        p_mad_gm_channel_specific_t chs = NULL;

        LOG_IN();
        ch  = in->channel;
        chs = ch->specific;
        FAILURE("unimplemented");
        LOG_OUT();
}

void
mad_gm_send_buffer(p_mad_link_t   l,
		   p_mad_buffer_t b) {
        LOG_IN();
        FAILURE("unimplemented");
        LOG_OUT();
}

void
mad_gm_receive_buffer(p_mad_link_t    l,
		      p_mad_buffer_t *b) {
        LOG_IN();
        FAILURE("unimplemented");
        LOG_OUT();
}

void
mad_gm_send_buffer_group_1(p_mad_link_t         l,
			   p_mad_buffer_group_t bg) {
        LOG_IN();
        if (!tbx_empty_list(&(bg->buffer_list))) {
                tbx_list_reference_t ref;

                tbx_list_reference_init(&ref, &(bg->buffer_list));
                do {
                        mad_gm_send_buffer(l, tbx_get_list_reference_object(&ref));
                }
                while (tbx_forward_list_reference(&ref));
        }
        LOG_OUT();
}

void
mad_gm_receive_sub_buffer_group_1(p_mad_link_t         l,
				  p_mad_buffer_group_t bg) {
        LOG_IN();
        if (!tbx_empty_list(&(bg->buffer_list))) {
                tbx_list_reference_t ref;

                tbx_list_reference_init(&ref, &(bg->buffer_list));
                do {
                        mad_gm_receive_buffer(l, tbx_get_list_reference_object(&ref));
                } while (tbx_forward_list_reference(&ref));
        }
        LOG_OUT();
}

void
mad_gm_send_buffer_group(p_mad_link_t         l,
			 p_mad_buffer_group_t bg) {
        LOG_IN();
        mad_gm_send_buffer_group_1(l, bg);
        LOG_OUT();
}

void
mad_gm_receive_sub_buffer_group(p_mad_link_t         l,
				tbx_bool_t           first_sub_group
				TBX_UNUSED,
				p_mad_buffer_group_t bg) {
        LOG_IN();
        mad_gm_receive_sub_buffer_group_1(l, bg);
        LOG_OUT();
}

void
mad_gm_link_exit(p_mad_link_t l) {
        p_mad_gm_link_specific_t ls = NULL;

        LOG_IN();
        ls = l->specific;
        TBX_FREE(ls);
        l->specific = ls = NULL;
        LOG_OUT();
}

void
mad_gm_connection_exit(p_mad_connection_t in,
                       p_mad_connection_t out) {

        LOG_IN();
        if (in) {
                p_mad_gm_connection_specific_t is = NULL;
                is = in->specific;
                TBX_FREE(is);
                in->specific = is = NULL;
        }

        if (out) {
                p_mad_gm_connection_specific_t os = NULL;
                os = out->specific;
                TBX_FREE(os);
                out->specific = os = NULL;
        }
        LOG_OUT();
}

void
mad_gm_channel_exit(p_mad_channel_t ch) {
        p_mad_gm_channel_specific_t chs = NULL;

        LOG_IN();
        chs = ch->specific;
        TBX_FREE(chs);
        ch->specific = chs = NULL;
        LOG_OUT();
}

void
mad_gm_adapter_exit(p_mad_adapter_t a) {
        p_mad_gm_adapter_specific_t as = NULL;

        LOG_IN();
        as = a->specific;
        TBX_FREE(as);
        a->specific = as = NULL;
        LOG_OUT();
}

void
mad_gm_driver_exit(p_mad_driver_t d) {
        p_mad_gm_driver_specific_t ds = NULL;

        LOG_IN();
        gm_finalize();
        ds = d->specific;
        TBX_FREE(ds);
        d->specific = ds = NULL;
        LOG_OUT();
}


/*--------------------------------------------------------*/

#if 0
#undef  FAILURE_CONTEXT
#define FAILURE_CONTEXT "Mad/GM: "

DEBUG_DECLARE(gm)
#undef DEBUG_NAME
#define DEBUG_NAME gm

#define USER_MSG_LENGTH 1024
#define SYS_MSG_LENGTH    64
/*
 * Enums
 * -----
 */

typedef enum e_mad_gm_link_id
{
  default_link_id = 0,
  nb_link,
} mad_gm_link_id_t, *p_mad_gm_link_id_t;

typedef enum e_mad_gm_tag
{
  sys_tag = 0,
  user_tag,
} mad_gm_tag_id_t, *p_mad_gm_tag_id_t;

/*
 * local structures
 * ----------------
 */
typedef struct s_mad_gm_driver_specific
{
  int dummy;
} mad_gm_driver_specific_t, *p_mad_gm_driver_specific_t;

typedef struct s_mad_gm_adapter_specific
{
  int device_id;
  int next_port_id;
} mad_gm_adapter_specific_t, *p_mad_gm_adapter_specific_t;

typedef struct s_mad_gm_channel_specific
{
  unsigned int    node_id;
  int             port_id;
  struct gm_port *port;
  void 		 *recv_sys_buffer;
  unsigned int    recv_sys_buffer_size;
  void           *recv_user_buffer;
  unsigned int    recv_user_buffer_size;
  volatile int    lock;
} mad_gm_channel_specific_t, *p_mad_gm_channel_specific_t;

typedef struct s_mad_gm_connection_specific
{
  int           remote_node_id;
  int  	        remote_port_id;
  void         *send_sys_buffer;
  unsigned int  send_sys_buffer_size;
} mad_gm_connection_specific_t, *p_mad_gm_connection_specific_t;

typedef struct s_mad_gm_link_specific
{
  void         *send_user_buffer;
  unsigned int  send_user_buffer_size;
} mad_gm_link_specific_t, *p_mad_gm_link_specific_t;


/*
 * static functions
 * ----------------
 */
#undef DEBUG_NAME
#define DEBUG_NAME mad3

static
void
mad_gm_control(gm_status_t gm_status)
{
  char *msg = NULL;

  LOG_IN();
  switch (gm_status)
    {
    case GM_SUCCESS:
      break;

    case GM_SEND_TIMED_OUT:
      msg = "send timed out";
      break;

    case GM_SEND_REJECTED:
      msg = "send rejected";
      break;

    case GM_SEND_TARGET_PORT_CLOSED:
      msg = "send target port closed";
      break;

    case GM_SEND_TARGET_NODE_UNREACHABLE:
      msg = "send target node unreachable";
      break;

    case GM_SEND_DROPPED:
      msg = "send dropped";
      break;

    case GM_SEND_PORT_CLOSED:
      msg = "send port closed";
      break;

    default:
      msg = "unknown error";
      break;
    }

  if (msg)
    FAILURE(msg);

  LOG_OUT();
}

static
void
__mad_gm_send_callback(struct gm_port *port TBX_UNUSED,
		       void           *cntxt,
		       gm_status_t     gms)
{
  p_mad_gm_channel_specific_t chs = cntxt;

  LOG_IN();
  mad_gm_control(gms);
  chs->lock = 0;
  LOG_OUT();
}

static
void
mad_gm_write(p_mad_link_t   l,
	     p_mad_buffer_t b)
{
  p_mad_gm_link_specific_t       ls  = NULL;
  p_mad_connection_t             out = NULL;
  p_mad_gm_connection_specific_t os  = NULL;
  p_mad_channel_t                ch  = NULL;
  p_mad_gm_channel_specific_t    chs = NULL;

  LOG_IN();
  ls  = l->specific;
  out = l->connection;
  os  = out->specific;
  ch  = out->channel;
  chs = ch->specific;

  while (mad_more_data(b))
    {
      unsigned int len = 0;

      len = min(USER_MSG_LENGTH, b->bytes_written - b->bytes_read);

      gm_bcopy(b->buffer + b->bytes_read, ls->send_user_buffer, len);

      while(chs->lock);
      chs->lock = 1;

      if (!gm_num_send_tokens (chs->port))
	FAILURE("not enough send tokens");

      gm_send_with_callback(chs->port,
			    ls->send_user_buffer,
			    ls->send_user_buffer_size,
			    len,
			    GM_LOW_PRIORITY,
			    os->remote_node_id,
			    os->remote_port_id,
			    __mad_gm_send_callback,
			    chs);
      b->bytes_read += len;
    }

  while(chs->lock);
  LOG_OUT();
}

static
void
mad_gm_read(p_mad_link_t   l,
	    p_mad_buffer_t b)
{
  p_mad_connection_t              in   = NULL;
  p_mad_gm_connection_specific_t  is   = NULL;
  p_mad_channel_t                 ch   = NULL;
  p_mad_gm_channel_specific_t     chs  = NULL;
  struct gm_port                 *port = NULL;
  gm_recv_event_t                *e    = NULL;

  LOG_IN();
  in   = l->connection;
  is   = in->specific;
  ch   = in->channel;
  chs  = ch->specific;
  port = chs->port;

  while (!mad_buffer_full(b))
    {
      while (1)
	{
	  e = gm_receive (port);
	  switch (gm_ntohc (e->recv.type))
	    {
	    case GM_FAST_PEER_RECV_EVENT:
	    case GM_FAST_RECV_EVENT:
	      if (gm_ntohp(e->recv.buffer) != chs->recv_user_buffer)
		FAILURE("unexpected message");

	      if (gm_ntohc(e->recv.tag) != user_tag)
		FAILURE("unexpected message");

	      gm_bcopy(gm_ntohp(e->recv.message),
		       b->buffer + b->bytes_written,
		       gm_ntohl(e->recv.length));

	      b->bytes_written += gm_ntohl(e->recv.length);
	      goto reception;

	      break;


	    case GM_PEER_RECV_EVENT:
	    case GM_RECV_EVENT:
	      if (gm_ntohp(e->recv.buffer) != chs->recv_user_buffer)
		FAILURE("unexpected message");

	      if (gm_ntohc(e->recv.tag) != user_tag)
		FAILURE("unexpected message");

	      gm_bcopy(gm_ntohp(e->recv.buffer),
		       b->buffer + b->bytes_written,
		       gm_ntohl(e->recv.length));

	      b->bytes_written += gm_ntohl(e->recv.length);
	      goto reception;

	      break;


	    case GM_NO_RECV_EVENT:
	      break;

	    default:
	      gm_unknown (port, e);
	      break;
	    }
	}

    reception:
      if (gm_num_receive_tokens(port))
	{
	  gm_provide_receive_buffer_with_tag(port,
					     chs->recv_user_buffer,
					     chs->recv_user_buffer_size,
					     GM_LOW_PRIORITY,
					     user_tag);
	}
      else
	FAILURE("not enough receive tokens");
    }
  LOG_OUT();
}


/*
 * Registration function
 * ---------------------
 */

void
mad_gm_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface = NULL;

#ifdef DEBUG
  DEBUG_INIT(gm);
#endif // DEBUG

  LOG_IN();
  TRACE("Registering GM driver");
  interface = driver->interface;

  driver->connection_type  = mad_unidirectional_connection;
  driver->buffer_alignment = 1;
  driver->name             = tbx_strdup("gm");

  interface->driver_init                = mad_gm_driver_init;
  interface->adapter_init               = mad_gm_adapter_init;
  interface->channel_init               = mad_gm_channel_init;
  interface->before_open_channel        = NULL;
  interface->connection_init            = mad_gm_connection_init;
  interface->link_init                  = mad_gm_link_init;
  interface->accept                     = mad_gm_accept;
  interface->connect                    = mad_gm_connect;
  interface->after_open_channel         = NULL;
  interface->before_close_channel       = NULL;
  interface->disconnect                 = mad_gm_disconnect;
  interface->after_close_channel        = NULL;
  interface->link_exit                  = NULL;
  interface->connection_exit            = NULL;
  interface->channel_exit               = mad_gm_channel_exit;
  interface->adapter_exit               = NULL;
  interface->driver_exit                = mad_gm_driver_exit;
  interface->choice                     = NULL;
  interface->get_static_buffer          = NULL;
  interface->return_static_buffer       = NULL;
  interface->new_message                = mad_gm_new_message;
  interface->finalize_message           = NULL;
#ifdef MAD_MESSAGE_POLLING
  interface->poll_message               = mad_gm_poll_message;
#endif // MAD_MESSAGE_POLLING
  interface->receive_message            = mad_gm_receive_message;
  interface->message_received           = mad_gm_message_received;
  interface->send_buffer                = mad_gm_send_buffer;
  interface->receive_buffer             = mad_gm_receive_buffer;
  interface->send_buffer_group          = mad_gm_send_buffer_group;
  interface->receive_sub_buffer_group   = mad_gm_receive_sub_buffer_group;
  LOG_OUT();
}


void
mad_gm_driver_init(p_mad_driver_t d)
{
  p_mad_gm_driver_specific_t ds  = NULL;
  gm_status_t                gms = GM_SUCCESS;

  LOG_IN();
  TRACE("Initializing GM driver");
  ds          = TBX_MALLOC(sizeof(mad_gm_driver_specific_t));
  d->specific = ds;
  gms         = gm_init();
  mad_gm_control(gms);
  LOG_OUT();
}

void
mad_gm_adapter_init(p_mad_adapter_t a)
{
  p_mad_gm_adapter_specific_t as        = NULL;
  p_tbx_string_t              param_str = NULL;
  int                         device_id =    0;

  LOG_IN();
  if (tbx_streq(a->dir_adapter->name, "default"))
    {
      device_id = 0;
    }
  else
    {
      device_id = strtol(a->dir_adapter->name, NULL, 0);
    }

  as               = TBX_MALLOC(sizeof(mad_gm_adapter_specific_t));
  as->device_id    = device_id;
  as->next_port_id = 2;
  a->specific      = as;

#if defined(USER_MSG_SIZE)
  a->mtu = USER_MSG_SIZE;
#else // defined(USER_MSG_SIZE)
  a->mtu = MAD_FORWARD_MAX_MTU;
#endif // defined(USER_MSG_SIZE)

  param_str    = tbx_string_init_to_int(device_id);
  a->parameter = tbx_string_to_cstring(param_str);
  tbx_string_free(param_str);
  param_str    = NULL;
  LOG_OUT();
}

void
mad_gm_channel_init(p_mad_channel_t ch)
{
  p_mad_gm_channel_specific_t  chs       = NULL;
  p_mad_adapter_t              a         = NULL;
  p_mad_gm_adapter_specific_t  as        = NULL;
  p_tbx_string_t               param_str = NULL;
  unsigned int                 node_id   =    0;
  int                          port_id   =    0;
  gm_status_t                  gms  	 = GM_SUCCESS;
  struct gm_port              *port 	 = NULL;

  LOG_IN();
  a 	  	= ch->adapter;
  as      	= a->specific;
  port_id 	= as->next_port_id++;

  gms     	= gm_open(&port, as->device_id, port_id, ch->name, GM_API_VERSION_1_1);
  mad_gm_control(gms);

  {
    unsigned int size_s = 0;
    unsigned int size_u = 0;

    size_s = gm_min_size_for_length(SYS_MSG_LENGTH);
    size_u = gm_min_size_for_length(USER_MSG_LENGTH);

    gms = gm_set_acceptable_sizes(port, GM_LOW_PRIORITY, size_s|size_u);
    mad_gm_control(gms);
  }

  gms           = gm_get_node_id(port, &node_id);
  mad_gm_control(gms);

  chs           = TBX_MALLOC(sizeof(mad_gm_channel_specific_t));
  chs->node_id  = node_id;
  chs->port_id  = port_id;
  chs->port     = port;
  ch->specific  = chs;

  param_str     = tbx_string_init_to_int(port_id);
  tbx_string_append_char(param_str, ' ');
  tbx_string_append_uint(param_str, node_id);
  ch->parameter = tbx_string_to_cstring(param_str);
  tbx_string_free(param_str);
  param_str     = NULL;

  chs->recv_sys_buffer      = gm_dma_malloc(chs->port, SYS_MSG_LENGTH);
  chs->recv_sys_buffer_size = gm_min_size_for_length(SYS_MSG_LENGTH);

  if (gm_num_receive_tokens(port))
    {
      gm_provide_receive_buffer_with_tag(chs->port,
					 chs->recv_sys_buffer,
					 chs->recv_sys_buffer_size,
					 GM_LOW_PRIORITY,
					 sys_tag);
    }
  else
    FAILURE("not enough receive tokens");


  chs->recv_user_buffer      = gm_dma_malloc(chs->port, USER_MSG_LENGTH);
  chs->recv_user_buffer_size = gm_min_size_for_length(USER_MSG_LENGTH);

  if (gm_num_receive_tokens(port))
    {
      gm_provide_receive_buffer_with_tag(chs->port,
					 chs->recv_user_buffer,
					 chs->recv_user_buffer_size,
					 GM_LOW_PRIORITY,
					 user_tag);
    }
  else
    FAILURE("not enough receive tokens");

  LOG_OUT();
}

void
mad_gm_connection_init(p_mad_connection_t in,
		       p_mad_connection_t out)
{
  LOG_IN();
  if (in)
    {
      p_mad_gm_connection_specific_t is = NULL;

      is           = TBX_CALLOC(1, sizeof(mad_gm_connection_specific_t));
      in->specific = is;
      in->nb_link  = nb_link;
    }

  if (out)
    {
      p_mad_gm_connection_specific_t os = NULL;

      os            = TBX_CALLOC(1, sizeof(mad_gm_connection_specific_t));
      out->specific = os;
      out->nb_link  = nb_link;
    }
  LOG_OUT();
}

void
mad_gm_link_init(p_mad_link_t l)
{
  p_mad_gm_link_specific_t ls = NULL;

  LOG_IN();
  ls 		 = TBX_CALLOC(1, sizeof(mad_gm_link_specific_t));
  l->specific    = ls;
  l->link_mode   = mad_link_mode_buffer;
  l->buffer_mode = mad_buffer_mode_dynamic;
  l->group_mode  = mad_group_mode_split;
  LOG_OUT();
}

void
mad_gm_accept(p_mad_connection_t   in,
	      p_mad_adapter_info_t ai)
{
  p_mad_gm_connection_specific_t is  = NULL;
  p_mad_channel_t                ch  = NULL;
  p_mad_gm_channel_specific_t    chs = NULL;
  p_mad_adapter_t                a   = NULL;
  p_mad_gm_adapter_specific_t    as  = NULL;

  LOG_IN();
  is  = in->specific;
  ch  = in->channel;
  chs = ch->specific;
  a   = ch->adapter;
  as  = a->specific;

  {
    char *ptr1 = NULL;
    char *ptr2 = NULL;

    is->remote_port_id  = strtol(ai->channel_parameter, &ptr1, 0);
    if ((!is->remote_port_id) && (ai->channel_parameter == ptr1))
      FAILURE("could not extract the remote port id");

    is->remote_node_id  = strtoul(ptr1, &ptr2, 0);
    if ((!is->remote_node_id) && (ptr1 == ptr2))
      FAILURE("could not extract the remote node id");
  }
  LOG_OUT();
}

void
mad_gm_connect(p_mad_connection_t   out,
	       p_mad_adapter_info_t ai)
{
  p_mad_gm_connection_specific_t os  = NULL;
  p_mad_channel_t                ch  = NULL;
  p_mad_gm_channel_specific_t    chs = NULL;
  p_mad_adapter_t                a   = NULL;
  p_mad_gm_adapter_specific_t    as  = NULL;
  p_mad_link_t                   l   = NULL;

  LOG_IN();
  os  = out->specific;
  ch  = out->channel;
  chs = ch->specific;
  a   = ch->adapter;
  as  = a->specific;

  {
    char *ptr1 = NULL;
    char *ptr2 = NULL;

    os->remote_port_id  = strtol(ai->channel_parameter, &ptr1, 0);
    if ((!os->remote_port_id) && (ai->channel_parameter == ptr1))
      FAILURE("could not extract the remote port id");

    os->remote_node_id  = strtoul(ptr1, &ptr2, 0);
    if ((!os->remote_node_id) && (ptr1 == ptr2))
      FAILURE("could not extract the remote node id");
  }

  os->send_sys_buffer      = gm_dma_malloc(chs->port, SYS_MSG_LENGTH);
  os->send_sys_buffer_size = gm_min_size_for_length(SYS_MSG_LENGTH);

  l = out->link_array[default_link_id];
  {
    p_mad_gm_link_specific_t ls = NULL;

    ls                        = l->specific;
    ls->send_user_buffer      = gm_dma_malloc(chs->port, USER_MSG_LENGTH);
    ls->send_user_buffer_size = gm_min_size_for_length(USER_MSG_LENGTH);
  }
  LOG_OUT();
}

void
mad_gm_disconnect(p_mad_connection_t cnx)
{
  p_mad_gm_connection_specific_t cs  = cnx->specific;
  p_mad_channel_t                ch  = NULL;
  p_mad_gm_channel_specific_t    chs = NULL;
  p_mad_link_t                   l   = NULL;

  LOG_IN();
  ch  = cnx->channel;
  chs = ch->specific;
  if (cnx->way == mad_incoming_connection)
    {
      gm_dma_free(chs->port, chs->recv_sys_buffer);
      chs->recv_sys_buffer = NULL;
      gm_dma_free(chs->port, chs->recv_user_buffer);
      chs->recv_user_buffer = NULL;
    }
  else if (cnx->way == mad_outgoing_connection)
    {
      gm_dma_free(chs->port, cs->send_sys_buffer);
      cs->send_sys_buffer = NULL;

      l = cnx->link_array[default_link_id];
      {
	p_mad_gm_link_specific_t ls = NULL;

	ls                   = l->specific;
	gm_dma_free(chs->port, ls->send_user_buffer);
	ls->send_user_buffer = NULL;
      }
    }
  else
    FAILURE("invalid connection way");
  LOG_OUT();
}

void
mad_gm_new_message(p_mad_connection_t out)
{
  p_mad_gm_connection_specific_t os  = NULL;
  p_mad_channel_t                ch  = NULL;
  p_mad_gm_channel_specific_t    chs = NULL;

  LOG_IN();
  os  = out->specific;
  ch  = out->channel;
  chs = ch->specific;

  while(chs->lock);
  chs->lock = 1;

  if (!gm_num_send_tokens (chs->port))
    FAILURE("not enough send tokens");

  gm_send_with_callback(chs->port,
			os->send_sys_buffer,
			os->send_sys_buffer_size,
			SYS_MSG_LENGTH,
			GM_LOW_PRIORITY,
			os->remote_node_id,
			os->remote_port_id,
			__mad_gm_send_callback,
			chs);
  LOG_OUT();
}

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_gm_poll_message(p_mad_channel_t channel)
{
  LOG_IN();
  FAILURE("unimplemented");
  LOG_OUT();
}
#endif // MAD_MESSAGE_POLLING

p_mad_connection_t
mad_gm_receive_message(p_mad_channel_t ch)
{
  p_mad_gm_channel_specific_t  chs       = NULL;
  p_tbx_darray_t               in_darray = NULL;
  p_mad_connection_t           in        = NULL;
  struct gm_port              *port      = NULL;
  gm_recv_event_t             *e         = NULL;

  LOG_IN();
  chs = ch->specific;
  port = chs->port;

  while (1)
    {
      e = gm_receive (port);
      switch (gm_ntohc (e->recv.type))
	{
	case GM_FAST_PEER_RECV_EVENT:
	case GM_FAST_RECV_EVENT:
	  gm_memorize_message (gm_ntohp(e->recv.buffer),
			       gm_ntohp(e->recv.message),
			       gm_ntohl(e->recv.length));
	case GM_PEER_RECV_EVENT:
	case GM_RECV_EVENT:
	  if (gm_ntohp(e->recv.buffer) != chs->recv_sys_buffer)
	    FAILURE("unexpected message");

	  if (gm_ntohc(e->recv.tag) != sys_tag)
	    FAILURE("unexpected message");

	  goto reception;

	  break;

	case GM_NO_RECV_EVENT:
	  break;

	default:
	  gm_unknown (port, e);
	  break;
	}
    }

reception:
  {
    int idx = 0;

    in_darray = ch->in_connection_darray;
    in        = tbx_darray_first_idx(in_darray, &idx);

    while (in)
      {
	p_mad_gm_connection_specific_t is = NULL;

	is = in->specific;

	if ((is->remote_node_id == gm_ntohs(e->recv.sender_node_id))
	    && (is->remote_port_id == gm_ntohs(e->recv.sender_port_id)))
	  {
	    LOG_OUT();

	    return in;
	  }

	in = tbx_darray_next_idx(in_darray, &idx);
      }
  }

  FAILURE("invalid channel state");
}

void
mad_gm_message_received(p_mad_connection_t in)
{
  p_mad_channel_t             ch  = NULL;
  p_mad_gm_channel_specific_t chs = NULL;

  LOG_IN();
  ch  = in->channel;
  chs = ch->specific;

  if (gm_num_receive_tokens(chs->port))
    {
      gm_provide_receive_buffer_with_tag(chs->port,
					 chs->recv_sys_buffer,
					 chs->recv_sys_buffer_size,
					 GM_LOW_PRIORITY,
					 sys_tag);
    }
  else
    FAILURE("not enough receive tokens");
  LOG_OUT();
}

void
mad_gm_send_buffer(p_mad_link_t   l,
		   p_mad_buffer_t b)
{
  LOG_IN();
  mad_gm_write(l, b);
  LOG_OUT();
}

void
mad_gm_receive_buffer(p_mad_link_t    l,
		      p_mad_buffer_t *b)
{
  LOG_IN();
  mad_gm_read(l, *b);
  LOG_OUT();
}

void
mad_gm_send_buffer_group_1(p_mad_link_t         l,
			   p_mad_buffer_group_t bg)
{
  LOG_IN();
  if (!tbx_empty_list(&(bg->buffer_list)))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &(bg->buffer_list));
      do
	{
	  mad_gm_write(l, tbx_get_list_reference_object(&ref));
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

void
mad_gm_receive_sub_buffer_group_1(p_mad_link_t         l,
				  p_mad_buffer_group_t bg)
{
  LOG_IN();
  if (!tbx_empty_list(&(bg->buffer_list)))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &(bg->buffer_list));
      do
	{
	  mad_gm_read(l, tbx_get_list_reference_object(&ref));
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

void
mad_gm_send_buffer_group(p_mad_link_t         l,
			 p_mad_buffer_group_t bg)
{
  LOG_IN();
  mad_gm_send_buffer_group_1(l, bg);
  LOG_OUT();
}

void
mad_gm_receive_sub_buffer_group(p_mad_link_t         l,
				tbx_bool_t           first_sub_group
				TBX_UNUSED,
				p_mad_buffer_group_t bg)
{
  LOG_IN();
  mad_gm_receive_sub_buffer_group_1(l, bg);
  LOG_OUT();
}



void
mad_gm_channel_exit(p_mad_channel_t ch)
{
  p_mad_gm_channel_specific_t chs = NULL;

  LOG_IN();
  chs = ch->specific;
  gm_close(chs->port);
  chs->port = NULL;
  TBX_FREE(chs);
  ch->specific = chs = NULL;
  LOG_OUT();
}

void
mad_gm_driver_exit(p_mad_driver_t d)
{
  p_mad_gm_driver_specific_t ds = NULL;

  LOG_IN();
  gm_finalize();
  ds = d->specific;
  TBX_FREE(ds);
  d->specific = ds = NULL;
  LOG_OUT();
}
#endif 
