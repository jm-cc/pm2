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

include $(PM2_ROOT)/make/common-rules.mak

ifdef PROG_RECURSIF
$(PROGRAM):

ifneq ($(PROGRAM),$(PROGNAME))
$(PROGNAME): $(PROGRAM)
endif

prog: $(PROGRAM)

all: $(PROGRAM)

.PHONY: $(PROGRAM) $(PROGNAME) prog

ifdef SHOW_FLAVOR
$(PROGRAM): showflavor
endif

ifdef SHOW_FLAGS
$(PROGRAM): showflags
endif

$(PROGRAM): $(PRG_PRG)

.PHONY: showflags
showflags:
	@echo $(CFLAGS)

showflavor:
	@echo Compiling for flavor: $(FLAVOR)

ifeq (,$(findstring _$(MAKECMDGOALS)_,_clean_ $(DO_NOT_GENERATE_MAK_FILES)))
# Target subdirectories
DUMMY_BUILD :=  $(foreach REP, $(PRG_REP_TO_BUILD), $(shell mkdir -p $(REP)))
ifeq ($(wildcard $(PRG_DEPENDS)),$(PRG_DEPENDS))
include $(PRG_DEPENDS)
endif
endif

$(PRG_DEPENDS): $(COMMON_DEPS) $(PRG_GEN_C_SOURCES) $(PRG_GEN_C_INC)
$(PRG_OBJECTS): $(PRG_GEN_OBJ)/%.o: $(PRG_GEN_DEP)/%.d $(COMMON_DEPS)

$(PRG_PRG): $(PRG_OBJECTS)
	$(PRG_PREFIX) $(CC) $(PRG_OBJECTS) $(LDFLAGS) -o $(PRG_PRG)
# C
$(PRG_C_OBJECTS): $(PRG_GEN_OBJ)/%$(PRG_EXT).o: $(PRG_SRC)/%.c
	$(PRG_PREFIX) $(CC) $(CFLAGS) -c $< -o $@

$(PRG_C_DEPENDS): $(PRG_GEN_DEP)/%$(PRG_EXT).d: $(PRG_SRC)/%.c
	$(PRG_PREFIX) $(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(PRG_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

# Assembleur
$(PRG_S_OBJECTS): $(PRG_GEN_OBJ)/%$(PRG_EXT).o: $(PRG_SRC)/%.S
	$(COMMON_HIDE) $(CC) -E -P $(CFLAGS) $< > $(PRG_OBJ_TO_S)
	$(PRG_PREFIX) $(AS) $(CFLAGS) -c $(PRG_OBJ_TO_S) -o $@

$(PRG_S_DEPENDS): $(PRG_GEN_DEP)/%$(PRG_EXT).d: $(PRG_SRC)/%.S
	$(PRG_PREFIX) $(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(PRG_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

# Compilation des fichiers sources generes
$(PRG_GEN_C_OBJECTS): $(PRG_GEN_OBJ)/%$(PRG_EXT).o: $(PRG_GEN_SRC)/%.c
	$(PRG_PREFIX) $(CC) $(CFLAGS) -c $< -o $@

$(PRG_GEN_C_DEPENDS): $(PRG_GEN_DEP)/%$(PRG_EXT).d: $(PRG_GEN_SRC)/%.c
	$(PRG_PREFIX) $(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(PRG_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

# Lex

$(PRG_GEN_C_L_SOURCES): $(PRG_GEN_SRC)/%$(PRG_EXT).c: $(PRG_SRC)/%.l
	$(COMMON_HIDE) $(LEX) $<; mv lex.yy.c $@

# Yacc
$(PRG_GEN_C_Y_SOURCES): $(PRG_GEN_SRC)/%$(PRG_EXT).c: $(PRG_SRC)/%.y
	$(COMMON_HIDE) $(YACC) $<; mv y.tab.c $@; mv y.tab.h $(PRG_GEN_C_TO_H)

$(PRG_GEN_C_Y_INC): $(PRG_GEN_INC)/%$(PRG_EXT).h: $(PRG_SRC)/%.y
	$(COMMON_HIDE) $(YACC) $<; mv y.tab.h $@; mv y.tab.h $(PRG_GEN_H_TO_C)

else # !PROG_RECURSIF: premier appel
MAKE_LIBS = set -e ; for modules in $(CONFIG_MODULES); do \
                if [ $$modules != $(PROGRAM) ] ; then \
		    $(MAKE) -C $(PM2_ROOT)/modules/$$modules ; \
                fi ;\
	    done 

.PHONY: $(PROGRAM)
$(PROGRAM):
	@ echo "Making libs"
	$(COMMON_HIDE) $(MAKE_LIBS)
	@ echo "Making program"
	$(COMMON_HIDE) $(MAKE) PROG_RECURSIF=true $@
	@ echo "Program ready"

.PHONY: clean prgclean repclean distclean
clean: prgclean repclean

prgclean:
	$(COMMON_CLEAN) $(RM) $(PRG_GEN_OBJ)/*$(PRG_EXT).o \
		$(PRG_GEN_DEP)/*$(PRG_EXT).d $(PRG_GEN_ASM)/*$(PRG_EXT).s \
		$(PRG_GEN_SRC)/*$(PRG_EXT).c $(PRG_GEN_INC)/*$(PRG_EXT).h \
		$(PRG_PRG)

repclean:
	@for rep in $(PRG_REP_TO_BUILD); do \
		if rmdir $$rep 2> /dev/null; then \
			echo "empty repertory $$rep removed" ; \
		fi ; \
	done

#distclean:
#	$(COMMON_CLEAN)$(RM) -r build
#	@set -e; 

.PHONY: help bannerhelpprg targethelpprg
help: globalhelp

bannerhelp: bannerhelpprg

bannerhelpprg:
	@echo "This is PM2 Makefile for module $(LIBRARY)"

targethelp: targethelpprg

targethelpprg:
ifneq ($(PROGRAM),$(PROGNAME))
	@echo "  prog|all|$(PROGRAM)|$(PROGNAME): build the program"
else
	@echo "  prog|all|$(PROGRAM): build the program"
endif
	@echo "  examples: build the examples of this program (if any)"
	@echo "  help: this help"
	@echo "  clean: clean program source tree for current flavor"
#	@echo "  distclean: clean program source tree for all flavors"

endif # !PROG_RECURSIF
