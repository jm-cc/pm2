#!/bin/sh

#
# PM2: Parallel Multithreaded Machine
# Copyright (C) 2010 "the PM2 team" (see AUTHORS file)
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
#

##################################################

WORKDIR=`mktemp -d /tmp/tmp.XXXXXXX`
ABS_SRCDIR=""

ABORT_ON_ERROR=0
COVERAGE_TEST=0
TEST_ID=0

##################################################

# args: file
absolute_path()
{
    cd `dirname $0`
    ABS_SRCDIR=`pwd -P`
    cd -
}

# args: tracefile patterns to remove
lcov_filter()
{
    tracefile="$1"
    words="$2"

    for w in ${words}
    do
	lcov -r "${tracefile}" '$w*' -o "${tracefile}.tmp"
	mv "${tracefile}.tmp" "${tracefile}"
    done
}

# arg: configure arguments
check_exec()
{
    if [ $? -ne 0 ]; then
	echo "Test execution failed ..."
	TEST_SET_FAILED="${TEST_SET_FAILED} - ${CURRENT_TEST}"
	if [ ${ABORT_ON_ERROR} -eq 1 ]; then
	    echo "Following test set(s) failed: ${TEST_SET_FAILED}"
	    echo "  >> options: $@"
	    echo "  >> workdir: ${WORKDIR}"
	    exit 1;
	fi
    fi
}

# arg: tool to check if it is installed
check_tool()
{
    cmd="$1"
    
    echo -n "Check whether ${cmd} is installed... "
    if ${cmd} --version >/dev/null 2>&1 || ${cmd} -v >/dev/null 2>&1; then
	echo "yes"
    else
	echo "no"
	echo
	echo "Please install the following tool: ${cmd}"
	exit 1
    fi
}

build_and_test()
{
    TEST_NUM=`expr ${TEST_NUM} + 1`
    if [ ${TEST_ID} -eq 0 ] || [ ${TEST_ID} -eq ${TEST_NUM} ]; then
	CURRENT_TEST="Test set ${TEST_NUM}"

	eval "printf '\n\n${CURRENT_TEST}  ${sched}\n\n' ${OUTPUT}"
	case ${TERM} in 
	    xterm*) printf "\033]0;${CURRENT_TEST}  ${sched}\007";;
	esac

	rm -rf ${WORKDIR}/build/*
	cd ${WORKDIR}/build

	eval "${SRCDIR}/configure $@ ${OUTPUT}"
	if [ $? -eq 0 ]; then
	    eval "${MAKE} ${MAKEOPTS} ${OUTPUT}"
	    check_exec "$@"

	    cd ${WORKDIR}

	    if [ ${COVERAGE_TEST} -eq 1 ]; then
		lcov -c -d "${WORKDIR}/build" -o "${WORKDIR}/cov/${CURRENT_TEST}.lcov"
		lcov_filter "${WORKDIR}/cov/${CURRENT_TEST}.lcov" "/usr tbx tests"
	    fi
	fi
    fi
}

exec_tests()
{
    build_and_test ${DEBUG_OPTIONS} ${TOPO_OPTIONS} --with-libtype=${libtype} 

    build_and_test ${COMMON_OPTIONS} ${DEBUG_OPTIONS} ${TOPO_OPTIONS} --with-libtype=${libtype}

    build_and_test ${COMMON_OPTIONS} ${DEBUG_OPTIONS} ${TOPO_OPTIONS} --with-libtype=${libtype} --enable-idle_pause

    if [ ${libtype} != pthread ]; then
	build_and_test ${COMMON_OPTIONS} ${DEBUG_OPTIONS} ${TOPO_OPTIONS} --with-libtype=${libtype} --enable-use_tls
    fi

    if [ ${topo} = smp ]; then
	build_and_test ${COMMON_OPTIONS} ${DEBUG_OPTIONS} ${TOPO_OPTIONS} --with-libtype=${libtype} --enable-smp_remote_tasklet
    fi
}

##################################################

MARCEL_TOPO="${MARCEL_TOPO:-mono smp numa}"
MARCEL_LIBTYPE="${MARCEL_LIBTYPE:-marcel pmarcel pthread}"
MARCEL_SCHEDULERS="${MARCEL_SCHEDULERS:-cache explode gang null spread steal}"

MAKE="${MAKE:-make}"
MAKEOPTS="check"

absolute_path $0
SRCDIR="${ABS_SRCDIR}/../.."

while [ $# -ne 0 ]; do
    case $1 in
	--abort-on-error) 
	    ABORT_ON_ERROR=1 ;;
	--coverage)
	    COV_OPTIONS="--enable-gcov" ;
	    COVERAGE_TEST=1 ;;
	--logfile)
	    OUTPUT=">>'$2' 2>&1" ;
	    [ -f "$2" ] && rm -i "$2" ;
	    shift ;;
	--test-id)
	    TEST_ID=$2 ;
	    shift ;;
	--build-only)
	    MAKEOPTS="" ;;
	--help)
	    echo
	    echo "Options:"
	    echo "    --abort-on-error: kills the script if the configure, build or the test process fails"
	    echo "    --coverage: runs test with the code coverage feature"
	    echo "    --logfile <filename>: copy output into the file designed by 'filename'"
	    echo "    --test-id <id>: runs the test designed by the identifier 'id'"
	    echo "    --build-only: runs compilation test only"
	    echo
	    echo "Environment variables:"
	    echo "    MARCEL_TOPO: ${MARCEL_TOPO}"
	    echo "    MARCEL_LIBTYPE: ${MARCEL_LIBTYPE}"
	    echo "    MARCEL_SCHEDULERS: ${MARCEL_SCHEDULERS}"
	    echo
	    exit 0 ;;
    esac
    shift
done

#################################################

## Create and jump to the workdir
mkdir ${WORKDIR}/build ; mkdir ${WORKDIR}/cov ; mkdir ${WORKDIR}/html 
cd ${WORKDIR}


COMMON_OPTIONS="--enable-atexit --enable-cleanup --enable-deviation --enable-exceptions --enable-keys --enable-migration --enable-once --enable-postexit --enable-signals --enable-suspend --enable-userspace --enable-fork"
DEBUG_OPTIONS="${COV_OPTIONS} --enable-bug_on --enable-maintainer_mode"


if [ ${COVERAGE_TEST} -eq 1 ]; then
    MARCEL_TEST_ITER=1
    export MARCEL_TEST_ITER

    check_tool "lcov"
    check_tool "genhtml" 
fi


TEST_NUM=0
unset PM2_ARGS
for topo in ${MARCEL_TOPO}
do
    case ${topo} in
	mono)
	    TOPO_OPTIONS="--with-topo=mono" ;;
	smp)
	    TOPO_OPTIONS="--with-topo=smp --enable-blocking --enable-stats" ;;
	numa)
	    TOPO_OPTIONS="--with-topo=numa --enable-bubbles --enable-blocking --enable-stats" ;;
    esac

    for libtype in ${MARCEL_LIBTYPE}
    do
	if [ ${topo} = numa ] && [ "${MAKEOPTS}" != "" ]; then
	    for sched in ${MARCEL_SCHEDULERS}
	    do
		PM2_ARGS="--marcel-bubble-scheduler ${sched}"
		export PM2_ARGS
		exec_tests
		unset PM2_ARGS
	    done
	else
	    exec_tests
	fi
    done
done


### End of script
if [ ${COVERAGE_TEST} -eq 1 ]; then
    genhtml --function-coverage --legend ${WORKDIR}/cov/*.lcov -o ${WORKDIR}/html -t "Marcel Coverage test results"
    echo "The report is located at : ${WORKDIR}/html"
else
    rm -rf ${WORKDIR}
fi

if [ -n "${TEST_SET_FAILED}" ]; then
    echo "Following test set(s) failed: ${TEST_SET_FAILED}"
    exit 1;
fi
echo "Tests done"
