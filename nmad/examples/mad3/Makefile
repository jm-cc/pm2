
# NewMadeleine
# Copyright (C) 2006 (see AUTHORS file)
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

ifeq ($(CONFIG_SCHED_OPT), y)
# When compiled with sched opt, the mad3 emulation proto defines
# the mad_nmad_* function set (which does not provide a permissive 
# receive operation), therefore we do not compile standard Madeleine examples
APP		:=
else
APP		:= mad_ping mad_mini mad_micro
endif

NM_PATH		:= ../..

include $(NM_PATH)/Makefile

#
