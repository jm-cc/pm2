
#include "rpc_defs.h"
#include <string.h>

static char ping_buf[MAX_BUF_SIZE] __MAD_ALIGNED__;

/* SAMPLE */

PACK_REQ_STUB(SAMPLE)
   old_mad_pack_str(MAD_IN_HEADER, arg->tab);
END_STUB

UNPACK_REQ_STUB(SAMPLE)
   old_mad_unpack_str(MAD_IN_HEADER, arg->tab);
END_STUB

PACK_RES_STUB(SAMPLE)
END_STUB

UNPACK_RES_STUB(SAMPLE)
END_STUB


/* INFO */

unsigned BUF_SIZE, NB_PING;

PACK_REQ_STUB(INFO)
   old_mad_pack_int(MAD_IN_HEADER, &BUF_SIZE, 1);
   old_mad_pack_int(MAD_IN_HEADER, &NB_PING, 1);
END_STUB

UNPACK_REQ_STUB(INFO)
   old_mad_unpack_int(MAD_IN_HEADER, &BUF_SIZE, 1);
   old_mad_unpack_int(MAD_IN_HEADER, &NB_PING, 1);
END_STUB

PACK_RES_STUB(INFO)
END_STUB

UNPACK_RES_STUB(INFO)
END_STUB


/* PING */

PACK_REQ_STUB(LRPC_PING)
   old_mad_pack_byte(BUF_PACKING, ping_buf, BUF_SIZE);
END_STUB

UNPACK_REQ_STUB(LRPC_PING)
   old_mad_unpack_byte(BUF_PACKING, ping_buf, BUF_SIZE);
END_STUB

PACK_RES_STUB(LRPC_PING)
   old_mad_pack_byte(BUF_PACKING, ping_buf, BUF_SIZE);
END_STUB

UNPACK_RES_STUB(LRPC_PING)
   old_mad_unpack_byte(BUF_PACKING, ping_buf, BUF_SIZE);
END_STUB


/* PING_ASYNC */

PACK_REQ_STUB(LRPC_PING_ASYNC)
   old_mad_pack_byte(BUF_PACKING, ping_buf, BUF_SIZE);
END_STUB

UNPACK_REQ_STUB(LRPC_PING_ASYNC)
   old_mad_unpack_byte(BUF_PACKING, ping_buf, BUF_SIZE);
END_STUB

PACK_RES_STUB(LRPC_PING_ASYNC)
END_STUB

UNPACK_RES_STUB(LRPC_PING_ASYNC)
END_STUB








