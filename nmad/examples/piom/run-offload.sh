#! /bin/bash

nodes=mistral10-mic1,mistral11-mic1
seq="0 1 2 5 10 20 30 40 50 100 150 200 500 1000 2000 5000"
args="-DNMAD_DRIVER=ibverbs" # -DPIOM_IDLE_GRANULARITY=5 -DPIOM_TIMER_PERIOD=4000 -DNMAD_STRATEGY=aggreg -DPIOM_BUSY_WAIT_USEC=10"

for c in ${seq}; do
    echo "# pio_offload c${c} nodes ${nodes}" > piom-offload-c${c}.dat
    padico-launch -q -n 2 -nodelist ${nodes} ${args} pio_offload -C ${c} -N 500 -W 5 | tee -a piom-offload-c${c}.dat
done

