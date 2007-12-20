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
 * MERCHANTABILITY or FITNESS FOR A PARTICULA R PURPOSE.  See the GNU
 * General Public License for more details.
 */


#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <tbx.h>

#include <nm_public.h>
#include "nm_pkt_wrap.h"
#include "nm_trk.h"
#include "nm_so_private.h"
#include "nm_so_pkt_wrap.h"
#include "nm_so_headers.h"
#include "nm_so_tracks.h"
#include "nm_so_sendrecv_interface.h"
#include "nm_so_interfaces.h"
#include "nm_so_debug.h"
#include "nm_log.h"
#include "nm_so_process_large_transfer.h"
#include "nm_so_process_unexpected.h"

#include <ccs_public.h>
#include <segment.h>

int
_nm_so_copy_data_in_iov(struct iovec *iov, uint32_t chunk_offset, void *data, uint32_t len){
  uint32_t chunk_len, pending_len = len;
  int offset = 0;
  int i = 0;

  while(1){
    if(offset + iov[i].iov_len > chunk_offset)
      break;

    offset += iov[i].iov_len;
    i++;
  }

  if(offset + iov[i].iov_len >= chunk_offset + len){
    /* Data are contiguous */
    memcpy(iov[i].iov_base + (chunk_offset-offset), data, len);
    goto out;
  }

  /* Data are on several ioec entries */

  /* first entry: it can be a fragment of entry */
  chunk_len = iov[i].iov_len - (chunk_offset - offset);

  memcpy(iov[i].iov_base + (chunk_offset - offset), data, chunk_len);

  pending_len -= chunk_len;
  offset += iov[i].iov_len;
  i++;

  /* following entries: only full part */
  while(pending_len){

    if(offset + iov[i].iov_len >= chunk_offset + pending_len){
      /* it is the last line */
      memcpy(iov[i].iov_base, data+(len-pending_len), pending_len);
      goto out;
    }

    chunk_len = iov[i].iov_len;

    memcpy(iov[i].iov_base, data+(len-pending_len), chunk_len);

    pending_len -= chunk_len;
    offset += iov[i].iov_len;
    i++;
  }

 out:
  return NM_ESUCCESS;
}

static int
iov_len(struct iovec *iov, int nb_entries){
  uint32_t len = 0;
  int i;

  for(i = 0; i < nb_entries; i++){
    len += iov[i].iov_len;
  }

  return len;
}

static int
datatype_size(struct DLOOP_Segment *segp){
  DLOOP_Handle handle;
  int data_sz;

  handle = segp->handle;
  CCSI_datadesc_get_size_macro(handle, data_sz); // * count?

  return data_sz;
}

/** Handle a source-less unpack.
 */
static int
____nm_so_unpack_any_src(struct nm_core *p_core,
                         uint8_t tag, uint8_t flag,
                         void *data_description, uint32_t len){
  struct nm_so_sched *p_so_sched = p_core->p_sched->sch_private;
  struct nm_gate *p_gate = NULL;
  struct nm_so_gate *p_so_gate = NULL;
  uint8_t *status = NULL;
  int seq, first, i, err = NM_ESUCCESS;

  NM_SO_TRACE("Unpack_ANY_src - tag = %u\n", tag);

  if(p_so_sched->any_src[tag].status & NM_SO_STATUS_UNPACK_HERE)
    TBX_FAILURE("Simultaneous any_src reception on the same tag");

  first = p_so_sched->next_gate_id;
  do {

    p_gate = p_core->gate_array + p_so_sched->next_gate_id;
    p_so_gate = p_gate->sch_private;

    seq = p_so_gate->recv_seq_number[tag];
    status = &p_so_gate->status[tag][seq];

    if(*status & NM_SO_STATUS_PACKET_HERE) {
      /* Wow! At least one data chunk already in! */
      NM_SO_TRACE("At least one data chunk already in on gate %u\n", p_gate->id);

      *status = 0;
      p_so_gate->recv_seq_number[tag]++;

      p_so_sched->any_src[tag].status |= NM_SO_STATUS_UNPACK_HERE;
      p_so_sched->any_src[tag].status |= flag;

      p_so_sched->any_src[tag].data = data_description;
      p_so_sched->any_src[tag].gate_id = p_gate->id;
      p_so_sched->any_src[tag].seq = seq;

      p_so_sched->pending_any_src_unpacks++;

      if(treat_unexpected(tbx_true, p_gate, tag, seq, len, data_description) == NM_ESUCCESS){
        goto out;
      }

      goto out;
    }

    p_so_sched->next_gate_id = (p_so_sched->next_gate_id + 1) % p_core->nb_gates;

  } while(p_so_sched->next_gate_id != first);

  NM_SO_TRACE("No data chunk already in - gate_id initialized at -1\n");
  p_so_sched->any_src[tag].status = 0;
  p_so_sched->any_src[tag].status |= NM_SO_STATUS_UNPACK_HERE;
  p_so_sched->any_src[tag].status |= flag;

  p_so_sched->any_src[tag].data = data_description;
  p_so_sched->any_src[tag].gate_id = -1;

  p_so_sched->any_src[tag].expected_len  = len;
  p_so_sched->any_src[tag].cumulated_len = 0;

  p_so_sched->pending_any_src_unpacks++;

  /* Make sure that each gate has a posted receive */
  for(i = 0; i < p_core->nb_gates; i++)
    nm_so_refill_regular_recv(&p_core->gate_array[i]);

 out:
  return err;
}

int
__nm_so_unpack_any_src(struct nm_core *p_core, uint8_t tag, void *data, uint32_t len)
{
    /* Nothing special to flag for the contiguous reception */
  uint8_t flag = 0;

  return ____nm_so_unpack_any_src(p_core, tag, flag, data, len);
}


int
__nm_so_unpackv_any_src(struct nm_core *p_core, uint8_t tag, struct iovec *iov, int nb_entries)
{
  /* Data will be receive in an iovec tab */
  uint8_t flag = 0;
  flag |= NM_SO_STATUS_UNPACK_IOV;

  return ____nm_so_unpack_any_src(p_core, tag, flag, iov, iov_len(iov, nb_entries));
}

int
__nm_so_unpack_datatype_any_src(struct nm_core *p_core, uint8_t tag, struct DLOOP_Segment *segp){

  /* Data will be receive through a datatype */
  uint8_t flag = 0;
  flag |= NM_SO_STATUS_IS_DATATYPE;

  return ____nm_so_unpack_any_src(p_core, tag, flag, segp, datatype_size(segp));
}


/** Handle a sourceful unpack.
 */
static int
____nm_so_unpack(struct nm_gate *p_gate,
                 uint8_t tag, uint8_t seq,
                 uint8_t flag,
                 void *data_description, uint32_t len){
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  uint8_t *status = &(p_so_gate->status[tag][seq]);
  int err;

  NM_SO_TRACE("Intern Unpack - gate_id = %u, tag = %u, seq = %u\n", p_gate->id, tag, seq);

  if(*status & NM_SO_STATUS_PACKET_HERE) {
    /* Wow! At least one chunk already in! */
    NM_SO_TRACE("At least one data chunk already in!\n");

    *status |= flag;

    /**********************************************************************************/
    /* Never access to the data stored in the 'unpack_here' side of the status matrix */
    /* It is required to retrieve the data stored in  the 'pkt_here' side before!     */
    /**********************************************************************************/

    if(treat_unexpected(tbx_false, p_gate, tag, seq, len, data_description) == NM_ESUCCESS){
      goto out;
    }

  } else {
    p_so_gate->recv[tag][seq].unpack_here.expected_len = len;
    p_so_gate->recv[tag][seq].unpack_here.cumulated_len = 0;
  }

  *status = 0;
  *status |= NM_SO_STATUS_UNPACK_HERE;
  *status |= flag;

  p_so_gate->recv[tag][seq].unpack_here.data = data_description;

  p_so_gate->pending_unpacks++;

  /* Check if we should post a new recv packet */
  nm_so_refill_regular_recv(p_gate);

  err = NM_ESUCCESS;

 out:
  return err;
}

int
__nm_so_unpack(struct nm_gate *p_gate,
	       uint8_t tag, uint8_t seq,
	       void *data, uint32_t len)
{
  /* Nothing special to flag for the contiguous reception */
  uint8_t flag = 0;

  return ____nm_so_unpack(p_gate, tag, seq, flag, data, len);
}

int
__nm_so_unpackv(struct nm_gate *p_gate,
                uint8_t tag, uint8_t seq,
                struct iovec *iov, int nb_entries)
{
  /* Data will be receive in an iovec tab */
  uint8_t flag = 0;
  flag |= NM_SO_STATUS_UNPACK_IOV;

  return ____nm_so_unpack(p_gate, tag, seq, flag, iov, iov_len(iov, nb_entries));
}

int
__nm_so_unpack_datatype(struct nm_gate *p_gate,
                        uint8_t tag, uint8_t seq,
                        struct DLOOP_Segment *segp)
{
  /* Data will be receive through a datatype */
  uint8_t flag = 0;
  flag |= NM_SO_STATUS_IS_DATATYPE;

  return ____nm_so_unpack(p_gate, tag, seq, flag, segp, datatype_size(segp));
}

/** Schedule and post new incoming buffers.
 */
int
nm_so_in_schedule(struct nm_sched *p_sched TBX_UNUSED)
{
  return NM_ESUCCESS;
}

static int
process_small_data(tbx_bool_t is_any_src,
                   struct nm_so_pkt_wrap *p_so_pw,
                   void *ptr,
                   uint32_t len, uint8_t tag, uint8_t seq,
                   uint32_t chunk_offset, uint8_t is_last_chunk){
  struct nm_gate *p_gate = p_so_pw->pw.p_gate;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched =  p_so_gate->p_so_sched;
  struct nm_so_interface_ops *interface = p_so_sched->current_interface;
  uint8_t *status = NULL;
  int32_t *p_cumulated_len;
  int32_t *p_expected_len;

  if(is_any_src){
    status = &(p_so_sched->any_src[tag].status);
    p_cumulated_len = &(p_so_sched->any_src[tag].cumulated_len);
    p_expected_len  = &(p_so_sched->any_src[tag].expected_len);

  } else {
    status = &(p_so_gate->status[tag][seq]);
    p_cumulated_len = &(p_so_gate->recv[tag][seq].unpack_here.cumulated_len);
    p_expected_len  = &(p_so_gate->recv[tag][seq].unpack_here.expected_len);
  }

  /* Save the real length to receive */
  if(is_last_chunk){
    *p_expected_len = chunk_offset + len;
  }

  if(is_any_src && p_so_sched->any_src[tag].gate_id == -1){
    p_so_gate->recv_seq_number[tag]++;
    p_so_sched->any_src[tag].gate_id = p_gate->id;
    p_so_sched->any_src[tag].seq = seq;
  }

  if(len){
    /* Copy data to its final destination */
    if(*status & NM_SO_STATUS_UNPACK_IOV){
      struct iovec *iov = NULL;
      if(is_any_src){
        iov = p_so_sched->any_src[tag].data;
      } else {
        iov = p_so_gate->recv[tag][seq].unpack_here.data;
      }

      /* Destination is organized in several non contiguous buffers */
      _nm_so_copy_data_in_iov(iov, chunk_offset, ptr, len);

    }else if (*status & NM_SO_STATUS_IS_DATATYPE) {
      DLOOP_Offset last = chunk_offset + len;
      struct DLOOP_Segment *segp = NULL;

      if(is_any_src){
        segp = p_so_sched->any_src[tag].data;
      } else {
        segp = p_so_gate->recv[tag][seq].unpack_here.data;
      }

      /* Data are described by a datatype */
      CCSI_Segment_unpack(segp, chunk_offset, &last, ptr);

    } else {
      void *data = NULL;
      if(is_any_src){
        data = p_so_sched->any_src[tag].data + chunk_offset;
      } else {
        data = p_so_gate->recv[tag][seq].unpack_here.data + chunk_offset;
      }

      /* Data are contiguous */
      memcpy(data, ptr, len);
    }
  }

  *p_cumulated_len += len;

  /* Verify if the communication is done */
  if(*p_cumulated_len >= *p_expected_len){
    NM_SO_TRACE("Wow! All data chunks were already in!\n");

    *status = 0;

    if(is_any_src){
      p_so_sched->pending_any_src_unpacks--;
      p_so_sched->any_src[tag].gate_id = -1;

    } else {
      p_so_gate->pending_unpacks--;

      p_so_gate->recv[tag][seq].pkt_here.header = NULL;
    }

    interface->unpack_success(p_gate, tag, seq, is_any_src);
  }
  return NM_SO_HEADER_MARK_READ;
}

static int
store_data_or_rdv(tbx_bool_t rdv,
                  void *header, uint8_t tag, uint8_t seq,
                  struct nm_so_pkt_wrap *p_so_pw){
  struct nm_gate *p_gate = p_so_pw->pw.p_gate;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;

  if(!(p_so_gate->status[tag][seq] & NM_SO_STATUS_PACKET_HERE)){

    p_so_gate->recv[tag][seq].pkt_here.header = header;
    p_so_gate->recv[tag][seq].pkt_here.p_so_pw = p_so_pw;

  } else {
    if(p_so_gate->recv[tag][seq].pkt_here.chunks == NULL){
      p_so_gate->recv[tag][seq].pkt_here.chunks = TBX_MALLOC(sizeof(struct list_head));
      INIT_LIST_HEAD(p_so_gate->recv[tag][seq].pkt_here.chunks);
    }

    struct nm_so_chunk *chunk = tbx_malloc(nm_so_chunk_mem);
    if (!chunk) {
      TBX_FAILURE("chunk allocation - err = -NM_ENOMEM");
    }

    chunk->header = header;
    chunk->p_so_pw = p_so_pw;

    list_add_tail(&chunk->link, p_so_gate->recv[tag][seq].pkt_here.chunks);
  }

  p_so_gate->status[tag][seq]|= NM_SO_STATUS_PACKET_HERE;
  if(rdv){
    p_so_gate->status[tag][seq] |= NM_SO_STATUS_RDV_HERE;
  }

  return NM_SO_HEADER_MARK_UNREAD;
}


/** Process a complete data request.
 */
static int
data_completion_callback(struct nm_so_pkt_wrap *p_so_pw,
                         void *ptr,
                         void *header, uint32_t len,
                         uint8_t proto_id, uint8_t seq,
                         uint32_t chunk_offset,
                         uint8_t is_last_chunk){
  struct nm_gate *p_gate = p_so_pw->pw.p_gate;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched =  p_so_gate->p_so_sched;
  uint8_t tag = proto_id - 128;

  NM_SO_TRACE("Recv completed for chunk : %p, len = %u, tag = %d, seq = %u, offset = %u, is_last_chunk = %u\n", ptr, len, tag, seq, chunk_offset, is_last_chunk);

  if(p_so_gate->status[tag][seq] & NM_SO_STATUS_UNPACK_HERE) {
    /* Cool! We already have a waiting unpack for this packet */
    return process_small_data(tbx_false, p_so_pw, ptr,
                              len, tag, seq, chunk_offset, is_last_chunk);

  } else if ((p_so_sched->any_src[tag].status & NM_SO_STATUS_UNPACK_HERE)
             &&
             ((p_so_sched->any_src[tag].gate_id == -1 && p_so_gate->recv_seq_number[tag] == seq)
              || (p_so_sched->any_src[tag].gate_id == p_gate->id && p_so_sched->any_src[tag].seq == seq))) {

    NM_SO_TRACE("Pending any_src reception\n");
    return process_small_data(tbx_true, p_so_pw, ptr,
                              len, tag, seq, chunk_offset, is_last_chunk);

  } else {
    /* Receiver process is not ready, so store the information in the
       recv array and keep the p_so_pw packet alive */
    NM_SO_TRACE("Store the data chunk with tag = %u, seq = %u\n", tag, seq);

    return store_data_or_rdv(tbx_false, header, tag, seq, p_so_pw);
  }
}

/** Process a complete rendez-vous request.
 */
static int
rdv_callback(struct nm_so_pkt_wrap *p_so_pw,
             void *rdv,
             uint8_t tag_id, uint8_t seq,
             uint32_t len, uint32_t chunk_offset, uint8_t is_last_chunk){
  struct nm_gate *p_gate = p_so_pw->pw.p_gate;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;
  uint8_t tag = tag_id - 128;
  uint8_t *status = &(p_so_gate->status[tag][seq]);
  int err;

  NM_SO_TRACE("RDV completed for tag = %d, seq = %u len = %u, offset = %u\n", tag, seq, len, chunk_offset);

  if(*status & NM_SO_STATUS_UNPACK_HERE) {
    /* Application is already ready! */
    NM_SO_TRACE("RDV for a classic exchange on tag %u, seq = %u, len = %u, chunk_offset = %u, is_last_chunk = %u\n", tag, seq, len, chunk_offset, is_last_chunk);

    err = nm_so_rdv_success(tbx_false, p_gate, tag, seq,
                            len, chunk_offset, is_last_chunk);

  } else if ((p_so_sched->any_src[tag].status & NM_SO_STATUS_UNPACK_HERE)
             &&
             ((p_so_sched->any_src[tag].gate_id == -1 && p_so_gate->recv_seq_number[tag] == seq)
              || (p_so_sched->any_src[tag].gate_id == p_gate->id && p_so_sched->any_src[tag].seq == seq))) {
    /* Check if the received rdv matches with an any_src request */
    NM_SO_TRACE("RDV for an ANY_SRC exchange for tag = %d, seq = %u len = %u, offset = %u\n", tag, seq, len, chunk_offset);

    err = nm_so_rdv_success(tbx_true, p_gate, tag, seq,
                            len, chunk_offset, is_last_chunk);

  } else {
    /* Store rdv request */
    NM_SO_TRACE("Store the RDV for tag = %d, seq = %u len = %u, offset = %u\n", tag, seq, len, chunk_offset);

    return store_data_or_rdv(tbx_true, rdv, tag, seq, p_so_pw);
  }

  return NM_SO_HEADER_MARK_READ;
}

/** Process a complete rendez-vous acknowledgement request.
 */
static int
ack_callback(struct nm_so_pkt_wrap *p_so_pw,
             uint8_t tag_id, uint8_t seq,
             uint8_t track_id, uint32_t chunk_offset){
  struct nm_gate *p_gate = p_so_pw->pw.p_gate;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;
  struct nm_so_pkt_wrap *p_so_large_pw = NULL;
  uint8_t tag = tag_id - 128;

  NM_SO_TRACE("ACK completed for tag = %d, seq = %u, offset = %u\n", tag, seq, chunk_offset);

  p_so_gate->pending_unpacks--;
  p_so_gate->status[tag][seq] |= NM_SO_STATUS_ACK_HERE;

  list_for_each_entry(p_so_large_pw, &p_so_gate->pending_large_send[tag], link) {
    NM_SO_TRACE("Searching the pw corresponding to the ack - cur_seq = %d - cur_offset = %d\n", p_so_large_pw->pw.seq, p_so_large_pw->chunk_offset);

    if(p_so_large_pw->pw.seq == seq && p_so_large_pw->chunk_offset == chunk_offset) {
      FUT_DO_PROBE3(FUT_NMAD_NIC_RECV_ACK_RNDV, p_so_large_pw, p_gate->id, 1/* large output list*/);
      list_del(&p_so_large_pw->link);

      if(p_so_gate->status[tag][seq] & NM_SO_STATUS_IS_DATATYPE
         || p_so_sched->any_src[tag].status & NM_SO_STATUS_IS_DATATYPE){
        int nb_blocks = 0;
        int last = p_so_large_pw->pw.length;
        int nb_entries = NM_SO_PREALLOC_IOV_LEN - p_so_large_pw->pw.v_nb;

        CCSI_Segment_count_contig_blocks(p_so_large_pw->pw.segp, 0, &last, &nb_blocks);

        if(nb_blocks < nb_entries){
          NM_SO_TRACE("The data will be sent through an iovec directly from their location\n");
          CCSI_Segment_pack_vector(p_so_large_pw->pw.segp,
                                   p_so_large_pw->pw.datatype_offset, (DLOOP_Offset *)&last,
                                   (DLOOP_VECTOR *)p_so_large_pw->pw.v,
                                   &nb_entries);
          NM_SO_TRACE("Pack with %d entries from byte %d to byte %d (%d bytes)\n", nb_entries, p_so_large_pw->pw.datatype_offset, last, last-p_so_large_pw->pw.datatype_offset);

        } else {
          NM_SO_TRACE("There is no enough space in the iovec - copy in a contiguous buffer and send\n");
          p_so_large_pw->datatype_copied_buf = tbx_true;

          p_so_large_pw->pw.v[0].iov_base = TBX_MALLOC(p_so_large_pw->pw.length);

          CCSI_Segment_pack(p_so_large_pw->pw.segp,
                            p_so_large_pw->pw.datatype_offset, &last,
                            p_so_large_pw->pw.v[0].iov_base);
          p_so_large_pw->pw.v[0].iov_len = last-p_so_large_pw->pw.datatype_offset;
          nb_entries = 1;
        }

        p_so_large_pw->pw.length = last - p_so_large_pw->pw.datatype_offset;
        p_so_large_pw->pw.v_nb += nb_entries;
        p_so_large_pw->pw.datatype_offset = last;
      }

      /* Send the data */
      _nm_so_post_send(p_gate, p_so_large_pw,
		       track_id % NM_SO_MAX_TRACKS,
		       track_id / NM_SO_MAX_TRACKS);

      return NM_SO_HEADER_MARK_READ;
    }
  }
  TBX_FAILURE("PANIC!\n");
}

static int
ack_chunk_callback(struct nm_so_pkt_wrap *p_so_pw,
                   uint8_t tag_id, uint8_t seq, uint32_t chunk_offset,
                   uint8_t track_id, uint32_t chunk_len){
  struct nm_gate *p_gate = p_so_pw->pw.p_gate;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;
  struct nm_so_pkt_wrap *p_so_large_pw, *p_so_large_pw2;
  uint8_t tag = tag_id - 128;

  NM_SO_TRACE("ACK_CHUNK completed for tag = %d, track_id = %u, seq = %u, chunk_len = %u, chunk_offset = %u\n", tag, track_id, seq, chunk_len, chunk_offset);

  list_for_each_entry(p_so_large_pw, &p_so_gate->pending_large_send[tag], link) {
    NM_SO_TRACE("Searching the pw corresponding to the ack_chunk - cur_seq = %d - cur_offset = %d\n", p_so_large_pw->pw.seq, p_so_large_pw->chunk_offset);

    if(p_so_large_pw->pw.seq == seq && p_so_large_pw->chunk_offset == chunk_offset) {
      list_del(&p_so_large_pw->link);

      // Les données à envoyer sont décrites par un datatype
      if(p_so_gate->status[tag][seq] & NM_SO_STATUS_IS_DATATYPE
         || p_so_sched->any_src[tag].status & NM_SO_STATUS_IS_DATATYPE){

        int length = p_so_large_pw->pw.length;
        uint32_t first = p_so_large_pw->pw.datatype_offset;
        uint32_t last  = first + chunk_len;
        int nb_entries = 1;
        int nb_blocks = 0;
        CCSI_Segment_count_contig_blocks(p_so_large_pw->pw.segp, first, &last, &nb_blocks);
        nb_entries = NM_SO_PREALLOC_IOV_LEN - p_so_large_pw->pw.v_nb;

        // dans p_so_large_pw, je pack les données à émettre pour qu'il y en est chunk_len o
        if(nb_blocks < nb_entries){
          CCSI_Segment_pack_vector(p_so_large_pw->pw.segp,
                                   first, (DLOOP_Offset *)&last,
                                   (DLOOP_VECTOR *)p_so_large_pw->pw.v,
                                   &nb_entries);
          NM_SO_TRACE("Pack with %d entries from byte %d to byte %d (%d bytes)\n", nb_entries, p_so_large_pw->pw.datatype_offset, last, last-p_so_large_pw->pw.datatype_offset);
        } else {
          // on a pas assez d'entrées dans notre pw pour pouvoir envoyer tout en 1 seule fois
          // donc on copie en contigu et on envoie
          p_so_large_pw->pw.v[0].iov_base = TBX_MALLOC(chunk_len);

          CCSI_Segment_pack(p_so_large_pw->pw.segp, first, &last, p_so_large_pw->pw.v[0].iov_base);
          p_so_large_pw->pw.v[0].iov_len = last-first;
          nb_entries = 1;
        }

        p_so_large_pw->pw.length = last - first;
        p_so_large_pw->pw.v_nb += nb_entries;
        p_so_large_pw->pw.datatype_offset = last;

        if(chunk_offset + chunk_len < length){
          //dans p_so_large_pw2, je laisse la fin du segment
          nm_so_pw_alloc(NM_SO_DATA_DONT_USE_HEADER, &p_so_large_pw2);
          p_so_large_pw2->pw.p_gate   = p_so_large_pw->pw.p_gate;
          p_so_large_pw2->pw.proto_id = p_so_large_pw->pw.proto_id;
          p_so_large_pw2->pw.seq      = p_so_large_pw->pw.seq;
          p_so_large_pw2->pw.length   = length;
          p_so_large_pw2->pw.v_nb     = 0;
          p_so_large_pw2->pw.segp     = p_so_large_pw->pw.segp;
          p_so_large_pw2->pw.datatype_offset = last;
          p_so_large_pw2->chunk_offset = p_so_large_pw2->pw.datatype_offset;

          list_add(&p_so_large_pw2->link, &p_so_gate->pending_large_send[tag]);
        } else {
          p_so_gate->pending_unpacks--;
        }

      // Les données sont envoyées depuis un buffer contigu ou un iov
      } else {
        if(chunk_len < p_so_large_pw->pw.length){
	  FUT_DO_PROBE3(FUT_NMAD_NIC_RECV_ACK_RNDV, p_so_large_pw,
	  		p_gate->id, 1/* large output list*/);
          nm_so_pw_split(p_so_large_pw, &p_so_large_pw2, chunk_len);
	  FUT_DO_PROBE5(FUT_NMAD_GATE_OPS_IN_TO_OUT_SPLIT, p_so_large_pw,
	  		p_so_large_pw2, chunk_len, p_gate->id, 1/* outlist*/);
          list_add(&p_so_large_pw2->link, &p_so_gate->pending_large_send[tag]);
        } else {
          p_so_gate->pending_unpacks--;
        }
      }

      /* Send the data */
      _nm_so_post_send(p_gate, p_so_large_pw,
                       track_id % NM_SO_MAX_TRACKS,
		       track_id / NM_SO_MAX_TRACKS);

      return NM_SO_HEADER_MARK_READ;
    }
  }
  TBX_FAILURE("PANIC!\n");
}

/** Process a complete successful incoming request.
 */
int
nm_so_in_process_success_rq(struct nm_sched	*p_sched,
                            struct nm_pkt_wrap	*p_pw)
{
  struct nm_so_pkt_wrap *p_so_pw = nm_pw2so(p_pw);
  struct nm_gate *p_gate = p_so_pw->pw.p_gate;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;
  int err;

  NM_SO_TRACE("Packet %p received completely (on track %d, gate %d)!\n", p_so_pw, p_pw->p_trk->id, (int)p_gate->id);

  if(p_pw->p_trk->id == TRK_SMALL) {
    /* Track 0 */
    NM_SO_TRACE("Reception of a small packet\n");
    p_so_gate->active_recv[p_pw->p_drv->id][TRK_SMALL] = 0;

    nm_so_pw_iterate_over_headers(p_so_pw,
				  data_completion_callback,
				  rdv_callback,
                                  ack_callback,
                                  ack_chunk_callback);

    if(p_so_gate->pending_unpacks ||
       p_so_gate->p_so_sched->pending_any_src_unpacks) {

      NM_LOG_VAL("pending_unpacks", p_so_gate->pending_unpacks);
      NM_LOG_VAL("pending_any_src_unpacks", p_so_gate->p_so_sched->pending_any_src_unpacks);

      /* Check if we should post a new recv packet */
      nm_so_refill_regular_recv(p_gate);
    }

  } else if(p_pw->p_trk->id == TRK_LARGE) {

    /* This is the completion of a large message. */
    uint8_t drv_id = p_pw->p_drv->id;
    uint8_t tag = p_so_pw->pw.proto_id - 128;
    uint8_t seq = p_so_pw->pw.seq;
    uint32_t len = p_pw->length;
    struct nm_so_interface_ops *interface= p_so_sched->current_interface;
    nm_so_strategy *cur_strat = p_so_sched->current_strategy;
    int nb_drivers = p_gate->p_sched->p_core->nb_drivers;

    NM_SO_TRACE("********Reception of a large packet\n");

    if(nb_drivers == 1){

      //1) C'est un datatype à extraire d'un tampon intermédiaire
      if(p_so_gate->status[tag][seq] & NM_SO_STATUS_UNPACK_RETRIEVE_DATATYPE
         || p_so_gate->p_so_sched->any_src[tag].status & NM_SO_STATUS_UNPACK_RETRIEVE_DATATYPE){ //if(p_pw->segp != NULL){
        DLOOP_Offset last = p_pw->length;

        // Les données on été réçues en contigues -> il faut les redistribuer vers leur destination finale\n");
        CCSI_Segment_unpack(p_pw->segp, 0, &last, p_pw->v[0].iov_base);

        if(last < p_pw->length){
          // on a pas reçu assez et on le sait car on a la taille exacte à recevoir dans le rdv
          p_so_pw->pw.v[0].iov_base += last; // idx global ou nb de bytes consommés?
          p_so_pw->pw.v[0].iov_len -= last;

          nm_so_direct_post_large_recv(p_gate, p_pw->p_drv->id, p_so_pw);
          goto out;

        } else {
          TBX_FREE(p_pw->v[0].iov_base);

          if( p_so_gate->p_so_sched->any_src[tag].status & NM_SO_STATUS_UNPACK_RETRIEVE_DATATYPE){
            /* Completion of an ANY_SRC unpack */
            p_so_sched->any_src[tag].status = 0;
            p_so_sched->any_src[tag].gate_id = -1;

            p_so_sched->pending_any_src_unpacks--;
            interface->unpack_success(p_gate, tag, seq, tbx_true);

          } else {
            p_so_gate->pending_unpacks--;

            /* Reset the status matrix*/
            p_so_gate->recv[tag][seq].pkt_here.header = NULL;
            p_so_gate->status[tag][seq] = 0;

            interface->unpack_success(p_gate, tag, seq, tbx_false);
          }

        }
        //2) C'est un any_src
      } else if(p_so_gate->p_so_sched->any_src[tag].status & NM_SO_STATUS_RDV_IN_PROGRESS) {
        NM_SO_TRACE("Completion of a large any_src message with tag %d et seq %d\n", tag, seq);

        p_so_sched->any_src[tag].cumulated_len += len;

        if(p_so_sched->any_src[tag].cumulated_len >= p_so_sched->any_src[tag].expected_len){
          /* Completion of an ANY_SRC unpack */
          p_so_sched->any_src[tag].status = 0;
          p_so_sched->any_src[tag].gate_id = -1;

          p_so_sched->pending_any_src_unpacks--;
          interface->unpack_success(p_gate, tag, seq, tbx_true);
        }

        // 3) On a reçu directement les données à leur destination finale
      } else {
        NM_SO_TRACE("****Completion of a large message on tag %d, seq %d and len = %d\n", tag, seq, len);

        p_so_gate->recv[tag][seq].unpack_here.cumulated_len += len;

        /* Verify if the communication is done */
        if(p_so_gate->recv[tag][seq].unpack_here.cumulated_len >= p_so_gate->recv[tag][seq].unpack_here.expected_len){
          NM_SO_TRACE("Wow! All data chunks in!\n");

          p_so_gate->pending_unpacks--;

          /* Reset the status matrix*/
          p_so_gate->recv[tag][seq].pkt_here.header = NULL;
          p_so_gate->status[tag][seq] = 0;

          interface->unpack_success(p_gate, tag, seq, tbx_false);
        }
       }

    } else { //Multirail case
      if((p_so_sched->any_src[tag].status & NM_SO_STATUS_RDV_IN_PROGRESS)
         &&
         (p_so_sched->any_src[tag].gate_id == p_gate->id)
         &&
         (p_so_sched->any_src[tag].seq == seq)) {
        NM_SO_TRACE("Completion of a large ANY_SRC message with tag %d et seq %d\n", tag, seq);

        p_so_sched->any_src[tag].cumulated_len += len;
        if(p_so_sched->any_src[tag].cumulated_len >= p_so_sched->any_src[tag].expected_len){
          /* Completion of an ANY_SRC unpack */
          p_so_sched->any_src[tag].status = 0;
          p_so_sched->any_src[tag].gate_id = -1;

          p_so_sched->pending_any_src_unpacks--;
          interface->unpack_success(p_gate, tag, seq, tbx_true);
        }

      } else {
        NM_SO_TRACE("Completion of a large message - tag %d, seq %d\n", tag, seq);

        p_so_gate->recv[tag][seq].unpack_here.cumulated_len += len;
        /* Verify if the communication is done */
        if(p_so_gate->recv[tag][seq].unpack_here.cumulated_len >= p_so_gate->recv[tag][seq].unpack_here.expected_len){
          NM_SO_TRACE("Wow! All data chunks in!\n");

          p_so_gate->pending_unpacks--;

          /* Reset the status matrix*/
          p_so_gate->recv[tag][seq].pkt_here.header = NULL;
          p_so_gate->status[tag][seq] = 0;

          interface->unpack_success(p_gate, tag, seq, tbx_false);
        }
      }
    }

    p_so_gate->active_recv[drv_id][TRK_LARGE] = 0;

    /* Free the wrapper */
    nm_so_pw_free(p_so_pw);

    /*--------------------------------------------*/
    /* Check if some recv requests were postponed */
    if(!list_empty(&p_so_gate->pending_large_recv)) {
      union nm_so_generic_ctrl_header ctrl;
      uint8_t trk_id = TRK_LARGE;

      struct nm_so_pkt_wrap *p_so_large_pw = nm_l2so(p_so_gate->pending_large_recv.next);

      NM_SO_TRACE("Retrieve wrapper waiting for a reception\n");
      // 1) C'est un iov à finir de recevoir
      if(p_so_gate->status[p_so_large_pw->pw.proto_id-128][p_so_large_pw->pw.seq]
         & NM_SO_STATUS_UNPACK_IOV
         || p_so_sched->any_src[p_so_large_pw->pw.proto_id-128].status & NM_SO_STATUS_UNPACK_IOV){

        /* Post next iov entry on driver drv_id */
        list_del(p_so_gate->pending_large_recv.next);

        if(p_so_large_pw->v[0].iov_len < p_so_large_pw->pw.length){
          struct nm_so_pkt_wrap *p_so_large_pw2 = NULL;

          /* Only post the first entry */
          nm_so_pw_split(p_so_large_pw, &p_so_large_pw2, p_so_large_pw->v[0].iov_len);

          list_add_tail(&p_so_large_pw2->link, &p_so_gate->pending_large_recv);
        }

        nm_so_direct_post_large_recv(p_gate, drv_id, p_so_large_pw);

        /* Launch the ACK */
        nm_so_build_multi_ack(p_gate, p_so_large_pw->pw.proto_id, p_so_large_pw->pw.seq, p_so_large_pw->chunk_offset, 1, &drv_id, (uint32_t *)(&p_so_large_pw->v[0].iov_len));



      // 2) C'est un datatype à finir de recevoir
      } else if ((p_so_gate->status[p_so_large_pw->pw.proto_id-128][p_so_large_pw->pw.seq]
                  & NM_SO_STATUS_IS_DATATYPE
                  || p_so_sched->any_src[p_so_large_pw->pw.proto_id-128].status
                  & NM_SO_STATUS_IS_DATATYPE)
                 && p_so_large_pw->pw.segp) {
        /* Post next iov entry on driver drv_id */
        list_del(p_so_gate->pending_large_recv.next);

        init_large_datatype_recv_with_multi_ack(p_so_large_pw);

      // 3) C'est un wrapper normal dont on a reçu un RDV
      } else {
        list_del(p_so_gate->pending_large_recv.next);

        if(nb_drivers == 1){ // we do not search to have more NICs
          nm_so_direct_post_large_recv(p_gate, drv_id, p_so_large_pw);

          nm_so_init_ack(&ctrl, p_so_large_pw->pw.proto_id, p_so_large_pw->pw.seq,
                         drv_id * NM_SO_MAX_TRACKS + trk_id, p_so_large_pw->chunk_offset);
          err = cur_strat->pack_ctrl(p_gate, &ctrl);

        } else {
          int nb_drv;
          uint8_t drv_ids[RAIL_MAX];
          uint32_t chunk_lens[RAIL_MAX];

          /* We ask the current strategy to find other available tracks for
             transfering this large data chunk. */
          err = cur_strat->extended_rdv_accept(p_gate, p_so_large_pw->pw.length,
                                               &nb_drv, drv_ids, chunk_lens);

          if(err == NM_ESUCCESS){
            if(nb_drv == 1){ // le réseau qui vient de se libérer est le seul disponible
              nm_so_direct_post_large_recv(p_gate, drv_id, p_so_large_pw);

              nm_so_init_ack(&ctrl, p_so_large_pw->pw.proto_id, p_so_large_pw->pw.seq,
                             drv_id * NM_SO_MAX_TRACKS + trk_id, p_so_large_pw->chunk_offset);
              err = cur_strat->pack_ctrl(p_gate, &ctrl);

            } else {
              err = nm_so_post_multiple_pw_recv(p_gate, p_so_large_pw, nb_drv, drv_ids, chunk_lens);
              err = nm_so_build_multi_ack(p_gate,
                                          p_so_large_pw->pw.proto_id, p_so_large_pw->pw.seq,
                                          p_so_large_pw->chunk_offset,
                                          nb_drv, drv_ids, chunk_lens);
            }
          }
        }
      }
    }
  }

 out:
  /* Hum... Well... We're done guys! */
  err = NM_ESUCCESS;
  return err;
}

/** Process a failed incoming request.
 */
int
nm_so_in_process_failed_rq(struct nm_sched	*p_sched,
                           struct nm_pkt_wrap	*p_pw,
                           int		_err TBX_UNUSED) {
  TBX_FAILURE("nm_so_in_process_failed_rq");
  return nm_so_in_process_success_rq(p_sched, p_pw);
}
