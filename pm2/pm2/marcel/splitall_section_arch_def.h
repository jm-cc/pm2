
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

/****************************************************************/
/* Fichiers préinclus dans tous les fichiers headers splités
 */
#section default[no-depend-previous,no-include-in-master-section]

#section compiler[no-show-section]
#depend "sys/marcel_flags.h"

#section macros[no-show-section]
#depend "[compiler]"

#section types[no-show-section]
#depend "marcel_utils.h[types]"
#depend "[macros]"

#section structures[no-show-section]
#depend "[types]"

#section variables[no-show-section]
#depend "[structures]"

#section functions[no-show-section]
#depend "[structures]"

#section declarations[no-show-section]
#depend "[functions]"
#depend "[variables]"

#section inline[no-show-section]
#depend "[declarations]"

#section marcel_compiler[no-include-in-main,no-show-section]
#depend "[compiler]"

#section marcel_macros[no-include-in-main,no-show-section]
#depend "[macros]"
#depend "[marcel_compiler]"

#section marcel_types[no-include-in-main,no-show-section]
#depend "[types]"
#depend "[marcel_macros]"

#section marcel_structures[no-include-in-main,no-show-section]
#depend "[structures]"
#depend "[marcel_types]"

#section marcel_variables[no-include-in-main,no-show-section]
#depend "[variables]"
#depend "[marcel_structures]"

#section marcel_functions[no-include-in-main,no-show-section]
#depend "[functions]"
#depend "[marcel_structures]"

#section marcel_declarations[no-include-in-main,no-show-section]
#depend "[declarations]"
#depend "[marcel_variables]"
#depend "[marcel_functions]"

#section marcel_inline[no-include-in-main,no-show-section]
#depend "[inline]"
#depend "[marcel_declarations]"

#section null

