
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

#ifndef LRPC_DEFS_EST_DEF
#define LRPC_DEFS_EST_DEF

#include <pm2.h>
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#ifndef PM2_RPC_DEFS_C
extern
#endif
BEGIN_LRPC_LIST
  TILE,
  RESULT,
  SPY,
  LOAD,
  QUIT
END_LRPC_LIST


/* TILE */

#define TILE_CACHE_SIZE 16
#define ENTSCALE_INT_SCALE_WIDTH 125
#define ENTSCALE_INT_ENTRY_WIDTH 40

enum { DISTRIB_RANDOM, DISTRIB_CYCLIC, DISTRIB_ON_FIRST };

typedef struct {
  gint pixelwidth;
  gint nbprocs;
  gint distrib;
} PixelizeValues;

typedef struct {
  gint run;
} PixelizeInterface;

typedef struct {
  gint x, y, w, h;
  gint width;
  guchar *data;
} PixelArea;

typedef void (*EntscaleIntCallbackFunc) (gint value, gpointer data);

typedef struct {
  GtkObject     *adjustment;
  GtkWidget     *entry;
  gint          constraint;
  EntscaleIntCallbackFunc	callback;
  gpointer	call_data;
} EntscaleIntData;

#define MAX_AREA_LEN     (64 * 64 * 4)  /* max tile size is 64 heith * 64 width * 4 bpp */
typedef guchar pixeldata[MAX_AREA_LEN];

LRPC_DECL_REQ(TILE, pixeldata data; int pixelwidth; int bpp; PixelArea area; int num;)
LRPC_DECL_RES(TILE,);

LRPC_DECL_REQ(RESULT, guchar *data; PixelArea area; int changed; int module;)
LRPC_DECL_RES(RESULT,);

LRPC_DECL_REQ(SPY,)
LRPC_DECL_RES(SPY,)

LRPC_DECL_REQ(LOAD, int load;)
LRPC_DECL_RES(LOAD,);

LRPC_DECL_REQ(QUIT,)
LRPC_DECL_RES(QUIT,)

#endif
