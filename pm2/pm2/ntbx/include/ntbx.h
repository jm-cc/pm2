
/*
 * NTbx.h
 * ======
 */

#ifndef NTBX_H
#define NTBX_H

/*
 * Checks
 * ------
 */
#ifndef __GNUC__
#error The GNU C Compiler is required to build the library
#endif __GNUC__

/*
 * Headers
 * -------
 */
#ifdef MARCEL
#include "marcel.h"
#endif /* MARCEL */
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "tbx.h"
#include "ntbx_types.h"
#include "ntbx_net_commands.h"

#ifdef NTBX_TCP
#include "ntbx_tcp.h"
#endif /* NTBX_TCP */

#include "ntbx_interface.h"

#endif /* NTBX_H */
