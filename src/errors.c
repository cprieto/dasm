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
#include "version.h"

/*@unused@*/
SVNTAG("$Id$");

#include <assert.h>
#include <stdio.h>

/* define the error table for asmerr() from errors.x */
#if defined(X)
#error infamous X macro already defined; aborting
#else
#define X(a,b,c) [a] = {a, b, c},
#endif
error_info_t sErrorDef[] =
{
#include "errors.x"
};
#undef X

/* TODO: globals that ended up here, need to be refactored eventually */
bool bStopAtEnd = false;
error_format_t F_error_format = ERRORFORMAT_DEFAULT;
error_level_t F_error_level = ERRORLEVEL_DEFAULT;
unsigned int nof_errors = 0;
unsigned int nof_warnings = 0;

/* TODO: another X macro hack? or just leave it? */
static const char *levels[] =
{
    [ERRORLEVEL_DEBUG] = "debug",
    [ERRORLEVEL_INFO] = "info",
    [ERRORLEVEL_NOTICE] = "notice",
    [ERRORLEVEL_WARNING] = "warning",
    [ERRORLEVEL_ERROR] = "error",
    [ERRORLEVEL_FATAL] = "fatal",
    [ERRORLEVEL_PANIC] = "***panic***",
};

void panic(const char *s)
{
  new_panic(ERROR_GENERIC_PANIC, s);
}

error_t asmerr(error_t err, bool bAbort, const char *sText)
{
    notify(err, bAbort ? ERRORLEVEL_FATAL : ERRORLEVEL_ERROR, sText);
    return EXIT_FAILURE;
}

/* TODO: original asmerr() just here for reference for now */
static error_t dasmerr(error_t err, bool bAbort, const char *sText);
static error_t dasmerr(error_t err, bool bAbort, const char *sText)
{
    const char *str;
    INCFILE *pincfile;
    /* file pointer we print error messages to */
    FILE *error_file = NULL;

    if ( err >= ERROR_MAX || err < 0 )
    {
        return asmerr( ERROR_BADERROR, true, "Bad error ERROR!" );
    }
    else
    {
        
        if (sErrorDef[err].bFatal)
            bStopAtEnd = true;
        
        for ( pincfile = pIncfile; pincfile->flags & INF_MACRO; pincfile=pincfile->next);
        str = sErrorDef[err].sDescription;

        /*
            New error format selection for 2.20.11 since some
            people *don't* use MS products. For historical
            reasons we currently send errors to stdout when
            they should really go to stderr, but we'll switch
            eventually I hope... [phf]
        */

        /* determine the file pointer to use */
        error_file = (F_listfile != NULL) ? FI_listfile : stdout;

        /* print first part of message, different formats offered */
        switch (F_error_format)
        {
            case ERRORFORMAT_WOE:
                /*
                    Error format for MS VisualStudio and relatives:
                    "file (line): error: string"
                */
                fprintf(error_file, "%s (%lu): error: ",
                        pincfile->name, pincfile->lineno);
                break;
            case ERRORFORMAT_DILLON:
                /*
                    Matthew Dillon's original format, except that
                    we don't distinguish writing to the terminal
                    from writing to the list file for now. Matt's
                    2.16 uses these:

                      "*line %4ld %-10s %s\n" (list file)
                      "line %4ld %-10s %s\n" (terminal)
                */
                fprintf(error_file, "line %7ld %-10s ",
                        pincfile->lineno, pincfile->name);
                break;
            case ERRORFORMAT_GNU:
                /*
                    GNU format error messages, from their coding
                    standards.
                */
                fprintf(error_file, "%s:%lu: error: ",
                        pincfile->name, pincfile->lineno);
                break;
            default:
                /* TODO: really panic here? [phf] */
                panic("Invalid error format, internal error!");
                break;
        }

        /* print second part of message, always the same for now */
        fprintf(error_file, str, sText ? sText : "");
        fprintf(error_file, "\n");
        
        if ( bAbort )
        {
            fprintf(error_file, "Aborting assembly\n");
            exit(EXIT_FAILURE);
        }
    }
    
    return err;
}

void notify(error_t _error, error_level_t level, const char *detail)
{
    /* normalized detail message */
    const char *msg = (detail != NULL) ? detail : "(no details)";
    /* file pointer we write the message to */
    FILE *out = (F_listfile != NULL) ? FI_listfile : stderr;
    /* include file we're in right now */
    INCFILE *file = pIncfile;
    /* level of severity description, grab from "levels" table later */
    const char *lev = NULL;

    assert(ERROR_NONE <= _error && _error < ERROR_MAX);
    assert(ERRORLEVEL_DEBUG <= level && level < ERRORLEVEL_MAX);

    if (level < F_error_level)
    {
        /* condition not severe enough */
        return;
    }

    lev = levels[level];

    /* find the file we're in */
    /* TODO: how does this work? why no NULL ptr check in original? */
    /* TODO: theory: find first non-macro, one is guaranteed to exist? */
    while (file != NULL && (file->flags & INF_MACRO))
    {
        file = file->next;
    }
    assert(file != NULL);

    /*
        New error format selection for 2.20.11 since some
        people *don't* use MS products. [phf]
    */

    /* print first part of message, different formats offered */
    switch (F_error_format)
    {
        case ERRORFORMAT_WOE:
            /*
                Error format for MS VisualStudio and relatives:
                "file (line): error: string"
            */
            fprintf(out, "%s (%lu): %s: ", file->name, file->lineno, lev);
            break;

        case ERRORFORMAT_DILLON:
            /*
                Matthew Dillon's original format, except that
                we don't distinguish writing to the terminal
                from writing to the list file for now. Matt's
                2.16 uses these:

                  "*line %4ld %-10s %s\n" (list file)
                  "line %4ld %-10s %s\n" (terminal)
            */
            fprintf(out, "line %7ld %-10s ", file->lineno, file->name);
            break;

        case ERRORFORMAT_GNU:
            /*
                GNU format error messages, from their coding
                standards.
            */
            fprintf(out, "%s:%lu: %s: ", file->name, file->lineno, lev);
            break;

        default:
            /* TODO: good idea? [phf] */
            assert(false);
            break;
    }

    /* print second part of message, always the same for now */
    fprintf(out, sErrorDef[_error].sDescription, msg);
    fprintf(out, "\n");

    /* maintain statistics about warnings and errors */
    /* TODO: count everything < PANIC? */
    if (level == ERRORLEVEL_WARNING)
    {
         nof_warnings += 1;
    }
    if (level == ERRORLEVEL_ERROR)
    {
         nof_errors +=1;
    }

    /* fatal and higher errors lead to (eventual) termination */
    if (level >= ERRORLEVEL_FATAL)
    {
        bStopAtEnd = true; /* stop after current pass */
    }
    if (level == ERRORLEVEL_PANIC)
    {
#if !defined(TEST)
/* hack for unit tests where we don't want to exit! */
        exit(EXIT_FAILURE); /* stop right now! */
#endif /* !defined(TEST) */
    }
}

/* helpers, could be inlined or macros or whatnot... */

void debug(error_t _error, const char *detail)
{
    notify(_error, ERRORLEVEL_DEBUG, detail);
}

void info(error_t _error, const char *detail)
{
    notify(_error, ERRORLEVEL_INFO, detail);
}

void notice(error_t _error, const char *detail)
{
    notify(_error, ERRORLEVEL_NOTICE, detail);
}

void warning(error_t _error, const char *detail)
{
    notify(_error, ERRORLEVEL_WARNING, detail);
}

void error(error_t _error, const char *detail)
{
    notify(_error, ERRORLEVEL_ERROR, detail);
}

void fatal(error_t _error, const char *detail)
{
    notify(_error, ERRORLEVEL_FATAL, detail);
}

void new_panic(error_t _error, const char *detail)
{
    notify(_error, ERRORLEVEL_PANIC, detail);
}

#if defined(TEST)
/* unit tests */
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
#endif /* defined(TEST) */

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4 autoindent: */
