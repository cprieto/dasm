The ERR directive should stop assembly right away.

  $ cat <<EOF >err.asm
  >  .processor 6502
  >  .err
  >  .end
  > EOF

Currently DASM uses a PANIC to immediately stop everything and exit.

  $ $TESTDIR/dasm err.asm
  err.asm (2): ***panic***: ERR pseudo-op encountered, aborting assembly!
  [1]

We could instead use a FATAL and finish the current pass, but that seems
counter to what ERR is described as in the manual. And it used to always
panic I believe?

The ERR directive as described does not have an argument.

  $ cat <<EOF >err.asm
  >  .processor 6502
  >  .err "This is an error!"
  >  .end
  > EOF

  $ $TESTDIR/dasm err.asm
  err.asm (2): ***panic***: ERR pseudo-op encountered, aborting assembly!
  [1]

Currently DASM does not diagnose this with a separate error. Also I feel like
maybe we should allow a string argument after the ERR directive. To display an
actual error message maybe? Opinions?
