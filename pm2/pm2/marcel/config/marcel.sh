
case " $PM2_LIBS " in
    *\ mad?\ *) ;;
    *) PM2_DEFAULT_LOADER=${PM2_ROOT}/marcel/bin/marcelload ;;
esac

if [ "${PM2_ARCH}" = RS6K_ARCH ]; then
  PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -mno-powerpc"
fi
PM2_MARCEL_LIBNAME=marcel
PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -fomit-frame-pointer"
if [ "$PM2_SYS" = SOLARIS_SYS ]; then
  PM2_MARCEL_LIBS="$PM2_MARCEL_LIBS -lrt"
fi
