///*
// * NewMadeleine
// * Copyright (C) 2006 (see AUTHORS file)
// *
// * This program is free software; you can redistribute it and/or modify
// * it under the terms of the GNU General Public License as published by
// * the Free Software Foundation; either version 2 of the License, or (at
// * your option) any later version.
// *
// * This program is distributed in the hope that it will be useful, but
// * WITHOUT ANY WARRANTY; without even the implied warranty of
// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// * General Public License for more details.
// */
//
//#include <tbx.h>
//#include <stdint.h>
//#include <sys/uio.h>
//#include <assert.h>
//
//#include "nm_so_private.h"
//#include "nm_so_strategies/nm_so_strat_exhaustive.h"
//
//#include "nm_so_tactics/nm_so_tactic_aggreg.h"
////#include "nm_so_tactics/nm_so_strat_....h"
//
//#include "nm_so_strategies_private.h"
//
////=========================================================
//#define MAX_NB_OP 10
//#define NB_PACK_BY_OPT 1
//
//
//
//struct nm_so_strat_exhaustive{
//    // allocateur rapide pour les operations
//    p_tbx_memory_t op_key;
//
//    // allocateur rapide pour les tableaux de pw
//    p_tbx_memory_t pending_tab_key;
//
//    // allocateur rapide pour les shorter_pw
//    p_tbx_memory_t spw_key;
//
//    // 2 piles d'operations
//    struct nm_so_se_op_stack *op_stack;
//    struct nm_so_se_op_stack *op_stack_ref;
//};
//
//
//#define NB_STRATEGIES 2
//static nm_so_tactic *tactics[] = {
//    &nm_so_tactic_aggreg,
//    NULL
//};
////nm_so_strategy2_aggregation};
//
//
///**************************************************************/
///**************************************************************/
///**************************************************************/
//
//static int
//nm_so_se_cpy_down_op(struct nm_so_se_op_stack *op_stack,
//                  struct nm_so_se_op_stack *op_stack_ref){
//    int err;
//
//    if(nm_so_stack_size(op_stack_ref->op_stack)){
//        err = nm_so_stack_pop(op_stack_ref->op_stack, NULL);
//        nm_so_control_error("nm_so_stack_pop", err);
//    }
//
//    struct nm_so_se_operation *op = NULL;
//    void *ptr = NULL;
//
//    err = nm_so_stack_down(op_stack->op_stack, &ptr);
//    nm_so_control_error("nm_so_stack_down", err);
//    op = ptr;
//
//    err = nm_so_stack_push(op_stack_ref->op_stack, op);
//    nm_so_control_error("nm_so_stack_push", err);
//
//    op_stack_ref->score = op_stack->score;
//
//    err = NM_ESUCCESS;
//    return err;
//}
//
//static struct nm_so_se_pending_tab *
//nm_so_se_cpy_tab(struct nm_so_se_pending_tab * pw_tab){
//    int i;
//    struct nm_so_se_pending_tab *cpy = TBX_MALLOC(sizeof(struct nm_so_se_pending_tab));
//
//    cpy->nb_pending_pw = pw_tab->nb_pending_pw;
//
//    for(i = 0; i < cpy->nb_pending_pw; i++){
//        cpy->tab[i]->index  = pw_tab->tab[i]->index;
//        cpy->tab[i]->len    = pw_tab->tab[i]->len;
//        cpy->tab[i]->nb_seg = pw_tab->tab[i]->nb_seg;
//        cpy->tab[i]->src_pw = pw_tab->tab[i]->src_pw;
//    }
//    return cpy;
//}
//
//static void
//nm_so_se_path_tree(int strategy_no,
//                struct nm_so_se_pending_tab *pw_tab,
//
//                int empty_application_nb,
//                int tree_altitude,
//                struct nm_so_se_op_stack *op_stack,
//                struct nm_so_se_op_stack *op_stack_ref,
//                struct nm_drv *drv){
//
//    int nb_applied_strategies = 0;
//    struct nm_so_se_pending_tab *pw_tab_to_use = NULL;
//
//    int score = 0;
//    int i;
//    int err;
//
//    if(strategy_no == -1)
//        goto down;
//
//    //DISP_VAL("nm_so_se_path_tree - essai de la strategie n°", strategy_no);
//
//    nm_so_tactic * tactic = tactics[strategy_no];
//    score = (*tactic)(drv, pw_tab, op_stack, tree_altitude);
//
//    if(score){
//        //DISP("Ajout d'une opération");
//        empty_application_nb = 0;
//    } else {
//        //DISP("PAS d'Ajout d'opération");
//        empty_application_nb++;
//    }
//
//
//
//    // conditions d'arrêt
//    if(nm_so_stack_size(op_stack->op_stack) == MAX_NB_OP
//       || pw_tab->nb_pending_pw == 1
//       || empty_application_nb == NB_STRATEGIES){
//
//        // on sauvegarde la 1ere op de la meilleure stratégie
//        if(!op_stack_ref->score
//           || op_stack->score < op_stack_ref->score){
//            nm_so_se_cpy_down_op(op_stack, op_stack_ref);
//        }
//
//        // on dépile
//        struct nm_so_se_operation *op = NULL;
//        void *ptr = NULL;
//
//        while(nm_so_stack_size(op_stack->op_stack)){
//
//            err = nm_so_stack_top(op_stack->op_stack, &ptr);
//            nm_so_control_error("nm_so_stack_top", err);
//            op = ptr;
//
//            if(op->altitude == tree_altitude){
//                err = nm_so_stack_pop(op_stack->op_stack, NULL);
//                nm_so_control_error("nm_so_stack_pop", err);
//
//            } else {
//                break;
//            }
//        }
//
//        return;
//    }
//
// down:
//    // application aux fils
//    for(i = 0; i < NB_STRATEGIES; i++){
//
//        if(i == strategy_no)
//            continue;
//
//        nb_applied_strategies++;
//
//        if(nb_applied_strategies < NB_STRATEGIES - 1){
//            //dupplication du tableau
//            pw_tab_to_use = nm_so_se_cpy_tab(pw_tab);
//        } else {
//            pw_tab_to_use = pw_tab;
//        }
//
//        nm_so_se_path_tree(i,
//                        pw_tab_to_use,
//                        empty_application_nb,
//                        tree_altitude + 1,
//                        op_stack, op_stack_ref,
//                        drv);
//
//    }
//}
//
//////////////// Point d'entrée //////////////////////////////
//static int
//nm_so_se_strategy_application(struct nm_gate *p_gate,
//                           struct nm_drv *driver,
//                           p_tbx_slist_t pre_list,
//                           struct nm_pkt_wrap **pp_pw){
//    int err;
//    int i;
//
//    struct nm_so_se_op_stack *op_stack =
//        TBX_MALLOC(sizeof(struct nm_so_se_op_stack));
//    err = nm_so_stack_create(&op_stack->op_stack, MAX_NB_OP);
//    nm_so_control_error("nm_so_stack_create", err);
//    op_stack->score = 0;
//
//    struct nm_so_se_op_stack *op_stack_ref =
//        TBX_MALLOC(sizeof(struct nm_so_se_op_stack));
//    err = nm_so_stack_create(&op_stack_ref->op_stack,
//                             NB_PACK_BY_OPT);
//    nm_so_control_error("nm_so_stack_create", err);
//    op_stack_ref->score = 0;
//
//
//    int pre_list_len = tbx_slist_get_length(pre_list);
//
//    struct nm_so_se_pending_tab *cpy_pw_tab = NULL;
//    struct nm_so_se_pending_tab *pw_tab = NULL;
//    pw_tab  = TBX_MALLOC(sizeof(struct nm_so_se_pending_tab));
//    cpy_pw_tab = TBX_MALLOC(sizeof(struct nm_so_se_pending_tab));
//#warning ERR_MALLOC
//
//    pw_tab->nb_pending_pw =
//        (pre_list_len < PENDING_PW_WINDOW_SIZE ?
//         pre_list_len : PENDING_PW_WINDOW_SIZE);
//    cpy_pw_tab->nb_pending_pw = pw_tab->nb_pending_pw;
//
//    tbx_slist_ref_to_head(pre_list);
//    for(i = 0; i < pw_tab->nb_pending_pw; i++){
//
//        pw_tab->tab[i] = TBX_MALLOC(sizeof(struct nm_so_se_shorter_pw));
//#warning MALLOC
//
//        pw_tab->tab[i]->index = i;
//        pw_tab->tab[i]->src_pw = tbx_slist_ref_get(pre_list);
//        assert(pw_tab->tab[i]->src_pw);
//        pw_tab->tab[i]->len    = pw_tab->tab[i]->src_pw->length;
//        pw_tab->tab[i]->nb_seg = pw_tab->tab[i]->src_pw->v_nb;
//
//
//        cpy_pw_tab->tab[i] = TBX_MALLOC(sizeof(struct nm_so_se_shorter_pw));
//#warning MALLOC
//
//        cpy_pw_tab->tab[i]->index  = pw_tab->tab[i]->index ;
//        cpy_pw_tab->tab[i]->src_pw = pw_tab->tab[i]->src_pw;
//        cpy_pw_tab->tab[i]->len    = pw_tab->tab[i]->len   ;
//        cpy_pw_tab->tab[i]->nb_seg = pw_tab->tab[i]->nb_seg;
//
//        tbx_slist_ref_forward(pre_list);
//    }
//
//
//    // construction de la suite optimale d'operations à appliquer
//    nm_so_se_path_tree(-1, pw_tab,
//                    0, 0,
//                    op_stack, op_stack_ref,
//                    driver);
//
//
//    // application de la strategie optimale
//    struct nm_core         *p_core = driver->p_core;
//    struct nm_pkt_wrap     *cur_pw = NULL;
//    struct nm_pkt_wrap     *p_pw = NULL;
//    struct nm_so_pkt_wrap  *so_pw = NULL;
//    struct nm_so_se_operation *op = NULL;
//    void *ptr = NULL;
//
//    while(nm_so_stack_size(op_stack_ref->op_stack)){
//        err = nm_so_stack_pop(op_stack_ref->op_stack, &ptr);
//        nm_so_control_error("nm_so_stack_pop", err);
//        op = ptr;
//
//        switch(op->operation_type){
//        case nm_so_se_aggregation:
//            err = nm_so_take_aggregation_pw(p_gate->p_sched, &p_pw);
//            nm_so_control_error("nm_so_take_aggregation_pw", err);
//            so_pw = p_pw->sched_priv;
//
//            for(i = 0; i < op->nb_pw; i++){
//                cur_pw = cpy_pw_tab->tab[op->pw_idx[i]]->src_pw;
//
//                err = nm_so_add_data(p_core, p_pw,
//                                     cur_pw->proto_id, cur_pw->length,
//                                     cur_pw->seq, cur_pw->v[0].iov_base);
//                nm_so_control_error("nm_so_add_data", err);
//
//                so_pw->aggregated_pws[so_pw->nb_aggregated_pws++] = cur_pw;
//            }
//            break;
//
//        case nm_so_se_split:
//            TBX_FAILURE("split operation not supported");
//        default:
//            TBX_FAILURE("strategy_application failed");
//        }
//    }
//
//    err = nm_so_update_global_header(p_pw, p_pw->v_nb, p_pw->length);
//    nm_so_control_error("nm_so_update_global_header", err);
//
//    *pp_pw = p_pw;
//
//    err = NM_ESUCCESS;
//
//    return err;
//}
//
//
///**************************************************************/
///**************************************************************/
///**************************************************************/
//
////=============================================================
//
//// Compute and apply the best possible packet rearrangement,
//// then return next packet to send
//static int try_and_commit(nm_so_strategy *strat,
//			  struct nm_gate *p_gate,
//			  struct nm_drv *driver,
//			  p_tbx_slist_t pre_list,
//			  struct nm_pkt_wrap **pp_pw)
//{
//  return nm_so_se_strategy_application(p_gate, driver, pre_list, pp_pw);
//}
//
//// Initialization
//static int init(nm_so_strategy *strat)
//{
//    int err;
//
//    struct nm_so_strat_exhaustive *se_strategy
//        = TBX_MALLOC(sizeof(struct nm_so_strat_exhaustive));
//
//    // allocateur rapide pour les operations
//    tbx_malloc_init(&se_strategy->op_key,
//                    sizeof(struct nm_so_se_operation),
//                    PENDING_PW_WINDOW_SIZE + 1, "so_se_operation");
//
//    // allocateur rapide pour les tableaux de pw
//    tbx_malloc_init(&se_strategy->pending_tab_key,
//                    sizeof(struct nm_so_se_pending_tab),
//                    PENDING_PW_WINDOW_SIZE + 1, "so_se_pending_tab");
//    // allocateur rapide pour les shorter_pw
//    tbx_malloc_init(&se_strategy->spw_key,
//                    sizeof(struct nm_so_se_shorter_pw),
//                    (PENDING_PW_WINDOW_SIZE + 1) * PENDING_PW_WINDOW_SIZE, "so_se_spw");
//
//    // 2 piles d'operations
//    struct nm_so_se_op_stack *op_stack =
//        TBX_MALLOC(sizeof(struct nm_so_se_op_stack));
//    err = nm_so_stack_create(&op_stack->op_stack, MAX_NB_OP);
//    nm_so_control_error("nm_so_stack_create", err);
//    se_strategy->op_stack = op_stack;
//
//    struct nm_so_se_op_stack *op_stack_ref =
//        TBX_MALLOC(sizeof(struct nm_so_se_op_stack));
//    err = nm_so_stack_create(&op_stack_ref->op_stack, MAX_NB_OP);
//    nm_so_control_error("nm_so_stack_create", err);
//    se_strategy->op_stack_ref = op_stack_ref;
//
//    strat->priv = se_strategy;
//
//    err = NM_ESUCCESS;
//    return err;
//}
//
//nm_so_strategy nm_so_strat_exhaustive = {
//  .init = init,
//  .try = NULL,
//  .commit = NULL,
//  .try_and_commit = try_and_commit,
//  .cancel = NULL,
//  .priv = NULL,
//};
