
/*
 * Mad_channel_inteface
 * ====================
 */

#ifndef MAD_CHANNEL_INTERFACE_H
#define MAD_CHANNEL_INTERFACE_H

/*
 * Functions 
 * ---------
 */
#ifdef MAD_FORWARDING
p_mad_user_channel_t
mad_open_channel(p_mad_madeleine_t   madeleine,
                 char               *name);
#else /* MAD_FORWARDING */
p_mad_channel_t
mad_open_channel(p_mad_madeleine_t   madeleine,
		 mad_adapter_id_t    adapter);
#endif /* MAD_FORWARDING */

void
mad_close_channels(p_mad_madeleine_t madeleine);

#endif /* MAD_CHANNEL_INTERFACE_H */
