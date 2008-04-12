
/*
 *  FTOBIN.C
 *
 *  (c)Copyright 1988, Matthew Dillon, All Rights Reserved.  See terms in
 *  README file.
 *
 *  FTOBIN format infile [outfile]
 *
 *  format: format used when assembling (asm705/asm65)
 *	    1,2,3	    -generate sequenced (raw) binary file
 *
 *  Used with formats 1 or 2 (format 3 is already raw binary).	This
 *  program converts formats 1 and 2 to format 3.  This is required
 *  because some assembly programs might want to reverse-index the
 *  origin (go back), usually to lay down a checksum, and this can only be
 *  done with format 2.
 *
 *  Restrictions:   Lowest address must be referenced first.
 *
 *  compilable on an ibm-pc or Amiga  _fmode is for Lattice C on the ibm,
 *  is IGNORED by Aztec C on the Amiga.  Note that INT and CHAR are not
 *  used as ibm's lattice C uses 16 bit ints and unsigned chars.  Change
 *  as needed.	No guarentees for the IBMPC version.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define IBM

typedef unsigned char ubyte;
typedef unsigned short uword;

#define PERLINE 16

static uword getwlh(FILE *in);
static void exiterr(const char *str);
static void convert(short format, FILE *in, FILE *out);

uword _fmode = 0;

int
main(int ac, char **av)
{
    short format;
    FILE *infile;
    FILE *outfile;

    _fmode = 0x8000;
    if (ac < 3) {
	puts("FTOBIN format infile [outfile]");
	puts("format 1,2, or 3.  3=raw");
	puts("(C)Copyright 1988 by Matthew Dillon, All Rights Reserved");
	exit(1);
    }
    format = strtol(av[1], NULL, 0);
    if (format < 1 || format > 3)
	exiterr("specify infile format 1, 2, or 3");
    if (format == 3) {
	puts("Note: Format 3 is already a raw binary file, output will");
	puts("be equivalent to input");
    }
    infile = fopen(av[2], "r");
    if (infile == NULL)
	exiterr("unable to open input file");
    outfile = (av[3]) ? fopen(av[3], "w") : stdout;
    if (outfile == NULL)
	exiterr("unable to open output file");
    convert(format, infile, outfile);
    fclose(infile);
    fclose(outfile);
    return(0);
}

static
void
exiterr(const char *str)
{
    fputs(str, stderr);
    fputs("\n", stderr);
    exit(1);
}

/*
 *  Formats:
 *
 *  1:	origin (word:lsb,msb) + data
 *  2:	origin (word:lsb,msb) + length (word:lsb,msb) + data  (repeat)
 *  3:	data
 *
 *  Hex output:
 *
 *  :lloooo00(ll bytes hex code)cc	  ll=# of bytes
 *					oooo=origin
 *					  cc=invert of checksum all codes
 */

static
void
convert(short format, FILE *in, FILE *out)
{
    uword org = 0;
    uword base = 0;
    long len;
    long maxseek = 0;
    ubyte buf[256];

    if (format < 3)
	base = org = getwlh(in);
    if (format == 2) {
	len = getwlh(in);
    } else {
	long begin = ftell(in);
	fseek(in, 0L, 2);
	len = ftell(in) - begin;
	fseek(in, begin, 0);
    }
    for (;;) {
	if (len > 0) {
	    while (len >= sizeof(buf)) {
		fread(buf, sizeof(buf), 1, in);
		fwrite(buf, sizeof(buf), 1, out);
		len -= sizeof(buf);
		org += sizeof(buf);
	    }
	    if (len) {
		fread(buf, (short)len, 1, in);
		fwrite(buf, (short)len, 1, out);
		org += len;
	    }
	}
	if (format == 2) {
	    if (maxseek < org - base)
		maxseek = org - base;
	    org = getwlh(in);
	    if (feof(in))
		break;
	    len = org - base;
	    if (len < 0) {
		puts("ERROR!  Reverse indexed to before beginning");
		puts("*Initial* origin must be the lowest address");
		return;
	    }
	    len -= maxseek;
	    if (len > 0)
		memset(buf, 255, sizeof(buf));
	    while (len > 0) {
		if (len >= sizeof(buf)) {
		    fwrite(buf, sizeof(buf), 1, out);
		    maxseek += sizeof(buf);
		    len -= sizeof(buf);
		} else {
		    fwrite(buf, (short)len, 1, out);
		    maxseek += len;
		    len = 0;
		}
	    }
	    fseek(out, (long)(org - base), 0);
	    len = getwlh(in);
	} else {
	    break;
	}
    }
}

static
uword
getwlh(FILE *in)
{
    uword result;

    result = getc(in);
    result += getc(in) << 8;
    return(result);
}

