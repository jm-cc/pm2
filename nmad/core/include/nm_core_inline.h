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

#ifndef NM_CORE_INLINE_H
#define NM_CORE_INLINE_H

#include "nm_pkt_wrap.h"
#include "nm_core.h"
#include "nm_sched.h"
#include "nm_log.h"

extern p_tbx_memory_t nm_core_pw_mem	;
extern p_tbx_memory_t nm_core_iov1_mem	;
extern p_tbx_memory_t nm_core_iov2_mem	;


/** Allocate a pkt wrapper struct.
 */
static
__inline__
int
nm_pkt_wrap_alloc(struct nm_core	 *p_core TBX_UNUSED,
                  struct nm_pkt_wrap	**pp_pkt_wrap,
                  uint8_t		  proto_id,
                  uint8_t		  seq) {
        struct nm_pkt_wrap	*p_pw	= NULL;
        int			 err;

        p_pw	= tbx_malloc(nm_core_pw_mem);
        if (!p_pw) {
                err	= -NM_ENOMEM;
                goto out;
        }

        memset(p_pw, 0, sizeof(struct nm_pkt_wrap));
        p_pw->proto_id	= proto_id;
        p_pw->seq	= seq;

        *pp_pkt_wrap	= p_pw;

        err = NM_ESUCCESS;

 out:
        return err;
}

/** Free a pkt wrapper struct.
 */
static
__inline__
int
nm_pkt_wrap_free(struct nm_core		*p_core TBX_UNUSED,
                 struct nm_pkt_wrap	*p_pw) {
        int	err;

        tbx_free(nm_core_pw_mem, p_pw);
        err = NM_ESUCCESS;

        return err;
}

/** Allocate a custom-sized iovec. */
static
__inline__
int
nm_iov_alloc(struct nm_core	 	*p_core TBX_UNUSED,
               struct nm_pkt_wrap	*p_pw,
               uint32_t			 v_size) {
        int	err;

        assert(!p_pw->v);
        assert(v_size);

        switch (v_size) {
        case 1:
                p_pw->v	= tbx_malloc(nm_core_iov1_mem);
                break;
        case 2:
                p_pw->v	= tbx_malloc(nm_core_iov2_mem);
                break;
        default:
                p_pw->v	= TBX_MALLOC(v_size*sizeof(struct iovec));
        }

        if (!p_pw->v) {
                err	= -NM_ENOMEM;
                goto out;
        }

        memset(p_pw->v, 0, v_size*sizeof(struct iovec));

        p_pw->v_size	= v_size;
        p_pw->v_first	= 0;
        p_pw->v_nb	= 0;

        err = NM_ESUCCESS;

 out:
        return	err;
}

/** Free a custom sized iovec.
 */
static
__inline__
int
nm_iov_free(struct nm_core	*p_core TBX_UNUSED,
            struct nm_pkt_wrap	*p_pw) {
        int	err;

        switch (p_pw->v_size) {
        case 1:
                tbx_free(nm_core_iov1_mem, p_pw->v);
                break;
        case 2:
                tbx_free(nm_core_iov2_mem, p_pw->v);
                break;
        default:
                TBX_FREE(p_pw->v);
                break;
        }

        p_pw->v		= NULL;
        p_pw->v_size	=    0;
        p_pw->v_first	=    0;
        p_pw->v_nb	=    0;

        err = NM_ESUCCESS;

        return	err;
}

/** Allocate iovec-corresponding entries for storing meta-data.
 */
static
__inline__
int
nm_iov_meta_alloc(struct nm_core	*p_core TBX_UNUSED,
                  struct nm_pkt_wrap	*p_pw) {
        int	err;

        assert(p_pw->v);
        assert(!p_pw->nm_v);

        p_pw->nm_v	=
                TBX_CALLOC(p_pw->v_size, sizeof(struct nm_iovec));

        if (!p_pw->nm_v) {
                err	= -NM_ENOMEM;
                goto out;
        }

        err = NM_ESUCCESS;

 out:
        return	err;
}

/** Free iovec meta-data.
 */
static
__inline__
int
nm_iov_meta_free(struct nm_core		*p_core TBX_UNUSED,
                 struct nm_pkt_wrap	*p_pw) {
        int	err;

        TBX_FREE(p_pw->nm_v);

        p_pw->nm_v	= NULL;

        err = NM_ESUCCESS;

        return	err;
}

/** Append a buffer to the iovec.
 */
static
__inline__
int
nm_iov_append_buf(struct nm_core	*p_core TBX_UNUSED,
                  struct nm_pkt_wrap	*p_pw,
                  void			*ptr,
                  uint64_t		 len) {
        uint32_t	o;
        int	err;

        o = p_pw->v_first+p_pw->v_nb;

        assert(p_pw->v_size);
        assert(p_pw->v);
        assert(o < p_pw->v_size);

        p_pw->v[o].iov_base	= ptr;
        p_pw->v[o].iov_len	= len;

        p_pw->v_nb++;
        p_pw->length += len;

        err = NM_ESUCCESS;

        return	err;
}

/** Prepend a buffer to the iovec.
 */
static
__inline__
int
nm_iov_prepend_buf(struct nm_core	*p_core TBX_UNUSED,
                  struct nm_pkt_wrap	*p_pw,
                  void			*ptr,
                  uint64_t		 len) {
        uint32_t	o;
        int	err;

        assert(p_pw->v_size);
        assert(p_pw->v);
        assert(p_pw->v_first);

        p_pw->v_first--;
        o = p_pw->v_first;

        p_pw->v[o].iov_base	= ptr;
        p_pw->v[o].iov_len	= len;

        p_pw->v_nb++;
        p_pw->length += len;

        err = NM_ESUCCESS;

        return	err;
}

/** Append an already existing iovec to another iovec.
 */
static
__inline__
int
nm_iov_append_iov(struct nm_core	*p_core TBX_UNUSED,
                  struct nm_pkt_wrap	*p_pw,
                  struct iovec		*iov,
                  struct nm_iovec	*nm_iov,
                  uint32_t		 iov_size,
                  uint64_t		 iov_len) {
        uint32_t	o;
        int	err;

        o = p_pw->v_first+p_pw->v_nb;

        assert(p_pw->v_size);
        assert(p_pw->v);
        assert(o + iov_size <= p_pw->v_size);

        memcpy(p_pw->v + o, iov, iov_size*sizeof(struct iovec));

        if (nm_iov) {
                assert(p_pw->nm_v);
                memcpy(p_pw->nm_v + o, nm_iov,
                       iov_size*sizeof(struct nm_iovec));
        }

        p_pw->length	+= iov_len;
        p_pw->v_nb	+= iov_size;

        err = NM_ESUCCESS;

        return	err;
}

/** Prepend an already existing iovec to another iovec.
 */
static
__inline__
int
nm_iov_prepend_iov(struct nm_core	*p_core TBX_UNUSED,
                  struct nm_pkt_wrap	*p_pw,
                  void			*iov,
                  struct nm_iovec	*nm_iov,
                  uint32_t		 iov_size,
                  uint64_t		 iov_len) {
        uint32_t	o;
        int	err;

        assert(p_pw->v_size);
        assert(p_pw->v);
        assert(iov_size <= p_pw->v_first);

        p_pw->v_first -= iov_size;
        o = p_pw->v_first;

        memcpy(p_pw->v + o, iov, iov_size*sizeof(struct iovec));

        if (nm_iov) {
                assert(p_pw->nm_v);
                memcpy(p_pw->nm_v + o, nm_iov,
                       iov_size*sizeof(struct nm_iovec));
        }

        p_pw->length	+= iov_len;
        p_pw->v_nb	+= iov_size;

        err = NM_ESUCCESS;

        return	err;
}

/** Prepare a pre-filled pkt_wrap struct.
   - gate_id == -1 means 'any gate'
 */
static
__inline__
int
__nm_core_wrap_buffer	(struct nm_core		 *p_core,
                         nm_gate_id_t  		  gate_id,
                         uint8_t		  proto_id,
                         uint8_t		  seq,
                         void			 *buf,
                         uint64_t		  len,
                         struct nm_pkt_wrap	**pp_pw) {
        struct nm_pkt_wrap	*p_pw	= NULL;
        struct nm_gate		*p_gate	= NULL;
        int err;

        /* check gate_id range
           -1 is for 'any gate'
         */
        if (gate_id < NM_ANY_GATE || gate_id >= p_core->nb_gates) {
                err	= -NM_EINVAL;
                goto out;
        }

        if (gate_id >= 0) {
                p_gate	= p_core->gate_array + gate_id;
                if (!p_gate) {
                        err	= -NM_EINVAL;
                        goto out;
                }
        }

        /* allocate packet wrapper
         */
        err	= nm_pkt_wrap_alloc(p_core, &p_pw, proto_id, seq);
        if (err != NM_ESUCCESS)
                goto out;

        /* allocate iov
         */
        err	= nm_iov_alloc(p_core, p_pw, 1);
        if (err != NM_ESUCCESS) {
                nm_pkt_wrap_free(p_core, p_pw);
                goto out;
        }

        /* append buffer to iovec
         */
        err	= nm_iov_append_buf(p_core, p_pw, buf, len);
        if (err != NM_ESUCCESS) {
                NM_DISPF("nm_iov_append_buf returned %d", err);
        }

        /* protocol
         */
        if (proto_id) {
                if (proto_id > 254) {
                        err = -NM_EINVAL;
                        goto out;
                }

                p_pw->p_proto	= p_core->p_proto_array[proto_id];
        } else {
                p_pw->p_proto	= NULL;
        }

        /* initialize other fields
         */
        p_pw->p_gate	= p_gate;

        /* done
         */
        *pp_pw	= p_pw;
        err = NM_ESUCCESS;

 out:
        return err;
}

/** Post a pkt wrapper to the pre-sched output list.
 */
static
__inline__
int
__nm_core_post_send	(struct nm_core		*p_core TBX_UNUSED,
                         struct nm_pkt_wrap	*p_pw) {
        int err;

        if (!p_pw->p_gate) {
                err	= -NM_EINVAL;
                goto out;
        }

        tbx_slist_append(p_pw->p_gate->pre_sched_out_list, p_pw);
        err = NM_ESUCCESS;

 out:
        return err;
}

/** Post a pkt wrapper to the pre-sched input list.
 */
static
__inline__
int
__nm_core_post_recv	(struct nm_core		*p_core,
                         struct nm_pkt_wrap	*p_pw) {
        int err;

        if (p_pw->p_gate) {
                tbx_slist_append(p_core->p_sched->submit_aux_recv_req, p_pw);
        } else {
                tbx_slist_append(p_core->p_sched->submit_perm_recv_req, p_pw);
        }

        err = NM_ESUCCESS;

        return err;
}

#endif /* NM_CORE_INLINE_H */
