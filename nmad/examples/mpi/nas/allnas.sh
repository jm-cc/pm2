#! /bin/sh

NAS_ROOT=${PWD}/NAS

list_name="bt dt is lu ft cg mg ep sp"
list_class="B"
list_nodes="4"
#list_mpi="openmpi-ob1 openmpi-csum madmpi madmpi-crc madmpi-xor madmpi-rcache"
#list_mpi="mpich2-nmad mpich2-nmad-fnv1a madmpi madmpi-fnv1a madmpi-rcache openmpi-ob1 openmpi-csum"
list_mpi="madmpi"

if [ "x${OAR_NODEFILE}" != "x" ]; then
    uniq $OAR_NODEFILE > ./machinefile
fi
touch result-${$}.dat
echo "# `hostname`" >> result-${$}.dat
echo "# `date`"     >> result-${$}.dat

seq=5

nas_run_bench() {

    bench=${name}.${class}.${nodes}

    make -C ${NAS_ROOT}/ clean
    make -C ${NAS_ROOT}/ ${name} NPROCS=${nodes} CLASS=${class}

    case ${mpi} in
	openmpi-ob1)
	    mpirun="mpirun -mca pml ob1 -mca plm_rsh_agent oarsh -machinefile ./machinefile -n ${nodes}"
	    ;;
	openmpi-csum)
	    mpirun="mpirun -mca pml csum -mca plm_rsh_agent oarsh -machinefile ./machinefile -n ${nodes}"
	    ;;
	madmpi)
	    mpirun="padico-launch -q -n ${nodes} -machinefile ./machinefile -DNMAD_DRIVER=ib" #-DPIOM_ENABLE_PROGRESSION=1 -DPIOM_BUSY_WAIT_USEC=1000 -DPIOM_BUSY_WAIT_GRANULARITY=100 -DPIOM_IDLE_GRANULARITY=1
	    ;;
	madmpi-crc)
	    mpirun="padico-launch -q -n ${nodes} -machinefile ./machinefile -DNMAD_DRIVER=ib -DNMAD_IBVERBS_CHECKSUM=crc"
	    ;;
	madmpi-fnv1a)
	    mpirun="padico-launch -q -n ${nodes} -machinefile ./machinefile -DNMAD_DRIVER=ib -DNMAD_IBVERBS_CHECKSUM=fnv1a"
	    ;;
	madmpi-rcache)
	    mpirun="padico-launch -q -n ${nodes} -machinefile ./machinefile -DNMAD_DRIVER=ib -DNMAD_IBVERBS_RCACHE=1"
	    ;;
	mpich2-nmad)
	    mpirun="mpirun -launcher-exec oarsh -env NMAD_DRIVER ibverbs -machinefile ./machinefile -n ${nodes}"
	    ;;
	mpich2-nmad-crc)
	    mpirun="mpirun -launcher-exec oarsh -env NMAD_DRIVER ibverbs -env NMAD_IBVERBS_CHECKSUM crc -machinefile ./machinefile -n ${nodes}"
	    ;;
	mpich2-nmad-fnv1a)
	    mpirun="mpirun -launcher-exec oarsh -env NMAD_DRIVER=ibverbs -env NMAD_IBVERBS_CHECKSUM fnv1a -machinefile ./machinefile -n ${nodes}"
	    ;;
	*)
	    echo "# unknown mpi: ${mpi}"
	    exit 1
	    ;;
    esac

    touch out/out-${$}-${mpi}-${bench}-time
    for n in `seq ${seq}`; do
	echo "# ## starting ${bench} (${mpi}) #$n"
	${mpirun} ${NAS_ROOT}/bin/${bench} | tee out/out-${$}-${mpi}-${bench}-${n}
	grep 'Time in' out/out-${$}-${mpi}-${bench}-${n} >> out/out-${$}-${mpi}-${bench}-time
    done
    cat out/out-${$}-${mpi}-${bench}-time

    t="`cut -d '=' -f 2 out/out-${$}-${mpi}-${bench}-time | sed -e 's/ //g' | sort | head -1`"
    echo "${mpi} ${bench} ${t}" >> result-${$}.dat
    cat result-${$}.dat
}

for mpi in ${list_mpi}; do
    echo "# preparing mpi: ${mpi}"
    case ${mpi} in
	openmpi*)
	    make -C ${HOME}/soft/x86_64/build/openmpi-1.4.3 install
	    ;;
	madmpi*)
	    make -C ${HOME}/soft/x86_64/build/nmad install
	    ;;
	mpich2*)
	    make -C ${HOME}/soft/x86_64/build/mpich2 install
	    ;;
    esac
    for name in ${list_name}; do
	for class in ${list_class}; do
	    for nodes in ${list_nodes}; do
		echo "# ## ## mpi: ${mpi}; name: ${name}; class: ${class}; nodes: ${nodes}"
		nas_run_bench
	    done
	done
    done
done

echo "# `date`"     >> result-${$}.dat

