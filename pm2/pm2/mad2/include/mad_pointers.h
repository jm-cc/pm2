
/*
 * Mad_pointers.h
 * ==============
 */

#ifndef MAD_POINTERS_H
#define MAD_POINTERS_H

/* ... Point-to-point connection ..................................... */
typedef struct s_mad_connection           *p_mad_connection_t;

/* ... Protocol generic interface .................................... */
typedef struct s_mad_driver_interface     *p_mad_driver_interface_t;

/* ... Transfer method ............................................... */
typedef struct s_mad_link                 *p_mad_link_t;

/* ... Protocol module ............................................... */
typedef struct s_mad_driver               *p_mad_driver_t;

/* ... Network interface card ........................................ */
typedef struct s_mad_adapter              *p_mad_adapter_t;

/* ... Cluster members ............................................... */
typedef struct s_mad_configuration        *p_mad_configuration_t;

/* ... Virtual buffers ............................................... */
typedef struct s_mad_buffer               *p_mad_buffer_t;

/* ... Virtual buffer pair ........................................... */
typedef struct s_mad_buffer_pair          *p_mad_buffer_pair_t;

/* ... Group of virtual buffers ...................................... */
typedef struct s_mad_buffer_group         *p_mad_buffer_group_t;

/* ... Communication channel ......................................... */
typedef struct s_mad_channel              *p_mad_channel_t;

/* ... Main madeleine object ......................................... */
typedef struct s_mad_madeleine            *p_mad_madeleine_t;

/* ... Madeleine module settings ..................................... */
typedef struct s_mad_settings             *p_mad_settings_t;

/* ... Adapter identification ........................................ */
typedef struct s_mad_adapter_description  *p_mad_adapter_description_t;

/* ... Set of adapter identifications ................................ */
typedef struct s_mad_adapter_set          *p_mad_adapter_set_t;

/* ... Channel supporting hosts ...................................... */
typedef struct s_mad_cluster              *p_mad_cluster_t;

/* ... Virtual channel characteristics ............................... */
typedef struct s_mad_channel_description  *p_mad_channel_description_t;

#ifdef MAD_FORWARDING
/* ... User channel characteristics .................................. */
typedef struct s_mad_user_channel         *p_mad_user_channel_t;
#endif /* MAD_FORWARDING */

#endif /* MAD_POINTERS_H */

