PM2_MAD2_LIBNAME=mad
PM2_MAD2_POST_CONF=true
PM2_MAD2_POST_CONF_FUNC() {
for prot in $PM2_PROTOCOLS; do
    PROT=`echo $prot | tr [a-z] [A-Z]`
    case $prot in
    mpi)
	PM2_CC=hcc
	;;
    sbp)
	PM2_MAD2_LIBS="$PM2_MAD2_LIBS -lded"
	;;
    sisci)
	SISCI_PATH=${SISCI_PATH:-${HOME}/DIS/src/SISCI}
	PM2_MAD2_CFLAGS="$PM2_MAD2_CFLAGS -I${SISCI_PATH}/api"
	PM2_MAD2_CFLAGS="$PM2_MAD2_CFLAGS -I${SISCI_PATH}/src"
	PM2_MAD2_LIBS="$PM2_MAD2_LIBS -L${SISCI_PATH}/api -lsisci"
	;;
    tcp)
	if [ "${PM2_SYS}" = SOLARIS_SYS ]; then
	    PM2_MAD2_LIBS="$PM2_MAD2_LIBS -lnsl -lsocket"
	fi
	;;
    via)
	PM2_MAD2_CFLAGS="$PM2_MAD2_CFLAGS -D_REENTRANT"
	PM2_MAD2_LIBS="$PM2_MAD2_LIBS -lvipl -lpthread"
	;;
    *)
	echo "Protocol $prot invalid for mad2" 1>&2
	exit 1
	;;
    esac
    case $prot in
    mpi|sbp|sisci|tcp|via)
	PM2_MAD2_CFLAGS="$PM2_MAD2_CFLAGS -DDRV_${PROT}=mad_${PROT}"
	PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD2_MAD1_MAIN_PROTO=mad_${PROT}"
	if [ $prot != via ]; then
	PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD2_MAD1_MAIN_PROTO_PARAM=NULL"
	else
	PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD2_MAD1_MAIN_PROTO_PARAM="'\"/dev/via_eth0\"'
	fi
	;;
    esac
    case $prot in
    mpi|sbp)
	PM2_MAD2_CFLAGS="$PM2_MAD2_CFLAGS -DEXTERNAL_SPAWN=DRV_${PROT}"
	;;
    esac
done
}
