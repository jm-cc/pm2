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
# Makefile : Net Toolbox
#########################

include $(MAD2_ROOT)/make/config.mak

# directories
ROOT   = $(NTBX_ROOT)
OBJ = $(ROOT)/obj
INC = $(ROOT)/include
SRC = $(ROOT)/src
LIB = $(ROOT)/lib

# objects
OBJS = $(OBJ)/ntbx_pack.o \
       $(OBJ)/ntbx.o

# srcs
SRCS = $(OBJ)/ntbx_pack.c \
       $(SRC)/ntbx.c

# headers
HEADERS = $(INC)/ntbx_interface.h \
          $(INC)/ntbx_types.h \
          $(INC)/ntbx.h 

# libs
LIB_NTBX = $(LIB)/libntbx.a

# config
ifdef NTBX_TCP
 OBJS    += $(OBJ)/ntbx_tcp.o
 SRCS    += $(SRC)/ntbx_tcp.c
 HEADERS += $(INC)/ntbx_tcp.h
endif

# Rules
# -----
all: lib 

lib: $(LIB_NTBX)

ps: 
	enscript -MA4 -1 -C -Ec -j -r -v -H --color --toc $(SRCS) $(HEADERS) -p $(ROOT)/ntbx.ps

html: 
	enscript -W html -MA4 -1 -C -Ec -j -r -v -H --color --toc $(SRCS) $(HEADERS) -p $(ROOT)/ntbx.html

rtf: 
	enscript -W rtf -MA4 -1 -C -Ec -j -r -v -H --color --toc $(SRCS) $(HEADERS) -p $(ROOT)/ntbx.rtf

$(LIB_NTBX): $(OBJS)
	ar -rc $(LIB_NTBX) $(OBJS); \
        ranlib $(LIB_NTBX)

clean:
	rm -f $(OBJS) $(LIB_NTBX)

clean_src:
	rm -f $(ROOT)/*~ $(SRC)/*~ $(INC)/*~ 

lines:
	wc -l $(SRC)/*.c $(INC)/*.h

$(OBJ)/ntbx.o: $(SRC)/ntbx.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJ)/ntbx_pack.o: $(SRC)/ntbx_pack.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJ)/ntbx_tcp.o: $(SRC)/ntbx_tcp.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@


#

