/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

#ifndef LRPC_DEFS_EST_DEF
#define LRPC_DEFS_EST_DEF

#include <pm2.h>
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

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
