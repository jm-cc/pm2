
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
 * Mad_mpi.c
 * =========
 */
/* #define DEBUG */
/* #define USE_MARCEL_POLL */

#if 0
/*
 * !!!!!!!!!!!!!!!!!!!!!!!! Workarounds !!!!!!!!!!!!!!!!!!!!
 * _________________________________________________________
 */
#define MARCEL_POLL_WA
#endif

#define MAD_MPI_POLLING_MODE \
    (MARCEL_EV_POLL_AT_TIMER_SIG | MARCEL_EV_POLL_AT_YIELD | MARCEL_EV_POLL_AT_IDLE)
/*
 * headerfiles
 * -----------
 */

/* MadII header files */
#include "madeleine.h"

/* system header files */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

/* protocol specific header files */
#include <mpi.h>

/*
 * macros and constants definition
 * -------------------------------
 */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif /* MAXHOSTNAMELEN */
#define MAD_MPI_SERVICE_TAG      0
#define MAD_MPI_TRANSFER_TAG     1
#define MAD_MPI_FLOW_CONTROL_TAG 2
#ifdef MARCEL
#define MAX_MPI_REQ 64
#endif /* MARCEL */

#define MAD_MPI_GROUP_MALLOC_THRESHOLD 256

/*
 * type definition
 * ---------------
 */


/*
 * local structures
 * ----------------
 */
#ifdef MARCEL
typedef struct s_mad_mpi_poll_arg
{
        marcel_pollinst_t pollinst;
        MPI_Request       request;
        MPI_Status        status;
} mad_mpi_poll_arg_t, *p_mad_mpi_poll_arg_t;
#endif /* MARCEL */

typedef struct s_mad_mpi_driver_specific
{
        TBX_SHARED;
        int nb_adapter;
} mad_mpi_driver_specific_t, *p_mad_mpi_driver_specific_t;

typedef struct s_mad_mpi_adapter_specific
{
        int dummy;
} mad_mpi_adapter_specific_t, *p_mad_mpi_adapter_specific_t;

typedef struct s_mad_mpi_channel_specific
{
        MPI_Comm communicator;
        int rank;
        int size;
} mad_mpi_channel_specific_t, *p_mad_mpi_channel_specific_t;

typedef struct s_mad_mpi_connection_specific
{
        int remote_rank;
} mad_mpi_connection_specific_t, *p_mad_mpi_connection_specific_t;

typedef struct s_mad_mpi_link_specific
{
        int dummy;
} mad_mpi_link_specific_t, *p_mad_mpi_link_specific_t;


#ifdef MARCEL
typedef struct s_mad_mpi_ev {
	struct marcel_ev_req	 inst;
        MPI_Request		 mpi_request;
        MPI_Status		*p_mpi_status;
} mad_mpi_ev_t, *p_mad_mpi_ev_t;
#endif /* MARCEL */

#ifdef MARCEL
static struct marcel_ev_server mad_mpi_ev_server = MARCEL_EV_SERVER_INIT(mad_mpi_ev_server, "Mad/MPI I/O");
#endif /* MARCEL */


/* Prototypes */

#ifdef MARCEL
static
void *
mad_mpi_malloc_hook(size_t len, const void *caller);

static
void *
mad_mpi_memalign_hook(size_t alignment, size_t len, const void *caller);

static
void
mad_mpi_free_hook(void *ptr, const void *caller);

static
void *
mad_mpi_realloc_hook(void *ptr, size_t len, const void *caller);

static
void
mad_mpi_malloc_initialize_hook(void);


/* Previous handlers */
static
void *
(*mad_mpi_old_malloc_hook)(size_t len, const void *caller) = NULL;

static
void *
(*mad_mpi_old_memalign_hook)(size_t alignment, size_t len, const void *caller) = NULL;

static
void
(*mad_mpi_old_free_hook)(void *PTR, const void *CALLER) = NULL;

static
void *
(*mad_mpi_old_realloc_hook)(void *PTR, size_t LEN, const void *CALLER) = NULL;

#endif /* MARCEL */

#ifdef MARCEL
/* Flag to prevent multiple hooking */
static
int mad_mpi_malloc_hooked = 0;
#endif /* MARCEL */



/*
 * static functions
 * ----------------
 */

static
inline
void
__mad_mpi_hook_lock(void) {
#ifdef MARCEL
        marcel_ev_lock(&mad_mpi_ev_server);
        //DISP("<mpi hook LOCK>");
#endif /* MARCEL */
}

static
inline
void
__mad_mpi_hook_unlock(void) {
#ifdef MARCEL
        //DISP("<mpi hook UNLOCK>");
        marcel_ev_unlock(&mad_mpi_ev_server);
#endif /* MARCEL */
}

#ifdef MARCEL
static
void
mad_mpi_install_hooks(void) {
        LOG_IN();
        mad_mpi_old_malloc_hook		= __malloc_hook;
        mad_mpi_old_memalign_hook	= __memalign_hook;
        mad_mpi_old_free_hook		= __free_hook;
        mad_mpi_old_realloc_hook		= __realloc_hook;

        if (__malloc_hook == mad_mpi_malloc_hook)
                TBX_FAILURE("hooks corrupted");

        if (__memalign_hook == mad_mpi_memalign_hook)
                TBX_FAILURE("hooks corrupted");

        if (__realloc_hook == mad_mpi_realloc_hook)
                TBX_FAILURE("hooks corrupted");

        if (__free_hook == mad_mpi_free_hook)
                TBX_FAILURE("hooks corrupted");

        __malloc_hook		= mad_mpi_malloc_hook;
        __memalign_hook		= mad_mpi_memalign_hook;
        __free_hook		= mad_mpi_free_hook;
        __realloc_hook		= mad_mpi_realloc_hook;
        LOG_OUT();
}

static
void
mad_mpi_remove_hooks(void) {
        LOG_IN();
        if (__malloc_hook == mad_mpi_old_malloc_hook)
                TBX_FAILURE("hooks corrupted");

        if (__memalign_hook == mad_mpi_old_memalign_hook)
                TBX_FAILURE("hooks corrupted");

        if (__realloc_hook == mad_mpi_old_realloc_hook)
                TBX_FAILURE("hooks corrupted");

        if (__free_hook == mad_mpi_old_free_hook)
                TBX_FAILURE("hooks corrupted");

        __malloc_hook		= mad_mpi_old_malloc_hook;
        __memalign_hook		= mad_mpi_old_memalign_hook;
        __free_hook		= mad_mpi_old_free_hook;
        __realloc_hook		= mad_mpi_old_realloc_hook;
        LOG_OUT();
}


static
void *
mad_mpi_malloc_hook(size_t len, const void *caller) {
        void *new_ptr = NULL;

        __mad_mpi_hook_lock();
        mad_mpi_remove_hooks();
        LOG_IN();
        new_ptr = malloc(len);
        LOG_OUT();
        mad_mpi_install_hooks();
        __mad_mpi_hook_unlock();

        return new_ptr;
}


static
void *
mad_mpi_memalign_hook(size_t alignment, size_t len, const void *caller) {
        void *new_ptr = NULL;

        __mad_mpi_hook_lock();
        mad_mpi_remove_hooks();
        LOG_IN();
        new_ptr = memalign(alignment, len);
        LOG_OUT();
        mad_mpi_install_hooks();
        __mad_mpi_hook_unlock();

        return new_ptr;
}


static
void *
mad_mpi_realloc_hook(void *ptr, size_t len, const void *caller) {
        void *new_ptr = NULL;

        __mad_mpi_hook_lock();
        mad_mpi_remove_hooks();
        LOG_IN();
        new_ptr = realloc(ptr, len);
        LOG_OUT();
        mad_mpi_install_hooks();
        __mad_mpi_hook_unlock();

        return new_ptr;
}


static
void
mad_mpi_free_hook(void *ptr, const void *caller) {

        __mad_mpi_hook_lock();
        mad_mpi_remove_hooks();
        LOG_IN();
        free(ptr);
        LOG_OUT();
        mad_mpi_install_hooks();
        __mad_mpi_hook_unlock();
}


static
void
mad_mpi_malloc_initialize_hook(void) {
        mad_mpi_malloc_hooked = 1;
        mad_mpi_install_hooks();
}
#endif /* MARCEL */

static
inline
void
mad_mpi_lock(void) {
#ifdef MARCEL
       __mad_mpi_hook_lock();
        mad_mpi_remove_hooks();
        //DISP("<mpi LOCK>");
#endif /* MARCEL */
}

static
inline
void
mad_mpi_unlock(void) {
#ifdef MARCEL
        //DISP("<mpi UNLOCK>");
        mad_mpi_install_hooks();
        __mad_mpi_hook_unlock();
#endif /* MARCEL */
}

#ifdef MARCEL
static
int
mad_mpi_do_poll(marcel_ev_server_t	server,
               marcel_ev_op_t		_op,
               marcel_ev_req_t		req,
               int			nb_ev,
               int			option) {
        p_mad_mpi_ev_t	p_ev	= NULL;
        int flag = 0;

        LOG_IN();
	p_ev = struct_up(req, mad_mpi_ev_t, inst);

        MPI_Test(&p_ev->mpi_request, &flag, p_ev->p_mpi_status);

        if (flag) {
                MARCEL_EV_REQ_SUCCESS(&(p_ev->inst));
        }
        LOG_OUT();

        return 0;
}
#endif /* MARCEL */

/*
 * exported functions
 * ------------------
 */

/* Registration function */
char *
mad_mpi_register(p_mad_driver_interface_t interface)
{
        LOG_IN();
        TRACE("Registering MPI driver");
        interface->driver_init                = mad_mpi_driver_init;
        interface->adapter_init               = mad_mpi_adapter_init;
        interface->channel_init               = mad_mpi_channel_init;
        interface->before_open_channel        = NULL;
        interface->connection_init            = mad_mpi_connection_init;
        interface->link_init                  = mad_mpi_link_init;
        interface->accept                     = mad_mpi_accept;
        interface->connect                    = mad_mpi_connect;
        interface->after_open_channel         = NULL;
        interface->before_close_channel       = NULL;
        interface->disconnect                 = NULL;
        interface->after_close_channel        = NULL;
        interface->link_exit                  = NULL;
        interface->connection_exit            = NULL;
        interface->channel_exit               = mad_mpi_channel_exit;
        interface->adapter_exit               = mad_mpi_adapter_exit;
        interface->driver_exit                = mad_mpi_driver_exit;
        interface->choice                     = NULL;
        interface->get_static_buffer          = NULL;
        interface->return_static_buffer       = NULL;
        interface->new_message                = mad_mpi_new_message;
        interface->receive_message            = mad_mpi_receive_message;
        interface->send_buffer                = mad_mpi_send_buffer;
        interface->receive_buffer             = mad_mpi_receive_buffer;
        interface->send_buffer_group          = mad_mpi_send_buffer_group;
        interface->receive_sub_buffer_group   = mad_mpi_receive_sub_buffer_group;
        LOG_OUT();

        return "mpi";
}

void
mad_mpi_driver_init(p_mad_driver_t    driver,
		   int               *p_argc,
		   char            ***p_argv)
{
        p_mad_mpi_driver_specific_t driver_specific;

        LOG_IN();
#ifdef MARCEL
        if (!mad_mpi_malloc_hooked) {
                mad_mpi_malloc_initialize_hook();
        }
#endif /* MARCEL */

        /* Driver module initialization code
           Note: called only once, just after
           module registration */
        driver->connection_type = mad_unidirectional_connection;

        /* Not used for now, but might be used in the future for
           dynamic buffer allocation */
        driver->buffer_alignment = 32;

        driver_specific = TBX_MALLOC(sizeof(mad_mpi_driver_specific_t));
        CTRL_ALLOC(driver_specific);
        driver->specific = driver_specific;
        TBX_INIT_SHARED(driver_specific);
        driver_specific->nb_adapter = 0;

#ifdef MARCEL
        marcel_ev_server_set_poll_settings(&mad_mpi_ev_server, MAD_MPI_POLLING_MODE, 1);
        marcel_ev_server_add_callback(&mad_mpi_ev_server, MARCEL_EV_FUNCTYPE_POLL_POLLONE, mad_mpi_do_poll);
#endif /* MARCEL */

        mad_mpi_lock();
        MPI_Init(p_argc, p_argv);
        mad_mpi_unlock();

        LOG_OUT();
}

void
mad_mpi_adapter_init(p_mad_adapter_t a)
{
        p_mad_mpi_adapter_specific_t   as	= NULL;

        LOG_IN();
        as = TBX_MALLOC(sizeof(mad_mpi_adapter_specific_t));

        if (strcmp(a->dir_adapter->name, "default")) {
                TBX_FAILURE("unsupported adapter");
        }

        a->parameter = tbx_strdup("-");
        LOG_OUT();
}

void
mad_mpi_channel_init(p_mad_channel_t ch)
{
        p_mad_mpi_channel_specific_t	chs			= NULL;
        p_tbx_string_t			parameter_string	= NULL;

        LOG_IN();
        chs = TBX_MALLOC(sizeof(mad_mpi_channel_specific_t));
        CTRL_ALLOC(chs);

        mad_mpi_lock();
        MPI_Comm_dup (MPI_COMM_WORLD, &chs->communicator);
        MPI_Comm_rank(MPI_COMM_WORLD, &chs->rank);
        MPI_Comm_size(MPI_COMM_WORLD, &chs->size);
        mad_mpi_unlock();

        ch->specific = chs;

        parameter_string   = tbx_string_init_to_int(chs->rank);
        ch->parameter = tbx_string_to_cstring(parameter_string);
        tbx_string_free(parameter_string);
        parameter_string   = NULL;
        LOG_OUT();
}

void
mad_mpi_connection_init(p_mad_connection_t in,
                        p_mad_connection_t out) {

        LOG_IN();
        if (in) {
                p_mad_mpi_connection_specific_t	is	= NULL;

                is = TBX_MALLOC(sizeof(mad_mpi_connection_specific_t));
                is->remote_rank = -1;
                in->specific = is;
                in->nb_link = 1;
        }

        if (out) {
                p_mad_mpi_connection_specific_t	os	= NULL;

                os = TBX_MALLOC(sizeof(mad_mpi_connection_specific_t));
                os->remote_rank = -1;
                out->specific = os;
                out->nb_link = 1;
        }
        LOG_OUT();
}


void
mad_mpi_link_init(p_mad_link_t lnk)
{
        LOG_IN();
        lnk->link_mode   = mad_link_mode_buffer;
        lnk->buffer_mode = mad_buffer_mode_dynamic;
        lnk->group_mode  = mad_group_mode_split;
        LOG_OUT();
}

void
mad_mpi_channel_exit(p_mad_channel_t ch)
{
        p_mad_mpi_channel_specific_t	chs	= NULL;

        LOG_IN();
        chs = ch->specific;

        mad_mpi_lock();
        MPI_Comm_free(&chs->communicator);
        mad_mpi_unlock();

        TBX_FREE(chs);
        ch->specific = NULL;
        LOG_OUT();
}

void
mad_mpi_adapter_exit(p_mad_adapter_t a)
{
        p_mad_mpi_adapter_specific_t as = NULL;

        LOG_IN();
        as		= a->specific;
        TBX_FREE(as);
        a->specific	= NULL;
        a->parameter	= NULL;
        LOG_OUT();
}

void
mad_mpi_connect_accept(p_mad_connection_t cnx,
                       p_mad_adapter_info_t ai)
{
        p_mad_mpi_connection_specific_t cs = NULL;

        LOG_IN();
        cs = cnx->specific;
        cs->remote_rank	= tbx_cstr_to_long(ai->channel_parameter);
        LOG_OUT();
}

void
mad_mpi_accept(p_mad_connection_t   in,
               p_mad_adapter_info_t ai)
{
        LOG_IN();
        mad_mpi_connect_accept(in, ai);
        LOG_OUT();
}

void
mad_mpi_connect(p_mad_connection_t   out,
                p_mad_adapter_info_t ai)
{
        LOG_IN();
        mad_mpi_connect_accept(out, ai);
        LOG_OUT();
}


void
mad_mpi_driver_exit(p_mad_driver_t d)
{
        LOG_IN();
        mad_mpi_lock();
        MPI_Finalize();
        mad_mpi_unlock();
        TBX_FREE (d->specific);
        d->specific = NULL;
        LOG_OUT();
}


void
mad_mpi_new_message(p_mad_connection_t out)
{
        p_mad_channel_t			ch		= NULL;
        p_mad_mpi_channel_specific_t	chs		= NULL;
        int				process_lrank	=   -1;

        LOG_IN();
        ch	= out->channel;
        chs	= ch->specific;

        process_lrank = (int)ch->process_lrank;
        MPI_Send(&process_lrank,
                 1,
                 MPI_INT,
                 out->remote_rank,
                 MAD_MPI_SERVICE_TAG,
                 chs->communicator);
        LOG_OUT();
}

p_mad_connection_t
mad_mpi_receive_message(p_mad_channel_t ch)
{
        p_mad_mpi_channel_specific_t	chs		= NULL;
        p_mad_connection_t		in		= NULL;
        p_tbx_darray_t			in_darray	= NULL;
        int				remote_lrank	=   -1;
        MPI_Status			status;

        LOG_IN();
        chs		= ch->specific;
        in_darray	= ch->in_connection_darray;

        MPI_Recv(&remote_lrank,
                 1,
                 MPI_INT,
                 MPI_ANY_SOURCE,
                 MAD_MPI_SERVICE_TAG,
                 chs->communicator,
                 &status);
        in	= tbx_darray_get(in_darray, remote_lrank);
        LOG_OUT();

        return in;
}

void
mad_mpi_send_buffer(p_mad_link_t     l,
		    p_mad_buffer_t   b)
{
        p_mad_connection_t		out	= NULL;
        p_mad_mpi_connection_specific_t	os	= NULL;
        p_mad_channel_t			ch	= NULL;
        p_mad_mpi_channel_specific_t	chs	= NULL;

        LOG_IN();
        out	= l->connection;
        ch	= out->channel;
        chs	= ch->specific;
        os	= out->specific;

#ifdef MARCEL
 {
        struct marcel_ev_wait	ev_w;
        mad_mpi_ev_t		ev = {
                .p_mpi_status	= MPI_STATUS_IGNORE
        };

        mad_mpi_lock();
        MPI_Isend(b->buffer,
                  b->length,
                  MPI_CHAR,
                  os->remote_rank,
                  MAD_MPI_TRANSFER_TAG,
                  chs->communicator,
                  &ev.mpi_request);
        mad_mpi_unlock();
        marcel_ev_wait(&mad_mpi_ev_server, &(ev.inst), &ev_w, 0);
 }
#else /* MARCEL */
        MPI_Send(b->buffer,
                 b->length,
                 MPI_CHAR,
                 os->remote_rank,
                 MAD_MPI_TRANSFER_TAG,
                 chs->communicator);
#endif /* MARCEL */
        b->bytes_read = b->length;
        LOG_OUT();
}

void
mad_mpi_receive_buffer(p_mad_link_t      l,
		       p_mad_buffer_t  *_b)
{
        p_mad_buffer_t			b 	= *_b;
        p_mad_connection_t		in	= NULL;
        p_mad_channel_t			ch	= NULL;
        p_mad_mpi_channel_specific_t	chs	= NULL;
        MPI_Status status;

        LOG_IN();
        in	= l->connection;
        ch	= in->channel;
        chs	= ch->specific;

#ifdef MARCEL
{
        struct marcel_ev_wait	ev_w;
        mad_mpi_ev_t		ev = {
                .p_mpi_status = &status
        };

        mad_mpi_lock();
        MPI_Irecv(b->buffer,
                  b->length,
                  MPI_CHAR,
                  in->remote_rank,
                  MAD_MPI_TRANSFER_TAG,
                  chs->communicator,
                  &ev.mpi_request);
        mad_mpi_unlock();
        marcel_ev_wait(&mad_mpi_ev_server, &(ev.inst), &ev_w, 0);
}
#else /* MARCEL */
        MPI_Recv(b->buffer,
                 b->length,
                 MPI_CHAR,
                 in->remote_rank,
                 MAD_MPI_TRANSFER_TAG,
                 chs->communicator,
                 &status);

        b->bytes_written = b->length;
#endif /* MARCEL */
        LOG_OUT();
}

void
mad_mpi_send_buffer_group(p_mad_link_t           l,
			  p_mad_buffer_group_t   bg)
{
        p_mad_connection_t		out	= NULL;
        p_mad_mpi_connection_specific_t	os	= NULL;
        p_mad_channel_t			ch	= NULL;
        p_mad_mpi_channel_specific_t	chs	= NULL;
        tbx_list_reference_t 		ref;
        int				n	= bg->buffer_list.length;
        MPI_Datatype			hindexed;

        LOG_IN();
        out	= l->connection;
        os	= out->specific;
        ch	= out->channel;
        chs	= ch->specific;

        if (tbx_empty_list(&(bg->buffer_list)))
                goto end;

        tbx_list_reference_init(&ref, &(bg->buffer_list));

#ifdef MARCEL
        mad_mpi_lock();
#endif /* MARCEL */

        if (n < MAD_MPI_GROUP_MALLOC_THRESHOLD) {
                int		length[n];
                MPI_Aint	displacement[n];
                int		i = 0;

                do {
                        p_mad_buffer_t b = NULL;

                        b = tbx_get_list_reference_object(&ref);

                        MPI_Address(b->buffer, &(displacement[i]));
                        length[i] = b->length;
                        i++;
                } while(tbx_forward_list_reference(&ref));

                MPI_Type_hindexed(n,
                                  length,
                                  displacement,
                                  MPI_BYTE,
                                  &hindexed);

                MPI_Type_commit(&hindexed);
        } else {
                int		*length		= NULL;
                MPI_Aint	*displacement	= NULL;
                int		 i		=    0;


                length		= TBX_CALLOC(n, sizeof(int));
                displacement	= TBX_CALLOC(n, sizeof(MPI_Aint));
                do {
                        p_mad_buffer_t b = NULL;

                        b = tbx_get_list_reference_object(&ref);

                        MPI_Address(b->buffer, &(displacement[i]));
                        length[i] = b->length;
                        i++;
                } while(tbx_forward_list_reference(&ref));

                MPI_Type_hindexed(n,
                                  length,
                                  displacement,
                                  MPI_BYTE,
                                  &hindexed);

                MPI_Type_commit(&hindexed);

                TBX_FREE(length);
                TBX_FREE(displacement);
        }


#ifdef MARCEL
 {
        struct marcel_ev_wait	ev_w;
        mad_mpi_ev_t		ev = {
                .p_mpi_status	= MPI_STATUS_IGNORE
        };

        MPI_Isend(MPI_BOTTOM,
                  1,
                  hindexed,
                  os->remote_rank,
                  MAD_MPI_TRANSFER_TAG,
                  chs->communicator,
                  &ev.mpi_request);
        mad_mpi_unlock();
        marcel_ev_wait(&mad_mpi_ev_server, &(ev.inst), &ev_w, 0);
 }
#else /* MARCEL */
        MPI_Send(MPI_BOTTOM,
                 1,
                 hindexed,
                 os->remote_rank,
                 MAD_MPI_TRANSFER_TAG,
                 chs->communicator);

        MPI_Type_free(&hindexed);
#endif /* MARCEL */

 end:
        LOG_OUT();
}

void
mad_mpi_receive_sub_buffer_group(p_mad_link_t           l,
				 tbx_bool_t             first_sub_group,
				 p_mad_buffer_group_t   bg)
{
        p_mad_connection_t		in	= NULL;
        p_mad_mpi_connection_specific_t	is	= NULL;
        p_mad_channel_t			ch	= NULL;
        p_mad_mpi_channel_specific_t	chs	= NULL;
        tbx_list_reference_t 		ref;
        int				n	= bg->buffer_list.length;
        MPI_Datatype			hindexed;
        MPI_Status			status;

        LOG_IN();
        in	= l->connection;
        is	= in->specific;
        ch	= in->channel;
        chs	= ch->specific;

        if (!first_sub_group)
                TBX_FAILURE("group split error");

        if (tbx_empty_list(&(bg->buffer_list)))
                goto end;


        tbx_list_reference_init(&ref, &(bg->buffer_list));

#ifdef MARCEL
        mad_mpi_lock();
#endif /* MARCEL */
        if (n < MAD_MPI_GROUP_MALLOC_THRESHOLD) {
                int		length[n];
                MPI_Aint	displacement[n];
                int		i = 0;

                do {
                        p_mad_buffer_t b = NULL;

                        b = tbx_get_list_reference_object(&ref);

                        MPI_Address(b->buffer, &(displacement[i]));
                        length[i] = b->length;
                        i++;
                } while(tbx_forward_list_reference(&ref));

                MPI_Type_hindexed(n,
                                  length,
                                  displacement,
                                  MPI_BYTE,
                                  &hindexed);

                MPI_Type_commit(&hindexed);
        } else {
                int		*length		= NULL;
                MPI_Aint	*displacement	= NULL;
                int		 i		=    0;


                length		= TBX_CALLOC(n, sizeof(int));
                displacement	= TBX_CALLOC(n, sizeof(MPI_Aint));
                do {
                        p_mad_buffer_t b = NULL;

                        b = tbx_get_list_reference_object(&ref);

                        MPI_Address(b->buffer, &(displacement[i]));
                        length[i] = b->length;
                        i++;
                } while(tbx_forward_list_reference(&ref));

                MPI_Type_hindexed(n,
                                  length,
                                  displacement,
                                  MPI_BYTE,
                                  &hindexed);

                MPI_Type_commit(&hindexed);

                TBX_FREE(length);
                TBX_FREE(displacement);
        }

#ifdef MARCEL
{
        struct marcel_ev_wait	ev_w;
        mad_mpi_ev_t		ev = {
                .p_mpi_status = &status
        };

        mad_mpi_lock();
        MPI_Irecv(MPI_BOTTOM,
                  1,
                  hindexed,
                  in->remote_rank,
                  MAD_MPI_TRANSFER_TAG,
                  chs->communicator,
                  &ev.mpi_request);
        mad_mpi_unlock();
        marcel_ev_wait(&mad_mpi_ev_server, &(ev.inst), &ev_w, 0);
}
#else /* MARCEL */
        MPI_Recv(MPI_BOTTOM,
                 1,
                 hindexed,
                 in->remote_rank,
                 MAD_MPI_TRANSFER_TAG,
                 chs->communicator,
                 &status);

        MPI_Type_free(&hindexed);
#endif /* MARCEL */

 end:
        LOG_OUT();
}


#if 0
/* External spawn support functions */

void
mad_mpi_external_spawn_init(p_mad_adapter_t   spawn_adapter
			    __attribute__ ((unused)),
			    int              *argc,
			    char            **argv)
{
        LOG_IN();
        /* External spawn initialization */
        MPI_Init(argc, &argv);
        LOG_OUT();
}

void
mad_mpi_configuration_init(p_mad_adapter_t       spawn_adapter
			   __attribute__ ((unused)),
			   p_mad_configuration_t configuration)
{
        ntbx_host_id_t               host_id;
        int                          rank;
        int                          size;

        LOG_IN();
        /* Code to get configuration information from the external spawn adapter */
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);

        configuration->local_host_id = (ntbx_host_id_t)rank;
        configuration->size          = (mad_configuration_size_t)size;
        configuration->host_name     =
                TBX_MALLOC(configuration->size * sizeof(char *));
        CTRL_ALLOC(configuration->host_name);

        for (host_id = 0;
             host_id < configuration->size;
             host_id++)
                {
                        configuration->host_name[host_id] = TBX_MALLOC(MAXHOSTNAMELEN + 1);
                        CTRL_ALLOC(configuration->host_name[host_id]);

                        if (host_id == rank)
                                {
                                        ntbx_host_id_t remote_host_id;

                                        gethostname(configuration->host_name[host_id], MAXHOSTNAMELEN);

                                        for (remote_host_id = 0;
                                             remote_host_id < configuration->size;
                                             remote_host_id++)
                                                {
                                                        int len;

                                                        len = strlen(configuration->host_name[host_id]);

                                                        if (remote_host_id != rank)
                                                                {
                                                                        MPI_Send(&len,
                                                                                 1,
                                                                                 MPI_INT,
                                                                                 remote_host_id,
                                                                                 MAD_MPI_SERVICE_TAG,
                                                                                 MPI_COMM_WORLD);
                                                                        MPI_Send(configuration->host_name[host_id],
                                                                                 len + 1,
                                                                                 MPI_CHAR,
                                                                                 remote_host_id,
                                                                                 MAD_MPI_SERVICE_TAG,
                                                                                 MPI_COMM_WORLD);
                                                                }
                                                }
                                }
                        else
                                {
                                        int len;
                                        MPI_Status status;

                                        MPI_Recv(&len,
                                                 1,
                                                 MPI_INT,
                                                 host_id,
                                                 MAD_MPI_SERVICE_TAG,
                                                 MPI_COMM_WORLD,
                                                 &status);

                                        MPI_Recv(configuration->host_name[host_id],
                                                 len + 1,
                                                 MPI_CHAR,
                                                 host_id,
                                                 MAD_MPI_SERVICE_TAG,
                                                 MPI_COMM_WORLD,
                                                 &status);
                                }
                }
        LOG_OUT();
}

void
mad_mpi_send_adapter_parameter(p_mad_adapter_t   spawn_adapter
			       __attribute__ ((unused)),
			       ntbx_host_id_t    remote_host_id,
			       char             *parameter)
{
        int len = strlen(parameter);

        LOG_IN();
        /* Code to send a string from the master node to one slave node */
        MPI_Send(&len,
                 1,
                 MPI_INT,
                 remote_host_id,
                 MAD_MPI_SERVICE_TAG,
                 MPI_COMM_WORLD);

        MPI_Send(parameter,
                 len + 1,
                 MPI_CHAR,
                 remote_host_id,
                 MAD_MPI_SERVICE_TAG,
                 MPI_COMM_WORLD);
        LOG_OUT();
}

void
mad_mpi_receive_adapter_parameter(p_mad_adapter_t   spawn_adapter
				  __attribute__ ((unused)),
				  char            **parameter)
{
        MPI_Status status;
        int len;

        LOG_IN();
        /* Code to receive a string from the master node */
        MPI_Recv(&len,
                 1,
                 MPI_INT,
                 -1,
                 MAD_MPI_SERVICE_TAG,
                 MPI_COMM_WORLD,
                 &status);

        *parameter = TBX_MALLOC(len);
        MPI_Recv(*parameter,
                 len + 1,
                 MPI_CHAR,
                 -1,
                 MAD_MPI_SERVICE_TAG,
                 MPI_COMM_WORLD,
                 &status);

        LOG_OUT();
}
#endif
