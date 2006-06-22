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
 * mad_mini.c
 * ===========
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "pm2_common.h"

#define NB_LOOPS 10000
#define BUFFER_LENGTH_MIN  4
#define BUFFER_LENGTH_MAX  32000 //(2*1024*1024) //32768


//#define CHRONO

#ifdef CHRONO
int    nb_chronos_client    = 0;
double chrono_client_3_4    = 0.0;
double chrono_client_4_5    = 0.0;
double chrono_client_5_6    = 0.0;
double chrono_client_6_7    = 0.0;



int    nb_chronos_pack2 = 0;
double chrono_pack2_1_2 = 0.0;
double chrono_pack2_2_3 = 0.0;
double chrono_pack2_1_3 = 0.0;


int    nb_chronos_wait_pack = 0;
double chrono_wait_pack_1_2 = 0.0;
double chrono_wait_pack_2_3 = 0.0;
double chrono_wait_pack_1_3 = 0.0;


int    nb_chronos_unpack2 = 0;
double chrono_unpack2_1_2 = 0.0;
double chrono_unpack2_2_3 = 0.0;
double chrono_unpack2_1_3 = 0.0;


int    nb_chronos_wait_unpack = 0;
double chrono_wait_unpack_1_2 = 0.0;
double chrono_wait_unpack_2_3 = 0.0;
double chrono_wait_unpack_1_3 = 0.0;
#endif // CHRONO

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
    unsigned int i  = 0;
    char *buffer = NULL;
    LOG_IN();

    buffer = TBX_MALLOC(length);
    for(i = 0; i < length; i++){
        buffer[i] = 'a' + (i % 23);
    }

    LOG_OUT();
    return buffer;
}

void
verify_data(char *user_buffer, unsigned int length){
    unsigned int i         = 0;
    tbx_bool_t   one_false = tbx_false;

    LOG_IN();
    for(i = 0; i < length; i++){
        if(user_buffer[i] != 'a' + (i % 23)){
            one_false = tbx_true;
            printf("data failed : %c - index = %d\n", user_buffer[i], i);
        }
    }
    if(one_false == tbx_false){
        DISP("Data OK");
    } else {
        DISP("DATA FAILED");
    }

    LOG_OUT();
}

void
client(p_mad_channel_t channel){
    p_mad_adapter_t adapter = NULL;
    unsigned int counter = 0;
    unsigned int cur_length = BUFFER_LENGTH_MIN;

    p_mad_track_set_t s_track_set = NULL;
    p_mad_track_set_t r_track_set = NULL;
    p_mad_track_t s_track = NULL;
    p_mad_track_t r_track = NULL;

    char *data_e = NULL;
    char *data_r = NULL;

    p_mad_iovec_t iovec_e = NULL;
    p_mad_iovec_t iovec_r = NULL;
    p_mad_iovec_t tmp = NULL;

    tbx_tick_t        t1;
    tbx_tick_t        t2;
    double            sum = 0.0;

#ifdef CHRONO
    tbx_tick_t        t3;
    tbx_tick_t        t4;
    tbx_tick_t        t5;
    tbx_tick_t        t6;
    tbx_tick_t        t7;
#endif //CHRONO

    LOG_IN();
    adapter = channel->adapter;

    s_track_set = adapter->s_track_set;
    s_track     = s_track_set->rdv_track;

    r_track_set = adapter->r_track_set;
    r_track     = r_track_set->rdv_track;

    data_e = init_and_fill_data(BUFFER_LENGTH_MAX);
    iovec_e = mad_iovec_create(0, //dest
                               NULL, 0, tbx_false, 0, 0);
    mad_iovec_add_data_at_index(iovec_e, data_e, cur_length, 0);
    iovec_e->track = s_track;


    data_r = init_data(BUFFER_LENGTH_MAX);
    iovec_r = mad_iovec_create(0, //dest
                               NULL, 0, tbx_false, 0, 0);
    mad_iovec_add_data_at_index(iovec_r, data_r, cur_length, 0);
    iovec_r->track = r_track;


    while(cur_length <= BUFFER_LENGTH_MAX) {

        mad_mx_isend(s_track, iovec_e);
        while(!mad_mx_s_test(s_track_set))
            ;

        mad_mx_irecv(r_track, iovec_r);
        do{
            tmp = mad_mx_r_test(r_track);
        }while(!tmp);

        //DISP("------------------------------");

        TBX_GET_TICK(t1);
        while (counter < NB_LOOPS) {

#ifdef CHRONO
            TBX_GET_TICK(t3);
            mad_mx_isend(s_track, iovec_e);
            TBX_GET_TICK(t4);
            while(!mad_mx_s_test(s_track_set))
                ;
            TBX_GET_TICK(t5);
            mad_mx_irecv(r_track, iovec_r);
            TBX_GET_TICK(t6);
            do{
                tmp = mad_mx_r_test(r_track);
            }while(!tmp);

            TBX_GET_TICK(t7);

            counter++;

            chrono_client_3_4 += TBX_TIMING_DELAY(t3, t4);
            chrono_client_4_5 += TBX_TIMING_DELAY(t4, t5);
            chrono_client_5_6 += TBX_TIMING_DELAY(t5, t6);
            chrono_client_6_7 += TBX_TIMING_DELAY(t6, t7);
            nb_chronos_client++;

#else
            mad_mx_isend(s_track, iovec_e);
            while(!mad_mx_s_test(s_track_set))
                ;

            mad_mx_irecv(r_track, iovec_r);
            do{
                tmp = mad_mx_r_test(r_track);
            }while(!tmp);

            counter++;
#endif // CHRONO

        }
        TBX_GET_TICK(t2);
        sum = TBX_TIMING_DELAY(t1, t2);

        printf("%9d   %9g   %9g\n",
               cur_length, sum / (NB_LOOPS * 2),
               (2.0 * NB_LOOPS * cur_length) / sum / 1.048576);

#ifdef CHRONO
        printf("mad_mx_isend                %9g\n", chrono_client_3_4 / nb_chronos_client);
        printf("mad_mx_wait en emission     %9g\n", chrono_client_4_5 / nb_chronos_client);
        printf("mad_mx_irecv                %9g\n", chrono_client_5_6 / nb_chronos_client);
        printf("mad_mx_wait en réception    %9g\n", chrono_client_6_7 / nb_chronos_client);
        printf("\n");

        nb_chronos_client    = 0;
        chrono_client_3_4    = 0.0;
        chrono_client_4_5    = 0.0;
        chrono_client_5_6    = 0.0;
        chrono_client_6_7    = 0.0;
#endif //CHRONO

        // next length
        cur_length *= 2;
        counter = 0;
        iovec_e->data[0].iov_len  = cur_length;
        iovec_r->data[0].iov_len  = cur_length;
    }

    TBX_FREE(data_e);
    TBX_FREE(data_r);
    mad_iovec_free(iovec_e);
    mad_iovec_free(iovec_r);
    LOG_OUT();
}

void
server(p_mad_channel_t channel){
    p_mad_adapter_t adapter = NULL;
    unsigned int counter    = 0;
    unsigned int cur_length = BUFFER_LENGTH_MIN;

    p_mad_track_set_t s_track_set = NULL;
    p_mad_track_set_t r_track_set = NULL;
    p_mad_track_t s_track = NULL;
    p_mad_track_t r_track = NULL;

    char *data_e = NULL;
    char *data_r = NULL;

    p_mad_iovec_t iovec_e = NULL;
    p_mad_iovec_t iovec_r = NULL;

    p_mad_iovec_t tmp = NULL;

    LOG_IN();
    adapter = channel->adapter;

    s_track_set = adapter->s_track_set;
    s_track = s_track_set->rdv_track;

    r_track_set = adapter->r_track_set;
    r_track = r_track_set->rdv_track;


    data_e = init_and_fill_data(BUFFER_LENGTH_MAX);
    iovec_e = mad_iovec_create(1, //dest
                               NULL, 0, tbx_false, 0, 0);
    mad_iovec_add_data_at_index(iovec_e, data_e, cur_length, 0);
    iovec_e->track = s_track;

    data_r = init_data(BUFFER_LENGTH_MAX);
    iovec_r = mad_iovec_create(1, //dest
                               NULL, 0, tbx_false, 0, 0);
    mad_iovec_add_data_at_index(iovec_r, data_r, cur_length, 0);
    iovec_r->track = r_track;

    while(cur_length <= BUFFER_LENGTH_MAX) {

        mad_mx_irecv(r_track, iovec_r);
        do{
            tmp = mad_mx_r_test(r_track);
        }while(!tmp);

        mad_mx_isend(s_track, iovec_e);
        while(!mad_mx_s_test(s_track_set))
            ;

        //DISP("------------------------------");

        mad_mx_irecv(r_track, iovec_r);
        do{
            tmp = mad_mx_r_test(r_track);
        }while(!tmp);

        while (counter < NB_LOOPS - 1) {
            mad_mx_isend(s_track, iovec_e);
            while(!mad_mx_s_test(s_track_set))
                ;

            mad_mx_irecv(r_track, iovec_r);
            do{
                tmp = mad_mx_r_test(r_track);
            }while(!tmp);

            counter++;
        }
        mad_mx_isend(s_track, iovec_e);
        while(!mad_mx_s_test(s_track_set))
            ;

        // next length
        cur_length*=2;
        counter = 0;
        iovec_e->data[0].iov_len  = cur_length;
        iovec_r->data[0].iov_len  = cur_length;
    }

    TBX_FREE(data_e);
    TBX_FREE(data_r);
    mad_iovec_free(iovec_e);
    mad_iovec_free(iovec_r);
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
        TBX_FAILURE("main : channel not found");

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
