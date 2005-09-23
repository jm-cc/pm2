#include <stdlib.h>
#include <stdio.h>
#include "madeleine.h"


void
submit_send(p_mad_adapter_t adapter,
            p_mad_track_t track,
            p_mad_iovec_t mad_iovec){
    LOG_IN();

    //DISP("-->submit");
    mad_iovec->track = track;
    mad_pipeline_add(adapter->s_track_set->in_more, mad_iovec);


    //DISP("<--submit");

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

    LOG_IN();
    max_nb_pending_iov = s_track_set->max_nb_pending_iov;
    cur_nb_pending_iov = s_track_set->cur_nb_pending_iov;
    s_ready_msg_slist  = adapter->s_ready_msg_list;
    interface = adapter->driver->interface;

    // amorce et remplissage du pipeline
    while(cur_nb_pending_iov < max_nb_pending_iov){

        //DISP("fill_s_pipeline");

        if(s_ready_msg_slist->length){
            //DISP("s_ready_msg_list");
            next = tbx_slist_extract(adapter->s_ready_msg_list);
        } else if(s_track_set->in_more->cur_nb_elm){
            //DISP("in more");
            next = mad_pipeline_remove(s_track_set->in_more);
        } else {
            if(!mad_s_optimize(adapter)){
                goto end;
            }
            next = mad_pipeline_remove(s_track_set->in_more);
        }

        mad_pipeline_add(next->track->pipeline, next);

        //DISP("ISEND");
        interface->isend(next->track,
                         next->remote_rank,
                         next->data,
                         next->total_nb_seg);
        s_track_set->cur_nb_pending_iov++;

        //DISP("next->track->status = MAD_MKP_PROGRESS;");
        next->track->status = MAD_MKP_PROGRESS;

        //recherche de nvx iovec à envoyer
        while(s_track_set->in_more->cur_nb_elm
              < s_track_set->in_more->length){
            if(!mad_s_optimize(adapter))
                goto end;
        }
    }
 end:
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

    LOG_IN();
    //DISP("-->s_mskp");
    interface = adapter->driver->interface;
    s_track_set = adapter->s_track_set;

    // amorce et remplissage du pipeline
    if(!s_track_set->cur_nb_pending_iov){
        mad_fill_s_pipeline(adapter, s_track_set);
    }

    for(i = 0; i < s_track_set->nb_track; i++){
        track = s_track_set->tracks_tab[i];

        if(track->status == MAD_MKP_PROGRESS){
            //DISP("-->s_mskp : mad_make_progress");

            track->status = mad_make_progress(adapter, track);
            status = track->status;

            //DISP_VAL("s_mkp : status", status);

            if(status == MAD_MKP_PROGRESS){
                //DISP("------------>s_mkp: ENVOYÉ");

                mad_iovec = mad_pipeline_remove(track->pipeline);
                s_track_set->cur_nb_pending_iov--;

                // remonte sur zone utilisateur
                mad_iovec_s_check(adapter, mad_iovec);

                // On poste le suivant
                mad_fill_s_pipeline(adapter, s_track_set);
            }
        }
    }


    //// amorce et remplissage du pipeline
    //mad_fill_s_pipeline(adapter, s_track_set);

    //DISP("<--s_mkp");
    LOG_OUT();
}

/*******************************************************/


static void
treat_unexpected(p_mad_adapter_t adapter){
    p_mad_driver_interface_t interface = NULL;
    p_tbx_slist_t unexpected_msg_list  = NULL;
    p_mad_iovec_t cur                  = NULL;
    tbx_bool_t    forward              = tbx_false;
    LOG_IN();
    interface = adapter->driver->interface;
    unexpected_msg_list = adapter->unexpected_msg_list;

    // s'il y a des données en attente
    if(unexpected_msg_list->length){
        tbx_slist_ref_to_head(unexpected_msg_list);

        while(1){
            cur = tbx_slist_ref_get(unexpected_msg_list);
            if(cur == NULL)
                break;

            if(mad_iovec_exploit_msg(adapter, cur->data[0].iov_base)){

                DISP_VAL("treat_unexpected : unpack_list->length", cur->channel->unpacks_list->length);

                // le mad_iovec est entièrement traité,
                // donc retrait de la liste des "unexpected"
                forward = tbx_slist_ref_extract_and_forward(unexpected_msg_list, (void **)cur);

                //if(!tbx_slist_ref_extract_and_forward(unexpected_msg_list, (void **)cur)){
                //    break;
                //}

                // libération du mad_iovec pré_posté
                interface->clean_pre_posted(cur);

                if(!forward)
                    break;

            } else {
                if(!tbx_slist_ref_forward(unexpected_msg_list))
                    break;
            }
        }

        DISP_VAL("<--terat_unexpected", unexpected_msg_list->length);
    }
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

    double chrono_treat_unexpected  = 0.0;
    double chrono_mad_mkp           = 0.0;
    double chrono_add_pre_posted    = 0.0;
    double chrono_exploit_msg       = 0.0;
    double chrono_total             = 0.0;


    LOG_IN();
    //DISP("-->r_mkp");

    driver = adapter->driver;
    interface = driver->interface;
    r_track_set = adapter->r_track_set;

    /*** on traite les unexpected ***/
    TBX_GET_TICK(t1);
    treat_unexpected(adapter);
    TBX_GET_TICK(t2);

    chrono_treat_unexpected = TBX_TIMING_DELAY(t1, t2);

    // pour chaque piste de réception
    for(i = 0; i < r_track_set->nb_track; i++){
        track = r_track_set->tracks_tab[i];

        if(track->status == MAD_MKP_PROGRESS
           || (track->pre_posted)){

            TBX_GET_TICK(t3);
            track->status = mad_make_progress(adapter, track);
            TBX_GET_TICK(t4);

            chrono_mad_mkp          = TBX_TIMING_DELAY(t3, t4);

            status = track->status;

            // si des données sont reçues
            if(status == MAD_MKP_PROGRESS){
                //DISP("r_mkp : RECU");
                mad_iovec = mad_pipeline_remove(track->pipeline);
                r_track_set->cur_nb_pending_iov--;

                if(track->pre_posted){
                    TBX_GET_TICK(t5);
                    // dépot d'une nouvelle zone pré-postée
                    interface->add_pre_posted(adapter,
                                              mad_iovec->track);

                    TBX_GET_TICK(t6);

                    // Traitement des données reçues
                    exploit_msg = mad_iovec_exploit_msg(adapter,
                                                        mad_iovec->data[0].iov_base);
                    TBX_GET_TICK(t7);

                    if(!exploit_msg){
                    //if(!mad_iovec_exploit_msg(adapter,
                    //                          mad_iovec->data[0].iov_base)){

                        // si tout n'a pas pu être traité
                        //DISP("AJOUT dans les liste des UNEXPECTED");
                        tbx_slist_append(adapter->unexpected_msg_list, mad_iovec);



                    } else {
                        //DISP("TRAItÉX");

                        // free du mad_iovec pré_posté
                        interface->clean_pre_posted(mad_iovec);
                    }

                    chrono_add_pre_posted   = TBX_TIMING_DELAY(t5, t6);
                    chrono_exploit_msg      = TBX_TIMING_DELAY(t6, t7);

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

    chrono_total            = TBX_TIMING_DELAY(t2, t8);

    printf("MAD_R_MAKE_PROGRESS : \n");
    if(chrono_treat_unexpected)
        printf("treat_unexpected   --> %9g\n", chrono_treat_unexpected);
    if(chrono_mad_mkp)
        printf("mad_mkp            --> %9g\n", chrono_mad_mkp);
    if(chrono_add_pre_posted)
        printf("add_pre_posted     --> %9g\n", chrono_add_pre_posted);
    if(chrono_exploit_msg)
        printf("exploi_msg         --> %9g\n", chrono_exploit_msg);
    printf("-------------------------------\n");
    printf("total              --> %9g\n", chrono_total);

    LOG_OUT();
    //DISP("<--r_mkp");
}
