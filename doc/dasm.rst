====
dasm
====

---------------------------------------------------
versatile macro assembler for 8-bit microprocessors
---------------------------------------------------

.. |date| date::
.. |time| date:: %H:%M

:Manual section: 1

:Authors: Matthew Dillon; Olaf "Rhialto" Seibert; Andrew Davie;
          Peter H. Froehlich; Brian Watson.
:Date: |date| |time|
:Version: 2.10.12
:Copyright: This document is licensed under the same terms as DASM
            itself, see the file COPYING for details.

WARNING
=======

This is "work in progress" meaning the documentation in this new format
is *not* complete yet. However, I did want to commit what I had so far.
As of right now, dasm.txt is still the "official" documentation. But not
for much longer. -phf

SYNOPSIS
========

dasm sourcefile {option}

DESCRIPTION
===========

The DASM macro assembler accepts a source file containing assembly
instructions for a variety of 8-bit microprocessors. It generates
a corresponding object file (named "a.out" by default) in a variety
of formats depending on the options given.

Supported Processors
--------------------

DASM officially supports the following 8-bit microprocessors:

  - MOS 6502 (including 6507, 6510, and others; illegal NMOS
    6502 opcodes are also available)

  - Motorola 6803

  - Motorola 68HC11 (extension of Motorola 6803)

  - Motorola 68705 (no relation to the 16/32-bit 68000 series)

  - Hitachi 6303 (extension of Motorola 6803)

  - Fairchild F8 (that's actually an architecture, we
    presumably support the F3850 microprocessor)

DASM may also work for a number of compatible microprocessors,
but only the above have been tested in some detail. We hope to
support the 65C02 soon.

OPTIONS
=======

Note that DASM's option syntax is fairly rigid: sourcefile *must*
be the first argument followed by any number of options. DASM does
*not* allow spaces between an option and its value; it also does
*not* support GNU getopt-style long options yet, they are here as
an exercise in wishful thinking. Finally, on Windows, a slash ("/")
can be used instead of the more traditional dash ("-").

-m<processor> --processor <processor>
  Force the given processor as if the directive ".PROCESSOR <processor>"
  had been given on the first line of the source file. Allows assembling
  files that contain no .PROCESSOR directive. When this option is used,
  any .PROCESSOR directives in the source file will be ignored, although
  they'll generate a warning if they're for a different CPU than the one
  set here.

-o<filename> --object-file <filename>
  Name of the object file produced by DASM.
  Default is "a.out".

-l<filename> --listing-file <filename>
  Name of the listing file produced by DASM.
  Default is no listing file.

-L<filename> --list-all <filename>
  As -l above, but the listing contains all passes.

-s<filename> --symbol-dump <filename>
  Name of the symbol table file produced by DASM.
  Default is no symbol table file.

-v<number> --verbose <number>
  Verbosity level from 0 to 4.
  Each level is described in more detail below.
  The default level is 0.

-d<number> --debug <number>
  Debug mode for DASM developers, see source for details.

-D<name[=expression] --define <name[=expression]>
  Define symbol with given name to value of given expression.
  If no expression is given, define the symbol to value 0.
  Equivalent to using the .SET directive (see below).

-M<name[=expression] --eqm-define <name[=expression]>
  Define symbol with given name to given expression.
  Not giving an expression *should* be an error, however
  we currently allow it making the symbol evaluate to 0.
  Equivalent to using the .EQM directive (see below).

-I<filename> --include-dir <filename>
  Name of directory to search for .INCLUDE and .INCBIN directives.

-p<number> --passes <number>
  Maximum number of passes to attempt.
  DASM might give up earlier.
  The default is 10.

-P<number> --sloppy-passes <number>
  Exact number of passes to attempt.
  DASM will do this many regardless (except when fatal errors occur).
  The default is 10.

-T<number> --symbol-sorting <number>
  Select the criterion for sorting the symbol table dump
  (see the -s option above).
  Option 0 sorts by symbol name, ascending.
  Option 1 sorts by symbol value, ascending.

-f<number> --object-format <number>
  Object file format.
  Format 1 has a two-byte origin header.
  Format 2 supports multiple hunks and allows reverse-indexed origins.
  Format 3 is raw binary without any headers.
  Each format is described in more detail below.
  The default is format 1.

-E<number> --error-format <number>
  Format of error messages (actually most messages produced by DASM).
  Format 0 is Microsoft style.
  Format 1 is Matthew Dillon's original style.
  Format 2 is GNU style.
  Each format is described in more detail below.
  The default is format 0 although it should really be format 2.

-V --version
  Display version number and exit.

Example::
  dasm master.asm -f2 -oout -llist -v3 -DVER=4

Format Options
--------------

DASM produces only binary output in one of three formats described
below. A separate linker is not supported; instead one has a master
assembly file which INCLUDEs all the modules.

1. *Default:* The output file contains a two-byte origin in LSB,MSB
   order, then data until the end of the file.
   *Restrictions:* Any instructions which generate output (within an
   initialized segment) must do so with an ascending PC.
   Initialized segments must occur in ascending order.

2. *Random Access Segment (RAS):* The output file contains one or
   more *hunks*. Each hunk consists of a two-byte origin (LSB,MSB),
   two-byte length (LSB,MSB), and that number of data bytes. The
   hunks occur in the same order as initialized segments in the
   assembly.  There are no restrictions to segment ordering (i.e.
   reverse indexed ORG statements are allowed). The next hunk
   begins after the previous hunk's data, until the end of the file.

3. *Raw:* The output file contains data only (format #1 without
   the two-byte origin header). Restrictions are the same as for
   format #1.

Verbose Options
---------------

0. Only warnings and errors are displayed.

1. The following additional information is displayed after each pass:

   - Segment list
   - Include file names
   - Statistics on why the assembler is going to make another pass::

       R1,R2 reason code: R3

     R1 is the number of times the assembler encountered something
     requiring another pass to resolve.
     R2 is the number of references to unknown symbols which occured
     in the pass (but only R1 determines the need for another pass).
     R3 is a BITMASK of the reasons why another pass is required, see
     below for bit designations.

2. Mismatches between program labels and equates are displayed
   on every pass (usually none occur in the first pass unless
   you have re-declared a symbol name). Displayed information
   for symbols::

     ???? = unknown value
     str  = symbol is a string
     eqm  = symbol is an .EQM macro
     (r)  = symbol has been referenced
     (s)  = symbol created with .SET or .EQM

3. Unresolved and unreferenced symbols are displayed every pass
   (unsorted, sorry).

4. An *entire* symbol list is displayed every pass.

DIRECTIVES (PSEUDO-OPS)
=======================

Directives are "instructions" to the assembler itself as opposed to
instructions to the CPU we are generating code for. DASM used to refer to
directives as "pseudo-ops" in previous iterations of the documentation,
however that is a bit of a misnomer. We'll use the word "directive" from
now on.

In the following, optional components of a directive are surrounded by
brackets whereas repeated components are surrounded by braces. So, for
example, "[label]" means "0 or 1 label" but "{,expression}" means "0 or
more expressions, each preceeded by a comma" as it were. Alternatives are
seperated by the pipe character, so "A|B" means "either A or B" of course.

List of Directives (and synonyms)
---------------------------------
- DC expression{,expression}

  - BYTE expression{,expression}
  - LONG expression{,expression}
  - WORD expression{,expression}
- DS expression[,filler]
- DV eqmlabel expression{,expression}
- ELSE
- ENDIF

  - EIF
- ENDM
- ERR
- HEX hex {hex}
- IF expression
- IFCONST expression
- IFNCONST expression
- INCBIN "file path"
- INCDIR "directory path"
- INCLUDE "file path"
- LIST ON|OFF|LOCALON|LOCALOFF
- MAC name
- MEXIT
- REPEAT expression
- REPEND
- SEG name

Description of Directives
-------------------------

INCLUDE "file name"
  Process the given assembly source file at this point.

[label] INCBIN "file name"
  Copy the given (binary) file *literally* into the generated object file at
  this point.

INCDIR "directory name"
  Add the given directory name to the list of paths where INCLUDE and INCBIN
  search their files. First, the names are tried relative to the *current*
  directory; if that fails and the name is not an absolute pathname, the list
  is tried. You can optionally end the name with a /. AmigaDOS filename
  conventions imply that two slashes at the end of an INCDIR (dir//) indicates
  the parent directory, and so does an INCLUDE /filename.

  The command-line option -Idir is equivalent to an INCDIR directive placed
  before the source file.

  Currently the list is not cleared between passes, but each exact directory
  name is added to the list only once. This may change in subsequent releases.

[label] SEG[.U] name
  This sets the current segment, creating it if neccessary. If a .U extension
  is specified on segment creation, the segment is an UNINITIALIZED segment.
  The .U is not needed when going back to an already created uninitialized
  segment, though it makes the code more readable.

[label] DC[.(B|W|L)] expression{,expression}
  Declare data in the current segment. No output is generated if within a .U
  segment. Note that the byte ordering for the selected processor is used for
  each entry. The default size extension is a byte. BYTE, WORD, and LONG are
  synonyms for DC.B, DC.W, and DC.L respectively.

[label] DS[.(B|W|L)] expression[,filler]
  Declare space (default filler is 0). Data is not generated if within an
  uninitialized segment. Note that the number of bytes generated is exp *
  entrysize (1,2, or 4). The default size extension is a byte. Note that the
  default filler is always 0 (has nothing to do with the ORG default filler).

[label] DV[.(B|W|L)] eqmlabel expression{,expression}
  This is equivalent to DC, but each exp in the list is passed through the
  symbolic expression specified by the EQM label. The expression is held in
  a special symbol dotdot '..' on each call to the EQM label. See EQM below.

[label] HEX hex {hex}
  This sets down raw HEX data, each "hex" is two characters in hexadecimal
  notation denoting one byte. Spaces are optional between bytes. NO EXPRESSIONS
  are allowed. Note that you do NOT place a "$" in front of the digits. This is
  a short form for creating tables compactly. Data is always laid down on a
  byte-by-byte basis.

  Example:
    .HEX 1A45 45 13254F 3E12

ERR
  Abort assembly at this point.

[label] ORG expression[,filler]
  Sets the current origin. You can also set the global default filler (a byte
  value) with this directive. Note that no filler is generated until the first
  data-generating opcode/directive after this one is encountered. Sequences
  like::

    .ORG  0,255
    .ORG  100,0
    .ORG  200
    .DC.B 23

  will result in 200 zeros and a 23. This allows you to specify some ORG, then
  change your mind and specify some other (lower address) ORG without causing
  an error (assuming nothing is generated in-between). Normally, DS and ALIGN
  are used to generate specific filler values.

[label] RORG expression
  This activates the relocatable origin. All generated addresses, including
  '.', although physically placed at the true origin, will use values from
  the relocatable origin. While in effect both the physical origin and
  relocatable origin are updated.

  The relocatable origin can skip around (no limitations). The relocatable
  origin is a function of the segment. That is, you can still SEG to another
  segment that does not have a relocatable origin activated, do other
  (independant) stuff there, and then switch back to the current segment and
  continue where you left off.

PROCESSOR model
  Do not quote. The model is one of the following: 6502, 6803, HD6303, 68705,
  68HC11, and F8. Can only be executed once and should be the first thing
  encountered by the assembler.

  The command-line option -mmodel is equivalent to a PROCESSOR directive
  placed before the source file (see above).

ECHO expression{,expression}
  The expressions (which may also be strings), are echoed on the screen and
  into the list file at this point.

[label] REND
  Deactivate the relocatable origin for the current segment. Generation uses
  the real origin for reference.

[label] ALIGN N[,filler]
  Align the current PC to an N-byte boundary. The default filler byte is always
  0 and has nothing to do with the default filler byte specified with ORG.

[label] SUBROUTINE name
  This isn't really a subroutine, but a boundary between sets of temporary
  labels which begin with a dot. Temporary label names are unique within
  segments of code bounded by SUBROUTINE::

		CHARLIE subroutine
			ldx #10
		.1	dex
			bne .1
		BEN	subroutine
			ldx #20
		.qq	dex
			bne .qq

  Automatic temporary label boundaries occur for each macro level. Usually
  temporary labels are used in macros and within actual subroutines (so you
  don't have to think up a thousand different names).

MAC name
  Declare a macro. Lines between MAC and ENDM are the macro. You cannot
  recursively declare a macro. You *can* recursively use a macro (reference
  a macro in a macro). No label is allowed to the left of MAC or ENDM.

  Arguments passed to macros are referenced with: {#}. The first argument
  passed to a macro would thus be {1}. You should always use *local* labels
  (.name) inside macros which you use more than once. {0} represents an
  *exact* substitution of the *entire* argument line.

ENDM
  End of macro definition. NO LABEL ALLOWED ON THE LEFT!

MEXIT
  Used in conjuction with conditionals. Exits the current macro level.

[label] LIST ON|OFF|LOCALON|LOCALOFF
  Globally turns listing on or off, starting with the current line.
  When you give LOCALON or LOCALOFF the effect is local to the current
  macro or included file. For a line to be listed both the global and
  local list switches must be on.

[label] REPEAT expression
  Repeat code between REPEAT and the matching REPEND 'exp' times. If
  exp <= 0 the REPEAT loop is ignored (with a warning); exp is evaluated
  once. For example::

		Y   SET     0
		    REPEAT  10
		X   SET     0
		    REPEAT  10
		    DC	    X,Y
		X   SET     X + 1
		    REPEND
		Y   SET     Y + 1
		    REPEND

  generates an output table: 0,0 1,0 2,0 ... 9,0  0,1 1,1 2,1 ... 9,1, etc...
  Labels within a REPEAT/REPEND should be temporary labels with a SUBROUTINE
  pseudo-op to keep them unique.

[label] REPEND
  The label to the left of REPEND is assigned *after* the loop *finishes*.

[label] IF expression
  Is TRUE if the expression result is defined *and* non-zero. Is FALSE if
  the expression result is defined *and* zero. Neither IF or ELSE will be
  executed if the expression result is undefined.

  If the expression is undefined, another assembly pass is automatically
  taken. If this happens, phase errors in the next pass only will not be
  reported unless the verboseness is 1 or more.

[label] ELSE
  ELSE for the current IF.

[label] ENDIF
  Terminate the current IF. EIF is (regretably) a synonym for ENDIF
  (please don't use EIF in new code).

[label] IFCONST expression
  Is TRUE if the expression result is defined, FALSE otherwise and NO
  error is generated if the expression is undefined.

[label] IFNCONST expression
  Is TRUE if the expression result is undefined, FALSE otherwise and NO
  error is generated if the expression is undefined.

EXIT CODES
==========

In most cases, DASM will exit with code `0` if it (most likely)
produced a correct object file; it will exit with code `1` if
it (most likely) couldn't produce a correct object file. Even on
a failure, however, a (partial) object file might still have been
generated. Obviously these rules only apply if DASM was actually
*asked* to produce an object file.

BUGS
====

Mostly features actually. Okay, just kidding. Yes, there are bugs.

SEE ALSO
========

* `ftohex(1)`
* `ftobin(1)`
* http://dasm-dillon.sourceforge.net/

HISTORY
=======

The DASM macro assembler was originally written by Matthew Dillon
sometime between 1987 and 1988. Here is an excerpt from his original
preface:

    Over the last year my work has included writing software to
    drive small single-chip microcomputers for various things (remote
    telemetry units for instance). I have had need to program quite a
    few different processors over that time. At the beginning, I used
    an awful macro assembler running on an IBM-PC. I *really* wanted
    to do it on my Amiga. Thus the writing of this program.

History gets fuzzy after that because many people ended up producing
many different versions of DASM. A complete accounting for all these
strands of development may never be possible.

What is certain is that in 1995 Olaf "Rhialto" Seibert added a lot
of features to DASM, most of which are still present to this day.
For some reason DASM became particulary popular for programming the
Atari 2600 VCS. Andrew Davie took up DASM in 2003 and worked on it
until about 2008 when Peter Froehlich offered to take over for no
reason in particular (he just wanted a convoluted piece of software
to maintain to brush up on his C skills).
