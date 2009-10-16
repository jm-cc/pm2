#!/bin/bash

flavor="sampling_mami"
x=$(pm2-flavor get --flavor=$flavor 2>/dev/null)
if [ -z "$x" ] ; then
    # create the flavor
    eval pm2-flavor set --flavor=\"$flavor\" \
        --modules=\"init marcel tbx memory\" \
        --common=\"fortran_target_none\" \
        --marcel=\"numa standard_main bubble_sched_null smp_smt_idle enable_stats\" \
        --memory=\"enable_mami\" \
        --all=\"build_static opt\"
fi

make -C $PM2_OBJROOT/memory FLAVOR=$flavor -j
make -C $PM2_OBJROOT/memory/examples/mami FLAVOR=$flavor sampling_for_memory_access
prog=$(pm2-which -f $flavor sampling_for_memory_access)

$prog
