#! /bin/bash

dirs="$@"

if [ "x${dirs}" = "x" ]; then
    dirs="$(echo *.dat)"
fi

echo "# dirs = ${dirs}"

echo "# generating collection.png..."
gm convert $( for d in ${dirs}; do echo ${d}.d/all.png; done ) -append collection.png

sendrecv_dat="$( ( for d in ${dirs}; do echo ${d}.d/mpi_bench_sendrecv.dat; done ) | tr '\n' ' ' )"

echo "# sendrecv = ${sendrecv_dat}"

gnuplot <<EOF
set term pngcairo noenhanced color  fontscale 1 size 1024,768
# TODO: we should load "term-png.gp", which would need to know absolute path

set key top left
set logscale x 2
set style data linespoints
set output "mpi_bench_sendrecv.png"
plot [512:] for [ file in "${sendrecv_dat}" ] file using 1:3 title file

EOF
