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

LIBRARY=mad1
LIBNAME=mad

#marcel_CC=gcc
#marcel_AS=gcc

include $(PM2_ROOT)/make/config.mak
include $(PM2_ROOT)/make/libs-vars.mak

LIB_PROT := $(shell $(PM2_CONFIG) --protocols $(LIBRARY))
LIB_C_SOURCES_ADDED := $(LIB_SRC)/$(LIB_PROT)/netinterf.c
LIB_C_OBJECTS_ADDED := $(LIB_GEN_OBJ)/netinterf$(LIB_EXT).o
LIB_C_PICS_ADDED := $(LIB_GEN_OBJ)/netinterf$(LIB_EXT).pic
LIB_C_DEPENDS_ADDED := $(LIB_GEN_DEP)/netinterf$(LIB_EXT).d
LIB_OBJECTS += $(LIB_C_OBJECTS_ADDED)
LIB_PICS += $(LIB_C_PICS_ADDED)

include $(PM2_ROOT)/make/libs-rules.mak

$(LIB_C_OBJECTS_ADDED): $(LIB_GEN_OBJ)/%$(LIB_EXT).o: $(LIB_SRC)/$(LIB_PROT)/%.c
	$(LIB_PREFIX) $(CC) $(CFLAGS) -c $< -o $@

$(LIB_C_PICS_ADDED): $(LIB_GEN_OBJ)/%$(LIB_EXT).pic: $(LIB_SRC)/$(LIB_PROT)/%.c
	$(LIB_PREFIX) $(CC) $(CFLAGS) -fPIC -c $< -o $@

$(LIB_C_DEPENDS_ADDED): $(LIB_GEN_DEP)/%$(LIB_EXT).d: $(LIB_SRC)/$(LIB_PROT)/%.c
	$(LIB_PREFIX) $(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(LIB_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'



