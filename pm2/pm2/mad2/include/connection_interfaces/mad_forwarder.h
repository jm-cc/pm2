
/*
 * Mad_forwarder.h
 * ===============
 */
#ifndef MAD_FORWARDER_H
#define MAD_FORWARDER_H

/*
 * Structures
 * ----------
 */

/*
 * Functions 
 * ---------
 */

void
mad_fwd_link_init(p_mad_link_t);

void
mad_fwd_new_message(p_mad_connection_t);

void
mad_fwd_send_buffer(p_mad_link_t,
		    p_mad_buffer_t);

void
mad_fwd_send_buffer_group(p_mad_link_t,
			  p_mad_buffer_group_t);

void
mad_fwd_receive_buffer(p_mad_link_t,
		       p_mad_buffer_t *);

void
mad_fwd_receive_sub_buffer_group(p_mad_link_t,
				 tbx_bool_t,
				 p_mad_buffer_group_t);

#endif /* MAD_FORWARDER_H */
