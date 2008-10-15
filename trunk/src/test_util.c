/*
    $Id$

    the DASM macro assembler (aka small systems cross assembler)

    Copyright (c) 1988-2002 by Matthew Dillon.
    Copyright (c) 1995 by Olaf "Rhialto" Seibert.
    Copyright (c) 2003-2008 by Andrew Davie.
    Copyright (c) 2008 by Peter H. Froehlich.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
 
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "util.h"
#include "errors.h"
#include "asm.h"

#include <assert.h>

/**
 * @file test_errors.c
 * @brief Unit tests for errors module.
 * @todo how do we keep panic* from aborting while unit test
 * runs? or maybe it doesn't matter if we switch to Check?
 */

#if 0
#if defined(TEST)
/* hack for unit tests where we don't want to exit! */
void new_panic(error_t _error, const char *s)
{
    (void) printf("Simulated Panic: %s\n", s);
}
void debug(error_t _error, const char *s)
{
    (void) printf("Simulated Debug: %s\n", s);
}
#endif /* defined(TEST) */
#endif

/* fakes for unit test */
FILE *FI_listfile = NULL;
char *F_listfile = NULL;
INCFILE *pIncfile = NULL;

int main(int argc, char *argv[])
{
    char *one;
    char *two;
    union align { long l; void *p; void (*fp)(void); };
    const unsigned int BIG = ((unsigned) 1) << 31;
    setprogname(argv[0]);
    /* fake a current file */
    pIncfile = malloc(sizeof(INCFILE));
    pIncfile->next = NULL;
    pIncfile->name = strdup("someFileName");
    pIncfile->lineno = 47;
    /* enable all messages */
    F_error_level = ERRORLEVEL_DEBUG;
    /* the tests */
    puts(getprogname());
    setprogname(argv[0]);
    puts(getprogname());
    printf("sizeof(align)==%zu\n", sizeof(union align));
    one = checked_malloc(BIG);
    puts("First malloc()ed!");
    two = checked_malloc(BIG);
    puts("Second malloc()ed! Should have caused a panic?");
    if (two) free(two);
    puts("Second free()d!");
    if (one) free(one);
    puts("First free()d!");
    one = small_alloc(4096);
    printf("First small_alloc()ed returned %p!\n", one);
    two = small_alloc(4096);
    assert(one < two);
    printf("Second small_alloc()ed returned %p!\n", two);
    one = small_alloc(10240);
    small_free_all();
    return EXIT_SUCCESS;
}

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4 autoindent: */
