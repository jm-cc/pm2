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
#	-DMINIMAL_PREEMPTION
#		Sets up a 'minimal preemption' mechanism that will
#		only give hand (from time to time) to a special
#		'message-receiver' thread.
#		This option *must* be set as a GLOBAL_OPTION.
#
#	-DPM2_TIMING
#		Sets the "profiling mode" on.
#
#	-DMIGRATE_IN_HEADER
#		Private Flag. Should *not* be used in regular
#		applications.
#
#	-DUSE_SAFE_MALLOC
#		Uses a small "safe malloc" library that trys to
#		verify bad malloc/free usage.
#

# Extension utilisee pour les fichiers binaires, objets, etc.
PM2_EXT		:=	

PM2_GEN_1	:=	-Wall
PM2_GEN_2	:=	-O6
PM2_GEN_3	:=	#-g
PM2_GEN_4	:=	#-DUSE_SAFE_MALLOC
PM2_GEN_5	:=	-fomit-frame-pointer
PM2_GEN_6	:=	#

PM2_USE_DSM	:=	yes
#PM2_USE_DSM	:=	no

PM2_OPT_0	:=	#-DPM2_DEBUG
PM2_OPT_1	:=	#-DPM2_TRACE
PM2_OPT_2	:=	#-DPM2_TIMING
PM2_OPT_3	:=	#-DMINIMAL_PREEMPTION
PM2_OPT_4	:=	#-DMIGRATE_IN_HEADER
PM2_OPT_5	:=	#
PM2_OPT_6	:=	#
PM2_OPT_7	:=	#
PM2_OPT_8	:=	#

#PM2_MAK_VERB	:=	verbose
#PM2_MAK_VERB	:=	normal
PM2_MAK_VERB	:=	quiet
#PM2_MAK_VERB	:=	silent

PM2_GEN_OPT	:=	$(PM2_GEN_1) $(PM2_GEN_2) $(PM2_GEN_3) \
			$(PM2_GEN_4) $(PM2_GEN_5) $(PM2_GEN_6)

PM2_OPT		:=	$(strip $(PM2_OPT_0) $(PM2_OPT_1) $(PM2_OPT_2) \
			$(PM2_OPT_3) $(PM2_OPT_4) $(PM2_OPT_5) \
			$(PM2_OPT_6) $(PM2_OPT_7) $(PM2_OPT_8))
