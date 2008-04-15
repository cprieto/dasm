#ifndef _DASM_ERRORS_H
#define _DASM_ERRORS_H

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

#include <stdbool.h>

/**
 * @file errors.h
 * @brief Error handling for DASM.
 */

/**
 * @brief Format of error messages for -E option.
 */
typedef enum
{
    ERRORFORMAT_DEFAULT,
    ERRORFORMAT_WOE = ERRORFORMAT_DEFAULT,
    ERRORFORMAT_DILLON,
    ERRORFORMAT_GNU,
    ERRORFORMAT_MAX
}
error_format_t;

/**
 * @brief Global that holds current error format for -E option.
 */
extern error_format_t F_error_format;

/**
 * @brief Global that tells us whether to stop after the current
 * pass? not sure [phf]
 *
 * @todo somehow get rid of this... :-/
 */
extern bool bStopAtEnd;

/**
 * @brief Severity of error messages.
 * @todo not used so far... :-/
 */
typedef enum
{
    /* displayed in debug mode, -d option */
    ERRORLEVEL_DEBUG,
    /* displayed in high verbose mode, -v option */
    ERRORLEVEL_INFO,
    /* displayed in low verbose mode, -v option */
    ERRORLEVEL_NOTICE,
    /* displayed if warnings enabled, -w option */
    ERRORLEVEL_DEFAULT,
    ERRORLEVEL_WARNING = ERRORLEVEL_DEFAULT,
    /* regular error, always displayed, assembly continues */
    ERRORLEVEL_ERROR,
    /* fatal error, always displayed, assembly stops */
    ERRORLEVEL_FATAL,
    /* panic insanity, always displayed, breaks out right away */
    ERRORLEVEL_PANIC,
    /* end of severity levels enum */
    ERRORLEVEL_MAX
}
error_level_t;

/* define the error codes for asmerr() from errors.x */
#if defined(X)
#error infamous X macro already defined; aborting
#else
#define X(a,b,c) a,
#endif
/**
 * @brief Error codes for DASM.
 */
typedef enum
{
#include "errors.x"
}
error_t;
#undef X

/**
 * @brief Severity and message for each error code.
 * @todo refactor! don't need error code again, severity enum type
 */
typedef struct
{
    /* ASM_ERROR_EQUATES value */
    error_t nErrorType;
    /* 0 = OK, non-zero = cannot continue compilation */
    bool bFatal;
    /* Error message */
    const char *sDescription;
}
ERROR_DEFINITION;

/**
 * @todo temporarily exported to get main.c to compile...
 */
extern ERROR_DEFINITION sErrorDef[];

/**
 * @brief An insane problem occurred, print message and terminate
 * DASM with EXIT_FAILURE immediately.
 *
 * @warning You really don't want to call this. Not ever. We should
 * have REAL error handling instead.
 */

void panic(const char *str);

/**
 * @brief Backwards compatible wrapper around new error handling
 * framework.
 *
 * @todo phase out...
 */

error_t asmerr(error_t err, bool bAbort, const char *sText);

#endif /* _DASM_ERRORS_H */

/* vim: set syntax=c tabstop=4 softtabstop=4 expandtab shiftwidth=4 autoindent: */
