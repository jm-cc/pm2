PM2_PROTOCOLS="$PM2_PROTOCOLS nmad-tcpdg"
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -DCONFIG_TCP"
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${PM2_ROOT}/nmad/drivers/tcpdg/include"
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -DDRV_TCPDG=mad_TCPDG"

