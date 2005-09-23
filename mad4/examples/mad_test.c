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

/*
 * mad_micro.c
 * ===========
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "pm2_common.h"

#define NB_LOOPS 1000
#define BUFFER_LENGTH_MIN  4
#define BUFFER_LENGTH_MAX  32000 //(2*1024*1024) //32768


char *
init_data(unsigned int length){
    char *buffer = NULL;
    LOG_IN();
    buffer = TBX_MALLOC(length);
    LOG_OUT();
    return buffer;
}


char *
init_and_fill_data(unsigned int length){
    unsigned int i         = 0;
    char *buffer = NULL;
    LOG_IN();

    buffer = TBX_MALLOC(length);
    for(i = 0; i < length-1; i++){
        buffer[i] = 'a' + (i % 23);
    }

    buffer[i] = '\0';
    LOG_OUT();
    return buffer;
}




p_mad_iovec_t
initialize_and_fill_mad_iovec(int my_rank,
                              int remote_rank,
                              int sequence,
                              p_mad_channel_t channel,
                              int length,
                              void *data){
    p_mad_adapter_t adapter = NULL;
    p_tbx_htable_t tracks_htable = NULL;
    p_mad_track_t track = NULL;

    p_mad_iovec_t mad_iovec = NULL;
    LOG_IN();

    mad_iovec = mad_iovec_create(sequence);
    mad_iovec->remote_rank = remote_rank;
    mad_iovec->channel = channel;

    mad_iovec->send_mode = mad_send_CHEAPER;
    mad_iovec->receive_mode = mad_receive_CHEAPER;

    mad_iovec_begin_struct_iovec(mad_iovec,
                                 my_rank);
    mad_iovec_add_data_header(mad_iovec, 1,
                              channel->id,
                              sequence,
                              length);
    mad_iovec_add_data(mad_iovec, data, length, 3);


    adapter = channel->adapter;
    tracks_htable = adapter->s_track_set->tracks_htable;
    track = tbx_htable_get(tracks_htable, "rdv");
    mad_iovec->track = track;

    return mad_iovec;
}


p_mad_iovec_t
initialize_mad_iovec(int my_rank,
                     int remote_rank,
                     int sequence,
                     p_mad_channel_t channel,
                     int length,
                     void *data){
    p_mad_adapter_t adapter = NULL;
    p_tbx_htable_t tracks_htable = NULL;
    p_mad_track_t track = NULL;

    p_mad_iovec_t mad_iovec = NULL;
    LOG_IN();

    mad_iovec = mad_iovec_create(sequence);
    mad_iovec->remote_rank = remote_rank;
    mad_iovec->channel = channel;

    mad_iovec->send_mode = mad_send_CHEAPER;
    mad_iovec->receive_mode = mad_receive_CHEAPER;

    mad_iovec_begin_struct_iovec(mad_iovec,
                                 my_rank);
    mad_iovec_add_data_header(mad_iovec, 1,
                              channel->id,
                              sequence,
                              length);
    mad_iovec_add_data(mad_iovec, data, length, 3);


    adapter = channel->adapter;
    tracks_htable = adapter->s_track_set->tracks_htable;
    track = tbx_htable_get(tracks_htable, "rdv");
    mad_iovec->track = track;

    return mad_iovec;
}


void
update_mad_iovec(p_mad_iovec_t mad_iovec,
                 int length){
    mad_iovec->length = length;
}


void
mad_pack2(p_mad_connection_t cnx, p_mad_iovec_t mad_iovec){
    p_mad_driver_interface_t interface = NULL;

    LOG_IN();
    interface = cnx->channel->adapter->driver->interface;

    mad_pipeline_add(mad_iovec->track->pipeline, mad_iovec);
    interface->isend(mad_iovec->track,
                     mad_iovec->remote_rank,
                     mad_iovec->data,
                     mad_iovec->total_nb_seg);
    //DISP("ISEND");
    LOG_OUT();
}


void
mad_unpack2(p_mad_connection_t cnx, p_mad_iovec_t mad_iovec){
    p_mad_driver_interface_t interface = NULL;

    LOG_IN();
    interface = cnx->channel->adapter->driver->interface;

    mad_pipeline_add(mad_iovec->track->pipeline, mad_iovec);
    interface->irecv(mad_iovec->track,
                     mad_iovec->data,
                     mad_iovec->total_nb_seg);
    //DISP("IRECV");
    LOG_OUT();
}


void
mad_wait(p_mad_adapter_t adapter, p_mad_track_t track){
    mad_mkp_status_t status = MAD_MKP_NO_PROGRESS;
    LOG_IN();

    while(status != MAD_MKP_PROGRESS){
        //DISP_VAL("status", status);
        status = mad_make_progress(adapter, track);
    }
    mad_pipeline_remove(track->pipeline);
    LOG_OUT();
}



void
client(p_mad_channel_t channel){
    p_mad_adapter_t adapter = NULL;
    p_mad_connection_t connection1 = NULL;
    p_mad_connection_t connection2 = NULL;

    p_mad_iovec_t mad_iovec_e = NULL;
    p_mad_iovec_t mad_iovec_r = NULL;

    unsigned int counter    = 0;
    unsigned int cur_length = BUFFER_LENGTH_MIN;

    tbx_tick_t        t1;
    tbx_tick_t        t2;
    double            sum = 0.0;

    char *data_e = NULL;

    LOG_IN();
    adapter = channel->adapter;

    data_e = init_and_fill_data(BUFFER_LENGTH_MAX);

    mad_iovec_e = initialize_and_fill_mad_iovec(1,
                                                0,
                                                87,
                                                channel,
                                                cur_length,
                                                data_e);
    TBX_GET_TICK(t1);
    connection1 = mad_begin_packing(channel, 0);

    while(counter < NB_LOOPS){
        mad_pack2(connection1, mad_iovec_e);
        mad_wait(adapter, mad_iovec_e->track);
        counter++;
    }

    TBX_GET_TICK(t2);

    connection1->lock = tbx_false;

    sum = TBX_TIMING_DELAY(t1, t2);

    printf("%9d   %9g  \n",
           cur_length, sum / NB_LOOPS);

    mad_iovec_free(mad_iovec_e, tbx_false);
    TBX_FREE(data_e);

    LOG_OUT();
}

void
server(p_mad_channel_t channel){
    p_mad_adapter_t adapter = NULL;
    p_mad_connection_t connection1 = NULL;
    p_mad_connection_t connection2 = NULL;

    p_mad_iovec_t mad_iovec_e = NULL;
    p_mad_iovec_t mad_iovec_r = NULL;

    unsigned int counter    = 0;
    unsigned int cur_length = BUFFER_LENGTH_MIN;

    char *data_r = NULL;

    LOG_IN();
    adapter = channel->adapter;

    data_r = init_data(BUFFER_LENGTH_MAX);
    mad_iovec_r = initialize_mad_iovec(0,
                                       1,
                                       87,
                                       channel,
                                       cur_length,
                                       data_r);

    connection1 = mad_begin_unpacking(channel);

    while(counter < NB_LOOPS){
        mad_unpack2(connection1, mad_iovec_r);
        mad_wait(adapter, mad_iovec_r->track);
        counter++;
    }

    channel->reception_lock = tbx_false;

    mad_iovec_free(mad_iovec_r, tbx_false);
    TBX_FREE(data_r);

    LOG_OUT();
}


int
main(int argc, char **argv) {
    p_mad_madeleine_t         	 madeleine         = NULL;
    p_mad_channel_t           	 channel           = NULL;
    p_ntbx_process_container_t	 pc                = NULL;
    ntbx_process_grank_t      	 my_global_rank    =   -1;
    ntbx_process_lrank_t      	 my_local_rank     =   -1;

    LOG_IN();
    common_pre_init (&argc, argv, NULL);
    common_post_init(&argc, argv, NULL);
    madeleine      = mad_get_madeleine();

    my_global_rank = madeleine->session->process_rank;
    channel = tbx_htable_get(madeleine->channel_htable, "test");
    if(channel == NULL)
        FAILURE("main : channel not found");

    pc             = channel->pc;
    my_local_rank  = ntbx_pc_global_to_local(pc, my_global_rank);

    if (my_local_rank == 1) {
        DISP("CLIENT");
        client(channel);
        DISP("FINI!!!");
    } else if (my_local_rank == 0) {
        DISP("SERVER");
        server(channel);
        DISP("FINI!!!");
    }
    common_exit(NULL);
    LOG_OUT();
    return 0;
}








///*
// * PM2: Parallel Multithreaded Machine
// * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
// *
// * This program is free software; you can redistribute it and/or modify
// * it under the terms of the GNU General Public License as published by
// * the Free Software Foundation; either version 2 of the License, or (at
// * your option) any later version.
// *
// * This program is distributed in the hope that it will be useful, but
// * WITHOUT ANY WARRANTY; without even the implied warranty of
// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// * General Public License for more details.
// */
//
///*
// * mad_test.c
// * ==========
// */
//
//#include <stdlib.h>
//#include <stdio.h>
//#include <string.h>
//#include <fcntl.h>
//#include <unistd.h>
//#include "pm2_common.h"
//
//// #include "madeleine.h"
//
//// Macros
////......................
//
//#ifdef MAX
//#undef MAX
//#endif
//
//#define MAX 16
//
//// Static variables
////......................
//
//static ntbx_process_grank_t process_rank = -1;
//static const char *const    token        = "token__";
//
//// Functions
////......................
//
//static
//tbx_bool_t
//play_with_channel(p_mad_madeleine_t  madeleine,
//		  char              *name)
//{
//  p_mad_channel_t            channel        = NULL;
//  p_ntbx_process_container_t pc             = NULL;
//  ntbx_process_lrank_t       my_local_rank  =   -1;
//  ntbx_process_lrank_t       its_local_rank =   -1;
//  tbx_bool_t                 status         = tbx_false;
//  ntbx_pack_buffer_t         buffer MAD_ALIGNED;
//
//  channel = tbx_htable_get(madeleine->channel_htable, name);
//  if (!channel)
//    {
//      return tbx_true;
//    }
//
//  pc = channel->pc;
//
//  my_local_rank = ntbx_pc_global_to_local(pc, process_rank);
//
//  if (my_local_rank)
//    {
//      p_tbx_string_t      string  = NULL;
//      p_mad_connection_t  in      = NULL;
//      p_mad_connection_t  out     = NULL;
//      int                 len     =    0;
//      int                 dyn_len =    0;
//      char               *dyn_buf = NULL;
//      char                buf[MAX] MAD_ALIGNED;
//
//      memset(buf, 0, MAX);
//
//#ifdef MAD_MESSAGE_POLLING
//      while (!(in = mad_begin_unpacking(channel)));
//#else // MAD_MESSAGE_POLLING
//      in = mad_begin_unpacking(channel);
//#endif // MAD_MESSAGE_POLLING
//
//      mad_unpack(in, &buffer, sizeof(buffer),
//		 mad_send_SAFER, mad_receive_EXPRESS);
//
//      len = ntbx_unpack_int(&buffer);
//      if ((len < 1) || (len > MAX))
//	FAILURE("invalid message length");
//
//      mad_unpack(in, buf, len, mad_send_CHEAPER, mad_receive_CHEAPER);
//      mad_unpack(in, &buffer, sizeof(buffer),
//		 mad_send_SAFER, mad_receive_EXPRESS);
//      dyn_len = ntbx_unpack_int(&buffer);
//      if (dyn_len < 1)
//	FAILURE("invalid message length");
//      dyn_buf = TBX_CALLOC(1, dyn_len);
//
//      mad_unpack(in, dyn_buf, dyn_len, mad_send_CHEAPER, mad_receive_CHEAPER);
//      mad_end_unpacking(in);
//
//      if (tbx_streq(buf, token))
//	status = tbx_true;
//
//      TBX_FREE(dyn_buf);
//      dyn_buf = NULL;
//
//      string  = tbx_string_init_to_cstring("the sender is ");
//      tbx_string_append_int(string, my_local_rank);
//      dyn_buf = tbx_string_to_cstring(string);
//      tbx_string_free(string);
//      string  = NULL;
//
//      its_local_rank = my_local_rank;
//
//      do
//	{
//	  if (ntbx_pc_next_local_rank(pc, &its_local_rank))
//	    {
//	      out = mad_begin_packing(channel, its_local_rank);
//	    }
//	  else
//	    {
//	      goto last;
//	    }
//	}
//      while (!out);
//
//      ntbx_pack_int(len, &buffer);
//      mad_pack(out, &buffer, sizeof(buffer),
//	       mad_send_SAFER, mad_receive_EXPRESS);
//      mad_pack(out, buf, len, mad_send_CHEAPER, mad_receive_CHEAPER);
//
//      dyn_len = strlen(dyn_buf) + 1;
//      ntbx_pack_int(dyn_len, &buffer);
//      mad_pack(out, &buffer, sizeof(buffer),
//	       mad_send_SAFER, mad_receive_EXPRESS);
//      mad_pack(out, dyn_buf, dyn_len, mad_send_CHEAPER, mad_receive_CHEAPER);
//      mad_end_packing(out);
//      TBX_FREE(dyn_buf);
//      dyn_buf = NULL;
//      return status;
//
//    last:
//      ntbx_pc_first_local_rank(pc, &its_local_rank);
//      out = mad_begin_packing(channel, its_local_rank);
//      while (!out);
//      {
//	if (!ntbx_pc_next_local_rank(pc, &its_local_rank))
//	  FAILURE("inconsistent error");
//      }
//
//      ntbx_pack_int(len, &buffer);
//      mad_pack(out, &buffer, sizeof(buffer),
//	       mad_send_SAFER, mad_receive_EXPRESS);
//      mad_pack(out, buf, len, mad_send_CHEAPER, mad_receive_CHEAPER);
//      dyn_len = strlen(dyn_buf) + 1;
//      ntbx_pack_int(dyn_len, &buffer);
//      mad_pack(out, &buffer, sizeof(buffer),
//	       mad_send_SAFER, mad_receive_EXPRESS);
//      mad_pack(out, dyn_buf, dyn_len, mad_send_CHEAPER, mad_receive_CHEAPER);
//      mad_end_packing(out);
//      TBX_FREE(dyn_buf);
//      dyn_buf = NULL;
//      return status;
//    }
//  else
//    {
//      p_tbx_string_t      string  = NULL;
//      p_mad_connection_t  in      = NULL;
//      p_mad_connection_t  out     = NULL;
//      char               *msg     = NULL;
//      int                 len     =    0;
//      int                 dyn_len =    0;
//      char               *dyn_buf = NULL;
//      tbx_bool_t          status  = tbx_false;
//      char                buf[MAX] MAD_ALIGNED;
//
//      msg = tbx_strdup(token);
//
//      its_local_rank = my_local_rank;
//      string  = tbx_string_init_to_cstring("the sender is ");
//      tbx_string_append_int(string, my_local_rank);
//      dyn_buf = tbx_string_to_cstring(string);
//      tbx_string_free(string);
//      string  = NULL;
//
//      do
//	{
//	  if (ntbx_pc_next_local_rank(pc, &its_local_rank))
//	    {
//	      out = mad_begin_packing(channel, its_local_rank);
//	    }
//	  else
//	    return tbx_true;
//	}
//      while (!out);
//
//      len = strlen(msg) + 1;
//      ntbx_pack_int(len, &buffer);
//      mad_pack(out, &buffer, sizeof(buffer),
//	       mad_send_SAFER, mad_receive_EXPRESS);
//      mad_pack(out, msg, len, mad_send_CHEAPER, mad_receive_CHEAPER);
//      dyn_len = strlen(dyn_buf) + 1;
//      ntbx_pack_int(dyn_len, &buffer);
//      mad_pack(out, &buffer, sizeof(buffer),
//	       mad_send_SAFER, mad_receive_EXPRESS);
//      mad_pack(out, dyn_buf, dyn_len, mad_send_CHEAPER, mad_receive_CHEAPER);
//      mad_end_packing(out);
//
//      TBX_FREE(msg);
//      msg = NULL;
//
//      TBX_FREE(dyn_buf);
//      dyn_buf = NULL;
//      memset(buf, 0, MAX);
//
//#ifdef MAD_MESSAGE_POLLING
//      while (!(in = mad_begin_unpacking(channel)));
//#else // MAD_MESSAGE_POLLING
//      in = mad_begin_unpacking(channel);
//#endif // MAD_MESSAGE_POLLING
//
//      mad_unpack(in, &buffer, sizeof(buffer),
//		 mad_send_SAFER, mad_receive_EXPRESS);
//
//      len = ntbx_unpack_int(&buffer);
//      if ((len < 1) || (len > MAX))
//	FAILURE("invalid message length");
//
//      mad_unpack(in, buf, len, mad_send_CHEAPER, mad_receive_CHEAPER);
//      mad_unpack(in, &buffer, sizeof(buffer),
//		 mad_send_SAFER, mad_receive_EXPRESS);
//      dyn_len = ntbx_unpack_int(&buffer);
//      if (dyn_len < 1)
//	FAILURE("invalid message length");
//      dyn_buf = TBX_CALLOC(1, dyn_len);
//
//      mad_unpack(in, dyn_buf, dyn_len, mad_send_CHEAPER, mad_receive_CHEAPER);
//      mad_end_unpacking(in);
//
//      if (tbx_streq(buf, token))
//	status = tbx_true;
//
//      TBX_FREE(dyn_buf);
//      dyn_buf = NULL;
//
//      return status;
//    }
//}
//
//int
//main(int    argc,
//     char **argv)
//{
//  p_mad_madeleine_t madeleine = NULL;
//  p_mad_session_t   session   = NULL;
//  p_tbx_slist_t     slist     = NULL;
//  tbx_bool_t        status    = tbx_true;
//
//  common_pre_init(&argc, argv, NULL);
//  common_post_init(&argc, argv, NULL);
//
//  madeleine    = mad_get_madeleine();
//  session      = madeleine->session;
//  process_rank = session->process_rank;
//  slist = madeleine->public_channel_slist;
//
//  if (!tbx_slist_is_nil(slist))
//    {
//      tbx_slist_ref_to_head(slist);
//      do
//	{
//	  char *name = NULL;
//
//	  name = tbx_slist_ref_get(slist);
//	  status &= play_with_channel(madeleine, name);
//
//	  mad_leonie_barrier();
//	}
//      while (tbx_slist_ref_forward(slist));
//    }
//
//  if (status)
//    {
//      LDISP("success");
//    }
//  else
//    {
//      LDISP("failure");
//    }
//
//  common_exit(NULL);
//
//  return 0;
//}
