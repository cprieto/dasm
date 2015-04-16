======
ftohex
======

--------------------------------------------
convert DASM object file to Intel HEX format
--------------------------------------------

.. |date| date::
.. |time| date:: %H:%M

:Manual section: 1

:Authors: Matthew Dillon; Olaf "Rhialto" Seibert; Andrew Davie;
          Peter H. Froehlich.
:Date: |date| |time|
:Version: 2.10.12
:Copyright: This document is licensed under the same terms as DASM
            itself, see the file COPYING for details.

SYNOPSIS
========

ftohex format objectfile [hexfile]

DESCRIPTION
===========

A companion tool to `dasm(1)`, ftohex converts a `dasm(1)` object file
to Intel HEX format suitable for, say, a GTEK prom programmer.

You must specify the format you used when you assembled the source for
ftohex to properly read the object file. Generally DASM's Random Access
Segment Format (format 2) is used for assembly as this generates the
smallest hex file. If hexfile is not given, ftohex writes to stdout.

EXAMPLE
=======

| dasm -f2 example.asm -oexample.out
| ftohex 2 example.out example.hex

EXIT CODES
==========

Generally ftohex will exit with code `0` if it was able to complete
the conversion; it will exit with code `1` if it encountered a
problem.

BUGS
====

Mostly features actually. Okay, just kidding. Yes, there are bugs.

SEE ALSO
========

* `dasm(1)`
* `ftobin(1)`
