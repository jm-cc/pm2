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

# LIBNAME -> nom de la librairie
#---------------------------------------------------------------------
LIBNAME=nmad
LIBDOC=newmadeleine

# Inclusion des makefiles generiques
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/libs-vars.mak


# NewMad core
# ----------------------------------------------------------
MOD_SRC += $(PM2_ROOT)/nmad/core/src

# Drivers
#-----------------------------------------------------------
MOD_SRC += $(foreach DRV, $(NMAD_DRIVERS), $(PM2_ROOT)/nmad/drivers/$(DRV) )

# Strategies
#-----------------------------------------------------------
MOD_SRC += $(foreach STRAT, $(NMAD_STRATEGIES), $(PM2_ROOT)/nmad/strategies/strat_$(STRAT))

# Interfaces
#---------------------------------------------------------------------
MOD_SRC += $(foreach INTERFACE, $(NMAD_INTERFACES), $(PM2_ROOT)/nmad/interfaces/$(INTERFACE)/src)


# Datatype de MPICH
#---------------------------------------------------------------------
MOD_SRC += $(PM2_ROOT)/nmad/ccs

# Sampling
#---------------------------------------------------------------------
MOD_SRC += $(PM2_ROOT)/nmad/drivers/sampling/src

# Regles communes aux librairies
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/libs-rules.mak

