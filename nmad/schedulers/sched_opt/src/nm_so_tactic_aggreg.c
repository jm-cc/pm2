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

#include <tbx.h>
#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include "nm_so_private.h"
#include "nm_so_tactics/nm_so_tactic_aggreg.h"

int
nm_so_tactic_aggreg(struct nm_drv *driver,
                    struct nm_so_se_pending_tab *pw_tab,
                    struct nm_so_se_op_stack *op_stack,
                    int tree_altitude){

    DISP("-->nm_so_strategy1_agregation");

    int nb_pending_pw = pw_tab->nb_pending_pw;
    struct nm_so_se_shorter_pw *cur_spw;

    struct nm_so_se_shorter_pw *spw = NULL;
    struct nm_so_se_operation *op = NULL;

    int len = 0;
    int nb_seg = 0;

    int i, j;
    int err;

    for(i = 0; i < nb_pending_pw; i++){
        cur_spw = pw_tab->tab[i];

        if(len + cur_spw->len < SMALL_THRESHOLD
           && len + cur_spw->len < AGGREGATED_PW_MAX_SIZE
           && nb_seg + cur_spw->nb_seg < AGGREGATED_PW_MAX_NB_SEG){
            if(!op){
                op = TBX_MALLOC(sizeof(struct nm_so_se_operation));
#warning MALLOC_A_ERADIQUER

                op->operation_type = nm_so_se_aggregation;
                op->nb_pw = 0;
                op->altitude = tree_altitude;

                spw = cur_spw;
                spw->src_pw = NULL;

            } else {
                spw->len    += cur_spw->len;
                spw->nb_seg += cur_spw->nb_seg;

                // decalage
                for(j = i + 1; j < nb_pending_pw; j++){
                    pw_tab->tab[j - 1] = pw_tab->tab[j];
                }
                i --;
                nb_pending_pw--;
            }

            op->pw_idx[op->nb_pw] = cur_spw->index;
            op->nb_pw++;

        }
    }

    if(op){
        err = nm_so_stack_push(op_stack->op_stack, op);
        nm_so_control_error("nm_so_stack_push", err);

        op_stack->score += nm_ns_evaluate(driver, spw->len);

        pw_tab->nb_pending_pw =  nb_pending_pw;

        // maj indices des shorter_pw
        for(i = 0; i < nb_pending_pw; i++){
            pw_tab->tab[i]->index = i;
        }
    }

    DISP_VAL("<--nm_so_strategy1_aggregation - score", op_stack->score);
    //err = NM_ESUCCESS;

    return op_stack->score;
}

//static int
//nm_so_strategy2_aggregation(struct nm_drv *driver,
//                           struct nm_so_se_pending_tab *pw_tab,
//                           struct nm_so_se_op_stack *op_stack,
//                           int tree_altitude){
//    DISP("-->nm_so_strategy2_aggregation");
//    DISP("<--nm_so_strategy2_aggregation");
//    return 0;
//}
