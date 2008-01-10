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
 * Mad_mx.c
 * =========
 */


#include "madeleine.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <myriexpress.h>

// Uncomment the following line if you are using a version >= 1.0.0
#define MX_VERSION_AFTER_1_0_0

#ifdef MX_VERSION_AFTER_1_0_0
#define MX_MATCH_MASK_BC   ((uint64_t)0xffffffff << 32)
#endif

/*
 * macros
 * ------
 */
#ifdef PIOMAN
#include "pioman.h"
#define MAD_MX_POLLING_MODE \
     (PIOM_POLL_AT_TIMER_SIG | PIOM_POLL_AT_YIELD | PIOM_POLL_AT_IDLE)

#elif defined(MARCEL)
#define MAD_MX_POLLING_MODE \
    (MARCEL_EV_POLL_AT_TIMER_SIG | MARCEL_EV_POLL_AT_YIELD | MARCEL_EV_POLL_AT_IDLE)
#endif /* MARCEL */

/*
 * local structures
 * ----------------
 */
typedef struct s_mad_mx_driver_specific
{
        int dummy;
} mad_mx_driver_specific_t, *p_mad_mx_driver_specific_t;

typedef struct s_mad_mx_adapter_specific
{
        uint64_t nic_id;
        uint32_t board_id;
} mad_mx_adapter_specific_t, *p_mad_mx_adapter_specific_t;

typedef struct s_mad_mx_channel_specific
{
        mx_endpoint_addr_t	 endpoint_addr;
        mx_endpoint_t		 endpoint;
        uint32_t		 endpoint_id;
        char			*first_packet;
        uint32_t		 first_packet_length;
} mad_mx_channel_specific_t, *p_mad_mx_channel_specific_t;

typedef struct s_mad_mx_connection_specific
{
        tbx_bool_t		first_incoming_packet_flag;
        tbx_bool_t		first_outgoing_packet_flag;
        uint32_t		remote_endpoint_id;
        mx_endpoint_addr_t	remote_endpoint_addr;
} mad_mx_connection_specific_t, *p_mad_mx_connection_specific_t;

typedef struct s_mad_mx_link_specific
{
        int dummy;
} mad_mx_link_specific_t, *p_mad_mx_link_specific_t;

#ifdef PIOMAN
typedef struct s_mad_mx_ev {
	struct piom_req         inst;
	mx_endpoint_t          endpoint;
	mx_request_t          *p_request;
	mx_return_t           *p_rc;
	mx_status_t           *p_status;
	uint32_t              *p_result;
} mad_mx_ev_t, *p_mad_mx_ev_t;

static struct piom_server mad_mx_ev_server = PIOM_SERVER_INIT(mad_mx_ev_server, "Mad/MX I/O");
static tbx_bool_t mad_mx_ev_server_started = tbx_false;

#elif defined(MARCEL)
typedef struct s_mad_mx_ev {
	struct marcel_ev_req	 inst;
        mx_endpoint_t		 endpoint;
        mx_request_t		*p_request;
        mx_return_t     	*p_rc;
        mx_status_t		*p_status;
        uint32_t		*p_result;
} mad_mx_ev_t, *p_mad_mx_ev_t;

/* With Marcel/Newsched, the locking is now done on the event server.
 * Since locking may occur outside Madeleine's context in malloc
 * handlers, the server pointer must be global
 */
static struct marcel_ev_server mad_mx_ev_server = MARCEL_EV_SERVER_INIT(mad_mx_ev_server, "Mad/MX I/O");
static tbx_bool_t mad_mx_ev_server_started = tbx_false;

#endif /* MARCEL */

#define GATHER_SCATTER_THRESHOLD	32768
#define FIRST_PACKET_THRESHOLD		32768
#define MAD_MX_GROUP_MALLOC_THRESHOLD   256

/*
 * static functions
 * ----------------
 */
static
inline
void
mad_mx_lock(void) {
#ifdef MARCEL
        marcel_extlib_protect();
        //DISP("<mx LOCK>");
#endif /* MARCEL */
}

static
inline
void
mad_mx_unlock(void) {
#ifdef MARCEL
        //DISP("<mx UNLOCK>");
        marcel_extlib_unprotect();
#endif /* MARCEL */
}

static
void
mad_mx_print_status(mx_status_t s) TBX_UNUSED;

static
void
mad_mx_print_status(mx_status_t s) {
        const char *msg = NULL;

        msg = mx_strstatus(s.code);
        DISP("mx status: %s", msg);
}

static
void
mad_mx_check_return(char *msg, mx_return_t return_code) {
        if (return_code != MX_SUCCESS) {
                const char *msg_mx = NULL;

                msg_mx = mx_strerror(return_code);

                DISP("%s failed with code %s = %d/0x%x", msg, msg_mx, return_code, return_code);
                TBX_FAILURE("mx error");
        }
}

static
void
mad_mx_startup_info(void) {
        mx_return_t	 return_code	= MX_SUCCESS;
        uint32_t 	 nb_nic		= 0;
        uint64_t	*nic_id_array	= NULL;
        uint32_t	 nb_native_ep	= 0;
        uint32_t 	 i		= 0;

        LOG_IN();
        DISP("Madeleine/MX: hardware configuration");

        // NICs count
        mad_mx_lock();
#ifdef MX_VERSION_AFTER_1_0_0
        return_code = mx_get_info(NULL, MX_NIC_COUNT, &nb_nic, sizeof(nb_nic), &nb_nic, sizeof(nb_nic));
#else // ! MX_VERSION_AFTER_1_0_0
        return_code = mx_get_info(NULL, MX_NIC_COUNT, &nb_nic, sizeof(nb_nic));
#endif // MX_VERSION_AFTER_1_0_0
        nb_nic = nb_nic & 0xff;

        mad_mx_unlock();
        mad_mx_check_return("mad_mx_startup_info", return_code);
        DISP("NIC count: %d", nb_nic);

        // NIC ids
        nic_id_array = TBX_MALLOC((nb_nic+1) * sizeof(*nic_id_array));
        mad_mx_check_return("mad_mx_startup_info", return_code);

        mad_mx_lock();
#ifdef MX_VERSION_AFTER_1_0_0
        return_code = mx_get_info(NULL, MX_NIC_IDS, nic_id_array, (nb_nic+1) * sizeof(*nic_id_array), nic_id_array, (nb_nic+1) * sizeof(*nic_id_array));
#else // ! MX_VERSION_AFTER_1_0_0
        return_code = mx_get_info(NULL, MX_NIC_IDS, nic_id_array, (nb_nic+1) * sizeof(*nic_id_array));
#endif // MX_VERSION_AFTER_1_0_0
        mad_mx_unlock();
        mad_mx_check_return("mad_mx_startup_info", return_code);

        for (i = 0; i < nb_nic; i++) {
                DISP("NIC %d id: %llx", i, (long long unsigned)nic_id_array[i]);
        }
        TBX_FREE(nic_id_array);
        nic_id_array = NULL;

        // Number of native endpoints
        mad_mx_lock();
#ifdef MX_VERSION_AFTER_1_0_0
        return_code = mx_get_info(NULL, MX_MAX_NATIVE_ENDPOINTS, &nb_native_ep, sizeof(nb_native_ep), &nb_native_ep, sizeof(nb_native_ep));
#else // ! MX_VERSION_AFTER_1_0_0
        return_code = mx_get_info(NULL, MX_MAX_NATIVE_ENDPOINTS, &nb_native_ep, sizeof(nb_native_ep));
#endif // MX_VERSION_AFTER_1_0_0
        mad_mx_unlock();
        mad_mx_check_return("mad_mx_startup_info", return_code);
        DISP("Native endpoint count: %d", nb_native_ep);

        LOG_OUT();
}

#ifdef PIOMAN
static
int
mad_mx_do_poll(piom_server_t	server,
               piom_op_t		_op,
               piom_req_t		req,
               int			nb_ev,
               int			option) {
        p_mad_mx_ev_t	p_ev	= NULL;

        LOG_IN();
	p_ev = struct_up(req, mad_mx_ev_t, inst);

        *(p_ev->p_rc)	= mx_test(p_ev->endpoint, p_ev->p_request, p_ev->p_status, p_ev->p_result);
        if (!(*(p_ev->p_rc) == MX_SUCCESS && !*(p_ev->p_result))) {
                piom_req_success(&(p_ev->inst));
        }
        LOG_OUT();

        return 0;
}
#elif defined(MARCEL)
static
int
mad_mx_do_poll(marcel_ev_server_t	server,
               marcel_ev_op_t		_op,
               marcel_ev_req_t		req,
               int			nb_ev,
               int			option) {
        p_mad_mx_ev_t	p_ev	= NULL;

        LOG_IN();
	p_ev = struct_up(req, mad_mx_ev_t, inst);

        *(p_ev->p_rc)	= mx_test(p_ev->endpoint, p_ev->p_request, p_ev->p_status, p_ev->p_result);
        if (!(*(p_ev->p_rc) == MX_SUCCESS && !*(p_ev->p_result))) {
                MARCEL_EV_REQ_SUCCESS(&(p_ev->inst));
        }
        LOG_OUT();

        return 0;
}
#endif /* MARCEL */


#if( defined(PIOMAN) && defined(MA__LWPS))

static int mad_mx_blockone(piom_server_t server,
			   piom_op_t _op,
			   piom_req_t req, int nb_ev, int option)
{                             // a preciser

	p_mad_mx_ev_t   p_ev    =  struct_up(req, mad_mx_ev_t, inst);
	if(p_ev->inst.state & PIOM_STATE_OCCURED)
		return 0;

	*(p_ev->p_rc)   = mx_wait(p_ev->endpoint, p_ev->p_request,
				  MX_INFINITE, p_ev->p_status, p_ev->p_result);

	if (!(*(p_ev->p_rc) == MX_SUCCESS && !*(p_ev->p_result)))
		piom_req_success(&(p_ev->inst));

	return 0;
}
#endif                                // MA__LWPS

static void
mad_mx_blocking_test(p_mad_channel_t	 ch,
                     mx_request_t	*p_request,
                     mx_status_t	*p_status){
        p_mad_mx_channel_specific_t	chs	= NULL;
        mx_endpoint_t			ep	= NULL;
        mx_status_t	 		status;
        uint32_t	 		result	= 0;
        mx_return_t 	 		rc 	= MX_SUCCESS;

        LOG_IN();
        chs	= ch->specific;
        ep	= chs->endpoint;

        if (!p_status) {
                p_status = &status;
        }

#ifdef PIOMAN
        struct piom_wait             ev_w;
	mad_mx_ev_t                   ev     =
		{
			.endpoint    =  ep,
			.p_request   =  p_request,
			.p_rc                = &rc,
			.p_status    =  p_status,
			.p_result    = &result
		};
	piom_wait(&mad_mx_ev_server, &(ev.inst), &ev_w, 0);

#elif defined(MARCEL)
 {
         struct marcel_ev_wait		 ev_w;
         mad_mx_ev_t 			 ev	=
                 {
                         .endpoint 	=  ep,
                         .p_request	=  p_request,
                         .p_rc		= &rc,
                         .p_status	=  p_status,
                         .p_result	= &result
                 };

         marcel_ev_wait(&mad_mx_ev_server, &(ev.inst), &ev_w, 0);
 }
#else /* MARCEL */
        do {
                rc = mx_test(chs->endpoint, p_request, p_status, &result);
        } while (rc == MX_SUCCESS && !result);
#endif /* MARCEL */

        mad_mx_check_return("mx_test", rc);
        LOG_OUT();
}

static void
mad_mx_irecv(p_mad_channel_t	 ch,
             mx_segment_t	*seg_list,
             uint32_t		 nb_seg,
             uint64_t		*p_match_info,
             uint64_t		 match_mask,
             mx_request_t	*p_request){
        p_mad_mx_channel_specific_t	chs	= NULL;
        mx_endpoint_t			ep	= NULL;
        mx_return_t     		rc	= MX_SUCCESS;

        LOG_IN();
        chs	= ch->specific;
        ep	= chs->endpoint;

        mad_mx_lock();
        rc	= mx_irecv(ep,
                      seg_list, nb_seg,
                      *p_match_info,
                      match_mask, NULL, p_request);
        mad_mx_unlock();
        mad_mx_check_return("mx_irecv", rc);
        LOG_OUT();
}

static void
mad_mx_isend(p_mad_connection_t	 out,
             mx_segment_t	*seg_list,
             uint32_t		 nb_seg,
             uint64_t		 match_info,
             mx_request_t 	*p_request){
        p_mad_mx_channel_specific_t	chs	= NULL;
        p_mad_mx_connection_specific_t	os	= NULL;
        mx_endpoint_t			ep	= NULL;
        mx_endpoint_addr_t		dest_address;
        mx_return_t  			rc	= MX_SUCCESS;

        LOG_IN();
        os 	= out->specific;
        chs	= out->channel->specific;
        ep 	= chs->endpoint;

        dest_address	= os->remote_endpoint_addr;

        mad_mx_lock();
        rc	= mx_isend(ep,
                           seg_list, nb_seg,
                           dest_address,
                           match_info,
                           NULL, p_request);
        mad_mx_unlock();
        mad_mx_check_return("mx_isend", rc);
        LOG_OUT();
}

static void
mad_mx_send(p_mad_connection_t	 out,
            mx_segment_t	*seg_list,
            uint32_t		 nb_seg,
            uint64_t		 match_info){
        mx_request_t	request	= 0;

        LOG_IN();
        mad_mx_isend(out,
                     seg_list, nb_seg,
                     match_info, &request);

        mad_mx_blocking_test(out->channel, &request, NULL);
        LOG_OUT();
}

static uint32_t
mad_mx_recv(p_mad_channel_t	 ch,
            mx_segment_t	*seg_list,
            uint32_t		 nb_seg,
            uint64_t		*p_match_info,
            uint64_t		 match_mask){
        mx_request_t	request;
        mx_status_t	status;
        uint32_t	length	= 0;

        LOG_IN();
        mad_mx_irecv(ch, seg_list, nb_seg, p_match_info, match_mask, &request);
        mad_mx_blocking_test(ch, &request, &status);

        *p_match_info	= status.match_info;
        length		= status.msg_length;
        LOG_OUT();

        return length;
}

/*
 * Registration function
 * ---------------------
 */


char *
mad_mx_register(p_mad_driver_interface_t interface) {
        LOG_IN();
        TRACE("Registering MX driver");
        interface->driver_init                = mad_mx_driver_init;
        interface->adapter_init               = mad_mx_adapter_init;
        interface->channel_init               = mad_mx_channel_init;
        interface->before_open_channel        = NULL;
        interface->connection_init            = mad_mx_connection_init;
        interface->link_init                  = mad_mx_link_init;
        interface->accept                     = mad_mx_accept;
        interface->connect                    = mad_mx_connect;
        interface->after_open_channel         = NULL;
        interface->before_close_channel       = NULL;
        interface->disconnect                 = mad_mx_disconnect;
        interface->after_close_channel        = NULL;
        interface->link_exit                  = NULL;
        interface->connection_exit            = mad_mx_connection_exit;
        interface->channel_exit               = mad_mx_channel_exit;
        interface->adapter_exit               = mad_mx_adapter_exit;
        interface->driver_exit                = mad_mx_driver_exit;
        interface->choice                     = NULL;
        interface->get_static_buffer          = NULL;
        interface->return_static_buffer       = NULL;
        interface->new_message                = mad_mx_new_message;
        interface->finalize_message           = NULL;
#ifdef MAD_MESSAGE_POLLING
        interface->poll_message               = mad_mx_poll_message;
#endif // MAD_MESSAGE_POLLING
        interface->receive_message            = mad_mx_receive_message;
        interface->message_received           = NULL;
        interface->send_buffer                = mad_mx_send_buffer;
        interface->receive_buffer             = mad_mx_receive_buffer;
        interface->send_buffer_group          = mad_mx_send_buffer_group;
        interface->receive_sub_buffer_group   = mad_mx_receive_sub_buffer_group;

#ifdef PIOMAN

#ifdef MA__LWPS
	/*  Lancement de 4 LWP de communication */
	piom_server_start_lwp(&mad_mx_ev_server, 4);
#endif /* MA__LWPS */

	piom_pcallback_t callback={.func=mad_mx_do_poll, .speed=PIOM_CALLBACK_FAST}; /* TODO : PIOM_CALLBACK_IMMEDIATE ? */

	piom_server_set_poll_settings(&mad_mx_ev_server, MAD_MX_POLLING_MODE, 1, -1);
	piom_server_add_callback(&mad_mx_ev_server, PIOM_FUNCTYPE_POLL_POLLONE, callback);

#ifdef MA__LWPS
	callback.func=mad_mx_blockone;
	callback.speed=PIOM_CALLBACK_FAST;
	piom_server_add_callback(&mad_mx_ev_server, PIOM_FUNCTYPE_BLOCK_WAITONE, callback);
#endif /* MA__LWPS */
#elif defined(MARCEL)
        marcel_ev_server_set_poll_settings(&mad_mx_ev_server, MAD_MX_POLLING_MODE, 1);
        marcel_ev_server_add_callback(&mad_mx_ev_server, MARCEL_EV_FUNCTYPE_POLL_POLLONE, mad_mx_do_poll);
#endif /* MARCEL */
        LOG_OUT();

        return "mx";
}


void
mad_mx_driver_init(p_mad_driver_t d, int *argc, char ***argv) {
        p_mad_mx_driver_specific_t	 ds		= NULL;
        mx_return_t			 return_code	= MX_SUCCESS;

        LOG_IN();
        d->buffer_alignment = 32;
        d->connection_type  = mad_bidirectional_connection;

        TRACE("Initializing MX driver");
        ds  		= TBX_MALLOC(sizeof(mad_mx_driver_specific_t));
        d->specific	= ds;

        mad_mx_lock();
        mx_set_error_handler(MX_ERRORS_RETURN);
        mad_mx_unlock();

        mad_mx_lock();
        return_code	= mx_init();
        mad_mx_unlock();
        mad_mx_check_return("mx_init", return_code);

        /* debug only */
#ifndef MX_NO_STARTUP_INFO
        mad_mx_startup_info();
#endif // MX_NO_STARTUP_INFO

#ifdef PIOMAN
        if (!mad_mx_ev_server_started) {
                piom_server_start(&mad_mx_ev_server);
                mad_mx_ev_server_started = tbx_true;
        }
#elif defined(MARCEL)
        if (!mad_mx_ev_server_started) {
                marcel_ev_server_start(&mad_mx_ev_server);
                mad_mx_ev_server_started = tbx_true;
        }
#endif /* MARCEL */
        LOG_OUT();
}

void
mad_mx_adapter_init(p_mad_adapter_t a) {
        p_mad_mx_adapter_specific_t	as	= NULL;
        uint64_t                        nic_id;
        char                            hostname[MX_MAX_HOSTNAME_LEN];

        LOG_IN();
        as		= TBX_MALLOC(sizeof(mad_mx_adapter_specific_t));

        if (strcmp(a->dir_adapter->name, "default")) {
                TBX_FAILURE("unsupported adapter");
        } else {
                as->board_id	= MX_ANY_NIC;
        }

        a->specific	= as;

        /* ID of the first Myrinet card */
        mx_board_number_to_nic_id(0, &nic_id);
        /* Hostname attached to this ID */
        mx_nic_id_to_hostname(nic_id, hostname);

        a->parameter	= tbx_strdup(hostname);
        LOG_OUT();
}

void
mad_mx_channel_init(p_mad_channel_t ch) {
        p_mad_mx_adapter_specific_t 	as = NULL;
        p_mad_mx_channel_specific_t	chs		= NULL;
        p_tbx_string_t			param_str	= NULL;
        mx_return_t			return_code	= MX_SUCCESS;
        uint32_t                        e_id     	= 1;
        const uint32_t			e_key		= 0xFFFFFFFF;
        mx_endpoint_t			e;
        mx_endpoint_addr_t		e_addr;

        LOG_IN();
        as =	ch->adapter->specific;

        mad_mx_lock();
        do {
                return_code	= mx_open_endpoint(as->board_id,
                                                   e_id,
                                                   e_key,
                                                   NULL,
                                                   0,
                                                   &e);
        } while (return_code == MX_BUSY && e_id++);
        mad_mx_unlock();
        mad_mx_check_return("mx_open_endpoint", return_code);

        //DISP("mad_mx_channel_init[%s]: e = %p", ch->name, e);
        //DISP_VAL("mad_mx_channel_init: eid", e_id);

        mad_mx_lock();
        return_code	= mx_get_endpoint_addr(e, &e_addr);
        mad_mx_unlock();
        mad_mx_check_return("mx_get_endpoint", return_code);

        chs			= TBX_MALLOC(sizeof(mad_mx_channel_specific_t));
        chs->endpoint			= e;
        chs->endpoint_id		= e_id;
        chs->first_packet		= TBX_MALLOC(FIRST_PACKET_THRESHOLD);
        chs->first_packet_length	= 0;
        ch->specific			= chs;

        param_str	= tbx_string_init_to_int(e_id);
        ch->parameter	= tbx_string_to_cstring(param_str);
        tbx_string_free(param_str);
        param_str	= NULL;
        //DISP_STR("mad_mx_channel_init: parameter", ch->parameter);
        LOG_OUT();
}

void
mad_mx_connection_init(p_mad_connection_t in,
                       p_mad_connection_t out) {
        p_mad_mx_connection_specific_t	cs	= NULL;

        LOG_IN();
        cs			= TBX_MALLOC(sizeof(mad_mx_connection_specific_t));
        cs->remote_endpoint_id	= 0;
        cs->first_incoming_packet_flag	= tbx_false;
        cs->first_outgoing_packet_flag	= tbx_false;

        in->specific		= cs;
        in->nb_link		= 1;

        out->specific		= cs;
        out->nb_link		= 1;
        LOG_OUT();
}

void
mad_mx_link_init(p_mad_link_t lnk) {
        LOG_IN();
        lnk->link_mode   = mad_link_mode_buffer_group;
        /*lnk->link_mode   = mad_link_mode_buffer;*/
        lnk->buffer_mode = mad_buffer_mode_dynamic;
        lnk->group_mode  = mad_group_mode_split;
        LOG_OUT();
}

static
void
mad_mx_accept_connect(p_mad_connection_t   cnx,
                      p_mad_adapter_info_t ai) {
        p_mad_mx_connection_specific_t	 cs		= NULL;
        p_mad_mx_channel_specific_t	 chs		= NULL;
        mx_return_t			 return_code	= MX_SUCCESS;
        uint64_t			 r_nic_id	= 0;
        const uint32_t			 e_key		= 0xFFFFFFFF;
        char				*r_hostname	= NULL;
        size_t				 r_hostname_len	= 0;
        mx_endpoint_addr_t 		 re_addr;

        LOG_IN();
        cs	= cnx->specific;
        chs	= cnx->channel->specific;

        r_hostname_len	= strlen(ai->dir_adapter->parameter) + 2;
        r_hostname	= TBX_MALLOC(r_hostname_len + 1);
        strcpy(r_hostname, ai->dir_adapter->parameter);

        //DISP_STR("mad_mx_accept_connect: parameter", ai->channel_parameter);
        {
                char *ptr = NULL;

                cs->remote_endpoint_id = strtol(ai->channel_parameter, &ptr, 0);
                if (ai->channel_parameter == ptr)
                        TBX_FAILURE("invalid channel parameter");
        }

        mad_mx_lock();
        return_code	= mx_hostname_to_nic_id(r_hostname, &r_nic_id);
        mad_mx_unlock();
        mad_mx_check_return("mx_hostname_to_nic_id", return_code);

        TBX_FREE(r_hostname);
        r_hostname	= NULL;
        r_hostname_len	= 0;

        mad_mx_lock();
        //DISP_VAL("mx_connect_accept: remote eid", cs->remote_endpoint_id);
        return_code	= mx_connect(chs->endpoint,
                                     r_nic_id,
                                     cs->remote_endpoint_id,
                                     e_key,
                                     MX_INFINITE,
                                     &re_addr);
        mad_mx_unlock();
        mad_mx_check_return("mx_connect", return_code);
        cs->remote_endpoint_addr	= re_addr;
        LOG_OUT();
}

void
mad_mx_accept(p_mad_connection_t   in,
              p_mad_adapter_info_t ai) {
        LOG_IN();
        mad_mx_accept_connect(in, ai);
        LOG_OUT();
}

void
mad_mx_connect(p_mad_connection_t   out,
               p_mad_adapter_info_t ai) {
        LOG_IN();
        mad_mx_accept_connect(out, ai);
        LOG_OUT();
}

void
mad_mx_disconnect(p_mad_connection_t c) {
        p_mad_mx_connection_specific_t cs = NULL;

        LOG_IN();
        cs = c->specific;
        LOG_OUT();
}

void
mad_mx_connection_exit(p_mad_connection_t in,
                p_mad_connection_t out) {
        p_mad_mx_connection_specific_t cs = NULL;

        LOG_IN();
        cs = in->specific;
        TBX_FREE(cs);
        in->specific	= NULL;
        out->specific	= NULL;
        LOG_OUT();
}

void
mad_mx_channel_exit(p_mad_channel_t ch) {
        p_mad_mx_channel_specific_t	chs		= NULL;
        mx_return_t			return_code	= MX_SUCCESS;

        LOG_IN();
        chs = ch->specific;
        mad_mx_lock();
        return_code	= mx_close_endpoint(chs->endpoint);
        mad_mx_unlock();
        mad_mx_check_return("mx_close_endpoint", return_code);
        TBX_FREE(chs->first_packet);
        chs->first_packet = NULL;

        TBX_FREE(chs);
        ch->specific	= NULL;

        TBX_FREE(ch->parameter);
        ch->parameter	= NULL;
        LOG_OUT();
}

void
mad_mx_adapter_exit(p_mad_adapter_t a) {
        p_mad_mx_adapter_specific_t as = NULL;

        LOG_IN();
        as		= a->specific;
        TBX_FREE(as);
        a->specific	= NULL;
        TBX_FREE(a->parameter);
        a->parameter	= NULL;
        LOG_OUT();
}

void
mad_mx_driver_exit(p_mad_driver_t d) {
        p_mad_mx_driver_specific_t	ds		= NULL;
        mx_return_t			return_code	= MX_SUCCESS;

        LOG_IN();
        TRACE("Finalizing MX driver");
        ds	= d->specific;
        TBX_FREE(ds);
        d->specific	= NULL;

        mad_mx_lock();
        return_code	= mx_finalize();
        mad_mx_unlock();
        mad_mx_check_return("mx_finalize", return_code);
        LOG_OUT();
}

void
mad_mx_new_message(p_mad_connection_t out){
        p_mad_mx_connection_specific_t	os	= NULL;

        LOG_IN();
        os	= out->specific;
        os->first_outgoing_packet_flag	= tbx_true;
        LOG_OUT();
}

p_mad_connection_t
mad_mx_receive_message(p_mad_channel_t ch) {
        p_mad_mx_channel_specific_t	chs		= NULL;
        p_mad_connection_t		in		= NULL;
        p_mad_mx_connection_specific_t	is		= NULL;
        p_tbx_darray_t			in_darray	= NULL;
        uint64_t 			match_info	= 0;
        mx_segment_t 			s;

        LOG_IN();
        chs		= ch->specific;
        in_darray	= ch->in_connection_darray;
        //match_info	= (uint64_t)ch->id <<32;
        match_info	= 0;

        s.segment_ptr	= chs->first_packet;
        s.segment_length	= FIRST_PACKET_THRESHOLD;

        //DISP("MX[%s]: receive_message, e = %p", ch->name, chs->endpoint);
        chs->first_packet_length = mad_mx_recv(ch, &s, 1, &match_info, MX_MATCH_MASK_BC);

        in	= tbx_darray_get(in_darray, match_info & 0xFFFFFFFF);
        is	= in->specific;

        is-> first_incoming_packet_flag		= tbx_true;
        LOG_OUT();
        return in;
}

void
mad_mx_send_buffer(p_mad_link_t     lnk,
                   p_mad_buffer_t   b) {
        p_mad_connection_t		out		= NULL;
        p_mad_mx_connection_specific_t	os		= NULL;
        uint64_t			match_info	= 0;
        mx_segment_t 			s;

        LOG_IN();
        out	= lnk->connection;
        os	= out->specific;

        //DISP("MX[%s]: sending %d bytes to %d, e = %p", lnk->connection->channel->name, b->bytes_written - b->bytes_read, lnk->connection->remote_rank, ((p_mad_mx_channel_specific_t)out->channel->specific)->endpoint);
        //match_info	= (uint64_t)out->channel->id <<32 | out->channel->process_lrank;
        match_info	= (uint64_t)out->channel->process_lrank;

        if (os->first_outgoing_packet_flag) {
                uint32_t length = 0;

                os->first_outgoing_packet_flag = tbx_false;

                length	= tbx_min(b->bytes_written - b->bytes_read,
                                  FIRST_PACKET_THRESHOLD);

                s.segment_ptr		= b->buffer + b->bytes_read;
                s.segment_length	= length;

                mad_mx_send(out, &s, 1, match_info);

                b->bytes_read += length;

                if (!mad_more_data(b))
                        goto no_more_data;
        }

        s.segment_ptr		= b->buffer + b->bytes_read;
        s.segment_length	= b->bytes_written - b->bytes_read;

        b->bytes_read += s.segment_length;

        mad_mx_send(out, &s, 1, match_info);

 no_more_data:
        ;
        LOG_OUT();
}

void
mad_mx_receive_buffer(p_mad_link_t    lnk,
                      p_mad_buffer_t *_buffer) {
        p_mad_buffer_t			b	  	= *_buffer;
        p_mad_connection_t		in		= NULL;
        p_mad_mx_connection_specific_t	is		= NULL;
        p_mad_channel_t			ch		= NULL;
        p_mad_mx_channel_specific_t	chs		= NULL;
        uint64_t			match_info	= 0;
        mx_segment_t			s;

        LOG_IN();
        //DISP("MX[%s]: receiving %d bytes from %d", lnk->connection->channel->name, (*_buffer)->length - (*_buffer)->bytes_written, lnk->connection->remote_rank);
        in	= lnk->connection;
        is	= in->specific;
        ch	= in->channel;
        chs	= ch->specific;

        if (is->first_incoming_packet_flag) {
                void *          data_ptr	= NULL;
                uint32_t	data_length	= 0;

                is->first_incoming_packet_flag = tbx_false;

                data_ptr	= b->buffer + b->bytes_written;
                data_length	= tbx_min(b->length - b->bytes_written, FIRST_PACKET_THRESHOLD);


                if (chs->first_packet_length != data_length)
                        TBX_FAILURE("invalid first packet length");

                memcpy(data_ptr, chs->first_packet, data_length);
                b->bytes_written	+= data_length;

                if (mad_buffer_full(b))
                        goto no_more_data;
        }

        s.segment_ptr		= b->buffer + b->bytes_written;
        s.segment_length	= b->length - b->bytes_written;

        b->bytes_written += s.segment_length;

        //match_info = (uint64_t)in->channel->id <<32 | in->remote_rank;
        match_info = (uint64_t) in->remote_rank;

        mad_mx_recv(ch, &s, 1, &match_info, MX_MATCH_MASK_NONE);

 no_more_data:
        ;
        LOG_OUT();
}

void
mad_mx_send_buffer_group_1(p_mad_link_t         lnk,
                           p_mad_buffer_group_t buffer_group) {
        LOG_IN();
        if (!tbx_empty_list(&(buffer_group->buffer_list))) {
                tbx_list_reference_t ref;

                tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
                do {
                        p_mad_buffer_t b = NULL;

                        b = tbx_get_list_reference_object(&ref);
                        mad_mx_send_buffer(lnk, b);
                } while(tbx_forward_list_reference(&ref));
        }
        LOG_OUT();
}

void
mad_mx_receive_sub_buffer_group_1(p_mad_link_t         lnk,
                                  p_mad_buffer_group_t buffer_group) {
        LOG_IN();
        if (!tbx_empty_list(&(buffer_group->buffer_list))) {
                tbx_list_reference_t ref;

                tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
                do {
                        p_mad_buffer_t b = NULL;

                        b = tbx_get_list_reference_object(&ref);
                        mad_mx_receive_buffer(lnk, &b);
                } while(tbx_forward_list_reference(&ref));
        }
        LOG_OUT();
}


void
mad_mx_send_buffer_group_2_process_list(mx_segment_t                   *seg_list,
                                        p_mad_buffer_group_t            buffer_group,
                                        p_mad_connection_t              out,
                                        p_mad_mx_connection_specific_t	os,
                                        uint64_t                        match_info) {
  tbx_list_reference_t	ref;
  uint32_t		offset	= 0;
  unsigned int		i	= 0;

  LOG_IN();
  tbx_list_reference_init(&ref, &(buffer_group->buffer_list));

  do {
    p_mad_buffer_t	b		= NULL;
    void *          data_ptr	= NULL;
    uint32_t	data_length	= 0;

    b = tbx_get_list_reference_object(&ref);

    if (os->first_outgoing_packet_flag){
      mx_segment_t s;

      os->first_outgoing_packet_flag = 0;

      // on envoie une premiere partie en guise de new_msg
      data_ptr	= b->buffer + b->bytes_read;
      data_length	= tbx_min(b->bytes_written - b->bytes_read, FIRST_PACKET_THRESHOLD);

      b->bytes_read		+= data_length;

      s.segment_ptr		 = data_ptr;
      s.segment_length	 = data_length;

      mad_mx_send(out, &s, 1, match_info);

      if(!mad_more_data(b))
        goto no_more_data;
    }

    data_ptr	 = b->buffer        + b->bytes_read;
    data_length	 =
      i?tbx_min(b->bytes_written - b->bytes_read,
                GATHER_SCATTER_THRESHOLD - offset)
      :b->bytes_written - b->bytes_read;

    b->bytes_read	+= data_length;

    seg_list[i].segment_ptr		= data_ptr;
    seg_list[i].segment_length	= data_length;

    i++;
    offset += data_length;

    if (offset >= GATHER_SCATTER_THRESHOLD){
      mad_mx_send(out, seg_list, i, match_info);

      if (mad_more_data(b)) {
        mx_segment_t s;

        data_ptr	+= data_length;
        data_length	 = b->bytes_written - b->bytes_read;

        b->bytes_read	+= data_length;

        s.segment_ptr		= data_ptr;
        s.segment_length	= data_length;

        mad_mx_send(out, &s, 1, match_info);
      }

      i = 0;
      offset = 0;
    }

  no_more_data:
    ;

  } while (tbx_forward_list_reference(&ref));

  if (offset) {
    mad_mx_send(out, seg_list, i, match_info);
  }

  LOG_OUT();
}


void
mad_mx_send_buffer_group_2(p_mad_link_t         lnk,
                           p_mad_buffer_group_t buffer_group) {
        p_mad_connection_t		out		= NULL;
        p_mad_mx_connection_specific_t	os		= NULL;
        p_mad_mx_channel_specific_t	chs		= NULL;
        uint64_t                        match_info	= 0;
        p_tbx_list_t 			buffer_list	= NULL;

        LOG_IN();
        out	= lnk->connection;
        os	= out->specific;
        chs	= out->channel->specific;
        match_info = (uint64_t)out->channel->id <<32 | out->channel->process_lrank;

        buffer_list = &buffer_group->buffer_list;

        if (!tbx_empty_list(buffer_list)) {

          if (buffer_list->length < MAD_MX_GROUP_MALLOC_THRESHOLD) {
            mx_segment_t		seg_list[buffer_list->length];

            mad_mx_send_buffer_group_2_process_list(seg_list, buffer_group, out, os, match_info);
          }
          else {
            mx_segment_t               *seg_list = NULL;

            seg_list = TBX_MALLOC(buffer_list->length * sizeof(mx_segment_t));

            mad_mx_send_buffer_group_2_process_list(seg_list, buffer_group, out, os, match_info);

            TBX_FREE(seg_list);
          }

        }
        LOG_OUT();
}

void
mad_mx_receive_sub_buffer_group_2_process_list(mx_segment_t                  *seg_list,
                                               mx_request_t                  *req_list,
                                               p_mad_buffer_group_t           buffer_group,
                                               p_mad_mx_connection_specific_t is,
                                               p_mad_channel_t		      ch,
                                               p_mad_mx_channel_specific_t    chs,
                                               uint64_t                       match_info) {
  tbx_list_reference_t	ref;
  uint32_t		offset	= 0;
  unsigned int		i	= 0;
  unsigned int		j	= 0;
  unsigned int		k	= 0;
  uint64_t              mi      = 0;

  LOG_IN();
  tbx_list_reference_init(&ref, &(buffer_group->buffer_list));

  do {
    p_mad_buffer_t b = NULL;
    void *          data_ptr	= NULL;
    uint32_t	data_length	= 0;

    b = tbx_get_list_reference_object(&ref);

    if (is->first_incoming_packet_flag){


      is->first_incoming_packet_flag = 0;

      //on a deja recu une premiere partie dans receive_message
      data_ptr	= b->buffer + b->bytes_written;
      data_length	= tbx_min(b->length - b->bytes_written, FIRST_PACKET_THRESHOLD);

      if (chs->first_packet_length != data_length)
        TBX_FAILURE("invalid first packet length");

      memcpy(data_ptr, chs->first_packet, data_length);

      b->bytes_written += data_length;

      if (mad_buffer_full(b))
        goto no_more_data;
    }

    data_ptr	= b->buffer + b->bytes_written;
    data_length	=
      i?tbx_min(b->length - b->bytes_written,
                GATHER_SCATTER_THRESHOLD - offset)
      :b->length - b->bytes_written;

    b->bytes_written	+= data_length;

    seg_list[i].segment_ptr		= data_ptr;
    seg_list[i].segment_length	= data_length;

    i++;
    offset += data_length;

    if (offset >= GATHER_SCATTER_THRESHOLD){
      mi	= match_info;
      mad_mx_irecv(ch, seg_list, i, &mi, MX_MATCH_MASK_NONE, req_list+j);
      j++;

      if (!mad_buffer_full(b)) {
        mx_segment_t s;

        data_ptr	+= data_length;
        data_length	 = b->length - b->bytes_written;

        b->bytes_written	+= data_length;

        s.segment_ptr		= data_ptr;
        s.segment_length	= data_length;

        mi	= match_info;
        mad_mx_irecv(ch, &s, 1, &mi, MX_MATCH_MASK_NONE, req_list+j);
        j++;
      }

      i = 0;
      offset = 0;
    }

  no_more_data:
    ;

  } while(tbx_forward_list_reference(&ref));

  if (offset) {
    mi 	= match_info;
    mad_mx_irecv(ch, seg_list, i, &mi, MX_MATCH_MASK_NONE, req_list+j);
    j++;
  }

  for(k = 0; k < j; k++){
    mad_mx_blocking_test(ch, req_list+k, NULL);
  }

  LOG_OUT();
}

void
mad_mx_receive_sub_buffer_group_2(p_mad_link_t         lnk,
                                  p_mad_buffer_group_t buffer_group) {
        p_mad_connection_t		in	    = NULL;
        p_mad_mx_connection_specific_t	is	    = NULL;
        p_mad_channel_t			ch	    = NULL;
        p_mad_mx_channel_specific_t	chs	    = NULL;
        uint64_t                        match_info  = 0;
        p_tbx_list_t                    buffer_list = NULL;

        LOG_IN();
        in	= lnk->connection;
        is	= in->specific;
        ch	= in->channel;
        chs	= ch->specific;
        match_info = (uint64_t)in->channel->id <<32 | in->remote_rank;

        buffer_list = &buffer_group->buffer_list;

        if (!tbx_empty_list(buffer_list)) {

          if (buffer_list->length < MAD_MX_GROUP_MALLOC_THRESHOLD) {
            mx_segment_t		seg_list[buffer_list->length];
            mx_request_t		req_list[buffer_list->length*2];

            mad_mx_receive_sub_buffer_group_2_process_list(seg_list, req_list, buffer_group, is, ch, chs, match_info);
          }
          else {
            mx_segment_t                *seg_list = NULL;
            mx_request_t                *req_list = NULL;

            seg_list = TBX_MALLOC(buffer_list->length * sizeof(mx_segment_t));
            req_list = TBX_MALLOC(buffer_list->length * 2 * sizeof(mx_request_t));

            mad_mx_receive_sub_buffer_group_2_process_list(seg_list, req_list, buffer_group, is, ch, chs, match_info);

            TBX_FREE(seg_list);
            TBX_FREE(req_list);
          }
        }
        LOG_OUT();
}

void
mad_mx_send_buffer_group(p_mad_link_t         lnk,
                         p_mad_buffer_group_t buffer_group) {
        LOG_IN();
        mad_mx_send_buffer_group_2(lnk, buffer_group);
        LOG_OUT();
}

void
mad_mx_receive_sub_buffer_group(p_mad_link_t         lnk,
                                tbx_bool_t           first_sub_group
                                __attribute__ ((unused)),
                                p_mad_buffer_group_t buffer_group) {
        LOG_IN();
        mad_mx_receive_sub_buffer_group_2(lnk, buffer_group);
        LOG_OUT();
}
