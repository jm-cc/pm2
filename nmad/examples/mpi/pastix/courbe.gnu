#
# NewMadeleine
# Copyright (C) 2006 (see AUTHORS file)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
set terminal png size 505,378
set grid
set output "pastix.png"
set style fill solid 1.000000 border -1
set style histogram clustered gap 1 title 0, 0, 0
set datafile missing '-'
set style data histograms
set ylabel "Time(ms)"
set title "Default vs Aggreg vs Memcpy-1send vs Extended" offset character 0, 0, 0 font ""
plot 'courbe.dat' using 5:xtic(3) ti "Default", '' u 6 ti "Aggreg", '' u 7 ti "Memcopy", '' u 8 ti "Extended"

