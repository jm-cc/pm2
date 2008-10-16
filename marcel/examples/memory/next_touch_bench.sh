bench() {
    pages=$1
    for i in $(seq 1 1 10) ; do
        pm2-load next_touch_bench -src 0 -dest 1 -pages $pages >> /tmp/next_touch_bench_$pages.dat
    done
    mean=$(awk 'BEGIN {sum=0} {sum+=$4} END {print sum/10}' /tmp/next_touch_bench_$pages.dat)
    echo "0	1	$pages	$mean"
}

rm -f /tmp/next_touch_bench.dat
(
    bench 1
    for p in $(seq 10 10 90) ; do
        bench $p
    done
    for p in $(seq 100 100 1000) ; do
        bench $p
    done
    for p in $(seq 2000 1000 5000) ; do
        bench $p
    done
) > /tmp/next_touch_bench.dat

(
cat <<EOF 
set logscale x 2
set logscale y 2
plot "/tmp/next_touch_bench.dat" using 3:4 with linespoints
pause -1
EOF
) > /tmp/next_touch_bench.gnp
gnuplot /tmp/next_touch_bench.gnp
