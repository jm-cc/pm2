/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                           

  NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

/*
 * leo_parser_types.h
 * ------------------
 */

#ifndef __LEO_PARSER_TYPES_H
#define __LEO_PARSER_TYPES_H

typedef enum
{
  leo_pro_adpt_none = 0,
  leo_pro_adpt_suffix,
  leo_pro_adpt_selector,
} leo_pro_adapter_selection_t, *p_leo_pro_adapter_selection_t;

typedef struct
{
  char  *name;
  char  *rule;
} leo_app_makefile_t, *p_leo_app_makefile_t;

typedef struct
{
  char  *alias;
  char  *adapter;
} leo_app_channel_t, *p_leo_app_channel_t;

typedef struct
{
  char                 *id;
  char                 *executable;
  char                 *path;
  p_tbx_list_t          hosts;
  p_tbx_list_t          channels;
  p_leo_app_makefile_t  makefile;
} leo_app_cluster_t, *p_leo_app_cluster_t;

typedef struct
{
  char         *id;
  p_tbx_list_t  cluster_list;
} leo_app_application_t, *p_leo_app_application_t;

typedef struct
{
  p_tbx_list_t  name_list;
  char         *model;
  char         *alias;
} leo_clu_host_name_t, *p_leo_clu_host_name_t;

typedef struct
{
  char         *id;
  p_tbx_list_t  hosts;
} leo_clu_cluster_t, *p_leo_clu_cluster_t;

typedef struct
{
  char *alias;
  char *protocol;
} leo_clu_model_adapter_t, *p_leo_clu_model_adapter_t;

typedef struct
{
  char         *id;
  p_tbx_list_t  domain_list;
  char         *model;
  char         *os;
  char         *archi;
  p_tbx_list_t  adapter_list;
  char         *nfs;
} leo_clu_host_model_t, *p_leo_clu_host_model_t;

typedef struct
{
  p_tbx_list_t        host_model_list;
  p_leo_clu_cluster_t cluster;
} leo_clu_cluster_file_t, * p_leo_clu_cluster_file_t;

typedef struct
{
  char        *alias;
  char        *macro;
  char        *module_name;
  char        *register_func;
  leo_pro_adapter_selection_t  adapter_select;
  tbx_bool_t   external_spawn;
} leo_pro_protocol_t, *p_leo_pro_protocol_t;

typedef struct
{
  p_leo_app_application_t  application;
  p_leo_clu_cluster_file_t cluster_file;
  p_tbx_list_t             protocol_table;
} leo_parser_result_t, *p_leo_parser_result_t;

#endif /* __LEO_PARSER_TYPES_H */
