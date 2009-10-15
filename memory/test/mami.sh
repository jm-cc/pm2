#-*-sh-*-
# PM2: Parallel Multithreaded Machine
# Copyright (C) 2008 "the PM2 team" (see AUTHORS file)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

# This file is meant to be `source'd by the various
# `test_mami*.level*' files.

prog="<undefined>"
script="<undefined>"
args=""
ld_preload=""
hosts="localhost"
cat > /tmp/pm2test_"${USER}"_expected <<EOF
EOF

if [ ! -d /sys/devices/system/node/node0 ] ; then
    echo "The machine does not seem to be NUMA-aware"
    pm2_skip_test
fi

NB_NODES=$(ls -d /sys/devices/system/node/node*| wc -w)
appdir="${PM2_OBJROOT}/memory/examples/mami"
check_all_lines=1

level=$(basename $_t .level_3)
if [ "$level" == "$_t" ] ; then
    flavor="test_memory_mami_marcel"
    modules="marcel"
    options="marcel fortran"
else
    flavor="test_memory_mami_pthread"
    modules=""
    options="pthread"
fi

create_test_flavor() {
# Creation de la flavor
    eval ${PM2_OBJROOT}/bin/pm2-flavor set --flavor=\"$flavor\" \
	--ext=\"\" \
	--modules=\"$modules tbx init memory\" \
	--init=\"topology\" \
        --memory=\"enable_mami $options\" \
	--marcel=\"numa bubbles enable_stats standard_main smp_smt_idle enable_stats dont_use_pthread fortran\" \
        --common=\"fortran_target_gfortran\" \
	--all=\"opt gdb debug\" --all=\"build_static\" $_output_redirect
}
