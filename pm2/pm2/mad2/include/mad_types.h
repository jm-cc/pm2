
/*
 * Mad_types.h
 * ===========
 */

#ifndef MAD_TYPES_H
#define MAD_TYPES_H

typedef void *p_mad_driver_specific_t;

/*
 * Data structure elements
 * -----------------------
 */
typedef long    mad_link_id_t,              *p_mad_link_id_t;
typedef long    mad_channel_id_t,           *p_mad_channel_id_t;
typedef int     mad_adapter_id_t,           *p_mad_adapter_id_t;
typedef long    mad_cluster_id_t,           *p_mad_cluster_id_t;
typedef int     mad_configuration_size_t,   *p_mad_configuration_size_t;
typedef int     mad_buffer_alignment_t,     *p_mad_buffer_alignment_t;

#endif /* MAD_TYPES_H */
