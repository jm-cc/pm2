#!/bin/sh

if [ "$MPI_NMAD_PROTOCOL" == "" ] ; then
    echo "Error. Set MPI_NMAD_PROTOCOL"
    exit
fi

machines=$(pm2conf|grep -v current|awk '{print $3}'|tr '\012' '_' | sed 's/_$//')
protocol=$MPI_NMAD_PROTOCOL

mkdir -p results/${machines}_$protocol/

make clean mpi_extended
../mpirun -machinefile ../nmad_xeon.lst -np 2 mpi_extended > results/${machines}_$protocol/result_extended_${machines}_${protocol}.txt 2>&1

cd results/${machines}_$protocol/

for chunk in 128 256 512 1024 ; do
    export chunk
    awk '$4 == ENVIRON["chunk"]' result_extended_${machines}_${protocol}.txt  > result_extended_${machines}_${protocol}_chunk$chunk.txt
done

seuil=10240

for chunk in 128 256 512 1024 ; do
    max_size=$(awk '{print $3}' result_extended_${machines}_${protocol}_chunk$chunk.txt |sort -nr|head -1)
    (cat <<EOF
set terminal jpeg
set title "Chunk size $chunk - Machines $machines - Protocol $protocol"
set xlabel "Message size"
set logscale x 2

set ylabel "Transfer time"
set output "result_extended_${machines}_${protocol}_chunk$chunk.jpg"
plot "result_extended_${machines}_${protocol}_chunk$chunk.txt" using 3:6 with linespoints title "1-bloc", "result_extended_${machines}_${protocol}_chunk$chunk.txt" using 3:7 with linespoints title "Extended"

set ylabel "Benefit"
set output "result_extended_${machines}_${protocol}_chunk${chunk}_benefit.jpg"
plot "result_extended_${machines}_${protocol}_chunk$chunk.txt" using 3:8 with linespoints title "Benefit"

set xrange [:$seuil]

set ylabel "Transfer time"
set output "result_extended_${machines}_${protocol}_chunk${chunk}_small.jpg"
plot "result_extended_${machines}_${protocol}_chunk$chunk.txt" using 3:6 with linespoints title "1-bloc", "result_extended_${machines}_${protocol}_chunk$chunk.txt" using 3:7 with linespoints title "Extended"

set ylabel "Benefit"
set output "result_extended_${machines}_${protocol}_chunk${chunk}_small_benefit.jpg"
plot "result_extended_${machines}_${protocol}_chunk$chunk.txt" using 3:8 with linespoints title "Benefit"

set xrange [$seuil:$max_size]

set ylabel "Transfer time"
set output "result_extended_${machines}_${protocol}_chunk${chunk}_large.jpg"
plot "result_extended_${machines}_${protocol}_chunk$chunk.txt" using 3:6 with linespoints title "1-bloc", "result_extended_${machines}_${protocol}_chunk$chunk.txt" using 3:7 with linespoints title "Extended"

set ylabel "Benefit"
set output "result_extended_${machines}_${protocol}_chunk${chunk}_large_benefit.jpg"
plot "result_extended_${machines}_${protocol}_chunk$chunk.txt" using 3:8 with linespoints title "Benefit"
EOF
    ) > courbe_${machines}_${protocol}_${chunk}.gnu
    gnuplot courbe_${machines}_${protocol}_$chunk.gnu
done

cd -
