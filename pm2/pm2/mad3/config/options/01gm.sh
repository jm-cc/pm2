GM_DIR=${GM_DIR:-~/gm}
PM2_PROTOCOLS="$PM2_PROTOCOLS gm"
PM2_MAD3_CFLAGS="$PM2_MAD3_CFLAGS -I${GM_DIR}/include"
PM2_MAD3_LIBS="$PM2_MAD3_LIBS -L${GM_DIR}/lib"
PM2_MAD3_LIBS="$PM2_MAD3_LIBS -lgm"
PM2_MAD3_CFLAGS="$PM2_MAD3_CFLAGS -DDRV_GM=mad_GM"
PM2_MAD3_CFLAGS="$PM2_MAD3_CFLAGS -DEXTERNAL_SPAWN=DRV_GM"
PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD3_MAD1_MAIN_PROTO=mad_GM"
PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD3_MAD1_MAIN_PROTO_PARAM=NULL"
