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
#
# The expansion of the macro MAR_OPTIONS may include the following
# definitions :
#
#	-DSTACK_CHECKING_ALLOWED
#		This option will allow threads to call the
#		`marcel_unusedstack_np' function in order to evaluate
#		the amount of stack (in bytes) they did not use
#		yet. Should only be defined when tuning applications.
#
#	-DSTACK_OVERFLOW_DETECT
#		This will use some protected areas of memory to trap
#		stack overflows.  Should not be defined when
#		efficiency is needed.
#
#	-DDO_NOT_CHAIN_BLOCKED_THREADS
#		This may be defined to speed up execution, but it
#		will not allow the `marcel_threadslist_np' function
#		to inspect the list of blocked threads. As a result,
#		the `th' command of the PM2 console will not display
#		the blocked threads. Should normaly be undefined.
#
#	-DMINIMAL_PREEMPTION
#		This may be set to disable automatic preemption
#		between threads.  Only a special (user-defined) thread
#		may be allowed to be preempted at regular intervals.
#
#	-DUSE_VIRTUAL_TIMER
#		Forces the scheduler to use the SIGVTALRM signal (rather
#		than SIGALRM) to implement automatic preemption.
#
#	-DMAR_TIMING
#		Sets the "profiling mode" on.
#
#	-DUSE_SAFE_MALLOC
#		Uses a small "safe malloc" library that trys to
#		verify bad malloc/free usage.
#
#	-DMAR_TRACE
#		Sets the "trace" mode on.
#

MAR_OPTIONS	=	#-DSTANDARD_MAIN -DUSE_PRIORITIES \
		        #-DDO_NOT_CHAIN_BLOCKED_THREADS

MAR_SMP		=	no
MAR_ACTI	=	no

MAR0	=	-Wall #-g -DDEBUG
MAR1	=	-O6
MAR2	=	#-DMAR_TRACE
MAR3	=	#-DMAR_TIMING
MAR4	=	#-DUSE_SAFE_MALLOC
MAR5	=	#-DACT_TIMER  #Pour les temps de deblockage
MAR6	=	#-DACT_VERBOSE  #Pour les messages ACTDEBUG()
MAR7	=	#-DENABLE_STACK_JUMPING
MAR8	=	

MAR_STANDALONE_OPTIONS	= $(MAR0) $(MAR1) $(MAR2) $(MAR3) $(MAR4) $(MAR5) $(MAR6) $(MAR7) $(MAR8)

#
# SEE ALSO :
#
# In the header file marcel.h, the macro USE_MACROS can be defined in
# order to improve the efficiency of some commonly used
# functions. This will however decrease the level of error checking,
# so define it at your own risk !
#

#
################# END OF CUSTOMIZATION SECTION ###################

ifndef MARCEL_ROOT
MARCEL_ROOT	:=	$(PM2_ROOT)/marcel
endif

ifndef PM2_ARCH_SYS
PM2_ARCH	:=	$(shell $(PM2_ROOT)/bin/pm2_arch)
PM2_SYS		:=	$(shell $(PM2_ROOT)/bin/pm2_sys)
PM2_ARCH_SYS	:=	$(shell basename $(PM2_SYS) _SYS)/$(shell basename $(PM2_ARCH) _ARCH)
endif

ifndef SMP
SMP	=	$(MAR_SMP)
endif

ifeq ($(SMP),yes)
MAR_STANDALONE_OPTIONS += -DSMP -D_REENTRANT
endif

ifeq ($(MAR_ACTI),yes)
MAR_OPTIONS	+=	-D__ACT__
endif

ifdef PM2_FLAGS
MAR_MAKEFILE	:=	$(MARCEL_ROOT)/Makefile $(MARCEL_ROOT)/.mar_pm2
else
MAR_MAKEFILE	:=	$(MARCEL_ROOT)/Makefile $(MARCEL_ROOT)/.mar_standalone
endif

MAR_SRC		:= $(MARCEL_ROOT)/source
MAR_DEP		:= $(MAR_SRC)/depend
MAR_OBJ		:= $(MAR_SRC)/obj/$(PM2_ARCH_SYS)
MAR_INC		:= $(MARCEL_ROOT)/include
MAR_LIBD	:= $(MARCEL_ROOT)/lib/$(PM2_ARCH_SYS)

ifneq ($(wildcard $(MAR_DEP)),$(MAR_DEP))
DUMMY_BUILD	:=	$(shell mkdir -p $(MAR_DEP))
endif

ifneq ($(wildcard $(MAR_OBJ)),$(MAR_OBJ))
DUMMY_BUILD	:=	$(shell mkdir -p $(MAR_OBJ))
endif

ifneq ($(wildcard $(MAR_LIBD)),$(MAR_LIBD))
DUMMY_BUILD	:=	$(shell mkdir -p $(MAR_LIBD))
endif

MAR_CC		=	gcc
MAR_AS		:=	$(MAR_CC)

ifdef PM2_FLAGS
MAR_CFLAGS	=	$(MAR_OPTIONS) -fomit-frame-pointer $(PM2_FLAGS) \
			-I$(MAR_INC) -I$(PM2_ROOT)/include
else
MAR_CFLAGS	=	$(MAR_OPTIONS) -fomit-frame-pointer $(MAR_STANDALONE_OPTIONS) \
			-D$(PM2_ARCH) -D$(PM2_SYS) \
			-I$(MAR_INC) $($(PM2_SYS)_CFL)
endif

MAR_ASFLAGS	=	-D$(PM2_ARCH) -D$(PM2_SYS)

MAR_LDFLAGS	=	
MAR_LIB		=	$(MAR_LIBD)/libmarcel.a
MAR_LLIB	=	

ifeq ($(SMP),yes)
MAR_LLIB	+=	-lpthread
ifeq ($(PM2_SYS),SOLARIS_SYS)
MAR_LLIB	+=	-lposix4
endif
endif

MAR_C_SOURCES	:=	$(wildcard $(MAR_SRC)/*.c)
MAR_S_SOURCES	:=	$(wildcard $(MAR_SRC)/*.S)

MAR_C_OBJECTS	:=	$(patsubst %.c,%.o,$(subst $(MAR_SRC),$(MAR_OBJ),$(MAR_C_SOURCES)))
MAR_S_OBJECTS	:=	$(patsubst %.S,%.o,$(subst $(MAR_SRC),$(MAR_OBJ),$(MAR_S_SOURCES)))
MAR_OBJECTS	:=	$(MAR_C_OBJECTS) $(MAR_S_OBJECTS)

MAR_C_DEPENDS	:=	$(patsubst %.o,%.d,$(subst $(MAR_OBJ),$(MAR_DEP),$(MAR_C_OBJECTS)))
MAR_S_DEPENDS	:=	$(patsubst %.o,%.d,$(subst $(MAR_OBJ),$(MAR_DEP),$(MAR_S_OBJECTS)))
MAR_DEPENDS	:=	$(strip $(MAR_C_DEPENDS) $(MAR_S_DEPENDS))

MAR_DEP_TO_OBJ	=	$(MAR_OBJ)/$(patsubst %.d,%.o,$(notdir $@))
MAR_OBJ_TO_S	=	$(MAR_SRC)/$(patsubst %.o,%.s,$(notdir $@))

.PHONY: mar_default
mar_default: $(MAR_LIB)

ifneq ($(MAKECMDGOALS),clean)
ifeq ($(wildcard $(MAR_DEPENDS)),$(MAR_DEPENDS))
include $(MAR_DEPENDS)
endif
endif

ifdef PM2_FLAGS
THE_MAKEFILES	=	$(PM2_MAKEFILE)
else
THE_MAKEFILES	=	$(MAR_MAKEFILE)
endif

$(MAR_OBJECTS): $(MAR_OBJ)/%.o: $(MAR_DEP)/%.d
$(MAR_DEPENDS): $(THE_MAKEFILES)

$(MAR_LIB): $(MAR_OBJECTS)
ifdef PM2_FLAGS
	rm -f $(MARCEL_ROOT)/.mar_standalone
else
	rm -f $(MARCEL_ROOT)/.mar_pm2
endif
	rm -f $(MAR_LIB)
	ar cr $(MAR_LIB) $(MAR_OBJECTS)

$(MARCEL_ROOT)/.mar_pm2:
	cp /dev/null $(MARCEL_ROOT)/.mar_pm2

$(MARCEL_ROOT)/.mar_standalone:
	cp /dev/null $(MARCEL_ROOT)/.mar_standalone


$(MAR_C_OBJECTS): $(MAR_OBJ)/%.o: $(MAR_SRC)/%.c
	$(MAR_CC) $(MAR_CFLAGS) -c $< -o $@

$(MAR_C_DEPENDS): $(MAR_DEP)/%.d: $(MAR_SRC)/%.c
	$(SHELL) -ec '$(MAR_CC) -MM $(MAR_CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(MAR_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

$(MAR_S_OBJECTS): $(MAR_OBJ)/%.o: $(MAR_SRC)/%.S
	$(MAR_CC) -E -P $(MAR_ASFLAGS) $< > $(MAR_OBJ_TO_S)
	$(MAR_AS) $(MAR_ASFLAGS) -c $(MAR_OBJ_TO_S) -o $@
	rm -f $(MAR_OBJ_TO_S)

$(MAR_S_DEPENDS): $(MAR_DEP)/%.d: $(MAR_SRC)/%.S
	$(SHELL) -ec '$(MAR_CC) -MM $(MAR_CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(MAR_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'


ifdef PM2_FLAGS
.PHONY: marclean
marclean:
	rm -f $(wildcard $(MAR_LIBD)/*.a $(MAR_OBJ)/*.o $(MAR_DEP)/*.d \
		$(MARCEL_ROOT)/examples/depend/*.d \
		$(MARCEL_ROOT)/examples/obj/$(PM2_ARCH_SYS)/*.o \
		$(MARCEL_ROOT)/examples/bin/$(PM2_ARCH_SYS)/*)

mardistclean:
	rm -rf $(wildcard $(MARCEL_ROOT)/lib \
		$(MARCEL_ROOT)/source/obj \
		$(MARCEL_ROOT)/source/depend \
		$(MARCEL_ROOT)/examples/depend \
		$(MARCEL_ROOT)/examples/obj \
		$(MARCEL_ROOT)/examples/bin)
else
.PHONY: clean distclean
clean:
	rm -f $(wildcard $(MAR_LIBD)/*.a $(MAR_OBJ)/*.o $(MAR_DEP)/*.d \
		$(MARCEL_ROOT)/examples/depend/*.d \
		$(MARCEL_ROOT)/examples/obj/$(PM2_ARCH_SYS)/*.o \
		$(MARCEL_ROOT)/examples/bin/$(PM2_ARCH_SYS)/*)

distclean:
	rm -rf $(wildcard $(MARCEL_ROOT)/lib \
		$(MARCEL_ROOT)/source/obj \
		$(MARCEL_ROOT)/source/depend \
		$(MARCEL_ROOT)/examples/depend \
		$(MARCEL_ROOT)/examples/obj \
		$(MARCEL_ROOT)/examples/bin)
endif


######################## Applications ########################

ifdef MAR_EX_DIR

ifeq ($(MAR_EX_DIR),.)
SRC_DIR		:=	./
else
SRC_DIR		:=	$(MAR_EX_DIR)
endif

THIS_MAKEFILE	:=	$(SRC_DIR)/Makefile

DEP_DIR	=	$(SRC_DIR)/depend
OBJ_DIR	=	$(SRC_DIR)/obj/$(PM2_ARCH_SYS)
BIN_DIR	=	$(SRC_DIR)/bin/$(PM2_ARCH_SYS)

ifneq ($(wildcard $(DEP_DIR)),$(DEP_DIR))
DUMMY_BUILD	:=	$(shell mkdir -p $(DEP_DIR))
endif

ifneq ($(wildcard $(OBJ_DIR)),$(OBJ_DIR))
DUMMY_BUILD	:=	$(shell mkdir -p $(OBJ_DIR))
endif

ifneq ($(wildcard $(BIN_DIR)),$(BIN_DIR))
DUMMY_BUILD	:=	$(shell mkdir -p $(BIN_DIR))
endif

SOURCES	:=	$(wildcard $(SRC_DIR)/*.c)
OBJECTS	:=	$(patsubst %.c,%.o,$(subst $(SRC_DIR),$(OBJ_DIR),$(SOURCES)))
DEPENDS	:=	$(patsubst %.c,%.d,$(subst $(SRC_DIR),$(DEP_DIR),$(SOURCES)))

DEP_TO_OBJ	=	$(OBJ_DIR)/$(patsubst %.d,%.o,$(notdir $@))

ifneq ($(MAKECMDGOALS),clean)
ifeq ($(wildcard $(DEPENDS)),$(DEPENDS))
include $(DEPENDS)
endif
endif

$(OBJECTS): $(OBJ_DIR)/%.o: $(DEP_DIR)/%.d
$(DEPENDS): $(THIS_MAKEFILE) $(MAR_MAKEFILE)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(MAR_CC) $(MAR_CFLAGS) -c $< -o $@

$(DEPENDS): $(DEP_DIR)/%.d: $(SRC_DIR)/%.c
	$(SHELL) -ec '$(MAR_CC) -MM $(MAR_CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

$(BIN_DIR)/%: $(OBJ_DIR)/%.o $(MAR_LIB)
	$(MAR_CC) $(MAR_CFLAGS) $(MAR_LDFLAGS) $^ -o $@ $(MAR_LLIB)

%: $(BIN_DIR)/% ;

.PHONY: clean_ex
clean: clean_ex

clean_ex:
	rm -f $(wildcard $(DEP_DIR)/*.d $(OBJ_DIR)/*.o $(BIN_DIR)/*)

endif