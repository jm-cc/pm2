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
    p_mad_driver_interface_t interface = NULL;
    p_mad_iovec_t next = NULL;

    LOG_IN();
    interface = adapter->driver->interface;

    // amorce et remplissage du pipeline
    while(s_track_set->cur_nb_pending_iov
          < s_track_set->max_nb_pending_iov){

        if(adapter->s_ready_msg_list->length){
            next = tbx_slist_extract(adapter->s_ready_msg_list);
        } else {
            next = mad_pipeline_remove(s_track_set->in_more);
        }

        if(next){
            mad_pipeline_add(next->track->pipeline, next);

            //DISP("ISEND");
            interface->isend(next->track,
                             next->remote_rank,
                             next->data,
                             next->total_nb_seg);
            s_track_set->cur_nb_pending_iov++;

            //recherche de nvx iovec à envoyer
            while(s_track_set->in_more->cur_nb_elm
                  < s_track_set->in_more->length){
                if(!mad_s_optimize(adapter))
                    goto end;
            }


        } else {
            while(s_track_set->in_more->cur_nb_elm
                  < s_track_set->in_more->length){
                if(!mad_s_optimize(adapter))
                    goto end;
            }
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

    for(i = 0; i < s_track_set->nb_track; i++){
        track = s_track_set->tracks_tab[i];

        status = mad_make_progress(adapter, track);

        //DISP_VAL("s_mkp : status", status);

        if(status == MAD_MKP_PROGRESS){
            //DISP("s_mkp : ENVOYE");
            mad_iovec = mad_pipeline_remove(track->pipeline);
            s_track_set->cur_nb_pending_iov--;

            // remonte sur zone utilisateur
            mad_iovec_s_check(adapter, mad_iovec);

            // On poste le suivant
            mad_fill_s_pipeline(adapter, s_track_set);
        }
    }

    // amorce et remplissage du pipeline
    mad_fill_s_pipeline(adapter, s_track_set);

    //DISP("<--s_mkp");
    LOG_OUT();
}


/*******************************************************/


static void
treat_unexpected(p_mad_adapter_t adapter){
    p_mad_driver_interface_t interface = NULL;
    p_tbx_slist_t unexpected_msg_list = NULL;
    p_mad_iovec_t cur = NULL;
    tbx_bool_t   ref_forward = tbx_true;
    uint32_t     idx = 0;
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
                // si le mad_iovec est entièrement traité
                // retrait de la liste des "unexpected"
                idx         = tbx_slist_ref_get_index(unexpected_msg_list);
                ref_forward = tbx_slist_ref_forward(unexpected_msg_list);
                tbx_slist_index_extract(unexpected_msg_list, idx);

                // libération du mad_iovec pré_posté
                interface->clean_pre_posted(cur);

                if(!ref_forward)
                    break;
            } else {
                if(!tbx_slist_ref_forward(unexpected_msg_list))
                    break;
            }
        }
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
    LOG_IN();
    //DISP("-->r_mkp");

    driver = adapter->driver;
    interface = driver->interface;
    r_track_set = adapter->r_track_set;

    // pour chaque piste de réception
    for(i = 0; i < r_track_set->nb_track; i++){
        track = r_track_set->tracks_tab[i];

        status = mad_make_progress(adapter, track);

        // si des données sont reçues
        if(status == MAD_MKP_PROGRESS){
            //DISP("r_mkp : RECU");
            mad_iovec = mad_pipeline_remove(track->pipeline);
            r_track_set->cur_nb_pending_iov--;

            if(track->pre_posted){
                // dépot d'une nouvelle zone pré-postée
                interface->add_pre_posted(adapter,
                                          mad_iovec->track);

                // Traitement des données reçues
                if(!mad_iovec_exploit_msg(adapter,
                                          mad_iovec->data[0].iov_base)){
                    // si tout n'a pas pu être traité
                    //DISP("AJOUT dans les liste des UNEXPECTED");
                    tbx_slist_append(adapter->unexpected_msg_list, mad_iovec);

                    //mad_iovec_print_msg(mad_iovec->data[0].iov_base);
                } else {
                    //DISP("TRAItÉX");
                    // free du mad_iovec pré_posté
                    interface->clean_pre_posted(mad_iovec);
                }
            } else {
                mad_iovec_get(mad_iovec->channel->unpacks_list,
                              mad_iovec->channel->id,
                              mad_iovec->remote_rank,
                              mad_iovec->sequence);
                mad_iovec_free(mad_iovec, tbx_false);

                // cherche un nouveau a envoyer
                //mad_iovec_search_rdv(adapter, track);
            }
        }

        //else if (status == nothing_to_do && !track->pre_posted) {
        //    // cherche un nouveau a envoyer
        //    //mad_iovec_search_rdv(adapter, track);
        //}

    }

    /*** on traite les unexpected ***/
    treat_unexpected(adapter);

    LOG_OUT();
    //DISP("<--r_mkp");
}




















//void
//mad_s_make_progress(p_mad_adapter_t adapter){
//    p_mad_driver_interface_t interface   = NULL;
//    p_mad_track_set_t s_track_set = NULL;
//    p_mad_track_t track = NULL;
//    p_mad_pipeline_t  pipeline    = NULL;
//    p_mad_iovec_t     mad_iovec   = NULL;
//    p_mad_iovec_t next = NULL;
//    tbx_bool_t   sent = tbx_false;
//
//    uint32_t i = 0;
//
//    LOG_IN();
//    //DISP("-->s_mskp");
//    interface = adapter->driver->interface;
//    s_track_set = adapter->s_track_set;
//
//
//    for(i = 0; i < s_track_set->nb_track; i++){
//        track = s_track_set->tracks_tab[i];
//        pipeline = track->pipeline;
//
//        if(!pipeline->cur_nb_elm){
//            goto end;
//        }
//
//        // A REFAIRE de begin à end
//        mad_iovec = mad_pipeline_get(pipeline);
//
//        sent = interface->test(mad_iovec->track);
//        if(sent){
//            //DISP("s_mkp : ENVOYE");
//            mad_pipeline_remove(pipeline);
//            s_track_set->cur_nb_pending_iov--;
//
//            mad_iovec_s_check(adapter, mad_iovec);
//
//            // On poste le suivant
//            if(adapter->s_ready_msg_list->length){
//                next = tbx_slist_extract(adapter->s_ready_msg_list);
//            } else {
//                next = mad_pipeline_remove(s_track_set->in_more);
//            }
//
//            if(next){
//                mad_pipeline_add(next->track->pipeline, next);
//
//                interface->isend(next->track,
//                                 next->remote_rank,
//                                 next->data,
//                                 next->total_nb_seg);
//                s_track_set->cur_nb_pending_iov++;
//
//                //recherche de nvx iovec à envoyer
//                while(s_track_set->in_more->cur_nb_elm
//                      < s_track_set->in_more->length){
//                    if(!mad_s_optimize(adapter))
//                        break;
//                }
//            }
//        }
//    }
//
// end:
//
//    // amorce et remplissage du pipeline
//    while(s_track_set->cur_nb_pending_iov
//          < s_track_set->max_nb_pending_iov){
//
//        if(adapter->s_ready_msg_list->length){
//            next = tbx_slist_extract(adapter->s_ready_msg_list);
//        } else {
//            next = mad_pipeline_remove(s_track_set->in_more);
//        }
//
//        if(next){
//            mad_pipeline_add(next->track->pipeline, next);
//
//            //DISP("ISEND");
//            interface->isend(next->track,
//                             next->remote_rank,
//                             next->data,
//                             next->total_nb_seg);
//            s_track_set->cur_nb_pending_iov++;
//        } else {
//            break;
//        }
//    }
//
//    // recherche de nvx iovec à envoyer
//    while(s_track_set->in_more->cur_nb_elm
//          < s_track_set->in_more->length){
//        if(!mad_s_optimize(adapter))
//            break;
//    }
//
//    //DISP("<--s_mkp");
//    LOG_OUT();
//}
//


//void
//mad_r_make_progress(p_mad_adapter_t adapter){
//    p_mad_driver_t driver         = NULL;
//    p_mad_driver_interface_t interface   = NULL;
//    p_mad_track_set_t r_track_set = NULL;
//    p_mad_track_t track = NULL;
//    p_mad_pipeline_t  pipeline    = NULL;
//    p_mad_iovec_t     mad_iovec   = NULL;
//    tbx_bool_t   received = tbx_false;
//    unsigned int i    = 0;
//    LOG_IN();
//    //DISP("-->r_mkp");
//
//    driver = adapter->driver;
//    interface = driver->interface;
//    r_track_set = adapter->r_track_set;
//
//    // pour chaque piste de réception
//    for(i = 0; i < r_track_set->nb_track; i++){
//        track = r_track_set->tracks_tab[i];
//        pipeline = track->pipeline;
//
//        if(!pipeline->cur_nb_elm){
//            goto end;
//        }
//
//        // A REFAIRE de begin à end
//        mad_iovec = mad_pipeline_get(pipeline);
//
//        received = interface->test(mad_iovec->track);
//        if(received){ // si des données sont reçues
//            //DISP("r_mkp : RECU");
//
//            mad_pipeline_remove(pipeline);
//
//            if(track->pre_posted){
//                // dépot d'une nouvelle zone pré-postée
//                interface->add_pre_posted(adapter,
//                                          mad_iovec->track);
//
//                // Traitement des données reçues
//                if(!mad_iovec_exploit_msg(adapter,
//                                          mad_iovec->data[0].iov_base)){
//                    // si tout n'a pas pu être traité
//                    //DISP("AJOUT dans les liste des UNEXPECTED");
//                    tbx_slist_append(adapter->unexpected_msg_list, mad_iovec);
//
//                    //mad_iovec_print_msg(mad_iovec->data[0].iov_base);
//                } else {
//                   //DISP("TRAItÉX");
//                    // free du mad_iovec pré_posté
//                    interface->clean_pre_posted(mad_iovec);
//                }
//            } else {
//                mad_iovec_get(mad_iovec->channel->unpacks_list,
//                              mad_iovec->channel->id,
//                              mad_iovec->remote_rank,
//                              mad_iovec->sequence);
//                mad_iovec_free(mad_iovec, tbx_false);
//
//                // cherche un nouveau a envoyer
//                //mad_iovec_search_rdv(adapter, track);
//            }
//        }
//    }
// end:
//    /*** on traite les unexpected ***/
//    {
//        p_tbx_slist_t unexpected_msg_list = NULL;
//        p_mad_iovec_t cur = NULL;
//        tbx_bool_t ref_forward = tbx_true;
//        uint32_t idx = 0;
//
//        unexpected_msg_list = adapter->unexpected_msg_list;
//
//        // s'il y a des données en attente
//        if(unexpected_msg_list->length){
//            tbx_slist_ref_to_head(unexpected_msg_list);
//            while(1){
//                cur = tbx_slist_ref_get(unexpected_msg_list);
//                if(cur == NULL)
//                    break;
//
//                if(mad_iovec_exploit_msg(adapter, cur->data[0].iov_base)){
//                    // si le mad_iovec est entièrement traité
//                    // retrait de la liste des "unexpected"
//                    idx         = tbx_slist_ref_get_index(unexpected_msg_list);
//                    ref_forward = tbx_slist_ref_forward(unexpected_msg_list);
//                    tbx_slist_index_extract(unexpected_msg_list, idx);
//
//                    // libération du mad_iovec pré_posté
//                    interface->clean_pre_posted(cur);
//
//                    if(!ref_forward)
//                        break;
//                } else {
//                    if(!tbx_slist_ref_forward(unexpected_msg_list))
//                        break;
//                }
//            }
//        }
//    }
//
//    LOG_OUT();
//    //DISP("<--r_mkp");
//}
