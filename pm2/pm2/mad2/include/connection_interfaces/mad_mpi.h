
/*
 * Mad_mpi.h
 * =========
 */
#ifndef MAD_MPI_H
#define MAD_MPI_H

/*
 * Structures
 * ----------
 */

/*
 * Functions 
 * ---------
 */

void
mad_mpi_register(p_mad_driver_t driver);

void
mad_mpi_driver_init(p_mad_driver_t);

void
mad_mpi_adapter_init(p_mad_adapter_t);

void
mad_mpi_adapter_configuration_init(p_mad_adapter_t);

/*----*/

void
mad_mpi_channel_init(p_mad_channel_t);

void
mad_mpi_before_open_channel(p_mad_channel_t);

void
mad_mpi_connection_init(p_mad_connection_t,
			p_mad_connection_t);

void
mad_mpi_link_init(p_mad_link_t);

void
mad_mpi_accept(p_mad_channel_t);

void
mad_mpi_connect(p_mad_connection_t);

void
mad_mpi_after_open_channel(p_mad_channel_t);

void
mad_mpi_before_close_channel(p_mad_channel_t);

void
mad_mpi_disconnect(p_mad_connection_t);

void
mad_mpi_after_close_channel(p_mad_channel_t);

void
mad_mpi_channel_exit(p_mad_channel_t);

void
mad_mpi_adapter_exit(p_mad_adapter_t);

void
mad_mpi_driver_exit(p_mad_driver_t);

p_mad_link_t
mad_mpi_choice(p_mad_connection_t,
	       size_t,
	       mad_send_mode_t,
	       mad_receive_mode_t);

void
mad_mpi_new_message(p_mad_connection_t);

p_mad_connection_t
mad_mpi_receive_message(p_mad_channel_t);

void
mad_mpi_send_buffer(p_mad_link_t,
		    p_mad_buffer_t);
  
void
mad_mpi_receive_buffer(p_mad_link_t,
		       p_mad_buffer_t *);

void
mad_mpi_send_buffer_group(p_mad_link_t,
			  p_mad_buffer_group_t);

void
mad_mpi_receive_sub_buffer_group(p_mad_link_t,
				 tbx_bool_t,
				 p_mad_buffer_group_t);

/*
 * p_mad_buffer_t
 * mad_mpi_get_static_buffer(p_mad_link_t lnk);
 *
 * void
 * mad_mpi_return_static_buffer(p_mad_link_t     lnk,
 *                             p_mad_buffer_t   buffer);
 */

void
mad_mpi_external_spawn_init(p_mad_adapter_t spawn_adapter,
			    int *argc, char **argv);

void
mad_mpi_configuration_init(p_mad_adapter_t       spawn_adapter,
			   p_mad_configuration_t configuration);

void
mad_mpi_send_adapter_parameter(p_mad_adapter_t  spawn_adapter,
			       ntbx_host_id_t   remote_host_id,
			       char            *parameter);

void
mad_mpi_receive_adapter_parameter(p_mad_adapter_t   spawn_adapter,
				  char            **parameter);

#endif /* MAD_MPI_H */
