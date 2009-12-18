/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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


#include <stdint.h>
#include <unistd.h>
#include <limits.h>
#include <sys/uio.h>
#include <assert.h>
#include <myriexpress.h>

#include <nm_private.h>

//#define DEBUG 1

/** Initial number of packet wrappers */
#define INITIAL_PW_NUM		16

#ifdef PIOMAN
piom_spinlock_t  nm_mx_lock;
#endif /* PIOMAN */

/** MX packet wrapper allocator */
static p_tbx_memory_t mx_pw_mem;

/** MX specific driver data */
struct nm_mx_drv
{
  uint32_t board_number;        /**< Board number */
  mx_endpoint_t ep;             /**< MX endpoint */
  uint32_t ep_id;               /**< Endpoint ID */
  struct nm_drv_cap caps;       /**< Capabilities of the driver */
  char*url;                     /**< driver url, containing MX hostname and endpoint ID */
  struct nm_mx_trk*trks_array;  /**< tracks of the MX driver*/
  int nb_trks;                  /**< number of tracks */
};

struct nm_mx_trk 
{
  /** Next value to use for match info */
  uint16_t next_peer_id;
  /** Match info to gate reverse mapping */
  nm_gate_t *gate_map;
};

/** MX specific connection data */
struct nm_mx_cnx 
{
  /** Remote endpoint addr */
  mx_endpoint_addr_t r_ep_addr;
  /** Remote endpoint id */
  uint32_t r_ep_id;
  /** MX match info (when sending) */
  uint64_t send_match_info;
  /** MX match info (when receiving) */
  uint64_t recv_match_info;
};

/** MX specific gate data */
struct nm_mx 
{
  struct nm_mx_cnx cnx_array[NM_SO_MAX_TRACKS];
};

/** MX specific packet wrapper data */
struct nm_mx_pkt_wrap 
{
  mx_endpoint_t *p_ep;
  mx_request_t rq;
#ifdef PROFILE
  int send_bool;
#endif
};

/** MX specific first administrative packet data */
struct nm_mx_adm_pkt_1 
{
  uint64_t match_info;
  char url[MX_MAX_HOSTNAME_LEN + 16];
};

/** MX specific second administrative packet data */
struct nm_mx_adm_pkt_2 
{
  uint64_t match_info;
};

/** Endpoint filter */
/* Note: if this value fails, please use the probable date of the
   granting of tenure ("titularisation" in french) to Brice:
   0x01042010. Sorry for the inconvenience. */
#define NM_MX_ENDPOINT_FILTER 0x31051969

/*
 * How the matching is used
 */

/** @name Administrative packets matching mask
 * Bit 63: administrative packet or not?
 */
/*@{*/
#define NM_MX_ADMIN_MATCHING_BITS 1
#define NM_MX_ADMIN_MATCHING_SHIFT 63
/*@}*/

/** @name Peer id matching mask
 * Bits 47-62: peer id
 */
/*@{*/
#define NM_MX_PEER_ID_MATCHING_BITS 16
#define NM_MX_PEER_ID_MATCHING_SHIFT 47
/*@}*/

/** @name Track id matching mask
 * Bits 39-46: track id (internally multiplexed in MX 1.2)
 */
/*@{*/
#define NM_MX_TRACK_ID_MATCHING_BITS 8
#define NM_MX_TRACK_ID_MATCHING_SHIFT 39
/*@}*/

/** @name Actual matching info generation */
/*@{*/
/** Regular messages matching info */
#define NM_MX_MATCH_INFO(peer, track) ( (((uint64_t)peer) << NM_MX_PEER_ID_MATCHING_SHIFT) | (((uint64_t)track) << NM_MX_TRACK_ID_MATCHING_SHIFT) )
/** Administrative message mask */
#define NM_MX_ADMIN_MATCH_MASK (UINT64_C(1) << NM_MX_ADMIN_MATCHING_SHIFT)
/** Connect matching info, with the administrative bit */
#define NM_MX_CONNECT_MATCH_INFO(track) ( UINT64_C(0xdeadbeef) | NM_MX_ADMIN_MATCH_MASK | (((uint64_t)track) << NM_MX_TRACK_ID_MATCHING_SHIFT) )
/** Accept matching info, with the administrative bit */
#define NM_MX_ACCEPT_MATCH_INFO(track) ( UINT64_C(0xbeefdead) | NM_MX_ADMIN_MATCH_MASK | (((uint64_t)track) << NM_MX_TRACK_ID_MATCHING_SHIFT) )
/*@}*/

/* MX 'NewMad_Driver' facet */

static int nm_mx_query(struct nm_drv *p_drv, struct nm_driver_query_param *params, int nparam);
static int nm_mx_init(struct nm_drv *p_drv, struct nm_trk_cap*trk_caps, int nb_trks);
static int nm_mx_close(struct nm_drv *p_drv);
static int nm_mx_connect(void*_status, struct nm_cnx_rq *p_crq);
static int nm_mx_accept(void*_status, struct nm_cnx_rq *p_crq);
static int nm_mx_disconnect(void*_status, struct nm_cnx_rq *p_crq);
static int nm_mx_post_send_iov(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_mx_post_recv_iov(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_mx_poll_iov(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_mx_poll_iov_locked(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_mx_cancel_recv_iov(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_mx_poll_any_iov(void*_status, struct nm_pkt_wrap **p_pw);
static struct nm_drv_cap*nm_mx_get_capabilities(struct nm_drv *p_drv);
static const char*nm_mx_get_driver_url(struct nm_drv *p_drv);
#ifdef PIOM_BLOCKING_CALLS
static int nm_mx_block_iov(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_mx_block_any_iov(void*_status, struct nm_pkt_wrap **p_pw);
#endif

static const struct nm_drv_iface_s nm_mx_driver =
  {
    .name               = "mx",

    .query              = &nm_mx_query,
    .init               = &nm_mx_init,
    .close              = &nm_mx_close,

    .connect		= &nm_mx_connect,
    .accept		= &nm_mx_accept,
    .disconnect         = &nm_mx_disconnect,

    .post_send_iov	= &nm_mx_post_send_iov,
    .post_recv_iov      = &nm_mx_post_recv_iov,

    .poll_send_iov      = &nm_mx_poll_iov,
    .poll_recv_iov      = &nm_mx_poll_iov,

#if MX_API >= 0x301
    .poll_send_any_iov  = &nm_mx_poll_any_iov,
    .poll_recv_any_iov  = &nm_mx_poll_any_iov,
#else
    .poll_send_any_iov  = NULL,
    .poll_recv_any_iov  = NULL,
#endif

    .cancel_recv_iov    = &nm_mx_cancel_recv_iov,

    .get_driver_url     = &nm_mx_get_driver_url,
    .get_capabilities   = &nm_mx_get_capabilities,

#ifdef PIOM_BLOCKING_CALLS
    .wait_recv_iov      = &nm_mx_block_iov,
    .wait_send_iov      = &nm_mx_block_iov,

#if MX_API >= 0x301
    .wait_recv_any_iov  = &nm_mx_block_any_iov,
    .wait_send_any_iov  = &nm_mx_block_any_iov
#else
    .wait_recv_any_iov  = NULL,
    .wait_send_any_iov  = NULL
#endif
#endif
  };

/* 'PadicoAdapter' facet for MX driver */

static void*nm_mx_instanciate(puk_instance_t, puk_context_t);
static void nm_mx_destroy(void*);

static const struct puk_adapter_driver_s nm_mx_adapter_driver =
  {
    .instanciate = &nm_mx_instanciate,
    .destroy     = &nm_mx_destroy
  };

/** Component declaration */
static int nm_mx_load(void)
{
  puk_component_declare("NewMad_Driver_mx",
			puk_component_provides("PadicoAdapter", "adapter", &nm_mx_adapter_driver),
			puk_component_provides("NewMad_Driver", "driver", &nm_mx_driver));
  return 0;
}
PADICO_MODULE_BUILTIN(NewMad_Driver_mx, &nm_mx_load, NULL, NULL);


/** Return capabilities */
static struct nm_drv_cap*nm_mx_get_capabilities(struct nm_drv *p_drv)
{
  struct nm_mx_drv* p_mx_drv = p_drv->priv;
  return &p_mx_drv->caps;
}

/** Instanciate functions */
static void*nm_mx_instanciate(puk_instance_t instance, puk_context_t context){
  struct nm_mx*status = TBX_MALLOC(sizeof (struct nm_mx));
  memset(status, 0, sizeof(struct nm_mx));
  return status;
}

static void nm_mx_destroy(void*_status){
  TBX_FREE(_status);
}

const static char*nm_mx_get_driver_url(struct nm_drv *p_drv)
{
  const struct nm_mx_drv* p_mx_drv = p_drv->priv;
  return p_mx_drv->url;
}

/*
 * Functions
 */

/** Display the MX return value */
static __tbx_inline__
void
nm_mx_check_return(const char *msg, mx_return_t return_code) 
{
  if (tbx_unlikely(return_code != MX_SUCCESS)) {
    const char *msg_mx = NULL;

    msg_mx = mx_strerror(return_code);

    DISP("%s failed with code %s = %d/0x%x", msg, msg_mx, return_code, return_code);
  }
}


#ifdef PM2_NUIOA
/** Return the preferred NUMA node of the board */
static
int 
nm_mx_get_numa_node(uint32_t board_number)
{
#if MX_API >= 0x301
  mx_return_t ret;
  uint32_t numa_node = PM2_NUIOA_ANY_NODE;
  uint32_t line_speed;

  /* get the type of link */
  ret = mx_get_info(NULL, MX_LINE_SPEED, &board_number, sizeof(board_number), &line_speed, sizeof(line_speed));
  if (ret != MX_SUCCESS)
    return PM2_NUIOA_ANY_NODE;
  
  /* if myrinet 2000, numa effect are negligible, do not return any numa indication */
  if (line_speed == MX_SPEED_2G)
    return PM2_NUIOA_ANY_NODE;
  
  ret = mx_get_info(NULL, MX_NUMA_NODE, &board_number, sizeof(board_number), &numa_node, sizeof(numa_node));
  if (ret != MX_SUCCESS)
    return PM2_NUIOA_ANY_NODE;
  else
    return numa_node == -1 ? PM2_NUIOA_ANY_NODE : (int) numa_node;
#else /* MX_API < 0x301 */
    return PM2_NUIOA_ANY_NODE;
#endif /* MX_API < 0x301 */
}
#endif /* PM2_NUIOA */

/** Maintain a usage counter for each board to distribute the workload */
static int *board_use_count = NULL;
static int total_use_count;
static uint32_t boards;

static
int
nm_mx_init_boards(void)
{
  mx_return_t	mx_ret	= MX_SUCCESS;
  int err;
  
  /* find number of boards */
  mx_ret = mx_get_info(NULL, MX_NIC_COUNT, NULL, 0, &boards, sizeof(boards));
  nm_mx_check_return("mx_get_info", mx_ret);
  
  /* init total usage counters */
  total_use_count = 0;
  
  /* allocate usage counters */
  board_use_count = TBX_CALLOC(boards, sizeof(*board_use_count));
  if (!board_use_count) {
    err = -NM_ENOMEM;
    goto out;
  }
  
  err = NM_ESUCCESS;
	
 out:
  return err;
}

/** Query MX resources */
static
int
nm_mx_query		(struct nm_drv *p_drv,
                         struct nm_driver_query_param *params,
			 int nparam) 
{
  mx_return_t	mx_ret	= MX_SUCCESS;
  uint32_t board_number;
  int i;
  int err;
  struct nm_mx_drv*p_mx_drv = NULL;
  
  /* private data                                                 */
  p_mx_drv	= TBX_MALLOC(sizeof (struct nm_mx_drv));
  if (!p_mx_drv) {
    err = -NM_ENOMEM;
    goto out;
  }

  memset(p_mx_drv, 0, sizeof (struct nm_mx_drv));
  
  /* init MX */
  mx_set_error_handler(MX_ERRORS_RETURN);
  mx_ret	= mx_init();
  if (mx_ret != MX_ALREADY_INITIALIZED)
    /* special return code only used by mx_init() */
    nm_mx_check_return("mx_init", mx_ret);
  if(mx_ret != MX_SUCCESS && mx_ret != MX_ALREADY_INITIALIZED)
    {
      fprintf(stderr, "MX: no device found.\n");
      return -NM_ENOTFOUND;
    }

  if (!board_use_count) {
    err = nm_mx_init_boards();
    if (err < 0)
      goto out;
  }

  board_number = 0xFFFFFFFF;
  for(i=0; i<nparam; i++) {
    switch (params[i].key) {
    case NM_DRIVER_QUERY_BY_INDEX:
	    board_number = params[i].value.index;
	    break;
    case NM_DRIVER_QUERY_BY_NOTHING:
	    break;
    default:
	    err = -NM_EINVAL;
	    goto out;
    }
  }

  if (board_number == 0xFFFFFFFF) {
    /* find the least used board */
    int min_count = INT_MAX;
    int min_index = -1;
    uint32_t j;
    for(j=0; j<boards; j++) {
      if (board_use_count[j] < min_count) {
        min_index = j;
	min_count = board_use_count[j];
      }
    }
    board_number = min_index;
    
  } else if (board_number >= boards) {
    /* if a was has been chosen before, check the number */
    err = -NM_EINVAL;
    goto out;
  }
  
  p_mx_drv->board_number = board_number;
  
  /* register the board here so that load_init_some may query multiple boards
   * before initializing them */
  board_use_count[p_mx_drv->board_number]++;
  total_use_count++;
  
  /* driver capabilities encoding					*/
  p_mx_drv->caps.has_trk_rq_dgram			= 1;
  p_mx_drv->caps.has_selective_receive		= 1;
  p_mx_drv->caps.has_concurrent_selective_receive	= 0;
#ifdef PIOM_BLOCKING_CALLS
  /* disabled for now because mx_wait doesn't work very well... */
  p_mx_drv->caps.is_exportable                    = 1;
#endif
#ifdef PM2_NUIOA
  p_mx_drv->caps.numa_node = nm_mx_get_numa_node(p_mx_drv->board_number);
  p_mx_drv->caps.latency = 269 ; /* from sr_ping */ 
  p_mx_drv->caps.bandwidth = 1220; /* from sr_ping, use MX_LINE_SPEED instead? */
#endif

  p_drv->priv = p_mx_drv;
  err = NM_ESUCCESS;
  
 out:
  return err;
}

static int nm_mx_init(struct nm_drv *p_drv, struct nm_trk_cap*trk_caps, int nb_trks)
{
  mx_endpoint_t		 ep;
  mx_endpoint_addr_t	 ep_addr;
  uint32_t		 ep_id;
  mx_return_t	mx_ret	= MX_SUCCESS;
  uint64_t	nic_id;
  char		hostname[MX_MAX_HOSTNAME_LEN];
  mx_param_t              *ep_params = NULL;
  uint32_t                 ep_params_count = 0;
  struct nm_mx_drv* p_mx_drv = p_drv->priv;
  
  tbx_malloc_init(&mx_pw_mem,   sizeof(struct nm_mx_pkt_wrap),
		  INITIAL_PW_NUM,   "nmad/mx/pw");
  
  mx_board_number_to_nic_id(p_mx_drv->board_number, &nic_id);
  mx_nic_id_to_hostname(nic_id, hostname);
#ifdef PIOMAN
  piom_spin_lock_init(&nm_mx_lock);
#endif /* PIOMAN */
#if 0 && MX_API >= 0x301
  /* disabled for now since it conflicts with test_any/wait_any, to be reworked */
  {
    /* Multiplex internal queues across tracks since we never match
     * among multiple tracks. This will reduce the length of the queues
     * and thus avoid long queue traversal caused by unrelated peers.
     */
    mx_param_t param;
    param.key = MX_PARAM_CONTEXT_ID;
    param.val.context_id.bits = NM_MX_TRACK_ID_MATCHING_BITS;
    param.val.context_id.shift = NM_MX_TRACK_ID_MATCHING_SHIFT;
    ep_params = &param;
    ep_params_count = 1;
  }
#endif
  
  /* mx endpoint
   */
  mx_ret = mx_open_endpoint(p_mx_drv->board_number,
			    MX_ANY_ENDPOINT,
			    NM_MX_ENDPOINT_FILTER,
			    ep_params, ep_params_count,
			    &ep);
  nm_mx_check_return("mx_open_endpoint", mx_ret);
  
  mx_ret = mx_get_endpoint_addr(ep, &ep_addr);
  nm_mx_check_return("mx_get_endpoint_addr", mx_ret);
  
  mx_ret = mx_decompose_endpoint_addr(ep_addr, &nic_id, &ep_id);
  nm_mx_check_return("mx_decompose_endpoint_addr", mx_ret);
  
  p_mx_drv->ep    = ep;
  p_mx_drv->ep_id = ep_id;
  
  /* compute URL */
  hostname[MX_MAX_HOSTNAME_LEN-1] = '\0';
  const int url_len = MX_MAX_HOSTNAME_LEN + 16;
  char url[url_len];
  snprintf(url, url_len, "%s/%d", hostname, p_mx_drv->ep_id);
  url[url_len - 1] = '\0';
  p_mx_drv->url = tbx_strdup(url);
  NM_TRACE_STR("p_drv->url", p_mx_drv->url);
  
  /* open requested tracks */
  p_mx_drv->nb_trks = nb_trks;
  p_mx_drv->trks_array = TBX_MALLOC(nb_trks * sizeof(struct nm_mx_trk));
  nm_trk_id_t trk_id;
  for(trk_id = 0; trk_id < nb_trks; trk_id++)
    {
        memset(&p_mx_drv->trks_array[trk_id], 0, sizeof (struct nm_mx_trk));
	p_mx_drv->trks_array[trk_id].next_peer_id = 1;

        /* track capabilities encoding					*/
        trk_caps[trk_id].rq_type	= nm_trk_rq_dgram;
        trk_caps[trk_id].iov_type	= nm_trk_iov_both_sym;
        trk_caps[trk_id].max_pending_send_request	= 1;
        trk_caps[trk_id].max_pending_recv_request	= 1;
        trk_caps[trk_id].max_single_request_length	= SSIZE_MAX;
        trk_caps[trk_id].max_iovec_request_length	= 32768;
#ifdef IOV_MAX
        trk_caps[trk_id].max_iovec_size		= IOV_MAX;
#else /* IOV_MAX */
        trk_caps[trk_id].max_iovec_size		= sysconf(_SC_IOV_MAX);
#endif /* IOV_MAX */
    }

  return NM_ESUCCESS;
}

/** Finalize the MX driver */
static int nm_mx_close(struct nm_drv *p_drv)
{
  struct nm_mx_drv* p_mx_drv = p_drv->priv;
  mx_return_t	mx_ret	= MX_SUCCESS;
  nm_trk_id_t trk_id;
  for(trk_id = 0; trk_id < p_mx_drv->nb_trks; trk_id++)
    {
      TBX_FREE(p_mx_drv->trks_array[trk_id].gate_map);
    }
  TBX_FREE(p_mx_drv->trks_array);

  mx_ret	= mx_close_endpoint(p_mx_drv->ep);
  nm_mx_check_return("mx_close_endpoint", mx_ret);
  
  board_use_count[p_mx_drv->board_number]--;
  total_use_count--;
  
  if (!total_use_count) {
    mx_ret	= mx_finalize();
    nm_mx_check_return("mx_finalize", mx_ret);
    
    tbx_malloc_clean(mx_pw_mem);
    TBX_FREE(board_use_count);
  }
  
  TBX_FREE(p_mx_drv);
  
  TBX_FREE(p_mx_drv->url);
  p_mx_drv->url = NULL;
  
  return NM_ESUCCESS;
}


/** Connect to a new MX peer */
static int nm_mx_connect(void*_status, struct nm_cnx_rq *p_crq)
{
  struct nm_mx	*status	= _status;
  struct nm_gate   *p_gate   = p_crq->p_gate;
  struct nm_drv    *p_drv    = p_crq->p_drv;
  struct nm_mx_drv *p_mx_drv = p_drv->priv;
  struct nm_mx_trk *p_mx_trk = &p_mx_drv->trks_array[p_crq->trk_id];
  struct nm_mx_cnx *p_mx_cnx = &status->cnx_array[p_crq->trk_id];
  char *url = strdup(p_crq->remote_drv_url);
  uint64_t		 r_nic_id	= 0;
  mx_return_t	mx_ret	= MX_SUCCESS;
  
  p_mx_trk->gate_map = TBX_REALLOC(p_mx_trk->gate_map, sizeof(nm_gate_t) * (p_mx_trk->next_peer_id + 1));
  p_mx_trk->gate_map[p_mx_trk->next_peer_id] = p_gate;
  
  NM_TRACEF("connect - drv_url: %s", url);
  char*ep_url = strchr(url, '/');
  if(!ep_url)
    {
      TBX_FAILUREF("MX: cannot parse url %s.\n", url);
    }
  *ep_url = '\0';
  ep_url++;

  mx_ret = mx_hostname_to_nic_id((char*)url, &r_nic_id);
  nm_mx_check_return("(connect) mx_hostname_to_nic_id", mx_ret);
  {
    char *ptr = NULL;
    p_mx_cnx->r_ep_id = strtol(ep_url, &ptr, 0);
  }

  NM_TRACEF("mx_connect -->");
  mx_ret = mx_connect(p_mx_drv->ep, r_nic_id, p_mx_cnx->r_ep_id,
		      NM_MX_ENDPOINT_FILTER, MX_INFINITE, &p_mx_cnx->r_ep_addr);
  NM_TRACEF("mx_connect <--");
  nm_mx_check_return("mx_connect", mx_ret);

  NM_TRACEF("send pkt1 -->");
  struct nm_mx_adm_pkt_1 pkt1;
  mx_request_t	rq;
  mx_status_t 	s;
  mx_segment_t	sg;
  uint32_t	r;
  
  strcpy(pkt1.url, p_mx_drv->url);
  pkt1.match_info	= NM_MX_MATCH_INFO(p_crq->trk_id, p_mx_trk->next_peer_id);
  p_mx_cnx->recv_match_info = pkt1.match_info;
  p_mx_trk->next_peer_id++;
  if (p_mx_trk->next_peer_id == (1 << NM_MX_PEER_ID_MATCHING_BITS) - 1)
    DISP("reached maximal number of peers %d", p_mx_trk->next_peer_id);
  
  NM_TRACEF("connect - pkt1.host_url: %s",	pkt1.host_url);
  NM_TRACEF("connect - pkt1.ep_url: %s",	pkt1.ep_url);
  NM_TRACEF("connect - pkt1.match_info (sender should contact us with this MI): %llu",	pkt1.match_info);
  
  sg.segment_ptr		= &pkt1;
  sg.segment_length	= sizeof(pkt1);
  mx_ret = mx_isend(p_mx_drv->ep, &sg, 1, p_mx_cnx->r_ep_addr,
		    NM_MX_CONNECT_MATCH_INFO(p_crq->trk_id), 0, &rq);
  nm_mx_check_return("mx_isend", mx_ret);
  mx_ret = mx_wait(p_mx_drv->ep, &rq, MX_INFINITE, &s, &r);
  nm_mx_check_return("mx_wait", mx_ret);
  NM_TRACEF("send pkt1 <--");
  
  NM_TRACEF("recv pkt2 -->");
  struct nm_mx_adm_pkt_2   pkt2;
  sg.segment_ptr		= &pkt2;
  sg.segment_length	= sizeof(pkt2);
  mx_ret = mx_irecv(p_mx_drv->ep, &sg, 1, NM_MX_ACCEPT_MATCH_INFO(p_crq->trk_id),
		    MX_MATCH_MASK_NONE, 0, &rq);
  nm_mx_check_return("mx_irecv", mx_ret);
  mx_ret = mx_wait(p_mx_drv->ep, &rq, MX_INFINITE, &s, &r);
  nm_mx_check_return("mx_wait", mx_ret);
  NM_TRACEF("recv pkt2 <--");
  
  p_mx_cnx->send_match_info	= pkt2.match_info;
  NM_TRACEF("connect - pkt2.match_info (we will contact our peer with this MI): %llu",	pkt2.match_info);
  
  NMAD_EVENT_NEW_TRK(p_gate, p_drv->id, p_crq->trk_id);
  
  free(url);

  return NM_ESUCCESS;
}

/** Accept the connection request from a MX peer */
static int nm_mx_accept(void*_status, struct nm_cnx_rq *p_crq)
{
  struct nm_mx *status = _status;
  struct nm_gate *p_gate = p_crq->p_gate;
  struct nm_drv *p_drv = p_crq->p_drv;
  struct nm_mx_drv *p_mx_drv = p_drv->priv;
  struct nm_mx_trk *p_mx_trk = &p_mx_drv->trks_array[p_crq->trk_id];
  struct nm_mx_cnx *p_mx_cnx = &status->cnx_array[p_crq->trk_id];
  uint64_t		 r_nic_id	= 0;
  mx_return_t	mx_ret	= MX_SUCCESS;

  p_mx_trk->gate_map = TBX_REALLOC(p_mx_trk->gate_map, sizeof(nm_gate_t) * (p_mx_trk->next_peer_id + 1));
  p_mx_trk->gate_map[p_mx_trk->next_peer_id] = p_gate;
  
  NM_TRACEF("recv pkt1 -->");
  struct nm_mx_adm_pkt_1   pkt1;
  mx_request_t	rq;
  mx_status_t 	s;
  mx_segment_t	sg;
  uint32_t	r;
  sg.segment_ptr		= &pkt1;
  sg.segment_length	= sizeof(pkt1);
  mx_ret = mx_irecv(p_mx_drv->ep, &sg, 1,
		    NM_MX_CONNECT_MATCH_INFO(p_crq->trk_id),
		    MX_MATCH_MASK_NONE, 0, &rq);
  nm_mx_check_return("mx_irecv", mx_ret);
  mx_ret = mx_wait(p_mx_drv->ep, &rq, MX_INFINITE, &s, &r);
  nm_mx_check_return("mx_wait", mx_ret);
  NM_TRACEF("recv pkt1 <--");
  NM_TRACEF("accept - pkt1.url: %s", pkt1.url);
  
  char*url = pkt1.url;
  char*ep_url = strchr(url, '/');
  if(!ep_url)
    {
      TBX_FAILUREF("MX: cannot parse url %s.\n", url);
    }
  *ep_url = '\0';
  ep_url++;
  {
    char *ptr = NULL;
    p_mx_cnx->r_ep_id = strtol(ep_url, &ptr, 0);
  }
  
  mx_ret = mx_hostname_to_nic_id(url, &r_nic_id);
  nm_mx_check_return("(accept) mx_hostname_to_nic_id", mx_ret);
   
  p_mx_cnx->send_match_info	= pkt1.match_info;
  NM_TRACEF("accept - pkt1.match_info (we will contact our peer with this MI): %llu",	pkt1.match_info);
  
  NM_TRACEF("mx_connect -->");
  mx_ret = mx_connect(p_mx_drv->ep, r_nic_id, p_mx_cnx->r_ep_id,
		      NM_MX_ENDPOINT_FILTER, MX_INFINITE, &p_mx_cnx->r_ep_addr);
  NM_TRACEF("mx_connect <--");
  nm_mx_check_return("mx_connect", mx_ret);
  
  NM_TRACEF("send pkt2 -->");
  struct nm_mx_adm_pkt_2   pkt2;
  pkt2.match_info	= NM_MX_MATCH_INFO(p_crq->trk_id, p_mx_trk->next_peer_id);
  p_mx_cnx->recv_match_info = pkt2.match_info;
  p_mx_trk->next_peer_id++;
  if (p_mx_trk->next_peer_id == (1 << NM_MX_PEER_ID_MATCHING_BITS) - 1)
    DISP("reached maximal number of peers %d", p_mx_trk->next_peer_id);
  
  NM_TRACEF("accept - pkt2.match_info (sender should contact us with this MI): %llu",	pkt2.match_info);
  
  sg.segment_ptr		= &pkt2;
  sg.segment_length	= sizeof(pkt2);
  mx_ret = mx_issend(p_mx_drv->ep, &sg, 1, p_mx_cnx->r_ep_addr,
		     NM_MX_ACCEPT_MATCH_INFO(p_crq->trk_id), 0, &rq);
  nm_mx_check_return("mx_issend", mx_ret);
  mx_ret = mx_wait(p_mx_drv->ep, &rq, MX_INFINITE, &s, &r);
  nm_mx_check_return("mx_wait", mx_ret);
  NM_TRACEF("send pkt2 <--");

  NMAD_EVENT_NEW_TRK(p_gate, p_drv->id, p_crq->trk_id);
    
  return NM_ESUCCESS;
}

/** Disconnect from a new MX peer */
static
int
nm_mx_disconnect	(void*_status,
			 struct nm_cnx_rq *p_crq) 
{
  int err = NM_ESUCCESS;
  
  return err;
}

/** Post a iov send request to MX */
static int nm_mx_post_send_iov(void*_status, struct nm_pkt_wrap *p_pw)
{
#ifdef PIOMAN
  piom_spin_lock(&nm_mx_lock);
#endif /* PIOMAN */
  struct nm_mx *status = _status;
  struct nm_drv *p_drv = p_pw->p_drv;
  struct nm_mx_drv *p_mx_drv = p_drv->priv;
  struct nm_mx_cnx *p_mx_cnx = &status->cnx_array[p_pw->trk_id];
  struct nm_mx_pkt_wrap	*p_mx_pw = tbx_malloc(mx_pw_mem);
  mx_return_t mx_ret = MX_SUCCESS;
  int err;
  p_pw->drv_priv = p_mx_pw;
#ifdef PROFILE
  p_mx_pw->send_bool = 1;
#endif
  p_mx_pw->p_ep	= &(p_mx_drv->ep);
  
  {
    mx_segment_t	 seg_list[p_pw->v_nb];
    struct iovec	*p_src = &p_pw->v[0];
    mx_segment_t	*p_dst = seg_list;
    int i;
    int len = 0;
    for (i = 0; i < p_pw->v_nb; i++) {
      p_dst->segment_ptr	= p_src->iov_base;
      p_dst->segment_length	= p_src->iov_len;
      len += p_dst->segment_length;
      p_src++;
      p_dst++;
    }

#ifdef DEBUG
    fprintf(stderr, "[MX] post send %d (pw=%p)\n", len, p_pw);
#endif
    
    NMAD_EVENT_SND_START(p_pw->p_gate, p_pw->p_drv->id, p_pw->trk_id, p_pw->length);
    
    mx_ret	= mx_isend(p_mx_drv->ep,
			   seg_list,
			   p_pw->v_nb,
			   p_mx_cnx->r_ep_addr,
			   p_mx_cnx->send_match_info,
			   p_pw,
			   &p_mx_pw->rq);
    nm_mx_check_return("mx_isend", mx_ret);
  }
  err = nm_mx_poll_iov_locked(_status, p_pw);
#ifdef PIOMAN
  piom_spin_unlock(&nm_mx_lock);
#endif /* PIOMAN */
  return err;
}

/** Post a iov receive request to MX */
static int nm_mx_post_recv_iov(void*_status, struct nm_pkt_wrap *p_pw)
{
#ifdef PIOMAN
  piom_spin_lock(&nm_mx_lock);
#endif /* PIOMAN */
  struct nm_mx	*status	= _status;
  struct nm_drv *p_drv = p_pw->p_drv;
  struct nm_mx_drv *p_mx_drv = p_drv->priv;
  struct nm_mx_pkt_wrap	*p_mx_pw = tbx_malloc(mx_pw_mem);;
  uint64_t		 match_mask	= 0;
  uint64_t		 match_info	= 0;
  mx_return_t		 mx_ret		= MX_SUCCESS;
  int err;
  
  p_pw->drv_priv = p_mx_pw;
  
  if (tbx_likely(p_pw->p_gate))
    {
      struct nm_mx_cnx*p_mx_cnx = &status->cnx_array[p_pw->trk_id];
      match_mask	= MX_MATCH_MASK_NONE;
      match_info	= p_mx_cnx->recv_match_info;
    } else {
      /* filter out administrative pkts */
      match_info	= 0;
      match_mask	= NM_MX_ADMIN_MATCH_MASK;
    }
#ifdef PROFILE
  p_mx_pw->send_bool = 0;
#endif
  
  p_mx_pw->p_ep		= &(p_mx_drv->ep);
  
  {
    mx_segment_t	 seg_list[p_pw->v_nb];
    struct iovec	*p_src	= &p_pw->v[0];
    mx_segment_t	*p_dst	= seg_list;
    int	 i;
    int len = 0;
    for (i = 0; i < p_pw->v_nb; i++) {
      p_dst->segment_ptr	= p_src->iov_base;
      p_dst->segment_length	= p_src->iov_len;
      len += p_dst->segment_length;
      p_src++;
      p_dst++;
    }
    
#ifdef DEBUG
    fprintf(stderr, "[MX] post recv %d (pw=%p)\n", len, p_pw);
#endif
    mx_ret	= mx_irecv(p_mx_drv->ep, seg_list, p_pw->v_nb, match_info,
			   match_mask, p_pw,&p_mx_pw->rq);
    nm_mx_check_return("mx_irecv", mx_ret);
  }
  err = nm_mx_poll_iov_locked(_status, p_pw);
#ifdef PIOMAN
  piom_spin_unlock(&nm_mx_lock);  
#endif /* PIOMAN */
  return err;
}

static int nm_mx_get_err(struct nm_pkt_wrap *p_pw,
			 mx_status_t        status,
			 mx_return_t         mx_ret)
{
  struct nm_mx_pkt_wrap	*p_mx_pw = p_pw->drv_priv;
  int err;
  
  if (tbx_unlikely(!p_pw->p_gate))
    {
      /* gate-less request */
      struct nm_mx_drv*p_mx_drv = p_pw->p_drv->priv;
      struct nm_mx_trk*p_mx_trk	= &p_mx_drv->trks_array[p_pw->trk_id];
      p_pw->p_gate = p_mx_trk->gate_map[status.match_info];
    }
#ifdef DEBUG
  if (status.code == MX_SUCCESS) {
	  fprintf(stderr, "\t[MX] Success (pw=%p)\n", p_pw);
  }
#endif
  
  if (tbx_unlikely(status.code != MX_SUCCESS)) {
    WARN("MX driver: request completed with non-successful status: %s",
	 mx_strstatus(status.code));
    switch (status.code) {
    case MX_STATUS_PENDING:
    case MX_STATUS_BUFFERED:
      err	= -NM_EINPROGRESS;
      break;
    case MX_STATUS_TIMEOUT:
      err	= -NM_ETIMEDOUT;
      break;
      
    case MX_STATUS_TRUNCATED:
      err	=  NM_ESUCCESS;
      break;
      
    case MX_STATUS_CANCELLED:
      err	= -NM_ECANCELED;
      break;
      
    case MX_STATUS_ENDPOINT_CLOSED:
      err	= -NM_ECLOSED;
      break;
      
    case MX_STATUS_ENDPOINT_UNREACHABLE:
      err	= -NM_EUNREACH;
      break;
      
    case MX_STATUS_REJECTED:
    case MX_STATUS_ENDPOINT_UNKNOWN:
    case MX_STATUS_BAD_SESSION:
    case MX_STATUS_BAD_KEY:
    case MX_STATUS_BAD_ENDPOINT:
    case MX_STATUS_BAD_RDMAWIN:
      err	= -NM_EINVAL;
      break;
      
    case MX_STATUS_ABORTED:
      err	= -NM_EABORTED;
      break;
      
    default:
      err	= -NM_EUNKNOWN;
      break;
    }
    
    goto out;
  }
  tbx_free(mx_pw_mem, p_mx_pw);
  p_pw->drv_priv = NULL;
  err = NM_ESUCCESS;
  
 out:
#ifdef PROFILE
  if (err == NM_ESUCCESS && !p_mx_pw->send_bool) {
    NMAD_EVENT_RCV_END(p_pw->p_gate, p_pw->p_drv->id, p_pw->trk_id, p_pw->length);
  }
#endif
  
  return err;
}

#ifdef PIOM_BLOCKING_CALLS
static int nm_mx_block_iov(void*_status, struct nm_pkt_wrap *p_pw)
{
  struct nm_mx_pkt_wrap	*p_mx_pw	= NULL;
  mx_return_t	mx_ret	= MX_SUCCESS;
  mx_status_t	status;
  uint32_t	result;
  p_mx_pw		= p_pw->drv_priv;
  
  mx_ret	= mx_wait(*(p_mx_pw->p_ep), &p_mx_pw->rq,MX_INFINITE, &status, &result);
  nm_mx_check_return("mx_test", mx_ret);
  
  if (tbx_unlikely(!result))
    return  -NM_EAGAIN;
  
  return nm_mx_get_err(p_pw, status, mx_ret);
}
#endif

/** Load MX operations */
static int nm_mx_poll_iov_locked(void*_status, struct nm_pkt_wrap *p_pw)
{
  struct nm_mx_pkt_wrap	*p_mx_pw = p_pw->drv_priv;;
  mx_status_t	status;
  uint32_t	result;
  assert(p_mx_pw != NULL);
  mx_return_t mx_ret = mx_test(*(p_mx_pw->p_ep), &p_mx_pw->rq, &status, &result);
  nm_mx_check_return("mx_test", mx_ret);
  if (tbx_unlikely(!result))
    return -NM_EAGAIN;
  
  return nm_mx_get_err(p_pw, status, mx_ret);
}

static int nm_mx_poll_iov(void*_status, struct nm_pkt_wrap *p_pw)
{
#ifdef PIOMAN
  piom_spin_lock(&nm_mx_lock);
  int err = nm_mx_poll_iov_locked(_status, p_pw);
  piom_spin_unlock(&nm_mx_lock);
  return err;
#else
  return nm_mx_poll_iov_locked(_status, p_pw);
#endif /* PIOMAN */
}

/** cancel a recv operation */
static int nm_mx_cancel_recv_iov(void*_status, struct nm_pkt_wrap *p_pw)
{
  int err = -NM_ENOTIMPL;
  struct nm_mx_pkt_wrap	*p_mx_pw = p_pw->drv_priv;
  uint32_t result;
  mx_return_t mx_ret = mx_cancel(*(p_mx_pw->p_ep), &p_mx_pw->rq, &result);
  if(mx_ret == MX_SUCCESS && result != 0)
    {
      tbx_free(mx_pw_mem, p_mx_pw);
      p_pw->drv_priv = NULL;
      err = NM_ESUCCESS;
    }
  else
    {
      err = -NM_EINPROGRESS;
    }
  return err;
}


#if MX_API >= 0x301
/** Poll the completion of any iov request in MX */
static int nm_mx_poll_any_iov(void*_status, struct nm_pkt_wrap **pp_pw)
{
  mx_return_t mx_ret = MX_SUCCESS;
  struct nm_pkt_wrap *p_pw;
  mx_status_t status;
  uint32_t result;
  struct nm_mx_drv* p_mx_drv = (*pp_pw)->p_drv->priv;
  /* poll any MX request, except the administrative ones */
  mx_ret = mx_test_any(p_mx_drv->ep, 0, NM_MX_ADMIN_MATCH_MASK, &status, &result);
  nm_mx_check_return("mx_test_any", mx_ret);
  
  if (tbx_unlikely(!result))
    return -NM_EAGAIN;
  
  p_pw = status.context;
  *pp_pw = p_pw;
  return nm_mx_get_err(p_pw, status, mx_ret);
}

#ifdef PIOM_BLOCKING_CALLS
/** Wait for the completion of any iov request in MX */
static int nm_mx_block_any_iov(void*_status, struct nm_pkt_wrap **pp_pw)
{
  mx_return_t mx_ret = MX_SUCCESS;
  struct nm_pkt_wrap *p_pw;
  mx_status_t status;
  uint32_t result;
  struct nm_mx_drv* p_mx_drv = (*pp_pw)->p_drv->priv;
  
  /* Wait for any MX request, except the administrative ones */
  mx_ret = mx_wait_any(p_mx_drv->ep, MX_INFINITE, 0, NM_MX_ADMIN_MATCH_MASK, &status, &result);
  nm_mx_check_return("mx_test_any", mx_ret);
  
  if (tbx_unlikely(!result))
    return -NM_EAGAIN;
  
  p_pw = status.context;
  *pp_pw = p_pw;
  return nm_mx_get_err(p_pw, status, mx_ret);
}
#endif /* PIOM_BLOCKING_CALLS */
#endif /* MX_API >= 0x301 */
