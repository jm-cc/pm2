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
 * Mad_sisci.c
 * ===========
 */
//#define USE_MARCEL_POLL
//#define OLD_SISCI
#define MAD_SISCI_POLLING_MODE \
    (MARCEL_POLL_AT_TIMER_SIG | MARCEL_POLL_AT_YIELD | MARCEL_POLL_AT_IDLE)

#undef PM2DEBUG
#ifdef MARCEL
#include "marcel.h"
#endif
#include "madeleine.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "sisci_api.h"
#include "sisci_types.h"
#include "sisci_error.h"

/*
 * constants
 * ---------
 */

/* Minimum alignment in bytes for using SCIMemCopy */
#define MAD_SISCI_SCIMEMCOPY_ALIGNMENT 4

/* Optimized SHMem */
     /*#define MAD_SISCI_OPT_MAX  112*/
#define MAD_SISCI_OPT_MAX  192
/* #define MAD_SISCI_OPT_MAX  0 */

/* SHMem */
#define MAD_SISCI_CHUNK_SIZE 65536

#define MAD_SISCI_BUFFER_SIZE (MAD_SISCI_CHUNK_SIZE + sizeof(mad_sisci_connection_status_t))
//#define MAD_SISCI_BUFFER_SIZE (65536 - sizeof(mad_sisci_connection_status_t))
//#define MAD_SISCI_BUFFER_SIZE (16384 - sizeof(mad_sisci_connection_status_t))
//#define MAD_SISCI_BUFFER_SIZE (32768- sizeof(mad_sisci_connection_status_t))
//#define MAD_SISCI_BUFFER_SIZE (65536 + sizeof(mad_sisci_connection_status_t))
//#define MAD_SISCI_BUFFER_SIZE (131072 + sizeof(mad_sisci_connection_status_t))
//#define MAD_SISCI_BUFFER_SIZE (32768 + sizeof(mad_sisci_connection_status_t))
//#define MAD_SISCI_BUFFER_SIZE (16384 + sizeof(mad_sisci_connection_status_t))
#define MAD_SISCI_MIN_SEG_SIZE (tbx_min(MAD_SISCI_CHUNK_SIZE, 8192))

/* Links */
#define MAD_SISCI_LINK_OPT      0
#define MAD_SISCI_LINK_REGULAR  1

#define C0 3
//#define C1 64
//#define C2 0x30
#define C1 32
#define C2 0x10
#define C3 0x7C
#define C4 0x10
#define C5 0x1C
//#define C3 0x00
//#define C4 0x00

/*
 * macros
 * ------
 */
/* status
 * note: 's' is supposed to be a pointer on a status structure
 */
#define MAD_SISCI_DISPLAY_ERROR(s) DISP("SISCI failure: " s "\n")
#define mad_sisci_set(s)    tbx_flag_set(&((s)->flag))
#define mad_sisci_clear(s)  tbx_flag_clear(&((s)->flag))
#define mad_sisci_toggle(s) tbx_flag_toggle(&((s)->flag))
#define mad_sisci_test(s)   tbx_flag_test(&((s)->flag))
#define mad_sisci_control() if (sisci_error != SCI_ERR_OK)\
 {mad_sisci_display_error(sisci_error);FAILURE("Aborting");} else {}

/*
 * local types
 * -----------
 */
typedef unsigned int   mad_sisci_adapter_id_t, *p_mad_sisci_adapter_id_t;
typedef int            mad_sisci_node_id_t,    *p_mad_sisci_node_id_t;
typedef unsigned int   mad_sisci_segment_id_t, *p_mad_sisci_segment_id_t;
typedef unsigned int   mad_sisci_segment_offset_t,
                                             *p_mad_sisci_segment_offset_t;
typedef unsigned int   mad_sisci_segment_size_t,
                                             *p_mad_sisci_segment_size_t;
typedef volatile void *mad_sisci_map_addr_t,   *p_mad_sisci_map_addr_t;

/*
 * local structures
 * ----------------
 */
typedef char mad_sisci_64B_block_t[64];
typedef char mad_sisci_1024B_block_t[1024];

typedef volatile union u_mad_sisci_status
{
  tbx_flag_t            flag;
  mad_sisci_64B_block_t buffer;
} mad_sisci_status_t, *p_mad_sisci_status_t;

typedef struct s_mad_sisci_connection_status
{
  mad_sisci_status_t      read;
  mad_sisci_1024B_block_t padding;
  mad_sisci_status_t      write;
} mad_sisci_connection_status_t, *p_mad_sisci_connection_status_t ;

typedef volatile struct s_mad_sisci_internal_segment_data
{
  mad_sisci_connection_status_t status;
  mad_sisci_node_id_t           node_id;
} mad_sisci_internal_segment_data_t, *p_mad_sisci_internal_segment_data_t;

typedef volatile struct s_mad_sisci_user_segment_data
{
  mad_sisci_connection_status_t status;
  char                          buffer[MAD_SISCI_BUFFER_SIZE];
} mad_sisci_user_segment_data_t, *p_mad_sisci_user_segment_data_t;

typedef struct s_mad_sisci_local_segment
{
  sci_local_segment_t        segment;
  sci_map_t                  map;
  mad_sisci_map_addr_t       map_addr;
  mad_sisci_segment_id_t     id;
  mad_sisci_segment_offset_t offset;
  mad_sisci_segment_size_t   size;
} mad_sisci_local_segment_t, *p_mad_sisci_local_segment_t;

typedef struct s_mad_sisci_remote_segment
{
  sci_remote_segment_t       segment;
  sci_map_t                  map;
  mad_sisci_map_addr_t       map_addr;
  mad_sisci_segment_id_t     id;
  mad_sisci_segment_offset_t offset;
  mad_sisci_segment_size_t   size;
  sci_sequence_t             sequence;
} mad_sisci_remote_segment_t, *p_mad_sisci_remote_segment_t;

typedef struct s_mad_sisci_driver_specific
{
  int nb_adapter;
#if defined(MARCEL) && defined(USE_MARCEL_POLL)
  struct marcel_ev_server mad_sisci_marcel_ev_server;
#endif
} mad_sisci_driver_specific_t, *p_mad_sisci_driver_specific_t;

typedef struct s_mad_sisci_adapter_specific
{
  mad_sisci_adapter_id_t local_adapter_id;
  mad_sisci_node_id_t    local_node_id;
  p_mad_sisci_node_id_t  remote_node_id;
} mad_sisci_adapter_specific_t, *p_mad_sisci_adapter_specific_t;

typedef struct s_mad_sisci_channel_specific
{
  /* Array of size configuration->size with adresse of flag for read */
  p_mad_sisci_status_t *read;
  int                   next;
  int                   max;
} mad_sisci_channel_specific_t, *p_mad_sisci_channel_specific_t;

typedef enum e_mad_sisci_poll_op
{
  mad_sisci_poll_channel,
  mad_sisci_poll_flag,
} mad_sisci_poll_op_t, *p_mad_sisci_poll_op_t;

typedef struct s_mad_sisci_poll_channel_data
{
  p_mad_channel_t    channel;
  p_mad_connection_t connection;
} mad_sisci_poll_channel_data_t, *p_mad_sisci_poll_channel_data_t;

typedef struct s_mad_sisci_poll_flag_data
{
  p_mad_sisci_status_t flag;
} mad_sisci_poll_flag_data_t, *p_mad_sisci_poll_flag_data_t;

typedef union u_mad_sisci_poll_data
{
  mad_sisci_poll_channel_data_t channel_op;
  mad_sisci_poll_flag_data_t    flag_op;
} mad_sisci_poll_data_t, *p_mad_sisci_poll_data_t;

typedef struct s_mad_sisci_marcel_ev_req
{
  struct marcel_ev_req  req;
  mad_sisci_poll_op_t   op;
  mad_sisci_poll_data_t data;
} mad_sisci_marcel_ev_req_t, *p_mad_sisci_marcel_ev_req_t;

typedef struct s_mad_sisci_connection_specific
{
  sci_desc_t                 sd[2];
  mad_sisci_local_segment_t  local_segment[2];
  mad_sisci_remote_segment_t remote_segment[2];
  int                        buffers_read;
  volatile tbx_bool_t        write_flag_flushed;
} mad_sisci_connection_specific_t, *p_mad_sisci_connection_specific_t;

typedef struct s_mad_sisci_link_specific
{
  int dummy;
} mad_sisci_link_specific_t, *p_mad_sisci_link_specific_t;

/*
 * static functions
 * ----------------
 */

static  char tmp[2048] __attribute__ ((aligned (4096)));


static
inline
void
cpy4(void *src,
     volatile void *dest,
     unsigned long nbytes)
{

  /*
   *

   We should use movntq instead of movq as store but it seems not to be available on PII

   *
   */

  __asm__ __volatile__(
    "    movl %0, %%esi \n\t"
    "    movl %1, %%edi \n\t"
    "    movl %2, %%ecx \n\t"
    "    movl %%ecx, %%ebx \n\t"
    "    shrl $11, %%ebx \n\t"
    "    jz 4f \n\t"

    "0: \n\t"
    "    pushl %%edi \n\t"
    "    mov %3, %%edi \n\t"
    "    mov $32, %%ecx \n\t"

    "1: \n\t"
//    "    prefetchnta 64(%%esi) \n\t"
//    "    prefetchnta 96(%%esi) \n\t"
    "    movq  0(%%esi), %%mm1 \n\t"
    "    movq  8(%%esi), %%mm2 \n\t"
    "    movq 16(%%esi), %%mm3 \n\t"
    "    movq 24(%%esi), %%mm4 \n\t"
    "    movq 32(%%esi), %%mm5 \n\t"
    "    movq 40(%%esi), %%mm6 \n\t"
    "    movq 48(%%esi), %%mm7 \n\t"
    "    movq 56(%%esi), %%mm0 \n\t"

    "    movq %%mm1,  0(%%edi) \n\t"
    "    movq %%mm2,  8(%%edi) \n\t"
    "    movq %%mm3, 16(%%edi) \n\t"
    "    movq %%mm4, 24(%%edi) \n\t"
    "    movq %%mm5, 32(%%edi) \n\t"
    "    movq %%mm6, 40(%%edi) \n\t"
    "    movq %%mm7, 48(%%edi) \n\t"
    "    movq %%mm0, 56(%%edi) \n\t"

    "    addl $64, %%esi \n\t"
    "    addl $64, %%edi \n\t"
    "    decl %%ecx \n\t"
    "    jnz 1b \n\t"

    "    popl %%edi \n\t"
    "    pushl %%esi \n\t"
    "    mov %3, %%esi \n\t"
    "    mov $32, %%ecx \n\t"

    "2: \n\t"
//    "    prefetchnta 64(%%esi) \n\t"
//    "    prefetchnta 96(%%esi) \n\t"
    "    movq  0(%%esi), %%mm1 \n\t"
    "    movq  8(%%esi), %%mm2 \n\t"
    "    movq 16(%%esi), %%mm3 \n\t"
    "    movq 24(%%esi), %%mm4 \n\t"
    "    movq 32(%%esi), %%mm5 \n\t"
    "    movq 40(%%esi), %%mm6 \n\t"
    "    movq 48(%%esi), %%mm7 \n\t"
    "    movq 56(%%esi), %%mm0 \n\t"
    "    movq %%mm1,  0(%%edi) \n\t"
    "    movq %%mm2,  8(%%edi) \n\t"
    "    movq %%mm3, 16(%%edi) \n\t"
    "    movq %%mm4, 24(%%edi) \n\t"
    "    movq %%mm5, 32(%%edi) \n\t"
    "    movq %%mm6, 40(%%edi) \n\t"
    "    movq %%mm7, 48(%%edi) \n\t"
    "    movq %%mm0, 56(%%edi) \n\t"
    "    addl $64, %%esi \n\t"
    "    addl $64, %%edi \n\t"
    "    decl %%ecx \n\t"
    "    jnz 2b \n\t"

    "    popl %%esi \n\t"
    "    decl %%ebx \n\t"
    "    jnz 0b \n\t"

    "    movl %2, %%ecx \n\t"
    "    movl %%ecx, %%ebx \n\t"
    "    movl $2047, %%eax \n\t"
    "    andl %%eax, %%ecx \n\t"
    "4: \n\t"
    "    movl %%ecx, %%ebx \n\t"
    "    shrl $6, %%ebx \n\t"
    "    jz 5f \n\t"

    "3: \n\t"
    "    prefetchnta 64(%%esi) \n\t"
    "    prefetchnta 96(%%esi) \n\t"
    "    movq  0(%%esi), %%mm1 \n\t"
    "    movq  8(%%esi), %%mm2 \n\t"
    "    movq 16(%%esi), %%mm3 \n\t"
    "    movq 24(%%esi), %%mm4 \n\t"
    "    movq 32(%%esi), %%mm5 \n\t"
    "    movq 40(%%esi), %%mm6 \n\t"
    "    movq 48(%%esi), %%mm7 \n\t"
    "    movq 56(%%esi), %%mm0 \n\t"
    "    movq %%mm1,  0(%%edi) \n\t"
    "    movq %%mm2,  8(%%edi) \n\t"
    "    movq %%mm3, 16(%%edi) \n\t"
    "    movq %%mm4, 24(%%edi) \n\t"
    "    movq %%mm5, 32(%%edi) \n\t"
    "    movq %%mm6, 40(%%edi) \n\t"
    "    movq %%mm7, 48(%%edi) \n\t"
    "    movq %%mm0, 56(%%edi) \n\t"
    "    addl $64, %%esi \n\t"
    "    addl $64, %%edi \n\t"
    "    decl %%ebx \n\t"
    "    jnz 3b \n\t"

    "5:  \n\t"
    "    emms \n\t"
    "    movl %2, %%ecx \n\t"
    "    movl %%ecx, %%ebx \n\t"
    "    movl $63, %%eax \n\t"
    "    andl %%eax, %%ecx \n\t"
    "    cld \n\t"
    "    rep movsb \n\t"
    : : "m" (src), "m" (dest), "m" (nbytes), "p" (tmp) : "esi", "edi", "eax", "ebx", "ecx", "cc", "memory");
  //: : "m" (src), "m" (dest), "m" (nbytes), "m" (tmp) : "esi", "edi", "eax", "ebx", "ecx", "cc", "memory");
}

static void
mad_sisci_display_error(sci_error_t error)
{
  DISP("SISCI error code : %u, %X", error, error);
  DISP("SISCI stripped error code : %u, %X",
       error & ~SCI_ERR_MASK, error & ~SCI_ERR_MASK);

  switch (error)
    {
    case SCI_ERR_OK:
      {
	MAD_SISCI_DISPLAY_ERROR("OK, should not have stopped here ...");
	break;
      }
    case SCI_ERR_BUSY:
      {
	MAD_SISCI_DISPLAY_ERROR("busy");
	break;
      }
    case SCI_ERR_FLAG_NOT_IMPLEMENTED:
      {
	MAD_SISCI_DISPLAY_ERROR("flag not implemented");
	break;
      }
    case SCI_ERR_ILLEGAL_FLAG:
      {
	MAD_SISCI_DISPLAY_ERROR("illegal flag");
	break;
      }
    case SCI_ERR_NOSPC:
      {
	MAD_SISCI_DISPLAY_ERROR("no space");
	break;
      }
    case SCI_ERR_API_NOSPC:
      {
	MAD_SISCI_DISPLAY_ERROR("no space (API)");
	break;
      }
    case SCI_ERR_HW_NOSPC:
      {
	MAD_SISCI_DISPLAY_ERROR("no space (hardware)");
	break;
      }
    case SCI_ERR_NOT_IMPLEMENTED:
      {
	MAD_SISCI_DISPLAY_ERROR("not implemented");
	break;
      }
    case SCI_ERR_ILLEGAL_ADAPTERNO:
      {
	MAD_SISCI_DISPLAY_ERROR("illegal adapter no");
	break;
      }
    case SCI_ERR_NO_SUCH_ADAPTERNO:
      {
	MAD_SISCI_DISPLAY_ERROR("no such adapter no");
	break;
      }
    case SCI_ERR_TIMEOUT:
      {
	MAD_SISCI_DISPLAY_ERROR("timeout");
	break;
      }
    case SCI_ERR_OUT_OF_RANGE:
      {
	MAD_SISCI_DISPLAY_ERROR("out of range");
	break;
      }
    case SCI_ERR_NO_SUCH_SEGMENT:
      {
	MAD_SISCI_DISPLAY_ERROR("no such segment");
	break;
      }
    case SCI_ERR_ILLEGAL_NODEID:
      {
	MAD_SISCI_DISPLAY_ERROR("illegal node id");
	break;
      }
    case SCI_ERR_CONNECTION_REFUSED:
      {
	MAD_SISCI_DISPLAY_ERROR("connection refused");
	break;
      }
    case SCI_ERR_SEGMENT_NOT_CONNECTED:
      {
	MAD_SISCI_DISPLAY_ERROR("segment not connected");
	break;
      }
    case SCI_ERR_SIZE_ALIGNMENT:
      {
	MAD_SISCI_DISPLAY_ERROR("size alignment");
	break;
      }
    case SCI_ERR_OFFSET_ALIGNMENT:
      {
	MAD_SISCI_DISPLAY_ERROR("offset alignment");
	break;
      }
    case SCI_ERR_ILLEGAL_PARAMETER:
      {
	MAD_SISCI_DISPLAY_ERROR("illegal parameter");
	break;
      }
    case SCI_ERR_MAX_ENTRIES:
      {
	MAD_SISCI_DISPLAY_ERROR("mad entries");
	break;
      }
    case SCI_ERR_SEGMENT_NOT_PREPARED:
      {
	MAD_SISCI_DISPLAY_ERROR("segment not prepared");
	break;
      }
    case SCI_ERR_ILLEGAL_ADDRESS:
      {
	MAD_SISCI_DISPLAY_ERROR("illegal address");
	break;
      }
    case SCI_ERR_ILLEGAL_OPERATION:
      {
	MAD_SISCI_DISPLAY_ERROR("illegal operation");
	break;
      }
    case SCI_ERR_ILLEGAL_QUERY:
      {
	MAD_SISCI_DISPLAY_ERROR("illegal query");
	break;
      }
    case SCI_ERR_SEGMENTID_USED:
      {
	MAD_SISCI_DISPLAY_ERROR("segment id used");
	break;
      }
    case SCI_ERR_SYSTEM:
      {
	MAD_SISCI_DISPLAY_ERROR("system");
	break;
      }
    case SCI_ERR_CANCELLED:
      {
	MAD_SISCI_DISPLAY_ERROR("cancelled");
	break;
      }
    case SCI_ERR_NO_SUCH_NODEID:
      {
	MAD_SISCI_DISPLAY_ERROR("no such node id");
	break;
      }
    case SCI_ERR_NODE_NOT_RESPONDING:
      {
	MAD_SISCI_DISPLAY_ERROR("node not responding");
	break;
      }
    case SCI_ERR_NO_REMOTE_LINK_ACCESS:
      {
	MAD_SISCI_DISPLAY_ERROR("no remote link access");
	break;
      }
    case SCI_ERR_NO_LINK_ACCESS:
      {
	MAD_SISCI_DISPLAY_ERROR("no link access");
	break;
      }
    case SCI_ERR_TRANSFER_FAILED:
      {
	MAD_SISCI_DISPLAY_ERROR("transfer failed");
	break;
      }
    default:
      {
	MAD_SISCI_DISPLAY_ERROR("Unknown error");
	break;
      }
    }
}


static void
mad_sisci_flush(p_mad_sisci_remote_segment_t segment)
{
#ifdef OLD_SISCI
   sci_error_t sisci_error = SCI_ERR_OK;
   LOG_IN();
   SCIStoreBarrier(segment->sequence, 0, &sisci_error);
   mad_sisci_control();
   LOG_OUT();
#else
   LOG_IN();
   SCIStoreBarrier(segment->sequence, 0);
   LOG_OUT();
#endif
}

/* mad_sisci_get_node_id: query an adapter for the local SCI node id
 * --------------------- */
static mad_sisci_node_id_t
mad_sisci_get_node_id(mad_sisci_adapter_id_t adapter_id)
{
  mad_sisci_node_id_t      node_id = 0;
  sci_desc_t               descriptor;
  sci_error_t              sisci_error;
#ifdef OLD_SISCI
   struct sci_query_adapter query_adapter;
#else
   sci_query_adapter_t      query_adapter;
#endif

   LOG_IN();

#ifndef OLD_SISCI
   SCIInitialize( 0 , &sisci_error);
   mad_sisci_control();
#endif

   SCIOpen(&descriptor, 0, &sisci_error);
   mad_sisci_control();

   query_adapter.subcommand     = SCI_Q_ADAPTER_NODEID;
   query_adapter.localAdapterNo = adapter_id;
   query_adapter.data           = &node_id;

  SCIQuery(SCI_Q_ADAPTER, &query_adapter, 0, &sisci_error);
  mad_sisci_control();

  SCIClose(descriptor, 0, &sisci_error);
  mad_sisci_control();

#ifndef OLD_SISCI
     SCITerminate();
#endif
  LOG_VAL("mad_sisci_get_node_id: val ", node_id);
  LOG_OUT();

  return node_id;
}

/* For marcel polling
 * --------------------- */
#if defined(MARCEL) && defined(USE_MARCEL_POLL)

static int mad_sisci_ev_pollone(marcel_ev_server_t server, 
				marcel_ev_op_t _op,
				marcel_ev_req_t req, 
				int nb_ev, int option)
{
  int status = 0;
  p_mad_sisci_marcel_ev_req_t info=NULL;

  LOG_IN();
  info=tbx_container_of(req, mad_sisci_marcel_ev_req_t, req);
  if (info->op == mad_sisci_poll_flag)
    {
      status = mad_sisci_test(info->data.flag_op.flag);
    }
  else if (info->op == mad_sisci_poll_channel)
    {
      p_mad_channel_t                channel          = NULL;
      p_mad_sisci_channel_specific_t channel_specific = NULL;
      p_mad_connection_t             in               = NULL;
      p_tbx_darray_t                 in_darray        = NULL;
      int                            next             =    0;
      int                            max              =    0;
      int                            i                =    0;

      channel          = info->data.channel_op.channel;
      channel_specific = channel->specific;
      in_darray        = channel->in_connection_darray;
      max              = channel_specific->max;
      next             = channel_specific->next;

      i = max;

      while (i--)
	{
	  next = (next + 1) % max;
	  in   = tbx_darray_get(in_darray, next);

	  if (in)
	    {
	      p_mad_sisci_connection_specific_t in_specific = NULL;

	      in_specific = in->specific;

	      if (!in_specific->write_flag_flushed)
		{
		  p_mad_sisci_remote_segment_t remote_segment = NULL;

		  remote_segment = &(in_specific->remote_segment[0]);

		  mad_sisci_flush(remote_segment);
		  in_specific->write_flag_flushed = tbx_true;
		}

	      if (mad_sisci_test(channel_specific->read[next]))
		{
		  info->data.channel_op.connection = in;
		  status                           =  1;
		  break;
		}
	    }
	}

      channel_specific->next = next;
    }
  else
    FAILURE("unknown polling operation");

  LOG_OUT();

  if (status) {
	  MARCEL_EV_REQ_SUCCESS(req);
  }

  return status;
}

inline static
void
mad_sisci_wait_for(p_mad_link_t         link,
		   p_mad_sisci_status_t flag)
{
  LOG_IN();
  if (!mad_sisci_test(flag))
    {
      p_mad_sisci_driver_specific_t    driver_specific = NULL;
      mad_sisci_marcel_ev_req_t req;
      struct marcel_ev_wait wait;

      driver_specific = link->connection->channel->adapter->driver->specific;

      req.op                = mad_sisci_poll_flag;
      req.data.flag_op.flag = flag;

      marcel_ev_wait(&driver_specific->mad_sisci_marcel_ev_server,
		     &req.req, &wait, 0);
    }
  LOG_OUT();
}

#else // MARCEL && USE_MARCEL_POLL
inline static
void
mad_sisci_wait_for(p_mad_link_t         link,
		   p_mad_sisci_status_t flag)
{
  while (!mad_sisci_test(flag))
    TBX_YIELD();
}

#endif // MARCEL && USE_MARCEL_POLL

/*
 * exported functions
 * ------------------
 */

void
mad_sisci_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface = NULL;

  LOG_IN();
  TRACE("Registering SISCI driver");
  interface = driver->interface;

  driver->connection_type = mad_bidirectional_connection;
  driver->buffer_alignment = 64;
  driver->name = tbx_strdup("sisci");

  interface->driver_init                = mad_sisci_driver_init;
  interface->adapter_init               = mad_sisci_adapter_init;
  interface->channel_init               = mad_sisci_channel_init;
  interface->before_open_channel        = mad_sisci_before_open_channel;
  interface->connection_init            = mad_sisci_connection_init;
  interface->link_init                  = mad_sisci_link_init;
  interface->accept                     = mad_sisci_accept;
  interface->connect                    = mad_sisci_connect;
  interface->after_open_channel         = mad_sisci_after_open_channel;
  interface->before_close_channel       = NULL;
  interface->disconnect                 = mad_sisci_disconnect;
  interface->after_close_channel        = NULL;
  interface->link_exit                  = NULL;
  interface->connection_exit            = NULL;
  interface->channel_exit               = mad_sisci_channel_exit;
  interface->adapter_exit               = mad_sisci_adapter_exit;
  interface->driver_exit                = NULL;
  interface->choice                     = mad_sisci_choice;
  interface->get_static_buffer          = NULL;
  interface->return_static_buffer       = NULL;
  interface->new_message                = NULL;
  interface->finalize_message           = NULL;
#ifdef MAD_MESSAGE_POLLING
  interface->poll_message               = mad_sisci_poll_message;
#endif /* MAD_MESSAGE_POLLING */
  interface->receive_message            = mad_sisci_receive_message;
  interface->message_received           = NULL;
  interface->send_buffer                = mad_sisci_send_buffer;
  interface->receive_buffer             = mad_sisci_receive_buffer;
  interface->send_buffer_group          = mad_sisci_send_buffer_group;
  interface->receive_sub_buffer_group   = mad_sisci_receive_sub_buffer_group;
  LOG_OUT();
}

void
mad_sisci_driver_init(p_mad_driver_t driver)
{
  p_mad_sisci_driver_specific_t driver_specific = NULL;
  struct marcel_ev_server *server = NULL;

  LOG_IN();
  TRACE("Initializing SISCI driver");
  driver_specific  = TBX_MALLOC(sizeof(mad_sisci_driver_specific_t));
  driver->specific = driver_specific;

#if defined(MARCEL) && defined(USE_MARCEL_POLL)
  server=&driver_specific->mad_sisci_marcel_ev_server;

  marcel_ev_server_init(server, "Mad SISCI");

  marcel_ev_server_set_poll_settings(server, 
				     MAD_SISCI_POLLING_MODE,
				     1);
  
  marcel_ev_server_add_callback(server,
				MARCEL_EV_FUNCTYPE_POLL_POLLONE,
				&mad_sisci_ev_pollone);
  marcel_ev_server_start(server);
  
#endif // MARCEL && USE_MARCEL_POLL
  LOG_OUT();
}

void
mad_sisci_adapter_init(p_mad_adapter_t adapter)
{
  p_mad_sisci_adapter_specific_t adapter_specific = NULL;
  p_tbx_string_t                 parameter_string = NULL;

  LOG_IN();
  adapter_specific  = TBX_CALLOC(1, sizeof(mad_sisci_adapter_specific_t));
  adapter->specific = adapter_specific;

  adapter->mtu      = 0xFFFFFFFFUL;

  if (!strcmp(adapter->dir_adapter->name, "default"))
    {
      adapter_specific->local_adapter_id = 0;
    }
  else
    {
      adapter_specific->local_adapter_id = atoi(adapter->dir_adapter->name);
    }

  adapter_specific->local_node_id =
    mad_sisci_get_node_id(adapter_specific->local_adapter_id);

  parameter_string   =
    tbx_string_init_to_int(adapter_specific->local_node_id);
  adapter->parameter = tbx_string_to_cstring(parameter_string);
  tbx_string_free(parameter_string);
  parameter_string   = NULL;
  LOG_OUT();
}

void
mad_sisci_channel_init(p_mad_channel_t channel)
{
  p_mad_dir_channel_t            dir_channel = NULL;
  p_mad_sisci_channel_specific_t specific    = NULL;
  ntbx_process_lrank_t           size        =   -1;

  LOG_IN();
  specific          = TBX_MALLOC(sizeof(mad_sisci_channel_specific_t));
  channel->specific = specific;
  dir_channel       = channel->dir_channel;
  size              = ntbx_pc_local_max(dir_channel->pc);
  specific->read    = TBX_MALLOC(sizeof(p_mad_sisci_status_t) * (size + 1));
  specific->next    = 0;
  specific->max     = size + 1;
  LOG_OUT();
}

void
mad_sisci_connection_init(p_mad_connection_t in,
			  p_mad_connection_t out)
{
  p_mad_channel_t                   channel          = NULL;
  p_mad_sisci_channel_specific_t    channel_specific = NULL;
  p_mad_adapter_t                   adapter          = NULL;
  p_mad_sisci_adapter_specific_t    adapter_specific = NULL;
  p_mad_sisci_connection_specific_t specific         = NULL;
  ntbx_process_lrank_t              lrank_l          =   -1;
  ntbx_process_lrank_t              lrank_r          =   -1;
  int                               segment          =    0;
  int                               ch_id            =    0;

  LOG_IN();
  specific      = TBX_MALLOC(sizeof(mad_sisci_connection_specific_t));
  in->specific  = specific;
  out->specific = specific;
  in->nb_link   = 2;
  out->nb_link  = 2;

  channel          = in->channel;
  channel_specific = channel->specific;
  adapter          = channel->adapter;
  adapter_specific = adapter->specific;

  ch_id = channel->dir_channel->id;

  if (channel->type == mad_channel_type_forwarding)
    {
      ch_id |= 0x80;
    }

  lrank_l = channel->process_lrank;
  lrank_r = in->remote_rank;

  for (segment = 0; segment < 2; segment++)
    {
      p_mad_sisci_local_segment_t     local_segment = NULL;
      p_mad_sisci_user_segment_data_t local_data    = NULL;
      sci_error_t                     sisci_error   = SCI_ERR_OK;
      int i                                         =    0;

      local_segment = &specific->local_segment[segment];

      local_segment->id =
	ch_id
	| (lrank_l << 8)
	| (lrank_r << 16)
	| (segment << 28);

      local_segment->size   = sizeof(mad_sisci_user_segment_data_t);
      local_segment->offset = 0;

      SCIOpen(&specific->sd[segment], 0, &sisci_error);
      mad_sisci_control();

      SCICreateSegment(specific->sd[segment],
		       &local_segment->segment,
		       local_segment->id,
		       local_segment->size,
		       0, NULL, 0, &sisci_error);
      mad_sisci_control();

      SCIPrepareSegment(local_segment->segment,
			adapter_specific->local_adapter_id,
			0, &sisci_error);
      mad_sisci_control();

      local_segment->map_addr =
	SCIMapLocalSegment(local_segment->segment,
			   &local_segment->map,
			   local_segment->offset,
			   local_segment->size,
			   NULL, 0, &sisci_error);
      mad_sisci_control();

     local_data = local_segment->map_addr;

      for (i = 0; i < MAD_SISCI_OPT_MAX; i++)
	{
	  local_data->status.read.buffer[i] = 0;
	}

      local_data->status.read.flag  = tbx_flag_clear;
      local_data->status.write.flag = tbx_flag_clear;


      if (!segment)
	{
	  channel_specific->read[lrank_r] = &(local_data->status.read);
	}

      SCISetSegmentAvailable(local_segment->segment,
			     adapter_specific->local_adapter_id,
			     0, &sisci_error);
      mad_sisci_control();
    }
  LOG_OUT();
}

void
mad_sisci_link_init(p_mad_link_t lnk)
{
  LOG_IN();
//  lnk->link_mode   = mad_link_mode_buffer_group;
  lnk->link_mode   = mad_link_mode_link_group;
  lnk->buffer_mode = mad_buffer_mode_dynamic;
  lnk->group_mode  = mad_group_mode_split;
  LOG_OUT();
}

void
mad_sisci_before_open_channel(p_mad_channel_t channel)
{
  LOG_IN();
  /* Nothing */
  LOG_OUT();
}

void
mad_sisci_connect(p_mad_connection_t   out,
		  p_mad_adapter_info_t ai)
{
  p_mad_channel_t                   channel            = NULL;
  p_mad_adapter_t                   adapter            = NULL;
  p_mad_dir_adapter_t               dir_remote_adapter = NULL;
  p_mad_sisci_adapter_specific_t    adapter_specific   = NULL;
  p_mad_sisci_connection_specific_t out_specific       = NULL;
  ntbx_process_lrank_t              lrank_l            =   -1;
  ntbx_process_lrank_t              lrank_r            =   -1;
  sci_error_t                       sisci_error        = SCI_ERR_OK;
  int                               channel_id         =    0;
  int                               segment            =    0;
  int                               remote_node_sci_id =    0;

  LOG_IN();
  channel            = out->channel;
  adapter            = channel->adapter;
  adapter_specific   = adapter->specific;
  out_specific       = out->specific;
  channel_id         = channel->dir_channel->id;
  dir_remote_adapter = ai->dir_adapter;
  remote_node_sci_id = atoi(dir_remote_adapter->parameter);

  if (channel->type == mad_channel_type_forwarding)
    {
      channel_id |= 0x80;
    }

  lrank_l = channel->process_lrank;
  lrank_r = out->remote_rank;

   for (segment = 0; segment < 2; segment++)
    {
      p_mad_sisci_remote_segment_t    remote_segment = NULL;
      p_mad_sisci_user_segment_data_t remote_data    = NULL;

      remote_segment = &(out_specific->remote_segment[segment]);

      remote_segment->id =
	channel_id
	| (lrank_r << 8)
	| (lrank_l << 16)
	| (segment << 28);

      remote_segment->size   = sizeof(mad_sisci_user_segment_data_t);
      remote_segment->offset = 0;

      do
	{
	  SCIConnectSegment(out_specific->sd[segment],
			    &remote_segment->segment,
			    remote_node_sci_id,
			    remote_segment->id,
			    adapter_specific->local_adapter_id,
			    0, NULL, SCI_INFINITE_TIMEOUT, 0, &sisci_error);

	  if (sisci_error != SCI_ERR_OK)
	    {
	      mad_sisci_display_error(sisci_error);
	      LOG("mad_sisci: could not connect, sleeping ...");
#ifdef MARCEL
	      marcel_delay(1000);
#else // MARCEL
	      sleep(1);
#endif // MARCEL
	      LOG("mad_sisci: could not connect, waking up");
	    }
	}
      while (sisci_error != SCI_ERR_OK);

      remote_segment->map_addr =
	SCIMapRemoteSegment(remote_segment->segment,
			    &remote_segment->map,
			    remote_segment->offset,
			    remote_segment->size,
			    NULL, 0, &sisci_error);
      mad_sisci_control();

      SCICreateMapSequence(remote_segment->map, &remote_segment->sequence,
			   0, &sisci_error);
      mad_sisci_control();

      remote_data = remote_segment->map_addr;
      mad_sisci_set(&remote_data->status.write);
      mad_sisci_flush(remote_segment);
      out_specific->write_flag_flushed = tbx_true;
    }
  LOG_OUT();
}

void
mad_sisci_accept(p_mad_connection_t   in,
		 p_mad_adapter_info_t ai)
{
  p_mad_channel_t                   channel            = NULL;
  p_mad_adapter_t                   adapter            = NULL;
  p_mad_dir_adapter_t               dir_remote_adapter = NULL;
  p_mad_sisci_adapter_specific_t    adapter_specific   = NULL;
  p_mad_sisci_connection_specific_t in_specific        = NULL;
  ntbx_process_lrank_t              lrank_l            =   -1;
  ntbx_process_lrank_t              lrank_r            =   -1;
  sci_error_t                       sisci_error        = SCI_ERR_OK;
  int                               channel_id         =    0;
  int                               segment            =    0;
  int                               remote_node_sci_id =    0;

  LOG_IN();
  channel            = in->channel;
  adapter            = channel->adapter;
  adapter_specific   = adapter->specific;
  in_specific        = in->specific;
  channel_id         = channel->dir_channel->id;
  dir_remote_adapter = ai->dir_adapter;
  remote_node_sci_id = atoi(dir_remote_adapter->parameter);

  if (channel->type == mad_channel_type_forwarding)
    {
      channel_id |= 0x80;
    }

  lrank_l = channel->process_lrank;
  lrank_r = in->remote_rank;

  for (segment = 0; segment < 2; segment++)
    {
      p_mad_sisci_remote_segment_t    remote_segment = NULL;
      p_mad_sisci_user_segment_data_t remote_data    = NULL;

      remote_segment = &(in_specific->remote_segment[segment]);

      remote_segment->id =
	channel_id
	| (lrank_r << 8)
	| (lrank_l << 16)
	| (segment << 28);

      remote_segment->size   = sizeof(mad_sisci_user_segment_data_t);
      remote_segment->offset = 0;

      do
	{
	  SCIConnectSegment(in_specific->sd[segment],
			    &remote_segment->segment,
			    remote_node_sci_id,
			    remote_segment->id,
			    adapter_specific->local_adapter_id,
			    0, NULL, SCI_INFINITE_TIMEOUT, 0, &sisci_error);

	  if (sisci_error != SCI_ERR_OK)
	    {
	      mad_sisci_display_error(sisci_error);
	      LOG("mad_sisci: could not connect, sleeping ...");
#ifdef MARCEL
	      marcel_delay(1000);
#else // MARCEL
	      sleep(1);
#endif // MARCEL
	      LOG("mad_sisci: could not connect, waking up");
	    }
	}
      while (sisci_error != SCI_ERR_OK);

      remote_segment->map_addr =
	SCIMapRemoteSegment(remote_segment->segment,
			    &remote_segment->map,
			    remote_segment->offset,
			    remote_segment->size,
			    NULL, 0, &sisci_error);
      mad_sisci_control();

      SCICreateMapSequence(remote_segment->map, &remote_segment->sequence,
			   0, &sisci_error);
      mad_sisci_control();

      remote_data = remote_segment->map_addr;
      mad_sisci_set(&remote_data->status.write);
      mad_sisci_flush(remote_segment);
      in_specific->write_flag_flushed = tbx_true;
    }
  LOG_OUT();
}

void
mad_sisci_after_open_channel(p_mad_channel_t channel)
{
  LOG_IN();
  /* Nothing */
  LOG_OUT();
}

void
mad_sisci_disconnect(p_mad_connection_t connection)
{
  p_mad_sisci_connection_specific_t   connection_specific =
    connection->specific;
  p_mad_channel_t                     channel             =
    connection->channel;
  p_mad_adapter_t                     adapter             = channel->adapter;
  p_mad_sisci_adapter_specific_t      adapter_specific    = adapter->specific;
  sci_error_t                         sisci_error         = SCI_ERR_OK;
  int k;

  LOG_IN();
  for (k = 0; k < 2; k++)
    {
      p_mad_sisci_local_segment_t  local_segment  =
	&(connection_specific->local_segment[k]);
      p_mad_sisci_remote_segment_t remote_segment =
	&(connection_specific->remote_segment[k]);

      SCIUnmapSegment(remote_segment->map, 0, &sisci_error);
      mad_sisci_control();

      SCIDisconnectSegment(remote_segment->segment, 0, &sisci_error);
      mad_sisci_control();

      SCIRemoveSequence(remote_segment->sequence, 0, &sisci_error);
      mad_sisci_control();

      SCISetSegmentUnavailable(local_segment->segment,
			       adapter_specific->local_adapter_id,
			       0, &sisci_error);
      mad_sisci_control();

      SCIUnmapSegment(local_segment->map, 0, &sisci_error);
      mad_sisci_control();

      SCIRemoveSegment(local_segment->segment, 0, &sisci_error);
      mad_sisci_control();

      SCIClose(connection_specific->sd[k], 0, &sisci_error);
      mad_sisci_control();
    }
  LOG_OUT();
}

p_mad_link_t
mad_sisci_choice(p_mad_connection_t connection,
		 size_t             size,
		 mad_send_mode_t    send_mode    __attribute__ ((unused)),
		 mad_receive_mode_t receive_mode __attribute__ ((unused)))
{
  if (size <= MAD_SISCI_OPT_MAX)
    {
      return connection->link_array[MAD_SISCI_LINK_OPT];
    }
  else
    {
      return connection->link_array[MAD_SISCI_LINK_REGULAR];
    }
}

void
mad_sisci_channel_exit(p_mad_channel_t channel)
{
  p_mad_sisci_channel_specific_t channel_specific = channel->specific;

  LOG_IN();
  TBX_FREE(channel_specific->read);
  TBX_FREE(channel_specific);
  channel->specific = NULL;
  LOG_OUT();
}

void
mad_sisci_adapter_exit(p_mad_adapter_t adapter)
{
  p_mad_sisci_adapter_specific_t adapter_specific = adapter->specific;

  LOG_IN();
  TBX_FREE(adapter_specific);
  adapter->specific = NULL;
  LOG_OUT();
}

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_sisci_poll_message(p_mad_channel_t channel)
{
  p_mad_sisci_channel_specific_t channel_specific = NULL;
  p_mad_connection_t             in               = NULL;
  p_tbx_darray_t                 in_darray        = NULL;
  int                            next             =    0;
  int                            max              =    0;
  int                            i                =    0;

  LOG_IN();
  channel_specific = channel->specific;
  in_darray        = channel->in_connection_darray;
  max              = channel_specific->max;
  next             = channel_specific->next;
  i                = max;

  while (i--)
    {
      next = (next + 1) % max;
      in   = tbx_darray_get(in_darray, next);

      if (in)
	{
	  p_mad_sisci_connection_specific_t in_specific = NULL;

	  in_specific = in->specific;

	  if (!in_specific->write_flag_flushed)
	    {
	      p_mad_sisci_remote_segment_t remote_segment = NULL;

	      remote_segment = &(in_specific->remote_segment[0]);

	      in_specific->write_flag_flushed = tbx_true;
	      mad_sisci_flush(remote_segment);
	    }

	  if (mad_sisci_test(channel_specific->read[next]))
	    goto found;
	}
    }

  in = NULL;

found:
  channel_specific->next = next;

  LOG_OUT();

  return in;
}
#endif /* MAD_MESSAGE_POLLING */

p_mad_connection_t
mad_sisci_receive_message(p_mad_channel_t channel)
{
  p_mad_sisci_channel_specific_t channel_specific = NULL;
  p_mad_connection_t             in               = NULL;
  p_tbx_darray_t                 in_darray        = NULL;
  int                            next             =    0;
  int                            max              =    0;

  LOG_IN();
  channel_specific = channel->specific;
  in_darray        = channel->in_connection_darray;
  max              = channel_specific->max;
  next             = channel_specific->next;

#if defined(MARCEL) && defined(USE_MARCEL_POLL)
  {
    p_mad_sisci_driver_specific_t    driver_specific = NULL;
    mad_sisci_marcel_ev_req_t req;
    struct marcel_ev_wait wait;

    driver_specific = channel->adapter->driver->specific;

    req.op                          = mad_sisci_poll_channel;
    req.data.channel_op.channel     = channel;
    req.data.channel_op.connection  = NULL;

    marcel_ev_wait(&driver_specific->mad_sisci_marcel_ev_server,
		   &req.req, &wait, 0);

    in = req.data.channel_op.connection;
  }
#else // MARCEL && USE_MARCEL_POLL
  while (tbx_true)
    {
      int i = 0;


      i = max;


      while (i--)
	{
	  next = (next + 1) % max;
	  in   = tbx_darray_get(in_darray, next);

	  if (in)
	    {
	      p_mad_sisci_connection_specific_t in_specific = NULL;

	      in_specific = in->specific;

	      if (!in_specific->write_flag_flushed)
		{
		  p_mad_sisci_remote_segment_t remote_segment = NULL;

		  remote_segment = &(in_specific->remote_segment[0]);

		  mad_sisci_flush(remote_segment);
		  in_specific->write_flag_flushed = tbx_true;
		}

	      if (mad_sisci_test(channel_specific->read[next]))
		goto found;
	    }
	}
      TBX_YIELD();
    }

found:
  channel_specific->next = next;
#endif // MARCEL && USE_MARCEL_POLL

  LOG_OUT();

  return in;
}


static void
mad_sisci_send_sci_buffer(p_mad_link_t   link,
		       p_mad_buffer_t buffer)
{
  p_mad_connection_t                out = link->connection;
  p_mad_sisci_connection_specific_t os  = out->specific;
  int k                                 = 0;
  size_t segment_size                   =
    os->remote_segment[0].size
    - sizeof(mad_sisci_connection_status_t);

  LOG_IN();

  if ((buffer->bytes_written - buffer->bytes_read) < (segment_size << C0))
    {
      segment_size = (buffer->bytes_written - buffer->bytes_read) >> C0;
      if (segment_size < MAD_SISCI_MIN_SEG_SIZE)
	segment_size = MAD_SISCI_MIN_SEG_SIZE;
    }

  if (mad_more_data(buffer))
    {
      do
	{
	  p_mad_sisci_local_segment_t       local_segment       =
	    &(os->local_segment[k]);
	  p_mad_sisci_remote_segment_t      remote_segment      =
	    &(os->remote_segment[k]);
	  char                             *source              =
	    buffer->buffer + buffer->bytes_read;
	  p_mad_sisci_user_segment_data_t   local_data          =
	    local_segment->map_addr;
	  p_mad_sisci_user_segment_data_t   remote_data         =
	    remote_segment->map_addr;
	  volatile p_mad_sisci_status_t     read                =
	    &remote_data->status.read;
	  volatile p_mad_sisci_status_t     write               =
	    &local_data->status.write;
	  size_t   size        =
	    tbx_min(buffer->bytes_written - buffer->bytes_read, segment_size);
	  size_t   mod_4       = size % 4;
	  sci_error_t sisci_error;

	  buffer->bytes_read += size;

	  mad_sisci_wait_for(link, write);

          if (((unsigned int) source) & (MAD_SISCI_SCIMEMCOPY_ALIGNMENT-1))
            {
              memcpy(remote_segment->map, source, size);
              source += size;
            }
          else
            {
              SCIMemCopy(source,
                         remote_segment->map,
                         sizeof(mad_sisci_connection_status_t),
                         size - mod_4,
                         0,
                         &sisci_error);
              mad_sisci_control();

              if (mod_4)
                {
                  volatile char    *destination =
                    remote_data->buffer + (size - mod_4);
                  source += size - mod_4;
                  size += 4 - mod_4;

                  *(unsigned int *)destination = 0;

                  while (mod_4--)
                    {
                      *destination++ = *source++;
                    }
                }
              else
                {
                  source += size;
                }
            }

/* 	  if (size & 60) */
/* 	    { */
/* 	      volatile int *destination = */
/* 		(volatile void *)remote_data->buffer + size; */

/* 	      do */
/* 		{ */
/* 		  *destination++ = 0; */
/* 		  size += 4; */
/* 		} */
/* 	      while (size & 60); */
/* 	    } */

	  if (size & 63)
	    {
	      volatile int *destination =
		(volatile void *)remote_data->buffer + size;

	      do
		{
		  *destination++ = 0;
		  size += 4;
		}
	      while (size & 63);
	    }

	  mad_sisci_flush(remote_segment);
	  mad_sisci_clear(write);
	  mad_sisci_set(read);
	  mad_sisci_flush(remote_segment);
	  k ^= 1;
	}
      while (mad_more_data(buffer));

      os->write_flag_flushed = tbx_true;
    }

  LOG_OUT();
}

static void
mad_sisci_receive_sci_buffer(p_mad_link_t   link,
			     p_mad_buffer_t buffer)
{
  p_mad_connection_t                connection          = link->connection;
  p_mad_sisci_connection_specific_t connection_specific =
    connection->specific;
  int k = 0;
  size_t segment_size                                   =
    connection_specific->remote_segment[0].size
    - sizeof(mad_sisci_connection_status_t);

  if ((buffer->length - buffer->bytes_written) < (segment_size << C0))
    {
      segment_size = (buffer->length - buffer->bytes_written) >> C0;
      if (segment_size < MAD_SISCI_MIN_SEG_SIZE)
	segment_size = MAD_SISCI_MIN_SEG_SIZE;
    }


  LOG_IN();

  if (!mad_buffer_full(buffer))
    {
      size_t  size;

      {
	p_mad_sisci_local_segment_t       local_segment       =
	  &(connection_specific->local_segment[k]);
	p_mad_sisci_remote_segment_t      remote_segment      =
	  &(connection_specific->remote_segment[k]);
	char                             *destination         =
	  buffer->buffer + buffer->bytes_written;
	p_mad_sisci_user_segment_data_t   local_data          =
	  local_segment->map_addr;
	p_mad_sisci_user_segment_data_t   remote_data         =
	  remote_segment->map_addr;
	p_mad_sisci_status_t              read                =
	  &local_data->status.read;
	p_mad_sisci_status_t              write               =
	  &remote_data->status.write;
	char   *source = local_data->buffer;
	size = tbx_min(buffer->length - buffer->bytes_written, segment_size);

	if (!connection_specific->write_flag_flushed)
	  {
	    connection_specific->write_flag_flushed = tbx_true;
	    mad_sisci_flush(remote_segment);
	  }

	mad_sisci_wait_for(link, read);

	/* memcpy(destination, source, size); */
	cpy4(source, destination, size);

	mad_sisci_clear(read);
	mad_sisci_set(write);
	buffer->bytes_written +=size;
	destination += size;
	k = 1;
      }

      while (!mad_buffer_full(buffer))
	{
	  p_mad_sisci_local_segment_t       local_segment       =
	    &(connection_specific->local_segment[k]);
	  p_mad_sisci_remote_segment_t      remote_segment      =
	    &(connection_specific->remote_segment[k]);
	  char                             *destination         =
	    buffer->buffer + buffer->bytes_written;
	  p_mad_sisci_user_segment_data_t   local_data          =
	    local_segment->map_addr;
	  p_mad_sisci_user_segment_data_t   remote_data         =
	    remote_segment->map_addr;
	  p_mad_sisci_status_t              read                =
	    &local_data->status.read;
	  p_mad_sisci_status_t              write               =
	    &remote_data->status.write;
	  char   *source = local_data->buffer;
	  size = tbx_min(buffer->length - buffer->bytes_written, segment_size);
	  mad_sisci_flush(remote_segment);
	  mad_sisci_wait_for(link, read);
//	  memcpy(destination, source, size);
          cpy4(source, destination, size);
	  mad_sisci_clear(read);
	  mad_sisci_set(write);
	  buffer->bytes_written +=size;
	  destination += size;
	  k ^= 1;
	}

      connection_specific->write_flag_flushed = tbx_false;
    }
  LOG_OUT();
}


static void
mad_sisci_send_sci_buffer_group_2(p_mad_link_t         link,
				  p_mad_buffer_group_t buffer_group)
{
  p_mad_connection_t                 out          = link->connection;
  p_mad_sisci_connection_specific_t  os           = out->specific;
  size_t                             offset       =    0;
  tbx_bool_t                         need_wait    = tbx_true;
  int                                k            =    0;
  p_mad_sisci_local_segment_t        ls           = NULL;
  p_mad_sisci_remote_segment_t       rs           = NULL;
  p_mad_sisci_user_segment_data_t    ld           = NULL;
  p_mad_sisci_user_segment_data_t    rd           = NULL;
  volatile p_mad_sisci_status_t      read         = NULL;
  volatile p_mad_sisci_status_t      write        = NULL;
  volatile char                     *destination  = NULL;
  size_t                             ds           =
    os->remote_segment[0].size
    - sizeof(mad_sisci_connection_status_t);
  size_t                             size         =    0;
  size_t                             mod_4        =    0;
  size_t                             aligned_size =    0;
  tbx_list_reference_t               ref;
  sci_error_t                        sisci_error;

  LOG_IN();
  {
    size_t gl = 0;

    tbx_list_reference_init(&ref, &buffer_group->buffer_list);

    do
      {
	p_mad_buffer_t  buffer = tbx_get_list_reference_object(&ref);
	gl += buffer->bytes_written - buffer->bytes_read;

	if (gl > (ds << C0))
	  goto transmission;
      }
    while (tbx_forward_list_reference(&ref));

    ds = gl >> C0;
    if (ds < MAD_SISCI_MIN_SEG_SIZE)
      ds = MAD_SISCI_MIN_SEG_SIZE;
  }


transmission:
  tbx_list_reference_init(&ref, &buffer_group->buffer_list);

  do
    {
      p_mad_buffer_t  buffer = tbx_get_list_reference_object(&ref);
      char           *source = buffer->buffer + buffer->bytes_read;

      while (mad_more_data(buffer))
	{
	  ls           = &(os->local_segment[k]);
	  rs           = &(os->remote_segment[k]);
	  ld           = ls->map_addr;
	  rd           = rs->map_addr;
	  read         = &rd->status.read;
	  write        = &ld->status.write;
	  destination  = rd->buffer + offset;
	  size         = tbx_min(buffer->bytes_written - buffer->bytes_read, ds - offset);
	  mod_4        = size % 4;
	  aligned_size = size - mod_4;

	  buffer->bytes_read += size;

	  if (need_wait)
	    {
	      mad_sisci_wait_for(link, write);
	      need_wait = 0;
	    }

          if (((unsigned int) source) & (MAD_SISCI_SCIMEMCOPY_ALIGNMENT-1))
            {
              memcpy(rs->map, source, size);
              source += size;
              offset += size;
            }
          else
            {
              SCIMemCopy(source, rs->map, sizeof(mad_sisci_connection_status_t) + offset,
                         aligned_size, 0, &sisci_error);
              mad_sisci_control();

              offset += aligned_size;
              source += aligned_size;

              while (mod_4--)        *(destination+(offset++)) = *source++;
            }

	  while (offset & 0x03)  *(destination+(offset++)) = 0;

	  if (tbx_aligned(offset, C1) >= ds)
	    {
	      while (offset & C2)
		{
		  *(unsigned int *)(destination+offset) = 0;
		  offset += 4;
		}

	      mad_sisci_flush(rs);
	      mad_sisci_clear(write);
	      mad_sisci_set(read);
	      mad_sisci_flush(rs);
	      k         ^= 1;
	      need_wait  = tbx_true;
	      offset     = 0;
	    }
	}
    }
  while (tbx_forward_list_reference(&ref));

  if (offset > 0)
    {
      while (offset & C2)
	{
	  *(unsigned int *)(destination+offset) = 0;
	  offset += 4;
	}

      mad_sisci_flush(rs);
      mad_sisci_clear(write);
      mad_sisci_set(read);
      mad_sisci_flush(rs);
    }

  os->write_flag_flushed = tbx_true;
  LOG_OUT();
}

static void
mad_sisci_receive_sci_buffer_group_2(p_mad_link_t         link,
				     p_mad_buffer_group_t buffer_group)
{
  p_mad_connection_t                 in     = link->connection;
  p_mad_sisci_connection_specific_t  is     = in->specific;
  tbx_bool_t                         need_wait = tbx_true;
  int                                k      =    0;
  p_mad_sisci_local_segment_t        ls     = NULL;
  p_mad_sisci_remote_segment_t       rs     = NULL;
  p_mad_sisci_user_segment_data_t    ld     = NULL;
  p_mad_sisci_user_segment_data_t    rd     = NULL;
  p_mad_sisci_status_t               read   = NULL;
  p_mad_sisci_status_t               write  = NULL;
  volatile char                     *source = NULL;
  size_t                             offset =    0;
  size_t                             size   =    0;
  size_t                             ss     =
    is->remote_segment[0].size - sizeof(mad_sisci_connection_status_t);
  tbx_list_reference_t               ref;

  LOG_IN();

 {
    size_t gl = 0;

    tbx_list_reference_init(&ref, &buffer_group->buffer_list);

    do
      {
	p_mad_buffer_t  buffer = tbx_get_list_reference_object(&ref);
	gl += buffer->length - buffer->bytes_written;

	if (gl > (ss << C0))
	  goto transmission;
      }
    while (tbx_forward_list_reference(&ref));

    ss = gl >> C0;
    if (ss < MAD_SISCI_MIN_SEG_SIZE)
      ss = MAD_SISCI_MIN_SEG_SIZE;
  }

transmission:
  tbx_list_reference_init(&ref, &buffer_group->buffer_list);

  if (!is->write_flag_flushed)
    {
      rs   = &(is->remote_segment[0]);
      rd   = rs->map_addr;
      ls     = &(is->local_segment[0]);
      ld     = ls->map_addr;
      read = &ld->status.read;

      is->write_flag_flushed = tbx_true;

      if (!mad_sisci_test(read))
	{
	  mad_sisci_flush(rs);
	}
    }

  do
    {
      p_mad_buffer_t  buffer      = tbx_get_list_reference_object(&ref);
      char           *destination = buffer->buffer + buffer->bytes_written;

      while (!mad_buffer_full(buffer))
	{
	  ls     = &(is->local_segment[k]);
	  rs     = &(is->remote_segment[k]);
	  ld     = ls->map_addr;
	  rd     = rs->map_addr;
	  read   = &ld->status.read;
	  write  = &rd->status.write;
	  source = ld->buffer;
	  size   = tbx_min(buffer->length - buffer->bytes_written, ss - offset);

	  buffer->bytes_written += size;

	  if (need_wait)
	    {
	      mad_sisci_wait_for(link, read);
	      need_wait = tbx_false;
	    }

	  //memcpy(destination, (const void *)(source + offset), size);
	  cpy4((const void *)(source + offset), destination, size);

	  offset += tbx_aligned(size, 4);
	  destination += size;

	  if (tbx_aligned(offset, C1) >= ss)
	    {
	      mad_sisci_clear(read);
	      mad_sisci_set(write);
	      mad_sisci_flush(rs);
	      is->write_flag_flushed = tbx_true;
	      k         ^= 1;
	      need_wait  = tbx_true;
	      offset     = 0;
	    }
	}
    }
  while (tbx_forward_list_reference(&ref));

  if (offset > 0)
    {
      mad_sisci_clear(read);
      mad_sisci_set(write);
      is->write_flag_flushed = tbx_false;
    }
  LOG_OUT();
}

static void
mad_sisci_send_sci_buffer_group_1(p_mad_link_t         link,
				  p_mad_buffer_group_t buffer_group)
{
  p_mad_connection_t                connection          = link->connection;
  p_mad_sisci_connection_specific_t connection_specific =
    connection->specific;
  p_mad_sisci_local_segment_t       local_segment       =
    &(connection_specific->local_segment[0]);
  p_mad_sisci_remote_segment_t      remote_segment      =
    &(connection_specific->remote_segment[0]);
  p_mad_sisci_user_segment_data_t   local_data          =
    local_segment->map_addr;
  p_mad_sisci_user_segment_data_t   remote_data         =
    remote_segment->map_addr;
  volatile p_mad_sisci_status_t     read                =
    &remote_data->status.read;
  volatile p_mad_sisci_status_t     write               =
    &local_data->status.write;
  tbx_list_reference_t              ref;
  volatile char                    *destination         =
    remote_data->buffer;
  size_t                            offset              = 0;
  size_t                            destination_size    =
    remote_segment->size - sizeof(mad_sisci_connection_status_t);

  LOG_IN();
  tbx_list_reference_init(&ref, &buffer_group->buffer_list);

  mad_sisci_wait_for(link, write);

  do
    {
      p_mad_buffer_t buffer = tbx_get_list_reference_object(&ref);
      char          *source = buffer->buffer + buffer->bytes_read;

      while (mad_more_data(buffer))
	{
	  size_t         size         =
	    tbx_min(buffer->bytes_written - buffer->bytes_read,
	    destination_size - offset);
	  size_t         mod_4        = size % 4;
	  size_t         aligned_size = size - mod_4;
	  sci_error_t    sisci_error;

	  buffer->bytes_read += size;

          if (((unsigned int) source) & (MAD_SISCI_SCIMEMCOPY_ALIGNMENT-1))
            {
              memcpy(remote_segment->map, source, size);
              source += size;
              offset += size;
            }
          else
            {
              SCIMemCopy(source,
                         remote_segment->map,
                         sizeof(mad_sisci_connection_status_t) + offset,
                         aligned_size,
                         0,
                         &sisci_error);
              mad_sisci_control();

              offset += aligned_size;
              source += aligned_size;

              while (mod_4--)
                {
                  *(destination+(offset++)) = *source++;
                }
            }

	  while (offset & 0x03)
	    {
	      *(destination+(offset++)) = 0;
	    }

	  if (tbx_aligned(offset, C1) >= destination_size)
	    {
	      while (offset & C2)
		{
		  *(unsigned int *)(destination+offset) = 0;
		  offset += 4;
		}

	      mad_sisci_flush(remote_segment);

	      mad_sisci_clear(write);
	      mad_sisci_set(read);
	      mad_sisci_flush(remote_segment);

	      mad_sisci_wait_for(link, write);

	      offset = 0;
	    }
	}
    }
  while (tbx_forward_list_reference(&ref));

  if (offset > 0)
    {
      while (offset & C2)
	{
	  *(unsigned int *)(destination+offset) = 0;
	  offset += 4;
	}

      mad_sisci_flush(remote_segment);

      mad_sisci_clear(write);
      mad_sisci_set(read);
      mad_sisci_flush(remote_segment);
    }

  connection_specific->write_flag_flushed = tbx_true;
  LOG_OUT();
}

static void
mad_sisci_receive_sci_buffer_group_1(p_mad_link_t         link,
				     p_mad_buffer_group_t buffer_group)
{
  p_mad_connection_t                connection          = link->connection;
  p_mad_sisci_connection_specific_t connection_specific =
    connection->specific;
  p_mad_sisci_local_segment_t       local_segment       =
    &(connection_specific->local_segment[0]);
  p_mad_sisci_remote_segment_t      remote_segment      =
    &(connection_specific->remote_segment[0]);
  p_mad_sisci_user_segment_data_t   local_data          =
    local_segment->map_addr;
  p_mad_sisci_user_segment_data_t   remote_data         =
    remote_segment->map_addr;
  p_mad_sisci_status_t              read                =
    &local_data->status.read;
  p_mad_sisci_status_t              write               =
    &remote_data->status.write;
  tbx_list_reference_t              ref;
  volatile char                    *source              =
    local_data->buffer;
  size_t                            offset              = 0;
  size_t                            source_size         =
    local_segment->size - sizeof(mad_sisci_connection_status_t);

  LOG_IN();

  tbx_list_reference_init(&ref, &buffer_group->buffer_list);

  if (!connection_specific->write_flag_flushed)
    {
      connection_specific->write_flag_flushed = tbx_true;

      if (!mad_sisci_test(read))
	{
	  mad_sisci_flush(remote_segment);

	  mad_sisci_wait_for(link, read);
	}
    }
  else
    {
      mad_sisci_wait_for(link, read);
    }

  do
    {
      p_mad_buffer_t buffer      = tbx_get_list_reference_object(&ref);
      char          *destination =
	buffer->buffer + buffer->bytes_written;

      while (!mad_buffer_full(buffer))
	{
	  size_t         size         =
	    tbx_min(buffer->length - buffer->bytes_written,
		source_size - offset);

	  buffer->bytes_written += size;
	  //memcpy(destination, (const void *)(source + offset), size);
	  cpy4((const void *)(source + offset), destination, size);

	  offset += tbx_aligned(size, 4);
	  destination += size;

	  if (tbx_aligned(offset, C1) >= source_size)
	    {
	      mad_sisci_clear(read);
	      mad_sisci_set(write);
	      mad_sisci_flush(remote_segment);
	      connection_specific->write_flag_flushed = tbx_true;
	      mad_sisci_wait_for(link, read);
	      offset = 0;
	    }
	}
    }
  while (tbx_forward_list_reference(&ref));

  if (offset > 0)
    {
      mad_sisci_clear(read);
      mad_sisci_set(write);
      connection_specific->write_flag_flushed = tbx_false;
    }
  LOG_OUT();
}

static
void
mad_sisci_send_sci_buffer_group_3(p_mad_link_t         lnk,
			  p_mad_buffer_group_t buffer_group)
{
  LOG_IN();
  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  mad_sisci_send_sci_buffer(lnk, tbx_get_list_reference_object(&ref));
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

static
void
mad_sisci_receive_sci_buffer_group_3(p_mad_link_t           lnk,
				     p_mad_buffer_group_t   buffer_group)
{
  LOG_IN();
  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  mad_sisci_receive_sci_buffer(lnk, tbx_get_list_reference_object(&ref));
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}


/*
 * Optimized routines
 * ------------------
 */
static void
mad_sisci_send_sci_buffer_opt(p_mad_link_t   link,
			      p_mad_buffer_t buffer)
{
  p_mad_connection_t                connection          = link->connection;
  p_mad_sisci_connection_specific_t connection_specific =
    connection->specific;
  p_mad_sisci_local_segment_t       local_segment       =
    &connection_specific->local_segment[0];
  p_mad_sisci_remote_segment_t      remote_segment      =
    &connection_specific->remote_segment[0];
  unsigned char                    *source              =
    buffer->buffer + buffer->bytes_read;
  p_mad_sisci_user_segment_data_t   local_data          =
    local_segment->map_addr;
  volatile p_mad_sisci_status_t     write               =
    &local_data->status.write;
  p_mad_sisci_user_segment_data_t   remote_data         =
    remote_segment->map_addr;
  register volatile unsigned int   *data_remote_ptr     =
    (volatile void *)remote_data->status.read.buffer;
  volatile unsigned int            *remote_ptr          = NULL;
  register unsigned int             descriptor          = 1;
  register unsigned int             mask_1              = 2;
  register size_t                   bread               =
    buffer->bytes_read;
  const size_t                      bwrite              =
    buffer->bytes_written;
  const size_t                      bwrite3             = bwrite - 3;
  int                               j                   = 0;

  LOG_IN();
  mad_sisci_wait_for(link, write);
  mad_sisci_clear(write);

#if 0
  __asm__(
    "    prefetchnta  32(%0) \n\t"
    "    prefetchnta  64(%0) \n\t"
    "    prefetchnta  96(%0) \n\t"
    "    prefetchnta 128(%0) \n\t"
    "    prefetchnta 160(%0) \n\t"
    : : "r" (source)
  );

  __asm__(
    "    prefetcht1  32(%0) \n\t"
    "    prefetcht1  64(%0) \n\t"
    "    prefetcht1  96(%0) \n\t"
    "    prefetcht1 128(%0) \n\t"
    "    prefetcht1 160(%0) \n\t"
    : : "r" (source)
  );
#endif


  remote_ptr = data_remote_ptr++;
  if (bwrite > 3)
    while (bread < bwrite3)
      {
	bread += 4;

	if (*(unsigned int *)source)
	  {
	    *data_remote_ptr++ = *(unsigned int *)source;
	    descriptor |= mask_1;
	  }

	source += 4;
	mask_1 <<= 1;

	j++;

	if (j >= 7)
	  {
	    *remote_ptr = descriptor;
	    descriptor  = 1;
	    mask_1      = 2;
	    j           = 0;

	    if (bread < bwrite)
	      {
		remote_ptr = data_remote_ptr++;
	      }
	  }
      }

  if (bread < bwrite)
    {
      unsigned int temp_buffer = 0;
      int          i           = 0;

      do
	{
	  temp_buffer += ((unsigned int)*source) << i;
	  source++;
	  i += 8;
	  bread++;
	}
      while (bread < bwrite);

      if (temp_buffer)
	{
	  *data_remote_ptr++ = temp_buffer;
	  descriptor |= mask_1;
	}
      *remote_ptr = descriptor;
    }
  else if (j)
    {
      *remote_ptr = descriptor;
    }

  if ((((unsigned int)data_remote_ptr) & C3) > C4)
    {
      /* while (((unsigned int)data_remote_ptr) & C2) */
      do
        {
          *data_remote_ptr++ = 0;
        }
      while (((unsigned int)data_remote_ptr) & C5);
    }

  mad_sisci_flush(remote_segment);
  buffer->bytes_read = bread;
  connection_specific->write_flag_flushed = tbx_true;
  LOG_OUT();
}

static void
mad_sisci_receive_sci_buffer_opt(p_mad_link_t   link,
				   p_mad_buffer_t buffer)
{
  p_mad_connection_t                connection          = link->connection;
  p_mad_sisci_connection_specific_t connection_specific =
    connection->specific;
  p_mad_sisci_local_segment_t       local_segment       =
    &connection_specific->local_segment[0];
  p_mad_sisci_remote_segment_t      remote_segment      =
    &connection_specific->remote_segment[0];
  unsigned char                    *destination         =
    buffer->buffer + buffer->bytes_written;
  p_mad_sisci_user_segment_data_t   local_data          =
    local_segment->map_addr;
  p_mad_sisci_user_segment_data_t   remote_data         =
    remote_segment->map_addr;
  p_mad_sisci_status_t              write               =
    &remote_data->status.write;
  volatile unsigned int            *local_ptr           =
    (volatile void *)local_data->status.read.buffer;
  unsigned int                      descriptor          = 0;
  unsigned int                      mask_1              = 2;
  int                               j                   = 0;
  const size_t                      blen                =
    buffer->length;
  const size_t                      blen3               =
    blen - 3;
  register size_t                   bwrite              =
    buffer->bytes_written;

  LOG_IN();
  if (!connection_specific->write_flag_flushed)
    {
      connection_specific->write_flag_flushed = tbx_true;
      mad_sisci_flush(remote_segment);
    }

#ifdef MARCEL
  while (!(descriptor = *local_ptr)) TBX_YIELD();
#endif /* MARCEL */

  if (blen > 3)
    while (bwrite < blen3)
      {

	if (!j)
	  {
	    while (!(descriptor = *local_ptr));
	    *local_ptr++ = 0;
	    mask_1       = 2;
	    j++;
	  }

	if (descriptor & mask_1)
	  {
	    unsigned int int_buffer;

	    while (!(int_buffer = *local_ptr));
	    *(unsigned int *)destination = int_buffer;
	    *local_ptr++ = 0;
	  }
	else
	  {
	    *(unsigned int *)destination = 0;
	  }

	mask_1 <<= 1;
	j = (j + 1) & 0x7;
	destination += 4;
	bwrite      += 4;
      }

  if (bwrite < blen)
    {
      unsigned int temp_buffer = 0;
      int          i           = 0;

      if (!j)
	{
	  while (!(descriptor = *local_ptr));
	  *local_ptr++ = 0;
	  mask_1       = 2;
	}

      if (descriptor & mask_1)
	{
	  while (!(temp_buffer = *local_ptr));
	  *local_ptr++ = 0;
	}
      else
	{
	  temp_buffer = 0;
	}

      do
	{
	  *destination++ = (unsigned char)(temp_buffer >> i);
	  i += 8;
	  bwrite++;
	}
      while (bwrite < blen);
    }

  mad_sisci_set(write);
  connection_specific->write_flag_flushed = tbx_false;
  //connection_specific->write_flag_flushed = tbx_true;
  //mad_sisci_flush(remote_segment);
  buffer->bytes_written = bwrite;
  LOG_OUT();
}

/*
 * send/receive interface
 * -----------------------
 */
void
mad_sisci_send_buffer(p_mad_link_t   lnk,
		      p_mad_buffer_t buffer)
{
  LOG_IN();
  if (lnk->id == MAD_SISCI_LINK_OPT)
    {
      mad_sisci_send_sci_buffer_opt(lnk, buffer);
    }
  else if (lnk->id == MAD_SISCI_LINK_REGULAR)
    {
      mad_sisci_send_sci_buffer(lnk, buffer);
    }
  else
    FAILURE("Invalid link ID");
  LOG_OUT();
}

void
mad_sisci_receive_buffer(p_mad_link_t   lnk,
			 p_mad_buffer_t *buffer)
{
  LOG_IN();
  if (lnk->id == MAD_SISCI_LINK_OPT)
    {
      mad_sisci_receive_sci_buffer_opt(lnk, *buffer);
    }
  else if (lnk->id == MAD_SISCI_LINK_REGULAR)
    {
      mad_sisci_receive_sci_buffer(lnk, *buffer);
    }
  else
    FAILURE("Invalid link ID");
  LOG_OUT();
}

void
mad_sisci_send_buffer_group(p_mad_link_t         lnk,
			    p_mad_buffer_group_t buffer_group)
{
  LOG_IN();
  if (   (lnk->id == MAD_SISCI_LINK_OPT)
      || (lnk->id == MAD_SISCI_LINK_REGULAR))
    {
      mad_sisci_send_sci_buffer_group_2(lnk, buffer_group);
    }
  else
    FAILURE("Invalid link ID");
  LOG_OUT();
}

void
mad_sisci_receive_sub_buffer_group(p_mad_link_t           lnk,
				   tbx_bool_t             first_sub_group
				   __attribute__ ((unused)),
				   p_mad_buffer_group_t   buffer_group)
{
  LOG_IN();
  if (   (lnk->id == MAD_SISCI_LINK_OPT)
      || (lnk->id == MAD_SISCI_LINK_REGULAR))
    {
      mad_sisci_receive_sci_buffer_group_2(lnk, buffer_group);
    }
  else
    FAILURE("Invalid link ID");
  LOG_OUT();
}

