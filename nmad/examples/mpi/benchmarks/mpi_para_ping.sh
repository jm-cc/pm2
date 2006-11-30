MX_DISABLE_SELF=1 MX_DISABLE_SHMEM=1 MX_RCACHE=1 numactl --physcpubind=0 --membind=0 $PWD/mpi_para_ping 
