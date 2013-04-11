#! /bin/bash

# hardwired- oops
seq=5

NAS_ROOT=${PWD}/NAS
if [ ! -r ${NAS_ROOT}/MPI_dummy/mpi_dummy.f ]; then
    echo "# directory ${NAS_ROOT} doesn't look like a NAS-MPI directory"
    exit 1
fi

bench="$1"
name=$( echo $bench | cut -f 1 -d '.' )
class=$( echo $bench | cut -f 2 -d '.' )
nodes=$( echo $bench | cut -f 3 -d '.' )
name=${name,,}
class=${class^^}
# canonical benchmark name
bench=${name}.${class}.${nodes}

mkdir -p out/out-${$}-${bench}
touch out/result-${$}-${bench}.dat

echo "# NAS name = ${name} ; class = ${class} ; nodes = ${nodes}" >> out/result-${$}-${bench}.dat
echo >> out/result-${$}-${bench}.dat

if [ "x${MPIRUN}" = "x" ]; then
    MPIRUN=mpirun
fi
if [ "x${MPIROOT}" = "x" ]; then
    MPICC=mpicc
    MPIF77=mpif77
else
    MPICC="${MPIROOT}/bin/mpicc"
    MPIF77="${MPIROOT}/bin/mpif77"
    MPIRUN="${MPIROOT}/bin/${MPIRUN}"
fi

#MPIRUN="mpirun -launcher-exec oarsh"
#MPIRUN="mpirun -mca pml ob1 -mca plm_rsh_agent oarsh"
#MPIRUN="padico-launch -q -DNMAD_DRIVER=ibverbs -DNMAD_IBVERBS_CHECKSUM=fnv1a"
#MPIRUN="padico-launch  -DNMAD_DRIVER=ibverbs"

echo "# MPIROOT = ${MPIROOT}" >> out/result-${$}-${bench}.dat
echo "# MPICC   = ${MPICC}"   >> out/result-${$}-${bench}.dat
echo "# MPIF77  = ${MPIF77}"  >> out/result-${$}-${bench}.dat
echo "# MPIRUN  = ${MPIRUN}"  >> out/result-${$}-${bench}.dat
echo "# seq     = ${seq}"     >> out/result-${$}-${bench}.dat
echo                          >> out/result-${$}-${bench}.dat

cat out/result-${$}-${bench}.dat

if [ ! -d ${NAS_ROOT}/${name^^} ]; then
    echo "# unkonwn NAS benchmark ${name}"
    exit 1
fi
case ${class} in
    A|B|C|D|E|S)
	;;
    *)
	echo "# unkown NAS class ${class}"
	exit 1
	;;
esac
if [ ! "${nodes}" -gt 0 ]; then
    echo "# bad number of nodes ${nodes}"
    exit 1
fi


mv ${NAS_ROOT}/config/make.def ${NAS_ROOT}/config/make.def.old
cat > ${NAS_ROOT}/config/make.def <<EOF
# automatically generated
MPIF77     = ${MPIF77}
FLINK      = \$(MPIF77)
FFLAGS     = -O4
FLINKFLAGS = -O
FMPI_LIP   = 
FMPI_INC   = 

MPICC      = ${MPICC}
CLINK      = \$(MPICC)
CFLAGS     = -O4
CLINKFLAGS = -O
CMPI_LIB   =
CMPI_INC   =

CC         = gcc -g -Wall

BINDIR = ../bin

RAND = randi8

EOF


make -C ${NAS_ROOT} clean
rc=$?
if [ ${rc} != 0 ]; then
    exit 1
fi
make -C ${NAS_ROOT} ${name} NPROCS=${nodes} CLASS=${class}
rc=$?
if [ ${rc} != 0 ]; then
    exit 1
fi

#uniq $OAR_NODEFILE > ./machinefile

for n in `seq ${seq}`; do
    echo "# starting #$n"
    ${MPIRUN} -machinefile ./machinefile -n ${nodes} ${NAS_ROOT}/bin/${bench} | tee out/out-${$}-${bench}/${$}-${bench}-${n}
done

touch out/out-${$}-${bench}/${$}-${bench}-time

for n in `seq ${seq}`; do
    grep 'Time in' out/out-${$}-${bench}/${$}-${bench}-${n} >> out/out-${$}-${bench}/${$}-${bench}-time
done

cat out/out-${$}-${bench}/${$}-${bench}-time

cut -d '=' -f 2 out/out-${$}-${bench}/${$}-${bench}-time | sed -e 's/ //g' | sort >> out/result-${$}-${bench}.dat

echo
echo "------------------------"
echo
cat out/result-${$}-${bench}.dat

