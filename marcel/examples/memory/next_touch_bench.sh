rm -f next_touch_bench.dat
for p in 1 10 50 ; do
    for i in $(seq 1 1 10) ; do
        pm2-load next_touch_bench -src 0 -dest 1 -pages $p >> next_touch_bench.dat
    done
done

for p in $(seq 100 100 1000) ; do
    for i in $(seq 1 1 10) ; do
        pm2-load next_touch_bench -src 0 -dest 1 -pages $p >> next_touch_bench.dat
    done
done

for p in $(seq 2000 1000 5000) ; do
    for i in $(seq 1 1 10) ; do
        pm2-load next_touch_bench -src 0 -dest 1 -pages $p >> next_touch_bench.dat
    done
done

(
cat <<EOF 
set logscale x 2
set logscale y 2
plot "next_touch_bench.dat" using 3:4 with linespoints
pause -1
EOF
) > next_touch_bench.gnp
gnuplot next_touch_bench.gnp
