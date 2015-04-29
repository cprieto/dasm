The .equ directive evaluates an expression and defines a symbol/name to
stand for that value; it is apparently meant to declare a "constant" of
some sort. The .set directive does the same thing but allows the value
to be changed later, presumably by another .set directive; this is more
of a "compile-time variable" I guess.

  $ cat <<EOF >equates.asm
  >  .processor 6502
  > one .equ 1+0
  >  .echo one
  > two .set 1+1
  >  .echo two
  > one .set 17-1
  >  .echo one
  > two .set 30+2
  >  .echo two
  >  .end
  > EOF

Currently DASM allows the .set to override the earlier .equ without as
much as a warning so really .equ is not very "constant" as it were. I
think we should change that behavior.

  $ $TESTDIR/dasm equates.asm
   $1
   $2
   some error/warning message?
   $10
   $20

Note that redefining "one" with an .equ directive does lead to errors;
but it results in repeated errors as DASM tries to run pass after pass
to maybe resolve the problem. Not sure what to do about that:

  $ cat <<EOF >equates.asm
  >  .processor 6502
  > one .equ 1+0
  >  .echo one
  > one .equ 17-1
  >  .echo one
  > EOF

  $ $TESTDIR/dasm equates.asm
   no useful expectation here

The .eqm directive works differently because the expression is *not*
evaluated right away; it is assigned to the symbol/name and evaluated
on use, kind of like "call by name" in Algol-60 or "lazy evaluation"
in Haskell (without the fancy memoization of course).

  $ cat <<EOF >equates.asm
  >  .processor 6502
  > one .set 2-1
  > add1 .eqm one+1
  >  .echo one
  >  .echo add1
  > one .set 16
  >  .echo add1
  > EOF

  $ $TESTDIR/dasm equates.asm
   $1
   $2
   $11

Actually kinda neat I think. :-) What happens for these directives when no
expression is given is not so neat however:

  $ cat <<EOF >equates.asm
  >  .processor 6502
  > one .equ
  >  .echo one
  > two .set
  >  .echo two
  > three .eqm
  >  .echo three
  > EOF

  $ $TESTDIR/dasm equates.asm
   $0
   $0
   $0

That is both bad and sad. But I am not sure we can change it so I am leaving
it as a "passed" test case for now. It really should be a failure, but this
might be related to my troubles with the "empty .echo" directive, seems that
DASM evaluates empty strings to 0 everywhere. Ouch!
