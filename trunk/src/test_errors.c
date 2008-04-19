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

#include "errors.h"
#include "asm.h"

/**
 * @file test_errors.c
 * @brief Unit tests for errors module.
 * @todo how do we keep panic* from aborting while unit test
 * runs? or maybe it doesn't matter if we switch to Check?
 */

/* fakes for unit test */
FILE *FI_listfile = NULL;
char *F_listfile = NULL;
INCFILE *pIncfile = NULL;

int main(void)
{
  /* fake a current file */
  pIncfile = malloc(sizeof(INCFILE));
  pIncfile->next = NULL;
  pIncfile->name = "someFileName";
  pIncfile->lineno = 47;
  /* enable all messages */
  F_error_level = ERRORLEVEL_DEBUG;
  /* test SUPER new API :-) */
  debug_fmt(MESSAGE_RANGE, "CRAZY", "DEBUG", 2, 14);
  error_fmt(MESSAGE_RANGE, "CRAZY", NULL, 2, 14);
  fatal_fmt(MESSAGE_RANGE, "CRAZY", "FATAL", 2, 14);
  panic_fmt(MESSAGE_RANGE, "CRAZY", NULL, 2, 14);
  /* test new API */
  notify(ERROR_PROCESSOR_NOT_SUPPORTED, ERRORLEVEL_ERROR, "new API notify()");
  debug(ERROR_PROCESSOR_NOT_SUPPORTED, "new API debug()");
  info(ERROR_PROCESSOR_NOT_SUPPORTED, "new API info()");
  notice(ERROR_PROCESSOR_NOT_SUPPORTED, "new API notice()");
  warning(ERROR_PROCESSOR_NOT_SUPPORTED, "new API warning()");
  error(ERROR_PROCESSOR_NOT_SUPPORTED, "new API error()");
  fatal(ERROR_PROCESSOR_NOT_SUPPORTED, "new API fatal()");
  new_panic(ERROR_PROCESSOR_NOT_SUPPORTED, "new API panic()");
  /* test wrappers for old API */
  panic("old API panic()");
  asmerr(ERROR_PROCESSOR_NOT_SUPPORTED, true, "old API asmerr()");
  return EXIT_SUCCESS;
}

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4 autoindent: */
