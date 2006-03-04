PM2_COMMON_LIBNAME=common

PM2_COMMON_CFLAGS="$PM2_COMMON_CFLAGS -Wall"

if [ "${PM2_ARCH}" = RS6K_ARCH ]; then
  PM2_COMMON_CFLAGS="$PM2_COMMON_CFLAGS -mno-powerpc"
fi

case "$PM2_SYS" in
    WIN*_SYS)
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
