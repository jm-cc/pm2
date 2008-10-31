#!/bin/bash

flavor="sampling_mami"
x=$(pm2-flavor get --flavor=$flavor 2>/dev/null)
if [ -z "$x" ] ; then
    # create the flavor
    eval pm2-flavor set --flavor=\"$flavor\" \
        --ext=\"\" \
        --modules=\"init marcel tbx\" \
        --common=\"fortran_target_none\" \
        --marcel=\"numa marcel_main bubble_sched_null smp_smt_idle enable_mami enable_numa_memory enable_stats\" \
        --all=\"build_static opt\"
fi

make -C $PM2_ROOT/marcel FLAVOR=$flavor -j
make -C $PM2_ROOT/marcel/examples/memory FLAVOR=$flavor sampling_for_memory_access
prog=$(pm2-which -f $flavor sampling_for_memory_access)

$prog
