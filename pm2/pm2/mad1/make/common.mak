#
#                      PM2 HIGH-PERF/ISOMALLOC
#           High Performance Parallel Multithreaded Machine
#                           version 3.0
#
#     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
#       Christian Perez, Jean-Francois Mehaut, Raymond Namyst
#
#            Laboratoire de l'Informatique du Parallelisme
#                        UMR 5668 CNRS-INRIA
#                 Ecole Normale Superieure de Lyon
#
#                      External Contributors:
#                 Yves Denneulin (LMC - Grenoble),
#                 Benoit Planquelle (LIFL - Lille)
#
#                    1998 All Rights Reserved
#
#
#                             NOTICE
#
# Permission to use, copy, modify, and distribute this software and
# its documentation for any purpose and without fee is hereby granted
# provided that the above copyright notice appear in all copies and
# that both the copyright notice and this permission notice appear in
# supporting documentation.
#
# Neither the institutions (Ecole Normale Superieure de Lyon,
# Laboratoire de L'informatique du Parallelisme, Universite des
# Sciences et Technologies de Lille, Laboratoire d'Informatique
# Fondamentale de Lille), nor the Authors make any representations
# about the suitability of this software for any purpose. This
# software is provided ``as is'' without express or implied warranty.
#

###################### CUSTOMIZATION SECTION #####################
#
# The expansion of the macro MAD_OPTIONS may include the following
# definitions :
#
#	-DMAD_TIMING
#		Sets the "trace mode" on.
#
#	-DUSE_SAFE_MALLOC
#		Uses a small "safe malloc" library that trys to
#		verify bad malloc/free usage.
#

MAD_OPTIONS		=	

MAD_PURIFY		=	no

MAD_STANDALONE_OPTIONS	=	-Wall -O6 # -DMAD_TIMING -DUSE_SAFE_MALLOC

################### END OF CUSTOMIZATION SECTION #################

ifndef MAD1_ROOT
MAD1_ROOT	=	$(PM2_ROOT)/mad1
endif

ifndef PM2_ARCH
PM2_ARCH	:=	$(shell $(PM2_ROOT)/bin/pm2_arch)
PM2_SYS		:=	$(shell $(PM2_ROOT)/bin/pm2_sys)
PM2_ARCH_SYS	:=	$(shell basename $(PM2_SYS) _SYS)/$(shell basename $(PM2_ARCH) _ARCH)
endif

include $(MAD1_ROOT)/make/custom/options.mak

include $(MAD1_ROOT)/make/archdep/$(shell basename $(PM2_SYS) _SYS).inc

MAD_MAKEFILE	=	$(MAD1_ROOT)/make/common.mak \
			$(MAD1_ROOT)/make/custom/options.mak

ifdef PM2_FLAGS
THE_MAD_MAKEFILES	=	$(MAD_MAKEFILE) $(PM2_ROOT)/make/common.mak $(MAD1_ROOT)/.mad_pm2
else
THE_MAD_MAKEFILES	=	$(MAD_MAKEFILE) $(MAD1_ROOT)/.mad_standalone
endif

MAD_SOBJ	=	$(MAD1_ROOT)/source/obj/$(PM2_ARCH_SYS)
MAD_INC		=	$(MAD1_ROOT)/include
MAD_SRC		=	$(MAD1_ROOT)/source

ifdef PM2_FLAGS
MAD_OBJECTS	=	$(MAD_SOBJ)/netinterf.o $(MAD_SOBJ)/madeleine.o
else
MAD_OBJECTS	=	$(MAD_SOBJ)/netinterf.o $(MAD_SOBJ)/madeleine.o \
			$(MAD_SOBJ)/timing.o $(MAD_SOBJ)/safe_malloc.o
endif

MAD_HEADERS	=	$(MAD_INC)/madeleine.h $(MAD_INC)/pointers.h \
			$(MAD_INC)/mad_types.h $(MAD_INC)/sys/netinterf.h \
			$(MAD_INC)/mad_timing.h $(MAD_INC)/sys/debug.h \
			$(THE_MAD_MAKEFILES)

MAD_LIBD	=	$(MAD1_ROOT)/lib/$(PM2_ARCH_SYS)
MAD_LIB		=	$(MAD_LIBD)/libmad.a
MAD_LLIB	=	-lmad $(NET_LLIBS) $(ARCHDLIB)

MAD_BIN		=	bin/$(PM2_ARCH_SYS)
MAD_OBJ		=	obj/$(PM2_ARCH_SYS)

OSF_SYS_CFL	=	-fomit-frame-pointer
UNICOS_SYS_CFL	=	-t cray-t3d
LINUX_SYS_CFL	=	-fomit-frame-pointer
FREEBSD_SYS_CFL	=	-fomit-frame-pointer
AIX_SYS_CFL	=	-fomit-frame-pointer
IRIX_SYS_CFL	=	-fomit-frame-pointer
SOLARIS_SYS_CFL	=	-fomit-frame-pointer

MAD_AFLAGS	=	$($(PM2_SYS)_CFL)

OSF_SYS_CC	=	gcc
UNICOS_SYS_CC	=	cc
LINUX_SYS_CC	=	gcc
FREEBSD_SYS_CC	=	gcc
AIX_SYS_CC	=	gcc
IRIX_SYS_CC	=	gcc
SOLARIS_SYS_CC	=	gcc

MAD_CC		=	$($(PM2_SYS)_CC)

ifdef PM2_FLAGS
MAD_CFLAGS	=	$(MAD_OPTIONS) $(PM2_FLAGS) \
			-DNET_ARCH=\"$(NET_INTERF)\" $(NET_INIT) \
			-I$(MAD_INC) -I$(MAR_INC) -I$(PM2_ROOT)/include
else
MAD_CFLAGS	=	$(MAD_OPTIONS) $(MAD_STANDALONE_OPTIONS) \
			-DNET_ARCH=\"$(NET_INTERF)\" $(NET_INIT) -I$(MAD_INC)
endif

MAD_LFLAGS	=	-L$(MAD_LIBD) $(NET_LFLAGS)
MAD_CC2		=	$(MAD_CC) $(MAD_CFLAGS) $(MAD_AFLAGS) -c -o $@ $(@F:.o=.c)

ifeq ($(MAD_PURIFY),yes)
MAD_LINK_PREFIX = purify
endif
MAD_LINK	=	$(MAD_LINK_PREFIX) $(MAD_CC) $(MAD_LFLAGS) -o $@ $(MAD_OBJ)/$(@F).o
MAD_MOVE	=	mv $@ $(MAD_BIN)

MAD_OSF_SYS_RANLIB		=	ranlib $(MAD_LIB)
MAD_AIX_SYS_RANLIB		=	ranlib $(MAD_LIB)
MAD_FREEBSD_SYS_RANLIB		=	ranlib $(MAD_LIB)

MAD_RANLIB	=	$(MAD_$(PM2_SYS)_RANLIB)

mad_default: $(MAD_LIB)

$(MAD_LIB): $(MAD_OBJECTS)
ifdef PM2_FLAGS
	rm -f $(MAD1_ROOT)/.mad_standalone
else
	rm -f $(MAD1_ROOT)/.mad_pm2
endif
	rm -f $(MAD_LIB)
	ar cr $(MAD_LIB) $(MAD_OBJECTS)
	$(MAD_RANLIB)

$(MAD1_ROOT)/.mad_pm2:
	cp /dev/null $(MAD1_ROOT)/.mad_pm2

$(MAD1_ROOT)/.mad_standalone:
	cp /dev/null $(MAD1_ROOT)/.mad_standalone

# madeleine :

$(MAD_SOBJ)/madeleine.o: $(MAD_SRC)/madeleine.c \
			$(MAD_HEADERS) $(MAR_HEADERS) $(THE_MAD_MAKEFILES)
	$(MAD_CC) $(MAD_CFLAGS) $(MAD_AFLAGS) -c -o $(MAD_SOBJ)/madeleine.o $(MAD_SRC)/madeleine.c


# netinterf :

$(MAD_SOBJ)/netinterf.o: $(MAD_SRC)/$(NET_INTERF)/netinterf.c \
			$(MAD_HEADERS) $(MAR_HEADERS) $(THE_MAD_MAKEFILES)
	$(MAD_CC) $(MAD_CFLAGS) $(NET_CFLAGS) $(MAD_AFLAGS) -c -o $(MAD_SOBJ)/netinterf.o $(MAD_SRC)/$(NET_INTERF)/netinterf.c


ifndef PM2_FLAGS
# timing :

$(MAD_SOBJ)/timing.o: $(MAD_SRC)/timing.c $(MAD_INC)/timing.h \
			$(THE_MAD_MAKEFILES)
	$(MAD_CC) $(MAD_CFLAGS) $(NET_CFLAGS) $(MAD_AFLAGS) -c -o $(MAD_SOBJ)/timing.o $(MAD_SRC)/timing.c

# safe_malloc :

$(MAD_SOBJ)/safe_malloc.o: $(MAD_SRC)/safe_malloc.c $(MAD_INC)/safe_malloc.h \
			$(THE_MAD_MAKEFILES)
	$(MAD_CC) $(MAD_CFLAGS) $(NET_CFLAGS) $(MAD_AFLAGS) -c -o $(MAD_SOBJ)/safe_malloc.o $(MAD_SRC)/safe_malloc.c

endif
