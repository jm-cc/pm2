MX_DIR=${MX_DIR:-/opt/mx}
PM2_PROTOCOLS="nmad-mx $PM2_PROTOCOLS"
PM2_NMAD_DRIVERS="mx $PM2_NMAD_DRIVERS"
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -DCONFIG_MX"
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${MX_DIR}/include"
PM2_NMAD_LD_PATH="$PM2_NMAD_LD_PATH -L${MX_DIR}/lib -Wl,-rpath -Wl,${MX_DIR}/lib"
PM2_NMAD_LIBS="$PM2_NMAD_LIBS -lmyriexpress"

PM2_NMAD_DYN_LIBS_DIR="${MX_DIR}/lib"

# mx_open_endpoint calls pthread_create, we need to link libpthread
# (or disable the use of pthread in MX)

#PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -D_REENTRANT"
#PM2_NMAD_LIBS="$PM2_NMAD_LIBS -lpthread"

