
case " $PM2_MARCEL_CFLAGS " in
    *\ -DMARCEL_SMP\ *)
	# Shall we use pthread ?
	case " $PM2_MARCEL_CFLAGS_KERNEL " in
	    *\ -DMARCEL_DONT_USE_POSIX_THREADS\ *)
		# No
		;;
	    *)
		# Yes, we should
		PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -D_REENTRANT"
		PM2_MARCEL_LIBS="$PM2_MARCEL_LIBS -lpthread"
		if [ "$PM2_SYS" = LINUX_SYS ]; then
		    PM2_LD_PRELOAD="${PM2_LD_PRELOAD:+${PM2_LD_PRELOAD}:}$HOME/lib/libpthread.so"
		fi
		;;
	esac
	;;
    *)
	;;
esac
