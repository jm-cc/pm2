
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include "rpc_defs.h"
#include <string.h>

/* TILE */

PACK_REQ_STUB(TILE)

     old_mad_pack_int(MAD_IN_HEADER, &arg->area.x, 1);
     old_mad_pack_int(MAD_IN_HEADER, &arg->area.y, 1);
     old_mad_pack_int(MAD_IN_HEADER, &arg->area.w, 1);
     old_mad_pack_int(MAD_IN_HEADER, &arg->area.h, 1);

     old_mad_pack_int(MAD_IN_HEADER, &arg->bpp, 1);
     old_mad_pack_int(MAD_IN_HEADER, &arg->pixelwidth, 1);
     old_mad_pack_int(MAD_IN_HEADER, &arg->num, 1);

     old_mad_pack_byte(MAD_IN_PLACE, arg->data, MAX_AREA_LEN);

END_STUB

UNPACK_REQ_STUB(TILE)

     old_mad_unpack_int(MAD_IN_HEADER, &arg->area.x, 1);
     old_mad_unpack_int(MAD_IN_HEADER, &arg->area.y, 1);
     old_mad_unpack_int(MAD_IN_HEADER, &arg->area.w, 1);
     old_mad_unpack_int(MAD_IN_HEADER, &arg->area.h, 1);

     old_mad_unpack_int(MAD_IN_HEADER, &arg->bpp, 1);
     old_mad_unpack_int(MAD_IN_HEADER, &arg->pixelwidth, 1);
     old_mad_unpack_int(MAD_IN_HEADER, &arg->num, 1);

     old_mad_unpack_byte(MAD_IN_PLACE, arg->data, MAX_AREA_LEN);

     arg->area.data = arg->data;

END_STUB

PACK_RES_STUB(TILE)
END_STUB

UNPACK_RES_STUB(TILE)
END_STUB

/* RESULT */

static guchar _tempo[MAX_AREA_LEN];

PACK_REQ_STUB(RESULT)

     old_mad_pack_int(MAD_IN_HEADER, &arg->area.x, 1);
     old_mad_pack_int(MAD_IN_HEADER, &arg->area.y, 1);
     old_mad_pack_int(MAD_IN_HEADER, &arg->area.w, 1);
     old_mad_pack_int(MAD_IN_HEADER, &arg->area.h, 1);

     old_mad_pack_int(MAD_IN_HEADER, &arg->module, 1);
     old_mad_pack_int(MAD_IN_HEADER, &arg->changed, 1);

     if(arg->changed)
        old_mad_pack_byte(MAD_IN_PLACE, arg->data, MAX_AREA_LEN);

END_STUB

UNPACK_REQ_STUB(RESULT)

     old_mad_unpack_int(MAD_IN_HEADER, &arg->area.x, 1);
     old_mad_unpack_int(MAD_IN_HEADER, &arg->area.y, 1);
     old_mad_unpack_int(MAD_IN_HEADER, &arg->area.w, 1);
     old_mad_unpack_int(MAD_IN_HEADER, &arg->area.h, 1);

     old_mad_unpack_int(MAD_IN_HEADER, &arg->module, 1);
     old_mad_unpack_int(MAD_IN_HEADER, &arg->changed, 1);

     if(arg->changed)
        old_mad_unpack_byte(MAD_IN_PLACE, _tempo, MAX_AREA_LEN);

     arg->data = _tempo; /* en mode quick, c'est ok ! */

END_STUB

PACK_RES_STUB(RESULT)
END_STUB

UNPACK_RES_STUB(RESULT)
END_STUB

/* SPY */

PACK_REQ_STUB(SPY)
END_STUB

UNPACK_REQ_STUB(SPY)
END_STUB

PACK_RES_STUB(SPY)
END_STUB

UNPACK_RES_STUB(SPY)
END_STUB

/* LOAD */

PACK_REQ_STUB(LOAD)
     old_mad_pack_int(MAD_IN_HEADER, &arg->load, 1);
END_STUB

UNPACK_REQ_STUB(LOAD)
     old_mad_unpack_int(MAD_IN_HEADER, &arg->load, 1);
END_STUB

PACK_RES_STUB(LOAD)
END_STUB

UNPACK_RES_STUB(LOAD)
END_STUB

/* QUIT */

PACK_REQ_STUB(QUIT)
END_STUB

UNPACK_REQ_STUB(QUIT)
END_STUB

PACK_RES_STUB(QUIT)
END_STUB

UNPACK_RES_STUB(QUIT)
END_STUB

