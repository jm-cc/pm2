PM2_MAD1_LIBNAME=mad
PM2_DEFAULT_LOADER=${PM2_ROOT}/mad1/bin/tcp/madload
PM2_MAD1_MODULE_DEPEND_LIB="${PM2_MAD1_MODULE_DEPEND_LIB} marcel"

    case ${PM2_SYS} in
    AIX_SYS)
	PM2_MAD1_LIBS="$PM2_MAD1_LIBS -lbsd"
	;;
    FREEBSD_SYS)
	;;
    IRIX_SYS)
	PM2_MAD1_LIBS="$PM2_MAD1_LIBS -lsun -lmutex"
	;;
    LINUX_SYS)
	;;
    OSF_SYS)
	;;
    SOLARIS_SYS)
	PM2_MAD1_LIBS="$PM2_MAD1_LIBS -lnsl -lsocket"
	;;
    UNICOS_SYS)
	;;
    esac
