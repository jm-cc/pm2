
#ifndef MAD_CLUSTER_H
#define MAD_CLUSTER_H

typedef enum
{
  mad_channel_default = 0,
} mad_channel_description_id_t, *p_mad_channel_description_id_t;

typedef struct s_mad_channel_description
{
  mad_channel_description_id_t    id;
  /* mad_channel_description_alias_t alias; */
  mad_driver_id_t                 driver_id;
  mad_adapter_id_t                adapter_id;
} mad_channel_description_t;

typedef struct s_mad_cluster
{
  TBX_SHARED;
  mad_cluster_id_t            id;
  mad_configuration_t         configuration;
  p_mad_channel_description_t channel_description_array;
} mad_cluster_t;

#endif /* MAD_CLUSTER_H */
