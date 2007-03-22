#!/bin/sh

for i in $(seq 1 10) ; do
    mpirun -np 2 -machinefile ../nmad_xeon.lst mpi_pastix >./courbe_$i.dat 2>&1
done
