PM2_PROTOCOLS="$PM2_PROTOCOLS nmad-sisci"
SISCI_PATH=${SISCI_PATH:-/opt/DIS}
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -DCONFIG_SISCI"
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${PM2_ROOT}/nmad/drivers/sisci/include -I${SISCI_PATH}/include"
PM2_NMAD_LD_PATH="$PM2_NMAD_LD_PATH -L${SISCI_PATH}/lib"
PM2_NMAD_LIBS="$PM2_NMAD_LIBS -lsisci"
