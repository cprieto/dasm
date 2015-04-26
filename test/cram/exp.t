We use the ECHO directive here, but we really just test expression evaluation
and ignore any ECHO-related problems. We start with just numbers:

  $ cat <<EOF >exp.asm
  >  .processor 6502
  >  .echo 256
  >  .echo 0400
  >  .echo %100000000
  >  .echo \$100
  >  .end
  > EOF

These all seem to work just fine, and it would be a huge surprise if they
did not.

  $ $TESTDIR/dasm exp.asm
   $100
   $100
   $100
   $100

Now we get to binary operators for integer arithmetic.

  $ cat <<EOF >exp.asm
  >  .processor 6502
  >  .echo 1+2
  >  .echo 2*3
  >  .echo 1+2*3
  >  .echo (1+2)*3
  >  .echo [1+2]*3
  >  .echo 2-1
  >  .echo 1-2
  >  .echo 3-2-1
  >  .echo (3-2)-1
  >  .echo 3-(2-1)
  >  .echo 3 -( 2- 1  )
  >  .echo 200/10/4
  >  .echo (200/10)/4
  >  .echo 200/(10/4)
  >  .echo 111%43%4
  >  .echo (111%43)%4
  >  .echo 111%(43%4)
  >  .echo 10%3
  >  .echo -10%3
  >  .echo 10%-3
  >  .echo -10%-3
  >  .end
  > EOF

Again, those seem to work just fine, including evaluation order for chained
subtractions, divisions, and modulos. The modulo operator has C semantics, so
the sign we use in the test cases is based on C99/C11 where the dividend
determines.

  $ $TESTDIR/dasm exp.asm
   $3
   $6
   $7
   $9
   $9
   $1
   $ffffffffffffffff
   $0
   $0
   $2
   $2
   $5
   $5
   $64
   $1
   $1
   $0
   $1
   $ffffffffffffffff
   $1
   $ffffffffffffffff

TODO logical operations, more precedence tests, others?
