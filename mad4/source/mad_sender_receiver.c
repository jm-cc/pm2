#include <stdlib.h>
#include <stdio.h>
#include "madeleine.h"


int    nb_chronos_fill = 0;
int    nb_fill_echec = 0;
double chrono_fill_1_2 = 0.0;
double chrono_fill_2_3 = 0.0;
double chrono_fill_3_4 = 0.0;
double chrono_fill_1_4 = 0.0;

int    nb_chronos_s_mkp = 0;
double chrono_s_mkp = 0.0;


static void
mad_search_next(p_mad_adapter_t adapter,
                    p_mad_track_set_t s_track_set){
    p_mad_driver_t           driver            = NULL;
    p_tbx_slist_t            s_ready_msg_slist = NULL;
    p_mad_driver_interface_t interface         = NULL;
    LOG_IN();
    driver             = adapter->driver;
    s_ready_msg_slist  = adapter->s_ready_msg_list;
    interface          = driver->interface;

    if(s_ready_msg_slist->length){
        s_track_set->next = tbx_slist_extract(s_ready_msg_slist);

    } else {
        s_track_set->next = mad_s_optimize(adapter);

    }
    LOG_OUT();
}

static void
mad_send_cur(p_mad_adapter_t adapter,
             p_mad_track_set_t s_track_set){
    p_mad_driver_t           driver            = NULL;
    p_tbx_slist_t            s_ready_msg_slist = NULL;
    p_mad_driver_interface_t interface         = NULL;
    p_mad_iovec_t cur = NULL;
    LOG_IN();
    driver             = adapter->driver;
    s_ready_msg_slist  = adapter->s_ready_msg_list;
    interface          = driver->interface;

    if(s_track_set->next){
        s_track_set->cur = s_track_set->next;

        mad_search_next(adapter, s_track_set);

    } else if(s_ready_msg_slist->length){
        s_track_set->cur = tbx_slist_extract(s_ready_msg_slist);

        mad_search_next(adapter, s_track_set);
    }else {
        s_track_set->cur = mad_s_optimize(adapter);

        if(!s_track_set->cur){
            goto end;
        }
        mad_search_next(adapter, s_track_set);
    }

    cur = s_track_set->cur;

    interface->isend(cur->track,
                     cur->remote_rank,
                     cur->data,
                     cur->total_nb_seg);

    driver->nb_pack_to_send -= cur->nb_packs;
    cur->track->status = MAD_MKP_PROGRESS;
 end:
    LOG_OUT();
}

void
mad_s_make_progress(p_mad_adapter_t adapter){
    p_mad_driver_t driver = NULL;
    p_mad_driver_interface_t interface   = NULL;
    p_mad_track_set_t s_track_set = NULL;
    p_mad_iovec_t cur = NULL;
    p_mad_track_t track = NULL;
    mad_mkp_status_t status = MAD_MKP_NO_PROGRESS;
    LOG_IN();
    driver   = adapter->driver;
    interface = driver->interface;
    s_track_set = adapter->s_track_set;
    cur = s_track_set->cur;


    // amorce
    if(!cur && driver->nb_pack_to_send){
        mad_send_cur(adapter, s_track_set);
    } else {
        track = cur->track;
        status = track->status;

        if(status == MAD_MKP_PROGRESS){
            track->status = mad_make_progress(adapter, track);
            status = track->status;

            if(status == MAD_MKP_PROGRESS){
                // remonte sur zone utilisateur
                mad_iovec_s_check(adapter, cur);
                // envoi du suivant
                mad_send_cur(adapter, s_track_set);
            }
        }
    }
    LOG_OUT();
}

/*******************************************************/


static void
treat_unexpected(void){
    //p_mad_madeleine_t madeleine        = NULL;
    //int nb_channels = 0;
    //p_mad_channel_t channel = NULL;
    //p_mad_adapter_t adapter            = NULL;
    //p_mad_driver_t driver = NULL;
    //p_mad_driver_interface_t interface = NULL;
    //p_tbx_slist_t unexpected_msg_list  = NULL;
    //p_mad_iovec_t cur                  = NULL;
    //tbx_bool_t    forward              = tbx_false;
    //int i = 0;
    //int unexpected_recovery_threshold = 0;
    //LOG_IN();
    //
    //madeleine = mad_get_madeleine();
    //nb_channels = madeleine->nb_channels;
    //
    //for(i = 0 ; i < nb_channels; i++){
    //    channel = madeleine->channel_tab[i];
    //    adapter = channel->adapter;
    //    driver = adapter->driver;
    //    interface = driver->interface;
    //    unexpected_msg_list = channel->unexpected;
    //    unexpected_recovery_threshold = driver->unexpected_recovery_threshold;
    //
    //    if(channel->blocked &&
    //       unexpected_msg_list->length < unexpected_recovery_threshold){
    //        // envoi d'un message de déblocage
    //
    //    } else if(unexpected_msg_list->length){ // s'il y a des données en attente
    //        tbx_slist_ref_to_head(unexpected_msg_list);
    //
    //        while(1){
    //            cur = tbx_slist_ref_get(unexpected_msg_list);
    //            if(cur == NULL)
    //                break;
    //
    //            if(mad_iovec_exploit_msg(adapter, cur->data[0].iov_base)){
    //                // le mad_iovec est entièrement traité,
    //                // donc retrait de la liste des "unexpected"
    //                forward = tbx_slist_ref_extract_and_forward(unexpected_msg_list, (void **)cur);
    //
    //                // libération du mad_iovec pré_posté
    //                interface->clean_pre_posted(cur);
    //
    //                if(!forward)
    //                    break;
    //
    //            } else {
    //                if(!tbx_slist_ref_forward(unexpected_msg_list))
    //                    break;
    //            }
    //        }
    //    }
    //}
    //LOG_OUT();
}

void
mad_r_make_progress(p_mad_adapter_t adapter){
    p_mad_driver_t           driver         = NULL;
    p_mad_driver_interface_t interface   = NULL;
    p_mad_track_set_t        r_track_set = NULL;
    p_mad_track_t            track = NULL;
    mad_mkp_status_t         status = MAD_MKP_NO_PROGRESS;
    p_mad_iovec_t            mad_iovec   = NULL;
    unsigned int i    = 0, j = 0;
    tbx_bool_t               exploit_msg = tbx_false;
    LOG_IN();
    driver = adapter->driver;
    interface = driver->interface;
    r_track_set = adapter->r_track_set;

    // on traite les unexpected
    treat_unexpected();

    // pour chaque piste de réception
    for(i = 0; i < 2 && j < r_track_set->nb_pending; i++){
        mad_iovec = r_track_set->reception_curs[i];
        if(mad_iovec){
            j++;

            track  = mad_iovec->track;
            status = track->status;

            if(status == MAD_MKP_PROGRESS || (track->pre_posted)){
                track->status = mad_make_progress(adapter, track);
                status = track->status;

                // si des données sont reçues
                if(status == MAD_MKP_PROGRESS){
                    if(track->pre_posted){
                        // dépot d'une nouvelle zone pré-postée
                        interface->add_pre_posted(adapter,
                                                  r_track_set,
                                                  track);
                        // Traitement des données reçues
                        exploit_msg = mad_iovec_exploit_msg(adapter,
                                                            mad_iovec->data[0].iov_base);
                        if(exploit_msg){
                            // on rend le mad_iovec pré_posté
                            mad_pipeline_add(adapter->pre_posted,
                                             mad_iovec);
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

                    //else if (status == nothing_to_do && !track->pre_posted) {
                    //    // cherche un nouveau a envoyer
                    //    //mad_iovec_search_rdv(adapter, track);
                    //}
                }
            }
        }
    }
    LOG_OUT();
}

