#!/bin/sh
#-*-sh-*-

# PM2: Parallel Multithreaded Machine
# Copyright (C) 2009 "the PM2 team" (see AUTHORS file)
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


# Check the conformance of `read-topology' under $PM2_FLAVOR for all
# the Linux sysfs hierarchies available under `topology-sysfs/'.
# Return true on success.

PM2_FLAVOR="${PM2_FLAVOR:-marcel-topology}"
export PM2_FLAVOR

builddir="`pm2-config --builddir marcel`"
read_topology="$builddir/examples/bin/read-topology"

example_dir="$PM2_ROOT/marcel/examples"
topology_dir="$example_dir/topology-sysfs"

error()
{
    echo $@ 2>&1
}

# topology_cpu_count TOPOLOGY-DIR
#
# Print the number of CPUs listed in the sysfs hierarchy under
# TOPOLOGY-DIR.
topology_cpu_count()
{
    ( cd "$1"/sys/devices/system/cpu ; echo cpu[0-9]* | wc -w )
}

# flavor_max_cpu_count
#
# Print the maximum number of CPUs supported in the current flavor's
# CPU set implementation.  Default to 32.
flavor_max_cpu_count()
{
    local options="`pm2-flavor get | grep 'nbmaxcpus:' \
       | sed -e's/.*nbmaxcpus:\([0-9]\+\).*$/\1/g'`"

    echo "${options:-32}"
}

# test_topology NAME TOPOLOGY-DIR
#
# Test the topology under TOPOLOGY-DIR.  Return true on success.
test_topology ()
{
    # XXX: `local' may well be a bashism.
    local name="$1"
    local dir="$2"

    local output="`mktemp`"

    if ! pm2-load read-topology -v				\
	--marcel-topology-fsys-root "$dir" > "$output"
    then
	result=1
    else
	diff -uBb "$dir/output" "$output"
	result=$?
    fi

    rm "$output"

    return $result
}

# test_eligible TOPOLOGY-DIR
#
# Return true if the topology under TOPOLOGY-DIR is eligible for
# testing with the current flavor.
test_eligible()
{
    local dir="$1"

    [ -d "$dir" -a -f "$dir/output" ] &&				\
	[ `flavor_max_cpu_count` -ge `topology_cpu_count "$dir"` ]
}


if [ ! -d "$topology_dir" ]
then
    error "Could not find topology directory \`$topology_dir'."
    exit 1
fi

if [ ! -x "$read_topology" ]
then
    error "Could not find executable file \`$read_topology' (erroneous \$PM2_FLAVOR?)."
    exit 1
fi

test_count=0
tests_passed=0

for topology in "$topology_dir"/*.tar.gz
do
    dir="`mktemp -d`"

    if ! ( cd "$dir" && gunzip -c "$topology" | tar xf - )
    then
	error "failed to extract topology \`$topology'"
    else
	actual_dir="`echo "$dir"/*`"

	if test_eligible "$actual_dir"
	then
	    test_count="`expr $test_count + 1`"

	    if test_topology "`basename $topology`" "$actual_dir"
	    then
		tests_passed="`expr $tests_passed + 1`"
		echo "PASS: `basename $topology`"
	    else
		echo "FAIL: `basename $topology`"
	    fi
	else
	    echo "SKIP: `basename $topology`"
	fi
    fi

    rm -rf "$dir"
done

echo "$tests_passed/$test_count tests passed"
exec test $tests_passed -eq $test_count
