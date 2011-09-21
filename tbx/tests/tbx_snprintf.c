/* Copyright (C) 1991, 1995, 1997, 1998, 2004, 2006
   Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */


/** Concat of:
 *  - glibc-2.14/stdio-common/tst-sprintf.c
 *  - glibc-2.14/stdio-common/tst-sprintf2.c
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include "tbx.h"


#define BUFSZ 100


/** Check tbx version of s(n)printf */
#define sprintf(buf, ...) tbx_sprintf(buf, __VA_ARGS__)
#define snprintf(buf, size, ...) tbx_snprintf(buf, size, __VA_ARGS__)


static int tst_gnu_sprintf(void)
{
	char buf[100];
	int result = 0;

	if (sprintf (buf, "%.0ls", L"foo") != 0
	    || strlen (buf) != 0)
	{
		puts ("sprintf (buf, \"%.0ls\", L\"foo\") produced some output");
		result = 1;
	}
	
#define SIZE (1024*70000)
#define STR(x) #x

	char *dst = malloc (SIZE + 1);

	if (dst == NULL)
	{
		puts ("memory allocation failure");
		result = 1;
	}
	else
	{
		sprintf (dst, "%*s", SIZE, "");
		if (strnlen (dst, SIZE + 1) != SIZE)
		{
			puts ("sprintf (dst, \"%*s\", " STR(SIZE) ", \"\") did not produce enough output");
			result = 1;
		}
		free (dst);
	}
	
#if 0
	if (sprintf (buf, "%1$d%3$.*2$s%4$d", 7, 67108863, "x", 8) != 3
	    || strcmp (buf, "7x8") != 0)
	{
		printf ("sprintf (buf, \"%%1$d%%3$.*2$s%%4$d\", 7, 67108863, \"x\", 8) produced `%s' output", buf);
		result = 1;
	}

	if (sprintf (buf, "%67108863.16\"%d", 7) != 14
	    || strcmp (buf, "%67108863.16\"7") != 0)
	{
		printf ("sprintf (buf, \"%%67108863.16\\\"%%d\", 7) produced `%s' output", buf);
		result = 1;
	}

	if (sprintf (buf, "%*\"%d", 0x3ffffff, 7) != 11
	    || strcmp (buf, "%67108863\"7") != 0)
	{
		printf ("sprintf (buf, \"%%*\\\"%%d\", 0x3ffffff, 7) produced `%s' output", buf);
		result = 1;
	}
#endif

	return result;
}


static int tst_gnu_sprintf2(void)
{
#if LDBL_MANT_DIG >= 106
	volatile union { long double l; long long x[2]; } u, v;
	char buf[64];
#endif
	int result = 0;

#if LDBL_MANT_DIG == 106 || LDBL_MANT_DIG == 113
#  define COMPARE_LDBL(u, v)						\
	((u).l == (v).l && (u).x[0] == (v).x[0] && (u).x[1] == (v).x[1])
#else
#  define COMPARE_LDBL(u, v) ((u).l == (v).l)
#endif

#define TEST(val)							\
	do								\
	{								\
		u.l = (val);						\
		snprintf (buf, sizeof buf, "%LaL", u.l);		\
		if (strcmp (buf, #val) != 0)				\
		{							\
			printf ("Error on line %d: %s != %s\n", __LINE__, buf, #val); \
			result = 1;					\
		}							\
		if (sscanf (#val, "%La", &v.l) != 1 || !COMPARE_LDBL (u, v)) \
		{							\
			printf ("Error sscanf on line %d: %La != %La\n", __LINE__, \
				u.l, v.l);				\
			result = 1;					\
		}							\
		/* printf ("%s %La %016Lx %016Lx\n", #val, u.l, u.x[0], u.x[1]); */ \
	}								\
	while (0)

#if LDBL_MANT_DIG >= 106
#  if LDBL_MANT_DIG == 106
	TEST (0x0.ffffffffffffp-1022L);
	TEST (0x0.ffffffffffff1p-1022L);
	TEST (0x0.fffffffffffffp-1022L);
#  endif
	TEST (0x1p-1022L);
	TEST (0x1.0000000000001p-1022L);
	TEST (0x1.00000000001e7p-1022L);
	TEST (0x1.fffffffffffffp-1022L);
	TEST (0x1p-1021L);
	TEST (0x1.00000000000008p-1021L);
	TEST (0x1.0000000000001p-1021L);
	TEST (0x1.00000000000018p-1021L);
	TEST (0x1.0000000000000f8p-1017L);
	TEST (0x1.0000000000001p-1017L);
	TEST (0x1.000000000000108p-1017L);
	TEST (0x1.000000000000dcf8p-1013L);
	TEST (0x1.000000000000ddp-1013L);
	TEST (0x1.000000000000dd08p-1013L);
	TEST (0x1.ffffffffffffffffffffffffffp-1L);
	TEST (0x1.ffffffffffffffffffffffffff8p-1L);
	TEST (0x1p+0L);
	TEST (0x1.000000000000000000000000008p+0L);
	TEST (0x1.00000000000000000000000001p+0L);
	TEST (0x1.000000000000000000000000018p+0L);
	TEST (0x1.23456789abcdef123456789abc8p+0L);
	TEST (0x1.23456789abcde7123456789abc8p+0L);
	TEST (0x1.23456789abcdef123456789abc8p+64L);
	TEST (0x1.23456789abcde7123456789abc8p+64L);
	TEST (0x1.123456789abcdef123456789p-969L);
#  if LDBL_MANT_DIG == 106
	TEST (-0x1.2d71957cc1263bbbeb1d365f1e8p-969L);
	TEST (0x1.23456789abcdef0123456789abp-970L);
	TEST (0x1.579bde02468acp-1001L);
	TEST (0x0.abcdef0123456p-1022L);
	TEST (0x1.abcdef0123456p-1022L);
	TEST (0x1.abcdef012345678p-1014L);
	TEST (0x1.abcdef0123456f8p-1014L);
#  endif
#endif
	return result;
}


#undef sprintf
#undef snprintf

static int cmp_snprintf_result(char *s, ...)
{
	va_list glibc_args;
	char glibc_output[BUFSZ];
	int glibc_state;

	va_list tbx_args;
	char tbx_output[BUFSZ];
	int tbx_state;

	va_start(glibc_args, s);
	va_copy(tbx_args, glibc_args);

	tbx_state = tbx_vsnprintf(tbx_output, BUFSZ, s, tbx_args);
	va_end(tbx_args);

	glibc_state = vsnprintf(glibc_output, BUFSZ, s, glibc_args);
	va_end(glibc_args);

	if ((tbx_state == glibc_state) && (!strcmp(tbx_output, glibc_output)))
		return 1;

	return 0;
}

static int tst_snprintf_integers(void)
{
	if (! cmp_snprintf_result("%d", 100000000) ||
	    ! cmp_snprintf_result("%d", -10000000) ||
	    ! cmp_snprintf_result("%x", 100000000) ||
	    ! cmp_snprintf_result("%x", -10000000) ||
	    ! cmp_snprintf_result("%X", 100000000) ||
	    ! cmp_snprintf_result("%X", -10000000) ||
	    ! cmp_snprintf_result("%u", 100000000) ||
	    ! cmp_snprintf_result("%u", -10000000) ||
	    ! cmp_snprintf_result("%o", 100000000) ||
	    ! cmp_snprintf_result("%o", -10000000) ||
	    ! cmp_snprintf_result("%c", 100000000) ||
	    ! cmp_snprintf_result("%c", -10000000) ||
	    ! cmp_snprintf_result("%p", snprintf))
		return EXIT_FAILURE;
		
	return EXIT_SUCCESS;
}


static int tst_snprintf_format(void)
{
	if (! cmp_snprintf_result("%-15c", 'a')    ||
	    ! cmp_snprintf_result("%*c", -15, 'a') ||
	    ! cmp_snprintf_result("%15c", 'a')     ||
	    ! cmp_snprintf_result("%+15c", 'a')    ||
	    ! cmp_snprintf_result("%*c", 15, 'a')  ||
	    ! cmp_snprintf_result("%-15s", "a")    ||
	    ! cmp_snprintf_result("%*s", -15, "a") ||
	    ! cmp_snprintf_result("%15s", "a")     ||
	    ! cmp_snprintf_result("%+15s", "a")    ||
	    ! cmp_snprintf_result("%*s", 15, "a")  ||
	    ! cmp_snprintf_result("%0d", 1000000)  ||
	    ! cmp_snprintf_result("%-0d", 100000)  ||
	    ! cmp_snprintf_result("% d", 100000)   ||
	    ! cmp_snprintf_result("% d", -10000)   ||
	    ! cmp_snprintf_result("%#o", 0100)     ||
	    ! cmp_snprintf_result("%#o", 100)      ||
	    ! cmp_snprintf_result("%#x", 0x100)    ||
	    ! cmp_snprintf_result("%#X", 0x100)    ||
	    ! cmp_snprintf_result("%#X", 100))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}


static int tst_snprintf_misc(void)
{
	char buf[BUFSZ];

	memset(buf, 63, BUFSZ);
	if (! cmp_snprintf_result("%s", buf) ||
	    ! cmp_snprintf_result(""))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}


int main()
{
	if (EXIT_FAILURE == tst_gnu_sprintf()) {
		fprintf(stderr, "tst_gnu_snprintf: FAIL\n");
		return EXIT_FAILURE;
	} else
		printf("tst_gnu_snprintf: PASS\n");

	if (EXIT_FAILURE == tst_gnu_sprintf2()) {
		fprintf(stderr, "tst_gnu_snprintf2: FAIL\n");
		return EXIT_FAILURE;
	} else
		printf("tst_gnu_snprintf2: PASS\n");

	if (EXIT_FAILURE == tst_snprintf_integers()) {
		fprintf(stderr, "tst_snprintf_integers: FAIL\n");
		return EXIT_FAILURE;
	} else
		printf("tst_snprintf_integers: PASS\n");

	if (EXIT_FAILURE == tst_snprintf_format()) {
		fprintf(stderr, "tst_snprintf_format: FAIL\n");
		return EXIT_FAILURE;
	} else
		printf("tst_snprintf_format: PASS\n");

	if (EXIT_FAILURE == tst_snprintf_misc()) {
		fprintf(stderr, "tst_snprintf_misc: FAIL\n");
		return EXIT_FAILURE;
	} else
		printf("tst_snprintf_misc: PASS\n");

	return EXIT_SUCCESS;
}
