BIP_DIR=${BIP_DIR:-/usr/local/bip}
PM2_PROTOCOLS="$PM2_PROTOCOLS bip"
PM2_LOADER=$PM2_ROOT/mad3/bin/mad3bip_load
PM2_MAD3_CFLAGS="$PM2_MAD3_CFLAGS -I${BIP_DIR}/include"
PM2_MAD3_LIBS="$PM2_MAD3_LIBS -L${BIP_DIR}/lib"
PM2_MAD3_LIBS="$PM2_MAD3_LIBS -lbip"
PM2_MAD3_CFLAGS="$PM2_MAD3_CFLAGS -DDRV_BIP=mad_BIP"
PM2_MAD3_CFLAGS="$PM2_MAD3_CFLAGS -DEXTERNAL_SPAWN=DRV_BIP"
PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD3_MAD1_MAIN_PROTO=mad_BIP"
PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD3_MAD1_MAIN_PROTO_PARAM=NULL"
