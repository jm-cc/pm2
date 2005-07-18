#include <stdlib.h>
#include <stdio.h>

#include "madeleine.h"

static p_mad_iovec_t
build_a_rdv(p_mad_adapter_t  adapter,
            p_mad_iovec_t    large_to_send){
    p_mad_iovec_t rdv = NULL;
    unsigned int index = 0;
    sequence_t sequence = 0;
    rank_t my_rank = 0;

    p_mad_channel_t    channel = NULL;
    p_mad_madeleine_t madeleine = NULL;
    p_mad_channel_t *channel_tab = NULL;

    LOG_IN();
    madeleine = mad_get_madeleine();
    channel_tab = madeleine->channel_tab;
    channel = channel_tab[large_to_send->channel_id];
    if(!channel){
        FAILURE("fill_and_send : channel not found");
    }
    my_rank = (rank_t)channel->process_lrank;

    rdv = mad_iovec_create(large_to_send->channel_id, sequence);
    rdv->remote_rank = large_to_send->remote_rank;
    rdv->area_nb_seg[0] = 2;
    rdv->channel = large_to_send->channel;

    index = mad_iovec_begin_struct_iovec(rdv, my_rank);
    index = mad_iovec_add_rdv(rdv, index,
                              large_to_send->channel_id,
                              large_to_send->sequence);
    LOG_OUT();
    return rdv;
}

static void
format_mad_iovec(p_mad_iovec_t mad_iovec){
    unsigned int nb_seg = 0;
    rank_t my_rank = 0;
    unsigned int i = 0;

    p_mad_channel_t    channel = NULL;
    p_mad_madeleine_t madeleine = NULL;
    p_mad_channel_t *channel_tab = NULL;

    channel_id_t channel_id = 0;
    sequence_t sequence = 0;
    length_t length = 0;

    LOG_IN();
    nb_seg = mad_iovec->total_nb_seg;

    madeleine = mad_get_madeleine();
    channel_tab = madeleine->channel_tab;
    channel = channel_tab[mad_iovec->channel_id];
    if(!channel){
        FAILURE("fill_and_send : channel not found");
    }
    my_rank = (rank_t)channel->process_lrank;

    for(i = nb_seg + 1; i > 0; i--){
        mad_iovec->data[i] = mad_iovec->data[i - 2];
    }

    channel_id = mad_iovec->channel_id;
    sequence = mad_iovec->sequence;
    length = mad_iovec->length;

    mad_iovec_begin_struct_iovec(mad_iovec,
                                 my_rank);

    mad_iovec_add_header_data(mad_iovec, 1,
                              channel_id,
                              sequence,
                              length);

    LOG_OUT();
}


static p_mad_iovec_t
mad_iovec_copy_and_format(p_mad_iovec_t mad_iovec){
    p_mad_iovec_t new_mad_iovec = NULL;

    unsigned int nb_seg = 0;
    rank_t my_rank = 0;
    unsigned int i = 0;

    p_mad_channel_t    channel = NULL;
    p_mad_madeleine_t madeleine = NULL;
    p_mad_channel_t *channel_tab = NULL;

    channel_id_t channel_id = 0;
    sequence_t sequence = 0;
    length_t length = 0;

    LOG_IN();
    madeleine = mad_get_madeleine();
    channel_tab = madeleine->channel_tab;
    channel = channel_tab[mad_iovec->channel_id];
    if(!channel){
        FAILURE("fill_and_send : channel not found");
    }
    my_rank = (rank_t)channel->process_lrank;

    new_mad_iovec = mad_iovec_create(mad_iovec->channel_id, mad_iovec->sequence);
    new_mad_iovec->remote_rank = mad_iovec->remote_rank;
    new_mad_iovec->channel = channel;
    new_mad_iovec->total_nb_seg =  mad_iovec->total_nb_seg;
    new_mad_iovec->channel_id = mad_iovec->channel_id;
    new_mad_iovec->sequence = mad_iovec->sequence;
    new_mad_iovec->length = mad_iovec->length;
    new_mad_iovec->send_mode = mad_iovec->send_mode;
    new_mad_iovec->receive_mode = mad_iovec->receive_mode;

    nb_seg = new_mad_iovec->total_nb_seg;
    for(i = nb_seg + 1; i > 0; i--){
        new_mad_iovec->data[i] = mad_iovec->data[i - 2];
    }

    channel_id = new_mad_iovec->channel_id;
    sequence = new_mad_iovec->sequence;
    length = new_mad_iovec->length;

    mad_iovec_begin_struct_iovec(new_mad_iovec,
                                 my_rank);

    mad_iovec_add_header_data(new_mad_iovec, 1,
                              channel_id,
                              sequence,
                              length);
    LOG_OUT();
    return new_mad_iovec;
}


static tbx_bool_t
mad_take(p_mad_driver_t driver,
         p_mad_iovec_t mad_iovec,
         p_mad_iovec_t next_mad_iovec,
         tbx_bool_t express){
    p_mad_driver_interface_t interface = NULL;

    mad_receive_mode_t first_receive_mode;
    length_t next_len;
    tbx_bool_t taken = tbx_false;
    unsigned int limit_size = 0;
    LOG_IN();

    interface = driver->interface;
    first_receive_mode = mad_iovec->receive_mode;
    next_len = next_mad_iovec->length;
    limit_size = interface->limit_size();

    if(express && (next_len > limit_size)){
        //DISP("deja un express");
        taken = tbx_false;
        goto end;
    }

    if(first_receive_mode == mad_receive_CHEAPER){
        taken =  tbx_true;
    } else if (first_receive_mode == mad_receive_EXPRESS) {
        if(next_len <= limit_size){
            taken = tbx_true;
        } else {
            taken = tbx_false;
        }
    } else {
        FAILURE("receive mode invalid");
    }

 end:
    LOG_OUT();
    return taken;
}

p_mad_iovec_t
mad_s_optimize(p_mad_adapter_t adapter,
               p_mad_on_going_t to_send){
    p_mad_driver_t           driver      = NULL;
    p_mad_driver_interface_t interface   = NULL;
    p_tbx_slist_t            s_msg_slist = NULL;
    p_mad_iovec_t            mad_iovec   = NULL;
    p_mad_iovec_t            first_mad_iovec   = NULL;
    p_mad_iovec_t            next_mad_iovec   = NULL;
    length_t                 limit_len = 0;
    unsigned int no_entries = 0;
    tbx_bool_t express = tbx_false;
    LOG_IN();

    driver               = adapter->driver;
    interface            = driver->interface;
    s_msg_slist          = driver->s_msg_slist;
    limit_len            = interface->limit_size();

    if(!s_msg_slist->length)
        return NULL;

    mad_iovec = tbx_slist_remove_from_head(s_msg_slist);
    if(!mad_iovec)
        goto end;

    if(mad_iovec->length <= limit_len){
        mad_iovec = mad_iovec_copy_and_format(mad_iovec);
        no_entries = 3;
    } else {
        rank_t my_rank = 0;
        p_mad_channel_t    channel = NULL;
        p_mad_madeleine_t madeleine = NULL;
        p_mad_channel_t *channel_tab = NULL;
        p_mad_iovec_t    large_mad_iovec   = NULL;

        // create a mad_iovec
        large_mad_iovec = mad_iovec;

        madeleine = mad_get_madeleine();
        channel_tab = madeleine->channel_tab;
        channel = channel_tab[mad_iovec->channel_id];
        if(!channel){
            FAILURE("fill_and_send : channel not found");
        }
        my_rank = (rank_t)channel->process_lrank;

        tbx_slist_append(adapter->large_mad_iovec_list, large_mad_iovec);

        mad_iovec = mad_iovec_create(large_mad_iovec->channel_id, 0);
        mad_iovec->remote_rank = large_mad_iovec->remote_rank;
        mad_iovec->channel = channel;
        mad_iovec->send_mode = large_mad_iovec->send_mode;
        mad_iovec->receive_mode = large_mad_iovec->receive_mode;

        no_entries = mad_iovec_begin_struct_iovec(mad_iovec, my_rank);
        no_entries = mad_iovec_add_rdv(mad_iovec, no_entries, large_mad_iovec->channel_id, large_mad_iovec->sequence);
    }

    first_mad_iovec = mad_iovec;

    if(first_mad_iovec->receive_mode == mad_receive_EXPRESS)
        express = tbx_true;

    while(1){
        if(!s_msg_slist->length)
            goto end;

        if(no_entries >= 32)
            goto end;

        next_mad_iovec = tbx_slist_index_get(s_msg_slist, 0);
        if(!next_mad_iovec)
            goto end;

        if((next_mad_iovec->length <= limit_len
            && mad_iovec->length + next_mad_iovec->length < limit_len)
           || (next_mad_iovec->length > limit_len
               && mad_iovec->length +  MAD_IOVEC_RDV_SIZE < limit_len)){

            if(mad_take(driver, first_mad_iovec, next_mad_iovec, express)){
                tbx_slist_remove_from_head(s_msg_slist);
                if(next_mad_iovec->length <= limit_len){
                    no_entries = mad_iovec_add_header_data(mad_iovec,
                                                           no_entries,
                                                           next_mad_iovec->channel_id,
                                                           next_mad_iovec->sequence,
                                                           next_mad_iovec->length);
                    mad_iovec_add_data2(mad_iovec,
                                        next_mad_iovec->data[0].iov_base,
                                        next_mad_iovec->length,
                                        no_entries);
                    no_entries++;
                } else {
                    no_entries = mad_iovec_add_rdv(mad_iovec,
                                                   no_entries,
                                                   next_mad_iovec->channel_id,
                                                   next_mad_iovec->sequence);
                    tbx_slist_append(adapter->large_mad_iovec_list,
                                     next_mad_iovec);
                }
                first_mad_iovec = next_mad_iovec;

                if(first_mad_iovec->receive_mode == mad_receive_EXPRESS)
                    express = tbx_true;
            } else {
                goto end;
            }
        } else {
            goto end;
        }
    }
 end:
    interface->create_areas(mad_iovec);
    LOG_OUT();
    return mad_iovec;
}
