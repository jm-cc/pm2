#!/bin/sh

sizes="112440 144528 29000 398056 638264 384168"

grep Benefit courbe_1.dat > courbe.dat

for s in $sizes ; do
    echo ; echo
    echo "Taille $s"
    export s
    inf=$(awk '$3 == ENVIRON["s"] {print $1}' courbe_1.dat)
    sup=$(awk '$3 == ENVIRON["s"] {print $2}' courbe_1.dat)
    nb_messages=$(awk '$3 == ENVIRON["s"] {print $4}' courbe_1.dat)
    defaut=0
    aggreg=0
    memcopy=0
    esend=0
    gain=0
    nb=0
    for e in $(seq 1 10) ; do
        if [ -f courbe_$e.dat ]  ; then
            defaut_aux=$(awk '$3 == ENVIRON["s"] {print $5}' courbe_$e.dat)
            aggreg_aux=$(awk '$3 == ENVIRON["s"] {print $6}' courbe_$e.dat)
            memcopy_aux=$(awk '$3 == ENVIRON["s"] {print $7}' courbe_$e.dat)
            esend_aux=$(awk '$3 == ENVIRON["s"] {print $8}' courbe_$e.dat)
            gain_aux=$(awk '$3 == ENVIRON["s"] {print $9}' courbe_$e.dat)
            defaut=$(echo $defaut + $defaut_aux | bc)
            aggreg=$(echo $aggreg + $aggreg_aux | bc)
            memcopy=$(echo $memcopy + $memcopy_aux | bc)
            esend=$(echo $esend + $esend_aux | bc)
            gain=$(echo $gain + $gain_aux | bc)
            nb=$(echo $nb + 1 | bc)
            echo "$defaut_aux $aggreg_aux $memcopy_aux $esend_aux $gain_aux $nb"
        fi
    done
    defaut=$(echo $defaut / $nb | bc -l | sed 's/0*$//' | sed 's/^\./0./')
    aggreg=$(echo $aggreg / $nb | bc -l | sed 's/0*$//' | sed 's/^\./0./')
    memcopy=$(echo $memcopy / $nb | bc -l | sed 's/0*$//' | sed 's/^\./0./')
    esend=$(echo $esend / $nb | bc -l | sed 's/0*$//' | sed 's/^\./0./')
    gain=$(echo $gain / $nb | bc -l | sed 's/0*$//' | sed 's/^\./0./')

    echo "$inf         $sup      $s           $nb_messages             $defaut          $aggreg           $memcopy        $esend       $gain" >> courbe.dat
done

gnuplot courbe.gnu

