[ "$PM2_SYS" = IRIX_SYS ] && PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -fno-omit-frame-pointer"
PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -I${PM2_ROOT}/marcel/autogen-include -D__STDC_LIMIT_MACROS"

case " $PM2_LIBS " in
    *\ mad?\ *) ;;
    *) PM2_DEFAULT_LOADER=${PM2_ROOT}/marcel/bin/marcelload_conf_not_needed ;;
esac

if [ "$PM2_SYS" = SOLARIS_SYS ]; then
    PM2_MARCEL_LIBS="$PM2_MARCEL_LIBS -lsocket"
    if [ "$PM2_ARCH" = X86_ARCH ]; then
	PM2_MARCEL_LIBS="$PM2_MARCEL_LIBS -lrt"
    else
	PM2_MARCEL_LIBS="$PM2_MARCEL_LIBS -lposix4"
    fi
fi

if [ "$PM2_SYS" = OSF_SYS ]; then
    PM2_MARCEL_LIBS="$PM2_MARCEL_LIBS -lrt"
fi

if [ "$PM2_SYS" = LINUX_SYS ]; then
    VERSION=`uname -r | cut -d . -f 1`
    PATCHLEVEL=`uname -r | cut -d . -f 2`
    PM2_MARCEL_CFLAGS_KERNEL="$PM2_MARCEL_CFLAGS_KERNEL -DLINUX_VERSION=\"\\\"$VERSION.$PATCHLEVEL\\\"\""
    [ "$VERSION" -lt 2 -o "$PATCHLEVEL" -lt 6 ] && PM2_MARCEL_CFLAGS_KERNEL="$PM2_MARCEL_CFLAGS_KERNEL -DOLD_ITIMER_REAL"
fi

PM2_MARCEL_GENERATE_INCLUDE=true
