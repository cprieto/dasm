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
 *  OPS.C
 *
 *  Handle mnemonics and pseudo ops
 */

#include "asm.h"
#include "errors.h"
#include "symbols.h"
#include "util.h"
#include "version.h"

#include <assert.h>
#include <ctype.h>

/*@unused@*/
SVNTAG("$Id$");

unsigned char Gen[256];
static unsigned char OrgFill = DEFORGFILL;
int	 Glen;

extern MNEMONIC    Mne6502[];
extern MNEMONIC    Mne6803[];
extern MNEMONIC    MneHD6303[];
extern MNEMONIC    Mne68705[];
extern MNEMONIC    Mne68HC11[];
extern MNEMONIC    MneF8[];

static void genfill(int32_t fill, long bytes, int size);
static void pushif(bool xbool);
static int gethexdig(int c);

/*
*  An opcode modifies the SEGMENT flags in the following ways:
*/

void v_processor(const char *str, MNEMONIC *dummy)
{
    static bool bCalled = false;
    static unsigned long Processor = 0;
    unsigned long PreviousProcessor = Processor;

    assert(str != NULL);

    Processor = 0;

    if (strcmp(str,"6502") == 0) {
        if (!bCalled) {
            addhashtable(Mne6502);
        }
        MsbOrder = false;	    /*	lsb,msb */
        Processor = 6502;
    }

    if (strcmp(str,"6803") == 0) {
        if (!bCalled) {
            addhashtable(Mne6803);
        }
        MsbOrder = true;	    /*	msb,lsb */
        Processor = 6803;
    }

    if (match_either_case(str, "HD6303")) {
        if (!bCalled) {
            addhashtable(Mne6803);
            addhashtable(MneHD6303);
        }
        MsbOrder = true;	    /*	msb,lsb */
        Processor = 6303;
    }

    if (strcmp(str,"68705") == 0) {
        if (!bCalled) {
            addhashtable(Mne68705);
        }
        MsbOrder = true;	    /*	msb,lsb */
        Processor = 68705;
    }

    if (match_either_case(str, "68HC11")) {
        if (!bCalled) {
            addhashtable(Mne68HC11);
        }
        MsbOrder = true;	    /*	msb,lsb */
        Processor = 6811;
    }

    if (match_either_case(str, "F8")) {
		if (!bCalled) {
			addhashtable(MneF8);
        }
		MsbOrder = true;
        Processor = 0xf8;
    }

    bCalled = true;

    if (Processor == 0) {
        fatal_fmt("Processor '%s' not supported!", str);
    }

    if (PreviousProcessor != 0 && Processor != PreviousProcessor) {
        fatal_fmt("Only one processor type may be selected!");
    }
}

#define badcode(mne,adrmode)  (!(mne->okmask & (1L << adrmode)))

void v_mnemonic(const char *str, MNEMONIC *mne)
{
    int addrmode;
    SYMBOL *sym;
    unsigned int opcode;
    short opidx;
    SYMBOL *symbase;
    unsigned int opsize;

    assert(str != NULL);
    assert(mne != NULL);
    
    Csegment->flags |= SF_REF;
    programlabel();
    symbase = eval(str, true);
    
    if (bTrace) {
        printf("PC: %04lx  MNEMONIC: %s  addrmode: %d  ", Csegment->org, mne->name, symbase->addrmode);
    }

    for (sym = symbase; sym != NULL; sym = sym->next) {
        if ((sym->flags & SYM_UNKNOWN) != 0) {
            ++Redo;
            Redo_why |= REASON_MNEMONIC_NOT_RESOLVED;
        }
    }
    sym = symbase;
    
    if ((mne->flags & MF_IMOD) != 0) {
        if (sym->next != NULL) {
            sym->addrmode = AM_BITMOD;
            if ((mne->flags & MF_REL) != 0 && sym->next != NULL) {
                sym->addrmode = AM_BITBRAMOD;
            }
        }
    }
    addrmode = sym->addrmode;
    if ((sym->flags & SYM_UNKNOWN) != 0 || sym->value >= 0x100) {
        opsize = 2;
    }
    else {
        opsize = (sym->value != 0) ? 1 : 0;
    }
    
    while (badcode(mne,addrmode) && convert_address_mode(addrmode) != 0) {
        addrmode = convert_address_mode(addrmode);
    }
    
    if (bTrace) {
        printf("mnemask: %08lx adrmode: %d  Cvt[am]: %d\n", mne->okmask, addrmode, convert_address_mode(addrmode));
    }
    
    if (badcode(mne,addrmode)) {
        /* [phf] removed
        char sBuffer[128];
        sprintf( sBuffer, "%s %s", mne->name, str );
        asmerr( ERROR_ILLEGAL_ADDRESSING_MODE, false, sBuffer );
        */
        error_fmt("Invalid addressing mode '%s %s'.", mne->name, str);
        FreeSymbolList(symbase);
        return;
    }
    
    if (Mnext >= 0 && Mnext < NUMOC) {           /*	Force	*/
        addrmode = Mnext;
        if (badcode(mne,addrmode)) {
            /* [phf] removed
            asmerr( ERROR_ILLEGAL_FORCED_ADDRESSING_MODE, false, mne->name );
            */
            error_fmt("Invalid forced addressing mode on '%s'.", mne->name);
            FreeSymbolList(symbase);
            return;
        }
    }
    
    if (bTrace) {
        printf("final addrmode = %d\n", addrmode);
    }
    
    while (opsize > operand_size(addrmode)) {
        if (convert_address_mode(addrmode) == 0 || badcode(mne,convert_address_mode(addrmode)))
        {
            /* [phf] removed
            char sBuffer[128];
            */
            
            if ((sym->flags & SYM_UNKNOWN) != 0) {
                break;
            }
            
            /* [phf] removed
            sprintf( sBuffer, "%s %s", mne->name, str );
            asmerr( ERROR_ADDRESS_MUST_BE_LT_100, false, sBuffer );
            */
            error_fmt(ERROR_ADDRESS_RANGE_DETAIL, str, mne->name, 0, 255);
            break;
        }
        addrmode = convert_address_mode(addrmode);
    }
    opcode = mne->opcode[addrmode];
    opidx = 1 + (opcode > 0xFF);
    if (opidx == 2) {
        Gen[0] = opcode >> 8;
        Gen[1] = opcode;
    }
    else {
        Gen[0] = opcode;
    }
    
    switch(addrmode)
    {
    case AM_BITMOD:
        sym = symbase->next;
        if ((sym->flags & SYM_UNKNOWN) == 0 && sym->value >= 0x100) {
            /* [phf] removed
            asmerr( ERROR_ADDRESS_MUST_BE_LT_100, false, NULL );
            */
            error_fmt(ERROR_ADDRESS_RANGE, 0, 255);
            /* TODO: why no detail in original? */
        }
        Gen[opidx++] = sym->value;
        
        if ((symbase->flags & SYM_UNKNOWN) == 0) {
            if (symbase->value > 7) {
                /* [phf] removed
                asmerr( ERROR_ILLEGAL_BIT_SPECIFICATION, false, str );
                */
                error_fmt(ERROR_INVALID_BIT, str);
            }
            else {
                Gen[0] += symbase->value << 1;
            }
        }
        break;
        
    case AM_BITBRAMOD:
        if ((symbase->flags & SYM_UNKNOWN) == 0) {
            if (symbase->value > 7) {
                /* [phf] removed
                asmerr( ERROR_ILLEGAL_BIT_SPECIFICATION, false, str );
                */
                error_fmt(ERROR_INVALID_BIT, str);
            }
            else {
                Gen[0] += symbase->value << 1;
            }
        }
        
        sym = symbase->next;
        
        if ((sym->flags & SYM_UNKNOWN) == 0 && sym->value >= 0x100) {
            /* [phf] removed
            asmerr( ERROR_ADDRESS_MUST_BE_LT_100, false, NULL );
            */
            error_fmt(ERROR_ADDRESS_RANGE, 0, 255);
            /* TODO: why no detail in original? */
        }
        
        Gen[opidx++] = sym->value;
        sym = sym->next;
        break;
        
    case AM_REL:
        break;
        
    default:
        if (operand_size(addrmode) > 0) {
            Gen[opidx++] = sym->value;
        }
        if (operand_size(addrmode) == 2) {
            if (MsbOrder) {
                Gen[opidx-1] = sym->value >> 8;
                Gen[opidx++] = sym->value;
            }
            else {
                Gen[opidx++] = sym->value >> 8;
            }
        }
        sym = sym->next;
        break;
    }
    
    if ((mne->flags & MF_MASK) != 0)
    {
        if (sym != NULL) {
            if ((sym->flags & SYM_UNKNOWN) == 0 && sym->value >= 0x100) {
                /* [phf] removed
                asmerr( ERROR_ADDRESS_MUST_BE_LT_100, false, NULL );
                */
                error_fmt(ERROR_ADDRESS_RANGE, 0, 255);
                /* TODO: why no detail in original? */
            }
            
            Gen[opidx] = sym->value;
            sym = sym->next;
        }
        else {
            /* [phf] removed
            asmerr( ERROR_NOT_ENOUGH_ARGS, true, NULL );
            */
            fatal_fmt(ERROR_INVALID_ARGS);
            /* TODO: fatal? really? how about str for details? */
        }
        
        ++opidx;
    }
    
    if ((mne->flags & MF_REL) != 0 || addrmode == AM_REL)
    {
        ++opidx;		/*  to end of instruction   */
        
        if (sym == NULL)
        {
            /* [phf] removed
            asmerr( ERROR_NOT_ENOUGH_ARGS, true, NULL );
            */
            fatal_fmt(ERROR_INVALID_ARGS);
            /* TODO: fatal? really? how about str for details? */
        }
        else if ((sym->flags & SYM_UNKNOWN) == 0) {
            long    pc;
            dasm_flag_t pcf;
            long    dest;
            /* TODO: this code seems to be in isPCKnown() as well */
            pc = (Csegment->flags & SF_RORG) ? Csegment->rorg : Csegment->org;
            pcf= (Csegment->flags & SF_RORG) ? Csegment->rflags : Csegment->flags;
            
            if ((pcf & (SF_UNKNOWN|2)) == 0) {
                dest = sym->value - pc - opidx;
                /* [phf] mnef8.c generate_branch() checks different range! */
                if (dest >= 128 || dest < -128) {
                    /* [phf] removed
                    char sBuffer[64];
                    sprintf( sBuffer, "%ld", dest );
                    asmerr( ERROR_BRANCH_OUT_OF_RANGE, false, sBuffer );
                    */
                    error_fmt(ERROR_BRANCH_RANGE, dest);
                    /* TODO: this doesn't say by how much it's out of range! */
                }
            }
            else {
                /* Don't bother - we'll take another pass */
                dest = 0;
            }
            Gen[opidx-1] = dest & 0xFF;     /*	byte before end of inst.    */
        }
    }
    Glen = opidx;
    generate();
    FreeSymbolList(symbase);
}

void v_trace(const char *str, MNEMONIC *dummy)
{
    assert(str != NULL);
    bTrace = (str[1] == 'n');
}

void v_list(const char *str, MNEMONIC *dummy)
{
    programlabel();
    assert(str != NULL);
    
    Glen = 0; /* Only so outlist() works */

    /*
      The strncmp() cascade that used to be here compared less
      than the whole string (only to the first difference). In
      match_either_case() we compare the whole string. I doubt
      anyone will complain, but you never know... [phf]
    */

    if (match_either_case(str, "LOCALOFF")) {
        pIncfile->flags |= INF_NOLIST;
    }
    else if (match_either_case(str, "LOCALON")) {
        pIncfile->flags &= ~INF_NOLIST;
    }
    else if (match_either_case(str, "OFF")) {
        ListMode = false;
    }
    else {
        ListMode = true;
    }
}

/*
  @brief Return a malloc()ed duplicate of the given string
  with leading and trailing quotation marks removed.
*/
static char *getfilename(const char *str)
{
    char *buf;
    char *end;

    assert(str != NULL);

    /* Skip leading quote. TODO: while? [phf] */
    if (*str == '\"') {
        str++;
    }

    buf = checked_malloc(strlen(str)+1);
    strcpy(buf, str);
        
    /* Find trailing quote and kill it. */
    for (end = buf; *end != '\0' && *end != '\"'; ++end);
    *end = 0;
        
    return buf;
}

void
v_include(const char *str, MNEMONIC *dummy)
{
    char *buf;

    assert(str != NULL);

    programlabel();
    buf = getfilename(str);
    
    pushinclude(buf);
    
    free(buf);
}

void
v_incbin(const char *str, MNEMONIC *dummy)
{
    char *buf;
    FILE *binfile;

    assert(str != NULL);

    programlabel();
    buf = getfilename(str);
    
    binfile = pfopen(buf, "rb");
    if (binfile != NULL) {
        if (Redo != 0) {
            /* optimize: don't actually read the file if not needed */
            fseek(binfile, 0, SEEK_END);
            Glen = ftell(binfile);
            generate();     /* does not access Gen[] if Redo is set */
        }
        else {
            for (;;) {
                Glen = fread(Gen, 1, sizeof(Gen), binfile);
                if (Glen <= 0) {
                    break;
                }
                generate();
            }
        }
        if (fclose(binfile) != 0) {
            warning_fmt("Problem closing binary include file '%s'.\n", buf);
        }
    }
    else {
        warning_fmt("Unable to open binary include file '%s'.\n", buf);
    }
    
    free(buf);
    Glen = 0;		    /* don't list hexdump */
}



void
v_seg(const char *str, MNEMONIC *dummy)
{
    SEGMENT *seg;

    assert(str != NULL);
    
    for (seg = Seglist; seg != NULL; seg = seg->next) {
        if (strcmp(str, seg->name) == 0) {
            Csegment = seg;
            programlabel();
            return;
        }
    }
    Csegment = seg = zero_malloc(sizeof(SEGMENT));
    seg->next = Seglist;
    seg->name = strcpy(checked_malloc(strlen(str)+1), str);
    seg->flags= seg->rflags = seg->initflags = seg->initrflags = SF_UNKNOWN;
    Seglist = seg;
    if (Mnext == AM_BSS)
        seg->flags |= SF_BSS;
    programlabel();
}

void
v_hex(const char *str, MNEMONIC *dummy)
{
    int i;
    int result;

    assert(str != NULL);

    programlabel();
    Glen = 0;
    for (i = 0; str[i] != '\0'; ++i) {
        if (str[i] == ' ')
            continue;
        result = (gethexdig(str[i]) << 4) + gethexdig(str[i+1]);
        if (str[++i] == 0)
            break;
        Gen[Glen++] = result;
    }
    generate();
}

static int gethexdig(int c)
{
    /* [phf] removed
    char sBuffer[64];
    */
    c = toupper(c);
    
    if ('0' <= c && c <= '9') {
        return c - '0';
    }

    /* [phf] removed
    if ('a' <= c && c <= 'f')
        return c - 'a' + 10;
    */
    
    if ('A' <= c && c <= 'F') {
        return c - 'A' + 10;
    }
    
    /* [phf] removed
    sprintf( sBuffer, "Bad Hex Digit %c", c );
    asmerr( ERROR_SYNTAX_ERROR, false, sBuffer );
    */
    error_fmt("Bad hex digit '%c'!", c);
    
    /* TODO: refactor into error handling code */
    (void) puts("(Must be a valid hex digit)");
    if (FI_listfile != NULL) {
        fputs("(Must be a valid hex digit)\n", FI_listfile);
    }
    
    return 0;
}

void
v_err(const char *str, MNEMONIC *dummy)
{
    programlabel();
    /* [phf] removed
    asmerr( ERROR_ERR_PSEUDO_OP_ENCOUNTERED, true, NULL );
    */
    fatal_fmt("ERR pseudo-op encountered, aborting assembly!");
    /* TODO: really exit() here? that's almost like panic? */
    exit(EXIT_FAILURE);
}

void
v_dc(const char *str, MNEMONIC *mne)
{
    SYMBOL *sym;
    SYMBOL *tmp;
    unsigned long  value;
    const char *macstr = NULL; /* "might be used uninitialised" */
    bool vmode = false;

    assert(str != NULL);
    assert(mne != NULL);
    
    Glen = 0;
    programlabel();


    /* for byte, .byte, word, .word, long, .long */
    if (mne->name[0] != 'd') {
        static char sTmp[4];
        strcpy(sTmp, "x.x");
        sTmp[2] = mne->name[0];
        findext(sTmp);
    }

	/* F8... */

    /* db, dw, dd */
    if ( (mne->name[0] == 'd') && (mne->name[1] != 'c') ) {
        static char sTmp[4];
        strcpy(sTmp, "x.x");
        if ('d' == mne->name[1]) {
			sTmp[2] = 'l';
		} else {
            sTmp[2] = mne->name[1];
		}
        findext(sTmp);
    }

	/* ...F8 */
	


    if (mne->name[1] == 'v') {
        int i;
        vmode = true;
        for (i = 0; str[i] && str[i] != ' '; ++i);
        tmp = findsymbol(str, i);
        str += i;
        if (tmp == NULL) {
            (void) puts("EQM label not found");
            return;
        }
        if ((tmp->flags & SYM_MACRO) != 0) {
            macstr = tmp->string;
        }
        else
        {
            (void) puts("must specify EQM label for DV");
            return;
        }
    }
    sym = eval(str, false);
    for (; sym; sym = sym->next) {
        value = sym->value;
        if ((sym->flags & SYM_UNKNOWN) != 0) {
            ++Redo;
            Redo_why |= REASON_DC_NOT_RESOVED;
        }
        if ((sym->flags & SYM_STRING) != 0) {
            const char *ptr = sym->string;
            while ((value = *ptr) != 0) {
                if (vmode) {
                    setspecial(value, 0);
                    tmp = eval(macstr, false);
                    value = tmp->value;
                    if ((tmp->flags & SYM_UNKNOWN) != 0) {
                        ++Redo;
                        Redo_why |= REASON_DV_NOT_RESOLVED_PROBABLY;
                    }
                    FreeSymbolList(tmp);
                }
                switch(Mnext) {
                default: /* TODO: defense? or AM_BYTE really default? [phf] */
                case AM_BYTE:
                    Gen[Glen++] = value & 0xFF;
                    break;
                case AM_WORD:
                    if (MsbOrder) {
                        Gen[Glen++] = (value >> 8) & 0xFF;
                        Gen[Glen++] = value & 0xFF;
                    }
                    else
                    {
                        Gen[Glen++] = value & 0xFF;
                        Gen[Glen++] = (value >> 8) & 0xFF;
                    }
                    break;
                case AM_LONG:
                    if (MsbOrder) {
                        Gen[Glen++] = (value >> 24)& 0xFF;
                        Gen[Glen++] = (value >> 16)& 0xFF;
                        Gen[Glen++] = (value >> 8) & 0xFF;
                        Gen[Glen++] = value & 0xFF;
                    }
                    else
                    {
                        Gen[Glen++] = value & 0xFF;
                        Gen[Glen++] = (value >> 8) & 0xFF;
                        Gen[Glen++] = (value >> 16)& 0xFF;
                        Gen[Glen++] = (value >> 24)& 0xFF;
                    }
                    break;
                }
                ++ptr;
            }
        }
        else
        {
            if (vmode) {
                setspecial(value, sym->flags);
                tmp = eval(macstr, false);
                value = tmp->value;
                if ((tmp->flags & SYM_UNKNOWN) != 0) {
                    ++Redo;
                    Redo_why |= REASON_DV_NOT_RESOLVED_COULD;
                }
                FreeSymbolList(tmp);
            }
            switch(Mnext) {
            default: /* TODO: defense? [phf] */
            case AM_BYTE:
                Gen[Glen++] = value & 0xFF;
                break;
            case AM_WORD:
                if (MsbOrder) {
                    Gen[Glen++] = (value >> 8) & 0xFF;
                    Gen[Glen++] = value & 0xFF;
                }
                else
                {
                    Gen[Glen++] = value & 0xFF;
                    Gen[Glen++] = (value >> 8) & 0xFF;
                }
                break;
            case AM_LONG:
                if (MsbOrder) {
                    Gen[Glen++] = (value >> 24)& 0xFF;
                    Gen[Glen++] = (value >> 16)& 0xFF;
                    Gen[Glen++] = (value >> 8) & 0xFF;
                    Gen[Glen++] = value & 0xFF;
                }
                else
                {
                    Gen[Glen++] = value & 0xFF;
                    Gen[Glen++] = (value >> 8) & 0xFF;
                    Gen[Glen++] = (value >> 16)& 0xFF;
                    Gen[Glen++] = (value >> 24)& 0xFF;
                }
                break;
            }
        }
    }
    generate();
    FreeSymbolList(sym);
}



void
v_ds(const char *str, MNEMONIC *dummy)
{
    SYMBOL *sym;
    int mult = 1;
    long filler = 0;

    assert(str != NULL);
    
    if (Mnext == AM_WORD) {
        mult = 2;
    }
    else if (Mnext == AM_LONG) {
        mult = 4;
    }
    programlabel();
    if ((sym = eval(str, false)) != NULL) {
        if (sym->next != NULL) {
            filler = sym->next->value;
        }
        if ((sym->flags & SYM_UNKNOWN) != 0) {
            ++Redo;
            Redo_why |= REASON_DS_NOT_RESOLVED;
        }
        else {
            if (sym->next != NULL && (sym->next->flags & SYM_UNKNOWN) != 0) {
                ++Redo;
                Redo_why |= REASON_DS_NOT_RESOLVED;
            }
            genfill(filler, sym->value, mult);
        }
        FreeSymbolList(sym);
    }
    /* TODO: what if eval returns NULL? */
    assert(sym != NULL);
}

void
v_org(const char *str, MNEMONIC *dummy)
{
    SYMBOL *sym;

    assert(str != NULL);
    
    sym = eval(str, false);
    assert(sym != NULL);

    Csegment->org = sym->value;
    
    if ((sym->flags & SYM_UNKNOWN) != 0) {
        Csegment->flags |= SYM_UNKNOWN;
    }
    else {
        Csegment->flags &= ~SYM_UNKNOWN;
    }

    if ((Csegment->initflags & SYM_UNKNOWN) != 0) {
        Csegment->initorg = sym->value;
        Csegment->initflags = sym->flags;
    }

    if (sym->next != NULL)
    {
        OrgFill = sym->next->value;
        if ((sym->next->flags & SYM_UNKNOWN) != 0)
        {
            /* [phf] removed
            asmerr( ERROR_VALUE_UNDEFINED, true, NULL );
            */
            fatal_fmt("Value undefined!");
            /* TODO: fatal? what about more passes? what about details? */
        }
    }
    
    programlabel();
    FreeSymbolList(sym);
}

void
v_rorg(const char *str, MNEMONIC *dummy)
{
    SYMBOL *sym = eval(str, false);

    assert(str != NULL);
    assert(sym != NULL);
    
    Csegment->flags |= SF_RORG;
    if (sym->addrmode != AM_IMP) {
        Csegment->rorg = sym->value;
        if ((sym->flags & SYM_UNKNOWN) != 0) {
            Csegment->rflags |= SYM_UNKNOWN;
        }
        else {
            Csegment->rflags &= ~SYM_UNKNOWN;
        }
        if ((Csegment->initrflags & SYM_UNKNOWN) != 0) {
            Csegment->initrorg = sym->value;
            Csegment->initrflags = sym->flags;
        }
    }
    programlabel();
    FreeSymbolList(sym);
}

void
v_rend(const char *str, MNEMONIC *dummy)
{
    programlabel();
    Csegment->flags &= ~SF_RORG;
}

void
v_align(const char *str, MNEMONIC *dummy)
{
    SYMBOL *sym = eval(str, false);
    unsigned char fill = 0;
    /* was "unsigned char rorg = Csegment->flags & SF_RORG;" and it
       might just be used as a bool, not really flags... [phf] */
    dasm_flag_t rorg = Csegment->flags & SF_RORG;

    assert(str != NULL);
    assert(sym != NULL);
    
    if (rorg) {
        Csegment->rflags |= SF_REF;
    }
    else {
        Csegment->flags |= SF_REF;
    }
    if (sym->next != NULL) {
        if ((sym->next->flags & SYM_UNKNOWN) != 0) {
            ++Redo;
            Redo_why |= REASON_ALIGN_NOT_RESOLVED;
        }
        else {
            fill = sym->next->value;
        }
    }
    /* TODO: major code duplication in next if/else */
    if (rorg) {
        if (((Csegment->rflags | sym->flags) & SYM_UNKNOWN) != 0) {
            ++Redo;
            Redo_why |= REASON_ALIGN_RELOCATABLE_ORIGIN_NOT_KNOWN;
        }
        else {
            long n = sym->value - (Csegment->rorg % sym->value);
            if (n != sym->value) {
                genfill(fill, n, 1);
            }
        }
    }
    else {
        if (((Csegment->flags | sym->flags) & SYM_UNKNOWN) != 0) {
            ++Redo;
            Redo_why |= REASON_ALIGN_NORMAL_ORIGIN_NOT_KNOWN;
        }
        else {
            long n = sym->value - (Csegment->org % sym->value);
            if (n != sym->value) {
                genfill(fill, n, 1);
            }
        }
    }
    FreeSymbolList(sym);
    programlabel();
}

void
v_subroutine(const char *str, MNEMONIC *dummy)
{
    ++Lastlocalindex;
    Localindex = Lastlocalindex;
    programlabel();
}

void
v_equ(const char *str, MNEMONIC *dummy)
{
    SYMBOL *sym = eval(str, false);
    SYMBOL *lab;

    assert(str != NULL);
    assert(sym != NULL);
    
    /*
        If we encounter a line of the form
            . = expr
            . EQU expr
            * = expr
            * EQU expr
        treat it as one of
            org expr
            rorg expr
        depending on whether we have a relocatable origin now or not.
    */
    if (strlen(Av[0]) == 1) {
        if (Av[0][0] == '*') {
            Av[0][0] = '.';
        }
        if (Av[0][0] == '.') {
            if ((Csegment->flags & SF_RORG) != 0) {
                v_rorg(str, dummy);
            }
            else {
                v_org(str, dummy);
            }
            return;
        }
    }

#if 0
    if (strlen(Av[0]) == 1 && (Av[0][0] == '.'
        || (Av[0][0] == '*' && (Av[0][0] = '.') && 1)           /*AD: huh?*/
        )) {
        /* Av[0][0] = '\0'; */
        if ((Csegment->flags & SF_RORG) != 0) {
            v_rorg(str, dummy);
        }
        else {
            v_org(str, dummy);
        }
        return;
    }
#endif
    
    lab = findsymbol(Av[0], strlen(Av[0]));
    if (lab == NULL) {
        lab = CreateSymbol(Av[0], strlen(Av[0]));
    }
    if ((lab->flags & SYM_UNKNOWN) == 0)
    {
        if ((sym->flags & SYM_UNKNOWN) != 0)
        {
            ++Redo;
            Redo_why |= REASON_EQU_NOT_RESOLVED;
        }
        else
        {
            if (lab->value != sym->value)
            {
                /* [phf] removed
                asmerr( ERROR_EQU_VALUE_MISMATCH, false, NULL );
                */
                error_fmt("EQU: Value mismatch.");
                printf("old value: $%04lx  new value: $%04lx\n",
                    lab->value, sym->value);
                ++Redo;
                Redo_why |= REASON_EQU_VALUE_MISMATCH;
            }
        }
    }
    
    lab->value = sym->value;
    lab->flags = sym->flags & (SYM_UNKNOWN|SYM_STRING);
    lab->string = sym->string;
    sym->flags &= ~(SYM_STRING|SYM_MACRO);
    
    /* List the value */
    {
        unsigned long v = lab->value;
        
        Glen = 0;
        if (v > 0x0000FFFF)
        {
            Gen[Glen++] = v >> 24;
            Gen[Glen++] = v >> 16;
        }
        Gen[Glen++] = v >>  8;
        Gen[Glen++] = v;
    }
    
    FreeSymbolList(sym);
}

void
v_eqm(const char *str, MNEMONIC *dummy)
{
    SYMBOL *lab;
    size_t len = strlen(Av[0]);

    assert(str != NULL);
    
    if ((lab = findsymbol(Av[0], len)) != NULL) {
        if ((lab->flags & SYM_STRING) != 0) {
            free(lab->string);
        }
    }
    else
    {
        lab = CreateSymbol(Av[0], len);
    }
    lab->value = 0;
    lab->flags = SYM_STRING | SYM_SET | SYM_MACRO;
    lab->string = strcpy(checked_malloc(strlen(str)+1), str);
}

void
v_echo(const char *str, MNEMONIC *dummy)
{
    SYMBOL *sym = eval(str, false);
    SYMBOL *s;
    char buf[256];
    int len;

    assert(str != NULL);
    assert(sym != NULL); /* maybe not needed, we check NULL below? [phf] */
    
    for (s = sym; s != NULL; s = s->next) {
        if ((s->flags & SYM_UNKNOWN) == 0) {
            if ((s->flags & (SYM_MACRO|SYM_STRING)) != 0) {
                len = snprintf(buf, sizeof(buf), "%s", s->string);
                assert(len < (int)sizeof(buf));
            }
            else {
                len = snprintf(buf, sizeof(buf), "$%lx", s->value);
                assert(len < (int)sizeof(buf));
            }
            if (FI_listfile != NULL) {
                fprintf(FI_listfile, " %s", buf);
            }
            printf(" %s", buf);
        }
    }
    (void) puts("");
    if (FI_listfile != NULL) {
        putc('\n', FI_listfile);
    }
}

void v_set(const char *str, MNEMONIC *dummy)
{
    SYMBOL *sym = eval(str, false);
    SYMBOL *lab;

    assert(str != NULL);
    assert(sym != NULL);
    
    lab = findsymbol(Av[0], strlen(Av[0]));
    if (lab == NULL) {
        lab = CreateSymbol(Av[0], strlen(Av[0]));
    }
    lab->value = sym->value;
    lab->flags = sym->flags & (SYM_UNKNOWN|SYM_STRING);
    lab->string = sym->string;
    sym->flags &= ~(SYM_STRING|SYM_MACRO);
    FreeSymbolList(sym);
}

void
v_execmac(const char *str, MACRO *mac)
{
    INCFILE *inc;
    STRLIST *base;
    STRLIST **psl, *sl;
    const char *sone; /* used to be "s1" which clashed with "sl" [phf] */

    assert(str != NULL);
    assert(mac != NULL);
    
    programlabel();
    
    if (Mlevel == MAXMACLEVEL) {
        (void) puts("infinite macro recursion");
        return;
    }
    ++Mlevel;
    base = checked_malloc(sizeof(STRLIST)-STRLISTSIZE+strlen(str)+1);
    base->next = NULL;
    strcpy(base->buf, str);
    psl = &base->next;
    while (*str != '\0' && *str != '\n') {
        sone = str;
        while (*str != '\0' && *str != '\n' && *str != ',')
            ++str;
        sl = checked_malloc(sizeof(STRLIST)-STRLISTSIZE+1+(str-sone));
        sl->next = NULL;
        *psl = sl;
        psl = &sl->next;
        memcpy(sl->buf, sone, (str-sone));
        sl->buf[str-sone] = 0;
        if (*str == ',')
            ++str;
        while (*str == ' ')
            ++str;
    }
    
    inc = zero_malloc(sizeof(INCFILE));
    inc->next = pIncfile;
    inc->name = mac->name;
    inc->fi   = pIncfile->fi;	/* garbage */
    inc->lineno = 0;
    inc->flags = INF_MACRO;
    inc->saveidx = Localindex;
    
    inc->savedolidx = Localdollarindex;
    
    inc->strlist = mac->strlist;
    inc->args	  = base;
    pIncfile = inc;
    
    ++Lastlocalindex;
    Localindex = Lastlocalindex;
    
    ++Lastlocaldollarindex;
    Localdollarindex = Lastlocaldollarindex;
    
}

void v_end(const char *str, MNEMONIC *dummy)
{
    /* Only ENDs current file and any macro calls within it */
    
    while ((pIncfile->flags & INF_MACRO) != 0) {
        v_endm(NULL, NULL);
    }
    
    fseek(pIncfile->fi, 0, SEEK_END);
}

void
v_endm(const char *str, MNEMONIC *dummy)
{
    INCFILE *inc = pIncfile;
    STRLIST *args, *an;

    assert(inc != NULL);
    
    /* programlabel(); contrary to documentation */
    if ((inc->flags & INF_MACRO) != 0) {
        --Mlevel;
        for (args = inc->args; args != NULL; args = an) {
            an = args->next;
            free(args);
        }
        Localindex = inc->saveidx;
        
        Localdollarindex = inc->savedolidx;
        
        pIncfile = inc->next;
        free(inc);
        return;
    }
    (void) puts("not within a macro");
}

void
v_mexit(const char *str, MNEMONIC *dummy)
{
    v_endm(NULL, NULL);
}

void
v_ifconst(const char *str, MNEMONIC *dummy)
{
    SYMBOL *sym;
    assert(str != NULL);
    
    programlabel();
    sym = eval(str, false);
    assert(sym != NULL);
    pushif(sym->flags == 0);
    FreeSymbolList(sym);
}

void
v_ifnconst(const char *str, MNEMONIC *dummy)
{
    SYMBOL *sym;
    assert(str != NULL);
    
    programlabel();
    sym = eval(str, false);
    assert(sym != NULL);
    pushif(sym->flags != 0);
    FreeSymbolList(sym);
}

void
v_if(const char *str, MNEMONIC *dummy)
{
    SYMBOL *sym;
    assert(str != NULL);
    
    if (!Ifstack->xtrue || !Ifstack->acctrue) {
        pushif(0);
        return;
    }
    programlabel();
    sym = eval(str, false);
    assert(sym != NULL);
    if (sym->flags != 0) {
        ++Redo;
        Redo_why |= REASON_IF_NOT_RESOLVED;
        pushif(0);
        Ifstack->acctrue = false;
        
        Redo_if |= 1;
        
    }
    else {
        pushif(!!sym->value);
    }
    FreeSymbolList(sym);
}

void v_else(const char *str, MNEMONIC *dummy)
{
    if (Ifstack->acctrue && (Ifstack->flags & IFF_BASE) == 0) {
        programlabel();
        Ifstack->xtrue = !Ifstack->xtrue;
    }
}

void
v_endif(const char *str, MNEMONIC *dummy)
{
    IFSTACK *ifs = Ifstack;
    assert(ifs != NULL);
    
    if ((ifs->flags & IFF_BASE) == 0) {
        if (ifs->acctrue) {
            programlabel();
        }
        if (ifs->file != pIncfile) {
            (void) puts("too many endif's");
        }
        else {
            Ifstack = ifs->next;
            free(ifs);
        }
    }
}

void v_repeat(const char *str, MNEMONIC *dummy)
{
    REPLOOP *rp;
    SYMBOL *sym;
    assert(str != NULL);
    
    if (!Ifstack->xtrue || !Ifstack->acctrue) {
        pushif(0);
        return;
    }
    programlabel();
    sym = eval(str, false);
    assert(sym != NULL);
    if (sym->value == 0) {
        pushif(0);
        FreeSymbolList(sym);
        return;
    }
    
    /* Don't allow negative values for REPEAT loops [AD] */
    /* TODO: refactor with == 0 case above? [phf] */
    
    if (sym->value < 0)
    {
        pushif( 0 );
        FreeSymbolList(sym);

        /* [phf] removed
        asmerr( ERROR_REPEAT_NEGATIVE, false, NULL );
        */
        error_fmt("REPEAT parameter < 0 (ignored).");
        return;
    }
    
    rp = zero_malloc(sizeof(REPLOOP));
    rp->next = Reploop;
    rp->file = pIncfile;
    if ((pIncfile->flags & INF_MACRO) != 0) {
        rp->seek = (long)pIncfile->strlist;
    }
    else {
        rp->seek = ftell(pIncfile->fi);
    }
    rp->lineno = pIncfile->lineno;
    rp->count = sym->value;
    if ((rp->flags = sym->flags) != 0) {
        ++Redo;
        Redo_why |= REASON_REPEAT_NOT_RESOLVED;
    }
    Reploop = rp;
    FreeSymbolList(sym);
    pushif(1);
}

void
v_repend(const char *str, MNEMONIC *dummy)
{
    if (!Ifstack->xtrue || !Ifstack->acctrue) {
        v_endif(NULL,NULL);
        return;
    }
    if (Reploop != NULL && Reploop->file == pIncfile) {
        if (Reploop->flags == 0 && --Reploop->count) {
            if ((pIncfile->flags & INF_MACRO) != 0) {
                pIncfile->strlist = (STRLIST *)Reploop->seek;
            }
            else {
                fseek(pIncfile->fi, Reploop->seek, 0);
            }
            pIncfile->lineno = Reploop->lineno;
        }
        else {
            rmnode((void **)&Reploop, sizeof(REPLOOP));
            v_endif(NULL,NULL);
        }
        return;
    }
    (void) puts("no repeat");
    /* TODO: is this an error or a warning or what? [phf] */
}

static STRLIST *incdirlist;

void
v_incdir(const char *str, MNEMONIC *dummy)
{
    STRLIST **tail;
    char *buf;
    bool found = false;
    assert(str != NULL);
    
    buf = getfilename(str);
    
    for (tail = &incdirlist; *tail != NULL; tail = &(*tail)->next) {
        if (strcmp((*tail)->buf, buf) == 0) {
            found = true;
        }
    }
    
    if (!found) {
        STRLIST *newdir;
        /* TODO: I think size is calculated wrong here, too big... [phf] */
        newdir = small_alloc(STRLISTSIZE + 1 + strlen(buf));
        strcpy(newdir->buf, buf);
        *tail = newdir;
    }
    
    free(buf);
}

static void
addpart(char *dest, const char *dir, const char *file)
{
#if 0	/* not needed here */
    if (strchr(file, ':')) {
        strcpy(dest, file);
    }
    else
#endif
    {
        int pos;
        
        strcpy(dest, dir);
        pos = strlen(dest);
        if (pos > 0 && dest[pos-1] != ':' && dest[pos-1] != '/') {
            dest[pos] = '/';
            pos++;
        }
        strcpy(dest + pos, file);
    }
}

FILE *
pfopen(const char *name, const char *mode)
{
    FILE *f;
    STRLIST *incdir;
    char *buf;

    assert(name != NULL);
    assert(mode != NULL);

    f = fopen(name, mode);
    if (f != NULL) {
        return f;
    }
    
    /* Don't use the incdirlist for absolute pathnames */
    if (strchr(name, ':')) {
        return NULL;
    }

    /* TODO: the above looks like an Amiga leftover? the 512 below
       is fishy as well, wow... [phf] */
    
    buf = zero_malloc(512);
    
    for (incdir = incdirlist; incdir; incdir = incdir->next) {
        addpart(buf, incdir->buf, name);
        
        f = fopen(buf, mode);
        if (f != NULL) {
            break;
        }
    }
    
    free(buf);
    return f;
}


static long Seglen;
static long Seekback;

void
generate(void)
{
    long seekpos;
    static unsigned long org;
    int i;
    
    if (Redo == 0)
    {
        if ((Csegment->flags & SF_BSS) == 0)
        {
            for (i = Glen - 1; i >= 0; --i)
                CheckSum += Gen[i];
            
            if (Fisclear)
            {
                Fisclear = false;
                if ((Csegment->flags & SF_UNKNOWN) != 0)
                {
                    ++Redo;
                    Redo_why |= REASON_OBSCURE;
                    return;
                }
                
                org = Csegment->org;
                
                if ( F_format < FORMAT_RAW )
                {
                    putc((org & 0xFF), FI_temp);
                    putc(((org >> 8) & 0xFF), FI_temp);
                    
                    if ( F_format == FORMAT_RAS )
                    {
                        Seekback = ftell(FI_temp);
                        Seglen = 0;
                        putc(0, FI_temp);
                        putc(0, FI_temp);
                    }
                }
            }
            
            switch(F_format)
            {
            default:
                /* [phf] removed
                asmerr(ERROR_BAD_FORMAT, true,
                       "Unhandled internal format specifier!");
                */
                fatal_fmt("Bad output format specified."
                  "Unhandled internal format specifier!");
                break;

            case FORMAT_RAW:
            case FORMAT_DEFAULT:
                
                if (Csegment->org < org)
                {
                    printf("segment: %s %s  vs current org: %04lx\n",
                        Csegment->name, sftos(Csegment->org, Csegment->flags), org);
                    /* [phf] removed
                    asmerr( ERROR_ORIGIN_REVERSE_INDEXED, true, NULL );
                    */
                    fatal_fmt("Origin Reverse-indexed.");
                    exit(EXIT_FAILURE); /* TODO: necessary? why in the first place? */
                }
                
                while (Csegment->org != org)
                {
                    putc(OrgFill, FI_temp);
                    ++org;
                }
                
                fwrite(Gen, Glen, 1, FI_temp);
                break;
                
            case FORMAT_RAS:
                
                if (org != Csegment->org)
                {
                    org = Csegment->org;
                    seekpos = ftell(FI_temp);
                    fseek(FI_temp, Seekback, 0);
                    putc((Seglen & 0xFF), FI_temp);
                    putc(((Seglen >> 8) & 0xFF), FI_temp);
                    fseek(FI_temp, seekpos, 0);
                    putc((org & 0xFF), FI_temp);
                    putc(((org >> 8) & 0xFF), FI_temp);
                    Seekback = ftell(FI_temp);
                    Seglen = 0;
                    putc(0, FI_temp);
                    putc(0, FI_temp);
                }
                
                fwrite(Gen, Glen, 1, FI_temp);
                Seglen += Glen;
                break;
            
            }
            org += Glen;
        }
    }
    
    Csegment->org += Glen;
    
    if ((Csegment->flags & SF_RORG) != 0) {
        Csegment->rorg += Glen;
    }
}

void closegenerate(void)
{
    if (Redo == 0) {
        if (F_format == FORMAT_RAS) {
            fseek(FI_temp, Seekback, 0);
            putc((Seglen & 0xFF), FI_temp);
            putc(((Seglen >> 8) & 0xFF), FI_temp);
            fseek(FI_temp, 0L, 2);
        }
    }
}

static void genfill(int32_t fill, long entries, int size)
{
    long bytes;
    int i;
    unsigned char c3,c2,c1,c0;
    const int size_of_gen = sizeof(Gen);

    assert(entries >= 0);
    assert(size == 1 || size == 2 || size == 4);
    assert(size_of_gen > 0);

    if (entries == 0) {
        return;
    }

    bytes = entries * size;

    c3 = (fill >> 24) & 0xff;
    c2 = (fill >> 16) & 0xff;
    c1 = (fill >> 8) & 0xff;
    c0 = fill & 0xff;

    switch (size) {
        default:
            assert(false); /* defensive programming! [phf] */
            break;

        case 1:
            memset(Gen, c0, size_of_gen);
            break;

        case 2:
            for (i = 0; i < size_of_gen; i += 2) {
                if (MsbOrder) {
                    Gen[i+0] = c1;
                    Gen[i+1] = c0;
                }
                else {
                    Gen[i+0] = c0;
                    Gen[i+1] = c1;
                }
            }
            break;

        case 4:
            for (i = 0; i < size_of_gen; i += 4) {
                if (MsbOrder) {
                    Gen[i+0] = c3;
                    Gen[i+1] = c2;
                    Gen[i+2] = c1;
                    Gen[i+3] = c0;
                }
                else {
                    Gen[i+0] = c0;
                    Gen[i+1] = c1;
                    Gen[i+2] = c2;
                    Gen[i+3] = c3;
                }
            }
            break;
    }

    for (Glen = size_of_gen; bytes > size_of_gen; bytes -= size_of_gen) {
        generate();
    }
    Glen = bytes;
    generate();
}

static void pushif(bool xbool)
{
    IFSTACK *ifs = zero_malloc(sizeof(IFSTACK));
    ifs->next = Ifstack;
    ifs->file = pIncfile;
    ifs->flags = 0;
    ifs->xtrue  = xbool;
    ifs->acctrue = Ifstack->acctrue && Ifstack->xtrue;
    Ifstack = ifs;
}

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4 autoindent: */
