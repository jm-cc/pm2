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

if [ ! -d /sys/devices/system/node/node0 ] ; then
    echo "The machine does not seem to be NUMA-aware"
    pm2_skip_test
fi

flavor="test_marcel_mami"
appdir="${PM2_ROOT}/marcel/examples/memory"
check_all_lines=1

create_test_flavor() {
# Creation de la flavor
    eval ${PM2_ROOT}/bin/pm2-flavor set --flavor=\"$flavor\" \
	--ext=\"\" \
	--modules=\"marcel tbx init\" \
	--marcel=\"numa marcel_main bubble_sched_null smp_smt_idle enable_mami enable_stats\" \
	--all=\"opt\" --all=\"gdb\" --all=\"build_static\" $_output_redirect
}
