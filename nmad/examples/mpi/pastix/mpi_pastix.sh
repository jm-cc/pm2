#!/bin/sh

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

for i in $(seq 1 10) ; do
    mpirun -np 2 -machinefile ../nmad_xeon.lst mpi_pastix >./courbe_$i.dat 2>&1
done
