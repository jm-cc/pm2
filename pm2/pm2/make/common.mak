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
# The expansion of the macro OPTIONS may include the following
# definitions :
#
#	-DMINIMAL_PREEMPTION
#		Sets up a 'minimal preemption' mechanism that will
#		only give hand (from time to time) to a special
#		'message-receiver' thread.
#		This option *must* be set as a GLOBAL_OPTION.
#
#	-DUSE_DYN_ARRAYS
#		Uses a gcc feature that allow the declaration of
#		arrays the size of which depends on a variable. DON'T
#		USE this option if you plan to use thread migration
#		stuff without GLOBAL_ADDRESSING being set.
#
#	-DPM2_TIMING
#		Sets the "profiling mode" on.
#
#	-DMIGRATE_IN_HEADER
#		Private Flag. Should *not* be used in regular
#		applications.
#
#	-DUSE_SAFE_MALLOC
#		Uses a small "safe malloc" library that trys to
#		verify bad malloc/free usage.
#
# And also traditional gcc compiling options such as -g, -p, -DNDEBUG,
# etc.
#
# The options set in `GLOBAL_OPTIONS' will be used by MARCEL and
# MADELEINE too.
#

GLOBAL_OPTIONS	=	-Wall -O6 #-DMAR_TRACE #-DSTANDARD_MAIN -g -DUSE_SAFE_MALLOC

SMP		=	no
TOOLBOX         =       no
NETTOOLBOX      =       no
DSM		=	no

OPTIONS		=	#-DMIGRATE_IN_HEADER # -DPM2_TIMING


################### END OF CUSTOMIZATION SECTION #################

ifndef PM2_ARCH_SYS
PM2_ARCH	:=	$(shell $(PM2_ROOT)/bin/pm2_arch)
PM2_SYS		:=	$(shell $(PM2_ROOT)/bin/pm2_sys)
PM2_ARCH_SYS	:=	$(shell basename $(PM2_SYS) _SYS)/$(shell basename $(PM2_ARCH) _ARCH)
endif

PM2_FLAGS	=	-DPM2 -D$(PM2_SYS) -D$(PM2_ARCH) $(GLOBAL_OPTIONS)

ifeq ($(SMP),yes)
PM2_FLAGS += -DSMP -D_REENTRANT
endif

ifeq ($(DSM),yes)
PM2_FLAGS += -DDSM
endif

include $(PM2_ROOT)/make/mad.mak

ifeq ($(MAD2),yes)
TOOLBOX    = yes
NETTOOLBOX = yes
PM2_FLAGS += -DMAD2
else
PM2_FLAGS += -DMAD1
endif

ifeq ($(TOOLBOX),yes)
PM2_FLAGS += -DTBX
endif

ifeq ($(NETTOOLBOX),yes)
PM2_FLAGS += -DNTBX -DNTBX_TCP
endif

PM2_LIBD	=	$(PM2_ROOT)/lib/$(PM2_ARCH_SYS)
PM2_LIB		=	$(PM2_LIBD)/libpm2.a
PM2_LLIB	=	-lpm2

PM2_SRC		=	$(PM2_ROOT)/source
PM2_INC		=	$(PM2_ROOT)/include
PM2_SOBJ	=	$(PM2_SRC)/obj/$(PM2_ARCH_SYS)

PM2_BIN		=	bin/$(PM2_ARCH_SYS)
PM2_OBJ		=	obj/$(PM2_ARCH_SYS)

ifeq ($(TOOLBOX),yes)
default: pm2_default mad_default mar_default tbx_default ntbx_default
else
default: pm2_default mad_default mar_default
endif

include $(MARCEL_ROOT)/make/common.mak

ifeq ($(MAD2),yes)
 include $(MAD2_ROOT)/make/common.mak
else
 include $(MADELEINE_ROOT)/make/common.mak
endif

ifeq ($(DSM),yes)
include $(DSM_ROOT)/make/common.mak
endif

ifeq ($(TOOLBOX),yes)
include $(PM2_ROOT)/toolbox/make/common.mak
endif

ifeq ($(NETTOOLBOX),yes)
include $(PM2_ROOT)/toolbox/net/make/common.mak
endif

PM2_MAKEFILE	=	$(PM2_ROOT)/make/common.mak \
			$(PM2_ROOT)/make/mad.mak \
			$(MAR_MAKEFILE) \
			$(MAD_MAKEFILE) \
			$(DSM_MAKEFILE) \
                        $(TBX_MAKEFILE) \
                        $(NTBX_MAKEFILE)

OSF_SYS_CFL	=	-fomit-frame-pointer
UNICOS_SYS_CFL	=	-t cray-t3d
LINUX_SYS_CFL	=	-fomit-frame-pointer
FREEBSD_SYS_CFL	=	-fomit-frame-pointer
AIX_SYS_CFL	=	-fomit-frame-pointer
IRIX_SYS_CFL	=	-fomit-frame-pointer
SOLARIS_SYS_CFL	=	-fomit-frame-pointer

PM2_AFLAGS	=	$($(PM2_SYS)_CFL)

OSF_SYS_CC	=	gcc
UNICOS_SYS_CC	=	cc
LINUX_SYS_CC	=	gcc
FREEBSD_SYS_CC	=	gcc
AIX_SYS_CC	=	gcc
IRIX_SYS_CC	=	gcc
SOLARIS_SYS_CC	=	gcc

ifdef MAD_PRIVATE_CC
PM2_CC          =       $(MAD_PRIVATE_CC)
else
PM2_CC		=	$($(PM2_SYS)_CC)
endif

PM2_CFLAGS	=	$(PM2_FLAGS) \
			-I$(PM2_INC) -I$(MAD_INC) -I$(MAR_INC) 

ifeq ($(TOOLBOX),yes)
PM2_CFLAGS	+=	-I$(TBX_INC)
endif

ifeq ($(NETTOOLBOX),yes)
PM2_CFLAGS	+=	-I$(NTBX_INC)
endif

ifeq ($(DSM),yes)
PM2_CFLAGS	+=	-I$(DSM_INC)
endif

PM2_LFLAGS  	=	-L$(PM2_LIBD) $(MAD_LFLAGS) $(MAR_LFLAGS)

ifeq ($(DSM),yes)
PM2_LFLAGS	+=	$(DSM_LFLAGS)
endif

PM2_LIBS	=	$(PM2_LIB) $(MAD_LIB) $(MAR_LIB) $(DSM_LIB) 
PM2_LLIBS	=	$(PM2_LLIB) $(MAD_LLIB) $(MAR_LLIB) $(DSM_LLIB) 
ifeq ($(TOOLBOX),yes)
PM2_LIBS	+=	$(TBX_LIB)
PM2_LLIBS	+=	$(TBX_LLIB)
endif

ifeq ($(NETTOOLBOX),yes)
PM2_LIBS	+=	$(NTBX_LIB)
PM2_LLIBS	+=	$(NTBX_LLIB)
endif

PM2_LINK	=	$(PM2_CC) $(PM2_LFLAGS) -o $@ $(PM2_OBJ)/$(@F).o
PM2_MOVE	=	mv $@ $(PM2_BIN)

PM2_CC2		=	$(PM2_CC) $(PM2_CFLAGS) $(PM2_AFLAGS) -c -o $@ $(@F:.o=.c)

PM2_OBJECTS	=	$(PM2_SOBJ)/pm2.o $(PM2_SOBJ)/pm2_attr.o $(PM2_SOBJ)/console.o \
			$(PM2_SOBJ)/netserver.o $(PM2_SOBJ)/isomalloc.o \
			$(PM2_SOBJ)/block_alloc.o $(PM2_SOBJ)/timing.o \
			$(PM2_SOBJ)/isomalloc_rpc.o $(PM2_SOBJ)/safe_malloc.o \
			$(PM2_SOBJ)/bitmap.o $(PM2_SOBJ)/slot_distrib.o \
			$(PM2_SOBJ)/pm2_thread.o $(PM2_SOBJ)/pm2_printf.o \
			$(PM2_SOBJ)/pm2_migr.o $(PM2_SOBJ)/pm2_rpc.o \
			$(PM2_SOBJ)/pm2_mad.o

PM2_HEADERS	=	$(PM2_INC)/pm2.h $(PM2_INC)/pm2_attr.h \
			$(PM2_INC)/pm2_thread.h $(PM2_INC)/pm2_rpc.h \
			$(PM2_INC)/pm2_types.h $(PM2_INC)/pm2_mad.h \
			$(PM2_INC)/sys/debug.h \
			$(PM2_INC)/timing.h $(PM2_INC)/pm2_timing.h \
			$(PM2_INC)/isomalloc_timing.h $(PM2_INC)/safe_malloc.h \
			$(PM2_INC)/block_alloc.h $(PM2_INC)/isomalloc.h $(PM2_INC)/magic.h \
			$(PM2_INC)/sys/console.h $(PM2_INC)/sys/isomalloc_rpc.h \
			$(PM2_INC)/sys/slot_distrib.h \
			$(MAD_HEADERS) \
			$(MAR_HEADERS) \
			$(DSM_HEADERS) \
			$(PM2_MAKEFILE)

X11_INC		=	-I/usr/X11R6/include
X11_LIB		=	-L/usr/X11R6/lib

PM2_XCC		=	$(PM2_CC) $(PM2_CFLAGS) $(PM2_AFLAGS) -I$$OPENWINHOME/include -c $(X11_INC) -o $@ $(@F:.o=.c)

PM2_XLLIBS		=	$(X11_LIB) $(PM2_LLIBS) -lxview -lolgx -lX11

PM2_XLINK		=	$(PM2_CC) $(PM2_LFLAGS) -L$$OPENWINHOME/lib -o $@ $(PM2_OBJ)/$(@F).o

PM2_OSF_SYS_RANLIB	=	ranlib $(PM2_LIB)
PM2_AIX_SYS_RANLIB	=	ranlib $(PM2_LIB)
PM2_FREEBSD_SYS_RANLIB	=	ranlib $(PM2_LIB)

RANLIB		=	$(PM2_$(PM2_SYS)_RANLIB)

pm2_default: $(PM2_LIB)

# librairie :

$(PM2_LIB): $(PM2_OBJECTS)
	rm -f $(PM2_LIB)
	ar cr $(PM2_LIB) $(PM2_OBJECTS)
	$(RANLIB)

# pm2 :

$(PM2_SOBJ)/pm2.o: $(PM2_SRC)/pm2.c \
		$(PM2_INC)/sys/netserver.h $(PM2_INC)/sys/bitmap.h \
		$(PM2_HEADERS) $(MAD_HEADERS) $(MAR_HEADERS) $(DSM_HEADERS) \
		$(PM2_MAKEFILE)
	$(PM2_CC) $(OPTIONS) $(PM2_CFLAGS) $(PM2_AFLAGS) -c -o $(PM2_SOBJ)/pm2.o $(PM2_SRC)/pm2.c

# pm2_rpc :

$(PM2_SOBJ)/pm2_rpc.o: $(PM2_SRC)/pm2_rpc.c \
		$(PM2_INC)/sys/netserver.h \
		$(PM2_HEADERS) $(MAD_HEADERS) $(MAR_HEADERS) $(DSM_HEADERS) \
		$(PM2_MAKEFILE)
	$(PM2_CC) $(OPTIONS) $(PM2_CFLAGS) $(PM2_AFLAGS) -c -o $(PM2_SOBJ)/pm2_rpc.o $(PM2_SRC)/pm2_rpc.c

# pm2_attr :

$(PM2_SOBJ)/pm2_attr.o: $(PM2_SRC)/pm2_attr.c \
		$(PM2_HEADERS) $(MAD_HEADERS) $(MAR_HEADERS) $(DSM_HEADERS) \
		$(PM2_MAKEFILE)
	$(PM2_CC) $(OPTIONS) $(PM2_CFLAGS) $(PM2_AFLAGS) -c -o $(PM2_SOBJ)/pm2_attr.o $(PM2_SRC)/pm2_attr.c

# pm2_mad :

$(PM2_SOBJ)/pm2_mad.o: $(PM2_SRC)/pm2_mad.c \
		$(PM2_HEADERS) $(MAD_HEADERS) $(MAR_HEADERS) $(DSM_HEADERS) \
		$(PM2_MAKEFILE)
	$(PM2_CC) $(OPTIONS) $(PM2_CFLAGS) $(NET_CFLAGS) -DNET_ARCH=\"$(NET_INTERF)\" $(PM2_AFLAGS) -c -o $(PM2_SOBJ)/pm2_mad.o $(PM2_SRC)/pm2_mad.c

# pm2_thread :

$(PM2_SOBJ)/pm2_thread.o: $(PM2_SRC)/pm2_thread.c \
		$(PM2_HEADERS) $(MAD_HEADERS) $(MAR_HEADERS) $(DSM_HEADERS) \
		$(PM2_MAKEFILE)
	$(PM2_CC) $(OPTIONS) $(PM2_CFLAGS) $(PM2_AFLAGS) -c -o $(PM2_SOBJ)/pm2_thread.o $(PM2_SRC)/pm2_thread.c

# pm2_printf :

$(PM2_SOBJ)/pm2_printf.o: $(PM2_SRC)/pm2_printf.c \
		$(PM2_INC)/sys/pm2_printf.h \
		$(PM2_HEADERS) $(MAD_HEADERS) $(MAR_HEADERS) $(DSM_HEADERS) \
		$(PM2_MAKEFILE)
	$(PM2_CC) $(OPTIONS) $(PM2_CFLAGS) $(PM2_AFLAGS) -c -o $(PM2_SOBJ)/pm2_printf.o $(PM2_SRC)/pm2_printf.c

# pm2_migr :

$(PM2_SOBJ)/pm2_migr.o: $(PM2_SRC)/pm2_migr.c \
		$(PM2_INC)/sys/pm2_migr.h \
		$(PM2_HEADERS) $(MAD_HEADERS) $(MAR_HEADERS) $(DSM_HEADERS) \
		$(PM2_MAKEFILE)
	$(PM2_CC) $(OPTIONS) $(PM2_CFLAGS) $(PM2_AFLAGS) -c -o $(PM2_SOBJ)/pm2_migr.o $(PM2_SRC)/pm2_migr.c

# console :

$(PM2_SOBJ)/console.o: $(PM2_SRC)/console.c \
		$(PM2_HEADERS) $(MAD_HEADERS) $(MAR_HEADERS) $(DSM_HEADERS) \
		$(PM2_MAKEFILE)
	$(PM2_CC) $(OPTIONS) $(PM2_CFLAGS) $(PM2_AFLAGS) -c -o $(PM2_SOBJ)/console.o $(PM2_SRC)/console.c


# netserver :

$(PM2_SOBJ)/netserver.o: $(PM2_SRC)/netserver.c \
		$(PM2_INC)/sys/netserver.h \
		$(PM2_HEADERS) $(MAD_HEADERS) $(MAR_HEADERS) $(DSM_HEADERS) \
		$(PM2_MAKEFILE)
	$(PM2_CC) $(OPTIONS) $(PM2_CFLAGS) $(PM2_AFLAGS) -c -o $(PM2_SOBJ)/netserver.o $(PM2_SRC)/netserver.c


# isomalloc :

$(PM2_SOBJ)/isomalloc.o: $(PM2_SRC)/isomalloc.c \
		$(PM2_INC)/sys/bitmap.h \
		$(PM2_INC)/sys/isomalloc_archdep.h \
		$(PM2_HEADERS) $(MAD_HEADERS) $(MAR_HEADERS) $(DSM_HEADERS) \
		$(PM2_MAKEFILE)
	$(PM2_CC) $(OPTIONS) $(PM2_CFLAGS) $(PM2_AFLAGS) -c -o $(PM2_SOBJ)/isomalloc.o $(PM2_SRC)/isomalloc.c


# block_alloc :

$(PM2_SOBJ)/block_alloc.o: $(PM2_SRC)/block_alloc.c \
		$(PM2_INC)/sys/bitmap.h \
		$(PM2_HEADERS) $(MAD_HEADERS) $(MAR_HEADERS) $(DSM_HEADERS) \
		$(PM2_MAKEFILE)
	$(PM2_CC) $(OPTIONS) $(PM2_CFLAGS) $(PM2_AFLAGS) -c -o $(PM2_SOBJ)/block_alloc.o $(PM2_SRC)/block_alloc.c


# isomalloc_rpc :

$(PM2_SOBJ)/isomalloc_rpc.o: $(PM2_SRC)/isomalloc_rpc.c \
		$(PM2_HEADERS) $(MAD_HEADERS) $(MAR_HEADERS) $(DSM_HEADERS) \
		$(PM2_MAKEFILE)
	$(PM2_CC) $(OPTIONS) $(PM2_CFLAGS) $(PM2_AFLAGS) -c -o $(PM2_SOBJ)/isomalloc_rpc.o $(PM2_SRC)/isomalloc_rpc.c


# bitmap :

$(PM2_SOBJ)/bitmap.o: $(PM2_SRC)/bitmap.c \
		$(PM2_INC)/sys/bitmap.h \
		$(PM2_HEADERS) $(MAD_HEADERS) $(MAR_HEADERS) $(DSM_HEADERS) \
		$(PM2_MAKEFILE)
	$(PM2_CC) $(OPTIONS) $(PM2_CFLAGS) $(PM2_AFLAGS) -c -o $(PM2_SOBJ)/bitmap.o $(PM2_SRC)/bitmap.c


# slot_distrib :

$(PM2_SOBJ)/slot_distrib.o: $(PM2_SRC)/slot_distrib.c \
		$(PM2_INC)/sys/slot_distrib.h \
		$(PM2_HEADERS) $(MAD_HEADERS) $(MAR_HEADERS) $(DSM_HEADERS) \
		$(PM2_MAKEFILE)
	$(PM2_CC) $(OPTIONS) $(PM2_CFLAGS) $(PM2_AFLAGS) -c -o $(PM2_SOBJ)/slot_distrib.o $(PM2_SRC)/slot_distrib.c


# timing :

$(PM2_SOBJ)/timing.o: $(PM2_SRC)/timing.c $(PM2_INC)/timing.h \
		$(PM2_MAKEFILE)
	$(PM2_CC) $(OPTIONS) $(PM2_CFLAGS) $(PM2_AFLAGS) -c -o $(PM2_SOBJ)/timing.o $(PM2_SRC)/timing.c


# safe_malloc :

$(PM2_SOBJ)/safe_malloc.o: $(PM2_SRC)/safe_malloc.c $(PM2_INC)/safe_malloc.h \
		$(PM2_MAKEFILE)
	$(PM2_CC) $(OPTIONS) $(PM2_CFLAGS) $(PM2_AFLAGS) -c -o $(PM2_SOBJ)/safe_malloc.o $(PM2_SRC)/safe_malloc.c


