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


include $(PM2_ROOT)/make/libs-vars.mak

ifeq (,$(findstring _$(MAKECMDGOALS)_,_clean_ $(DO_NOT_GENERATE_MAK_FILES)))
-include $(PM2_MAK_DIR)/profile-specific.mak
endif

# The profile.c file includes fut_entries.h, but it cannot be actually
# detected by make depend since there would be a circular dependency
# problem...
ifneq ($(MAKECMDGOALS),fut)
$(LIB_GEN_OBJ)/profile.o: $(LIB_GEN_INC)/fut_entries.h
endif

ifneq ($(LIB_GEN_BIN),)
DUMMY_BUILD := $(shell mkdir -p $(LIB_GEN_BIN))
endif

.PHONY: default libs fut_files

libs: $(LIB_GEN_INC)/fut_entries.h $(LIB_GEN_BIN)/fut_print

$(LIB_GEN_INC)/fut_entries.h: $(LIB_GEN_STAMP)/fut_stamp
	$(LIB_PREFIX) ${PM2_ROOT}/bin/pm2-build-fut-table -f $(FLAVOR)

$(LIB_GEN_BIN)/fut_print: $(LIB_GEN_INC)/fut_entries.h

$(LIB_GEN_BIN)/fut_print: ./bin/src/fut_print.c
	$(LIB_PREFIX) $(CC) -g -o $@ -I./include -I${LIB_GEN_INC} $<

$(LIB_GEN_STAMP)/fut_stamp: $(LIB_STAMP_FLAVOR)
	$(COMMON_HIDE) set -e; \
	if [ ! -d $(LIB_GEN_STAMP) ] ; then mkdir -p $(LIB_GEN_STAMP) ; fi
	$(COMMON_HIDE) touch $@

# Nothing can be done at 'do_h generation time'
dot_h: ;

clean:
	@echo "THIS HAVE TO BE FIXED. NOTHING DONE TO CLEAN FOR NOW."
	@echo "THIS HAVE TO BE FIXED. NOTHING DONE TO CLEAN FOR NOW."
	@echo "THIS HAVE TO BE FIXED. NOTHING DONE TO CLEAN FOR NOW."
	@echo "THIS HAVE TO BE FIXED. NOTHING DONE TO CLEAN FOR NOW."

$(PM2_MAK_DIR)/profile-specific.mak: $(LIB_STAMP_FLAVOR)
	@_PM2_OUTPUT=`$(PM2_CONFIG) --includedir $(LIBRARY)`; \
	if [ -n "$$_PM2_OUTPUT" ]; then mkdir -p `dirname $@`; \
		echo "LIB_GEN_INC := $$_PM2_OUTPUT" > $@ ;\
	fi;

include $(PM2_ROOT)/make/libs-rules.mak
