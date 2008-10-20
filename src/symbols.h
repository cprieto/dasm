#ifndef _DASM_SYMBOLS_H
#define _DASM_SYMBOLS_H

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

#include "asm.h"

#include <stdbool.h>

void  setspecial(int value, int flags);
SYMBOL *allocsymbol(void);
SYMBOL *findsymbol(const char *str, int len);
SYMBOL *CreateSymbol(const char *str, int len);
void FreeSymbolList(SYMBOL *sym);
void programlabel(void);
void debug_symbol_hash_collisions(void);
void clearrefs(void);
void ShowSymbols(FILE *file, bool bTableSort);
size_t ShowUnresolvedSymbols(void);

#endif /* _DASM_SYMBOLS_H */

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4 autoindent: */
