
/*
 * leonie.h
 * --------
 */

#ifndef __LEONIE_H
#define __LEONIE_H

/*
 * Checks
 * ------
 */
#ifndef __GNUC__
#error The GNU C Compiler is required to build this tool
#endif __GNUC__

#include "pm2debug.h"
#include "ntbx.h"
#include "tbx.h"
#include "leoparse.h"

/* Leonie: data structures */
#include "leo_pointers.h"
#include "leo_types.h"
#include "leo_main.h"

/* Leonie: internal interface */
#include "leo_net.h"
#include "leo_swann.h"
#include "leo_interface.h"

#endif /* __LEONIE_H */
