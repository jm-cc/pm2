#!/usr/bin/env pm2-sh

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

log()
{
    if [ ! -z "$PM2_SCRIPT_DEBUG" ]; then
        echo "`uname -n`[$$]: $*" 1>&2
    fi
}

_pm2_load_error() # msg
{
    echo $*
    exit 1
}

#echo "[ pm2-pre-script.sh " ${@:+"$@"} " ]"

debug_file=""
valgrind=""

while [ $# -gt 0 ]; do
    case "$1" in
	--script-debug)
	    PM2_SCRIPT_DEBUG=on
	    export PM2_SCRIPT_DEBUG
	    log "Using script debug mode"
	    log "Running: $*"
	    shift
	    ;;
        --use-local-flavor)
	    shift
	    PM2_USE_LOCAL_FLAVOR=on
	    export PM2_USE_LOCAL_FLAVOR
	    log "Using local flavor"
	    ;;
	--preload)
	    shift
	    LD_PRELOAD="${LD_PRELOAD:+${LD_PRELOAD}:}$1"
	    export LD_PRELOAD
	    shift
	    if [ -n "$debug_file" ]; then
		echo "set environment LD_PRELOAD $LD_PRELOAD" >> $debug_file
	    fi
	    log "Using ld preload [${LD_PRELOAD}]"
	    ;;
	--export)
	    shift
	    var="$1"
	    shift
	    value="$1"
	    shift
	    eval $var=\"\$value\"
	    eval export $var
	    if [ -n "$debug_file" ]; then
		echo "set environment $var $value" >> $debug_file
	    fi
	    log "Exporting [$var] set to [$value]"
	    ;;
	--prescript)
	    shift
	    prescript="$1"
	    shift
	    log "Using [$prescript] as a pre-script environment"
	    ;;
	--debug|--valgrind)
	    [ "$1" == --valgrind ] && valgrind=yes
	    shift
	    # tempo file
	    num=0
	    while [ -f /tmp/pm2debug.$num ] ; do
		num=`expr $num + 1`
	    done
	    debug_file=/tmp/pm2debug.$num
	    cp /dev/null $debug_file
	    . "$PM2_ROOT/bin/pm2-arch" --source-mode
	    for i in $_PM2_CONFIG_MODULES ; do
	        gdbinit="$PM2_ROOT/modules/$i/scripts/gdbinit-$PM2_ARCH"
		[ -r "$gdbinit" ] && cat "$gdbinit" >> $debug_file
	        gdbinit="$PM2_ROOT/modules/$i/scripts/gdbinit"
		[ -r "$gdbinit" ] && cat "$gdbinit" >> $debug_file
	    done
	    log "Using debug mode"
	    ;;
	--strace)
	    shift
	    # tempo file
	    num=0
	    while [ -f /tmp/madstrace.$num ] ; do
		num=`expr $num + 1`
	    done
	    strace_file=/tmp/madstrace.$num
	    cp /dev/null $strace_file
	    log "Using strace mode"
	    ;;
	--)
	    shift
	    break
	    ;;
	*)
	    echo "Error in pm2-pre-script: unknown argument ($1)" >&2
	    exit 1
	    ;;
    esac
done

PM2_CMD_NAME="$1"
export PM2_CMD_NAME
shift

if [ "$PM2_USE_LOCAL_FLAVOR" = on ]; then

    prog_args=${@:+"$@"}

    # Récupération de (des) chemin(s) d'accès au fichier exécutable
    set --  --source-mode $PM2_CMD_NAME
    . ${PM2_ROOT}/bin/pm2-which

    if [ $_PM2_WHICH_NB_FOUND -gt 1 ] ; then
	exit 1
    fi

    prog="$_PM2_WHICH_RESULT"

    # Librairies dynamiques (LD_PRELOAD)
    if [ -n "$_PM2_CONFIG_PRELOAD" ]; then
	LD_PRELOAD="${LD_PRELOAD:+${LD_PRELOAD}:}$_PM2_CONFIG_PRELOAD"
	export LD_PRELOAD
	if [ -n "$debug_file" ]; then
	    echo "set environment LD_PRELOAD $LD_PRELOAD" >> $debug_file
	fi
	log "set environment LD_PRELOAD $LD_PRELOAD"
    fi

    # LD library path
    PM2_LD_LIBRARY_PATH="$_PM2_CONFIG_LD_LIBRARY_PATH"

    if [ -n "$PM2_LD_LIBRARY_PATH" ]; then
	LD_LIBRARY_PATH="${PM2_LD_LIBRARY_PATH}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
	export LD_LIBRARY_PATH
	if [ -n "$debug_file" ]; then
	    echo "set environment LD_LIBRARY_PATH $LD_LIBRARY_PATH" >> $debug_file
	fi
	log "set environment LD_LIBRARY_PATH $LD_LIBRARY_PATH"
    fi

    # Chemin d'accès au chargeur
    [ -x $_PM2_CONFIG_LOADER ] || _pm2_load_error "Fatal Error: \"$_PM2_CONFIG_LOADER\" exec file not found!"

    case $_PM2_CONFIG_LOADER in
	*conf_not_needed)
	    # No need to try to access the pm2 conf file
	    ;;
	*)
	    # Récupération du chemin d'accès au fichier de config des machines
	    set --  --source-mode
	    . ${PM2_ROOT}/bin/pm2-conf-file

	    [ -f $PM2_CONF_FILE ] || _pm2_load_error "Error: PM2 is not yet configured this flavor (please run pm2-conf)."
	    ;;
    esac

    set -- $prog_args

else

    if [ -n "$PM2_LD_LIBRARY_PATH" ]; then
        LD_LIBRARY_PATH="${PM2_LD_LIBRARY_PATH}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
        export LD_LIBRARY_PATH
	if [ -n "$debug_file" ]; then
	    echo "set environment LD_LIBRARY_PATH $LD_LIBRARY_PATH" >> $debug_file
	fi
	log "set environment LD_LIBRARY_PATH $LD_LIBRARY_PATH"
    fi


    prog=$PM2_CMD_NAME

fi

# Debug ?
if [ -n "$debug_file" ]; then

    echo "set args " ${@:+"$@"} >> $debug_file

    title="$prog.$HOST"
fi

if [ -n "$valgrind" ]; then

    log "Executing: valgrind -v --leak-check=yes --db-attach=yes --db-command=\"gdb -x $debug_file -nw %f %p\" $prog $*"
    valgrind -v --show-reachable=yes --leak-check=yes --db-attach=yes --db-command="gdb -x $debug_file -nw %f %p" $prog $*

    rm -f $debug_file

elif [ -n "$debug_file" ]; then

    log "Executing: gdb -x $debug_file $prog"
    gdb -x $debug_file $prog

    rm -f $debug_file

elif [ -n "$strace_file" ]; then

    log "Executing: exec strace -o $strace_file $prog $*"
    exec strace -f -o $strace_file $prog ${@:+"$@"}

elif [ -n "$prescript" ]; then

    log "Executing: exec $prescript $prog $*"
    exec $prescript $prog ${@:+"$@"}

else

    log "Executing: exec $prog $*"
    exec $prog ${@:+"$@"}

fi
