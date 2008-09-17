#!/bin/bash

flavor="sampling_mami"
x=$(pm2-flavor get --flavor=$flavor 2>/dev/null)
if [ -z "$x" ] ; then
    # create the flavor
    eval pm2-flavor set --flavor=\"$flavor\" \
        --ext=\"\" \
        --modules=\"init marcel tbx\" \
        --common=\"fortran_target_none\" \
        --marcel=\"numa marcel_main bubble_sched_null smp_smt_idle enable_mami enable_stats\" \
        --all=\"build_static opt\"
fi

prog=$(pm2-which -f $flavor sampling_for_memory_migration 2>/dev/null)
if [ -z "$prog" ] ; then
    make -C $PM2_ROOT/marcel FLAVOR=$flavor -j 
    make -C $PM2_ROOT/marcel/examples/memory FLAVOR=$flavor sampling_for_memory_migration
    prog=$(pm2-which -f $flavor sampling_for_memory_migration)
fi

nodes=$(ls -d /sys/devices/system/node/node* | sed 's:/sys/devices/system/node/node::g')

for node in $(echo $nodes) ; do
    numactl --physcpubind=$node $prog -src $node
done
