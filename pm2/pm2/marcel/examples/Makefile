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

.PHONY: default all
default: all

MAR_EX_DIR	:=	.

include ../Makefile

all:	$(BIN_DIR)/active	\
	$(BIN_DIR)/cleanup	\
	$(BIN_DIR)/contsw	\
	$(BIN_DIR)/create	\
	$(BIN_DIR)/daube	\
	$(BIN_DIR)/delay	\
	$(BIN_DIR)/derout	\
	$(BIN_DIR)/eclone	\
	$(BIN_DIR)/force_raise	\
	$(BIN_DIR)/kbd		\
	$(BIN_DIR)/keys		\
	$(BIN_DIR)/matrix	\
	$(BIN_DIR)/once		\
	$(BIN_DIR)/poll		\
	$(BIN_DIR)/raise	\
	$(BIN_DIR)/sample	\
	$(BIN_DIR)/stack_jump	\
	$(BIN_DIR)/sumtime	\
	$(BIN_DIR)/timeout	\
	$(BIN_DIR)/tsp		\
	$(BIN_DIR)/tube		\
	$(BIN_DIR)/usersched	\
	$(BIN_DIR)/userspace


$(BIN_DIR)/eclone: $(OBJ_DIR)/clone.o

$(OBJ_DIR)/eclone.o: $(SRC_DIR)/eclone.c
	$(MAR_CC) $(MAR_CFLAGS) -O0 -c $< -o $@


$(BIN_DIR)/tsp: $(OBJ_DIR)/tsp-main.o $(OBJ_DIR)/tsp-job.o

$(OBJ_DIR)/tsp.o: $(SRC_DIR)/tsp.c
	$(MAR_CC) $(MAR_CFLAGS) -DMARCEL -DMT -c $< -o $@

$(OBJ_DIR)/tsp-main.o: $(SRC_DIR)/tsp-main.c
	$(MAR_CC) $(MAR_CFLAGS) -DMARCEL -DMT -c $< -o $@

$(OBJ_DIR)/tsp-job.o: $(SRC_DIR)/tsp-job.c
	$(MAR_CC) $(MAR_CFLAGS) -DMARCEL -DMT -c $< -o $@
