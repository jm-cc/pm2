
#ifndef NETSERVER_EST_DEF
#define NETSERVER_EST_DEF

#include "marcel.h"
#include "madeleine.h"

enum {
   NETSERVER_END,
   NETSERVER_RAW_RPC  /* Must be the last one */
};

#ifdef MAD2
void netserver_start(p_mad_channel_t channel, unsigned priority);
#else
void netserver_start(unsigned priority);
#endif

void netserver_wait_end(void);

void netserver_stop(void);

#endif
