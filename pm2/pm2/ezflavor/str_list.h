
#ifndef STR_LIST_IS_DEF
#define STR_LIST_IS_DEF

#include <glib.h>

char *string_new(char *str);

GList *string_list_from_parser(void);

void string_list_destroy(GList **l);

#endif
