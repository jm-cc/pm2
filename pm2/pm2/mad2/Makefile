#
#                       PM2 HIGH-PERF/ISOMALLOC
#            High Performance Parallel Multithreaded Machine
#                            version 3.0
#
#      Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
#        Christian Perez, Jean-Francois Mehaut, Raymond Namyst
#
#             Laboratoire de l'Informatique du Parallelisme
#                         UMR 5668 CNRS-INRIA
#                  Ecole Normale Superieure de Lyon
#
#                       External Contributors:
#                  Yves Denneulin (LMC - Grenoble),
#                  Benoit Planquelle (LIFL - Lille)
#
#                     1998 All Rights Reserved
#
#
#                              NOTICE
#
#  Permission to use, copy, modify, and distribute this software and
#  its documentation for any purpose and without fee is hereby granted
#  provided that the above copyright notice appear in all copies and
#  that both the copyright notice and this permission notice appear in
#  supporting documentation.
#
#  Neither the institutions (Ecole Normale Superieure de Lyon,
#  Laboratoire de L'informatique du Parallelisme, Universite des
#  Sciences et Technologies de Lille, Laboratoire d'Informatique
#  Fondamentale de Lille), nor the Authors make any representations
#  about the suitability of this software for any purpose. This
#  software is provided ``as is'' without express or implied warranty.


#
# Makefile : Madeleine 2
##########################

#..Settings..................................................................#

# Debug
# DEBUG = on

# Trace
# TRACE = on

# Timing
# TIMING = on

#  OS 
#----------
OS_SOLARIS = on
# OS_LINUX = on

#  Drivers
#-----------
# TCP
DRV_TCP = on
# VIA
# DRV_VIA = on
# SISCI
# DRV_SISCI = on
# SBP
# DRV_SBP = on
# MPI
# DRV_MPI = on

# PM2
# PM2 = on

#..Private.part..............................................................#

# directories
OBJ = $(MAD2_ROOT)/obj
INC = $(MAD2_ROOT)/include
CI_INC = $(INC)/connection_interfaces
SRC = $(MAD2_ROOT)/src
CI_SRC = $(SRC)/connection_interfaces
LIB = $(MAD2_ROOT)/lib

# applications
APPS = mad_test \
       mad_ping

# applications objects
APPS_OBJS = mad_test.o \
            mad_ping.o

# objects
OBJS = $(OBJ)/mad_channel.o \
            $(OBJ)/mad_memory_management.o \
            $(OBJ)/mad_list_management.o \
            $(OBJ)/mad_communication.o \
            $(OBJ)/mad_malloc.o \
            $(OBJ)/mad_timing.o \
            $(OBJ)/mad_buffers.o \
            $(OBJ)/madeleine.o

# srcs
SRCS = $(SRC)/mad_channel.c \
            $(SRC)/mad_communication.c \
            $(SRC)/mad_malloc.c \
            $(SRC)/mad_timing.c \
            $(SRC)/mad_memory_management.c \
            $(SRC)/mad_list_management.c \
            $(SRC)/mad_buffers.c \
            $(SRC)/madeleine.c

# headers
HEADERS = $(INC)/mad_adapter.h \
               $(INC)/mad_adapter_description.h \
               $(INC)/mad_buffer_interface.h \
               $(INC)/mad_buffers.h \
               $(INC)/mad_channel.h \
               $(INC)/mad_channel_interface.h \
               $(INC)/mad_communication_interface.h \
               $(INC)/mad_configuration.h \
               $(INC)/mad_connection.h \
               $(INC)/mad_driver.h \
               $(INC)/mad_driver_interface.h \
               $(INC)/mad_link.h \
               $(INC)/mad_list.h \
               $(INC)/mad_list_interface.h \
               $(INC)/mad_macros.h \
               $(INC)/mad_main.h \
               $(INC)/mad_malloc.h \
               $(INC)/mad_malloc_interface.h \
               $(INC)/mad_memory_interface.h \
               $(INC)/mad_modes.h \
               $(INC)/mad_pointers.h \
               $(INC)/mad_registration.h \
               $(INC)/mad_timing.h \
               $(INC)/mad_types.h \
               $(INC)/madeleine.h 

# libs
LIB_MAD2 = $(LIB)/libmad2.a

# compiler
CC = gcc
# CC = egcc

# preprocessor flags
CPPFLAGS = 

# compiler flags
# CFLAGS = -O6 -I$(INC) -I$(CI_INC) -I$(LI_INC) -Wall $(VIA_CFLAGS)
CFLAGS = -g -I$(INC) -I$(CI_INC) -I$(LI_INC) -Wall -W -Wbad-function-cast -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes $(VIA_CFLAGS)
# CFLAGS = -pg -I$(INC) -I$(CI_INC) -I$(LI_INC) -Wall $(VIA_CFLAGS)
# CFLAGS = -fomit-frame-pointer -m486 -malign-loops=2 -malign-jumps=2 -malign-functions=2 -O6 -I$(INC) -I$(CI_INC) -I$(LI_INC) -Wall $(VIA_CFLAGS)
# CFLAGS = -Di686 -fexpensive-optimizations -finline-functions -Dpentiumpro -mcpu=pentiumpro -march=pentiumpro -O6 -fomit-frame-pointer -I$(INC) -I$(CI_INC) -I$(LI_INC) -Wall $(VIA_CFLAGS)

# linker flags 
# LDFLAGS = -pg 
LDFLAGS =

# PM2
# ---
ifdef PM2
 CFLAGS += -DPM2 -D$(PM2_ARCH) -D$(PM2_SYS) -I$(MARCEL_ROOT)/include
 LDFLAGS += $(MARCEL_ROOT)/lib/libmarcel.a
endif

# Drivers
# -------
# TCP specific
ifdef DRV_TCP
  OBJS    += $(OBJ)/mad_tcp.o 
  HEADERS += $(CI_INC)/mad_tcp.h
  SRCS    += $(CI_SRC)/mad_tcp.c
  CFLAGS  += -DDRV_TCP=mad_TCP
  ifdef OS_SOLARIS
    LDFLAGS += -lnsl -lsocket
  endif
endif

# M-VIA specific
ifdef DRV_VIA
  OBJS    += $(OBJ)/mad_via.o 
  HEADERS += $(CI_INC)/mad_via.h
  SRCS    += $(CI_SRC)/mad_via.c
  CFLAGS  += -DDRV_VIA=mad_VIA -D_REENTRANT
  LDFLAGS += -lvipl -lpthread
endif

# SISCI specific ;
ifdef DRV_SISCI
  SISCI_PATH = /usr/src/DIS_990816/src/SISCI
  OBJS    += $(OBJ)/mad_sisci.o 
  HEADERS += $(CI_INC)/mad_sisci.h
  SRCS    += $(CI_SRC)/mad_sisci.c
  CFLAGS  += -DDRV_SISCI=mad_SISCI \
             -I$(SISCI_PATH)/api/ \
             -I$(SISCI_PATH)/src/
  LDFLAGS += -L$(SISCI_PATH)/api/ -lsisci
endif

# SBP specific
ifdef DRV_SBP
  OBJS    += $(OBJ)/mad_sbp.o 
  HEADERS += $(CI_INC)/mad_sbp.h
  SRCS    += $(CI_SRC)/mad_sbp.c
  CFLAGS  += -DDRV_SBP=mad_SBP -DEXTERNAL_SPAWN=DRV_SBP
  LDFLAGS += -lded
endif

# MPI specific
ifdef DRV_MPI
  OBJS    += $(OBJ)/mad_mpi.o 
  HEADERS += $(CI_INC)/mad_mpi.h
  SRCS    += $(CI_SRC)/mad_mpi.c
  CFLAGS  += -DDRV_MPI=mad_MPI -DEXTERNAL_SPAWN=DRV_MPI
#  LDFLAGS += 
endif

# Debug mode
# ----------
ifdef DEBUG
  CFLAGS += -DDEBUG
endif

# Trace mode
# ----------
ifdef TRACE
  CFLAGS += -DTRACE
endif

# Timing mode
# ----------
ifdef TIMING
  CFLAGS += -DTIMING
endif

# Rules
# -----
all: lib $(APPS)

lib: $(LIB_MAD2)

ps: 
	enscript -MA4 -1 -C -Ec -j -r -v -H --color --toc $(SRCS) $(HEADERS) -p $(MAD2_ROOT)/mad_2.ps

html: 
	enscript -W html -MA4 -1 -C -Ec -j -r -v -H --color --toc $(SRCS) $(HEADERS) -p $(MAD2_ROOT)/mad_2.html

rtf: 
	enscript -W rtf -MA4 -1 -C -Ec -j -r -v -H --color --toc $(SRCS) $(HEADERS) -p $(MAD2_ROOT)/mad_2.rtf

mad_ping: mad_ping.o $(LIB_MAD2)
	$(CC) $< $(LIB_MAD2) $(LDFLAGS) -o $@

mad_ping.o: mad_ping.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

mad_test: mad_test.o $(LIB_MAD2)
	$(CC) $< $(LIB_MAD2) $(LDFLAGS) -o $@ 

mad_test.o: mad_test.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(LIB_MAD2): $(OBJS)
	ar -rc $(LIB_MAD2) $(OBJS); \
        ranlib $(LIB_MAD2)

clean:
	rm -f $(APPS) $(APPS_OBJS) $(OBJS) $(LIB_MAD2)

clean_src:
	rm -f $(MAD2_ROOT)/*~ $(SRC)/*~ $(CI_SRC)/*~ $(INC)/*~ $(CI_INC)/*~

lines:
	wc -l $(MAD2_ROOT)/*.c \
              $(SRC)/*.c $(CI_SRC)/*.c \
              $(INC)/*.h $(CI_INC)/*.h

$(OBJ)/madeleine.o: $(SRC)/madeleine.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJ)/mad_channel.o: $(SRC)/mad_channel.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJ)/mad_buffers.o: $(SRC)/mad_buffers.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJ)/mad_list_management.o: $(SRC)/mad_list_management.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJ)/mad_memory_management.o: $(SRC)/mad_memory_management.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJ)/mad_communication.o: $(SRC)/mad_communication.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJ)/mad_malloc.o: $(SRC)/mad_malloc.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJ)/mad_timing.o: $(SRC)/mad_timing.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJ)/mad_tcp.o: $(CI_SRC)/mad_tcp.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJ)/mad_via.o: $(CI_SRC)/mad_via.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJ)/mad_sisci.o: $(CI_SRC)/mad_sisci.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJ)/mad_sbp.o: $(CI_SRC)/mad_sbp.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJ)/mad_mpi.o: $(CI_SRC)/mad_mpi.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

#

