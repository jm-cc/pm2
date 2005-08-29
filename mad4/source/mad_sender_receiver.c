#include <stdlib.h>
#include <stdio.h>
#include "madeleine.h"


void
submit_send(p_mad_adapter_t adapter,
            p_mad_track_t track,
            p_mad_iovec_t mad_iovec){
    p_tbx_slist_t pipeline = NULL;
    LOG_IN();

    pipeline = adapter->s_track_set->pipeline;

    mad_iovec->track = track;

    tbx_slist_append(pipeline, mad_iovec);
    adapter->s_track_set->pipeline_nb_elm++;

    LOG_OUT();
}


void
mad_s_make_progress(p_mad_adapter_t adapter){
    p_mad_driver_interface_t interface   = NULL;
    p_mad_track_set_t s_track_set = NULL;
    p_tbx_slist_t  pipeline    = NULL;
    p_mad_iovec_t     mad_iovec   = NULL;
    tbx_bool_t   sent = tbx_false;
    LOG_IN();

    interface = adapter->driver->interface;
    s_track_set = adapter->s_track_set;
    pipeline = s_track_set->pipeline;

    if(!pipeline->length){
        goto fill;
    }

    //DISP("----------------------------");
    //DISP_VAL("-->s_mkp : pipeline_nb_elm", s_track_set->pipeline_nb_elm);
    //DISP_VAL("s_pipeline->length", pipeline->length);
    //DISP("----------------------------");

    mad_iovec = tbx_slist_index_get(pipeline, 0);

    // le pipeline n'est pas amorcé mais il contient des packs à envoyer
    if(!s_track_set->started){
        interface->isend(mad_iovec->track,
                         mad_iovec->remote_rank,
                         mad_iovec->data,
                         mad_iovec->total_nb_seg);

        s_track_set->started = tbx_true;

    } else { // le pipeline est amorcé
        sent = interface->test(mad_iovec->track);

        if(sent){
            //DISP("s_mkp : ENVOYE");

            tbx_slist_extract(pipeline);
            s_track_set->pipeline_nb_elm--;
            mad_iovec_s_check(adapter, mad_iovec);


            // On poste le suivant en attente
            if(!pipeline->length){
                // le pipeline est vide
                s_track_set->started = tbx_false;
            } else {
                mad_iovec = tbx_slist_index_get(pipeline, 0);

                interface->isend(mad_iovec->track,
                                 mad_iovec->remote_rank,
                                 mad_iovec->data,
                                 mad_iovec->total_nb_seg);
            }
        }

    }

 fill:
    while(s_track_set->pipeline_nb_elm
          < s_track_set->pipeline_size_max){
        if(!mad_s_optimize(adapter))
            break;
    }

    if(!s_track_set->pipeline_nb_elm){
        s_track_set->started = tbx_false;
    }
    //DISP("<--s_mkp");
    LOG_OUT();
}


void
mad_r_make_progress(p_mad_adapter_t adapter){
    p_mad_driver_t driver         = NULL;
    p_mad_driver_interface_t interface   = NULL;
    p_mad_track_set_t r_track_set = NULL;
    p_tbx_slist_t  pipeline    = NULL;
    p_mad_iovec_t     mad_iovec   = NULL;
    tbx_bool_t   received = tbx_false;
    unsigned int i    = 0;
    LOG_IN();

    driver = adapter->driver;
    interface = driver->interface;
    r_track_set = adapter->r_track_set;
    pipeline = r_track_set->pipeline;

    for(i = 0; i < r_track_set->pipeline_nb_elm; i++){
        mad_iovec = tbx_slist_index_get(pipeline, i);
        received = interface->test(mad_iovec->track);

        if(received){
            //DISP("r_mkp : RECU");

            tbx_slist_index_extract(pipeline, i);
            r_track_set->pipeline_nb_elm--;

            if(mad_iovec->track->pre_posted){
                interface->add_pre_posted(adapter,
                                          mad_iovec->track);
                if(!mad_iovec_exploit_msg(adapter,
                                          mad_iovec->data[0].iov_base)){
                    tbx_slist_append(adapter->unexpected_msg_list, mad_iovec);
                } else {
                    interface->clean_pre_posted(mad_iovec);
                }

            } else {
                mad_iovec_get(mad_iovec->channel->unpacks_list,
                              mad_iovec->channel->id,
                              mad_iovec->remote_rank,
                              mad_iovec->sequence);
                mad_iovec_free(mad_iovec, tbx_false);
            }
        }
    }

    /*** on remplit le pipeline ***/
    if(r_track_set->pipeline_nb_elm < r_track_set->pipeline_size_max
       && adapter->r_ready_msg_list->length){
        mad_iovec = tbx_slist_extract(adapter->r_ready_msg_list);

        tbx_slist_append(pipeline, mad_iovec);
        interface->irecv(mad_iovec->track,
                         mad_iovec->data,
                         mad_iovec->total_nb_seg);
        r_track_set->pipeline_nb_elm++;
    }


    /*** on traite les unexpected ***/
    {
        p_tbx_slist_t unexpected_msg_list = NULL;
        p_mad_iovec_t cur = NULL;
        tbx_bool_t ref_forward = tbx_true;
        uint32_t idx = 0;

        unexpected_msg_list = adapter->unexpected_msg_list;

        if(unexpected_msg_list->length){
            tbx_slist_ref_to_head(unexpected_msg_list);
            while(1){
                cur = tbx_slist_ref_get(unexpected_msg_list);
                if(cur == NULL)
                    break;

                //DISP("UNEXPECTED");

                if(mad_iovec_exploit_msg(adapter, cur->data[0].iov_base)){
                    // si epuisé, le retirer de la liste des unexpected
                    idx = tbx_slist_ref_get_index(unexpected_msg_list);
                    ref_forward = tbx_slist_ref_forward(unexpected_msg_list);
                    tbx_slist_index_extract(unexpected_msg_list, idx);

                    // FREE!!!!!!!
                    //mad_iovec_free(cur, tbx_true);
                    interface->clean_pre_posted(mad_iovec);

                    if(!ref_forward)
                        break;
                } else {
                    if(!tbx_slist_ref_forward(unexpected_msg_list))
                        break;
                }
            }
        }
    }
    LOG_OUT();
}

































//void
//mad_s_make_progress(p_mad_driver_t driver,
//                    p_mad_adapter_t adapter){
//    p_mad_driver_interface_t interface = NULL;
//    //p_mad_adapter_t   adapter   = NULL;
//    LOG_IN();
//    //DISP("-->s_mkp");
//    //adapter =  tbx_htable_get(driver->adapter_htable, "default");
//
//    interface = driver->interface;
//    interface->s_make_progress(adapter);
//    LOG_OUT();
//    //DISP("<--s_mkp");
//}
//
//
//
//void
//mad_r_make_progress(p_mad_driver_t driver,
//                    p_mad_adapter_t adapter,
//                    p_mad_channel_t channel){
//    p_mad_driver_interface_t interface = NULL;
//    //p_mad_adapter_t   adapter   = NULL;
//    LOG_IN();
//    //DISP("-->r_mkp");
//    //adapter =  tbx_htable_get(driver->adapter_htable, "default");
//
//    interface = driver->interface;
//    interface->r_make_progress(adapter, channel);
//    LOG_OUT();
//    //DISP("<--r_mkp");
//}










// MX
//void
//mad_mx_s_make_progress(p_mad_adapter_t adapter){
//    p_mad_connection_t            connection = NULL;
//    p_mad_track_t                 s_track = NULL;
//    p_mad_pipeline_t              s_pipeline = NULL;
//    p_mad_on_going_t              s_og     = NULL;
//    p_mad_mx_on_going_specific_t  s_og_s     = NULL;
//    unsigned int                  status      = -1;
//
//    LOG_IN();
//    s_track    = adapter->s_track;
//    s_pipeline = s_track->pipelines[0];
//
//    if(s_pipeline->index_first == -1){
//        goto fill;
//    }
//
//    s_og = s_pipeline->on_goings[s_pipeline->index_first];
//    s_og_s = s_og->specific;
//
//    status = mad_mx_test(s_og_s->endpoint, s_og_s->mx_request);
//    if(!status){
//        return;
//    }
//
//    connection = s_og->lnk->connection;
//
//    if(connection->packs_list){
//        if(s_og->mad_iovec->area_nb_seg[s_og->area_id + 1] == 0){
//            if(s_og->mad_iovec->length > MAD_MX_SMALL_MSG_SIZE){
//                mad_iovec_get(connection->packs_list,
//                              s_og->mad_iovec->channel_id,
//                              s_og->mad_iovec->remote_rank,
//                              s_og->mad_iovec->sequence);
//                //*DISP_VAL("mad_mx_s_mkp : EXTRACTION : cnx->packs_list -len", tbx_slist_get_length(connection->packs_list));
//            } else {
//                mad_iovec_s_check(connection->packs_list,
//                                  s_og->mad_iovec->remote_rank,
//                                  MAD_MX_SMALL_MSG_SIZE,
//                                  s_og->mad_iovec);
//            }
//
//            if(s_og->mad_iovec->length < MAD_MX_SMALL_MSG_SIZE){
//                mad_iovec_free(s_og->mad_iovec, tbx_true);
//            } else {
//                mad_iovec_free(s_og->mad_iovec, tbx_false);
//            }
//        }
//    }
//
//    if(s_pipeline->index_first == s_pipeline->index_last){
//        s_pipeline->index_first = -1;
//        s_pipeline->index_last = s_pipeline->index_first;
//    } else {
//        s_pipeline->index_first = s_pipeline->index_last;
//    }
//
// fill:
//    fill_and_send(adapter, s_pipeline);
//    LOG_OUT();
//}
//
//
//static
//void
//fill_and_post_reception(p_mad_adapter_t adapter,
//                        p_mad_pipeline_t pipeline){
//    p_mad_iovec_t     mad_iovec    = NULL;
//    p_mad_on_going_t *on_goings        = NULL;
//    p_mad_on_going_t  last_og       = NULL;
//    unsigned int      nb_elm     = -1;
//    unsigned int      i = 0;
//    p_mad_mx_on_going_specific_t ogs = NULL;
//    p_tbx_slist_t receiving_list = NULL;
//    p_mad_mx_adapter_specific_t as = NULL;
//    p_mad_channel_t    channel = NULL;
//    p_mad_connection_t connection = NULL;
//    int idx_first = -1;
//    int idx_last = -1;
//
//    LOG_IN();
//    on_goings        = pipeline->on_goings;
//    nb_elm           = pipeline->nb_elm;
//    receiving_list   = adapter->receiving_list;
//    as = adapter->specific;
//
//    if(pipeline->index_first == -1 && pipeline->index_last == -1){
//        if(!receiving_list->length)
//            goto end;
//
//        pipeline->index_first = 0;
//        pipeline->index_last  = 0;
//
//        idx_first = pipeline->index_first;
//
//        mad_iovec = tbx_slist_extract(receiving_list);
//       //*DISP_VAL("fill_and_post_reception : EXTRACTION : adapter->receiving_list -len", tbx_slist_get_length(adapter->receiving_list));
//
//        on_goings[idx_first]->area_id   = 0;
//        on_goings[idx_first]->mad_iovec = mad_iovec;
//
//        ogs = on_goings[idx_first]->specific;
//        ogs->endpoint = as->endpoint2;
//
//        channel = mad_iovec->channel;
//        connection = tbx_darray_get(channel->in_connection_darray,
//                                    mad_iovec->remote_rank);
//        on_goings[idx_first]->lnk = connection->link_array[0];
//
//        mad_mx_receive_iovec(on_goings[idx_first]->lnk, on_goings[i]);
//    }
//
//    idx_first = pipeline->index_first;
//    idx_last = idx_first + 1 %2;
//
//    last_og = on_goings[idx_first];
//    mad_iovec = last_og->mad_iovec;
//    if(mad_iovec->area_nb_seg[last_og->area_id + 1] == 0){
//        if(!receiving_list->length)
//            goto end;
//
//        mad_iovec = tbx_slist_extract(receiving_list);
//       //*DISP_VAL("fill_and_post_reception : EXTRACTION : adapter->receiving_list -len", tbx_slist_get_length(adapter->receiving_list));
//
//        on_goings[idx_last]->area_id   = 0;
//        on_goings[idx_last]->mad_iovec = mad_iovec;
//
//        ogs = on_goings[idx_last]->specific;
//        ogs->endpoint = as->endpoint2;
//
//        channel = mad_iovec->channel;
//        connection = tbx_darray_get(channel->in_connection_darray,
//                                    mad_iovec->remote_rank);
//        on_goings[idx_last]->lnk = connection->link_array[0];
//
//        pipeline->index_last = idx_last;
//        mad_mx_receive_iovec(on_goings[idx_last]->lnk, on_goings[idx_last]);
//    } else {
//        on_goings[idx_last]->area_id   = last_og->area_id + 1;
//        on_goings[idx_last]->mad_iovec = mad_iovec;
//
//        ogs = on_goings[idx_last]->specific;
//        ogs->endpoint = as->endpoint2;
//
//        pipeline->index_last = idx_last;
//        mad_mx_receive_iovec(on_goings[idx_last]->lnk, on_goings[idx_last]);
//    }
// end:
//    LOG_OUT();
//}
//
//static void
//pre_post_irecv(p_mad_adapter_t adapter,
//               p_mad_pipeline_t r_pipeline){
//    p_mad_mx_adapter_specific_t as =NULL;
//    p_mad_on_going_t	*on_goings	= NULL;
//    unsigned int	 nb_elm		= -1;
//    unsigned int	 index_last	= -1;
//    mx_request_t	*p_request	=NULL;
//
//    int			idx_first	= -1;
//    int			idx_last	= -1;
//
//    LOG_IN();
//    as = adapter->specific;
//    on_goings        = r_pipeline->on_goings;
//    nb_elm           = r_pipeline->nb_elm;
//    index_last       = r_pipeline->index_last;
//
//    if(r_pipeline->index_first == -1 && r_pipeline->index_last == -1){
//        p_mad_on_going_t		og  = NULL;
//        p_mad_mx_on_going_specific_t	ogs = NULL;
//
//        r_pipeline->index_first = 0;
//        r_pipeline->index_last  = 0;
//
//        og	= on_goings[r_pipeline->index_first];
//        ogs	= og->specific;
//
//        ogs->endpoint	= as->endpoint1;
//        p_request	=  ogs->mx_request;
//
//        mad_mx_irecv(ogs->endpoint,
//                     (mx_segment_t *)og->mad_iovec->data,
//                     1,
//                     0,
//                     MX_MATCH_MASK_NONE,
//                     p_request);
//
//        og->status = 1;
//    }
//
//    idx_first	= r_pipeline->index_first;
//    idx_last	= (idx_first + 1) %2;
//
//    if(!on_goings[idx_last]->status){
//        p_mad_on_going_t 		og  = NULL;
//        p_mad_mx_on_going_specific_t	ogs = NULL;
//
//        og	= on_goings[idx_last];
//        ogs	= og->specific;
//
//        ogs->endpoint	= as->endpoint1;
//        p_request	= ogs->mx_request;
//
//        mad_mx_irecv(ogs->endpoint,
//                     (mx_segment_t *)og->mad_iovec->data,
//                     1,
//                     0,
//                     MX_MATCH_MASK_NONE,
//                     p_request);
//
//        og->status = 1;
//    }
//
//    r_pipeline->index_first = idx_first;
//    r_pipeline->index_last  = idx_last ;
//    LOG_OUT();
//}
//
//static
//void
//mad_mx_shift_pipeline(p_mad_pipeline_t r_pipeline){
//    unsigned int t = 0;
//
//    LOG_IN();
//    assert(r_pipeline->index_first != r_pipeline->index_last);
//
//    t		= r_pipeline->index_first;
//    r_pipeline->index_first	= r_pipeline->index_last;
//    r_pipeline->index_last	= t;
//
//    r_pipeline->on_goings[r_pipeline->index_last]->status = 0;
//    LOG_OUT();
//}
//
//void
//mad_mx_r_make_progress(p_mad_adapter_t adapter, p_mad_channel_t channel){
//    p_mad_mx_channel_specific_t  chs        = NULL;
//    p_mad_track_t                r_track    = NULL;
//    p_mad_pipeline_t             r_pipeline = NULL;
//    p_mad_on_going_t             r_og       = NULL;
//    p_mad_mx_on_going_specific_t r_og_s     = NULL;
//    int                          status     = -1;
//
//
//    LOG_IN();
//    chs = channel->specific;
//    r_track    = adapter->r_track;
//    r_pipeline = r_track->pipelines[0];
//
//
//    if(chs->first_incoming_packet_flag){
//        TRACE("**********Traitement du recv_msg");
//        mad_iovec_exploit_msg(adapter, chs->recv_msg->segment_ptr);
//        chs->first_incoming_packet_flag = tbx_false;
//    }
//
//    /*** On traite les réceptions pre-postées ***/
//    if(r_pipeline->index_first == -1 && r_pipeline->index_last == -1){
//        pre_post_irecv(adapter, r_pipeline);
//        goto large;
//    }
//
//    r_og  = r_pipeline->on_goings[r_pipeline->index_first];
//    r_og_s = r_og->specific;
//
//    status  = mad_mx_test(r_og_s->endpoint, r_og_s->mx_request);
//
//    if(status){
//        //DISP("RECEPTION d'un petit");
//
//        mad_iovec_exploit_og(adapter, r_og);
//        mad_mx_shift_pipeline(r_pipeline);
//        pre_post_irecv(adapter, r_pipeline);
//    }
//
// large:
//
//    /*** On traite la partie qui reçoit les gros messages ***/
//    r_pipeline = r_track->pipelines[1];
//
//    if(r_pipeline->index_last == -1){
//        fill_and_post_reception(adapter, r_pipeline);
//        goto unexpected;
//    }
//
//    r_og   = r_pipeline->on_goings[r_pipeline->index_first];
//    r_og_s = r_og->specific;
//
//    status      = mad_mx_test(r_og_s->endpoint, r_og_s->mx_request);
//    if(!status){
//        goto unexpected;
//    }
//
//
//    //DISP("RECEPTION d'un msg de grande taille");
//
//    if(channel->unpacks_list){
//        if(r_og->mad_iovec->area_nb_seg[r_og->area_id + 1] == 0){
//            mad_iovec_get(adapter->driver->r_msg_slist,
//                          r_og->mad_iovec->channel_id,
//                          r_og->mad_iovec->remote_rank,
//                          r_og->mad_iovec->sequence);
//           //*DISP_VAL("mad_mx_r_mkp : EXTRACTION : driver->r_msg_list -len", tbx_slist_get_length(adapter->driver->r_msg_slist));
//
//            channel = r_og->lnk->connection->channel;
//            mad_iovec_get(channel->unpacks_list,
//                          r_og->mad_iovec->channel_id,
//                          r_og->mad_iovec->remote_rank,
//                          r_og->mad_iovec->sequence);
//           //*DISP_VAL("mad_mx_r_mkp :  EXTRACTION : channel->unpacks_list -len", tbx_slist_get_length(channel->unpacks_list));
//
//            mad_iovec_free(r_og->mad_iovec, tbx_false);
//        }
//    }
//
//
//    if(r_pipeline->index_first == r_pipeline->index_last){
//        r_pipeline->index_first = -1;
//        r_pipeline->index_last = r_pipeline->index_first;
//    } else {
//        r_pipeline->index_first = r_pipeline->index_last;
//    }
//    fill_and_post_reception(adapter, r_pipeline);
//
//
// unexpected:
//    /*** on traite les unexpected ***/
//    {
//        p_tbx_slist_t waiting_list = NULL;
//        p_mad_iovec_t cur = NULL;
//        p_mad_iovec_t searched = NULL;
//        tbx_bool_t ref_forward = tbx_true;
//
//        waiting_list = adapter->waiting_list;
//
//        if(waiting_list->length){
//            tbx_slist_ref_to_head(waiting_list);
//            while(1){
//                cur = tbx_slist_ref_get(waiting_list);
//                if(cur == NULL)
//                    break;
//
//
//                //DISP_VAL("mad_mx_r_mkp : unexpected :  EXTRACTION : adapter->driver->r_msg_slist -len", tbx_slist_get_length(adapter->driver->r_msg_slist));
//
//                searched = mad_iovec_get(adapter->driver->r_msg_slist,
//                                         cur->channel_id,
//                                         cur->remote_rank,
//                                         cur->sequence);
//                //DISP_VAL("mad_mx_r_mkp : unexpected :  EXTRACTION : adapter->driver->r_msg_slist -len", tbx_slist_get_length(adapter->driver->r_msg_slist));
//
//                if(searched){
//                    void * data           = cur->data[0].iov_base;
//                    tbx_slist_index_t idx = 0;
//
//
//                    mad_iovec_get(channel->unpacks_list,
//                                  cur->channel_id,
//                                  cur->remote_rank,
//                                  cur->sequence);
//
//                    memcpy(searched->data[0].iov_base, data, searched->data[0].iov_len);
//
//                    idx = tbx_slist_ref_get_index(waiting_list);
//                    ref_forward = tbx_slist_ref_forward(waiting_list);
//                    tbx_slist_index_extract(waiting_list, idx);
//
//                    //*DISP_VAL("r_mkp : u : EXTRACTION : waiting_list -len", tbx_slist_get_length(waiting_list));
//
//                    mad_iovec_free(cur, tbx_false);
//                    mad_iovec_free(searched, tbx_false);
//
//                    if(!ref_forward)
//                        break;
//                } else {
//                    if(!tbx_slist_ref_forward(waiting_list))
//                        break;
//                }
//            }
//        }
//    }
//
//    LOG_OUT();
//
//}
