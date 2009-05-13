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

#include "pm2_common.h" /* Here! */

void do_main_code(p_mad_madeleine_t madeleine);

int main(int argc, char *argv[]) {
  common_pre_init(&argc, argv, NULL); /* Here! */
  common_post_init(&argc, argv, NULL);/* Here! */

  do_main_code(mad_get_madeleine());
  common_exit(NULL);/* Here! */
  exit(EXIT_SUCCESS);/* Here! */
}

void do_main_code(p_mad_madeleine_t madeleine) {
  p_mad_channel_t channel = NULL;
  ntbx_process_lrank_t my_local_rank = -1;/* Here! */
  p_mad_connection_t out = NULL;
  p_mad_connection_t in = NULL;

  channel = tbx_htable_get(madeleine->channel_htable, "pm2");
  my_local_rank = ntbx_pc_global_to_local(channel->pc,/* Here! */
					  madeleine->session->process_rank);/* Here! */

  if (my_local_rank == 0) { /* Here! */
    char hostname[100];
    char data[50] = "";
    char *rdata = NULL;
    int rlen, len = 0;

    strcpy(data, "Hello World from ");   /* Prepare the data to be sent */
    gethostname(hostname, 100);
    strcat(data, hostname);
    len = strlen(data);

    out = mad_begin_packing(channel, 1); /* Open the out connection *//* Here! */
    mad_pack(out, &len, sizeof(len), mad_send_SAFER, mad_receive_EXPRESS);
    mad_pack(out, data, len, mad_send_CHEAPER, mad_receive_CHEAPER);
    mad_end_packing(out);                /* End of the sent *//* Here! */

    in = mad_begin_unpacking(channel);   /* Starts the reception *//* Here! */
    mad_unpack(in, &rlen, sizeof(rlen), mad_send_SAFER, mad_receive_EXPRESS);
    rdata = TBX_CALLOC(1, rlen);
    mad_unpack(in, rdata, rlen, mad_send_CHEAPER, mad_receive_CHEAPER);
    mad_end_unpacking(in);               /* Ends the reception. *//* Here! */

    if (strcmp(data, rdata) == 0)
      printf("success\n");
    else
      printf("failure. Strings differ: <%s> != <%s>\n", data, rdata);
  }
  else if (my_local_rank == 1) { /* Here! */
    int len = 0;
    char *data = NULL;

    in = mad_begin_unpacking(channel);   /* Starts the reception *//* Here! */
    mad_unpack(in, &len, sizeof(len), mad_send_SAFER, mad_receive_EXPRESS);
    data = TBX_CALLOC(1, len);
    mad_unpack(in, data, len, mad_send_CHEAPER, mad_receive_CHEAPER);
    mad_end_unpacking(in);               /* Ends the reception. *//* Here! */

    out = mad_begin_packing(channel, 0); /* Open the out connection *//* Here! */
    mad_pack(out, &len, sizeof(len), mad_send_SAFER, mad_receive_EXPRESS);
    mad_pack(out, data, len, mad_send_CHEAPER, mad_receive_CHEAPER);
    mad_end_packing(out);                /* End of the sent *//* Here! */
  }
}
