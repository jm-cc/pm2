

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

export FLAVOR_DEFAULT:=marcel

PROGS := active	cleanup	contsw create daube delay derout eclone force_raise \
	kbd keys matrix nojoin once poll raise realtime sample schedpolicy \
	smptest stack_jump sumtime sumtree timed timeout tsp tube usersched \
	userspace vpmask

include $(PM2_ROOT)/make/apps.mak


ifdef APP_RECURSIF


$(APP_BIN)/eclone$(APP_EXT): $(APP_OBJ)/clone$(APP_EXT).o

$(APP_OBJ)/eclone$(APP_EXT).o: $(SRC_DIR)/eclone.c
	$(COMMON_PREFIX) $(CC) $(CFLAGS) -O0 -c $< -o $@

$(APP_BIN)/tsp$(APP_EXT): $(APP_OBJ)/tsp-main$(APP_EXT).o $(APP_OBJ)/tsp-job$(APP_EXT).o -lm

$(APP_OBJ)/tsp$(APP_EXT).o: $(SRC_DIR)/tsp.c
	$(COMMON_PREFIX) $(CC) $(CFLAGS) -DMT -c $< -o $@

$(APP_OBJ)/tsp-main$(APP_EXT).o: $(SRC_DIR)/tsp-main.c
	$(COMMON_PREFIX) $(CC) $(CFLAGS) -DMT -c $< -o $@

$(APP_OBJ)/tsp-job$(APP_EXT).o: $(SRC_DIR)/tsp-job.c
	$(COMMON_PREFIX) $(CC) $(CFLAGS) -DMT -c $< -o $@

endif

