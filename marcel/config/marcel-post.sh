
if defined_in MARCEL_SMP PM2_MARCEL_CFLAGS || defined_in MARCEL_NUMA PM2_MARCEL_CFLAGS; then
    # Shall we use pthread ?
    if not_defined_in MARCEL_DONT_USE_POSIX_THREADS PM2_MARCEL_CFLAGS ; then
	# Yes, we should
	PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -D_REENTRANT"
	PM2_MARCEL_LIBS="$PM2_MARCEL_LIBS -lpthread"
	# For being able to bind kernel threads to cpus.
	[ "$PM2_SYS" = OSF_SYS ] && PM2_MARCEL_LIBS="$PM2_MARCEL_LIBS -lnuma"
    fi
fi

# since it pulls libpthread, we can not use -lrt if we are not to either use or
# provide a libpthread
if [ "$PM2_SYS" = LINUX_SYS ] &&
   not_defined_in MARCEL_DONT_USE_POSIX_THREADS PM2_MARCEL_CFLAGS &&
   defined_in MARCEL_POSIX PM2_MARCEL_CFLAGS ; then
    PM2_MARCEL_LIBS="$PM2_MARCEL_LIBS -lrt"
fi

if defined_in MARCEL_NUMA PM2_MARCEL_CFLAGS; then
    PM2_MARCEL_LIBS="$PM2_MARCEL_LIBS -lm"
fi

case "$PM2_SYS" in
    LINUX_SYS|GNU_SYS)
	GNU_LINKER=yes
	;;
esac

case "$PM2_SYS" in
    FREEBSD_SYS)
	PREFIX=_
	;;
esac

# can be set by 'pthread' option
PM2_MARCEL_LIBNAME=${PM2_MARCEL_LIBNAME:-marcel}

