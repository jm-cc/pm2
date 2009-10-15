PM2_PROTOCOLS="$PM2_PROTOCOLS mad-mpi"
PM2_MAD3_CFLAGS="$PM2_MAD3_CFLAGS -DDRV_MPI=mad_MPI"
PM2_MAD3_CFLAGS="$PM2_MAD3_CFLAGS -D_AIX -D_AIX32 -D_IBMR2 -D_POWER -I/usr/lpp/\ppe.poe/include"
PM2_MAD3_LIBS="/usr/lpp/ppe.poe/lib/crt0.o $PM2_MAD3_LIBS -L/usr/lpp/\
ppe.poe/lib -L/usr/lpp/ppe.poe/lib/ip -lmpi -lvtd -lc -L/opt/lib/gcc-lib/rs6000\-ibm-aix4.1.4.0/2.7.2.3 -lgcc -lc -lg -nostdlib"
PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD3_MAD1_MAIN_PROTO=mad_MPI"
PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD3_MAD1_MAIN_PROTO_PARAM=NULL"
PM2_LOADER=$PM2_SRCROOT/mad3/bin/mad3mpl_load
