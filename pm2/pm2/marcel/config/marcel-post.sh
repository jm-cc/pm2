
if defined_in MARCEL_SMP PM2_MARCEL_CFLAGS ; then
    # Shall we use pthread ?
    if not_defined_in MARCEL_DONT_USE_POSIX_THREADS PM2_MARCEL_CFLAGS_KERNEL ; then
	# Yes, we should
	PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -D_REENTRANT"
	PM2_MARCEL_LIBS="$PM2_MARCEL_LIBS -lpthread"
	if [ "$PM2_SYS" = LINUX_SYS -a "$PM2_ARCH" != IA64_ARCH ]; then
	    PM2_LD_PRELOAD="${PM2_LD_PRELOAD:+${PM2_LD_PRELOAD}:}$PM2_ROOT/lib/$PM2_SYS/$PM2_ARCH/libpthread.so"
	fi
    fi
fi

# can be set by 'pthread' option
PM2_MARCEL_LIBNAME=${PM2_MARCEL_LIBNAME:-marcel}

