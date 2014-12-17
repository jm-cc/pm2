#! /bin/bash

graphdir=graph500-2.1.4/mpi
nodeslist="4 8 16"
n=3
scale=10
binflavors="simple replicated replicated_csc"
mpilist="nmad-mini nmad-nothread nmad-pthread openmpi"

for mpi in ${mpilist}; do

    case ${mpi} in
	nmad-mini)
	    ( cd ${HOME}/soft/pm2/pm2/scripts ; ./pm2-build-packages --prefix=${HOME}/soft/x86_64 --purge ./nmad-mini.conf )
	    mpirun="mpirun -DNMAD_DRIVER=ibverbs"
	    mpicc=mpicc
	    mpilibs="-lmpio"
	    ;;
	nmad-nothread)
            ( cd ${HOME}/soft/pm2/pm2/scripts ; ./pm2-build-packages --prefix=${HOME}/soft/x86_64 --purge ./bench-nmad+pioman+nothread.conf )
            mpirun="mpirun -DNMAD_DRIVER=ibverbs"
            mpicc=mpicc
            mpilibs="-lmpio"
	    ;;
	nmad-pthread)
            ( cd ${HOME}/soft/pm2/pm2/scripts ; ./pm2-build-packages --prefix=${HOME}/soft/x86_64 --purge ./bench-nmad+pioman+pthread.conf )
            mpirun="mpirun -DNMAD_DRIVER=ibverbs"
            mpicc=mpicc
            mpilibs="-lmpio"
            ;;
	openmpi)
	    cat $OAR_NODE_FILE | uniq > ./nodes
	    mpirun="${HOME}/soft/openmpi-1.4.3/x86_64/bin/mpirun -machinefile ./nodes -mca plm_rsh_agent oarsh"
	    mpicc=${HOME}/soft/openmpi-1.4.3/x86_64/bin/mpicc
	    mpilibs=
	    ;;
    esac
    
    for f in ${binflavors}; do
	echo "# ## ${f}"
	export MPICC="${mpicc}"
	export MPI_LIBS="${mpilibs}"
	make -C ${graphdir} clean
	make -C ${graphdir} graph500_mpi_${f}
	sleep 1 # for sloppy NFS

	for nodes in ${nodeslist}; do
	    outdir=${nodes}x-${f}-${scale}
	    mkdir -p ${outdir}
	    for i in `seq ${n}`; do
		echo "# ## run #${i}/${n}"
		outfile=${outdir}/${mpi}+${i}.out
		${mpirun} -n ${nodes} ./graph500-2.1.4/mpi/graph500_mpi_${f} ${scale} | tee ${outfile}
		echo
		echo "# done."
		echo
	    done
	done
    done
done

