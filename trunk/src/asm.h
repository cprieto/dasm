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
 *  ASM65.H
 *
 *  Structures and definitions
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
    Hack for Windows compatibilty (or at least Visual Studio
    compatibility I guess?). Note that I have no idea if the
    __WINDOWS__ symbol is really the right one to check for,
    please let me know if it's not. [phf]
*/
#if defined(__WINDOWS__)
/* no stdbool.h so fake it */
#define bool int
#define false (0==1)
#define true (!false)
/* no strcasecmp so fake it using stricmp */
#define strcasecmp stricmp
/* strange paths :-) */
#define DASM_PATH_SEPARATOR '\'
#else
#include <stdbool.h>
#include <strings.h>
#define DASM_PATH_SEPARATOR '/'
#endif /* defined(__WINDOWS__) */

#define OlafFreeFormat    0    /* Decide on looks of word if it is opcode */
#define OlafHashFormat    1    /* Decide on # and ^ if it is an opcode */

#if OlafHashFormat && OlafFreeFormat
#error This cannot be!
#endif

/* for -T option [phf] */
typedef enum
{
  SORTMODE_DEFAULT,
  SORTMODE_ALPHA = SORTMODE_DEFAULT,
  SORTMODE_ADDRESS,
  SORTMODE_MAX
} sortmode_t;

enum FORMAT
{
    FORMAT_DEFAULT = 1,
    FORMAT_RAS,
    FORMAT_RAW,
    FORMAT_MAX
};

/* ??? [phf] */
#define MAX_SYM_LEN 1024
/* maximum length genfill() will generate */
#define MAX_FILL_LEN 64*1024

    enum REASON_CODES
    {
        REASON_MNEMONIC_NOT_RESOLVED = 1 << 0,
        REASON_OBSCURE = 1 << 1,                        /* fix this! */
        REASON_DC_NOT_RESOVED = 1 << 2,
        REASON_DV_NOT_RESOLVED_PROBABLY = 1 << 3,
        REASON_DV_NOT_RESOLVED_COULD = 1 << 4,
        REASON_DS_NOT_RESOLVED = 1 << 5,
        REASON_ALIGN_NOT_RESOLVED = 1 << 6,
        REASON_ALIGN_RELOCATABLE_ORIGIN_NOT_KNOWN = 1 << 7,
        REASON_ALIGN_NORMAL_ORIGIN_NOT_KNOWN = 1 << 8,
        REASON_EQU_NOT_RESOLVED = 1 << 9,
        REASON_EQU_VALUE_MISMATCH = 1 << 10,
        REASON_IF_NOT_RESOLVED = 1 << 11,
        REASON_REPEAT_NOT_RESOLVED = 1 << 12,
        REASON_FORWARD_REFERENCE = 1 << 13,
        REASON_PHASE_ERROR = 1 << 14
    };


#define DEFORGFILL  255
#define SHASHSIZE   1024
#define MHASHSIZE   1024
#define SHASHAND    (SHASHSIZE-1) /*0x03FF*/
#define MHASHAND    (MHASHSIZE-1) /*0x03FF*/
#define MAXMACLEVEL 32
#define TAB        9


	enum ADDRESS_MODES {
		AM_IMP,					/*    implied         */
		AM_IMM8,				/*    immediate 8  bits   */
		AM_IMM16,		        /*    immediate 16 bits   */
		AM_BYTEADR,				/*    address 8 bits        */
		AM_BYTEADRX,			/*    address 16 bits     */
		AM_BYTEADRY,			/*    relative 8 bits     */
		AM_WORDADR,				/*    index x 0 bits        */
		AM_WORDADRX,			/*    index x 8 bits        */
		AM_WORDADRY,			/*    index x 16 bits     */
		AM_REL,					/*    bit inst. special   */
		AM_INDBYTEX,			/*    bit-bra inst. spec. */
		AM_INDBYTEY,			/*    index y 0 bits        */
		AM_INDWORD,				/*    index y 8 bits        */
		AM_0X,					/*    index x 0 bits        */
		AM_0Y,					/*    index y 0 bits        */
		AM_BITMOD,				/*    ind addr 8 bits     */
		AM_BITBRAMOD,			/*    ind addr 16 bits    */

		AM_SYMBOL,
		AM_EXPLIST,
		AM_LONG,
		AM_BSS,

		NUMOC
	};

#define AF_IMP					( 1L << AM_IMP )
#define AF_IMM8					( 1L << AM_IMM8 )
#define AF_IMM16				( 1L << AM_IMM16 )
#define AF_BYTEADR				( 1L << AM_BYTEADR )
#define AF_BYTEADRX				( 1L << AM_BYTEADRX )
#define AF_BYTEADRY				( 1L << AM_BYTEADRY )
#define AF_WORDADR				( 1L << AM_WORDADR )
#define AF_WORDADRX				( 1L << AM_WORDADRX )
#define AF_WORDADRY				( 1L << AM_WORDADRY )
#define AF_REL					( 1L << AM_REL )
#define AF_INDBYTEX				( 1L << AM_INDBYTEX )
#define AF_INDBYTEY				( 1L << AM_INDBYTEY )
#define AF_INDWORD				( 1L << AM_INDWORD )
#define AF_0X					( 1L << AM_0X )
#define AF_0Y					( 1L << AM_0Y )
#define AF_BITMOD				( 1L << AM_BITMOD )
#define AF_BITBRAMOD			( 1L << AM_BITBRAMOD )

#define AM_BYTE					AM_BYTEADR
#define AM_WORD					AM_WORDADR


typedef struct _STRLIST STRLIST;
struct _STRLIST
{
    /* next string in list? [phf] */
    STRLIST *next;
    /* the actual string? [phf] */
    char buf[4];
    /*
        TODO: actual code in main.c and ops.c where STRLIST gets
        malloc()ed indicates that buf[4] is a hack that basically
        emulates a "flexible array member" of C99 fame; should be
        replaced with a properly allocated buffer! the main.c use
        is probably wrong btw... [phf]
    */
};

#define STRLISTSIZE    4

#define MF_IF					0x04
#define MF_MACRO				0x08
#define MF_MASK					0x10    /*  has mask argument (byte)    */
#define MF_REL					0x20    /*  has rel. address (byte)    */
#define MF_IMOD					0x40    /*  instruction byte mod.    */
#define MF_ENDM					0x80    /*  is v_endm            */

typedef struct _MNEMONIC MNEMONIC;
struct _MNEMONIC
{
    /* next mnemonic in hash list */
    MNEMONIC *next;
    /* dispatch */
    void (*vect)(char *, MNEMONIC *);
    /* actual name */
    const char *name;
    /* special flags */
    unsigned char flags;
    /* TODO: ??? [phf] */
    unsigned long okmask;
    /* hex codes, byte or word (>xFF) opcodes */
    unsigned int opcode[NUMOC];
};

/* MNEMONIC with all fields 0, used as end-of-table marker. */
#define MNEMONIC_NULL {NULL, NULL, NULL, 0, 0, {0,}}

typedef struct _MACRO MACRO;
struct _MACRO
{
    MACRO *next;
    void (*vect)(char *, MACRO *);
    char *name;
    unsigned char flags;
    STRLIST *strlist;
};

#define INF_MACRO   0x01
#define INF_NOLIST  0x02

typedef struct _INCFILE INCFILE;
struct _INCFILE
{
    /* previously pushed context */
    INCFILE *next;
    /* file name */
    char *name;
    /* file handle */
    FILE *fi;
    /* line number in file */
    unsigned long lineno;
    /* flags (macro) */
    unsigned char flags;

    /* Only if Macro */

    /* arguments to macro */
    STRLIST *args;
    /* current string list */
    STRLIST *strlist;
    /* save localindex */
    unsigned long saveidx;
    /* save localdollarindex */
    unsigned long savedolidx;
};

#define RPF_UNKNOWN 0x01    /*      value unknown     */

typedef struct _REPLOOP REPLOOP;
struct _REPLOOP
{
    /* previously pushed context */
    REPLOOP *next;
    /* repeat count */
    unsigned long count;
    /* seek to top of repeat */
    unsigned long seek;
    /* line number of line before */
    unsigned long lineno;
    /* which include file are we in */
    INCFILE *file;
    /* TODO: ??? [phf] */
    unsigned char flags;
};

#define IFF_UNKNOWN 0x01    /*      value unknown        */
#define IFF_BASE    0x04

typedef struct _IFSTACK IFSTACK;
struct _IFSTACK
{
    /* previous IF */
    IFSTACK *next;
    /* which include file are we in */
    INCFILE *file;
    /* TODO: ??? [phf] */
    unsigned char flags;
    /* 1 if true, 0 if false */
    unsigned char xtrue;
    /* accumulatively true (not incl this one) */
    unsigned char acctrue;
};

#define SF_UNKNOWN  0x01    /* ORG unknown */
#define SF_REF      0x04    /* ORG referenced */
#define SF_BSS      0x10    /* uninitialized area (U flag) */
#define SF_RORG     0x20    /* relocatable origin active */

typedef struct _SEGMENT SEGMENT;
struct _SEGMENT
{
    /* next segment in segment list */
    SEGMENT *next;
    /* name of segment */
    char *name;
    /* for ORG */
    unsigned char flags;
    /* for RORG */
    unsigned char rflags;
    /* current org */
    unsigned long org;
    /* current rorg */
    unsigned long rorg;
    /* TODO: ??? all these ??? [phf] */
    unsigned long initorg;
    unsigned long initrorg;
    unsigned char initflags;
    unsigned char initrflags;
};

#define SYM_UNKNOWN 0x01    /*      value unknown     */
#define SYM_REF     0x04    /*      referenced        */
#define SYM_STRING  0x08    /*      result is a string    */
#define SYM_SET     0x10    /*      SET instruction used    */
#define SYM_MACRO   0x20    /*      symbol is a macro    */
#define SYM_MASREF  0x40    /*      master reference    */

typedef struct _SYMBOL SYMBOL;
struct _SYMBOL
{
    /* next symbol in hash list */
    SYMBOL *next;
    /* symbol name or string if expr. */
    char *name;
    /* if symbol is actually a string */
    char *string;
    /* flags */
    unsigned char flags;
    /* addressing mode (expressions) */
    unsigned char addrmode;
    /* current value, never EVER change this to unsigned! [phf] */
    long value;
    /* name length */
    unsigned int namelen;
};

extern SYMBOL    *SHash[];
extern MNEMONIC    *MHash[];
extern INCFILE    *pIncfile;
extern REPLOOP    *Reploop;
extern SEGMENT    *Seglist;
extern IFSTACK    *Ifstack;

extern SEGMENT    *Csegment;  /*      current segment */
extern char    *Av[];
extern char    Avbuf[];
/*extern unsigned int Adrbytes[];*/
extern unsigned int Cvt[];
extern MNEMONIC    Ops[];
extern unsigned int    Opsize[];
extern int    Mnext;          /*    mnemonic extension    */
extern unsigned int    Mlevel;

extern bool bTrace;
extern bool     Xdebug;
extern unsigned char    MsbOrder;
extern unsigned char    Outputformat;
extern unsigned long    Redo_why;

extern int Redo;
extern int Redo_eval;

extern unsigned long    Redo_if;
extern unsigned long    Localindex, Lastlocalindex;
extern unsigned long    Localdollarindex, Lastlocaldollarindex;
extern int   F_format;
extern sortmode_t F_sortmode; /* -T option [phf] */
extern unsigned char    F_verbose;
extern const char    *F_outfile;
extern char    *F_listfile;
extern char    *F_symfile;
extern FILE    *FI_listfile;
extern FILE    *FI_temp;
extern unsigned char    Fisclear;
extern unsigned long    Plab, Pflags;
extern char    Inclevel;
extern char    ListMode;
extern unsigned long    Processor;

extern unsigned long  CheckSum;

/* main.c */
/*extern unsigned char Listing;*/
void    findext(char *str);
char   *sftos(long val, int flags);
void    rmnode(void **base, int bytes);
void    addhashtable(MNEMONIC *mne);
void    pushinclude(char *str);

/* symbols.c */
void    setspecial(int value, int flags);
SYMBOL *allocsymbol(void);
SYMBOL *findsymbol(const char *str, int len);
SYMBOL *CreateSymbol( const char *str, int len );
void    FreeSymbolList(SYMBOL *sym);
void    programlabel(void);

/* ops.c */
extern    unsigned char Gen[];
extern    int Glen;
void    v_set(char *str, MNEMONIC *);
void    v_mexit(char *str, MNEMONIC *);
void    closegenerate(void);
void    generate(void);

void v_list(char *, MNEMONIC *);
void v_include(char *, MNEMONIC *);
void v_seg(char *, MNEMONIC *);
void v_dc(char *, MNEMONIC *);
void v_ds(char *, MNEMONIC *);
void v_org(char *, MNEMONIC *);
void v_rorg(char *, MNEMONIC *);
void v_rend(char *, MNEMONIC *);
void v_align(char *, MNEMONIC *);
void v_subroutine(char *, MNEMONIC *);
void v_equ(char *, MNEMONIC *);
void v_eqm(char *, MNEMONIC *);
void v_set(char *, MNEMONIC *);
void v_macro(char *, MNEMONIC *);
void v_endm(char *, MNEMONIC *);
void v_mexit(char *, MNEMONIC *);
void v_ifconst(char *, MNEMONIC *);
void v_ifnconst(char *, MNEMONIC *);
void v_if(char *, MNEMONIC *);
void v_else(char *, MNEMONIC *);
void v_endif(char *, MNEMONIC *);
void v_repeat(char *, MNEMONIC *);
void v_repend(char *, MNEMONIC *);
void v_err(char *, MNEMONIC *);
void v_hex(char *, MNEMONIC *);
void v_trace(char *, MNEMONIC *);
void v_end(char *, MNEMONIC *);
void v_echo(char *, MNEMONIC *);
void v_processor(char *, MNEMONIC *);
void v_incbin(char *, MNEMONIC *);
void v_incdir(char *, MNEMONIC *);
void v_execmac(char *str, MACRO *mac);
void v_mnemonic(char *str, MNEMONIC *mne);

FILE *pfopen(const char *, const char *);


/* exp.c */
SYMBOL *eval(const char *str, int wantmode);

/* end of asm.h */

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4 autoindent: */
