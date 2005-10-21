#ifndef MAD_OPTIMIZER_H
#define MAD_OPTIMIZER_H

p_mad_iovec_t
mad_s_optimize(p_mad_adapter_t);

void
initialize_tracks(p_mad_adapter_t);

void
initialize_network_benchmarcks(p_mad_driver_t);

#endif // MAD_OPTIMIZER_H
