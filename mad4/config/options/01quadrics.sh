PM2_PROTOCOLS="$PM2_PROTOCOLS qsnet"

PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -DCONFIG_QSNET"

PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -I${PM2_ROOT}/nmad/drivers/qsnet/include"

PM2_MAD4_LIBS="$PM2_MAD4_LIBS -lelan"


NMAD_CMDLINE="$NMAD_CMDLINE CONFIG_QSNET=y"
