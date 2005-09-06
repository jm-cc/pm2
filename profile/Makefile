

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

LIBRARY=profile

include $(PM2_ROOT)/make/libs-vars.mak

# The profile.c file includes fut_entries.h, but it cannot be actually
# detected by make depend since there would be a circular dependency
# problem...
ifneq ($(MAKECMDGOALS),fut)
$(MOD_GEN_OBJ)/profile.o: $(MOD_GEN_INC)/fut_entries.h
endif

.PHONY: default libs fut_files

#libs: $(MOD_GEN_BIN)/fut_print
libs: $(MOD_GEN_INC)/fut_entries.h
#libs: ./include/fut.h

$(MOD_GEN_INC)/fut_entries.h: $(MOD_GEN_STAMP)/fut_stamp ${PM2_ROOT}/bin/pm2-build-fut-table
	$(COMMON_BUILD)
	$(COMMON_MAIN) ${PM2_ROOT}/bin/pm2-build-fut-table -f $(FLAVOR)

$(MOD_GEN_BIN)/fut_print: $(MOD_GEN_INC)/fut_entries.h

$(MOD_GEN_BIN)/fut_print: ./bin/src/fut_print.c
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) -g -o $@ -I./include -I${MOD_GEN_INC} $< -lfxt -lbfd -DFUT $(filter -L%, $(LDFLAGS))

$(MOD_GEN_STAMP)/fut_stamp: $(MOD_STAMP_FLAVOR)
	$(COMMON_BUILD)
	$(COMMON_MAIN) touch $@

# Nothing can be done at 'do_h generation time'
#dot_h: include/fxt ;

#include/fxt:
#	@echo "**********************************************************"
#	@echo "Please, set a symbolic link from $(PM2_ROOT)/$(LIBRARY)/$@ to your distribution of FxT"
#	@echo 
#	@echo "ln -s .../FxT/tools $(PM2_ROOT)/$(LIBRARY)/$@"
#	@echo "**********************************************************"
#	@exit 1

include $(PM2_ROOT)/make/libs-rules.mak
