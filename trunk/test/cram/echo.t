The ECHO directive allows for output during assembly.

  $ cat <<EOF >echo.asm
  >  .processor 6502
  >  .echo
  >  .end
  > EOF

An "empty" ECHO prints a "$0" and I do not believe that is correct? If anything
it should print an empty line, should it not? Opinions?

  $ $TESTDIR/dasm echo.asm
  

Testing with a few symbols/labels next.

  $ cat <<EOF >echo.asm
  >  .processor 6502
  > one .equ 1
  >  .echo one
  >  .echo two
  > two .equ 2
  >  .echo three
  >  .end
  > EOF

An ECHO with an unresolved label seems to result in an empty line even if the
label *should* resolve in the second pass. Note that the single space in front
of the output is apparently intentional; only Matthew Dillon knows why.

  $ $TESTDIR/dasm echo.asm
   $1
   $2
  some error message?

Now we try dealing with strings in ECHO.

  $ cat <<EOF >echo.asm
  >  .processor 6502
  >  .echo "eins"
  >  .echo "crap" "zero"
  >  .echo "zwei", "drei"
  >  .echo "A string" 10
  >  .end
  > EOF

At least we get syntax errors in the lines that have the expressions *not*
separated by commas, that is great. What is less great is that we get some
kind of output anyway, namely the *second* expression: notice how "crap"
and "A string" are missing but "zero" and 10 trigger output. It is unclear
what the "correct" output should be in those cases.

  $ $TESTDIR/dasm echo.asm
  echo.asm (?): error: Syntax error in '"crap" "zero"'! (glob)
  echo.asm (?): error: Syntax error in '"A string" 10'! (glob)
   eins
   zero
   zwei drei
   $a
  [1]

Finally for some numbers and basic arithmetic, just for kicks.

  $ cat <<EOF >echo.asm
  >  .processor 6502
  >  .echo 7
  >  .echo 12 / 5
  >  .echo 12 % 5
  >  .echo 255
  >  .echo \$ff
  >  .echo \$0ff
  >  .echo \$0ffaaffbb
  >  .end
  > EOF

Looks good. Note that we had to escape the dollar signs in the input, otherwise
cram shoots us in the foot.

  $ $TESTDIR/dasm echo.asm
   $7
   $2
   $2
   $ff
   $ff
   $ff
   $ffaaffbb

And that is it for now with tests of the ECHO directive.
