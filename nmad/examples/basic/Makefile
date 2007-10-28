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

export FLAVOR_DEFAULT:=nmad-basic

# FLAVOR -> flavor utilisee pour la compilation
#---------------------------------------------------------------------
ifndef FLAVOR
  ifdef PM2_FLAVOR
    export FLAVOR:=$(PM2_FLAVOR)
  else # PM2_FLAVOR
    ifdef FLAVOR_DEFAULT
      export FLAVOR:=$(FLAVOR_DEFAULT)
    else # FLAVOR_DEFAULT
      export FLAVOR:=default
    endif # FLAVOR_DEFAULT
  endif # PM2_FLAVOR
endif # FLAVOR

PROTOCOLS	:=	$(shell $(PM2_ROOT)/bin/pm2-config --flavor=$(FLAVOR) --getvar=PM2_NMAD_PROTO)
BASIC		:=	$(findstring proto_basic,$(PROTOCOLS))

ifeq ($(BASIC),proto_basic)
  MOD_SRC += $(PM2_ROOT)/nmad/examples/basic
else
  $(error "Protocol proto_basic is not enabled in the flavor '$(FLAVOR)'")
endif

SCHEDULERS	:=	$(shell $(PM2_ROOT)/bin/pm2-config --flavor=$(FLAVOR) --schedulers)
MINI_ALT	:=	$(findstring sched_mini_alt,$(SCHEDULERS))

ifeq ($(MINI_ALT),sched_mini_alt)
  PROGS		:=	basic_ping_any
else
  PROGS		:=	basic basic_ping basic_ping_entrelace basic_multithreaded_ping
endif

include $(PM2_ROOT)/make/apps.mak

$(MOD_GEN_BIN)/basic$(MOD_EXT): $(patsubst %.o, %$(MOD_EXT).o, $(if $(basic-objs), $(basic-objs), basic.o)) basic_common.o
$(MOD_GEN_BIN)/basic$(MOD_EXT): LDFLAGS += $(basic-ldflags) -lm
$(MOD_GEN_BIN)/basic$(MOD_EXT): $(MOD_STAMP_FILES) | flavor
$(MOD_GEN_OBJ)/basic$(MOD_EXT).o: CFLAGS += $(basic-cflags)
$(MOD_GEN_OBJ)/basic$(MOD_EXT).o: $(MOD_STAMP_FILES) | flavor

$(MOD_GEN_BIN)/basic_ping$(MOD_EXT): $(patsubst %.o, %$(MOD_EXT).o, $(if $(basic-ping-objs), $(basic-ping-objs), basic_ping.o)) basic_common.o
$(MOD_GEN_BIN)/basic_ping$(MOD_EXT): LDFLAGS += $(basic-ping-ldflags) -lm
$(MOD_GEN_BIN)/basic_ping$(MOD_EXT): $(MOD_STAMP_FILES) | flavor
$(MOD_GEN_OBJ)/basic_ping$(MOD_EXT).o: CFLAGS += $(basic-ping-cflags)
$(MOD_GEN_OBJ)/basic_ping$(MOD_EXT).o: $(MOD_STAMP_FILES) | flavor

