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

#include <stdio.h>

/* define the error table for asmerr() from errors.x */
#if defined(X)
#error infamous X macro already defined; aborting
#else
#define X(a,b,c) {a, b, c},
#endif
ERROR_DEFINITION sErrorDef[] =
{
#include "errors.x"
};
#undef X

bool bStopAtEnd = false;

error_format_t F_error_format = ERRORFORMAT_DEFAULT;

void panic(const char *s)
{
    (void) printf("Panic: %s\n", s);
#if !defined(TEST)
/* hack for unit tests where we don't want to exit! */
    exit(EXIT_FAILURE);
#endif /* !defined(TEST) */
}

error_t asmerr(error_t err, bool bAbort, const char *sText)
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

