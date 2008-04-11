#ifndef _DASM_UTIL_H
#define _DASM_UTIL_H

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

/**
 * @file util.h
 * @brief Utility functions for string manipulation and memory allocation.
 */

/**
 * @brief A fatal error occurred, print a message and terminate process
 * with EXIT_FAILURE.
 *
 * @warning You really don't want to call this. Not ever. We should have
 * REAL error handling instead.
 */

void panic(const char *str);

/**
 * @brief Wrapper for malloc(3) that terminates DASM with panic() if no
 * memory is available.
 */

/*@null@*/
void *checked_malloc(size_t bytes);

/**
 * @brief Wrapper for checked_malloc() that zero's the allocated chunk
 * of memory.
 *
 * @warning Uses memset(3) internally, which may not initialize pointers
 * or floats/doubles correctly to NULL or 0.0 on some (strange) machines.
 */

/*@null@*/
void *zero_malloc(size_t bytes);

/**
 * @brief A hash function for strings.
 */

unsigned int hash_string(const char *string, size_t length);

/**
 * @brief Convert string to lower case, destructively.
 */

char *strlower(char *str);

#if !defined(__APPLE__) && !defined(__BSD__)

/**
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 *
 * @warning
 *   On BSD (including OS X) this function is defined in the C library!
 */

size_t strlcat(char *dst, const char *src, size_t siz);

/**
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 *
 * @warning
 *   On BSD (including OS X) this function is defined in the C library!
 */

size_t strlcpy(char *dst, const char *src, size_t siz);

#endif /* !defined(__APPLE__) && !defined(__BSD__) */

#endif /* _DASM_UTIL_H */

