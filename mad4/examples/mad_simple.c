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

#include "mad_simple.h"

int main(int argc, char *argv[]) {
  /*
   * Initialisation des diverses bibliotheques.
   */
  common_pre_init(&argc, argv, NULL);
  common_post_init(&argc, argv, NULL);

  /*
   * Recuperation de l'objet Madeleine.
   */
  p_mad_madeleine_t madeleine = mad_get_madeleine();
  do_main_code(madeleine);
  common_exit(NULL);
  return 0;
}

void do_main_code(p_mad_madeleine_t madeleine) {
  p_mad_channel_t channel = NULL;          /* The communication channel */
  ntbx_process_lrank_t my_local_rank = -1; /* The 'local' rank of the process in the current channel */
  p_ntbx_process_container_t pc = NULL;    /* The set of processes in the current channel */
  ntbx_process_grank_t process_rank;       /* The globally unique process rank  */
  int i;

  /* Get a reference to the corresponding channel structure */
  channel = tbx_htable_get(madeleine->channel_htable, CHANNEL_NAME);
  /* If that fails, it means that our process does not belong to the channel */
  if (!channel) {
    DISP("I don't belong to this channel");
    return;
  }

  /* Get the set of processes in the channel */
  pc = channel->pc;

  if (pc->count < 2) {
    LDISP("Number of processes should be at least 2");
    return;
  }

  /* Convert the globally unique process rank to its _local_ rank in the current channel */
  process_rank  = madeleine->session->process_rank;
  my_local_rank = ntbx_pc_global_to_local(pc, process_rank);

  if (my_local_rank == 0) {
    /* i am the client */
     for(i=1 ; i<=PINGPONG ; i++) {
      senddata(channel, 1, i);

      int recu = -1;
      receivedata(channel, &recu);

      LDISP("client %d has send %d and received %d", i, i, recu);
     } // end for
  } // end if
  else {
    /* i am a server */
    for(i=1 ; i<=PINGPONG ; i++) {

      /* receive data from the preceeding process */
      int recu;
      receivedata(channel, &recu);
      LDISP("server %d has received %d", i, recu);
      fflush(stdout);

      senddata(channel, 0, recu);
    } // end for
  } // end else
}

int senddata(p_mad_channel_t channel, ntbx_process_lrank_t dest, int i) {
  ntbx_pack_buffer_t buffer MAD_ALIGNED;
  p_mad_connection_t out = NULL;

  /* Take the out connection */
  out = mad_begin_packing(channel, dest);
  if (!out) FAILURE("invalid state");

  /* Send the data */
  ntbx_pack_int(i, &buffer);
  mad_pack(out, &buffer, sizeof(buffer), mad_send_SAFER, mad_receive_EXPRESS);

  /* End of the sent */
  mad_end_packing(out);

  return i;
}

void receivedata(p_mad_channel_t channel, int *data) {
  ntbx_pack_buffer_t buffer MAD_ALIGNED;
  p_mad_connection_t in = NULL;

  /* Starts the reception */
  in = mad_begin_unpacking(channel);

  /* Get the data */
  mad_unpack(in, &buffer, sizeof(buffer), mad_send_SAFER, mad_receive_EXPRESS);
  *data = ntbx_unpack_int(&buffer);

  /* Ends the reception. Flush any pending receive_CHEAPER */
  mad_end_unpacking(in);

}
