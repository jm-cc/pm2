#! /bin/bash

mkdir -p ./out-flypack
hosts=william0,william1

benches="dataprop flypack_local flypack_copy flypack_slicer flypack_generator flypack manpack"

s=1
smax=1024

while [ ${s} -le ${smax} ]; do
    echo "# ## ${s}"
    for b in ${benches}; do
	echo "# ## start: ${b} [ ${s} ]"
	padico-launch -n 2 -q -nodelist ${hosts} -DFLYPACK_CHUNK_SIZE=${s} nm_bench_${b} -N 100 | tee out-flypack/out-${b}-${s}.dat
	rc=$?
	echo "# ## rc = ${rc}"
    done
    s=$(( ${s} * 2 ))
done
