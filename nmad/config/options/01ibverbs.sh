if [ "x${IBHOME}" = "x" ]; then
  if [ "x${IB_ROOT}" != "x" ]; then
    IBHOME="${IB_ROOT}"
  elif [ -r /usr/local/include/infiniband/verbs.h ]; then
    IBHOME="/usr/local"
  else
    IBHOME="/usr"
  fi
fi

if [ "x${IBLIBPATH}" = "x" ]; then
  IBLIBPATH=${IBHOME}/lib
fi

PM2_PROTOCOLS="ibverbs $PM2_PROTOCOLS"
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -DCONFIG_IBVERBS"
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${PM2_ROOT}/nmad/drivers/ibverbs/include -I${IBHOME}/include"
PM2_NMAD_LD_PATH="$PM2_NMAD_LD_PATH -L${IBLIBPATH}"
PM2_NMAD_LIBS="$PM2_NMAD_LIBS -libverbs"

