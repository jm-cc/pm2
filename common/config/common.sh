PM2_COMMON_LIBNAME=common

PM2_COMMON_CFLAGS="$PM2_COMMON_CFLAGS -Wall -fno-common"

if [ "${PM2_ARCH}" = RS6K_ARCH ]; then
  PM2_COMMON_CFLAGS="$PM2_COMMON_CFLAGS -mcpu=power"
fi

if [ "${PM2_ARCH}" = PPC_ARCH ]; then
  PM2_COMMON_CFLAGS="$PM2_COMMON_CFLAGS -mcpu=powerpc"
fi


case "$PM2_SYS" in
    CYGWIN*_SYS|MINGWIN*_SYS)
	PM2_COMMON_CFLAGS="$PM2_COMMON_CFLAGS -DWIN_SYS"
	;;
    DARWIN_SYS)
        PM2_COMMON_CFLAGS="$PM2_COMMON_CFLAGS -no-cpp-precomp"
	# -traditional-cpp makes darwin's gcc 3.3 crash
        ;;
    *)
	;;
esac

[ -n "$PM2_DEV" ] && PM2_COMMON_CFLAGS="$PM2_COMMON_CFLAGS -DPM2_DEV"
