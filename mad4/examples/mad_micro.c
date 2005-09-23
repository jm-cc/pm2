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


float chrono_client[NB_LOOPS][3];
float chrono_pack  [NB_LOOPS][3];
int  loop_counter = 0;

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

    //buffer[i] = '\0';
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
    mad_iovec_add_data(mad_iovec, data, length, 2);

    return mad_iovec;
}



p_mad_iovec_t
initialize_mad_iovec_for_emission(int my_rank,
                                  int remote_rank,
                                  int sequence,
                                  p_mad_channel_t channel,
                                  int length,
                                  void *data){
    p_mad_adapter_t adapter = NULL;
    p_mad_track_set_t s_track_set = NULL;
    p_mad_track_t track = NULL;

    p_mad_iovec_t mad_iovec = NULL;
    LOG_IN();

    mad_iovec = initialize_mad_iovec(my_rank, remote_rank,
                                     sequence, channel,
                                     length, data);

    adapter = channel->adapter;
    s_track_set = adapter->s_track_set;
    track = s_track_set->rdv_track;
    mad_iovec->track = track;

    return mad_iovec;
}


p_mad_iovec_t
initialize_mad_iovec_for_reception(int my_rank,
                                  int remote_rank,
                                  int sequence,
                                  p_mad_channel_t channel,
                                  int length,
                                  void *data){
    p_mad_adapter_t adapter = NULL;
    p_mad_track_set_t r_track_set = NULL;
    p_mad_track_t track = NULL;

    p_mad_iovec_t mad_iovec = NULL;
    LOG_IN();

    mad_iovec = initialize_mad_iovec(my_rank, remote_rank,
                                     sequence, channel,
                                     length, data);

    adapter = channel->adapter;
    r_track_set = adapter->r_track_set;
    track = r_track_set->rdv_track;
    mad_iovec->track = track;

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
    //if(user_buffer[i]){
    //    one_false = tbx_true;
    //    //DISP_VAL("data failed : index", i);
    //}

    if(one_false == tbx_false){
        DISP("Data OK");
        //DISP_STR("Data OK -->", user_buffer);
    } else {
        DISP("DATA FAILED");
        //DISP_STR("DATA FAILED--->", user_buffer);
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
    p_mad_driver_interface_t interface = NULL;

    //tbx_tick_t        t1;
    //tbx_tick_t        t2;
    //tbx_tick_t        t3;
    //double            sum = 0.0;
    LOG_IN();
    interface = cnx->channel->adapter->driver->interface;


    //TBX_GET_TICK(t1);
    mad_pipeline_add(mad_iovec->track->pipeline, mad_iovec);
    //TBX_GET_TICK(t2);
    interface->isend(mad_iovec->track,
                     mad_iovec->remote_rank,
                     mad_iovec->data,
                     mad_iovec->total_nb_seg);
    //TBX_GET_TICK(t3);

    //chrono_pack[loop_counter][0] = TBX_TIMING_DELAY(t1, t3);
    //chrono_pack[loop_counter][1] = TBX_TIMING_DELAY(t1, t2);
    //chrono_pack[loop_counter][2] = TBX_TIMING_DELAY(t2, t3);
    //loop_counter ++;

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
mad_wait_pack(p_mad_adapter_t adapter, p_mad_track_t track){
    mad_mkp_status_t status = MAD_MKP_NO_PROGRESS;

    //int counter = 0;
    LOG_IN();

    while(status != MAD_MKP_PROGRESS){
        status = mad_make_progress(adapter, track);

        //if(status == MAD_MKP_PROGRESS)
        //    DISP_VAL("PACK : test ok au tour de boucle", counter);
        //counter++;
    }
    mad_pipeline_remove(track->pipeline);
    LOG_OUT();
}

void
mad_wait_unpack(p_mad_adapter_t adapter, p_mad_track_t track){
    mad_mkp_status_t status = MAD_MKP_NO_PROGRESS;

    //int counter = 0;
    LOG_IN();

    while(status != MAD_MKP_PROGRESS){
        status = mad_make_progress(adapter, track);

        //if(status == MAD_MKP_PROGRESS)
        //    DISP_VAL("UNPACK : test ok au tour de boucle", counter);
        //counter++;
    }
    mad_pipeline_remove(track->pipeline);
    LOG_OUT();
}


void disp_data(char *data, int length){
    int i = 0;
    for(i=0; i < length; i++){
        printf("%c", data[i]);
    }
    printf("\n");
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

    //tbx_tick_t        t1;
    //tbx_tick_t        t2;
    //tbx_tick_t        t3;
    tbx_tick_t        t4;
    tbx_tick_t        t5;
    double            sum = 0.0;

    char *data_e = NULL;
    char *data_r = NULL;
    LOG_IN();
    adapter = channel->adapter;

    data_e = init_and_fill_data(BUFFER_LENGTH_MAX);

    mad_iovec_e = initialize_mad_iovec_for_emission(1, 0,
                                                    87, channel,
                                                    cur_length,
                                                    data_e);

    data_r = init_data(BUFFER_LENGTH_MAX);

    mad_iovec_r = initialize_mad_iovec_for_reception(1, 0,
                                                     87, channel,
                                                     cur_length,
                                                     data_r);


    while(cur_length <= BUFFER_LENGTH_MAX) {
        connection1 = mad_begin_packing(channel, 0);
        mad_pack2(connection1, mad_iovec_e);
        mad_wait_pack(adapter, mad_iovec_e->track);

        connection2 = mad_begin_unpacking(channel);
        mad_unpack2(connection2, mad_iovec_r);
        mad_wait_unpack(adapter, mad_iovec_r->track);

        //DISP("------------------------------");

        TBX_GET_TICK(t4);
        while (counter < NB_LOOPS) {

            //TBX_GET_TICK(t1);
            mad_pack2(connection1, mad_iovec_e);
            //TBX_GET_TICK(t2);
            mad_wait_pack(adapter, mad_iovec_e->track);
            //TBX_GET_TICK(t3);

            //chrono_client[counter][0]= TBX_TIMING_DELAY(t1, t3);
            //chrono_client[counter][1]= TBX_TIMING_DELAY(t1, t2);
            //chrono_client[counter][2]= TBX_TIMING_DELAY(t2, t3);

            mad_unpack2(connection2,mad_iovec_r);
            mad_wait_unpack(adapter, mad_iovec_r->track);

            //verify_data(data_r, cur_length);
            //disp_data(data_r, cur_length);
            data_r[0] = 'g';

            counter++;
        }
        TBX_GET_TICK(t5);

        connection1->lock = tbx_false;
        channel->reception_lock = tbx_false;

        sum = TBX_TIMING_DELAY(t4, t5);

        printf("%9d   %9g   %9g\n",
               cur_length, sum / (NB_LOOPS * 2),
               (2.0 * NB_LOOPS * cur_length) / sum / 1.048576);

        // next length
        cur_length *= 2;
        counter = 0;
        connection1 = NULL;
        connection2 = NULL;

        update_mad_iovec(mad_iovec_e, cur_length);
        update_mad_iovec(mad_iovec_r, cur_length);

    }
    mad_iovec_free(mad_iovec_e, tbx_false);
    mad_iovec_free(mad_iovec_r, tbx_false);

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

    data_r = init_data(BUFFER_LENGTH_MAX);
    mad_iovec_r = initialize_mad_iovec_for_reception(0, 1,
                                                     87, channel,
                                                     cur_length,
                                                     data_r);




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

    mad_iovec_free(mad_iovec_e, tbx_false);
    mad_iovec_free(mad_iovec_r, tbx_false);

    TBX_FREE(data_e);
    TBX_FREE(data_r);
    LOG_OUT();
}


void
disp_chrono_client(){
    int   i = 0;
    float a = 0;
    float b = 0;
    float c = 0;
    LOG_IN();

    printf("CLIENT\n");

    printf("  Total  =   Pack2  +  Wait\n");

    //for(i = 0; i < NB_LOOPS; i++){
    //    int j = 0;
    //    for(j = 0; j < 3; j++){
    //        printf("%9g ", chrono_client[i][j]);
    //    }
    //    printf("\n");
    //}

    printf("------------------------------\n");

    for(i = 0; i < NB_LOOPS; i++){
        a += chrono_client[i][0];
    }
    a = a / NB_LOOPS;

    for(i = 0; i < NB_LOOPS; i++){
        b += chrono_client[i][1];
    }
    b = b / NB_LOOPS;

    for(i = 0; i < NB_LOOPS; i++){
        c += chrono_client[i][2];
    }
    c = c / NB_LOOPS;

    printf("%9g %9g %9g\n", a, b, c);

    printf("\n");
    LOG_OUT();
}


void
disp_chrono_pack(){
    int   i = 0;
    float a = 0;
    float b = 0;
    float c = 0;
    LOG_IN();

    printf("PACK2\n");

    printf("    Total  = add_pipeline + i->isend \n");

    //for(i = 0; i < NB_LOOPS; i++){
    //    int j = 0;
    //    for(j = 0; j < 3; j++){
    //        printf("%12g ", chrono_pack[i][j]);
    //    }
    //    printf("\n");
    //}

    printf("------------------------------\n");

    for(i = 0; i < NB_LOOPS; i++){
        a += chrono_pack[i][0];
    }
    a = a / NB_LOOPS;

    for(i = 0; i < NB_LOOPS; i++){
        b += chrono_pack[i][1];
    }
    b = b / NB_LOOPS;

    for(i = 0; i < NB_LOOPS; i++){
        c += chrono_pack[i][2];
    }
    c = c / NB_LOOPS;

    printf("%12g %12g %12g\n", a, b, c);

    printf("\n");
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

        //disp_chrono_client();

    } else if (my_local_rank == 0) {
        DISP("SERVER");
        server(channel);
        DISP("FINI!!!");
    }

    //disp_chrono_pack();

    common_exit(NULL);
    LOG_OUT();
    return 0;
}























//int
//main(int argc, char **argv) {
//        p_mad_madeleine_t          madeleine      = NULL;
//        p_mad_channel_t            channel        = NULL;
//        p_ntbx_process_container_t pc             = NULL;
//        ntbx_process_grank_t       my_global_rank =   -1;
//        ntbx_process_lrank_t       my_local_rank  =   -1;
//
//        /*
//         * Initialisation des diverses bibliothèques.
//         */
//        common_pre_init (&argc, argv, NULL);
//        common_post_init(&argc, argv, NULL);
//
//        /*
//         * Récupération de l'objet Madeleine.
//         */
//        madeleine      = mad_get_madeleine();
//
//        /*
//         * Récupération du numéro global du processus (unique dans la session).
//         */
//        my_global_rank = madeleine->session->process_rank;
//
//        /*
//         * Récupération de l'objet 'channel' correspondant au canal "test"
//         * défini dans le fichier de configuration.
//         */
//        channel = tbx_htable_get(madeleine->channel_htable, "test");
//
//        /*
//         * Conversion du numéro global du processus en numéro local au
//         * niveau du canal.
//         *
//         * Opération en deux temps:
//         *
//         * 1) on récupère le "process container" du canal qui est un tableau
//         * à double entrées rang local/rang global
//         *
//         * 2) on convertit le numéro global du processus en un numéro local
//         * dans le contexte du canal. Le contexte est fourni par le process
//         * container.
//         */
//        pc = channel->pc;
//        my_local_rank = ntbx_pc_global_to_local(pc, my_global_rank);
//
//        /*
//         * On ne s'occupe que des processus de numéro local 0 ou 1 du canal.
//         * Les autres n'interviennent pas dans ce programme.
//         */
//        if (my_local_rank == 0) {
//                /*
//                 * Le processus de numéro local '0' va jouer le rôle de l'émetteur.
//                 */
//
//                /*
//                 * L'objet "connection" pour émettre.
//                 */
//                p_mad_connection_t  out    = NULL;
//
//                int                 len    =    0;
//                char               *chaine = "Hello, world";
//
//                /*
//                 * On demande l'émission d'un message sur le canal
//                 * "channel", vers le processus de numéro local "1".
//                 */
//                out  = mad_begin_packing(channel, 1);
//
//                len = strlen(chaine)+1;
//
//                /*
//                 * On envoie la longueur de la chaîne en Express.
//                 */
//                mad_pack(out, &len,   sizeof(len), mad_send_CHEAPER, mad_receive_EXPRESS);
//
//                /*
//                 * On envoie la chaîne proprement dite, en Cheaper.
//                 */
//                mad_pack(out, chaine, len,         mad_send_CHEAPER, mad_receive_CHEAPER);
//
//                /*
//                 * On indique que le message est entièrement construit
//                 * et que toutes les données non encore envoyées
//                 * doivent partir.
//                 */
//                mad_end_packing(out);
//        } else if (my_local_rank == 1) {
//                /*
//                 * Le processus de numéro local '1' va jouer le rôle du récepteur.
//                 */
//
//                /*
//                 * L'objet "connection" pour recevoir.
//                 */
//                p_mad_connection_t  in  = NULL;
//
//                int                 len =    0;
//                char               *msg = NULL;
//
//                /*
//                 * On demande la réception du premier "message"
//                 * disponible. Note: il n'est pas possible de
//                 * spécifier "de qui" on veut recevoir un message.
//                 */
//                in = mad_begin_unpacking(channel);
//
//                /*
//                 * On reçoit la longueur de la chaîne en Express car
//                 * on a immédiatement besoin de cette information pour
//                 * allouer la mémoire nécessaire à la réception de la
//                 * chaîne proprement dite.
//                 */
//                mad_unpack(in, &len, sizeof(len), mad_send_CHEAPER, mad_receive_EXPRESS);
//
//                /*
//                 * On alloue de la mémoire pour recevoir la chaîne.
//                 */
//                msg = malloc(len);
//
//                /*
//                 * On reçoit la chaîne de caractères.
//                 */
//                mad_unpack(in, msg,  len,         mad_send_CHEAPER, mad_receive_CHEAPER);
//
//                /*
//                 * On indique la fin de la réception. C'est seulement
//                 * après l'appel à end_unpacking que la disponibilité
//                 * des données marquées "receive_CHEAPER" sont
//                 * garanties.
//                 */
//                mad_end_unpacking(in);
//
//                /*
//                 * On affiche le message pour montrer qu'on l'a bien reçu.
//                 */
//                printf("msg = [%s]\n", msg);
//        }
//
//        /*
//         * On termine l'exécution des bibliothèques.
//         */
//        common_exit(NULL);
//
//        return 0;
//}
