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

LIBRARY=mad2
LIBNAME=mad

#marcel_CC=gcc
#marcel_AS=gcc

include $(PM2_ROOT)/make/config.mak
include $(PM2_ROOT)/make/libs-vars.mak

-include $(PM2_MAK_DIR)/mad2-specific.mak

LIB_SRC2=$(LIB_SRC)/connection_interfaces
LIB_C_SOURCES_ADDED := $(foreach PROT, $(MAD2_PROTOCOLS), $(LIB_SRC2)/mad_$(PROT).c)
LIB_C_OBJECTS_ADDED := $(patsubst %.c,%$(LIB_EXT).o,$(subst $(LIB_SRC2),$(LIB_GEN_OBJ),$(LIB_C_SOURCES_ADDED)))
LIB_C_PICS_ADDED := $(patsubst %.c,%$(LIB_EXT).pic,$(subst $(LIB_SRC2),$(LIB_GEN_OBJ),$(LIB_C_SOURCES_ADDED)))
LIB_C_DEPENDS_ADDED := $(patsubst %.o,%.d,$(subst $(LIB_GEN_OBJ),$(LIB_GEN_DEP),$(LIB_C_OBJECTS_ADDED)))
LIB_C_PREPROC_ADDED := $(patsubst %.c,%$(LIB_EXT).i,$(subst $(LIB_SRC2),$(LIB_GEN_CPP),$(LIB_C_SOURCES_ADDED)))
LIB_C_FUT_ADDED :=  $(patsubst %.i,%.fut,$(LIB_C_PREPROC_ADDED))
LIB_OBJECTS += $(LIB_C_OBJECTS_ADDED)
LIB_PICS += $(LIB_C_PICS_ADDED)
LIB_PREPROC += $(LIB_C_PREPROC_ADDED)
LIB_FUT += $(LIB_C_FUT_ADDED)
include $(PM2_ROOT)/make/libs-rules.mak

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),distclean)
ifneq ($(wildcard $(LIB_C_DEPENDS_ADDED)),)
-include $(LIB_C_DEPENDS_ADDED)
endif
endif
endif

$(LIB_C_OBJECTS_ADDED): $(LIB_GEN_OBJ)/%$(LIB_EXT).o: $(LIB_SRC2)/%.c
	$(LIB_PREFIX) $(CC) $(CFLAGS) -c $< -o $@

$(LIB_C_PICS_ADDED): $(LIB_GEN_OBJ)/%$(LIB_EXT).pic: $(LIB_SRC2)/%.c
	$(LIB_PREFIX) $(CC) $(CFLAGS) -fPIC -c $< -o $@

$(LIB_C_DEPENDS_ADDED): $(LIB_GEN_DEP)/%$(LIB_EXT).d: $(LIB_SRC2)/%.c
	$(LIB_PREFIX) $(SHELL) -ec '$(CC) -MM $(CFLAGS) -DDEPEND $< \
		| sed '\''s/.*:/$(subst /,\/,$(LIB_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

$(LIB_C_PREPROC_ADDED): $(LIB_GEN_CPP)/%$(LIB_EXT).i: $(LIB_SRC2)/%.c
	$(LIB_PREFIX) $(CC) -E -P -DPREPROC $(CFLAGS) $< > $@

$(PM2_MAK_DIR)/mad2-specific.mak: $(LIB_STAMP_FLAVOR)
	@_PM2_OUTPUT=`$(PM2_CONFIG) --protocols $(LIBRARY)`;\
	if [ -n "$$_PM2_OUTPUT" ]; then mkdir -p `dirname $@`; \
	echo "MAD2_PROTOCOLS := $$_PM2_OUTPUT" > $@ ;\
	fi;

