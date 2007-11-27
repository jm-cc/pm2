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

