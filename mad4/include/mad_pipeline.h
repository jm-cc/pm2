#ifndef MAD_PIPELINE_H
#define MAD_PIPELINE_H

typedef struct s_mad_pipeline_t{
    void **pipeline;
    uint32_t       length;
    uint32_t       cur_nb_elm;
    uint32_t       begin;
    uint32_t       end;
}mad_pipeline_t, *p_mad_pipeline_t;


p_mad_pipeline_t
mad_pipeline_create(uint32_t);

void
mad_pipeline_free(p_mad_pipeline_t);

void *
mad_pipeline_get(p_mad_pipeline_t);

void
mad_pipeline_add(p_mad_pipeline_t, void *);

void *
mad_pipeline_remove(p_mad_pipeline_t);

#endif //MAD_PIPELINE_H
