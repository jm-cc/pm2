# -*- mode: makefile;-*-

# PM2: Parallel Multithreaded Machine
# Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

include $(PM2_ROOT)/make/objs-rules.mak

.PHONY: all examples
all: $(APPS_LIST)
	$(COMMON_HIDE) echo $(APPS_LIST) sucessfully generated

examples: all

.PHONY: flavor
flavor:
	$(COMMON_HIDE) echo "Generating libraries..."
	$(COMMON_MAIN) $(MAKE) -C $(PM2_ROOT)
	$(COMMON_HIDE) echo "Generating libraries: done"

$(MOD_GEN_BIN)/%$(MOD_EXT): $(MOD_STAMP_FILES)
	$(COMMON_LINK)
	$(COMMON_MAIN) $(CC) $(filter %.o, $^) $(LDFLAGS) -o $@

.PHONY: $(APPS_LIST)
$(APPS_LIST): %: flavor $(MOD_GEN_BIN)/%$(MOD_EXT)

# Generation des fichiers 'fut'
#.PHONY: fut
#fut:
#	$(COMMON_BUILD)
#	$(COMMON_MAIN) $(MAKE) -C . APP_RECURSIF=true $(APP_CPP)/$(TARGET).i $(APP_CPP)/$(TARGET).fut

#$(APP_CPP)/%$(LIB_EXT).i: $(SRC_DIR)/%.c
#	$(COMMON_BUILD)
#	$(COMMON_MAIN) $(CC) -E -P -DPREPROC $(CFLAGS) $< > $@

#$(APP_CPP)/%.fut: $(APP_CPP)/%.i
#	$(COMMON_BUILD)
#	$(COMMON_MAIN) cp /dev/null $@
#	$(COMMON_HIDE) gcc -c -O0 $< -o /tmp/foo-$(USER).o
#	$(COMMON_HIDE) nm /tmp/foo-$(USER).o | fgrep this_is_the_ | sed -e 's/^.*this_is_the_//' >> $@
#	$(COMMON_HIDE) rm -f /tmp/foo-$(USER).o
#	$(COMMON_HIDE) touch $(LIB_GEN_STAMP)/fut_stamp

#else # APP_RECURSIF


# Inclusion du cache de configuration spécific des programmes
#---------------------------------------------------------------------
MOD_LINKED_OBJECTS=$(APPS_LIST)
$(PM2_MAK_DIR)/$(MODULE)-specific.mak: \
		$(if $(strip	$(filter-out $(MOD_LINKED_OBJECTS_SAVED), $(MOD_LINKED_OBJECTS)) \
				$(filter-out $(MOD_LINKED_OBJECTS), $(MOD_LINKED_OBJECTS_SAVED))), \
			FORCE)
$(PM2_MAK_DIR)/$(MODULE)-specific.mak: \
		$(if $(strip $(filter-out $(MOD_OBJECTS_SAVED), $(MOD_BASE:%=%.o)) \
				$(filter-out $(MOD_BASE:%=%.o), $(MOD_OBJECTS_SAVED))), \
			FORCE)

$(PM2_MAK_DIR)/$(MODULE)-specific.mak: $(MAIN_STAMP_FLAVOR) $(COMMON_DEPS) $(MAKEFILE)
	$(COMMON_MAKE)
	$(COMMON_HIDE) mkdir -p $(PM2_MAK_DIR)
	$(COMMON_HIDE) ( echo MOD_LINKED_OBJECTS_SAVED=$(MOD_LINKED_OBJECTS) ; \
		$(foreach p, $(APPS_LIST), \
		echo '$$(MOD_GEN_BIN)/$p$$(MOD_EXT): $(patsubst %.o, %$$(MOD_EXT).o, \
			$(if $($p-objs), $($p-objs), $p.o))'; \
		$(if $(strip $($p-ldflags)), \
			echo '$$(MOD_GEN_BIN)/$p$$(MOD_EXT): LDFLAGS += $$($p-ldflags)';) \
		) ) > $@
	$(COMMON_HIDE) ( echo MOD_OBJECTS_SAVED=$(MOD_BASE:%=%.o) ; \
		$(foreach o, $(MOD_BASE), \
		$(if $(strip $($o-cflags)), \
			echo '$$(MOD_GEN_OBJ)/$o$$(MOD_EXT).o: CFLAGS += $$($o-cflags)';) \
		) ) >> $@


.PHONY: FORCE
#$(patsubst %.o, %$(MOD_EXT).o, \
#				$(if $($*-objs), $($*-objs), $*.o)
