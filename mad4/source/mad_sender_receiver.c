#include <stdlib.h>
#include <stdio.h>
#include "madeleine.h"


int    nb_send_cur = 0;
double chrono_send_cur = 0.0;
double chrono_send_cur_isend = 0.0;

int    nb_send_next = 0;
double chrono_send_next = 0.0;

int    nb_chronos_s_mkp = 0;
double chrono_s_mkp = 0.0;

p_mad_iovec_t (*get_next_packet)(p_mad_adapter_t) = NULL;

static void
mad_search_next(p_mad_adapter_t adapter,
                    p_mad_track_set_t s_track_set){
    p_mad_driver_t           driver            = NULL;
    p_tbx_slist_t            s_ready_msg_slist = NULL;
    p_mad_driver_interface_t interface         = NULL;

    tbx_tick_t t1, t2;
    LOG_IN();
    TBX_GET_TICK(t1);
    driver             = adapter->driver;
    s_ready_msg_slist  = adapter->s_ready_msg_list;
    interface          = driver->interface;

    if(s_ready_msg_slist->length){
        s_track_set->next = tbx_slist_extract(s_ready_msg_slist);

    } else {
        s_track_set->next = get_next_packet(adapter);

    }

    TBX_GET_TICK(t2);
    nb_send_next++;
    chrono_send_next += TBX_TIMING_DELAY(t1, t2);
    LOG_OUT();
}

static void
mad_send_cur(p_mad_adapter_t adapter,
             p_mad_track_set_t s_track_set){
    p_mad_driver_t           driver            = NULL;
    p_tbx_slist_t            s_ready_msg_slist = NULL;
    p_mad_driver_interface_t interface         = NULL;
    p_mad_iovec_t cur = NULL;

    tbx_tick_t t1, t2, t3;
    LOG_IN();
    driver             = adapter->driver;
    s_ready_msg_slist  = adapter->s_ready_msg_list;
    interface          = driver->interface;

    TBX_GET_TICK(t1);
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
    driver->nb_pack_to_send -= s_track_set->cur->nb_packs;

    if(driver->nb_pack_to_send)
        mad_search_next(adapter, s_track_set);

    cur = s_track_set->cur;

    TBX_GET_TICK(t2);

    //mad_iovec_print(cur);

    interface->isend(cur->track,
                     cur->remote_rank,
                     cur->data,
                     cur->total_nb_seg);
    s_track_set->status = MAD_MKP_PROGRESS;

    TBX_GET_TICK(t3);
    nb_send_cur++;
    chrono_send_cur += TBX_TIMING_DELAY(t1, t3);
    chrono_send_cur_isend += TBX_TIMING_DELAY(t2, t3);

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
    p_mad_track_t track = NULL;
    mad_mkp_status_t status = MAD_MKP_NO_PROGRESS;
    LOG_IN();
    driver      = adapter->driver;
    interface   = driver->interface;
    s_track_set = adapter->s_track_set;
    cur         = s_track_set->cur;
    status      = s_track_set->status;

    // amorce
    if(status == MAD_MKP_NOTHING_TO_DO){
        mad_send_cur(adapter, s_track_set);
    } else {
        track  = cur->track;
        status = s_track_set->status;

        s_track_set->status = mad_make_progress(adapter, track);
        status              = s_track_set->status;

        if(status == MAD_MKP_PROGRESS){
            //DISP("ENVOYE");

            // remont�e sur zone utilisateur
            if(cur->need_rdv){
                p_mad_madeleine_t      madeleine   = NULL;
                channel_id_t           channel_id  = 0;
                p_mad_channel_t        channel     = NULL;
                p_mad_connection_t     cnx         = NULL;
                rank_t                 remote_rank = 0;

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
                mad_iovec_s_check(adapter, cur);
                s_track_set->cur = NULL;
            }

            // envoi du suivant
            if(driver->nb_pack_to_send){
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
    p_mad_pipeline_t    unexpected = NULL;
    int cur_nb_unexpected             = 0;
    int unexpected_total_nb           = 0;

    p_mad_iovec_t            cur            = NULL;


    LOG_IN();
    unexpected_total_nb = adapter->unexpected_total_nb;
    unexpected          = adapter->unexpected;

    while(cur_nb_unexpected < unexpected_total_nb){
        cur = mad_pipeline_index_get(unexpected, cur_nb_unexpected);

        if(mad_iovec_exploit_msg(adapter, cur->data[0].iov_base)){
            // le mad_iovec est enti�rement trait�,
            // donc retrait de la liste des "unexpected"
            mad_pipeline_extract(unexpected, cur_nb_unexpected);

            // lib�ration du mad_iovec pr�_post�
            mad_pipeline_add(adapter->pre_posted, cur);

            break;
        }
        cur_nb_unexpected++;
    }
    LOG_OUT();
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
    treat_unexpected(adapter);

    if(r_track_set->status == MAD_MKP_NOTHING_TO_DO)
        goto end;

    // pour chaque piste de r�ception
    for(i = 0; i < 2 && j < r_track_set->nb_pending; i++){
        mad_iovec = r_track_set->reception_curs[i];
        if(mad_iovec){
            j++;

            track               = mad_iovec->track;
            r_track_set->status = mad_make_progress(adapter, track);
            status              = r_track_set->status;

            // si des donn�es sont re�ues
            if(status == MAD_MKP_PROGRESS){
                r_track_set->reception_curs[i] = NULL;

                if(track->pre_posted){
                    //DISP("RECEPTION d'un petit");

                    // d�pot d'une nouvelle zone pr�-post�e
                    interface->add_pre_posted(adapter,
                                              r_track_set);
                    // Traitement des donn�es re�ues
                    exploit_msg = mad_iovec_exploit_msg(adapter,
                                                        mad_iovec->data[0].iov_base);

                    if(exploit_msg){
                        // on rend le mad_iovec pr�_post�
                        mad_pipeline_add(adapter->pre_posted,
                                         mad_iovec);
                    } else { // unexpected
                        mad_pipeline_add(adapter->unexpected, mad_iovec);
                    }

                } else {
                    mad_iovec = mad_iovec_get(mad_iovec->channel->unpacks_list,
                                              mad_iovec->channel->id,
                                              mad_iovec->remote_rank,
                                              mad_iovec->sequence);
                    mad_iovec_free(mad_iovec);

                    // cherche un nouveau � envoyer
                    if(adapter->rdv->length)
                        mad_iovec_search_rdv(adapter, track);
                }
            }
        }
    }
 end:
    LOG_OUT();
}

