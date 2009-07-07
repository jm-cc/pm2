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

#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <nm_private.h>
#include "nm_so_policy_priority.h"
#include "nm_so_strat_qos.h"

struct nm_so_policy_priority_gate{
  struct list_head out_list[NM_SO_NB_PRIORITIES];
  unsigned nb_packets[NM_SO_NB_PRIORITIES];
 };

static int
pack_ctrl(void *private,
	  union nm_so_generic_ctrl_header *p_ctrl)
{
  uint8_t tag = p_ctrl->a.tag_id - 128;
  uint8_t priority = nm_so_get_priority(tag);
  struct list_head *out_list =
    &((struct nm_so_policy_priority_gate *)private)->out_list[priority];
  struct nm_pkt_wrap *p_so_pw = NULL;
  int err;

  /* We first try to find an existing packet to form an aggregate */
  list_for_each_entry(p_so_pw, out_list, link)
    {
      if(p_so_pw->length <= 32 - NM_SO_CTRL_HEADER_SIZE)
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
  ((struct nm_so_policy_priority_gate *)private)->nb_packets[priority]++;

 out:
  return err;
}

static int
pack(struct nm_gate *p_gate, void *private,
     uint8_t tag, uint8_t seq,
     void *data, uint32_t len)
{
  uint8_t priority = nm_so_get_priority(tag);
  struct list_head *out_list =
    &((struct nm_so_policy_priority_gate *)private)->out_list[priority];
  struct nm_pkt_wrap *p_so_pw;
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
	// There's not enough room to add our data to this paquet /
	goto next;

      if(len <= NM_SO_COPY_ON_SEND_THRESHOLD && size <= h_rlen)
	// We can copy data into the header zone /
	flags = NM_SO_DATA_USE_COPY;
      else
	if(p_so_pw->v_nb == NM_SO_PREALLOC_IOV_LEN)
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
    ((struct nm_so_policy_priority_gate *)private)->nb_packets[priority]++;
  } else {
    /* Large packets can not be sent immediately : we have to issue a
       RdV request. */

    /* First allocate a packet wrapper */
    err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq,
                                            data, len,
					    0, 1,
                                            NM_PW_NOHEADER,
                                            &p_so_pw);
    if(err != NM_ESUCCESS)
      goto out;

    /* Then place it into the appropriate list of large pending
       "sends". */
    list_add_tail(&p_so_pw->link,
                  &(nm_so_tag_get(&p_gate->tags, tag)->pending_large_send));

    /* Finally, generate a RdV request */
    {
      union nm_so_generic_ctrl_header ctrl;

      nm_so_init_rdv(&ctrl, tag + 128, seq, len, 0, 1);

      err = pack_ctrl(private, &ctrl);

      if(err != NM_ESUCCESS)
	goto out;
    }
  }

  err = NM_ESUCCESS;

 out:
  return err;
}

static int
try_and_commit(struct nm_gate *p_gate, void *private)
{
  uint8_t i;

  for(i = 0; i < NM_SO_NB_PRIORITIES; i++)
    {
      struct nm_so_policy_priority_gate *p_so_pg = ((struct nm_so_policy_priority_gate *)private);
      struct list_head *out_list = &p_so_pg->out_list[i];
      struct nm_pkt_wrap *p_so_pw;

      if(list_empty(out_list))
	/* We're done */
	goto next;

      /* Simply take the head of the list */
      p_so_pw = nm_l2so(out_list->next);
      list_del(out_list->next);
      ((struct nm_so_policy_priority_gate *)private)->nb_packets[i]--;
      /* Finalize packet wrapper */
      nm_so_pw_finalize(p_so_pw);

      /* Post packet on track 0 */
      nm_core_post_send(p_gate, p_so_pw, NM_TRK_SMALL, NM_SO_DEFAULT_NET);

      goto out;

    next:
      ;
    }

 out:
  ;

  return NM_ESUCCESS;
}

static int
init_gate(void **addr_private)
{
  struct nm_so_policy_priority_gate *priv =
    TBX_MALLOC(sizeof(struct nm_so_policy_priority_gate));
  uint8_t i;

  for(i = 0; i < NM_SO_NB_PRIORITIES; i++)
      INIT_LIST_HEAD(&priv->out_list[i]);

  for(i = 0; i < NM_SO_NB_PRIORITIES; i++)
    priv->nb_packets[i] = 0;

  addr_private[0] = priv;

  return NM_ESUCCESS;
}


static int
exit_gate(void **addr_private)
{
  struct nm_so_policy_priority_gate *priv = addr_private[0];

  TBX_FREE(priv);

  return NM_ESUCCESS;
}

static int
busy(void *private)
{
  struct nm_so_policy_priority_gate * priv = (struct nm_so_policy_priority_gate *)private;
  uint8_t i;
  unsigned res = 0;

  for(i = 0; i < NM_SO_NB_PRIORITIES; i++)
    res += priv->nb_packets[i];

  return res;
}


nm_so_policy nm_so_policy_priority = {
  .pack_ctrl = pack_ctrl,
  .pack = pack,
  .try_and_commit = try_and_commit,
  .init_gate = init_gate,
  .exit_gate = exit_gate,
  .busy = busy,
  .priv = NULL,
};

#endif /* NMAD_QOS */
