
#include "rpc_defs.h"

/* FAC */

PACK_REQ_STUB(DICHOTOMY)
   old_mad_pack_int(MAD_BY_COPY, &arg->inf, 1);
   old_mad_pack_int(MAD_BY_COPY, &arg->sup, 1);
END_STUB

UNPACK_REQ_STUB(DICHOTOMY)
   old_mad_unpack_int(MAD_BY_COPY, &arg->inf, 1);
   old_mad_unpack_int(MAD_BY_COPY, &arg->sup, 1);
END_STUB

PACK_RES_STUB(DICHOTOMY)
   old_mad_pack_int(MAD_BY_COPY, &arg->res, 1);
END_STUB

UNPACK_RES_STUB(DICHOTOMY)
   old_mad_unpack_int(MAD_BY_COPY, &arg->res, 1);
END_STUB


