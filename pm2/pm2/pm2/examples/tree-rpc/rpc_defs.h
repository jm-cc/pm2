
#ifndef LRPC_DEFS_EST_DEF
#define LRPC_DEFS_EST_DEF

#include "pm2.h"

BEGIN_LRPC_LIST
  DICHOTOMY
END_LRPC_LIST


/* FAC */

LRPC_DECL_REQ(DICHOTOMY, int inf; int sup;)
LRPC_DECL_RES(DICHOTOMY, int res;)


#endif
