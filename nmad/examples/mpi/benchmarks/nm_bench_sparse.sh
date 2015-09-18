#! /bin/bash

mkdir -p ./out-sparse
hosts=william0,william1

benches="mpi_bench_sparse_vector"

s=1
smax=1024

while [ ${s} -le ${smax} ]; do
    echo "# ## ${s}"
    for b in ${benches}; do
	echo "# ## start: ${b} [ ${s} ]"
	padico-launch -n 2 -q -nodelist ${hosts} -DBLOCKSIZE=${s} ${b} -N 15 | tee out-sparse/out-${b}-${s}.dat
	rc=$?
	echo "# ## rc = ${rc}"
    done
    s=$(( ${s} * 2 ))
done
