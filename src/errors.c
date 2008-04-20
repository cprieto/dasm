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

/*
    TODO: I may have made a big mistake in trying to cram everything
    DASM has to say into one single interface called notify(). There
    *is* a difference between DASM errors (like out of memory, can't
    open file, etc.) and ASSEMBLY errors (like wrong opcodes, can't
    open INCLUDE file, etc.) which comes down to whether there is a
    current file or not. Right now I have to "fake" things when we
    don't have a current file, but I am not sure that's good. Maybe
    there should be *one* function to deal with DASM errors, and
    *another* to deal with ASSEMBLY erros... DASM errors don't need
    (for the most part) codes either, they can just take printf()
    form (e.g. eprintf/dprintf/iprintf whatnot in TPOP sense).
*/

#include "errors.h"

#include "asm.h"
#include "version.h"

/*@unused@*/
SVNTAG("$Id$");

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

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
char source_location_buffer[SOURCE_LOCATION_LENGTH];

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

    /* TODO: fixed range checking for -Wextra below, but what if
       enum is *not* unsigned in other compilers? hmmm... [phf] */
    if ( err >= ERROR_MAX /*|| err < 0*/ )
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

/* helper to print the first part of an error message */
static void print_part_one(FILE *out, const INCFILE *file, const char *level)
{
    /*
        New error format selection for 2.20.11 since some
        people *don't* use MS products. [phf]
    */

    /*
        TODO: I simply replaced "error" with the current level,
        not sure that works for WOE? Let's check... GNU is fine
        btw, doesn't requite "error" in the message... [phf]
    */

    /*
        TODO: What if we're to produce an error message before
        there's even one file open, so we have no INCFILE? We
        need a different format for that, even if just in case!
        Error handling should not depend purely on source code
        analysis, right? Command line options come to mind... [phf]
    */

    switch (F_error_format)
    {
        case ERRORFORMAT_WOE:
            /*
                Error format for MS VisualStudio and relatives:
                "file (line): error: string"
            */
            if (file != NULL)
            {
                fprintf(out, "%s (%lu): %s: ", file->name, file->lineno, level);
            }
            else
            {
                fprintf(out, "%s: ", level);
            }
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
            if (file != NULL)
            {
                fprintf(out, "line %7ld %-10s ", file->lineno, file->name);
            }
            else
            {
                /* nothing to print in this case... */
            }
            break;

        case ERRORFORMAT_GNU:
            /*
                GNU format error messages, from their coding
                standards: "source-file-name:lineno: message"
            */
            if (file != NULL)
            {
                fprintf(out, "%s:%lu: %s: ", file->name, file->lineno, level);
            }
            else
            {
                fprintf(out, "dasm: %s: ", level);
            }
            break;

        default:
            /* TODO: good idea? [phf] */
            assert(false);
            break;
    }
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

    /* TODO: fixed range checking for -Wextra below, but what if
       enum is *not* unsigned in other compilers? hmmm... [phf] */
    assert(/*ERROR_NONE <= _error &&*/ _error < ERROR_MAX);
    assert(/*ERRORLEVEL_DEBUG <= level &&*/ level < ERRORLEVEL_MAX);

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
    /*
        took this out to accomodate the fact that there might not be
        a file yet... [phf]
    */
    /*assert(file != NULL);*/

    /* print first part of message, different formats offered */
    print_part_one(out, file, lev);

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
        exit(EXIT_FAILURE); /* stop right now! */
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

/****** NEW SHIT *******/

#define NOTIFY_BUFFER_SIZE 1024
static char notify_buffer[NOTIFY_BUFFER_SIZE];

static void vnotify(error_level_t level, const char *fmt, va_list ap)
{
    /*
        used to call vasprintf()/free() but we switched to a
        global buffer with less overhead for this frequently
        called function [phf]
    */
    const int max = NOTIFY_BUFFER_SIZE;

    if (level < F_error_level) { return; }

    if (vsnprintf(notify_buffer, max, fmt, ap) >= max)
    {
        panic("Buffer overflow in vnotify()!");
    }

    notify(ERROR_GENERIC_DEBUG, level, notify_buffer);
}

/* avoid code replication through macros, sweet [phf] */
#define IMPLEMENT_FMT(level) \
    va_list ap; \
    va_start(ap, fmt); \
    vnotify(level, fmt, ap); \
    va_end(ap)
#define DEFINE_FMT(name, level) \
void name(const char *fmt, ...) \
{ \
    IMPLEMENT_FMT(ERRORLEVEL_DEBUG); \
}

void notify_fmt(error_level_t level, const char *fmt, ...)
{
    IMPLEMENT_FMT(level);
}

DEFINE_FMT(debug_fmt, ERRORLEVEL_DEBUG)
DEFINE_FMT(info_fmt, ERRORLEVEL_INFO)
DEFINE_FMT(notice_fmt, ERRORLEVEL_NOTICE)
DEFINE_FMT(warning_fmt, ERRORLEVEL_WARNING)
DEFINE_FMT(error_fmt, ERRORLEVEL_ERROR)
DEFINE_FMT(fatal_fmt, ERRORLEVEL_FATAL)
DEFINE_FMT(panic_fmt, ERRORLEVEL_PANIC)

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4 autoindent: */
