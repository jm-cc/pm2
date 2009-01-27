
if defined_in MARCEL_SMP PM2_MARCEL_CFLAGS || defined_in MARCEL_NUMA PM2_MARCEL_CFLAGS; then
    # Shall we use pthread ?
    if not_defined_in MARCEL_DONT_USE_POSIX_THREADS PM2_MARCEL_CFLAGS ; then
	# Yes, we should
	PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -D_REENTRANT"
	PM2_MARCEL_LIBS="$PM2_MARCEL_LIBS -lpthread"
	if [ "$PM2_SYS" = LINUX_SYS -a "$PM2_ARCH" != IA64_ARCH ]; then
	    if nm `ldd /bin/ls | grep pthread` 2> /dev/null | grep -sq tls ; then
		# pthread with TLS used on this system, nothing to do
		:
	    else
		# old libpthread used.
		# Need a specific one to be able to move the stack
		PM2_LD_PRELOAD="${PM2_LD_PRELOAD:+${PM2_LD_PRELOAD}:}$PM2_ROOT/lib/$PM2_SYS/$PM2_ARCH/libpthread.so"
	    fi
	fi
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
    CYGWIN*_SYS|MINGWWIN*_SYS|LINUX_SYS|GNU_SYS)
	GNU_LINKER=yes
	;;
esac

case "$PM2_SYS" in
    CYGWIN*_SYS|MINGWWIN*_SYS)
	CC=${PM2_CC:-gcc}
	case `$CC -dumpmachine` in
	    *mingw*)
		PM2_MARCEL_LIBS="$PM2_MARCEL_LIBS -lwsock32"
	    ;;
	esac
	;;
esac

case "$PM2_SYS" in
    CYGWIN*_SYS|MINGWWIN*_SYS|FREEBSD_SYS)
	PREFIX=_
	;;
esac

if [ "$PM2_SYS" != OSF_SYS ]; then
    if [ "$PM2_MARCEL_BUILD_DYNAMIC" = yes ]; then
	if [ "$GNU_LINKER" = yes ]; then
	    PM2_MARCEL_EARLY_LDFLAGS_KERNEL="${PM2_ROOT}/marcel/scripts/marcel$PREFIX.lds"
	else
	    PM2_MARCEL_EARLY_OBJECT_FILES_KERNEL="_marcel_link.pic"
	fi
    else
	if [ "$PM2_MARCEL_BUILD_STATIC" = yes ]; then
	    if [ "$GNU_LINKER" = yes ]; then
		PM2_MARCEL_EARLY_LDFLAGS="${PM2_ROOT}/marcel/scripts/marcel$PREFIX.lds"
	    else
		PM2_MARCEL_EARLY_OBJECT_FILES="_marcel_link.o"
	    fi
	fi
    fi
fi

# can be set by 'pthread' option
PM2_MARCEL_LIBNAME=${PM2_MARCEL_LIBNAME:-marcel}

