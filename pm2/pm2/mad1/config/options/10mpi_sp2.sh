PM2_PROTOCOLS=mpi_sp2
SP2_DIR=${SP2_DIR:-/usr/lpp/ppe.poe}
PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -I${SP2_DIR}/include '-DMPI_Init(a,b)=poe_remote_main();MPI_Init(a,b)'"
PM2_MAD1_LIBS="$PM2_MAD1_LIBS -L${SP2_DIR}/lib -L${SP2_DIR}/lib/ip -lmpi"
PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -DNET_ARCH="'\"'mpi_sp2'\"'
PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -DNETINTERF_INIT=mad_mpi_netinterf_init"
