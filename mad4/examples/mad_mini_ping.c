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

#define NB_LOOPS 10000
#define BUFFER_LENGTH_MIN  4
#define BUFFER_LENGTH_MAX  32000 //(2*1024*1024) //32768


int    nb_chronos_client    = 0;
double chrono_client_3_4    = 0.0;
double chrono_client_4_5    = 0.0;
double chrono_client_5_6    = 0.0;
double chrono_client_6_7    = 0.0;

double temps_emission  = 0.0;
double temps_reception = 0.0;

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


char *
init_data(unsigned int length){
    char *buffer = NULL;
    LOG_IN();
    buffer = TBX_MALLOC(length);

    if((char)buffer[0] % 4)
        FAILURE("buffer non aligné");

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

p_mad_iovec_t
initialize_mad_iovec(int my_rank,
                     int remote_rank,
                     int sequence,
                     p_mad_channel_t channel,
                     int length,
                     void *data){
    p_mad_iovec_t mad_iovec = NULL;
    LOG_IN();

    mad_iovec = mad_iovec_create(remote_rank, channel,
                                 sequence,
                                 tbx_false,
                                 mad_send_CHEAPER,
                                 mad_receive_CHEAPER);
    mad_iovec_begin_struct_iovec(mad_iovec);
    mad_iovec_add_data_header(mad_iovec,
                              channel->id,
                              sequence,
                              length);
    mad_iovec_add_data(mad_iovec, data, length);
    mad_iovec_update_global_header(mad_iovec);
    LOG_OUT();
    return mad_iovec;
}

p_mad_iovec_t
initialize_mad_iovec_for_emission(int my_rank,
                                  int remote_rank,
                                  int sequence,
                                  p_mad_channel_t channel,
                                  int length,
                                  void *data){
    p_mad_adapter_t   adapter     = NULL;
    p_mad_track_set_t s_track_set = NULL;
    p_mad_track_t     track       = NULL;
    p_mad_iovec_t     mad_iovec   = NULL;
    LOG_IN();
    adapter = channel->adapter;
    s_track_set = adapter->s_track_set;
    track = s_track_set->rdv_track;

    mad_iovec = initialize_mad_iovec(my_rank, remote_rank,
                                     sequence, channel,
                                     length, data);
    mad_iovec->track = track;
    LOG_OUT();
    return mad_iovec;
}


p_mad_iovec_t
initialize_mad_iovec_for_reception(int my_rank,
                                  int remote_rank,
                                  int sequence,
                                  p_mad_channel_t channel,
                                  int length,
                                  void *data){
    p_mad_adapter_t   adapter     = NULL;
    p_mad_track_set_t r_track_set = NULL;
    p_mad_track_t     track       = NULL;
    p_mad_iovec_t     mad_iovec   = NULL;
    LOG_IN();
    adapter = channel->adapter;
    r_track_set = adapter->r_track_set;
    track = r_track_set->rdv_track;

    mad_iovec = initialize_mad_iovec(my_rank, remote_rank,
                                     sequence, channel,
                                     length, data);
    mad_iovec->track = track;
    LOG_OUT();
    return mad_iovec;
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
    if(!one_false){
        DISP("Data OK");
    } else {
        DISP("DATA FAILED");
    }
    LOG_OUT();
}


void
update_mad_iovec(p_mad_iovec_t mad_iovec,
                 int length){
    mad_iovec->length = length;
    mad_iovec->data[2].iov_len = length;
}


void
mad_pack2(p_mad_connection_t cnx, p_mad_iovec_t mad_iovec){
    p_mad_adapter_t a = NULL;
    p_mad_driver_interface_t interface = NULL;

    LOG_IN();
    a = cnx->channel->adapter;
    interface = a->driver->interface;

    interface->isend(mad_iovec->track,
                     mad_iovec);
    LOG_OUT();
}


void
mad_unpack2(p_mad_connection_t cnx, p_mad_iovec_t mad_iovec){
    p_mad_channel_t channel = NULL;
    p_mad_driver_interface_t interface = NULL;

    LOG_IN();
    channel = cnx->channel;
    interface = channel->adapter->driver->interface;

    tbx_slist_append(channel->unpacks_list, mad_iovec);

    interface->irecv(mad_iovec->track,
                     mad_iovec);
    LOG_OUT();
}


void
mad_wait_pack(p_mad_adapter_t adapter, p_mad_track_t track){
    mad_mkp_status_t status = MAD_MKP_NO_PROGRESS;

    LOG_IN();
    while(status != MAD_MKP_PROGRESS){
        status = mad_s_make_progress(adapter);
    }
    LOG_OUT();
}

void
mad_wait_unpack(p_mad_channel_t channel){
    mad_mkp_status_t status = MAD_MKP_NO_PROGRESS;

    LOG_IN();
    while(status != MAD_MKP_PROGRESS){
        status = mad_r_make_progress(adapter);
    }

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

    char *data_e = NULL;
    char *data_r = NULL;

    tbx_tick_t        t1;
    tbx_tick_t        t2;
    tbx_tick_t        t3;
    tbx_tick_t        t4;
    tbx_tick_t        t5;
    tbx_tick_t        t6;
    tbx_tick_t        t7;
    double            sum = 0.0;
    extern tbx_tick_t chrono_test;
    LOG_IN();
    adapter = channel->adapter;

    data_e = init_and_fill_data(BUFFER_LENGTH_MAX);

    mad_iovec_e = initialize_mad_iovec_for_emission(1, 0,
                                                    87, channel,
                                                    cur_length,
                                                    data_e);

    mad_iovec_e->track = adapter->s_track_set->cpy_track;

    data_r = init_data(BUFFER_LENGTH_MAX);

    mad_iovec_r = initialize_mad_iovec_for_reception(1, 0,
                                                     87, channel,
                                                     cur_length,
                                                     data_r);
    mad_iovec_r->track = adapter->r_track_set->cpy_track;

    while(cur_length <= BUFFER_LENGTH_MAX) {
        connection1 = mad_begin_packing(channel, 0);
        mad_pack2(connection1, mad_iovec_e);
        mad_wait_pack(adapter, mad_iovec_e->track);

        connection2 = mad_begin_unpacking(channel);
        mad_unpack2(connection2, mad_iovec_r);
        mad_wait_unpack(adapter, mad_iovec_r->track);

        //DISP("------------------------------");

        TBX_GET_TICK(t1);
        while (counter < NB_LOOPS) {
            TBX_GET_TICK(t3);
            mad_pack2(connection1, mad_iovec_e);
            TBX_GET_TICK(t4);
            mad_wait_pack(adapter, mad_iovec_e->track);
            TBX_GET_TICK(t5);

            temps_emission  += TBX_TIMING_DELAY(t3, chrono_test);

            mad_unpack2(connection2,mad_iovec_r);
            TBX_GET_TICK(t6);
            mad_wait_unpack(adapter, mad_iovec_r->track);
            TBX_GET_TICK(t7);

            temps_reception += TBX_TIMING_DELAY(chrono_test, t7);

            data_r[0] = 'g';

            counter++;

            chrono_client_3_4 += TBX_TIMING_DELAY(t3, t4);
            chrono_client_4_5 += TBX_TIMING_DELAY(t4, t5);
            chrono_client_5_6 += TBX_TIMING_DELAY(t5, t6);
            chrono_client_6_7 += TBX_TIMING_DELAY(t6, t7);
            nb_chronos_client++;
        }
        TBX_GET_TICK(t2);

        connection1->lock = tbx_false;
        channel->reception_lock = tbx_false;

        sum = TBX_TIMING_DELAY(t1, t2);

        printf("%9d   %9g   %9g\n",
               cur_length, sum / (NB_LOOPS * 2),
               (2.0 * NB_LOOPS * cur_length) / sum / 1.048576);


        printf("En émission, temps entre le mad_pack et le succès du mx_test            %9g\n", temps_emission / nb_chronos_client);
        printf("En réception, temps entre le succès du mx_test et la fin de la remontée %9g\n", temps_reception / nb_chronos_client);
        printf("\n");

        printf("mad_pack            %9g\n", chrono_client_3_4 / nb_chronos_client);
        DISP("\t-->MAD_PACK");
        printf("\tpipeline_add      %9g\n", chrono_pack2_1_2 / nb_chronos_pack2);
        printf("\tisend             %9g\n", chrono_pack2_2_3 / nb_chronos_pack2);
        printf("\ttotal             %9g\n", chrono_pack2_1_3 / nb_chronos_pack2);
        printf("\n");

        printf("mad_wait_pack       %9g\n", chrono_client_4_5 / nb_chronos_client);
        DISP("\t-->MAD_WAIT_PACK");
        printf("\twhile(s_mkp)      %9g\n", chrono_wait_pack_1_2 / nb_chronos_wait_pack);
        printf("\tpipeline_remove   %9g\n", chrono_wait_pack_2_3 / nb_chronos_wait_pack);
        printf("\ttotal             %9g\n", chrono_wait_pack_1_3 / nb_chronos_wait_pack);
        printf("\n");

        printf("mad_unpack          %9g\n", chrono_client_5_6 / nb_chronos_client);
        DISP("\t-->MAD_UNPACK");
        printf("\tpipeline_add      %9g\n", chrono_unpack2_1_2 / nb_chronos_unpack2);
        printf("\tirecv             %9g\n", chrono_unpack2_2_3 / nb_chronos_unpack2);
        printf("\ttotal             %9g\n", chrono_unpack2_1_3 / nb_chronos_unpack2);
        printf("\n");

        printf("mad_wait_unpack     %9g\n", chrono_client_6_7 / nb_chronos_client);
        DISP("\t-->MAD_WAIT_UNPACK");
        printf("\twhile(r_mkp       %9g\n", chrono_wait_unpack_1_2 / nb_chronos_wait_unpack);
        printf("\tpipeline_remove   %9g\n", chrono_wait_unpack_2_3 / nb_chronos_wait_unpack);
        printf("\ttotal             %9g\n", chrono_wait_unpack_1_3 / nb_chronos_wait_unpack);
        printf("-------------------------------\n");

        //printf("mad_wait_pack       %9g\n", chrono_client_4_5 / nb_chronos_client);
        //printf("mad_unpack          %9g\n", chrono_client_5_6 / nb_chronos_client);
        //printf("mad_wait_unpack     %9g\n", chrono_client_6_7 / nb_chronos_client);
        //printf("\n");

        nb_chronos_client    = 0;
        chrono_client_3_4    = 0.0;
        chrono_client_4_5    = 0.0;
        chrono_client_5_6    = 0.0;
        chrono_client_6_7    = 0.0;
        temps_emission       = 0.0;
        temps_reception      = 0.0;

        // next length
        cur_length *= 2;
        counter = 0;
        connection1 = NULL;
        connection2 = NULL;

        update_mad_iovec(mad_iovec_e, cur_length);
        update_mad_iovec(mad_iovec_r, cur_length);

    }
    mad_iovec_free(mad_iovec_e);
    mad_iovec_free(mad_iovec_r);

    TBX_FREE(data_e);
    TBX_FREE(data_r);
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

    char *data_e = NULL;
    char *data_r = NULL;

    LOG_IN();
    adapter = channel->adapter;

    data_e = init_and_fill_data(BUFFER_LENGTH_MAX);
    mad_iovec_e = initialize_mad_iovec_for_emission(0, 1,
                                                    87, channel,
                                                    cur_length,
                                                    data_e);
    mad_iovec_e->track = adapter->s_track_set->cpy_track;

    data_r = init_data(BUFFER_LENGTH_MAX);
    mad_iovec_r = initialize_mad_iovec_for_reception(0, 1,
                                                     87, channel,
                                                     cur_length,
                                                     data_r);
    mad_iovec_r->track = adapter->r_track_set->cpy_track;

    while(cur_length <= BUFFER_LENGTH_MAX) {
        connection1 = mad_begin_unpacking(channel);
        mad_unpack2(connection1, mad_iovec_r);
        mad_wait_unpack(adapter, mad_iovec_r->track);

        connection2 = mad_begin_packing(channel, 1);
        mad_pack2(connection2, mad_iovec_e);
        mad_wait_pack(adapter, mad_iovec_e->track);

        //DISP("------------------------------");

        mad_unpack2(connection1, mad_iovec_r);
        mad_wait_unpack(adapter, mad_iovec_r->track);

        while (counter < NB_LOOPS - 1) {
            mad_pack2(connection2, mad_iovec_e);
            mad_wait_pack(adapter, mad_iovec_e->track);

            mad_unpack2(connection1, mad_iovec_r);
            mad_wait_unpack(adapter, mad_iovec_r->track);
            //verify_data(data_r, cur_length);
            data_r[0] = 'j';

            counter++;
        }
        mad_pack2(connection2, mad_iovec_e);
        mad_wait_pack(adapter, mad_iovec_e->track);

        channel->reception_lock = tbx_false;
        connection2->lock = tbx_false;

        // next length
        cur_length*=2;
        counter = 0;
        connection1 = NULL;
        connection2 = NULL;

        update_mad_iovec(mad_iovec_e, cur_length);
        update_mad_iovec(mad_iovec_r, cur_length);
    }

    mad_iovec_free(mad_iovec_e);
    mad_iovec_free(mad_iovec_r);

    TBX_FREE(data_e);
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
