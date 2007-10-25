PM2_NMAD_LIBNAME=nmad
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${PM2_ROOT}/nmad/core/include"
# PM2_DEFAULT_LOADER=$PM2_ROOT/nmad/bin/madload
PM2_NMAD_MODULE_DEPEND_LIB="${PM2_NMAD_MODULE_DEPEND_LIB} tbx ntbx marcel"

# tcpdg is included by default
PM2_PROTOCOLS="$PM2_PROTOCOLS tcpdg"
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${PM2_ROOT}/nmad/drivers/tcpdg/include"
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${PM2_ROOT}/nmad/drivers/sampling/include"
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -DDRV_TCPDG=mad_TCPDG"

PM2_DEFAULT_LOADER=${PM2_ROOT}/nmad/bin/nmadload_conf_not_needed
