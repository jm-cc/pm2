
/*
 * Mad_forward_interface.h
 * =======================
 */

/* *** Note: Rajouter Lionel dans le copyright *** */
#ifndef MAD_FORWARD_INTERFACE_H
#define MAD_FORWARD_INTERFACE_H

void 
mad_forward(p_mad_channel_t in_channel);

p_mad_channel_t 
mad_find_channel(ntbx_host_id_t        remote_host_id,
		 p_mad_user_channel_t  user_channel,
		 ntbx_host_id_t       *gateway);

void
mad_polling(p_mad_channel_t in_channel);

/* *** Note : a supprimer des que possible *** */
extern p_mad_driver_interface_t fwd_interface;

#endif /* MAD_FORWARD_INTERFACE_H */

