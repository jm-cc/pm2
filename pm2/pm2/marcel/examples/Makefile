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

SRC_DIR     :=  ./
APP_CFLAGS  :=
APP_LDFLAGS :=
APP_LLIBS   :=

include ../Makefile

all:	$(APP_BIN)/active	\
	$(APP_BIN)/cleanup	\
	$(APP_BIN)/contsw	\
	$(APP_BIN)/create	\
	$(APP_BIN)/daube	\
	$(APP_BIN)/delay	\
	$(APP_BIN)/derout	\
	$(APP_BIN)/eclone	\
	$(APP_BIN)/force_raise	\
	$(APP_BIN)/kbd		\
	$(APP_BIN)/keys		\
	$(APP_BIN)/matrix	\
	$(APP_BIN)/once		\
	$(APP_BIN)/poll		\
	$(APP_BIN)/raise	\
	$(APP_BIN)/sample	\
	$(APP_BIN)/stack_jump	\
	$(APP_BIN)/sumtime	\
	$(APP_BIN)/timeout	\
	$(APP_BIN)/tsp		\
	$(APP_BIN)/tube		\
	$(APP_BIN)/usersched	\
	$(APP_BIN)/userspace


$(APP_BIN)/eclone: $(APP_OBJ)/clone.o

$(APP_OBJ)/eclone.o: $(SRC_DIR)/eclone.c
	$(MARCEL_CC) $(MARCEL_CFLAGS) -O0 -c $< -o $@


$(APP_BIN)/tsp: $(APP_OBJ)/tsp-main.o $(APP_OBJ)/tsp-job.o

$(APP_OBJ)/tsp.o: $(SRC_DIR)/tsp.c
	$(MARCEL_CC) $(MARCEL_CFLAGS) -DMARCEL -DMT -c $< -o $@

$(APP_OBJ)/tsp-main.o: $(SRC_DIR)/tsp-main.c
	$(MARCEL_CC) $(MARCEL_CFLAGS) -DMARCEL -DMT -c $< -o $@

$(APP_OBJ)/tsp-job.o: $(SRC_DIR)/tsp-job.c
	$(MARCEL_CC) $(MARCEL_CFLAGS) -DMARCEL -DMT -c $< -o $@
