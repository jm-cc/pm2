/*
 * leo_types.h
 * ----------
 */

#ifndef __LEO_TYPES_H
#define __LEO_TYPES_H

typedef void              *leo_file_handle_t;
typedef leo_file_handle_t *p_file_handle_t;

typedef void undefined_t, *p_undefined_t;

typedef struct s_leo_application_cluster
{
  char          *id;
  char          *executable;
  char          *path;
  p_tbx_slist_t  host_list;
  p_tbx_slist_t  channel_list;
} leo_application_cluster_t, *p_leo_application_cluster_t;

typedef struct s_leo_cluster_definition
{
  char                *id;
  p_tbx_slist_t        host_model_list;
  p_tbx_slist_t        host_list;
  p_leo_mad_module_t   mad_module;
  p_leo_swann_module_t swann_module;
} leo_cluster_definition_t, *p_leo_cluster_definition_t;

#endif /* __LEO_TYPES_H */
