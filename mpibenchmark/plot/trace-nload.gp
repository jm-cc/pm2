# -*- mode: Gnuplot;-*-

#  NewMadeleine
#  Copyright (C) 2015 (see AUTHORS file)
# 
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or (at
#  your option) any later version.
# 
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License for more details.

# Trace 2D graph for x=size; y=number of threads

# This file is not supposed to be used standalone.

set macros

filename  = ARG1
bench     = ARG2
outdir    = filename . ".d/"
benchfile = bench . "-ref.dat"
ref_file  = benchref . ".dat"

print "# filename:  " . filename
print "# outdir:    " . outdir
print "# bench:     " . bench
print "# benchfile: " . benchfile
print "# ref_file:  " . ref_file

stats ref_file using 1:2 name "LATREF" nooutput
stats benchfile using 2 name "PARAMS" nooutput

lat0      = LATREF_min_y
max_size  = LATREF_max_x
max_param = PARAMS_max
max_ratio = 10000

print "# lat0:      " . sprintf("%g", lat0)
print "# max_size:  " . sprintf("%d", max_size)
print "# max_param: " . sprintf("%d", max_param)

set view map
set pm3d # interpolate 0,0
set key bmargin box
set key off
set title filename." / ".bench
set xlabel "Message size (bytes)" 
set ylabel "Threads number" 
set cblabel "Overhead (ratio)"


set xtics ( "1 KB" 1024, "8 KB" 8*1024, "64 KB" 64*1024, "512 KB" 512*1024, "4 MB" 4*1024*1024, "32 MB" 32*1024*1024 )
set xrange [2048:max_size]
set yrange [0:max_param]
set cbrange [0.1:max_ratio]
set logscale x 2
set nologscale y
set logscale cb

set output bench.".".OUT_EXT

ov(l, latref) = ( (l > latref) ? ((l-latref)/latref) : 0 )
min(a, b) = (a < b) ? a : b

splot benchfile using 1:2:(min(max_ratio,(($3-$4)/$4))) with pm3d

