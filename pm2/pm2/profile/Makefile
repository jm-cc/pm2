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

LIBRARY=profile
#LIBNAME=profile

#marcel_CC=gcc
#marcel_AS=gcc

ifdef APP_RECURSIF

include $(PM2_ROOT)/make/config.mak
include $(PM2_ROOT)/make/libs-vars.mak

-include $(PM2_MAK_DIR)/profile-specific.mak

ifneq ($(MAKECMDGOALS),fut)
$(LIB_GEN_OBJ)/profile.o: $(LIB_GEN_INC)/fut_entries.h
endif

include $(PM2_ROOT)/make/libs-rules.mak

$(PM2_MAK_DIR)/profile-specific.mak: $(LIB_STAMP_FLAVOR)
	@_PM2_OUTPUT=`$(PM2_CONFIG) --includedir $(LIBRARY)`; \
	if [ -n "$$_PM2_OUTPUT" ]; then mkdir -p `dirname $@`; \
	echo "LIB_GEN_INC := $$_PM2_OUTPUT" > $@ ;\
	fi;

else # !APP_RECURSIF

default:

include $(PM2_ROOT)/make/config.mak
include $(PM2_ROOT)/make/libs-vars.mak

# Target subdirectories
#DUMMY_BUILD :=  $(shell mkdir -p $(PM2_MAK_DIR))

-include $(PM2_MAK_DIR)/profile-specific.mak

# Target subdirectories
#DUMMY_BUILD :=  $(shell mkdir -p $(LIB_GEN_INC))

.PHONY: default fut_files lib fut
default: fut_files $(LIB_GEN_INC)/fut_entries.h lib

.PHONY: fut_files
fut_files:
	$(COMMON_HIDE) $(MAKE) -C ${PM2_ROOT} fut

$(LIB_GEN_INC)/fut_entries.h: $(LIB_GEN_STAMP)/fut_stamp
	$(LIB_PREFIX) ${PM2_ROOT}/bin/pm2-build-fut-table -f $(FLAVOR)

$(LIB_GEN_STAMP)/fut_stamp: ;

lib:
	$(COMMON_HIDE) $(MAKE) APP_RECURSIF=true

fut:
	$(COMMON_HIDE) $(MAKE) APP_RECURSIF=true $@

$(PM2_MAK_DIR)/profile-specific.mak: $(LIB_STAMP_FLAVOR)
	@_PM2_OUTPUT=`$(PM2_CONFIG) --includedir $(LIBRARY)`; \
	if [ -n "$$_PM2_OUTPUT" ]; then mkdir -p `dirname $@`; \
	echo "LIB_GEN_INC := $$_PM2_OUTPUT" > $@ ;\
	fi;

endif