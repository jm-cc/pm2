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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */


/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 "the PM2 team" (see AUTHORS file)
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
 * basic_ping.c
 * ============
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include <nm_public.h>

/* Choose a scheduler
 */
#include <nm_public.h>
#if defined CONFIG_SCHED_MINI_ALT
#  include <nm_mini_alt_public.h>
#elif defined CONFIG_SCHED_OPT
#  include <nm_so_public.h>
#else
#  include <nm_mini_public.h>
#endif

/* Choose an API
 */
#include <nm_basic_public.h>

/* Choose a driver
 */
#if defined CONFIG_MX
#  include <nm_mx_public.h>
#elif defined CONFIG_GM
#  include <nm_gm_public.h>
#else
#  include <nm_tcp_public.h>
#endif

#include <tbx.h>

//#define CHRONO


// Types
//......................
typedef struct s_ping_result
{
    int                  size;
    double               latency;
    double               bandwidth_mbit;
    double               bandwidth_mbyte;
} ping_result_t, *p_ping_result_t;

// Static variables
//......................

static const int nb_loops       = 100;
#define NB_PACKS 3
#define NB_PACKS_INIT 3

static const int param_min_size = 65536; //32768;
static const int param_max_size = 65536; //32768; //1024*1024*2;
static const int param_step     = 0; /* 0 = progression log. */
static const int param_nb_tests = 1; //5;
static const int param_no_zero  = 1;


static struct nm_core		*p_core		= NULL;
static struct nm_proto		*p_proto	= NULL;
static uint8_t			 drv_id		=    0;
static nm_gate_id_t              gate_id	=    0;


static unsigned char *main_buffer[2];

static struct nm_basic_rq **p_rq_tab[2];
static int nb_success_courant[2];

static int nb_boucles_courant[2];
static int nb_packs_courant[2];
static int iteration_courante[2];
static int thread_courant = 0;

// Functions
//......................


static __inline__
void
b_isend(void *buf, uint64_t len) {
    int err;

    err = nm_basic_isend(p_proto, 0, thread_courant,
                         buf, len, &p_rq_tab[thread_courant][nb_packs_courant[thread_courant]]);
    if (err != NM_ESUCCESS) {
        printf("nm_basic_isend returned err = %d\n", err);
        goto out;
    }
    return;

 out:
    exit(EXIT_FAILURE);
}

static __inline__
void b_irecv(void *buf, uint64_t len) {
    int err;

    err= nm_basic_irecv(p_proto, 0, thread_courant,
                        buf, len, &p_rq_tab[thread_courant][nb_packs_courant[thread_courant]]);

    if (err != NM_ESUCCESS) {
        printf("nm_basic_isend returned err = %d\n", err);
        goto out;
    }
    return;

 out:
    exit(EXIT_FAILURE);
}

static __inline__
int
b_test(void){
    int err;

    err = nm_basic_test(p_rq_tab[thread_courant][nb_success_courant[thread_courant]]);

    return err;
}

static int cpt = 0;

static __inline__
tbx_bool_t
changement_contexte(void){
    //cpt++;
    //if(cpt == 4){
    //    cpt = 0;
    //    return tbx_true;
    //}
    return tbx_true;
}



static void
ping(void) {
    int size = 0;
    int err;
    int nb_packs;

    for(nb_packs = NB_PACKS_INIT; nb_packs <= NB_PACKS; nb_packs++){

        for (size = param_min_size;
             size <= param_max_size;
             size = param_step?size + param_step:size * 2) {
            const int       _size   = (!size && param_no_zero)?1:size;
            ping_result_t	results = { 0, 0.0, 0.0, 0.0 };
            double          sum     = 0.0;
            tbx_tick_t      t1;
            tbx_tick_t      t2;

            sum = 0;

            nb_boucles_courant[0] = nb_loops;
            nb_boucles_courant[1] = nb_loops;

            iteration_courante[0] = 0;
            iteration_courante[1] = 0;

            nb_packs_courant[0] = 0;
            nb_packs_courant[1] = 0;

            nb_success_courant[0] = 0;
            nb_success_courant[1] = 0;

            TBX_GET_TICK(t1);
            while(nb_boucles_courant[0] && nb_boucles_courant[1]){

                switch(iteration_courante[thread_courant]){
                case 0:
                    //printf("case %d - %d\n", iteration_courante[thread_courant], thread_courant);
                    b_isend(main_buffer[thread_courant], _size);

                    nb_packs_courant[thread_courant]++;

                    if(nb_packs_courant[thread_courant] ==  nb_packs){
                        iteration_courante[thread_courant]++;
                        nb_packs_courant[thread_courant] = 0;
                    }

                    if(changement_contexte()){
                        thread_courant = 1 - thread_courant;

                        break;
                    }

                    break;

                case 1:
                    err = b_test();

                    if(err == NM_ESUCCESS){
                        //printf("case %d - %d - SUCCESS\n", iteration_courante[thread_courant], thread_courant);

                        nb_success_courant[thread_courant]++;
                        if(nb_success_courant[thread_courant] == nb_packs){
                            iteration_courante[thread_courant]++;
                            nb_success_courant[thread_courant] = 0;

                            //printf("##########################\n");
                            //printf("##########################\n");

                        }

                        if(changement_contexte()){
                            thread_courant = 1 - thread_courant;
                            break;
                        }
                    } else {
                        //printf("case %d - %d - ECHEC\n", iteration_courante[thread_courant], thread_courant);

                        thread_courant = 1 - thread_courant;
                        break;
                    }

                    break;

                case 2:
                    //printf("case %d - %d\n", iteration_courante[thread_courant], thread_courant);
                    b_irecv(main_buffer[thread_courant], _size);

                    nb_packs_courant[thread_courant]++;
                    if(nb_packs_courant[thread_courant] ==  nb_packs){
                        iteration_courante[thread_courant]++;
                        nb_packs_courant[thread_courant] = 0;
                    }

                    if(changement_contexte()){
                        thread_courant = 1 - thread_courant;
                        break;
                    }

                    break;

                case 3:
                    err = b_test();
                    if(err == NM_ESUCCESS){
                        //printf("case %d - %d - SUCCESS\n", iteration_courante[thread_courant], thread_courant);

                        nb_success_courant[thread_courant]++;

                        if(nb_success_courant[thread_courant] == nb_packs){
                            iteration_courante[thread_courant] = 0;
                            nb_boucles_courant[thread_courant]--;
                            nb_success_courant[thread_courant] = 0;
                        }

                        if(changement_contexte()){
                            thread_courant = 1 - thread_courant;
                            break;
                        }
                    } else {
                        //printf("case %d - %d - ECHEC\n", iteration_courante[thread_courant], thread_courant);

                        thread_courant = 1 - thread_courant;
                        break;
                    }

                    break;

                default:
                    TBX_FAILURE("invalid state");
                }
            }

            if(nb_boucles_courant[0]){
                thread_courant = 0;
            } else if(nb_boucles_courant[1]){
                thread_courant = 1;
            } else {
                continue;
            }

            switch(iteration_courante[thread_courant]){
            case 0:
                for( ; nb_packs_courant[thread_courant] < nb_packs; nb_packs_courant[thread_courant]++){
                    b_isend(main_buffer[thread_courant], _size);
                }
                nb_packs_courant[thread_courant] = 0;

            case 1:
                while(nb_success_courant[thread_courant] != nb_packs){
                    err = b_test();
                    if(err == NM_ESUCCESS){
                        nb_success_courant[thread_courant]++;
                    }
                }
                nb_success_courant[thread_courant] = 0;

            case 2:
                for( ; nb_packs_courant[thread_courant] < nb_packs;  nb_packs_courant[thread_courant]++){
                    b_irecv(main_buffer[thread_courant], _size);
                }
                nb_packs_courant[thread_courant] = 0;

            case 3:
                while(nb_success_courant[thread_courant] != nb_packs){
                    err = b_test();
                    if(err == NM_ESUCCESS){
                        nb_success_courant[thread_courant]++;
                    }
                }
                nb_success_courant[thread_courant] = 0;
            }
            nb_boucles_courant[thread_courant]--;



            while(nb_boucles_courant[thread_courant]){

                for(nb_packs_courant[thread_courant] = 0 ;
                    nb_packs_courant[thread_courant] < nb_packs;
                    nb_packs_courant[thread_courant]++){

                    b_isend(main_buffer[thread_courant], _size);
                }

                while(nb_success_courant[thread_courant] != nb_packs){
                    err = b_test();
                    if(err == NM_ESUCCESS){
                        nb_success_courant[thread_courant]++;
                    }
                }
                nb_success_courant[thread_courant] = 0;

                for(nb_packs_courant[thread_courant] = 0;
                    nb_packs_courant[thread_courant] < nb_packs;
                    nb_packs_courant[thread_courant]++){

                    b_irecv(main_buffer[thread_courant], _size);
                }

                while(nb_success_courant[thread_courant] != nb_packs){
                    err = b_test();
                    if(err == NM_ESUCCESS){
                        nb_success_courant[thread_courant]++;
                    }
                }
                nb_success_courant[thread_courant] = 0;

                nb_boucles_courant[thread_courant]--;
            }

            TBX_GET_TICK(t2);

            sum += TBX_TIMING_DELAY(t1, t2);

            printf("############## %d packs de taille = %d - sum = %f   bw = %f\n",
                   nb_packs, _size,
                   sum / ( 2 * nb_loops * nb_packs),
                   (_size * nb_loops * nb_packs) / sum  );





#ifdef CHRONO
            // Partie émission
            printf("----------------------EMISSION\n");

            extern int nb_sched_out;
            extern double chrono_sched_out;
            printf("\n");
            printf("nb appel à sched_out = %d\n", nb_sched_out);
            printf("temps total passé dans sched_out -> %f\n", chrono_sched_out);
            printf("temps moyen passé dans sched_out -> %f\n", chrono_sched_out / nb_sched_out);
            nb_sched_out = 0;
            chrono_sched_out = 0;

            extern int    nb_search_next;
            extern double chrono_search_next;
            printf("\n");
            printf("    nb appel à search_next = %d\n", nb_search_next);
            printf("    temps total passé dans search_next -> %f\n", chrono_search_next);
            printf("    temps moyen passé dans search_next -> %f\n", chrono_search_next / nb_search_next);
            nb_search_next = 0;
            chrono_search_next = 0;

            //---------------------------

            extern int nb_success_out;
            extern double chrono_success_out;
            printf("\n");
            printf("nb appel à success_out = %d\n", nb_success_out);
            printf("temps total passé dans success_out -> %f\n", chrono_success_out);
            printf("temps moyen passé dans success_out -> %f\n", chrono_success_out / nb_success_out);
            nb_success_out = 0;
            chrono_success_out = 0;

            extern int nb_free_agregated;
            extern double chrono_free_agregated;
            printf("\n");
            printf("    nb appel à free_agregated = %d\n", nb_free_agregated);
            printf("    temps total passé dans free_agregated -> %f\n", chrono_free_agregated);
            printf("    temps moyen passé dans free_agregated -> %f\n", chrono_free_agregated / nb_free_agregated);
            nb_free_agregated = 0;
            chrono_free_agregated = 0;

            printf("\n");
            printf("----------------------RECEPTION\n");
            // Partie réception

            extern double chrono_sched_in;
            extern int    nb_sched_in;
            printf("nb appel à sched_in = %d\n", nb_sched_in);
            printf("temps total passé dans sched_in -> %f\n", chrono_sched_in);
            printf("temps moyen passé dans sched_in -> %f\n", chrono_sched_in / nb_sched_in);
            nb_sched_in = 0;
            chrono_sched_in = 0;

            extern double chrono_treat_unexpected;
            extern int    nb_treat_unexpected;
            printf("\n");
            printf("    nb appel à treat_unexpected = %d\n", nb_treat_unexpected);
            if(nb_treat_unexpected){
                printf("    temps total passé dans treat_unexpected -> %f\n", chrono_treat_unexpected);
                printf("    temps moyen passé dans treat_unexpected -> %f\n", chrono_treat_unexpected / nb_treat_unexpected);
            }
            nb_treat_unexpected = 0;
            chrono_treat_unexpected = 0;

            extern double chrono_treatment_waiting_rdv;
            extern int    nb_treatment_waiting_rdv;
            printf("\n");
            printf("    temps total passé dans le traitement des rdv en attente -> %f\n", chrono_treatment_waiting_rdv);
            printf("    temps moyen passé dans le traitement des rdv en attente -> %f\n", chrono_treatment_waiting_rdv / nb_treatment_waiting_rdv);
            nb_sched_in = 0;
            chrono_sched_in = 0;


            //extern double chrono_schedule_ack;
            //extern int    nb_schedule_ack;
            //printf("\n");
            //printf("    nb appel à schedule_ack = %d\n", nb_schedule_ack);
            //printf("    temps total passé dans schedule_ack -> %f\n", chrono_schedule_ack);
            //printf("    temps moyen passé dans schedule_ack -> %f\n", chrono_schedule_ack / nb_schedule_ack);
            //nb_schedule_ack = 0;
            //chrono_schedule_ack = 0;

            extern double chrono_take_pre_posted;
            extern int    nb_take_pre_posted;
            printf("\n");
            printf("    nb appel à take_pre_posted = %d\n", nb_take_pre_posted);
            printf("    temps total passé dans take_pre_posted -> %f\n", chrono_take_pre_posted);
            printf("    temps moyen passé dans take_pre_posted -> %f\n", chrono_take_pre_posted / nb_take_pre_posted);
            nb_take_pre_posted = 0;
            chrono_take_pre_posted = 0;

            //-------------------
            extern double chrono_success_in;
            extern int    nb_success_in;
            printf("\n");
            printf("nb appel à success_in = %d\n", nb_success_in);
            printf("temps total passé dans success_in -> %f\n", chrono_success_in);
            printf("temps moyen passé dans success_in -> %f\n", chrono_success_in / nb_success_in);
            nb_success_in = 0;
            chrono_success_in = 0;

            extern double chrono_open_data;
            extern int nb_open_data;
            printf("\n");
            printf("    nb appel à open_data = %d\n", nb_open_data);
            printf("    temps total passé dans open_data -> %f\n", chrono_open_data);
            printf("    temps moyen passé dans open_data -> %f\n", chrono_open_data / nb_open_data);
            nb_open_data = 0;
            chrono_open_data = 0;

            extern int    nb_rdv;
            extern double chrono_rdv;
            printf("\n");
            printf("        nb de rdv recus = %d\n", nb_rdv);
            printf("        temps total pour un rdv -> %f\n", chrono_rdv);
            printf("        temps moyen pour un rdv -> %f\n", chrono_rdv / nb_rdv);
            nb_rdv = 0;
            chrono_rdv = 0;

            extern int nb_data;
            extern double chrono_data;
            printf("\n");
            printf("        nb de blocs de données recus = %d\n", nb_data);
            printf("        temps total pour un bloc -> %f\n", chrono_data);
            printf("        temps moyen pour un bloc -> %f\n", chrono_data / nb_data);
            nb_data = 0;
            chrono_data = 0;

#endif
        }
    }
}

static
void
pong(void) {
    int size = 0;
    int err;
    int nb_packs;

    for(nb_packs = NB_PACKS_INIT; nb_packs <= NB_PACKS; nb_packs++){

        for (size = param_min_size;
             size <= param_max_size;
             size = param_step?size + param_step:size * 2) {
            const int _size = (!size && param_no_zero)?1:size;

            nb_boucles_courant[0] = nb_loops;
            nb_boucles_courant[1] = nb_loops;

            iteration_courante[0] = 0;
            iteration_courante[1] = 0;

            nb_packs_courant[0] = 0;
            nb_packs_courant[1] = 0;

            nb_success_courant[0] = 0;
            nb_success_courant[1] = 0;

            while(nb_boucles_courant[0] && nb_boucles_courant[1]){

                switch(iteration_courante[thread_courant]){
                case 0:
                    //printf("case %d - %d\n", iteration_courante[thread_courant], thread_courant);
                    b_irecv(main_buffer[thread_courant], _size);

                    nb_packs_courant[thread_courant]++;

                    if(nb_packs_courant[thread_courant] ==  nb_packs){
                        iteration_courante[thread_courant]++;
                        nb_packs_courant[thread_courant] = 0;
                    }

                    if(changement_contexte()){
                        thread_courant = 1 - thread_courant;
                        break;
                    }

                case 1:
                    err = b_test();
                    if(err == NM_ESUCCESS){
                        //printf("case %d - %d - SUCCESS\n", iteration_courante[thread_courant], thread_courant);

                        nb_success_courant[thread_courant]++;

                        if(nb_success_courant[thread_courant] == nb_packs){
                            iteration_courante[thread_courant]++;
                            nb_success_courant[thread_courant] = 0;

                            //printf("##########################\n");
                            //printf("##########################\n");
                        }

                        if(changement_contexte()){
                            thread_courant = 1 - thread_courant;
                            break;
                        }
                    } else {
                        //printf("case %d - %d - ECHEC\n", iteration_courante[thread_courant], thread_courant);

                        thread_courant = 1 - thread_courant;
                        break;
                    }

                case 2:
                    //printf("case %d - %d\n", iteration_courante[thread_courant], thread_courant);
                    b_isend(main_buffer[thread_courant], _size);

                    nb_packs_courant[thread_courant]++;

                    if(nb_packs_courant[thread_courant] ==  nb_packs){
                        iteration_courante[thread_courant]++;
                        nb_packs_courant[thread_courant] = 0;
                    }

                    if(changement_contexte()){
                        thread_courant = 1 - thread_courant;
                        break;
                    }

                case 3:
                    err = b_test();
                    if(err == NM_ESUCCESS){
                        //printf("case %d - %d - SUCCESS\n", iteration_courante[thread_courant], thread_courant);

                        nb_success_courant[thread_courant]++;

                        if(nb_success_courant[thread_courant] == nb_packs){
                            iteration_courante[thread_courant] = 0;
                            nb_boucles_courant[thread_courant]--;
                            nb_success_courant[thread_courant] = 0;
                        }

                        if(changement_contexte()){
                            thread_courant = 1 - thread_courant;
                            break;
                        }
                    } else {
                        //printf("case %d - %d - ECHEC\n", iteration_courante[thread_courant], thread_courant);

                        thread_courant = 1 - thread_courant;
                        break;
                    }
                }
            }

            if(nb_boucles_courant[0]){
                thread_courant = 0;
            } else if(nb_boucles_courant[1]){
                thread_courant = 1;
            } else {
                continue;
            }

            switch(iteration_courante[thread_courant]){
            case 0:
                for( ; nb_packs_courant[thread_courant] < nb_packs; nb_packs_courant[thread_courant]++){
                    b_irecv(main_buffer[thread_courant], _size);
                }
                nb_packs_courant[thread_courant] = 0;

            case 1:
                while(nb_success_courant[thread_courant] != nb_packs){
                    err = b_test();
                    if(err == NM_ESUCCESS){
                        nb_success_courant[thread_courant]++;
                    }
                }
                nb_success_courant[thread_courant] = 0;

            case 2:
                for( ; nb_packs_courant[thread_courant] < nb_packs;  nb_packs_courant[thread_courant]++){
                    b_isend(main_buffer[thread_courant], _size);
                }
                nb_packs_courant[thread_courant] = 0;

            case 3:
                while(nb_success_courant[thread_courant] != nb_packs){
                    err = b_test();
                    if(err == NM_ESUCCESS){
                        nb_success_courant[thread_courant]++;
                    }
                }
                nb_success_courant[thread_courant] = 0;
            }
            nb_boucles_courant[thread_courant]--;


            while(nb_boucles_courant[thread_courant]){

                for(nb_packs_courant[thread_courant] = 0 ;
                    nb_packs_courant[thread_courant] < nb_packs;
                    nb_packs_courant[thread_courant]++){

                    b_irecv(main_buffer[thread_courant], _size);
                }

                while(nb_success_courant[thread_courant] != nb_packs){
                    err = b_test();
                    if(err == NM_ESUCCESS){
                        nb_success_courant[thread_courant]++;
                    }
                }
                nb_success_courant[thread_courant] = 0;

                for(nb_packs_courant[thread_courant] = 0;
                    nb_packs_courant[thread_courant] < nb_packs;
                    nb_packs_courant[thread_courant]++){
                    b_isend(main_buffer[thread_courant], _size);
                }

                while(nb_success_courant[thread_courant] != nb_packs){
                    err = b_test();
                    if(err == NM_ESUCCESS){
                        nb_success_courant[thread_courant]++;
                    }
                }
                nb_success_courant[thread_courant] = 0;

                nb_boucles_courant[thread_courant]--;
            }
        }
    }
}


int
session_init(int    argc,
             char **argv) {
    char	*r_url	= NULL;
    char	*l_url	= NULL;
    int err;

#if defined CONFIG_SCHED_MINI_ALT
    err = nm_core_init(&argc, argv, &p_core, nm_mini_alt_load);
#elif defined CONFIG_SCHED_OPT
    err = nm_core_init(&argc, argv, &p_core, nm_so_load);
#else
    err = nm_core_init(&argc, argv, &p_core, nm_mini_load);
#endif
    if (err != NM_ESUCCESS) {
        printf("nm_core_init returned err = %d\n", err);
        goto out;
    }

    argc--;
    argv++;

    if (argc) {
        r_url	= *argv;
        printf("running as client using remote url: [%s]\n", r_url);

        argc--;
        argv++;
    } else {
        printf("running as server\n");
    }

    err = nm_core_proto_init(p_core, nm_basic_load, &p_proto);
    if (err != NM_ESUCCESS) {
        printf("nm_core_proto_init returned err = %d\n", err);
        goto out;
    }

#if defined CONFIG_MX
    err = nm_core_driver_load_init(p_core, nm_mx_load, &drv_id, &l_url);
#elif defined CONFIG_GM
    err = nm_core_driver_load_init(p_core, nm_gm_load, &drv_id, &l_url);
#else
    err = nm_core_driver_load_init(p_core, nm_tcp_load, &drv_id, &l_url);
#endif
    if (err != NM_ESUCCESS) {
        printf("nm_core_driver_load_init returned err = %d\n", err);
        goto out;
    }

    err = nm_core_gate_init(p_core, &gate_id);
    if (err != NM_ESUCCESS) {
        printf("nm_core_gate_init returned err = %d\n", err);
        goto out;
    }

    if (!r_url) {
        /* server
         */
        printf("local url: [%s]\n", l_url);

        err = nm_core_gate_accept(p_core, gate_id, drv_id, NULL);
        if (err != NM_ESUCCESS) {
            printf("nm_core_gate_accept returned err = %d\n", err);
            goto out;
        }
    } else {
        /* client
         */
        err = nm_core_gate_connect(p_core, gate_id, drv_id, r_url);
        if (err != NM_ESUCCESS) {
            printf("nm_core_gate_connect returned err = %d\n", err);
            goto out;
        }
    }

    return !!r_url;

 out:
    exit(EXIT_FAILURE);
}

int
main(int    argc,
     char **argv) {
    int is_master;

    is_master	= session_init(argc, argv);

    main_buffer[0] = TBX_MALLOC(param_max_size);
    main_buffer[1] = TBX_MALLOC(param_max_size);

    p_rq_tab[0] = TBX_MALLOC(NB_PACKS * sizeof(struct nm_basic_rq *));
    p_rq_tab[1] = TBX_MALLOC(NB_PACKS * sizeof(struct nm_basic_rq *));

    if (is_master) {
        ping();
    } else {
        pong();
    }

    printf("Exiting\n");

    return 0;
}


