#! /bin/bash

dirs="$@"

if [ "x${dirs}" = "x" ]; then
    dirs="$(echo *.dat)"
fi

echo "# dirs = ${dirs}"

echo "# generating collection.png..."
gm convert $( for d in ${dirs}; do echo ${d}.d/all.png; done ) -append collection.png

sendrecv_dat="$( ( for d in ${dirs}; do echo ${d}.d/mpi_bench_sendrecv.dat; done ) | tr '\n' ' ' )"
noncontig_dat="$( ( for d in ${dirs}; do echo ${d}.d/mpi_bench_noncontig.dat; done ) | tr '\n' ' ' )"

echo "# sendrecv = ${sendrecv_dat}"

gnuplot <<EOF
set term pngcairo noenhanced color  fontscale 1 size 1024,768
# TODO: we should load "term-png.gp", which would need to know absolute path

set key top left
set logscale x 2
set style data linespoints

set xtics ( "1 KB" 1024, "8 KB" 8*1024, "64 KB" 64*1024, "512 KB" 512*1024, "4 MB" 4*1024*1024, "32 MB" 32*1024*1024 )
set xlabel "Message size (bytes)"
set ylabel "Bandwidth (MB/s)"

set style line 1 lc rgb "red"
set style line 2 lc rgb "blue"
set style line 3 lc rgb "magenta"
set style line 4 lc rgb "cyan"
set style line 5 lc rgb "forest-green"
set style increment user

set output "mpi_bench_sendrecv.png"
plot [512:] for [ file in "${sendrecv_dat}" ] file using 1:3 title file

set output "mpi_bench_noncontig.png"
plot [512:] for [ file in "${noncontig_dat}" ] file using 1:3 title file

EOF
