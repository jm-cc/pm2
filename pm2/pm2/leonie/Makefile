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

###################### CUSTOMIZATION SECTION #####################

LEO_OPTIONS =  -g -Wall # -DDEBUG -O6 -DMAD_TIMING -DUSE_SAFE_MALLOC

################### END OF CUSTOMIZATION SECTION #################

ifndef LEO_ROOT
LEO_ROOT =  $(PM2_ROOT)/leonie
endif

LEONIE          := yes

ifndef PM2_ARCH_SYS
PM2_ARCH     :=  $(shell $(PM2_ROOT)/bin/pm2_arch)
PM2_SYS      :=  $(shell $(PM2_ROOT)/bin/pm2_sys)
PM2_ARCH_SYS :=  $(shell basename $(PM2_SYS) _SYS)/$(shell basename $(PM2_ARCH) _ARCH)
endif

LEO_MAKEFILE =  $(LEO_ROOT)/Makefile

LEO_OBJ  :=  $(LEO_ROOT)/obj
LEO_SRC  :=  $(LEO_ROOT)/src
LEO_LY   :=  $(LEO_ROOT)/lex_yacc
LEO_DEP  :=  $(LEO_SRC)/depend
LEO_INC  :=  $(LEO_ROOT)/include
LEO_BIND  =  $(LEO_ROOT)/bin/$(PM2_ARCH_SYS)

ifneq ($(wildcard $(LEO_DEP)),$(LEO_DEP))
DUMMY_BUILD :=  $(shell mkdir -p $(LEO_DEP))
endif

ifneq ($(wildcard $(LEO_OBJ)),$(LEO_OBJ))
DUMMY_BUILD :=  $(shell mkdir -p $(LEO_OBJ))
endif

ifneq ($(wildcard $(LEO_BIND)),$(LEO_BIND))
DUMMY_BUILD :=  $(shell mkdir -p $(LEO_BIND))
endif

ifndef TBX_ROOT
TBX_ROOT :=  $(LEO_ROOT)/../toolbox
endif

ifndef NTBX_ROOT
NTBX_ROOT :=  $(TBX_ROOT)/net
endif

LEO_SUB_MAKEFILES := $(TBX_ROOT)/Makefile $(NTBX_ROOT)/Makefile

LEO_REG_SOURCES :=  $(wildcard $(LEO_SRC)/*.c)
LEO_LY_SOURCES  :=  $(LEO_LY)/y.tab.c $(LEO_LY)/lex.yy.c
LEO_REG_OBJECTS :=  $(patsubst %.c,%.o,$(subst $(LEO_SRC),$(LEO_OBJ),$(LEO_REG_SOURCES)))
LEO_LY_OBJECTS  :=  $(patsubst %.c,%.o,$(subst $(LEO_LY),$(LEO_OBJ),$(LEO_LY_SOURCES)))
LEO_OBJECTS     :=  $(LEO_REG_OBJECTS) $(LEO_LY_OBJECTS) 

LEO_APP   =  $(LEO_BIND)/leonie
LEO_LLIBS =  $(ARCHDLIB) -lfl  $(TBX_LIB) $(NTBX_LIB) $(TBX_LLIB) $(NTBX_LLIB)

LEO_REG_DEPENDS :=  $(patsubst %.o,%.d,$(subst $(LEO_OBJ),$(LEO_DEP),$(LEO_REG_OBJECTS)))
LEO_LY_DEPENDS  :=  $(patsubst %.o,%.d,$(subst $(LEO_OBJ),$(LEO_DEP),$(LEO_LY_OBJECTS)))
LEO_DEPENDS     :=  ($strip $(LEO_REG_DEPENDS)) ($strip $(LEO_LY_DEPENDS))
LEO_DEP_TO_OBJ   =  $(LEO_OBJ)/$(patsubst %.d,%.o,$(notdir $@))

MASTER         :=  leonie
MASTER_CC      :=  gcc
MASTER_OPTIONS :=  $(LEO_OPTIONS)
MASTER_DEP     :=  $(LEO_MAKEFILE) $(LEO_SUB_MAKEFILE)

LEO_LEX  =  flex
LEO_YACC =  bison -y
LEO_CC   =  $(MASTER_CC)

LEO_CFLAGS  =  $(MASTER_OPTIONS) $(PM2_FLAGS) \
               -I$(LEO_INC) -I$(LEO_LY) -I$(TBX_ROOT)/include \
               -I$(SWANN_ROOT)/include -I$(NTBX_ROOT)/include
LEO_LDFLAGS = 

ifneq ($(MAKECMDGOALS),clean)
ifeq ($(wildcard $(LEO_DEPENDS)),$(LEO_DEPENDS))
include $(LEO_DEPENDS)
endif
endif

.PHONY: leo_default
leo_default: $(LEO_APP)

include $(LEO_SUB_MAKEFILES)

$(LEO_OBJECTS): $(LEO_OBJ)/%.o: $(LEO_DEP)/%.d
$(LEO_DEPENDS): $(MASTER_DEP)

$(LEO_APP): $(LEO_OBJECTS) tbx_default ntbx_default
	@ echo "  -  $(@F)"
	$(LEO_CC) -o $(LEO_APP) $(LEO_OBJECTS) $(LEO_LLIBS)

$(LEO_REG_OBJECTS): $(LEO_OBJ)/%.o: $(LEO_SRC)/%.c ${LEO_LY}/y.tab.h
	@ echo "  -  $(@F)"
	$(LEO_CC) $(LEO_CFLAGS) $(LEO_FLAGS) -c $< -o $@

$(LEO_REG_DEPENDS): $(LEO_DEP)/%.d: $(LEO_SRC)/%.c ${LEO_LY}/y.tab.h
	@ echo "  -  $(@F)"
	$(SHELL) -ec '$(LEO_CC) -MM $(LEO_CFLAGS) $(LEO_FLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(LEO_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

$(LEO_LY_OBJECTS): $(LEO_OBJ)/%.o: $(LEO_LY)/%.c ${LEO_LY}/y.tab.h
	@ echo "  -  $(@F)"
	$(LEO_CC) $(LEO_CFLAGS) $(LEO_FLAGS) -c $< -o $@

$(LEO_LY_DEPENDS): $(LEO_DEP)/%.d: $(LEO_LY)/%.c ${LEO_LY}/y.tab.h
	@ echo "  -  $(@F)"
	$(SHELL) -ec '$(LEO_CC) -MM $(LEO_CFLAGS) $(LEO_FLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(LEO_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

${LEO_LY}/lex.yy.c:  $(LEO_LY)/leo_lexer.l $(LEO_LY)/y.tab.h 
	cd $(LEO_LY); $(LEX) leo_lexer.l

${LEO_LY}/y.tab.c ${LEO_LY}/y.tab.h: $(LEO_LY)/leo_parser.y
	cd $(LEO_LY); $(YACC) -d leo_parser.y

.PHONY: clean distclean
clean: tbxclean ntbxclean
	@ echo "Cleaning leonie"
	rm -f $(LEO_APP) \
	rm -f $(wildcard $(LEO_OBJ)/*.o $(LEO_DEP)/*.d) \
	rm -f $(LEO_LY)/y.tab.c \
	rm -f $(LEO_LY)/y.tab.h \
	rm -f $(LEO_LY)/lex.yy.c

distclean: tbxdistclean ntbxdistclean
	rm -rf $(wildcard $(LEO_ROOT)/bin \
		$(LEO_ROOT)/obj \
		$(LEO_ROOT)/source/depend) 
	rm -f $(wildcard $(LEO_OBJ)/*.o $(LEO_DEP)/*.d) \
	rm -f $(LEO_LY)/y.tab.c \
	rm -f $(LEO_LY)/y.tab.h \
	rm -f $(LEO_LY)/lex.yy.c

