#! /bin/bash
export LC_ALL=C

file="$1"
outdir="${file}.d"

if [ ! -r "${file}" ]; then
    echo "# ${0}: cannot read file '${file}'." >&2
    exit 1
fi

mkdir -p ${outdir}

bench_list=$( sed -e 's/# bench: \(mpi_bench_[a-zA-Z0-9_]*\) begin/\1/p;d' ${file} | tr '\n\r' ' ' )

echo
echo "# ## parsing file ${file}..."
echo 
echo "# ## bench list:"
echo "# ${bench_list}"
echo 

echo
echo "# ## generating normalized files..."
echo
for b in ${bench_list}; do
    echo "# processing bench: ${b}"
    outfile=${outdir}/${b}.dat
    echo "# This file is automatically generated from ${file} for ${b} in machine-readable format." > ${outfile}
    echo "# Generated on $( date ) on host $( hostname )" >> ${outfile}
    sed -r -f /dev/stdin ${file} >> ${outfile} <<EOF
/${b} begin/,/${b} end/ !d
s/^[ \t]*//g
s/\r//g;s/\t/ /g
/^#/!s/[ \t]+/\t/g
EOF
    echo "# ${b}: done."
    echo
done

echo
echo "# ## computing overlap ratio..."
echo

size_list=$( sed -e "/^#/ d" ${outdir}/mpi_bench_sendrecv.dat | cut -f 1 | tr '\n' ' ' )
echo "# sizes: ${size_list}"

mpi_bench_extract_line() {
    b=$1
    s=$2
    benchfile=${outdir}/${b}.dat
    sed -e "/${b} begin/,/${b} end/ !d;/^#/ d;/^${s}\t/ !d" ${benchfile}
}

mpi_bench_extract_line_param() {
    b=$1
    s=$2
    p=$3
    benchfile=${outdir}/${b}.dat
    sed -e "/${b}\/${p} begin/,/${b}\/${p} end/ !d;/^#/ d;/^${s}\t/ !d" ${benchfile}
}

for b in mpi_bench_overlap_sender mpi_bench_overlap_recv mpi_bench_overlap_bidir mpi_bench_overlap_sender_noncontig; do
    case ${b} in
	*noncontig*)
	    benchref=mpi_bench_noncontig
	    ;;
	*)
	    benchref=mpi_bench_sendrecv
	    ;;
    esac
    benchfile=${outdir}/${b}.dat
    echo "# file ${benchfile}"
    params=$( sed -e "s+^# bench: ${b}/\(.*\) begin+\1+p;d" ${benchfile} | tr '\n' ' ' )
    echo "# params list: ${params}"
    latref0=$( mpi_bench_extract_line ${benchref} 0 | cut -f 2 )
    echo "# generated from ${b}"                    > ${outdir}/${b}-ratio2d.dat
    echo "# size | comp.time | ratio | rtt | lat0" >> ${outdir}/${b}-ratio2d.dat
    for s in ${size_list}; do
	echo "# size: ${s}"
	latref=$( mpi_bench_extract_line ${benchref} ${s} | cut -f 2 )
	int_latref=$( echo ${latref} | cut -f 1 -d '.' )
	echo "# generated from ${b} for size=${s}" > ${outdir}/${b}-s${s}.dat
	echo "# reference latency: ${latref}"     >> ${outdir}/${b}-s${s}.dat
	echo "# comp.time | ratio | rtt."         >> ${outdir}/${b}-s${s}.dat
	for p in ${params}; do
	    rtt=$( mpi_bench_extract_line_param ${b} ${s} ${p} | cut -f 2 )
	    ratio=$( bc << EOF
scale=2
lat = ${rtt} - ${latref0}
if( ${p} < ${latref}) {
  if(${p} > 0 && lat > ${latref}) (( lat - ${latref} ) / ${p}) else 0
} else {
  if(lat > ${p}) (( lat - ${p} ) / ${latref}) else 0
}
EOF
		 )
	    echo "${p} ${ratio} ${rtt}"                >> ${outdir}/${b}-s${s}.dat
	    echo "${s} ${p} ${ratio} ${rtt} ${latref}" >> ${outdir}/${b}-ratio2d.dat
	done
	echo  >> ${outdir}/${b}-ratio2d.dat
    done

    gnuplot <<EOF
set term pdf
set output "${outdir}/${b}-ratio2d.pdf"
set view map
set pm3d interpolate 0,0
splot [2048:][0:20000][0:]  "${outdir}/${b}-ratio2d.dat" using 1:2:(\$3>2?2:\$3) with pm3d, "${outdir}/${benchref}.dat" using 1:2:(2) with linespoints lw 2
EOF
    
    echo
done
