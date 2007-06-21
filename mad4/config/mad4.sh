PM2_MAD4_LIBNAME=mad
PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -I${PM2_ROOT}/nmad/core/include"
# PM2_DEFAULT_LOADER=$PM2_ROOT/mad4/bin/madload
PM2_MAD4_MODULE_DEPEND_LIB="${PM2_MAD4_MODULE_DEPEND_LIB} tbx ntbx marcel"

# tcpdg is included by default
PM2_PROTOCOLS="$PM2_PROTOCOLS tcpdg"
PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -I${PM2_ROOT}/nmad/drivers/tcpdg/include"
PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -I${PM2_ROOT}/nmad/drivers/sampling/include"
PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -DDRV_TCPDG=mad_TCPDG"

PM2_DEFAULT_LOADER=${PM2_ROOT}/mad4/bin/mad4load_conf_not_needed
