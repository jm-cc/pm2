if [ "x${IBHOME}" = "x" ]; then
  if [ "x${IB_ROOT}" != "x" ]; then
    IBHOME="${IB_ROOT}"
  elif [ -r /usr/local/include/infiniband/verbs.h ]; then
    IBHOME="/usr/local"
  else
    IBHOME="/usr"
  fi
fi
PM2_PROTOCOLS="ibverbs $PM2_PROTOCOLS"
PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -DCONFIG_IBVERBS"
PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -I${PM2_ROOT}/nmad/drivers/ibverbs/include -I${IBHOME}/include"
PM2_MAD4_LD_PATH="$PM2_MAD4_LD_PATH -L${IBHOME}/lib"
PM2_MAD4_LIBS="$PM2_MAD4_LIBS -libverbs"

