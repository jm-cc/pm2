#!/bin/sh
##########

DBG_SCRIPT = mad2_run_ddd.sh
# DBG_SCRIPT = mad2_run_gdb.sh

echo "Calling mpirun..."
mpirun N -x DISPLAY ${DBG_SCRIPT}$*

##########
