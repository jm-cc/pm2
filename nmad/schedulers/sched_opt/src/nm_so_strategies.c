#include <tbx.h>
#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include "nm_so_private.h"


#define PENDING_PW_WINDOW_SIZE 10
#define MAX_NB_OP 10
#define NB_PACK_BY_OPT 1

#define NB_STRATEGIES 2

struct nm_so_shorter_pw{
    int index;

    // info discriminante
    int len;
    int nb_seg;

    struct nm_pkt_wrap *src_pw;
};


struct nm_so_pending_tab{
    struct nm_so_shorter_pw *tab[PENDING_PW_WINDOW_SIZE];
    int nb_pending_pw;
};


enum nm_so_operation_type{
    nm_so_aggregation,
    nm_so_split
};

struct nm_so_operation{
    int operation_type;

    int pw_idx[PENDING_PW_WINDOW_SIZE];
    int nb_pw;

    int nb_seg;

    int altitude;
};


struct nm_so_op_stack{
    struct nm_so_stack *op_stack;
    double score;
};



typedef int (*strategy) (struct nm_drv *,
                         struct nm_so_pending_tab *,
                         struct nm_so_op_stack *,
                         int tree_altitude);

/**************************************************************/
/**************************************************************/
/**************************************************************/

static int
nm_so_strategy1_aggregation(struct nm_drv *driver,
                           struct nm_so_pending_tab *pw_tab,
                           struct nm_so_op_stack *op_stack,
                           int tree_altitude){

    DISP("-->nm_so_strategy1_agregation");

    int nb_pending_pw = pw_tab->nb_pending_pw;
    struct nm_so_shorter_pw *cur_spw;

    struct nm_so_shorter_pw *spw = NULL;
    struct nm_so_operation *op = NULL;

    int len = 0;
    int nb_seg = 0;

    int i, j;

    DISP_VAL("nb_pending_pw", nb_pending_pw);
    for(i = 0; i < nb_pending_pw; i++){
        DISP_VAL("i", i);

        cur_spw = pw_tab->tab[i];

        if(len + cur_spw->len < SMALL_THRESHOLD
           && len + cur_spw->len < AGGREGATED_PW_MAX_SIZE
           && nb_seg + cur_spw->nb_seg < AGGREGATED_PW_MAX_NB_SEG){

            DISP("ICI");

            if(!op){
                op = TBX_MALLOC(sizeof(struct nm_so_operation));
                op->operation_type = nm_so_aggregation;
                op->nb_pw = 0;
                op->altitude = tree_altitude;

                spw = cur_spw;
                //spw->index = -1;
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
        DISP_PTR("stack_push(op) dans", op_stack);
        nm_so_stack_push(op_stack->op_stack, op);

        DISP_VAL("nm_so_strategy1_aggregation, type de l'op", op->operation_type);

        op_stack->score += nm_ns_evaluate(driver, spw->len);

        pw_tab->nb_pending_pw =  nb_pending_pw;

        // maj indices des shoter
        for(i = 0; i < nb_pending_pw; i++){
            pw_tab->tab[i]->index = i;
        }
    }

    DISP_VAL("<--nm_so_strategy1_aggregation - score", op_stack->score);
    return op_stack->score;
}

static int
nm_so_strategy2_aggregation(struct nm_drv *driver,
                           struct nm_so_pending_tab *pw_tab,
                           struct nm_so_op_stack *op_stack,
                           int tree_altitude){
    DISP("-->nm_so_strategy2_aggregation");
    return 0;
    DISP("<--nm_so_strategy2_aggregation");
}


strategy strategy_tab[NB_STRATEGIES] =
    {nm_so_strategy1_aggregation,
     nm_so_strategy2_aggregation};

/**************************************************************/
/**************************************************************/
/**************************************************************/


static void
nm_so_cpy_down_op(struct nm_so_op_stack *op_stack,
                  struct nm_so_op_stack *op_stack_ref){

    if(nm_so_stack_size(op_stack_ref->op_stack)){
        nm_so_stack_pop(op_stack_ref->op_stack);
    }

    nm_so_stack_push(op_stack_ref->op_stack,
                     nm_so_stack_down(op_stack->op_stack));

    op_stack_ref->score = op_stack->score;
}

static struct nm_so_pending_tab *
nm_so_cpy_tab(struct nm_so_pending_tab * pw_tab){
    DISP("-->nm_so_cpy_tab");
    int i;

    struct nm_so_pending_tab *cpy = TBX_MALLOC(sizeof(struct nm_so_pending_tab));

    cpy->nb_pending_pw = pw_tab->nb_pending_pw;

    for(i = 0; i < cpy->nb_pending_pw; i++){
        cpy->tab[i]->index  = pw_tab->tab[i]->index;
        cpy->tab[i]->len    = pw_tab->tab[i]->len;
        cpy->tab[i]->nb_seg = pw_tab->tab[i]->nb_seg;
        cpy->tab[i]->src_pw = pw_tab->tab[i]->src_pw;
    }
    DISP("<--nm_so_cpy_tab");
    return cpy;
}

static void
nm_so_path_tree(int strategy_no,
                struct nm_so_pending_tab *pw_tab,

                int empty_application_nb,
                int tree_altitude,
                struct nm_so_op_stack *op_stack,
                struct nm_so_op_stack *op_stack_ref,
                struct nm_drv *drv){

    int nb_applied_strategies = 0;
    struct nm_so_pending_tab *pw_tab_to_use = NULL;

    int score = 0;

    DISP_VAL("nm_so_path_tree - essai de la strategie n°", strategy_no);
    score = strategy_tab[strategy_no](drv, pw_tab, op_stack,
                                      tree_altitude);

    if(score){
        DISP("Ajout d'une opération");
        empty_application_nb = 0;
    } else {
        DISP("PAS d'Ajout d'opération");
        empty_application_nb++;
    }



    // conditions d'arrêt
    if(nm_so_stack_size(op_stack->op_stack) == MAX_NB_OP
       || pw_tab->nb_pending_pw == 1
       || empty_application_nb == NB_STRATEGIES){

        // on sauvegarde la 1ere op de la meilleure stratégie
        if(!op_stack_ref->score
           || op_stack->score < op_stack_ref->score){
            nm_so_cpy_down_op(op_stack, op_stack_ref);
        }

        DISP_VAL("nb_op empilées", nm_so_stack_size(op_stack->op_stack));
        // on dépile
        struct nm_so_operation *op = NULL;
        while(nm_so_stack_size(op_stack->op_stack)){

            op = nm_so_stack_top(op_stack->op_stack);

            if(op->altitude == tree_altitude){
                DISP_PTR("stack_pop(op) de", op_stack);
                nm_so_stack_pop(op_stack->op_stack);
            } else {
                break;
            }
        }

        return;
    }

    // application aux fils
    int i;
    for(i = 0; i < NB_STRATEGIES; i++){

        if(i == strategy_no)
            continue;

        nb_applied_strategies++;

        if(nb_applied_strategies < NB_STRATEGIES - 1){
            //dupplication du tableau
            pw_tab_to_use = nm_so_cpy_tab(pw_tab);
        } else {
            pw_tab_to_use = pw_tab;
        }

        nm_so_path_tree(i,
                        pw_tab_to_use,
                        empty_application_nb,
                        tree_altitude + 1,
                        op_stack, op_stack_ref,
                        drv);

    }
}

////////////// Point d'entrée //////////////////////////////
struct nm_pkt_wrap *
nm_so_strategy_application(struct nm_gate *p_gate,
                           struct nm_drv *driver,
                           p_tbx_slist_t pre_list){
    int i;

    struct nm_so_op_stack *op_stack =
        TBX_MALLOC(sizeof(struct nm_so_op_stack));
    op_stack->op_stack = nm_so_stack_create(MAX_NB_OP);
    op_stack->score = 0;

    struct nm_so_op_stack *op_stack_ref =
        TBX_MALLOC(sizeof(struct nm_so_op_stack));
    op_stack_ref->op_stack = nm_so_stack_create(NB_PACK_BY_OPT);
    op_stack_ref->score = 0;

    int pre_list_len = tbx_slist_get_length(pre_list);

    struct nm_so_pending_tab *cpy_pw_tab = NULL;
    struct nm_so_pending_tab *pw_tab = NULL;
    pw_tab = TBX_MALLOC(sizeof(struct nm_so_pending_tab));
    cpy_pw_tab = TBX_MALLOC(sizeof(struct nm_so_pending_tab));

    pw_tab->nb_pending_pw =
        (pre_list_len < PENDING_PW_WINDOW_SIZE ?
         pre_list_len : PENDING_PW_WINDOW_SIZE);
    cpy_pw_tab->nb_pending_pw = pw_tab->nb_pending_pw;

    tbx_slist_ref_to_head(pre_list);
    for(i = 0; i < pw_tab->nb_pending_pw; i++){

        pw_tab->tab[i] = TBX_MALLOC(sizeof(struct nm_so_shorter_pw));
        pw_tab->tab[i]->index = i;
        pw_tab->tab[i]->src_pw = tbx_slist_ref_get(pre_list);
        assert(pw_tab->tab[i]->src_pw);
        pw_tab->tab[i]->len    = pw_tab->tab[i]->src_pw->length;
        pw_tab->tab[i]->nb_seg = pw_tab->tab[i]->src_pw->v_nb;


        cpy_pw_tab->tab[i] = TBX_MALLOC(sizeof(struct nm_so_shorter_pw));
        cpy_pw_tab->tab[i]->index  = pw_tab->tab[i]->index ;
        cpy_pw_tab->tab[i]->src_pw = pw_tab->tab[i]->src_pw;
        cpy_pw_tab->tab[i]->len    = pw_tab->tab[i]->len   ;
        cpy_pw_tab->tab[i]->nb_seg = pw_tab->tab[i]->nb_seg;

        tbx_slist_ref_forward(pre_list);
    }


    // construction de la suite optimale d'operations à appliquer
    nm_so_path_tree(0, pw_tab,
                    0, 0,
                    op_stack, op_stack_ref,
                    driver);


    // application de la strategie optimale
    struct nm_core         *p_core = driver->p_core;
    struct nm_pkt_wrap     *cur_pw = NULL;
    struct nm_pkt_wrap     *p_pw = NULL;
    struct nm_so_pkt_wrap  *so_pw = NULL;
    struct nm_so_operation *op = NULL;

    DISP_VAL("nm_so_strategy_application - ref_stack_size", nm_so_stack_size(op_stack_ref->op_stack));

    while(nm_so_stack_size(op_stack_ref->op_stack)){
        op = nm_so_stack_pop(op_stack_ref->op_stack);

        DISP_VAL("nm_so_strategy_application, type de l'op", op->operation_type);

        switch(op->operation_type){
        case nm_so_aggregation:
            p_pw = nm_so_take_aggregation_pw(p_gate->p_sched);
            so_pw = p_pw->sched_priv;

            for(i = 0; i < op->nb_pw; i++){
                cur_pw = cpy_pw_tab->tab[op->pw_idx[i]]->src_pw;

                nm_so_add_data(p_core, p_pw,
                               cur_pw->proto_id, cur_pw->length,
                               cur_pw->seq, cur_pw->v[0].iov_base);

                so_pw->aggregated_pws[so_pw->nb_aggregated_pws++] = cur_pw;
            }
            break;

        case nm_so_split:

        default:
            TBX_FAILURE("strategy_application failed");
        }
    }

    nm_so_update_global_header(p_pw, p_pw->v_nb, p_pw->length);

    return p_pw;
}
