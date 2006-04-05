MX_DIR=${MX_DIR:-/opt/mx}
PM2_PROTOCOLS="$PM2_PROTOCOLS mx"
PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -I${MX_DIR}/include"
PM2_MAD4_LD_PATH="$PM2_MAD4_LD_PATH -L${MX_DIR}/lib"
# PM2_MAD4_LIBS="$PM2_MAD4_LIBS -lmyriexpress"
PM2_MAD4_LIBS="$PM2_MAD4_LIBS ${MX_DIR}/lib/libmyriexpress.a"
PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -DDRV_MX=mad_MX"
PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD4_MAD1_MAIN_PROTO=mad_MX"
PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD4_MAD1_MAIN_PROTO_PARAM=NULL"


# mx_open_endpoint calls pthread_create, we need to link libpthread
# (or disable the use of pthread in MX)

PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -D_REENTRANT"
PM2_MAD4_LIBS="$PM2_MAD4_LIBS -lpthread"
