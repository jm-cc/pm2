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
LIBRARY:=leoparse

# LIBNAME -> nom de la librairie
#---------------------------------------------------------------------
LIBNAME:=leoparse

# Regle par defaut
#---------------------------------------------------------------------
.PHONY: default
default:

# Lex & Yacc
#---------------------------------------------------------------------
LEX  :=  flex
YACC :=  bison -y -d

# Inclusion des makefiles generiques
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/config.mak
include $(PM2_ROOT)/make/libs-vars.mak

# Yacc
#---------------------------------------------------------------------
LIB_Y_SOURCES := $(wildcard $(LIB_SRC)/tools/*.y)
LIB_Y_BASE    := $(foreach I, $(LIB_Y_SOURCES), $(notdir $(basename $I)))
LIB_Y_C_SRCS  := $(foreach I, $(LIB_Y_BASE), $(LIB_GEN_SRC)/$I$(LIB_EXT).c)
LIB_Y_OBJECTS := $(foreach I, $(LIB_Y_BASE), $(LIB_GEN_OBJ)/$I$(LIB_EXT).o)
LIB_Y_PICS    := $(foreach I, $(LIB_Y_BASE), $(LIB_GEN_OBJ)/$I$(LIB_EXT).pic)
LIB_Y_PREPROC := $(foreach I, $(LIB_Y_BASE), $(LIB_GEN_CPP)/$I$(LIB_EXT).i)
LIB_Y_FUT     := $(foreach I, $(LIB_Y_BASE), $(LIB_GEN_CPP)/$I$(LIB_EXT).fut)
LIB_Y_DEPENDS := $(foreach I, $(LIB_Y_BASE), $(LIB_GEN_DEP)/$I$(LIB_EXT).d)
LIB_Y_INCS    := $(foreach I, $(LIB_Y_BASE), $(LIB_GEN_INC)/$I$(LIB_EXT).h)

# Lex
#---------------------------------------------------------------------
LIB_L_SOURCES := $(wildcard $(LIB_SRC)/tools/*.l)
LIB_L_BASE    := $(foreach I, $(LIB_L_SOURCES), $(notdir $(basename $I)))
LIB_L_C_SRCS  := $(foreach I, $(LIB_L_BASE), $(LIB_GEN_SRC)/$I$(LIB_EXT).c)
LIB_L_OBJECTS := $(foreach I, $(LIB_L_BASE), $(LIB_GEN_OBJ)/$I$(LIB_EXT).o)
LIB_L_PICS    := $(foreach I, $(LIB_L_BASE), $(LIB_GEN_OBJ)/$I$(LIB_EXT).pic)
LIB_L_PREPROC := $(foreach I, $(LIB_L_BASE), $(LIB_GEN_CPP)/$I$(LIB_EXT).i)
LIB_L_FUT     := $(foreach I, $(LIB_L_BASE), $(LIB_GEN_CPP)/$I$(LIB_EXT).fut)
LIB_L_DEPENDS := $(foreach I, $(LIB_L_BASE), $(LIB_GEN_DEP)/$I$(LIB_EXT).d)

# Ajouts
#---------------------------------------------------------------------
LIB_OBJECTS += $(LIB_Y_OBJECTS) $(LIB_L_OBJECTS)
LIB_PICS    += $(LIB_Y_PICS)    $(LIB_L_PICS)
LIB_PREPROC += $(LIB_Y_PREPROC) $(LIB_L_PREPROC)
LIB_FUT     += $(LIB_Y_FUT)     $(LIB_L_FUT)

# Regles communes aux librairies
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/libs-rules.mak

# Inclusion du makefile de dependance des drivers
#---------------------------------------------------------------------
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),distclean)
ifneq ($(wildcard $(LIB_Y_DEPENDS)),)
-include $(LIB_Y_DEPENDS)
endif
ifneq ($(wildcard $(LIB_L_DEPENDS)),)
-include $(LIB_L_DEPENDS)
endif
endif
endif

# Regle par defaut: make dans le repertoire de PM2
#---------------------------------------------------------------------
default:
	$(MAKE) -C $(PM2_ROOT)

# Dependence .d -> .c
#---------------------------------------------------------------------
$(LIB_DEPENDS): $(LIB_Y_C_SRCS) $(LIB_L_C_SRCS)

# Dependence .h -> .c
#---------------------------------------------------------------------
$(LIB_Y_INCS): $(LIB_Y_C_SRCS)

# Construction des .c et .h a partir des .y
#---------------------------------------------------------------------
$(LIB_Y_C_SRCS): $(LIB_GEN_SRC)/%$(LIB_EXT).c: $(LIB_SRC)/tools/%.y
	echo $(YACC) -b $(LIB_GEN_TMP)/y $<
	$(LIB_PREFIX) $(YACC) -b $(LIB_GEN_TMP)/y $<
	echo mv $(LIB_GEN_TMP)/y.tab.c $@
	mv $(LIB_GEN_TMP)/y.tab.c $@
	echo mv $(LIB_GEN_TMP)/y.tab.h $(LIB_GEN_INC)/$(notdir $(basename $@)).h
	mv $(LIB_GEN_TMP)/y.tab.h $(LIB_GEN_INC)/$(notdir $(basename $@)).h

# Construction des .c a partir des .l
#---------------------------------------------------------------------
$(LIB_L_C_SRCS): $(LIB_GEN_SRC)/%$(LIB_EXT).c: $(LIB_SRC)/tools/%.l
	$(LIB_PREFIX) $(LEX) -t $< >$@

# Constructions .l
#---------------------------------------------------------------------
$(LIB_L_OBJECTS): $(LIB_GEN_OBJ)/%$(LIB_EXT).o: $(LIB_GEN_SRC)/%.c
	$(LIB_PREFIX) $(CC) $(CFLAGS) -c $< -o $@

$(LIB_L_PICS): $(LIB_GEN_OBJ)/%$(LIB_EXT).pic: $(LIB_GEN_SRC)/%.c
	$(LIB_PREFIX) $(CC) $(CFLAGS) -fPIC -c $< -o $@

# -> le separateur du sed est `|'
# Note: interet de l'ajout de $@ comme dependance ?
$(LIB_L_DEPENDS): $(LIB_GEN_DEP)/%$(LIB_EXT).d: $(LIB_GEN_SRC)/%.c
	$(LIB_PREFIX) $(SHELL) -ec '$(CC) -MM $(CFLAGS) -DDEPEND $< \
		| sed '\''s|.*:|$(LIB_DEP_TO_OBJ) $@ :|g'\'' > $@'

$(LIB_L_PREPROC): $(LIB_GEN_CPP)/%$(LIB_EXT).i: $(LIB_GEN_SRC)/%.c
	$(LIB_PREFIX) $(CC) -E -P -DPREPROC $(CFLAGS) $< > $@

# Constructions .y
#---------------------------------------------------------------------
$(LIB_Y_OBJECTS): $(LIB_GEN_OBJ)/%$(LIB_EXT).o: $(LIB_GEN_SRC)/%.c
	$(LIB_PREFIX) $(CC) $(CFLAGS) -c $< -o $@

$(LIB_Y_PICS): $(LIB_GEN_OBJ)/%$(LIB_EXT).pic: $(LIB_GEN_SRC)/%.c
	$(LIB_PREFIX) $(CC) $(CFLAGS) -fPIC -c $< -o $@

# -> le separateur du sed est `|'
# Note: interet de l'ajout de $@ comme dependance ?
$(LIB_Y_DEPENDS): $(LIB_GEN_DEP)/%$(LIB_EXT).d: $(LIB_GEN_SRC)/%.c
	$(LIB_PREFIX) $(SHELL) -ec '$(CC) -MM $(CFLAGS) -DDEPEND $< \
		| sed '\''s|.*:|$(LIB_DEP_TO_OBJ) $@ :|g'\'' > $@'

$(LIB_Y_PREPROC): $(LIB_GEN_CPP)/%$(LIB_EXT).i: $(LIB_GEN_SRC)/%.c
	$(LIB_PREFIX) $(CC) -E -P -DPREPROC $(CFLAGS) $< > $@

######################################################################