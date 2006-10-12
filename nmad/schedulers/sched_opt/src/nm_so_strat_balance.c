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

#include <tbx.h>
#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include "nm_so_private.h"
#include "nm_so_strategies/nm_so_strat_balance.h"
#include "nm_so_pkt_wrap.h"
#include "nm_so_tracks.h"

struct nm_so_strat_balance_gate {
  /* list of raw outgoing packets */
  struct list_head out_list;
};

int out_list_len = 0;
//int active_drv[NM_SO_MAX_NETS];

/* Add a new control "header" to the flow of outgoing packets */
static int pack_ctrl(struct nm_gate *p_gate,
		     union nm_so_generic_ctrl_header *p_ctrl)
{
  struct nm_so_pkt_wrap *p_so_pw = NULL;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_strat_balance_gate *p_so_sa_gate
    = (struct nm_so_strat_balance_gate *)p_so_gate->strat_priv;
  int flags = 0;
  int err;


  // on ne s'agr�ge qu'� un paquet de ctrl dont la taille est < 64
  /* We first try to find an existing packet to form an aggregate */
  list_for_each_entry(p_so_pw, &p_so_sa_gate->out_list, link) {
    uint32_t used_space = p_so_pw->header_index->iov_len;
    uint32_t ctrl_size = NM_SO_CTRL_HEADER_SIZE;

    if(used_space + ctrl_size >  64)
      /* There's not enough room to add our ctrl header
         to this paquet */
      goto next;

    err = nm_so_pw_add_control(p_so_pw, p_ctrl);
    goto out;

  next:
    ;
  }

  /* Simply form a new packet wrapper */
  err = nm_so_pw_alloc_and_fill_with_control(p_ctrl,
					     &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  /* Add the control packet to the out_list */
  list_add(&p_so_pw->link, &p_so_sa_gate->out_list);
  out_list_len++;

 out:
  return err;
}

/* Handle the arrival of a new packet. The strategy may already apply
   some optimizations at this point */
static int pack(struct nm_gate *p_gate,
		uint8_t tag, uint8_t seq,
		void *data, uint32_t len)
{
  struct nm_so_pkt_wrap *p_so_pw;
  struct nm_so_gate *p_so_gate = (struct nm_so_gate *)p_gate->sch_private;
  struct nm_so_strat_balance_gate *p_so_sa_gate
    = (struct nm_so_strat_balance_gate *)p_so_gate->strat_priv;
  int flags = 0;
  int err;

  //DISP("-->pack");

  p_so_gate->status[tag][seq] &= ~NM_SO_STATUS_SEND_COMPLETED;

  if(len <= NM_SO_MAX_SMALL) {
    /* Small packet */

    // si on a plus de 2 paquets en attente, on agr�ge
    if( out_list_len > 2){
 
      /* We first try to find an existing packet to form an aggregate */
      list_for_each_entry(p_so_pw, &p_so_sa_gate->out_list, link) {
        uint32_t h_rlen = nm_so_pw_remaining_header_area(p_so_pw);
        uint32_t size = NM_SO_DATA_HEADER_SIZE + nm_so_aligned(len);

        if(size <= h_rlen)
          /* We can copy data into the header zone */
          flags = NM_SO_DATA_USE_COPY;
        else
          /* There's not enough room to add our data to this paquet */
	  goto next;

        err = nm_so_pw_add_data(p_so_pw, tag + 128, seq, data, len, flags);
        goto out;

      next:
        ;
      }
    }
 
    flags = NM_SO_DATA_USE_COPY;

    /* We didn't have a chance to form an aggregate, so simply form a
       new packet wrapper and add it to the out_list */
    err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq,
					    data, len,
					    flags,
					    &p_so_pw);
    if(err != NM_ESUCCESS)
      goto out;

    list_add_tail(&p_so_pw->link, &p_so_sa_gate->out_list);
    out_list_len ++;

  } else {
    /* Large packets can not be sent immediately : we have to issue a
       RdV request. */

    /* First allocate a packet wrapper */
    err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq,
                                            data, len,
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

      nm_so_init_rdv(&ctrl, tag + 128, seq);

      err = pack_ctrl(p_gate, &ctrl);
      if(err != NM_ESUCCESS)
	goto out;
    }

    /* Check if we should post a new recv packet: we're waiting for an
       ACK! */
    nm_so_refill_regular_recv(p_gate);

  }

  err = NM_ESUCCESS;

 out:
  //DISP("<--pack");
  return err;
}

/* Compute and apply the best possible packet rearrangement, then
   return next packet to send */
static int try_and_commit(struct nm_gate *p_gate)
{

  //DISP("-->try_and_commit");

  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct list_head *out_list =
    &((struct nm_so_strat_balance_gate *)p_so_gate->strat_priv)->out_list;
  struct nm_so_pkt_wrap *p_so_pw;
  int drv_id;

 start:
  if(list_empty(out_list))
    /* We're done */
    goto out;

  for(drv_id = NM_SO_MAX_NETS -1 ; drv_id >= 0; drv_id--)
    if (p_so_gate->active_send[drv_id ][TRK_SMALL] == 0
        && p_so_gate->active_send[drv_id][TRK_LARGE] == 0)
      /* We found an idle NIC */
      goto next;


  /* We didn't found any idle NIC, so we're done*/
  goto out;

 next:
  /* Simply take the head of the list */
  p_so_pw = nm_l2so(out_list->next);
  list_del(out_list->next);
  out_list_len --;

  /* Finalize packet wrapper */
  nm_so_pw_finalize(p_so_pw);

  /* Post packet on track 0 */
  _nm_so_post_send(p_gate, p_so_pw, TRK_SMALL, drv_id);

  /* Loop and see if we can feed another NIC with a ready paquet */
  goto start;

 out:
  //DISP("<--try_and_commit");

    return NM_ESUCCESS;
}

/* Initialization */
static int init(void)
{
  printf("[loading strategy: <balance>]\n");
  return NM_ESUCCESS;
}

static int init_gate(struct nm_gate *p_gate)
{
  struct nm_so_strat_balance_gate *priv
    = TBX_MALLOC(sizeof(struct nm_so_strat_balance_gate));

  INIT_LIST_HEAD(&priv->out_list);

  ((struct nm_so_gate *)p_gate->sch_private)->strat_priv = priv;

  return NM_ESUCCESS;
}


nm_so_strategy nm_so_strat_balance = {
  .init = init,
  .init_gate = init_gate,
  .pack = pack,
  .pack_ctrl = pack_ctrl,
  .try = NULL,
  .commit = NULL,
  .try_and_commit = try_and_commit,
  .cancel = NULL,
  .priv = NULL,
};
