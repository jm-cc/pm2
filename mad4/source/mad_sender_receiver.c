#include <stdlib.h>
#include <stdio.h>
#include "madeleine.h"


int    nb_chronos_fill = 0;
double chrono_fill_pipeline = 0.0;

int    nb_chronos_s_mkp = 0;
double chrono_s_mkp = 0.0;

//double chrono_fill_1_2 = 0.0;
//double chrono_fill_2_3 = 0.0;
//double chrono_fill_1_3 = 0.0;

//int    nb_chronos_s_mkp = 0;
//double chrono_s_mkp_1_2 = 0.0;
//double chrono_s_mkp_2_3 = 0.0;
//double chrono_s_mkp_3_4 = 0.0;
//double chrono_s_mkp_4_5 = 0.0;
//double chrono_s_mkp_5_6 = 0.0;
//double chrono_s_mkp_6_7 = 0.0;
//double chrono_s_mkp_7_8 = 0.0;
//double chrono_s_mkp_8_9 = 0.0;
//double chrono_s_mkp_1_9 = 0.0;
//


int    nb_chronos_r_mkp = 0;
double chrono_add_pre_posted    = 0.0;
double chrono_exploit_msg       = 0.0;
double chrono_r_mkp_toto             = 0.0;



void
submit_send(p_mad_adapter_t adapter,
            p_mad_track_t track,
            p_mad_iovec_t mad_iovec){
    LOG_IN();
    mad_iovec->track = track;
    mad_pipeline_add(adapter->s_track_set->in_more, mad_iovec);
    LOG_OUT();
}


static void
mad_fill_s_pipeline(p_mad_adapter_t adapter,
                    p_mad_track_set_t s_track_set){
    uint32_t max_nb_pending_iov = 0;
    uint32_t cur_nb_pending_iov = 0;
    p_tbx_slist_t s_ready_msg_slist = NULL;
    p_mad_driver_interface_t interface = NULL;
    p_mad_iovec_t next = NULL;

    tbx_tick_t tick_debut;
    tbx_tick_t tick_fin;

    LOG_IN();
    nb_chronos_fill++;
    TBX_GET_TICK(tick_debut);

    max_nb_pending_iov = s_track_set->max_nb_pending_iov;
    cur_nb_pending_iov = s_track_set->cur_nb_pending_iov;
    s_ready_msg_slist  = adapter->s_ready_msg_list;
    interface = adapter->driver->interface;

    // amorce et remplissage du pipeline
    while(cur_nb_pending_iov < max_nb_pending_iov){
        if(s_ready_msg_slist->length){
            next = tbx_slist_extract(adapter->s_ready_msg_list);
        } else if(s_track_set->in_more->cur_nb_elm){
            next = mad_pipeline_remove(s_track_set->in_more);
        } else {
            if(!mad_s_optimize(adapter)){
                goto end;
            }
            next = mad_pipeline_remove(s_track_set->in_more);
        }
        mad_pipeline_add(next->track->pipeline, next);

        interface->isend(next->track,
                         next->remote_rank,
                         next->data,
                         next->total_nb_seg);
        s_track_set->cur_nb_pending_iov++;

        next->track->status = MAD_MKP_PROGRESS;

        //recherche de nvx iovec à envoyer
        while(s_track_set->in_more->cur_nb_elm
              < s_track_set->in_more->length){
            if(!mad_s_optimize(adapter))
                goto end;
        }
    }
 end:

    TBX_GET_TICK(tick_fin);
    chrono_fill_pipeline += TBX_TIMING_DELAY(tick_debut, tick_fin);
    //chrono_fill_1_2 += TBX_TIMING_DELAY(t1, t2);
    //chrono_fill_2_3 += TBX_TIMING_DELAY(t2, t3);
    //chrono_fill_1_3 += TBX_TIMING_DELAY(t1, t3);


    LOG_OUT();
}



void
mad_s_make_progress(p_mad_adapter_t adapter){
    p_mad_driver_interface_t interface   = NULL;
    p_mad_track_set_t s_track_set = NULL;
    p_mad_track_t track = NULL;
    mad_mkp_status_t status = MAD_MKP_NO_PROGRESS;
    p_mad_iovec_t mad_iovec = NULL;
    uint32_t i = 0;

    tbx_tick_t tick_debut;
    tbx_tick_t tick_fin;
    LOG_IN();

    nb_chronos_s_mkp++;
    TBX_GET_TICK(tick_debut);

    interface = adapter->driver->interface;
    s_track_set = adapter->s_track_set;

    // amorce et remplissage du pipeline
    if(!s_track_set->cur_nb_pending_iov){
        mad_fill_s_pipeline(adapter, s_track_set);
    }

    //TBX_GET_TICK(apres_remplissage);

    for(i = 0; i < s_track_set->nb_track; i++){
        track = s_track_set->tracks_tab[i];

        if(track->status == MAD_MKP_PROGRESS){
            track->status = mad_make_progress(adapter, track);
            status = track->status;

            if(status == MAD_MKP_PROGRESS){
                mad_iovec = mad_pipeline_remove(track->pipeline);
                s_track_set->cur_nb_pending_iov--;

                // remonte sur zone utilisateur
                mad_iovec_s_check(adapter, mad_iovec);

                // On poste le suivant
                mad_fill_s_pipeline(adapter, s_track_set);
            }
        }
    }
    TBX_GET_TICK(tick_fin);
    chrono_s_mkp += TBX_TIMING_DELAY(tick_debut, tick_fin);
    LOG_OUT();
}

/*******************************************************/


static void
treat_unexpected(void){
    p_mad_madeleine_t madeleine        = NULL;
    int nb_channels = 0;
    p_mad_channel_t channel = NULL;
    p_mad_adapter_t adapter            = NULL;
    p_mad_driver_t driver = NULL;
    p_mad_driver_interface_t interface = NULL;
    p_tbx_slist_t unexpected_msg_list  = NULL;
    p_mad_iovec_t cur                  = NULL;
    tbx_bool_t    forward              = tbx_false;
    int i = 0;
    int unexpected_recovery_threshold = 0;
    LOG_IN();

//    madeleine = mad_get_madeleine();
//    nb_channels = madeleine->nb_channels;
//
//    for(i = 0 ; i < nb_channels; i++){
//        channel = madeleine->channel_tab[i];
//        adapter = channel->adapter;
//        driver = adapter->driver;
//        interface = driver->interface;
//        unexpected_msg_list = channel->unexpected;
//        unexpected_recovery_threshold = driver->unexpected_recovery_threshold;
//
//        if(channel->blocked &&
//           unexpected_msg_list->length < unexpected_recovery_threshold){
//            // envoi d'un message de déblocage
//
//        } else if(unexpected_msg_list->length){ // s'il y a des données en attente
//            tbx_slist_ref_to_head(unexpected_msg_list);
//
//            while(1){
//                cur = tbx_slist_ref_get(unexpected_msg_list);
//                if(cur == NULL)
//                    break;
//
//                if(mad_iovec_exploit_msg(adapter, cur->data[0].iov_base)){
//                    // le mad_iovec est entièrement traité,
//                    // donc retrait de la liste des "unexpected"
//                    forward = tbx_slist_ref_extract_and_forward(unexpected_msg_list, (void **)cur);
//
//                    // libération du mad_iovec pré_posté
//                    interface->clean_pre_posted(cur);
//
//                    if(!forward)
//                        break;
//
//                } else {
//                    if(!tbx_slist_ref_forward(unexpected_msg_list))
//                        break;
//                }
//            }
//        }
//    }
    LOG_OUT();
}


void
mad_r_make_progress(p_mad_adapter_t adapter){
    p_mad_driver_t driver         = NULL;
    p_mad_driver_interface_t interface   = NULL;
    p_mad_track_set_t r_track_set = NULL;
    p_mad_track_t track = NULL;
    mad_mkp_status_t status = MAD_MKP_NO_PROGRESS;
    p_mad_iovec_t     mad_iovec   = NULL;
    unsigned int i    = 0;
    tbx_bool_t   exploit_msg = tbx_false;

    tbx_tick_t        t1;
    tbx_tick_t        t2;
    tbx_tick_t        t3;
    tbx_tick_t        t4;
    tbx_tick_t        t5;
    tbx_tick_t        t6;
    tbx_tick_t        t7;
    tbx_tick_t        t8;



    LOG_IN();

    //nb_chronos_r_mkp++;

    //DISP("-->r_mkp");

    driver = adapter->driver;
    interface = driver->interface;
    r_track_set = adapter->r_track_set;

    /*** on traite les unexpected ***/
    //TBX_GET_TICK(t1);
    //treat_unexpected();
    TBX_GET_TICK(t2);
    //chrono_treat_unexpected = TBX_TIMING_DELAY(t1, t2);

    // pour chaque piste de réception
    for(i = 0; i < r_track_set->nb_track; i++){
        track = r_track_set->tracks_tab[i];

        if(track->status == MAD_MKP_PROGRESS
           || (track->pre_posted)){

            TBX_GET_TICK(t3);
            track->status = mad_make_progress(adapter, track);
            TBX_GET_TICK(t4);
            //chrono_mad_mkp          += TBX_TIMING_DELAY(t3, t4);

            status = track->status;

            // si des données sont reçues
            if(status == MAD_MKP_PROGRESS){
                //DISP("r_mkp : RECU");
                mad_iovec = mad_pipeline_remove(track->pipeline);
                r_track_set->cur_nb_pending_iov--;

                if(track->pre_posted){
                    nb_chronos_r_mkp++;
                    TBX_GET_TICK(t5);

                    // dépot d'une nouvelle zone pré-postée
                    interface->add_pre_posted(adapter,
                                              mad_iovec->track);

                    TBX_GET_TICK(t6);

                    // Traitement des données reçues
                    exploit_msg = mad_iovec_exploit_msg(adapter,
                                                        mad_iovec->data[0].iov_base);
                    TBX_GET_TICK(t7);

                    if(exploit_msg){
                        // on rend le mad_iovec pré_posté
                        mad_pipeline_add(adapter->pre_posted,
                                         mad_iovec);
                    }

                    chrono_add_pre_posted   += TBX_TIMING_DELAY(t5, t6);
                    chrono_exploit_msg      += TBX_TIMING_DELAY(t6, t7);

                } else {
                    mad_iovec_get(mad_iovec->channel->unpacks_list,
                                  mad_iovec->channel->id,
                                  mad_iovec->remote_rank,
                                  mad_iovec->sequence);

                    mad_iovec_free(mad_iovec, tbx_false);

                    // cherche un nouveau a envoyer
                    //mad_iovec_search_rdv(adapter, track);
                }



            //else if (status == nothing_to_do && !track->pre_posted) {
            //    // cherche un nouveau a envoyer
            //    //mad_iovec_search_rdv(adapter, track);
            //}
            }

        }
    }

    TBX_GET_TICK(t8);

    //hrono_r_mkp            += TBX_TIMING_DELAY(t2, t8);

    LOG_OUT();
    //DISP("<--r_mkp");
}
