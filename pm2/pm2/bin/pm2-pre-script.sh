#!/bin/sh

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


export PM2_CMD_PREFIX
PM2_CMD_PREFIX="${PM2_ROOT}/bin/pm2-pre-script.sh"

#echo "[ pm2-pre-script.sh $@ ]"

debug_file=""

while [ $# -gt 0 ]; do
    case "$1" in
	--preload)
	    PM2_CMD_PREFIX="$PM2_CMD_PREFIX $1 $2"
	    shift
	    LD_PRELOAD="${LD_PRELOAD:+${LD_PRELOAD}:}$1"
	    export LD_PRELOAD
	    shift
	    if [ -n "$debug_file" ]; then
		echo "set environment LD_PRELOAD $LD_PRELOAD" >> $debug_file
	    fi
	    ;;
	--export)
	    PM2_CMD_PREFIX="$PM2_CMD_PREFIX $1 $2 $3"
	    shift
	    var="$1"
	    shift
	    eval value="$1"
	    shift
	    # $value contient déjà des guillemets...
	    eval $var="$value"
	    eval export $var
	    if [ -n "$debug_file" ]; then
		echo "set environment $var $value" >> $debug_file
	    fi
	    ;;
	--debug)
	    PM2_CMD_PREFIX="$PM2_CMD_PREFIX $1"
	    shift
	    # tempo file
	    num=0
	    while [ -f /tmp/maddebug.$num ] ; do
		num=`expr $num + 1`
	    done
	    debug_file=/tmp/maddebug.$num
	    cp /dev/null $debug_file
	    ;;
	--)
	    PM2_CMD_PREFIX="$PM2_CMD_PREFIX $1"
	    shift
	    break
	    ;;
	*)
	    echo "Error in pm2-pre-script: unknown argument ($1)" >&2
	    exit 1
	    ;;
    esac
done

# Debug ?
if [ -n "$debug_file" ]; then

    prog=$1
    shift

    echo "set args $@" >> $debug_file

    title="$prog.$HOST"
    xterm -title $title -e gdb -x $debug_file $prog

    rm -f $debug_file

else # debug

    exec "$@"

fi
