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


AM_CPPFLAGS  = -DDEBUG -imacros $(abs_top_builddir)/config.h -I $(abs_top_srcdir)/include -I $(abs_top_builddir)/include $(GLOBAL_AM_CPPFLAGS) $(PKG_AM_CPPFLAGS) $(TBX_CFLAGS)
AM_CFLAGS    = $(GLOBAL_AM_CFLAGS)
AM_LDFLAGS   = $(GLOBAL_AM_LDFLAGS) $(TBX_LIBS)  -no-install


noinst_HEADERS = bubble-testing.h pthread-abi.h tsp-job.h tsp-types.h tsp.h

clean-local:
	$(RM_F) $(SCRIPT_TESTS)


## Build a pthread or pmarcel lib
if IS_LIBPTHREAD
  TESTS_ENVIRONMENT = $(LOADER_BIN) LD_PRELOAD=$(abs_top_builddir)/source/.libs/libpthread.so
  LDADD = $(abs_top_builddir)/source/libpthread.la $(GLOBAL_AM_LDADD)
else
TESTS_ENVIRONMENT = $(LOADER_BIN)
if IS_LIBPMARCEL
  LDADD = $(abs_top_builddir)/source/libpmarcel.la $(GLOBAL_AM_LDADD)
else
  LDADD = $(abs_top_builddir)/source/libmarcel.la  $(GLOBAL_AM_LDADD)
endif
endif


## test loader program
LOADER_BIN     = $(abs_top_builddir)/tests/$(LOADER)
loader_SOURCES = test_loader.c
loader_LDADD   =


## tsp test is cut in multiple files
tsp_SOURCES   = tsp.c tsp-main.c tsp-job.c
tsp_LDADD     = $(LDADD) -lm

# c++ tests
pthread_cxx_SOURCES = pthread_cxx.cpp
pthread_cxx_initializer_SOURCES = pthread_cxx_initializer.cpp
simple_cxx_SOURCES = simple_cxx.cc

# fortran tests
simple_fc_SOURCES = simple_fc.F90


LOADER        = loader
MISC_TESTS    =	barrier-1 barrier-2 barrier-3 barrier-native contsw create create2 implicit-join 	\
		test-activity tube active cleanup gs_sumtime sumtime prio realtime smpheavy stack_jump 	\
		keys suspend timeout userspace smptest tsp attr1 attr3 spin1 spin2 detach1 setjmp	\
		stack1 stack2 stack3 stack4 once once1 once2 once3 once4 nojoin join1 join4 cond cond8 	\
		mutex1 mutex2 mutex3 mutex5 mutex5a mutex7 mutex7a recursivemutex kill2 kill3 tls exit1 \
		exit2 cancel1 cancel8 cancel10 cancel12 cancel13 cancel19 exec2 exec3 system fork+exec 	\
		fork fork1 fork2 fork4 recursive-fork sem1 sem2 sem2a sem5 sem6 sem10 signal1 signal4 	\
		signal5 sigwait	sigmask1 sigmask2 sigmask3 sigaction dflsigblock sigprocmask sigsuspend	\
		sigpending waittime disable keys-pthread semfile1 semfile2 thsigqueue kill_other_threads\
		pthread-abi-cond1 pthread-abi-cond2 pthread-abi-fork write				\
		pthread-abi-fork+exec pthread-abi-simple pthread-abi-mutex1 pthread-abi-mutex2 		\
		pthread-abi-mutex3 pthread-abi-mutex5 pthread-abi-rwlock1 pthread-abi-rwlock2 		\
		pthread-abi-rwlock3 pthread-abi-rwlock6 pthread-abi-rwlock7 pthread-abi-sigmask1 	\
		pthread-abi-sigmask2 pthread-abi-sigmask3 pthread_cxx pthread_cxx_initializer simple_cxx
BUBBLE_TESTS  = bubble-cache-01 bubble-cache-02 bubble-cache-03 bubble-cache-04 bubble-cache-05 	\
		bubble-cache-06 bubble-cache-07 bubble-cache-steal-01 bubble-null 			\
		bubble-memory-01 bubble-memory-02 bubble-memory-03 bubble-memory-04			\
		schedule-on-thread-holder bubble-sched-lookup bubble-sched-cmdline schedpolicy
NUMA_TESTS    = blocking futex pingpong heavyalloc shake vpmask

if HAVE_FORTRAN
  MISC_TESTS += simple_fc
endif


check_PROGRAMS = $(LOADER) $(MISC_TESTS) $(PTHREAD_TESTS) $(BUBBLE_TESTS) $(NUMA_TESTS) 
TESTS =  $(MISC_TESTS) $(PTHREAD_TESTS) $(BUBBLE_TESTS) $(NUMA_TESTS)
