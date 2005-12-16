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
 * Mad_shmem.c
 * ==========
 */


#include "madeleine.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>


/*
 * defines
 * ---------
 */
#define MAD_SHMEM_BUFFER 4096

/*
 * local structures
 * ----------------
 */
typedef struct s_mad_shmem_driver_specific {
        int dummy;
} mad_shmem_driver_specific_t, *p_mad_shmem_driver_specific_t;

typedef struct s_mad_shmem_adapter_specific {
        int dummy;
} mad_shmem_adapter_specific_t, *p_mad_shmem_adapter_specific_t;

typedef struct s_mad_shmem_channel_specific {
  tbx_darray_index_t last_idx;
} mad_shmem_channel_specific_t, *p_mad_shmem_channel_specific_t;

typedef struct s_mad_shmem_connection_specific {
        char *out_buf;
        char *in_buf;
} mad_shmem_connection_specific_t, *p_mad_shmem_connection_specific_t;

typedef struct s_mad_shmem_link_specific {
        int dummy;
} mad_shmem_link_specific_t, *p_mad_shmem_link_specific_t;

typedef struct s_mad_shmem_ctrl {
        volatile size_t	nb_bytes;
        volatile off_t	next_w;
        volatile off_t	next_r;
        volatile unsigned int	lock;
} mad_shmem_ctrl_t, *p_mad_shmem_ctrl_t;

/*
 * constants
 * ---------
 */
static const size_t	ctrl_len	= sizeof(mad_shmem_ctrl_t);
static const size_t	data_len	= MAD_SHMEM_BUFFER - ctrl_len;

/*
 * static functions
 * ----------------
 */

#ifdef X86_ARCH
static
unsigned int
mad_shmem_tas(volatile unsigned int *p_v) {
        unsigned int r;

        __asm__ __volatile__(
                             "lock; xchgl %0, %1"
                             : "=q" (r), "=m" (*p_v)
                             : "0"  (1), "m"  (*p_v)
                             : "memory");

        return r;
}
#else
#error shmem driver not available on this architecture
#endif

#ifdef MARCEL
#error unimplemented
#else /* !MARCEL */
static
void
mad_shmem_write(char		*sh_buf,
                p_mad_buffer_t	 b) {
        p_mad_shmem_ctrl_t	ctrl	= (p_mad_shmem_ctrl_t)sh_buf;

        sh_buf	+= ctrl_len;

        LOG_IN();
        while (mad_more_data(buffer)) {
                void	*ptr		= b->buffer        + b->bytes_read;
                size_t	 len		= b->bytes_written - b->bytes_read;
                size_t	 avail_len	= 0;
                off_t	 w		= 0;

                while (1) {
                        while (mad_shmem_tas(&ctrl->lock))
                                sched_yield;

                        avail_len	= data_len - ctrl->nb_bytes;

                        if (avail_len)
                                break;

                        ctrl->lock = 0;
                }

                len	= tbx_min(len, avail_len);
                w	= ctrl->next_w;

                if (w+len < data_len) {
                        memcpy(sh_buf+w, ptr, len);

                        if (w+len == data_len) {
                                ctrl->next_w	 = 0;
                        } else {
                                ctrl->next_w	+= len;
                        }
                } else {
                        size_t	_len	= data_len-w;

                        memcpy(sh_buf+w, ptr,      _len);
                        memcpy(sh_buf,   ptr+_len,  len-_len);

                        ctrl->next_w	= len-_len;
                }

                ctrl->nb_bytes	+= len;
                ctrl->lock	 = 0;
        }

        buffer->bytes_read	+= len;
        LOG_OUT();
}
#endif /* MARCEL */

#ifdef MARCEL
#error unimplemented
#else /* !MARCEL */
static
void
mad_shmem_read(char		*sh_buf,
               p_mad_buffer_t	 b) {
        p_mad_shmem_ctrl_t	ctrl	= (p_mad_shmem_ctrl_t)sh_buf;

        sh_buf	+= ctrl_len;

        LOG_IN();
        while (!mad_buffer_full(buffer)) {
                void	*ptr	= b->buffer + b->bytes_written;
                size_t	 len	= b->length - b->bytes_written;
                off_t	 r		= 0;

                while (1) {
                        while (mad_shmem_tas(&ctrl->lock))
                                sched_yield;

                        if (ctrl->nb_bytes)
                                break;

                        ctrl->lock = 0;
                }

                len	= tbx_min(len, ctrl->nb_bytes);
                r	= ctrl->next_r;

                if (r+len < data_len) {
                        memcpy(ptr, sh_buf+r, len);

                        if (r+len == data_len) {
                                ctrl->next_r	 = 0;
                        } else {
                                ctrl->next_r	+= len;
                        }
                } else {
                        size_t	_len	= data_len-r;

                        memcpy(ptr,      sh_buf+r, _len);
                        memcpy(ptr+_len, sh_buf,    len-_len);

                        ctrl->next_r	= len-_len;
                }

                ctrl->nb_bytes	-= len;
                ctrl->lock	 = 0;
        }

        buffer->bytes_written	+= len;
        LOG_OUT();
}
#endif /* MARCEL */

/*
 * Registration function
 * ---------------------
 */

char *
mad_shmem_register(p_mad_driver_interface_t interface)
{
        LOG_IN();
        TRACE("Registering SHMEM driver");

        interface->driver_init                = mad_shmem_driver_init;
        interface->adapter_init               = mad_shmem_adapter_init;
        interface->channel_init               = mad_shmem_channel_init;
        interface->before_open_channel        = NULL;
        interface->connection_init            = mad_shmem_connection_init;
        interface->link_init                  = mad_shmem_link_init;
        interface->accept                     = mad_shmem_accept;
        interface->connect                    = mad_shmem_connect;
        interface->after_open_channel         = mad_shmem_after_open_channel;
        interface->before_close_channel       = NULL;
        interface->disconnect                 = mad_shmem_disconnect;
        interface->after_close_channel        = NULL;
        interface->link_exit                  = NULL;
        interface->connection_exit            = NULL;
        interface->channel_exit               = NULL;
        interface->adapter_exit               = NULL;
        interface->driver_exit                = NULL;
        interface->choice                     = NULL;
        interface->get_static_buffer          = NULL;
        interface->return_static_buffer       = NULL;
        interface->new_message                = NULL;
        interface->finalize_message           = NULL;
#ifdef MAD_MESSAGE_POLLING
        interface->poll_message               = mad_shmem_poll_message;
#endif // MAD_MESSAGE_POLLING
        interface->receive_message            = mad_shmem_receive_message;
        interface->message_received           = NULL;
        interface->send_buffer                = mad_shmem_send_buffer;
        interface->receive_buffer             = mad_shmem_receive_buffer;
        interface->send_buffer_group          = mad_shmem_send_buffer_group;
        interface->receive_sub_buffer_group   = mad_shmem_receive_sub_buffer_group;
        LOG_OUT();

        return "shmem";
}


void
mad_shmem_driver_init(p_mad_driver_t driver, int *argc, char ***argv)
{
        p_mad_shmem_driver_specific_t driver_specific = NULL;

        LOG_IN();
        TRACE("Initializing SHMEM driver");
        driver->connection_type  = mad_bidirectional_connection;
        driver->buffer_alignment = 32;
        driver_specific  = TBX_MALLOC(sizeof(mad_shmem_driver_specific_t));
        driver->specific = driver_specific;
        LOG_OUT();
}

void
mad_shmem_adapter_init(p_mad_adapter_t adapter)
{
        p_mad_shmem_adapter_specific_t	adapter_specific	= NULL;
        p_tbx_string_t		 	parameter_string	= NULL;
        int 					sock			= 0;
        socklen_t				len			=
                sizeof (struct sockaddr_in);
        pid_t					pid			= 0;
        struct sockaddr_in			address;
        struct sockaddr_in			temp;

        LOG_IN();
        if (!tbx_streq(adapter->dir_adapter->name, "default"))
                FAILURE("unsupported adapter");

        adapter_specific	= TBX_MALLOC(sizeof(mad_shmem_adapter_specific_t));
        adapter->specific	= adapter_specific;
        adapter->mtu		= MAD_SHMEM_BUFFER - sizeof(mad_shmem_ctrl_t);

        pid			= getpid();
        parameter_string	= tbx_string_init_to_int((int)pid);
        adapter->parameter	= tbx_string_to_cstring(parameter_string);
        tbx_string_free(parameter_string);
        parameter_string	= NULL;
        LOG_OUT();
}

void
mad_shmem_channel_init(p_mad_channel_t ch)
{
        p_mad_shmem_channel_specific_t chs = NULL;

        LOG_IN();
        chs		= TBX_MALLOC(sizeof(mad_shmem_channel_specific_t));
        chs->next_idx	= 0;
        ch->specific	= chs;
        LOG_OUT();
}

void
mad_shmem_connection_init(p_mad_connection_t in,
                          p_mad_connection_t out)
{
        p_mad_shmem_connection_specific_t	  cs		= NULL;
        p_mad_adapter_t			 	  a		= NULL;
        p_tbx_string_t		 		  filename	= NULL;
        char					*_filename	= NULL;
        int					  fd		=    0;
        int					  v		=    0;

        LOG_IN();
        a	= in->channel->adapter;

        cs		= TBX_MALLOC(sizeof(mad_shmem_connection_specific_t));
        cs->out_buf	= NULL;

        filename	= tbx_string_init_to_cstring(a->parameter);

        tbx_string_append_cstring	(filename, "-");
        tbx_string_append_cstring	(filename, channel->name);
        tbx_string_append_cstring	(filename, "-");
        tbx_string_append_int		(filename, in->remote_rank);

        _filename	= tbx_string_to_cstring(filename);
        tbx_string_free(filename);
        filename	= NULL;

        v	= shm_open(_filename, O_CREAT|O_TRUNC|O_RDWR, 0600);
        if (v < 0)
                ERROR("shm_open");

        fd	= v;

        v	= ftruncate(fd, (off_t)MAD_SHMEM_BUFFER);
        if (v < 0)
                ERROR("ftruncate");

        cs->in_ptr	= mmap(0, (size_t)MAD_SHMEM_BUFFER,
                               PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if (!cs->in_ptr)
                ERROR("mmap");

        v	= close(fd);
        if (v < 0)
                ERROR("close");

        in->specific	= out->specific = cs;
        in->nb_link	= 1;
        out->nb_link	= 1;
        LOG_OUT();
}

void
mad_shmem_link_init(p_mad_link_t lnk)
{
        LOG_IN();
        lnk->link_mode   = mad_link_mode_buffer;
        lnk->buffer_mode = mad_buffer_mode_dynamic;
        lnk->group_mode  = mad_group_mode_aggregate;
        LOG_OUT();
}

void
mad_shmem_after_open_channel(p_mad_channel_t channel)
{
        p_mad_shmem_channel_specific_t	chs		= NULL;
        p_tbx_darray_t			in_darray	= NULL;

        LOG_IN();
        chs		= ch->specific;
        in_darray	= channel->in_connection_darray;
        tbx_darray_first_idx(in_darray, &chs->next_idx);
        LOG_OUT();
}

void
mad_shmem_accept(p_mad_connection_t	in		TBX_UNUSED,
                 p_mad_adapter_info_t	adapter_info	TBX_UNUSED)
{
        LOG_IN();
        /* Nothing */
        LOG_OUT();
}

void
mad_shmem_connect(p_mad_connection_t   out,
                  p_mad_adapter_info_t ai)
{
        p_mad_shmem_connection_specific_t	  cs		= NULL;
        p_mad_dir_adapter_t			  ra		= NULL;
        p_tbx_string_t		 		  filename	= NULL;
        char					*_filename	= NULL;
        int					  fd		=    0;
        int					  v		=    0;

        LOG_IN();
        cs	= out->specific;
        ra	= ai->dir_adapter;

        filename	= tbx_string_init_to_cstring(ra->parameter);

        tbx_string_append_cstring	(filename, "-");
        tbx_string_append_cstring	(filename, channel->name);
        tbx_string_append_cstring	(filename, "-");
        tbx_string_append_int		(filename, in->remote_rank);

        _filename	= tbx_string_to_cstring(filename);
        tbx_string_free(filename);
        filename	= NULL;

        v	= shm_open(_filename, O_RDWR, 0600);
        if (v < 0)
                ERROR("shm_open");

        fd	= v;

        v	= shm_unlink(_filename);
        if (v < 0)
                ERROR("shm_unlink");

        cs->out_ptr	= mmap(0, (size_t)MAD_SHMEM_BUFFER,
                               PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if (!cs->out_ptr)
                ERROR("mmap");

        v	= close(fd);
        if (v < 0)
                ERROR("close");
        LOG_OUT();
}

void
mad_shmem_disconnect(p_mad_connection_t cnx)
{
        p_mad_shmem_connection_specific_t	cs	= cnx->specific;
        int					v	=    0;

        LOG_IN();
        if (cs->in_ptr) {
                v	= munmap(cs->in_ptr, MAD_SHMEM_BUFFER);
                if (v < 0)
                        ERROR("munmap");

                cs->in_ptr	= NULL;
        }

        if (cs->out_ptr) {
                v	= munmap(cs->out_ptr, MAD_SHMEM_BUFFER);
                if (v < 0)
                        ERROR("munmap");

                cs->out_ptr	= NULL;
        }
        LOG_OUT();
}

#ifdef MAD_MESSAGE_POLLING
#error unimplemented
p_mad_connection_t
mad_shmem_poll_message(p_mad_channel_t ch)
{
        return NULL;
}
#endif // MAD_MESSAGE_POLLING

#ifdef MARCEL
#error unimplemented
#else /* !MARCEL */
p_mad_connection_t
mad_shmem_receive_message(p_mad_channel_t ch)
{
        p_mad_shmem_channel_specific_t	chs		= NULL;
        p_tbx_darray_t			in_darray	= NULL;
        p_mad_connection_t		first_in	= NULL;
        p_mad_connection_t		in		= NULL;

        LOG_IN();
        chs		= ch->specific;
        in_darray	= channel->in_connection_darray;

        while (1) {
                p_mad_shmem_connection_specific_t	is	= NULL;
                p_mad_shmem_ctrl_t			ctrl	= NULL;

                in	= tbx_darray_get(in_darray, &chs->next_idx);

                if (!tbx_darray_next_idx(in_darray, &chs->idx))
                    tbx_darray_first_idx(in_darray, &chs->idx);

                is	= in->specific;
                ctrl	= (p_mad_shmem_ctrl_t)is->in_buf;

                if (!mad_shmem_tas(&ctrl->lock)) {
                        size_t	nb_bytes	= ctrl->nb_bytes;

                        ctrl->lock	= 0;
                        if (nb_bytes)
                                break;
                }

                if (!first_in) {
                        first_in = in;
                } else if (first_in == in) {
                        sched_yield();
                }
        }
        LOG_OUT();

        return in;
}
#endif /* MARCEL */

void
mad_shmem_send_buffer(p_mad_link_t     lnk,
                      p_mad_buffer_t   buffer)
{
        p_mad_shmem_connection_specific_t cs =
                lnk->connection->specific;

        LOG_IN();
        mad_shmem_write(cs->out_buf, buffer);
        LOG_OUT();
}

void
mad_shmem_receive_buffer(p_mad_link_t    lnk,
                         p_mad_buffer_t *buffer)
{
        p_mad_shmem_connection_specific_t cs =
                lnk->connection->specific;

        LOG_IN();
        mad_shmem_read(cs->is_buf, *buffer);
        LOG_OUT();
}

void
mad_shmem_send_buffer_group_1(p_mad_link_t         lnk,
                              p_mad_buffer_group_t buffer_group)
{
        LOG_IN();
        if (!tbx_empty_list(&(buffer_group->buffer_list)))
                {
                        p_mad_shmem_connection_specific_t connection_specific =
                                lnk->connection->specific;
                        tbx_list_reference_t            ref;

                        tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
                        do
                                {
                                        mad_shmem_write(connection_specific->socket,
                                                        tbx_get_list_reference_object(&ref));
                                }
                        while(tbx_forward_list_reference(&ref));
                }
        LOG_OUT();
}

void
mad_shmem_receive_sub_buffer_group_1(p_mad_link_t         lnk,
                                     p_mad_buffer_group_t buffer_group)
{
        LOG_IN();
        if (!tbx_empty_list(&(buffer_group->buffer_list)))
                {
                        p_mad_shmem_connection_specific_t connection_specific =
                                lnk->connection->specific;
                        tbx_list_reference_t            ref;

                        tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
                        do
                                {
                                        mad_shmem_read(connection_specific->socket,
                                                       tbx_get_list_reference_object(&ref));
                                }
                        while(tbx_forward_list_reference(&ref));
                }
        LOG_OUT();
}

void
mad_shmem_send_buffer_group(p_mad_link_t         lnk,
                            p_mad_buffer_group_t buffer_group)
{
        LOG_IN();
        mad_shmem_send_buffer_group_1(lnk, buffer_group);
        LOG_OUT();
}

void
mad_shmem_receive_sub_buffer_group(p_mad_link_t         lnk,
                                   tbx_bool_t           first_sub_group
				   __attribute__ ((unused)),
                                   p_mad_buffer_group_t buffer_group)
{
        LOG_IN();
        mad_shmem_receive_sub_buffer_group_1(lnk, buffer_group);
        LOG_OUT();
}

