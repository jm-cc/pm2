GM_DIR=${GM_DIR:-/opt/gm}
PM2_PROTOCOLS="$PM2_PROTOCOLS gm"
PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -I${GM_DIR}/include"
PM2_MAD4_LIBS="$PM2_MAD4_LIBS -L${GM_DIR}/lib"
# PM2_MAD4_LIBS="$PM2_MAD4_LIBS -lgm"
PM2_MAD4_LIBS="$PM2_MAD4_LIBS ${GM_DIR}/lib/libgm.a"
PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -DDRV_GM=mad_GM"
PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD4_MAD1_MAIN_PROTO=mad_GM"
PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD4_MAD1_MAIN_PROTO_PARAM=NULL"
