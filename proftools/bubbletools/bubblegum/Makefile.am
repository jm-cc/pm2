#
# PM2: Parallel Multithreaded Machine
# Copyright (C) 2010 "the PM2 team" (see AUTHORS file)
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
#


AM_CPPFLAGS = 	-I$(abs_top_srcdir)/include $(GLOBAL_AM_CPPFLAGS) $(INTERNAL_CPPFLAGS)\
		 $(TBX_CFLAGS) $(FXT_CFLAGS) $(FREETYPE2_CFLAGS) $(GTKGLEXT_CFLAGS)
AM_CFLAGS   = 	$(GLOBAL_AM_CFLAGS)
AM_LDFLAGS  = 	$(GLOBAL_AM_LDFLAGS) $(TBX_LIBS) $(FXT_LIBS) $(FREETYPE2_LIBS) $(GTKGLEXT_LIBS)
LDADD       = 	$(GLOBAL_AM_LDADD)


include_HEADERS = 	include/actions.h include/bulle.h include/interfaceGaucheDessin.h include/menus.h	\
			include/rearangement.h include/texture.h include/zone.h include/animation.h 		\
			include/geneC.h include/load.h include/polices.h include/rightwindow.h 			\
			include/toolbars.h include/bubble_gl_anim.h include/interfaceGauche.h 			\
			include/mainwindow.h include/raccourcis.h include/save.h include/util.h

bin_PROGRAMS =		bubblegum
bubblegum_SOURCES =	source/actions.c source/animation.c source/bubble_gl_anim.c source/bubble_gl_interface.c\
			source/bulle.c source/geneC.c source/interfaceGauche.c source/interfaceGaucheDessin.c	\
			source/load.c source/mainwindow.c source/menus.c source/polices.c source/raccourcis.c	\
			source/rearangement.c source/rightwindow.c source/save.c source/texture.c		\
			source/toolbars.c source/tracefile_multi source/util.c source/zone.c