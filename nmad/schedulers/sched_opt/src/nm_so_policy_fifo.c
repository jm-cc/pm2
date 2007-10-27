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

#ifdef NMAD_QOS

#include <tbx.h>
#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include "nm_so_private.h"
#include "nm_so_strategies/nm_so_strat_qos/nm_so_policy_fifo.h"
#include "nm_so_pkt_wrap.h"
#include "nm_so_tracks.h"


struct nm_so_policy_fifo_gate{
  struct list_head out_list;
  unsigned nb_packets;
};

static int
pack_ctrl(void *private,
	  union nm_so_generic_ctrl_header *p_ctrl)
{
  struct list_head *out_list =
    &((struct nm_so_policy_fifo_gate *)private)->out_list;
  struct nm_so_pkt_wrap *p_so_pw = NULL;
  int err;

  /* We first try to find an existing packet to form an aggregate */
  list_for_each_entry(p_so_pw, out_list, link)
    {
      if(p_so_pw->pw.length <= 32 - NM_SO_CTRL_HEADER_SIZE)
	{
	  err = nm_so_pw_add_control(p_so_pw, p_ctrl);
	  goto out;
	}
    }

  /* Simply form a new packet wrapper */
  err = nm_so_pw_alloc_and_fill_with_control(p_ctrl,
					     &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  /* Add the control packet to the out_list */
  list_add_tail(&p_so_pw->link, out_list);
  ((struct nm_so_policy_fifo_gate *)private)->nb_packets++;

 out:
  return err;
}

static int
pack(struct nm_gate *p_gate, void *private,
     uint8_t tag, uint8_t seq,
     void *data, uint32_t len)
{
  struct list_head *out_list =
    &((struct nm_so_policy_fifo_gate *)private)->out_list;
  struct nm_so_pkt_wrap *p_so_pw;
  struct nm_so_gate *p_so_gate = (struct nm_so_gate *)p_gate->sch_private;
  int flags = 0;
  int err;

  if(len <= NM_SO_MAX_SMALL) {
    /* Small packet */

    /* We first try to find an existing packet to form an aggregate */
    list_for_each_entry(p_so_pw, out_list, link) {
      uint32_t h_rlen = nm_so_pw_remaining_header_area(p_so_pw);
      uint32_t d_rlen = nm_so_pw_remaining_data(p_so_pw);
      uint32_t size = NM_SO_DATA_HEADER_SIZE + nm_so_aligned(len);

      if(size > d_rlen || NM_SO_DATA_HEADER_SIZE > h_rlen)
	/* There's not enough room to add our data to this paquet */
	goto next;

      if(len <= NM_SO_COPY_ON_SEND_THRESHOLD && size <= h_rlen)
	/* We can copy data into the header zone */
	flags = NM_SO_DATA_USE_COPY;
      else
	if(p_so_pw->pw.v_nb == NM_SO_PREALLOC_IOV_LEN)
	  goto next;

      err = nm_so_pw_add_data(p_so_pw, tag + 128, seq, data, len, 0, 1, flags);
      goto out;

    next:
      ;
    }

    if(len <= NM_SO_COPY_ON_SEND_THRESHOLD)
      flags = NM_SO_DATA_USE_COPY;

    /* We didn't have a chance to form an aggregate, so simply form a
       new packet wrapper and add it to the out_list */
    err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq,
					    data, len,
					    0, 1,
					    flags,
					    &p_so_pw);
    if(err != NM_ESUCCESS)
      goto out;

    list_add_tail(&p_so_pw->link, out_list);
    ((struct nm_so_policy_fifo_gate *)private)->nb_packets++;

  } else {
    /* Large packets can not be sent immediately : we have to issue a
       RdV request. */

    /* First allocate a packet wrapper */
    err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq,
                                            data, len,
					    0, 1,
                                            NM_SO_DATA_DONT_USE_HEADER,
                                            &p_so_pw);
    if(err != NM_ESUCCESS)
      goto out;

    /* Then place it into the appropriate list of large pending
       "sends". */
    list_add_tail(&p_so_pw->link,
                  &(p_so_gate->pending_large_send[tag]));


    /* Signal we're waiting for an ACK */
    p_so_gate->pending_unpacks++;

    /* Finally, generate a RdV request */
    {
      union nm_so_generic_ctrl_header ctrl;

      nm_so_init_rdv(&ctrl, tag + 128, seq, len, 0, 1);

      err = pack_ctrl(private, &ctrl);
      if(err != NM_ESUCCESS)
	goto out;
    }

    /* Check if we should post a new recv packet: we're waiting for an
       ACK! */
    nm_so_refill_regular_recv(p_gate);

  }

  err = NM_ESUCCESS;

 out:
  return err;
}

static int
try_and_commit(struct nm_gate *p_gate, void *private)
{
   struct list_head *out_list =
    &((struct nm_so_policy_fifo_gate *)private)->out_list;
  struct nm_so_pkt_wrap *p_so_pw;

  if(list_empty(out_list))
    /* We're done */
    goto out;

  /* Simply take the head of the list */
  p_so_pw = nm_l2so(out_list->next);
  list_del(out_list->next);
  ((struct nm_so_policy_fifo_gate *)private)->nb_packets--;

  /* Finalize packet wrapper */
  nm_so_pw_finalize(p_so_pw);

  /* Post packet on track 0 */
  _nm_so_post_send(p_gate, p_so_pw, TRK_SMALL, NM_SO_DEFAULT_NET);

 out:
  return NM_ESUCCESS;
}

static int
init_gate(void **addr_private)
{
  struct nm_so_policy_fifo_gate *priv =
    TBX_MALLOC(sizeof(struct nm_so_policy_fifo_gate));

  INIT_LIST_HEAD(&priv->out_list);

  addr_private[0] = priv;
  priv->nb_packets = 0;

  return NM_ESUCCESS;
}


static int
exit_gate(void **addr_private)
{
  struct nm_so_policy_fifo_gate *priv = addr_private[0];

  TBX_FREE(priv);

  return NM_ESUCCESS;
}

static int
busy(void *private)
{
  return ((struct nm_so_policy_fifo_gate *)private)->nb_packets;
}


nm_so_policy nm_so_policy_fifo = {
  .pack_ctrl = pack_ctrl,
  .pack = pack,
  .try_and_commit = try_and_commit,
  .init_gate = init_gate,
  .exit_gate = exit_gate,
  .busy = busy,
  .priv = NULL,
};

#endif /* NMAD_QOS */
