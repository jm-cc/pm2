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
 * mad_ping.c
 * ==========
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "pm2_common.h"

#define NB_LOOPS 2
#define BUFFER_LENGTH_MIN  4
#define BUFFER_LENGTH_MAX  128 //(2*1024*1024) //32768

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
    unsigned int i = 0;
    char *buffer   = NULL;
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
        //DISP("Data OK");
    } else {
        DISP("DATA FAILED");
    }

    LOG_OUT();
}


void
client(p_mad_channel_t channel){
    p_mad_connection_t connection1 = NULL;
    p_mad_connection_t connection2 = NULL;

    char *buffer_e = NULL;
    char *buffer_r = NULL;

    unsigned int counter    = 0;
    unsigned int cur_length = BUFFER_LENGTH_MIN;

    tbx_tick_t        t1;
    tbx_tick_t        t2;
    double            sum = 0.0;

    LOG_IN();

    buffer_e  = init_and_fill_data(BUFFER_LENGTH_MAX);
    buffer_r  = init_data(BUFFER_LENGTH_MAX);


    while(cur_length <= BUFFER_LENGTH_MAX) {
        connection1 = mad_begin_packing(channel, 0);
        mad_pack(connection1,
                 buffer_e,
                 cur_length,
                 mad_send_CHEAPER,
                 mad_receive_CHEAPER);

        mad_wait_packs(connection1);

        connection2 = mad_begin_unpacking(channel);
        mad_unpack(connection2,
                   buffer_r,
                   cur_length,
                   mad_send_CHEAPER,
                   mad_receive_CHEAPER);

        mad_wait_unpacks(connection2);

        // ------------------------------

        TBX_GET_TICK(t1);
        while (counter < NB_LOOPS) {
            mad_pack(connection1,
                     buffer_e,
                     cur_length,
                     mad_send_CHEAPER,
                     mad_receive_CHEAPER);

            mad_wait_packs(connection1);

            mad_unpack(connection2,
                       buffer_r,
                       cur_length,
                       mad_send_CHEAPER,
                       mad_receive_CHEAPER);

            mad_wait_unpacks(connection2);

            counter++;
        }
        mad_end_packing(connection1);
        mad_end_unpacking(connection2);

        TBX_GET_TICK(t2);
        sum = TBX_TIMING_DELAY(t1, t2);

        printf("latence : %f \n", sum / (NB_LOOPS * 2));
        printf("debit :   %f \n", (2.0 * NB_LOOPS * cur_length) / sum / 1.048576);


        //DISP("%9d   %g   %g",
        //     cur_length,
        //     sum / (NB_LOOPS * 2),
        //     (2.0 * NB_LOOPS * cur_length) / sum / 1.048576);

        // next length
        cur_length*=2;
        counter = 0;
        connection1 = NULL;
        connection2 = NULL;
    }

    TBX_FREE(buffer_e);
    TBX_FREE(buffer_r);

    LOG_OUT();
}

void
server(p_mad_channel_t channel){
    p_mad_connection_t connection1 = NULL;
    p_mad_connection_t connection2 = NULL;

    char *buffer_e = NULL;
    char *buffer_r = NULL;

    unsigned int counter    = 0;
    unsigned int cur_length = BUFFER_LENGTH_MIN;

    LOG_IN();
    buffer_e  = init_and_fill_data(BUFFER_LENGTH_MAX);
    buffer_r  = init_data(BUFFER_LENGTH_MAX);


    while(cur_length <= BUFFER_LENGTH_MAX) {

        connection1 = mad_begin_unpacking(channel);
        mad_unpack(connection1,
                   buffer_r,
                   cur_length,
                   mad_send_CHEAPER,
                   mad_receive_CHEAPER);

        mad_wait_unpacks(connection1);


        connection2 = mad_begin_packing(channel, 1);
        mad_pack(connection2,
                 buffer_e,
                     cur_length,
                     mad_send_CHEAPER,
                     mad_receive_CHEAPER);

        mad_wait_packs(connection2);

        // ------------------------------

        mad_unpack(connection1,
                   buffer_r,
                   cur_length,
                   mad_send_CHEAPER,
                   mad_receive_CHEAPER);

        mad_wait_unpacks(connection1);

        while (counter < NB_LOOPS - 1) {
            mad_pack(connection2,
                     buffer_e,
                     cur_length,
                     mad_send_CHEAPER,
                     mad_receive_CHEAPER);

            mad_wait_packs(connection2);

            mad_unpack(connection1,
                       buffer_r,
                       cur_length,
                       mad_send_CHEAPER,
                       mad_receive_CHEAPER);

            mad_wait_unpacks(connection1);

            counter++;
        }

        mad_pack(connection2,
                 buffer_e,
                 cur_length,
                 mad_send_CHEAPER,
                 mad_receive_CHEAPER);

        mad_end_unpacking(connection1);
        mad_end_packing(connection2);

        // next length
        cur_length*=2;
        counter = 0;
        connection1 = NULL;
        connection2 = NULL;
    }

    TBX_FREE(buffer_e);
    TBX_FREE(buffer_r);

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

    } else if (my_local_rank == 0) {
        DISP("SERVER");
        server(channel);
    }
    common_exit(NULL);
    LOG_OUT();
    return 0;
}

