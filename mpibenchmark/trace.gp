# -*- mode: Gnuplot;-*-

reset
set macros

if (!exists("GNUPLOT_OUT")) {
    GNUPLOT_OUT = "png"
    print "# GNUPLOT_OUT not defined- taking default: png"
}
if (GNUPLOT_OUT eq "png") {
    GNUPLOT_TERM = "png size 1024,768"
} else {
    if (GNUPLOT_OUT eq "pdf") {
	GNUPLOT_TERM = "pdfcairo color fontscale 0.3"
    } else {
	print "# undefined output format: " . GNUPLOT_OUT
	exit
    }
}

filename  = ARG1
bench     = ARG2
outdir    = filename . ".d/"
benchfile = bench . "-ref.dat"

print "# filename:  " . filename
print "# outdir:    " . outdir
print "# bench:     " . bench
print "# benchfile: " . benchfile

cd outdir

benchref  = "mpi_bench_sendrecv"
ref_file  = benchref . ".dat"

print "# ref_file:  " . ref_file

stats ref_file using 1:2 name "LATREF" nooutput
stats benchfile using 2 name "PARAMS" nooutput

lat0      = LATREF_min_y

print "# lat0:      " . sprintf("%g", lat0)

max_size = LATREF_max_x
max_param = PARAMS_max

print "# max_size:  " . sprintf("%d", max_size)
print "# max_param: " . sprintf("%d", max_param)

set term png size 1024,768
set view map
set pm3d interpolate 0,0
set key bmargin box
set key off
set title filename
set xlabel "Message size (bytes)"
set ylabel "Computation time (usec.)"
set cblabel bench


set xtics ( "1 KB" 1024, "8 KB" 8*1024, "64 KB" 64*1024, "512 KB" 512*1024, "4 MB" 4*1024*1024, "32 MB" 32*1024*1024 )
set xrange [2048:max_size]
set yrange [20:max_param]
set cbrange [0:2]
set logscale x 2
set logscale y 10

set output "direct-".bench.".png"

lat(rtt, l0) = (rtt > l0) ? (rtt - l0) : 0
ov(l, c, latref) = ( (c < latref) ? ((l - latref) / c) : ((l - c) / latref))
min(a, b) = (a < b) ? a : b

splot benchfile using 1:2:(min(2, ov(lat($3, lat0), $2, $4))) with pm3d, \
      ref_file using 1:2:(1) with linespoints lc 2

