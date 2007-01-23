MX_DIR=${MX_DIR:-/opt/mx}
PM2_PROTOCOLS="$PM2_PROTOCOLS mx"
PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -DCONFIG_MX"
PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -I${PM2_ROOT}/nmad/drivers/mx/include -I${MX_DIR}/include"
PM2_MAD4_LD_PATH="$PM2_MAD4_LD_PATH -L${MX_DIR}/lib"
PM2_MAD4_LIBS="$PM2_MAD4_LIBS -lmyriexpress"

PM2_MAD4_DYN_LIBS_DIR="${MX_DIR}/lib"

# mx_open_endpoint calls pthread_create, we need to link libpthread
# (or disable the use of pthread in MX)

#PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -D_REENTRANT"
#PM2_MAD4_LIBS="$PM2_MAD4_LIBS -lpthread"

