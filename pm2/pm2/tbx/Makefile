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
# Makefile : Toolbox
######################

include $(MAD2_ROOT)/make/config.mak

# directories
ROOT  = $(TBX_ROOT)
OBJ   = $(ROOT)/obj
INC   = $(ROOT)/include
SRC   = $(ROOT)/src
LIB   = $(ROOT)/lib

# objects
OBJS = $(OBJ)/tbx_list_management.o \
       $(OBJ)/tbx_malloc.o \
       $(OBJ)/tbx_timing.o \
       $(OBJ)/tbx.o

# srcs
SRCS = $(SRC)/tbx_list_management.c \
       $(SRC)/tbx_malloc.c \
       $(SRC)/tbx_timing.c \
       $(SRC)/tbx.c

# headers
HEADERS = $(INC)/tbx_interface.h \
          $(INC)/tbx_macros.h \
          $(INC)/tbx_malloc.h \
          $(INC)/tbx_list.h \
          $(INC)/tbx_pointers.h \
          $(INC)/tbx_timing.h \
          $(INC)/tbx_types.h \
          $(INC)/tbx.h

# libs
LIB_TBX = $(LIB)/libtbx.a

# Rules
# -----
all: lib 

lib: $(LIB_TBX)

ps: 
	enscript -MA4 -1 -C -Ec -j -r -v -H --color --toc $(SRCS) $(HEADERS) -p $(ROOT)/tbx_2.ps

html: 
	enscript -W html -MA4 -1 -C -Ec -j -r -v -H --color --toc $(SRCS) $(HEADERS) -p $(ROOT)/tbx_2.html

rtf: 
	enscript -W rtf -MA4 -1 -C -Ec -j -r -v -H --color --toc $(SRCS) $(HEADERS) -p $(ROOT)/tbx_2.rtf

$(LIB_TBX): $(OBJS)
	ar -rc $(LIB_TBX) $(OBJS); \
        ranlib $(LIB_TBX)

clean:
	rm -f $(OBJS) $(LIB_TBX)

clean_src:
	rm -f $(ROOT)/*~ $(SRC)/*~ $(INC)/*~ 

lines:
	wc -l $(SRC)/*.c $(INC)/*.h

$(OBJ)/tbx.o: $(SRC)/tbx.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJ)/tbx_list_management.o: $(SRC)/tbx_list_management.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJ)/tbx_malloc.o: $(SRC)/tbx_malloc.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJ)/tbx_timing.o: $(SRC)/tbx_timing.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

#

