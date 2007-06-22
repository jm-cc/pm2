
#-*-sh-*-
# PM2: Parallel Multithreaded Machine
# Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

# Configuration
hosts=${TESTCONFIG:-"localhost localhost"}
net=${TESTNET:-tcp}
mpi=""
strat="strat_default"
flavor="test_mad4_${mpi}_${net}_${strat}"

appdir="${PM2_ROOT}/mad4/examples"
prog="sr_hello_larger"
args="--x --p --l"

rm -f /tmp/pm2test_"${USER}"_expected
(cat <<EOF
Messages successfully received
Messages successfully received
EOF
) > /tmp/pm2test_"${USER}"_expected

. ./include/mad4_default

# Verification et compilation du lanceur leonie
compile_leonie
