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

#..Private.part..............................................................#

# directories
LEO_ROOT = ${MAD2_ROOT}/../leonie
OBJ = $(LEO_ROOT)/obj
INC = $(LEO_ROOT)/include
SRC = $(LEO_ROOT)/src
BIN = $(LEO_ROOT)/bin
LY  = ${LEO_ROOT}/lex_yacc

# applications
APPS = $(BIN)/leonie

# objects
OBJS = $(OBJ)/leonie.o \
       $(OBJ)/leo_parser.o  \
       $(OBJ)/leo_lexer.o 

# srcs
SRCS = $(SRC)/leonie.c \
       $(LY)/leo_lexer.l \
       $(LY)/leo_parser.y

# headers
HEADERS = $(INC)/leonie.h \
          $(INC)/leo_parser_types.h

# compiler
# CC = purify gcc
CC = gcc
# CC = egcc

# preprocessor flags
CPPFLAGS = -I$(INC) 

# compiler flags
# CFLAGS = -O6 -Wall 
# CFLAGS = -g
CFLAGS = -g -Wall -W -Wbad-function-cast -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes
# CFLAGS = -pg -Wall
# CFLAGS = -fomit-frame-pointer -m486 -malign-loops=2 -malign-jumps=2 -malign-functions=2 -O6 -Wall
# CFLAGS = -Di686 -fexpensive-optimizations -finline-functions -Dpentiumpro -mcpu=pentiumpro -march=pentiumpro -O6 -fomit-frame-pointer -Wall

# linker flags 
# LDFLAGS = -pg -ll -ly
#LDFLAGS = -ll -ly
LDFLAGS = -lfl

# LEX = lex
LEX = flex

# YACC = yacc
YACC = bison -y

# ToolBox
# -------
TBX_ROOT  = $(MAD2_ROOT)/../toolbox
CPPFLAGS += -I$(TBX_ROOT)/include
LDFLAGS += $(TBX_ROOT)/lib/libtbx.a

# PM2
# ---
ifdef PM2
 CPPFLAGS += -DPM2 -D$(PM2_ARCH) -D$(PM2_SYS) 
# CFLAGS  += 
# LDFLAGS += 
endif

# Debug mode
# ----------
ifdef DEBUG
  CPPFLAGS += -DDEBUG
endif

# Trace mode
# ----------
ifdef TRACE
  CPPFLAGS += -DTRACE
endif

# Timing mode
# ----------
ifdef TIMING
  CPPFLAGS += -DTIMING
endif

# dependencies
DEP_DIRS = $(TBX_ROOT)
DEP_LIBS = $(TBX_ROOT)/lib

# goals transfer
ifeq ($(MAKECMDGOALS), clean)
DEP_GOAL = clean
else
DEP_GOAL = all
endif

# Rules
# -----
.PHONY: all ps html rtf clean clean_src lines dep_dirs $(DEP_DIRS) conf

all: dep_dirs $(APPS) conf

ps: 
	enscript -MA4 -1 -C -Ec -j -r -v -H --color --toc $(SRCS) $(HEADERS) -p $(LEO_ROOT)/leonie.ps

html: 
	enscript -W html -MA4 -1 -C -Ec -j -r -v -H --color --toc $(SRCS) $(HEADERS) -p $(LEO_ROOT)/leonie.html

rtf: 
	enscript -W rtf -MA4 -1 -C -Ec -j -r -v -H --color --toc $(SRCS) $(HEADERS) -p $(LEO_ROOT)/leonie.rtf

$(BIN)/leonie: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@ 

clean: dep_dirs
	rm -f $(APPS) $(APPS_OBJS) $(OBJS) $(LIB_LEO) $(LY)/y.tab.h
	rm -f $(LY)/y.tab.h $(LY)/y.tab.c $(LY)/lex.yy.c

clean_src:
	rm -f $(LEO_ROOT)/*~ $(SRC)/*~ $(INC)/*~ $(LY)/*~

lines:
	wc -l $(SRC)/*.c $(LY)/*.y $(LY)/*.l $(INC)/*.h 

$(OBJ)/leonie.o: $(SRC)/leonie.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJ)/leo_parser.o: $(LY)/y.tab.c $(LY)/y.tab.h $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJ)/leo_lexer.o: $(LY)/lex.yy.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(LY)/y.tab.h $(LY)/y.tab.c: $(LY)/leo_parser.y
	TMPDIR=`pwd`; cd $(LY); $(YACC) -d $(LY)/leo_parser.y; cd $(TMPDIR) 

$(LY)/lex.yy.c: $(LY)/leo_lexer.l $(LY)/y.tab.h 
	TMPDIR=`pwd`; cd $(LY); $(LEX) $(LY)/leo_lexer.l; cd $(TMPDIR) 

dep_dirs: $(DEP_DIRS)

$(DEP_DIRS):
	$(MAKE) -C $@ $(DEP_GOAL)

conf: $(CONF)

#---------------------------------------------------------------------
