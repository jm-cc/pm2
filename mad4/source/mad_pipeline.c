#include "madeleine.h"

p_mad_pipeline_t
mad_pipeline_create(uint32_t length){
    p_mad_pipeline_t pipeline = NULL;

    LOG_IN();
    pipeline = TBX_MALLOC(sizeof(mad_pipeline_t));
    pipeline->pipeline = TBX_MALLOC(length * sizeof(void *));
    pipeline->length = length;
    pipeline->cur_nb_elm = 0;
    pipeline->begin = 0;
    pipeline->end = 0;
    LOG_OUT();

    return pipeline;
}

void
mad_pipeline_free(p_mad_pipeline_t pipeline){
    LOG_IN();
    TBX_FREE(pipeline->pipeline);
    TBX_FREE(pipeline);
    LOG_OUT();
}



void *
mad_pipeline_get(p_mad_pipeline_t pipeline){
    void * object = NULL;
    LOG_IN();
    if(pipeline->cur_nb_elm)
        object =  pipeline->pipeline[pipeline->begin];
    LOG_OUT();
    return object;
}


void *
mad_pipeline_index_get(p_mad_pipeline_t pipeline,
                       uint32_t index){
    void * object = NULL;
    LOG_IN();
    //if(index <= pipeline->end)
    object =  pipeline->pipeline[(pipeline->begin + index) % pipeline->length];
    LOG_OUT();
    return object;
}




void
mad_pipeline_add(p_mad_pipeline_t pipeline,
                 void * object){
    LOG_IN();
    pipeline->pipeline[pipeline->end] = object;
    pipeline->end = (pipeline->end + 1) % pipeline->length;
    pipeline->cur_nb_elm++;
    LOG_OUT();
}


void *
mad_pipeline_extract(p_mad_pipeline_t pipeline,
                     uint32_t index){
    void *object = NULL;
    int i = 0;

    LOG_IN();
    //DISP("-->pipeline_extract");
    if(index >= pipeline->cur_nb_elm)
        FAILURE("mad_pipeline : index > nb of elm");

    if(index == 0){
        object = mad_pipeline_remove(pipeline);
        goto end;
    }

    object = pipeline->pipeline[index];

    // voir si on decale du debut ou de la fin!!!!
    for(i = index; i < pipeline->cur_nb_elm - 1; i++){
        pipeline[i] = pipeline[i+1];
    }

    pipeline->cur_nb_elm--;
    pipeline->end = (pipeline->end - 1) % pipeline->length;

 end:
    //DISP("<--pipeline_extract");
    LOG_OUT();
    return object;
}

void *
mad_pipeline_remove(p_mad_pipeline_t pipeline){
    void *object = NULL;
    LOG_IN();
    //DISP("-->pipeline_remove");
    if(!pipeline->cur_nb_elm)
        FAILURE("empty mad_pipeline");

    object = pipeline->pipeline[pipeline->begin];
    pipeline->pipeline[pipeline->begin] = NULL;
    pipeline->begin = (pipeline->begin + 1) % pipeline->length;
    pipeline->cur_nb_elm--;
    //DISP("<--pipeline_remove");
    LOG_OUT();
    return object;
}
