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
#include <malloc.h>
#include <gm.h>

#if defined(MARCEL)
#  define MAD_GM_MARCEL_POLL
#  undef TBX_LOCK
#  undef TBX_UNLOCK
#  warning Code still using old lock_task/unlock_task
#  define TBX_LOCK() marcel_extlib_protect()
#  define TBX_UNLOCK() marcel_extlib_unprotect()
#endif

#define MAD_GM_MEMORY_CACHE 1

/*
 *  Macros
 */
#if 0
#  define __RDTSC__ ({jlong time;__asm__ __volatile__ ("rdtsc" : "=A" (time));time;})
#else
#  define __RDTSC__ 0LL
#endif

/* Debugging macros */
#if 0
#  define __trace__(s, ...) fprintf(stderr, "[%Ld]:%s:%d: "s"\n", __RDTSC__, __FUNCTION__, __LINE__ , ## __VA_ARGS__)
#else
#  define __trace__(s, ...)
#endif

#if 0
#  define __disp__(s, ...)  fprintf(stderr, "[%Ld]:%s:%d: "s"\n", __RDTSC__, __FUNCTION__, __LINE__ , ## __VA_ARGS__)
#  define __in__()          fprintf(stderr, "[%Ld]:%s:%d: -->\n", __RDTSC__, __FUNCTION__, __LINE__)
#  define __out__()         fprintf(stderr, "[%Ld]:%s:%d: <--\n", __RDTSC__, __FUNCTION__, __LINE__)
#  define __err__()         fprintf(stderr, "[%Ld]:%s:%d: <!!\n", __RDTSC__, __FUNCTION__, __LINE__)
#else
#  define __disp__(s, ...)
#  define __in__()
#  define __out__()
#  define __err__()
#endif

/* Error message macros */
#if 1
#  define __error__(s, ...) fprintf(stderr, "[%Ld]:%s:%d: *error* "s"\n", __RDTSC__, __FUNCTION__, __LINE__ , ## __VA_ARGS__)
#else
#  define __error__(s, ...)
#endif

#define __gmerror__(x) (mad_gm_error((x), __LINE__))

#define MAD_GM_MAX_BLOCK_LEN	 	(2*1024*1024)
#define MAD_GM_PACKET_LEN                4096
#define MAD_GM_PACKET_HDR		 8
#define MAD_GM_CACHE_GRANULARITY	 0x1000
#define MAD_GM_CACHE_SIZE		 0x1000
#define MAD_GM_PACKET_CACHE_SIZE	 0x10

#define MAD_GM_POLLING_MODE \
    (MARCEL_POLL_AT_TIMER_SIG | MARCEL_POLL_AT_YIELD | MARCEL_POLL_AT_IDLE)

#define MAD_GM_LINK_CPY 0
#define MAD_GM_LINK_RDV 1

typedef struct s_mad_gm_cache *p_mad_gm_cache_t;
typedef struct s_mad_gm_cache {
        void               *ptr;
        int                 len;
        int                 ref_count;
        p_mad_gm_cache_t    next;
        volatile int        dirty;
} mad_gm_cache_t;

typedef struct s_mad_gm_port {
	struct gm_port     *p_gm_port;
        int                 number;
        int                 local_port_id;
        unsigned int        local_node_id;
        char                local_unique_node_id[6];
        //p_mad_gm_cache_t    cache_head;
        p_mad_adapter_t     adapter;
        p_tbx_darray_t      out_darray;
        p_tbx_darray_t      in_darray;
        p_mad_connection_t  active_input;
        unsigned char      *packet;
        unsigned int        packet_size;
        int                 packet_length;
        p_tbx_slist_t       packet_cache;
} mad_gm_port_t, *p_mad_gm_port_t;

#ifdef MAD_GM_MARCEL_POLL
typedef struct s_mad_gm_poll_channel_data {
        p_mad_channel_t    ch;
        p_mad_connection_t c;
} mad_gm_poll_channel_data_t, *p_mad_gm_poll_channel_data;

typedef struct s_mad_gm_poll_sem_data {
        marcel_sem_t *sem;
} mad_gm_poll_sem_data_t;

typedef union u_mad_gm_poll_data {
        mad_gm_poll_channel_data_t channel_op;
        mad_gm_poll_sem_data_t   sem_op;
} mad_gm_poll_data_t, *p_mad_gm_poll_data_t;

typedef enum e_mad_gm_poll_op {
        mad_gm_poll_channel_op,
        mad_gm_poll_sem_op,
} mad_gm_poll_op_t, *p_mad_gm_poll_op_t;

typedef struct s_mad_gm_poll_req {
        mad_gm_poll_op_t   op;
        mad_gm_poll_data_t data;
        p_mad_gm_port_t    port;
} mad_gm_poll_req_t, *p_mad_gm_poll_req_t;
#endif

typedef struct s_mad_gm_request {
        p_mad_gm_port_t     port;
        p_mad_connection_t  in;
        p_mad_connection_t  out;
        gm_status_t         status;
} mad_gm_request_t, *p_mad_gm_request_t;

typedef struct s_mad_gm_driver_specific {
        int                 dummy;
#ifdef MAD_GM_MARCEL_POLL
        marcel_pollid_t     gm_pollid;
#endif
} mad_gm_driver_specific_t, *p_mad_gm_driver_specific_t;

typedef struct s_mad_gm_adapter_specific {
        int                 device_id;
        p_mad_gm_port_t     port;
} mad_gm_adapter_specific_t, *p_mad_gm_adapter_specific_t;

typedef struct s_mad_gm_channel_specific {
        int                 size;
        int                 next;
} mad_gm_channel_specific_t, *p_mad_gm_channel_specific_t;

typedef struct s_mad_gm_connection_specific {
        p_mad_buffer_t      buffer;
        int                 length;
        int                 local_id;
        int                 remote_id;
        int                 remote_dev_id;
        int                 remote_port_id;
        unsigned int        remote_node_id;
        char                remote_unique_node_id[6];
        int                 first;
        p_mad_gm_cache_t    cache;
        p_mad_gm_request_t  request;
        unsigned char      *packet;
        int                 packet_size;
        int                 packet_length;
        volatile int        state;
        p_tbx_slist_t	    cpy_buffer_slist;

#ifdef MAD_GM_MARCEL_POLL
        marcel_sem_t        sem;
#else
        volatile int        send_lock;
        volatile int        receive_lock;
#endif
} mad_gm_connection_specific_t, *p_mad_gm_connection_specific_t;

typedef struct s_mad_gm_link_specific {
        int                 dummy;
} mad_gm_link_specific_t, *p_mad_gm_link_specific_t;

/*
 *  Prototypes
 * ____________
 */
static
void
mad_gm_free_hook(void *ptr, const void *caller);

static
void *
mad_gm_realloc_hook(void *ptr, size_t len, const void *caller);

static
void
mad_gm_malloc_initialize_hook(void);

/*
 *  Global variables
 * __________________
 */

static
const int mad_gm_pub_port_array[] = {
        2,
        4,
        5,
        6,
        7,
        -1
};

static
const int mad_gm_nb_ports = 5;

static
p_mad_gm_cache_t mad_gm_cache_array[8] = {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
};

static
void (*mad_gm_old_free_hook)(void *PTR, const void *CALLER) = NULL;

static
void * (*mad_gm_old_realloc_hook)(void *PTR, size_t LEN, const void *CALLER) = NULL;

#if MAD_GM_MEMORY_CACHE
void (*__malloc_initialize_hook) (void) = mad_gm_malloc_initialize_hook;
#endif // MAD_GM_MEMORY_CACHE

static
int mad_gm_malloc_hooked = 0;

static TBX_CRITICAL_SECTION(mad_gm_access);
static TBX_CRITICAL_SECTION(mad_gm_reception);


/*
 *  Functions
 * ___________
 */

static
void
mad_gm_malloc_initialize_hook(void) {
        mad_gm_malloc_hooked = 1;
        mad_gm_old_free_hook	= __free_hook;
        mad_gm_old_realloc_hook	= __realloc_hook;
        __free_hook		= mad_gm_free_hook;
        __realloc_hook		= mad_gm_realloc_hook;
}

static
void
mad_gm_uncache(void *ptr)
{
        int i = 0;

        for (i = 0; i < mad_gm_nb_ports; i++) {
                p_mad_gm_cache_t cache = mad_gm_cache_array[i];

                while (cache) {
                        if (ptr  >=  cache->ptr
                            &&
                            ptr  <  cache->ptr+cache->len) {
                                cache->dirty = 1;
                        }

                        cache = cache->next;
                }
        }
}

static
void *
mad_gm_realloc_hook(void *ptr, size_t len, const void *caller) {
        void *new_ptr = NULL;

        __free_hook		= mad_gm_old_free_hook;
        __realloc_hook		= mad_gm_old_realloc_hook;

        mad_gm_uncache(ptr);
        new_ptr = realloc(ptr, len);

        mad_gm_old_free_hook	= __free_hook;
        mad_gm_old_realloc_hook	= __realloc_hook;

        __free_hook		= mad_gm_free_hook;
        __realloc_hook		= mad_gm_realloc_hook;

        return new_ptr;
}


static
void
mad_gm_free_hook(void *ptr, const void *caller) {
        __free_hook		= mad_gm_old_free_hook;
        __realloc_hook		= mad_gm_old_realloc_hook;

        mad_gm_uncache(ptr);
        free(ptr);

        mad_gm_old_free_hook	= __free_hook;
        mad_gm_old_realloc_hook	= __realloc_hook;

        __free_hook		= mad_gm_free_hook;
        __realloc_hook		= mad_gm_realloc_hook;
}


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

#if defined (GM_API_VERSION_1_5) && GM_API_VERSION >= GM_API_VERSION_1_5
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
mad_gm_extract_info(p_mad_adapter_info_t ai,
                    p_mad_connection_t   c) {
	gm_status_t                    gms = GM_SUCCESS;
        p_mad_gm_adapter_specific_t    as  = NULL;
        p_mad_gm_connection_specific_t cs  = NULL;

        LOG_IN();
        as  = c->channel->adapter->specific;
        cs  = c->specific;

        {
                char         *str  = NULL;
                char         *ptr1 = NULL;
                char         *ptr2 = NULL;
                char         *ptr3 = NULL;
                char         *pa   = NULL;
                char         *pb   = NULL;
                signed   int  si   =    0;
                unsigned int  ui   =    0;

                str = tbx_strdup(ai->dir_adapter->parameter);

                si = strtol(str, &ptr1, 0);
                if (str == ptr1) {
                        __error__("could not extract the remote device id");
                        goto error;
                }
                cs->remote_dev_id = si;

                si = strtol(ptr1, &ptr2, 0);
                if (ptr1 == ptr2) {
                        __error__("could not extract the remote port id");
                        goto error;
                }
                cs->remote_port_id = si;

                ui = strtoul(ptr2, &ptr3, 0);
                if (ptr2 == ptr3) {
                        __error__("could not extract the remote node id");
                        goto error;
                }
                cs->remote_node_id = ui;

#if defined (GM_API_VERSION_2_0) && GM_API_VERSION >= GM_API_VERSION_2_0
                {
                        int i = 0;

                        pa = ptr3;
                        pb = ptr3;

                        for (i = 0; i < 6; i++) {
                                int val = 0;
                                val = strtol(pa, &pb, 0);
                                if (pa == pb) {
                                        __error__("could not extract the remote unique node id");
                                        goto error;
                                }

                                pa = pb;
                                cs->remote_unique_node_id[i] = (char)val;
                        }
                }

                gms = gm_unique_id_to_node_id(as->port->p_gm_port, cs->remote_unique_node_id, &(cs->remote_node_id));
                if (gms != GM_SUCCESS) {
                        __error__("gm_unique_id_to_node_id failed");
                        __gmerror__(gms);
                        goto error;
                }
#endif

                TBX_FREE(str);
                str = NULL;
        }

        {
                char         *str  = NULL;
                char         *ptr1 = NULL;
                signed   int  si   =    0;

                str = tbx_strdup(ai->connection_parameter);

                si = strtol(str, &ptr1, 0);
                if (str == ptr1) {
                        __error__("could not extract the remote connection id");
                        goto error;
                }
                cs->remote_id = si;

                TBX_FREE(str);
                str = NULL;
        }
        LOG_OUT();
        return;

 error:
        FAILURE("mad_gm_extract_info failed");
        LOG_OUT();
}

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
        p_mad_gm_cache_t  *p_cache_head   = NULL;

        LOG_IN();
	p_gm_port =  port->p_gm_port;
        p_cache_head	= &(mad_gm_cache_array[port->number]);
        p_cache   = p_cache_head;

        mad_gm_round2page(&ptr, &len);
#if MAD_GM_MEMORY_CACHE
        while ((cache = *p_cache)) {
                if (!cache->dirty
                    &&
                    ptr      >=  cache->ptr
                    &&
                    ptr+len  <=  cache->ptr+cache->len) {
                        cache->ref_count++;

                        if (p_cache != p_cache_head) {
                                *p_cache = cache->next;
                                cache->next = *p_cache_head;
                                *p_cache_head = cache;
                        }

                        goto success;
                }

                if (cache->dirty) {

                        if (!cache->ref_count) {
                                if (p_cache == p_cache_head) {
                                        *p_cache_head = cache->next;
                                }

                                *p_cache = cache->next;

                                {
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

                                        cache->next = NULL;
                                }

                                TBX_FREE(cache);
                        } else {
                                // DISP("cache entry dirty but ref_count is not null:  %p->%p, refcount = %d", cache->ptr, cache->ptr+cache->len, cache->ref_count);
                                p_cache = &(cache->next);
                        }
                } else {
                        p_cache = &(cache->next);
                }
        }
#endif /* MAD_GM_MEMORY_CACHE */
        cache = TBX_MALLOC(sizeof(mad_gm_cache_t));

        gms = gm_register_memory(p_gm_port, ptr, len);
        if (gms) {
                __error__("memory registration failed");
                __gmerror__(gms);
                TBX_FREE(cache);
                cache = NULL;
                goto error;
        }

        cache->dirty	 = 0;
        cache->ptr       = ptr;
        cache->len       = len;
        cache->ref_count = 1;
        cache->next      = *p_cache_head;

        // DISP("new cache entry: %p->%p", ptr, ptr+len);

        *p_cache_head = cache;
        // DISP("port_num = %d, cache_head = %p", port->number, mad_gm_cache_array[port->number]);

#if MAD_GM_MEMORY_CACHE
 success:
#endif /* MAD_GM_MEMORY_CACHE */
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
                p_mad_gm_cache_t *p_cache_head   = NULL;
                int               i       =    0;

                p_cache_head	= &(mad_gm_cache_array[port->number]);
                p_cache   = p_cache_head;

                while (*p_cache != cache) {
                        p_cache = &((*p_cache)->next);
                        i++;
                }

#if MAD_GM_MEMORY_CACHE
                if (cache->dirty || i >= MAD_GM_CACHE_SIZE)
#endif /* MAD_GM_MEMORY_CACHE */
                        {
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
void
mad_gm_callback(struct gm_port *p_gm_port,
               void           *ptr,
               gm_status_t     gms) {
        p_mad_gm_request_t rq   = NULL;
        p_mad_gm_port_t    port = NULL;

        LOG_IN();
	if (gms != GM_SUCCESS) {
	  __gmerror__(gms);
	  FAILURE("send request failed");
	}

        rq         = ptr;
        rq->status = gms;
        port       = rq->port;

        if (rq->out) {
                p_mad_connection_t             out = NULL;
                p_mad_gm_connection_specific_t os  = NULL;

                out = rq->out;
                os  = out->specific;

                if (os->state == 1) {
                        os->state = 2;
                } else if (os->state == 3) {
                        os->state = 0;
                } else {
                        __error__("invalid state");
                        goto error;
                }

#ifdef MAD_GM_MARCEL_POLL
                marcel_sem_V(&os->sem);
#else
                os->send_lock  = 0;
#endif
        } else if (rq->in) {
                p_mad_connection_t in = NULL;
                p_mad_gm_connection_specific_t is  = NULL;

                in = rq->in;
                is = in->specific;

#ifdef MAD_GM_MARCEL_POLL
                marcel_sem_V(&is->sem);
#else
                is->send_lock = 0;
#endif
        } else {
                __error__("invalid request");
                goto error;
        }

        LOG_OUT();
        return;

 error:
        FAILURE("mad_gm_callback failed");
        LOG_OUT();
}

static
void
mad_gm_fill_header(void       *_hdr,
                   tbx_bool_t  _incoming,
                   tbx_bool_t  _cpy,
                   size_t      _len,
                   int         _remote_id)
{
        unsigned char *const hdr       = _hdr;
        const unsigned int   incoming  = (_incoming?1:0) << 0;
        const unsigned int   cpy       = (_cpy?1:0)      << 1;
        const unsigned int   len       = (_len)          << 2;
        const unsigned int   remote_id = _remote_id;
        const unsigned int   data      = len | cpy | incoming;

        LOG_IN();
        hdr[0] = (data      >>  0) & 0xFF;
        hdr[1] = (data      >>  8) & 0xFF;
        hdr[2] = (data      >> 16) & 0xFF;
        hdr[3] = (data      >> 24) & 0xFF;
        hdr[4] = (remote_id >>  0) & 0xFF;
        hdr[5] = (remote_id >>  8) & 0xFF;
        hdr[6] = (remote_id >> 16) & 0xFF;
        hdr[7] = (remote_id >> 24) & 0xFF;
        LOG_OUT();
}


static
void
mad_gm_port_poll(p_mad_gm_port_t port) {
	struct gm_port  *p_gm_port = NULL;
        gm_recv_event_t *p_event   = NULL;

        LOG_IN();
        p_gm_port = port->p_gm_port;

#ifdef MAD_GM_MARCEL_POLL
        p_event = gm_receive(port->p_gm_port);
#else
        p_event = gm_blocking_receive(port->p_gm_port);
#endif

        switch (gm_ntohc(p_event->recv.type)) {

        case GM_FAST_HIGH_PEER_RECV_EVENT:
        case GM_FAST_HIGH_RECV_EVENT:
                {
                        int            code           =    0;
                        int            cnx_id         =    0;
                        unsigned char *msg            = NULL;
                        unsigned char *packet         = NULL;
                        int            remote_node_id =    0;

                        TRACE("mad_gm_port_poll: got a packet");

                        packet = gm_ntohp(p_event->recv.buffer);
                        msg = gm_ntohp(p_event->recv.message);

                        TRACE("mad_gm_port_poll: - packet is a control message");

                        code   |= (int)(((unsigned int)msg[0]) <<  0);
                        code   |= (int)(((unsigned int)msg[1]) <<  8);
                        code   |= (int)(((unsigned int)msg[2]) << 16);
                        code   |= (int)(((unsigned int)msg[3]) << 24);
                        cnx_id |= (int)(((unsigned int)msg[4]) <<  0);
                        cnx_id |= (int)(((unsigned int)msg[5]) <<  8);
                        cnx_id |= (int)(((unsigned int)msg[6]) << 16);
                        cnx_id |= (int)(((unsigned int)msg[7]) << 24);

                        remote_node_id =
                                gm_ntohs(p_event->recv.sender_node_id);

                        if ((code & 1) == 0) {
                                p_mad_connection_t             in = NULL;
                                p_mad_gm_connection_specific_t is = NULL;

                                TRACE("mad_gm_port_poll: - packet is for an incoming connection");
                                in = tbx_darray_get(port->in_darray, cnx_id);
                                is = in->specific;

                                if ((code & 2) == 0) {
                                        TRACE("mad_gm_port_poll: - packet is a request for rendez-vous");
                                        gm_provide_receive_buffer_with_tag(port->p_gm_port,
                                                                           port->packet,
                                                                           port->packet_size,
                                                                           GM_HIGH_PRIORITY, 1);


#ifdef MAD_GM_MARCEL_POLL
                                        marcel_sem_V(&is->sem);
#else
                                        is->receive_lock = 0;
#endif
                                } else {
                                        void           *ptr = NULL;
                                        size_t          len = 0;
                                        p_mad_buffer_t  b   = NULL;

                                        TRACE("mad_gm_port_poll: - packet is an eager data transfer");
                                        ptr = port->packet;
                                        len = ((unsigned int)code) >> 2;
                                        TRACE("mad_gm_port_poll: - packet data length = %d", len);

                                        gm_memorize_message(msg, ptr, len+MAD_GM_PACKET_HDR);

                                        b                = mad_alloc_buffer_struct();
                                        b->buffer        = ptr + MAD_GM_PACKET_HDR;
                                        b->length        = len;
                                        b->bytes_written = len;
                                        b->bytes_read    = 0;
                                        b->type          = mad_static_buffer;
                                        b->specific      = ptr;
                                        tbx_slist_append(is->cpy_buffer_slist, b);


                                        if (tbx_slist_get_length(port->packet_cache)) {
                                                port->packet = tbx_slist_pop(port->packet_cache);
                                        } else {
                                                port->packet = gm_dma_malloc(p_gm_port, MAD_GM_PACKET_LEN);
                                        }

                                        if (!port->packet) {
                                                __error__("could not allocate a dma buffer");
                                                goto error;
                                        }

                                        gm_provide_receive_buffer_with_tag(port->p_gm_port,
                                                                           port->packet,
                                                                           port->packet_size,
                                                                           GM_HIGH_PRIORITY, 1);
#ifdef MAD_GM_MARCEL_POLL
                                        marcel_sem_V(&is->sem);
#else
                                        is->receive_lock = 0;
#endif
                                }
                        } else if ((code & 1) == 1) {
                                p_mad_connection_t             out = NULL;
                                p_mad_gm_connection_specific_t os  = NULL;

                                TRACE("mad_gm_port_poll: - packet is for an outgoing connection");
                                TRACE("mad_gm_port_poll: - packet is an ack for rendez-vous");
                                gm_provide_receive_buffer_with_tag(port->p_gm_port,
                                                                   port->packet,
                                                                   port->packet_size,
                                                                   GM_HIGH_PRIORITY, 1);

                                out = tbx_darray_get(port->out_darray, cnx_id);
                                os = out->specific;

#ifdef MAD_GM_MARCEL_POLL
                                marcel_sem_V(&os->sem);
#else
                                os->receive_lock = 0;
#endif
                        } else {
                                __error__("invalid code");
                                goto error;
                        }
                }
                break;

        case GM_HIGH_PEER_RECV_EVENT:
        case GM_HIGH_RECV_EVENT:
                {
                        int            code           =    0;
                        int            cnx_id         =    0;
                        unsigned char *msg            = NULL;
                        unsigned char *packet         = NULL;
                        int            remote_node_id =    0;

                        TRACE("mad_gm_port_poll: got a packet");

                        packet = gm_ntohp(p_event->recv.buffer);
                        msg = gm_ntohp(p_event->recv.message);

                        TRACE("mad_gm_port_poll: - packet is a control message");

                        code   |= (int)(((unsigned int)packet[0]) <<  0);
                        code   |= (int)(((unsigned int)packet[1]) <<  8);
                        code   |= (int)(((unsigned int)packet[2]) << 16);
                        code   |= (int)(((unsigned int)packet[3]) << 24);
                        cnx_id |= (int)(((unsigned int)packet[4]) <<  0);
                        cnx_id |= (int)(((unsigned int)packet[5]) <<  8);
                        cnx_id |= (int)(((unsigned int)packet[6]) << 16);
                        cnx_id |= (int)(((unsigned int)packet[7]) << 24);

                        remote_node_id =
                                gm_ntohs(p_event->recv.sender_node_id);

                        if ((code & 1) == 0) {
                                p_mad_connection_t             in = NULL;
                                p_mad_gm_connection_specific_t is = NULL;

                                TRACE("mad_gm_port_poll: - packet is for an incoming connection");
                                in = tbx_darray_get(port->in_darray, cnx_id);
                                is = in->specific;

                                if ((code & 2) == 0) {
                                        TRACE("mad_gm_port_poll: - packet is a request for rendez-vous");
                                        gm_provide_receive_buffer_with_tag(port->p_gm_port,
                                                                           port->packet,
                                                                           port->packet_size,
                                                                           GM_HIGH_PRIORITY, 1);


#ifdef MAD_GM_MARCEL_POLL
                                        marcel_sem_V(&is->sem);
#else
                                        is->receive_lock = 0;
#endif
                                } else {
                                        void           *ptr = NULL;
                                        size_t          len = 0;
                                        p_mad_buffer_t  b   = NULL;

                                        TRACE("mad_gm_port_poll: - packet is an eager data transfer");
                                        ptr = port->packet;
                                        len = ((unsigned int)code) >> 2;
                                        TRACE("mad_gm_port_poll: - packet data length = %d", len);

                                        b                = mad_alloc_buffer_struct();
                                        b->buffer        = ptr + MAD_GM_PACKET_HDR;
                                        b->length        = len;
                                        b->bytes_written = len;
                                        b->bytes_read    = 0;
                                        b->type          = mad_static_buffer;
                                        b->specific      = ptr;
                                        tbx_slist_append(is->cpy_buffer_slist, b);

                                        if (tbx_slist_get_length(port->packet_cache)) {
                                                port->packet = tbx_slist_pop(port->packet_cache);
                                        } else {
                                                port->packet = gm_dma_malloc(p_gm_port, MAD_GM_PACKET_LEN);
                                        }

                                        if (!port->packet) {
                                                __error__("could not allocate a dma buffer");
                                                goto error;
                                        }

                                        gm_provide_receive_buffer_with_tag(port->p_gm_port,
                                                                           port->packet,
                                                                           port->packet_size,
                                                                           GM_HIGH_PRIORITY, 1);
#ifdef MAD_GM_MARCEL_POLL
                                        marcel_sem_V(&is->sem);
#else
                                        is->receive_lock = 0;
#endif
                                }
                        } else if ((code & 1) == 1) {
                                p_mad_connection_t             out = NULL;
                                p_mad_gm_connection_specific_t os  = NULL;

                                TRACE("mad_gm_port_poll: - packet is for an outgoing connection");
                                TRACE("mad_gm_port_poll: - packet is an ack for rendez-vous");
                                gm_provide_receive_buffer_with_tag(port->p_gm_port,
                                                                   port->packet,
                                                                   port->packet_size,
                                                                   GM_HIGH_PRIORITY, 1);

                                out = tbx_darray_get(port->out_darray, cnx_id);
                                os = out->specific;

#ifdef MAD_GM_MARCEL_POLL
                                marcel_sem_V(&os->sem);
#else
                                os->receive_lock = 0;
#endif
                        } else {
                                __error__("invalid code");
                                goto error;
                        }
                }
                break;

        case GM_PEER_RECV_EVENT:
        case GM_RECV_EVENT:
                {
                        p_mad_connection_t             in = NULL;
                        p_mad_gm_connection_specific_t is = NULL;

                        TRACE("mad_gm_port_poll: got a packet");
                        TRACE("mad_gm_port_poll: - packet is a rendez-vous data message");
                        in = port->active_input;
			is = in->specific;
#ifdef MAD_GM_MARCEL_POLL
                        marcel_sem_V(&is->sem);
#else
                        is->receive_lock = 0;
#endif
                }
                break;

        case GM_NO_RECV_EVENT:
                break;

        default:
                gm_unknown(port->p_gm_port, p_event);
                break;
        }
        LOG_OUT();
        return;

 error:
        FAILURE("mad_gm_port_poll failed");
        LOG_OUT();
}

#ifdef MAD_GM_MARCEL_POLL
static void
mad_gm_marcel_group(marcel_pollid_t id)
{
  return;
}

static
int
mad_gm_do_poll(p_mad_gm_poll_req_t rq) {
        p_mad_gm_port_t port = NULL;

        LOG_IN();
        if (!TBX_CRITICAL_SECTION_TRY_ENTERING(mad_gm_access)) {
                goto end;
        }

        port = rq->port;
        mad_gm_port_poll(port);

        if (rq->op == mad_gm_poll_channel_op) {
                p_mad_channel_t                ch        = rq->data.channel_op.ch;
                p_mad_gm_channel_specific_t    chs       = ch->specific;
                p_mad_connection_t             in        = NULL;
                p_tbx_darray_t                 in_darray = NULL;
                int                            r         =    0;
                int                            max       =    0;
                int                            i         =    0;
                int                            next      =    0;

                in_darray = ch->in_connection_darray;
                next = chs->next;
                max  = chs->size;
                i    = max;

                while (i--) {
                        next = (next + 1) % max;
                        in   = tbx_darray_get(in_darray, next);

                        if (in) {
                                p_mad_gm_connection_specific_t is = NULL;

                                is = in->specific;
                                r = marcel_sem_try_P(&is->sem);
                                if (r) {
                                        rq->data.channel_op.c = in;
                                        break;
                                }
                        }
                }

                chs->next = next;

                TBX_CRITICAL_SECTION_LEAVE(mad_gm_access);

                return r;
        } else if (rq->op == mad_gm_poll_sem_op) {
                marcel_sem_t *s = rq->data.sem_op.sem;
                int             r = 0;

                r = marcel_sem_try_P(s);
                TBX_CRITICAL_SECTION_LEAVE(mad_gm_access);

                return r;
        } else
                FAILURE("invalid operation");

end:
        LOG_OUT();

        return 0;
}

static void *
mad_gm_marcel_fast_poll(marcel_pollid_t id,
                        any_t           req,
                        boolean         first_call) {
        void *status = MARCEL_POLL_FAILED;

        LOG_IN();
        if (mad_gm_do_poll((p_mad_gm_poll_req_t) req)) {
                status = MARCEL_POLL_SUCCESS(id);
        }
        LOG_OUT();

        return status;
}

static void *
mad_gm_marcel_poll(marcel_pollid_t id,
                   unsigned        active,
                   unsigned        sleeping,
                   unsigned        blocked) {
        p_mad_gm_poll_req_t  req = NULL;
        void                *status = MARCEL_POLL_FAILED;

        LOG_IN();
        FOREACH_POLL(id) { 
		GET_ARG(id, req);
                if (mad_gm_do_poll((p_mad_gm_poll_req_t) req)) {
                        status = MARCEL_POLL_SUCCESS(id);
                        goto found;
                }
        }

 found:
        LOG_OUT();

        return status;
}

static
void
mad_gm_wait_for_sem(marcel_pollid_t  pollid,
		    p_mad_gm_port_t  port,
		    marcel_sem_t  *m) {
        LOG_IN();
        if (!marcel_sem_try_P(m)) {
                mad_gm_poll_req_t req;

                req.op       = mad_gm_poll_sem_op;
                req.port     = port;
                req.data.sem_op.sem = m;

                marcel_poll(pollid, &req);
        }
        LOG_OUT();
}

#endif

static
p_mad_gm_port_t
mad_gm_port_open(int device_id) {
	gm_status_t      gms       = GM_SUCCESS;
        p_mad_gm_port_t  port      =       NULL;
	struct gm_port  *p_gm_port =       NULL;
	int              port_id   =          0;
	unsigned int     node_id   =          0;
        int              n         =          0;

        LOG_IN();
        while ((port_id = mad_gm_pub_port_array[n]) != -1) {
                gms = gm_open(&p_gm_port, device_id, port_id,
                              "mad_gm", GM_API_VERSION_1_1);
                if (gms == GM_SUCCESS) {
                        goto found;
                }

                if (gms != GM_BUSY) {
                        __error__("gm_open failed");
                        __gmerror__(gms);
                        goto error;
                }
                n++;
	}

        __error__("no more GM ports");
        goto error;


 found:
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

#if defined (GM_API_VERSION_2_0) && GM_API_VERSION >= GM_API_VERSION_2_0
	gms = gm_node_id_to_unique_id(p_gm_port, node_id, port->local_unique_node_id);

	if (gms != GM_SUCCESS) {
                __error__("gm_node_id_to_unique_id failed");
		__gmerror__(gms);
                goto error;
	}
#endif

        port->out_darray    = tbx_darray_init();
        port->in_darray     = tbx_darray_init();
        port->packet        = gm_dma_malloc(p_gm_port, MAD_GM_PACKET_LEN);
        if (!port->packet) {
                __error__("could not allocate a dma buffer");
                goto error;
        }

        port->packet_length = MAD_GM_PACKET_LEN;
        port->packet_size   = gm_min_size_for_length(port->packet_length);
        port->packet_cache  = tbx_slist_nil();

        gm_provide_receive_buffer_with_tag(port->p_gm_port, port->packet,
                                           port->packet_size,
                                           GM_HIGH_PRIORITY, 1);
        LOG_OUT();
        return port;

 error:
        FAILURE("mad_gm_port_open failed");
        return NULL;
}



void
mad_gm_register(p_mad_driver_t driver)
{
        p_mad_driver_interface_t interface = NULL;

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
        interface->choice                     = mad_gm_choice;
        interface->get_static_buffer          = mad_gm_get_static_buffer;
        interface->return_static_buffer       = mad_gm_return_static_buffer;
        interface->new_message                = NULL;
        interface->finalize_message           = NULL;
#ifdef MAD_MESSAGE_POLLING
        interface->poll_message               = mad_gm_poll_message;
#endif // MAD_MESSAGE_POLLING
        interface->receive_message            = mad_gm_receive_message;
        interface->message_received           = NULL;
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
#if MAD_GM_MEMORY_CACHE
        if (!mad_gm_malloc_hooked) {
                mad_gm_malloc_initialize_hook();
        }
#endif // MAD_GM_MEMORY_CACHE

        ds          = TBX_MALLOC(sizeof(mad_gm_driver_specific_t));

#ifdef MAD_GM_MARCEL_POLL
        ds->gm_pollid =
                marcel_pollid_create(mad_gm_marcel_group,
                                     mad_gm_marcel_poll,
                                     mad_gm_marcel_fast_poll,
                                     MAD_GM_POLLING_MODE);
#endif

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
        p_mad_gm_port_t             port      = NULL;
        int                         device_id =    0;

        LOG_IN();
        if (tbx_streq(a->dir_adapter->name, "default")) {
                device_id = 0;
        } else {
                device_id = strtol(a->dir_adapter->name, NULL, 0);
        }

        TBX_CRITICAL_SECTION_ENTER(mad_gm_access);
        port = mad_gm_port_open(device_id);
        TBX_CRITICAL_SECTION_LEAVE(mad_gm_access);
        if (!port) {
                __error__("mad_gm_open_port failed");
                goto error;
        }

        port->adapter = a;

        as            = TBX_MALLOC(sizeof(mad_gm_adapter_specific_t));
        as->device_id = device_id;
        as->port      = port;

        param_str    = tbx_string_init_to_int(device_id);
        tbx_string_append_char(param_str, ' ');
        tbx_string_append_int(param_str,  port->local_port_id);
        tbx_string_append_char(param_str, ' ');
        tbx_string_append_int(param_str,  port->local_node_id);

        {
                int i = 0;
                for (i = 0; i < 6; i++) {
                        tbx_string_append_char(param_str, ' ');
                        tbx_string_append_int(param_str,  (int)port->local_unique_node_id[i]);
                        //
                }
        }

        a->parameter = tbx_string_to_cstring(param_str);
        a->mtu       = MAD_FORWARD_MAX_MTU;
        a->specific  = as;

        tbx_string_free(param_str);
        param_str    = NULL;
        LOG_OUT();
        return;

 error:
        FAILURE("mad_gm_adapter_init failed");
        LOG_OUT();
}

void
mad_gm_channel_init(p_mad_channel_t ch) {
        p_mad_gm_channel_specific_t chs = NULL;
        p_mad_dir_channel_t         dch = NULL;

        LOG_IN();
        dch          = ch->dir_channel;
        chs          = TBX_MALLOC(sizeof(mad_gm_channel_specific_t));
        chs->size    = 1 + ntbx_pc_local_max(dch->pc);
        chs->next    = 0;
        ch->specific = chs;
        LOG_OUT();
}

void
mad_gm_connection_init(p_mad_connection_t in,
		       p_mad_connection_t out) {
        p_mad_channel_t             ch    = NULL;
        p_mad_adapter_t             a     = NULL;
        p_mad_gm_adapter_specific_t as    = NULL;
        p_mad_gm_port_t             port  = NULL;
        const int nb_link = 2;

        LOG_IN();
        ch   = (in?:out)->channel;
        a    =  ch->adapter;
        as   =  a->specific;
        port =  as->port;

        if (in) {
                p_mad_gm_connection_specific_t is = NULL;

                is           =
                        TBX_CALLOC(1, sizeof(mad_gm_connection_specific_t));
                in->specific = is;
                in->nb_link  = nb_link;

                is->buffer          =       NULL;
                is->length          =          0;
                is->local_id        =         -1;
                is->remote_id       =         -1;
                is->remote_dev_id   =         -1;
                is->remote_port_id  =         -1;
                is->remote_node_id  =          0;
                is->first           =          1;
                is->cache           =       NULL;
                is->request         =
                        TBX_CALLOC(1, sizeof(mad_gm_connection_specific_t));
                is->request->in     = in;
                is->request->out    =       NULL;
                is->request->status = GM_SUCCESS;
                is->request->port   = port;
                is->state           = 0;
                is->cpy_buffer_slist = tbx_slist_nil();
                TBX_CRITICAL_SECTION_ENTER(mad_gm_access);
                is->packet          = gm_dma_malloc(port->p_gm_port,
                                                    MAD_GM_PACKET_HDR);
                TBX_CRITICAL_SECTION_LEAVE(mad_gm_access);

                if (!is->packet) {
                        __error__("could not allocate a dma buffer");
                        goto error;
                }

                is->packet_length = MAD_GM_PACKET_HDR;
                is->packet_size   = gm_min_size_for_length(MAD_GM_PACKET_LEN);

#ifdef MAD_GM_MARCEL_POLL
                marcel_sem_init(&is->sem, 0);
#else
                is->send_lock    = 1;
                is->receive_lock = 1;
#endif

                {
                        p_tbx_darray_t darray    = port->in_darray;
                        p_tbx_string_t param_str = NULL;

                        TBX_LOCK_SHARED(darray);
                        is->local_id = tbx_darray_length(darray);
                        tbx_darray_expand_and_set(darray, is->local_id, in);
                        TBX_UNLOCK_SHARED(darray);

                        param_str    = tbx_string_init_to_int(is->local_id);
                        in->parameter = tbx_string_to_cstring(param_str);
                        tbx_string_free(param_str);
                        param_str    = NULL;
                }
        }

        if (out) {
                p_mad_gm_connection_specific_t os = NULL;

                os            = TBX_CALLOC(1, sizeof(mad_gm_connection_specific_t));
                out->specific = os;
                out->nb_link  = nb_link;

                os->buffer          =       NULL;
                os->length          =          0;
                os->local_id        =         -1;
                os->remote_id       =         -1;
                os->remote_dev_id   =         -1;
                os->remote_port_id  =         -1;
                os->remote_node_id  =          0;
                os->first           =          1;
                os->cache           =       NULL;
                os->request         =
                        TBX_CALLOC(1, sizeof(mad_gm_connection_specific_t));
                os->request->in     =       NULL;
                os->request->out    = out;
                os->request->status = GM_SUCCESS;
                os->request->port   = port;
                os->state           = 0;
                os->cpy_buffer_slist = tbx_slist_nil();
                TBX_CRITICAL_SECTION_ENTER(mad_gm_access);
                os->packet = gm_dma_malloc(port->p_gm_port, MAD_GM_PACKET_HDR);
                TBX_CRITICAL_SECTION_LEAVE(mad_gm_access);

                if (!os->packet) {
                        __error__("could not allocate a dma buffer");
                        goto error;
                }

                os->packet_length = MAD_GM_PACKET_HDR;
                os->packet_size   = gm_min_size_for_length(MAD_GM_PACKET_LEN);

#ifdef MAD_GM_MARCEL_POLL
                marcel_sem_init(&os->sem, 0);
#else
                os->send_lock    = 1;
                os->receive_lock = 1;
#endif

                {
                        p_tbx_darray_t darray    = port->out_darray;
                        p_tbx_string_t param_str = NULL;

                        TBX_LOCK_SHARED(darray);
                        os->local_id = tbx_darray_length(darray);
                        tbx_darray_expand_and_set(darray, os->local_id, out);
                        TBX_UNLOCK_SHARED(darray);

                        param_str    = tbx_string_init_to_int(os->local_id);
                        out->parameter = tbx_string_to_cstring(param_str);
                        tbx_string_free(param_str);
                        param_str    = NULL;
                }
        }
        LOG_OUT();
        return;

 error:
        FAILURE("mad_gm_connection_init failed");
}

void
mad_gm_link_init(p_mad_link_t l) {
        LOG_IN();
        if (l->id == MAD_GM_LINK_CPY) {
                p_mad_gm_link_specific_t ls = NULL;

                ls 	       = TBX_CALLOC(1, sizeof(mad_gm_link_specific_t));
                l->specific    = ls;
                l->link_mode   = mad_link_mode_buffer;
                l->buffer_mode = mad_buffer_mode_static;
                l->group_mode  = mad_group_mode_split;
        } else if (l->id == MAD_GM_LINK_RDV) {
                p_mad_gm_link_specific_t ls = NULL;

                ls 	       = TBX_CALLOC(1, sizeof(mad_gm_link_specific_t));
                l->specific    = ls;
                l->link_mode   = mad_link_mode_buffer;
                l->buffer_mode = mad_buffer_mode_dynamic;
                l->group_mode  = mad_group_mode_split;
        } else
                FAILURE("invalid link id");

        LOG_OUT();
}

void
mad_gm_accept(p_mad_connection_t   in,
	      p_mad_adapter_info_t ai) {
        p_mad_gm_connection_specific_t is  = NULL;

        LOG_IN();
	is = in->specific;
        mad_gm_extract_info(ai, in);
        is->packet[0] = 1;
        is->packet[1] = 0;
        is->packet[2] = 0;
        is->packet[3] = 0;
        is->packet[4] = (is->remote_id >>  0) & 0xFF;
        is->packet[5] = (is->remote_id >>  8) & 0xFF;
        is->packet[6] = (is->remote_id >> 16) & 0xFF;
        is->packet[7] = (is->remote_id >> 24) & 0xFF;
        LOG_OUT();
}

void
mad_gm_connect(p_mad_connection_t   out,
	       p_mad_adapter_info_t ai) {
        p_mad_gm_connection_specific_t os = NULL;

        LOG_IN();
	os = out->specific;
        mad_gm_extract_info(ai, out);
        os->packet[0] = 0;
        os->packet[1] = 0;
        os->packet[2] = 0;
        os->packet[3] = 0;
        os->packet[4] = (os->remote_id >>  0) & 0xFF;
        os->packet[5] = (os->remote_id >>  8) & 0xFF;
        os->packet[6] = (os->remote_id >> 16) & 0xFF;
        os->packet[7] = (os->remote_id >> 24) & 0xFF;
        LOG_OUT();
}

void
mad_gm_disconnect(p_mad_connection_t cnx) {
        LOG_IN();
        //FAILURE("unimplemented");
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

p_mad_link_t
mad_gm_choice(p_mad_connection_t connection,
              size_t             size,
              mad_send_mode_t    send_mode,
              mad_receive_mode_t receive_mode)
{
        p_mad_link_t lnk = NULL;

        LOG_IN();
#if 1
        if (size < MAD_GM_PACKET_LEN-MAD_GM_PACKET_HDR) {
                 lnk = connection->link_array[MAD_GM_LINK_CPY];
        } else {
                 lnk = connection->link_array[MAD_GM_LINK_RDV];
        }
#else
        lnk = connection->link_array[MAD_GM_LINK_RDV];
#endif
        LOG_OUT();

        return lnk;
}

#if 0
void
mad_gm_new_message(p_mad_connection_t out) {
        p_mad_gm_connection_specific_t os = NULL;

        LOG_IN();
        os = out->specific;
        FAILURE("unimplemented");
        LOG_OUT();
}
#endif

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
        p_mad_gm_channel_specific_t chs    = NULL;
        p_tbx_darray_t              darray = NULL;
        p_mad_connection_t          in     = NULL;
        p_mad_adapter_t             a      = NULL;
        p_mad_gm_adapter_specific_t as     = NULL;
        p_mad_driver_t              d      = NULL;
        p_mad_gm_driver_specific_t  ds     = NULL;
        p_mad_gm_port_t             port   = NULL;

        LOG_IN();
        chs    = ch->specific;
        a      = ch->adapter;
        as     = a->specific;
        d      = a->driver;
        ds     = d->specific;
        port   = as->port;
        darray = ch->in_connection_darray;

#ifdef MAD_GM_MARCEL_POLL
        {
	        p_mad_gm_connection_specific_t is = NULL;
                mad_gm_poll_req_t req;

                req.op                 = mad_gm_poll_channel_op;
                req.port               = port;
                req.data.channel_op.ch = ch;
                req.data.channel_op.c  = NULL;

                marcel_poll(ds->gm_pollid, &req);

                in = req.data.channel_op.c;
		is = in->specific;
		is->first = 1;
        }
#else
        while (1) {
                p_mad_connection_t _in = NULL;

                _in = tbx_darray_get(darray, chs->next);
                chs->next = (chs->next + 1) % tbx_darray_length(darray);

                if (_in) {
                        p_mad_gm_connection_specific_t _is = _in->specific;

                        if (_is->receive_lock) {
                                mad_gm_port_poll(port);
                        }

                        if (!_is->receive_lock) {
                                _is->receive_lock =   1;
                                _is->first	  =   1;
                                in        	  = _in;

                                break;
                        }
                }
        }
#endif
        LOG_OUT();

        return in;
}

#if 0
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
#endif

static
void
mad_gm_send_buffer_cpy(p_mad_link_t   l,
                       p_mad_buffer_t b) {
        p_mad_connection_t             out  = NULL;
        p_mad_gm_connection_specific_t os   = NULL;
        p_mad_adapter_t                a    = NULL;
        p_mad_gm_adapter_specific_t    as   = NULL;
        p_mad_driver_t                 d    = NULL;
        p_mad_gm_driver_specific_t     ds   = NULL;
        p_mad_gm_port_t                port = NULL;

        LOG_IN();
        out  = l->connection;
        os   = out->specific;
        a    = out->channel->adapter;
        as   = a->specific;
        d    = a->driver;
        ds   = d->specific;

        port = as->port;

        os->state = 3;
        os->request->status = GM_SUCCESS;

        mad_gm_fill_header(b->specific,
                           tbx_false,
                           tbx_true,
                           b->bytes_written,
                           os->remote_id);

        TBX_CRITICAL_SECTION_ENTER(mad_gm_access);
	TBX_LOCK();
        gm_send_with_callback(port->p_gm_port, b->specific,
                              os->packet_size, b->bytes_written+MAD_GM_PACKET_HDR,
                              GM_HIGH_PRIORITY,
                              os->remote_node_id, os->remote_port_id,
                              mad_gm_callback, os->request);

	TBX_UNLOCK();
        TBX_CRITICAL_SECTION_LEAVE(mad_gm_access);
        b->bytes_read = b->bytes_written;

#ifdef MAD_GM_MARCEL_POLL
        mad_gm_wait_for_sem(ds->gm_pollid, port, &os->sem);
#else
        while (os->send_lock) {
                mad_gm_port_poll(port);
        }

        os->send_lock = 1;
#endif

        TBX_CRITICAL_SECTION_ENTER(mad_gm_access);
        if (tbx_slist_get_length(as->port->packet_cache) < MAD_GM_PACKET_CACHE_SIZE) {
                tbx_slist_push(as->port->packet_cache, b->specific);
        } else {
                gm_dma_free(as->port->p_gm_port, b->specific);
        }
        TBX_CRITICAL_SECTION_LEAVE(mad_gm_access);
        os->buffer = NULL;
        LOG_OUT();
}

static
void
mad_gm_receive_buffer_cpy(p_mad_link_t     l,
                          p_mad_buffer_t *pb) {
        p_mad_connection_t             in   = NULL;
        p_mad_gm_connection_specific_t is   = NULL;
        p_mad_adapter_t                a    = NULL;
        p_mad_gm_adapter_specific_t    as   = NULL;
        p_mad_driver_t                 d    = NULL;
        p_mad_gm_driver_specific_t     ds   = NULL;
        p_mad_gm_port_t                port = NULL;

        LOG_IN();
        in   = l->connection;
        is   = in->specific;
        a    = in->channel->adapter;
        as   = a->specific;
        d    = a->driver;
        ds   = d->specific;
        port = as->port;

        if (is->first) {
                is->first = 0;
        } else {
#ifdef MAD_GM_MARCEL_POLL
		mad_gm_wait_for_sem(ds->gm_pollid, port, &is->sem);
#else
                while (is->receive_lock) {
                        mad_gm_port_poll(port);
                }

                is->receive_lock = 1;
#endif
        }

        *pb = tbx_slist_extract(is->cpy_buffer_slist);
        LOG_OUT();
}

static
void
mad_gm_send_buffer_group_cpy_1(p_mad_link_t         l,
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

static
void
mad_gm_receive_sub_buffer_group_cpy_1(p_mad_link_t         l,
				  p_mad_buffer_group_t bg) {
        LOG_IN();
        if (!tbx_empty_list(&(bg->buffer_list))) {
                tbx_list_reference_t ref;

                tbx_list_reference_init(&ref, &(bg->buffer_list));
                do {
                        p_mad_buffer_t buffer = NULL;

                        buffer = tbx_get_list_reference_object(&ref);
                        mad_gm_receive_buffer(l, &buffer);
                } while (tbx_forward_list_reference(&ref));
        }
        LOG_OUT();
}

static
void
mad_gm_send_buffer_group_cpy(p_mad_link_t         l,
                             p_mad_buffer_group_t bg) {
        LOG_IN();
        mad_gm_send_buffer_group_cpy_1(l, bg);
        LOG_OUT();
}

static
void
mad_gm_receive_sub_buffer_group_cpy(p_mad_link_t         l,
                                    tbx_bool_t           first_sub_group
                                    TBX_UNUSED,
                                    p_mad_buffer_group_t bg) {
        LOG_IN();
        mad_gm_receive_sub_buffer_group_cpy_1(l, bg);
        LOG_OUT();
}

static
p_mad_buffer_t
mad_gm_get_static_buffer_cpy(p_mad_link_t l)
{
        p_mad_gm_adapter_specific_t as = NULL;
        p_mad_buffer_t  b   = NULL;
        void           *ptr = NULL;

        LOG_IN();
        as = l->connection->channel->adapter->specific;

        TBX_CRITICAL_SECTION_ENTER(mad_gm_access);
        if (tbx_slist_get_length(as->port->packet_cache)) {
                ptr = tbx_slist_pop(as->port->packet_cache);
        } else {
                ptr = gm_dma_malloc(as->port->p_gm_port, MAD_GM_PACKET_LEN);
        }
        TBX_CRITICAL_SECTION_LEAVE(mad_gm_access);

        if (!ptr) {
                __error__("could not allocate a dma buffer");
                goto error;
        }

	b                = mad_alloc_buffer_struct();
	b->buffer        = ptr + MAD_GM_PACKET_HDR;
	b->length        = MAD_GM_PACKET_LEN - MAD_GM_PACKET_HDR;
	b->bytes_written = 0;
	b->bytes_read    = 0;
	b->type          = mad_static_buffer;
	b->specific      = ptr;
        LOG_OUT();

        return b;

 error:
        FAILURE("mad_gm_get_static_buffer_cpy failed");

        LOG_OUT();

        return NULL;
}

static
void
mad_gm_return_static_buffer_cpy(p_mad_link_t   l,
                                p_mad_buffer_t b)
{
        p_mad_gm_adapter_specific_t as = NULL;

        LOG_IN();
        as = l->connection->channel->adapter->specific;

        TBX_CRITICAL_SECTION_ENTER(mad_gm_access);
        if (tbx_slist_get_length(as->port->packet_cache) < MAD_GM_PACKET_CACHE_SIZE) {
                tbx_slist_push(as->port->packet_cache, b->specific);
        } else {
                gm_dma_free(as->port->p_gm_port, b->specific);
        }
        TBX_CRITICAL_SECTION_LEAVE(mad_gm_access);

        b->buffer        = 0;
        b->length        = 0;
        b->bytes_written = 0;
        b->bytes_read    = 0;
        b->type          = 0;
        b->specific      = 0;
        LOG_OUT();
}

static
void
mad_gm_send_buffer_rdv(p_mad_link_t   l,
                       p_mad_buffer_t b) {
        p_mad_connection_t             out  = NULL;
        p_mad_gm_connection_specific_t os   = NULL;
        p_mad_adapter_t                a    = NULL;
        p_mad_gm_adapter_specific_t    as   = NULL;
        p_mad_driver_t                 d    = NULL;
        p_mad_gm_driver_specific_t     ds   = NULL;
        p_mad_gm_port_t                port = NULL;

        LOG_IN();
        out  = l->connection;
        os   = out->specific;
        a    = out->channel->adapter;
        as   = a->specific;
        d    = a->driver;
        ds   = d->specific;

        port = as->port;

        os->state = 1;
        os->request->status = GM_SUCCESS;

        TBX_CRITICAL_SECTION_ENTER(mad_gm_access);
	TBX_LOCK();
        gm_send_with_callback(port->p_gm_port, os->packet,
                              os->packet_size, os->packet_length,
                              GM_HIGH_PRIORITY,
                              os->remote_node_id, os->remote_port_id,
                              mad_gm_callback, os->request);

        mad_gm_register_block(port, b->buffer, b->length, &os->cache);
	TBX_UNLOCK();
        TBX_CRITICAL_SECTION_LEAVE(mad_gm_access);


#ifdef MAD_GM_MARCEL_POLL
        mad_gm_wait_for_sem(ds->gm_pollid, port, &os->sem);
#else
        while (os->receive_lock) {
                mad_gm_port_poll(port);
        }

        os->receive_lock = 1;
#endif


#ifdef MAD_GM_MARCEL_POLL
        mad_gm_wait_for_sem(ds->gm_pollid, port, &os->sem);
#else
        while (os->send_lock) {
                mad_gm_port_poll(port);
        }

        os->send_lock = 1;
#endif


        os->buffer = b;

        while (mad_more_data(b)) {
                int len = tbx_min(MAD_GM_MAX_BLOCK_LEN,
                                  b->bytes_written - b->bytes_read);

                os->length          = len;
                os->request->status = GM_SUCCESS;
                os->state           = 3;

                TBX_CRITICAL_SECTION_ENTER(mad_gm_access);
		TBX_LOCK();
                gm_send_with_callback(port->p_gm_port,
                                      b->buffer + b->bytes_read,
                                      gm_min_size_for_length(MAD_GM_MAX_BLOCK_LEN),
                                      os->length, GM_LOW_PRIORITY,
                                      os->remote_node_id, os->remote_port_id,
                                      mad_gm_callback, os->request);
		TBX_UNLOCK();
                TBX_CRITICAL_SECTION_LEAVE(mad_gm_access);


#ifdef MAD_GM_MARCEL_POLL
                mad_gm_wait_for_sem(ds->gm_pollid, port, &os->sem);
#else
                while (os->send_lock) {
                        mad_gm_port_poll(port);
                }

                os->send_lock = 1;
#endif
                b->bytes_read += len;


        }

        TBX_CRITICAL_SECTION_ENTER(mad_gm_access);
	TBX_LOCK();
        mad_gm_deregister_block(port, os->cache);
	TBX_UNLOCK();
        TBX_CRITICAL_SECTION_LEAVE(mad_gm_access);
        os->cache = NULL;
        os->buffer  = NULL;
        os->length  = 0;
        LOG_OUT();
}

static
void
mad_gm_receive_buffer_rdv(p_mad_link_t     l,
                          p_mad_buffer_t *pb) {
        p_mad_buffer_t                 b    = NULL;
        p_mad_connection_t             in   = NULL;
        p_mad_gm_connection_specific_t is   = NULL;
        p_mad_adapter_t                a    = NULL;
        p_mad_gm_adapter_specific_t    as   = NULL;
        p_mad_driver_t                 d    = NULL;
        p_mad_gm_driver_specific_t     ds   = NULL;
        p_mad_gm_port_t                port = NULL;

        LOG_IN();
        b = *pb;

        in   = l->connection;
        is   = in->specific;
        a    = in->channel->adapter;
        as   = a->specific;
        d    = a->driver;
        ds   = d->specific;
        port = as->port;

        if (is->first) {
                is->first = 0;
        } else {
#ifdef MAD_GM_MARCEL_POLL
		mad_gm_wait_for_sem(ds->gm_pollid, port, &is->sem);
#else
                while (is->receive_lock) {
                        mad_gm_port_poll(port);
                }

                is->receive_lock = 1;
#endif
        }

        TBX_CRITICAL_SECTION_ENTER(mad_gm_reception);
        is->request->status = GM_SUCCESS;
        TBX_CRITICAL_SECTION_ENTER(mad_gm_access);
	TBX_LOCK();
        gm_send_with_callback(port->p_gm_port, is->packet,
                              is->packet_size, is->packet_length,
                              GM_HIGH_PRIORITY,
                              is->remote_node_id, is->remote_port_id,
                              mad_gm_callback, is->request);
        mad_gm_register_block(port, b->buffer, b->length, &is->cache);
	TBX_UNLOCK();
        TBX_CRITICAL_SECTION_LEAVE(mad_gm_access);


#ifdef MAD_GM_MARCEL_POLL
        mad_gm_wait_for_sem(ds->gm_pollid, port, &is->sem);
#else
        while (is->send_lock) {
                mad_gm_port_poll(port);
        }

        is->send_lock = 1;
#endif


        is->buffer = b;
        port->active_input = in;

        while (!mad_buffer_full(b)) {
                int len = tbx_min(MAD_GM_MAX_BLOCK_LEN,
                              b->length - b->bytes_written);
                is->length = len;
                TBX_CRITICAL_SECTION_ENTER(mad_gm_access);
		TBX_LOCK();
                gm_provide_receive_buffer_with_tag(port->p_gm_port,
                                                   b->buffer+b->bytes_written,
                                                   gm_min_size_for_length(MAD_GM_MAX_BLOCK_LEN),
                                                   GM_LOW_PRIORITY, 0);
		TBX_UNLOCK();
                TBX_CRITICAL_SECTION_LEAVE(mad_gm_access);


#ifdef MAD_GM_MARCEL_POLL
                mad_gm_wait_for_sem(ds->gm_pollid, port, &is->sem);
#else
                while (is->receive_lock) {
                        mad_gm_port_poll(port);
                }

                is->receive_lock = 1;
#endif
                b->bytes_written += len;
        }

        TBX_CRITICAL_SECTION_LEAVE(mad_gm_reception);

        TBX_CRITICAL_SECTION_ENTER(mad_gm_access);
	TBX_LOCK();
        mad_gm_deregister_block(port, is->cache);
	TBX_UNLOCK();
        TBX_CRITICAL_SECTION_LEAVE(mad_gm_access);
        is->cache = NULL;
        LOG_OUT();
}

static
void
mad_gm_send_buffer_group_rdv_1(p_mad_link_t         l,
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

static
void
mad_gm_receive_sub_buffer_group_rdv_1(p_mad_link_t         l,
                                      p_mad_buffer_group_t bg) {
        LOG_IN();
        if (!tbx_empty_list(&(bg->buffer_list))) {
                tbx_list_reference_t ref;

                tbx_list_reference_init(&ref, &(bg->buffer_list));
                do {
                        p_mad_buffer_t buffer = NULL;

                        buffer = tbx_get_list_reference_object(&ref);
                        mad_gm_receive_buffer(l, &buffer);
                } while (tbx_forward_list_reference(&ref));
        }
        LOG_OUT();
}

static
void
mad_gm_send_buffer_group_rdv(p_mad_link_t         l,
                             p_mad_buffer_group_t bg) {
        LOG_IN();
        mad_gm_send_buffer_group_rdv_1(l, bg);
        LOG_OUT();
}

static
void
mad_gm_receive_sub_buffer_group_rdv(p_mad_link_t         l,
                                    tbx_bool_t           first_sub_group
                                    TBX_UNUSED,
                                    p_mad_buffer_group_t bg) {
        LOG_IN();
        mad_gm_receive_sub_buffer_group_rdv_1(l, bg);
        LOG_OUT();
}

void
mad_gm_send_buffer(p_mad_link_t   l,
                   p_mad_buffer_t buffer)
{
        LOG_IN();
        if (l->id == MAD_GM_LINK_CPY)
                {
                        mad_gm_send_buffer_cpy(l, buffer);
                }
        else if (l->id == MAD_GM_LINK_RDV)
                {
                        mad_gm_send_buffer_rdv(l, buffer);
                }
        else
                FAILURE("invalid link id");

        LOG_OUT();
}

void
mad_gm_receive_buffer(p_mad_link_t    l,
                      p_mad_buffer_t *pbuffer)
{
        LOG_IN();
        if (l->id == MAD_GM_LINK_CPY)
                {
                        mad_gm_receive_buffer_cpy(l, pbuffer);
                }
        else if (l->id == MAD_GM_LINK_RDV)
                {
                        mad_gm_receive_buffer_rdv(l, pbuffer);
                }
        else
                FAILURE("invalid link id");

        LOG_OUT();
}

/* Buffer group transfer */
void
mad_gm_send_buffer_group(p_mad_link_t         l,
                         p_mad_buffer_group_t buffer_group)
{
        LOG_IN();
        if (l->id == MAD_GM_LINK_CPY)
                {
                        mad_gm_send_buffer_group_cpy(l, buffer_group);
                }
        else if (l->id == MAD_GM_LINK_RDV)
                {
                        mad_gm_send_buffer_group_rdv(l, buffer_group);
                }
        else
                FAILURE("invalid link id");

        LOG_OUT();
}

void
mad_gm_receive_sub_buffer_group(p_mad_link_t         l,
                                tbx_bool_t           first_subgroup TBX_UNUSED,
                                p_mad_buffer_group_t buffer_group)
{
        LOG_IN();
        if (l->id == MAD_GM_LINK_CPY)
                {
                        mad_gm_receive_sub_buffer_group_cpy(l, first_subgroup, buffer_group);
                }
        else if (l->id == MAD_GM_LINK_RDV)
                {
                        mad_gm_receive_sub_buffer_group_rdv(l, first_subgroup, buffer_group);
                }
        else
                FAILURE("invalid link id");

        LOG_OUT();
}

p_mad_buffer_t
mad_gm_get_static_buffer(p_mad_link_t l)
{
        p_mad_buffer_t b = NULL;

        LOG_IN();
        if (l->id == MAD_GM_LINK_CPY)
                {
                        b = mad_gm_get_static_buffer_cpy(l);
                }
        else
                FAILURE("invalid link id");

        LOG_OUT();

        return b;
}

void
mad_gm_return_static_buffer(p_mad_link_t   l,
                            p_mad_buffer_t b)
{
        LOG_IN();
        if (l->id == MAD_GM_LINK_CPY)
                {
                        mad_gm_return_static_buffer_cpy(l, b);
                }
        else
                FAILURE("invalid link id");

        LOG_OUT();
}
