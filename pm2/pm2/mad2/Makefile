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

# LIBRARY -> nom du module
#---------------------------------------------------------------------
LIBRARY=mad2

# LIBNAME -> nom de la librairie
#---------------------------------------------------------------------
LIBNAME=mad

# Inclusion des makefiles generiques
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/libs-vars.mak

# Inclusion du cache specifique a madeleine
#---------------------------------------------------------------------
ifeq (,$(findstring _$(MAKECMDGOALS)_,_clean_ $(DO_NOT_GENERATE_MAK_FILES)))
-include $(PM2_MAK_DIR)/mad2-specific.mak
endif

# Gestion des drivers : prise en compte des fichiers
#---------------------------------------------------------------------
LIB_SRC2=$(LIB_SRC)/connection_interfaces

LIB_C_SOURCES_ADDED := $(foreach PROT, $(MAD2_PROTOCOLS), $(LIB_SRC2)/mad_$(PROT).c)
LIB_C_PREPROC_ADDED := $(foreach PROT, $(MAD2_PROTOCOLS), $(LIB_GEN_CPP)/mad_$(PROT)$(LIB_EXT).i)
LIB_C_FUT_ADDED     := $(foreach PROT, $(MAD2_PROTOCOLS), $(LIB_GEN_CPP)/mad_$(PROT)$(LIB_EXT).fut)
LIB_C_DEPENDS_ADDED := $(foreach PROT, $(MAD2_PROTOCOLS), $(LIB_GEN_DEP)/mad_$(PROT)$(LIB_EXT).d)
LIB_C_OBJECTS_ADDED := $(foreach PROT, $(MAD2_PROTOCOLS), $(LIB_GEN_OBJ)/mad_$(PROT)$(LIB_EXT).o)
LIB_C_PICS_ADDED    := $(foreach PROT, $(MAD2_PROTOCOLS), $(LIB_GEN_OBJ)/mad_$(PROT)$(LIB_EXT).pic)

LIB_OBJECTS += $(LIB_C_OBJECTS_ADDED)
LIB_PICS    += $(LIB_C_PICS_ADDED)
LIB_PREPROC += $(LIB_C_PREPROC_ADDED)
LIB_FUT     += $(LIB_C_FUT_ADDED)

# Regles communes aux librairies
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/libs-rules.mak

# Inclusion du makefile de dependance des drivers
#---------------------------------------------------------------------
ifeq (,$(findstring _$(MAKECMDGOALS)_,_clean_ $(DO_NOT_GENERATE_MAK_FILES)))
ifneq ($(wildcard $(LIB_C_DEPENDS_ADDED)),)
-include $(LIB_C_DEPENDS_ADDED)
endif
endif

# Construction des drivers
#---------------------------------------------------------------------
$(LIB_C_OBJECTS_ADDED): $(LIB_GEN_OBJ)/%$(LIB_EXT).o: $(LIB_SRC2)/%.c
	$(LIB_PREFIX) $(CC) $(CFLAGS) -c $< -o $@

$(LIB_C_PICS_ADDED): $(LIB_GEN_OBJ)/%$(LIB_EXT).pic: $(LIB_SRC2)/%.c
	$(LIB_PREFIX) $(CC) $(CFLAGS) -fPIC -c $< -o $@

# -> le separateur du sed est `|'
# Note: interet de l'ajout de $@ comme dependance ?
$(LIB_C_DEPENDS_ADDED): $(LIB_GEN_DEP)/%$(LIB_EXT).d: $(LIB_SRC2)/%.c
	$(LIB_PREFIX) $(SHELL) -ec '$(CC) -MM $(CFLAGS) -DDEPEND $< \
		| sed '\''s|.*:|$(LIB_DEP_TO_OBJ) $@ :|g'\'' > $@'

$(LIB_C_PREPROC_ADDED): $(LIB_GEN_CPP)/%$(LIB_EXT).i: $(LIB_SRC2)/%.c
	$(LIB_PREFIX) $(CC) -E -P -DPREPROC $(CFLAGS) $< > $@

# Construction du cache specifique a Madeleine II
#---------------------------------------------------------------------
$(PM2_MAK_DIR)/mad2-specific.mak: $(LIB_STAMP_FLAVOR)
	@_PM2_OUTPUT=`$(PM2_CONFIG) --protocols $(LIBRARY)`;\
	if [ -n "$$_PM2_OUTPUT" ]; then mkdir -p `dirname $@`; \
	echo "MAD2_PROTOCOLS := $$_PM2_OUTPUT" > $@ ;\
	fi;

