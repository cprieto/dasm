
/*
*  MAIN.C
*
*  (c)Copyright 1988, Matthew Dillon, All Rights Reserved.
*     Freely Distributable (for non-profit) ONLY.  No redistribution
*     of any modified text files or redistribution of a subset of the
*     source is allowed.  Redistribution of modified binaries IS allowed
*     under the above terms.
*
*  DASM   sourcefile
*
*  NOTE: must handle mnemonic extensions and expression decode/compare.
*/

#include "asm.h"

#define MAXLINE 256
#define ISEGNAME    "INITIAL CODE SEGMENT"

char *cleanup(char *buf);
MNE *parse(char *buf);
void panic(char *str);
MNE *findmne(char *str);
void clearsegs(void);
void clearrefs(void);

static uword hash1(char *str);
static void outlistfile(char *);

//uword _fmode = 0;    /*  was trying to port to 16 bit IBM-PC lattice C */
/*      but failed    */

ubyte     Disable_me;
ubyte     StopAtEnd = 0;
char     *Extstr;
ubyte     Listing = 1;
int     pass;
#if OlafListAll
ubyte     F_ListAllPasses = 0;
#endif
#if OlafPasses
ubyte     F_passes = 10;
ubyte     F_Passes;
#else
#define  F_passes 10
#endif

const char name[] = "DASM V2.20.04, Macro Assembler (C)1988-2003";


int nTableSort = 0;                 // Sorting preference for symbol table output


char *Errors[] = {
        
        "OK",
        "Check command-line format.",
        "Unable to open file.",
        "Source is not resolvable.",
        "Too many passes (%s).",

        "Syntax Error '%s'.",
        "Expression table overflow.",
        "Unbalanced Braces [].",
        "Division by zero.",
        "Unknown Mnemonic '%s'.",
        "Illegal Addressing mode '%s'.",
        "Illegal forced Addressing mode on '%s'.",
        "Not enough args passed to Macro.",
        "Premature EOF.",
        "Illegal character.",
        "Branch out of range (%s bytes).",
        "ERR pseudo-op encountered.",
        "Origin Reverse-indexed.",
        "EQU: Value mismatch.",
        "Value in '%s' must be <$100.",
        "Illegal bit specification.",
        "Not enough args.",
        "Label mismatch...\n --> %s",
        "Value Undefined.",
        "Processor '%s' not supported.",
        "REPEAT parameter < 0 (ignored).",        /* 21 ERROR_REPEAT_NEGATIVE */

        NULL
};


int MainShadow(int ac, char **av);

int
main(int ac, char **av)
{
    int nError = MainShadow( ac, av );
    if ( nError )
        printf( "Fatal assembly error: %s\n", Errors[ nError ] );
    return nError;
}



int CountUnresolvedSymbols()
{
    SYMBOL *sym;
    int nUnresolved = 0;
    int i;

    /* Pre-count unresolved symbols */
    for (i = 0; i < SHASHSIZE; ++i)
        for (sym = SHash[i]; sym; sym = sym->next)
            if ( sym->flags & SYM_UNKNOWN )
                nUnresolved++;

    return nUnresolved;
}


int ShowUnresolvedSymbols()
{
    SYMBOL *sym;
    int i;

    int nUnresolved = CountUnresolvedSymbols();
    if ( nUnresolved )
    {
        printf( "--- Unresolved Symbol List\n" );

        /* Display unresolved symbols */
        for (i = 0; i < SHASHSIZE; ++i)
            for (sym = SHash[i]; sym; sym = sym->next)
                if ( sym->flags & SYM_UNKNOWN )
                    printf( "%-24s %s\n", sym->name, sftos( sym->value, sym->flags ) );

        printf( "--- %d Unresolved Symbol%c\n\n", nUnresolved, ( nUnresolved == 1 ) ? ' ' : 's' );
    }

    return nUnresolved;
}


int CompareAlpha( const void *arg1, const void *arg2 )
{
    /* Simple alphabetic ordering comparison function for quicksort */

    SYMBOL **sym1, **sym2;

    sym1 = (SYMBOL **) arg1;
    sym2 = (SYMBOL **) arg2;

    return _stricmp( (*sym1)->name, (*sym2)->name );
}

int CompareAddress( const void *arg1, const void *arg2 )
{
    /* Simple alphabetic ordering comparison function for quicksort */

    SYMBOL **sym1, **sym2;

    sym1 = (SYMBOL **) arg1;
    sym2 = (SYMBOL **) arg2;

    return (*sym1)->value - (*sym2)->value;
}


void ShowSymbols()
{
    /* Display sorted (!) symbol table - if it runs out of memory, table will be displayed unsorted */

    SYMBOL **symArray;
    SYMBOL *sym;
    int i;
    int nSymbols = 0;

    printf("--- Symbol List");

    /* Sort the symbol list either via name, or by value */

    /* First count the number of symbols */
    for (i = 0; i < SHASHSIZE; ++i)
        for (sym = SHash[i]; sym; sym = sym->next)
            nSymbols++;

    /* Malloc an array of pointers to data */

    symArray = malloc( sizeof( SYMBOL * ) * nSymbols );
    if ( !symArray )
    {
        printf( " (unsorted - not enough memory to sort!)\n" );

        /* Display complete symbol table */
        for (i = 0; i < SHASHSIZE; ++i)
            for (sym = SHash[i]; sym; sym = sym->next)
                printf("%-24s %s\n", sym->name, sftos(sym->value, sym->flags));
    }
    else
    {
        /* Copy the element pointers into the symbol array */

        bool bRepeat;
        int nPtr = 0;

        for (i = 0; i < SHASHSIZE; ++i)
            for (sym = SHash[i]; sym; sym = sym->next)
                symArray[ nPtr++ ] = sym;
            
        if ( nTableSort )
        {
            printf( " (sorted by address)\n" );
            qsort( symArray, nPtr, sizeof( SYMBOL * ), CompareAddress );           /* Sort via address */
        }
        else
        {
            printf( " (sorted by symbol)\n" );
            qsort( symArray, nPtr, sizeof( SYMBOL * ), CompareAlpha );              /* Sort via name */
        }

        /* now display sorted list */

        for ( i = 0; i < nPtr; i++ )
            printf( "%-24s %s\n", symArray[ i ]->name,
                sftos( symArray[ i ]->value, symArray[ i ]->flags ) );
            
        free( symArray );
    }

    puts("--- End of Symbol List.\n" );

}



void ShowSegments()
{
    SEGMENT *seg;
    char *bss;
    char *sFormat = "%-24s %-3s %-8s %-8s %-8s %-8s\n\0";
    
    

    printf("\n----------------------------------------------------------------------\n");
    printf( sFormat, "SEGMENT NAME", "", "INIT PC", "INIT RPC", "FINAL PC", "FINAL RPC" );
   
    for (seg = Seglist; seg; seg = seg->next)
    {
        bss = (seg->flags & SF_BSS) ? "[u]" : "   ";

        printf( sFormat, seg->name, bss,
            sftos(seg->initorg, seg->initflags), sftos(seg->initrorg, seg->initrflags),
            sftos(seg->org, seg->flags), sftos(seg->rorg, seg->rflags) );
    }
    puts("----------------------------------------------------------------------");

    printf( "%d references to unknown symbols.\n", Redo_eval );
    printf( "%d events requiring another assembler pass.\n", Redo );
    
    if ( Redo_why )
    {
        if ( Redo_why & REASON_MNEMONIC_NOT_RESOLVED )
            printf( " - Expression in mnemonic not resolved.\n" );

        if ( Redo_why & REASON_OBSCURE )
            printf( " - Obscure reason - to be documented :)\n" );

        if ( Redo_why & REASON_DC_NOT_RESOVED )
            printf( " - Expression in a DC not resolved.\n" );

        if ( Redo_why & REASON_DV_NOT_RESOLVED_PROBABLY )
            printf( " - Expression in a DV not resolved (probably in DV's EQM symbol).\n" );
        
        if ( Redo_why & REASON_DV_NOT_RESOLVED_COULD )
            printf( " - Expression in a DV not resolved (could be in DV's EQM symbol).\n" );

        if ( Redo_why & REASON_DS_NOT_RESOLVED )
            printf( " - Expression in a DS not resolved.\n" );
    
        if ( Redo_why & REASON_ALIGN_NOT_RESOLVED )
            printf( " - Expression in an ALIGN not resolved.\n" );
    
        if ( Redo_why & REASON_ALIGN_RELOCATABLE_ORIGIN_NOT_KNOWN )
            printf( " - ALIGN: Relocatable origin not known (if in RORG at the time).\n" );
    
        if ( Redo_why & REASON_ALIGN_NORMAL_ORIGIN_NOT_KNOWN )
            printf( " - ALIGN: Normal origin not known	(if in ORG at the time).\n" );
    
        if ( Redo_why & REASON_EQU_NOT_RESOLVED )
            printf( " - EQU: Expression not resolved.\n" );
    
        if ( Redo_why & REASON_EQU_VALUE_MISMATCH )
            printf( " - EQU: Value mismatch from previous pass (phase error).\n" );
    
        if ( Redo_why & REASON_IF_NOT_RESOLVED )
            printf( " - IF: Expression not resolved.\n" );
    
        if ( Redo_why & REASON_REPEAT_NOT_RESOLVED )
            printf( " - REPEAT: Expression not resolved.\n" );
    
        if ( Redo_why & REASON_FORWARD_REFERENCE )
            printf( " - Label defined after it has been referenced (forward reference).\n" );
    
        if ( Redo_why & REASON_PHASE_ERROR )
            printf( " - Label value is different from that of the previous pass (phase error).\n" );
    }

    printf( "\n" );

}



int
MainShadow(int ac, char **av)
{
    int nError = ERROR_NONE;

    char buf[MAXLINE];
    int i;
    MNE *mne;
    ulong oldredo = -1;
    ulong oldwhy = 0;
    ulong oldeval = 0;
    
    addhashtable(Ops);
    pass = 1;
    
    if (ac < 2)
    {
        
fail:
        puts("redistributable for non-profit only");
        puts("");
        puts("DASM sourcefile [options]");
        puts(" -f#      output format");
        puts(" -oname   output file");
        puts(" -lname   list file");
#if OlafListAll
        puts(" -Lname   list file, containing all passes");
#endif
        puts(" -sname   symbol dump");
        puts(" -v#      verboseness");
        puts(" -t#      Symbol Table sorting preference (#1 = by address.  default #0 = alphabetic)" );
        puts(" -Dname=exp   define label");
        puts(" -Mname=exp   define label as in EQM");

#if OlafIncdir
        puts(" -Idir    search directory for include and incbin");
#endif
#if OlafPasses
        puts(" -p#      max number of passes");
        puts(" -P#      max number of passes, with less checks");
#endif
        return ERROR_COMMAND_LINE;
    }

    puts(name);
    
    for (i = 2; i < ac; ++i)
    {
        if ( ( av[i][0] == '-' ) || ( av[i][0] == '/' ) )
        {
            register char *str = av[i]+2;
            switch(av[i][1])
            {

            case 'T':
                nTableSort = atoi( str );
                break;

            case 'd':
                Xdebug = atoi(str) != 0;
                printf( "Debug trace %s\n", Xdebug ? "ON" : "OFF" );
                break;

            case 'M':
            case 'D':
                while (*str && *str != '=')
                    ++str;
                if (*str == '=')
                {
                    *str = 0;
                    ++str;
                }
                else
                {
                    str = "0";
                }
                Av[0] = av[i]+2;

                if (av[i][1] == 'M')
                    v_eqm(str, NULL);
                else
                    v_set(str, NULL);
                break;

            case 'f':   /*  F_format    */
                F_format = atoi(str);
                if (F_format < 1 || F_format > 3)
                    panic("Illegal format specification");
                break;

            case 'o':   /*  F_outfile   */
                F_outfile = str;
nofile:
                if (*str == 0)
                    panic("-o Switch requires file name.");
                break;
#if OlafListAll
            case 'L':
                F_ListAllPasses = 1;
                /* fall through to 'l' */
#endif
            case 'l':   /*  F_listfile  */
                F_listfile = str;
                goto nofile;
#if OlafPasses
            case 'P':   /*  F_Passes   */
                F_Passes = 1;
                /* fall through to 'p' */
            case 'p':   /*  F_passes   */
                F_passes = atoi(str);
                break;
#endif
            case 's':   /*  F_symfile   */
                F_symfile = str;
                goto nofile;
            case 'v':   /*  F_verbose   */
                F_verbose = atoi(str);
                break;
            case 't':   /*  F_temppath  */
                F_temppath = str;
                break;
#if OlafIncdir
            case 'I':
                v_incdir(str, NULL);
                break;
#endif
            default:
                goto fail;
            }
            continue;
        }
        goto fail;
    }
    
    /*    INITIAL SEGMENT */
    
    {
        register SEGMENT *seg = (SEGMENT *)permalloc(sizeof(SEGMENT));
        seg->name = strcpy(permalloc(sizeof(ISEGNAME)), ISEGNAME);
        seg->flags= seg->rflags = seg->initflags = seg->initrflags = SF_UNKNOWN;
        Csegment = Seglist = seg;
    }
    /*    TOP LEVEL IF    */
    {
        register IFSTACK *ifs = (IFSTACK *)zmalloc(sizeof(IFSTACK));
        ifs->file = NULL;
        ifs->flags = IFF_BASE;
        ifs->acctrue = 1;
        ifs->xtrue  = 1;
        Ifstack = ifs;
    }


nextpass:


    if ( F_verbose )
    {
        puts("");
        printf("START OF PASS: %d\n", pass);
    }

    Localindex = Lastlocalindex = 0;
#if OlafDol
    Localdollarindex = Lastlocaldollarindex = 0;
#endif
    _fmode = 0x8000;
    FI_temp = fopen(F_outfile, "w");
    _fmode = 0;
    Fisclear = 1;
    CheckSum = 0;
    if (FI_temp == NULL) {
        printf("unable to [re]open '%s'\n", F_outfile);
        return ERROR_FILE_ERROR;
    }
    if (F_listfile) {
#if OlafListAll
        FI_listfile = fopen(F_listfile,
            F_ListAllPasses && (pass > 1)? "a" : "w");
#else
        FI_listfile = fopen(F_listfile, "w");
#endif
        if (FI_listfile == NULL) {
            printf("unable to [re]open '%s'\n", F_listfile);
            return ERROR_FILE_ERROR;
        }
    }
    pushinclude(av[1]);

    while (Incfile) {
        for (;;) {
            char *comment;
            if (Incfile->flags & INF_MACRO) {
                if (Incfile->strlist == NULL) {
                    Av[0] = "";
                    v_mexit(NULL, NULL);
                    continue;
                }
                strcpy(buf, Incfile->strlist->buf);
                Incfile->strlist = Incfile->strlist->next;
            } else {
                if (fgets(buf, MAXLINE, Incfile->fi) == NULL)
                    break;
            }

            if (Xdebug)
                printf("%08lx %s\n", (unsigned long)Incfile, buf);

            comment = cleanup(buf);
            ++Incfile->lineno;
            mne = parse(buf);
            
            if (Av[1][0])
            {
                if (mne)
                {
                    if ((mne->flags & MF_IF) || (Ifstack->xtrue && Ifstack->acctrue))
                        (*mne->vect)(Av[2], mne);
                }
                else
                {
                    if (Ifstack->xtrue && Ifstack->acctrue)
                        asmerr( ERROR_UNKNOWN_MNEMONIC, false, Av[1] );
                }

            }
            else
            {
                if (Ifstack->xtrue && Ifstack->acctrue)
                    programlabel();
            }
            
            if (F_listfile && ListMode)
                outlistfile(comment);
        }
        
        while (Reploop && Reploop->file == Incfile)
            rmnode((void **)&Reploop, sizeof(REPLOOP));

        while (Ifstack->file == Incfile)
            rmnode((void **)&Ifstack, sizeof(IFSTACK));
        
        fclose(Incfile->fi);
        free(Incfile->name);
        --Inclevel;
        rmnode((void **)&Incfile, sizeof(INCFILE));
        
        if (Incfile)
        {
        /*
        if (F_verbose > 1)
        printf("back to: %s\n", Incfile->name);
            */
            if (F_listfile)
                fprintf(FI_listfile, "------- FILE %s\n", Incfile->name);
        }
    }



    if ( F_verbose >= 1 )
        ShowSegments();

    if ( F_verbose >= 3 )
    {
        if ( !Redo || ( F_verbose == 4 ) )
            ShowSymbols();

        ShowUnresolvedSymbols();
    }
    
    closegenerate();
    fclose(FI_temp);
    if (FI_listfile)
        fclose(FI_listfile);
    if (Redo) {
#if OlafPasses
        if (!F_Passes)
#endif
            if (Redo == oldredo && Redo_why == oldwhy && Redo_eval == oldeval)
            {
                ShowUnresolvedSymbols();
                return ERROR_NOT_RESOLVABLE;
            }

            oldredo = Redo;
            oldwhy = Redo_why;
            oldeval = Redo_eval;
            Redo = 0;
            Redo_why = 0;
            Redo_eval = 0;
#if OlafPhase
            Redo_if <<= 1;
#endif
            ++pass;
            
            if (StopAtEnd)
            {
                printf("Unrecoverable error(s) in pass, aborting assembly!\n");
            }
            else if (pass > F_passes)
            {
                char sBuffer[64];
                itoa( pass, sBuffer, 10);
                /*printf("More than %d passes, something *must* be wrong!\n", F_passes);*/
                return asmerr( ERROR_TOO_MANY_PASSES, false, sBuffer );

            }
            else
            {
                clearrefs();
                clearsegs();
                goto nextpass;
            }
    }
    if (F_symfile) {
        FILE *fi = fopen(F_symfile, "w");
        if (fi) {
            register SYMBOL *sym;
            puts("dumping symbols...");
            for (i = 0; i < SHASHSIZE; ++i) {
                for (sym = SHash[i]; sym; sym = sym->next) {
                    fprintf(fi, "%-24s %s", sym->name, sftos(sym->value, sym->flags));
                    if (sym->flags & SYM_STRING)
                        fprintf(fi, " \"%s\"", sym->string);
                    putc('\n', fi);
                }
            }
            fclose(fi);
        } else {
            printf("unable to open symbol dump file '%s'\n", F_symfile);
        }
    }

    printf( "Complete.\n" );

    return nError;
}


int
tabit(char *buf1, char *buf2)
{
    register char *bp, *ptr;
    register int j, k;
    
    bp = buf2;
    ptr= buf1;
    for (j = 0; *ptr && *ptr != '\n'; ++ptr, ++bp, j = (j+1)&7) {
        *bp = *ptr;
        if (*ptr == '\t') {
            /* optimize out spaces before the tab */
            while (j > 0 && bp[-1] == ' ') {
                bp--;
                j--;
            }
            j = 0;
            *bp = '\t';         /* recopy the tab */
        }
        if (j == 7 && *bp == ' ' && bp[-1] == ' ') {
            k = j;
            while (k-- >= 0 && *bp == ' ')
                --bp;
            *++bp = '\t';
        }
    }
    while (bp != buf2 && (bp[-1] == ' ' || bp[-1] == '\t'))
        --bp;
    *bp++ = '\n';
    *bp = '\0';
    return((int)(bp - buf2));
}

static void
outlistfile(comment)
char *comment;
{
    char xtrue;
    char c;
    static char buf1[MAXLINE+32];
    static char buf2[MAXLINE+32];
    char *ptr;
    char *dot;
    register int i, j;
    
#if OlafList
    if (Incfile->flags & INF_NOLIST)
        return;
#endif
    
    xtrue = (Ifstack->xtrue && Ifstack->acctrue) ? ' ' : '-';
    c = (Pflags & SF_BSS) ? 'U' : ' ';
    ptr = Extstr;
    dot = "";
    if (ptr)
        dot = ".";
    else
        ptr = "";
    
    sprintf(buf1, "%7ld %c%s", Incfile->lineno, c, sftos(Plab, Pflags & 7));
    j = strlen(buf1);
    for (i = 0; i < Glen && i < 4; ++i, j += 3)
        sprintf(buf1+j, "%02x ", Gen[i]);
    if (i < Glen && i == 4)
        xtrue = '*';
    for (; i < 4; ++i) {
        buf1[j] = buf1[j+1] = buf1[j+2] = ' ';
        j += 3;
    }
    sprintf(buf1+j-1, "%c%-10s %s%s%s\t%s\n",
        xtrue, Av[0], Av[1], dot, ptr, Av[2]);
    if (comment[0]) { /*  tab and comment */
        j = strlen(buf1) - 1;
        sprintf(buf1+j, "\t;%s", comment);
    }
    fwrite(buf2, tabit(buf1,buf2), 1, FI_listfile);
    Glen = 0;
    Extstr = NULL;
}

char *
sftos(long val, int flags)
{
    static char buf[64];
    static char c;
    register char *ptr = (c) ? buf : buf + 32;
    
    memset( buf, 0, 64 );

    c = 1 - c;

    sprintf(ptr, "%04lx ", val);

    if (flags & SYM_UNKNOWN)
        strcat( ptr, "???? ");
    else
        strcat( ptr, "     " );

    if (flags & SYM_STRING)
        strcat( ptr, "str ");
    else
        strcat( ptr, "    " );

    if (flags & SYM_MACRO)
        strcat( ptr, "eqm ");
    else
        strcat( ptr, "    " );
    
 
    if (flags & (SYM_MASREF|SYM_SET))
    {
        strcat( ptr, "(" );
    }
    else
        strcat( ptr, " " );

    if (flags & (SYM_MASREF))
        strcat( ptr, "R" );
    else
        strcat( ptr, " " );


    if (flags & (SYM_SET))
        strcat( ptr, "S" );
    else
        strcat( ptr, " " );

    if (flags & (SYM_MASREF|SYM_SET))
    {
        strcat( ptr, ")" );
    }
    else
        strcat( ptr, " " );
    
    
    return ptr;
}

void
clearsegs(void)
{
    register SEGMENT *seg;
    
    for (seg = Seglist; seg; seg = seg->next) {
        seg->flags = (seg->flags & SF_BSS) | SF_UNKNOWN;
        seg->rflags= seg->initflags = seg->initrflags = SF_UNKNOWN;
    }
}

void
clearrefs(void)
{
    register SYMBOL *sym;
    register short i;
    
    for (i = 0; i < SHASHSIZE; ++i)
        for (sym = SHash[i]; sym; sym = sym->next)
            sym->flags &= ~SYM_REF;
}

char *
cleanup(char *buf)
{
    register char *str;
    register STRLIST *strlist;
    register int arg, add;
    char *comment = "";
    
    for (str = buf; *str; ++str)
    {
        switch(*str)
        {
        case ';':
            comment = (char *)str + 1;
            /*    FALL THROUGH    */
        case '\r':
        case '\n':
            goto br2;
        case TAB:
            *str = ' ';
            break;
        case '\'':
            ++str;
            if (*str == TAB)
                *str = ' ';
            if (*str == '\n' || *str == 0)
            {
                str[0] = ' ';
                str[1] = 0;
            }
            if (str[0] == ' ')
                str[0] = '\x80';
            break;
        case '\"':
            ++str;
            while (*str && *str != '\"')
            {
                if (*str == ' ')
                    *str = '\x80';
                ++str;
            }
            if (*str != '\"')
            {
                asmerr( ERROR_SYNTAX_ERROR, false, buf );
                --str;
            }
            break;
        case '{':
            if (Disable_me)
                break;

            if (Xdebug)
                printf("macro tail: '%s'\n", str);

            arg = atoi(str+1);
            for (add = 0; *str && *str != '}'; ++str)
                --add;
            if (*str != '}')
            {
                puts("end brace required");
                --str;
                break;
            }
            --add;
            ++str;

            if (Xdebug)
                printf("add/str: %d '%s'\n", add, str);

            for (strlist = Incfile->args; arg && strlist;)
            {
                --arg;
                strlist = strlist->next;
            }

            if (strlist)
            {
                add += strlen(strlist->buf);
                
                if (Xdebug)
                    printf("strlist: '%s' %d\n", strlist->buf, strlen(strlist->buf));

                if (str + add + strlen(str) + 1 > buf + MAXLINE)
                {
                    if (Xdebug)
                        printf("str %8ld buf %8ld (add/strlen(str)): %d %ld\n",
                        (unsigned long)str, (unsigned long)buf, add, (long)strlen(str));
                    panic("failure1");
                }

                memmove(str + add, str, strlen(str)+1);
                str += add;
                if (str - strlen(strlist->buf) < buf)
                    panic("failure2");
                memmove(str - strlen(strlist->buf), strlist->buf, strlen(strlist->buf));
                str -= strlen(strlist->buf);
                if (str < buf || str >= buf + MAXLINE)
                    panic("failure 3");
                --str;      /*  for loop increments string    */
            }
            else
            {
                asmerr( ERROR_NOT_ENOUGH_ARGUMENTS_PASSED_TO_MACRO, false, NULL );
                goto br2;
            }
            break;
        }
    }

br2:
    while(str != buf && *(str-1) == ' ')
        --str;
    *str = 0;
    
    return comment;
}

void
panic(char *str)
{
    puts(str);
    exit(1);
}

/*
*  .dir    direct              x
*  .ext    extended              x
*  .r          relative              x
*  .x          index, no offset          x
*  .x8     index, byte offset          x
*  .x16    index, word offset          x
*  .bit    bit set/clr
*  .bbr    bit and branch
*  .imp    implied (inherent)          x
*  .b                      x
*  .w                      x
*  .l                      x
*  .u                      x
*/


void
findext(char *str)
{
    Mnext = -1;
    Extstr = NULL;
#if OlafDotop
    if (str[0] == '.') {    /* Allow .OP for OP */
        return;
    }
#endif
    while (*str && *str != '.')
        ++str;
    if (*str) {
        *str = 0;
        ++str;
        Extstr = str;
        switch(str[0]|0x20) {
        case '0':
        case 'i':
            Mnext = AM_IMP;
            switch(str[1]|0x20) {
            case 'x':
                Mnext = AM_0X;
                break;
            case 'y':
                Mnext = AM_0Y;
                break;
            case 'n':
                Mnext = AM_INDWORD;
                break;
            }
            return;
            case 'd':
            case 'b':
            case 'z':
                switch(str[1]|0x20) {
                case 'x':
                    Mnext = AM_BYTEADRX;
                    break;
                case 'y':
                    Mnext = AM_BYTEADRY;
                    break;
                case 'i':
                    Mnext = AM_BITMOD;
                    break;
                case 'b':
                    Mnext = AM_BITBRAMOD;
                    break;
                default:
                    Mnext = AM_BYTEADR;
                    break;
                }
                return;
                case 'e':
                case 'w':
                case 'a':
                    switch(str[1]|0x20) {
                    case 'x':
                        Mnext = AM_WORDADRX;
                        break;
                    case 'y':
                        Mnext = AM_WORDADRY;
                        break;
                    default:
                        Mnext = AM_WORDADR;
                        break;
                    }
                    return;
                    case 'l':
                        Mnext = AM_LONG;
                        return;
                    case 'r':
                        Mnext = AM_REL;
                        return;
                    case 'u':
                        Mnext = AM_BSS;
                        return;
        }
    }
}

/*
*  bytes arg will eventually be used to implement a linked list of free
*  nodes.
*  Assumes *base is really a pointer to a structure with .next as the first
*  member.
*/

void
rmnode(void **base, int bytes)
{
    void *node;
    
    if ((node = *base) != NULL) {
        *base = *(void **)node;
        free(node);
    }
}

/*
*  Parse into three arguments: Av[0], Av[1], Av[2]
*/
MNE *
parse(char *buf)
{
    register int i, j;
    MNE *mne = NULL;
    
    i = 0;
    j = 1;
#if OlafFreeFormat
    /* Skip all initial spaces */
    while (buf[i] == ' ')
        ++i;
#endif
#if OlafHashFormat
        /*
        * If the first non-space is a ^, skip all further spaces too.
        * This means what follows is a label.
        * If the first non-space is a #, what follows is a directive/opcode.
    */
    while (buf[i] == ' ')
        ++i;
    if (buf[i] == '^') {
        ++i;
        while (buf[i] == ' ')
            ++i;
    } else if (buf[i] == '#') {
        buf[i] = ' ';   /* label separator */
    } else
        i = 0;
#endif
    Av[0] = Avbuf + j;
    while (buf[i] && buf[i] != ' ') {
#if OlafColon
        if (buf[i] == ':') {
            i++;
            break;
        }
#endif
        if ((unsigned char)buf[i] == 0x80)
            buf[i] = ' ';
        Avbuf[j++] = buf[i++];
    }
    Avbuf[j++] = 0;
#if OlafFreeFormat
    /* Try if the first word is an opcode */
    findext(Av[0]);
    mne = findmne(Av[0]);
    if (mne != NULL) {
    /* Yes, it is. So there is no label, and the rest
    * of the line is the argument
        */
        Avbuf[0] = 0;    /* Make an empty string */
        Av[1] = Av[0];    /* The opcode is the previous first word */
        Av[0] = Avbuf;    /* Point the label to the empty string */
    } else
#endif
    {    /* Parse the second word of the line */
        while (buf[i] == ' ')
            ++i;
        Av[1] = Avbuf + j;
        while (buf[i] && buf[i] != ' ') {
            if ((unsigned char)buf[i] == 0x80)
                buf[i] = ' ';
            Avbuf[j++] = buf[i++];
        }
        Avbuf[j++] = 0;
        /* and analyse it as an opcode */
        findext(Av[1]);
        mne = findmne(Av[1]);
    }
    /* Parse the rest of the line */
    while (buf[i] == ' ')
        ++i;
    Av[2] = Avbuf + j;
    while (buf[i]) {
        if (buf[i] == ' ') {
            while(buf[i+1] == ' ')
                ++i;
        }
        if ((unsigned char)buf[i] == 0x80)
            buf[i] = ' ';
        Avbuf[j++] = buf[i++];
    }
    Avbuf[j] = 0;
    
    return mne;
}



MNE *
findmne(char *str)
{
    register int i;
    register char c;
    register MNE *mne;
    char buf[64];
    
#if OlafDotop
    if (str[0] == '.') {    /* Allow .OP for OP */
        str++;
    }
#endif
    for (i = 0; (c = str[i]); ++i) {
        if (c >= 'A' && c <= 'Z')
            c += 'a' - 'A';
        buf[i] = c;
    }
    buf[i] = 0;
    for (mne = MHash[hash1(buf)]; mne; mne = mne->next) {
        if (strcmp(buf, mne->name) == 0)
            break;
    }
    return(mne);
}

void
v_macro(char *str, MNE *dummy)
{
    STRLIST *base;
    int defined = 0;
    register STRLIST **slp, *sl;
    register MACRO *mac;    /* slp, mac: might be used uninitialised */
    register MNE   *mne;
    register uword i;
    char buf[MAXLINE];
    int skipit = !(Ifstack->xtrue && Ifstack->acctrue);
    
    strlower(str);
    if (skipit) {
        defined = 1;
    } else {
        defined = (findmne(str) != NULL);
        if (F_listfile && ListMode)
            outlistfile("");
    }
    if (!defined) {
        base = NULL;
        slp = &base;
        mac = (MACRO *)permalloc(sizeof(MACRO));
        i = hash1(str);
        mac->next = (MACRO *)MHash[i];
        mac->vect = v_execmac;
        mac->name = strcpy(permalloc(strlen(str)+1), str);
        mac->flags = MF_MACRO;
        MHash[i] = (MNE *)mac;
    }
    while (fgets(buf, MAXLINE, Incfile->fi)) {
        char *comment;
        
        if (Xdebug)
            printf("%08lx %s\n", (unsigned long)Incfile, buf);
        
        ++Incfile->lineno;
        Disable_me = 1;
        comment = cleanup(buf);
        Disable_me = 0;
        mne = parse(buf);
        if (Av[1][0]) {
            if (mne && mne->flags & MF_ENDM) {
                if (!defined)
                    mac->strlist = base;
                return;
            }
        }
        if (!skipit && F_listfile && ListMode)
            outlistfile(comment);
        if (!defined) {
            sl = (STRLIST *)permalloc(STRLISTSIZE+1+strlen(buf));
            strcpy(sl->buf, buf);
            *slp = sl;
            slp = &sl->next;
        }
    }
    asmerr( ERROR_PREMATURE_EOF, true, NULL );
}


void
addhashtable(MNE *mne)
{
    register int i, j;
    uword opcode[NUMOC];
    
    for (; mne->vect; ++mne) {
        memcpy(opcode, mne->opcode, sizeof(mne->opcode));
        for (i = j = 0; i < NUMOC; ++i) {
            mne->opcode[i] = 0;     /* not really needed */
            if (mne->okmask & (1L << i))
                mne->opcode[i] = opcode[j++];
        }
        i = hash1(mne->name);
        mne->next = MHash[i];
        MHash[i] = mne;
    }
}


static uword
hash1(char *str)
{
    register uword result = 0;
    
    while (*str)
        result = (result << 2) ^ *str++;
    return(result & MHASHAND);
}

void
pushinclude(char *str)
{
    register INCFILE *inf;
    register FILE *fi;
    
    if ((fi = pfopen(str, "r")) != NULL) {
        if (F_verbose > 1 && F_verbose != 5 )
            printf("%.*s Including file \"%s\"\n", Inclevel*4, "", str);
        ++Inclevel;
        if (F_listfile)
#if OlafPasses
            fprintf(FI_listfile, "------- FILE %s LEVEL %d PASS %d\n", str, Inclevel, pass);
#else
        fprintf(FI_listfile, "------- FILE %s\n", str);
#endif
        inf = (INCFILE *)zmalloc(sizeof(INCFILE));
        inf->next    = Incfile;
        inf->name    = strcpy(ckmalloc(strlen(str)+1), str);
        inf->fi = fi;
        inf->lineno = 0;
        Incfile = inf;
        return;
    }
    printf("Warning: Unable to open '%s'\n", str);
    return;
}

char Stopend[] = {
    1,1,1,1,1,1,1,1,1,1,0,1,1,0,1,1,1,0,0,1,1
};



int asmerr(int err, bool abort, char *sText )
{
    char *str;
    INCFILE *incfile;
    
    if (Stopend[err])
        StopAtEnd = 1;
    for (incfile = Incfile; incfile->flags & INF_MACRO; incfile=incfile->next);
    str = Errors[err];
    
#ifdef DAD
    
    // Error output format changed to be Visual-Studio compatible.
    // Output now file (line): error: string
    
    if (F_listfile)
    {
        fprintf(FI_listfile, "%s (%d): error: ", incfile->name, incfile->lineno );
        fprintf(FI_listfile, str, sText ? sText : "" );
        fprintf(FI_listfile, "\n" );
    }
    printf( "%s (%d): error: ", incfile->name, incfile->lineno );
    printf( str, sText ? sText : "" );
    printf( "\n" );

    
#else
    
    if (F_listfile)
        fprintf(FI_listfile, "*line %7ld %-10s %s\n", incfile->lineno, incfile->name, str);
    printf("line %7ld %-10s %s\n", incfile->lineno, incfile->name, str);
    
#endif
    
    if ( abort )
    {
        puts("Aborting assembly");
        if (F_listfile)
            fputs("Aborting assembly\n", FI_listfile);
        
        exit( 1 );
    }

    return err;
}

char *
zmalloc(int bytes)
{
    char *ptr = malloc(bytes);
    if (ptr)
    {
        memset(ptr, 0, bytes);
        return(ptr);
    }
    panic("unable to malloc");
    return NULL;
}

char *
ckmalloc(int bytes)
{
    char *ptr = malloc(bytes);
    if (ptr)
    {
        return(ptr);
    }
    panic("unable to malloc");
    return NULL;
}

char *
permalloc(int bytes)
{
    static char *buf;
    static int left;
    char *ptr;
    
    /* Assume sizeof(union align) is a power of 2 */
    
    union align
    {
        long l;
        void *p;
        void (*fp)(void);
    };
    
    bytes = (bytes + sizeof(union align)-1) & ~(sizeof(union align)-1);
    if (bytes > left)
    {
        if ((buf = malloc(ALLOCSIZE)) == NULL)
            panic("unable to malloc");
        memset(buf, 0, ALLOCSIZE);
        left = ALLOCSIZE;
        if (bytes > left)
            panic("software error");
    }
    ptr = buf;
    buf += bytes;
    left -= bytes;
    return(ptr);
}

char *
strlower(char *str)
{
    register char c;
    register char *ptr;
    
    for (ptr = str; (c = *ptr); ++ptr)
    {
        if (c >= 'A' && c <= 'Z')
            *ptr = c | 0x20;
    }
    return(str);
}

