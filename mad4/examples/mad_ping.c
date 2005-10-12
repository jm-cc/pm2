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

#define NB_LOOPS 1000
#define WARMUP_LOOPS 10
#define BUFFER_LENGTH_MIN  4
#define BUFFER_LENGTH_MAX  (2*1024*1024) //32768

int    nb_chronos_client    = 0;
double chrono_client_3_4    = 0.0;
double chrono_client_4_5    = 0.0;
double chrono_client_5_6    = 0.0;
double chrono_client_6_7    = 0.0;

double temps_emission  = 0.0;
double temps_reception = 0.0;

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
        }
    }
    if(user_buffer[i]){
        one_false = tbx_true;
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
    p_mad_connection_t connection1 = NULL;
    p_mad_connection_t connection2 = NULL;

    char *buffer_e = NULL;
    char *buffer_r = NULL;

    unsigned int counter    = 0;
    unsigned int cur_length = BUFFER_LENGTH_MIN;

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

    buffer_e  = init_and_fill_data(BUFFER_LENGTH_MAX);
    buffer_r  = init_data(BUFFER_LENGTH_MAX);

    while(cur_length <= BUFFER_LENGTH_MAX) {
        connection1 = mad_begin_packing(channel, 0);

        while (counter++ < WARMUP_LOOPS) {
            mad_pack(connection1,
                     buffer_e,
                     cur_length,
                     mad_send_CHEAPER,
                     mad_receive_CHEAPER);
            mad_wait_packs(connection1);

            if (counter == 1) {
                connection2 = mad_begin_unpacking(channel);
            }
            mad_unpack(connection2,
                       buffer_r,
                       cur_length,
                       mad_send_CHEAPER,
                       mad_receive_CHEAPER);
            mad_wait_unpacks(connection2);
            //verify_data(buffer_r, cur_length);
            buffer_r[0] = 'g';
        }
        counter = 0;

        //DISP("------------------------------");
        TBX_GET_TICK(t1);
        while (counter < NB_LOOPS) {
            TBX_GET_TICK(t3);
            mad_pack(connection1,
                     buffer_e,
                     cur_length,
                     mad_send_CHEAPER,
                     mad_receive_CHEAPER);
            TBX_GET_TICK(t4);
            mad_wait_packs(connection1);
            TBX_GET_TICK(t5);

            temps_emission  += TBX_TIMING_DELAY(t3, chrono_test);

            mad_unpack(connection2,
                       buffer_r,
                       cur_length,
                       mad_send_CHEAPER,
                       mad_receive_CHEAPER);
            TBX_GET_TICK(t6);
            mad_wait_unpacks(connection2);
            TBX_GET_TICK(t7);

            temps_reception += TBX_TIMING_DELAY(chrono_test, t7);
            //verify_data(buffer_r, cur_length);
            buffer_r[0] = 'g';
            counter++;

            chrono_client_3_4 += TBX_TIMING_DELAY(t3, t4);
            chrono_client_4_5 += TBX_TIMING_DELAY(t4, t5);
            chrono_client_5_6 += TBX_TIMING_DELAY(t5, t6);
            chrono_client_6_7 += TBX_TIMING_DELAY(t6, t7);
            nb_chronos_client++;
        }
        //DISP("T8");
        mad_end_packing(connection1);
        mad_end_unpacking(connection2);
        //DISP("T9");

        TBX_GET_TICK(t2);
        sum = TBX_TIMING_DELAY(t1, t2);

        printf("%9d   %9g   %9g\n",
               cur_length, sum / (NB_LOOPS * 2),
               (2.0 * NB_LOOPS * cur_length) / sum / 1.048576);

        //printf("En �mission, temps entre le mad_pack et le succ�s du mx_test            %9g\n", temps_emission / nb_chronos_client);
        //printf("En r�ception, temps entre le succ�s du mx_test et la fin de la remont�e %9g\n", temps_reception / nb_chronos_client);
        //printf("\n");
        //
        //printf("mad_pack            %9g\n", chrono_client_3_4 / nb_chronos_client);
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
        while (counter++ < WARMUP_LOOPS) {

            mad_unpack(connection1,
                       buffer_r,
                       cur_length,
                       mad_send_CHEAPER,
                       mad_receive_CHEAPER);
            mad_wait_unpacks(connection1);

            if (counter == 1) {
                connection2 = mad_begin_packing(channel, 1);
            }

            mad_pack(connection2,
                     buffer_e,
                     cur_length,
                     mad_send_CHEAPER,
                     mad_receive_CHEAPER);
            mad_wait_packs(connection2);
        }

        counter = 0;

        //DISP("------------------------------");
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
            //DISP("T6");

            mad_unpack(connection1,
                       buffer_r,
                       cur_length,
                       mad_send_CHEAPER,
                       mad_receive_CHEAPER);
            mad_wait_unpacks(connection1);
            buffer_r[0] = 'g';

            counter++;
        }

        mad_pack(connection2,
                 buffer_e,
                 cur_length,
                 mad_send_CHEAPER,
                 mad_receive_CHEAPER);
        mad_wait_packs(connection2);

        mad_end_unpacking(connection1);
        mad_end_packing(connection2);
        //DISP("T12");

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

    //extern int nb_chronos;
    //extern double chrono_remove    ;
    //extern double chrono_add       ;
    //extern double chrono_mx_irecv  ;
    //extern double chrono_total     ;
    //
    //extern int nb_chronos_mkp;
    //extern double chrono_mkfp_1_2;
    //extern double chrono_mkfp_2_3;
    //extern double chrono_mkfp_3_4;
    //extern double chrono_mkfp_4_5;
    //extern double chrono_mkfp_1_5;
    //
    //extern double min_chrono_mkfp_1_2;
    //extern double min_chrono_mkfp_2_3;
    //extern double min_chrono_mkfp_3_4;
    //extern double min_chrono_mkfp_4_5;
    //extern double min_chrono_mkfp_1_5;
    //
    //extern double max_chrono_mkfp_1_2;
    //extern double max_chrono_mkfp_2_3;
    //extern double max_chrono_mkfp_3_4;
    //extern double max_chrono_mkfp_4_5;
    //extern double max_chrono_mkfp_1_5;
    //
    //extern int    nb_chronos_exploit   ;
    //extern double chrono_exploit_1_2   ;
    //
    //extern int    nb_chronos_treat_data;
    //extern double chrono_treat_data_1_2;
    //extern double chrono_treat_data_2_3;
    //extern double chrono_treat_data_3_4;
    //extern double chrono_treat_data_5_6;
    //extern double chrono_treat_data_6_7;
    //extern double chrono_treat_data_8_9;
    //extern double chrono_treat_data_1_9;
    //
    //
    //extern int    nb_chronos_r_mkp;
    //extern double chrono_treat_unexpected;
    //extern double chrono_mad_mkp         ;
    //extern double chrono_add_pre_posted  ;
    //extern double chrono_exploit_msg     ;
    //extern int    nb_chronos_fill;
    //extern int    nb_fill_echec;
    //extern double chrono_fill_1_2;
    //extern double chrono_fill_2_3;
    //extern double chrono_fill_3_4;
    //extern double chrono_fill_1_4;
    //
    extern int    nb_chronos_optimize;
    extern double chrono_optimize_1_2;
    extern double chrono_optimize_2_3;
    extern double chrono_optimize_3_4;
    extern double chrono_optimize_1_4;
    //
    //extern int    nb_chronos_s_mkp;
    //extern int    nb_chronos_r_mkp;
    //
    //extern int    nb_chronos_mad_r_mkp;
    //extern int    nb_chronos_mad_s_mkp;
    //extern double chrono_r_mkp;
    //extern double chrono_s_mkp;
    //
    //extern int    nb_chronos_mad_r_mkp_2;
    //extern int    nb_chronos_mad_s_mkp_2;
    //extern double chrono_r_mkp_2;
    //extern double chrono_s_mkp_2;

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


    //printf("MAD_WAIT_UNPACK\n");
    //printf("\tMAD_R_MKP_PROGRESS --> %d fois\n", nb_chronos_r_mkp);
    //printf("\t \tMAD_MX_ADD_PRE_POSTED : \n");
    //printf("\t \tremove   --> %9g\n", chrono_remove/nb_chronos);
    //printf("\t \tadd      --> %9g\n", chrono_add/nb_chronos);
    //printf("\t \tmx_irecv --> %9g\n", chrono_mx_irecv/nb_chronos);
    //printf("\t \t-------------------------------\n");
    //printf("\t \ttotal    --> %9g\n", chrono_total/nb_chronos);
    //
    //printf("\n");
    //printf("\n");
    //
    //printf("\t \tMAD_MAKE_PROGRESS\n");
    //printf("\t \tdebut          %9g\n", chrono_mkfp_1_2/nb_chronos_mkp);
    //printf("\t \tpipeline get   %9g\n", chrono_mkfp_2_3/nb_chronos_mkp);
    //printf("\t \tinterface test %9g\n", chrono_mkfp_3_4/nb_chronos_mkp);
    //printf("\t \tfin            %9g\n", chrono_mkfp_4_5/nb_chronos_mkp);
    //printf("\t \t-------------------------------\n");
    //printf("\t \ttotal          %9g\n", chrono_mkfp_1_5/nb_chronos_mkp);
    //printf("\t \t-------------------------------\n");
    //printf("\t \tmin debut          %9g\n", min_chrono_mkfp_1_2);
    //printf("\t \tmin pipeline get   %9g\n", min_chrono_mkfp_2_3);
    //printf("\t \tmin interface test %9g\n", min_chrono_mkfp_3_4);
    //printf("\t \tmin fin            %9g\n", min_chrono_mkfp_4_5);
    //printf("\t \tmin total          %9g\n", min_chrono_mkfp_1_5);
    //printf("\t \t-------------------------------\n");
    //printf("\t \tmax debut          %9g\n", max_chrono_mkfp_1_2);
    //printf("\t \tmax pipeline get   %9g\n", max_chrono_mkfp_2_3);
    //printf("\t \tmax interface test %9g\n", max_chrono_mkfp_3_4);
    //printf("\t \tmax fin            %9g\n", max_chrono_mkfp_4_5);
    //printf("\t \tmax total          %9g\n", max_chrono_mkfp_1_5);
    //
    //printf("\n");
    //printf("\n");
    //
    //printf("\t \tMAD_EXPLOIT\n");
    //printf("\t \ttotal     %9g\n", chrono_exploit_1_2/nb_chronos_exploit);
    //
    //printf("\n");
    //printf("\n");

    //printf("\t\t\tMAD_TREAT_DATA\n");
    //printf("\t\t\tmad_iovec_get   %9g\n", chrono_treat_data_1_2/nb_chronos_treat_data);
    //printf("\t\t\tif              %9g\n", chrono_treat_data_2_3/nb_chronos_treat_data);
    //printf("\t\t\tmemcpy          %9g\n", chrono_treat_data_3_4/nb_chronos_treat_data);
    //printf("\t\t\tset_type        %9g\n", chrono_treat_data_5_6/nb_chronos_treat_data);
    //printf("\t\t\tset_length      %9g\n", chrono_treat_data_6_7/nb_chronos_treat_data);
    //printf("\t\t\tmad_iovec_free  %9g\n", chrono_treat_data_8_9/nb_chronos_treat_data);
    //printf("\t\t\ttotal           %9g\n", chrono_treat_data_1_9/nb_chronos_treat_data);
    //
    //printf("\n");
    //printf("\n");
    //
    //
    //
    //
    //
    //printf("chrono_add_pre_posted     %9g\n", chrono_add_pre_posted  /nb_chronos_r_mkp);
    //printf("chrono_exploit_msg        %9g\n", chrono_exploit_msg     /nb_chronos_r_mkp);
    //printf("chrono_total              %9g\n", chrono_total           /nb_chronos_r_mkp_toto);




    //printf("\n");
    //
    //printf("WAIT_PACK\n");
    //printf("\tMAD_S_MKP --> %d fois\n", nb_chronos_s_mkp);
    //
    //printf("\t\tFILL_S_PIPELINE\n");
    //printf("nb de fois ou on cherche mais qu'il n'y a rien � envoyer   %d\n", nb_fill_echec);
    //printf("\t\trecuperation du nouvel mad_iovec       %9g\n", chrono_fill_1_2/nb_chronos_fill);
    //printf("\t\tadd_pipeline                           %9g\n", chrono_fill_2_3/nb_chronos_fill);
    //printf("\t\tisend                                  %9g\n", chrono_fill_3_4/nb_chronos_fill);
    //printf("\t\t-------------------------------\n");
    //printf("\t\ttotal                                  %9g\n", chrono_fill_1_4/nb_chronos_fill);
    //
    //printf("\n");
    //printf("\n");
    //
    printf("\t\t\tOPTIMIZE\n");
    printf("\t\t\textract first            %9g\n", chrono_optimize_1_2/nb_chronos_optimize);
    printf("\t\t\tinit mad_iovec                %9g\n", chrono_optimize_2_3/nb_chronos_optimize);
    printf("\t\t\tcontinue mad_iovec                %9g\n", chrono_optimize_3_4/nb_chronos_optimize);
    printf("\t\t\t-------------------------------\n");
    printf("\t\t\ttotal                 %9g\n", chrono_optimize_1_4/nb_chronos_optimize);


    //printf("\t \tMAD_WAIT_UNPACKS\n");
    //printf("\t \tnb appels � s_mkp = %d\n", nb_chronos_mad_s_mkp);
    //printf("\t \tnb appels � r_mkp = %d\n", nb_chronos_mad_r_mkp);
    //printf("\t \tr_make_progress   --> %9g\n", chrono_r_mkp/nb_chronos_mad_r_mkp);
    //if(nb_chronos_mad_s_mkp){
    //    printf("\t \ts_make_progress   --> %9g\n", chrono_s_mkp/nb_chronos_mad_s_mkp);
    //}
    //
    //printf("\n");
    //printf("\n");
    //
    //
    //printf("\t \tMAD_WAIT_PACKS\n");
    //printf("\t \tnb appels � s_mkp = %d\n", nb_chronos_mad_s_mkp_2);
    //printf("\t \tnb appels � r_mkp = %d\n", nb_chronos_mad_r_mkp_2);
    //printf("\t \ts_make_progress   --> %9g\n", chrono_s_mkp_2/nb_chronos_mad_s_mkp_2);
    //if(nb_chronos_mad_r_mkp_2){
    //    printf("\t \t r_make_progress   --> %9g\n", chrono_r_mkp_2/nb_chronos_mad_r_mkp_2);
    //}
    //
    //printf("\n");
    //printf("\n");
    //
    //printf("TOTAL --> nb appel � s_mkp = %d\n", nb_chronos_s_mkp);
    //printf("      --> nb appel � r_mkp = %d\n", nb_chronos_r_mkp);

    common_exit(NULL);
    LOG_OUT();
    return 0;
}
