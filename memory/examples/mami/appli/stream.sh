!dd=$(date "+%Y_%m_%d_%H:%M")
for e in $(seq 0 9) ; do EXTRAPAGES=$e pm2-load stream > result_${dd}_0$e; done
for e in $(seq 10 20) ; do EXTRAPAGES=$e pm2-load stream > result_${dd}_$e; done
