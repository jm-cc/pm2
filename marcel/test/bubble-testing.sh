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
# `test_bubble-affinity*.level*' files.

if [ -z "$prog" ]
then
		echo "error: \`\$prog' is undefined."
		exit 1
fi

# Configuration
flavor="test_marcel"
appdir="${PM2_ROOT}/marcel/examples"
args=""
hosts="localhost"
cat > /tmp/pm2test_"${USER}"_expected <<EOF
PASS: scheduling entities were distributed as expected
EOF

# Creation de la flavor
eval ${PM2_ROOT}/bin/pm2-flavor set --flavor="$flavor"		\
	--ext=""						\
	--modules="\"marcel tbx init\"" --marcel="numa"		\
	--marcel="spinlock" --marcel="marcel_main"		\
	--marcel="standard_main" --marcel="pmarcel"		\
	--marcel="enable_stats"					\
	--marcel="bubble_sched_affinity"			\
	--marcel="bug_on" --marcel="malloc_preempt_debug"	\
	--tbx="safe_malloc" --tbx="parano_malloc"		\
	--all="gdb" --all="build_static" $_output_redirect

# Local Variables:
# tab-width: 8
# End:
# vim:ts=8:
