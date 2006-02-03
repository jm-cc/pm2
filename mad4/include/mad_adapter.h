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
 * Mad_adapter.h
 * ==============
 */

#ifndef MAD_ADAPTER_H
#define MAD_ADAPTER_H

typedef struct s_mad_adapter_info
{
    p_mad_dir_node_t     dir_node;
    p_mad_dir_adapter_t  dir_adapter;
    char                *channel_parameter;
    char                *connection_parameter;
} mad_adapter_info_t;

typedef struct s_mad_adapter
{
    // Common use fields
    TBX_SHARED;
    p_mad_driver_t           driver;
    mad_adapter_id_t         id;
    char                    *selector;
    char                    *parameter;
    unsigned int             mtu;
    p_tbx_htable_t           channel_htable;
    p_mad_dir_adapter_t      dir_adapter;



    /*-----------------------------*/
    /* pending communications */
    p_mad_track_set_t s_track_set;
    p_mad_track_set_t r_track_set;

    /* msg waiting for a transmission */
    p_tbx_slist_t s_ready_msg_list;

    /* msg waiting for an acknowlegment */
    p_tbx_slist_t waiting_acknowlegment_list;

    /* received rdv */
    p_tbx_slist_t rdv_to_treat;

    /* prepared buffer to pre-post */
    p_mad_pipeline_t pre_posted;

    /* for the flow control of the unexpected */
    int nb_released_unexpecteds;
    p_mad_pipeline_t unexpecteds;   // all the unexpected
    int *nb_authorized_sendings; //for each destination

    int needed_sendings; // ensure the sending of the acknowlegment even if there is no pending unpack
    int  needed_receptions; // ensure the reception of the acknowlegment even if there is no pending pack

    /*-----------------------------*/

    p_mad_driver_specific_t  specific;
} mad_adapter_t;

#endif /* MAD_ADAPTER_H */
