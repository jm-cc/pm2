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

#
#	-DMAD_TIMING
#		Sets the "trace mode" on.
#
#	-DUSE_SAFE_MALLOC
#		Uses a small "safe malloc" library that trys to
#		verify bad malloc/free usage.
#


# Extension utilisee pour les fichiers binaires, objets, etc.
MAD_EXT		:=	-maddebug

MAD_GEN_1	:=	-Wall
MAD_GEN_2	:=	#-O6
MAD_GEN_3	:=	-g
MAD_GEN_4	:=	#-DUSE_SAFE_MALLOC
MAD_GEN_5	:=	-fomit-frame-pointer
MAD_GEN_6	:=	#

MAD_OPT_0	:=	#-DMAD_TIMING
MAD_OPT_1	:=	#-DUSE_MARCEL_POLL
MAD_OPT_2	:=	#-DUSE_BLOCKING_IO
MAD_OPT_3	:=	-DMAD_DEBUG
MAD_OPT_4	:=	#-DMAD_TRACE
MAD_OPT_5	:=	#-DMAD_TIMING
MAD_OPT_6	:=	#
MAD_OPT_7	:=	#
MAD_OPT_8	:=	#

#MAD_MAK_VERB	:=	verbose
MAD_MAK_VERB	:=	normal
#MAD_MAK_VERB	:=	quiet
#MAD_MAK_VERB	:=	silent

MAD_GEN_OPT	:=	$(MAD_GEN_1) $(MAD_GEN_2) $(MAD_GEN_3) \
			$(MAD_GEN_4) $(MAD_GEN_5) $(MAD_GEN_6)

MAD_OPT		:=	$(strip $(MAD_OPT_0) $(MAD_OPT_1) $(MAD_OPT_2) \
			$(MAD_OPT_3) $(MAD_OPT_4) $(MAD_OPT_5) \
			$(MAD_OPT_6) $(MAD_OPT_7) $(MAD_OPT_8))

