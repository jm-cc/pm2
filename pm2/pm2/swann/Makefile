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
# Makefile : Swann
###################

include $(MAD2_ROOT)/make/config.mak

# directories
ROOT = ${SWN_ROOT}
OBJ = $(ROOT)/obj
INC = $(ROOT)/include
SRC = $(ROOT)/src
BIN = $(ROOT)/bin

# applications
APPS = $(BIN)/swann

# objects
OBJS = $(OBJ)/swann.o

# srcs
SRCS = $(SRC)/swann.c

# headers
HEADERS = $(INC)/swann.h

# ToolBox
# -------
LDFLAGS += $(TBX_ROOT)/lib/libtbx.a

# Net ToolBox
# -----------
LDFLAGS += $(NTBX_ROOT)/lib/libntbx.a

# dependencies
DEP_DIRS = $(TBX_ROOT) $(NTBX_ROOT)

# Rules
# -----
.PHONY: all ps html rtf clean clean_src lines dep_dirs $(DEP_DIRS) conf

all: dep_dirs $(APPS) conf

ps: 
	enscript -MA4 -1 -C -Ec -j -r -v -H --color --toc $(SRCS) $(HEADERS) -p $(ROOT)/swann.ps

html: 
	enscript -W html -MA4 -1 -C -Ec -j -r -v -H --color --toc $(SRCS) $(HEADERS) -p $(ROOT)/swann.html

rtf: 
	enscript -W rtf -MA4 -1 -C -Ec -j -r -v -H --color --toc $(SRCS) $(HEADERS) -p $(ROOT)/swann.rtf

$(BIN)/swann: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@ 

clean: dep_dirs
	rm -f $(APPS) $(APPS_OBJS) $(OBJS) $(LIB_SWA) 

clean_src:
	rm -f $(ROOT)/*~ $(SRC)/*~ $(INC)/*~ $(LY)/*~

lines:
	wc -l $(SRC)/*.c $(INC)/*.h 

$(OBJ)/swann.o: $(SRC)/swann.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

dep_dirs: $(DEP_DIRS)

$(DEP_DIRS):
	$(MAKE) -C $@ $(DEP_GOAL)

conf: $(CONF)

#---------------------------------------------------------------------
