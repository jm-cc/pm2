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


include ../make/common.mak

default: all

all: simple token ping multiping


# simple :

$(MAD_OBJ)/simple.o: simple.c $(MAD_HEADERS)
	$(MAD_CC2)

simple: $(MAD_OBJ)/simple.o $(MAD_LIB)
	$(MAD_LINK) $(MAD_LLIB)
	$(MAD_MOVE)


# token :

$(MAD_OBJ)/token.o: token.c $(MAD_HEADERS)
	$(MAD_CC2)

token: $(MAD_OBJ)/token.o $(MAD_LIB)
	$(MAD_LINK) $(MAD_LLIB)
	$(MAD_MOVE)


# pong :

$(MAD_OBJ)/pong.o: pong.c $(MAD_HEADERS)
	$(MAD_CC2)

pong: $(MAD_OBJ)/pong.o $(MAD_LIB)
	$(MAD_LINK) $(MAD_LLIB)
	$(MAD_MOVE)


# multiping :

$(MAD_OBJ)/multiping.o: multiping.c $(MAD_HEADERS)
	$(MAD_CC2)

multiping: $(MAD_OBJ)/multiping.o $(MAD_LIB)
	$(MAD_LINK) $(MAD_LLIB)
	$(MAD_MOVE)

