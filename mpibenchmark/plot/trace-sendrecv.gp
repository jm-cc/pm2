# -*- mode: Gnuplot;-*-

#  NewMadeleine
#  Copyright (C) 2016 (see AUTHORS file)
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

# This file is not supposed to be used standalone.

filename  = ARG1
bench     = ARG2
benchfile = bench . ".dat"
outdir    = filename . ".d/"

print "# filename:  " . filename
print "# outdir:    " . outdir
print "# bench:     " . bench
print "# benchfile: " . benchfile

set key bmargin box
set key off
set xlabel "Message size (bytes)"
set logscale x 2

set ylabel "Latency (usec.)"
set title "Latency for ".bench
set output bench."-lat.".OUT_EXT
plot [:4096][0:] benchfile using 1:5 with lines lc 1, benchfile using 1:5:8:9 with errorbars lc 1

set xtics ( "1 KB" 1024, "8 KB" 8*1024, "64 KB" 64*1024, "512 KB" 512*1024, "4 MB" 4*1024*1024, "32 MB" 32*1024*1024 )
set ylabel "Bandiwdth (MB/s)"
set title "Bandwidth for ".bench
set output bench."-bw.".OUT_EXT
plot [2048:][0:] benchfile using 1:($1/$5) with lines lc 1, benchfile using 1:($1/$5):($1/$8):($1/$9) with errorbars lc 1
