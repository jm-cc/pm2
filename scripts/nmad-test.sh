#! /bin/sh

# automated nmad tests and benchmark

if [ "x${PM2_ROOT}" != "x" -a -r "${PM2_ROOT}" ]; then
    pm2_srcdir=${PM2_ROOT}
else
    echo "cannot find PM2 src dir- please set PM2_ROOT."
    exit 1
fi

usage() {
    cat 1>&2 <<EOF
usage: $0 <target> <driver> <host0> <host1>
  where <target> is 'bench' or 'tests'
EOF
}

# ## Compute parameters

driver=$2
if [ "x${driver}" = "x" ]; then
    usage
    exit 1
fi

target=$1
case ${target} in
    bench|tests)
	;;
    *)
	usage
	exit 1
	;;
esac

host0="$3"
host1="$4"
if [ "x${host0}" = "x" -o "x${host1}" = "x" ]; then
    usage
    exit 1
fi
NMAD_HOSTS=${host0},${host1}

tmpdir=`mktemp -d ${pm2_srcdir}/tmp-nmad-${USER}-XXXXXXXX`
if [ ! -w ${tmpdir} ]; then
    echo "$0: cannot write in $tempdir" 1>&2 
    exit 1
fi
trap "rm -r ${tmpdir}" EXIT
prefix=${tmpdir}
out=/tmp/nmad-${target}-${USER}-$$


# ## Build

( ./pm2-build-packages --prefix=${prefix} --purge - <<EOF
common_options="--disable-debug --enable-optimize"
cmd export MAKEFLAGS="-j 10"
padico_srcdir=${pm2_srcdir}/padicotm/externals/PadicoTM
pkg tbx
pkg \${padico_srcdir}/Puk --disable-trace
pkg \${padico_srcdir}/PukABI
pkg ${pm2_srcdir}/nmad --enable-mpi --enable-nuioa 
pkg \${padico_srcdir}/PadicoTM
cmd make -C \${prefix}/build/nmad/ examples
EOF
) | tee ${out}.build 2>&1
rc=$?
if [ ${rc} != 0 ]; then
    echo "$0: build failed." 1>&2
    echo
    cat ${out}.build
    exit 1
fi

# ## Run test/bench

export PATH=${prefix}/bin:${PATH}
export LD_LIBRARY_PATH=${prefix}/lib:${LD_LIBRARY_PATH}
export PKG_CONFIG_PATH=${prefix}/lib/pkgconfig:${PKG_CONFIG_PATH}
make -C ${prefix}/build/nmad ${target} NMAD_HOSTS=${NMAD_HOSTS} NMAD_DRIVER=${driver} | tee ${out}.result


# ## Post-processing for benchmarks

case ${target} in
    bench)
	echo
	echo "%-------------------"
	echo
	echo "Bench summary"
	echo
	grep "latency:" ${out}.result
	echo
	;;
esac
rm ${out}.result ${out}.build

