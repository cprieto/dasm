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

#include "symbols.h"

#include "asm.h"
#include "errors.h"
#include "util.h"
#include "version.h"

#include <assert.h>
#include <limits.h>

/*@unused@*/
SVNTAG("$Id$");

#define SHASHSIZE   1024
#define SHASHAND    (SHASHSIZE-1) /*0x03FF*/

/*symbol hash table */
static SYMBOL *SHash[SHASHSIZE];

static SYMBOL org;
static SYMBOL special;
static SYMBOL specchk;

/* Name of symbol file if we should write one. */
/*@null@*/
static const char *symbol_file_name = NULL;

static unsigned int hash_symbol(const char *str, size_t len)
{
    return hash_string(str, len) & SHASHAND;
}

void setspecial(int value, int flags)
{
    special.value = value;
    special.flags = flags;
}

SYMBOL *findsymbol(const char *str, size_t len)
{
    unsigned int h1;
    SYMBOL *sym;
    char buf[MAX_SYM_LEN + 14];     /* historical */

    assert(str != NULL);
    assert(len > 0);
    
    if ( len > MAX_SYM_LEN )
        len = MAX_SYM_LEN;
    
    if (str[0] == '.')
    {
        if (len == 1)
        {
            if (Csegment->flags & SF_RORG)
            {
                org.flags = Csegment->rflags & SYM_UNKNOWN;
                org.value = Csegment->rorg;
            }
            else
            {
                org.flags = Csegment->flags & SYM_UNKNOWN;
                org.value = Csegment->org;
            }
            return &org;
        }
        if (len == 2 && str[1] == '.')
            return &special;
        if (len == 3 && str[1] == '.' && str[2] == '.')
        {
            specchk.flags = 0;
            specchk.value = CheckSum;
            return &specchk;
        }
        assert(len < INT_MAX);
        sprintf(buf, "%ld%.*s", Localindex, (int) len, str);
        len = strlen(buf);
        str = buf;
    }
    
    else if (str[len - 1] == '$')
    {
        assert(len < INT_MAX);
        sprintf(buf, "%ld$%.*s", Localdollarindex, (int) len, str);
        len = strlen(buf);
        str = buf;
    }
    
    h1 = hash_symbol(str, len);
    for (sym = SHash[h1]; sym != NULL; sym = sym->next)
    {
        if ((sym->namelen == len) && (memcmp(sym->name, str, len) == 0))
            break;
    }
    return sym;
}

SYMBOL *CreateSymbol(const char *str, size_t len)
{
    SYMBOL *sym;
    unsigned int h1;
    char buf[ MAX_SYM_LEN + 14 ];           /* historical */
    char *name;

    assert(str != NULL);
    assert(len > 0);

    if (len > MAX_SYM_LEN )
        len = MAX_SYM_LEN;
    
    if (str[0] == '.')
    {
        assert(len < INT_MAX);
        sprintf(buf, "%ld%.*s", Localindex, (int) len, str);
        len = strlen(buf);
        str = buf;
    }
    
    
    else if (str[len - 1] == '$')
    {
        assert(len < INT_MAX);
        sprintf(buf, "%ld$%.*s", Localdollarindex, (int) len, str);
        len = strlen(buf);
        str = buf;
    }
    
    sym = allocsymbol();
    name = small_alloc(len + 1);
    memcpy(name, str, len); /* small_alloc zeros the array for us */
    sym->name = name;
    sym->namelen = len;
    h1 = hash_symbol(str, len);
    sym->next = SHash[h1];
    sym->flags= SYM_UNKNOWN;
    SHash[h1] = sym;
    return sym;
}

/*
*  Label Support Routines
*/

void programlabel(void)
{
    size_t len;
    SYMBOL *sym;
    SEGMENT *cseg = Csegment;
    char *str;
    unsigned char rorg = cseg->flags & SF_RORG;
    unsigned char cflags = (rorg) ? cseg->rflags : cseg->flags;
    unsigned long   pc = (rorg) ? cseg->rorg : cseg->org;
    
    Plab = cseg->org;
    Pflags = cseg->flags;
    str = Av[0];
    if (*str == 0)
        return;
    len = strlen(str);


    if (str[len-1] == ':')
        --len;
    
    if (str[0] != '.' && str[len-1] != '$')
    {
        Lastlocaldollarindex++;
        Localdollarindex = Lastlocaldollarindex;
    }
    
    /*
    *	Redo:	unknown and referenced
    *		referenced and origin not known
    *		known and phase error	 (origin known)
    */
    
    if ((sym = findsymbol(str, len)) != NULL)
    {
        if ((sym->flags & (SYM_UNKNOWN|SYM_REF)) == (SYM_UNKNOWN|SYM_REF))
        {
            ++Redo;
            Redo_why |= REASON_FORWARD_REFERENCE;
            if (Xdebug)
                printf("redo 13: '%s' %04x %04x\n", sym->name, sym->flags, cflags);
        }
        else if ((cflags & SYM_UNKNOWN) && (sym->flags & SYM_REF))
        {
            ++Redo;
            Redo_why |= REASON_FORWARD_REFERENCE;
        }
        else if (!(cflags & SYM_UNKNOWN) && !(sym->flags & SYM_UNKNOWN))
        {
            if (pc != sym->value)
            {
                /*
                    If we had an unevaluated IF expression in the
                    previous pass, don't complain about phase errors
                    too loudly.
                */
                if (F_verbose >= 1 || !(Redo_if & (REASON_OBSCURE)))
                {
                    /* [phf] removed
                    char sBuffer[ MAX_SYM_LEN * 2 ];
                    sprintf( sBuffer, "%s %s", sym->name, sftos( sym->value, 0 ) );
                    */
                    /* TODO: the following was already removed before [phf]
                       started hacking, it looks like the way Andrew
                       put the error message together, some information
                       might be missing? need to check with Matt's DASM */
                    /*, sftos(sym->value,
                    sym->flags) ); , sftos(pc, cflags & 7));*/
                    /* [phf] removed:
                    asmerr( ERROR_LABEL_MISMATCH, false, sBuffer );
                    */
                    error_fmt("Label mismatch...\n --> %s %s",
                              sym->name, sftos(sym->value, 0));
                }
                ++Redo;
                Redo_why |= REASON_PHASE_ERROR;
            }
        }
    }
    else
    {
        sym = CreateSymbol( str, len );
    }
    sym->value = pc;
    sym->flags = (sym->flags & ~SYM_UNKNOWN) | (cflags & SYM_UNKNOWN);
}

/*
  Custom memory management for SYMBOLs.
*/

static SYMBOL *symbol_free_list = NULL;

SYMBOL *allocsymbol(void)
{
    SYMBOL *sym;

    if (symbol_free_list != NULL) {
        sym = symbol_free_list;
        symbol_free_list = symbol_free_list->next;
        memset(sym, 0, sizeof(SYMBOL));
    }
    else {
        sym = small_alloc(sizeof(SYMBOL));
    }

    return sym;
}

static void freesymbol(SYMBOL *sym)
{
    assert(sym != NULL);

    if ((sym->flags & SYM_STRING) != 0) {
        free(sym->string); /* TODO: really how we allocate those? [phf] */
    }
    sym->next = symbol_free_list;
    symbol_free_list = sym;
}

/* empty list okay to free */
void FreeSymbolList(SYMBOL *sym)
{
    SYMBOL *next;

    while (sym != NULL) {
        next = sym->next;
        freesymbol(sym);
        sym = next;
    }
}

void debug_symbol_hash_collisions(void)
{
    SYMBOL *sym;
    int sym_collisions = 0;
    int i;
    bool first;

    for (i = 0; i < SHASHSIZE; i++)
    {
      first = true;
      for (sym = SHash[i]; sym != NULL; sym = sym->next)
      {
        if (!first) {
          sym_collisions += 1;
        }
        first = false;
      }
    }

    printf("Collisions for SYMBOLS: %d\n", sym_collisions);
}

void clearrefs(void)
{
    SYMBOL *sym;
    size_t i;

    for (i = 0; i < SHASHSIZE; i++) {
        for (sym = SHash[i]; sym != NULL; sym = sym->next) {
            sym->flags &= ~SYM_REF;
        }
    }
}

static size_t CountUnresolvedSymbols(void)
{
    SYMBOL *sym;
    size_t nUnresolved = 0;
    size_t i;
    
    /* Pre-count unresolved symbols */
    for (i = 0; i < SHASHSIZE; i++) {
        for (sym = SHash[i]; sym != NULL; sym = sym->next) {
            if ((sym->flags & SYM_UNKNOWN) != 0) {
                nUnresolved++;
            }
        }
    }
            
	return nUnresolved;
}

size_t ShowUnresolvedSymbols(void)
{
    SYMBOL *sym;
    size_t i;
    
    size_t nUnresolved = CountUnresolvedSymbols();
    if (nUnresolved > 0)
    {
        printf("--- Unresolved Symbol List\n");
        
        for (i = 0; i < SHASHSIZE; i++) {
            for (sym = SHash[i]; sym != NULL; sym = sym->next) {
                if ((sym->flags & SYM_UNKNOWN) != 0) {
                    printf(
                        "%-24s %s\n",
                        sym->name,
                        sftos(sym->value, sym->flags)
                    );
                }
            }
        }
                
        printf(
            "--- %zu Unresolved Symbol%c\n\n",
            nUnresolved,
            (nUnresolved == 1) ? ' ' : 's'
        );
    }
    
    return nUnresolved;
}

static int CompareAlpha( const void *arg1, const void *arg2 )
{
    /* Simple alphabetic ordering comparison function for quicksort */

    const SYMBOL *sym1 = *(SYMBOL * const *) arg1;
    const SYMBOL *sym2 = *(SYMBOL * const *) arg2;
    
    /*
       The cast above is wild, thank goodness the Linux man page
       for qsort(3) has an example explaining it... :-) [phf]

       TODO: Note that we compare labels case-insensitive here which
       is not quite right; I believe we should be case-sensitive as
       in other contexts where symbols (labels) are compared. But
       the old CompareAlpha() was case-insensitive as well, so I
       didn't want to change that right now... [phf]
    */

    return strcasecmp(sym1->name, sym2->name);
}

static int CompareAddress( const void *arg1, const void *arg2 )
{
    /* Simple numeric ordering comparison function for quicksort */
    
    const SYMBOL *sym1 = *(SYMBOL * const *) arg1;
    const SYMBOL *sym2 = *(SYMBOL * const *) arg2;
    
    return sym1->value - sym2->value;
}

/*
  Display symbol table. Sorted if enough memory, unsorted otherwise.
  bTableSort true -> by address, false -> by name [phf]
*/
void ShowSymbols(FILE *file, bool bTableSort)
{
    SYMBOL **symArray;
    SYMBOL *sym;
    size_t i;
    size_t nSymbols = 0;

    fprintf(file, "--- Symbol List");

    /* First count the number of symbols */
    for (i = 0; i < SHASHSIZE; i++) {
        for (sym = SHash[i]; sym != NULL; sym = sym->next) {
            nSymbols++;
        }
    }
        
    /* Malloc an array of pointers to data */
    symArray = (SYMBOL**) malloc(nSymbols * sizeof(SYMBOL*));
    if (symArray == NULL) {
        fprintf(file, " (unsorted - not enough memory to sort!)\n");
            
        /* Display complete symbol table */
        for (i = 0; i < SHASHSIZE; i++) {
            for (sym = SHash[i]; sym != NULL; sym = sym->next) {
                fprintf(file, "%-24s %s\n", sym->name, sftos(sym->value, sym->flags));
            }
        }
    }
    else {
        size_t nPtr = 0;
         
        /* Copy the element pointers into the symbol array */
        for (i = 0; i < SHASHSIZE; i++) {
            for (sym = SHash[i]; sym != NULL; sym = sym->next) {
                symArray[nPtr++] = sym;
            }
        }
                
        if ( bTableSort ) {
            /* Sort via address */
            fprintf(file, " (sorted by address)\n");
            qsort(symArray, nPtr, sizeof(SYMBOL*), CompareAddress);
        }
        else {
            /* Sort via name */
            fprintf(file, " (sorted by symbol)\n");
            qsort(symArray, nPtr, sizeof(SYMBOL*), CompareAlpha);
        }
                
        /* Now display sorted list */
                
        for (i = 0; i < nPtr; i++) {
            /* TODO: format is different here that above [phf] */
            fprintf(file, "%-24s %-12s", symArray[i]->name, sftos(symArray[i]->value, symArray[i]->flags));

            if (symArray[i]->flags & SYM_STRING) {
                /* If a string, display actual string */
                /* TODO: we don't do this above? [phf] */
                fprintf(file, " \"%s\"", symArray[i]->string);
            }
            fprintf(file, "\n");
        }
                
        free(symArray);
    }
        
    fputs("--- End of Symbol List.\n", file);
}

/* Functions for writing symbol files. */

void set_symbol_file_name(const char *name)
{
    assert(symbol_file_name == NULL);
    assert(name != NULL);
    symbol_file_name = name;
}

void DumpSymbolTable(bool bTableSort)
{
    if (symbol_file_name != NULL)
    {
        FILE *fi = fopen(symbol_file_name, "w");
        if (fi != NULL) {
            ShowSymbols(fi, bTableSort);
            if (fclose(fi) != 0) {
                warning_fmt("Problem closing symbol file '%s'.\n",
                            symbol_file_name);
            }
        }
        else {
            warning_fmt("Unable to open symbol dump file '%s'.\n",
                        symbol_file_name);
        }
    }
}

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4 autoindent: */
