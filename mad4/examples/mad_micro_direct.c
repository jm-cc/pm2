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
 * mad_micro_direct.c
 * ==================
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "pm2_common.h"


tbx_tick_t wait_pack_tick;
tbx_tick_t wait_unpack_tick;

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


void
verify_data(char *user_buffer, unsigned int length){
    unsigned int i         = 0;
    tbx_bool_t   one_false = tbx_false;

    LOG_IN();
    for(i = 0; i < length-1; i++){
        if(user_buffer[i] != 'a' + (i % 23)){
            one_false = tbx_true;
            //DISP_VAL("data failed : index", i);
        }
    }
    if(user_buffer[i]){
        one_false = tbx_true;
        //DISP_VAL("data failed : index", i);
    }
    if(one_false == tbx_false){
        DISP("Data OK");
        //DISP_STR("Data OK -->", user_buffer);
    } else {
        DISP("DATA FAILED");
        //DISP_STR("DATA FAILED--->", user_buffer);
    }

    LOG_OUT();
}


int
main(int argc, char **argv) {
    p_mad_madeleine_t         	 madeleine         = NULL;
    char                       	*name		   = NULL;
    p_mad_channel_t           	 channel           = NULL;
    p_ntbx_process_container_t	 pc                = NULL;
    ntbx_process_grank_t      	 my_global_rank    =   -1;
    ntbx_process_lrank_t      	 my_local_rank     =   -1;
    p_mad_connection_t connection = NULL;

    extern tbx_tick_t tick_mx_test;
    tbx_tick_t tick_wait_pack;
    tbx_tick_t tick_wait_unpack;
    LOG_IN();
    common_pre_init (&argc, argv, NULL);
    common_post_init(&argc, argv, NULL);

    madeleine      = mad_get_madeleine();
    my_global_rank = madeleine->session->process_rank;
    name	= tbx_slist_index_get(madeleine->public_channel_slist, 0);
    channel  = tbx_htable_get(madeleine->channel_htable, name);
    pc             = channel->pc;
    my_local_rank  = ntbx_pc_global_to_local(pc, my_global_rank);

    if (my_local_rank == 0) {
        unsigned int len1, len2, len3, len4, len5;
        char *buffer1, *buffer2, *buffer3, *buffer4, *buffer5;

        DISP_VAL("Je suis l'émetteur ----------> ", my_local_rank);
        len1 = 6;
        buffer1 = init_and_fill_data(len1);

        len2 = 128;
        buffer2 = init_and_fill_data(len2);

        len3 = 1048576;
        buffer3 = init_and_fill_data(len3);

        len4 = 4;
        buffer4 = init_and_fill_data(len4);

        len5 = 1048576;
        buffer5 = init_and_fill_data(len5);


        //verify_data(buffer1, len1);
        //verify_data(buffer2, len2);
        //verify_data(buffer3, len3);
        //verify_data(buffer4, len4);

        connection = mad_begin_packing(channel, 1);
        if(!connection)
            DISP("mad_micro_direct: PAS de CONNECTION");

        mad_pack(connection,
                 buffer1,
                 len1,
                 mad_send_CHEAPER,
                 mad_receive_CHEAPER);


        mad_wait_packs(connection);
        TBX_GET_TICK(tick_wait_pack);


        //
        ////DISP("################################");
        //mad_wait_packs(connection);
        //
        //mad_pack(connection,
        //         buffer2,
        //         len2,
        //         mad_send_CHEAPER,
        //         mad_receive_CHEAPER);
        //
        ////DISP("################################");
        //
        //mad_pack(connection,
        //         buffer3,
        //         len3,
        //         mad_send_CHEAPER,
        //         mad_receive_CHEAPER);
        //
        ////DISP("################################");
        //
        //mad_pack(connection,
        //         buffer4,
        //         len4,
        //         mad_send_CHEAPER,
        //         mad_receive_CHEAPER);

        //DISP("################################");

        //mad_pack(connection,
        //         buffer5,
        //         len5,
        //         mad_send_CHEAPER,
        //         mad_receive_CHEAPER);

        //DISP("################################");

        mad_end_packing(connection);

        TBX_FREE(buffer1);
        TBX_FREE(buffer2);
        TBX_FREE(buffer3);
        TBX_FREE(buffer4);
        TBX_FREE(buffer5);

        DISP_VAL("test/wait_pack --->",
                 TBX_TIMING_DELAY(tick_mx_test, tick_wait_pack));

    } else if (my_local_rank == 1) {
        unsigned int len1, len2, len3, len4, len5;
        char *buffer1, *buffer2, *buffer3, *buffer4, *buffer5;

        DISP_VAL("Je suis le récepteur -----> ", my_local_rank);
        len1 = 6;
        buffer1 = init_data(len1);

        len2 = 128;
        buffer2 = init_data(len2);

        len3 = 1048576;
        buffer3 = init_data(len3);

        len4 = 4;
        buffer4 = init_data(len4);

        len5 = 1048576;
        buffer5 = init_data(len5);

        connection = mad_begin_unpacking(channel);

        mad_unpack(connection,
                   buffer1,
                   len1,
                   mad_send_CHEAPER,
                   mad_receive_CHEAPER);

        mad_wait_unpacks(connection);

        TBX_GET_TICK(tick_wait_pack);

        //
        ////DISP("################################");
        //mad_wait_unpacks(connection);
        //
        //mad_unpack(connection,
        //           buffer2,
        //           len2,
        //           mad_send_CHEAPER,
        //           mad_receive_CHEAPER);

        //DISP("################################");

        //sleep(2);
        //mad_unpack(connection,
        //           buffer3,
        //           len3,
        //           mad_send_CHEAPER,
        //           mad_receive_CHEAPER);

        //DISP("################################");

        //mad_unpack(connection,
        //           buffer4,
        //           len4,
        //           mad_send_CHEAPER,
        //           mad_receive_CHEAPER);

        //DISP("################################");

        //mad_unpack(connection,
        //           buffer5,
        //           len5,
        //           mad_send_CHEAPER,
        //           mad_receive_CHEAPER);




        //DISP("################################");

        mad_end_unpacking(connection);

        verify_data(buffer1, len1);
        //verify_data(buffer2, len2);
        //verify_data(buffer3, len3);
        //verify_data(buffer4, len4);
        //verify_data(buffer5, len5);

        TBX_FREE(buffer1);
        TBX_FREE(buffer2);
        TBX_FREE(buffer3);
        TBX_FREE(buffer4);
        TBX_FREE(buffer5);


        DISP_VAL("test/wait_unpack --->",
                 TBX_TIMING_DELAY(tick_mx_test, tick_wait_unpack));

    }
    common_exit(NULL);
    LOG_OUT();
    return 0;
}
