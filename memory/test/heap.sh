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

result=$(grep -lE '[,-]' /sys/devices/system/node/online)
if [ -z "$result" ] ; then
    echo "The machine does not seem to be NUMA-aware"
    pm2_skip_test
fi

flavor="test_memory_heap"
appdir="${PM2_OBJROOT}/memory/examples/heap/simple"
check_all_lines=1

create_test_flavor() {
# Creation de la flavor
    eval ${PM2_OBJROOT}/bin/pm2-flavor set --flavor=\"$flavor\" \
	--modules=\"marcel tbx init memory\" \
	--init=\"topology\" \
        --memory=\"enable_heap_allocator\" \
	--marcel=\"numa bubbles enable_stats main smp_smt_idle\" \
	--all=\"opt gdb debug\" --all=\"build_static\" $_output_redirect
}
