PM2_MAD1_LIBNAME=mad
PM2_MAD1_POST_CONF=true
PM2_MAD1_POST_CONF_FUNC() {
    prot="$PM2_PROTOCOLS"
    PROT="`echo $prot | tr [a-z] [A-Z]`"
    BIP_DIR=${BIP_DIR:-/usr/local/bip}
    LAM_DIR=${LAM_DIR:-${HOME}/lam}
    SP2_DIR=${SP2_DIR:-/usr/lpp/ppe.poe}
    SISCI_DIR=${SISCI_DIR:-${HOME}/DIS/src/SISCI}
    case "$prot" in
    bip)
	PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -I${BIP_DIR}/include"
	PM2_MAD1_LIBS="$PM2_MAD1_LIBS -L${BIP_DIR}/lib -lbip"
	;;
    mpi-bip)
	PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -I${BIP_DIR}/include -I${BIP_DIR}/include/mpi"
	PM2_MAD1_LIBS="$PM2_MAD1_LIBS -L${BIP_DIR}/lib -L${BIP_DIR}/lib/mpi_gnu -lmpi -lbip"
	;;
    mpi-lam)
	PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -I${LAM_DIR}/h"
	PM2_MAD1_LIBS="$PM2_MAD1_LIBS -L${LAM_DIR}/lib -lmpi -largs -ltrillium -ltstdio -lt"
	;;
    mpi-sgi)
	PM2_MAD1_LIBS="$PM2_MAD1_LIBS -lmpi"
	;;
    mpi-sp2)
	PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -I${SP2_DIR}/include '-DMPI_Init(a,b)=poe_remote_main();MPI_Init(a,b)'"
	PM2_MAD1_LIBS="$PM2_MAD1_LIBS -L${SP2_DIR}/lib -L${SP2_DIR}/lib/ip -lmpi"	
	;;
    pvm)
	PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -I${PVM_ROOT}/include"
	PM2_MAD1_LIBS="$PM2_MAD1_LIBS -L${PVM_ROOT}/lib/${PVM_ARCH} -lpvm3"
	;;
    sbp)
	PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -DCANT_SEND_TO_SELF"
	PM2_MAD1_LIBS="$PM2_MAD1_LIBS -lded"
	;;
    shm)
	PM2_MAD1_LIBS="$PM2_MAD1_LIBS -lposix4"
	;;
    sisci)
	PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -DCANT_SEND_TO_SELF -I${SISCI_DIR}/api -I${SISCI_DIR}/src"
	PM2_MAD1_LIBS="$PM2_MAD1_LIBS -L${SISCI_DIR}/api -lsisci"	
	;;
    tcp)
	;;
    via)
	PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -DMAD_VIA -D_REENTRANT -DCANT_SEND_TO_SELF -DVIA_DEV_NAME="'\"/dev/via_eth0\"'
	PM2_MAD1_LIBS="$PM2_MAD1_LIBS -lvipl -lpthread"
	;;
    *)
	echo "Protocol $prot invalid for mad1" 1>&2
	exit 1
	;;
    esac
    PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -DNET_ARCH="'\"'${prot}'\"'
    case $prot in
    mpi-*)
	PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -DNETINTERF_INIT=mad_mpi_netinterf_init"
	;;
    *)
	PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -DNETINTERF_INIT=mad_${prot}_netinterf_init"
	;;
    esac

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
}
