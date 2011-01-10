if [ "x${IBHOME}" = "x" ]; then
  if [ "x${IB_ROOT}" != "x" ]; then
    IBHOME="${IB_ROOT}"
  elif [ -r /opt/OFED/include/infiniband/verbs.h ]; then
    IBHOME="/opt/OFED"
  elif [ -r /usr/local/include/infiniband/verbs.h ]; then
    IBHOME="/usr/local"
  else
    IBHOME="/usr"
  fi
fi

if [ "x${IBLIBPATH}" = "x" ]; then
  IBLIBPATH=${IBHOME}/lib
fi

PM2_PROTOCOLS="nmad-ibverbs $PM2_PROTOCOLS"
PM2_NMAD_DRIVERS="ibverbs $PM2_NMAD_DRIVERS"
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -march=native"
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -DCONFIG_IBVERBS"
if [ "x${IBHOME}" != "x/usr" ]; then
    PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${IBHOME}/include"
    PM2_NMAD_LD_PATH="$PM2_NMAD_LD_PATH -L${IBLIBPATH} -Wl,-rpath,${IBLIBPATH}"
fi
PM2_NMAD_LIBS="$PM2_NMAD_LIBS -libverbs"
PM2_NMAD_COMPONENTS="$PM2_NMAD_COMPONENTS NewMad_ibverbs_bycopy NewMad_ibverbs_rcache NewMad_ibverbs_adaptrdma NewMad_ibverbs_regrdma NewMad_ibverbs_auto NewMad_ibverbs_lr2"
