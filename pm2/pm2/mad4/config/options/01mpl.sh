PM2_PROTOCOLS="$PM2_PROTOCOLS mpi"
PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -DDRV_MPI=mad_MPI"
PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -D_AIX -D_AIX32 -D_IBMR2 -D_POWER -I/usr/lpp/\ppe.poe/include"
PM2_MAD4_LIBS="/usr/lpp/ppe.poe/lib/crt0.o $PM2_MAD4_LIBS -L/usr/lpp/\
ppe.poe/lib -L/usr/lpp/ppe.poe/lib/ip -lmpi -lvtd -lc -L/opt/lib/gcc-lib/rs6000\-ibm-aix4.1.4.0/2.7.2.3 -lgcc -lc -lg -nostdlib"
PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD4_MAD1_MAIN_PROTO=mad_MPI"
PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD4_MAD1_MAIN_PROTO_PARAM=NULL"
PM2_LOADER=$PM2_ROOT/mad4/bin/mad4mpl_load
