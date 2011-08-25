#
# PM2: Parallel Multithreaded Machine
# Copyright (C) 2011 "the PM2 team" (see AUTHORS file)
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


ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS	= -I$(abs_top_srcdir)
AM_CFLAGS   	= -Wall $(CFLAGS)
AM_LDFLAGS  	= -Wl,-lpthread


EXTRA_DIST	=	main.c autogen.sh m4

bin_PROGRAMS 	= 	contested_mutex_lock contested_mutex_trylock 		\
			uncontested_mutex_lock uncontested_mutex_trylock	\
			contested_sem_wait contested_sem_trywait		\
			uncontested_sem_wait uncontested_sem_trywait		\
			contested_spin_lock contested_spin_trylock 		\
			uncontested_spin_lock uncontested_spin_trylock		\
			uncontested_create contested_barrier_wait		\
			contested_rwlock_rdlock contested_rwlock_tryrdlock	\
			contested_rwlock_wrlock contested_rwlock_trywrlock	\
			uncontested_rwlock_rdlock uncontested_rwlock_tryrdlock	\
			uncontested_rwlock_wrlock uncontested_rwlock_trywrlock	\
			contested_cond_signal contested_cond_broadcast
