#!/bin/sh

if [ "$MPI_NMAD_PROTOCOL" == "" ] ; then
    echo "Error. Set MPI_NMAD_PROTOCOL"
    exit
fi

machines=$(pm2conf|grep -v current|awk '{print $3}'|tr '\012' '_' | sed 's/_$//')
nb_machines=$(pm2conf|grep -v current|awk '{print $3}'|wc -l)
protocol=$MPI_NMAD_PROTOCOL

mkdir -p results/${machines}_${protocol}/

echo $machines | tr '_' '\012' > results/${machines}_${protocol}/machines.lst

make clean mpi_extended
../mpirun -machinefile results/${machines}_${protocol}/machines.lst -np $nb_machines mpi_extended > results/${machines}_${protocol}/result_extended_${machines}_${protocol}.txt 2>&1

cd results/${machines}_${protocol}/

seuil=10240

for chunk in 128 256 512 1024 ; do
    export chunk
    awk '$4 == ENVIRON["chunk"]' result_extended_${machines}_${protocol}.txt  > result_extended_${machines}_${protocol}_chunk$chunk.txt
    max_size=$(awk '{print $3}' result_extended_${machines}_${protocol}_chunk$chunk.txt |sort -nr|head -1)

    (cat <<EOF
set terminal png size 505,378
set title "Chunk size $chunk - Machines $machines - Protocol $protocol"
set xlabel "Message size"
set logscale x 2
set xtics ("512" 512, "1K" 1024, "2K" 2048,"4K" 4096,"8K" 8192,"16K" 16384,"32K" 32768,"64K" 65536,"128K" 131072, "256K" 262144,"512K" 524288,"1M" 1048576,"2M" 2097152,"4M" 4194304, "8M" 8388608)

set ylabel "Transfer time"
set output "result_extended_${machines}_${protocol}_chunk$chunk.png"
plot "result_extended_${machines}_${protocol}_chunk$chunk.txt" using 3:6 with linespoints title "Struct datatype", "result_extended_${machines}_${protocol}_chunk$chunk.txt" using 3:7 with linespoints title "Extended"

set ylabel "Benefit"
set output "result_extended_${machines}_${protocol}_chunk${chunk}_benefit.png"
plot "result_extended_${machines}_${protocol}_chunk$chunk.txt" using 3:8 with linespoints notitle

set xrange [:$seuil]

set ylabel "Transfer time"
set output "result_extended_${machines}_${protocol}_chunk${chunk}_small.png"
plot "result_extended_${machines}_${protocol}_chunk$chunk.txt" using 3:6 with linespoints title "Struct datatype", "result_extended_${machines}_${protocol}_chunk$chunk.txt" using 3:7 with linespoints title "Extended"

set ylabel "Benefit"
set output "result_extended_${machines}_${protocol}_chunk${chunk}_small_benefit.png"
plot "result_extended_${machines}_${protocol}_chunk$chunk.txt" using 3:8 with linespoints notitle

set xrange [$seuil:$max_size]

set ylabel "Transfer time"
set output "result_extended_${machines}_${protocol}_chunk${chunk}_large.png"
plot "result_extended_${machines}_${protocol}_chunk$chunk.txt" using 3:6 with linespoints title "Struct datatype", "result_extended_${machines}_${protocol}_chunk$chunk.txt" using 3:7 with linespoints title "Extended"

set ylabel "Benefit"
set output "result_extended_${machines}_${protocol}_chunk${chunk}_large_benefit.png"
plot "result_extended_${machines}_${protocol}_chunk$chunk.txt" using 3:8 with linespoints notitle
EOF
    ) > courbe_${machines}_${protocol}_${chunk}.gnu
    gnuplot courbe_${machines}_${protocol}_$chunk.gnu

    rm courbe_${machines}_${protocol}_$chunk.gnu
    rm result_extended_${machines}_${protocol}_chunk$chunk.txt
done

cd -
