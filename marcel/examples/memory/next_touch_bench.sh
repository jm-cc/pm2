rm -f next_touch_bench.dat
for p in 1 10 50 ; do
    pm2-load next_touch_bench -src 0 -dest 1 -pages $p >> next_touch_bench.dat
done

for p in $(seq 100 100 1000) ; do
    pm2-load next_touch_bench -src 0 -dest 1 -pages $p >> next_touch_bench.dat
done

for p in $(seq 1000 1000 5000) ; do
    pm2-load next_touch_bench -src 0 -dest 1 -pages $p >> next_touch_bench.dat
done

(
cat <<EOF 
plot "next_touch_bench.dat" using 3:4 with linespoints
pause -1
EOF
) > next_touch_bench.gnp
gnuplot next_touch_bench.gnp
