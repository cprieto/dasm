; $Id$
;
; Test negative literals.
;
; This came about because of the "-1 bug" in 2.20.10 and
; will hopefully ensure that no such bug ever returns to
; haunt us again...
;
; Explaining how negative numbers get handled is quite a
; mess, so bear with me. Consider the case of not having
; negative numbers first: Obviously, arguments between 0
; and 255 should be allowed for the whole byte range. If
; we want to *still* allow that range in the presence of
; negative numbers, eight bits are obviously not enough:
; Just eights bits would give us -128 to 127 instead. So
; we have to go to at least nine bits. That, however, is
; a bit messy: DASM is an eight bit assembler after all.
; Luckily, DASM's arithmetic has always been documented
; as being 32 bit wide, so there is actually no problem
; with simply doing everything in 32 bit. However, we'd
; like to avoid the < operator to select the LSB in most
; cases, so we have to somehow coerce back to eight bit
; automatically when necessary. Luckily, DASM does this
; already in most places where it is necessary. So that
; leaves only the matter of checking the range of values
; to resolve. After all, DASM should produce some kind
; of notice if we're using values that are out of range,
; even in if an automatic coercion will ensure that we
; don't produce a wrong binary. The simplest range to
; check at this point is -256 to 255; however, the range
; we really want is smaller, -128 to 255; it's not a
; nice range for sure, but it covers exactly the values
; people would expect from eight bit, whether signed or
; unsigned. So that's what we're going to do. :-)
;
; Peter H. Froehlich
; phf at acm dot org

	.processor	6502
	.org		0
	.trace		off

; Let's start by printing some values that illustrate DASM
; arithmetic by itself.

	.echo	0	; prints $0
	.echo	+0	; TODO: syntax error? needs a fix...
	.echo	-0	; prints $0
	.echo	~0	; prints $ffffffff
	.echo	<~0	; prints $ff
	.echo	1	; prints $1
	.echo	+1	; TODO: syntax error? needs a fix...
	.echo	-1	; prints $ffffffff
	.echo	0-1	; prints $ffffffff
	.echo	<-1	; prints $ff
	.echo	<0-1	; prints $ffffffff
	.echo	<(0-1)	; prints $ff
	.echo	<[0-1]	; prints $ff

; Now let's try things in the context of an LDA instruction
; which wants an eight bit value.

	lda	#0	; generates a9 00 as expected
	lda	#+0	; TODO: syntax error? needs a fix...
	lda	#-0	; generates a9 00 as expected

	lda	#1	; generates a9 01 as expected
	lda	#+1	; TODO: syntax error? needs a fix...
	lda	#<-1	; generates a9 ff as expected
	lda	#-1	; generates a9 ff as expected *without* < operator

	lda	#127	; generates a9 7f as expected
	lda	#128	; generates a9 80 as expected
	lda	#129	; generates a9 81 as expected

	lda	#-127	; generates a9 81 as expected
	lda	#-128	; generates a9 80 as expected
	lda	#-129	; FAILS with range check

	lda	#254	; generates a9 fe as expected
	lda	#255	; generates a9 ff as expected
	lda	#256	; FAILS with range check

	lda	#-254	; FAILS with range check
	lda	#-255	; FAILS with range check
	lda	#-256	; FAILS with range check
	lda	#-257	; FAILS with range check

	lda	#1024	; FAILS with range check
	lda	#-1024	; FAILS with range check

; Added for Andrew's bug report 2015/04/20; these all behave
; as they should as far as I can tell. Regarding the range
; check, we can either have one or not. I prefer to have one
; but that MUST mean that there's a message for the line
; marked "FAILS" below. The value is, strictly speaking,
; out of the -128..255 range I postulated when I first added
; range checking. There's no logical reason to go beyond that
; range for a single byte. Maybe we should "demote" the message
; to a warning. And actually I think we need one more warning,
; see the line marked "SILENTLY" below.

	.echo	$80	; prints $80
	.echo	-$80	; prints $ffffff80
	.echo	<-$80	; prints $80
	.echo	~$80	; prints $ffffff7f
	.echo	<~$80	; prints $7f

	lda	$80	; generates A5 80
	lda	-$80	; generates A5 80
	lda	<-$80	; generates A5 80
	lda	~$80	; SILENTLY generates AD 7F FF
	lda	<~$80	; generates A5 7F

	lda	#$80	; generates A9 80
	lda	#-$80	; generates A9 80
	lda	#<-$80	; generates A9 80
	lda	#~$80	; FAILS with range check! but generates A9 7F
	lda	#<~$80	; generates A9 7F

	.end
