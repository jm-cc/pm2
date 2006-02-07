#include <stdlib.h>
#include <stdio.h>
#include "madeleine.h"

p_mad_iovec_t (*get_next_packet)(p_mad_adapter_t) = NULL;

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
        s_track_set->next = get_next_packet(adapter);

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
        s_track_set->next = NULL;

    } else if(s_ready_msg_slist->length){
        s_track_set->cur = tbx_slist_extract(s_ready_msg_slist);

    } else {
        s_track_set->cur = get_next_packet(adapter);

        if(!s_track_set->cur){
            s_track_set->status = MAD_MKP_NOTHING_TO_DO;
            goto end;
        }
    }

    mad_iovec_update_global_header(s_track_set->cur);
    mad_iovec_add_authorized_sendings(s_track_set->cur,
                                      adapter->nb_released_unexpecteds);
    mad_iovec_update_global_header(s_track_set->cur);

    // on considère les packs constituant l'iovec comme envoyé
    driver->nb_packs_to_send -= s_track_set->cur->nb_packs;

    if(driver->nb_packs_to_send)
        mad_search_next(adapter, s_track_set);

    cur = s_track_set->cur;
    interface->isend(cur->track, cur);

    s_track_set->status = MAD_MKP_PROGRESS;

 end:
    LOG_OUT();
}

void
mad_engine_init(p_mad_iovec_t (*get_next_packet_f)(p_mad_adapter_t)) {
    get_next_packet = get_next_packet_f;
}

void
mad_s_make_progress(p_mad_adapter_t adapter){
    p_mad_driver_t driver = NULL;
    p_mad_driver_interface_t interface   = NULL;
    p_mad_track_set_t s_track_set = NULL;
    p_mad_iovec_t cur = NULL;
    mad_mkp_status_t status = MAD_MKP_NO_PROGRESS;
    LOG_IN();
    driver      = adapter->driver;
    interface   = driver->interface;
    s_track_set = adapter->s_track_set;
    cur         = s_track_set->cur;
    status      = s_track_set->status;


    //DISP("-->s_mkp");


    // amorce
    //if(status == MAD_MKP_NOTHING_TO_DO){
    if(!cur){
        mad_send_cur(adapter, s_track_set);
    } else {
        s_track_set->status = s_mad_make_progress(adapter);
        status              = s_track_set->status;

        if(status == MAD_MKP_PROGRESS){
            //DISP("s_mkp : ENVOYE");

            // remontée sur zone utilisateur
            if(cur->need_rdv){
                p_mad_madeleine_t      madeleine   = NULL;
                channel_id_t           channel_id  = 0;
                p_mad_channel_t        channel     = NULL;
                p_mad_connection_t     cnx         = NULL;
                rank_t                 remote_rank = 0;

                //DISP("ENVOI d'un gros");

                channel_id  = cur->channel->id;
                remote_rank = cur->remote_rank;
                madeleine   = mad_get_madeleine();
                channel     = madeleine->channel_tab[channel_id];
                cnx         = tbx_darray_get(channel->out_connection_darray,
                                             remote_rank);

                mad_iovec_get(cnx->packs_list,
                              channel_id,
                              remote_rank,
                              cur->sequence);
                mad_iovec_free(cur);
                s_track_set->cur = NULL;

            } else {
                //DISP("ENVOI d'un petit");
                mad_iovec_s_check(adapter, cur);
                s_track_set->cur = NULL;
                //DISP("s_check OK!");
            }

            // envoi du suivant
            if(driver->nb_packs_to_send){
                mad_send_cur(adapter, s_track_set);
            } else {
                s_track_set->status = MAD_MKP_NOTHING_TO_DO;
            }
        }
    }
    LOG_OUT();
}

/*******************************************************/


static void
treat_unexpected(p_mad_adapter_t adapter){
    p_mad_pipeline_t    unexpecteds = NULL;
    int unexpected_idx             = 0;
    int unexpected_total_nb           = 0;
    p_mad_iovec_t current_unexpected = NULL;

    LOG_IN();
    unexpecteds         = adapter->unexpecteds;
    unexpected_total_nb = unexpecteds->cur_nb_elm;

    for(unexpected_idx = 0; unexpected_idx < unexpected_total_nb; unexpected_idx++){
        //DISP("-->treat_unexpected");

        current_unexpected = mad_pipeline_index_get(unexpecteds, unexpected_idx);

        if(mad_iovec_exploit_msg(adapter, current_unexpected->data[0].iov_base)){
            // le mad_iovec est entièrement traité,
            // donc retrait de la liste des "unexpected"
            mad_pipeline_extract(unexpecteds, unexpected_idx);

            // libération du mad_iovec pré_posté
            mad_pipeline_add(adapter->pre_posted, current_unexpected);
            return;
        }
    }
    LOG_OUT();
}

void
mad_r_make_progress(p_mad_adapter_t adapter){
    p_mad_driver_t           driver         = NULL;
    p_mad_driver_interface_t interface   = NULL;
    p_mad_track_set_t        r_track_set = NULL;
    p_mad_track_t            track = NULL;
    p_mad_iovec_t            mad_iovec   = NULL;
    unsigned int i    = 0, j = 0;
    tbx_bool_t               exploit_msg = tbx_false;
    int nb_track = 0;
    int nb_pending_reception = 0;
    tbx_bool_t *reception_tracks_in_use = NULL;
    LOG_IN();
    driver                  = adapter->driver;
    interface               = driver->interface;
    r_track_set             = adapter->r_track_set;
    nb_track                = r_track_set->nb_track;
    nb_pending_reception    = r_track_set->nb_pending_reception;
    reception_tracks_in_use = r_track_set->reception_tracks_in_use;


    //DISP("r_mkp");


    // on traite les unexpected
    treat_unexpected(adapter);


    // traite que s'il y a des réceptions en cours
    if(r_track_set->status == MAD_MKP_NOTHING_TO_DO)
        goto end;


    // pour chaque piste de réception
    for(i = 0; i < nb_track && j < nb_pending_reception; i++){

        //DISP_VAL("reception_tracks_in_use : i", i);
        //DISP_VAL("in use?", reception_tracks_in_use[i]);

        if(reception_tracks_in_use[i]){
            j++;

            track = r_track_set->tracks_tab[i];
            mad_iovec = r_mad_make_progress(adapter, track);

            // si des données sont reçues
            if(mad_iovec){
                track->pending_reception[mad_iovec->remote_rank] = NULL;

                if(track->pre_posted){
                   //DISP("r_mkp : RECEPTION d'un petit");
                    // dépot d'une nouvelle zone pré-postée
                    interface->replace_pre_posted(adapter, track,
                                                  mad_iovec->remote_rank);

                    // Traitement des données reçues
                    exploit_msg = mad_iovec_exploit_msg(adapter,
                                                        mad_iovec->data[0].iov_base);

                    if(exploit_msg){
                       //DISP("r_mkp : RECEPTION d'un petit TRAITE");
                        // on rend le mad_iovec pré_posté
                        mad_pipeline_add(adapter->pre_posted,
                                         mad_iovec);

                    } else { // unexpected
                       //DISP("r_mkp : RECEPTION d'un petit UNEXPECTED");
                        mad_pipeline_add(adapter->unexpecteds, mad_iovec);
                    }

                } else {
                    //DISP("r_mkp : RECEPTION d'un long");
                    mad_iovec = mad_iovec_get(mad_iovec->channel->unpacks_list,
                                              mad_iovec->channel->id,
                                              mad_iovec->remote_rank,
                                              mad_iovec->sequence);

                    r_track_set->rdv_track->pending_reception[mad_iovec->remote_rank] = NULL;
                    r_track_set->rdv_track->nb_pending_reception--;
                    r_track_set->reception_tracks_in_use[r_track_set->rdv_track->id] = tbx_false;

                    mad_iovec_free(mad_iovec);

                    //DISP_VAL("unpack_list -len", mad_iovec->channel->unpacks_list->length);
                    //DISP("");

                    // cherche un nouveau à envoyer
                    if(adapter->rdv_to_treat->length)
                        mad_iovec_search_rdv(adapter, track);
                }
            }
        }
    }
 end:
    LOG_OUT();
}
