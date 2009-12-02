/* -*- mode: c; c-basic-offset: 8; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=8:tabstop=8:
 *
 * This file has been imported from PVFS2 implementation of BMI (MX driver)
 *
 *   Copyright (C) 2007 Myricom, Inc.
 *   Author: Myricom, Inc. <help at myri.com>
 *
 *   See COPYING in top-level directory.
 *
 * NewMadeleine
 * Copyright (C) 2009 (see AUTHORS file)
 */

#ifndef NMAD_BMI_INTERFACE_H
#define NMAD_BMI_INTERFACE_H

#include <nm_public.h>
/* BMI is only available for 'huge tags' (ie. at least 64 bits) */
#ifdef NM_TAGS_AS_INDIRECT_HASH

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <nm_private.h>
#include <nm_sendrecv_interface.h>
#include "bmi-types.h"
#include "bmi-method-support.h"

#ifdef __GNUC__
/* confuses debugger */
/* #  define __hidden __attribute__((visibility("hidden"))) */
#  define __hidden
#  define __unused __attribute__((unused))
#else
#  define __hidden
#  define __unused
#endif

typedef struct tbx_fast_list_head list_t;       /* easier to type */

#define BNM_MAGIC            0x70766673 /* "pvfs" - for MX filter */
#define BNM_VERSION          0x100      /* version number */

/* Allocate 20 RX structs per peer */
#define BNM_PEER_RX_NUM      (20)
/* On servers only, these RX structs will also have a 8 KB buffer to
 * receive unexpected messages */
#define BNM_UNEXPECTED_SIZE  (8 << 10)
#define BNM_UNEXPECTED_NUM   2  /* buffers allocated per rx */

#define BNM_MEM_ACCT  0         /* track number of bytes alloc's and freed */
#define BNM_LOGGING   0         /* use MPE logging routines */


/* The server will get unexpected messages from new clients.
 * Pre-allocate some rxs to catch these. The worst case is when
 * every client tries to connect at once. In this case, set this
 * value to the number of clients in PVFS. */
#define BNM_SERVER_RXS       (100)

/* BNM [UN]EXPECTED msgs use the 64-bits of the match info as follows:
 *    Bits      Description
 *    60-63     Msg type
 *    52-59     Reserved
 *    32-51     Peer id (of the sender, assigned by receiver)
 *     0-31     bmi_msg_tag_t
 */

/* BNM CONN_[REQ|ACK] msgs use the 64-bits of the match info as follows:
 *    Bits      Description
 *    60-63     Msg type
 *    52-59     Reserved
 *    32-51     Peer id (to use when contacting the sender)
 *     0-31     Version
 */
#define BNM_NBITS_ID    28
#define BNM_NBITS_TAG   32
#define BNM_NBITS_MSG   4


//#define BNM_MSG_SHIFT   60
//#define BNM_ID_SHIFT    32

#define BNM_ID_SHIFT    (BNM_NBITS_TAG)
#define BNM_TAG_SHIFT   (0)
#define BNM_MSG_SHIFT   (BNM_NBITS_TAG+BNM_NBITS_ID)

#define BNM_MAX_ID      ((UINT64_C(1)<<BNM_NBITS_ID)   -1)
#define BNM_MAX_TAG     ((UINT64_C(1)<<BNM_NBITS_TAG)  -1)
#define BNM_MAX_MSG     ((UINT64_C(1)<<BNM_NBITS_MSG)  -1)

#define BNM_ID_MASK     (BNM_MAX_ID <<BNM_ID_SHIFT)
#define BNM_TAG_MASK    (BNM_MAX_TAG<<BNM_TAG_SHIFT)
#define BNM_MSG_MASK    (BNM_MAX_MSG<<BNM_MSG_SHIFT)

#define BNM_MATCH_GET_MSG_TYPE(_match)                                  \
        ((enum bnm_msg_type) (((_match) & BNM_MSG_MASK) >> BNM_MSG_SHIFT))
#define BNM_MATCH_GET_ID(_match)                                \
        ((uint32_t) (((_match) & BNM_ID_MASK) >> BNM_ID_SHIFT))
#define BNM_MATCH_GET_TAG(_match)                                  \
        ((uint32_t) (((_match) & BNM_TAG_MASK) >> BNM_TAG_SHIFT))

#define BNM_TIMEOUT     (20 * 1000)       /* msg timeout in milliseconds */

#ifdef BNM_MEM_TWEAK
struct bnm_buffer
{
        list_t              nmb_list;     /* hang on used/idle lists */
        int                 nmb_used;     /* count how many times used */
        void               *nmb_buffer;   /* the actual buffer */
};
#endif

/* Global interface state */
struct bnm_data
{
        int                 bnm_method_id;      /* BMI id assigned to us */
        struct nm_core      core;             /* Interface used for this connexion */

        char               *bnm_peername;       /* my mx://hostname/board/ep_id  */
        char               *bnm_hostname;       /* my hostname */
        uint32_t            bnm_board;          /* my MX board index */
        uint32_t            bnm_ep_id;          /* my MX endpoint ID */
        //mx_endpoint_t       bnm_ep;             /* my MX endpoint */
        uint32_t            bnm_sid;            /* my MX session id */
        int                 bnm_is_server;      /* am I a server? */

        list_t              bnm_peers;          /* list of all peers */

        list_t              bnm_txs;            /* list of all txs for cleanup */
        list_t              bnm_idle_txs;       /* available for sending */

        list_t              bnm_rxs;            /* list of all rxs for cleanup */
        list_t              bnm_idle_rxs;       /* available for receiving */

        list_t              bnm_canceled;       /* canceled reqs waiting for test */

        list_t              bnm_unex_txs;       /* completed unexpected sends */
        list_t              bnm_unex_rxs;       /* completed unexpected recvs */

        uint32_t            bnm_next_id;        /* for the next peer_id */
#ifdef MARCEL
        marcel_mutex_t      bnm_peers_lock;     /* peers list lock */
        marcel_mutex_t      bnm_idle_txs_lock;  /* idle_txs lock */
        marcel_mutex_t      bnm_idle_rxs_lock;  /* idle_rxs lock */
        marcel_mutex_t      bnm_canceled_lock;  /* canceled list lock */
        marcel_mutex_t      bnm_unex_txs_lock;  /* completed unexpected sends lock */
        marcel_mutex_t      bnm_unex_rxs_lock;  /* completed unexpected recvs lock */

        marcel_mutex_t      bnm_lock;           /* global lock - use for global rxs,
                                                   global txs, next_id, etc. */
#endif

#ifdef BNM_MEM_TWEAK
        list_t              bnm_idle_buffers;
        list_t              bnm_used_buffers;
        int                 bnm_misses;

        list_t              bnm_idle_unex_buffers;
        list_t              bnm_used_unex_buffers;
        int                 bnm_unex_misses;

#ifdef MARCEL
        marcel_mutex_t         bnm_idle_buffers_lock;
        marcel_mutex_t         bnm_used_buffers_lock;
        marcel_mutex_t         bnm_idle_unex_buffers_lock;
        marcel_mutex_t         bnm_used_unex_buffers_lock;
#endif

#endif
};

enum bnm_peer_state {
        BNM_PEER_DISCONNECT     = -1,       /* disconnecting, failed, etc. */
        BNM_PEER_INIT           =  0,       /* ready for connect */
        BNM_PEER_WAIT           =  1,       /* trying to connect */
        BNM_PEER_READY          =  2,       /* connected */
};

struct bnm_method_addr
{
        struct bmi_method_addr  *nmm_map;        /* peer's bmi_method_addrt */
        const char              *nmm_peername;   /* mx://hostname/board/ep_id  */
        const char              *nmm_hostname;   /* peer's hostname */
        uint32_t                 nmm_board;      /* peer's MX board index */
        uint32_t                 nmm_ep_id;      /* peer's MX endpoint ID */
        struct bnm_peer         *nmm_peer;       /* peer pointer */
};

struct bnm_peer
{
        /* todo: useless ? */
        struct bmi_method_addr *nmp_map;        /* his bmi_method_addr * */
        struct bmx_method_addr *nmp_mxmap;      /* his bmx_method_addr */
        //ajout
        //        const char              *nmm_peername;   /* mx://hostname/board/ep_id  */
        struct BMI_addr        *p_addr;
        const char              *peername;  
        nm_gate_t               p_gate;

        int                     nmp_exist;      /* have we connected before? */

        enum bnm_peer_state     nmp_state;      /* INIT, WAIT, READY, DISCONNECT */
        uint32_t                nmp_tx_id;      /* id assigned to me by peer */
        uint32_t                nmp_rx_id;      /* id I assigned to peer */

        /* todo: useless ? */
        list_t                  nmp_queued_txs; /* waiting on READY */
        list_t                  nmp_queued_rxs; /* waiting on INIT (if in DISCONNECT) */
        list_t                  nmp_pending_rxs; /* in-flight rxs (in case of cancel) */
        int                     nmp_refcount;   /* queued and pending txs and pending rxs */

        list_t                  nmp_list;       /* hang this on bmx_peers */
#ifdef MARCEL
        marcel_mutex_t          nmp_lock;       /* peer lock */
#endif
};

enum bnm_req_type {
        BNM_REQ_TX      = 1,
        BNM_REQ_RX      = 2,
};

enum bnm_ctx_state {
        BNM_CTX_INIT            = 0,
        BNM_CTX_IDLE            = 1,    /* state when on tx/rx idle list */
        BNM_CTX_PREP            = 2,    /* not really useful */
        BNM_CTX_QUEUED          = 3,    /* queued_[txs|rxs] */
        BNM_CTX_PENDING         = 4,    /* posted */
        BNM_CTX_COMPLETED       = 5,    /* waiting for user test */
        BNM_CTX_CANCELED        = 6,    /* canceled and waiting user test */
};

enum bnm_msg_type {
        BNM_MSG_ICON_REQ        = 0xb,      /* iconnect() before CONN_REQ */
        BNM_MSG_CONN_REQ        = 0xc,      /* bmx connect request message */
        BNM_MSG_ICON_ACK        = 0x9,      /* iconnect() before CONN_ACK */
        BNM_MSG_CONN_ACK        = 0xa,      /* bmx connect ack msg */
        BNM_MSG_UNEXPECTED      = 0xd,      /* unexpected msg */
        BNM_MSG_EXPECTED        = 0xe,      /* expected message */
};

struct bnm_ctx
{
        enum bnm_req_type   nmc_type;       /* TX or RX */
        enum bnm_ctx_state  nmc_state;      /* INIT, IDLE, PREP, PENDING, ... */
        enum bnm_msg_type   nmc_msg_type;   /* UNEXPECTED, EXPECTED, ... */

        nm_sr_request_t     nmc_request;    /* NMad request */
        nm_sr_status_t      nmc_nmstat;     /* NM status */

        list_t              nmc_global_list; /* hang on global list for cleanup */
        list_t              nmc_list;       /* hang on idle, queued and pending lists */

        struct method_op   *nmc_mop;        /* method op */
        struct bnm_peer    *nmc_peer;       /* owning peer */
        bmi_msg_tag_t       nmc_tag;        /* BMI tag (nm_tag_t) */
        uint64_t            nmc_match;      /* match info */

        void               *nmc_seg;        /* local MX segment */
        void               *nmc_buffer;     /* local buffer for server unexpected rxs
                                               and client CONN_REQ only */
        struct iovec       *nmc_seg_list;   /* NM  array for _list funcs */
        int                 nmc_nseg;       /* number of segments */
        bmi_size_t          nmc_nob;        /* number of bytes (int64_t) */


        uint64_t            nmc_get;        /* # of times returned from idle list */
        uint64_t            nmc_put;        /* # of times returned to idle list */
};

struct bnm_connreq
{
        char                nmm_peername[256]; /* my peername nm://hostname/... */
} __attribute__ ((__packed__)) ;

/* BMI is only available for 'huge tags' (ie. at least 64 bits) */
#endif /* NM_TAGS_AS_INDIRECT_HASH */

#endif  /* NMAD_BMI_INTERFACE_H */
