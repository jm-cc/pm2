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

CONFIG_MODULES=marcel

ifndef FLAVOR
export FLAVOR:=marcel
endif

PROGS := active	cleanup	contsw create daube delay derout eclone force_raise \
	kbd keys matrix once poll raise sample stack_jump sumtime timeout \
	tsp tube usersched userspace

include $(PM2_ROOT)/make/config.mak
include $(PM2_ROOT)/make/apps.mak


ifdef APP_RECURSIF


$(APP_BIN)/eclone$(APP_EXT): $(APP_OBJ)/clone$(APP_EXT).o

$(APP_OBJ)/eclone$(APP_EXT).o: $(SRC_DIR)/eclone.c
	$(COMMON_PREFIX) $(CC) $(CFLAGS) -O0 -c $< -o $@

$(APP_BIN)/tsp$(APP_EXT): $(APP_OBJ)/tsp-main$(APP_EXT).o $(APP_OBJ)/tsp-job$(APP_EXT).o -lm

$(APP_OBJ)/tsp$(APP_EXT).o: $(SRC_DIR)/tsp.c
	$(COMMON_PREFIX) $(CC) $(CFLAGS) -DMT -c $< -o $@

$(APP_OBJ)/tsp-main$(APP_EXT).o: $(SRC_DIR)/tsp-main.c
	$(COMMON_PREFIX) $(CC) $(CFLAGS) -DMT -c $< -o $@

$(APP_OBJ)/tsp-job$(APP_EXT).o: $(SRC_DIR)/tsp-job.c
	$(COMMON_PREFIX) $(CC) $(CFLAGS) -DMT -c $< -o $@

endif

