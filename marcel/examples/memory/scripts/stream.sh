for e in $(seq 0 9) ; do EXTRAPAGES=$e pm2-load stream > result_0$e 2>&1; done
for e in $(seq 10 20) ; do EXTRAPAGES=$e pm2-load stream > result_$e 2>&1; done
