/* -*- mode: c; c-basic-offset: 8; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=8:tabstop=8:
 *
 *   Copyright (C) 2007 Myricom, Inc.
 *   Author: Myricom, Inc. <help at myri.com>
 *
 *   See COPYING in top-level directory.
 */

#include "nmad_bmi_interface.h"

struct bnm_data *bmi_nm = NULL; /* global state for bmi_nm */
nm_core_t p_core = NULL;

/* create a matching info depending on the tag and the type of message 
 * this matching info is passed to nmad as a 64 bits tag.
 */
static void
bnm_create_match(struct bnm_ctx *ctx)
{
        int             connect = 0;
        uint64_t        type    = (uint64_t) ctx->nmc_msg_type;
        uint64_t        id      = 0ULL;
        uint64_t        tag     = (uint64_t) ctx->nmc_tag;

        if (ctx->nmc_msg_type == BNM_MSG_CONN_REQ || 
            ctx->nmc_msg_type == BNM_MSG_CONN_ACK) {
                connect = 1;
        }

        if ((ctx->nmc_type == BNM_REQ_TX && connect == 0) ||
            (ctx->nmc_type == BNM_REQ_RX && connect == 1)) {

                id = (uint64_t) ctx->nmc_peer->nmp_tx_id;

        } else if ((ctx->nmc_type == BNM_REQ_TX && connect == 1) ||
                   (ctx->nmc_type == BNM_REQ_RX && connect == 0)) {
                id = (uint64_t) ctx->nmc_peer->nmp_rx_id;
        } 
        
        if ((id >> 20) != 0) {
                *(int*)0=0;
        }

        ctx->nmc_match = (type << BNM_MSG_SHIFT) | (id << BNM_ID_SHIFT) | tag;

        return;
}


static int BMI_nm_initialize (bmi_method_addr_p listen_addr, int method_id, int init_flags)
{
        bmi_nm = TBX_MALLOC(sizeof(struct bnm_data));
        p_core = &bmi_nm->core;

        nm_sr_init(p_core);

        /* if we are a server, open an endpoint now. If a client, wait until first
         * sendunexpected or first recv. */
        if (init_flags & BMI_INIT_SERVER) {

        }else{
        }
        return 0;
}

static int BMI_nm_finalize (void)
{
        return 0;
}

static int BMI_nm_set_info (int option, void *inout_parameter)
{
        return 0;
}

static int BMI_nm_get_info (int option, void *inout_parameter)
{
        return 0;
}

static void *BMI_nm_memalloc (bmi_size_t size, int type)
{
        return 0;
}

static int BMI_nm_memfree (void *buffer, bmi_size_t size, int type)
{
        return 0;
}

int BMI_nm_unexpected_free (void *buf)
{
        return 0;
}

int BMI_nm_post_send_common (bmi_op_id_t *id, struct bmi_method_addr *remote_map,
                             int numbufs, const void *const *buffers, 
                             const bmi_size_t *sizes, bmi_size_t total_size, 
                             bmi_msg_tag_t tag, void *user_ptr,
                             bmi_context_id context_id, int is_unexpected,
                             bmi_hint hints)
{
        struct bnm_ctx          *tx     = NULL;
        struct method_op        *mop    = NULL;
        struct bnm_method_addr  *nmmap  = NULL;
        struct bnm_peer         *peer   = NULL;
        int                      ret    = 0;

        nmmap = remote_map->method_data;

        peer = nmmap->nmm_peer;
        /* todo: useless ? */
        //bmx_peer_addref(peer); /* add ref and hold until test or testcontext */

        /* get idle tx, if available, otherwise alloc one */
        tx = TBX_MALLOC(sizeof(struct bnm_ctx));
        tx->nmc_state = BNM_CTX_PREP;
        tx->nmc_type = BNM_REQ_TX;

        if (numbufs > 1) {
                /* map buffers: create an iovec and fill it */
                int             i       = 0;
                struct iovec    *iov    = NULL;

                iov=TBX_MALLOC(numbufs * sizeof(struct iovec));
                if (iov == NULL) {
                        //bnm_put_idle_tx(tx);
                        //bnm_peer_decref(peer);
                        ret = -BMI_ENOMEM;
                        goto out;
                }
                tx->nmc_seg_list = iov;
                for (i = 0; i < numbufs; i++) {
                        iov[i].iov_base = (void *) buffers[i];
                        iov[i].iov_len  = sizes[i];
                        tx->nmc_nob += sizes[i];
                }
        }

        tx->nmc_nseg = numbufs;

//        if (is_unexpected && tx->mxc_nob > (long long) BMX_UNEXPECTED_SIZE) {
//                bmx_put_idle_tx(tx);
//                bmx_peer_decref(peer);
//                ret = -BMI_EINVAL;
//                goto out;
//        }

        tx->nmc_tag = tag;
        tx->nmc_peer = peer;

        if (!is_unexpected) {
                tx->nmc_msg_type = BNM_MSG_EXPECTED;
        } else {
                tx->nmc_msg_type = BNM_MSG_UNEXPECTED;
        }

        mop=TBX_MALLOC(sizeof(struct method_op));

        /* todo: get rid of this */
        //id_gen_fast_register(&mop->op_id, mop);

        /* todo: really usefull ? */
        mop->addr = remote_map;  /* set of function pointers, essentially */
        mop->method_data = tx;
        mop->user_ptr = user_ptr;
        mop->context_id = context_id;
        *id = mop->op_id;
        tx->nmc_mop = mop;

        bnm_create_match(tx);
        if (numbufs == 1) {
                nm_sr_isend(p_core, tx->nmc_peer->gate, tx->nmc_match, *buffers,*sizes, &tx->nmc_request);
        }else{
                nm_sr_isend_iov(p_core, tx->nmc_peer->gate, tx->nmc_match, tx->nmc_seg_list, tx->nmc_nseg, &tx->nmc_request);
        }
        ret = 0;
out:
        return ret;

}

static int
BMI_nm_post_send(bmi_op_id_t *id, struct bmi_method_addr *remote_map,
                 const void *buffer, bmi_size_t size,
                 enum bmi_buffer_type buffer_flag __unused,
                 bmi_msg_tag_t tag, void *user_ptr, bmi_context_id context_id,
                 bmi_hint hints)
{
        return BMI_nm_post_send_common(id, remote_map, 1, &buffer, &size, size,
                                    tag, user_ptr, context_id, 0, hints);
}

static int
BMI_nm_post_send_list(bmi_op_id_t *id, struct bmi_method_addr *remote_map,
                      const void *const *buffers, const bmi_size_t *sizes, int list_count,
                      bmi_size_t total_size, enum bmi_buffer_type buffer_flag __unused,
                      bmi_msg_tag_t tag, void *user_ptr, bmi_context_id context_id,
                      bmi_hint hints)
{
        return BMI_nm_post_send_common(id, remote_map, list_count, buffers, sizes, 
                                    total_size, tag, user_ptr, context_id, 0,
                                    hints);
}

static int
BMI_nm_post_sendunexpected(bmi_op_id_t *id, struct bmi_method_addr *remote_map,
                           const void *buffer, bmi_size_t size,
                           enum bmi_buffer_type buffer_flag __unused,
                           bmi_msg_tag_t tag, void *user_ptr, bmi_context_id context_id,
                           bmi_hint hints)
{
        return BMI_nm_post_send_common(id, remote_map, 1, &buffer, &size, size,
                                    tag, user_ptr, context_id, 1, hints);
}

static int
BMI_nm_post_sendunexpected_list(bmi_op_id_t *id, struct bmi_method_addr *remote_map,
                                const void *const *buffers, const bmi_size_t *sizes, int list_count,
                                bmi_size_t total_size, enum bmi_buffer_type buffer_flag __unused,
                                bmi_msg_tag_t tag, void *user_ptr, bmi_context_id context_id,
                                bmi_hint hints)
{
        return BMI_nm_post_send_common(id, remote_map, list_count, buffers, sizes, 
                                    total_size, tag, user_ptr, context_id, 1,
                                    hints);
}


static int
BMI_nm_post_recv_common(bmi_op_id_t *id, struct bmi_method_addr *remote_map,
                     int numbufs, void *const *buffers, const bmi_size_t *sizes,
                     bmi_size_t tot_expected_len, bmi_msg_tag_t tag,
                     void *user_ptr, bmi_context_id context_id,
                     bmi_hint hints)
{
        int                      ret    = 0;
        struct bnm_ctx          *rx     = NULL;
        struct method_op        *mop    = NULL;
        struct bnm_method_addr  *nmmap  = NULL;
        struct bnm_peer         *peer   = NULL;

        nmmap = remote_map->method_data;
        peer = nmmap->nmm_peer;
        /* todo: useless ? */
        //bnm_peer_addref(peer); /* add ref and hold until test or testcontext */

        /* get idle tx, if available, otherwise alloc one */
        rx = TBX_MALLOC(sizeof(struct bnm_ctx));
        rx->nmc_state = BNM_CTX_PREP;

        rx->nmc_tag = tag;
        rx->nmc_msg_type = BNM_MSG_EXPECTED;
        rx->nmc_peer = peer;

        if (numbufs > 1) {
                /* map buffers: create an iovec and fill it */
                int             i       = 0;
                struct iovec    *iov    = NULL;
                //nm_segment_t    *segs   = NULL;

                iov = TBX_MALLOC(numbufs * sizeof(struct iovec));

                if (iov == NULL) {
                        // bnm_put_idle_rx(rx);
                        //bnm_peer_decref(peer);
                        ret = -BMI_ENOMEM;
                        goto out;
                }
                rx->nmc_seg_list = iov;
                for (i = 0; i < numbufs; i++) {
                        iov[i].iov_base = (void *) buffers[i];
                        iov[i].iov_len = sizes[i];
                        rx->nmc_nob += sizes[i];
                }
        }
        rx->nmc_nseg = numbufs;
        
        mop = TBX_MALLOC(sizeof(*mop));
        if (mop == NULL) {
                //bnm_put_idle_rx(rx);
                //bnm_peer_decref(peer);
                ret = -BMI_ENOMEM;
                goto out;
        }
        /* todo: get rid of this */
        //id_gen_fast_register(&mop->op_id, mop);

        /* todo: useless ? */
        mop->addr = remote_map;  /* set of function pointers, essentially */
        mop->method_data = rx;
        mop->user_ptr = user_ptr;
        mop->context_id = context_id;
        *id = mop->op_id;
        rx->nmc_mop = mop;

        bnm_create_match(rx);
        if (rx->nmc_nseg == 1) {
                nm_sr_irecv(p_core, rx->nmc_peer->gate, rx->nmc_match, *buffers, *sizes, &rx->nmc_request);
        } else {
                nm_sr_irecv_iov(p_core, rx->nmc_peer->gate, rx->nmc_match, rx->nmc_seg_list, rx->nmc_nseg, &rx->nmc_request);
        }
        ret = 0;

out:
        return ret;
}

static int
BMI_nm_post_recv(bmi_op_id_t *id, struct bmi_method_addr *remote_map,
                 void *buffer, bmi_size_t expected_len, bmi_size_t *actual_len __unused,
                 enum bmi_buffer_type buffer_flag __unused, bmi_msg_tag_t tag, void *user_ptr,
                 bmi_context_id context_id,
                 bmi_hint hints)
{
        return  BMI_nm_post_recv_common(id, remote_map, 1, &buffer, &expected_len,
                                     expected_len, tag, user_ptr, context_id,
                                     hints);
}

static int
BMI_nm_post_recv_list(bmi_op_id_t *id, struct bmi_method_addr *remote_map,
                      void *const *buffers, const bmi_size_t *sizes, int list_count,
                      bmi_size_t tot_expected_len, bmi_size_t *tot_actual_len __unused,
                      enum bmi_buffer_type buffer_flag __unused, bmi_msg_tag_t tag, void *user_ptr,
                      bmi_context_id context_id,
                      bmi_hint hints)
{
        return  BMI_nm_post_recv_common(id, remote_map, list_count, buffers, sizes,
                                     tot_expected_len, tag, user_ptr, context_id,
                                     hints);

}


static int
BMI_nm_test(bmi_op_id_t id, int *outcount, bmi_error_code_t *err,
            bmi_size_t *size, void **user_ptr, int max_idle_time __unused,
            bmi_context_id context_id __unused)
{
        int         result = 0;
        struct method_op *mop   = NULL;
        struct bnm_ctx   *ctx   = NULL;
        struct bnm_peer  *peer  = NULL;

        //bmx_connection_handlers();

        mop = (struct method_op*)id;
        ctx = mop->method_data;
        peer = ctx->nmc_peer;

        switch (ctx->nmc_state) {
        case BNM_CTX_CANCELED:
                {
                /* we are racing with testcontext */
                marcel_mutex_lock(&bmi_nm->bnm_canceled_lock);
                if (ctx->nmc_state != BNM_CTX_CANCELED) {
                        marcel_mutex_unlock(&bmi_nm->bnm_canceled_lock);
                        return 0;
                }
                list_del_init(&ctx->nmc_list);
                marcel_mutex_unlock(&bmi_nm->bnm_canceled_lock);
                *outcount = 1;
                *err = ctx->nmc_nmstat;
                if (ctx->nmc_mop) {
                        if (user_ptr) {
                                *user_ptr = ctx->nmc_mop->user_ptr;
                        }
                        /* todo: get rid of this */
                        //id_gen_fast_unregister(ctx->nmc_mop->op_id);
                        TBX_FREE(ctx->nmc_mop);
                }
                //bnm_peer_decref(peer);
                //if (ctx->nmc_type == BNM_REQ_TX) {
                //        bnm_put_idle_tx(ctx);
                //} else {
                //        bnm_put_idle_rx(ctx);
                //}
                break;
                }
        case BNM_CTX_PENDING:
                {
                /* racing with nm_test_any() in textcontext? */
                if(ctx->nmc_type == BNM_REQ_TX) {
                        result = nm_sr_stest(p_core, &ctx->nmc_request);
                } else {
                        result = nm_sr_rtest(p_core, &ctx->nmc_request);
                }

                if (result == NM_ESUCCESS) {
                     
                        *outcount = 1;
                        ctx->nmc_nmstat = NM_ESUCCESS;
                        *err = 0;
                        size_t tmp_size;
                        nm_sr_get_size(p_core, &ctx->nmc_request, &tmp_size);
                        *size=tmp_size;
                        
                        if (ctx->nmc_mop) {
                                if (user_ptr) {
                                        *user_ptr = ctx->nmc_mop->user_ptr;
                                }
                                
                                TBX_FREE(ctx->nmc_mop);
                        }
                        //bnm_deq_pending_ctx(ctx);
                        //if (ctx->nmc_type == BNM_REQ_TX) {
                        //        bnm_put_idle_tx(ctx);
                        //} else {
                        //        bnm_put_idle_rx(ctx);
                        //}
                        //bnm_peer_decref(peer);
                }
                break;
                }
        default:
                {
                }
        }

        return 0;
}

/* BMI_nm_testcontext()
 * 
 * Checks to see if any messages from the specified context have completed.
 *
 * returns 0 on success, -errno on failure
 */
int BMI_nm_testcontext (int incount, bmi_op_id_t *outids, int *outcount,
                        bmi_error_code_t *errs, bmi_size_t *sizes, void **user_ptrs,
                        int max_idle_time, bmi_context_id context_id)
{
#if 0
        int             i               = 0;
        int             completed       = 0;
        int             old             = 0;
        uint64_t        match           = 0ULL;
        uint64_t        mask            = BNM_MASK_MSG;
        struct bnm_ctx  *ctx            = NULL;
        struct bnm_peer *peer           = NULL;
        list_t          *canceled       = &bmi_nm->bnm_canceled;
        int             wait            = 0;
        static int      count           = 0;
        int             print           = 0;

        //bmx_connection_handlers();

        /* always return canceled messages first */
        while (completed < incount && !list_empty(canceled)) {
                marcel_mutex_lock(&bmi_nm->bnm_canceled_lock);
                ctx = list_entry(canceled->next, struct bnm_ctx, nmc_list);
                list_del_init(&ctx->nmc_list);
                /* change state in case test is trying to reap it as well */
                ctx->nmc_state = BNM_CTX_COMPLETED;
                marcel_mutex_unlock(&bmi_nm->bnm_canceled_lock);

                peer = ctx->nmc_peer;
                outids[completed] = ctx->nmc_mop->op_id;
                errs[completed] = ctx->nmc_nmstat.code;
                if (user_ptrs)
                        user_ptrs[completed] = ctx->nmc_mop->user_ptr;

                TBX_FREE(ctx->nmc_mop);
                completed++;
        }

        /* return completed messages
         * we will always try (incount - completed) times even
         *     if some iterations have no result */

        match = (uint64_t) BNM_MSG_EXPECTED << BNM_MSG_SHIFT;
        for (i = completed; i < incount; i++) {
                uint32_t        result  = 0;
                nm_status_t     status;

                old = completed;

                if (wait == 0 || wait == 2) {
                        nm_test_any(bmi_nm->bnm_ep, match, mask, &status, &result);
                        if (!result && wait == 0 && max_idle_time > 0) wait = 1;
                } else { /* wait == 1 */
                        nm_wait_any(bmi_nm->bnm_ep, max_idle_time, match, mask, 
                                    &status, &result);
                        wait = 2;
                }
                if (result) {
                        ctx = (struct bnm_ctx *) status.context;
                        bnm_deq_pending_ctx(ctx);
                        peer = ctx->nmc_peer;

                        outids[completed] = ctx->nmc_mop->op_id;
                        if (status.code == NM_SUCCESS) {
                                errs[completed] = 0;
                                sizes[completed] = status.xfer_length;
                        } else {
                                errs[completed] = bnm_nm_to_bmi_errno(status.code);
                        }
                        if (user_ptrs)
                                user_ptrs[completed] = ctx->nmc_mop->user_ptr;

                        /* todo: get rid of this */
                        //id_gen_fast_unregister(ctx->nmc_mop->op_id);
                        TBX_FREE(ctx->nmc_mop);
                        completed++;

                }
        }

        /* check for completed unexpected sends */

        match = (uint64_t) BNM_MSG_UNEXPECTED << BNM_MSG_SHIFT;

        old = completed;

        for (i = completed; i < incount; i++) {
                uint32_t        result          = 0;
                nm_status_t     status;
                list_t          *unex_txs       = &bmi_nm->bnm_unex_txs;
                int             again           = 1;

                ctx = NULL;

                marcel_mutex_lock(&bmi_nm->bnm_unex_txs_lock);
                if (!list_empty(unex_txs)) {
                        ctx = list_entry(unex_txs->next, struct bnm_ctx, nmc_list);
                        peer = ctx->nmc_peer;
                        list_del_init(&ctx->nmc_list);
                        result = 1;
                }
                marcel_mutex_unlock(&bmi_nm->bnm_unex_txs_lock);

                while (!ctx && again) {
                        again = 0;
                        nm_test_any(bmi_nm->bnm_ep, match, mask, &status, &result);
                        if (result) {
                                ctx = (struct bnm_ctx *) status.context;
                                bnm_deq_pending_ctx(ctx);
                                peer = ctx->nmc_peer;
                                if (ctx->nmc_type == BNM_REQ_RX) {
                                        /* queue until testunexpected is called */
                                        bnm_q_unex_ctx(ctx);
                                        result = 0;
                                        again = 1;
                                        ctx = NULL;
                                }
                        }
                }
                if (result) {

                        outids[completed] = ctx->nmc_mop->op_id;
                        if (status.code == NM_SUCCESS) {
                                errs[completed] = 0;
                                sizes[completed] = status.xfer_length;
                        } else {
                                errs[completed] = bnm_nm_to_bmi_errno(status.code);
                                bnm_peer_disconnect(peer, 0, BMI_ENETRESET);
                        }
                        if (user_ptrs)
                                user_ptrs[completed] = ctx->nmc_mop->user_ptr;

                        /* todo: get rid of this */
                        //id_gen_fast_unregister(ctx->nmc_mop->op_id);
                        TBX_FREE(ctx->nmc_mop);
                        completed++;
                        
                }
        }

        *outcount = completed;
        return completed;
#else
        return 0;
#endif
}

int BMI_nm_testunexpected (int incount __unused, int *outcount,
                           struct bmi_method_unexpected_info *ui, int max_idle_time __unused)
{
        return 0;
}

bmi_method_addr_p BMI_nm_method_addr_lookup (const char *id)
{
        return 0;
}

int BMI_nm_open_context (bmi_context_id context_id __unused)
{
        return 0;
}

void BMI_nm_close_context (bmi_context_id context_id __unused)
{
        return;
}

int BMI_nm_cancel (bmi_op_id_t id, bmi_context_id context_id __unused)
{
        return 0;
}

const char* BMI_nm_rev_lookup (struct bmi_method_addr *meth)
{
        return NULL;
}
