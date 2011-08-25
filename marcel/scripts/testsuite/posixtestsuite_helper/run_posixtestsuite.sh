rm conformance/interfaces/sigqueue/4-1.c
#!/bin/sh

#
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
#


WORKDIR=`mktemp -d /tmp/tmp.XXXXXXX`
ABS_SRCDIR=""
PTS_NAME=posixtestsuite-1.5.2
PTS_NAME_SHA1="7fa215c8e5a53f58d324cef1997edfea52f3d678"
TEST_SET_NAME="${TEST_SET_NAME:-SEM SIG THR TPS}"
MAKE="${MAKE:-make}"


# args: file
absolute_path()
{
    cd `dirname $0`
    ABS_SRCDIR=`pwd -P`
    cd -
}


### Prepare posixtestsuite
absolute_path $0
SRCDIR="${ABS_SRCDIR}/../../.."

# Download posixtestsuite source code if archive is not found
cd    /tmp
if test ! -f ${PTS_NAME}.tar.gz; then
    wget  http://sourceforge.net/projects/posixtest/files/posixtest/${PTS_NAME}/${PTS_NAME}.tar.gz
    if [ $? -ne 0 ]; then
	echo "Download failed"
	exit 1
    fi
fi

# Check archive integrity
EXPECTED="${PTS_NAME_SHA1} */tmp/${PTS_NAME}.tar.gz"
RESULT=`shasum -a 1 -b /tmp/${PTS_NAME}.tar.gz`
if test $? -ne 0 || test "$EXPECTED" != "$RESULT"; then
    echo "Archive integrity check fails"
    exit 1
fi

########################################################################################
# Patch or remove tests which are false
#######################################
cd    ${WORKDIR}
tar   zxvf /tmp/${PTS_NAME}.tar.gz
cd    posixtestsuite
for patch_file in `ls ${SRCDIR}/scripts/testsuite/posixtestsuite_helper/patches`;
do
    echo "*** Applying patch file ${patch_file} ***"
    patch -p0 < ${SRCDIR}/scripts/testsuite/posixtestsuite_helper/patches/${patch_file}
done

## Remove sem tests which use fork()
rm conformance/interfaces/sem_wait/7-1.c
rm conformance/interfaces/sem_init/3-2.c
rm conformance/interfaces/sem_init/3-3.c
rm conformance/interfaces/sem_post/8-1.c
rm conformance/interfaces/sem_unlink/2-2.c

## Remove SIG tests which use non supported stuffs (like rt signals ...)
rm conformance/interfaces/sigaltstack/1-1.c
rm conformance/interfaces/sigaltstack/6-1.c
rm conformance/interfaces/sigaltstack/7-1.c
rm conformance/interfaces/sigqueue/1-1.c
rm conformance/interfaces/sigqueue/4-1.c
rm conformance/interfaces/sigqueue/5-1.c
rm conformance/interfaces/sigqueue/6-1.c
rm conformance/interfaces/sigqueue/7-1.c
rm conformance/interfaces/sigqueue/8-1.c
rm conformance/interfaces/sigqueue/9-1.c
rm conformance/interfaces/sigwait/2-1.c
rm conformance/interfaces/sigwait/7-1.c
rm conformance/interfaces/sigwaitinfo/2-1.c
rm conformance/interfaces/sigwaitinfo/7-1.c
rm conformance/interfaces/sigwaitinfo/8-1.c
rm conformance/interfaces/sigaction/29-1.c
rm conformance/interfaces/sigaction/10-1.c

## Remove THR tests which will fail (non-init parameters, unimplemented functions, ...)
rm conformance/interfaces/pthread_barrierattr_getpshared/2-1.c
rm conformance/interfaces/pthread_barrierattr_setpshared/1-1.c
rm conformance/interfaces/pthread_attr_setscope/5-1.c
rm conformance/interfaces/pthread_barrier_wait/6-1.c
rm conformance/interfaces/pthread_create/10-1.c
rm conformance/interfaces/pthread_condattr_getpshared/1-2.c
rm conformance/interfaces/pthread_condattr_setpshared/1-2.c
rm conformance/interfaces/pthread_mutexattr_getpshared/1-2.c
rm conformance/interfaces/pthread_mutexattr_setpshared/2-2.c
rm conformance/interfaces/pthread_mutexattr_setpshared/1-1.c
rm conformance/interfaces/pthread_mutex_destroy/2-2.c
rm conformance/interfaces/pthread_mutex_destroy/5-2.c
rm conformance/interfaces/pthread_mutex_init/speculative/5-2.c
rm conformance/interfaces/pthread_mutex_trylock/1-2.c
rm conformance/interfaces/pthread_mutex_trylock/2-1.c
rm conformance/interfaces/pthread_mutex_trylock/4-2.c
rm conformance/interfaces/pthread_mutex_trylock/4-3.c
rm conformance/interfaces/pthread_rwlockattr_getpshared/2-1.c
rm conformance/interfaces/pthread_rwlockattr_setpshared/1-1.c
rm conformance/interfaces/pthread_attr_setschedpolicy/speculative/5-1.c
rm conformance/interfaces/pthread_cond_destroy/2-1.c
rm conformance/interfaces/pthread_cond_init/2-2.c
rm conformance/interfaces/pthread_cond_init/4-2.c
rm conformance/interfaces/pthread_cond_init/4-1.c
rm conformance/interfaces/pthread_cond_init/1-2.c
rm conformance/interfaces/pthread_cond_init/1-3.c
rm conformance/interfaces/pthread_cond_signal/1-2.c
rm conformance/interfaces/pthread_cond_timedwait/2-4.c
rm conformance/interfaces/pthread_cond_timedwait/2-5.c
rm conformance/interfaces/pthread_cond_timedwait/2-6.c
rm conformance/interfaces/pthread_cond_timedwait/2-7.c
rm conformance/interfaces/pthread_cond_timedwait/4-2.c
rm conformance/interfaces/pthread_cond_wait/2-2.c
rm conformance/interfaces/pthread_cond_wait/2-3.c
rm conformance/interfaces/pthread_mutexattr_gettype/speculative/3-1.c
### Missing definitions to build tests
rm conformance/interfaces/pthread_mutex_getprioceiling/*.c
rm conformance/interfaces/pthread_mutexattr_getprioceiling/*.c
rm conformance/interfaces/pthread_mutexattr_setprioceiling/*.c
rm conformance/interfaces/pthread_mutexattr_getprotocol/*.c
rm conformance/interfaces/pthread_mutexattr_setprotocol/*.c
echo

########################################################################################

### Build marcel libpthread version
cd    ${WORKDIR}
${SRCDIR}/configure --with-libtype=pthread --enable-bubbles --enable-stats --with-topo=numa --prefix=${WORKDIR}/marcel
${MAKE} && ${MAKE} install
if [ $? -ne 0 ]; then
    echo "Build failed !"
    exit 1
fi
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:${WORKDIR}/marcel/lib/pkgconfig
echo

### Marcel dependant files
cd   ${WORKDIR}/posixtestsuite
pkg-config --cflags --libs marcel > LDFLAGS
echo  "${WORKDIR}/marcel/bin/marcel-exec" > EXEC_PREFIX

### Run tests
for  test_set in $TEST_SET_NAME; 
do
     ./run_tests $test_set
done

if test "$TEST_TYPE" != integration; then
    cp   logfile ${SRCDIR}/marcel_pts.log
    exit 0
fi

## Todo: diff with expected results
grep -i failed logfile
if test $? -eq 0; then
    exit 0
fi

grep -i interrupted logfile
if test $? -eq 0; then
    exit 0
fi
