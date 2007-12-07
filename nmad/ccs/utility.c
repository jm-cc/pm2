/*
 *  (C) 2006 by Argonne National Laboratory.
 *   Contact:  Darius Buntinas
 */
#include "ccsi.h"

void *
CCS_malloc (int size)
{
    return malloc (size);
}

void
CCS_free (void *p)
{
    free (p);
}

