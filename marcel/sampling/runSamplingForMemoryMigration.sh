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

make -C $PM2_ROOT/marcel FLAVOR=$flavor -j 
make -C $PM2_ROOT/marcel/examples/memory FLAVOR=$flavor sampling_for_memory_migration
prog=$(pm2-which -f $flavor sampling_for_memory_migration)

nodes=$(ls -d /sys/devices/system/node/node* | sed 's:/sys/devices/system/node/node::g')

for node in $(echo $nodes) ; do
    $prog -src $node
done

pathname=$PM2_SAMPLING_DIR
if [ -z "$pathname" ] ; then
    pathname="/var/local/pm2"
fi
pathname=$pathname"/marcel"

hostname=$(uname -n)
(
    head -1 $pathname/sampling_for_memory_migration_${hostname}_source_0.dat
    for node in $(echo $nodes) ; do
        grep -v Source "$pathname/sampling_for_memory_migration_${hostname}_source_${node}.dat"
    done
) > $pathname/sampling_for_memory_migration_${hostname}.dat

$(dirname $0)/analyzeSamplingForMemoryMigration.pl
